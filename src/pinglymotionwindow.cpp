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
#include "panelcontent.h"
#include "selectedjointpanel.h"
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
#include "playback.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/cameraoverride.h"
#include "scriptlab/environmentoverride.h"
#include "scriptlab/viewmodesoverride.h"
#include "scriptlab/effectspreview.h"
#include "theme.h"
#include "menuspec.h"
#include "emptystate.h"
#include "guidancespec.h"
#include "hintcard.h"
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

PinglyMotionWindow::PinglyMotionWindow() :
  QMainWindow(nullptr),
  ui(new Ui::PinglyMotionWindow),
  m_pinglyMotion(new PinglyMotion(this)),
  updateChecker(nullptr, this),
  cubeMesh(new Mesh(Constants::cubeMeshFilePath(), Mesh::Shape::CUBE)),
  sphereMesh(new Mesh(QString(), Mesh::Shape::SPHERE)),  // built in code, no file
  coneMesh(new Mesh(QString(), Mesh::Shape::CONE)),      // built in code, no file
  torusMesh(new Mesh(Constants::torusMeshFilePath(), Mesh::Shape::TORUS)),
  m_animFilter(PinglyMotionWindow::tr("Animation Files (*.dingle *.avm *.bvh)")),
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
      QSize(850,600));
    if(!startWindow.useGeometry || !restoreGeometry(UserSettings::mainWindowGeometry()))
      resize(startWindow.size);
  }

  ui->optionsLoopAction->setChecked(UserSettings::loop());
  m_transport->setLoop(UserSettings::loop());
  ui->optionsSkeletonAction->setChecked(UserSettings::skeleton());
  ui->optionsJointLimitsAction->setChecked(UserSettings::jointLimits());

  ui->optionsProtectFirstFrameAction->setChecked(UserSettings::protectFirstFrame());

  // Options > Body: which avatar the 3D view shows (only what you see, not the animation)
  QMenu* bodyMenu=new QMenu(tr("Body"),this);
  bodyMenu->setToolTipsVisible(true);
  ui->menuOptions->addMenu(bodyMenu);
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

  // Options > Joints: show or hide groups of Bento joints (display only)
  QMenu* jointsMenu=ui->menuOptions->addMenu(tr("Joints"));
  const struct { int group; const char* text; } groups[]=
  {
    {JointGroups::Wings,QT_TR_NOOP("Wings")},
    {JointGroups::Tail,QT_TR_NOOP("Tail")},
    {JointGroups::HindLimbs,QT_TR_NOOP("Hind limbs")},
    {JointGroups::Face,QT_TR_NOOP("Face")},
    {JointGroups::FingersToes,QT_TR_NOOP("Fingers and toes")}
  };
  for(const auto& entry : groups)
  {
    QAction* action=jointsMenu->addAction(tr(entry.text));
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
  QAction* hideInListsAction=jointsMenu->addAction(tr("Hide in lists"));
  hideInListsAction->setCheckable(true);
  hideInListsAction->setToolTip(tr("Leaves hidden joints out of the timeline and the joint list. Off: they show dimmed."));
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
  QAction* lightAction=new QAction(tr("Lighting..."),this);
  lightAction->setToolTip(tr("Change how the body is lit: brightness, shine, warmth and the shadow on the floor."));
  ui->menuOptions->insertAction(bodyMenu->menuAction(),lightAction);
  connect(lightAction,&QAction::triggered,this,&PinglyMotionWindow::showLightDialog);

  // panel layout from last time (size, place, floating panels). Only a layout
  // saved by this version of the panels is loaded (docs/archive/PANELS_PLAN.md, 7);
  // the old window_state with the Properties panel is left alone, so the
  // default layout is used once after the move to three panels.
  const PanelLayout::StartupChoice startup=PanelLayout::chooseStartup(
    UserSettings::panelLayout(),UserSettings::panelLayoutVersion(),size(),true);
  if(startup.useSavedLayout) restoreState(UserSettings::panelLayout(),PanelLayout::CurrentVersion);
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
  // the timeline stands visible at every start; hiding it lasts for this session only
  PanelVisibility::showAtStart(m_timelineDock,ui->optionsShowTimelineAction);
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

  connect(ui->animationView, &AnimationView::propDragged, this, &PinglyMotionWindow::propDragged);

  connect(ui->animationView, &AnimationView::propScaled, this, &PinglyMotionWindow::propScaled);

  connect(ui->animationView, &AnimationView::propRotated, this, &PinglyMotionWindow::propRotated);

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
  // the Script Lab panel shows and changes the same RLV state as rlv_command (issue 288)
  m_scriptLabPanel->setState(&m_controlApi->rlv());
  m_scriptLabPanel->setSession(&m_controlApi->lslSession());
  // and the same folder watch as lab_watch (issue 292)
  m_scriptLabPanel->setWatcher(&m_controlApi->lab());
  // and shows what a Live run does in the 3D view, off until its box is ticked (issue 293)
  EffectsPreview* effects=new EffectsPreview(this);
  ui->animationView->setEffectsPreview(effects);
  m_scriptLabPanel->setEffects(effects);
  // and props as prims (lane 1 job E): a prop named like a prim of the watched object shows the
  // colour and transparency its scripts gave that prim; the other props keep their own colour
  connect(m_scriptLabPanel,&ScriptLabPanel::propTintsChanged,this,[this]()
  {
    const QList<LabTint::Tint> tints=m_scriptLabPanel->propTints();
    PropManager* props=scene->propManager();
    for(int i=0;i<props->count();++i)
    {
      Prop* prop=props->at(i);
      prop->clearTint();
      for(const LabTint::Tint& tint : tints)
      {
        if(tint.prim!=prop->name()) continue;
        const QColor colour=tint.hasColour ? QColor::fromRgbF(tint.colour[0],tint.colour[1],tint.colour[2]) : prop->drawColour();
        prop->setTint(colour,tint.hasAlpha ? tint.alpha : 1.0f);
      }
    }
    ui->animationView->update();
  });
  // and lets the RLV camera limits limit the 3D view's camera, off until its box is ticked (job 7b)
  CameraOverride* cameraOverride=new CameraOverride(this);
  cameraOverride->setState(&m_controlApi->rlv());
  ui->animationView->setCameraOverride(cameraOverride);
  m_scriptLabPanel->setCameraOverride(cameraOverride);
  // and shows the sky and draw distance a script asks for as the background and fog, off until its box is ticked (job 7c)
  EnvironmentOverride* environmentOverride=new EnvironmentOverride(this);
  environmentOverride->setState(&m_controlApi->rlv());
  ui->animationView->setEnvironmentOverride(environmentOverride);
  m_scriptLabPanel->setEnvironmentOverride(environmentOverride);
  // and the view modes a script holds (hidden avatar, plain textures, overlay), off until its box is ticked (job 7c)
  ViewModesOverride* viewModesOverride=new ViewModesOverride(this);
  viewModesOverride->setState(&m_controlApi->rlv());
  ui->animationView->setViewModesOverride(viewModesOverride);
  m_scriptLabPanel->setViewModesOverride(viewModesOverride);

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
  ui->menuOptions->addAction(m_auroraAction);
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

// slot gets called by AnimationView::mouseMoveEvent()
void PinglyMotionWindow::propDragged(Prop* prop,double x,double y,double z)
{
  double newX = prop->xPosition() + x;
  double newY = prop->yPosition() + y;
  double newZ = prop->zPosition() + z;
  prop->setPosition(newX, newY, newZ);
  updatePropSpins(prop->position(), prop->rotation(), prop->scale());
}

// slot gets called by AnimationView::mouseMoveEvent()
void PinglyMotionWindow::propScaled(Prop* prop,double x,double y,double z)
{
  double newX = prop->xScale() + x;
  double newY = prop->yScale() + y;
  double newZ = prop->zScale() + z;
  prop->setScale(newX, newY, newZ);
  updatePropSpins(prop->position(), prop->rotation(), prop->scale());
}

// slot gets called by AnimationView::mouseMoveEvent()
void PinglyMotionWindow::propRotated(Prop* prop,double x,double y,double z)
{
  double newX = prop->xRotation() + x;
  double newY = prop->yRotation() + y;
  double newZ = prop->zRotation() + z;
  prop->setRotation(newX, newY, newZ);
  updatePropSpins(prop->position(), prop->rotation(), prop->scale());
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
  // are saved with the layout like the other panels.
  m_scriptLabPanel=new ScriptLabPanel;
  m_scriptLabDock=ScriptLabPanel::makeDock(this,m_scriptLabPanel);

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

  ui->menuView->addSection(QCoreApplication::translate("MenuSpec",panelsHeading));
  ui->menuView->addAction(showJointsAction);
  ui->menuView->addAction(showJointCardAction);
  ui->menuView->addAction(ui->optionsShowTimelineAction);
  ui->menuView->addAction(scriptLabAction);
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
    UserSettings::setExpertMode(on);
    // the timeline's Paste mirrored button is an Expert mode control (UX phase 3b, the mockup)
    if(m_panels && m_panels->pasteMirrored) m_panels->pasteMirrored->setVisible(on);
    // and the Hands tab's joint grid (UX phase 2c)
    refreshHandsPanel();
    // the Edit curve button follows through UserSettings::watchExpertMode; the mockup's sentences
    statusBar()->showMessage(on ? GuidanceSpec::tr(GuidanceSpec::expertOnMessage,"GuidanceSpec")
                                : GuidanceSpec::tr(GuidanceSpec::expertOffMessage,"GuidanceSpec"),5000);
  });
  ui->menuView->addAction(m_expertModeAction);

  m_textSizeAction=specAction(this,viewTextSize,false,text(viewTextSize).arg(UserSettings::textSizePercent()));
  connect(m_textSizeAction,&QAction::triggered,this,[this]()
  {
    setTextSizePercent(TextSize::stepUp(UserSettings::textSizePercent()));
  });
  ui->menuView->addAction(m_textSizeAction);
  QAction* textSmallerAction=specAction(this,viewTextSmaller,false);
  connect(textSmallerAction,&QAction::triggered,this,[this]()
  {
    setTextSizePercent(TextSize::stepDown(UserSettings::textSizePercent()));
  });
  ui->menuView->addAction(textSmallerAction);
  QAction* textResetAction=specAction(this,viewTextReset,false);
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
    if(m_selectedJoint) m_selectedJoint->setHintsOn(on);
    refreshHandsPanel();
    m_tipsSwitch->setOn(on);
    if(m_hintCards) m_hintCards->setHintsOn(on);   // the hint cards follow the setting (UX phase 6a)
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

// UX phase 1b: Edit as in the mockup: Undo, Redo, Set key, Paste mirrored. Undo and Redo are not
// built yet and are greyed with a reason; Set key presses the key button. Cut, Copy and Paste (the
// timeline's selected stretch, UX phase 3b) stay below them: the mockup has no such items yet.
void PinglyMotionWindow::buildEditMenu()
{
  using namespace MenuSpec;
  ui->menuEdit->setToolTipsVisible(true);
  ui->menuEdit->clear();

  m_undoAction=specAction(this,editUndo,true);
  m_redoAction=specAction(this,editRedo,true);
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
  m_topBar=new TopBar(ui->toolBar,sources,this);
  m_topBar->setDark(UserSettings::darkTheme());
  connect(m_topBar,&TopBar::themeRequested,this,[this]()
  {
    if(m_lightThemeAction) m_lightThemeAction->toggle();
  });
  connect(m_topBar,&TopBar::setupRequested,this,&PinglyMotionWindow::openSetupDialog);

  setupViewToolbar();

  m_emptyState=new EmptyState(ui->animationView);
  m_emptyState->hide();
  connect(m_emptyState,&EmptyState::startChosen,this,&PinglyMotionWindow::startFromLibrary);
  connect(m_emptyState,&EmptyState::newChosen,this,[this](){ fileNew(); });

  buildHintCards();
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
  m_viewToolbar->setPartShown(Part::Floor,UserSettings::showFloor());
  m_viewToolbar->setPartShown(Part::Rings,UserSettings::showRings());
  m_viewToolbar->setPartShown(Part::JointDots,UserSettings::skeleton());
  m_viewToolbar->setPartShown(Part::ToolNames,UserSettings::viewToolNames());

  connect(m_viewToolbar,&ViewToolbar::toolChosen,this,&PinglyMotionWindow::chooseViewTool);
  connect(view,&AnimationView::viewToolRequested,this,&PinglyMotionWindow::chooseViewTool);
  connect(m_viewToolbar,&ViewToolbar::partToggled,this,&PinglyMotionWindow::chooseViewPart);
  // Joint dots is Options > Show skeleton: either one changes both
  connect(ui->optionsSkeletonAction,&QAction::toggled,m_viewToolbar,[this](bool on)
  {
    m_viewToolbar->setPartShown(ViewToolSpec::Part::JointDots,on);
  });
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
      // the same setting as Options > Show skeleton, through its own action
      ui->optionsSkeletonAction->setChecked(shown);
      break;
    case Part::ToolNames:
      UserSettings::setViewToolNames(shown);
      break;
  }
  if(m_viewToolbar) m_viewToolbar->setPartShown(part,shown);
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

// no animation open: the empty state covers the 3D view, the panels and the actions that need an
// animation are greyed with "Open an animation first."
void PinglyMotionWindow::setEmptyMode(bool empty)
{
  if(empty==m_empty) return;
  m_empty=empty;

  const QList<QAction*> needing={ui->fileSaveAction,ui->fileSaveAsAction,ui->fileExportAnimAction,
    ui->fileExportForSecondLifeAction,ui->fileCloseAction,ui->fileAddAction,ui->editCutAction,
    ui->editCopyAction,ui->editPasteAction,ui->toolsMirrorAction,ui->toolsOptimizeBVHAction};
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
  // the view toolbar has nothing to look at while the empty state covers the 3D view
  if(m_viewToolbar) m_viewToolbar->setVisible(!empty);
  // the timeline shows one line instead of its rows (UX phase 3a)
  ui->timelineView->setNoAnimation(empty);
  updateEditActions();
  updateWindowTitle();
}

QList<QDockWidget*> PinglyMotionWindow::panelDocks() const
{
  return {m_jointsDock,m_jointCardDock,m_timelineDock,m_scriptLabDock};
}

// View > Reset layout: the default layout built in code, all three panels
// docked and shown. Panels hidden by Tab count as shown again.
void PinglyMotionWindow::resetPanelLayout()
{
  m_applyingTab=true;
  m_tabState.reset();
  PanelWindow::applyDefaultLayout(this,m_jointsDock,m_jointCardDock,m_timelineDock);
  // the Script Lab goes back to its side and closes, as at the first start
  m_scriptLabDock->setFloating(false);
  addDockWidget(Qt::RightDockWidgetArea,m_scriptLabDock);
  m_scriptLabDock->hide();
  // this is the wide layout now; rearranged again if the window has no room for it
  m_reflower.wideLayoutChanged(this,reflowDocks());
  m_applyingTab=false;
  m_hidePanelsAction->setChecked(false);
  ui->optionsShowTimelineAction->setChecked(true);
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
  // every widget gets a font change event; this window's changeEvent fits the panels
  TextSize::apply(UserSettings::textSizePercent());
  if(m_textSizeAction)
    m_textSizeAction->setText(MenuSpec::text(MenuSpec::viewTextSize).arg(UserSettings::textSizePercent()));
  // rearranged at once, before the window is laid out for the new size, so a
  // layout that no longer fits never makes the window grow
  updateReflow();
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

// joint picker in the properties panel: tabs Body / Face / Hands / Other and
// a search box above a table with the joints of that tab, by friendly name
// under section headings (#14, step 2). Replaces the old "Edit Part"
// drop-down list. Only the list is filtered; the 3D view shows all joints.
void PinglyMotionWindow::buildJointPicker()
{
  m_jointInfo.reset(new JointInfo);
  QString error;
  if(!m_jointInfo->load(Constants::pinglyMotionDataPath()+"/firestorm",&error))
    qWarning("joint names not loaded, the joint list shows file names: %s",qPrintable(error));

  // Left / Right / Mirror (posing UX phase 2a, docs/mockup): which side of a
  // mirror pair an edit also changes (Animation::setMirrored()), and which
  // side the joint table below lists (JointList::sideOf()). Mirror (both
  // sides) is the default, as in the mockup.
  QWidget* sideRow=new QWidget(ui->jointPicker);
  sideRow->setObjectName("jointSideRow");
  QHBoxLayout* sideLayout=new QHBoxLayout(sideRow);
  sideLayout->setContentsMargins(0,0,0,0);

  m_leftSideButton=new QPushButton(tr("Left"),sideRow);
  m_leftSideButton->setObjectName("jointSideLeftButton");
  m_leftSideButton->setCheckable(true);
  m_leftSideButton->setToolTip(tr("Left side only"));
  m_leftSideButton->setStatusTip(m_leftSideButton->toolTip());

  m_rightSideButton=new QPushButton(tr("Right"),sideRow);
  m_rightSideButton->setObjectName("jointSideRightButton");
  m_rightSideButton->setCheckable(true);
  m_rightSideButton->setToolTip(tr("Right side only"));
  m_rightSideButton->setStatusTip(m_rightSideButton->toolTip());

  m_mirrorSideButton=new QPushButton(tr("Mirror"),sideRow);
  m_mirrorSideButton->setObjectName("jointSideMirrorButton");
  m_mirrorSideButton->setCheckable(true);
  m_mirrorSideButton->setChecked(true);
  m_mirrorSideButton->setIcon(LucideIcons::icon(QStringLiteral("flip-horizontal-2"),
                                                 Theme::color(Theme::Token::Text),
                                                 Theme::color(Theme::Token::TextDisabled)));
  m_mirrorSideButton->setToolTip(tr("Mirror: turning one side turns the other side too"));
  m_mirrorSideButton->setStatusTip(m_mirrorSideButton->toolTip());

  m_sideGroup=new QButtonGroup(this);
  m_sideGroup->setExclusive(true);
  m_sideGroup->addButton(m_leftSideButton);
  m_sideGroup->addButton(m_rightSideButton);
  m_sideGroup->addButton(m_mirrorSideButton);

  sideLayout->addWidget(m_leftSideButton);
  sideLayout->addWidget(m_rightSideButton);
  sideLayout->addWidget(m_mirrorSideButton);

  connect(m_leftSideButton,&QPushButton::clicked,this,[this]() { setJointSide(QLatin1Char('L')); });
  connect(m_rightSideButton,&QPushButton::clicked,this,[this]() { setJointSide(QLatin1Char('R')); });
  connect(m_mirrorSideButton,&QPushButton::clicked,this,[this]() { setJointSide(QChar()); });

  m_jointTabs=new QTabBar(ui->jointPicker);
  m_jointTabs->setObjectName("jointTabs");
  m_jointTabs->setDocumentMode(true);
  m_jointTabs->setExpanding(true);
  m_jointTabs->setDrawBase(false);
  // same order as JointGroups::Tab
  m_jointTabs->addTab(tr("Body"));
  m_jointTabs->addTab(tr("Face"));
  m_jointTabs->addTab(tr("Hands"));
  m_jointTabs->addTab(tr("Other"));

  m_jointSearch=new QLineEdit(ui->jointPicker);
  m_jointSearch->setObjectName("jointSearch");
  m_jointSearch->setPlaceholderText(tr("Search joints"));
  m_jointSearch->setClearButtonEnabled(true);
  m_jointSearch->setToolTip(tr("Type part of a joint name, for example \"elbow\" or \"left hand\".\n"
                               "Finds joints on all tabs. Press Enter to pick the first one."));

  m_jointTable=new QTableWidget(ui->jointPicker);
  m_jointTable->setObjectName("jointTable");
  m_jointTable->setColumnCount(1);
  m_jointTable->setHorizontalHeaderLabels(QStringList() << tr("Joint"));
  m_jointTable->horizontalHeader()->setStretchLastSection(true);
  m_jointTable->horizontalHeader()->setHighlightSections(false);
  m_jointTable->verticalHeader()->hide();
  m_jointTable->verticalHeader()->setDefaultSectionSize(m_jointTable->fontMetrics().height()+6);
  m_jointTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_jointTable->setSelectionMode(QAbstractItemView::SingleSelection);
  m_jointTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_jointTable->setShowGrid(false);
  m_jointTable->setAlternatingRowColors(true);
  m_jointTable->setMinimumHeight(10*m_jointTable->verticalHeader()->defaultSectionSize());

  // the Hands tab (UX phase 2c, docs/mockup): ready-made finger poses, Bend each
  // finger and the joint grid, shown instead of the table on the Hands tab
  m_handsPanel=new HandsPanel(HandPose::presets(*m_jointInfo),ui->jointPicker);
  m_handsPanel->hide();
  connect(m_handsPanel,&HandsPanel::changeRequested,this,&PinglyMotionWindow::handsChangeRequested);
  connect(m_handsPanel,&HandsPanel::jointPicked,this,&PinglyMotionWindow::handsJointPicked);
  connect(m_handsPanel,&HandsPanel::statusMessage,this,[this](const QString& text) { statusBar()->showMessage(text,5000); });

  // Whole pose: Flip, T-pose and Reset all, every joint of the current
  // frame, not just the selected one (posing UX phase 2a, docs/mockup)
  QWidget* wholePoseRow=new QWidget(ui->jointPicker);
  wholePoseRow->setObjectName("wholePoseRow");
  QHBoxLayout* wholePoseLayout=new QHBoxLayout(wholePoseRow);
  wholePoseLayout->setContentsMargins(0,0,0,0);

  m_flipPoseButton=new QPushButton(tr("Flip"),wholePoseRow);
  m_flipPoseButton->setObjectName("flipPoseButton");
  m_flipPoseButton->setIcon(LucideIcons::icon(QStringLiteral("flip-horizontal-2"),
                                               Theme::color(Theme::Token::Text),
                                               Theme::color(Theme::Token::TextDisabled)));
  m_flipPoseButton->setToolTip(tr("Flip the whole pose left to right"));
  m_flipPoseButton->setStatusTip(m_flipPoseButton->toolTip());

  m_tPoseButton=new QPushButton(tr("T-pose"),wholePoseRow);
  m_tPoseButton->setObjectName("tPoseButton");
  m_tPoseButton->setToolTip(tr("T-pose: arms straight out to the sides"));
  m_tPoseButton->setStatusTip(m_tPoseButton->toolTip());

  m_resetAllButton=new QPushButton(tr("Reset all"),wholePoseRow);
  m_resetAllButton->setObjectName("resetAllButton");
  m_resetAllButton->setIcon(LucideIcons::icon(QStringLiteral("rotate-ccw"),
                                               Theme::color(Theme::Token::Text),
                                               Theme::color(Theme::Token::TextDisabled)));
  m_resetAllButton->setToolTip(tr("Reset all joints of this frame"));
  m_resetAllButton->setStatusTip(m_resetAllButton->toolTip());
  // the danger button of docs/DESIGN_SYSTEM.md, set directly: this row is
  // plain Qt widgets, not transportlook.cpp's custom-painted buttons (left
  // for a later look pass, docs/UX_DECISIONS.md)
  {
    QPalette dangerPalette=m_resetAllButton->palette();
    dangerPalette.setColor(QPalette::Button,Theme::color(Theme::Token::DangerFill));
    dangerPalette.setColor(QPalette::ButtonText,Theme::color(Theme::Token::OnAccent));
    m_resetAllButton->setPalette(dangerPalette);
    m_resetAllButton->setAutoFillBackground(true);
  }

  wholePoseLayout->addWidget(m_flipPoseButton);
  wholePoseLayout->addWidget(m_tPoseButton);
  wholePoseLayout->addWidget(m_resetAllButton);

  connect(m_flipPoseButton,&QPushButton::clicked,this,&PinglyMotionWindow::flipPoseClicked);
  connect(m_tPoseButton,&QPushButton::clicked,this,&PinglyMotionWindow::tPoseClicked);
  connect(m_resetAllButton,&QPushButton::clicked,this,&PinglyMotionWindow::resetAllClicked);

  QVBoxLayout* layout=new QVBoxLayout(ui->jointPicker);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(2);
  layout->addWidget(sideRow);
  layout->addWidget(m_jointTabs);
  layout->addWidget(m_jointSearch);
  layout->addWidget(m_jointTable);
  layout->addWidget(m_handsPanel);
  layout->addWidget(wholePoseRow);

  // a click on a tab shows that tab again, so the search is cleared
  connect(m_jointTabs,&QTabBar::currentChanged,this,[this](int)
  {
    QSignalBlocker block(m_jointSearch);
    m_jointSearch->clear();
    fillJointTable();
  });
  connect(m_jointSearch,&QLineEdit::textChanged,this,[this](const QString&) { fillJointTable(); });
  // Enter picks the first joint found
  connect(m_jointSearch,&QLineEdit::returnPressed,this,[this]()
  {
    for(int row=0;row<nodeMapping.count();row++)
    {
      if(nodeMapping[row]<0) continue;
      m_jointTable->setCurrentCell(row,0);
      break;
    }
  });
  // a new row (mouse or arrow keys), or a click on the marked row while
  // that joint is not selected in the view (at start, after a background click)
  connect(m_jointTable,&QTableWidget::currentCellChanged,this,[this](int row)
  {
    if(row<0 || row>=nodeMapping.count() || nodeMapping[row]<0) return;
    m_selectedTrack=nodeMapping[row];
    partChoice();
  });
  connect(m_jointTable,&QTableWidget::cellClicked,this,[this](int row)
  {
    if(row<0 || row>=nodeMapping.count() || nodeMapping[row]<0) return;
    const QString name=ui->animationView->partNames().value(nodeMapping[row]);
    if(currentPart && currentPart->name()==name) return;
    m_selectedTrack=nodeMapping[row];
    partChoice();
  });
}

// fill the joint table with the joints of the current tab, or with the joints
// found by the search box (on all tabs)
// track numbers: 0 = position, then every skeleton node, "Site" = End Site
void PinglyMotionWindow::fillJointTable()
{
  const QStringList names=ui->animationView->partNames();
  QVector<bool> hidden(names.count());
  for(int track=0;track<names.count();track++)
    hidden[track]=JointGroups::trackHidden(track);
  const QVector<JointList::Row> rows=JointList::rows(*m_jointInfo,names,m_jointTabs->currentIndex(),
                                                     m_jointSearch->text(),hidden,UserSettings::hideJointsInLists(),
                                                     m_jointSide);

  QSignalBlocker block(m_jointTable);
  nodeMapping.clear();
  m_jointTable->setRowCount(0);
  m_jointTable->setRowCount(rows.count());

  QFont headingFont=m_jointTable->font();
  headingFont.setBold(true);

  for(int row=0;row<rows.count();row++)
  {
    const JointList::Row& r=rows[row];
    QTableWidgetItem* item=new QTableWidgetItem(r.text);
    if(r.heading)
    {
      // not selectable; arrow keys skip it
      item->setFlags(Qt::NoItemFlags);
      item->setFont(headingFont);
      item->setForeground(palette().color(QPalette::Active,QPalette::WindowText));
    }
    else
    {
      // joints under a heading are indented a little
      item->setText(QStringLiteral("   ")+r.text);
      QString tip;
      if(!r.slName.isEmpty())
        tip=tr("Second Life name: %1").arg(r.slName);
      if(r.fileName!=r.slName && r.fileName!=JointList::POSITION_TRACK)
        tip+=(tip.isEmpty() ? QString() : QStringLiteral("\n"))+tr("Name in the file: %1").arg(r.fileName);
      if(r.fileName==JointList::POSITION_TRACK)
        tip=tr("Moves the whole figure.");
      item->setToolTip(tip);
      if(r.dimmed)
        item->setForeground(palette().color(QPalette::Disabled,QPalette::Text));
    }
    m_jointTable->setItem(row,0,item);
    nodeMapping << r.track;
  }
  showSelectedTrack();

  // the Hands tab shows the finger poses instead of the list; a search shows the list again
  if(m_handsPanel)
  {
    const bool hands=m_jointTabs->currentIndex()==JointGroups::HandsTab && m_jointSearch->text().isEmpty();
    m_jointTable->setVisible(!hands);
    m_handsPanel->setVisible(hands);
    if(hands) refreshHandsPanel();
  }
}

// The Hands tab follows the window: an animation open, why nothing can change
// now, the Left / Right / Mirror switch, the picked joint, both hands' angles at
// this frame, Expert mode and hints (UX phase 2c)
void PinglyMotionWindow::refreshHandsPanel()
{
  if(!m_handsPanel || !m_handsPanel->isVisibleTo(ui->jointPicker)) return;
  Animation* anim=scene ? scene->getAnimation() : nullptr;
  HandsPanel::State state;
  state.open=!m_empty && anim;
  if(state.open && playstate!=PLAYSTATE_STOPPED)
    state.blocked=QCoreApplication::translate("MenuSpec",MenuSpec::setKeyStopPlaying);
  else if(state.open && protect)
    state.blocked=QCoreApplication::translate("MenuSpec",MenuSpec::setKeyLocked);
  state.sideMode=m_jointSide;
  state.pickedJoint=currentPart ? currentPart->name() : QString();
  if(state.open)
  {
    state.left=HandPose::read(anim,HandPose::Side::Left);
    state.right=HandPose::read(anim,HandPose::Side::Right);
  }
  state.expert=UserSettings::expertMode();
  state.hints=UserSettings::tipsOn();
  m_handsPanel->setState(state);
}

// a preset, a finger slider, a grid cell or Reset hand: each changed joint gets a key
// at this frame (a turn is a key at once, as with the Selected joint panel's sliders)
void PinglyMotionWindow::handsChangeRequested(const QVector<HandPose::Write>& writes,const QString& message)
{
  Animation* anim=scene->getAnimation();
  if(m_empty || !anim || protect || playstate!=PLAYSTATE_STOPPED) return;
  const int changed=HandPose::apply(anim,writes);
  if(currentPart)
  {
    const QVector3D rotation=anim->getRotation(currentPart);
    setX(rotation.x());
    setY(rotation.y());
    setZ(rotation.z());
  }
  updateKeyBtn();
  ui->animationView->update();
  refreshHandsPanel();
  // the frame as the frame counter shows it (from 1)
  statusBar()->showMessage(message+QLatin1Char(' ')+HandsSpec::keySet(anim->getFrame()+1,changed),5000);
}

// a row of the joint grid was picked: pick that joint, as a click on the figure does
void PinglyMotionWindow::handsJointPicked(const QString& name)
{
  Animation* anim=scene->getAnimation();
  if(m_empty || !anim) return;
  if(BVHNode* node=HandPose::node(anim,name)) ui->animationView->selectPart(node);
}

// mark the selected joint in the table, if it is listed
void PinglyMotionWindow::showSelectedTrack()
{
  QSignalBlocker block(m_jointTable);
  const int row=(m_selectedTrack<0) ? -1 : nodeMapping.indexOf(m_selectedTrack);
  if(row<0)
  {
    m_jointTable->clearSelection();
    m_jointTable->setCurrentItem(nullptr);
    return;
  }
  m_jointTable->setCurrentCell(row,0);
  m_jointTable->scrollToItem(m_jointTable->item(row,0));
}

// select a joint in the table and switch to its tab (does not select it in the view);
// while searching, the search stays if the joint is among the joints found
void PinglyMotionWindow::selectTrackInTable(int track)
{
  m_selectedTrack=track;
  const QStringList names=ui->animationView->partNames();
  if(track>=0 && track<names.count())
  {
    const bool listed=nodeMapping.contains(track);
    const int tab=JointGroups::tabOf(names[track]);
    if(!listed && !m_jointSearch->text().isEmpty())
    {
      QSignalBlocker block(m_jointSearch);
      m_jointSearch->clear();
    }
    if(!listed && tab!=m_jointTabs->currentIndex())
    {
      QSignalBlocker block(m_jointTabs);
      m_jointTabs->setCurrentIndex(tab);
    }
    // a joint of the side the Left / Right control is hiding (picked in the
    // 3D view, or by a track number): Mirror shows both sides again
    if(!listed && !m_jointSide.isNull() && JointList::sideOf(*m_jointInfo,names[track])==
       (m_jointSide==QLatin1Char('L') ? QChar('R') : QChar('L')))
    {
      setJointSide(QChar());
      return;
    }
    if(!listed)
    {
      fillJointTable();
      return;
    }
  }
  showSelectedTrack();
}

// Left / Right / Mirror (posing UX phase 2a): which side of a mirror pair an
// edit also changes, and which side the joint table lists
void PinglyMotionWindow::setJointSide(QChar side)
{
  if(m_jointSide==side) return;
  m_jointSide=side;
  if(Animation* anim=scene->getAnimation())
    anim->setMirrored(side.isNull());
  fillJointTable();
}

void PinglyMotionWindow::flipPoseClicked()
{
  Animation* anim=scene->getAnimation();
  if(!anim) return;
  anim->flipPose();
  updateInputs();
}

void PinglyMotionWindow::tPoseClicked()
{
  Animation* anim=scene->getAnimation();
  if(!anim) return;
  anim->tPose();
  updateInputs();
}

void PinglyMotionWindow::resetAllClicked()
{
  Animation* anim=scene->getAnimation();
  if(!anim) return;
  anim->resetAllToFrameZero();
  updateInputs();
}

// The Selected joint panel shows "Nothing to turn yet" without an animation,
// "Pick a joint" without a picked joint, else the picked joint (UX phase 2b)
void PinglyMotionWindow::refreshJointPanel()
{
  refreshHandsPanel();
  if(!m_selectedJoint) return;
  Animation* anim=scene ? scene->getAnimation() : nullptr;
  if(m_empty || !anim)
  {
    m_selectedJoint->showNothing();
    return;
  }
  if(!currentPart)
  {
    m_selectedJoint->showPickJoint();
    return;
  }
  SelectedJointPanel::Joint joint;
  const QString fileName=currentPart->name();
  const bool isPosition=currentPart->type==BVH_POS;
  joint.name=isPosition ? JointList::friendlyName(*m_jointInfo,JointList::POSITION_TRACK) : m_jointInfo->uniqueName(fileName);
  if(joint.name.isEmpty()) joint.name=JointList::friendlyName(*m_jointInfo,fileName);
  if(const JointInfo::Joint* info=m_jointInfo->joint(fileName)) joint.slName=info->sl;
  joint.area=isPosition ? static_cast<int>(JointGroups::OtherTab) : static_cast<int>(JointGroups::tabOf(fileName));
  joint.keys=currentPart->numKeyframes();
  joint.canTurn=!isPosition;
  joint.canMove=isPosition || currentPart->type==BVH_ROOT;
  // changed: not as in the first frame, so Reset joint has something to do
  if(isPosition)
    joint.changed=!qFuzzyCompare(anim->getPosition()+QVector3D(1,1,1),currentPart->frameData(0).position()+QVector3D(1,1,1));
  else
    joint.changed=!qFuzzyCompare(anim->getRotation(currentPart)+QVector3D(1,1,1),currentPart->frameData(0).rotation()+QVector3D(1,1,1));
  m_selectedJoint->showJoint(joint);
}

QString PinglyMotionWindow::jointBlockedReason() const
{
  if(playstate!=PLAYSTATE_STOPPED) return QCoreApplication::translate("MenuSpec",MenuSpec::setKeyStopPlaying);
  if(protect) return QCoreApplication::translate("MenuSpec",MenuSpec::setKeyLocked);
  return QCoreApplication::translate("MenuSpec",MenuSpec::setKeyPickJoint);
}

// Reset joint: the picked joint as it is in the first frame, at this frame
void PinglyMotionWindow::resetJointClicked()
{
  Animation* anim=scene->getAnimation();
  if(m_empty || !anim || !currentPart || protect) return;
  if(currentPart->type==BVH_POS)
  {
    const QVector3D first=currentPart->frameData(0).position();
    anim->setPosition(first.x(),first.y(),first.z());
  }
  else
  {
    const QVector3D first=currentPart->frameData(0).rotation();
    anim->setRotation(currentPart,first.x(),first.y(),first.z());
  }
  updateInputs();
  ui->animationView->update();
}

// "Pick the right shoulder for me" on the Pick a joint card
void PinglyMotionWindow::pickDemoJoint()
{
  Animation* anim=scene->getAnimation();
  if(m_empty || !anim) return;
  const QString wanted=QLatin1String(SelectedJointSpec::pickDemoJoint);
  for(int track=1;track<400;track++)
  {
    const QString name=anim->getPartName(track);
    if(name.isEmpty()) break;
    if(name==wanted)
    {
      // the same way as a click on the figure: the view tells the window (partClicked)
      ui->animationView->selectPart(anim->getNode(track));
      return;
    }
  }
}

// fill the joint table and "Attach To" from the skeleton
// track numbers: 0 = position, then every skeleton node, "Site" = End Site
void PinglyMotionWindow::buildPartLists()
{
  const QStringList names=ui->animationView->partNames();

  ui->attachToComboBox->clear();
  ui->attachToComboBox->addItem(tr("(not attached)"));
  // attachment point number equals the track number
  for(int track=1;track<names.count();track++)
    ui->attachToComboBox->addItem(names[track]=="Site" ? "-" : names[track]);  // tr-ok: "Site" is the BVH end-site name, not a text

  fillJointTable();
}

// joint groups were shown or hidden: redraw the view, timeline and part list
void PinglyMotionWindow::jointGroupsChanged()
{
  ui->animationView->jointGroupsChanged();
  ui->timelineView->layoutChanged();

  const int attachIndex=ui->attachToComboBox->currentIndex();
  QSignalBlocker blockAttach(ui->attachToComboBox);
  buildPartLists();
  ui->attachToComboBox->setCurrentIndex(attachIndex);
}

// a joint was chosen in the joint table (or the file was opened)
void PinglyMotionWindow::partChoice()
{
  const int nodeNumber=m_selectedTrack;
  // selectPart will fire partClicked signal, so we don't bother
  // about updating controls or currentPart pointer ourselves here
  ui->animationView->selectPart(nodeNumber);
  ui->timelineView->selectTrack(nodeNumber);

  // the focus stays in the joint table, so the arrow keys walk the list
  enableProps(false);
}

// gets called whenever a body part rotation slider is moved
void PinglyMotionWindow::rotationSlider(const QObject* slider)
{
  Animation* anim=scene->getAnimation();
  QVector3D rotation = anim->getRotation(currentPart);

  double x = rotation.x();
  double y = rotation.y();
  double z = rotation.z();

  if(slider==ui->xRotationSlider)
  {
    x=getX();
    setX(x);
  }
  else if(slider==ui->yRotationSlider)
  {
    y=getY();
    setY(y);
  }
  else if(slider==ui->zRotationSlider)
  {
    z=getZ();
    setZ(z);
  }

  if(ui->animationView->getSelectedPart())
  {
    anim->setRotation(ui->animationView->getSelectedPart(),x,y,z);
    ui->animationView->update();
  }

  updateKeyBtn();
}

// gets called whenever a body part rotation value field gets changed
void PinglyMotionWindow::rotationValue()
{
  double x=ui->xRotationEdit->text().toDouble();
  double y=ui->yRotationEdit->text().toDouble();
  double z=ui->zRotationEdit->text().toDouble();

  double min_x = SliderScale::toValue(ui->xRotationSlider->minimum());
  double min_y = SliderScale::toValue(ui->yRotationSlider->minimum());
  double min_z = SliderScale::toValue(ui->zRotationSlider->minimum());

  double max_x = SliderScale::toValue(ui->xRotationSlider->maximum());
  double max_y = SliderScale::toValue(ui->yRotationSlider->maximum());
  double max_z = SliderScale::toValue(ui->zRotationSlider->maximum());

  if(x<min_x) x=min_x;
  if(y<min_y) y=min_y;
  if(z<min_z) z=min_z;

  if(x>max_x) x=max_x;
  if(y>max_y) y=max_y;
  if(z>max_z) z=max_z;

  setX(x);
  setY(y);
  setZ(z);

  Animation* anim=scene->getAnimation();
  if(ui->animationView->getSelectedPart())
  {
    anim->setRotation(ui->animationView->getSelectedPart(), x, y, z);
    ui->animationView->update();
  }

  updateKeyBtn();
}

void PinglyMotionWindow::positionSlider(const QObject* slider)
{
  Animation* anim=scene->getAnimation();
  QVector3D position = anim->getPosition();

  double x = position.x();
  double y = position.y();
  double z = position.z();

  if(slider==ui->xPositionSlider)
  {
    x=getXPos();
    setXPos(x);
  }
  else if(slider==ui->yPositionSlider)
  {
    y=getYPos();
    setYPos(y);
  }
  else if(slider==ui->zPositionSlider)
  {
    z=getZPos();
    setZPos(z);
  }

  scene->getAnimation()->setPosition(x,y,z);
  ui->animationView->update();

  updateKeyBtn();
}

void PinglyMotionWindow::positionValue()
{

  double x=ui->xPositionEdit->text().toDouble();
  double y=ui->yPositionEdit->text().toDouble();
  double z=ui->zPositionEdit->text().toDouble();

  double min_x = SliderScale::toValue(ui->xPositionSlider->minimum());
  double min_y = SliderScale::toValue(ui->yPositionSlider->minimum());
  double min_z = SliderScale::toValue(ui->zPositionSlider->minimum());

  double max_x = SliderScale::toValue(ui->xPositionSlider->maximum());
  double max_y = SliderScale::toValue(ui->yPositionSlider->maximum());
  double max_z = SliderScale::toValue(ui->zPositionSlider->maximum());

  if(x<min_x) x=min_x;
  if(y<min_y) y=min_y;
  if(z<min_z) z=min_z;

  if(x>max_x) x=max_x;
  if(y>max_y) y=max_y;
  if(z>max_z) z= max_z;

  setXPos(x);
  setYPos(y);
  setZPos(z);

  scene->getAnimation()->setPosition(x,y,z);
  ui->animationView->update();

  updateKeyBtn();
}

void PinglyMotionWindow::updateInputs()
{
  // deactivate redraw to reduce interface "jitter" during updating
  setUpdatesEnabled(false);

  Animation* anim=scene->getAnimation();
  if (anim)
  {
    if(currentPart != NULL)
    {
      QVector3D rotation = anim->getRotation(currentPart);

      double x = rotation.x();
      double y = rotation.y();
      double z = rotation.z();

      RotationLimits rotLimits=anim->getRotationLimits(currentPart);

      double xMin=rotLimits.minimum.x();
      double yMin=rotLimits.minimum.y();
      double zMin=rotLimits.minimum.z();
      double xMax=rotLimits.maximum.x();
      double yMax=rotLimits.maximum.y();
      double zMax=rotLimits.maximum.z();

      if(currentPart->type==BVH_ROOT)
      {
        ui->xRotationSlider->setRange(-359 * m_precision, 359 * m_precision);
        ui->yRotationSlider->setRange(-359 * m_precision, 359 * m_precision);
        ui->zRotationSlider->setRange(-359 * m_precision, 359 * m_precision);
      }
      else
      {
        ui->xRotationSlider->setRange(SliderScale::toSlider(xMin), SliderScale::toSlider(xMax));
        ui->yRotationSlider->setRange(SliderScale::toSlider(yMin), SliderScale::toSlider(yMax));
        ui->zRotationSlider->setRange(SliderScale::toSlider(zMin), SliderScale::toSlider(zMax));
      }

      setX(x);
      setY(y);
      setZ(z);
    }
    else
    {
      enableAvatarRotation(false);
    }

    enableAvatarPosition(!protect);
    QVector3D position = anim->getPosition();

    setXPos(position.x());
    setYPos(position.y());
    setZPos(position.z());

    // we do that in nextPlaystate() now

    ui->framesSpin->setValue(anim->getNumberOfFrames());
    ui->currentFrameSlider->setMaximum(anim->getNumberOfFrames()-1);
    // Frame back / Frame forward: not past the first or the last frame
    if(m_transport) m_transport->setFrame(anim->getFrame(),anim->getNumberOfFrames());
    ui->fpsSpin->setValue(anim->fps());

    // prevent feedback loop
    ui->scaleSpin->blockSignals(true);
    ui->scaleSpin->setValue(anim->getAvatarScale()*100.0+0.1);  // +0.1 to overcome rounding errors
    ui->scaleSpin->blockSignals(false);

    updateKeyBtn();
  }

  if(playstate==PLAYSTATE_STOPPED)
    emit enableInputs(true);
  else
    emit enableInputs(false);

  if(ui->propNameCombo->count())
    enableProps(true);
  else
    enableProps(false);

  // reactivate redraw
  setUpdatesEnabled(true);
}

void PinglyMotionWindow::updateKeyBtn()
{
  Animation* anim=scene->getAnimation();

  int frame=anim->getFrame();
  int partIndex=ui->animationView->getSelectedPartIndex();
  bool hasKeyframe=anim->isKeyFrame(partIndex,frame);

  // the easing tiles show the choice of the pose change that ends at this key
  EasingTiles* tiles=ui->timelineView->easingTiles();
  int startFrame=0;
  if(hasKeyframe && currentPart != NULL && EasingChoice::segmentEndingAt(currentPart,frame,startFrame))
    tiles->setChoice(EasingChoice::choiceOf(currentPart,startFrame,frame));
  else
    tiles->setNoKey();

  updateEditActions();
}

void PinglyMotionWindow::enableInputs(bool state)
{
  // protection overrides state for keyframe button (updateEditActions below
  // greys Set key with its reason)
  if(protect) state=false;

  // do not enable rotation if we have no part selected
  if(currentPart == NULL) state=false;
  enableAvatarRotation(state);
  updateEditActions();
}





void PinglyMotionWindow::setAvatarShape(int shape)
{
  Animation* anim=scene->getAnimation();
  if(!anim) return;

  if(shape==0)
    anim->setFigureType(Animation::FIGURE_FEMALE);
  else
    anim->setFigureType(Animation::FIGURE_MALE);
  ui->animationView->update();
}

void PinglyMotionWindow::setAvatarScale(int percent)
{
  scene->getAnimation()->setAvatarScale(percent/100.0);
  ui->animationView->update();
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

// Menu Action: Edit / Paste
void PinglyMotionWindow::toolsOptimizeBVH()
{
  scene->getAnimation()->optimize();
  updateInputs();
}

// Menu Action: Options / Skeleton
void PinglyMotionWindow::setSkeleton(bool on)
{
  UserSettings::setSkeleton(on);
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

// Menu Action: View / Timeline
void PinglyMotionWindow::showTimeline(bool on)
{
  m_timelineDock->setVisible(on);
}

// Menu Action: Options / Settings
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


void PinglyMotionWindow::setX(float x)
{
  setSliderValue(ui->xRotationSlider,ui->xRotationEdit,x);
}

void PinglyMotionWindow::setY(float y)
{
  setSliderValue(ui->yRotationSlider,ui->yRotationEdit,y);
}

void PinglyMotionWindow::setZ(float z)
{
  setSliderValue(ui->zRotationSlider,ui->zRotationEdit,z);
}

float PinglyMotionWindow::getX()
{
  return SliderScale::toValue(ui->xRotationSlider->value());
}

float PinglyMotionWindow::getY()
{
  return SliderScale::toValue(ui->yRotationSlider->value());
}

float PinglyMotionWindow::getZ()
{
  return SliderScale::toValue(ui->zRotationSlider->value());
}

void PinglyMotionWindow::setXPos(float x)
{
  setSliderValue(ui->xPositionSlider,ui->xPositionEdit,x);
}

void PinglyMotionWindow::setYPos(float y)
{
  setSliderValue(ui->yPositionSlider,ui->yPositionEdit,y);
}

void PinglyMotionWindow::setZPos(float z)
{
  setSliderValue(ui->zPositionSlider,ui->zPositionEdit,z);
}

float PinglyMotionWindow::getXPos()
{
  return SliderScale::toValue(ui->xPositionSlider->value());
}

float PinglyMotionWindow::getYPos()
{
  return SliderScale::toValue(ui->yPositionSlider->value());
}

float PinglyMotionWindow::getZPos()
{
  return SliderScale::toValue(ui->zPositionSlider->value());
}










// helper function to prevent feedback between the two widgets
void PinglyMotionWindow::setSliderValue(QSlider* slider,QLineEdit* edit,float value)
{
  slider->blockSignals(true);
  edit->blockSignals(true);
  slider->setValue(SliderScale::toSlider(value));
  edit->setText(QString::number(value));
  edit->blockSignals(false);
  slider->blockSignals(false);
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

  const int answer=box.exec();
  if(answer==QMessageBox::Discard) return true;
  if(answer!=QMessageBox::Save) return false;

  // save every unsaved animation; stop if one is not saved (cancelled or failed)
  for(Animation* animation : unsaved)
  {
    selectAnimation(animation);
    if(!fileSave() || animation->dirty()) return false;
  }
  return true;
}

// empty out the open files list
bool PinglyMotionWindow::clearOpenFiles()
{
  if(!askToSaveChanges()) return false;

  timeline->setAnimation(0);
  scene->clear();
  currentPart = NULL;
  openFiles.clear();
  ui->selectAnimationCombo->clear();
  animationIds.clear();
  setCurrentFile(m_untitledName);
  longestRunningTime=0.0;

  return true;
}

// convenience function to set window title in a defined way
void PinglyMotionWindow::setCurrentFile(const QString& fileName)
{
  // "Pose: changed from ..." on the Hands tab belongs to the file it was picked in
  if(m_handsPanel && fileName!=currentFile) m_handsPanel->forgetPicks();
  currentFile=fileName;
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

  // enable export to second life if current file name is not the default untitled name
  if(!m_empty) ui->fileExportForSecondLifeAction->setEnabled(currentFile != m_untitledName);
}




// options, window place and panel layout, remembered for next time
void PinglyMotionWindow::saveUserSettings()
{
  UserSettings::setSkeleton(ui->optionsSkeletonAction->isChecked());
  UserSettings::setJointLimits(ui->optionsJointLimitsAction->isChecked());
  UserSettings::setProtectFirstFrame(ui->optionsProtectFirstFrameAction->isChecked());

  // panels hidden by Tab are not remembered: the program starts with them shown
  if(m_tabState.isHidden()) togglePanelsHidden();
  UserSettings::setMainWindowGeometry(saveGeometry());
  // the user's own wide layout, also while the panels are rearranged (docs/UX_DECISIONS.md, 6c)
  UserSettings::setPanelLayout(m_reflower.layoutToSave(this));
  UserSettings::setPanelLayoutVersion(PanelLayout::CurrentVersion);

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

// -------------------------------------------------------------------------
// autoconnection from designer UI

// ------- Menu Action Slots --------














void PinglyMotionWindow::on_editCutAction_triggered()
{
  editCut();
}

void PinglyMotionWindow::on_editCopyAction_triggered()
{
  editCopy();
}

void PinglyMotionWindow::on_editPasteAction_triggered()
{
  editPaste();
}

void PinglyMotionWindow::on_toolsOptimizeBVHAction_triggered()
{
  toolsOptimizeBVH();
}

void PinglyMotionWindow::on_toolsMirrorAction_triggered()
{
  Animation* anim=scene->getAnimation();
  anim->mirror(currentPart);
  updateInputs();
}

void PinglyMotionWindow::on_optionsSkeletonAction_toggled(bool on)
{
  setSkeleton(on);
}

void PinglyMotionWindow::on_optionsJointLimitsAction_toggled(bool on)
{
  setJointLimits(on);
  if(m_selectedJoint) m_selectedJoint->setLimitsOn(on);
}

void PinglyMotionWindow::on_optionsLoopAction_toggled(bool on)
{
  setLoop(on);
}

void PinglyMotionWindow::on_optionsProtectFirstFrameAction_toggled(bool on)
{
  setProtectFirstFrame(on);
}

void PinglyMotionWindow::on_optionsShowTimelineAction_toggled(bool on)
{
  showTimeline(on);
}

void PinglyMotionWindow::on_optionsConfigurePinglyMotionAction_triggered()
{
  configure();
}

void PinglyMotionWindow::on_helpAboutAction_triggered()
{
  helpAbout();
}

void PinglyMotionWindow::on_helpWhatsNewAction_triggered()
{
  helpWhatsNew();
}

// Help > Open log: the local error log (issue 239) in the system's text viewer
void PinglyMotionWindow::on_helpOpenLogAction_triggered()
{
  const QString path = AppLog::filePath();
  if (path.isEmpty() || !QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
    statusBar()->showMessage(tr("The log could not be opened."), 5000);
}

// Help > Clear log: empties the local error log
void PinglyMotionWindow::on_helpClearLogAction_triggered()
{
  statusBar()->showMessage(AppLog::clear() ? tr("The log is empty now.")
                                           : tr("The log could not be cleared."), 5000);
}

// ------- Additional Toolbar Element Slots --------

void PinglyMotionWindow::on_resetCameraAction_triggered()
{
  emit resetCamera();
}

// ------- UI Element Slots --------

void PinglyMotionWindow::on_selectAnimationCombo_activated(int which)
{
  animationChanged(which);
}

void PinglyMotionWindow::on_figureCombo_activated(int newShape)
{
  setAvatarShape(newShape);
}

void PinglyMotionWindow::on_scaleSpin_valueChanged(int newValue)
{
  setAvatarScale(newValue);
}

void PinglyMotionWindow::on_xRotationEdit_editingFinished()
{
  rotationValue();
}

void PinglyMotionWindow::on_xRotationSlider_valueChanged(int)
{
  rotationSlider(sender());
}

void PinglyMotionWindow::on_yRotationEdit_editingFinished()
{
  rotationValue();
}

void PinglyMotionWindow::on_yRotationSlider_valueChanged(int)
{
  rotationSlider(sender());
}

void PinglyMotionWindow::on_zRotationEdit_editingFinished()
{
  rotationValue();
}

void PinglyMotionWindow::on_zRotationSlider_valueChanged(int)
{
  rotationSlider(sender());
}

void PinglyMotionWindow::on_xPositionEdit_editingFinished()
{
  positionValue();
}

void PinglyMotionWindow::on_xPositionSlider_valueChanged(int)
{
  positionSlider(sender());
}

void PinglyMotionWindow::on_yPositionEdit_editingFinished()
{
  positionValue();
}

void PinglyMotionWindow::on_yPositionSlider_valueChanged(int)
{
  positionSlider(sender());
}

void PinglyMotionWindow::on_zPositionEdit_editingFinished()
{
  positionValue();
}

void PinglyMotionWindow::on_zPositionSlider_valueChanged(int)
{
  positionSlider(sender());
}
























// end autoconnection from designer UI
// -------------------------------------------------------------------------
