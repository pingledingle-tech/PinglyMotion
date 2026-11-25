/*
 * PinglyMotion Script Lab - the Script Lab panel: Script Lab for beginners (job U3): record and replay, the work layout, the example project, the steps and the tips
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QInputDialog>
#include <QDir>
#include <QFile>
#include <QScrollArea>
#include <QTabBar>
#include <QFont>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QListWidget>
#include <QCheckBox>
#include <QToolButton>

#include "labtree.h"
#include "channelmapview.h"
#include "textsize.h"
#include "lucideicons.h"
#include "labplayer.h"
#include "theme.h"
#include "usersettings.h"
#include "scriptlabpanelui.h"

#include "scriptlabpanel.h"

using namespace LabUi;

// --- Script Lab for beginners (job U3) ----------------------------------------

void ScriptLabPanel::styleEffectsButton()
{
  if (!m_effectsBox) return;
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  const QString selection = Theme::color(Theme::Token::Selection, dark).name(QColor::HexRgb).toUpper();
  // off: a normal button with the eye; on: the selection fill, on_accent text and a check mark
  // (DESIGN_SYSTEM.md, "States": selected has a fill and a shape)
  m_effectsBox->setStyleSheet(QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:checked { background: %6; color: %7; border: 1px solid %6; }"
    "QPushButton:focus { border: 2px solid %8; }")
    .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border))
    .arg(RadiusControl).arg(SpacingM)
    .arg(selection, QLatin1String(t.onAccent), QLatin1String(t.focusRing)));
  const bool on = m_effectsBox->isChecked();
  QFont effectsFont = m_effectsBox->font();   // bold when on: a shape besides the fill (P10)
  effectsFont.setWeight(on ? QFont::DemiBold : QFont::Normal);
  m_effectsBox->setFont(effectsFont);
  m_effectsBox->setIcon(LucideIcons::icon(on ? QStringLiteral("check") : QStringLiteral("eye"),
                                          QColor(QLatin1String(on ? t.onAccent : t.text)), QColor(QLatin1String(t.textMuted))));
  m_effectsBox->setAccessibleDescription(on ? tr("On: the 3D view shows what the scripts do.") : tr("Off."));
  styleRecordButton();
}

// --- record and replay (job A3) -------------------------------------------------------------

void ScriptLabPanel::styleRecordButton()
{
  if (!m_recordButton) return;
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  const QString selection = Theme::color(Theme::Token::Selection, dark).name(QColor::HexRgb).toUpper();
  // the same on/off look as Show effects in 3D view: on has the selection fill, the word and a square
  m_recordButton->setStyleSheet(QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:checked { background: %6; color: %7; border: 1px solid %6; }"
    "QPushButton:focus { border: 2px solid %8; }")
    .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border))
    .arg(RadiusControl).arg(SpacingM)
    .arg(selection, QLatin1String(t.onAccent), QLatin1String(t.focusRing)));
  const bool on = m_recordButton->isChecked();
  QFont recordFont = m_recordButton->font();   // bold when on, as Show effects in 3D view
  recordFont.setWeight(on ? QFont::DemiBold : QFont::Normal);
  m_recordButton->setFont(recordFont);
  m_recordButton->setIcon(LucideIcons::icon(on ? QStringLiteral("square") : QStringLiteral("video"),
                                            QColor(QLatin1String(on ? t.onAccent : t.text)), QColor(QLatin1String(t.textMuted))));
}

bool ScriptLabPanel::setRecording(bool on)
{
  if (!m_watcher) return false;
  bool changed = false;
  m_recipeNote.clear();
  m_recipeNoteIsError = false;
  if (on && !m_watcher->recording())
  {
    const LabWatcher::Result result = m_watcher->startRecording();
    if (!result.ok())
    {
      m_recipeNote = tr("Error: %1").arg(result.error);
      m_recipeNoteIsError = true;
    }
    changed = result.ok();
  }
  else if (!on && m_watcher->recording())
  {
    const bool none = m_watcher->recordedSteps().isEmpty();
    changed = m_watcher->stopRecording();
    if (none) m_recipeNote = tr("Recording stopped. No steps were recorded, so the recipe stays as it was.");
  }
  refreshRecipe();
  return changed;
}

bool ScriptLabPanel::replayNow()
{
  if (!m_watcher) return false;
  m_recipeNote.clear();
  m_recipeNoteIsError = false;
  const LabWatcher::Result result = m_watcher->replayRecipe();
  if (!result.ok())
  {
    m_recipeNote = tr("Error: %1").arg(result.error);
    m_recipeNoteIsError = true;
  }
  refreshRecipe();
  return result.ok();
}

void ScriptLabPanel::setExpertMode(bool expert)
{
  const bool changed = m_expert != expert;
  m_expert = expert;
  if (changed && m_mapSection) m_mapSection->setExpanded(expert);
  refreshRecipe();
}

void ScriptLabPanel::refreshRecipe()
{
  if (!m_recipeRow) return;
  refreshImprove();   // queue 1 item 5: a replay brings a new weight to compare
  const bool object = m_watcher && m_watcher->watching() && m_watcher->linkset();
  m_recipeRow->setVisible(object);
  m_playerEmpty->setVisible(!object);   // job UX-B: the tab says what fills it
  m_recipeSection->setVisible(object && m_expert);
  // job B3: the player shows the newest replay, when it has moments
  m_player->setExpertMode(m_expert);
  m_player->setReplay(object ? m_watcher->lastReplay() : QJsonObject());
  m_player->setVisible(object && m_player->momentCount() > 0);
  if (!object)
  {
    m_recordButton->setChecked(false);
    styleRecordButton();
    return;
  }
  const bool recording = m_watcher->recording();
  m_recordButton->setChecked(recording);
  m_recordButton->setText(recording ? tr("Stop recording") : tr("Record what I do"));
  m_recordButton->setAccessibleDescription(recording ? tr("Recording.") : tr("Not recording."));
  styleRecordButton();

  QStringList lines;
  bool bad = m_recipeNoteIsError;
  const QJsonObject replay = m_watcher->lastReplay();
  auto strings = [](const QJsonValue& v)
  {
    QStringList out;
    for (const QJsonValue& s : v.toArray()) out << s.toString();
    return out;
  };
  if (recording)
  {
    lines << tr("Recording. Click props and menu buttons. Steps so far: %1.").arg(m_watcher->recordedSteps().size());
    if (!primPropsShown()) lines << tr("Turn on Show effects in 3D view to click props.");
  }
  else if (!m_recipeNote.isEmpty())
    lines << m_recipeNote;
  else if (!replay.isEmpty())
  {
    const int steps = replay.value("steps").toInt();
    const int done = replay.value("done").toInt();
    if (!replay.value("ran").toBool())
    {
      bad = true;
      lines << tr("The object did not run, so your steps were not replayed.");
      lines << strings(replay.value("changes"));
    }
    else
    {
      lines << (done == steps ? tr("Replayed your %n step(s).", nullptr, steps) : tr("Replayed %1 of %2 steps.").arg(done).arg(steps));
      const QStringList changes = strings(replay.value("changes"));
      if (replay.value("first").toBool())
      {
        lines << tr("This is the first time, so there is nothing to compare.");
        lines << tr("Now:");
        for (const QString& f : strings(replay.value("facts"))) lines << QStringLiteral("  ") + f;
      }
      else if (changes.isEmpty())
        lines << tr("Nothing changed since last time.");
      else
      {
        lines << tr("What changed since last time:");
        for (const QString& c : changes) lines << QStringLiteral("  ") + c;
      }
    }
  }
  else if (m_watcher->hasRecipe())
    lines << tr("The recipe is replayed after the next save.");
  else
    lines << tr("No recipe yet. Press Record what I do, then click your props.");
  m_recipeStatus->setText(lines.join(QLatin1Char('\n')));
  const bool dark = isDark(palette());
  QPalette p = m_recipeStatus->palette();
  p.setColor(QPalette::WindowText, bad ? dangerColour(dark) : palette().color(QPalette::WindowText));
  m_recipeStatus->setPalette(p);

  // Expert: the file and its text
  m_recipeFileLine->setText(m_watcher->hasRecipe()
                              ? tr("File: %1 in the object folder. %2 is kept next to it.").arg(LabRecipe::recipeFile(), LabRecipe::resultFile())
                              : tr("No recipe file yet. Recording makes %1 in the object folder.").arg(LabRecipe::recipeFile()));
  QString text;
  if (m_watcher->hasRecipe())
  {
    QFile file(m_watcher->recipePath());
    if (file.open(QIODevice::ReadOnly)) text = QString::fromUtf8(file.read(LabRecipe::MAX_FILE_BYTES));
  }
  if (m_recipeText->toPlainText() != text) m_recipeText->setPlainText(text);
  m_replayButton->setEnabled(m_watcher->hasRecipe() && !recording);
  m_replayButton->setToolTip(m_replayButton->isEnabled() ? tr("Repeats the recipe's steps now, without a save.")
                                                         : recording ? tr("Stop recording first.") : tr("Record what you do first."));
}

int ScriptLabPanel::sideBySideWidth()
{
  // room for the tree (about 300 px) and the editor with its suggestion list (about 440 px)
  return TextSize::scaled(760);
}

void ScriptLabPanel::updateWorkLayout()
{
  if (!m_workLayout) return;
  // job UX-B: the width of the Live page's own window while it is popped out
  const int room = m_pages[LivePage] && isPoppedOut(LivePage) ? m_pages[LivePage]->width() : width();
  const bool wide = room >= sideBySideWidth();
  if (wide == m_sideBySide && m_workLayout->direction() == (wide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom)) return;
  m_sideBySide = wide;
  m_workLayout->setDirection(wide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
  // side by side the width is shared 2 to 3; stacked, each column takes the height it needs
  m_workLayout->setStretch(0, wide ? 2 : 0);
  m_workLayout->setStretch(1, wide ? 3 : 0);
  // side by side, both columns start at the top
  m_workLayout->setAlignment(m_treeColumn, wide ? Qt::AlignTop : Qt::Alignment());
  m_workLayout->setAlignment(m_editorColumn, wide ? Qt::AlignTop : Qt::Alignment());
  // the page's layout keeps the old size of the work area until it is told: stacked needs far more height
  m_workLayout->invalidate();
  m_workArea->updateGeometry();
}

QString ScriptLabPanel::exampleObjectsFolder() const
{
  if (!m_objectsFolder.isEmpty()) return m_objectsFolder;
  return QDir::cleanPath(EffectsPreview::defaultExamplesFolder() + QStringLiteral("/objects"));
}

QString ScriptLabPanel::scriptsRoot() const
{
  return m_scriptsRoot.isEmpty() ? LabBeginner::defaultScriptsRoot() : m_scriptsRoot;
}

bool ScriptLabPanel::watchingProject() const
{
  return !m_projectFolder.isEmpty() && m_watcher && m_watcher->watching() && m_watcher->linkset()
         && QDir::cleanPath(m_watcher->folder()) == QDir::cleanPath(QDir::fromNativeSeparators(m_projectFolder));
}

bool ScriptLabPanel::makeExampleProject()
{
  QString error;
  const LabExample example = LabBeginner::firstExample(exampleObjectsFolder(), &error);
  if (!example.isValid())
  {
    m_refusal = tr("Error: %1").arg(error);
    refreshLive();
    return false;
  }
  const QString folder = LabBeginner::copyExample(example, scriptsRoot(), &error);
  if (folder.isEmpty())
  {
    m_refusal = tr("Error: %1").arg(error);
    refreshLive();
    return false;
  }
  m_projectFolder = folder;
  m_stepExample = example;
  m_doneSteps.clear();
  // the props first, so the colours of the first run find them by name
  if (m_hooks.showPropsTab) m_hooks.showPropsTab();
  if (m_hooks.addProp)
    for (const LabExample::Prop& prop : example.props) m_hooks.addProp(prop);
  const QList<LabExample::Entry> steps = example.steps();
  for (int i = 0; i < steps.size(); ++i)
    if (steps.at(i).control == QLatin1String("Props") || steps.at(i).control == QLatin1String("Live scripts")) m_doneSteps.insert(i);
  m_stepsShown = true;
  if (!watchFolder(folder)) return false;
  showPage(LivePage);
  m_effectsBox->setChecked(true);
  refreshLive();
  return true;
}

bool ScriptLabPanel::makeNewFolder(const QString& name)
{
  QString error;
  const QString folder = LabBeginner::makeFolder(scriptsRoot(), name, &error);
  if (folder.isEmpty())
  {
    m_refusal = tr("Error: %1").arg(error);
    refreshLive();
    return false;
  }
  return watchFolder(folder);
}

void ScriptLabPanel::chooseNewFolder()
{
  bool ok = false;
  const QString name = QInputDialog::getText(this, tr("New folder"),
                                             tr("Name of the new folder. It goes in %1.").arg(QDir::toNativeSeparators(scriptsRoot())),
                                             QLineEdit::Normal, tr("My object"), &ok);
  if (ok) makeNewFolder(name);
}

QAction* ScriptLabPanel::makeStepsAction(QObject* parent)
{
  QAction* action = new QAction(tr("Script Lab steps"), parent);
  action->setObjectName("helpScriptLabStepsAction");
  action->setToolTip(tr("Show the steps of the example project again."));
  action->setStatusTip(action->toolTip());
  connect(action, &QAction::triggered, this, &ScriptLabPanel::showSteps);
  return action;
}

void ScriptLabPanel::showSteps()
{
  // no example made yet: the steps of the first example, whose Watch step makes the project
  if (!m_stepExample.isValid()) m_stepExample = LabBeginner::firstExample(exampleObjectsFolder());
  m_stepsShown = true;
  showPage(LivePage);
  refreshSteps();
  if (m_stepCard->isVisible())
  {
    for (QPushButton* b : m_stepCard->stepButtons())
      if (b->isEnabled()) { b->setFocus(Qt::OtherFocusReason); break; }
    if (QScrollArea* scroll = qobject_cast<QScrollArea*>(m_pages[LivePage])) scroll->ensureWidgetVisible(m_stepCard);
  }
}

void ScriptLabPanel::showLiveScripts()
{
  // the Live scripts tab to the front; its page from the top, where the folder (or the empty
  // state with Watch folder) is
  showPage(LivePage);
  QScrollArea* scroll = qobject_cast<QScrollArea*>(m_pages[LivePage]);
  if (scroll) scroll->verticalScrollBar()->setValue(0);
  // a short frame round the tab and round the top of the page, so the press always shows
  // something, also when the page was open already (the project owner, 24 Nov 2026)
  const bool dark = isDark(palette());
  QTabBar* bar = m_tabs->tabBar();
  if (tabIndex(LivePage) >= 0) LabHighlight::show(bar, bar->tabRect(tabIndex(LivePage)), dark);
  QWidget* top = m_liveEmpty && !m_liveEmpty->isHidden() ? static_cast<QWidget*>(m_liveEmpty) : m_liveFolder;
  if (scroll && top && scroll->widget())
  {
    const QPoint at = top->mapTo(scroll->widget(), QPoint(0, 0));
    QRect area(at, top->size());
    if (top == m_liveFolder) area = QRect(QPoint(0, at.y()), QSize(scroll->widget()->width(), top->height()));
    LabHighlight::show(scroll->widget(), area.adjusted(-SpacingXs, -SpacingXs, SpacingXs, SpacingXs), dark);
  }
  // the keys go to the tab, so Left and Right move between the pages from here (job UX-B: to the
  // page itself when it is a window of its own)
  if (tabIndex(LivePage) >= 0) bar->setFocus(Qt::OtherFocusReason);
  else m_pages[LivePage]->setFocus(Qt::OtherFocusReason);
}

void ScriptLabPanel::showHelp(const QString& topic)
{
  if (!m_helpView)
  {
    m_helpView = new LabHelpView(exampleObjectsFolder(), this);
    m_helpView->setWindowFlag(Qt::Window, true);
    // the guide page's button: watch the folder it made (job UX-D)
    connect(m_helpView, &LabHelpView::openFolderRequested, this, [this](const QString& folder) {
      if (watchFolder(folder)) showPage(LivePage);
    });
  }
  m_helpView->applyStyle(isDark(palette()));
  if (!m_helpView->showTopic(topic)) m_helpView->showTopic(QStringLiteral("start"));
  m_helpView->show();
  m_helpView->raise();
  m_helpView->activateWindow();
  m_helpView->topicList()->setFocus(Qt::OtherFocusReason);
}

void ScriptLabPanel::closeSteps()
{
  m_stepsShown = false;
  refreshSteps();
}

void ScriptLabPanel::refreshSteps()
{
  if (!m_stepCard) return;
  const bool show = m_stepsShown && m_stepExample.isValid();
  m_stepCard->setVisible(show);
  if (!show) return;
  m_stepCard->setExample(m_stepExample.title, m_stepExample.texts(QStringLiteral("see")), m_stepExample.texts(QStringLiteral("next")));
  const QList<LabExample::Entry> entries = m_stepExample.steps();
  int lastProps = -1;
  for (int i = 0; i < entries.size(); ++i)
    if (entries.at(i).control == QLatin1String("Props")) lastProps = i;
  bool allProps = !m_stepExample.props.isEmpty() && m_hooks.hasProp;
  if (allProps)
    for (const LabExample::Prop& prop : m_stepExample.props) allProps = allProps && m_hooks.hasProp(prop.name);
  QList<LabStepCard::Step> steps;
  for (int i = 0; i < entries.size(); ++i)
  {
    const LabExample::Entry& e = entries.at(i);
    LabStepCard::Step step;
    step.text = e.text;
    const QString notReady = tr("This step does not work in the program yet.");
    if (e.control == QLatin1String("Props") && i == lastProps)
    {
      step.button = tr("Add the props");
      step.enabled = bool(m_hooks.addProp);
      step.reason = notReady;
      step.done = allProps;
    }
    else if (e.control == QLatin1String("Props"))
    {
      step.button = tr("Show Props");
      step.enabled = bool(m_hooks.showPropsTab);
      step.reason = notReady;
      step.done = m_doneSteps.contains(i) || allProps;
    }
    else if (e.control == QLatin1String("Live scripts"))
    {
      step.button = tr("Show Live scripts");
      step.done = m_doneSteps.contains(i) || watchingProject();
    }
    else if (e.control == QLatin1String("Watch folder..."))
    {
      step.button = tr("Watch the folder");
      step.done = watchingProject();
    }
    else if (e.control == QLatin1String("Show effects in 3D view"))
    {
      step.button = tr("Show effects");
      step.done = m_effectsBox->isChecked() && watchingProject();
    }
    else
    {
      // Touch, Say and the dialog buttons do not reach a watched folder yet (docs/EXAMPLES.md)
      step.button = e.control;
      step.enabled = false;
      step.reason = notReady;
    }
    steps << step;
  }
  m_stepCard->setSteps(steps);
}

bool ScriptLabPanel::doStep(int index)
{
  const QList<LabExample::Entry> entries = m_stepExample.steps();
  if (index < 0 || index >= entries.size()) return false;
  const QList<LabStepCard::Step> shown = m_stepCard->steps();
  if (index < shown.size() && !shown.at(index).enabled) return false;
  const QString control = entries.at(index).control;
  int lastProps = -1;
  for (int i = 0; i < entries.size(); ++i)
    if (entries.at(i).control == QLatin1String("Props")) lastProps = i;
  bool ok = true;
  if (control == QLatin1String("Props") && index == lastProps)
  {
    if (!m_hooks.addProp) return false;
    if (m_hooks.showPropsTab) m_hooks.showPropsTab();
    for (const LabExample::Prop& prop : m_stepExample.props) ok = m_hooks.addProp(prop) && ok;
  }
  else if (control == QLatin1String("Props"))
  {
    if (!m_hooks.showPropsTab) return false;
    m_hooks.showPropsTab();
  }
  else if (control == QLatin1String("Live scripts"))
    showLiveScripts();
  else if (control == QLatin1String("Watch folder..."))
  {
    // the example project's folder; made first when there is none yet
    if (m_projectFolder.isEmpty()) ok = makeExampleProject();
    else ok = watchFolder(m_projectFolder);
  }
  else if (control == QLatin1String("Show effects in 3D view"))
    m_effectsBox->setChecked(true);
  else
    return false;
  if (ok) m_doneSteps.insert(index);
  refreshSteps();
  return ok;
}

bool ScriptLabPanel::selectPrim(const QString& name)
{
  if (!m_followBox->isChecked()) return false;
  return m_folderTree->selectPrim(name);
}

void ScriptLabPanel::buildTips()
{
  // where each tip stands: next to its word (prim, root prim and notecard over the tree;
  // sandbox under the explanation; the RLV words under the Viewer rules row)
  QVBoxLayout* outer = qobject_cast<QVBoxLayout*>(layout());
  for (const LabBeginner::Tip& tip : LabBeginner::jargonTips())
  {
    LabTipCard* card = new LabTipCard(tip, this);
    card->hide();
    const QString key = QLatin1String(tip.key);
    if (key == QLatin1String("prim") || key == QLatin1String("rootPrim") || key == QLatin1String("notecard"))
    {
      card->setParent(m_treeSection->body());
      m_treeSection->bodyLayout()->insertWidget(1, card);   // under the line on what the tree shows
    }
    else if (key == QLatin1String("channel") || key == QLatin1String("listener"))
    {
      card->setParent(m_mapSection->body());
      m_mapSection->bodyLayout()->insertWidget(1, card);   // job A4: under the line on what the map shows
    }
    else
    {
      // in a row with the panel's side margins, as the rows around it; a hidden card leaves no gap
      QHBoxLayout* row = new QHBoxLayout;
      row->setContentsMargins(SpacingM, 0, SpacingM, 0);
      row->addWidget(card);
      outer->insertLayout(key == QLatin1String("sandbox") ? outer->indexOf(m_explainFrame) + 1 : outer->indexOf(m_tabs), row);
    }
    // the tip's word is also the tooltip of its word, so it can be read again after closing
    if (key == QLatin1String("viewerRules")) m_modeLabel->setToolTip(tip.text);
    connect(card, &LabTipCard::closed, this, &ScriptLabPanel::closeTip);
    m_tipCards << card;
  }
}

LabTipCard* ScriptLabPanel::shownTip() const
{
  for (LabTipCard* card : m_tipCards)
    if (!card->isHidden()) return card;
  return nullptr;
}

void ScriptLabPanel::closeTip(const QString& key)
{
  QStringList closed = UserSettings::scriptLabTipsClosed().split(QLatin1Char(','), Qt::SkipEmptyParts);
  if (!closed.contains(key)) closed << key;
  UserSettings::setScriptLabTipsClosed(closed.join(QLatin1Char(',')));
  const bool hadFocus = shownTip() && shownTip()->isAncestorOf(focusWidget());
  refreshTips();
  // the keyboard stays in the panel: on the next tip's close button, or the panel
  if (hadFocus)
  {
    if (LabTipCard* next = shownTip()) next->closeButton()->setFocus(Qt::OtherFocusReason);
    else setFocus(Qt::OtherFocusReason);
  }
}

void ScriptLabPanel::refreshTips()
{
  if (m_tipCards.isEmpty()) return;
  // value first: no tips before something is watched, and none while hints are off (charter P4)
  const bool watching = m_watcher && m_watcher->watching();
  const QStringList closed = UserSettings::scriptLabTipsClosed().split(QLatin1Char(','), Qt::SkipEmptyParts);
  bool notecards = false;
  for (QTreeWidgetItemIterator it(m_folderTree); *it; ++it)
    if ((*it)->data(LabFolderTree::NameColumn, LabFolderTree::KindRole).toInt() == LabFolderTree::NotecardRow) notecards = true;
  const bool treeShown = m_treeSection->isVisibleTo(this) && m_treeSection->isExpanded() && m_folderTree->objectItem();
  LabTipCard* due = nullptr;
  if (m_hintsOn && watching)
    for (LabTipCard* card : m_tipCards)
    {
      const QString key = card->key();
      if (closed.contains(key)) continue;
      bool anchored = true;
      if (key == QLatin1String("prim") || key == QLatin1String("rootPrim")) anchored = treeShown;
      else if (key == QLatin1String("notecard")) anchored = treeShown && notecards;
      else if (key == QLatin1String("channel") || key == QLatin1String("listener"))
        anchored = m_mapSection->isVisibleTo(this) && m_mapSection->isExpanded() && !m_mapView->map().arrows.isEmpty();
      if (!anchored) continue;
      due = card;
      break;
    }
  for (LabTipCard* card : m_tipCards) card->setVisible(card == due);
}

