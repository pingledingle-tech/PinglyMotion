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

// Part of PinglyMotionWindow: building the panels, menus, top bar, hint cards, helper and view toolbar.
// (Split out of pinglymotionwindow.cpp; nothing here changes what the window does.)

// The 3D view fills the window. Three panels (dock widgets) stand around it
// (docs/archive/PANELS_PLAN.md, job P2): Joints on the left, Selected joint on the
// right, Timeline at the bottom. Each can be resized, moved to another side,
// put as a tab on another panel, or torn off into its own window. A panel
// only goes where its shape fits: the timeline is wide (top or bottom), the
// other two are tall (left or right).
void PinglyMotionWindow::buildDocks()
{
  // GroupedDragging (job P6): panels docked together on the same side, or
  // tabbed together, move together while one of them is dragged, instead of
  // leaving the others behind.
  setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::GroupedDragging);

  // the three panels and the 3D view, from the designer file (src/panelcontent.h)
  m_panels.reset(new MainPanels);
  m_panels->build(this,ui.data());
  m_timelineDock=m_panels->timelineDock;
  m_jointsDock=m_panels->jointsDock;
  m_jointCardDock=m_panels->jointCardDock;
  m_jointsScroll=m_panels->jointsScroll;
  m_jointCardScroll=m_panels->jointCardScroll;

  // the Selected joint panel (UX phase 2b): the limits switch is the same
  // setting as Options > Stop joints at their furthest turn
  m_selectedJoint=m_panels->selectedJoint;
  // the Selected prop page and the prop handles in the 3D view (Mockup 2, pinglymotionwindowprops.cpp)
  setupPropEditing();
  connect(m_selectedJoint,&SelectedJointPanel::limitsToggled,ui->optionsJointLimitsAction,&QAction::setChecked);
  connect(m_selectedJoint,&SelectedJointPanel::resetRequested,this,&PinglyMotionWindow::resetJointClicked);
  connect(m_selectedJoint,&SelectedJointPanel::pickDemoRequested,this,&PinglyMotionWindow::pickDemoJoint);
  // UX phase 4b part 2: Pull with keys, and the drag choice says what a drag does now
  connect(m_selectedJoint,&SelectedJointPanel::pullKeysRequested,this,[this]() { ui->animationView->startKeyPull(false); });
  connect(m_selectedJoint,&SelectedJointPanel::dragChoiceChanged,this,[this](bool pullLimb)
  {
    statusBar()->showMessage(SelectedJointSpec::tr2(pullLimb ? SelectedJointSpec::dragChosenPull : SelectedJointSpec::dragChosenTurn),5000);
  });

  // Start, Stop, Replay from start, Frame back, Frame forward and Loop
  // (docs/UX_DECISIONS.md section 1)
  m_transport=m_panels->transport;
  connect(m_transport,&TransportBar::startClicked,this,&PinglyMotionWindow::startPlaying);
  connect(m_transport,&TransportBar::stopClicked,this,&PinglyMotionWindow::stopPlaying);
  connect(m_transport,&TransportBar::replayClicked,this,&PinglyMotionWindow::replayFromStart);
  connect(m_transport,&TransportBar::backClicked,this,[this]() { stepFrame(-1); });
  connect(m_transport,&TransportBar::forwardClicked,this,[this]() { stepFrame(1); });
  connect(m_transport,&TransportBar::loopToggled,ui->optionsLoopAction,&QAction::setChecked);

  // the timeline (UX phase 3a): "Set the first key" does what Set key does, and
  // the rows show the joints by the names of the Joints list
  connect(ui->timelineView,&TimelineView::setKeyRequested,this,&PinglyMotionWindow::setKeyFromMenu);
  ui->timelineView->setNameOf([this](const QString& fileName) { return jointDisplayName(fileName); });
  // UX phase 3b: the selected stretch says what it holds, and Copy and Cut use it;
  // Paste mirrored beside Set key (Expert mode only, as in the mockup)
  connect(ui->timelineView->getTimeline(),&Timeline::selectionChanged,this,[this]()
  {
    const QString message=ClipSpec::selectedMessage(ui->timelineView->getTimeline()->selection());
    if(!message.isEmpty()) statusBar()->showMessage(message,5000);
    updateEditActions();
  });
  connect(m_panels->pasteMirrored,&QPushButton::clicked,this,&PinglyMotionWindow::pasteMirroredFromMenu);
  m_panels->pasteMirrored->setVisible(UserSettings::expertMode());

  // the default layout is built in code (src/panelwindow.h), the same one
  // View > Reset layout brings back
  PanelWindow::applyDefaultLayout(this,m_jointsDock,m_jointCardDock,m_timelineDock);

  // --- Script Lab panel (issue 288): RLV commands and what the avatar may
  // still do. Hidden until View > Script Lab; place, size and visibility
  // are saved with the layout like the other panels. The controller makes it
  // and connects it to the rest of the window (job R5, src/scriptlabcontroller.h).
  m_scriptLab=new ScriptLabController(this);
  m_scriptLabPanel=m_scriptLab->panel();
  m_scriptLabDock=m_scriptLab->dock();

  // --- Library panel (roadmap item 6, issue 19; docs/UX_DECISIONS.md 6l):
  // ready-made moves to start from. Hidden until View > Library; a tab next
  // to Selected joint on the right; saved with the layout like the others.
  m_libraryPanel=new LibraryPanel;
  m_libraryPanel->setLibraryFolder(ControlApi::defaultLibraryFolder());
  m_libraryPanel->setHintsOn(UserSettings::tipsOn());
  m_libraryDock=LibraryPanel::makeDock(this,m_libraryPanel);
  tabifyDockWidget(m_jointCardDock,m_libraryDock);
  m_jointCardDock->raise();
  connect(m_libraryPanel,&LibraryPanel::useRequested,this,&PinglyMotionWindow::useLibraryClip);
  connect(m_libraryPanel,&LibraryPanel::statusMessage,this,[this](const QString& text) { statusBar()->showMessage(text,5000); });
  // opening brings the panel to the front of its side and puts the keyboard in the search field
  connect(m_libraryDock->toggleViewAction(),&QAction::triggered,this,[this](bool on)
  {
    if(on)
    {
      m_libraryDock->raise();
      m_libraryPanel->focusSearch();
    }
    statusBar()->showMessage(LibrarySpec::tr(on ? LibrarySpec::openedPanel : LibrarySpec::closedPanel),5000);
  });

  // View menu: a tick for each panel, then Hide panels (Tab) and Reset
  // layout. The Timeline item comes from the designer file; the other two
  // are the panels' own.
  QAction* showJointsAction=m_jointsDock->toggleViewAction();
  showJointsAction->setObjectName("viewJointsAction");
  showJointsAction->setText(tr("Joints"));
  QAction* showJointCardAction=m_jointCardDock->toggleViewAction();
  showJointCardAction->setObjectName("viewJointCardAction");
  showJointCardAction->setText(tr("Selected joint"));
  ui->menuView->insertAction(ui->optionsShowTimelineAction,showJointsAction);
  ui->menuView->insertAction(ui->optionsShowTimelineAction,showJointCardAction);
  // closing the timeline with its own button unticks Timeline (closing the window does not)
  PanelVisibility::linkMenuItem(this,m_timelineDock,ui->optionsShowTimelineAction);
  ui->menuView->addAction(m_scriptLabDock->toggleViewAction());

  ui->menuView->addSeparator();
  // Hide panels (job P4): Tab while the 3D view has the keys. The shortcut
  // is tied to the 3D view only (Qt::WidgetShortcut), so Tab still moves
  // between the fields of the panels and works in the search box. The item
  // is ticked while the panels are hidden.
  m_hidePanelsAction=new QAction(tr("Hide panels"),this);
  m_hidePanelsAction->setObjectName("viewHidePanelsAction");
  m_hidePanelsAction->setCheckable(true);
  m_hidePanelsAction->setToolTip(tr("Hide all panels to see only the figure. Tab again brings them back."));
  m_hidePanelsAction->setShortcut(QKeySequence(Qt::Key_Tab));
  m_hidePanelsAction->setShortcutContext(Qt::WidgetShortcut);
  ui->animationView->addAction(m_hidePanelsAction);
  ui->menuView->addAction(m_hidePanelsAction);
  connect(m_hidePanelsAction,&QAction::triggered,this,&PinglyMotionWindow::togglePanelsHidden);
  // Tab that reaches the 3D view as a key (the shortcut did not take it)
  connect(ui->animationView,&AnimationView::tabPressed,this,&PinglyMotionWindow::togglePanelsHidden);
  // a panel opened from the View menu while hidden ends the hidden state;
  // a panel closed while hidden stays closed on the next Tab
  for(QDockWidget* dock : panelDocks())
  {
    const QString name=dock->objectName();
    connect(dock->toggleViewAction(),&QAction::toggled,this,[this,name](bool on)
    {
      if(m_applyingTab || !m_tabState.isHidden()) return;
      if(on) m_tabState.panelOpened(name);
      else m_tabState.panelClosed(name);
      m_hidePanelsAction->setChecked(m_tabState.isHidden());
    });
  }

  QAction* resetLayoutAction=new QAction(tr("Reset layout"),this);
  resetLayoutAction->setObjectName("viewResetLayoutAction");
  resetLayoutAction->setToolTip(tr("Put the panels back where they were at the first start."));
  ui->menuView->addAction(resetLayoutAction);
  connect(resetLayoutAction,&QAction::triggered,this,&PinglyMotionWindow::resetPanelLayout);
}

namespace
{
  // an item from tests/menutexts' table: its name, text and, when it is not
  // built yet, the reason it is greyed (tip and status line). keyAsLabel puts
  // the key after a tab, so the menu shows it but no shortcut is made (the
  // 3D view already takes the number pad itself, and the rest has no action yet).
  QAction* specAction(QObject* parent,const MenuSpec::Item& item,bool keyAsLabel,const QString& shownText=QString())
  {
    QAction* action=new QAction(parent);
    action->setObjectName(QLatin1String(item.id));
    const QString text=shownText.isEmpty() ? MenuSpec::text(item) : shownText;
    action->setText(keyAsLabel ? MenuSpec::label(item,text) : text);
    if(!keyAsLabel && item.key[0]) action->setShortcut(QKeySequence(QLatin1String(item.key)));
    if(item.reason[0])
    {
      const QString reason=MenuSpec::reason(item);
      action->setEnabled(false);
      action->setToolTip(reason);
      action->setStatusTip(reason);
    }
    return action;
  }

  // a key that works while the 3D view has the keys, like Tab for Hide panels,
  // so it never takes a letter typed into a field
  void keyInView(QAction* action,AnimationView* view)
  {
    action->setShortcutContext(Qt::WidgetShortcut);
    view->addAction(action);
  }
}

// UX phase 1a: File, View and Help as in the mockup (docs/mockup/index.html).
// The texts, keys and reasons are in src/menuspec.h; tests/menutexts checks them.
void PinglyMotionWindow::buildMenus()
{
  using namespace MenuSpec;
  AnimationView* view=ui->animationView;

  // File: the items that come from the designer file get their tip here
  ui->menuFile->setToolTipsVisible(true);

  // View, rebuilt in the mockup's order
  QAction* showJointsAction=m_jointsDock->toggleViewAction();
  QAction* showJointCardAction=m_jointCardDock->toggleViewAction();
  QAction* scriptLabAction=m_scriptLabDock->toggleViewAction();
  scriptLabAction->setObjectName(QLatin1String(viewScriptLab.id));
  scriptLabAction->setShortcut(QKeySequence(QLatin1String(viewScriptLab.key)));
  QAction* resetLayoutAction=findChild<QAction*>(QLatin1String(viewResetLayout.id));
  ui->menuView->clear();
  ui->menuView->setToolTipsVisible(true);

  ui->menuView->addSection(QCoreApplication::translate("MenuSpec",lookFromHeading));
  struct Look { const Item* item; float height; float angle; };
  for(const Look& look : {Look{&viewFront,0.0f,0.0f},Look{&viewSide,0.0f,90.0f},Look{&viewTop,90.0f,0.0f}})
  {
    QAction* action=specAction(this,*look.item,true);
    const float height=look.height,angle=look.angle;
    connect(action,&QAction::triggered,this,[this,height,angle]()
    {
      ui->animationView->camera()->setView(height,angle);
      ui->animationView->update();
    });
    ui->menuView->addAction(action);
  }
  QAction* homeAction=specAction(this,viewHome,false);
  keyInView(homeAction,view);
  connect(homeAction,&QAction::triggered,this,&PinglyMotionWindow::on_resetCameraAction_triggered);
  ui->menuView->addAction(homeAction);

  // UX job U2: every view setting in the View menu (src/viewmenuspec.h): the view toolbar's
  // Show items (the same settings), Click and bend, Joint groups, Body and Lighting
  ui->menuView->addSection(ViewMenuSpec::tr(ViewMenuSpec::showHeading));
  for(ViewToolSpec::Part part : ViewToolSpec::parts)
  {
    const Item& item=*ViewMenuSpec::partItems[int(part)];
    QAction* action=specAction(this,item,false);
    action->setCheckable(true);
    if(part==ViewToolSpec::Part::JointDots)
    {
      action->setToolTip(ViewMenuSpec::tr(ViewMenuSpec::jointDotsTip));
      action->setStatusTip(ViewMenuSpec::tr(ViewMenuSpec::jointDotsTip));
    }
    action->setChecked(viewPartShown(part));
    connect(action,&QAction::toggled,this,[this,part](bool on) { chooseViewPart(part,on); });
    m_viewPartActions[int(part)]=action;
    ui->menuView->addAction(action);
  }
  m_clickAndBendAction=specAction(this,ViewMenuSpec::viewClickAndBend,false);
  m_clickAndBendAction->setCheckable(true);
  m_clickAndBendAction->setChecked(UserSettings::clickAndBend());
  m_clickAndBendAction->setToolTip(ViewMenuSpec::tr(ViewMenuSpec::clickAndBendTip));
  m_clickAndBendAction->setStatusTip(ViewMenuSpec::tr(ViewMenuSpec::clickAndBendTip));
  keyInView(m_clickAndBendAction,view);
  connect(m_clickAndBendAction,&QAction::toggled,this,&PinglyMotionWindow::setClickAndBend);
  ui->menuView->addAction(m_clickAndBendAction);
  if(m_jointGroupsMenu) ui->menuView->addMenu(m_jointGroupsMenu);
  if(m_bodyMenu) ui->menuView->addMenu(m_bodyMenu);
  if(m_lightingAction) ui->menuView->addAction(m_lightingAction);
  // Fog and the floor's transparency (from the old Settings window); fog is off at the start
  m_fogAction=specAction(this,ViewMenuSpec::viewFog,false);
  m_fogAction->setCheckable(true);
  m_fogAction->setChecked(UserSettings::fog());
  m_fogAction->setToolTip(ViewMenuSpec::tr(ViewMenuSpec::fogTip));
  m_fogAction->setStatusTip(ViewMenuSpec::tr(ViewMenuSpec::fogTip));
  connect(m_fogAction,&QAction::toggled,this,&PinglyMotionWindow::setFog);
  ui->menuView->addAction(m_fogAction);
  m_floorTransparencyMenu=new QMenu(ViewMenuSpec::tr(ViewMenuSpec::floorTransparencyMenu),this);
  m_floorTransparencyMenu->setObjectName(QStringLiteral("viewFloorTransparencyMenu"));
  m_floorTransparencyMenu->setToolTipsVisible(true);
  m_floorTransparencyMenu->menuAction()->setToolTip(ViewMenuSpec::tr(ViewMenuSpec::floorTransparencyTip));
  {
    QActionGroup* group=new QActionGroup(this);
    for(int percent : ViewMenuSpec::floorTransparencyChoices)
    {
      QAction* choice=m_floorTransparencyMenu->addAction(tr("%1%").arg(percent));
      choice->setObjectName(QStringLiteral("viewFloorTransparency%1Action").arg(percent));
      choice->setCheckable(true);
      choice->setData(percent);
      choice->setActionGroup(group);
      connect(choice,&QAction::triggered,this,[this,percent](){ setFloorTransparency(percent); });
    }
  }
  updateFloorTransparencyChecks();
  ui->menuView->addMenu(m_floorTransparencyMenu);

  ui->menuView->addSection(QCoreApplication::translate("MenuSpec",panelsHeading));
  ui->menuView->addAction(showJointsAction);
  ui->menuView->addAction(showJointCardAction);
  ui->menuView->addAction(ui->optionsShowTimelineAction);
  ui->menuView->addAction(scriptLabAction);
  ui->menuView->addAction(m_libraryDock->toggleViewAction());
  ui->menuView->addAction(m_hidePanelsAction);
  if(resetLayoutAction) ui->menuView->addAction(resetLayoutAction);
  ui->menuView->addAction(specAction(this,viewNextArea,true));

  ui->menuView->addSeparator();
  QAction* lightAction=specAction(this,viewLightTheme,false);
  lightAction->setCheckable(true);
  lightAction->setChecked(!UserSettings::darkTheme());
  keyInView(lightAction,view);
  m_lightThemeAction=lightAction;
  connect(lightAction,&QAction::toggled,this,[this](bool light)
  {
    UserSettings::setDarkTheme(!light);
    Theme::apply(!light);
    if(m_topBar) m_topBar->setDark(!light);
    if(m_workMode) m_workMode->applyLook();
    if(m_helper) m_helper->applyLook();
    if(m_emptyState) m_emptyState->applyLook();
    if(m_viewToolbar) m_viewToolbar->applyLook();
    if(m_reasonLine) m_reasonLine->applyLook();
    if(m_selectedJoint) m_selectedJoint->applyLook();
    if(m_handsPanel) m_handsPanel->applyLook();
    fitPanelWidths();
    fillJointTable();
    ui->animationView->update();
  });
  ui->menuView->addAction(lightAction);

  m_expertModeAction=specAction(this,viewExpertMode,false);
  m_expertModeAction->setCheckable(true);
  m_expertModeAction->setChecked(UserSettings::expertMode());
  keyInView(m_expertModeAction,view);
  connect(m_expertModeAction,&QAction::toggled,this,[this](bool on)
  {
    // the panels of the mode we leave are put away, those of the new mode come back (UX-E)
    const bool layoutChanged=switchPanelLayoutForMode(on);
    UserSettings::setExpertMode(on);
    updateBeginnerRow();
    // the timeline's Paste mirrored button is an Expert mode control (UX phase 3b, the mockup)
    if(m_panels && m_panels->pasteMirrored) m_panels->pasteMirrored->setVisible(on);
    // and the Hands tab's joint grid (UX phase 2c)
    refreshHandsPanel();
    // the timeline's Only changed joints switch (UX job U2)
    ui->timelineView->setExpert(on);
    // two figures (6m): the add commands, the Figures box, Make active and Remove
    refreshFigures();
    // the Edit curve button follows through UserSettings::watchExpertMode; the mockup's sentences
    QString message=on ? GuidanceSpec::tr(GuidanceSpec::expertOnMessage,"GuidanceSpec")
                       : GuidanceSpec::tr(GuidanceSpec::expertOffMessage,"GuidanceSpec");
    // P12: the panels that opened or closed are named, so nothing changes without a word
    if(layoutChanged) message+=QLatin1Char(' ')+BeginnerView::tr(on ? BeginnerView::expertLayoutMessage
                                                                    : BeginnerView::beginnerLayoutMessage);
    statusBar()->showMessage(message,8000);
  });
  ui->menuView->addAction(m_expertModeAction);
  // Beginner mode's two buttons above the Selected joint panel (UX-E)
  connect(m_panels->showJointsButton,&QPushButton::clicked,this,[this]()
  {
    m_jointsDock->show();
    m_jointsDock->raise();
    statusBar()->showMessage(tr("The Joints list is open. View > Joints closes it."),5000);
  });
  connect(m_panels->showMoreButton,&QPushButton::clicked,this,[this]()
  {
    if(m_expertModeAction) m_expertModeAction->setChecked(true);
  });
  connect(m_jointsDock,&QDockWidget::visibilityChanged,this,[this](bool){ updateBeginnerRow(); });
  updateBeginnerRow();

  // UX job U2: the timeline's row filter, as it was left last time
  ui->timelineView->setExpert(UserSettings::expertMode());
  ui->timelineView->setRowFilter(TimelineSpec::areaFromKey(UserSettings::timelineArea()),UserSettings::timelineChangedOnly());
  connect(ui->timelineView,&TimelineView::rowFilterChosen,this,[this](int area,bool changedOnly)
  {
    const bool areaChanged=TimelineSpec::areaKey(area)!=UserSettings::timelineArea();
    UserSettings::setTimelineArea(TimelineSpec::areaKey(area));
    UserSettings::setTimelineChangedOnly(changedOnly);
    statusBar()->showMessage(areaChanged ? TimelineSpec::areaMessage(area)
                                         : TimelineSpec::changedOnlyMessage(changedOnly),5000);
  });

  m_textSizeAction=specAction(this,viewTextSize,false,text(viewTextSize).arg(UserSettings::textSizePercent()));
  // every keyboard layout's keys (TextSize::biggerKeys: Ctrl and the plus key next to 0 on a
  // Nordic layout, Ctrl+= on a US layout, the number pad), the menu shows the first
  m_textSizeAction->setShortcuts(TextSize::biggerKeys());
  connect(m_textSizeAction,&QAction::triggered,this,[this]()
  {
    setTextSizePercent(TextSize::stepUp(UserSettings::textSizePercent()));
  });
  ui->menuView->addAction(m_textSizeAction);
  QAction* textSmallerAction=specAction(this,viewTextSmaller,false);
  textSmallerAction->setShortcuts(TextSize::smallerKeys());
  connect(textSmallerAction,&QAction::triggered,this,[this]()
  {
    setTextSizePercent(TextSize::stepDown(UserSettings::textSizePercent()));
  });
  ui->menuView->addAction(textSmallerAction);
  QAction* textResetAction=specAction(this,viewTextReset,false);
  textResetAction->setShortcuts(TextSize::resetKeys());
  connect(textResetAction,&QAction::triggered,this,[this]() { setTextSizePercent(TextSize::Default); });
  ui->menuView->addAction(textResetAction);
  QAction* viewSetupAction=specAction(this,viewSetup,false);
  connect(viewSetupAction,&QAction::triggered,this,&PinglyMotionWindow::openSetupDialog);
  ui->menuView->addAction(viewSetupAction);

  // Help
  ui->menuHelp->setToolTipsVisible(true);
  QAction* whatsNew=ui->helpWhatsNewAction;
  ui->menuHelp->clear();
  QAction* helpSetupAction=specAction(this,helpSetup,false);
  connect(helpSetupAction,&QAction::triggered,this,&PinglyMotionWindow::openSetupDialog);
  ui->menuHelp->addAction(helpSetupAction);
  QAction* helpWelcomeAction=specAction(this,helpWelcome,false);
  connect(helpWelcomeAction,&QAction::triggered,this,&PinglyMotionWindow::showWelcomeCard);
  ui->menuHelp->addAction(helpWelcomeAction);
  // the steps of the Script Lab example project again (job U3); the panel opens with them
  ui->menuHelp->addAction(m_scriptLab->makeStepsAction(this));
  m_hintsAction=specAction(this,helpHints,false);
  m_hintsAction->setCheckable(true);
  m_hintsAction->setChecked(UserSettings::tipsOn());
  keyInView(m_hintsAction,view);
  connect(m_hintsAction,&QAction::toggled,this,[this](bool on)
  {
    UserSettings::setTipsOn(on);
    ui->timelineView->easingTiles()->setTipsOn(on);
    ui->timelineView->setHintsOn(on);
    m_scriptLabPanel->setHintsOn(on);
    m_libraryPanel->setHintsOn(on);
    if(m_selectedJoint) m_selectedJoint->setHintsOn(on);
    refreshHandsPanel();
    m_tipsSwitch->setOn(on);
    if(m_hintCards) m_hintCards->setHintsOn(on);   // the hint cards follow the setting (UX phase 6a)
    if(m_helper) m_helper->setOn(on);   // the helper is the same setting (job UX-C)
  });
  ui->menuHelp->addAction(m_hintsAction);
  ui->menuHelp->addSeparator();
  QAction* helpKeysAction=specAction(this,helpKeys,false);
  keyInView(helpKeysAction,view);
  connect(helpKeysAction,&QAction::triggered,this,&PinglyMotionWindow::openKeysMouseDialog);
  ui->menuHelp->addAction(helpKeysAction);
  QAction* helpCameraAction=specAction(this,helpCamera,false);
  connect(helpCameraAction,&QAction::triggered,this,&PinglyMotionWindow::openCameraKeysDialog);
  ui->menuHelp->addAction(helpCameraAction);
  ui->menuHelp->addAction(specAction(this,helpCharter,true));
  ui->menuHelp->addSeparator();
  ui->menuHelp->addAction(whatsNew);
  ui->menuHelp->addAction(ui->helpOpenLogAction);
  ui->menuHelp->addAction(ui->helpClearLogAction);
  ui->menuHelp->addAction(ui->helpAboutAction);
}

// UX phase 1b: Edit as in the mockup: Undo, Redo, Set key, Paste mirrored. Undo and Redo work
// since job B1 (buildUndo(), src/pinglymotionwindowundo.cpp); Set key presses the key button. Cut,
// Copy and Paste (the timeline's selected stretch, UX phase 3b) stay below them: the mockup has no
// such items yet.
void PinglyMotionWindow::buildEditMenu()
{
  using namespace MenuSpec;
  ui->menuEdit->setToolTipsVisible(true);
  ui->menuEdit->clear();

  m_undoAction=specAction(this,editUndo,false);
  m_redoAction=specAction(this,editRedo,false);
  buildUndo();
  m_setKeyAction=specAction(this,editSetKey,false);
  keyInView(m_setKeyAction,ui->animationView);
  connect(m_setKeyAction,&QAction::triggered,this,&PinglyMotionWindow::setKeyFromMenu);
  m_pasteMirroredAction=specAction(this,editPasteMirrored,false);
  connect(m_pasteMirroredAction,&QAction::triggered,this,&PinglyMotionWindow::pasteMirroredFromMenu);

  ui->menuEdit->addAction(m_undoAction);
  ui->menuEdit->addAction(m_redoAction);
  ui->menuEdit->addSeparator();
  ui->menuEdit->addAction(m_setKeyAction);
  ui->menuEdit->addAction(m_pasteMirroredAction);
  ui->menuEdit->addSeparator();
  ui->menuEdit->addAction(ui->editCutAction);
  ui->menuEdit->addAction(ui->editCopyAction);
  ui->menuEdit->addAction(ui->editPasteAction);
  // Lock the first frame was in Options (gone): it stays as a choice, next to Set key's reasons
  ui->menuEdit->addSeparator();
  ui->menuEdit->addAction(ui->optionsProtectFirstFrameAction);
  updateEditActions();
}

// Set key (the timeline's main button, Edit > Set key, K and "Set the first key"):
// a key for the picked joint at this frame; it never takes a key away (UX phase 3a).
// A turn already keeps itself as a key, so on a frame with a key the status
// line says so instead.
void PinglyMotionWindow::setKeyFromMenu()
{
  Animation* anim=scene->getAnimation();
  if(m_empty || !anim || currentPart==nullptr || !ui->keyframeButton->isEnabled()) return;
  const bool added=TimelineSpec::addKey(anim,currentPart);
  if(added)
  {
    ui->animationView->update();
    updateKeyBtn();
  }
  statusBar()->showMessage(TimelineSpec::setKeyMessage(added,anim->getFrame(),jointDisplayName(currentPart->name())),5000);
  if(m_helper) m_helper->notify(QLatin1String(HelperSpec::keySet));
}

// the name of a joint as the Joints list shows it ("Left Shoulder"), or its file name
QString PinglyMotionWindow::jointDisplayName(const QString& fileName) const
{
  if(!m_jointInfo) return fileName;
  // the name that tells left and right apart ("Left Arm: Collar"), as the
  // Selected joint panel shows it; the Avatar Position by its list name
  QString name=(fileName==JointList::POSITION_TRACK) ? QString() : m_jointInfo->uniqueName(fileName);
  if(name.isEmpty()) name=JointList::friendlyName(*m_jointInfo,fileName);
  return name.isEmpty() ? fileName : name;
}

// Set key is greyed with the reason when it cannot work right now
void PinglyMotionWindow::updateEditActions()
{
  refreshJointPanel();
  if(!m_setKeyAction) return;
  const QString why=TimelineSpec::setKeyBlocked(!m_empty,currentPart!=nullptr,playstate!=PLAYSTATE_STOPPED,protect);
  m_setKeyAction->setEnabled(why.isEmpty());
  m_setKeyAction->setToolTip(why);
  m_setKeyAction->setStatusTip(why);
  // Undo and Redo need an open animation that is not playing
  if(m_undo)
    m_undo->setBlocked(m_empty ? QCoreApplication::translate("MenuSpec",MenuSpec::openFirst)
                               : (playstate!=PLAYSTATE_STOPPED ? QCoreApplication::translate("MenuSpec",MenuSpec::setKeyStopPlaying) : QString()));
  // the timeline's Set key button and "Set the first key" are greyed the same way,
  // and the reason line shows why (src/reasonline.h)
  ui->keyframeButton->setEnabled(why.isEmpty());
  ui->keyframeButton->setToolTip(why);
  ui->keyframeButton->setStatusTip(why);
  ui->timelineView->setFirstKeyBlocked(why);

  // UX phase 3b: Copy and Cut need rows to copy, Paste and Paste mirrored a copied
  // stretch; each is greyed with its reason (design charter P6)
  Timeline* line=ui->timelineView->getTimeline();
  const bool playing=playstate!=PLAYSTATE_STOPPED;
  const QString copyWhy=ClipSpec::copyBlocked(!m_empty,line->rows().rowCount()==0,playing);
  Animation* anim=scene ? scene->getAnimation() : nullptr;
  const QString pasteWhy=ClipSpec::pasteBlocked(!m_empty,m_clip.isEmpty(),playing,protect,anim ? anim->getFrame() : 0);
  auto grey=[](QObject* control,const QString& reason)
  {
    if(QAction* action=qobject_cast<QAction*>(control))
    {
      action->setEnabled(reason.isEmpty());
      action->setToolTip(reason);
      action->setStatusTip(reason);
    }
    else if(QPushButton* button=qobject_cast<QPushButton*>(control))
    {
      button->setEnabled(reason.isEmpty());
      button->setToolTip(reason);
      button->setStatusTip(reason);
    }
  };
  grey(ui->editCopyAction,copyWhy);
  grey(ui->editCutAction,copyWhy);
  grey(ui->editPasteAction,pasteWhy);
  if(m_pasteMirroredAction) grey(m_pasteMirroredAction,pasteWhy);
  if(m_panels && m_panels->pasteMirrored) grey(m_panels->pasteMirrored,pasteWhy);
}

// UX phase 1b: the top bar (src/topbar.h) and the empty state (src/emptystate.h)
void PinglyMotionWindow::buildTopBar()
{
  TopBar::Sources sources;
  sources.undo=m_undoAction;
  sources.redo=m_redoAction;
  sources.save=ui->fileSaveAction;
  sources.exportAnim=ui->fileExportAnimAction;
  sources.scriptLab=m_scriptLabDock->toggleViewAction();
  sources.library=m_libraryDock->toggleViewAction();
  sources.resetLayout=findChild<QAction*>(QLatin1String(MenuSpec::viewResetLayout.id));
  m_topBar=new TopBar(ui->toolBar,sources,this);
  m_topBar->setDark(UserSettings::darkTheme());
  // the Animate / Script switch at the left end of the bar (UX-F)
  m_workMode->addToBar(ui->toolBar);
  connect(m_topBar,&TopBar::themeRequested,this,[this]()
  {
    if(m_lightThemeAction) m_lightThemeAction->toggle();
  });
  connect(m_topBar,&TopBar::setupRequested,this,&PinglyMotionWindow::openSetupDialog);

  setupViewToolbar();
  setupFigures();

  m_emptyState=new EmptyState(ui->animationView);
  m_emptyState->hide();
  connect(m_emptyState,&EmptyState::startChosen,this,&PinglyMotionWindow::startFromLibrary);
  connect(m_emptyState,&EmptyState::newChosen,this,[this](){ fileNew(); });

  buildHintCards();
  buildHelper();
}

// UX phase 6a: the hint cards (src/hintcard.h), one at a time by the part of the window they talk about
void PinglyMotionWindow::buildHintCards()
{
  m_hintCards=new HintCards(this,[this](const QString& name) -> QWidget*
  {
    if(name==QLatin1String("view")) return ui->animationView;
    if(name==QLatin1String("selected")) return m_selectedJoint;
    if(name==QLatin1String("setKey")) return m_panels ? m_panels->setKey : nullptr;
    if(name==QLatin1String("start")) return m_panels && m_panels->transport ? m_panels->transport->startButton() : nullptr;
    if(name==QLatin1String("tiles")) return ui->timelineView->easingTiles();
    if(name==QLatin1String("joints")) return m_jointsDock;
    if(name==QLatin1String("setup")) return m_topBar ? m_topBar->setupWidget() : nullptr;
    return nullptr;
  });
  connect(m_hintCards,&HintCards::hintsOffRequested,this,[this]()
  {
    if(m_hintsAction) m_hintsAction->setChecked(false);
    statusBar()->showMessage(GuidanceSpec::tr(GuidanceSpec::hintsOffMessage,"HintCard"),8000);
  });
  // the close button or Esc on a card closes it; the next card waits (UX small fixes)
  connect(m_hintCards,&HintCards::cardClosed,this,[this]()
  {
    statusBar()->showMessage(GuidanceSpec::tr(GuidanceSpec::closedMessage,"HintCard"),8000);
  });
  // turning a joint with a slider also counts, as dragging a ring does (partDragged)
  for(QSlider* slider : {ui->xRotationSlider,ui->yRotationSlider,ui->zRotationSlider})
  {
    connect(slider,&QAbstractSlider::sliderMoved,this,[this](){ m_hintCards->markTurned(); });
    connect(slider,&QAbstractSlider::actionTriggered,this,[this](){ m_hintCards->markTurned(); });
  }
  // a card follows its part when a panel is shown or hidden
  for(QDockWidget* dock : {m_jointsDock,m_jointCardDock,m_timelineDock})
    if(dock) connect(dock,&QDockWidget::visibilityChanged,m_hintCards,[this](bool){ m_hintCards->refresh(); });
  m_hintCards->setHintsOn(UserSettings::tipsOn());
  m_hintCards->setAnimationOpen(!m_empty);
}

// Job UX-C: helper mode (src/helpermode.h). The Helper switch at the top is the same setting as
// Help > Show hints, the status bar switch and Setup; the hint cards move into the helper panel.
void PinglyMotionWindow::buildHelper()
{
  m_helper=new HelperMode(this,m_hintCards,
    [this](const QString& name){ return helperTarget(name); },
    [this](const QString& event){ return helperAlreadyDone(event); },
    [this](){ return centralWidget() ? centralWidget()->geometry() : QRect(); });
  // the switch stands right of Animate / Script, before the divider that follows them
  const QList<QAction*> barActions=ui->toolBar->actions();
  m_helper->addToBar(ui->toolBar,barActions.value(barActions.indexOf(m_workMode->scriptAction())+1));
  connect(m_helper,&HelperMode::switchClicked,this,[this](bool on)
  {
    if(m_hintsAction) m_hintsAction->setChecked(on);
    statusBar()->showMessage(HelperSpec::tr(on ? HelperSpec::onMessage : HelperSpec::offMessage),8000);
  });
  connect(m_helper,&HelperMode::offRequested,this,[this]()
  {
    if(m_hintsAction) m_hintsAction->setChecked(false);
    statusBar()->showMessage(HelperSpec::tr(HelperSpec::offMessage),8000);
  });
  // what the user does that a lesson step waits for (picking, turning, Set key and Start are sent
  // where they happen)
  connect(m_libraryDock->toggleViewAction(),&QAction::toggled,m_helper,[this](bool on)
  {
    if(on) m_helper->notify(QLatin1String(HelperSpec::libraryShown));
  });
  connect(m_workMode->scriptAction(),&QAction::toggled,m_helper,[this](bool on)
  {
    if(on) m_helper->notify(QLatin1String(HelperSpec::scriptSide));
  });
  connect(m_workMode->animateAction(),&QAction::toggled,m_helper,[this](bool on)
  {
    if(on) m_helper->notify(QLatin1String(HelperSpec::animateSide));
  });
  for(QSlider* slider : {ui->xRotationSlider,ui->yRotationSlider,ui->zRotationSlider})
  {
    connect(slider,&QAbstractSlider::sliderMoved,m_helper,[this](){ m_helper->notify(QLatin1String(HelperSpec::turned)); });
    connect(slider,&QAbstractSlider::actionTriggered,m_helper,[this](){ m_helper->notify(QLatin1String(HelperSpec::turned)); });
  }
  // a step's control may come or go with its panel
  for(QDockWidget* dock : {m_jointsDock,m_jointCardDock,m_timelineDock,m_libraryDock,m_scriptLabDock})
    if(dock) connect(dock,&QDockWidget::visibilityChanged,m_helper,[this](bool){ m_helper->refresh(); });
  m_helper->setAnimationOpen(!m_empty);
  m_helper->setOn(UserSettings::tipsOn());
}

// the controls the hint cards and the lesson steps talk about (HelperSpec targets)
QWidget* PinglyMotionWindow::helperTarget(const QString& name) const
{
  if(name==QLatin1String("view")) return ui->animationView;
  if(name==QLatin1String("selected")) return m_selectedJoint;
  if(name==QLatin1String("setKey")) return m_panels ? m_panels->setKey : nullptr;
  if(name==QLatin1String("start")) return m_panels && m_panels->transport ? m_panels->transport->startButton() : nullptr;
  if(name==QLatin1String("tiles")) return ui->timelineView->easingTiles();
  if(name==QLatin1String("joints")) return m_jointsDock;
  if(name==QLatin1String("setup")) return m_topBar ? m_topBar->setupWidget() : nullptr;
  if(name==QLatin1String("library") && m_topBar) return m_topBar->widgetFor(m_topBar->button(MenuSpec::topLibrary.id));
  if(name==QLatin1String("resetLayout") && m_topBar) return m_topBar->widgetFor(m_topBar->button(MenuSpec::topResetLayout.id));
  if(name==QLatin1String("scriptSwitch") && m_workMode) return m_workMode->widgetFor(m_workMode->scriptAction());
  if(name==QLatin1String("animateSwitch") && m_workMode) return m_workMode->widgetFor(m_workMode->animateAction());
  return nullptr;
}

// a lesson step whose thing is already so is skipped
bool PinglyMotionWindow::helperAlreadyDone(const QString& event) const
{
  if(event==QLatin1String(HelperSpec::picked)) return currentPart!=nullptr;
  if(event==QLatin1String(HelperSpec::libraryShown)) return m_libraryDock && !m_libraryDock->isHidden();
  if(event==QLatin1String(HelperSpec::scriptSide)) return m_workMode && m_workMode->side()==WorkMode::Side::Script;
  return false;
}

// Two figures in one scene, Expert mode only (docs/UX_DECISIONS.md 6m): the Figures box at the
// top right of the 3D view, Alt+1 and Alt+2, the timeline's Make active and Remove, a click on
// the other figure or its greyed rows, and the Library panel's Add as a second figure
void PinglyMotionWindow::setupFigures()
{
  AnimationView* view=ui->animationView;
  m_figuresBox=new FiguresBox(view);
  connect(m_figuresBox,&FiguresBox::activateRequested,this,&PinglyMotionWindow::makeFigureActive);
  for(int i=0;i<FigureSpec::maxFigures;i++)
  {
    QAction* action=new QAction(this);
    action->setObjectName(QStringLiteral("figure%1Action").arg(i+1));
    action->setShortcut(QKeySequence(QStringLiteral("Alt+%1").arg(i+1)));
    action->setShortcutContext(Qt::WindowShortcut);
    action->setEnabled(false);
    connect(action,&QAction::triggered,this,[this,i]() { makeFigureActive(i); });
    addAction(action);
    m_figureActions << action;
  }
  // a click on the other figure: in Beginner mode nothing changes and the status line says why
  view->setFigureGuard([this](int)
  {
    if(UserSettings::expertMode()) return true;
    statusBar()->showMessage(FigureSpec::tr(FigureSpec::beginnerSwitch),5000);
    return false;
  });
  connect(view,&AnimationView::viewDrawn,this,&PinglyMotionWindow::placeFigureTags);
  connect(ui->timelineView,&TimelineView::makeActiveRequested,this,&PinglyMotionWindow::makeFigureActive);
  connect(ui->timelineView,&TimelineView::removeRequested,this,[this](int figure) { removeFigure(figure); });
  connect(ui->timelineView,&TimelineView::otherJointClicked,this,[this](int figure,int track)
  {
    if(!UserSettings::expertMode())
    {
      statusBar()->showMessage(FigureSpec::tr(FigureSpec::beginnerSwitch),5000);
      return;
    }
    makeFigureActive(figure);
    if(activeFigure()!=figure) return;
    ui->timelineView->selectTrack(track);
    ui->animationView->selectPart(track);
  });
  if(m_libraryPanel)
    connect(m_libraryPanel,&LibraryPanel::addFigureRequested,this,&PinglyMotionWindow::addLibraryClipAsFigure);
  refreshFigures();
}

// UX phase 4b: the view toolbar at the top left of the 3D view (docs/UX_DECISIONS.md 6b)
void PinglyMotionWindow::setupViewToolbar()
{
  using Part=ViewToolSpec::Part;
  AnimationView* view=ui->animationView;
  m_viewToolbar=new ViewToolbar(view);

  // the saved choices
  const ViewTool tool=Camera::toolFor(UserSettings::viewTool());
  view->setViewTool(tool);
  m_viewToolbar->setTool(tool);
  view->setShowFloor(UserSettings::showFloor());
  view->setShowRings(UserSettings::showRings());
  view->setShowJointDots(UserSettings::showJointDots());
  view->setClickAndBend(UserSettings::clickAndBend());
  m_viewToolbar->setPartShown(Part::Floor,UserSettings::showFloor());
  m_viewToolbar->setPartShown(Part::Rings,UserSettings::showRings());
  m_viewToolbar->setPartShown(Part::JointDots,UserSettings::showJointDots());
  m_viewToolbar->setPartShown(Part::ToolNames,UserSettings::viewToolNames());

  connect(m_viewToolbar,&ViewToolbar::toolChosen,this,&PinglyMotionWindow::chooseViewTool);
  connect(view,&AnimationView::viewToolRequested,this,&PinglyMotionWindow::chooseViewTool);
  connect(m_viewToolbar,&ViewToolbar::partToggled,this,&PinglyMotionWindow::chooseViewPart);
  connect(m_viewToolbar,&ViewToolbar::focusRequested,this,&PinglyMotionWindow::focusPickedJoint);
  connect(m_viewToolbar,&ViewToolbar::wholeFigureRequested,this,[this,view]()
  {
    view->frameWholeFigure();
    statusBar()->showMessage(ViewToolSpec::tr(ViewToolSpec::wholeFigureDone),5000);
  });
  connect(m_viewToolbar,&ViewToolbar::homeRequested,this,[this]()
  {
    on_resetCameraAction_triggered();
    statusBar()->showMessage(ViewToolSpec::tr(ViewToolSpec::homeDone),5000);
  });
  connect(m_viewToolbar,&ViewToolbar::lookRequested,this,[this,view](int index)
  {
    if(index<0 || index>=ViewToolSpec::lookCount) return;
    const ViewToolSpec::Look& look=ViewToolSpec::looks[index];
    view->camera()->setView(look.height,look.angle);
    view->update();
    statusBar()->showMessage(ViewToolSpec::lookingFromMessage(Camera::viewNameFor(look.height,look.angle)),5000);
  });
  connect(m_viewToolbar,&ViewToolbar::endChosen,this,[this,view](ViewToolSpec::End end)
  {
    const ViewToolSpec::EndSpec& spec=ViewToolSpec::endSpec(end);
    view->camera()->lookFrom(spec.height,spec.angle);
    view->update();
    statusBar()->showMessage(ViewToolSpec::lookingFromMessage(Camera::viewNameFor(spec.height,spec.angle)),5000);
  });
  // dragging the widget's background turns the view as Look around does (half a degree per pixel)
  connect(m_viewToolbar,&ViewToolbar::dragged,this,[view](int dx,int dy)
  {
    view->camera()->rotate(dy*0.5f,dx*0.5f);
    view->update();
  });
  connect(view,&AnimationView::viewDrawn,m_viewToolbar,[this,view]()
  {
    m_viewToolbar->setCameraAngles(view->camera()->xRotation(),view->camera()->yRotation());
  });
  connect(view,&AnimationView::hoverHintChanged,this,&PinglyMotionWindow::showHoverHint);

  // dragging a joint (UX phase 4b part 2, docs/UX_DECISIONS.md 6a): it starts only when a key
  // can be set (the same reasons as Set key), and the label names the joints as the Joints list
  view->setPullGuard([this]()
  {
    return TimelineSpec::setKeyBlocked(!m_empty,true,playstate!=PLAYSTATE_STOPPED,protect);
  });
  view->setJointNamer([this](const QString& fileName) { return jointDisplayName(fileName); });
  connect(view,&AnimationView::pullFinished,this,&PinglyMotionWindow::pullFinished);
  connect(view,&AnimationView::pullCancelled,this,[this]()
  {
    statusBar()->showMessage(ViewToolSpec::tr(ViewToolSpec::pullCancelled),5000);
  });
  connect(view,&AnimationView::statusMessage,this,[this](const QString& text) { statusBar()->showMessage(text,5000); });
}

