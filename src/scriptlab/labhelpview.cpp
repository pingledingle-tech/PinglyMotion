/*
 * PinglyMotion - Script Lab help, out of the working parts (see labhelpview.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "labhelpview.h"

#include <algorithm>

#include <QDir>
#include <QEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollBar>
#include <QFontMetrics>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

#include "dialogfit.h"
#include "labbeginner.h"
#include "theme.h"

namespace
{
  constexpr const char* kHighlightName = "labHighlight";
  constexpr int TopicRole = Qt::UserRole + 1;

  QString esc(const QString& text)
  {
    return text.toHtmlEscaped();
  }

  QString paragraphs(const QStringList& lines)
  {
    QString out;
    for (const QString& line : lines) out += QStringLiteral("<p>%1</p>").arg(esc(line));
    return out;
  }

  QString bullets(const QStringList& lines)
  {
    if (lines.isEmpty()) return QString();
    QString out = QStringLiteral("<ul>");
    for (const QString& line : lines) out += QStringLiteral("<li>%1</li>").arg(esc(line));
    return out + QStringLiteral("</ul>");
  }

  QString numbered(const QStringList& lines)
  {
    if (lines.isEmpty()) return QString();
    QString out = QStringLiteral("<ol>");
    for (const QString& line : lines) out += QStringLiteral("<li>%1</li>").arg(esc(line));
    return out + QStringLiteral("</ol>");
  }

  QString heading(const QString& text, const QString& colour = QString())
  {
    if (colour.isEmpty()) return QStringLiteral("<h3>%1</h3>").arg(esc(text));
    return QStringLiteral("<h3 style=\"color:%1\">%2</h3>").arg(colour, esc(text));
  }

  // the examples next to the program, by their order
  QList<LabExample> examples(const QString& folder)
  {
    QList<LabExample> out;
    const QDir dir(folder);
    for (const QString& name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
    {
      const LabExample example = LabBeginner::readExample(dir.filePath(name));
      if (example.isValid()) out << example;
    }
    std::stable_sort(out.begin(), out.end(), [](const LabExample& a, const LabExample& b) { return a.order < b.order; });
    return out;
  }
}

LabHelpView::LabHelpView(const QString& examplesFolder, QWidget* parent) :
  QDialog(parent), m_examplesFolder(examplesFolder)
{
  setObjectName(QStringLiteral("scriptLabHelpView"));
  setWindowTitle(tr("Script Lab help"));
  setModal(false);
  setAttribute(Qt::WA_DeleteOnClose, false);

  QVBoxLayout* layout = DialogFit::contentLayout(this);
  layout->setSpacing(Theme::Size::spacingM);
  QHBoxLayout* row = new QHBoxLayout;
  row->setSpacing(Theme::Size::spacingL);

  m_list = new QListWidget(this);
  m_list->setObjectName(QStringLiteral("scriptLabHelpTopics"));
  m_list->setAccessibleName(tr("Help topics"));
  m_list->setUniformItemSizes(false);
  m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  row->addWidget(m_list, 0);

  QVBoxLayout* right = new QVBoxLayout;
  right->setSpacing(Theme::Size::spacingS);
  m_heading = new QLabel(this);
  m_heading->setObjectName(QStringLiteral("scriptLabHelpHeading"));
  m_heading->setWordWrap(true);
  right->addWidget(m_heading);
  m_text = new QTextBrowser(this);
  m_text->setObjectName(QStringLiteral("scriptLabHelpText"));
  m_text->setOpenLinks(false);
  m_text->setFrameShape(QFrame::NoFrame);
  m_text->setMinimumWidth(Theme::Size::controlDefault * 12);
  m_text->setMinimumHeight(Theme::Size::controlDefault * 10);
  right->addWidget(m_text, 1);
  row->addLayout(right, 1);
  layout->addLayout(row, 1);

  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->addStretch(1);
  m_close = new QPushButton(tr("Close"), this);
  m_close->setObjectName(QStringLiteral("scriptLabHelpClose"));
  m_close->setMinimumHeight(Theme::Size::controlDefault);
  connect(m_close, &QPushButton::clicked, this, &QDialog::close);
  buttons->addWidget(m_close);
  DialogFit::finish(this, buttons, QSize(720, 520));

  fill();
  connect(m_list, &QListWidget::currentRowChanged, this, [this](int) {
    const QString key = currentTopic();
    if (key.isEmpty()) return;
    const int i = m_keys.indexOf(key);
    m_heading->setText(topicTitles().value(i));
    m_text->setHtml(topicHtml(key));
  });
  showTopic(QStringLiteral("start"));
  m_list->setFocus(Qt::OtherFocusReason);
  applyStyle(Theme::color(Theme::Token::Background).lightness() < 128);
}

void LabHelpView::fill()
{
  m_list->clear();
  m_keys.clear();
  auto add = [this](const QString& key, const QString& title) {
    QListWidgetItem* item = new QListWidgetItem(title, m_list);
    item->setData(TopicRole, key);
    m_keys << key;
  };
  add(QStringLiteral("start"), tr("Start here"));
  add(QStringLiteral("folder"), tr("How to lay out a folder"));
  add(QStringLiteral("words"), tr("Words"));
  add(QStringLiteral("hints"), tr("Hints"));
  const QList<LabExample> list = examples(m_examplesFolder);
  if (!list.isEmpty())
  {
    // the heading over the examples: not a topic, it cannot be picked
    QListWidgetItem* head = new QListWidgetItem(tr("Examples"), m_list);
    head->setFlags(Qt::NoItemFlags);
    QFont bold = head->font();
    bold.setWeight(QFont::DemiBold);
    head->setFont(bold);
  }
  for (const LabExample& example : list)
    add(QStringLiteral("example:") + example.object, tr("Example: %1").arg(example.title));
}

QStringList LabHelpView::topicTitles() const
{
  QStringList out;
  for (int i = 0; i < m_list->count(); ++i)
    if (!m_list->item(i)->data(TopicRole).toString().isEmpty()) out << m_list->item(i)->text();
  return out;
}

QString LabHelpView::currentTopic() const
{
  const QListWidgetItem* item = m_list->currentItem();
  return item ? item->data(TopicRole).toString() : QString();
}

bool LabHelpView::showTopic(const QString& key)
{
  for (int i = 0; i < m_list->count(); ++i)
    if (m_list->item(i)->data(TopicRole).toString() == key)
    {
      m_list->setCurrentRow(i);
      // setCurrentRow sends no signal when the row is already current
      m_heading->setText(m_list->item(i)->text());
      m_text->setHtml(topicHtml(key));
      return true;
    }
  return false;
}

QString LabHelpView::topicHeading() const
{
  return m_heading->text();
}

QString LabHelpView::topicText() const
{
  return m_text->toPlainText();
}

QString LabHelpView::topicHtml(const QString& key) const
{
  const QString warning = Theme::color(Theme::Token::Warning, m_dark).name();
  if (key == QLatin1String("start"))
  {
    return paragraphs({tr("Script Lab tests Second Life scripts safely on your PC."),
                       tr("Make the example project first. It shows a result at once."),
                       tr("Then follow the steps of an example.")})
           + numbered({tr("Press Make example project on the Live scripts tab."),
                       tr("Press Show effects in 3D view, so it shows a tick."),
                       tr("Click a prop in the 3D view.")})
           + paragraphs({tr("Help, Script Lab steps shows the steps of the example again.")});
  }
  if (key == QLatin1String("folder"))
  {
    // the maker guide (job U3), moved here from the Live scripts page: rules and warnings apart
    return heading(tr("Rules"))
           + bullets({tr("One folder is one object. Each subfolder is one prim."),
                      tr("Files in the top folder or in Root form the root prim."),
                      tr("Put scripts (.lsl) and notecards (.txt) in their prim's folder."),
                      tr("Give every script and notecard its own name.")})
           + heading(tr("Warnings"), warning)
           + bullets({tr("Copy scripts from Second Life by hand: select all, copy, paste."),
                      tr("The sandbox is never the same as Second Life. Test there last.")})
           + heading(tr("Example"))
           + QStringLiteral("<pre>%1</pre>").arg(esc(tr("MyObject/        one object\n"
                                                        "  Root/          the root prim\n"
                                                        "    main.lsl\n"
                                                        "    settings.txt   a notecard\n"
                                                        "  Cuff_left/     another prim\n"
                                                        "    cuff.lsl")));
  }
  if (key == QLatin1String("words"))
  {
    QString out = paragraphs({tr("The words Script Lab uses, in plain words.")}) + QStringLiteral("<ul>");
    for (const LabBeginner::Tip& tip : LabBeginner::jargonTips())
      out += QStringLiteral("<li><b>%1</b>: %2</li>").arg(esc(tip.word), esc(tip.text));
    return out + QStringLiteral("</ul>");
  }
  if (key == QLatin1String("hints"))
  {
    return paragraphs({tr("Hints are small cards next to the part they talk about."),
                       tr("The cross closes a hint. Turn Help > Show hints off and on for the next one."),
                       tr("A closed word tip in Script Lab does not come back. All the words are under Words here.")});
  }
  if (key.startsWith(QLatin1String("example:")))
  {
    const QString object = key.mid(int(qstrlen("example:")));
    for (const LabExample& example : examples(m_examplesFolder))
    {
      if (example.object != object) continue;
      QStringList steps;
      for (const LabExample::Entry& step : example.steps()) steps << step.text;
      QString out = heading(tr("What it is")) + paragraphs(example.texts(QStringLiteral("what")))
                    + heading(tr("Steps")) + numbered(steps);
      if (!example.texts(QStringLiteral("piece")).isEmpty())
        out += heading(tr("The pieces")) + bullets(example.texts(QStringLiteral("piece")));
      if (!example.texts(QStringLiteral("word")).isEmpty())
        out += heading(tr("New words")) + bullets(example.texts(QStringLiteral("word")));
      if (!example.texts(QStringLiteral("see")).isEmpty())
        out += heading(tr("You should see")) + bullets(example.texts(QStringLiteral("see")));
      if (!example.texts(QStringLiteral("next")).isEmpty())
        out += heading(tr("Then try")) + paragraphs(example.texts(QStringLiteral("next")));
      return out;
    }
  }
  return QString();
}

void LabHelpView::applyStyle(bool dark)
{
  if (m_styling) return;
  m_styling = true;
  m_dark = dark;
  const QColor text = Theme::color(Theme::Token::Text, dark);
  const QColor surface = Theme::color(Theme::Token::Surface, dark);
  const QColor raised = Theme::color(Theme::Token::SurfaceRaised, dark);
  const QColor selection = Theme::color(Theme::Token::Selection, dark);
  const QColor onAccent = Theme::color(Theme::Token::OnAccent, dark);
  const QColor muted = Theme::color(Theme::Token::TextMuted, dark);
  QPalette p = m_text->palette();
  p.setColor(QPalette::Base, surface);
  p.setColor(QPalette::Text, text);
  m_text->setPalette(p);
  // the picked topic: the selection fill with on_accent text, and bold (DESIGN_SYSTEM.md, "States")
  m_list->setStyleSheet(QStringLiteral(
    "QListWidget { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: %5px; }"
    "QListWidget::item { padding: %6px %5px; border-radius: %7px; }"
    "QListWidget::item:selected { background: %8; color: %9; font-weight: 600; }"
    "QListWidget::item:disabled { color: %10; }"
    "QListWidget:focus { border: 2px solid %11; }")
    .arg(raised.name(), text.name(), Theme::color(Theme::Token::Border, dark).name())
    .arg(Theme::Size::radiusControl).arg(Theme::Size::spacingS).arg(Theme::Size::spacingXs).arg(Theme::Size::radiusSmall)
    .arg(selection.name(), onAccent.name(), muted.name(), Theme::color(Theme::Token::FocusRing, dark).name()));
  QFont headingFont = font();   // the "heading" type token: 20 px next to the 13 px body
  headingFont.setWeight(QFont::DemiBold);
  if (headingFont.pointSizeF() > 0) headingFont.setPointSizeF(headingFont.pointSizeF() * 20.0 / 13.0);
  m_heading->setFont(headingFont);
  // the list is as wide as its longest topic, so no topic is cut
  int widest = 0;
  for (int i = 0; i < m_list->count(); ++i)
  {
    QFont f = m_list->font();
    f.setWeight(QFont::DemiBold);
    widest = std::max(widest, QFontMetrics(f).horizontalAdvance(m_list->item(i)->text()));
  }
  m_list->setMinimumWidth(widest + 4 * Theme::Size::spacingS + m_list->verticalScrollBar()->sizeHint().width());
  const QString key = currentTopic();
  if (!key.isEmpty()) m_text->setHtml(topicHtml(key));
  m_styling = false;
}

void LabHelpView::changeEvent(QEvent* event)
{
  QDialog::changeEvent(event);
  if (event->type() == QEvent::PaletteChange || event->type() == QEvent::FontChange)
    applyStyle(Theme::color(Theme::Token::Background).lightness() < 128);
}

// ---------------------------------------------------------------------------------------------

LabHighlight::LabHighlight(QWidget* target, const QRect& area, bool dark) :
  QWidget(target), m_area(area), m_dark(dark)
{
  setObjectName(QString::fromLatin1(kHighlightName));
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_NoSystemBackground);
  setFocusPolicy(Qt::NoFocus);
  setGeometry(target->rect());
  raise();
}

LabHighlight* LabHighlight::on(QWidget* target)
{
  if (!target) return nullptr;
  return static_cast<LabHighlight*>(target->findChild<QWidget*>(QString::fromLatin1(kHighlightName), Qt::FindDirectChildrenOnly));
}

LabHighlight* LabHighlight::show(QWidget* target, const QRect& area, bool dark)
{
  if (!target) return nullptr;
  delete on(target);
  LabHighlight* frame = new LabHighlight(target, area, dark);
  frame->QWidget::show();
  QPointer<LabHighlight> guard(frame);
  QTimer::singleShot(ShowMs, frame, [guard]() { if (guard) guard->deleteLater(); });
  return frame;
}

void LabHighlight::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const int w = Theme::Size::focusRingWidth;
  const QRectF r = QRectF(m_area.intersected(rect())).adjusted(w / 2.0, w / 2.0, -w / 2.0, -w / 2.0);
  // a light selection fill and the focus ring colour round it: the same "this one" signs as the
  // rest of the program (DESIGN_SYSTEM.md, "States"), and a frame, so not by colour alone
  QColor fill = Theme::color(Theme::Token::Selection, m_dark);
  fill.setAlphaF(0.22);
  painter.setPen(QPen(Theme::color(Theme::Token::FocusRing, m_dark), w));
  painter.setBrush(fill);
  painter.drawRoundedRect(r, Theme::Size::radiusControl, Theme::Size::radiusControl);
}
