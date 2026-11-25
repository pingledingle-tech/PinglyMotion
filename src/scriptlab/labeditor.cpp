/*
 * PinglyMotion Script Lab - the script editor in the Live page (lane 1, job F1)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFocusEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSaveFile>
#include <QThread>
#include <QScrollBar>
#include <QTextBlock>
#include <QToolTip>
#include <QVBoxLayout>

#include "logging.h"
#include "lsl/lsl2slua.h"
#include "lslhighlighter.h"
#include "rlvsuggest.h"
#include "lucideicons.h"
#include "scriptlabsection.h"
#include "theme.h"
#include "usersettings.h"

#include "labeditor.h"

namespace
{
  using namespace Theme::Size;

  QTime (*g_clock)() = nullptr;
  QTime now() { return g_clock ? g_clock() : QTime::currentTime(); }

  bool paletteIsDark(const QPalette& palette) { return palette.color(QPalette::Window).lightness() < 128; }

  QColor token(Theme::Token t, bool dark) { return Theme::color(t, dark); }

  // which characters of an LSL text are code (not inside a string or a comment)
  QVector<bool> codeMask(const QString& text)
  {
    const int n = int(text.size());
    QVector<bool> code(n, true);
    int i = 0;
    while (i < n)
    {
      const QChar c = text.at(i);
      if (c == QLatin1Char('/') && i + 1 < n && text.at(i + 1) == QLatin1Char('/'))
      {
        while (i < n && text.at(i) != QLatin1Char('\n')) code[i++] = false;
        continue;
      }
      if (c == QLatin1Char('/') && i + 1 < n && text.at(i + 1) == QLatin1Char('*'))
      {
        code[i] = code[i + 1] = false;
        i += 2;
        while (i < n && !(text.at(i) == QLatin1Char('*') && i + 1 < n && text.at(i + 1) == QLatin1Char('/'))) code[i++] = false;
        if (i < n) { code[i] = false; if (i + 1 < n) code[i + 1] = false; i += 2; }
        continue;
      }
      if (c == QLatin1Char('"'))
      {
        code[i++] = false;
        while (i < n && text.at(i) != QLatin1Char('"') && text.at(i) != QLatin1Char('\n'))
        {
          if (text.at(i) == QLatin1Char('\\') && i + 1 < n) code[i++] = false;
          code[i++] = false;
        }
        if (i < n && text.at(i) == QLatin1Char('"')) code[i++] = false;
        continue;
      }
      ++i;
    }
    return code;
  }

  QString leadingSpace(const QString& line)
  {
    int i = 0;
    while (i < line.size() && (line.at(i) == QLatin1Char(' ') || line.at(i) == QLatin1Char('\t'))) ++i;
    return line.left(i);
  }

  QPushButton* makeButton(const QString& text, const QString& name, QWidget* parent)
  {
    QPushButton* b = new QPushButton(text, parent);
    b->setObjectName(name);
    b->setMinimumHeight(controlCompact);
    return b;
  }
}

// ---------------------------------------------------------------------------------------------
// LabCodeEdit

class LabCodeEdit::Numbers : public QWidget
{
  public:
    explicit Numbers(LabCodeEdit* edit) : QWidget(edit), m_edit(edit) { setObjectName("labEditorLineNumbers"); }
    QSize sizeHint() const override { return QSize(m_edit->numberWidth(), 0); }
  protected:
    void paintEvent(QPaintEvent* event) override { m_edit->paintNumbers(event); }
  private:
    LabCodeEdit* m_edit;
};

LabCodeEdit::LabCodeEdit(QWidget* parent) :
  QPlainTextEdit(parent),
  m_numbers(new Numbers(this)),
  m_highlighter(nullptr)
{
  setObjectName("labEditorText");
  setLineWrapMode(QPlainTextEdit::NoWrap);
  setTabChangesFocus(false);
  m_dark = paletteIsDark(palette());
  m_highlighter = new LslHighlighter(document(), m_dark);
  applyFixedFont();
  connect(this, &QPlainTextEdit::blockCountChanged, this, [this](int) { updateMargin(); });
  connect(this, &QPlainTextEdit::updateRequest, this, [this](const QRect& rect, int dy)
  {
    if (dy) m_numbers->scroll(0, dy);
    else m_numbers->update(0, rect.y(), m_numbers->width(), rect.height());
    if (dy && suggestionsVisible()) placePopup();
  });
  connect(this, &QPlainTextEdit::cursorPositionChanged, this, &LabCodeEdit::updateBracketPair);
  connect(this, &QPlainTextEdit::textChanged, this, &LabCodeEdit::updateBracketPair);
  updateMargin();

  // the suggestion list (job F2): a child of the text box, never a window of its own and never
  // the focus, so the keys stay in the text and the list cannot take Enter by itself
  m_popup = new QFrame(this);
  m_popup->setObjectName("labSuggestPopup");
  m_popup->setFocusPolicy(Qt::NoFocus);
  m_popup->setAutoFillBackground(true);
  QVBoxLayout* popup = new QVBoxLayout(m_popup);
  popup->setContentsMargins(spacingXs, spacingXs, spacingXs, spacingXs);
  popup->setSpacing(spacingXs);
  m_list = new QListWidget(m_popup);
  m_list->setObjectName("labSuggestList");
  m_list->setFocusPolicy(Qt::NoFocus);
  m_list->setSelectionMode(QAbstractItemView::SingleSelection);
  m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_list->setUniformItemSizes(true);
  m_list->setAccessibleName(tr("Suggestions"));
  popup->addWidget(m_list);
  m_help = new QLabel(m_popup);
  m_help->setObjectName("labSuggestHelp");
  m_help->setWordWrap(true);
  popup->addWidget(m_help);
  m_detail = new QLabel(m_popup);
  m_detail->setObjectName("labSuggestDetail");
  m_detail->setWordWrap(true);
  popup->addWidget(m_detail);
  // an RLV command's example (job R3): shown in both modes, Ctrl+Enter puts it in (charter P12)
  m_example = new QLabel(m_popup);
  m_example->setObjectName("labSuggestExample");
  m_example->setWordWrap(true);
  m_example->setTextFormat(Qt::PlainText);
  popup->addWidget(m_example);
  m_hint = new QLabel(tr("Press Down to pick, then Enter or Tab. Esc closes the list."), m_popup);
  m_hint->setObjectName("labSuggestHint");
  m_hint->setWordWrap(true);
  popup->addWidget(m_hint);
  m_popup->hide();
  connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item)
  {
    setSuggestionRow(m_list->row(item));
    takeSuggestion();
  });
  stylePopup();

  // RLV commands after "@" inside the owner-say text (job R3)
  m_rlv = std::make_shared<LabSuggest::RlvProvider>();
  m_engine.addProvider(m_rlv);
}

void LabCodeEdit::setRlvMode(Rlv::Mode mode)
{
  m_rlv->setMode(mode);
  if (suggestionsVisible()) refreshSuggestions();
}

Rlv::Mode LabCodeEdit::rlvMode() const
{
  return m_rlv->mode();
}

QString LabCodeEdit::suggestionRowText(int row) const
{
  return row >= 0 && row < m_list->count() ? m_list->item(row)->text() : QString();
}

void LabCodeEdit::applyFixedFont()
{
  // the system's fixed-width face; only the face is set here, so the size still comes from the
  // program's font and follows View > Text size and Setup (charter P9)
  QFont font;
  font.setFamilies(QStringList{QFontDatabase::systemFont(QFontDatabase::FixedFont).family()});
  font.setStyleHint(QFont::Monospace);
  font.setFixedPitch(true);
  m_settingFont = true;
  setFont(font);
  m_settingFont = false;
  fontChanged();
}

void LabCodeEdit::fontChanged()
{
  setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * IndentWidth);
  updateMargin();
}

void LabCodeEdit::setDark(bool dark)
{
  m_dark = dark;
  m_highlighter->setDark(dark);
  stylePopup();
  setExtraSelections(makeMarks());
  m_numbers->update();
  viewport()->update();
}

void LabCodeEdit::changeEvent(QEvent* event)
{
  QPlainTextEdit::changeEvent(event);
  if (event->type() == QEvent::FontChange && !m_settingFont) fontChanged();
  else if (event->type() == QEvent::PaletteChange && paletteIsDark(palette()) != m_dark) setDark(paletteIsDark(palette()));
}

int LabCodeEdit::numberWidth() const
{
  const int digits = int(QString::number(qMax(1, blockCount())).size());
  // an icon place for the error or warning mark, then the number
  return spacingXs + iconSmall + spacingXs + fontMetrics().horizontalAdvance(QLatin1Char('9')) * qMax(2, digits) + spacingS;
}

void LabCodeEdit::updateMargin()
{
  setViewportMargins(numberWidth(), 0, 0, 0);
  const QRect inner = contentsRect();
  m_numbers->setGeometry(QRect(inner.left(), inner.top(), numberWidth(), inner.height()));
}

void LabCodeEdit::resizeEvent(QResizeEvent* event)
{
  QPlainTextEdit::resizeEvent(event);
  updateMargin();
  if (suggestionsVisible()) placePopup();
}

QList<int> LabCodeEdit::errorLines() const
{
  QList<int> out;
  for (const LabProblem& p : m_problems)
    if (p.error && !out.contains(p.line)) out << p.line;
  std::sort(out.begin(), out.end());
  return out;
}

QList<int> LabCodeEdit::warningLines() const
{
  const QList<int> errors = errorLines();
  QList<int> out;
  for (const LabProblem& p : m_problems)
    if (!p.error && !errors.contains(p.line) && !out.contains(p.line)) out << p.line;
  std::sort(out.begin(), out.end());
  return out;
}

void LabCodeEdit::paintNumbers(QPaintEvent* event)
{
  QPainter painter(m_numbers);
  painter.fillRect(event->rect(), palette().color(QPalette::Window));
  const QList<int> errors = errorLines();
  const QList<int> warnings = warningLines();
  const QColor danger = token(Theme::Token::Danger, m_dark);
  const QColor warning = token(Theme::Token::Warning, m_dark);
  const QColor muted = token(Theme::Token::TextMuted, m_dark);
  const QColor disabled = token(Theme::Token::TextDisabled, m_dark);
  const QPixmap errorIcon = LucideIcons::icon(QStringLiteral("circle-x"), danger, disabled).pixmap(iconSmall, iconSmall);
  const QPixmap warningIcon = LucideIcons::icon(QStringLiteral("triangle-alert"), warning, disabled).pixmap(iconSmall, iconSmall);
  QTextBlock block = firstVisibleBlock();
  int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  const int lineHeight = fontMetrics().height();
  while (block.isValid() && top <= event->rect().bottom())
  {
    const int bottom = top + qRound(blockBoundingRect(block).height());
    if (block.isVisible() && bottom >= event->rect().top())
    {
      const int line = block.blockNumber() + 1;
      const bool error = errors.contains(line);
      const bool warn = warnings.contains(line);
      if (error || warn)
        painter.drawPixmap(spacingXs, top + qMax(0, (lineHeight - iconSmall) / 2), error ? errorIcon : warningIcon);
      QFont font = this->font();
      font.setWeight(error ? QFont::Bold : QFont::Normal);
      painter.setFont(font);
      painter.setPen(error ? danger : warn ? warning : muted);
      painter.drawText(0, top, m_numbers->width() - spacingS, lineHeight, Qt::AlignRight, QString::number(line));
    }
    block = block.next();
    top = bottom;
  }
}

void LabCodeEdit::setProblems(const QList<LabProblem>& problems)
{
  m_problems = problems;
  setExtraSelections(makeMarks());
  m_numbers->update();
}

QList<QTextEdit::ExtraSelection> LabCodeEdit::makeMarks() const
{
  QList<QTextEdit::ExtraSelection> marks;
  for (const LabProblem& p : m_problems)
  {
    const QTextBlock block = document()->findBlockByNumber(p.line - 1);
    if (!block.isValid()) continue;
    QTextEdit::ExtraSelection mark;
    mark.cursor = QTextCursor(block);
    mark.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    mark.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    mark.format.setUnderlineColor(token(p.error ? Theme::Token::Danger : Theme::Token::Warning, m_dark));
    mark.format.setToolTip(p.message);
    marks << mark;
  }
  return marks;
}

int LabCodeEdit::matchingBracket(const QString& text, int pos)
{
  if (pos < 0 || pos >= text.size()) return -1;
  static const QString open = QStringLiteral("([{");
  static const QString close = QStringLiteral(")]}");
  const QChar c = text.at(pos);
  const int o = int(open.indexOf(c));
  const int cl = int(close.indexOf(c));
  if (o < 0 && cl < 0) return -1;
  const QVector<bool> code = codeMask(text);
  if (!code.at(pos)) return -1;
  const QChar mine = c;
  const QChar other = o >= 0 ? close.at(o) : open.at(cl);
  const int step = o >= 0 ? 1 : -1;
  int depth = 0;
  for (int i = pos; i >= 0 && i < text.size(); i += step)
  {
    if (!code.at(i)) continue;
    const QChar ch = text.at(i);
    if (ch == mine) ++depth;
    else if (ch == other && --depth == 0) return i;
  }
  return -1;
}

void LabCodeEdit::updateBracketPair()
{
  const QPair<int, int> before = m_bracketPair;
  m_bracketPair = qMakePair(-1, -1);
  const QString text = document()->toPlainText();
  const int pos = textCursor().position();
  // the bracket right of the cursor first, then the one left of it
  for (int at : {pos, pos - 1})
  {
    const int match = matchingBracket(text, at);
    if (match >= 0)
    {
      m_bracketPair = qMakePair(qMin(at, match), qMax(at, match));
      break;
    }
  }
  if (before != m_bracketPair) viewport()->update();
}

void LabCodeEdit::paintEvent(QPaintEvent* event)
{
  QPlainTextEdit::paintEvent(event);
  if (m_bracketPair.first < 0) return;
  // the pair: a box in the focus ring colour round each bracket (a shape, not only a colour)
  QPainter painter(viewport());
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(token(Theme::Token::FocusRing, m_dark));
  pen.setWidthF(1.5);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);
  for (int pos : {m_bracketPair.first, m_bracketPair.second})
  {
    QTextCursor c(document());
    c.setPosition(pos);
    const QRect r = cursorRect(c);
    const int w = fontMetrics().horizontalAdvance(document()->characterAt(pos));
    painter.drawRoundedRect(QRectF(r.left() + 0.5, r.top() + 0.5, w, r.height() - 1), radiusSmall / 2.0, radiusSmall / 2.0);
  }
}

bool LabCodeEdit::event(QEvent* event)
{
  if (event->type() == QEvent::ShortcutOverride)
  {
    // the editor's own keys win over the window's while the text box has the focus
    // (File > Save is Ctrl+S too; Tab hides panels only in the 3D view, but keep it here anyway)
    QKeyEvent* key = static_cast<QKeyEvent*>(event);
    const Qt::KeyboardModifiers mods = key->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);
    const int k = key->key();
    const bool ctrl = mods == Qt::ControlModifier;
    if ((ctrl && (k == Qt::Key_S || k == Qt::Key_F || k == Qt::Key_G || k == Qt::Key_Space)) || k == Qt::Key_Tab || k == Qt::Key_Backtab
        || (mods == Qt::NoModifier && (k == Qt::Key_Escape || k == Qt::Key_F3)))
    {
      event->accept();
      return true;
    }
  }
  if (event->type() == QEvent::ToolTip)
  {
    // the message of an error or warning line, where the mouse rests on it
    QHelpEvent* help = static_cast<QHelpEvent*>(event);
    const QPoint inViewport = viewport()->mapFrom(this, help->pos());
    const int line = cursorForPosition(inViewport).blockNumber() + 1;
    QStringList messages;
    for (const LabProblem& p : m_problems)
      if (p.line == line) messages << p.message;
    if (!messages.isEmpty())
    {
      QToolTip::showText(help->globalPos(), messages.join(QLatin1Char('\n')), this);
      return true;
    }
  }
  return QPlainTextEdit::event(event);
}

void LabCodeEdit::focusInEvent(QFocusEvent* event)
{
  QPlainTextEdit::focusInEvent(event);
  if (!m_hadFocus)
  {
    m_hadFocus = true;
    emit firstFocus();
  }
}

void LabCodeEdit::indentSelection(bool back)
{
  QTextCursor cursor = textCursor();
  const int start = cursor.selectionStart();
  const int end = cursor.selectionEnd();
  QTextBlock first = document()->findBlock(start);
  QTextBlock last = document()->findBlock(end);
  // a selection that ends at the start of a line does not take that line
  if (last != first && end == last.position()) last = last.previous();
  cursor.beginEditBlock();
  for (QTextBlock b = first; b.isValid(); b = b.next())
  {
    QTextCursor c(b);
    if (back)
    {
      const QString lead = leadingSpace(b.text());
      int remove = 0;
      while (remove < lead.size() && remove < IndentWidth && lead.at(remove) == QLatin1Char(' ')) ++remove;
      if (remove == 0 && !lead.isEmpty() && lead.at(0) == QLatin1Char('\t')) remove = 1;
      c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, remove);
      c.removeSelectedText();
    }
    else
      c.insertText(QString(IndentWidth, QLatin1Char(' ')));
    if (b == last) break;
  }
  cursor.endEditBlock();
}

void LabCodeEdit::keyPressEvent(QKeyEvent* event)
{
  const int k = event->key();
  const Qt::KeyboardModifiers mods = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);
  const QString typed = event->text();
  // the suggestion list takes its keys first (arrows; Enter and Tab only once moved into it)
  if (suggestionKey(event)) return;
  editKey(event);
  afterKey(k, mods, typed);
}

void LabCodeEdit::editKey(QKeyEvent* event)
{
  const int k = event->key();
  const Qt::KeyboardModifiers mods = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);

  if (k == Qt::Key_Escape && mods == Qt::NoModifier)
  {
    m_escArmed = true;
    emit escapePressed();
    event->accept();
    return;
  }
  const bool armed = m_escArmed;
  if (k != Qt::Key_Shift) m_escArmed = false;

  if (mods == Qt::ControlModifier && k == Qt::Key_S) { emit saveRequested(); return; }
  if (mods == Qt::ControlModifier && k == Qt::Key_F) { emit findRequested(); return; }
  if (mods == Qt::ControlModifier && k == Qt::Key_G) { emit goToLineRequested(); return; }

  if (k == Qt::Key_Tab && mods == Qt::NoModifier)
  {
    if (armed) { QWidget::focusNextPrevChild(true); return; }   // QPlainTextEdit's own refuses while Tab types
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && document()->findBlock(cursor.selectionStart()) != document()->findBlock(cursor.selectionEnd()))
    {
      indentSelection(false);
      return;
    }
    // spaces to the next indent stop
    const int column = cursor.positionInBlock();
    cursor.insertText(QString(IndentWidth - column % IndentWidth, QLatin1Char(' ')));
    return;
  }
  if (k == Qt::Key_Backtab || (k == Qt::Key_Tab && mods == Qt::ShiftModifier))
  {
    if (armed) { QWidget::focusNextPrevChild(false); return; }
    indentSelection(true);
    return;
  }

  if ((k == Qt::Key_Return || k == Qt::Key_Enter) && (mods == Qt::NoModifier || mods == Qt::ShiftModifier))
  {
    // a new line keeps the indent; one step more after "{"; "{|}" opens a block with the cursor inside
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    const QString line = cursor.block().text();
    const int column = cursor.positionInBlock();
    const QString before = line.left(column);
    const QString after = line.mid(column);
    const QString lead = leadingSpace(line).left(column);
    const QString step(IndentWidth, QLatin1Char(' '));
    const bool opens = before.trimmed().endsWith(QLatin1Char('{'));
    const bool closesNext = after.trimmed().startsWith(QLatin1Char('}'));
    // drop the spaces right of the cursor, so the new line starts at the indent
    int spaces = 0;
    while (spaces < after.size() && after.at(spaces) == QLatin1Char(' ')) ++spaces;
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, spaces);
    cursor.removeSelectedText();
    if (opens && closesNext)
    {
      cursor.insertText(QLatin1Char('\n') + lead + step);
      const int inside = cursor.position();
      cursor.insertText(QLatin1Char('\n') + lead);
      cursor.setPosition(inside);
    }
    else
      cursor.insertText(QLatin1Char('\n') + lead + (opens ? step : QString()));
    cursor.endEditBlock();
    setTextCursor(cursor);
    ensureCursorVisible();
    return;
  }

  const QString typed = event->text();
  if (typed == QStringLiteral("}") && !textCursor().hasSelection())
  {
    // "}" on a line with only spaces before it goes back one step
    QTextCursor cursor = textCursor();
    const QString before = cursor.block().text().left(cursor.positionInBlock());
    if (!before.isEmpty() && before.trimmed().isEmpty())
    {
      cursor.beginEditBlock();
      int remove = 0;
      while (remove < IndentWidth && remove < before.size() && before.at(before.size() - 1 - remove) == QLatin1Char(' ')) ++remove;
      cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, remove);
      cursor.insertText(QStringLiteral("}"));
      cursor.endEditBlock();
      setTextCursor(cursor);
      return;
    }
  }
  if (typed == QStringLiteral("\""))
  {
    QTextCursor cursor = textCursor();
    const QString text = document()->toPlainText();
    const int pos = cursor.position();
    const QChar next = pos < text.size() ? text.at(pos) : QChar();
    const QChar prev = pos > 0 ? text.at(pos - 1) : QChar();
    if (cursor.hasSelection())
    {
      // a selection gets quotes round it
      const QString inside = cursor.selectedText();
      cursor.beginEditBlock();
      cursor.insertText(QLatin1Char('"') + inside + QLatin1Char('"'));
      cursor.endEditBlock();
      setTextCursor(cursor);
      return;
    }
    if (next == QLatin1Char('"') && prev != QLatin1Char('\\'))
    {
      // the closing quote is already there: step over it
      cursor.movePosition(QTextCursor::Right);
      setTextCursor(cursor);
      return;
    }
    const bool inCode = pos >= text.size() || codeMask(text).value(pos, true);
    const bool wordNext = !next.isNull() && (next.isLetterOrNumber() || next == QLatin1Char('_'));
    if (prev != QLatin1Char('\\') && inCode && !wordNext)
    {
      cursor.beginEditBlock();
      cursor.insertText(QStringLiteral("\"\""));
      cursor.endEditBlock();
      cursor.movePosition(QTextCursor::Left);
      setTextCursor(cursor);
      return;
    }
  }
  QPlainTextEdit::keyPressEvent(event);
}

// ---------------------------------------------------------------------------------------------
// Suggestions (lane 1, job F2)

bool LabCodeEdit::suggestionsVisible() const
{
  return m_popup && !m_popup->isHidden();
}

QStringList LabCodeEdit::suggestionNames() const
{
  QStringList names;
  for (const LabSuggest::Item& it : m_items) names << it.name;
  return names;
}

void LabCodeEdit::setSuggestOnRequestOnly(bool on)
{
  m_onRequestOnly = on;
  if (on && suggestionsVisible() && !m_requested) hideSuggestions();
}

void LabCodeEdit::setExpertHelp(bool on)
{
  m_expertHelp = on;
  setSuggestionRow(m_row);
  if (suggestionsVisible()) placePopup();
}

void LabCodeEdit::setSuggestHints(bool on)
{
  m_suggestHints = on;
  m_hint->setVisible(on);
  if (suggestionsVisible()) placePopup();
}

bool LabCodeEdit::showSuggestions()
{
  m_held = false;
  m_requested = true;
  const QList<LabSuggest::Item> items = m_engine.suggest(toPlainText(), textCursor().position(), true);
  if (items.isEmpty())
  {
    hideSuggestions();
    return false;
  }
  fillSuggestions(items);
  return true;
}

void LabCodeEdit::hideSuggestions()
{
  m_popup->hide();
  m_items.clear();
  m_list->clear();
  m_row = -1;
  m_inList = false;
  m_requested = false;
}

void LabCodeEdit::refreshSuggestions()
{
  if (m_held) { hideSuggestions(); return; }
  if (!suggestionsVisible() && m_onRequestOnly) return;
  const bool requested = suggestionsVisible() && m_requested;
  const QList<LabSuggest::Item> items = m_engine.suggest(toPlainText(), textCursor().position(), requested);
  if (items.isEmpty()) { hideSuggestions(); return; }
  m_requested = requested;
  fillSuggestions(items);
}

void LabCodeEdit::refreshAfterSymbol()
{
  // a character that cannot be in a name closes the list, unless a provider has something for
  // the place it leads to without being asked: "@", ":", "=" and "," in an RLV line (job R3)
  if (m_held || (!suggestionsVisible() && m_onRequestOnly)) { hideSuggestions(); return; }
  const QList<LabSuggest::Item> items = m_engine.suggest(toPlainText(), textCursor().position(), false);
  if (items.isEmpty()) { hideSuggestions(); return; }
  m_requested = false;
  fillSuggestions(items);
}

void LabCodeEdit::fillSuggestions(const QList<LabSuggest::Item>& items)
{
  m_items = items;
  m_inList = false;   // a new list: Enter and Tab are a line break and an indent again
  m_list->clear();
  for (const LabSuggest::Item& it : items)
  {
    // a mark such as "RLVa only" is a word after the name, never only a colour (charter P10)
    QListWidgetItem* row = new QListWidgetItem(it.mark.isEmpty() ? it.name : tr("%1 (%2)").arg(it.name, it.mark), m_list);
    row->setToolTip(it.mark.isEmpty() ? LabSuggest::kindWord(it.kind) : LabSuggest::kindWord(it.kind) + QStringLiteral(" · ") + it.mark);
  }
  m_list->clearSelection();
  m_popup->show();
  m_popup->raise();
  setSuggestionRow(0);
  placePopup();
}

void LabCodeEdit::setSuggestionRow(int row)
{
  if (m_items.isEmpty()) { m_row = -1; m_help->clear(); m_detail->clear(); m_example->clear(); m_example->hide(); return; }
  m_row = qBound(0, row, int(m_items.size()) - 1);
  const LabSuggest::Item& it = m_items.at(m_row);
  m_help->setText(it.plain);
  m_detail->setText(it.detail);
  m_detail->setVisible(m_expertHelp && !it.detail.isEmpty());
  m_example->setText(it.example.isEmpty() ? QString() : tr("Ctrl+Enter puts in the example: %1").arg(it.example));
  m_example->setVisible(!it.example.isEmpty());
  m_hint->setVisible(m_suggestHints);
  // the chosen row: the selection fill and bold text (a shape, not only a colour, charter P10);
  // before an arrow key nothing is chosen, the help shows the first row
  for (int i = 0; i < m_list->count(); ++i)
  {
    QFont f = m_list->font();
    f.setWeight(m_inList && i == m_row ? QFont::DemiBold : QFont::Normal);
    m_list->item(i)->setFont(f);
  }
  if (m_inList)
  {
    m_list->setCurrentRow(m_row);
    m_list->item(m_row)->setSelected(true);
  }
  else
    m_list->clearSelection();
  m_list->scrollToItem(m_list->item(m_row));
}

void LabCodeEdit::placePopup()
{
  if (!suggestionsVisible()) return;
  const QFontMetrics fm(m_list->font());
  // the bold row is the tallest one: room for it in every row
  QFont bold = m_list->font();
  bold.setWeight(QFont::DemiBold);
  const int rowHeight = qMax(QFontMetrics(bold).height() + spacingXs, m_list->sizeHintForRow(0));
  const int rows = qMin(6, int(m_list->count()));
  m_list->setFixedHeight(rows * rowHeight + 2 * m_list->frameWidth());
  const QRect inner = contentsRect();
  const int width = qMax(120, qMin(inner.width() - 2 * spacingXs, qMax(fm.horizontalAdvance(QLatin1Char('m')) * 34, 240)));
  m_popup->setFixedWidth(width);
  // the height the wrapped help lines need at this width
  m_popup->layout()->invalidate();
  m_popup->layout()->activate();
  const int wanted = m_popup->layout()->hasHeightForWidth() ? m_popup->layout()->totalHeightForWidth(width)
                                                           : m_popup->layout()->totalSizeHint().height();
  const int height = qMin(wanted, inner.height());
  m_popup->setFixedHeight(height);
  const QRect cursor = cursorRect().translated(viewport()->geometry().topLeft());
  int x = qBound(inner.left(), cursor.left(), qMax(inner.left(), inner.right() - width));
  int y = cursor.bottom() + 2;
  if (y + height > inner.bottom() && cursor.top() - 2 - height >= inner.top()) y = cursor.top() - 2 - height;
  y = qMax(inner.top(), qMin(y, inner.bottom() - height));
  m_popup->move(x, y);
}

void LabCodeEdit::stylePopup()
{
  if (!m_popup) return;
  const QColor raised = token(Theme::Token::SurfaceRaised, m_dark);
  const QColor border = token(Theme::Token::Border, m_dark);
  const QColor text = token(Theme::Token::Text, m_dark);
  const QColor muted = token(Theme::Token::TextMuted, m_dark);
  m_popup->setStyleSheet(QStringLiteral(
    "QFrame#labSuggestPopup { background: %1; border: 1px solid %2; border-radius: %3px; }"
    "QListWidget#labSuggestList { background: %1; color: %4; border: none; }"
    "QListWidget#labSuggestList::item:selected { background: %5; color: %6; }"
    "QLabel#labSuggestHelp { color: %4; background: transparent; }"
    "QLabel#labSuggestDetail, QLabel#labSuggestHint, QLabel#labSuggestExample { color: %7; background: transparent; }")
    .arg(raised.name(), border.name()).arg(radiusSmall)
    .arg(text.name(), token(Theme::Token::Selection, m_dark).name(), token(Theme::Token::OnAccent, m_dark).name(), muted.name()));
}

bool LabCodeEdit::suggestionKey(QKeyEvent* event)
{
  const int k = event->key();
  const Qt::KeyboardModifiers mods = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);
  if (mods == Qt::ControlModifier && k == Qt::Key_Space)
  {
    m_escArmed = false;
    if (!suggestionsVisible()) showSuggestions();
    event->accept();
    return true;
  }
  if (!suggestionsVisible()) return false;
  if (mods == Qt::NoModifier && (k == Qt::Key_Down || k == Qt::Key_Up))
  {
    if (!m_inList)
    {
      m_inList = true;
      setSuggestionRow(k == Qt::Key_Down ? 0 : int(m_items.size()) - 1);
    }
    else
    {
      const int n = int(m_items.size());
      setSuggestionRow((m_row + (k == Qt::Key_Down ? 1 : -1) + n) % n);
    }
    event->accept();
    return true;
  }
  if (mods == Qt::NoModifier && (k == Qt::Key_PageDown || k == Qt::Key_PageUp) && m_inList)
  {
    setSuggestionRow(m_row + (k == Qt::Key_PageDown ? 5 : -5));
    event->accept();
    return true;
  }
  // Ctrl+Enter puts in the highlighted command's example (job R3); a key of its own, so plain
  // Enter still never takes anything by itself
  if (mods == Qt::ControlModifier && (k == Qt::Key_Return || k == Qt::Key_Enter)
      && m_row >= 0 && m_row < m_items.size() && !m_items.at(m_row).example.isEmpty())
  {
    takeExample();
    event->accept();
    return true;
  }
  if (mods == Qt::NoModifier && (k == Qt::Key_Return || k == Qt::Key_Enter || k == Qt::Key_Tab))
  {
    if (m_inList)
    {
      takeSuggestion();
      event->accept();
      return true;
    }
    hideSuggestions();   // not moved into the list: a line break or an indent as always
    return false;
  }
  if (k == Qt::Key_Escape)
  {
    hideSuggestions();
    m_held = true;       // it stays closed until typing goes on
    return false;        // and Esc still arms the way out (Esc, then Tab)
  }
  return false;
}

void LabCodeEdit::afterKey(int key, Qt::KeyboardModifiers mods, const QString& typed)
{
  if (key == Qt::Key_Escape) return;
  const bool plainTyping = !typed.isEmpty() && typed.at(0).isPrint() && !(mods & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
  if (plainTyping)
  {
    m_held = false;
    if (m_engine.language().isNameChar(typed.at(typed.size() - 1))) refreshSuggestions();
    else refreshAfterSymbol();
    return;
  }
  if (key == Qt::Key_Backspace || key == Qt::Key_Delete)
  {
    if (suggestionsVisible()) refreshSuggestions();
    return;
  }
  if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Meta) return;
  if (suggestionsVisible()) hideSuggestions();
}

void LabCodeEdit::takeSuggestion()
{
  if (m_row < 0 || m_row >= m_items.size()) return;
  const LabSuggest::Item item = m_items.at(m_row);
  const QString text = toPlainText();
  const LabSuggest::Context context = m_engine.context(text, textCursor().position(), true);
  const LabSuggest::Insertion insertion = LabSuggest::insertionFor(item, text.mid(context.cursor));
  // one undo step: the typed part of the name is replaced by the whole name
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  const int from = item.replaceFrom >= 0 && item.replaceFrom <= context.cursor ? item.replaceFrom : context.prefixStart;
  cursor.setPosition(from);
  cursor.setPosition(context.cursor, QTextCursor::KeepAnchor);
  cursor.insertText(insertion.text);
  cursor.endEditBlock();
  cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, insertion.cursorBack);
  setTextCursor(cursor);
  hideSuggestions();
  emit suggestionTaken(item.name);
}

void LabCodeEdit::takeExample()
{
  if (m_row < 0 || m_row >= m_items.size()) return;
  const LabSuggest::Item item = m_items.at(m_row);
  const int end = textCursor().position();
  if (item.example.isEmpty() || item.exampleFrom < 0 || item.exampleFrom > end) return;
  // one undo step: the command typed so far is replaced by the whole example
  QTextCursor cursor = textCursor();
  cursor.beginEditBlock();
  cursor.setPosition(item.exampleFrom);
  cursor.setPosition(end, QTextCursor::KeepAnchor);
  cursor.insertText(item.exampleInsert);
  cursor.endEditBlock();
  setTextCursor(cursor);
  hideSuggestions();
  emit suggestionTaken(item.name);
}

void LabCodeEdit::focusOutEvent(QFocusEvent* event)
{
  QPlainTextEdit::focusOutEvent(event);
  hideSuggestions();
}

void LabCodeEdit::mousePressEvent(QMouseEvent* event)
{
  hideSuggestions();
  QPlainTextEdit::mousePressEvent(event);
}

// ---------------------------------------------------------------------------------------------
// LabEditor

LabEditor::LabEditor(QWidget* parent) :
  QWidget(parent)
{
  setObjectName("labEditor");
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(spacingS);

  // --- starter help: no script yet (charter P4)
  m_starter = new ScriptLabFrame(this);
  m_starter->setObjectName("labEditorStarter");
  QVBoxLayout* starter = new QVBoxLayout(m_starter);
  starter->setContentsMargins(spacingL, spacingL, spacingL, spacingL);
  starter->setSpacing(spacingS);
  m_starterHeading = new QLabel(tr("No script open"), m_starter);
  m_starterHeading->setObjectName("labEditorStarterHeading");
  QFont heading = m_starterHeading->font();
  heading.setWeight(QFont::DemiBold);
  m_starterHeading->setFont(heading);
  starter->addWidget(m_starterHeading);
  m_starterText = new QLabel(tr("Start from an example, or save a .lsl file in the folder."), m_starter);
  m_starterText->setObjectName("labEditorStarterText");
  m_starterText->setWordWrap(true);
  starter->addWidget(m_starterText);
  m_exampleButton = new QPushButton(tr("New from example"), m_starter);
  m_exampleButton->setObjectName("labEditorNewFromExample");
  m_exampleButton->setMinimumHeight(controlDefault);
  m_exampleMenu = new QMenu(m_exampleButton);
  m_exampleMenu->setObjectName("labEditorExampleMenu");
  m_exampleButton->setMenu(m_exampleMenu);
  connect(m_exampleMenu, &QMenu::aboutToShow, this, &LabEditor::fillExampleMenu);
  QHBoxLayout* starterButtons = new QHBoxLayout;
  starterButtons->addWidget(m_exampleButton);
  starterButtons->addStretch(1);
  starter->addLayout(starterButtons);
  layout->addWidget(m_starter);

  // the short card on what an RLV line is (hidden with hints off)
  m_rlvCard = new ScriptLabFrame(this);
  m_rlvCard->setObjectName("labEditorRlvCard");
  QHBoxLayout* card = new QHBoxLayout(m_rlvCard);
  card->setContentsMargins(spacingM, spacingM, spacingM, spacingM);
  card->setSpacing(spacingS);
  m_rlvIcon = new QLabel(m_rlvCard);
  m_rlvIcon->setObjectName("labEditorRlvCardIcon");
  card->addWidget(m_rlvIcon, 0, Qt::AlignTop);
  QVBoxLayout* cardText = new QVBoxLayout;
  cardText->setSpacing(spacingXs);
  m_rlvTitle = new QLabel(tr("What is an RLV line?"), m_rlvCard);
  m_rlvTitle->setObjectName("labEditorRlvCardTitle");
  m_rlvTitle->setFont(heading);
  cardText->addWidget(m_rlvTitle);
  m_rlvText = new QLabel(tr("A script sends an RLV line to the viewer. It starts with @, like @detach=n. "
                            "The viewer then blocks or allows one thing."), m_rlvCard);
  m_rlvText->setObjectName("labEditorRlvCardText");
  m_rlvText->setWordWrap(true);
  m_rlvText->setTextInteractionFlags(Qt::TextSelectableByMouse);
  cardText->addWidget(m_rlvText);
  card->addLayout(cardText, 1);
  layout->addWidget(m_rlvCard);

  // --- the tool row
  QHBoxLayout* tools = new QHBoxLayout;
  tools->setSpacing(spacingS);
  m_save = makeButton(tr("Save"), QStringLiteral("labEditorSave"), this);
  m_findButton = makeButton(tr("Find"), QStringLiteral("labEditorFind"), this);
  m_findButton->setToolTip(tr("Find text in the script (Ctrl+F)"));
  m_goToButton = makeButton(tr("Go to line"), QStringLiteral("labEditorGoTo"), this);
  m_goToButton->setToolTip(tr("Jump to a line by its number (Ctrl+G)"));
  tools->addWidget(m_save);
  // job U3: "unsaved changes" right under the Save button, in words and the warning colour; a line of
  // its own, so the tool row keeps its width when the language choice shows too
  m_unsaved = new QLabel(this);
  m_unsaved->setObjectName("labEditorUnsaved");
  m_unsaved->setTextFormat(Qt::PlainText);
  m_unsaved->setText(tr("● unsaved changes"));
  // its room is kept while it is hidden, so Find and Go to line never move (charter P12)
  QSizePolicy keep = m_unsaved->sizePolicy();
  keep.setRetainSizeWhenHidden(true);
  m_unsaved->setSizePolicy(keep);
  m_unsaved->hide();
  QFont unsavedFont = m_unsaved->font();   // the "caption" type token
  if (unsavedFont.pointSizeF() > 0) unsavedFont.setPointSizeF(unsavedFont.pointSizeF() * 11.0 / 13.0);
  m_unsaved->setFont(unsavedFont);
  tools->addWidget(m_findButton);
  tools->addWidget(m_goToButton);
  tools->addStretch(1);
  // Convert to SLua: at the right end of the message line under the text box (the tool row is full
  // in a narrow panel), so showing it moves nothing (charter P12)
  m_convertButton = makeButton(tr("Convert to SLua"), QStringLiteral("labEditorConvert"), this);
  m_convertButton->setToolTip(tr("Show this script as SLua text. Your script is not changed and nothing is run."));
  {
    // its room is kept while it is hidden, so the line never changes height when it shows (charter P12)
    QSizePolicy keep = m_convertButton->sizePolicy();
    keep.setRetainSizeWhenHidden(true);
    m_convertButton->setSizePolicy(keep);
  }
  m_convertButton->hide();
  // the language choice (job F3): at the right end, after the stretch, so showing it moves
  // nothing (charter P12); only while the Lua switch in Setup is on
  m_languageLabel = new QLabel(tr("Language"), this);
  m_languageLabel->setObjectName("labEditorLanguageLabel");
  m_languageBox = new QComboBox(this);
  m_languageBox->setObjectName("labEditorLanguage");
  m_languageBox->setMinimumHeight(controlCompact);
  m_languageBox->addItem(QStringLiteral("LSL"), QStringLiteral("lsl"));   // tr-ok: the language's name
  m_languageBox->addItem(tr("SLua (Lua, trial)"), QStringLiteral("slua"));
  m_languageBox->setToolTip(tr("The script language. Lua is a trial: no checks or suggestions yet."));
  m_languageLabel->setBuddy(m_languageBox);
  tools->addWidget(m_languageLabel);
  tools->addWidget(m_languageBox);
  m_languageLabel->hide();
  m_languageBox->hide();
  connect(m_convertButton, &QPushButton::clicked, this, [this]() { convertToSlua(); });
  connect(m_languageBox, QOverload<int>::of(&QComboBox::activated), this, [this](int index)
  {
    const QString id = m_languageBox->itemData(index).toString();
    UserSettings::setScriptLanguage(id);   // remembered; followSettings() applies it
    setLanguageChoice(id);
    if (languageInUse() == QLatin1String("slua")) setMessage(tr("Lua is a trial. No Lua suggestions or checks yet."), 0);
  });
  layout->addLayout(tools);
  layout->addWidget(m_unsaved);

  // --- the find row (Ctrl+F): Enter or F3 next, Shift+Enter previous, Esc closes
  m_findRow = new QWidget(this);
  m_findRow->setObjectName("labEditorFindRow");
  QHBoxLayout* findRow = new QHBoxLayout(m_findRow);
  findRow->setContentsMargins(0, 0, 0, 0);
  findRow->setSpacing(spacingS);
  QLabel* findLabel = new QLabel(tr("Find"), m_findRow);
  m_findField = new QLineEdit(m_findRow);
  m_findField->setObjectName("labEditorFindField");
  m_findField->setMinimumHeight(controlCompact);
  m_findField->setToolTip(tr("Enter finds the next match, Shift+Enter the one before. Esc closes."));
  findLabel->setBuddy(m_findField);
  QPushButton* next = makeButton(tr("Next"), QStringLiteral("labEditorFindNext"), m_findRow);
  next->setToolTip(tr("Next match (Enter or F3)"));
  QPushButton* previous = makeButton(tr("Previous"), QStringLiteral("labEditorFindPrevious"), m_findRow);
  previous->setToolTip(tr("Match before (Shift+Enter)"));
  QPushButton* closeFind = makeButton(tr("Close"), QStringLiteral("labEditorFindClose"), m_findRow);
  closeFind->setToolTip(tr("Close the find row (Esc)"));
  m_findResult = new QLabel(m_findRow);
  m_findResult->setObjectName("labEditorFindResult");
  findRow->addWidget(findLabel);
  findRow->addWidget(m_findField, 1);
  findRow->addWidget(next);
  findRow->addWidget(previous);
  findRow->addWidget(closeFind);
  layout->addWidget(m_findRow);
  layout->addWidget(m_findResult);
  m_findRow->hide();
  m_findResult->hide();
  connect(next, &QPushButton::clicked, this, [this]() { find(m_findField->text(), false); });
  connect(previous, &QPushButton::clicked, this, [this]() { find(m_findField->text(), true); });
  connect(closeFind, &QPushButton::clicked, this, [this]() { m_findRow->hide(); m_findResult->hide(); m_edit->setFocus(); });
  m_findField->installEventFilter(this);

  // --- the go-to-line row (Ctrl+G): Enter jumps, Esc closes
  m_goToRow = new QWidget(this);
  m_goToRow->setObjectName("labEditorGoToRow");
  QHBoxLayout* goToRow = new QHBoxLayout(m_goToRow);
  goToRow->setContentsMargins(0, 0, 0, 0);
  goToRow->setSpacing(spacingS);
  QLabel* goToLabel = new QLabel(tr("Go to line"), m_goToRow);
  m_goToField = new QLineEdit(m_goToRow);
  m_goToField->setObjectName("labEditorGoToField");
  m_goToField->setMinimumHeight(controlCompact);
  m_goToField->setValidator(new QIntValidator(1, 999999, m_goToField));
  m_goToField->setToolTip(tr("Type a line number and press Enter. Esc closes."));
  goToLabel->setBuddy(m_goToField);
  QPushButton* go = makeButton(tr("Go"), QStringLiteral("labEditorGoToGo"), m_goToRow);
  QPushButton* closeGoTo = makeButton(tr("Close"), QStringLiteral("labEditorGoToClose"), m_goToRow);
  closeGoTo->setToolTip(tr("Close the go to line row (Esc)"));
  goToRow->addWidget(goToLabel);
  goToRow->addWidget(m_goToField, 1);
  goToRow->addWidget(go);
  goToRow->addWidget(closeGoTo);
  layout->addWidget(m_goToRow);
  m_goToRow->hide();
  connect(go, &QPushButton::clicked, this, [this]() { if (goToLine(m_goToField->text().toInt())) m_goToRow->hide(); });
  connect(closeGoTo, &QPushButton::clicked, this, [this]() { m_goToRow->hide(); m_edit->setFocus(); });
  m_goToField->installEventFilter(this);

  // --- the changed-outside question: asked once, nothing happens until it is answered
  m_outsideBar = new ScriptLabFrame(this);
  m_outsideBar->setObjectName("labEditorOutsideBar");
  QVBoxLayout* outside = new QVBoxLayout(m_outsideBar);
  outside->setContentsMargins(spacingS, spacingS, spacingS, spacingS);
  outside->setSpacing(spacingS);
  QHBoxLayout* outsideTop = new QHBoxLayout;
  outsideTop->setSpacing(spacingS);
  m_outsideIcon = new QLabel(m_outsideBar);
  m_outsideIcon->setObjectName("labEditorOutsideIcon");
  outsideTop->addWidget(m_outsideIcon, 0, Qt::AlignTop);
  m_outsideText = new QLabel(tr("The file changed outside."), m_outsideBar);
  m_outsideText->setObjectName("labEditorOutsideText");
  m_outsideText->setWordWrap(true);
  outsideTop->addWidget(m_outsideText, 1);
  outside->addLayout(outsideTop);
  QHBoxLayout* outsideButtons = new QHBoxLayout;
  outsideButtons->setSpacing(spacingS);
  m_keep = makeButton(tr("Keep my changes"), QStringLiteral("labEditorKeepMine"), m_outsideBar);
  m_keep->setToolTip(tr("Keep your text. Saving then writes it over the file."));
  m_load = makeButton(tr("Load the new one"), QStringLiteral("labEditorLoadNew"), m_outsideBar);
  m_load->setToolTip(tr("Show the file as it is now. Ctrl+Z brings your changes back."));
  m_difference = makeButton(tr("Show the difference"), QStringLiteral("labEditorShowDifference"), m_outsideBar);
  m_difference->setToolTip(tr("Lines with - are yours, lines with + are in the file."));
  outsideButtons->addWidget(m_keep);
  outsideButtons->addWidget(m_load);
  outsideButtons->addWidget(m_difference);
  outsideButtons->addStretch(1);
  outside->addLayout(outsideButtons);
  layout->addWidget(m_outsideBar);
  m_outsideBar->hide();
  connect(m_keep, &QPushButton::clicked, this, [this]() { answerOutsideChange(KeepMine); });
  connect(m_load, &QPushButton::clicked, this, [this]() { answerOutsideChange(LoadNew); });
  connect(m_difference, &QPushButton::clicked, this, [this]() { answerOutsideChange(ShowDifference); });

  m_differenceView = new QPlainTextEdit(this);
  m_differenceView->setObjectName("labEditorDifference");
  m_differenceView->setReadOnly(true);
  m_differenceView->setLineWrapMode(QPlainTextEdit::NoWrap);
  m_differenceView->setToolTip(tr("Lines with - are yours, lines with + are in the file."));
  layout->addWidget(m_differenceView);
  m_differenceView->hide();

  // --- the SLua result (read-only, over the text box)
  m_convertPane = new QWidget(this);
  m_convertPane->setObjectName("labEditorConvertPane");
  QVBoxLayout* convertLayout = new QVBoxLayout(m_convertPane);
  convertLayout->setContentsMargins(0, 0, 0, 0);
  convertLayout->setSpacing(spacingS);
  m_convertHeading = new QLabel(m_convertPane);
  m_convertHeading->setObjectName("labEditorConvertHeading");
  m_convertHeading->setTextFormat(Qt::PlainText);
  m_convertHeading->setWordWrap(true);
  m_convertNotes = new QLabel(m_convertPane);
  m_convertNotes->setObjectName("labEditorConvertNotes");
  m_convertNotes->setTextFormat(Qt::PlainText);
  m_convertNotes->setWordWrap(true);
  m_convertNotes->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_convertView = new QPlainTextEdit(m_convertPane);
  m_convertView->setObjectName("labEditorConvertView");
  m_convertView->setReadOnly(true);
  m_convertView->setLineWrapMode(QPlainTextEdit::NoWrap);
  m_convertView->setToolTip(tr("The converted SLua text. It is only shown: your script is not changed."));
  m_convertView->setMinimumHeight(6 * m_convertView->fontMetrics().height() + 2 * spacingS);
  QHBoxLayout* convertButtons = new QHBoxLayout;
  convertButtons->setSpacing(spacingS);
  m_convertCopy = makeButton(tr("Copy SLua"), QStringLiteral("labEditorConvertCopy"), m_convertPane);
  m_convertCopy->setToolTip(tr("Copy the converted SLua text"));
  m_convertClose = makeButton(tr("Close"), QStringLiteral("labEditorConvertClose"), m_convertPane);
  m_convertClose->setToolTip(tr("Close the SLua text"));
  convertButtons->addWidget(m_convertCopy);
  convertButtons->addWidget(m_convertClose);
  convertButtons->addStretch(1);
  convertLayout->addWidget(m_convertHeading);
  convertLayout->addWidget(m_convertNotes);
  convertLayout->addWidget(m_convertView, 1);
  convertLayout->addLayout(convertButtons);
  layout->addWidget(m_convertPane);
  m_convertPane->hide();
  connect(m_convertCopy, &QPushButton::clicked, this, [this]()
  {
    QGuiApplication::clipboard()->setText(m_convertView->toPlainText());
    setMessage(tr("The SLua text is copied."), 0);
  });
  connect(m_convertClose, &QPushButton::clicked, this, [this]() { m_convertPane->hide(); m_edit->setFocus(); });

  // --- the text box
  m_edit = new LabCodeEdit(this);
  m_edit->setToolTip(tr("Type to change the script. Ctrl+S saves and runs it."));
  m_edit->setMinimumHeight(10 * m_edit->fontMetrics().height() + 2 * spacingS);
  {
    // the difference in the same fixed-width face (only the face: the size follows the text size)
    QFont face;
    face.setFamilies(QStringList{QFontDatabase::systemFont(QFontDatabase::FixedFont).family()});
    face.setStyleHint(QFont::Monospace);
    face.setFixedPitch(true);
    m_differenceView->setFont(face);
    m_convertView->setFont(face);
  }
  layout->addWidget(m_edit, 1);

  // --- the message line (what happened last, in words)
  m_message = new QLabel(this);
  m_message->setObjectName("labEditorMessage");
  m_message->setWordWrap(true);
  m_message->setTextInteractionFlags(Qt::TextSelectableByMouse);
  QHBoxLayout* messageRow = new QHBoxLayout;
  messageRow->setSpacing(spacingS);
  messageRow->addWidget(m_message, 1);
  messageRow->addWidget(m_convertButton, 0, Qt::AlignTop);
  layout->addLayout(messageRow);

  connect(m_save, &QPushButton::clicked, this, [this]() { save(); });
  connect(m_findButton, &QPushButton::clicked, this, &LabEditor::showFindRow);
  connect(m_goToButton, &QPushButton::clicked, this, &LabEditor::showGoToRow);
  connect(m_edit, &LabCodeEdit::saveRequested, this, [this]() { save(); });
  connect(m_edit, &LabCodeEdit::findRequested, this, &LabEditor::showFindRow);
  connect(m_edit, &LabCodeEdit::goToLineRequested, this, &LabEditor::showGoToRow);
  connect(m_edit, &LabCodeEdit::firstFocus, this, [this]()
  {
    if (m_message->text().isEmpty()) setMessage(tr("Tab adds an indent. To move on, press Esc, then Tab."), 0);
  });
  connect(m_edit, &LabCodeEdit::escapePressed, this, [this]()
  {
    m_findRow->hide();
    m_findResult->hide();
    m_goToRow->hide();
    setMessage(tr("Press Tab to move on, or keep typing."), 0);
  });
  connect(m_edit->document(), &QTextDocument::modificationChanged, this, [this](bool) { updateState(); });

  setTabOrder(m_exampleButton, m_save);
  setTabOrder(m_save, m_findButton);
  setTabOrder(m_findButton, m_goToButton);
  setTabOrder(m_goToButton, m_languageBox);
  setTabOrder(m_languageBox, m_findField);
  setTabOrder(m_findField, m_goToField);
  setTabOrder(m_goToField, m_keep);
  setTabOrder(m_keep, m_load);
  setTabOrder(m_load, m_difference);
  setTabOrder(m_difference, m_differenceView);
  setTabOrder(m_differenceView, m_convertView);
  setTabOrder(m_convertView, m_convertCopy);
  setTabOrder(m_convertCopy, m_convertClose);
  setTabOrder(m_convertClose, m_edit);
  setTabOrder(m_edit, m_convertButton);

  // suggestions and the language follow Setup and Beginner or Expert (jobs F2 and F3)
  UserSettings::watchExpertMode(this, [this](bool) { followSettings(); });
  UserSettings::watchScriptEditor(this, [this]() { followSettings(); });
  followSettings();

  applyStyle();
  updateState();
}

void LabEditor::followSettings()
{
  m_edit->setExpertHelp(UserSettings::expertMode());
  m_edit->setSuggestOnRequestOnly(UserSettings::suggestOnRequestOnly());
  m_languageChoice = UserSettings::scriptLanguage();
  setLuaSwitch(UserSettings::luaSwitch());
}

void LabEditor::setLuaSwitch(bool on)
{
  m_luaSwitch = on;
  applyLanguage();
}

void LabEditor::setLanguageChoice(const QString& id)
{
  m_languageChoice = id == QLatin1String("slua") ? QStringLiteral("slua") : QStringLiteral("lsl");
  applyLanguage();
}

QString LabEditor::languageInUse() const
{
  return m_luaSwitch ? m_languageChoice : QStringLiteral("lsl");
}

void LabEditor::applyLanguage()
{
  // with the switch off nothing says Lua and the language is LSL (charter P5); the choice is kept
  const bool show = m_luaSwitch && !m_path.isEmpty();
  m_languageLabel->setVisible(show);
  m_convertButton->setVisible(show);
  if (!show) m_convertPane->hide();
  m_languageBox->setVisible(show);
  m_languageBox->setCurrentIndex(m_languageChoice == QLatin1String("slua") ? 1 : 0);
  const QString use = languageInUse();
  if (m_edit->suggestEngine().language().id() != use)
  {
    m_edit->hideSuggestions();
    if (use == QLatin1String("slua")) m_edit->suggestEngine().setLanguage(std::make_shared<LabSuggest::SluaLanguage>());
    else m_edit->suggestEngine().setLanguage(std::make_shared<LabSuggest::LslLanguage>());
  }
}

bool LabEditor::convertToSlua()
{
  if (m_path.isEmpty()) return false;
  const lsl::Lsl2Slua::Result result = lsl::Lsl2Slua::convert(m_edit->toPlainText());
  QStringList notes;
  if (!result.ok())
  {
    m_convertHeading->setText(tr("This script cannot be converted yet: it has errors."));
    for (const lsl::SluaWarning& e : result.errors) notes << tr("Line %1: %2").arg(e.line).arg(e.message);
    m_convertView->clear();
  }
  else
  {
    m_convertHeading->setText(result.warnings.empty()
      ? tr("Converted to SLua. Nothing needs a closer look.")
      : tr("Converted to SLua. %n note(s) need a closer look.", nullptr, int(result.warnings.size())));
    for (const lsl::SluaWarning& w : result.warnings)
      notes << tr("Line %1: %2 What to do: %3").arg(w.line).arg(w.message, w.fix);
    m_convertView->setPlainText(result.text);
  }
  notes << tr("The SLua text is only shown. Your script is not changed, and nothing is run. It is not tested in Second Life.");
  m_convertNotes->setText(notes.join(QLatin1Char('\n')));
  m_convertCopy->setEnabled(result.ok());
  m_convertView->setVisible(result.ok());
  m_convertPane->show();
  m_convertHeading->setFocus();
  return result.ok();
}

void LabEditor::setClockForTests(QTime (*clock)())
{
  g_clock = clock;
}

bool LabEditor::eventFilter(QObject* watched, QEvent* event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent* key = static_cast<QKeyEvent*>(event);
    const int k = key->key();
    if (watched == m_findField)
    {
      if (k == Qt::Key_Escape) { m_findRow->hide(); m_findResult->hide(); m_edit->setFocus(); return true; }
      if (k == Qt::Key_Return || k == Qt::Key_Enter || k == Qt::Key_F3)
      {
        find(m_findField->text(), (key->modifiers() & Qt::ShiftModifier) != 0);
        return true;
      }
    }
    if (watched == m_goToField)
    {
      if (k == Qt::Key_Escape) { m_goToRow->hide(); m_edit->setFocus(); return true; }
      if (k == Qt::Key_Return || k == Qt::Key_Enter)
      {
        if (goToLine(m_goToField->text().toInt())) m_goToRow->hide();
        return true;
      }
    }
  }
  if (event->type() == QEvent::ShortcutOverride && (watched == m_findField || watched == m_goToField))
  {
    QKeyEvent* key = static_cast<QKeyEvent*>(event);
    if (key->key() == Qt::Key_Escape || key->key() == Qt::Key_F3) { event->accept(); return true; }
  }
  return QWidget::eventFilter(watched, event);
}

bool LabEditor::isModified() const
{
  return m_edit->document()->isModified();
}

QString LabEditor::unsavedMark() const
{
  return isModified() ? tr("● unsaved changes") : QString();
}

QString LabEditor::readDisk(bool* ok) const
{
  *ok = false;
  if (m_path.isEmpty()) return QString();
  QFile file(m_path);
  if (!file.open(QIODevice::ReadOnly)) return QString();
  const QByteArray bytes = file.read(4 * 1024 * 1024);
  *ok = true;
  QString text = QString::fromUtf8(bytes);
  text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  return text;
}

void LabEditor::loadText(const QString& text, bool undoable)
{
  if (!undoable)
  {
    m_edit->setPlainText(text);
    m_edit->document()->setModified(false);
    return;
  }
  // one undo step that keeps the history, and the cursor on the same line and column where it can
  const int line = m_edit->textCursor().blockNumber();
  const int column = m_edit->textCursor().positionInBlock();
  const int scroll = m_edit->verticalScrollBar()->value();
  QTextCursor all(m_edit->document());
  all.beginEditBlock();
  all.select(QTextCursor::Document);
  all.insertText(text);
  all.endEditBlock();
  m_edit->document()->setModified(false);
  const QTextBlock block = m_edit->document()->findBlockByNumber(qMin(line, m_edit->document()->blockCount() - 1));
  QTextCursor cursor(block);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qMin(column, block.length() - 1));
  m_edit->setTextCursor(cursor);
  m_edit->verticalScrollBar()->setValue(scroll);
}

bool LabEditor::openFile(const QString& absolutePath, const QString& name)
{
  if (absolutePath == m_path)
  {
    m_name = name;
    checkDisk();
    updateState();
    return true;
  }
  if (isModified()) return false;   // never replace unsaved edits
  m_path = absolutePath;
  m_name = name;
  bool ok = false;
  const QString text = readDisk(&ok);
  m_saved = ok ? text : QString();
  m_outsidePending = false;
  m_acknowledgedDisk.clear();
  m_waitingForRun = false;
  showOutsideBar(false);
  m_convertPane->hide();
  loadText(m_saved, false);   // another file: a fresh undo history
  m_edit->setProblems({});
  setMessage(QString(), 0);
  updateState();
  return true;
}

bool LabEditor::closeFile()
{
  if (isModified()) return false;
  m_path.clear();
  m_name.clear();
  m_saved.clear();
  m_outsidePending = false;
  m_acknowledgedDisk.clear();
  m_waitingForRun = false;
  showOutsideBar(false);
  m_convertPane->hide();
  loadText(QString(), false);
  m_edit->setProblems({});
  setMessage(QString(), 0);
  updateState();
  return true;
}

bool LabEditor::checkDisk()
{
  bool ok = false;
  const QString disk = readDisk(&ok);
  if (!ok || disk == m_saved) return false;
  if (!isModified())
  {
    // nothing typed: show the new text, as one undo step
    m_saved = disk;
    loadText(disk, true);
    setMessage(tr("Loaded the changed file. Ctrl+Z brings the old text back."), 0);
    updateState();
    return true;
  }
  if (disk == m_acknowledgedDisk) return false;              // already answered: keep my changes
  if (m_outsidePending && disk == m_outsideDisk) return false;   // already asked about this text
  m_outsideDisk = disk;
  m_outsidePending = true;
  showOutsideBar(true);
  updateState();
  return true;
}

void LabEditor::showOutsideBar(bool on)
{
  m_outsideBar->setVisible(on);
  if (!on) m_differenceView->hide();
}

void LabEditor::answerOutsideChange(OutsideChoice choice)
{
  if (!m_outsidePending) return;
  switch (choice)
  {
    case KeepMine:
      m_acknowledgedDisk = m_outsideDisk;
      m_outsidePending = false;
      showOutsideBar(false);
      setMessage(tr("Your changes are kept. Save to write them to the file."), 0);
      break;
    case LoadNew:
      m_saved = m_outsideDisk;
      m_outsidePending = false;
      m_acknowledgedDisk.clear();
      showOutsideBar(false);
      loadText(m_outsideDisk, true);
      setMessage(tr("Loaded the new file. Ctrl+Z brings your changes back."), 0);
      break;
    case ShowDifference:
      m_differenceView->setPlainText(differenceText(m_edit->toPlainText(), m_outsideDisk));
      m_differenceView->show();
      break;
  }
  updateState();
}

QString LabEditor::differenceText(const QString& mine, const QString& theirs)
{
  const QStringList a = mine.split(QLatin1Char('\n'));
  const QStringList b = theirs.split(QLatin1Char('\n'));
  // the same lines at the start and the end are not compared again
  int head = 0;
  while (head < a.size() && head < b.size() && a.at(head) == b.at(head)) ++head;
  int tail = 0;
  while (tail < a.size() - head && tail < b.size() - head && a.at(a.size() - 1 - tail) == b.at(b.size() - 1 - tail)) ++tail;
  const int n = int(a.size()) - head - tail;
  const int m = int(b.size()) - head - tail;
  QStringList out;
  for (int i = 0; i < head; ++i) out << QStringLiteral("  ") + a.at(i);
  // the table has (n + 1) * (m + 1) cells: counted in qsizetype (no int overflow) and bounded,
  // so a very long file never allocates a huge table; nothing to compare when one side is empty
  const qsizetype cells = (qsizetype(n) + 1) * (qsizetype(m) + 1);
  if (n == 0 || m == 0 || cells > DifferenceTableLimit)
  {
    // one side empty, or too big to compare line by line: all of yours, then all of the new one
    for (int i = 0; i < n; ++i) out << QStringLiteral("- ") + a.at(head + i);
    for (int j = 0; j < m; ++j) out << QStringLiteral("+ ") + b.at(head + j);
  }
  else
  {
    // longest common lines (dynamic programming), then walk it
    const qsizetype width = qsizetype(m) + 1;
    QVector<int> table(cells, 0);
    const auto at = [&](int i, int j) -> int& { return table[qsizetype(i) * width + j]; };
    for (int i = n - 1; i >= 0; --i)
      for (int j = m - 1; j >= 0; --j)
        at(i, j) = a.at(head + i) == b.at(head + j) ? at(i + 1, j + 1) + 1 : qMax(at(i + 1, j), at(i, j + 1));
    int i = 0;
    int j = 0;
    while (i < n || j < m)
    {
      if (i < n && j < m && a.at(head + i) == b.at(head + j)) { out << QStringLiteral("  ") + a.at(head + i); ++i; ++j; }
      else if (j < m && (i == n || at(i, j + 1) >= at(i + 1, j))) { out << QStringLiteral("+ ") + b.at(head + j); ++j; }
      else { out << QStringLiteral("- ") + a.at(head + i); ++i; }
    }
  }
  for (int k = int(a.size()) - tail; k < a.size(); ++k) out << QStringLiteral("  ") + a.at(k);
  return out.join(QLatin1Char('\n'));
}

namespace
{
  // Writes the whole file through a temporary file that replaces it (QSaveFile); "" when it worked,
  // otherwise the reason. On Windows another program (a virus scanner, the search indexer, a cloud
  // sync tool) often holds a file open for a moment right after it was written, and then replacing
  // it fails with "Access is denied"; so a failed attempt is tried again a few times, a little
  // later each time (about a third of a second in all), before the save counts as failed (job T2).
  QString saveWhole(const QString& path, const QByteArray& bytes)
  {
    const int attempts = 5;
    QString error;
    for (int attempt = 1; attempt <= attempts; ++attempt)
    {
      QSaveFile file(path);
      file.setDirectWriteFallback(true);
      if (file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit()) return QString();
      error = file.errorString();
      if (error.isEmpty()) error = QCoreApplication::translate("LabEditor", "the file could not be written");
      if (attempt < attempts)
      {
        qCWarning(lcFile).noquote() << "Script Lab: saving a script failed (attempt" << attempt << "of" << attempts << "):" << error << "Trying again.";
        QThread::msleep(20u << (attempt - 1));   // 20, 40, 80, 160 ms
      }
    }
    return error;
  }
}

bool LabEditor::save()
{
  if (m_path.isEmpty())
  {
    setMessage(tr("Nothing to save. Open a script first."), 2);
    return false;
  }
  const bool crlf = [&]() { QFile f(m_path); return f.open(QIODevice::ReadOnly) && f.read(65536).contains("\r\n"); }();
  const QString text = m_edit->toPlainText();
  QString out = text;
  if (crlf) out.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));   // keep the file's own line ends
  const QString error = saveWhole(m_path, out.toUtf8());
  if (!error.isEmpty())
  {
    setMessage(tr("Not saved: %1. Check the folder and try again.").arg(error), 2);
    return false;
  }
  m_saved = text;
  m_edit->document()->setModified(false);   // the undo history stays
  m_outsidePending = false;
  m_acknowledgedDisk.clear();
  showOutsideBar(false);
  m_savedAt = now();
  m_waitingForRun = true;
  setMessage(tr("Saved %1. Running it now.").arg(m_savedAt.toString(QStringLiteral("HH:mm"))), 0);
  updateState();
  emit saved(m_path);
  return true;
}

void LabEditor::reportRun(int errors, int warnings, const QString& notRun)
{
  m_waitingForRun = false;
  const QString time = m_savedAt.isValid() ? m_savedAt.toString(QStringLiteral("HH:mm")) : now().toString(QStringLiteral("HH:mm"));
  if (!notRun.isEmpty())
  {
    setMessage(tr("Saved %1. Not run: %2").arg(time, notRun), 2);
    return;
  }
  QString count = errors == 1 ? tr("1 error") : tr("%1 errors").arg(errors);
  if (warnings == 1) count += tr(", 1 warning");
  else if (warnings > 1) count += tr(", %1 warnings").arg(warnings);
  setMessage(tr("Saved and run %1. %2.").arg(time, count), errors > 0 ? 2 : 1);
}

bool LabEditor::goToLine(int line)
{
  const int lines = m_edit->document()->blockCount();
  if (line < 1 || line > lines)
  {
    setMessage(lines == 1 ? tr("There is no line %1. The script has 1 line.").arg(line)
                          : tr("There is no line %1. The script has %2 lines.").arg(line).arg(lines), 2);
    return false;
  }
  QTextCursor cursor(m_edit->document()->findBlockByNumber(line - 1));
  m_edit->setTextCursor(cursor);
  m_edit->centerCursor();
  m_edit->setFocus(Qt::OtherFocusReason);
  return true;
}

bool LabEditor::find(const QString& text, bool backwards)
{
  if (text.isEmpty()) return false;
  QTextDocument* doc = m_edit->document();
  QTextDocument::FindFlags flags;
  if (backwards) flags |= QTextDocument::FindBackward;
  QTextCursor found = doc->find(text, m_edit->textCursor(), flags);
  if (found.isNull())
  {
    // round from the other end
    QTextCursor start(doc);
    if (backwards) start.movePosition(QTextCursor::End);
    found = doc->find(text, start, flags);
  }
  // how many matches, and which one this is
  int total = 0;
  int index = 0;
  for (QTextCursor c = doc->find(text, 0); !c.isNull(); c = doc->find(text, c))
  {
    ++total;
    if (!found.isNull() && c.selectionStart() == found.selectionStart()) index = total;
  }
  m_findResult->show();
  if (found.isNull())
  {
    m_findResult->setText(tr("Not found: %1").arg(text));
    QPalette p = m_findResult->palette();
    p.setColor(QPalette::WindowText, token(Theme::Token::Danger, paletteIsDark(palette())));
    m_findResult->setPalette(p);
    return false;
  }
  m_edit->setTextCursor(found);
  m_edit->centerCursor();
  m_findResult->setText(tr("Match %1 of %2").arg(index).arg(total));
  QPalette p = m_findResult->palette();
  p.setColor(QPalette::WindowText, token(Theme::Token::TextMuted, paletteIsDark(palette())));
  m_findResult->setPalette(p);
  return true;
}

void LabEditor::showFindRow()
{
  if (m_path.isEmpty()) return;
  m_goToRow->hide();
  m_findRow->show();
  const QString selected = m_edit->textCursor().selectedText();
  if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator)) m_findField->setText(selected);
  m_findField->selectAll();
  m_findField->setFocus(Qt::ShortcutFocusReason);
}

void LabEditor::showGoToRow()
{
  if (m_path.isEmpty()) return;
  m_findRow->hide();
  m_findResult->hide();
  m_goToRow->show();
  m_goToField->setText(QString::number(m_edit->textCursor().blockNumber() + 1));
  m_goToField->selectAll();
  m_goToField->setFocus(Qt::ShortcutFocusReason);
}

void LabEditor::setExamplesFolder(const QString& folder)
{
  m_examplesFolder = folder;
  updateState();
}

QStringList LabEditor::exampleFiles() const
{
  if (m_examplesFolder.isEmpty()) return {};
  return QDir(m_examplesFolder).entryList(QStringList{QStringLiteral("*.lsl")}, QDir::Files, QDir::Name);
}

void LabEditor::fillExampleMenu()
{
  m_exampleMenu->clear();
  for (const QString& file : exampleFiles())
  {
    QAction* action = m_exampleMenu->addAction(file);
    connect(action, &QAction::triggered, this, [this, file]() { newFromExample(file); });
  }
}

bool LabEditor::newFromExample(const QString& exampleFile)
{
  if (isModified())
  {
    setMessage(tr("Save or undo your changes first."), 2);
    return false;
  }
  if (m_targetFolder.isEmpty())
  {
    setMessage(tr("Watch a folder first. The new script is saved there."), 2);
    return false;
  }
  QFile in(QDir(m_examplesFolder).filePath(exampleFile));
  if (!in.open(QIODevice::ReadOnly))
  {
    setMessage(tr("The example %1 cannot be read.").arg(exampleFile), 2);
    return false;
  }
  QString text = QString::fromUtf8(in.read(4 * 1024 * 1024));
  text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  // its own name in the folder, or name_2.lsl and so on when that is taken
  const QDir target(m_targetFolder);
  const QString base = QFileInfo(exampleFile).completeBaseName();
  QString name = exampleFile;
  for (int i = 2; target.exists(name); ++i) name = QStringLiteral("%1_%2.lsl").arg(base).arg(i);
  m_path = target.filePath(name);
  m_name = name;
  m_saved.clear();
  m_outsidePending = false;
  m_acknowledgedDisk.clear();
  m_waitingForRun = false;
  showOutsideBar(false);
  loadText(text, false);
  m_edit->setProblems({});
  m_edit->document()->setModified(true);   // not on disk yet
  setMessage(tr("New script %1 from an example. Press Ctrl+S to save and run it.").arg(name), 0);
  updateState();
  m_edit->setFocus(Qt::OtherFocusReason);
  return true;
}

void LabEditor::setHintsOn(bool on)
{
  m_hintsOn = on;
  m_edit->setSuggestHints(on);
  updateState();
}

void LabEditor::setMessage(const QString& text, int tone)
{
  m_message->setText(text);
  m_message->setVisible(!text.isEmpty());
  m_messageTone = tone;
  const bool dark = paletteIsDark(palette());
  QPalette p = m_message->palette();
  p.setColor(QPalette::WindowText, token(tone == 1 ? Theme::Token::Success : tone == 2 ? Theme::Token::Danger : Theme::Token::TextMuted, dark));
  m_message->setPalette(p);
}

void LabEditor::updateState()
{
  const bool open = !m_path.isEmpty();
  m_starter->setVisible(!open);
  m_rlvCard->setVisible(!open && m_hintsOn);
  const bool haveExamples = !exampleFiles().isEmpty();
  m_exampleButton->setEnabled(haveExamples);
  m_exampleButton->setToolTip(haveExamples ? tr("Open an example script as a new file in this folder")
                                           : tr("The example scripts are not next to the program."));
  for (QWidget* w : QList<QWidget*>{m_save, m_findButton, m_goToButton, m_edit}) w->setVisible(open);
  applyLanguage();
  if (!open)
  {
    m_findRow->hide();
    m_findResult->hide();
    m_goToRow->hide();
  }
  const bool modified = isModified();
  const bool canSave = open && (modified || !QFileInfo::exists(m_path));
  m_save->setEnabled(canSave);
  m_save->setToolTip(canSave ? tr("Save the script and run it (Ctrl+S)") : tr("Nothing to save. Change the script first."));
  // its line is kept while a file is open (nothing moves when it shows), and gone with no file
  QSizePolicy keep = m_unsaved->sizePolicy();
  if (keep.retainSizeWhenHidden() != open)
  {
    keep.setRetainSizeWhenHidden(open);
    m_unsaved->setSizePolicy(keep);
  }
  m_unsaved->setVisible(open && modified);
  if (modified != m_lastModified)
  {
    m_lastModified = modified;
    emit modifiedChanged(modified);
  }
}

void LabEditor::applyStyle()
{
  const bool dark = paletteIsDark(palette());
  const QColor border = token(Theme::Token::Border, dark);
  const QColor surface = token(Theme::Token::Surface, dark);
  const QColor warning = token(Theme::Token::Warning, dark);
  const QColor disabled = token(Theme::Token::TextDisabled, dark);
  const QColor text = token(Theme::Token::Text, dark);
  m_starter->setLook(border, QColor(), radiusPanel, true);
  m_rlvCard->setLook(border, Theme::mix(surface, token(Theme::Token::AccentText, dark), 0.08), radiusPanel, false);
  m_rlvIcon->setPixmap(LucideIcons::icon(QStringLiteral("lightbulb"), token(Theme::Token::AccentText, dark), disabled).pixmap(iconSmall, iconSmall));
  m_outsideBar->setLook(warning, Theme::mix(surface, warning, 0.10), radiusControl, false);
  m_outsideIcon->setPixmap(LucideIcons::icon(QStringLiteral("triangle-alert"), warning, disabled).pixmap(iconSmall, iconSmall));
  QPalette muted = m_starterText->palette();
  muted.setColor(QPalette::WindowText, token(Theme::Token::TextMuted, dark));
  m_starterText->setPalette(muted);
  // normal buttons (surface_raised fill, border, text), Save with its icon; greyed with text_muted and a
  // half border (job U3: higher contrast, so the word on a greyed button stays readable at 4.5:1)
  const QString normal = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:disabled { color: %6; border: 1px solid %8; }"
    "QPushButton:focus { border: 2px solid %7; }")
    .arg(token(Theme::Token::SurfaceRaised, dark).name(), text.name(), border.name())
    .arg(radiusControl).arg(spacingM)
    .arg(token(Theme::Token::TextMuted, dark).name(), token(Theme::Token::FocusRing, dark).name(),
         Theme::mix(token(Theme::Token::SurfaceRaised, dark), border, 0.5).name());
  QPalette unsaved = m_unsaved->palette();
  unsaved.setColor(QPalette::WindowText, warning);
  m_unsaved->setPalette(unsaved);
  for (QPushButton* b : findChildren<QPushButton*>()) b->setStyleSheet(normal);
  m_save->setIcon(LucideIcons::icon(QStringLiteral("save"), text, disabled));
  m_exampleButton->setIcon(LucideIcons::icon(QStringLiteral("file-code"), text, disabled));
  setMessage(m_message->text(), m_messageTone);
}

void LabEditor::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::PaletteChange) applyStyle();
}

bool LabEditor::event(QEvent* event)
{
  return QWidget::event(event);
}
