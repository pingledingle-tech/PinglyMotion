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

#include <QActionGroup>
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
#include <QDir>
#include <QFileDialog>
#include "startfolder.h"
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
#include "propfile.h"
#include "constants.h"
#include "controlapi.h"
#include "controlserver.h"
#include "controlindicator.h"
#include "auroralink.h"
#include <QStatusBar>
#include "prop.h"
#include "jointgroups.h"
#include "jointcard.h"
#include "jointinfo.h"
#include "jointturn.h"
#include "jointlist.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "paneltitlebar.h"
#include "pinglymotion.h"
#include "scene.h"
#include "timeline.h"
#include "timelineview.h"
#include "updatersettings.h"
#include "usersettings.h"
#include "usersettingsdialog.h"

#include "panelvisibility.h"
#include "pinglymotionwindow.h"
#include "theme.h"
#include "ui_pinglymotionwindow.h"
#include "whatsnewdialog.h"
#include "wheelguard.h"

// What the remote control commands do in the window: the same steps as clicking.
class WindowControlHost : public ControlHost
{
  public:
    explicit WindowControlHost(PinglyMotionWindow* window) : w(window) {}

    Animation* animation() override { return w->scene->getAnimation(); }

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
      discardChanges();
      w->fileOpen(path);
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
      // the play button steps through stopped, looping and playing
      for (int step = 0; step < 3 && (w->playstate == PLAYSTATE_STOPPED) == on; step++)
        w->nextPlaystate();
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

    QVector3D camera() override
    {
      const Camera* c = w->ui->animationView->camera();
      return QVector3D(c->xRotation(), c->yRotation(), c->distance());
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
  m_animFilter("Animation Files (*.dingle *.avm *.bvh)"),
  m_dingleFilter(QObject::tr("PinglyMotion animation (*.dingle)")),
  m_propFilter("Props (*.prp)"),
  m_untitledName(DingleSave::defaultName()),
  m_precision(100),
  m_controlAction(nullptr),
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
  frameDataValid=false;
  currentPart=0;
  longestRunningTime=0.0;
  scene = ui->animationView->initializeScene(this);
  m_pinglyMotion->setScene(scene);
  ui->animationView->setMeshModels(cubeMesh, sphereMesh, coneMesh);

  // fill the joint lists from the Bento skeleton
  buildPartLists();

  // prepare play button icons
  stopIcon=QIcon(":/icons/icons/stop.png");
  playIcon=QIcon(":/icons/icons/play.png");
  loopIcon=QIcon(":/icons/icons/loop.png");

  // playback stopped by default
  setPlaystate(PLAYSTATE_STOPPED);

  UserSettings::readSettings();
  Theme::apply(UserSettings::darkTheme());
  fitPanelWidths();
  // the heading colour in the joint list comes from the theme
  fillJointTable();

  // the window's place, size, maximised state and screen from last time
  // (docs/PANELS_PLAN.md, job P3). Qt puts a window back onto a screen when
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
  ui->optionsSkeletonAction->setChecked(UserSettings::skeleton());
  ui->optionsJointLimitsAction->setChecked(UserSettings::jointLimits());

  // Options > Dark theme
  QAction* darkThemeAction=new QAction(tr("Dark theme"),this);
  darkThemeAction->setCheckable(true);
  darkThemeAction->setChecked(UserSettings::darkTheme());
  ui->menuOptions->addAction(darkThemeAction);
  connect(darkThemeAction,&QAction::toggled,this,[this](bool dark)
  {
    UserSettings::setDarkTheme(dark);
    Theme::apply(dark);
    fitPanelWidths();
    fillJointTable();
    ui->animationView->update();
  });
  ui->optionsProtectFirstFrameAction->setChecked(UserSettings::protectFirstFrame());

  // Options > Body: which avatar the 3D view shows (only what you see, not the animation)
  QMenu* bodyMenu=new QMenu(tr("Body"),this);
  bodyMenu->setToolTipsVisible(true);
  ui->menuOptions->addMenu(bodyMenu);
  QActionGroup* bodyGroup=new QActionGroup(this);
  struct BodyChoice { const char* key; AvatarSkin::Body body; QString text; QString tip; };
  const BodyChoice bodyChoices[]={
    {"ruth2",AvatarSkin::RuthBody,tr("Ruth2"),
     tr("The open source Ruth2 avatar, with fingers and a face that follow the Bento bones.")},
    {"max",AvatarSkin::MaxBody,tr("Max"),
     tr("The open source Max avatar, with fingers and a face that follow the Bento bones.")}};
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
  hideInListsAction->setToolTip(tr("Leave hidden joints out of the timeline and the part list. Off: they are listed dimmed."));
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

  // Options > Light...: brightness, shine and warmth of the body light
  ui->animationView->setBodyLight(UserSettings::lightBrightness(),UserSettings::lightShine(),
                                  UserSettings::lightWarmth(),UserSettings::floorShadow());
  QAction* lightAction=new QAction(tr("Light..."),this);
  lightAction->setToolTip(tr("Change how the body is lit: brightness, shine, warmth and the shadow on the floor."));
  ui->menuOptions->insertAction(bodyMenu->menuAction(),lightAction);
  connect(lightAction,&QAction::triggered,this,&PinglyMotionWindow::showLightDialog);

  // panel layout from last time (size, place, floating panels). Only a layout
  // saved by this version of the panels is loaded (docs/PANELS_PLAN.md, 7);
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
  // the timeline stands visible at every start; hiding it lasts for this session only
  PanelVisibility::showAtStart(m_timelineDock,ui->optionsShowTimelineAction);


  connect(m_pinglyMotion.data(), &PinglyMotion::repaint, ui->animationView, static_cast<void (AnimationView::*)()>(&AnimationView::repaint));
  connect(scene, SIGNAL(repaint()), ui->animationView, SLOT(update()));
  connect(scene, SIGNAL(animationSelected(Animation*)), this, SLOT(selectAnimation(Animation*)));
  connect(this, SIGNAL(protectFrame(bool)), scene, SLOT(protectFrame(bool)));

  connect(ui->animationView, SIGNAL(storeCameraPosition(int)), scene, SLOT(storeCameraPosition(int)));
  connect(ui->animationView, SIGNAL(restoreCameraPosition(int)), scene, SLOT(restoreCameraPosition(int)));

  connect(ui->animationView,SIGNAL(partClicked(BVHNode*,
                                           QVector3D,
                                           RotationLimits,
                                           QVector3D)),
                     this,SLOT(partClicked(BVHNode*,
                                           QVector3D,
                                           RotationLimits,
                                           QVector3D)));

  connect(ui->animationView,SIGNAL(partDragged(BVHNode*,double,double,double)),
                     this,SLOT(partDragged(BVHNode*,double,double,double)));

  connect(ui->animationView,SIGNAL(propClicked(Prop*)),this,SLOT(propClicked(Prop*)));

  connect(ui->animationView,SIGNAL(propDragged(Prop*,double,double,double)),
                     this,SLOT(propDragged(Prop*,double,double,double)));

  connect(ui->animationView,SIGNAL(propScaled(Prop*,double,double,double)),
                     this,SLOT(propScaled(Prop*,double,double,double)));

  connect(ui->animationView,SIGNAL(propRotated(Prop*,double,double,double)),
                     this,SLOT(propRotated(Prop*,double,double,double)));

  connect(ui->animationView,SIGNAL(backgroundClicked()),this,SLOT(backgroundClicked()));
  connect(ui->animationView,SIGNAL(animationSelected(Animation*)),this,SLOT(selectAnimation(Animation*)));

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

  connect(this,SIGNAL(enableEaseInOut(bool)),ui->easeInOutGroup,SLOT(setEnabled(bool)));

  connect(&timer,SIGNAL(timeout()),this,SLOT(frameTimeout()));

  connect(this,SIGNAL(resetCamera()),ui->animationView,SLOT(resetCamera()));

  connect(ui->animationView,SIGNAL(partClicked(int)),ui->timelineView,SLOT(selectTrack(int)));
  connect(ui->animationView,SIGNAL(backgroundClicked()),ui->timelineView,SLOT(backgroundClicked()));

  timeline=ui->timelineView->getTimeline();
  connect(timeline,SIGNAL(positionCenter(int)),ui->timelineView,SLOT(scrollTo(int)));
  connect(timeline,SIGNAL(trackClicked(int)),ui->animationView,SLOT(selectPart(int)));

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

  // Options > Allow remote control: lets other programs and AI tools pose and play the animation
  m_controlHost.reset(new WindowControlHost(this));
  m_controlApi.reset(new ControlApi(m_controlHost.data()));
  m_controlServer.reset(new ControlServer(m_controlApi.data()));
  m_controlAction=new QAction(tr("Allow remote control (AI and other programs)"),this);
  m_controlAction->setCheckable(true);
  m_controlAction->setToolTip(tr("Lets programs on this computer pose and play the animation. Off every time PinglyMotion starts, unless started with --control."));
  ui->menuOptions->addAction(m_controlAction);
  connect(m_controlAction,&QAction::toggled,this,[this](bool on){ setRemoteControl(on,true); });

  // status bar (#88): "Tips on" switch, remote control and Aurora link, left to right;
  // messages still show on the left
  statusBar()->setStyleSheet(QStringLiteral("QStatusBar::item { border: none; }"));
  m_tipsSwitch=new TipsSwitch(this);
  m_tipsSwitch->setOn(UserSettings::tipsOn());
  statusBar()->addPermanentWidget(m_tipsSwitch);
  // only stored for now; the help tips themselves come in a later step of #88
  connect(m_tipsSwitch,&TipsSwitch::toggled,this,[](bool on){ UserSettings::setTipsOn(on); });

  // is remote control on, who is connected, what it did last (issue #43)
  m_controlIndicator=new ControlIndicator(this);
  statusBar()->addPermanentWidget(m_controlIndicator);
  connect(m_controlIndicator,&ControlIndicator::toggleRequested,m_controlAction,&QAction::toggle);
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

  // command line: pinglymotion [--control] [animation file]
  QStringList args = QApplication::arguments();
  args.removeFirst();
  const bool startWithControl=args.removeAll("--control")>0;
  QString fileFromCommandLine;
  for(const QString& arg : args)
    if(!arg.startsWith("--") && fileFromCommandLine.isEmpty()) fileFromCommandLine=arg;
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
    // started with --control: no information box, nobody is waiting to click it away
    setRemoteControl(true,false);
    QSignalBlocker blocker(m_controlAction);
    m_controlAction->setChecked(m_controlServer->running());
  }
}

void PinglyMotionWindow::setRemoteControl(bool on,bool showInfo)
{
  if(on)
  {
    QString error;
    if(!m_controlServer->start(ControlServer::DEFAULT_PORT,&error) || !m_controlServer->writeConnectionFile(&error))
    {
      m_controlServer->stop();
      QSignalBlocker blocker(m_controlAction);
      m_controlAction->setChecked(false);
      QMessageBox::warning(this,tr("Remote control"),tr("Remote control could not be started:\n%1").arg(error));
      updateWindowTitle();
      return;
    }
    updateWindowTitle();
    if(showInfo)
    {
      QMessageBox box(QMessageBox::Information,tr("Remote control is on"),
        tr("<p>Programs on this computer can now pose and play the animation.</p>"
           "<p>Address: %1<br>The secret key is in:<br>%2</p>"
           "<p>Only programs running as you can read that file. Turn this off in the Options menu when you do not need it.</p>"
           "<p><a href=\"%3\">How to connect an AI assistant</a></p>")
          .arg(m_controlServer->url().toHtmlEscaped(),
               QDir::toNativeSeparators(ControlServer::connectionFilePath()).toHtmlEscaped(),
               QStringLiteral("https://github.com/pingledingle-tech/PinglyMotion/blob/main/docs/CONTROL.md#set-up-an-ai-assistant")),
        QMessageBox::Ok,this);
      box.setTextFormat(Qt::RichText);
      box.setTextInteractionFlags(Qt::TextBrowserInteraction);
      box.exec();
    }
  }
  else
  {
    m_controlServer->removeConnectionFile();
    m_controlServer->stop();
    updateWindowTitle();
  }
}

// the title says so while other programs can control PinglyMotion
void PinglyMotionWindow::updateWindowTitle()
{
  QString title=Constants::windowTitle(currentFile);
  if(m_controlServer && m_controlServer->running()) title+=tr(" - remote control on");
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
  QTimer::singleShot(0, this, SLOT(afterShow()));
}

void PinglyMotionWindow::afterShow()
{
  updateChecker.onStartup();
}

// slot gets called by AnimationView::mousePressEvent()
void PinglyMotionWindow::partClicked(BVHNode* node, QVector3D rotation, RotationLimits limits, QVector3D position)
{
  ui->avatarPropsTab->setCurrentIndex(0);
  enableProps(false);

  if(!node)
  {
    qDebug("pinglymotion::partClicked(node,...): node==0!");
    return;
  }

  if(node)
  {
    qDebug("pinglymotion::partClicked(node): %s",node->name().toLatin1().constData());
    currentPart=node;

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
      ui->xRotationSlider->setRange((int)(limits.minimum.x() * m_precision), (int)(limits.maximum.x() * m_precision));
      ui->yRotationSlider->setRange((int)(limits.minimum.y() * m_precision), (int)(limits.maximum.y() * m_precision));
      ui->zRotationSlider->setRange((int)(limits.minimum.z() * m_precision), (int)(limits.maximum.z() * m_precision));
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

    updateJointCard();

    // show the user if this part has a key frame here
    updateKeyBtn();
  }
}

// slot gets called by AnimationView::mouseMoveEvent()
void PinglyMotionWindow::partDragged(BVHNode* node,double x,double y,double z)
{
  if(node)
  {
    // check if this frame is protected, or the joint locked on the card
    if(!protect && !isLocked(node))
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
      updateJointCardTrackball();

      updateKeyBtn();
    }
  }
  else qDebug("pinglymotion::partDragged(): node==0!");
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
  updateJointCard();
  emit enableEaseInOut(false);
  selectTrackInTable(0);
  updateKeyBtn();
}

// The 3D view fills the window. Three panels (dock widgets) stand around it
// (docs/PANELS_PLAN.md, job P2): Joints on the left, Selected joint on the
// right, Timeline at the bottom. Each can be resized, moved to another side,
// put as a tab on another panel, or torn off into its own window. A panel
// only goes where its shape fits: the timeline is wide (top or bottom), the
// other two are tall (left or right).
void PinglyMotionWindow::buildDocks()
{
  setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks);

  // --- timeline panel: tracks, frame slider, play and key frame buttons,
  // loop in / out, frame count and FPS
  QWidget* timelinePanel=new QWidget;
  QVBoxLayout* timelineLayout=new QVBoxLayout(timelinePanel);
  timelineLayout->setContentsMargins(4,4,4,4);
  // the old layout kept the timeline at a fixed height; in a panel it grows
  ui->timelineView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  ui->timelineView->setMinimumSize(200,100);
  timelineLayout->addWidget(ui->timelineView,1);
  QHBoxLayout* playLayout=new QHBoxLayout;
  playLayout->addWidget(ui->currentFrameSlider,1);
  playLayout->addWidget(ui->playButton);
  playLayout->addWidget(ui->keyframeButton);
  timelineLayout->addLayout(playLayout);

  QHBoxLayout* rangeLayout=new QHBoxLayout;
  rangeLayout->addWidget(ui->loopInLabel);
  rangeLayout->addWidget(ui->loopInSpinBox);
  rangeLayout->addWidget(ui->loopInPercentLabel);
  rangeLayout->addSpacing(12);
  rangeLayout->addWidget(ui->loopOutLabel);
  rangeLayout->addWidget(ui->loopOutSpinBox);
  rangeLayout->addWidget(ui->loopOutPercentLabel);
  rangeLayout->addStretch(1);
  rangeLayout->addWidget(ui->frameLabel);
  rangeLayout->addWidget(ui->currentFrameLabel);
  rangeLayout->addWidget(ui->ofLabel);
  rangeLayout->addWidget(ui->framesSpin);
  rangeLayout->addSpacing(12);
  rangeLayout->addWidget(ui->fpsLabel);
  rangeLayout->addWidget(ui->fpsSpin);
  timelineLayout->addLayout(rangeLayout);

  m_timelineDock=new QDockWidget(tr("Timeline"),this);
  m_timelineDock->setObjectName(PanelLayout::TimelineDock);
  m_timelineDock->setWidget(timelinePanel);
  m_timelineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  m_timelineDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
  m_timelineDock->setTitleBarWidget(new PanelTitleBar(m_timelineDock));

  // A tall panel keeps its width, whatever the window around it does. If the
  // window is too low for all of it, it scrolls. A wheel over a slider or spin
  // box scrolls the panel instead of changing the value (issue #82: the figure
  // moved while the panel was scrolled).
  auto makeTallPanel=[this](const QString& title,const char* objectName,QWidget* panel,QScrollArea*& scroll)
  {
    QDockWidget* dock=new QDockWidget(title,this);
    dock->setObjectName(QString::fromLatin1(objectName));
    scroll=new QScrollArea;
    scroll->setWidget(panel);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    dock->setWidget(scroll);
    (new WheelGuard(scroll))->guardChildrenOf(panel);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setTitleBarWidget(new PanelTitleBar(dock));
    return dock;
  };

  // --- Joints panel: the Avatar / Props switch (the tabs), with the joint
  // tabs, search and list on Avatar, the prop list on Props
  QWidget* jointsPanel=new QWidget;
  QVBoxLayout* jointsLayout=new QVBoxLayout(jointsPanel);
  jointsLayout->setContentsMargins(4,4,4,4);
  jointsLayout->addWidget(ui->avatarPropsTab,1);
  m_jointsDock=makeTallPanel(tr("Joints"),PanelLayout::JointsDock,jointsPanel,m_jointsScroll);

  // --- Selected joint panel: the joint card on the Avatar side (#88,
  // step 4), the position, scale and rotation boxes of the prop on the Props
  // side. The card takes the old slider and value field widgets from the
  // designer file, so their connections stay; the empty boxes are hidden.
  // The page follows the Avatar / Props switch.
  m_jointCard=new JointCard(m_precision,
    {JointCard::AxisRow{ui->xRotationSlider,ui->xRotationEdit},
     JointCard::AxisRow{ui->yRotationSlider,ui->yRotationEdit},
     JointCard::AxisRow{ui->zRotationSlider,ui->zRotationEdit}},
    {JointCard::AxisRow{ui->xPositionSlider,ui->xPositionEdit},
     JointCard::AxisRow{ui->yPositionSlider,ui->yPositionEdit},
     JointCard::AxisRow{ui->zPositionSlider,ui->zPositionEdit}});
  ui->rotationGroupBox->hide();
  ui->positionGroupBox->hide();
  connect(m_jointCard,&JointCard::ikToggled,this,&PinglyMotionWindow::setSelectedJointIk);
  connect(m_jointCard,&JointCard::lockToggled,this,&PinglyMotionWindow::setSelectedJointLocked);
  connect(m_jointCard,&JointCard::resetClicked,this,&PinglyMotionWindow::resetSelectedJoint);
  connect(m_jointCard,&JointCard::trackballTurned,this,
          [this](double yaw,double pitch,JointTurn::Frame frame){ turnSelectedJoint(yaw,pitch,frame); });
  ui->animationView->installEventFilter(this);
  QWidget* jointCard=m_jointCard;
  QWidget* propCard=new QWidget;
  QVBoxLayout* propCardLayout=new QVBoxLayout(propCard);
  propCardLayout->setContentsMargins(0,0,0,0);
  propCardLayout->addWidget(ui->propPositionGroup);
  propCardLayout->addWidget(ui->propScaleGroup);
  propCardLayout->addWidget(ui->propRotationGroup);
  propCardLayout->addStretch(1);
  QStackedWidget* cardStack=new QStackedWidget;
  cardStack->setObjectName("jointCardStack");
  cardStack->addWidget(jointCard);
  cardStack->addWidget(propCard);
  cardStack->setCurrentIndex(ui->avatarPropsTab->currentIndex());
  connect(ui->avatarPropsTab,&QTabWidget::currentChanged,cardStack,&QStackedWidget::setCurrentIndex);
  QWidget* cardPanel=new QWidget;
  QVBoxLayout* cardPanelLayout=new QVBoxLayout(cardPanel);
  cardPanelLayout->setContentsMargins(4,4,4,4);
  cardPanelLayout->addWidget(cardStack,1);
  m_jointCardDock=makeTallPanel(tr("Selected joint"),PanelLayout::JointCardDock,cardPanel,m_jointCardScroll);

  // --- 3D view in the middle (replaces the old fixed layout)
  QWidget* central=new QWidget;
  QVBoxLayout* centralLayout=new QVBoxLayout(central);
  centralLayout->setContentsMargins(0,0,0,0);
  centralLayout->addWidget(ui->animationView);
  setCentralWidget(central);

  // the default layout is built in code (src/panelwindow.h), the same one
  // View > Reset layout brings back
  PanelWindow::applyDefaultLayout(this,m_jointsDock,m_jointCardDock,m_timelineDock);

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

QList<QDockWidget*> PinglyMotionWindow::panelDocks() const
{
  return {m_jointsDock,m_jointCardDock,m_timelineDock};
}

// View > Reset layout: the default layout built in code, all three panels
// docked and shown. Panels hidden by Tab count as shown again.
void PinglyMotionWindow::resetPanelLayout()
{
  m_applyingTab=true;
  m_tabState.reset();
  PanelWindow::applyDefaultLayout(this,m_jointsDock,m_jointCardDock,m_timelineDock);
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

// Options > Light...: three sliders that change the body light at once.
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
void PinglyMotionWindow::fitPanelWidths()
{
  for(QScrollArea* scroll : {m_jointsScroll,m_jointCardScroll})
  {
    QWidget* panel=scroll->widget();
    panel->ensurePolished();
    const int width=qMax(panel->sizeHint().width(),panel->minimumSizeHint().width());
    scroll->setFixedWidth(width+scroll->verticalScrollBar()->sizeHint().width());
  }
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

  QVBoxLayout* layout=new QVBoxLayout(ui->jointPicker);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(2);
  layout->addWidget(m_jointTabs);
  layout->addWidget(m_jointSearch);
  layout->addWidget(m_jointTable);

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
                                                     m_jointSearch->text(),hidden,UserSettings::hideJointsInLists());

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
        tip+=(tip.isEmpty() ? QString() : QStringLiteral("\n"))+tr("Name in the animation file: %1").arg(r.fileName);
      if(r.fileName==JointList::POSITION_TRACK)
        tip=tr("Moves the whole avatar.");
      item->setToolTip(tip);
      if(r.dimmed)
        item->setForeground(palette().color(QPalette::Disabled,QPalette::Text));
    }
    m_jointTable->setItem(row,0,item);
    nodeMapping << r.track;
  }
  showSelectedTrack();
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
    if(!listed)
    {
      fillJointTable();
      return;
    }
  }
  showSelectedTrack();
}

// fill the joint table and "Attach To" from the skeleton
// track numbers: 0 = position, then every skeleton node, "Site" = End Site
void PinglyMotionWindow::buildPartLists()
{
  const QStringList names=ui->animationView->partNames();

  ui->attachToComboBox->clear();
  ui->attachToComboBox->addItem(tr("* not attached *"));
  // attachment point number equals the track number
  for(int track=1;track<names.count();track++)
    ui->attachToComboBox->addItem(names[track]=="Site" ? "-" : names[track]);

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
  updateJointCardTrackball();

  updateKeyBtn();
}

// gets called whenever a body part rotation value field gets changed
void PinglyMotionWindow::rotationValue()
{
  double x=ui->xRotationEdit->text().toDouble();
  double y=ui->yRotationEdit->text().toDouble();
  double z=ui->zRotationEdit->text().toDouble();

  double min_x = ui->xRotationSlider->minimum() / m_precision;
  double min_y = ui->yRotationSlider->minimum() / m_precision;
  double min_z = ui->zRotationSlider->minimum() / m_precision;

  double max_x = ui->xRotationSlider->maximum() / m_precision;
  double max_y = ui->yRotationSlider->maximum() / m_precision;
  double max_z = ui->zRotationSlider->maximum() / m_precision;

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
  updateJointCardTrackball();

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
  qDebug("pinglymotion::positionValue()");

  double x=ui->xPositionEdit->text().toDouble();
  double y=ui->yPositionEdit->text().toDouble();
  double z=ui->zPositionEdit->text().toDouble();

  double min_x = ui->xPositionSlider->minimum() / m_precision;
  double min_y = ui->yPositionSlider->minimum() / m_precision;
  double min_z = ui->zPositionSlider->minimum() / m_precision;

  double max_x = ui->xPositionSlider->maximum() / m_precision;
  double max_y = ui->yPositionSlider->maximum() / m_precision;
  double max_z = ui->zPositionSlider->maximum() / m_precision;

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
        ui->xRotationSlider->setRange((int)(xMin * m_precision), (int)(xMax * m_precision));
        ui->yRotationSlider->setRange((int)(yMin * m_precision), (int)(yMax * m_precision));
        ui->zRotationSlider->setRange((int)(zMin * m_precision), (int)(zMax * m_precision));
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
    //  playButton->setIcon(playing ? stopIcon : playIcon);

    updateJointCard();

    ui->framesSpin->setValue(anim->getNumberOfFrames());
    ui->currentFrameSlider->setMaximum(anim->getNumberOfFrames()-1);
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

  if (frameDataValid)
    ui->editPasteAction->setEnabled(true);
  else
    ui->editPasteAction->setEnabled(false);

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
  qDebug("pinglymotion::updateKeyBtn(): anim=%p",(const void*) anim);

  // make sure we don't send a toggle signal on display update
  ui->keyframeButton->blockSignals(true);

  int frame=anim->getFrame();
  int partIndex=ui->animationView->getSelectedPartIndex();
  bool hasKeyframe=anim->isKeyFrame(partIndex,frame);
  ui->keyframeButton->setDown(hasKeyframe);

  // re-enable toggle signal
  ui->keyframeButton->blockSignals(false);

  if(hasKeyframe && currentPart != NULL)
  {
    emit enableEaseInOut(true);
    ui->easeInCheck->setChecked(anim->easeIn(currentPart,anim->getFrame()));
    ui->easeOutCheck->setChecked(anim->easeOut(currentPart,anim->getFrame()));
  }
  else
    emit enableEaseInOut(false);

//  timeline->repaint();
}

void PinglyMotionWindow::enableInputs(bool state)
{
  // protection overrides state for keyframe button
  if(protect) state=false;
  ui->keyframeButton->setEnabled(state);

  // do not enable rotation if we have no part selected
  if(currentPart == NULL) state=false;
  enableAvatarRotation(state);
}

void PinglyMotionWindow::frameTimeout()
{
  // only if we are still playing
  if(playstate!=PLAYSTATE_STOPPED)
  {
    Animation* anim=scene->getAnimation();
    if(anim)
    {
      // don't show protected frames color on playback to avoid flicker
      emit protectFrame(false);
      // cycle through frames, restart at looping point
      scene->stepForward();

      if(anim->getFrame()==(anim->getNumberOfFrames()-1) && playstate==PLAYSTATE_PLAYING)
      {
        timer.stop();
        setPlaystate(PLAYSTATE_STOPPED);
      }

      updateInputs();
      return;
    }
  }
}

void PinglyMotionWindow::nextPlaystate()
{
//  qDebug("pinglymotion::nextPlaystate(): current playstate %d",(int) playstate);

  Animation* anim=scene->getAnimation();

  switch(playstate)
  {
    case PLAYSTATE_STOPPED:
    {
      // if we're suposed to loop and the current frame is not past loop out point
      if(UserSettings::loop() && anim->getFrame()<anim->getLoopOutPoint())
      {
        // start looping animation
        setPlaystate(PLAYSTATE_LOOPING);
        anim->setPlaystate(PLAYSTATE_LOOPING);
      }
      else
      {
        // start one-shot animation
        setPlaystate(PLAYSTATE_PLAYING);
        anim->setPlaystate(PLAYSTATE_PLAYING);
      }

      timer.start((int)(1.0/anim->fps()*1000.0));
      break;
    }
    case PLAYSTATE_LOOPING:
    {
      setPlaystate(PLAYSTATE_PLAYING);
      anim->setPlaystate(PLAYSTATE_PLAYING);
      break;
    }
    case PLAYSTATE_PLAYING:
    {
      // take care of locks, key frames ...
      frameSlider(ui->currentFrameSlider->value());
      setPlaystate(PLAYSTATE_STOPPED);
      anim->setPlaystate(PLAYSTATE_STOPPED);

      break;
    }
    default:
      qDebug("pinglymotion::nextPlaystate(): unknown playstate %d",(int) playstate);
  }

  updateInputs();
}

void PinglyMotionWindow::setFPS(int fps)
{
  qDebug("pinglymotion::setFPS(%d)",fps);

  Animation* anim=scene->getAnimation();
  if(!anim) return;

  // sanity check
  if(fps<1) fps=1;
  else if(fps>50) fps=50;

  anim->setFPS(fps);
  calculateLongestRunningTime();
}

void PinglyMotionWindow::frameSlider(int position)
{
  // check if we are at the first frame and if it's protected
  if(position==0 && UserSettings::protectFirstFrame()) protect=true;
  else protect=false;

  emit protectFrame(protect);
  setPlaystate(PLAYSTATE_STOPPED);
  scene->setFrame(position);

  updateInputs();
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

void PinglyMotionWindow::numFramesChanged(int num)
{
  if(num<1) num=1;
  Animation* anim=scene->getAnimation();
  anim->setNumberOfFrames(num);
  calculateLongestRunningTime();

  // re-check loop boundaries (if loop set at all)
  if(anim->getLoopInPoint()!=-1)
  {
    setLoopInPoint(anim->getLoopInPoint()+1);
    setLoopOutPoint(anim->getLoopOutPoint()+1);
  }

  updateInputs();
}

void PinglyMotionWindow::easeInChanged(int change)
{
  bool ease=false;
  if(change==Qt::Checked) ease=true;

  Animation* anim=scene->getAnimation();
  anim->setEaseIn(currentPart,anim->getFrame(),ease);
}

void PinglyMotionWindow::easeOutChanged(int change)
{
  bool ease=false;
  if(change==Qt::Checked) ease=true;

  Animation* anim=scene->getAnimation();
  anim->setEaseOut(currentPart,anim->getFrame(),ease);
}

// ------ Menu Action Slots (Callbacks) -----------

// Menu action: File / New
void PinglyMotionWindow::fileNew()
{
  if(!clearOpenFiles()) return;
  emit clearProps();

  Animation* anim=new Animation(ui->animationView->getBVH());

  // set timeline animation first, because ...
  timeline->setAnimation(anim);
  // ... setting animation here will delete all old animations
  scene->setAnimation(anim);
  selectAnimation(anim);

  // add new animation to internal list
  animationIds.append(anim);
  calculateLongestRunningTime();
  // add new animation to combo box
  addToOpenFiles(m_untitledName);

  if(UserSettings::protectFirstFrame())
  {
//    qDebug("pinglymotion::fileNew(): adding loop points for protected frame 1 animation");
    // skip first frame, since it's protected anyway
    anim->setFrame(1);
    setCurrentFrame(1);
    setLoopInPoint(2);
  }
  else
  {
//    qDebug("pinglymotion::fileNew(): adding loop points for unprotected frame 1 animation");
    anim->setFrame(0);
    setCurrentFrame(0);
    setLoopInPoint(1);
  }
  setLoopOutPoint(anim->getNumberOfFrames());

  // show frame as unprotected
  emit protectFrame(false);
  protect=false;

  // FIXME: code duplication
  connect(anim,SIGNAL(currentFrame(int)),this,SLOT(setCurrentFrame(int)));

  selectTrackInTable(1);

  setPlaystate(PLAYSTATE_STOPPED);

  updateInputs();
  updateFps();

  enableAvatarRotation(false);
  enableAvatarPosition(true);
  enableProps(false);

  anim->setDirty(false);
}

QString PinglyMotionWindow::selectFileToOpen(const QString& caption)
{
   //// For some unknown reason passing "this" locks up the OSX pinglymotion window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString file = QFileDialog::getOpenFileName(NULL, caption, StartFolder::forDialogs(UserSettings::lastPath()), m_animFilter);
#else
   QString file = QFileDialog::getOpenFileName(this, caption, StartFolder::forDialogs(UserSettings::lastPath()), m_animFilter);
#endif
  if(!file.isEmpty())
  {
    QFileInfo fileInfo(file);
    if(!fileInfo.exists())
    {
      QMessageBox::warning(this,QObject::tr("Load Animation File"),QObject::tr("<qt>Animation file not found:<br />%1</qt>").arg(file));
      file=QString();
    }
    else
      UserSettings::setLastPath(fileInfo.path());
  }

  return file;
}

// Menu action: File / Open ...
void PinglyMotionWindow::fileOpen()
{
  fileOpen(QString());
}

void PinglyMotionWindow::fileOpen(const QString& name)
{
  QString file=name;

  if(file.isEmpty())
    file=selectFileToOpen(tr("Select Animation File to Load"));

  if(!file.isEmpty())
  {
    // check the file before closing the open ones
    if(!checkAnimationFile(file)) return;
    if(!clearOpenFiles()) return;
    emit clearProps();
    addAnimationFile(file);
  }

  // update timeline and animation view with the currently selected body part
  // this helps to sync the visual selections
  partChoice();
}

// Menu action: File / Add New Animation ...
void PinglyMotionWindow::fileAdd()
{
  fileAdd(QString());
}

void PinglyMotionWindow::fileAdd(const QString& name)
{
  QString file=name;

  if(file.isEmpty())
    file=selectFileToOpen(tr("Select Animation File to Add"));

  if(!file.isEmpty() && checkAnimationFile(file))
    addAnimationFile(file);
}

// shows why when the file cannot be opened; the reason is kept for remote control
bool PinglyMotionWindow::checkAnimationFile(const QString& file)
{
  m_lastOpenError.clear();
  if(!QFile::exists(file))
    m_lastOpenError=tr("Animation file not found.");
  else
    BVH::checkFile(file,&m_lastOpenError);

  if(m_lastOpenError.isEmpty()) return true;
  QMessageBox::warning(this,tr("Load Animation File"),
                       tr("<qt>%1<br><br>%2</qt>").arg(file.toHtmlEscaped(),m_lastOpenError.toHtmlEscaped()));
  return false;
}

// adds a file that checkAnimationFile() has accepted
void PinglyMotionWindow::addAnimationFile(const QString& file)
{
  {
    // an old .avm file shows up under its new name, <same name>.dingle, and counts as changed
    const bool oldAvm=QFileInfo(file).suffix().toLower()==QLatin1String("avm");
    const QString shownName=oldAvm ? DingleSave::suggestedName(file,m_untitledName) : file;
    if(oldAvm) m_avmOrigin.insert(shownName,file);
    addToOpenFiles(shownName);
    Animation* anim=new Animation(ui->animationView->getBVH(),file);
    animationIds.append(anim);
    if(ui->animationView->getBVH()->lastLoadedIgnoredKeys>0 || ui->animationView->getBVH()->lastLoadedNewerVersion)
      QMessageBox::information(this,tr("Open Animation File"),
                               tr("This file comes from a newer version of PinglyMotion.\nSome parts of it are not shown here.\nSaving it will remove those parts. Save under another name to keep the original."));
    calculateLongestRunningTime();

    setCurrentFile(shownName);

    scene->addAnimation(anim);
    timeline->setAnimation(anim);
    selectAnimation(anim);

//    qDebug("pinglymotion::fileAdd(): checking for loop points");
    // no loop in point? must be a BVH or an older avm. set a sane default
    if(anim->getLoopInPoint()==-1)
    {
//      qDebug("pinglymotion::fileAdd(): no loop points, adding new");
      // first set loop out point to avoid clamping of loop in point
      setLoopOutPoint(anim->getNumberOfFrames());

      if(UserSettings::protectFirstFrame())
      {
//        qDebug("pinglymotion::fileAdd(): adding loop points for protected frame 1 animation");
        anim->setFrame(1);
        setCurrentFrame(1);
        setLoopInPoint(2);
      }
      else
      {
//        qDebug("pinglymotion::fileAdd(): adding loop points for unprotected frame 1 animation");
        anim->setFrame(0);
        setCurrentFrame(0);
        setLoopInPoint(1);
      }
    }
    else
    {
//      qDebug("pinglymotion::fileAdd(): reading saved loop points");
      setLoopInPoint(anim->getLoopInPoint()+1);
      setLoopOutPoint(anim->getLoopOutPoint()+1);
    }

    // FIXME: code duplication
    connect(anim,SIGNAL(currentFrame(int)),this,SLOT(setCurrentFrame(int)));

    ui->animationView->selectPart(m_selectedTrack);
    updateInputs();
    updateFps();
    anim->setDirty(oldAvm);

// makeshift tool for new independant playback testing
// anim->setPlaystate(PLAYSTATE_LOOPING);

  }
}

// Menu Action: File / Save
// Save writes .dingle only. A new animation, an opened .avm and an opened .bvh
// ask for a .dingle name first. Returns true when the animation was saved.
bool PinglyMotionWindow::fileSave()
{
  if(DingleSave::needsName(currentFile,m_untitledName))
    return fileSaveAs();

  // an opened .avm is saved as .dingle with the same name; the old file goes away after that worked
  const QString avmFile=m_avmOrigin.value(currentFile);
  if(!avmFile.isEmpty() && !checkFileOverwrite(QFileInfo(currentFile))) return false;

  QString error;
  if(!scene->getAnimation()->saveBVH(currentFile,&error))
  {
    QMessageBox::warning(this,tr("Save Animation File"),error);
    return false;
  }
  if(!avmFile.isEmpty()) replaceOldAvm(avmFile,currentFile);
  return true;
}

// Menu Action: File / Save As...
// returns true when the animation was saved
bool PinglyMotionWindow::fileSaveAs()
{
  QString suggested=DingleSave::suggestedName(currentFile,m_untitledName);
  if(QFileInfo(suggested).isRelative())
    suggested=QDir(UserSettings::lastPath()).filePath(suggested);
  QString selectedFilter=m_dingleFilter;

   //// For some unknown reason passing "this" locks up the OSX pinglymotion window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString file = QFileDialog::getSaveFileName(NULL, tr("Save Animation File"), suggested, m_dingleFilter, &selectedFilter, QFileDialog:: DontConfirmOverwrite);
#else
   QString file = QFileDialog::getSaveFileName(this, tr("Save Animation File"), suggested, m_dingleFilter, &selectedFilter, QFileDialog:: DontConfirmOverwrite);
#endif

  if(file.isEmpty()) return false;
  file=DingleSave::withDingleEnding(file);

  QFileInfo fileInfo(file);

  // if the file didn't exist yet or the user accepted to overwrite it, save it.
  if(!checkFileOverwrite(fileInfo)) return false;

  const QString oldFile=m_avmOrigin.value(currentFile,currentFile);
  QString error;
  if(!scene->getAnimation()->saveBVH(file,&error))
  {
    QMessageBox::warning(this,tr("Save Animation File"),error);
    return false;
  }
  m_avmOrigin.remove(currentFile);
  setCurrentFile(file);
  UserSettings::setLastPath(fileInfo.path());
  // update animation selector combo box
  ui->selectAnimationCombo->setItemText(ui->selectAnimationCombo->currentIndex(),fileInfo.baseName());
  openFiles[ui->selectAnimationCombo->currentIndex()]=file;
  ui->fileExportForSecondLifeAction->setEnabled(true);

  // an old .avm file is replaced: it goes to the recycle bin, now that the new file is saved
  if(DingleSave::replacesOldFile(oldFile,file)) replaceOldAvm(oldFile,file);
  return true;
}

void PinglyMotionWindow::replaceOldAvm(const QString& avmFile,const QString& newFile)
{
  const QString title=tr("Save Animation File");
  m_avmOrigin.remove(newFile);
  if(!QFileInfo::exists(avmFile)) return;

  // read the new file back and compare it with the animation before the old file is touched
  QFile saved(newFile);
  bool same=false;
  if(saved.open(QIODevice::ReadOnly) && saved.size()<=Dingle::MAX_FILE_BYTES)
  {
    const Dingle::Result read=Dingle::parse(saved.readAll());
    same=read.ok() && DingleSave::sameAnimation(*read.animation,ui->animationView->getBVH()->snapshot(scene->getAnimation(),AnimFile::Format::Avm));
  }
  if(!same)
  {
    QMessageBox::warning(this,title,tr("Your animation was saved, but the new file did not pass the check.\nThe old file %1 was left where it is.").arg(QFileInfo(avmFile).fileName()));
    return;
  }
  if(!QFile::moveToTrash(avmFile))
    QMessageBox::information(this,title,tr("Your animation was saved.\nThe old file %1 could not be moved to the recycle bin.\nIt is still there. You can delete it yourself.").arg(QFileInfo(avmFile).fileName()));
}

// Menu Action: File / Export For Second Life
void PinglyMotionWindow::fileExportForSecondLife()
{
  // FIXME: think of a sensible thing to do when the animation has not been saved
  //        as .avm yet
  if (currentFile != m_untitledName)
  {
    QFileInfo fileInfo(currentFile);
    QString exportName=fileInfo.path()+"/"+fileInfo.completeBaseName()+".bvh";

    qDebug("pinglymotion::fileExportForSecondLife(): exporting animation as '%s'.",exportName.toLatin1().constData());
    QString error;
    if(!scene->getAnimation()->saveBVH(exportName,&error))
    {
      QMessageBox::warning(this,QObject::tr("Export for Second Life"),error);
      return;
    }
    QMessageBox::information(this,QObject::tr("Export for Second Life"),QObject::tr("Animation was exported for Second Life as:\n%1").arg(exportName));
  }
}

// Menu Action: File / Export as Second Life .anim...
// Save stays for .dingle files; this writes the file that Second Life uploads.
void PinglyMotionWindow::fileExportAnim()
{
  const QString title=tr("Export as Second Life .anim");
  Animation* animation=scene->getAnimation();
  if(!animation)
  {
    QMessageBox::information(this,title,AnimExportUi::problemBeforeExport(AnimFile::Animation()));
    return;
  }

  const AnimFile::Animation model=ui->animationView->getBVH()->snapshot(animation,AnimFile::Format::Bvh);
  const QString problem=AnimExportUi::problemBeforeExport(model);
  if(!problem.isEmpty())
  {
    QMessageBox::warning(this,title,problem);
    return;
  }

  AnimExportDialog dialog(AnimExportUi::durationSeconds(model),this);
  if(dialog.exec()!=QDialog::Accepted) return;

  QString suggested=AnimExportUi::suggestedFileName(currentFile,m_untitledName);
  if(QFileInfo(suggested).isRelative())
    suggested=QDir(UserSettings::lastPath()).filePath(suggested);
  QString file=QFileDialog::getSaveFileName(this,title,suggested,tr("Second Life animation (*.anim)"),nullptr,QFileDialog::DontConfirmOverwrite);
  if(file.isEmpty()) return;
  file=AnimExportUi::withAnimEnding(file);
  const QFileInfo fileInfo(file);
  if(!checkFileOverwrite(fileInfo)) return;

  const AnimExport::Options options=AnimExportUi::toOptions(dialog.settings(),model.frameTime,
                                                           animation->getLoopInPoint(),animation->getLoopOutPoint());
  const AnimExport::Result result=AnimExport::write(model,options);
  if(!result.ok())
  {
    QMessageBox::warning(this,title,AnimExportUi::explainFailure(result.error));
    return;
  }

  QSaveFile out(file);
  if(!out.open(QIODevice::WriteOnly) || out.write(result.bytes)!=result.bytes.size() || !out.commit())
  {
    QMessageBox::warning(this,title,AnimExportUi::explainFailure(
                         tr("The file could not be written: %1").arg(out.errorString())));
    return;
  }
  UserSettings::setLastPath(fileInfo.path());

  QString message=tr("Saved as:\n%1").arg(fileInfo.fileName());
  if(!result.warnings.isEmpty())
    message+=QStringLiteral("\n\n")+tr("Please note:")+QStringLiteral("\n")+result.warnings.join(QLatin1Char('\n'));
  QMessageBox::information(this,title,message);
}

// reads the Second Life skeleton that imports are put on
bool PinglyMotionWindow::loadSkeleton(AnimFile::Animation& skeleton,const QString& title)
{
  QFile file(Constants::femaleBvhPath());
  AnimFile::Result parsed;
  if(file.open(QIODevice::ReadOnly) && file.size()<=AnimFile::MAX_FILE_BYTES)
    parsed=AnimFile::parseBvh(file.readAll());
  if(!parsed.ok())
  {
    QMessageBox::warning(this,title,tr("The skeleton of the program could not be read.\nReinstall the program, then try again."));
    return false;
  }
  skeleton=*parsed.animation;
  return true;
}

// opens an animation made from another file as a new animation that has no file yet
bool PinglyMotionWindow::openConvertedAnimation(const AnimFile::Animation& anim,const QString& name,int loopIn,int loopOut,const QString& title)
{
  // the program opens animations from files, so the data goes through a short-lived one
  QTemporaryDir folder;
  const QString path=folder.filePath(QStringLiteral("imported.bvh"));
  QSaveFile out(path);
  const QByteArray bytes=AnimFile::write(anim);
  if(!folder.isValid() || !out.open(QIODevice::WriteOnly) || out.write(bytes)!=bytes.size() || !out.commit())
  {
    QMessageBox::warning(this,title,tr("The animation could not be opened.\nFree some disk space, then try again."));
    return false;
  }

  if(!checkAnimationFile(path)) return false;
  if(!clearOpenFiles()) return false;
  emit clearProps();
  addAnimationFile(path);

  // the animation belongs to no file: Save asks for a name
  setCurrentFile(m_untitledName);
  const int index=ui->selectAnimationCombo->currentIndex();
  if(index>=0 && index<openFiles.size())
  {
    openFiles[index]=m_untitledName;
    ui->selectAnimationCombo->setItemText(index,name);
  }
  Animation* animation=scene->getAnimation();
  if(loopIn>0 && loopOut>0)
  {
    setLoopOutPoint(loopOut);
    setLoopInPoint(loopIn);
  }
  animation->setDirty(true);
  ui->fileExportForSecondLifeAction->setEnabled(false);
  partChoice();
  return true;
}

// Menu Action: File / Import Second Life .anim...
void PinglyMotionWindow::fileImportAnim()
{
  const QString title=tr("Import Second Life .anim");
  const QString file=QFileDialog::getOpenFileName(this,title,StartFolder::forDialogs(UserSettings::lastPath()),tr("Second Life animation (*.anim)"));
  if(file.isEmpty()) return;
  UserSettings::setLastPath(QFileInfo(file).path());

  QFile input(file);
  if(!input.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this,title,tr("This file could not be opened.\n%1\nCheck that the file is still there, then try again.").arg(input.errorString()));
    return;
  }
  if(input.size()>AnimImport::MAX_FILE_BYTES)
  {
    QMessageBox::warning(this,title,tr("This file is too big for an animation.\nPick a Second Life .anim file, then try again."));
    return;
  }
  const QByteArray bytes=input.read(AnimImport::MAX_FILE_BYTES+1);

  AnimFile::Animation skeleton;
  if(!loadSkeleton(skeleton,title)) return;

  const AnimImportConvert::Result result=AnimImportConvert::fromBytes(bytes,skeleton);
  if(!result.ok())
  {
    QMessageBox::warning(this,title,result.error);
    return;
  }
  if(!openConvertedAnimation(*result.animation,QFileInfo(file).completeBaseName(),result.loopIn,result.loopOut,title)) return;

  QString message=tr("Opened %1.").arg(QFileInfo(file).fileName());
  if(!result.notes.isEmpty())
    message+=QStringLiteral("\n\n")+tr("What could not be imported:")+QStringLiteral("\n- ")+result.notes.join(QStringLiteral("\n- "));
  QMessageBox::information(this,title,message);
}

// Menu Action: File / Import BVH from another skeleton...
void PinglyMotionWindow::fileImportBvh()
{
  const QString title=tr("Import a BVH from another skeleton");
  const QString file=QFileDialog::getOpenFileName(this,title,StartFolder::forDialogs(UserSettings::lastPath()),tr("BVH files (*.bvh)"));
  if(file.isEmpty()) return;
  UserSettings::setLastPath(QFileInfo(file).path());

  QFile input(file);
  if(!input.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this,title,tr("This file could not be opened.\n%1\nCheck that the file is still there, then try again.").arg(input.errorString()));
    return;
  }
  if(input.size()>AnimFile::MAX_FILE_BYTES)
  {
    QMessageBox::warning(this,title,tr("This file is too big for an animation.\nPick a smaller BVH file, then try again."));
    return;
  }
  const AnimFile::Result parsed=AnimFile::parseBvh(input.read(AnimFile::MAX_FILE_BYTES+1));
  if(!parsed.ok())
  {
    const QString where=parsed.line>0 ? tr("Line %1: %2").arg(parsed.line).arg(parsed.error) : parsed.error;
    QMessageBox::warning(this,title,tr("This file could not be read.\n%1\nCheck that it is a BVH file, then try again.").arg(where));
    return;
  }

  AnimFile::Animation skeleton;
  if(!loadSkeleton(skeleton,title)) return;

  RetargetDialog dialog(*parsed.animation,skeleton,QFileInfo(file).fileName(),
                        [this](const QString& name){ return m_jointInfo ? m_jointInfo->friendlyName(name) : QString(); },this);
  if(dialog.exec()!=QDialog::Accepted || !dialog.result().ok()) return;

  openConvertedAnimation(*dialog.result().animation,QFileInfo(file).completeBaseName(),-1,-1,title);
}

// Menu Action: File / Import glTF animation...
void PinglyMotionWindow::fileImportGltf()
{
  const QString title=tr("Import glTF animation");
  const QString file=QFileDialog::getOpenFileName(this,title,StartFolder::forDialogs(UserSettings::lastPath()),tr("glTF files (*.glb *.gltf)"));
  if(file.isEmpty()) return;
  UserSettings::setLastPath(QFileInfo(file).path());

  QFile input(file);
  if(!input.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this,title,tr("This file could not be opened.\n%1\nCheck that the file is still there, then try again.").arg(input.errorString()));
    return;
  }
  if(input.size()>GltfImport::MAX_FILE_BYTES)
  {
    QMessageBox::warning(this,title,tr("This file is too big for an animation.\nPick a smaller file, then try again."));
    return;
  }
  const GltfImport::ReadResult read=GltfImport::read(input.read(GltfImport::MAX_FILE_BYTES+1));
  if(!read.ok())
  {
    QMessageBox::warning(this,title,GltfUi::readProblem(read.error));
    return;
  }

  GltfImport::Options options;
  if(GltfUi::needsChoice(*read.document))
  {
    GltfImportDialog choose(*read.document,this);
    if(choose.exec()!=QDialog::Accepted) return;
    options.animation=choose.animation();
    options.skin=choose.skeleton();
  }

  const GltfImport::SourceResult source=GltfImport::toSource(*read.document,options);
  if(!source.ok())
  {
    QMessageBox::warning(this,title,GltfUi::animationProblem(source.error));
    return;
  }

  AnimFile::Animation skeleton;
  if(!loadSkeleton(skeleton,title)) return;

  RetargetDialog dialog(*source.animation,skeleton,QFileInfo(file).fileName(),
                        [this](const QString& name){ return m_jointInfo ? m_jointInfo->friendlyName(name) : QString(); },this);
  dialog.setNotes(source.notes);
  if(dialog.exec()!=QDialog::Accepted || !dialog.result().ok()) return;

  QString name=QFileInfo(file).completeBaseName();
  if(read.document->animations.size()>1 && options.animation<read.document->animations.size() && !read.document->animations[options.animation].name.isEmpty())
    name+=QStringLiteral(" - ")+read.document->animations[options.animation].name;
  openConvertedAnimation(*dialog.result().animation,name,-1,-1,title);
}

// Menu Action: File / Load Props...
void PinglyMotionWindow::fileLoadProps()
{
   //// For some unknown reason passing "this" locks up the OSX pinglymotion window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString fileName = QFileDialog::getOpenFileName(nullptr, QString(), UserSettings::lastPath(), m_propFilter);
#else
   QString fileName = QFileDialog::getOpenFileName(this, QString(), UserSettings::lastPath(), m_propFilter);
#endif

  if(fileName.isEmpty()) return;

  // read and check the whole file first, so a bad file leaves the props as they are
  QFile file(fileName);
  QList<PropRecord> records;
  QString error;
  if(!file.open(QIODevice::ReadOnly))
    error=file.errorString();
  else if(file.size()>PropFile::MAX_FILE_BYTES)
    error=tr("The prop file is larger than %1 KB.").arg(PropFile::MAX_FILE_BYTES/1024);
  else
    PropFile::parse(file.readAll(),records,&error);

  if(!error.isEmpty())
  {
    QMessageBox::warning(this,tr("Load Props"),tr("<qt>%1<br><br>%2</qt>").arg(fileName.toHtmlEscaped(),error.toHtmlEscaped()));
    return;
  }

  emit clearProps();
  for(const PropRecord& record : records)
  {
    const auto propType=static_cast<Prop::PropType>(record.type);
    QSharedPointer<Mesh> mesh;
    switch(propType)
    {
      case Prop::PropType::Box:    mesh=cubeMesh;   break;
      case Prop::PropType::Sphere: mesh=sphereMesh; break;
      case Prop::PropType::Cone:   mesh=coneMesh;   break;
      case Prop::PropType::Torus:  mesh=torusMesh;  break;
    }

    const Prop* prop=scene->addProp(propType,mesh,
                                    record.attachment,
                                    record.position[0],record.position[1],record.position[2],
                                    record.scale[0],record.scale[1],record.scale[2],
                                    record.rotation[0],record.rotation[1],record.rotation[2]);
    if(prop)
    {
      ui->propNameCombo->addItem(prop->name());
      ui->propNameCombo->setCurrentIndex(ui->propNameCombo->count()-1);
      selectProp();
    }
  }
}

// Menu Action: File / Save Props...
void PinglyMotionWindow::fileSaveProps()
{
   //// For some unknown reason passing "this" locks up the OSX pinglymotion window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save Props"), currentFile, m_propFilter);
#else
   QString fileName = QFileDialog::getSaveFileName(this, tr("Save Props"), currentFile, m_propFilter);
#endif

  if(!fileName.isEmpty())
  {
    QFileInfo fileInfo(fileName);
    // make sure file has proper extension (.prp)
    if(fileInfo.suffix().toLower()!="prp")
      fileName+=".prp";

    // check if file exists
    // Obsolete for Qt 4.x
    if(!checkFileOverwrite(fileInfo)) return;

    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly))
    {
      for(int index=0;index<ui->propNameCombo->count();index++)
      {
        Prop* prop=scene->getPropByName(ui->propNameCombo->itemText(index));
        QStringList properties;
        properties.append(QString::number(prop->propType()));
        properties.append(QString::number(prop->xPosition()));
        properties.append(QString::number(prop->yPosition()));
        properties.append(QString::number(prop->zPosition()));
        properties.append(QString::number(prop->xScale()));
        properties.append(QString::number(prop->yScale()));
        properties.append(QString::number(prop->zScale()));
        properties.append(QString::number(prop->xRotation()));
        properties.append(QString::number(prop->yRotation()));
        properties.append(QString::number(prop->zRotation()));
        properties.append(QString::number(prop->isAttached()));
        QString line=properties.join(" ")+"\n";
        file.write(line.toLatin1().constData(),line.length());
      } // for
    }
  }
}

// Menu Action: File / Exit
void PinglyMotionWindow::fileExit()
{
  if(!clearOpenFiles())
    return;

  saveUserSettings();

  // remove all widgets and close the main form
  qApp->exit(0);
}

// Menu Action: Edit / Cut
void PinglyMotionWindow::editCut()
{
//  qDebug("pinglymotion::editCut()");
  scene->getAnimation()->cutFrame();
  frameDataValid=true;
  updateInputs();
}

// Menu Action: Edit / Copy
void PinglyMotionWindow::editCopy()
{
  scene->getAnimation()->copyFrame();
  frameDataValid=true;
  updateInputs();
}

// Menu Action: Edit / Paste
void PinglyMotionWindow::editPaste()
{
  if(frameDataValid)
  {
    scene->getAnimation()->pasteFrame();
    ui->animationView->update();
    updateInputs();
  }
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

  // update play state
  if(playstate==PLAYSTATE_LOOPING)
    setPlaystate(PLAYSTATE_PLAYING);
  else
    // just update play button icon (in case we're stopped it changes from/to looping icon)
    setPlaystate(playstate);
}

// Menu Action: Options / Joint Limits
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

// Menu Action: Options / Protect First Frame
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

// Menu Action: Options / Configure PinglyMotion
void PinglyMotionWindow::configure()
{
  UserSettingsDialog* dialog=new UserSettingsDialog(this);
  connect(dialog,SIGNAL(configChanged()),this,SLOT(configChanged()));

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
  connect(dialog.data(), SIGNAL(checkUpdates()),
          &updateChecker, SLOT(checkUpdates()));
  connect(&updateChecker, SIGNAL(updateCheckFinished()),
          dialog.data(), SLOT(updateCheckFinished()));
  dialog->exec();
}

void PinglyMotionWindow::helpWhatsNew()
{
  updateChecker.showNews();
}

// checks if a file already exists at the given path and displays a warning message
// returns true if it's ok to save/overwrite, else returns false
bool PinglyMotionWindow::checkFileOverwrite(const QFileInfo& fileInfo)
{
  // get file info
  if(fileInfo.exists())
  {
    int answer=QMessageBox::question(this,tr("File Exists"),tr("A file with the name \"%1\" does already exist. Do you want to overwrite it?").arg(fileInfo.fileName()),QMessageBox::Yes|QMessageBox::No);
    if(answer==QMessageBox::No) return false;
  }
  return true;
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
  return ui->xRotationSlider->value() / m_precision;
}

float PinglyMotionWindow::getY()
{
  return ui->yRotationSlider->value() / m_precision;
}

float PinglyMotionWindow::getZ()
{
  return ui->zRotationSlider->value() / m_precision;
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
  return ui->xPositionSlider->value() / m_precision;
}

float PinglyMotionWindow::getYPos()
{
  return ui->yPositionSlider->value() / m_precision;
}

float PinglyMotionWindow::getZPos()
{
  return ui->zPositionSlider->value() / m_precision;
}

void PinglyMotionWindow::enableProps(bool state)
{
  ui->propPositionGroup->setEnabled(state);
  ui->propScaleGroup->setEnabled(state);
  ui->propRotationGroup->setEnabled(state);
  ui->attachToLabel->setEnabled(state);
  ui->attachToComboBox->setEnabled(state);
}

void PinglyMotionWindow::enableAvatarRotation(bool state)
{
  ui->rotationGroupBox->setEnabled(state);
  if(m_jointCard) m_jointCard->setEditable(state);
}

void PinglyMotionWindow::enableAvatarPosition(bool state)
{
  ui->positionGroupBox->setEnabled(state);
  if(m_jointCard) m_jointCard->setMoveEditable(state);
}

// --- the selected-joint card (#88, step 4)

// name, IK, lock and limits of the selected joint on the card
void PinglyMotionWindow::updateJointCard()
{
  if(!m_jointCard) return;
  Animation* anim=scene ? scene->getAnimation() : nullptr;
  if(!anim || !currentPart)
  {
    m_jointCard->setIk(false,false);
    m_jointCard->setLocked(false);
    m_jointCard->setLimited(false);
    m_jointCard->setJoint(JointCard::NoJoint,JointCard::Header());
    updateJointCardTrackball();
    return;
  }

  JointCard::Kind kind=JointCard::Joint;
  if(currentPart->type==BVH_POS) kind=JointCard::PositionTrack;
  else if(currentPart->type==BVH_ROOT) kind=JointCard::Root;

  m_jointCard->setIk(Animation::hasIK(currentPart->name()),anim->getIK(currentPart));
  m_jointCard->setLocked(isLocked(currentPart));
  // the pelvis turns all the way round, and with Options > Joint Limits off
  // the sliders go from -180 to 180: no Second Life limit to warn about
  m_jointCard->setLimited(kind==JointCard::Joint && UserSettings::jointLimits());
  m_jointCard->setJoint(kind,JointCard::headerFor(m_jointInfo.data(),currentPart->name()));
  m_jointCard->refresh();
  updateJointCardTrackball();
}

// the ball shows the joint as it is turned on the screen
void PinglyMotionWindow::updateJointCardTrackball()
{
  if(!m_jointCard) return;
  Animation* anim=scene ? scene->getAnimation() : nullptr;
  if(!anim || !currentPart || currentPart->type==BVH_POS)
  {
    m_jointCard->setTrackballOrientation(viewRotation());
    return;
  }
  const QQuaternion local=JointTurn::fromEuler(anim->getRotation(currentPart),rotationOrder(currentPart));
  m_jointCard->setTrackballOrientation(viewRotation()*parentRotation(currentPart)*local);
}

bool PinglyMotionWindow::isLocked(const BVHNode* node) const
{
  return node && m_lockedJoints.contains(node->name());
}

QString PinglyMotionWindow::rotationOrder(const BVHNode* node)
{
  QString order;
  for(int i=0;i<node->numChannels;i++)
  {
    switch(node->channelType[i])
    {
      case BVH_XROT: order+=QLatin1Char('X'); break;
      case BVH_YROT: order+=QLatin1Char('Y'); break;
      case BVH_ZROT: order+=QLatin1Char('Z'); break;
      default: break;
    }
  }
  return order;
}

namespace
{
  // the joints from "from" down to "target", without "target"; false when
  // target is not below from
  bool jointsAbove(BVHNode* from,const BVHNode* target,QList<BVHNode*>& path)
  {
    if(from==target) return true;
    path.append(from);
    for(int i=0;i<from->numChildren();i++)
      if(jointsAbove(from->child(i),target,path)) return true;
    path.removeLast();
    return false;
  }
}

QQuaternion PinglyMotionWindow::parentRotation(BVHNode* node)
{
  Animation* anim=scene->getAnimation();
  QList<BVHNode*> path;
  QQuaternion parent;
  if(!jointsAbove(anim->getMotion(),node,path)) return parent;
  for(BVHNode* joint : path)
    parent=parent*JointTurn::fromEuler(anim->getRotation(joint),rotationOrder(joint));
  return parent;
}

// the camera turns the scene around X (up and down) after turning it around Y
// (round the figure), see Camera
QQuaternion PinglyMotionWindow::viewRotation() const
{
  const Camera* camera=ui->animationView->camera();
  if(!camera) return QQuaternion();
  return QQuaternion::fromAxisAndAngle(1,0,0,camera->xRotation())*
         QQuaternion::fromAxisAndAngle(0,1,0,camera->yRotation());
}

// the IK switch on the card: the same as Ctrl + double-click on a hand or foot
void PinglyMotionWindow::setSelectedJointIk(bool on)
{
  Animation* anim=scene->getAnimation();
  if(!anim || !currentPart || !Animation::hasIK(currentPart->name())) return;
  anim->setIK(currentPart,on);
  ui->animationView->update();
  updateJointCard();
}

// the lock on the card: a locked joint is left alone by the sliders, the
// trackball and the rings in the 3D view
void PinglyMotionWindow::setSelectedJointLocked(bool locked)
{
  if(!currentPart) return;
  if(locked) m_lockedJoints.insert(currentPart->name());
  else m_lockedJoints.remove(currentPart->name());
}

// Reset on the card: the joint back to its rest pose (0, 0, 0) in this frame;
// the avatar position back to where it stands in the first frame
void PinglyMotionWindow::resetSelectedJoint()
{
  Animation* anim=scene->getAnimation();
  if(!anim || !currentPart || protect || isLocked(currentPart)) return;
  if(currentPart->type==BVH_POS)
  {
    const QVector3D start=anim->getNode(0)->frameData(0).position();
    anim->setPosition(start.x(),start.y(),start.z());
    setXPos(start.x());
    setYPos(start.y());
    setZPos(start.z());
  }
  else
  {
    anim->setRotation(currentPart,0,0,0);
    setX(0);
    setY(0);
    setZ(0);
  }
  ui->animationView->update();
  updateJointCardTrackball();
  updateKeyBtn();
}

// the trackball was dragged: turn the joint around the up and the sideways
// axis of the chosen frame; the limits are applied as for the rings
void PinglyMotionWindow::turnSelectedJoint(double yawDegrees,double pitchDegrees,int frame)
{
  Animation* anim=scene->getAnimation();
  if(!anim || !currentPart || currentPart->type==BVH_POS || protect || isLocked(currentPart)) return;
  const QVector3D before=anim->getRotation(currentPart);
  const QString order=rotationOrder(currentPart);
  const QQuaternion parent=parentRotation(currentPart);
  const QQuaternion view=viewRotation();
  const JointTurn::Frame turnFrame=static_cast<JointTurn::Frame>(frame);
  QVector3D after=JointTurn::turn(before,order,parent,view,turnFrame,QVector3D(0,1,0),yawDegrees);
  after=JointTurn::turn(after,order,parent,view,turnFrame,QVector3D(1,0,0),pitchDegrees);
  const QVector3D delta=after-before;
  partDragged(currentPart,delta.x(),delta.y(),delta.z());
}

bool PinglyMotionWindow::eventFilter(QObject* watched,QEvent* event)
{
  // Ctrl + double-click switches IK in the 3D view after this event is
  // handled; the card shows the new state afterwards
  if(watched==ui->animationView && event->type()==QEvent::MouseButtonDblClick)
    QTimer::singleShot(0,this,[this]{ updateJointCard(); });
  return QMainWindow::eventFilter(watched,event);
}

void PinglyMotionWindow::updatePropSpins(const QVector3D& position, const QVector3D& rotation, const QVector3D& scale)
{
  ui->propXPosSpin->blockSignals(true);
  ui->propYPosSpin->blockSignals(true);
  ui->propZPosSpin->blockSignals(true);

  ui->propXPosSpin->setValue(static_cast<int>(position.x()));
  ui->propYPosSpin->setValue(static_cast<int>(position.y()));
  ui->propZPosSpin->setValue(static_cast<int>(position.z()));

  ui->propXPosSpin->blockSignals(false);
  ui->propYPosSpin->blockSignals(false);
  ui->propZPosSpin->blockSignals(false);

  ui->propXRotSpin->blockSignals(true);
  ui->propYRotSpin->blockSignals(true);
  ui->propZRotSpin->blockSignals(true);

  ui->propXRotSpin->setValue((360 + static_cast<int>(rotation.x())) % 360);
  ui->propYRotSpin->setValue((360 + static_cast<int>(rotation.y())) % 360);
  ui->propZRotSpin->setValue((360 + static_cast<int>(rotation.z())) % 360);

  ui->propXRotSpin->blockSignals(false);
  ui->propYRotSpin->blockSignals(false);
  ui->propZRotSpin->blockSignals(false);

  ui->propXScaleSpin->blockSignals(true);
  ui->propYScaleSpin->blockSignals(true);
  ui->propZScaleSpin->blockSignals(true);

  ui->propXScaleSpin->setValue(static_cast<int>(scale.x()));
  ui->propYScaleSpin->setValue(static_cast<int>(scale.y()));
  ui->propZScaleSpin->setValue(static_cast<int>(scale.z()));

  ui->propXScaleSpin->blockSignals(false);
  ui->propYScaleSpin->blockSignals(false);
  ui->propZScaleSpin->blockSignals(false);
}

void PinglyMotionWindow::onNewProp(const QString& propName)
{
  ui->propNameCombo->addItem(propName);
  ui->propNameCombo->setCurrentIndex(ui->propNameCombo->count() - 1);
  selectProp();
  ui->attachToComboBox->setCurrentIndex(0);
}

void PinglyMotionWindow::onSelectProp(bool isSelected, int attachmentPoint)
{
  ui->propNameCombo->setEnabled(isSelected);
  ui->deletePropButton->setEnabled(isSelected);
  if (isSelected)
  {
    ui->attachToComboBox->setCurrentIndex(attachmentPoint);
  }
  ui->animationView->selectProp();
}

void PinglyMotionWindow::onDeleteProp(const QString propName)
{
  for (int index = 0; index < ui->propNameCombo->count(); index++)
  {
    if (ui->propNameCombo->itemText(index) == propName)
    {
      ui->propNameCombo->removeItem(index);
      selectProp();
    }
  }
}

void PinglyMotionWindow::onClearProps()
{
  ui->propNameCombo->clear();
}

void PinglyMotionWindow::selectProp()
{
  emit selectProp(ui->propNameCombo->currentIndex());
}

// helper function to prevent feedback between the two widgets
void PinglyMotionWindow::setSliderValue(QSlider* slider,QLineEdit* edit,float value)
{
  slider->blockSignals(true);
  edit->blockSignals(true);
  slider->setValue((int)(value * m_precision));
  edit->setText(QString::number(value));
  edit->blockSignals(false);
  slider->blockSignals(false);
  // the card's range texts and limit warning follow (the slider's own
  // signals were blocked)
  if(m_jointCard) m_jointCard->refresh();
}

void PinglyMotionWindow::updateFps()
{
  int fps=scene->getAnimation()->fps();

  // guard against division by zero
  if(fps)
  {
    // don't send FPS change back to Animation object
    ui->framesSpin->blockSignals(true);
    ui->fpsSpin->setValue(fps);
    // re-enable FPS signal
    ui->framesSpin->blockSignals(false);
  }
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
//  animationView->setFrame(frame);
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
  qDebug("pinglymotion::animationChanged(%d)",which);

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
  ui->fileExportForSecondLifeAction->setEnabled(currentFile != m_untitledName);
}

// set loop in point (user view, so always +1)
void PinglyMotionWindow::setLoopInPoint(int inFrame)
{
  Animation* anim=scene->getAnimation();

  // FIXME: Temporary fix for a seg fault caused
  //  by adding then deleting a prop immediately
  //  before closing the PinglyMotion window
  if (!anim) return;

  int numOfFrames=anim->getNumberOfFrames();
  int outFrame=anim->getLoopOutPoint();
  qDebug("pinglymotion::setLoopInPoint(%d) (%d frames)",inFrame,numOfFrames);

  if(inFrame>numOfFrames) inFrame=numOfFrames;
  if(inFrame>outFrame) inFrame=outFrame+1;
  if(inFrame<1) inFrame=1;

  anim->setLoopInPoint(inFrame-1);

  ui->loopInSpinBox->setValue(inFrame);

  ui->loopInPercentLabel->setText(QString("(%1%)").arg(inFrame*100/numOfFrames));
}

// set loop out point (user view, so always +1)
void PinglyMotionWindow::setLoopOutPoint(int outFrame)
{
  Animation* anim=scene->getAnimation();
  int numOfFrames=anim->getNumberOfFrames();
  int inFrame=anim->getLoopInPoint();
  qDebug("pinglymotion::setLoopOutPoint(%d) (%d frames)",outFrame,numOfFrames);

  if(outFrame>numOfFrames) outFrame=numOfFrames;
  if((outFrame-1)<inFrame) outFrame=inFrame+1;
  if(outFrame<1) outFrame=1;

  anim->setLoopOutPoint(outFrame-1);

  ui->loopOutSpinBox->setValue(outFrame);

  ui->loopOutPercentLabel->setText(QString("(%1%)").arg(outFrame*100/numOfFrames));
}

void PinglyMotionWindow::setPlaystate(PlayState state)
{
//  qDebug("pinglymotion::setPlaystate(): setting playstate %d",(int) state);
  // set state
  playstate=state;

  // set play button icons according to play state
  if(state==PLAYSTATE_STOPPED)
    ui->playButton->setIcon(UserSettings::loop() ? loopIcon : playIcon);
  else if(state==PLAYSTATE_LOOPING)
    ui->playButton->setIcon(playIcon);
  else if(state==PLAYSTATE_PLAYING)
    ui->playButton->setIcon(stopIcon);
  else
    qDebug("pinglymotion::setPlaystate(): unknown playstate %d",(int) state);
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
  UserSettings::setPanelLayout(saveState(PanelLayout::CurrentVersion));
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
  qDebug("pinglymotion::calculateLongestRunningTime()");
  longestRunningTime=0.0;
  for(unsigned int index=0;index< (unsigned int) animationIds.count();index++)
  {
    Animation* anim=animationIds.at(index);
    double time=anim->getNumberOfFrames()/((double) anim->fps());
    if(time>longestRunningTime) longestRunningTime=time;
  }
  qDebug("pinglymotion::calculateLongestRunningTime(): longestRunningTime now: %f seconds",longestRunningTime);
  return longestRunningTime;
}

// -------------------------------------------------------------------------
// autoconnection from designer UI

// ------- Menu Action Slots --------

void PinglyMotionWindow::on_fileNewAction_triggered()
{
  fileNew();
}

void PinglyMotionWindow::on_fileOpenAction_triggered()
{
  fileOpen();
}

void PinglyMotionWindow::on_fileAddAction_triggered()
{
  fileAdd();
}

void PinglyMotionWindow::on_fileSaveAction_triggered()
{
  fileSave();
}

void PinglyMotionWindow::on_fileSaveAsAction_triggered()
{
  fileSaveAs();
}

void PinglyMotionWindow::on_fileImportGltfAction_triggered()
{
  fileImportGltf();
}

void PinglyMotionWindow::on_fileImportBvhAction_triggered()
{
  fileImportBvh();
}

void PinglyMotionWindow::on_fileImportAnimAction_triggered()
{
  fileImportAnim();
}

void PinglyMotionWindow::on_fileExportAnimAction_triggered()
{
  fileExportAnim();
}

void PinglyMotionWindow::on_fileExportForSecondLifeAction_triggered()
{
  fileExportForSecondLife();
}

void PinglyMotionWindow::on_fileLoadPropsAction_triggered()
{
  fileLoadProps();
}

void PinglyMotionWindow::on_fileSavePropsAction_triggered()
{
  fileSaveProps();
}

void PinglyMotionWindow::on_fileExitAction_triggered()
{
  fileExit();
}

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

void PinglyMotionWindow::on_easeInCheck_stateChanged(int newState)
{
  easeInChanged(newState);
}

void PinglyMotionWindow::on_easeOutCheck_stateChanged(int newState)
{
  easeOutChanged(newState);
}

void PinglyMotionWindow::on_newBoxPropButton_clicked()
{
  emit newProp(Prop::Box, cubeMesh);
}

void PinglyMotionWindow::on_newSpherePropButton_clicked()
{
  emit newProp(Prop::Sphere, sphereMesh);
}

void PinglyMotionWindow::on_newConePropButton_clicked()
{
  emit newProp(Prop::Cone, coneMesh);
}

void PinglyMotionWindow::on_newTorusPropButton_clicked()
{
  emit newProp(Prop::Torus, torusMesh);
}

void PinglyMotionWindow::on_propNameCombo_activated(int index)
{
  selectProp();
}

void PinglyMotionWindow::on_deletePropButton_clicked()
{
  emit deleteProp();
}

void PinglyMotionWindow::on_attachToComboBox_activated(int attachmentPoint)
{
  // FIXME: find better solution for filtering endpoint for joints
  if (ui->attachToComboBox->currentText() == "-")
  {
    attachmentPoint = 0;
  }
  emit attachProp(attachmentPoint);
}

void PinglyMotionWindow::on_propXPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PinglyMotionWindow::on_propYPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PinglyMotionWindow::on_propZPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PinglyMotionWindow::on_propXScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PinglyMotionWindow::on_propYScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PinglyMotionWindow::on_propZScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PinglyMotionWindow::on_propXRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

void PinglyMotionWindow::on_propYRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

void PinglyMotionWindow::on_propZRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

void PinglyMotionWindow::on_currentFrameSlider_valueChanged(int newValue)
{
  frameSlider(newValue);
}

void PinglyMotionWindow::on_playButton_clicked()
{
  nextPlaystate();
}

void PinglyMotionWindow::on_keyframeButton_toggled(bool on)
{
  qDebug("on_keyframeButton_toggled(%d)",(int) on);
  scene->getAnimation()->toggleKeyFrame(currentPart); // (animationView->getSelectedPart());
  ui->animationView->update();
}

void PinglyMotionWindow::on_loopInSpinBox_editingFinished()
{
  setLoopInPoint(ui->loopInSpinBox->value());
}

void PinglyMotionWindow::on_loopOutSpinBox_editingFinished()
{
  setLoopOutPoint(ui->loopOutSpinBox->value());
}

void PinglyMotionWindow::on_framesSpin_valueChanged(int newValue)
{
  numFramesChanged(newValue);
}

void PinglyMotionWindow::on_fpsSpin_valueChanged(int newValue)
{
  setFPS(newValue);
}

// end autoconnection from designer UI
// -------------------------------------------------------------------------
