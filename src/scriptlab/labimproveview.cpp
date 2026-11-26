/*
 * PinglyMotion Script Lab - the page «Improve a setup» (queue 1, item 5, parts b and c). See labimproveview.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "labimproveview.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

#include "lucideicons.h"
#include "theme.h"

namespace
{
  bool isDark(const QWidget* w) { return w->palette().color(QPalette::Window).lightness() < 128; }

  // the role of a suggestion's file and line in the list
  constexpr int FileRole = Qt::UserRole;
  constexpr int LineRole = Qt::UserRole + 1;

  // a word-wrapped label whose size hint is the height its text takes at the width it has (as the Hands tab and the
  // prop edit card do), so a long line wraps instead of asking for a narrow, tall label
  class WrapLabel : public QLabel
  {
    public:
      explicit WrapLabel(QWidget* parent) : QLabel(parent)
      {
        setTextFormat(Qt::PlainText);
        setWordWrap(true);
      }
      QSize sizeHint() const override
      {
        int word = 0;
        for (const QString& part : text().split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts))
          word = std::max(word, fontMetrics().horizontalAdvance(part));
        const QMargins m = contentsMargins();
        const int least = word + m.left() + m.right() + 1;
        return QSize(least, heightForWidth(std::max(width(), least)));
      }
      QSize minimumSizeHint() const override { return QSize(sizeHint().width(), fontMetrics().height()); }
  };
}

LabImproveView::LabImproveView(QWidget* parent)
  : QWidget(parent)
{
  setObjectName("scriptLabImprove");
  m_layout = new QVBoxLayout(this);
  m_layout->setContentsMargins(0, 0, 0, 0);
  m_layout->setSpacing(Theme::Size::spacingS);

  // LABTWO-12, -13 and -17, the three steps as this program does them
  m_title = new QLabel(tr("Improve a finished setup"), this);
  m_title->setObjectName("scriptLabImproveTitle");
  m_title->setTextFormat(Qt::PlainText);
  m_layout->addWidget(m_title);
  m_steps = new WrapLabel(this);
  m_steps->setText(tr("1. Watch the folder as one object on Live scripts.\n2. Record what it does today on Player.\n3. Read below what can be better."));
  m_steps->setObjectName("scriptLabImproveSteps");
  m_layout->addWidget(m_steps);

  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->setSpacing(Theme::Size::spacingS);
  m_import = new QPushButton(tr("Import a setup"), this);   // LABTWO-14
  m_import->setObjectName("scriptLabImproveImport");
  m_import->setAccessibleName(tr("Import a setup"));
  m_import->setToolTip(tr("Pick the folder of a setup you made before. It is watched as one object."));
  m_import->setMinimumHeight(Theme::Size::controlCompact);
  m_import->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  buttons->addWidget(m_import);
  m_record = new QPushButton(tr("Record today's behaviour"), this);   // LABTWO-15
  m_record->setObjectName("scriptLabImproveRecord");
  m_record->setAccessibleName(tr("Record today's behaviour"));
  m_record->setMinimumHeight(Theme::Size::controlCompact);
  m_record->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  buttons->addWidget(m_record);
  buttons->addStretch(1);
  m_layout->addLayout(buttons);
  connect(m_import, &QPushButton::clicked, this, &LabImproveView::importRequested);
  connect(m_record, &QPushButton::clicked, this, &LabImproveView::recordRequested);

  m_status = new WrapLabel(this);
  m_status->setObjectName("scriptLabImproveStatus");
  m_layout->addWidget(m_status);

  // the weight (part b)
  m_weightTitle = heading(tr("Weight"), "scriptLabImproveWeightTitle");
  m_weight = new WrapLabel(this);
  m_weight->setObjectName("scriptLabImproveWeight");
  m_layout->addWidget(m_weight);
  m_table = new QTreeWidget(this);
  m_table->setObjectName("scriptLabImproveTable");
  m_table->setAccessibleName(tr("Memory and messages per script"));
  m_table->setColumnCount(7);
  m_table->setHeaderLabels({tr("Script"), tr("Kept"), tr("Values"), tr("Sent"), tr("Got"), tr("Not used"), tr("Estimate")});
  m_table->headerItem()->setToolTip(1, tr("The memory Second Life keeps for the script: 64 KB, or the limit it set with llSetMemoryLimit"));
  m_table->headerItem()->setToolTip(2, tr("The memory of the script's values at the end, a rough count. Code is not counted."));
  m_table->headerItem()->setToolTip(3, tr("Link messages and chat the script sent"));
  m_table->headerItem()->setToolTip(4, tr("Link messages and chat the script got and handled"));
  m_table->headerItem()->setToolTip(5, tr("Messages the script got and did nothing with"));
  m_table->headerItem()->setToolTip(6, tr("A rough estimate of the memory the script needs in Second Life: its code (9 bytes for each token of the source) plus its values at the end. Compare it with Kept; it can be about 30 percent off."));
  m_table->setRootIsDecorated(false);
  m_table->setUniformRowHeights(true);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_table->header()->setStretchLastSection(false);
  m_table->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_table->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_layout->addWidget(m_table);
  m_tableNote = new WrapLabel(this);
  m_tableNote->setText(tr("Second Life gives each script 64 KB. Estimate is the code plus the values, a rough estimate that can be about "
                          "30 percent off: measure in Second Life (llGetUsedMemory) before you trust it."));
  m_tableNote->setObjectName("scriptLabImproveTableNote");
  m_layout->addWidget(m_tableNote);

  m_beforeTitle = heading(tr("Before and after"), "scriptLabImproveBeforeTitle");
  m_before = new WrapLabel(this);
  m_before->setObjectName("scriptLabImproveBefore");
  m_layout->addWidget(m_before);

  // what can be better (part c), LABTWO-20
  m_betterTitle = heading(tr("What can be better"), "scriptLabImproveBetterTitle");
  m_better = new QListWidget(this);
  m_better->setObjectName("scriptLabImproveBetter");
  m_better->setAccessibleName(tr("What can be better"));
  m_better->setWordWrap(true);
  m_better->setTextElideMode(Qt::ElideNone);
  m_better->setResizeMode(QListView::Adjust);
  m_better->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_better->setToolTip(tr("Press Enter or double-click a line about a script to see it in the editor"));
  m_layout->addWidget(m_better);
  connect(m_better, &QListWidget::itemActivated, this, [this](QListWidgetItem* item)
  {
    const QString file = item->data(FileRole).toString();
    const int line = item->data(LineRole).toInt();
    if (!file.isEmpty() && line > 0) emit placeRequested(file, line);
  });

  // can this run here (part c)
  m_runTitle = heading(tr("Can this run here?"), "scriptLabImproveRunTitle");
  m_run = new QTreeWidget(this);
  m_run->setObjectName("scriptLabImproveRun");
  m_run->setAccessibleName(tr("Can this run here?"));
  m_run->setColumnCount(1);
  m_run->setHeaderHidden(true);
  m_run->setWordWrap(true);
  m_run->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_run->setToolTip(tr("Open a script's row to see what works here and what is missing"));
  m_layout->addWidget(m_run);
  m_runNote = new WrapLabel(this);
  m_runNote->setObjectName("scriptLabImproveRunNote");
  m_layout->addWidget(m_runNote);

  const int line = fontMetrics().height();
  m_table->setMinimumHeight(5 * line);
  m_better->setMinimumHeight(5 * line);
  m_run->setMinimumHeight(5 * line);

  // tab order follows the screen (charter P8)
  setTabOrder(m_import, m_record);
  setTabOrder(m_record, m_table);
  setTabOrder(m_table, m_better);
  setTabOrder(m_better, m_run);

  applyStyle();
  refresh();
}

QLabel* LabImproveView::heading(const QString& text, const char* name)
{
  QLabel* label = new QLabel(text, this);
  label->setObjectName(QString::fromLatin1(name));
  label->setTextFormat(Qt::PlainText);
  m_layout->addWidget(label);
  return label;
}

void LabImproveView::setContent(const Content& content)
{
  m_content = content;
  refresh();
}

void LabImproveView::refresh()
{
  const Content& c = m_content;
  const bool dark = isDark(this);
  // the buttons: Import always; Record only for a watched object that is not recording, else greyed with the reason (P6)
  m_record->setEnabled(c.watched && !c.recording);
  m_record->setToolTip(!c.watched ? tr("Import a setup first.")
                       : c.recording ? tr("Recording now: stop it on Player.")
                       : tr("Go to Player and record what the setup does today. Every later save replays it and compares."));
  m_record->setAccessibleDescription(m_record->toolTip());

  if (!c.watched) m_status->setText(tr("Watch a folder as one object on Live scripts, or press Import a setup. Then this page shows how heavy it is and what can be better."));
  else if (!c.ran) m_status->setText(tr("The object did not run, so nothing was measured: %1").arg(c.notRun));
  else m_status->setText(QString());
  m_status->setVisible(!m_status->text().isEmpty());
  QPalette statusInk = m_status->palette();
  statusInk.setColor(QPalette::WindowText, Theme::color(c.watched && !c.ran ? Theme::Token::Danger : Theme::Token::TextMuted, dark));
  m_status->setPalette(statusInk);

  const bool measured = c.watched && c.ran && c.weight.valid;
  for (QWidget* w : {static_cast<QWidget*>(m_weightTitle), static_cast<QWidget*>(m_weight), static_cast<QWidget*>(m_table),
                     static_cast<QWidget*>(m_tableNote), static_cast<QWidget*>(m_beforeTitle), static_cast<QWidget*>(m_before),
                     static_cast<QWidget*>(m_betterTitle), static_cast<QWidget*>(m_better)})
    w->setVisible(measured);
  const bool reported = c.watched && !c.reports.isEmpty();
  m_runTitle->setVisible(reported);
  m_run->setVisible(reported);

  // the weight
  m_weight->setText(LabWeight::summary(c.weight).join(QLatin1Char('\n')));
  m_table->clear();
  for (const LabWeight::Script& s : c.weight.scripts)
  {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_table, {s.name, LabWeight::kb(s.limit), LabWeight::kb(s.used), QString::number(s.sent),
                                                          QString::number(s.received), QString::number(s.unused),
                                                          s.estimate() < 0 ? tr("?") : LabWeight::kb(s.estimate())});
    for (int col = 1; col < 7; ++col) item->setTextAlignment(col, Qt::AlignRight | Qt::AlignVCenter);
    QString tip = tr("%1, in %2.").arg(s.name, s.prim);
    if (s.ignored > 0) tip += QLatin1Char(' ') + tr("%1 of its messages were not used by the script that got them.").arg(s.ignored);
    if (s.stopped) tip += QLatin1Char(' ') + tr("It stopped.");
    item->setToolTip(0, tip);
  }

  // before and after
  QStringList before;
  if (!c.replayed) before << tr("Record what the setup does on Player. Every later save replays it and shows the weight before and after.");
  else if (!c.compared) before << tr("This is the first replay with the weight, so there is nothing to compare yet.");
  else if (c.changes.isEmpty()) before << tr("Nothing changed in the weight since the last replay.");
  else before << c.changes;
  m_before->setText(before.join(QLatin1Char('\n')));

  // what can be better
  m_better->clear();
  const QIcon idea = LucideIcons::icon(QStringLiteral("triangle-alert"), Theme::color(Theme::Token::Warning, dark), Theme::color(Theme::Token::TextMuted, dark));
  if (c.suggestions.isEmpty())
    new QListWidgetItem(tr("Nothing found. The setup looks fine by these rules."), m_better);
  for (const LabImprove::Suggestion& s : c.suggestions)
  {
    QListWidgetItem* item = new QListWidgetItem(idea, s.text, m_better);
    item->setData(FileRole, s.file);
    item->setData(LineRole, s.line);
    if (!s.file.isEmpty() && s.line > 0) item->setToolTip(tr("%1, line %2. Press Enter to see it in the editor.").arg(s.file).arg(s.line));
  }

  // can this run here: the verdict in words and an icon, then what works and what is missing (P10)
  m_run->clear();
  for (const LabImprove::Report& r : c.reports)
  {
    Theme::Token token = Theme::Token::Success;
    QString icon = QStringLiteral("circle-check");
    if (r.verdict == LabImprove::Verdict::RunsWithNotes) { token = Theme::Token::Warning; icon = QStringLiteral("triangle-alert"); }
    else if (r.verdict != LabImprove::Verdict::Runs) { token = Theme::Token::Danger; icon = QStringLiteral("circle-x"); }
    const QColor colour = Theme::color(token, dark);
    QTreeWidgetItem* item = new QTreeWidgetItem(m_run, {tr("%1: %2").arg(r.script, LabImprove::verdictWords(r.verdict))});
    item->setIcon(0, LucideIcons::icon(icon, colour, Theme::color(Theme::Token::TextMuted, dark)));
    item->setToolTip(0, r.file);
    for (const QString& w : r.works) new QTreeWidgetItem(item, {tr("Works: %1").arg(w)});
    for (const QString& m : r.missing) new QTreeWidgetItem(item, {tr("Missing: %1").arg(m)});
  }
  m_runNote->setText(reported && c.logFull ? tr("The event log was full, so a list of events may be short. Stops and calls are counted in full.") : QString());
  m_runNote->setVisible(!m_runNote->text().isEmpty());
}

void LabImproveView::applyStyle()
{
  const bool dark = isDark(this);
  for (QLabel* l : {m_title, m_weightTitle, m_beforeTitle, m_betterTitle, m_runTitle})
  {
    QFont f = l->font();
    f.setWeight(QFont::Weight(Theme::Size::labelWeight));
    l->setFont(f);
  }
  for (QLabel* l : {m_steps, m_tableNote, m_runNote})
  {
    QPalette muted = l->palette();
    muted.setColor(QPalette::WindowText, Theme::color(Theme::Token::TextMuted, dark));
    l->setPalette(muted);
  }
  // normal buttons, as the panel's (docs/DESIGN_SYSTEM.md, "Shape"); greyed: text_muted and a half border; focus ring
  auto hex = [dark](Theme::Token token) { return Theme::color(token, dark).name(QColor::HexRgb).toUpper(); };
  const QString halfBorder = Theme::mix(Theme::color(Theme::Token::SurfaceRaised, dark), Theme::color(Theme::Token::Border, dark), 0.5)
                               .name(QColor::HexRgb).toUpper();
  const QString normal = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:disabled { color: %6; border: 1px solid %8; }"
    "QPushButton:focus { border: 2px solid %7; }")
    .arg(hex(Theme::Token::SurfaceRaised), hex(Theme::Token::Text), hex(Theme::Token::Border))
    .arg(Theme::Size::radiusControl).arg(Theme::Size::spacingM)
    .arg(hex(Theme::Token::TextMuted), hex(Theme::Token::FocusRing), halfBorder);
  m_import->setStyleSheet(normal);
  m_record->setStyleSheet(normal);
  const QColor text = Theme::color(Theme::Token::Text, dark), muted = Theme::color(Theme::Token::TextMuted, dark);
  m_import->setIcon(LucideIcons::icon(QStringLiteral("import"), text, muted));
  m_record->setIcon(LucideIcons::icon(QStringLiteral("video"), text, muted));
}

void LabImproveView::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::PaletteChange && !m_styling)
  {
    m_styling = true;
    applyStyle();
    refresh();
    m_styling = false;
  }
}

QString LabImproveView::statusLine() const { return m_status->isVisibleTo(this) ? m_status->text() : QString(); }

QStringList LabImproveView::weightLines() const
{
  return m_weight->isVisibleTo(this) && !m_weight->text().isEmpty() ? m_weight->text().split(QLatin1Char('\n')) : QStringList();
}

QStringList LabImproveView::beforeAfterLines() const
{
  return m_before->isVisibleTo(this) ? m_before->text().split(QLatin1Char('\n')) : QStringList();
}

QStringList LabImproveView::suggestionLines() const
{
  QStringList out;
  if (!m_better->isVisibleTo(this)) return out;
  for (int i = 0; i < m_better->count(); ++i) out << m_better->item(i)->text();
  return out;
}

QStringList LabImproveView::estimateRows() const
{
  QStringList out;
  if (!m_table->isVisibleTo(this)) return out;
  for (int i = 0; i < m_table->topLevelItemCount(); ++i)
    out << m_table->topLevelItem(i)->text(0) + QStringLiteral(" | ") + m_table->topLevelItem(i)->text(6);
  return out;
}

QStringList LabImproveView::tableRows() const
{
  QStringList out;
  if (!m_table->isVisibleTo(this)) return out;
  for (int i = 0; i < m_table->topLevelItemCount(); ++i)
  {
    QStringList cells;
    for (int col = 0; col < 6; ++col) cells << m_table->topLevelItem(i)->text(col);   // the estimate column is in estimateRows()
    out << cells.join(QStringLiteral(" | "));
  }
  return out;
}

QStringList LabImproveView::runRows() const
{
  QStringList out;
  if (!m_run->isVisibleTo(this)) return out;
  for (int i = 0; i < m_run->topLevelItemCount(); ++i) out << m_run->topLevelItem(i)->text(0);
  return out;
}
