/***************************************************************************
 *   Copyright (C) 2006 by Zi Ree                                          *
 *   Zi Ree @ SecondLife                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QScopedValueRollback>
#include <QActionGroup>
#include <cstdlib>
#include <QCursor>
#include <QButtonGroup>
#include <QMap>
#include <QMenu>
#include <QRegularExpression>
#include <QDockWidget>
#include <QScrollArea>
#include <QStackedWidget>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QStyle>
#include <QDir>
#include <QFileDialog>
#include "startfolder.h"
#include "logging.h"
#include "applog.h"
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QScopedPointer>
#include <QSignalBlocker>
#include <QSharedPointer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QSlider>
#include <QCheckBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QTabBar>
#include <QTableWidget>

#include "aboutdialog.h"
#include "animexportdialog.h"
#include "animimportconvert.h"
#include "dinglesave.h"
#include "gltfimport.h"
#include "gltfimportdialog.h"
#include "retargetdialog.h"
#include "animationview.h"
#include "bvh.h"
#include "dingle.h"
#include "easingtiles.h"
#include "figures.h"
#include "propfile.h"
#include "constants.h"
#include "controlapi.h"
#include "controlserver.h"
#include "controlindicator.h"
#include "aiassistant.h"
#include "aiconnectdialog.h"
#include "auroralink.h"
#include "sliderscale.h"
#include <QStatusBar>
#include "prop.h"
#include "jointgroups.h"
#include "jointinfo.h"
#include "handpose.h"
#include "handstab.h"
#include "jointlist.h"
#include "lucideicons.h"
#include "textsize.h"
#include "dialogfit.h"
#include "panelcontent.h"
#include "selectedjointpanel.h"
#include "beginnerview.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "paneltitlebar.h"
#include "pinglymotion.h"
#include "scene.h"
#include "timeline.h"
#include "timelinenotes.h"
#include "timelinerows.h"
#include "timelineview.h"
#include "updatersettings.h"
#include "usersettings.h"
#include "usersettingsdialog.h"

#include "panelvisibility.h"
#include "propmanager.h"
#include "pinglymotionwindow.h"
#include "undohistory.h"
#include "playback.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlabcontroller.h"
#include "librarypanel.h"
#include "theme.h"
#include "menuspec.h"
#include "viewmenuspec.h"
#include "emptystate.h"
#include "guidancespec.h"
#include "hintcard.h"
#include "helpermode.h"
#include "helperspec.h"
#include "welcomedialog.h"
#include "topbar.h"
#include "reasonline.h"
#include "setupdialog.h"
#include "camera.h"
#include "camerahabitspec.h"
#include "camerakeysdialog.h"
#include "keysmousedialog.h"
#include "transportbar.h"
#include "ui_pinglymotionwindow.h"
#include "whatsnewdialog.h"
#include "wheelguard.h"
#include "viewtoolbar.h"

// Part of PinglyMotionWindow: the 3D view tools, empty state, panel layout, text size, dialogs, easing, edit and option menu actions.
// (Split out of pinglymotionwindow.cpp; nothing here changes what the window does.)

// A pull was released: each joint that bent gets a key with its new turn at this frame (a turn
// is a key at once, as with the sliders), and the status line says which.
void PinglyMotionWindow::pullFinished(BVHNode* pulled,const QList<QPair<BVHNode*,QVector3D>>& turns,bool turnOnly)
{
  Animation* anim=scene->getAnimation();
  if(!anim || !pulled || turns.isEmpty()) return;
  if(m_hintCards) m_hintCards->markPulled();
  QStringList driven;
  for(const auto& turn : turns)
  {
    anim->setRotation(turn.first,turn.second.x(),turn.second.y(),turn.second.z());
    if(!turnOnly) driven << jointDisplayName(turn.first->name());
  }
  if(currentPart)
  {
    const QVector3D rotation=anim->getRotation(currentPart);
    setX(rotation.x());
    setY(rotation.y());
    setZ(rotation.z());
  }
  updateKeyBtn();
  ui->animationView->update();
  // the frame as the frame counter shows it (from 1)
  statusBar()->showMessage(ViewToolSpec::pullDoneMessage(jointDisplayName(pulled->name()),driven,anim->getFrame()+1,turnOnly),8000);
}

void PinglyMotionWindow::chooseViewTool(ViewTool tool)
{
  UserSettings::setViewTool(QLatin1String(Camera::toolId(tool)));
  ui->animationView->setViewTool(tool);
  if(m_viewToolbar) m_viewToolbar->setTool(tool);
  statusBar()->showMessage(ViewToolSpec::toolChosenMessage(tool),5000);
}

// the saved choice of a Show item of the view toolbar (and View > Show)
bool PinglyMotionWindow::viewPartShown(ViewToolSpec::Part part)
{
  using Part=ViewToolSpec::Part;
  switch(part)
  {
    case Part::Floor: return UserSettings::showFloor();
    case Part::Rings: return UserSettings::showRings();
    case Part::JointDots: return UserSettings::showJointDots();
    case Part::ToolNames: return UserSettings::viewToolNames();
  }
  return true;
}

// View > Click and bend on the figure (UX job U2): remembered, and said in the status line
void PinglyMotionWindow::setClickAndBend(bool on)
{
  UserSettings::setClickAndBend(on);
  ui->animationView->setClickAndBend(on);
  if(m_clickAndBendAction && m_clickAndBendAction->isChecked()!=on)
  {
    QSignalBlocker block(m_clickAndBendAction);
    m_clickAndBendAction->setChecked(on);
  }
  statusBar()->showMessage(ViewMenuSpec::clickAndBendMessage(on),5000);
}

void PinglyMotionWindow::chooseViewPart(ViewToolSpec::Part part,bool shown)
{
  using Part=ViewToolSpec::Part;
  switch(part)
  {
    case Part::Floor:
      UserSettings::setShowFloor(shown);
      ui->animationView->setShowFloor(shown);
      break;
    case Part::Rings:
      UserSettings::setShowRings(shown);
      ui->animationView->setShowRings(shown);
      break;
    case Part::JointDots:
      // View > Joint dots (Options > Show skeleton before UX job U2)
      UserSettings::setShowJointDots(shown);
      ui->animationView->setShowJointDots(shown);
      break;
    case Part::ToolNames:
      UserSettings::setViewToolNames(shown);
      break;
  }
  if(m_viewToolbar) m_viewToolbar->setPartShown(part,shown);
  // the same item in View > Show (UX job U2) follows
  if(QAction* action=m_viewPartActions[int(part)])
  {
    QSignalBlocker block(action);
    action->setChecked(shown);
  }
  statusBar()->showMessage(ViewToolSpec::shownMessage(part,shown),5000);
}

void PinglyMotionWindow::focusPickedJoint()
{
  if(currentPart && ui->animationView->focusOnPickedJoint())
    statusBar()->showMessage(ViewToolSpec::tr(ViewToolSpec::focusDone).arg(jointDisplayName(currentPart->name())),5000);
  else
    statusBar()->showMessage(ViewToolSpec::tr(ViewToolSpec::focusNoJoint),5000);
}

void PinglyMotionWindow::showHoverHint(const QString& hint)
{
  if(hint.isEmpty() || !UserSettings::tipsOn())
  {
    // take away only our own hint, never another message
    if(!m_shownHoverHint.isEmpty() && statusBar()->currentMessage()==m_shownHoverHint) statusBar()->clearMessage();
    m_shownHoverHint.clear();
    return;
  }
  m_shownHoverHint=hint;
  statusBar()->showMessage(hint);
}

// a start choice of the empty state: the clip of the reference library, by the same route as
// the control command load_reference (checked against the index, then opened)
void PinglyMotionWindow::startFromLibrary(const QString& clipId)
{
  const QJsonObject answer=m_controlApi->execute(QJsonObject{{"command",QStringLiteral("load_reference")},{"id",clipId}});
  if(!answer.value(QStringLiteral("ok")).toBool())
  {
    statusBar()->showMessage(answer.value(QStringLiteral("error")).toString(),10000);
    return;
  }
  // the clip's credit stays visible, as its licence asks
  statusBar()->showMessage(answer.value(QStringLiteral("reference")).toObject().value(QStringLiteral("credit")).toString(),10000);
}

// Use this in the Library panel (docs/UX_DECISIONS.md 6l): with unsaved changes a small window
// asks first (Cancel, Open without saving, Save, then open); then the clip opens by the route
// of load_reference, and the status line says what was opened
void PinglyMotionWindow::useLibraryClip(const QString& clipId)
{
  const LibraryClip* clip=m_libraryPanel->index().find(clipId);
  if(!clip) return;
  bool unsaved=false;
  for(Animation* anim : animationIds) if(anim->dirty()) unsaved=true;
  if(unsaved)
  {
    QScopedPointer<QMessageBox> box(LibraryPanel::unsavedBox(this,clip->name));
    box->exec();
    const LibraryPanel::UnsavedChoice choice=LibraryPanel::choiceOf(box.data());
    if(choice==LibraryPanel::UnsavedChoice::Cancel) return;
    if(choice==LibraryPanel::UnsavedChoice::SaveThenOpen && !fileSave()) return;
    // the user chose to let the changes go (or they are saved now)
    for(Animation* anim : animationIds) anim->setDirty(false);
  }
  QString message;
  LibraryPanel::openClip(m_controlApi.data(),clipId,message);
  statusBar()->showMessage(message,10000);
}

// no animation open: the empty state covers the 3D view, the panels and the actions that need an
// animation are greyed with "Open an animation first."
void PinglyMotionWindow::setEmptyMode(bool empty)
{
  if(empty==m_empty) return;
  m_empty=empty;

  const QList<QAction*> needing={ui->fileSaveAction,ui->fileSaveAsAction,ui->fileExportAnimAction,
    ui->fileExportForSecondLifeAction,ui->fileCloseAction,ui->fileAddAction,ui->editCutAction,
    ui->editCopyAction,ui->editPasteAction};
  const QString why=QCoreApplication::translate("MenuSpec",MenuSpec::openFirst);
  for(QAction* action : needing)
  {
    if(empty)
    {
      m_beforeEmpty.insert(action,{action->isEnabled(),action->toolTip(),action->statusTip()});
      action->setEnabled(false);
      action->setToolTip(why);
      action->setStatusTip(why);
    }
    else if(m_beforeEmpty.contains(action))
    {
      const ActionState before=m_beforeEmpty.take(action);
      action->setEnabled(before.enabled);
      action->setToolTip(before.tip);
      action->setStatusTip(before.status);
    }
  }
  for(QDockWidget* dock : {m_jointsDock,m_jointCardDock,m_timelineDock})
    if(dock && dock->widget())
    {
      dock->widget()->setEnabled(!empty);
      // the reason line names the panel and says why it is greyed out (src/reasonline.h)
      dock->widget()->setProperty("reasonName",empty ? dock->windowTitle() : QString());
      dock->widget()->setProperty("reasonWhy",empty ? why : QString());
    }

  if(m_emptyState)
  {
    m_emptyState->setVisible(empty);
    m_emptyState->raise();
    if(empty) m_emptyState->choiceButtons().last()->setFocus();
  }
  if(m_topBar) m_topBar->setAnimationOpen(!empty);
  if(m_hintCards) m_hintCards->setAnimationOpen(!empty);
  if(m_helper) m_helper->setAnimationOpen(!empty);
  // the view toolbar has nothing to look at while the empty state covers the 3D view
  if(m_viewToolbar) m_viewToolbar->setVisible(!empty);
  // the timeline shows one line instead of its rows (UX phase 3a)
  ui->timelineView->setNoAnimation(empty);
  updateEditActions();
  // two figures (6m): nothing of it shows while the empty state covers the view
  refreshFigures();
  updateWindowTitle();
}

QList<QDockWidget*> PinglyMotionWindow::panelDocks() const
{
  return {m_jointsDock,m_jointCardDock,m_timelineDock,m_scriptLabDock,m_libraryDock};
}

// View > Reset layout: the default layout built in code, all three panels
// docked and shown. Panels hidden by Tab count as shown again.
void PinglyMotionWindow::resetPanelLayout()
{
  // the lesson step "Press Reset layout" goes on (job UX-C)
  if(m_helper) m_helper->notify(QLatin1String(HelperSpec::layoutReset));
  // hiding the Script Lab here is not a switch to Animate (UX-F)
  WorkMode::Switcher::Hold hold(m_workMode);
  m_applyingTab=true;
  m_tabState.reset();
  // Reset layout brings back all three panels in both modes, the Joints list on the left too
  // (WP-01 A1: after Expert mode, Beginner mode and Reset layout the left panel stayed closed).
  // Beginner mode's own default, with the Joints list closed, is only for a first start and for a
  // mode without a saved layout.
  PanelWindow::applyDefaultLayout(this,m_jointsDock,m_jointCardDock,m_timelineDock);
  // the Script Lab goes back to its side and closes, its tabs back in it (job UX-B)
  m_scriptLab->resetLayout();
  // the library too: a closed tab next to Selected joint
  m_libraryDock->setFloating(false);
  tabifyDockWidget(m_jointCardDock,m_libraryDock);
  m_libraryDock->hide();
  // this is the wide layout now; rearranged again if the window has no room for it
  m_reflower.wideLayoutChanged(this,reflowDocks());
  m_applyingTab=false;
  m_hidePanelsAction->setChecked(false);
  ui->optionsShowTimelineAction->setChecked(true);
  // both sides start again; on the Script side the Script Lab is rebuilt big and the rest closed
  m_workMode->layoutsReset();
  if(m_workMode->side()==WorkMode::Side::Script)
  {
    m_reflower.wideLayoutChanged(this,reflowDocks());
    ui->optionsShowTimelineAction->setChecked(!m_timelineDock->isHidden());
    updateBeginnerRow();
    statusBar()->showMessage(WorkMode::tr(WorkMode::resetScriptMessage),5000);
    return;
  }
  updateBeginnerRow();
  statusBar()->showMessage(UserSettings::expertMode()
                             ? tr("Layout reset. The panels are back where they were at the first start.")
                             : BeginnerView::tr(BeginnerView::resetBeginnerMessage),5000);
}

// The Joints list's button and Show curves and finger grid are shown in Beginner mode only; the
// first also only while the Joints list is closed (src/beginnerview.h)
void PinglyMotionWindow::updateBeginnerRow()
{
  if(!m_panels || !m_panels->beginnerRow) return;
  const bool expert=UserSettings::expertMode();
  m_panels->showJointsButton->setVisible(BeginnerView::showJointsButtonVisible(expert,!m_jointsDock->isHidden()));
  m_panels->showMoreButton->setVisible(BeginnerView::showMoreButtonVisible(expert));
  m_panels->beginnerRow->setVisible(!m_panels->showJointsButton->isHidden() || !m_panels->showMoreButton->isHidden());
}

// Beginner and Expert mode each keep their own panel layout (UX-E, the saving of UX-A): the
// layout of the mode that is left is put away, the one of the mode that starts is restored. A
// mode that has none yet keeps the panels where they are and only opens or closes the Joints
// list, so nothing else jumps. Answers true when the Joints list opened or closed.
bool PinglyMotionWindow::switchPanelLayoutForMode(bool expert)
{
  if(expert==UserSettings::expertMode()) return false;
  // on the Script side the layout is the Script side's own: no slot is touched, the Animate side
  // of the new mode comes back with the next switch (UX-F)
  if(m_workMode && m_workMode->side()==WorkMode::Side::Script) return false;
  if(m_tabState.isHidden()) togglePanelsHidden();
  UserSettings::setPanelLayoutFor(!expert,m_reflower.layoutToSave(this));
  UserSettings::setPanelLayoutVersion(PanelLayout::CurrentVersion);
  const bool jointsWereOpen=!m_jointsDock->isHidden();
  const QByteArray saved=UserSettings::panelLayoutFor(expert);
  m_applyingTab=true;
  if(PanelLayout::isLayoutUsable(saved,UserSettings::panelLayoutVersion(),size()))
  {
    ScriptLabPanel::setLayoutRestoring(true);   // a floating Script Lab keeps its saved place (UX-B)
    restoreState(saved,PanelLayout::CurrentVersion);
    ScriptLabPanel::setLayoutRestoring(false);
    PanelWindow::pullFloatingPanelsOntoScreen(panelDocks(),PanelWindow::screenRects());
    // the timeline stands visible, as at every start; the Library only opens when asked for
    PanelVisibility::showAtStart(m_timelineDock,ui->optionsShowTimelineAction);
    m_libraryDock->hide();
  }
  else
    m_jointsDock->setVisible(BeginnerView::jointsListInDefaultLayout(expert));
  m_reflower.wideLayoutChanged(this,reflowDocks());
  m_applyingTab=false;
  m_hidePanelsAction->setChecked(false);
  return jointsWereOpen!=!m_jointsDock->isHidden();
}

// Tab, or View > Hide panels: every open panel goes away (docked and
// floating); the next Tab brings back exactly those. With no panel open
// there is nothing to hide and nothing changes.
void PinglyMotionWindow::togglePanelsHidden()
{
  m_applyingTab=true;
  const bool hidden=PanelWindow::toggleHidden(m_tabState,panelDocks());
  m_applyingTab=false;
  m_hidePanelsAction->setChecked(hidden);
  if(!hidden) ui->optionsShowTimelineAction->setChecked(!m_timelineDock->isHidden());
}

// Options > Lighting...: three sliders that change the body light at once.
// The dialog stays open while you work; the values are saved with the
// other settings.
void PinglyMotionWindow::showLightDialog()
{
  if(QDialog* open=findChild<QDialog*>("lightDialog"))
  {
    open->show();
    open->raise();
    open->activateWindow();
    return;
  }

  QDialog* dialog=new QDialog(this);
  dialog->setObjectName("lightDialog");
  dialog->setWindowTitle(tr("Light"));
  QFormLayout* form=new QFormLayout(dialog);

  auto makeSlider=[dialog](int value,const QString& low,const QString& high)
  {
    QSlider* slider=new QSlider(Qt::Horizontal,dialog);
    slider->setRange(0,100);
    slider->setValue(value);
    slider->setMinimumWidth(220);
    QWidget* row=new QWidget(dialog);
    QHBoxLayout* layout=new QHBoxLayout(row);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(new QLabel(low,row));
    layout->addWidget(slider,1);
    layout->addWidget(new QLabel(high,row));
    return qMakePair(slider,row);
  };
  const auto brightness=makeSlider(UserSettings::lightBrightness(),tr("Dark"),tr("Bright"));
  const auto shine=makeSlider(UserSettings::lightShine(),tr("Matt"),tr("Shiny"));
  const auto warmth=makeSlider(UserSettings::lightWarmth(),tr("Cool"),tr("Warm"));
  form->addRow(tr("Brightness:"),brightness.second);
  form->addRow(tr("Shine:"),shine.second);
  form->addRow(tr("Warmth:"),warmth.second);
  QCheckBox* shadowBox=new QCheckBox(tr("Shadow on the floor"),dialog);
  shadowBox->setChecked(UserSettings::floorShadow());
  form->addRow(QString(),shadowBox);

  if(!ui->animationView->bodyLightAvailable())
  {
    QLabel* note=new QLabel(tr("The 3D view needs OpenGL 3.3, which this computer does not have."),dialog);
    note->setWordWrap(true);
    form->addRow(note);
    for(QSlider* slider : {brightness.first,shine.first,warmth.first}) slider->setEnabled(false);
    shadowBox->setEnabled(false);
  }

  QDialogButtonBox* buttons=new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Close,dialog);
  form->addRow(buttons);

  auto apply=[this,b=brightness.first,s=shine.first,w=warmth.first,shadowBox]()
  {
    UserSettings::setLightBrightness(b->value());
    UserSettings::setLightShine(s->value());
    UserSettings::setLightWarmth(w->value());
    UserSettings::setFloorShadow(shadowBox->isChecked());
    ui->animationView->setBodyLight(b->value(),s->value(),w->value(),shadowBox->isChecked());
  };
  for(QSlider* slider : {brightness.first,shine.first,warmth.first})
    connect(slider,&QSlider::valueChanged,dialog,apply);
  connect(shadowBox,&QCheckBox::toggled,dialog,apply);
  connect(buttons->button(QDialogButtonBox::Reset),&QPushButton::clicked,dialog,
          [b=brightness.first,s=shine.first,w=warmth.first,shadowBox]()
  {
    b->setValue(50);
    s->setValue(50);
    w->setValue(50);
    shadowBox->setChecked(true);
  });
  connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::hide);

  dialog->show();
}

// The Joints and Selected joint panels are as wide as their contents need -
// no more, no less. Called again when the theme changes, since that changes
// the widget sizes.
// When the panels rearrange (docs/UX_DECISIONS.md, 6c) they may be wider.
void PinglyMotionWindow::fitPanelWidths()
{
  m_panels->fit(m_reflower.mode());
}

PanelWindow::Docks PinglyMotionWindow::reflowDocks() const
{
  PanelWindow::Docks docks;
  docks.joints=m_jointsDock;
  docks.jointCard=m_jointCardDock;
  docks.timeline=m_timelineDock;
  docks.scriptLab=m_scriptLabDock;
  docks.library=m_libraryDock;
  return docks;
}

void PinglyMotionWindow::scheduleReflow()
{
  if(m_reflowPending) return;
  m_reflowPending=true;
  QTimer::singleShot(0,this,[this]() { m_reflowPending=false; updateReflow(); });
}

void PinglyMotionWindow::updateReflow()
{
  // a maximised or full-screen window may still be on its way to its size
  if(!isVisible() || isMinimized()) return;
  const PanelLayout::ReflowNeeds needs=m_panels->needs(this);
  const PanelLayout::Reflow next=PanelLayout::chooseReflow(width(),needs);
  const PanelLayout::Reflow was=m_reflower.mode();
  // the timeline's rows break into shorter lines when they do not fit on one
  m_panels->setTimelineWrapped(PanelLayout::wrapTimeline(next,width(),needs));
  if(next==was) return;
  qCInfo(lcView,"panels: %s layout for %d px (wide needs %d, medium %d) at %d percent text",
         PanelLayout::reflowName(next),width(),PanelLayout::wideNeed(needs),PanelLayout::mediumNeed(needs),
         TextSize::percent());
  m_applyingTab=true;
  m_reflower.setMode(this,reflowDocks(),next);
  // fixed widths in their own columns, loose when they share a place
  fitPanelWidths();
  m_applyingTab=false;
}

void PinglyMotionWindow::resizeEvent(QResizeEvent* event)
{
  QMainWindow::resizeEvent(event);
  scheduleReflow();
}

void PinglyMotionWindow::changeEvent(QEvent* event)
{
  QMainWindow::changeEvent(event);
  if(event->type()==QEvent::FontChange)
  {
    // the text size changed. A widget with a style sheet keeps the font it
    // had when the sheet was applied, so the sheet is applied again.
    for(QWidget* widget : findChildren<QWidget*>())
    {
      const QString sheet=widget->styleSheet();
      if(sheet.isEmpty()) continue;
      widget->setStyleSheet(QString());
      widget->setStyleSheet(sheet);
    }
    // rows and panels as big as the new text needs
    m_jointTable->verticalHeader()->setDefaultSectionSize(m_jointTable->fontMetrics().height()+6);
    m_jointTable->setMinimumHeight(10*m_jointTable->verticalHeader()->defaultSectionSize());

    fitPanelWidths();
    scheduleReflow();
  }
  else if(event->type()==QEvent::WindowStateChange)
    scheduleReflow();
}

void PinglyMotionWindow::setTextSizePercent(int percent)
{
  UserSettings::setTextSizePercent(percent);
  // the panels' sizes at the old text size, so Ctrl+0 brings them back exactly (UX small fixes)
  const QList<DialogFit::PanelSize> panels=DialogFit::measurePanels(this,panelDocks());
  // every widget gets a font change event; this window's changeEvent fits the panels
  TextSize::apply(UserSettings::textSizePercent());
  if(m_textSizeAction)
    m_textSizeAction->setText(MenuSpec::text(MenuSpec::viewTextSize).arg(UserSettings::textSizePercent()));
  // rearranged at once, before the window is laid out for the new size, so a
  // layout that no longer fits never makes the window grow
  updateReflow();
  // then each panel gets its new smallest size plus the room the user gave it: a panel pushed
  // wider at 200 percent is as wide again as before when the text goes back (DialogFit::followTextSize)
  QTimer::singleShot(0,this,[this,panels]() { DialogFit::followTextSize(this,panels); });
}

// UX phase 1d: Setup, reachable from the gear button in the top bar, View > Setup and Help >
// Setup. Every choice the dialog emits is applied through the same action or method the window's
// own menu items use, so the window and the dialog never disagree (docs/UX_DECISIONS.md, 6c).
void PinglyMotionWindow::openSetupDialog()
{
  SetupDialog dialog(this);
  connect(&dialog,&SetupDialog::expertModeRequested,this,[this](bool expert)
  {
    if(m_expertModeAction) m_expertModeAction->setChecked(expert);
  });
  connect(&dialog,&SetupDialog::darkThemeRequested,this,[this](bool dark)
  {
    if(m_lightThemeAction) m_lightThemeAction->setChecked(!dark);
  });
  connect(&dialog,&SetupDialog::textSizeRequested,this,[this](int percent)
  {
    setTextSizePercent(percent);
  });
  connect(&dialog,&SetupDialog::hintsRequested,this,[this](bool on)
  {
    if(m_hintsAction) m_hintsAction->setChecked(on);
  });
  // the Script Lab editor follows these through UserSettings::watchScriptEditor (lane 1, F2 and F3)
  connect(&dialog,&SetupDialog::suggestOnRequestRequested,this,[](bool on) { UserSettings::setSuggestOnRequestOnly(on); });
  connect(&dialog,&SetupDialog::luaSwitchRequested,this,[](bool on) { UserSettings::setLuaSwitch(on); });
  connect(&dialog,&SetupDialog::cameraHabitRequested,this,&PinglyMotionWindow::chooseCameraHabit);
  connect(&dialog,&SetupDialog::showCameraKeysRequested,this,&PinglyMotionWindow::openCameraKeysDialog);
  // AI Assistant and Aurora link (docs/UX_DECISIONS.md 6h): the same code as the status bar
  // switches. After every request, and whenever the real state changes, the dialog shows the real
  // state again (a failed start leaves the switch off, with the warning from setRemoteControl).
  const auto showConnections=[this,&dialog]()
  {
    dialog.setAiAssistant(m_controlServer->running(),m_controlIndicator->text());
    dialog.setAuroraLink(m_auroraLink->enabled(),m_auroraIndicator->text());
  };
  showConnections();
  connect(&dialog,&SetupDialog::aiAssistantRequested,&dialog,[this,showConnections](bool on)
  {
    setRemoteControl(on);
    showConnections();
  });
  connect(&dialog,&SetupDialog::auroraLinkRequested,&dialog,[this,showConnections](bool on)
  {
    if(m_auroraAction) m_auroraAction->setChecked(on);
    showConnections();
  });
  connect(m_auroraLink,&AuroraLink::connectedChanged,&dialog,[showConnections](bool){ showConnections(); });
  connect(m_controlServer.data(),&ControlServer::contact,&dialog,[showConnections](){ showConnections(); });
  connect(m_controlServer.data(),&ControlServer::refused,&dialog,[showConnections](){ showConnections(); });
  bool welcomeAgain=false;
  connect(&dialog,&SetupDialog::welcomeRequested,this,[&dialog,&welcomeAgain]() { welcomeAgain=true; dialog.accept(); });
  dialog.exec();
  // the welcome card opens after Setup has closed, never on top of it
  if(welcomeAgain) showWelcomeCard();
}

// Help > Camera and keys..., and the Setup dialog's "Show all mouse and keys" button
void PinglyMotionWindow::openCameraKeysDialog()
{
  CameraKeysDialog dialog(this);
  connect(&dialog,&CameraKeysDialog::cameraHabitRequested,this,&PinglyMotionWindow::chooseCameraHabit);
  dialog.exec();
}

void PinglyMotionWindow::chooseCameraHabit(const QString& habit)
{
  UserSettings::setCameraHabit(habit);
  ui->animationView->setCameraHabit(Camera::habitFor(UserSettings::cameraHabit()));
  statusBar()->showMessage(CameraHabitSpec::chosenMessage(CameraHabitSpec::find(UserSettings::cameraHabit())),5000);
}

// Help > Keys and mouse (?)
void PinglyMotionWindow::openKeysMouseDialog()
{
  KeysMouseDialog dialog(this);
  dialog.exec();
}


void PinglyMotionWindow::easingChoiceClicked(Easing::Preset preset)
{
  Animation* anim=scene->getAnimation();
  if(!anim || !currentPart) return;
  const int frame=anim->getFrame();
  int startFrame=0;
  if(!EasingChoice::segmentEndingAt(currentPart,frame,startFrame)) return;
  EasingChoice::setChoice(anim,currentPart,startFrame,frame,preset);
  updateKeyBtn();
}

// ------ Menu Action Slots (Callbacks) -----------
























// Menu Action: Edit / Cut: copy the stretch, then take its keys away (frame 0 keeps its key)
void PinglyMotionWindow::editCut()
{
  Animation* anim=scene->getAnimation();
  if(m_empty || !anim || !ui->editCutAction->isEnabled()) return;
  const TimelineRange range=ui->timelineView->getTimeline()->copyRange();
  if(range.isEmpty()) return;
  m_clip.copy(anim,range);
  TimelineClip::removeKeys(anim,range);
  ui->animationView->update();
  updateInputs();
  statusBar()->showMessage(ClipSpec::copyMessage(range,true),5000);
}

// Menu Action: Edit / Copy
void PinglyMotionWindow::editCopy()
{
  Animation* anim=scene->getAnimation();
  if(m_empty || !anim || !ui->editCopyAction->isEnabled()) return;
  const TimelineRange range=ui->timelineView->getTimeline()->copyRange();
  if(range.isEmpty()) return;
  m_clip.copy(anim,range);
  updateEditActions();
  statusBar()->showMessage(ClipSpec::copyMessage(range,false),5000);
}

// Menu Action: Edit / Paste and Edit / Paste mirrored: the copied stretch from the current frame on
void PinglyMotionWindow::pasteClip(bool mirrored)
{
  Animation* anim=scene->getAnimation();
  if(m_empty || !anim) return;
  if(!ClipSpec::pasteBlocked(true,m_clip.isEmpty(),playstate!=PLAYSTATE_STOPPED,protect,anim->getFrame()).isEmpty()) return;
  const int frame=anim->getFrame();
  const TimelineClip::PasteResult result=m_clip.paste(anim,frame,mirrored);
  ui->animationView->update();
  updateInputs();
  updateKeyBtn();
  statusBar()->showMessage(ClipSpec::pasteMessage(result,frame,mirrored),8000);
}

void PinglyMotionWindow::editPaste()
{
  pasteClip(false);
}

void PinglyMotionWindow::pasteMirroredFromMenu()
{
  pasteClip(true);
}

// Menu Action: Options / Loop
void PinglyMotionWindow::setLoop(bool on)
{
  UserSettings::setLoop(on);
  // the Loop switch under the timeline and Options > Loop playback show the same
  m_transport->setLoop(on);

  // while playing: switched off, play to the last frame and stop; switched
  // on, go on repeating
  Animation* anim=scene->getAnimation();
  if(!anim) return;
  const PlayState state=Playback::loopSwitched(playstate,on,anim->getFrame(),anim->getLoopOutPoint());
  if(state!=playstate)
  {
    setPlaystate(state);
    anim->setPlaystate(state);
  }
}

// Menu Action: Options / Stop joints at their furthest turn
void PinglyMotionWindow::setJointLimits(bool on)
{
  UserSettings::setJointLimits(on);
  Animation* anim=scene->getAnimation();
  if(anim)
  {
    ui->animationView->update();
    updateInputs();
  }
}

// Menu Action: Options / Lock the first frame
void PinglyMotionWindow::setProtectFirstFrame(bool on)
{
  UserSettings::setProtectFirstFrame(on);
  if(on && ui->currentFrameSlider->value()==0) protect=true;
  else protect=false;

  emit protectFrame(protect);
  updateInputs();
}

// Menu Action: View / Fog
void PinglyMotionWindow::setFog(bool on)
{
  UserSettings::setFog(on);
  ui->animationView->update();
}

// Menu Action: View / Floor transparency
void PinglyMotionWindow::setFloorTransparency(int percent)
{
  UserSettings::setFloorTranslucency(percent);
  updateFloorTransparencyChecks();
  ui->animationView->update();
}

// the check mark goes to the choice nearest to the stored value (an old value that is not in the list)
void PinglyMotionWindow::updateFloorTransparencyChecks()
{
  if(!m_floorTransparencyMenu) return;
  const int now=UserSettings::floorTranslucency();
  int best=ViewMenuSpec::floorTransparencyChoices[0];
  for(int percent : ViewMenuSpec::floorTransparencyChoices)
    if(std::abs(percent-now)<std::abs(best-now)) best=percent;
  for(QAction* choice : m_floorTransparencyMenu->actions())
    choice->setChecked(choice->data().toInt()==best);
}

// Menu Action: View / Timeline
void PinglyMotionWindow::showTimeline(bool on)
{
  m_timelineDock->setVisible(on);
}

// The old Settings window. No menu entry opens it any more (docs/UX_DECISIONS.md 6h); the code stays
// so nothing already built is lost.
void PinglyMotionWindow::configure()
{
  UserSettingsDialog* dialog=new UserSettingsDialog(this);
  connect(dialog, &UserSettingsDialog::configChanged, this, &PinglyMotionWindow::configChanged);

  dialog->exec();

  delete dialog;
}

void PinglyMotionWindow::configChanged()
{
  ui->animationView->update();
}

// Menu Action: Help / About ...
void PinglyMotionWindow::helpAbout()
{
  QScopedPointer<AboutDialog> dialog(new AboutDialog(this));
  connect(dialog.data(), &AboutDialog::checkUpdates,
          &updateChecker, &UpdateChecker::checkUpdates);
  connect(&updateChecker, &UpdateChecker::updateCheckFinished,
          dialog.data(), &AboutDialog::updateCheckFinished);
  dialog->exec();
}

void PinglyMotionWindow::helpWhatsNew()
{
  updateChecker.showNews();
}

