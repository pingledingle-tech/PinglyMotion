/*
 * PinglyMotion Script Lab - the Script Lab panel (issue 288)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonArray>
#include <QListWidget>
#include <QPainter>
#include <QScrollBar>
#include <QSet>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextEdit>
#include <QUrl>
#include <QEvent>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "labwatcher.h"
#include "paneltitlebar.h"
#include "rlvbehaviours.h"
#include "rlvstate.h"

#include "scriptlabpanel.h"

const char* const ScriptLabPanel::DockName = "scriptLabDock";
const char* const ScriptLabPanel::ViewActionName = "viewScriptLabAction";
const char* const ScriptLabPanel::DefaultObject = "panel";

namespace
{
  // Copied from docs/design_tokens.json (themes.dark / themes.light). The test
  // tests/scriptlabpanel reads that file and checks every value here.
  struct Tokens
  {
    const char* success;
    const char* danger;
    const char* dangerFill;
    const char* textMuted;
    const char* accent;
    const char* onAccent;
    const char* border;
    const char* focusRing;
    const char* warning;
    const char* surfaceRaised;
    const char* text;
    const char* textDisabled;
  };
  const Tokens DarkTokens = {"#56D39A", "#FF7B72", "#C93A32", "#AEB2BA", "#3272DD", "#FFFFFF", "#767B85", "#9CC2FF", "#F2C14E",
                             "#35373C", "#E6E7EA", "#868A93"};
  const Tokens LightTokens = {"#18794E", "#B42318", "#B42318", "#535963", "#1F63C9", "#FFFFFF", "#7A808A", "#1F63C9", "#8A5A00",
                              "#E9EBEF", "#1C1E22", "#7A808A"};

  const Tokens& tokens(bool dark) { return dark ? DarkTokens : LightTokens; }

  // design_tokens.json: spacing.s, spacing.m, spacing.l, control_height.default, radius.control
  const int SpacingS = 8;
  const int SpacingM = 12;
  const int SpacingL = 16;
  const int ControlHeight = 32;
  const int RadiusControl = 6;

  const int LogLines = 500;

  // a section name: the "label" type token (body size, semibold)
  QLabel* sectionLabel(const QString& text, QWidget* parent)
  {
    QLabel* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    return label;
  }

  // A read-only script view with line numbers in a strip on the left. Lines
  // with an error have their number in the danger colour and bold, and the
  // line is underlined (the error list says the same in words, P10).
  class ScriptView : public QPlainTextEdit
  {
    public:
      explicit ScriptView(QWidget* parent) :
        QPlainTextEdit(parent),
        m_numbers(new Numbers(this))
      {
        setReadOnly(true);
        setLineWrapMode(QPlainTextEdit::NoWrap);
        setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        connect(this, &QPlainTextEdit::blockCountChanged, this, [this](int) { updateMargin(); });
        connect(this, &QPlainTextEdit::updateRequest, this, [this](const QRect& rect, int dy)
        {
          if (dy) m_numbers->scroll(0, dy);
          else m_numbers->update(0, rect.y(), m_numbers->width(), rect.height());
        });
        updateMargin();
      }

      void setMarks(const QSet<int>& errorLines, const QColor& danger, const QColor& muted)
      {
        m_errorLines = errorLines;
        m_danger = danger;
        m_muted = muted;
        m_numbers->update();
      }

      int numberWidth() const
      {
        const int digits = QString::number(qMax(1, blockCount())).size();
        return SpacingS + fontMetrics().horizontalAdvance(QLatin1Char('9')) * qMax(2, digits) + SpacingS;
      }

      void paintNumbers(QPaintEvent* event)
      {
        QPainter painter(m_numbers);
        painter.fillRect(event->rect(), palette().color(QPalette::Window));
        QTextBlock block = firstVisibleBlock();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        while (block.isValid() && top <= event->rect().bottom())
        {
          const int bottom = top + qRound(blockBoundingRect(block).height());
          if (block.isVisible() && bottom >= event->rect().top())
          {
            const int line = block.blockNumber() + 1;
            const bool error = m_errorLines.contains(line);
            QFont font = this->font();
            font.setWeight(error ? QFont::Bold : QFont::Normal);
            painter.setFont(font);
            painter.setPen(error ? m_danger : m_muted);
            painter.drawText(0, top, m_numbers->width() - SpacingS, fontMetrics().height(), Qt::AlignRight, QString::number(line));
          }
          block = block.next();
          top = bottom;
        }
      }

    protected:
      void resizeEvent(QResizeEvent* event) override
      {
        QPlainTextEdit::resizeEvent(event);
        const QRect inner = contentsRect();
        m_numbers->setGeometry(QRect(inner.left(), inner.top(), numberWidth(), inner.height()));
      }

    private:
      class Numbers : public QWidget
      {
        public:
          explicit Numbers(ScriptView* view) : QWidget(view), m_view(view) {}
          QSize sizeHint() const override { return QSize(m_view->numberWidth(), 0); }
        protected:
          void paintEvent(QPaintEvent* event) override { m_view->paintNumbers(event); }
        private:
          ScriptView* m_view;
      };

      void updateMargin() { setViewportMargins(numberWidth(), 0, 0, 0); }

      Numbers* m_numbers;
      QSet<int> m_errorLines;
      QColor m_danger;
      QColor m_muted;
  };

  QString fieldText(const QJsonValue& value)
  {
    if (value.isString()) return value.toString();
    if (value.isDouble()) return QString::number(value.toDouble(), 'g', 10);
    if (value.isBool()) return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isArray())
    {
      QStringList parts;
      for (const QJsonValue& v : value.toArray()) parts << fieldText(v);
      return parts.join(QStringLiteral(", "));
    }
    return QString();
  }
}

ScriptLabPanel::ScriptLabPanel(QWidget* parent) :
  QWidget(parent)
{
  setObjectName("scriptLabPanel");

  QVBoxLayout* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(0);
  m_tabs = new QTabWidget(this);
  m_tabs->setObjectName("scriptLabTabs");
  m_tabs->setDocumentMode(true);
  outer->addWidget(m_tabs);

  // --- page 1: RLV lines typed by hand (issue 288)
  QWidget* commandsPage = new QWidget(m_tabs);
  commandsPage->setObjectName("scriptLabCommandsPage");
  QVBoxLayout* layout = new QVBoxLayout(commandsPage);
  layout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  layout->setSpacing(SpacingS);

  // --- object and command line
  QGridLayout* input = new QGridLayout;
  input->setHorizontalSpacing(SpacingS);
  input->setVerticalSpacing(SpacingS);

  QLabel* objectLabel = new QLabel(tr("Object"), this);
  m_object = new QLineEdit(QString::fromLatin1(DefaultObject), this);
  m_object->setObjectName("scriptLabObject");
  m_object->setMaxLength(RlvState::MAX_OBJECT_ID_LENGTH);
  m_object->setMinimumHeight(ControlHeight);
  m_object->setToolTip(tr("The object that sends the line, as a script in it would with llOwnerSay. "
                          "Each object holds its own restrictions."));
  objectLabel->setBuddy(m_object);

  QLabel* commandLabel = new QLabel(tr("Command"), this);
  m_command = new QLineEdit(this);
  m_command->setObjectName("scriptLabCommand");
  m_command->setPlaceholderText(QStringLiteral("@detach=n,sendim=n"));
  m_command->setMaxLength(1024);
  m_command->setMinimumHeight(ControlHeight);
  m_command->setClearButtonEnabled(true);
  m_command->setToolTip(tr("An RLV line as a script sends it with llOwnerSay, for example @detach=n,sendim=n or @clear. "
                           "Press Enter to run it."));
  commandLabel->setBuddy(m_command);

  m_run = new QPushButton(tr("Run"), this);
  m_run->setObjectName("scriptLabRun");
  m_run->setMinimumHeight(ControlHeight);
  m_run->setToolTip(tr("Run the line (Enter)"));

  m_reset = new QPushButton(tr("Reset"), this);
  m_reset->setObjectName("scriptLabReset");
  m_reset->setMinimumHeight(ControlHeight);
  m_reset->setToolTip(tr("Remove every restriction of every object, as if all of them were taken off"));

  // the buttons get a row of their own, so the command field keeps its
  // width when the panel is docked narrow beside the others
  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->setSpacing(SpacingS);
  buttons->addWidget(m_run);
  buttons->addStretch(1);
  buttons->addWidget(m_reset);

  input->addWidget(objectLabel, 0, 0);
  input->addWidget(m_object, 0, 1);
  input->addWidget(commandLabel, 1, 0);
  input->addWidget(m_command, 1, 1);
  input->addLayout(buttons, 2, 1);
  input->setColumnStretch(1, 1);
  layout->addLayout(input);

  // --- restrictions per object
  layout->addSpacing(SpacingL - SpacingS);
  layout->addWidget(sectionLabel(tr("Restrictions"), this));
  m_empty = new QLabel(tr("No restrictions. Type a line above, for example @detach=n, and press Enter."), this);
  m_empty->setObjectName("scriptLabEmpty");
  m_empty->setWordWrap(true);
  layout->addWidget(m_empty);
  m_restrictions = new QTreeWidget(this);
  m_restrictions->setObjectName("scriptLabRestrictions");
  m_restrictions->setHeaderLabels({tr("Behaviour"), tr("Option"), tr("Object")});
  m_restrictions->setRootIsDecorated(false);
  m_restrictions->setUniformRowHeights(true);
  m_restrictions->setSelectionMode(QAbstractItemView::NoSelection);
  m_restrictions->header()->setSectionResizeMode(QHeaderView::Stretch);
  m_restrictions->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_restrictions->setToolTip(tr("Every restriction that is held, and which object holds it"));
  m_restrictions->setMinimumHeight(4 * m_restrictions->fontMetrics().height() + 2 * SpacingS);
  layout->addWidget(m_restrictions, 2);

  // --- actions, allowed or blocked
  layout->addSpacing(SpacingL - SpacingS);
  layout->addWidget(sectionLabel(tr("Actions"), this));
  m_actions = new QTreeWidget(this);
  m_actions->setObjectName("scriptLabActions");
  m_actions->setHeaderLabels({tr("Action"), tr("State")});
  m_actions->setRootIsDecorated(false);
  m_actions->setUniformRowHeights(true);
  m_actions->setSelectionMode(QAbstractItemView::NoSelection);
  m_actions->header()->setStretchLastSection(false);
  m_actions->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_actions->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_actions->setToolTip(tr("What the avatar may do now: Allowed or Blocked"));
  for (const Rlv::Action& action : Rlv::actions())
  {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_actions);
    item->setText(0, action.label);
    item->setData(0, Qt::UserRole, action.id);
  }
  m_actions->setMinimumHeight(6 * m_actions->fontMetrics().height() + 2 * SpacingS);
  layout->addWidget(m_actions, 3);

  // --- replies and messages
  layout->addSpacing(SpacingL - SpacingS);
  layout->addWidget(sectionLabel(tr("Replies and messages"), this));
  m_log = new QPlainTextEdit(this);
  m_log->setObjectName("scriptLabLog");
  m_log->setReadOnly(true);
  m_log->setMaximumBlockCount(LogLines);
  m_log->setToolTip(tr("The lines you ran, what happened, and the replies a script would hear on its channel"));
  m_log->setMinimumHeight(4 * m_log->fontMetrics().height() + 2 * SpacingS);
  layout->addWidget(m_log, 2);

  m_tabs->addTab(commandsPage, tr("Commands"));
  m_tabs->setTabToolTip(0, tr("Type RLV lines by hand and see what the avatar may still do"));
  buildLivePage();

  connect(m_command, &QLineEdit::returnPressed, this, &ScriptLabPanel::runCommandField);
  connect(m_run, &QPushButton::clicked, this, &ScriptLabPanel::runCommandField);
  connect(m_reset, &QPushButton::clicked, this, &ScriptLabPanel::resetAll);

  // tab order follows the screen (charter P8)
  setTabOrder(m_object, m_command);
  setTabOrder(m_command, m_run);
  setTabOrder(m_run, m_reset);
  setTabOrder(m_reset, m_restrictions);
  setTabOrder(m_restrictions, m_actions);
  setTabOrder(m_actions, m_log);

  applyStyle();
  refresh();
  refreshLive();
}

void ScriptLabPanel::setState(RlvState* state)
{
  if (m_changedConnection) disconnect(m_changedConnection);
  m_state = state;
  if (state) m_changedConnection = connect(state, &RlvState::changed, this, &ScriptLabPanel::refresh);
  refresh();
}

bool ScriptLabPanel::isDark(const QPalette& palette)
{
  return palette.color(QPalette::Window).lightness() < 128;
}

QColor ScriptLabPanel::successColour(bool dark) { return QColor(QString::fromLatin1(tokens(dark).success)); }
QColor ScriptLabPanel::dangerColour(bool dark) { return QColor(QString::fromLatin1(tokens(dark).danger)); }
QColor ScriptLabPanel::mutedColour(bool dark) { return QColor(QString::fromLatin1(tokens(dark).textMuted)); }

void ScriptLabPanel::applyStyle()
{
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  // main button: accent fill; Reset removes restrictions, so it is a danger
  // button with its word on it (docs/DESIGN_SYSTEM.md, "Shape")
  const QString button = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %1; border-radius: %3px; padding: 0 %4px; }"
    "QPushButton:focus { border: 2px solid %5; }");
  m_run->setStyleSheet(button.arg(QLatin1String(t.accent), QLatin1String(t.onAccent)).arg(RadiusControl).arg(SpacingM)
                       .arg(QLatin1String(t.focusRing)));
  m_reset->setStyleSheet(button.arg(QLatin1String(t.dangerFill), QLatin1String(t.onAccent)).arg(RadiusControl).arg(SpacingM)
                         .arg(QLatin1String(t.focusRing)));
  if (!m_watch) return;
  // Live page: Watch folder is the main button there; Stop watching and Open
  // in editor are normal buttons (surface_raised fill, border, text), greyed
  // out with text_disabled (DESIGN_SYSTEM.md, "Shape" and "States")
  m_watch->setStyleSheet(button.arg(QLatin1String(t.accent), QLatin1String(t.onAccent)).arg(RadiusControl).arg(SpacingM)
                         .arg(QLatin1String(t.focusRing)));
  const QString normal = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:disabled { color: %6; }"
    "QPushButton:focus { border: 2px solid %7; }")
    .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border))
    .arg(RadiusControl).arg(SpacingM)
    .arg(QLatin1String(t.textDisabled), QLatin1String(t.focusRing));
  m_unwatch->setStyleSheet(normal);
  m_openEditor->setStyleSheet(normal);
  QPalette hint = m_empty->palette();
  hint.setColor(QPalette::WindowText, QColor(QString::fromLatin1(t.textMuted)));
  m_empty->setPalette(hint);
}

void ScriptLabPanel::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  // the theme was switched: new token colours
  if (event->type() == QEvent::PaletteChange)
  {
    applyStyle();
    refresh();
    refreshLive();
  }
}

void ScriptLabPanel::refresh()
{
  const bool dark = isDark(palette());

  m_restrictions->clear();
  QVector<RlvState::Restriction> held;
  if (m_state) held = m_state->restrictions();
  for (const RlvState::Restriction& r : held)
  {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_restrictions);
    item->setText(0, r.behaviour);
    item->setText(1, r.option);
    item->setText(2, r.object);
  }
  m_empty->setVisible(held.isEmpty());

  QMap<QString, bool> allowed;
  if (m_state) allowed = m_state->actions();
  for (int i = 0; i < m_actions->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* item = m_actions->topLevelItem(i);
    const bool ok = allowed.value(item->data(0, Qt::UserRole).toString(), true);
    item->setText(1, ok ? tr("Allowed") : tr("Blocked"));
    item->setData(1, Qt::UserRole, ok);
    item->setForeground(1, ok ? successColour(dark) : dangerColour(dark));
    QFont font = item->font(1);
    font.setWeight(ok ? QFont::Normal : QFont::DemiBold);   // a second signal besides the word
    item->setFont(1, font);
  }
}

void ScriptLabPanel::appendLog(const QString& line)
{
  m_log->appendPlainText(line);
}

void ScriptLabPanel::runCommandField()
{
  const QString text = m_command->text().trimmed();
  if (text.isEmpty())
  {
    appendLog(tr("Type a line first, for example @detach=n."));
    return;
  }
  if (runLine(text)) m_command->clear();
}

bool ScriptLabPanel::runLine(const QString& text)
{
  if (!m_state)
  {
    appendLog(tr("Script Lab is not ready yet."));
    return false;
  }
  QString object = m_object->text().trimmed();
  if (object.isEmpty()) object = QString::fromLatin1(DefaultObject);

  appendLog(QStringLiteral("%1: %2").arg(object, text));
  const RlvState::ApplyResult result = m_state->apply(object, text);
  if (!result.ok())
  {
    appendLog(tr("  Refused: %1").arg(result.error));
    return false;
  }
  for (const RlvState::Outcome& outcome : result.outcomes)
  {
    QString line = QStringLiteral("  %1: %2").arg(outcome.command.text, Rlv::resultName(outcome.result));
    if (!outcome.reason.isEmpty()) line += QStringLiteral(" (%1)").arg(outcome.reason);
    appendLog(line);
  }
  for (const RlvState::Reply& reply : result.replies)
    appendLog(tr("  Reply on channel %1: %2").arg(reply.channel).arg(reply.text));
  return true;
}

void ScriptLabPanel::resetAll()
{
  if (!m_state) return;
  const int removed = static_cast<int>(m_state->restrictions().size());
  m_state->reset();
  appendLog(tr("Reset: %n restriction(s) removed.", nullptr, removed));
  // reset() only signals when something changed; show the empty state anyway
  refresh();
}

QDockWidget* ScriptLabPanel::makeDock(QMainWindow* window, ScriptLabPanel* panel)
{
  QDockWidget* dock = new QDockWidget(tr("Script Lab"), window);
  dock->setObjectName(QString::fromLatin1(DockName));
  dock->setWidget(panel);
  dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dock->setTitleBarWidget(new PanelTitleBar(dock));

  QAction* action = dock->toggleViewAction();
  action->setObjectName(QString::fromLatin1(ViewActionName));
  action->setText(tr("Script Lab"));
  action->setToolTip(tr("Test RLV commands: type a line such as @detach=n and see what the avatar may still do"));
  action->setStatusTip(action->toolTip());

  // hidden until asked for: someone who only animates never sees it
  window->addDockWidget(Qt::RightDockWidgetArea, dock);
  dock->hide();
  return dock;
}

// --- Live (issue 292) -------------------------------------------------------

void ScriptLabPanel::buildLivePage()
{
  QWidget* page = new QWidget(m_tabs);
  page->setObjectName("scriptLabLivePage");
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  layout->setSpacing(SpacingS);

  // folder and its buttons
  QLabel* folderLabel = new QLabel(tr("Folder"), page);
  m_liveFolder = new QLineEdit(page);
  m_liveFolder->setObjectName("scriptLabLiveFolder");
  m_liveFolder->setReadOnly(true);
  m_liveFolder->setMinimumHeight(ControlHeight);
  m_liveFolder->setPlaceholderText(tr("No folder watched"));
  m_liveFolder->setToolTip(tr("The folder with your .lsl scripts. It is only read, never changed."));
  folderLabel->setBuddy(m_liveFolder);

  m_watch = new QPushButton(tr("Watch folder..."), page);
  m_watch->setObjectName("scriptLabWatch");
  m_watch->setMinimumHeight(ControlHeight);
  m_watch->setToolTip(tr("Choose the folder with your .lsl scripts. Each time you save one, it is checked and run again here."));

  m_unwatch = new QPushButton(tr("Stop watching"), page);
  m_unwatch->setObjectName("scriptLabUnwatch");
  m_unwatch->setMinimumHeight(ControlHeight);

  QGridLayout* folderGrid = new QGridLayout;
  folderGrid->setHorizontalSpacing(SpacingS);
  folderGrid->setVerticalSpacing(SpacingS);
  folderGrid->addWidget(folderLabel, 0, 0);
  folderGrid->addWidget(m_liveFolder, 0, 1);
  QHBoxLayout* folderButtons = new QHBoxLayout;
  folderButtons->setSpacing(SpacingS);
  folderButtons->addWidget(m_watch);
  folderButtons->addStretch(1);
  folderButtons->addWidget(m_unwatch);
  folderGrid->addLayout(folderButtons, 1, 1);
  folderGrid->setColumnStretch(1, 1);
  layout->addLayout(folderGrid);

  // what happened last, in words (colour only backs it up)
  m_liveStatus = new QLabel(page);
  m_liveStatus->setObjectName("scriptLabLiveStatus");
  m_liveStatus->setWordWrap(true);
  m_liveStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(m_liveStatus);

  // the script, read only, with line numbers
  layout->addSpacing(SpacingL - SpacingS);
  QHBoxLayout* scriptRow = new QHBoxLayout;
  scriptRow->setSpacing(SpacingS);
  m_liveScript = sectionLabel(tr("Script"), page);
  m_liveScript->setObjectName("scriptLabLiveScript");
  m_openEditor = new QPushButton(tr("Open in editor"), page);
  m_openEditor->setObjectName("scriptLabOpenEditor");
  m_openEditor->setMinimumHeight(ControlHeight);
  scriptRow->addWidget(m_liveScript, 1);
  scriptRow->addWidget(m_openEditor);
  layout->addLayout(scriptRow);
  m_scriptView = new ScriptView(page);
  m_scriptView->setObjectName("scriptLabScriptView");
  m_scriptView->setToolTip(tr("The script as it is on disk, read only. Change it in your own editor and save."));
  m_scriptView->setMinimumHeight(4 * m_scriptView->fontMetrics().height() + 2 * SpacingS);
  layout->addWidget(m_scriptView, 3);

  // errors and warnings with line and column
  layout->addSpacing(SpacingL - SpacingS);
  layout->addWidget(sectionLabel(tr("Errors and warnings"), page));
  m_problems = new QListWidget(page);
  m_problems->setObjectName("scriptLabProblems");
  m_problems->setToolTip(tr("Press Enter or double-click a line to show it in the script"));
  m_problems->setMinimumHeight(2 * m_problems->fontMetrics().height() + 2 * SpacingS);
  m_problems->setWordWrap(true);
  layout->addWidget(m_problems, 1);

  // what the run did
  layout->addSpacing(SpacingL - SpacingS);
  layout->addWidget(sectionLabel(tr("Event log"), page));
  m_runLog = new QPlainTextEdit(page);
  m_runLog->setObjectName("scriptLabRunLog");
  m_runLog->setReadOnly(true);
  m_runLog->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  m_runLog->setToolTip(tr("What the script did in the last run: what it said, its listens, timers and RLV commands. Times are simulated seconds."));
  m_runLog->setMinimumHeight(3 * m_runLog->fontMetrics().height() + 2 * SpacingS);
  layout->addWidget(m_runLog, 2);

  m_tabs->addTab(page, tr("Live"));
  m_tabs->setTabToolTip(1, tr("Watch a folder of .lsl scripts: each saved change is checked and run again here"));

  connect(m_watch, &QPushButton::clicked, this, &ScriptLabPanel::chooseFolder);
  connect(m_unwatch, &QPushButton::clicked, this, &ScriptLabPanel::stopWatching);
  connect(m_openEditor, &QPushButton::clicked, this, &ScriptLabPanel::openInEditor);
  connect(m_problems, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) { showProblem(m_problems->row(item)); });

  setTabOrder(m_liveFolder, m_watch);
  setTabOrder(m_watch, m_unwatch);
  setTabOrder(m_unwatch, m_openEditor);
  setTabOrder(m_openEditor, m_scriptView);
  setTabOrder(m_scriptView, m_problems);
  setTabOrder(m_problems, m_runLog);
}

void ScriptLabPanel::setWatcher(LabWatcher* watcher)
{
  for (const QMetaObject::Connection& c : m_watcherConnections) disconnect(c);
  m_watcherConnections.clear();
  m_watcher = watcher;
  if (watcher)
  {
    m_watcherConnections << connect(watcher, &LabWatcher::resultChanged, this, &ScriptLabPanel::refreshLive);
    m_watcherConnections << connect(watcher, &LabWatcher::watchChanged, this, &ScriptLabPanel::refreshLive);
  }
  m_refusal.clear();
  refreshLive();
}

bool ScriptLabPanel::watchFolder(const QString& folder)
{
  if (!m_watcher) return false;
  const LabWatcher::Result result = m_watcher->watch(QDir::fromNativeSeparators(folder));
  if (!result.ok())
  {
    m_refusal = tr("Error: %1").arg(result.error);
    refreshLive();
    return false;
  }
  m_refusal.clear();
  refreshLive();
  return true;
}

void ScriptLabPanel::stopWatching()
{
  if (m_watcher) m_watcher->unwatch();
  m_refusal.clear();
  refreshLive();
}

void ScriptLabPanel::chooseFolder()
{
  const QString start = (m_watcher && m_watcher->watching()) ? m_watcher->folder() : QDir::homePath();
  const QString folder = QFileDialog::getExistingDirectory(this, tr("Choose the folder with your scripts"), start);
  if (!folder.isEmpty()) watchFolder(folder);
}

void ScriptLabPanel::openInEditor()
{
  if (!m_watcher) return;
  const QString path = m_watcher->absolutePath(m_shownFile);
  if (path.isEmpty()) return;
  // the system's own program for .lsl files; nothing is written here
  if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
    setLiveStatus(tr("Error: no program on this computer opens .lsl files. Open %1 in your editor yourself, "
                     "or choose a program for .lsl files in the system settings.").arg(QDir::toNativeSeparators(path)), 2);
}

void ScriptLabPanel::showProblem(int index)
{
  if (index < 0 || index >= m_shownProblems.size()) return;
  const QJsonObject problem = m_shownProblems.at(index);
  const QTextBlock block = m_scriptView->document()->findBlockByNumber(problem.value("line").toInt() - 1);
  if (!block.isValid()) return;
  QTextCursor cursor(block);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qBound(0, problem.value("column").toInt() - 1, block.length() - 1));
  m_scriptView->setTextCursor(cursor);
  m_scriptView->centerCursor();
  m_scriptView->setFocus(Qt::OtherFocusReason);
}

void ScriptLabPanel::setLiveStatus(const QString& text, int tone)
{
  m_liveStatus->setText(text);
  m_liveTone = tone;
  const bool dark = isDark(palette());
  QPalette p = m_liveStatus->palette();
  p.setColor(QPalette::WindowText, tone == 1 ? successColour(dark) : tone == 2 ? dangerColour(dark) : mutedColour(dark));
  m_liveStatus->setPalette(p);
}

QString ScriptLabPanel::logLine(const QJsonObject& entry)
{
  const QString type = entry.value("type").toString();
  const QString time = QString::number(entry.value("t").toDouble(), 'f', 3) + QStringLiteral(" s  ");
  if (type == QLatin1String("owner_say")) return time + QStringLiteral("owner_say: ") + entry.value("text").toString();
  if (type == QLatin1String("say") || type == QLatin1String("whisper") || type == QLatin1String("shout") || type == QLatin1String("region_say"))
    return time + QStringLiteral("%1 on channel %2: %3").arg(type, fieldText(entry.value("channel")), entry.value("text").toString());
  if (type == QLatin1String("state")) return time + QStringLiteral("state: ") + entry.value("name").toString();
  if (type == QLatin1String("event"))
    return time + QStringLiteral("event: %1(%2)").arg(entry.value("name").toString(), fieldText(entry.value("args")));
  if (type == QLatin1String("error") || type == QLatin1String("limit") || type == QLatin1String("unsupported"))
    return time + QStringLiteral("%1 at line %2, column %3: %4").arg(type, fieldText(entry.value("line")),
                                                                      fieldText(entry.value("column")), entry.value("message").toString());
  if (type == QLatin1String("rlv_command"))
  {
    QString line = time + QStringLiteral("rlv_command: ") + entry.value("text").toString();
    if (entry.contains("error")) line += QStringLiteral(" (refused: %1)").arg(entry.value("error").toString());
    for (const QJsonValue& reply : entry.value("replies").toArray())
      line += QStringLiteral("; reply on channel %1: %2").arg(fieldText(reply.toObject().value("channel")), reply.toObject().value("text").toString());
    return line;
  }
  // every other entry: its fields in a fixed order
  QStringList parts;
  const QStringList keys = entry.keys();
  for (const QString& key : keys)
    if (key != QLatin1String("t") && key != QLatin1String("type")) parts << key + QStringLiteral("=") + fieldText(entry.value(key));
  return time + type + (parts.isEmpty() ? QString() : QStringLiteral(": ") + parts.join(QStringLiteral(", ")));
}

void ScriptLabPanel::refreshLive()
{
  if (!m_liveStatus) return;
  const bool dark = isDark(palette());
  const bool watching = m_watcher && m_watcher->watching();

  m_liveFolder->setText(watching ? QDir::toNativeSeparators(m_watcher->folder()) : QString());
  // a folder watch that just started (here or through lab_watch) brings the Live page to the front
  if (watching && !m_wasWatching) m_tabs->setCurrentIndex(1);
  m_wasWatching = watching;
  m_unwatch->setEnabled(watching);
  m_unwatch->setToolTip(watching ? tr("Stop running the scripts when they change")
                                 : tr("Not watching a folder. Choose one with Watch folder first."));

  const QJsonObject last = watching ? m_watcher->lastResult() : QJsonObject();
  const QString file = last.value("file").toString();

  // the script view: read the file again only when another file is shown
  // or the file was run again
  ScriptView* view = static_cast<ScriptView*>(m_scriptView);
  if (last.isEmpty())
  {
    m_shownFile.clear();
    view->clear();
  }
  else
  {
    m_shownFile = file;
    QString text;
    QFile in(m_watcher->absolutePath(file));
    if (QFileInfo(in.fileName()).size() <= LabWatcher::MAX_FILE_BYTES && in.open(QIODevice::ReadOnly))
      text = QString::fromUtf8(in.read(LabWatcher::MAX_FILE_BYTES));
    if (view->toPlainText() != text) view->setPlainText(text);
  }
  m_liveScript->setText(file.isEmpty() ? tr("Script") : tr("Script: %1").arg(file));
  m_openEditor->setEnabled(!m_shownFile.isEmpty());
  m_openEditor->setToolTip(m_shownFile.isEmpty() ? tr("No script yet. Watch a folder and save a .lsl file in it first.")
                                                 : tr("Open the script in the program your computer uses for .lsl files"));

  // errors and warnings, with marks in the script
  m_problems->clear();
  m_shownProblems.clear();
  QSet<int> errorLines;
  QList<QTextEdit::ExtraSelection> marks;
  const auto addProblems = [&](const QJsonArray& list, bool error)
  {
    for (const QJsonValue& value : list)
    {
      const QJsonObject p = value.toObject();
      const int line = p.value("line").toInt();
      const int column = p.value("column").toInt();
      QListWidgetItem* item = new QListWidgetItem(
        (error ? tr("Error, line %1, column %2: %3") : tr("Warning, line %1, column %2: %3")).arg(line).arg(column).arg(p.value("message").toString()),
        m_problems);
      item->setForeground(error ? dangerColour(dark) : QColor(QString::fromLatin1(tokens(dark).warning)));
      m_shownProblems << p;
      if (error) errorLines.insert(line);
      const QTextBlock block = view->document()->findBlockByNumber(line - 1);
      if (block.isValid())
      {
        QTextEdit::ExtraSelection mark;
        mark.cursor = QTextCursor(block);
        mark.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        mark.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        mark.format.setUnderlineColor(error ? dangerColour(dark) : QColor(QString::fromLatin1(tokens(dark).warning)));
        mark.format.setToolTip(item->text());
        marks << mark;
      }
    }
  };
  addProblems(last.value("errors").toArray(), true);
  // a run that a runtime error or a limit stopped also marks its line
  const QJsonObject run = last.value("result").toObject();
  if (run.value("stopped").toBool())
  {
    const QJsonArray log = run.value("log").toArray();
    for (const QJsonValue& value : log)
    {
      const QJsonObject entry = value.toObject();
      const QString type = entry.value("type").toString();
      if (type == QLatin1String("error") || type == QLatin1String("limit") || type == QLatin1String("unsupported"))
        addProblems(QJsonArray{QJsonObject{{"line", entry.value("line")}, {"column", entry.value("column")},
                                           {"message", entry.value("message")}}}, true);
    }
  }
  addProblems(last.value("warnings").toArray(), false);
  view->setExtraSelections(marks);
  view->setMarks(errorLines, dangerColour(dark), mutedColour(dark));

  // the event log of the run
  QStringList lines;
  for (const QJsonValue& value : run.value("log").toArray()) lines << logLine(value.toObject());
  if (run.value("log_truncated").toBool()) lines << tr("(the log was cut here: the run wrote more than it keeps)");
  const QString logText = lines.join(QLatin1Char('\n'));
  if (m_runLog->toPlainText() != logText) m_runLog->setPlainText(logText);

  // the status line, a word first
  if (!m_refusal.isEmpty()) { setLiveStatus(m_refusal, 2); return; }
  if (!watching)
  {
    setLiveStatus(tr("No folder is watched. Choose the folder with your .lsl scripts: each time you save one, it is checked and run again here."), 0);
    return;
  }
  if (last.isEmpty())
  {
    setLiveStatus(m_watcher->scripts().isEmpty()
                    ? tr("Watching. No .lsl file in this folder yet: save one here and it runs at once.")
                    : tr("Watching %n script(s). Save one to run it.", nullptr, int(m_watcher->scripts().size())), 0);
    return;
  }
  const int runNumber = last.value("run").toInt();
  if (last.contains("read_error"))
  {
    setLiveStatus(tr("Error: %1 was not run (run %2). %3").arg(file).arg(runNumber).arg(last.value("read_error").toString()), 2);
    return;
  }
  const int errors = last.value("errors").toArray().size();
  if (errors > 0)
  {
    setLiveStatus(tr("Error: %1 has %n error(s) and was not run (run %2). Fix the line shown below and save.", nullptr, errors)
                    .arg(file).arg(runNumber), 2);
    return;
  }
  QString rlvText;
  const QJsonArray held = run.value("rlv").toObject().value("state").toObject().value("restrictions").toArray();
  // only for a script that sent RLV lines (or left restrictions held)
  if (run.value("rlv").toObject().value("lines").toInt() > 0 || !held.isEmpty())
  {
    QStringList names;
    for (const QJsonValue& r : held)
    {
      const QJsonObject o = r.toObject();
      names << (o.value("option").toString().isEmpty() ? o.value("behaviour").toString()
                                                       : o.value("behaviour").toString() + QLatin1Char(':') + o.value("option").toString());
    }
    rlvText = names.isEmpty() ? tr(" No RLV restrictions at the end.") : tr(" RLV restrictions at the end: %1.").arg(names.join(QStringLiteral(", ")));
  }
  if (run.value("stopped").toBool())
  {
    setLiveStatus(tr("Error: %1 ran and was stopped (run %2): %3").arg(file).arg(runNumber).arg(run.value("stop_reason").toString()) + rlvText, 2);
    return;
  }
  setLiveStatus(tr("OK: %1 ran without errors (run %2), %n log line(s).", nullptr, int(run.value("log").toArray().size()))
                  .arg(file).arg(runNumber) + rlvText, 1);
}
