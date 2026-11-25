/*
 * PinglyMotion Script Lab - the Script Lab panel: effects, camera, sky and view modes in the 3D view, and choosing a restriction from the list
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QComboBox>
#include <QFontDatabase>
#include <QSet>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "effectspreview.h"
#include "labbeginner.h"
#include "labeditor.h"
#include "labtree.h"
#include "channelmapview.h"
#include "labchains.h"
#include "labwatcher.h"
#include "lsl/control.h"
#include "lsl/session.h"
#include "objectsview.h"
#include "paneltitlebar.h"
#include "rlvbehaviours.h"
#include "textsize.h"
#include "rlvstate.h"
#include "scriptlabsection.h"
#include "spherecontrols.h"
#include "lsl/lint.h"
#include "lucideicons.h"
#include "labdialogbox.h"
#include "labplayer.h"
#include "labrestrictionchoice.h"
#include "theme.h"
#include "usersettings.h"
#include "scriptlabpanelui.h"

#include "scriptlabpanel.h"

using namespace LabUi;

// --- Script Lab step 7 (issue 293): effects in the 3D view

void ScriptLabPanel::setEffects(EffectsPreview* preview)
{
  for (const QMetaObject::Connection& c : m_effectsConnections) disconnect(c);
  m_effectsConnections.clear();
  m_effects = preview;
  if (preview)
  {
    preview->setEnabled(m_effectsBox && m_effectsBox->isChecked());
    m_effectsConnections << connect(preview, &EffectsPreview::changed, this, &ScriptLabPanel::refreshEffects);
  }
  connectEffectsToWatcher();
  refreshEffects();
}

void ScriptLabPanel::connectEffectsToWatcher()
{
  for (const QMetaObject::Connection& c : m_effectsWatcherConnections) disconnect(c);
  m_effectsWatcherConnections.clear();
  // every new Live run (and the end of a watch), and every lsl_run and lsl_event, goes to the
  // preview, which does nothing while it is off; the newest of the two is shown when it is ticked
  if (!m_effects) return;
  if (m_watcher)
  {
    auto show = [this]() { m_effectsFromSession = false; if (m_effects) m_effects->showLatest(m_watcher); };
    m_effectsWatcherConnections << connect(m_watcher, &LabWatcher::resultChanged, this, show);
    m_effectsWatcherConnections << connect(m_watcher, &LabWatcher::watchChanged, this, show);
  }
  if (m_session)
  {
    auto show = [this]() { m_effectsFromSession = true; if (m_effects) m_effects->showSession(m_session); };
    m_effectsWatcherConnections << connect(m_session, &lsl::Session::changed, this, show);
  }
}

bool ScriptLabPanel::primPropsShown() const
{
  return m_effectsBox && m_effectsBox->isChecked() && m_watcher && m_watcher->watching() && m_watcher->linkset();
}

void ScriptLabPanel::updatePrimProps()
{
  // props from prims (job A1): every prim of the watched object gets a prop of its name
  const bool show = primPropsShown();
  const QJsonObject folder = show ? shownFolder() : QJsonObject();
  bool added = false;
  if (show && m_hooks.addProp && m_hooks.hasProp)
  {
    // the props of the folder's example.json (an example project), read once per folder
    if (m_primExampleFolder != m_watcher->folder())
    {
      m_primExampleFolder = m_watcher->folder();
      m_primExample = LabBeginner::readExample(m_primExampleFolder).props;
    }
    for (const LabExample::Prop& prop : LabPrimProps::propsFor(folder, m_primExample))
      if (!m_hooks.hasProp(prop.name)) added = m_hooks.addProp(prop) || added;
  }
  // props that move: a prop stands where the script put its prim; it jumps there, no animation
  m_primMoves = show ? LabPrimProps::movesFromFolder(folder) : QList<LabPrimProps::Move>();
  if (m_hooks.placeProp && m_hooks.propPlace)
  {
    QSet<QString> moved;
    for (const LabPrimProps::Move& move : m_primMoves)
    {
      if (!m_primBase.contains(move.prim))
      {
        LabExample::Prop base;
        if (!m_hooks.propPlace(move.prim, base)) continue;
        m_primBase.insert(move.prim, base);
      }
      m_hooks.placeProp(LabPrimProps::moved(m_primBase.value(move.prim), move));
      moved.insert(move.prim);
    }
    // a prop that no script moves now goes back to where it stood
    for (auto it = m_primBase.begin(); it != m_primBase.end();)
    {
      if (moved.contains(it.key())) { ++it; continue; }
      m_hooks.placeProp(it.value());
      it = m_primBase.erase(it);
    }
  }
  // a new prop gets its tint at once
  updatePropTints(added);
  updateChains();
}

void ScriptLabPanel::updateChains()
{
  // job A4, part 2: chains and ribbons between two prims, while the props are shown
  QVector<LabChains::Chain> chains;
  if (primPropsShown())
  {
    const QJsonObject folder = m_watcher->folderResult();
    chains = LabChains::fromFolderResult(folder, m_state && LabChains::objectLocked(folder, *m_state));
  }
  if (chains == m_chains) return;
  m_chains = chains;
  emit chainsChanged();
}

QJsonObject ScriptLabPanel::shownFolder() const
{
  if (!m_watcher) return QJsonObject();
  return m_player && m_player->showing() ? m_player->folderAt(m_watcher->folderResult()) : m_watcher->folderResult();
}

bool ScriptLabPanel::touchPrim(const QString& name, const QPoint& at)
{
  if (!primPropsShown()) return false;
  // job B3: a click is an input to the world of now, so the props show now again first
  if (m_player) m_player->showNow();
  const int link = m_watcher->linkOf(name);
  if (link < 1) return false;
  m_menuAnchor = at;   // a menu the touch makes stands next to the click (job A2)
  const LabWatcher::Result result = m_watcher->touch(link);
  if (!result.ok())
  {
    setLiveStatus(tr("Error: the touch on %1 was not sent. %2").arg(name, result.error), 2);
    return false;
  }
  return true;
}

void ScriptLabPanel::updatePropTints(bool force)
{
  // props as prims (lane 1 job E): only while the effects are shown and the object is watched
  const bool show = primPropsShown();
  const QList<LabTint::Tint> tints = show ? LabTint::fromFolder(shownFolder()) : QList<LabTint::Tint>();
  if (tints == m_propTints && !force) return;
  m_propTints = tints;
  emit propTintsChanged();
}

void ScriptLabPanel::effectsToggled(bool on)
{
  styleEffectsButton();
  refreshSteps();   // the step "Show effects" follows the button
  updatePrimProps();
  refreshMenuBox();   // job A2: the menu box shows only with the props
  if (!m_effects) return;
  m_effects->setEnabled(on);
  // ticked: show the newest run at once, without waiting for the next save
  if (on && m_effectsFromSession) m_effects->showSession(m_session);
  else if (on) m_effects->showLatest(m_watcher);
  refreshEffects();
}

void ScriptLabPanel::refreshEffects()
{
  if (!m_effectsStatus) return;
  const QStringList lines = m_effects && m_effects->enabled() ? m_effects->lines() : QStringList();
  m_effectsStatus->setText(lines.join(QLatin1Char('\n')));
  m_effectsStatus->setVisible(!lines.isEmpty());
  // an error starts with the word Error; the danger colour only backs it up (P10)
  const bool dark = isDark(palette());
  QPalette p = m_effectsStatus->palette();
  p.setColor(QPalette::WindowText, m_effects && m_effects->hasError() ? dangerColour(dark) : mutedColour(dark));
  m_effectsStatus->setPalette(p);
}

// --- Job 7b: camera limits in the 3D view

void ScriptLabPanel::setCameraOverride(CameraOverride* cameraOverride)
{
  if (m_cameraConnection) disconnect(m_cameraConnection);
  m_camera = cameraOverride;
  if (cameraOverride)
  {
    cameraOverride->setEnabled(m_cameraBox->isChecked() && !m_hiddenByUser);
    m_cameraConnection = connect(cameraOverride, &CameraOverride::changed, this, &ScriptLabPanel::refreshCamera);
  }
  refreshCamera();
}

void ScriptLabPanel::refreshCamera()
{
  if (!m_cameraStatus) return;
  const QStringList lines = m_camera ? m_camera->lines() : QStringList();
  m_cameraStatus->setText(lines.join(QLatin1Char('\n')));
  m_cameraStatus->setVisible(!lines.isEmpty());
  // muted text; the words say everything, no colour carries meaning here (P10)
  QPalette p = m_cameraStatus->palette();
  p.setColor(QPalette::WindowText, mutedColour(isDark(palette())));
  m_cameraStatus->setPalette(p);
}

// --- Job 7c: sky and draw distance in the 3D view

void ScriptLabPanel::setEnvironmentOverride(EnvironmentOverride* environmentOverride)
{
  if (m_environmentConnection) disconnect(m_environmentConnection);
  m_environment = environmentOverride;
  if (environmentOverride)
  {
    environmentOverride->setEnabled(m_environmentBox->isChecked() && !m_hiddenByUser);
    m_environmentConnection = connect(environmentOverride, &EnvironmentOverride::changed, this, &ScriptLabPanel::refreshEnvironment);
  }
  m_sphereControls->setOverride(environmentOverride);
  refreshEnvironment();
}

void ScriptLabPanel::refreshEnvironment()
{
  if (!m_environmentStatus) return;
  // the sliders work while the box is ticked; the part's count says when their sphere is in the view
  m_sphereControls->setAvailable(m_environment && m_environmentBox->isChecked());
  m_sphereSection->setCount(m_environment && m_environmentBox->isChecked() && m_environment->hasTrySphere() ? tr("sliders on") : QString());
  const QStringList lines = m_environment ? m_environment->lines() : QStringList();
  m_environmentStatus->setText(lines.join(QLatin1Char('\n')));
  m_environmentStatus->setVisible(!lines.isEmpty());
  // muted text; the words say everything, no colour carries meaning here (P10)
  QPalette p = m_environmentStatus->palette();
  p.setColor(QPalette::WindowText, mutedColour(isDark(palette())));
  m_environmentStatus->setPalette(p);
}

// --- Job 7c: view modes in the 3D view

void ScriptLabPanel::setViewModesOverride(ViewModesOverride* viewModesOverride)
{
  if (m_viewModesConnection) disconnect(m_viewModesConnection);
  m_viewModes = viewModesOverride;
  if (viewModesOverride)
  {
    viewModesOverride->setEnabled(m_viewModesBox->isChecked() && !m_hiddenByUser);
    m_viewModesConnection = connect(viewModesOverride, &ViewModesOverride::changed, this, &ScriptLabPanel::refreshViewModes);
  }
  refreshViewModes();
}

void ScriptLabPanel::refreshViewModes()
{
  if (!m_viewModesStatus) return;
  const QStringList lines = m_viewModes ? m_viewModes->lines() : QStringList();
  m_viewModesStatus->setText(lines.join(QLatin1Char('\n')));
  m_viewModesStatus->setVisible(!lines.isEmpty());
  // muted text; the words say everything, no colour carries meaning here (P10)
  QPalette p = m_viewModesStatus->palette();
  p.setColor(QPalette::WindowText, mutedColour(isDark(palette())));
  m_viewModesStatus->setPalette(p);
}

// --- Choose a restriction from the list (job A2) --------------------------------

void ScriptLabPanel::addActionChoices()
{
  // made last in the constructor, so the choices sit at the end of the focus chain; they take no
  // keys anyway (the list does: Space or Enter)
  for (int i = 0; i < m_actions->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* item = m_actions->topLevelItem(i);
    const Rlv::Action action{item->data(0, Qt::UserRole).toString(), item->text(0)};
    // job A2: choose Allowed or Blocked in the row itself; the word stays in the cell under it
    QComboBox* choice = new QComboBox(m_actions);
    choice->setObjectName("scriptLabActionChoice");
    choice->addItem(tr("Allowed"));
    choice->addItem(tr("Blocked"));
    choice->setProperty("action", action.id);
    choice->setAccessibleName(action.label);
    choice->setAutoFillBackground(true);
    choice->setMinimumHeight(ControlCompact);
    choice->setFocusPolicy(Qt::NoFocus);   // the keys go through the list itself (Space or Enter), one Tab stop
    m_actions->setItemWidget(item, 1, choice);
    connect(choice, QOverload<int>::of(&QComboBox::activated), this, [this, id = action.id](int index) { chooseAction(id, index == 1); });
  }
}

void ScriptLabPanel::buildChoiceBox(QWidget* parent, QVBoxLayout* layout)
{
  m_choiceBox = new QFrame(parent);
  m_choiceBox->setObjectName("scriptLabChoiceBox");
  m_choiceBox->setFrameShape(QFrame::StyledPanel);
  QVBoxLayout* box = new QVBoxLayout(m_choiceBox);
  box->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  box->setSpacing(SpacingS);

  m_choiceTitle = new QLabel(m_choiceBox);
  m_choiceTitle->setObjectName("scriptLabChoiceTitle");
  m_choiceTitle->setTextFormat(Qt::PlainText);
  m_choiceTitle->setWordWrap(true);
  QFont titleFont = m_choiceTitle->font();
  titleFont.setWeight(QFont::DemiBold);
  m_choiceTitle->setFont(titleFont);
  box->addWidget(m_choiceTitle);

  m_choiceLine = new QLabel(m_choiceBox);
  m_choiceLine->setObjectName("scriptLabChoiceLine");
  m_choiceLine->setTextFormat(Qt::PlainText);
  m_choiceLine->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_choiceLine->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_choiceLine->setToolTip(tr("The RLV line that was made and run"));
  box->addWidget(m_choiceLine);

  m_choiceExplain = new QLabel(m_choiceBox);
  m_choiceExplain->setObjectName("scriptLabChoiceExplain");
  m_choiceExplain->setTextFormat(Qt::PlainText);
  m_choiceExplain->setWordWrap(true);
  m_choiceExplain->setTextInteractionFlags(Qt::TextSelectableByMouse);
  box->addWidget(m_choiceExplain);

  // a distance or a channel that stays open: a field, used again with Apply or Enter
  m_choiceValueRow = new QWidget(m_choiceBox);
  QHBoxLayout* valueRow = new QHBoxLayout(m_choiceValueRow);
  valueRow->setContentsMargins(0, 0, 0, 0);
  valueRow->setSpacing(SpacingS);
  m_choiceValueLabel = new QLabel(m_choiceValueRow);
  m_choiceValueLabel->setObjectName("scriptLabChoiceValueLabel");
  m_choiceValue = new QLineEdit(m_choiceValueRow);
  m_choiceValue->setObjectName("scriptLabChoiceValue");
  m_choiceValue->setMinimumHeight(ControlHeight);
  m_choiceValue->setMaxLength(16);
  m_choiceValueLabel->setBuddy(m_choiceValue);
  m_choiceApply = new QPushButton(tr("Apply"), m_choiceValueRow);
  m_choiceApply->setObjectName("scriptLabChoiceApply");
  m_choiceApply->setMinimumHeight(ControlHeight);
  m_choiceApply->setToolTip(tr("Make the line again with this value, and run it (Enter)"));
  valueRow->addWidget(m_choiceValueLabel);
  valueRow->addWidget(m_choiceValue, 1);
  valueRow->addWidget(m_choiceApply);
  box->addWidget(m_choiceValueRow);
  connect(m_choiceApply, &QPushButton::clicked, this, [this]() { applyChoiceValue(); });
  connect(m_choiceValue, &QLineEdit::returnPressed, this, [this]() { applyChoiceValue(); });

  m_useInScript = new QPushButton(tr("Use in script"), m_choiceBox);
  m_useInScript->setObjectName("scriptLabUseInScript");
  m_useInScript->setMinimumHeight(ControlHeight);
  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->setSpacing(SpacingS);
  buttons->addWidget(m_useInScript);
  // why the button is greyed, readable without hovering (charter P6)
  m_useInScriptReason = new QLabel(m_choiceBox);
  m_useInScriptReason->setObjectName("scriptLabUseInScriptReason");
  m_useInScriptReason->setTextFormat(Qt::PlainText);
  m_useInScriptReason->setWordWrap(true);
  buttons->addWidget(m_useInScriptReason, 1);
  box->addLayout(buttons);
  connect(m_useInScript, &QPushButton::clicked, this, [this]() { useChoiceInScript(); });

  m_choiceResult = new QLabel(m_choiceBox);
  m_choiceResult->setObjectName("scriptLabChoiceResult");
  m_choiceResult->setTextFormat(Qt::PlainText);
  m_choiceResult->setWordWrap(true);
  m_choiceResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
  box->addWidget(m_choiceResult);

  m_choiceBox->hide();
  layout->addWidget(m_choiceBox);
}

QComboBox* ScriptLabPanel::actionChoice(const QString& actionId) const
{
  for (int i = 0; i < m_actions->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* item = m_actions->topLevelItem(i);
    if (item->data(0, Qt::UserRole).toString() == actionId) return qobject_cast<QComboBox*>(m_actions->itemWidget(item, 1));
  }
  return nullptr;
}

void ScriptLabPanel::setChoiceResult(const QString& text, int tone)
{
  m_choiceResult->setText(text);
  m_choiceResult->setVisible(!text.isEmpty());
  const bool dark = isDark(palette());
  QPalette p = m_choiceResult->palette();
  p.setColor(QPalette::WindowText, tone == 1 ? successColour(dark) : tone == 2 ? dangerColour(dark) : mutedColour(dark));
  m_choiceResult->setPalette(p);
}

void ScriptLabPanel::showChoice(const QString& actionId, bool blocked, const QString& line, const QStringList& words,
                                const QString& result, int tone)
{
  const LabRestrictionChoice::Choice choice = LabRestrictionChoice::forAction(actionId, m_state ? m_state->mode() : Rlv::Mode::Rlva);
  if (m_choiceAction != actionId) m_choiceValue->clear();   // the field belongs to one row
  m_choiceAction = actionId;
  m_choiceRlvLine = line;
  m_choiceTitle->setText(blocked ? tr("%1: Blocked").arg(choice.label) : tr("%1: Allowed").arg(choice.label));
  m_choiceLine->setText(line);
  m_choiceLine->setVisible(!line.isEmpty());
  m_choiceExplain->setText(words.join(QLatin1Char('\n')));
  m_choiceExplain->setVisible(!words.isEmpty());
  const bool field = choice.value != LabRestrictionChoice::ValueKind::None && blocked;
  m_choiceValueRow->setVisible(field);
  if (choice.value == LabRestrictionChoice::ValueKind::Distance)
  {
    m_choiceValueLabel->setText(tr("Farther than (metres)"));
    m_choiceValue->setPlaceholderText(tr("Empty: the viewer's own limit"));
    m_choiceValue->setToolTip(tr("Blocks it only farther away than this. Empty uses the viewer's own limit."));
  }
  else if (choice.value == LabRestrictionChoice::ValueKind::ExceptChannel)
  {
    m_choiceValueLabel->setText(tr("Still allow channel"));
    m_choiceValue->setPlaceholderText(tr("Empty: none"));
    m_choiceValue->setToolTip(tr("This channel stays open while the others are blocked."));
  }
  setChoiceResult(result, tone);
  m_choiceBox->show();
  refreshUseInScript();
}

void ScriptLabPanel::refreshUseInScript()
{
  if (!m_useInScript) return;
  QString reason;
  if (m_choiceRlvLine.isEmpty()) reason = tr("There is no line to use yet. Choose Blocked first.");
  else if (!m_editor || m_editor->filePath().isEmpty()) reason = tr("Open a script on the Live scripts page first.");
  m_useInScript->setEnabled(reason.isEmpty());
  m_useInScriptReason->setText(reason);
  m_useInScriptReason->setVisible(!reason.isEmpty());
  QPalette muted = m_useInScriptReason->palette();
  muted.setColor(QPalette::WindowText, mutedColour(isDark(palette())));
  m_useInScriptReason->setPalette(muted);
  m_useInScript->setToolTip(reason.isEmpty() ? tr("Put %1 into the script, so the script sends this line.")
                                                 .arg(LabRestrictionChoice::scriptLine(m_choiceRlvLine))
                                             : reason);
}

bool ScriptLabPanel::chooseAction(const QString& actionId, bool blocked)
{
  if (!m_state || !actionChoice(actionId)) return false;
  const LabRestrictionChoice::Choice choice = LabRestrictionChoice::forAction(actionId, m_state->mode());
  QString object = m_object->text().trimmed();
  if (object.isEmpty()) object = QString::fromLatin1(DefaultObject);
  const auto followUp = qScopeGuard([this]() { refresh(); });   // the row shows the state again, whatever happened
  if (!choice.ok())
  {
    showChoice(actionId, blocked, QString(), QStringList(), choice.reason, 2);
    return false;
  }
  if (blocked)
  {
    const QString value = m_choiceAction == actionId ? m_choiceValue->text() : QString();
    const LabRestrictionChoice::Line line = LabRestrictionChoice::blockLine(choice, value);
    if (!line.ok())
    {
      showChoice(actionId, true, QString(), QStringList(), line.error, 2);
      return false;
    }
    const bool ran = runLine(line.text);
    showChoice(actionId, true, line.text, LabRestrictionChoice::explainBlock(choice, value),
               ran ? tr("Ran from %1. Blocked now.").arg(object) : tr("Refused. Replies and messages says why."), ran ? 1 : 2);
    return ran;
  }
  const QString lift = LabRestrictionChoice::allowLine(choice, m_state->restrictions(), object);
  const bool ran = !lift.isEmpty() && runLine(lift);
  const QStringList others = LabRestrictionChoice::otherHolders(choice, m_state->restrictions(), object);
  QString result;
  int tone = 1;
  if (!others.isEmpty())
  {
    result = tr("Still blocked, because %1 holds it too.").arg(others.join(QStringLiteral(", ")));
    tone = 2;
  }
  else if (lift.isEmpty()) result = tr("Nothing to lift. %1 held nothing of it.").arg(object);
  else result = ran ? tr("Ran from %1. Allowed now.").arg(object) : tr("Refused. Replies and messages says why.");
  if (!lift.isEmpty() && !ran) tone = 2;
  showChoice(actionId, false, lift, lift.isEmpty() ? QStringList() : LabRestrictionChoice::explainAllow(choice), result, tone);
  return ran;
}

bool ScriptLabPanel::applyChoiceValue()
{
  if (!m_state || m_choiceAction.isEmpty()) return false;
  const LabRestrictionChoice::Choice choice = LabRestrictionChoice::forAction(m_choiceAction, m_state->mode());
  const LabRestrictionChoice::Line line = LabRestrictionChoice::blockLine(choice, m_choiceValue->text());
  if (!line.ok())
  {
    setChoiceResult(line.error, 2);
    return false;
  }
  // the old limit of this object goes first, so only the new one is held
  QString object = m_object->text().trimmed();
  if (object.isEmpty()) object = QString::fromLatin1(DefaultObject);
  const QString lift = LabRestrictionChoice::allowLine(choice, m_state->restrictions(), object);
  if (!lift.isEmpty()) runLine(lift);
  return chooseAction(m_choiceAction, true);
}

bool ScriptLabPanel::useChoiceInScript()
{
  refreshUseInScript();
  if (!m_useInScript->isEnabled())
  {
    setChoiceResult(m_useInScript->toolTip(), 2);
    return false;
  }
  LabCodeEdit* edit = m_editor->codeEdit();
  const QString code = LabRestrictionChoice::scriptLine(m_choiceRlvLine);
  const LabRestrictionChoice::Insertion in = LabRestrictionChoice::insert(edit->toPlainText(), edit->textCursor().position(), code);
  // one undo step, so Ctrl+Z takes it out again
  QTextCursor cursor(edit->document());
  cursor.setPosition(in.at);
  cursor.insertText(in.added);
  cursor.setPosition(in.position + int(code.size()));
  edit->setTextCursor(cursor);
  showPage(LivePage);
  edit->ensureCursorVisible();
  edit->setFocus(Qt::OtherFocusReason);
  setChoiceResult(tr("Put in the script at line %1. Save to run it.").arg(in.line), 1);
  return true;
}

