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
#include <QTimer>
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
#include "documentwatch.h"
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

// What the remote control commands do in the window: the same steps as clicking.
class WindowControlHost : public ControlHost
{
  public:
    explicit WindowControlHost(PinglyMotionWindow* window) : w(window) {}

    // while the empty state shows, the commands see no animation (a clean placeholder stays behind it)
    Animation* animation() override { return w->m_empty ? nullptr : w->scene->getAnimation(); }

    void goToFrame(int frame) override
    {
      // the slider drives the program, as when you drag it yourself
      w->ui->currentFrameSlider->setValue(frame);
    }

    void refresh() override
    {
      w->updateInputs();
      w->updateKeyBtn();
      w->ui->animationView->update();
    }

    void selectJoint(const QString& name) override
    {
      Animation* anim = animation();
      for (int track = 1; track < 400; track++)
      {
        const QString found = anim->getPartName(track);
        if (found.isEmpty()) break;
        if (found == name)
        {
          w->ui->animationView->selectPart(anim->getNode(track));
          return;
        }
      }
    }

    QImage screenshot(int width, int height) override
    {
      return w->ui->animationView->renderImage(width, height);
    }

    bool newAnimation(QString& error) override
    {
      discardChanges();
      w->fileNew();
      if (!animation()) { error = "Could not start a new animation."; return false; }
      return true;
    }

    bool openFile(const QString& path, QString& error) override
    {
      return open(path, nullptr, error);
    }

    bool openFileData(const QString& path, const QByteArray& data, QString& error) override
    {
      return open(path, &data, error);
    }

    bool open(const QString& path, const QByteArray* data, QString& error)
    {
      discardChanges();
      // no modal message box while remote control opens a file: it would block the
      // answer (and a headless run) until someone clicks it; the reason goes back in 'error'
      const QScopedValueRollback<bool> quiet(w->m_quietDialogs, true);
      if (data) w->fileOpenData(path, *data);
      else w->fileOpen(path);
      if (!animation() || (w->currentFile != path && w->m_avmOrigin.value(w->currentFile) != path))
      {
        error = "The file could not be opened.";
        if (!w->m_lastOpenError.isEmpty()) error += " " + w->m_lastOpenError;
        return false;
      }
      return true;
    }

    bool saveFile(const QString& path, QString& error) override
    {
      if (!animation()) { error = "No animation is open."; return false; }
      const QFileInfo info(path);
      if (!QDir(info.absolutePath()).exists()) { error = "The folder does not exist: " + info.absolutePath(); return false; }
      w->storePropsInAnimation();
      if (!animation()->saveBVH(path, &error)) return false;
      if (path != w->currentFile)
      {
        w->setCurrentFile(path);
        w->ui->selectAnimationCombo->setItemText(w->ui->selectAnimationCombo->currentIndex(), info.baseName());
        w->openFiles[w->ui->selectAnimationCombo->currentIndex()] = path;
        w->ui->fileExportForSecondLifeAction->setEnabled(true);
      }
      return true;
    }

    void setPlaying(bool on) override
    {
      // the same as clicking Start or Stop under the timeline
      if (on) w->startPlaying();
      else w->stopPlaying();
    }

    bool playing() override { return w->playstate != PLAYSTATE_STOPPED; }

    QString fileName() override
    {
      return w->currentFile == w->m_untitledName ? QString() : w->currentFile;
    }

    bool unsavedChanges() override
    {
      for (Animation* anim : w->animationIds)
        if (anim->dirty()) return true;
      return false;
    }

    bool male() override
    {
      return animation() && animation()->getFigureType() == Animation::FIGURE_MALE;
    }

    void setMale(bool male) override
    {
      // the same as choosing it in the Figure box
      w->ui->figureCombo->setCurrentIndex(male ? 1 : 0);
      w->setAvatarShape(male ? 1 : 0);
    }

    void setCamera(float height, float angle, float distance) override
    {
      w->ui->animationView->camera()->orbit(height, angle, distance);
      w->ui->animationView->update();
    }

    void resetCamera() override
    {
      w->ui->animationView->resetCamera();
    }

    QVector3D jointPosition(BVHNode* joint) override
    {
      // where the 3D view draws it (the Picked joint framing's point); the pose maths if not drawn
      QVector3D position;
      if (joint && w->ui->animationView->drawnJointPosition(joint->name(), &position)) return position;
      return ControlHost::jointPosition(joint);
    }

    void setCameraFocus(float height, float angle, float distance, const QVector3D& target) override
    {
      // turn first, then look at the joint the way the F key does (Camera::lookAt keeps the turn)
      Camera* c = w->ui->animationView->camera();
      c->orbit(height, angle, distance);
      c->lookAt(target.x(), target.y(), target.z(), distance);
      w->ui->animationView->update();
    }

    QVector3D camera() override
    {
      const Camera* c = w->ui->animationView->camera();
      return QVector3D(c->xRotation(), c->yRotation(), c->distance());
    }

    void setLoopPoints(int loopIn, int loopOut) override
    {
      ControlHost::setLoopPoints(loopIn, loopOut);
      // the Loop In and Loop Out fields under the timeline show the new
      // points, as when the user types them (the animation already holds
      // valid points, so the window keeps them unchanged)
      w->setLoopOutPoint(loopOut);
      w->setLoopInPoint(loopIn);
    }

  private:
    // the command already checked that changes may be lost; avoid the question dialog
    void discardChanges()
    {
      for (Animation* anim : w->animationIds) anim->setDirty(false);
    }

    PinglyMotionWindow* w;
};

namespace
{
  // While it lives (the first seconds of a start with the default layout), every layout change of
  // the timeline's content gives the timeline the height its controls need (WP-01 A2).
  class TimelineStartFit : public QObject
  {
  public:
    TimelineStartFit(QMainWindow* window,QDockWidget* dock) : QObject(window), m_window(window), m_dock(dock)
    {
      if(QScrollArea* scroll=qobject_cast<QScrollArea*>(dock->widget())) scroll->widget()->installEventFilter(this);
      QTimer::singleShot(0,this,[this]() { fit(); });
    }
    bool eventFilter(QObject* watched,QEvent* event) override
    {
      if(event->type()==QEvent::LayoutRequest && !m_queued)
      {
        m_queued=true;
        QTimer::singleShot(0,this,[this]() { m_queued=false; fit(); });
      }
      return QObject::eventFilter(watched,event);
    }
  private:
    void fit() { if(!m_dock->isFloating() && !m_dock->isHidden()) PanelWindow::fitTimelineHeight(m_window,m_dock); }
    QMainWindow* m_window;
    QDockWidget* m_dock;
    bool m_queued=false;
  };
}

PinglyMotionWindow::PinglyMotionWindow() :
  QMainWindow(nullptr),
  ui(new Ui::PinglyMotionWindow),
  m_pinglyMotion(new PinglyMotion(this)),
  updateChecker(nullptr, this),
  cubeMesh(new Mesh(Constants::cubeMeshFilePath(), Mesh::Shape::CUBE)),
  sphereMesh(new Mesh(QString(), Mesh::Shape::SPHERE)),  // built in code, no file
  coneMesh(new Mesh(QString(), Mesh::Shape::CONE)),      // built in code, no file
  torusMesh(new Mesh(Constants::torusMeshFilePath(), Mesh::Shape::TORUS)),
  m_animFilter(PinglyMotionWindow::tr("Animation and Prop Files (*.dingle *.avm *.bvh *.prp)")),
  m_dingleFilter(PinglyMotionWindow::tr("PinglyMotion animation (*.dingle)")),
  m_propFilter(PinglyMotionWindow::tr("Props (*.prp)")),
  m_untitledName(DingleSave::defaultName()),
  m_precision(SliderScale::precision),
  m_controlIndicator(nullptr)
{
  QApplication::setOrganizationName(Constants::organizationName());
  QApplication::setOrganizationDomain(Constants::organizationDomain());
  QApplication::setApplicationName(Constants::applicationName());
  QApplication::setApplicationVersion(Constants::versionNumberString());

  ui->setupUi(this);
  buildJointPicker();
  buildDocks();

  // the build label shows from the first moment, not only after the first file
  setWindowTitle(Constants::windowTitle(m_untitledName));
  setAttribute(Qt::WA_DeleteOnClose);
  currentPart=0;
  longestRunningTime=0.0;
  scene = new Scene(AnimationView::firstObjectId(), this);
  ui->animationView->setScene(scene);
  m_pinglyMotion->setScene(scene);
  ui->animationView->setMeshModels(cubeMesh, sphereMesh, coneMesh);

  // fill the joint lists from the Bento skeleton
  buildPartLists();

  // playback stopped by default
  setPlaystate(PLAYSTATE_STOPPED);

  UserSettings::readSettings();
  Theme::apply(UserSettings::darkTheme());
  fitPanelWidths();
  // the heading colour in the joint list comes from the theme
  fillJointTable();

  // the window's place, size, maximised state and screen from last time
  // (docs/archive/PANELS_PLAN.md, job P3). Qt puts a window back onto a screen when
  // its saved place is gone. The old width and height are used only while
  // no geometry has been saved yet.
  {
    const PanelLayout::StartupWindow startWindow=PanelLayout::chooseWindow(
      UserSettings::mainWindowGeometry(),UserSettings::mainWindowWidth(),UserSettings::mainWindowHeight(),
      PanelLayout::defaultWindowSize(PanelWindow::screenRects().value(0,QRect(0,0,1280,860)).size()));
    if(!startWindow.useGeometry || !restoreGeometry(UserSettings::mainWindowGeometry()))
      resize(startWindow.size);
  }

  ui->optionsLoopAction->setChecked(UserSettings::loop());
  m_transport->setLoop(UserSettings::loop());
  ui->optionsJointLimitsAction->setChecked(UserSettings::jointLimits());

  ui->optionsProtectFirstFrameAction->setChecked(UserSettings::protectFirstFrame());

  // View > Body (Options > Body before UX job U2): which avatar the 3D view shows (only what
  // you see, not the animation); buildMenus() puts it into the View menu
  QMenu* bodyMenu=new QMenu(ViewMenuSpec::tr(ViewMenuSpec::bodyMenu),this);
  bodyMenu->setObjectName(QStringLiteral("viewBodyMenu"));
  bodyMenu->setToolTipsVisible(true);
  m_bodyMenu=bodyMenu;
  QActionGroup* bodyGroup=new QActionGroup(this);
  struct BodyChoice { const char* key; AvatarSkin::Body body; QString text; QString tip; };
  const BodyChoice bodyChoices[]={
    {"ruth2",AvatarSkin::RuthBody,tr("Ruth2"),
     tr("The open source Ruth2 avatar, with fingers and a face that follow the extra Second Life bones.")},
    {"max",AvatarSkin::MaxBody,tr("Max"),
     tr("The open source Max avatar, with fingers and a face that follow the extra Second Life bones.")}};
  QMap<QString,QAction*> bodyActions;
  for(const BodyChoice& choice : bodyChoices)
  {
    QAction* action=bodyMenu->addAction(choice.text);
    action->setCheckable(true);
    action->setToolTip(choice.tip);
    action->setData(QString::fromLatin1(choice.key));
    bodyGroup->addAction(action);
    bodyActions.insert(QString::fromLatin1(choice.key),action);
  }
  {
    QString current=UserSettings::body();
    AvatarSkin::Body wanted=AvatarSkin::RuthBody;
    for(const BodyChoice& choice : bodyChoices)
      if(current==QLatin1String(choice.key)) wanted=choice.body;
    if(!ui->animationView->setBody(wanted))   // could not load: the other body is shown
      current=ui->animationView->body()==AvatarSkin::MaxBody ? QStringLiteral("max") : QStringLiteral("ruth2");
    bodyActions.value(current,bodyActions.value(QStringLiteral("ruth2")))->setChecked(true);
  }

  // View > Joint groups (Options > Joints before UX job U2): show or hide groups of Bento
  // joints (display only); buildMenus() puts it into the View menu
  QMenu* jointsMenu=new QMenu(ViewMenuSpec::tr(ViewMenuSpec::jointGroupsMenu),this);
  jointsMenu->setObjectName(QStringLiteral("viewJointGroupsMenu"));
  jointsMenu->menuAction()->setToolTip(ViewMenuSpec::tr(ViewMenuSpec::jointGroupsTip));
  jointsMenu->menuAction()->setStatusTip(ViewMenuSpec::tr(ViewMenuSpec::jointGroupsTip));
  m_jointGroupsMenu=jointsMenu;
  for(const ViewMenuSpec::Group& entry : ViewMenuSpec::groups)
  {
    QAction* action=jointsMenu->addAction(ViewMenuSpec::tr(entry.text));
    action->setObjectName(QLatin1String(entry.id));
    action->setCheckable(true);
    action->setChecked(!(UserSettings::hiddenJointGroups() & entry.group));
    const int group=entry.group;
    connect(action,&QAction::toggled,this,[this,group](bool on)
    {
      int hidden=UserSettings::hiddenJointGroups();
      hidden=on ? (hidden & ~group) : (hidden | group);
      UserSettings::setHiddenJointGroups(hidden);
      jointGroupsChanged();
    });
  }
  jointsMenu->addSeparator();
  QAction* hideInListsAction=jointsMenu->addAction(ViewMenuSpec::tr(ViewMenuSpec::viewHideInLists.text));
  hideInListsAction->setObjectName(QLatin1String(ViewMenuSpec::viewHideInLists.id));
  hideInListsAction->setCheckable(true);
  hideInListsAction->setToolTip(ViewMenuSpec::tr(ViewMenuSpec::hideInListsTip));
  hideInListsAction->setChecked(UserSettings::hideJointsInLists());
  connect(hideInListsAction,&QAction::toggled,this,[this](bool on)
  {
    UserSettings::setHideJointsInLists(on);
    jointGroupsChanged();
  });
  jointsMenu->setToolTipsVisible(true);
  jointGroupsChanged();
  connect(bodyGroup,&QActionGroup::triggered,this,[this,bodyActions](QAction* action)
  {
    const QString key=action->data().toString();
    AvatarSkin::Body wanted=AvatarSkin::RuthBody;
    if(key==QLatin1String("max")) wanted=AvatarSkin::MaxBody;
    if(!ui->animationView->setBody(wanted))
    {
      const QString shown=ui->animationView->body()==AvatarSkin::MaxBody ? QStringLiteral("max") : QStringLiteral("ruth2");
      bodyActions.value(shown)->setChecked(true);
      UserSettings::setBody(shown);
      QMessageBox::warning(this,tr("Body"),tr("The avatar could not be loaded. Showing %1 instead.")
                           .arg(ui->animationView->body()==AvatarSkin::MaxBody ? tr("Max") : tr("Ruth2")));
      return;
    }
    UserSettings::setBody(key);
  });

  // Options > Lighting...: brightness, shine and warmth of the body light
  ui->animationView->setBodyLight(UserSettings::lightBrightness(),UserSettings::lightShine(),
                                  UserSettings::lightWarmth(),UserSettings::floorShadow());
  // View > Lighting... (Options before UX job U2); buildMenus() puts it into the View menu
  QAction* lightAction=new QAction(ViewMenuSpec::tr(ViewMenuSpec::viewLighting.text),this);
  lightAction->setObjectName(QLatin1String(ViewMenuSpec::viewLighting.id));
  lightAction->setToolTip(ViewMenuSpec::tr(ViewMenuSpec::lightingTip));
  m_lightingAction=lightAction;
  connect(lightAction,&QAction::triggered,this,&PinglyMotionWindow::showLightDialog);

  // panel layout from last time (size, place, floating panels). Only a layout
  // saved by this version of the panels is loaded (docs/archive/PANELS_PLAN.md, 7);
  // the old window_state with the Properties panel is left alone, so the
  // default layout is used once after the move to three panels.
  const PanelLayout::StartupChoice startup=PanelLayout::chooseStartup(
    UserSettings::panelLayoutFor(UserSettings::expertMode()),UserSettings::panelLayoutVersion(),size(),true);
  // each mode has its own layout (UX-E); a mode without one starts with its default, which in
  // Beginner mode has the Joints list closed. A floating Script Lab keeps its saved place and size
  // (job UX-B: it opens big only the first time)
  if(startup.useSavedLayout)
  {
    ScriptLabPanel::setLayoutRestoring(true);
    restoreState(UserSettings::panelLayoutFor(UserSettings::expertMode()),PanelLayout::CurrentVersion);
    ScriptLabPanel::setLayoutRestoring(false);
  }
  else
  {
    if(!BeginnerView::jointsListInDefaultLayout(UserSettings::expertMode())) m_jointsDock->hide();
    // the window has its size now: the timeline starts as high as its controls need (WP-01 A2)
    PanelWindow::fitTimelineHeight(this,m_timelineDock);
    // and again while the panel settles after the window is on screen: its rows wrap at the width
    // it gets, which makes it taller. Only the first seconds; after that the user's size stays.
    QObject* settle=new TimelineStartFit(this,m_timelineDock);
    QTimer::singleShot(3000,settle,&QObject::deleteLater);
  }
  // a floating panel whose place is on no screen any more (second screen
  // unplugged) is pulled back onto one (job P3)
  PanelWindow::pullFloatingPanelsOntoScreen(panelDocks(),PanelWindow::screenRects());
  // a saved layout may still have a panel on a side it no longer goes
  if(!m_jointsDock->isFloating() && !(dockWidgetArea(m_jointsDock) & m_jointsDock->allowedAreas()))
    addDockWidget(Qt::LeftDockWidgetArea,m_jointsDock);
  if(!m_jointCardDock->isFloating() && !(dockWidgetArea(m_jointCardDock) & m_jointCardDock->allowedAreas()))
    addDockWidget(Qt::RightDockWidgetArea,m_jointCardDock);
  if(!m_timelineDock->isFloating() && !(dockWidgetArea(m_timelineDock) & m_timelineDock->allowedAreas()))
    addDockWidget(Qt::BottomDockWidgetArea,m_timelineDock);
  if(!m_scriptLabDock->isFloating() && !(dockWidgetArea(m_scriptLabDock) & m_scriptLabDock->allowedAreas()))
    addDockWidget(Qt::RightDockWidgetArea,m_scriptLabDock);
  if(!m_libraryDock->isFloating() && !(dockWidgetArea(m_libraryDock) & m_libraryDock->allowedAreas()))
    tabifyDockWidget(m_jointCardDock,m_libraryDock);
  // the timeline stands visible at every start; hiding it lasts for this session only
  PanelVisibility::showAtStart(m_timelineDock,ui->optionsShowTimelineAction);
  // the Library never opens by itself (UX-A): it keeps its saved place and size and shows
  // when asked for (View > Library, Ctrl+Shift+B, the Library button)
  m_libraryDock->hide();
  // the Animate / Script switch (UX-F, src/workmode.h): the program starts on Animate
  {
    WorkMode::Hooks hooks;
    hooks.before=[this]()
    {
      if(m_tabState.isHidden()) togglePanelsHidden();
      m_applyingTab=true;
    };
    hooks.after=[this]()
    {
      m_reflower.wideLayoutChanged(this,reflowDocks());
      m_applyingTab=false;
      m_hidePanelsAction->setChecked(false);
      ui->optionsShowTimelineAction->setChecked(!m_timelineDock->isHidden());
      updateBeginnerRow();
    };
    hooks.layoutToSave=[this]() { return m_reflower.layoutToSave(this); };
    hooks.expert=[]() { return UserSettings::expertMode(); };
    m_workMode=new WorkMode::Switcher(this,reflowDocks(),hooks,this);
    m_workMode->normaliseAtStart();
  }
  // at large text or in a small window the panels rearrange (docs/UX_DECISIONS.md, 6c);
  // decided once the window has its size
  scheduleReflow();


  connect(m_pinglyMotion.data(), &PinglyMotion::repaint, ui->animationView, static_cast<void (AnimationView::*)()>(&AnimationView::repaint));
  connect(scene, &Scene::repaint, ui->animationView, qOverload<>(&QWidget::update));
  connect(scene, &Scene::animationSelected, this, &PinglyMotionWindow::selectAnimation);
  connect(this, &PinglyMotionWindow::protectFrame, scene, &Scene::protectFrame);

  connect(ui->animationView, &AnimationView::storeCameraPosition, scene, &Scene::storeCameraPosition);
  connect(ui->animationView, &AnimationView::restoreCameraPosition, scene, &Scene::restoreCameraPosition);

  connect(ui->animationView, qOverload<BVHNode*,QVector3D,RotationLimits,QVector3D>(&AnimationView::partClicked), this, &PinglyMotionWindow::partClicked);

  connect(ui->animationView, &AnimationView::partDragged, this, &PinglyMotionWindow::partDragged);

  connect(ui->animationView, &AnimationView::propClicked, this, &PinglyMotionWindow::propClicked);

  connect(ui->animationView, &AnimationView::backgroundClicked, this, &PinglyMotionWindow::backgroundClicked);
  connect(ui->animationView, &AnimationView::animationSelected, this, &PinglyMotionWindow::selectAnimation);

  connect(this, &PinglyMotionWindow::newProp, m_pinglyMotion.data(), &PinglyMotion::newProp);
  connect(this, static_cast<void (PinglyMotionWindow::*)(int)>(&PinglyMotionWindow::selectProp), m_pinglyMotion.data(), &PinglyMotion::selectProp);
  connect(this, &PinglyMotionWindow::attachProp, m_pinglyMotion.data(), &PinglyMotion::attachProp);
  connect(this, &PinglyMotionWindow::propPositionChanged, m_pinglyMotion.data(), &PinglyMotion::propPositionChanged);
  connect(this, &PinglyMotionWindow::propScaleChanged, m_pinglyMotion.data(), &PinglyMotion::propScaleChanged);
  connect(this, &PinglyMotionWindow::propRotationChanged, m_pinglyMotion.data(), &PinglyMotion::propRotationChanged);
  connect(this, &PinglyMotionWindow::deleteProp, m_pinglyMotion.data(), &PinglyMotion::deleteProp);
  connect(this, &PinglyMotionWindow::clearProps, m_pinglyMotion.data(), &PinglyMotion::clearProps);
  connect(m_pinglyMotion.data(), &PinglyMotion::newPropUi, this, &PinglyMotionWindow::onNewProp);
  connect(m_pinglyMotion.data(), &PinglyMotion::selectPropUi, this, &PinglyMotionWindow::onSelectProp);
  connect(m_pinglyMotion.data(), &PinglyMotion::deletePropUi, this, &PinglyMotionWindow::onDeleteProp);
  connect(m_pinglyMotion.data(), &PinglyMotion::clearPropsUi, this, &PinglyMotionWindow::onClearProps);
  connect(m_pinglyMotion.data(), &PinglyMotion::enablePropsUi, this, &PinglyMotionWindow::enableProps);
  connect(m_pinglyMotion.data(), &PinglyMotion::enableAvatarRotationUi, this, &PinglyMotionWindow::enableAvatarRotation);
  connect(m_pinglyMotion.data(), &PinglyMotion::enableAvatarPositionUi, this, &PinglyMotionWindow::enableAvatarPosition);
  connect(m_pinglyMotion.data(), &PinglyMotion::updatePropSpinsUi, this, &PinglyMotionWindow::updatePropSpins);

  // the easing tile row on the timeline (issue #53 step 5)
  connect(ui->timelineView->easingTiles(),&EasingTiles::choiceClicked,this,&PinglyMotionWindow::easingChoiceClicked);

  connect(&timer, &QTimer::timeout, this, &PinglyMotionWindow::frameTimeout);

  connect(this, &PinglyMotionWindow::resetCamera, ui->animationView, &AnimationView::resetCamera);
  // UX phase 4a: the mouse follows the camera habit chosen in Setup or Camera and keys
  ui->animationView->setCameraHabit(Camera::habitFor(UserSettings::cameraHabit()));

  connect(ui->animationView, qOverload<int>(&AnimationView::partClicked), ui->timelineView, &TimelineView::selectTrack);
  connect(ui->animationView, &AnimationView::backgroundClicked, ui->timelineView, &TimelineView::backgroundClicked);

  timeline=ui->timelineView->getTimeline();
  connect(timeline, &Timeline::positionCenter, ui->timelineView, &TimelineView::scrollTo);
  connect(timeline, &Timeline::trackClicked, ui->animationView, qOverload<int>(&AnimationView::selectPart));

  ui->xRotationSlider->setPageStep(10 * m_precision);
  ui->yRotationSlider->setPageStep(10 * m_precision);
  ui->zRotationSlider->setPageStep(10 * m_precision);
  ui->xPositionSlider->setPageStep(10 * m_precision);
  ui->yPositionSlider->setPageStep(10 * m_precision);
  ui->zPositionSlider->setPageStep(10 * m_precision);

  ui->currentFrameSlider->setPageStep(1);

  if (!updateChecker.hasNews())
  {
    ui->helpWhatsNewAction->setVisible(false);
  }

  // the AI Assistant (remote control): lets AI tools and other programs pose and play the animation
  m_controlHost.reset(new WindowControlHost(this));
  m_controlApi.reset(new ControlApi(m_controlHost.data()));
  m_controlServer.reset(new ControlServer(m_controlApi.data()));
  // the Script Lab: the same RLV state, LSL session and folder watch as the control commands, its
  // effects and overrides in the 3D view, and the scene's props (job R5, src/scriptlabcontroller.h)
  m_scriptLab->connectControl(m_controlApi.data());
  m_scriptLab->connectView(ui->animationView);
  {
    ScriptLabController::Meshes meshes{cubeMesh,sphereMesh,coneMesh,torusMesh};
    ScriptLabController::WindowHooks hooks;
    hooks.jointNames=[this]() { return ui->animationView->partNames(); };
    hooks.propAdded=[this](const Prop* prop) { ui->propNameCombo->addItem(prop->name()); };
    hooks.selectProp=[this](Prop* prop) { propClicked(prop); };
    hooks.showPropsTab=[this]() { ui->avatarPropsTab->setCurrentIndex(1); };
    hooks.propMoved=[this](const Prop* prop)
    {
      if(scene->propManager()->getSelectedPropId()==prop->id()) updatePropSpins(prop->position(),prop->rotation(),prop->scale());
    };
    hooks.redraw=[this]() { ui->animationView->update(); };
    m_scriptLab->connectScene(scene,meshes,hooks);
  }

  // status bar (#88): "Tips on" switch, remote control and Aurora link, left to right;
  // messages still show on the left
  statusBar()->setStyleSheet(QStringLiteral("QStatusBar::item { border: none; }"));
  m_tipsSwitch=new TipsSwitch(this);
  m_tipsSwitch->setOn(UserSettings::tipsOn());
  statusBar()->addPermanentWidget(m_tipsSwitch);
  // the hint under the easing tiles follows the setting; more tips come with #88
  ui->timelineView->easingTiles()->setTipsOn(UserSettings::tipsOn());
  // the notes line under the timeline follows the hints too (UX phase 3c)
  ui->timelineView->setHintsOn(UserSettings::tipsOn());
  connect(ui->timelineView,&TimelineView::notesHidden,this,[this]()
  {
    statusBar()->showMessage(NotesSpec::text(NotesSpec::hiddenMessage),5000);
  });
  connect(m_tipsSwitch,&TipsSwitch::toggled,this,[this](bool on)
  {
    UserSettings::setTipsOn(on);
    ui->timelineView->easingTiles()->setTipsOn(on);
    ui->timelineView->setHintsOn(on);
    m_scriptLabPanel->setHintsOn(on);   // the Script Lab panel's explanation line
    m_libraryPanel->setHintsOn(on);     // the Library panel's line on top
    if(m_selectedJoint) m_selectedJoint->setHintsOn(on);   // the Selected joint panel's hint line
    refreshHandsPanel();   // the Hands tab's hint lines
    if(m_hintCards) m_hintCards->setHintsOn(on);
    if(m_hintsAction) m_hintsAction->setChecked(on);   // Help > Show hints follows the switch
  });
  buildMenus();
  buildEditMenu();
  buildTopBar();

  // the AI Assistant button: off, waiting, who is connected (issues #43 and #7); always off at start
  m_controlIndicator=new ControlIndicator(this);
  statusBar()->addPermanentWidget(m_controlIndicator);
  connect(m_controlIndicator,&ControlIndicator::clicked,this,&PinglyMotionWindow::aiAssistantClicked);
  connect(m_controlIndicator,&ControlIndicator::turnOnRequested,this,[this](){ setRemoteControl(true); });
  connect(m_controlIndicator,&ControlIndicator::turnOffRequested,this,[this](){ setRemoteControl(false); });
  connect(m_controlIndicator,&ControlIndicator::connectAnotherRequested,this,&PinglyMotionWindow::showAiConnectDialog);
  connect(m_controlServer.data(),&ControlServer::contact,m_controlIndicator,&ControlIndicator::onContact);
  connect(m_controlServer.data(),&ControlServer::refused,m_controlIndicator,&ControlIndicator::onRefused);
  // the connection file must not stay behind when the program ends
  connect(qApp,&QCoreApplication::aboutToQuit,this,[this](){  if(m_controlServer) { m_controlServer->removeConnectionFile(); m_controlServer->stop(); } });

  // Options > Live link to Aurora: the pose shows on your avatar in the Aurora viewer
  m_auroraLink=new AuroraLink(this);
  m_auroraLink->setPoseSource([this]() -> QString
  {
    Animation* anim=scene->getAnimation();
    return anim ? anim->poseBvhText(anim->getFrame()) : QString();
  });
  m_auroraLink->setQuaternionSource([this]() -> QJsonObject
  {
    Animation* anim=scene->getAnimation();
    return anim ? anim->poseQuaternions(anim->getFrame()) : QJsonObject();
  });
  m_auroraAction=new QAction(tr("Live link to Aurora"),this);
  m_auroraAction->setCheckable(true);
  m_auroraAction->setToolTip(tr("Shows the pose on your avatar in the Aurora viewer while you work. "
                                "In Aurora, open LiveMesh and click Enable preview."));
  // no menu entry: the Aurora switch in Setup and in the status bar work this action (the old Options menu is gone)
  connect(m_auroraAction,&QAction::toggled,m_auroraLink,[this](bool on){ m_auroraLink->setEnabled(on); updateWindowTitle(); });
  connect(m_auroraLink,&AuroraLink::connectedChanged,this,[this](bool){ updateWindowTitle(); });
  connect(m_auroraLink,&AuroraLink::errorReceived,this,[this](const QString& message)
  {
    statusBar()->showMessage(tr("Aurora link: %1").arg(message),10000);
  });
  // status bar: Aurora link off, connecting or live; the dot turns it on or off
  m_auroraIndicator=new AuroraIndicator(this);
  statusBar()->addPermanentWidget(m_auroraIndicator);
  connect(m_auroraIndicator,&AuroraIndicator::toggleRequested,m_auroraAction,&QAction::toggle);

  // UX phase 1c: the status bar says why a greyed-out control is greyed out (charter P6)
  m_reasonLine=new ReasonLine(statusBar(),this);
  m_reasonLine->watchMenus();

  // command line: pinglymotion [--control] [animation file]
  QStringList args = QApplication::arguments();
  args.removeFirst();
  const bool startWithControl=args.removeAll("--control")>0;
  QString fileFromCommandLine;
  for(const QString& arg : args)
    if(!arg.startsWith("--") && fileFromCommandLine.isEmpty()) fileFromCommandLine=arg;
  m_startedWithFileOrControl=startWithControl || !fileFromCommandLine.isEmpty();
  if(!fileFromCommandLine.isEmpty())
  {
    fileOpen(fileFromCommandLine);
  }

  // if opening of files didn't work or no files were specified on the
  // command line, open a new one
  if(openFiles.count()==0) fileNew();

  updateInputs();

  if(startWithControl)
  {
    // started with --control (tests and scripts): on from the start, nobody clicks anything
    setRemoteControl(true);
  }
}

void PinglyMotionWindow::setRemoteControl(bool on)
{
#ifdef Q_OS_WASM
  // the web build has no control server: a web page cannot listen for connections
  // (docs/DEV.md, "Web-ready code", rule 3; remote control is not part of the web version)
  if(on)
  {
    QMessageBox::information(this,tr("AI Assistant"),tr("The AI Assistant needs the desktop program: in a web browser, AI tools cannot connect to PinglyMotion."));
    updateWindowTitle();
    return;
  }
#endif
  if(on)
  {
    if(m_controlServer->running()) return;
    QString error;
    // a new secret key every time (ControlServer::start)
    if(!m_controlServer->start(ControlServer::DEFAULT_PORT,&error) || !m_controlServer->writeConnectionFile(&error))
    {
      m_controlServer->stop();
      // never the user's account folder on screen
      error.replace(QDir::toNativeSeparators(QDir::homePath()),tr("<your user folder>"));
      error.replace(QDir::homePath(),tr("<your user folder>"));
      QMessageBox::warning(this,tr("AI Assistant"),tr("The AI Assistant could not be turned on:\n%1").arg(error));
      updateWindowTitle();
      return;
    }
    statusBar()->showMessage(tr("AI Assistant on. AI tools on this PC can now pose and play the animation."),5000);
  }
  else
  {
    // locks every AI out: the server stops and the file with the key is deleted
    m_controlServer->removeConnectionFile();
    m_controlServer->stop();
    statusBar()->showMessage(tr("AI Assistant off. Every AI is locked out and the key is deleted."),5000);
  }
  updateWindowTitle();
}

void PinglyMotionWindow::aiAssistantClicked()
{
  // the first time (no AI has ever connected and it is off): ask which AI
  if(!m_controlServer->running() && !AiAssistant::anyKnown())
  {
    showAiConnectDialog();
    return;
  }
  QMenu* menu=m_controlIndicator->buildMenu(this);
  QToolButton* button=m_controlIndicator->button();
  // above the button: the status bar is at the bottom of the window
  const QPoint corner=button->mapToGlobal(QPoint(0,0));
  menu->popup(QPoint(corner.x(),corner.y()-menu->sizeHint().height()));
}

void PinglyMotionWindow::showAiConnectDialog()
{
  AiConnectDialog dialog(m_controlServer->running(),this);
  connect(&dialog,&AiConnectDialog::turnOnRequested,this,[this,&dialog]()
  {
    setRemoteControl(true);
    dialog.setRunning(m_controlServer->running());
  });
  dialog.exec();
}

// the title says so while other programs can control PinglyMotion
void PinglyMotionWindow::updateWindowTitle()
{
  QString title=Constants::windowTitle(m_empty ? QString() : currentFile);
  // two figures (docs/UX_DECISIONS.md 6m): "figure 2 of 2" after the file name
  const QString figure=m_empty ? QString() : FigureSpec::titleText(activeFigure()+1,animationIds.count());
  if(!figure.isEmpty()) title+=QStringLiteral(" \u00B7 ")+figure;
  if(m_controlServer && m_controlServer->running()) title+=tr(" - AI Assistant on");
  if(m_controlIndicator) m_controlIndicator->setRunning(m_controlServer && m_controlServer->running());
  if(m_auroraLink)
    title+=AuroraIndicator::titleSuffixFor(AuroraIndicator::stateFor(m_auroraLink->enabled(),m_auroraLink->connected()));
  if(m_auroraIndicator && m_auroraLink) m_auroraIndicator->setStatus(m_auroraLink->enabled(),m_auroraLink->connected());
  setWindowTitle(title);
}

PinglyMotionWindow::~PinglyMotionWindow()
{
  fileExit();
}

void PinglyMotionWindow::queueAfterShow()
{
  // QTimer creates a timerEvent
  // - This is a convenient way to queue "afterShow" to
  //   run after the main window shows
  QTimer::singleShot(0, this, &PinglyMotionWindow::afterShow);
}

void PinglyMotionWindow::afterShow()
{
  updateChecker.onStartup();
  // the welcome card, on the first start only; not when a file was given on the command line or
  // the program was started for scripts and tests (--control), where nobody clicks anything
  if(WelcomeDialog::due() && !m_startedWithFileOrControl) showWelcomeCard();
}

// Help > Show the welcome card again, Setup's button, and the first start
void PinglyMotionWindow::showWelcomeCard()
{
  WelcomeDialog dialog(this);
  dialog.exec();
  const WelcomeDialog::Answer answer=dialog.answer();
  WelcomeDialog::apply(answer);
  // the same paths as the menu items and the Setup dialog, so everything shows the new choice
  if(m_expertModeAction && m_expertModeAction->isChecked()!=answer.expert) m_expertModeAction->setChecked(answer.expert);
  if(answer.expert && !answer.cameraHabit.isEmpty()) chooseCameraHabit(answer.cameraHabit);
  if(answer.skipped)
  {
    statusBar()->showMessage(GuidanceSpec::tr(GuidanceSpec::skipMessage,"WelcomeDialog"),8000);
    return;
  }
  startFromLibrary(answer.clipId);
  statusBar()->showMessage(GuidanceSpec::tr(GuidanceSpec::readyMessage,"WelcomeDialog"),8000);
}

// slot gets called by AnimationView::mousePressEvent()
void PinglyMotionWindow::partClicked(BVHNode* node, QVector3D rotation, RotationLimits limits, QVector3D position)
{
  ui->avatarPropsTab->setCurrentIndex(0);
  enableProps(false);

  if(!node)
  {
    qCWarning(lcView,"partClicked(node,...): node==0!");
    return;
  }

  if(node)
  {
    currentPart=node;
    if(m_hintCards) m_hintCards->setPicked(true);
    if(m_helper) m_helper->notify(QLatin1String(HelperSpec::picked));

    // show the joint in the joint table, on its tab
    const int track=ui->animationView->partNames().indexOf(currentPart->name());
    if(track>=0) selectTrackInTable(track);

    // do not send signals for moving sliders while changing the slider limits
    ui->xRotationSlider->blockSignals(true);
    ui->yRotationSlider->blockSignals(true);
    ui->zRotationSlider->blockSignals(true);

    // hip gets a full 360 degree limit, all others according to SL.lim
    // maybe this shouldn't be like this to allow for multi rotation spins
    if(currentPart->type==BVH_ROOT)
    {
      ui->xRotationSlider->setRange(-359 * m_precision, 359 * m_precision);
      ui->yRotationSlider->setRange(-359 * m_precision, 359 * m_precision);
      ui->zRotationSlider->setRange(-359 * m_precision, 359 * m_precision);
    }
    else
    {
      ui->xRotationSlider->setRange(SliderScale::toSlider(limits.minimum.x()), SliderScale::toSlider(limits.maximum.x()));
      ui->yRotationSlider->setRange(SliderScale::toSlider(limits.minimum.y()), SliderScale::toSlider(limits.maximum.y()));
      ui->zRotationSlider->setRange(SliderScale::toSlider(limits.minimum.z()), SliderScale::toSlider(limits.maximum.z()));
    }

    // re-enable signals
    ui->xRotationSlider->blockSignals(false);
    ui->yRotationSlider->blockSignals(false);
    ui->zRotationSlider->blockSignals(false);

    setX(rotation.x());
    setY(rotation.y());
    setZ(rotation.z());

    if(node->type==BVH_POS)
      enableAvatarRotation(false);
    else
      enableAvatarRotation(!protect);

    setXPos(position.x());
    setYPos(position.y());
    setZPos(position.z());

    // show the user if this part has a key frame here
    updateKeyBtn();
  }
}

// slot gets called by AnimationView::mouseMoveEvent()
void PinglyMotionWindow::partDragged(BVHNode* node,double x,double y,double z)
{
  if(m_hintCards) m_hintCards->markTurned();
  if(m_helper) m_helper->notify(QLatin1String(HelperSpec::turned));
  if(node)
  {
    // check if this frame is protected
    if(!protect)
    {
      // get animation object
      Animation* anim=scene->getAnimation();
      // get rotation values for selected part
      QVector3D rotation = anim->getRotation(node);
      // get rotation limits for part
      RotationLimits rotLimits=anim->getRotationLimits(node);

      // calculate new rotation (x, y, z are the modifiers)
      double newX = rotation.x() + x;
      double newY = rotation.y() + y;
      double newZ = rotation.z() + z;

      double xMin=rotLimits.minimum.x();
      double yMin=rotLimits.minimum.y();
      double zMin=rotLimits.minimum.z();
      double xMax=rotLimits.maximum.x();
      double yMax=rotLimits.maximum.y();
      double zMax=rotLimits.maximum.z();

      if(newX<xMin) newX=xMin;
      if(newX>xMax) newX=xMax;
      if(newY<yMin) newY=yMin;
      if(newY>yMax) newY=yMax;
      if(newZ<zMin) newZ=zMin;
      if(newZ>zMax) newZ=zMax;

      setX(newX);
      setY(newY);
      setZ(newZ);

      scene->getAnimation()->setRotation(node,newX,newY,newZ);
      ui->animationView->update();

      updateKeyBtn();
    }
  }
  else qCWarning(lcView,"partDragged(): node==0!");
}

// slot gets called by AnimationView::propClicked()
void PinglyMotionWindow::propClicked(Prop* prop)
{
  ui->avatarPropsTab->setCurrentIndex(1);

  // update prop name combo box
  for(int index=0;index<ui->propNameCombo->count();index++)
    if(ui->propNameCombo->itemText(index)==prop->name()) ui->propNameCombo->setCurrentIndex(index);

  // update prop value spinboxes
  selectProp();
}

// slot gets called by AnimationView::mouseButtonClicked()
void PinglyMotionWindow::backgroundClicked()
{
  currentPart=0;

  enableAvatarRotation(false);
  enableProps(false);
  ui->timelineView->easingTiles()->setNoKey();
  selectTrackInTable(0);
  updateKeyBtn();
}

// Adds a file to the open files list, and adds the entry in the combo box
void PinglyMotionWindow::addToOpenFiles(const QString& fileName)
{
    openFiles.append(fileName);

    QString fixedName=fileName;
    fixedName.remove(QRegularExpression(".*/"));
    fixedName.remove(QRegularExpression("(\\.bvh|\\.avm|\\.dingle)"));

    ui->selectAnimationCombo->addItem(fixedName);
}

// asks what to do with unsaved animations: Save (default), Discard or Cancel.
// returns true when it is fine to go on (saved or discarded)
bool PinglyMotionWindow::askToSaveChanges()
{
  QList<Animation*> unsaved;
  for(int index=0;index<animationIds.count();index++)
    if(animationIds.at(index)->dirty()) unsaved.append(animationIds.at(index));
  if(unsaved.isEmpty()) return true;

  QString text;
  if(unsaved.count()==1)
  {
    const int index=animationIds.indexOf(unsaved.first());
    text=tr("Do you want to save the changes to \"%1\"?").arg(ui->selectAnimationCombo->itemText(index));
  }
  else
    text=tr("%1 animations have unsaved changes. Do you want to save them?").arg(unsaved.count());

  QMessageBox box(QMessageBox::Warning,tr("Unsaved Changes"),text,
                  QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,this);
  box.setTextFormat(Qt::PlainText);
  box.setInformativeText(tr("If you don't save, your changes will be lost."));
  box.setDefaultButton(QMessageBox::Save);
  box.setEscapeButton(QMessageBox::Cancel);
  box.button(QMessageBox::Discard)->setText(tr("Don't Save"));

  // the box runs a nested event loop: the animations are held by QPointer and found again
  // afterwards, since one may be replaced or deleted while it is open (src/documentwatch.h)
  QList<QPointer<Animation>> toSave;
  for(Animation* animation : unsaved) toSave.append(animation);
  unsaved.clear();

  const int answer=box.exec();
  if(answer==QMessageBox::Discard) return true;
  if(answer!=QMessageBox::Save) return false;

  // save each of them; stop if one is not saved (cancelled or failed) or is gone
  for(const QPointer<Animation>& animation : toSave)
  {
    if(!animation || !animationIds.contains(animation.data()))
    {
      statusBar()->showMessage(DocumentWatch::changedMessage(),10000);
      return false;
    }
    selectAnimation(animation.data());
    if(!fileSave() || !animation || animation->dirty()) return false;
  }
  return true;
}

// empty out the open files list
bool PinglyMotionWindow::clearOpenFiles()
{
  if(!askToSaveChanges()) return false;

  timeline->setAnimation(0);
  // two figures (6m): the places go with the animations
  for(Animation* anim : animationIds) ui->animationView->setFigureOffset(anim,0.0f);
  m_figurePlace.clear();
  scene->clear();
  currentPart = NULL;
  openFiles.clear();
  ui->selectAnimationCombo->clear();
  animationIds.clear();
  setCurrentFile(m_untitledName);
  longestRunningTime=0.0;
  refreshFigures();

  return true;
}

// convenience function to set window title in a defined way
void PinglyMotionWindow::setCurrentFile(const QString& fileName)
{
  // "Pose: changed from ..." on the Hands tab belongs to the file it was picked in
  if(m_handsPanel && fileName!=currentFile) m_handsPanel->forgetPicks();
  currentFile=fileName;
  // a clip opened from the library gets the "Open now" chip in the Library panel
  if(m_libraryPanel) m_libraryPanel->setOpenFile(fileName);
  updateWindowTitle();
}

// this slot gets called from Animation::setFrame(int)
void PinglyMotionWindow::setCurrentFrame(int frame)
{
  // make sure current frame is only updated when no animation is playing (manual change,
  // program startup) or that only the currently selected animation updates the frame
  // position, so we don't have jumping back and forth while playing multiple animations
  if(playstate==PLAYSTATE_STOPPED || sender()==animationIds.at(ui->selectAnimationCombo->currentIndex()))
  {
    ui->currentFrameSlider->blockSignals(true);
    ui->currentFrameSlider->setValue(frame);
    ui->currentFrameSlider->blockSignals(false);
    ui->currentFrameLabel->setText(QString::number(frame+1));

    timeline->setCurrentFrame(frame);
    // check if we are at the first frame and if it's protected
    if(frame==0 && UserSettings::protectFirstFrame()) protect=true;
    else protect=false;
    emit protectFrame(protect);
    updateInputs();
    updateKeyBtn();
  }
}

// gets called by selecting an animation from the animation combo box
void PinglyMotionWindow::animationChanged(int which)
{

  // safety to check if "which" is out of bounds of loaded animations
  if(which>=openFiles.count()) return;

  // get animation pointer
  Animation* anim=animationIds.at(which);
  // select animation (will also update combo box, but better than duplicate code)
  selectAnimation(anim);
  ui->animationView->selectPart(0);
  currentPart = nullptr;
}

// gets called from AnimationView::animationSelected()
void PinglyMotionWindow::selectAnimation(Animation* animation)
{
  // find animation index in list of open files
  for(unsigned int index=0;index< (unsigned int) animationIds.count();index++)
  {
    // index found
    if(animationIds.at(index)==animation)
    {
      // prevent signal looping
      ui->animationView->blockSignals(true);
      scene->blockSignals(true);
      // update animation combo box
      ui->selectAnimationCombo->setCurrentIndex(index);
      // update window title
      setCurrentFile(openFiles.at(index));
      // update animation view (might already be active, depending on how this function was called)
      scene->selectAnimation(index);
      // re-enable signals
      ui->animationView->blockSignals(false);
      scene->blockSignals(false);
    }
  } // for

  // update avatar figure combo box
  int figure=0;
  if(animation->getFigureType()==Animation::FIGURE_MALE) figure=1;
  ui->figureCombo->setCurrentIndex(figure);

  // update timeline
  timeline->setAnimation(animation);
  updateInputs();
  // Undo and Redo work on this animation's own history
  if(m_undo) m_undo->setAnimation(animation);

  // enable export to second life if current file name is not the default untitled name
  if(!m_empty) ui->fileExportForSecondLifeAction->setEnabled(currentFile != m_untitledName);

  // two figures (6m): the box, the tags, the header rows and the chip follow the active figure
  refreshFigures();
}




// options, window place and panel layout, remembered for next time
void PinglyMotionWindow::saveUserSettings()
{
  UserSettings::setJointLimits(ui->optionsJointLimitsAction->isChecked());
  UserSettings::setProtectFirstFrame(ui->optionsProtectFirstFrameAction->isChecked());

  // panels hidden by Tab are not remembered: the program starts with them shown
  if(m_tabState.isHidden()) togglePanelsHidden();
  UserSettings::setMainWindowGeometry(saveGeometry());
  // the user's own wide layout, also while the panels are rearranged (docs/UX_DECISIONS.md, 6c)
  // Animate and Script each keep their layout: the side shown now is saved into its own slot
  m_workMode->saveCurrentLayout();

  UserSettings::writeSettings();
}

void PinglyMotionWindow::closeEvent(QCloseEvent* event)
{
  if(!clearOpenFiles())
    event->ignore();
  else
  {
    // closing with the window's X button keeps the settings too
    saveUserSettings();
    event->accept();
  }
}

// calculates the longest running time of all animations
double PinglyMotionWindow::calculateLongestRunningTime()
{
  longestRunningTime=0.0;
  for(unsigned int index=0;index< (unsigned int) animationIds.count();index++)
  {
    Animation* anim=animationIds.at(index);
    double time=anim->getNumberOfFrames()/((double) anim->fps());
    if(time>longestRunningTime) longestRunningTime=time;
  }
  return longestRunningTime;
}
