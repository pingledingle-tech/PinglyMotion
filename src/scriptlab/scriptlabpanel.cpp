/*
 * PinglyMotion Script Lab - the Script Lab panel (issue 288)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDockWidget>
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

#include "objectsview.h"
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
  };
  const Tokens DarkTokens = {"#56D39A", "#FF7B72", "#C93A32", "#AEB2BA", "#3272DD", "#FFFFFF", "#767B85", "#9CC2FF"};
  const Tokens LightTokens = {"#18794E", "#B42318", "#B42318", "#535963", "#1F63C9", "#FFFFFF", "#7A808A", "#1F63C9"};

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
}

ScriptLabPanel::ScriptLabPanel(QWidget* parent) :
  QWidget(parent)
{
  setObjectName("scriptLabPanel");

  QVBoxLayout* layout = new QVBoxLayout(this);
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
  m_actions->setMinimumHeight(7 * m_actions->fontMetrics().height() + 2 * SpacingS);
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

  // --- objects of the last LSL run and the newest dialog (step 5); hidden until a script has run
  m_objectsView = new ScriptLabObjectsView(this);
  layout->addWidget(m_objectsView, 2);

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
  setTabOrder(m_log, m_objectsView->list());

  applyStyle();
  refresh();
}

void ScriptLabPanel::setState(RlvState* state)
{
  if (m_changedConnection) disconnect(m_changedConnection);
  m_state = state;
  if (state) m_changedConnection = connect(state, &RlvState::changed, this, &ScriptLabPanel::refresh);
  refresh();
}

void ScriptLabPanel::setSession(lsl::Session* session)
{
  m_objectsView->setSession(session);
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
