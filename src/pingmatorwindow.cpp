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
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
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
#include <QTabBar>
#include <QTableWidget>

#include "aboutdialog.h"
#include "animationview.h"
#include "bvh.h"
#include "propfile.h"
#include "constants.h"
#include "controlapi.h"
#include "controlserver.h"
#include "controlindicator.h"
#include "auroralink.h"
#include <QStatusBar>
#include "prop.h"
#include "jointgroups.h"
#include "paneltitlebar.h"
#include "pingmator.h"
#include "scene.h"
#include "timeline.h"
#include "timelineview.h"
#include "updatersettings.h"
#include "usersettings.h"
#include "usersettingsdialog.h"

#include "pingmatorwindow.h"
#include "theme.h"
#include "ui_pingmatorwindow.h"
#include "whatsnewdialog.h"

// What the remote control commands do in the window: the same steps as clicking.
class WindowControlHost : public ControlHost
{
  public:
    explicit WindowControlHost(PingmatorWindow* window) : w(window) {}

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
      if (!animation() || w->currentFile != path)
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

    PingmatorWindow* w;
};

PingmatorWindow::PingmatorWindow() :
  QMainWindow(nullptr),
  ui(new Ui::PingmatorWindow),
  m_pingmator(new Pingmator(this)),
  updateChecker(nullptr, this),
  cubeMesh(new Mesh(Constants::cubeMeshFilePath(), Mesh::Shape::CUBE)),
  sphereMesh(new Mesh(QString(), Mesh::Shape::SPHERE)),  // built in code, no file
  coneMesh(new Mesh(QString(), Mesh::Shape::CONE)),      // built in code, no file
  torusMesh(new Mesh(Constants::torusMeshFilePath(), Mesh::Shape::TORUS)),
  m_animFilter("Animation Files (*.avm *.bvh)"),
  m_bvhFilter(QObject::tr("Second Life animation (*.bvh)")),
  m_avmFilter(QObject::tr("Pingmator animation (*.avm)")),
  m_propFilter("Props (*.prp)"),
  m_untitledName("Untitled.avm"),
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

  setWindowTitle("Pingmator");
  setAttribute(Qt::WA_DeleteOnClose);
  frameDataValid=false;
  currentPart=0;
  longestRunningTime=0.0;
  scene = ui->animationView->initializeScene(this);
  m_pingmator->setScene(scene);
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
  fitPropertiesWidth();

  resize(UserSettings::mainWindowWidth(),UserSettings::mainWindowHeight());

  ui->optionsLoopAction->setChecked(UserSettings::loop());
  ui->optionsSkeletonAction->setChecked(UserSettings::skeleton());
  ui->optionsJointLimitsAction->setChecked(UserSettings::jointLimits());
  ui->optionsShowTimelineAction->setChecked(UserSettings::showTimelinePanel());

  // Options > Dark theme
  QAction* darkThemeAction=new QAction(tr("Dark theme"),this);
  darkThemeAction->setCheckable(true);
  darkThemeAction->setChecked(UserSettings::darkTheme());
  ui->menuOptions->addAction(darkThemeAction);
  connect(darkThemeAction,&QAction::toggled,this,[this](bool dark)
  {
    UserSettings::setDarkTheme(dark);
    Theme::apply(dark);
    fitPropertiesWidth();
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
     tr("The open source Max avatar, with fingers and a face that follow the Bento bones.")},
    {"system",AvatarSkin::ClassicBody,tr("Second Life system avatar"),
     tr("The standard Second Life avatar, without Bento fingers and face.")}};
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
    if(!ui->animationView->setBody(wanted)) current=QStringLiteral("system");   // could not load: system avatar
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
    AvatarSkin::Body wanted=AvatarSkin::ClassicBody;
    if(key==QLatin1String("ruth2")) wanted=AvatarSkin::RuthBody;
    else if(key==QLatin1String("max")) wanted=AvatarSkin::MaxBody;
    if(!ui->animationView->setBody(wanted))
    {
      bodyActions.value(QStringLiteral("system"))->setChecked(true);
      UserSettings::setBody(QStringLiteral("system"));
      QMessageBox::warning(this,tr("Body"),tr("The avatar could not be loaded. Showing the system avatar."));
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
  connect(lightAction,&QAction::triggered,this,&PingmatorWindow::showLightDialog);

  if(!UserSettings::showTimelinePanel()) m_timelineDock->hide();

  // panel layout from last time (size, place, floating panels)
  m_defaultWindowState=saveState();
  if(!UserSettings::windowState().isEmpty()) restoreState(UserSettings::windowState());
  // a layout saved by an older version may have a panel on a side it no longer goes
  if(!m_propertiesDock->isFloating() && !(dockWidgetArea(m_propertiesDock) & m_propertiesDock->allowedAreas()))
    addDockWidget(Qt::RightDockWidgetArea,m_propertiesDock);
  if(!m_timelineDock->isFloating() && !(dockWidgetArea(m_timelineDock) & m_timelineDock->allowedAreas()))
    addDockWidget(Qt::BottomDockWidgetArea,m_timelineDock);


  connect(m_pingmator.data(), &Pingmator::repaint, ui->animationView, static_cast<void (AnimationView::*)()>(&AnimationView::repaint));
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

  connect(this, &PingmatorWindow::newProp, m_pingmator.data(), &Pingmator::newProp);
  connect(this, static_cast<void (PingmatorWindow::*)(int)>(&PingmatorWindow::selectProp), m_pingmator.data(), &Pingmator::selectProp);
  connect(this, &PingmatorWindow::attachProp, m_pingmator.data(), &Pingmator::attachProp);
  connect(this, &PingmatorWindow::propPositionChanged, m_pingmator.data(), &Pingmator::propPositionChanged);
  connect(this, &PingmatorWindow::propScaleChanged, m_pingmator.data(), &Pingmator::propScaleChanged);
  connect(this, &PingmatorWindow::propRotationChanged, m_pingmator.data(), &Pingmator::propRotationChanged);
  connect(this, &PingmatorWindow::deleteProp, m_pingmator.data(), &Pingmator::deleteProp);
  connect(this, &PingmatorWindow::clearProps, m_pingmator.data(), &Pingmator::clearProps);
  connect(m_pingmator.data(), &Pingmator::newPropUi, this, &PingmatorWindow::onNewProp);
  connect(m_pingmator.data(), &Pingmator::selectPropUi, this, &PingmatorWindow::onSelectProp);
  connect(m_pingmator.data(), &Pingmator::deletePropUi, this, &PingmatorWindow::onDeleteProp);
  connect(m_pingmator.data(), &Pingmator::clearPropsUi, this, &PingmatorWindow::onClearProps);
  connect(m_pingmator.data(), &Pingmator::enablePropsUi, this, &PingmatorWindow::enableProps);
  connect(m_pingmator.data(), &Pingmator::enableAvatarRotationUi, this, &PingmatorWindow::enableAvatarRotation);
  connect(m_pingmator.data(), &Pingmator::enableAvatarPositionUi, this, &PingmatorWindow::enableAvatarPosition);
  connect(m_pingmator.data(), &Pingmator::updatePropSpinsUi, this, &PingmatorWindow::updatePropSpins);

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
  m_controlAction->setToolTip(tr("Lets programs on this computer pose and play the animation. Off every time Pingmator starts, unless started with --control."));
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
  connect(m_auroraLink,&AuroraLink::warningReceived,this,[this](const QString& message)
  {
    statusBar()->showMessage(tr("Aurora link: %1").arg(message),10000);
  });
  // status bar: Aurora link off, connecting or live; the dot turns it on or off
  m_auroraIndicator=new AuroraIndicator(this);
  statusBar()->addPermanentWidget(m_auroraIndicator);
  connect(m_auroraIndicator,&AuroraIndicator::toggleRequested,m_auroraAction,&QAction::toggle);

  // command line: pingmator [--control] [animation file]
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

void PingmatorWindow::setRemoteControl(bool on,bool showInfo)
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

// the title says so while other programs can control Pingmator
void PingmatorWindow::updateWindowTitle()
{
  QString title="Pingmator "+Constants::buildLabel()+" ["+currentFile+"]";
  if(m_controlServer && m_controlServer->running()) title+=tr(" - remote control on");
  if(m_controlIndicator) m_controlIndicator->setRunning(m_controlServer && m_controlServer->running());
  if(m_auroraLink && m_auroraLink->enabled())
    title+=m_auroraLink->connected() ? tr(" - live in Aurora") : tr(" - waiting for Aurora");
  if(m_auroraIndicator && m_auroraLink) m_auroraIndicator->setStatus(m_auroraLink->enabled(),m_auroraLink->connected());
  setWindowTitle(title);
}

PingmatorWindow::~PingmatorWindow()
{
  fileExit();
}

void PingmatorWindow::queueAfterShow()
{
  // QTimer creates a timerEvent
  // - This is a convenient way to queue "afterShow" to
  //   run after the main window shows
  QTimer::singleShot(0, this, SLOT(afterShow()));
}

void PingmatorWindow::afterShow()
{
  updateChecker.onStartup();
}

// slot gets called by AnimationView::mousePressEvent()
void PingmatorWindow::partClicked(BVHNode* node, QVector3D rotation, RotationLimits limits, QVector3D position)
{
  ui->avatarPropsTab->setCurrentIndex(0);
  enableProps(false);

  if(!node)
  {
    qDebug("pingmator::partClicked(node,...): node==0!");
    return;
  }

  if(node)
  {
    qDebug("pingmator::partClicked(node): %s",node->name().toLatin1().constData());
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

    // show the user if this part has a key frame here
    updateKeyBtn();
  }
}

// slot gets called by AnimationView::mouseMoveEvent()
void PingmatorWindow::partDragged(BVHNode* node,double x,double y,double z)
{
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
  else qDebug("pingmator::partDragged(): node==0!");
}

// slot gets called by AnimationView::propClicked()
void PingmatorWindow::propClicked(Prop* prop)
{
  ui->avatarPropsTab->setCurrentIndex(1);

  // update prop name combo box
  for(int index=0;index<ui->propNameCombo->count();index++)
    if(ui->propNameCombo->itemText(index)==prop->name()) ui->propNameCombo->setCurrentIndex(index);

  // update prop value spinboxes
  selectProp();
}

// slot gets called by AnimationView::mouseMoveEvent()
void PingmatorWindow::propDragged(Prop* prop,double x,double y,double z)
{
  double newX = prop->xPosition() + x;
  double newY = prop->yPosition() + y;
  double newZ = prop->zPosition() + z;
  prop->setPosition(newX, newY, newZ);
  updatePropSpins(prop->position(), prop->rotation(), prop->scale());
}

// slot gets called by AnimationView::mouseMoveEvent()
void PingmatorWindow::propScaled(Prop* prop,double x,double y,double z)
{
  double newX = prop->xScale() + x;
  double newY = prop->yScale() + y;
  double newZ = prop->zScale() + z;
  prop->setScale(newX, newY, newZ);
  updatePropSpins(prop->position(), prop->rotation(), prop->scale());
}

// slot gets called by AnimationView::mouseMoveEvent()
void PingmatorWindow::propRotated(Prop* prop,double x,double y,double z)
{
  double newX = prop->xRotation() + x;
  double newY = prop->yRotation() + y;
  double newZ = prop->zRotation() + z;
  prop->setRotation(newX, newY, newZ);
  updatePropSpins(prop->position(), prop->rotation(), prop->scale());
}

// slot gets called by AnimationView::mouseButtonClicked()
void PingmatorWindow::backgroundClicked()
{
  currentPart=0;

  enableAvatarRotation(false);
  enableProps(false);
  emit enableEaseInOut(false);
  selectTrackInTable(0);
  updateKeyBtn();
}

// The 3D view fills the window. The timeline and the properties are panels
// (dock widgets) that can be resized, moved to another side, or torn off
// into their own window. Each panel only goes where its shape fits: the
// timeline is wide (top or bottom), the properties are tall (left or right).
void PingmatorWindow::buildDocks()
{
  setDockOptions(QMainWindow::AnimatedDocks);

  // --- timeline panel: tracks, frame slider, play and key frame buttons
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

  m_timelineDock=new QDockWidget(tr("Timeline"),this);
  m_timelineDock->setObjectName("timelineDock");
  m_timelineDock->setWidget(timelinePanel);
  m_timelineDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  m_timelineDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
  m_timelineDock->setTitleBarWidget(new PanelTitleBar(m_timelineDock));

  // --- properties panel: avatar / props tabs, loop points, frames and FPS
  QWidget* propertiesPanel=new QWidget;
  QVBoxLayout* propertiesLayout=new QVBoxLayout(propertiesPanel);
  propertiesLayout->setContentsMargins(4,4,4,4);
  propertiesLayout->addWidget(ui->avatarPropsTab,1);

  QGridLayout* loopLayout=new QGridLayout;
  loopLayout->addWidget(ui->loopInLabel,0,0);
  loopLayout->addWidget(ui->loopInSpinBox,0,1);
  loopLayout->addWidget(ui->loopInPercentLabel,0,2);
  loopLayout->addWidget(ui->loopOutLabel,1,0);
  loopLayout->addWidget(ui->loopOutSpinBox,1,1);
  loopLayout->addWidget(ui->loopOutPercentLabel,1,2);
  propertiesLayout->addLayout(loopLayout);

  QHBoxLayout* frameLayout=new QHBoxLayout;
  frameLayout->addWidget(ui->frameLabel);
  frameLayout->addWidget(ui->currentFrameLabel);
  frameLayout->addWidget(ui->ofLabel);
  frameLayout->addWidget(ui->framesSpin);
  frameLayout->addStretch(1);
  frameLayout->addWidget(ui->fpsLabel);
  frameLayout->addWidget(ui->fpsSpin);
  propertiesLayout->addLayout(frameLayout);

  m_propertiesDock=new QDockWidget(tr("Properties"),this);
  m_propertiesDock->setObjectName("propertiesDock");
  // The panel keeps its width, whatever the window around it does. If the
  // window is too low for all of it, it scrolls.
  QScrollArea* propertiesScroll=new QScrollArea;
  propertiesScroll->setWidget(propertiesPanel);
  propertiesScroll->setWidgetResizable(true);
  propertiesScroll->setFrameShape(QFrame::NoFrame);
  propertiesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_propertiesScroll=propertiesScroll;
  m_propertiesDock->setWidget(propertiesScroll);
  m_propertiesDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  m_propertiesDock->setTitleBarWidget(new PanelTitleBar(m_propertiesDock));

  // --- 3D view in the middle (replaces the old fixed layout)
  QWidget* central=new QWidget;
  QVBoxLayout* centralLayout=new QVBoxLayout(central);
  centralLayout->setContentsMargins(0,0,0,0);
  centralLayout->addWidget(ui->animationView);
  setCentralWidget(central);

  addDockWidget(Qt::RightDockWidgetArea,m_propertiesDock);
  addDockWidget(Qt::BottomDockWidgetArea,m_timelineDock);
  // the properties panel uses the full height, the timeline sits under the 3D view
  setCorner(Qt::BottomRightCorner,Qt::RightDockWidgetArea);
  resizeDocks({m_timelineDock},{280},Qt::Vertical);

  // Options > Show Properties, next to Show Timeline
  QAction* showPropertiesAction=m_propertiesDock->toggleViewAction();
  showPropertiesAction->setText(tr("Show Properties"));
  ui->menuOptions->insertAction(ui->optionsConfigurePingmatorAction,showPropertiesAction);
  // closing the timeline with its own button unticks Show Timeline
  connect(m_timelineDock->toggleViewAction(),&QAction::toggled,
          ui->optionsShowTimelineAction,&QAction::setChecked);

  // Options > Reset panel layout
  QAction* resetLayoutAction=new QAction(tr("Reset panel layout"),this);
  ui->menuOptions->addAction(resetLayoutAction);
  connect(resetLayoutAction,&QAction::triggered,this,[this]()
  {
    restoreState(m_defaultWindowState);
    m_timelineDock->setFloating(false);
    m_propertiesDock->setFloating(false);
    m_timelineDock->show();
    m_propertiesDock->show();
    ui->optionsShowTimelineAction->setChecked(true);
  });
}

// Options > Light...: three sliders that change the body light at once.
// The dialog stays open while you work; the values are saved with the
// other settings.
void PingmatorWindow::showLightDialog()
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

// The properties panel is as wide as its contents need - no more, no less.
// Called again when the theme changes, since that changes the widget sizes.
void PingmatorWindow::fitPropertiesWidth()
{
  QWidget* panel=m_propertiesScroll->widget();
  panel->ensurePolished();
  const int width=qMax(panel->sizeHint().width(),panel->minimumSizeHint().width());
  m_propertiesScroll->setFixedWidth(width+m_propertiesScroll->verticalScrollBar()->sizeHint().width());
}

// joint picker in the properties panel: tabs Body / Face / Hands / Other
// above a table with the joints of that tab. Replaces the old "Edit Part"
// drop-down list. Only the list is filtered; the 3D view shows all joints.
void PingmatorWindow::buildJointPicker()
{
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
  layout->setSpacing(0);
  layout->addWidget(m_jointTabs);
  layout->addWidget(m_jointTable);

  connect(m_jointTabs,&QTabBar::currentChanged,this,[this](int) { fillJointTable(); });
  // a new row (mouse or arrow keys), or a click on the marked row while
  // that joint is not selected in the view (at start, after a background click)
  connect(m_jointTable,&QTableWidget::currentCellChanged,this,[this](int row)
  {
    if(row<0 || row>=nodeMapping.count()) return;
    m_selectedTrack=nodeMapping[row];
    partChoice();
  });
  connect(m_jointTable,&QTableWidget::cellClicked,this,[this](int row)
  {
    if(row<0 || row>=nodeMapping.count()) return;
    const QString name=m_jointTable->item(row,0)->text();
    if(currentPart && currentPart->name()==name) return;
    m_selectedTrack=nodeMapping[row];
    partChoice();
  });
}

// fill the joint table with the joints of the current tab
// track numbers: 0 = position, then every skeleton node, "Site" = End Site
void PingmatorWindow::fillJointTable()
{
  const QStringList names=ui->animationView->partNames();
  const int tab=m_jointTabs->currentIndex();

  QSignalBlocker block(m_jointTable);
  nodeMapping.clear();
  m_jointTable->setRowCount(0);

  for(int track=0;track<names.count();track++)
  {
    const QString& name=names[track];
    if(name=="Site" || JointGroups::tabOf(name)!=tab) continue;
    // joints of hidden groups: left out, or listed in a dimmed colour
    if(JointGroups::trackHidden(track) && UserSettings::hideJointsInLists()) continue;

    QTableWidgetItem* item=new QTableWidgetItem(name);
    if(JointGroups::trackHidden(track))
      item->setForeground(palette().color(QPalette::Disabled,QPalette::Text));
    const int row=m_jointTable->rowCount();
    m_jointTable->insertRow(row);
    m_jointTable->setItem(row,0,item);
    nodeMapping << track;
  }
  showSelectedTrack();
}

// mark the selected joint in the table, if it is on the current tab
void PingmatorWindow::showSelectedTrack()
{
  QSignalBlocker block(m_jointTable);
  const int row=nodeMapping.indexOf(m_selectedTrack);
  if(row<0)
  {
    m_jointTable->clearSelection();
    m_jointTable->setCurrentItem(nullptr);
    return;
  }
  m_jointTable->setCurrentCell(row,0);
  m_jointTable->scrollToItem(m_jointTable->item(row,0));
}

// select a joint in the table and switch to its tab (does not select it in the view)
void PingmatorWindow::selectTrackInTable(int track)
{
  m_selectedTrack=track;
  const QStringList names=ui->animationView->partNames();
  if(track>=0 && track<names.count())
  {
    const int tab=JointGroups::tabOf(names[track]);
    if(tab!=m_jointTabs->currentIndex())
    {
      QSignalBlocker block(m_jointTabs);
      m_jointTabs->setCurrentIndex(tab);
      fillJointTable();
      return;
    }
  }
  showSelectedTrack();
}

// fill the joint table and "Attach To" from the skeleton
// track numbers: 0 = position, then every skeleton node, "Site" = End Site
void PingmatorWindow::buildPartLists()
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
void PingmatorWindow::jointGroupsChanged()
{
  ui->animationView->jointGroupsChanged();
  ui->timelineView->layoutChanged();

  const int attachIndex=ui->attachToComboBox->currentIndex();
  QSignalBlocker blockAttach(ui->attachToComboBox);
  buildPartLists();
  ui->attachToComboBox->setCurrentIndex(attachIndex);
}

// a joint was chosen in the joint table (or the file was opened)
void PingmatorWindow::partChoice()
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
void PingmatorWindow::rotationSlider(const QObject* slider)
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
void PingmatorWindow::rotationValue()
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

  updateKeyBtn();
}

void PingmatorWindow::positionSlider(const QObject* slider)
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

void PingmatorWindow::positionValue()
{
  qDebug("pingmator::positionValue()");

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

void PingmatorWindow::updateInputs()
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

void PingmatorWindow::updateKeyBtn()
{
  Animation* anim=scene->getAnimation();
  qDebug("pingmator::updateKeyBtn(): anim=%p",(const void*) anim);

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

void PingmatorWindow::enableInputs(bool state)
{
  // protection overrides state for keyframe button
  if(protect) state=false;
  ui->keyframeButton->setEnabled(state);

  // do not enable rotation if we have no part selected
  if(currentPart == NULL) state=false;
  enableAvatarRotation(state);
}

void PingmatorWindow::frameTimeout()
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

void PingmatorWindow::nextPlaystate()
{
//  qDebug("pingmator::nextPlaystate(): current playstate %d",(int) playstate);

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
      qDebug("pingmator::nextPlaystate(): unknown playstate %d",(int) playstate);
  }

  updateInputs();
}

void PingmatorWindow::setFPS(int fps)
{
  qDebug("pingmator::setFPS(%d)",fps);

  Animation* anim=scene->getAnimation();
  if(!anim) return;

  // sanity check
  if(fps<1) fps=1;
  else if(fps>50) fps=50;

  anim->setFPS(fps);
  calculateLongestRunningTime();
}

void PingmatorWindow::frameSlider(int position)
{
  // check if we are at the first frame and if it's protected
  if(position==0 && UserSettings::protectFirstFrame()) protect=true;
  else protect=false;

  emit protectFrame(protect);
  setPlaystate(PLAYSTATE_STOPPED);
  scene->setFrame(position);

  updateInputs();
}

void PingmatorWindow::setAvatarShape(int shape)
{
  Animation* anim=scene->getAnimation();
  if(!anim) return;

  if(shape==0)
    anim->setFigureType(Animation::FIGURE_FEMALE);
  else
    anim->setFigureType(Animation::FIGURE_MALE);
  ui->animationView->update();
}

void PingmatorWindow::setAvatarScale(int percent)
{
  scene->getAnimation()->setAvatarScale(percent/100.0);
  ui->animationView->update();
}

void PingmatorWindow::numFramesChanged(int num)
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

void PingmatorWindow::easeInChanged(int change)
{
  bool ease=false;
  if(change==Qt::Checked) ease=true;

  Animation* anim=scene->getAnimation();
  anim->setEaseIn(currentPart,anim->getFrame(),ease);
}

void PingmatorWindow::easeOutChanged(int change)
{
  bool ease=false;
  if(change==Qt::Checked) ease=true;

  Animation* anim=scene->getAnimation();
  anim->setEaseOut(currentPart,anim->getFrame(),ease);
}

// ------ Menu Action Slots (Callbacks) -----------

// Menu action: File / New
void PingmatorWindow::fileNew()
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
//    qDebug("pingmator::fileNew(): adding loop points for protected frame 1 animation");
    // skip first frame, since it's protected anyway
    anim->setFrame(1);
    setCurrentFrame(1);
    setLoopInPoint(2);
  }
  else
  {
//    qDebug("pingmator::fileNew(): adding loop points for unprotected frame 1 animation");
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

QString PingmatorWindow::selectFileToOpen(const QString& caption)
{
   //// For some unknown reason passing "this" locks up the OSX pingmator window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString file = QFileDialog::getOpenFileName(NULL, caption, UserSettings::lastPath(), m_animFilter);
#else
   QString file = QFileDialog::getOpenFileName(this, caption, UserSettings::lastPath(), m_animFilter);
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
void PingmatorWindow::fileOpen()
{
  fileOpen(QString());
}

void PingmatorWindow::fileOpen(const QString& name)
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
void PingmatorWindow::fileAdd()
{
  fileAdd(QString());
}

void PingmatorWindow::fileAdd(const QString& name)
{
  QString file=name;

  if(file.isEmpty())
    file=selectFileToOpen(tr("Select Animation File to Add"));

  if(!file.isEmpty() && checkAnimationFile(file))
    addAnimationFile(file);
}

// shows why when the file cannot be opened; the reason is kept for remote control
bool PingmatorWindow::checkAnimationFile(const QString& file)
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
void PingmatorWindow::addAnimationFile(const QString& file)
{
  {
    addToOpenFiles(file);
    Animation* anim=new Animation(ui->animationView->getBVH(),file);
    animationIds.append(anim);
    calculateLongestRunningTime();

    setCurrentFile(file);

    scene->addAnimation(anim);
    timeline->setAnimation(anim);
    selectAnimation(anim);

//    qDebug("pingmator::fileAdd(): checking for loop points");
    // no loop in point? must be a BVH or an older avm. set a sane default
    if(anim->getLoopInPoint()==-1)
    {
//      qDebug("pingmator::fileAdd(): no loop points, adding new");
      // first set loop out point to avoid clamping of loop in point
      setLoopOutPoint(anim->getNumberOfFrames());

      if(UserSettings::protectFirstFrame())
      {
//        qDebug("pingmator::fileAdd(): adding loop points for protected frame 1 animation");
        anim->setFrame(1);
        setCurrentFrame(1);
        setLoopInPoint(2);
      }
      else
      {
//        qDebug("pingmator::fileAdd(): adding loop points for unprotected frame 1 animation");
        anim->setFrame(0);
        setCurrentFrame(0);
        setLoopInPoint(1);
      }
    }
    else
    {
//      qDebug("pingmator::fileAdd(): reading saved loop points");
      setLoopInPoint(anim->getLoopInPoint()+1);
      setLoopOutPoint(anim->getLoopOutPoint()+1);
    }

    // FIXME: code duplication
    connect(anim,SIGNAL(currentFrame(int)),this,SLOT(setCurrentFrame(int)));

    ui->animationView->selectPart(m_selectedTrack);
    updateInputs();
    updateFps();
    anim->setDirty(false);

// makeshift tool for new independant playback testing
// anim->setPlaystate(PLAYSTATE_LOOPING);

  }
}

// Menu Action: File / Save
// returns true when the animation was saved
bool PingmatorWindow::fileSave()
{
  if (currentFile == m_untitledName)
    return fileSaveAs();

  QString error;
  if(!scene->getAnimation()->saveBVH(currentFile,&error))
  {
    QMessageBox::warning(this,tr("Save Animation File"),error);
    return false;
  }
  return true;
}

// Menu Action: File / Save As...
// returns true when the animation was saved
bool PingmatorWindow::fileSaveAs()
{
  // the user picks the file type: .bvh for Second Life, .avm keeps Pingmator's own data
  const QString filters=m_bvhFilter+";;"+m_avmFilter;
  QString selectedFilter=QFileInfo(currentFile).suffix().toLower()=="bvh" ? m_bvhFilter : m_avmFilter;

   //// For some unknown reason passing "this" locks up the OSX pingmator window. Possibly a QT4 bug, needs investigation
#ifdef __APPLE__
   QString file = QFileDialog::getSaveFileName(NULL, tr("Save Animation File"), currentFile, filters, &selectedFilter, QFileDialog:: DontConfirmOverwrite);
#else
   QString file = QFileDialog::getSaveFileName(this, tr("Save Animation File"), currentFile, filters, &selectedFilter, QFileDialog:: DontConfirmOverwrite);
#endif

  if(file.isEmpty()) return false;

  // make sure file has proper extension (either .bvh or .avm), taken from the chosen file type
  QString extension=QFileInfo(file).suffix().toLower();
  if(extension!="avm" && extension!="bvh")
    file+=(selectedFilter==m_bvhFilter) ? ".bvh" : ".avm";

  QFileInfo fileInfo(file);

  // if the file didn't exist yet or the user accepted to overwrite it, save it.
  if(!checkFileOverwrite(fileInfo)) return false;

  QString error;
  if(!scene->getAnimation()->saveBVH(file,&error))
  {
    QMessageBox::warning(this,tr("Save Animation File"),error);
    return false;
  }
  setCurrentFile(file);
  UserSettings::setLastPath(fileInfo.path());
  // update animation selector combo box
  ui->selectAnimationCombo->setItemText(ui->selectAnimationCombo->currentIndex(),fileInfo.baseName());
  openFiles[ui->selectAnimationCombo->currentIndex()]=file;
  ui->fileExportForSecondLifeAction->setEnabled(true);
  return true;
}

// Menu Action: File / Export For Second Life
void PingmatorWindow::fileExportForSecondLife()
{
  // FIXME: think of a sensible thing to do when the animation has not been saved
  //        as .avm yet
  if (currentFile != m_untitledName)
  {
    QFileInfo fileInfo(currentFile);
    QString exportName=fileInfo.path()+"/"+fileInfo.completeBaseName()+".bvh";

    qDebug("pingmator::fileExportForSecondLife(): exporting animation as '%s'.",exportName.toLatin1().constData());
    QString error;
    if(!scene->getAnimation()->saveBVH(exportName,&error))
    {
      QMessageBox::warning(this,QObject::tr("Export for Second Life"),error);
      return;
    }
    QMessageBox::information(this,QObject::tr("Export for Second Life"),QObject::tr("Animation was exported for Second Life as:\n%1").arg(exportName));
  }
}

// Menu Action: File / Load Props...
void PingmatorWindow::fileLoadProps()
{
   //// For some unknown reason passing "this" locks up the OSX pingmator window. Possibly a QT4 bug, needs investigation
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
void PingmatorWindow::fileSaveProps()
{
   //// For some unknown reason passing "this" locks up the OSX pingmator window. Possibly a QT4 bug, needs investigation
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
void PingmatorWindow::fileExit()
{
  if(!clearOpenFiles())
    return;

  saveUserSettings();

  // remove all widgets and close the main form
  qApp->exit(0);
}

// Menu Action: Edit / Cut
void PingmatorWindow::editCut()
{
//  qDebug("pingmator::editCut()");
  scene->getAnimation()->cutFrame();
  frameDataValid=true;
  updateInputs();
}

// Menu Action: Edit / Copy
void PingmatorWindow::editCopy()
{
  scene->getAnimation()->copyFrame();
  frameDataValid=true;
  updateInputs();
}

// Menu Action: Edit / Paste
void PingmatorWindow::editPaste()
{
  if(frameDataValid)
  {
    scene->getAnimation()->pasteFrame();
    ui->animationView->update();
    updateInputs();
  }
}

// Menu Action: Edit / Paste
void PingmatorWindow::toolsOptimizeBVH()
{
  scene->getAnimation()->optimize();
  updateInputs();
}

// Menu Action: Options / Skeleton
void PingmatorWindow::setSkeleton(bool on)
{
  UserSettings::setSkeleton(on);
}

// Menu Action: Options / Loop
void PingmatorWindow::setLoop(bool on)
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
void PingmatorWindow::setJointLimits(bool on)
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
void PingmatorWindow::setProtectFirstFrame(bool on)
{
  UserSettings::setProtectFirstFrame(on);
  if(on && ui->currentFrameSlider->value()==0) protect=true;
  else protect=false;

  emit protectFrame(protect);
  updateInputs();
}

// Menu Action: Options / Show Timeline
void PingmatorWindow::showTimeline(bool on)
{
  m_timelineDock->setVisible(on);
}

// Menu Action: Options / Configure Pingmator
void PingmatorWindow::configure()
{
  UserSettingsDialog* dialog=new UserSettingsDialog(this);
  connect(dialog,SIGNAL(configChanged()),this,SLOT(configChanged()));

  dialog->exec();

  delete dialog;
}

void PingmatorWindow::configChanged()
{
  ui->animationView->update();
}

// Menu Action: Help / About ...
void PingmatorWindow::helpAbout()
{
  QScopedPointer<AboutDialog> dialog(new AboutDialog(this));
  connect(dialog.data(), SIGNAL(checkUpdates()),
          &updateChecker, SLOT(checkUpdates()));
  connect(&updateChecker, SIGNAL(updateCheckFinished()),
          dialog.data(), SLOT(updateCheckFinished()));
  dialog->exec();
}

void PingmatorWindow::helpWhatsNew()
{
  updateChecker.showNews();
}

// checks if a file already exists at the given path and displays a warning message
// returns true if it's ok to save/overwrite, else returns false
bool PingmatorWindow::checkFileOverwrite(const QFileInfo& fileInfo)
{
  // get file info
  if(fileInfo.exists())
  {
    int answer=QMessageBox::question(this,tr("File Exists"),tr("A file with the name \"%1\" does already exist. Do you want to overwrite it?").arg(fileInfo.fileName()),QMessageBox::Yes|QMessageBox::No);
    if(answer==QMessageBox::No) return false;
  }
  return true;
}

void PingmatorWindow::setX(float x)
{
  setSliderValue(ui->xRotationSlider,ui->xRotationEdit,x);
}

void PingmatorWindow::setY(float y)
{
  setSliderValue(ui->yRotationSlider,ui->yRotationEdit,y);
}

void PingmatorWindow::setZ(float z)
{
  setSliderValue(ui->zRotationSlider,ui->zRotationEdit,z);
}

float PingmatorWindow::getX()
{
  return ui->xRotationSlider->value() / m_precision;
}

float PingmatorWindow::getY()
{
  return ui->yRotationSlider->value() / m_precision;
}

float PingmatorWindow::getZ()
{
  return ui->zRotationSlider->value() / m_precision;
}

void PingmatorWindow::setXPos(float x)
{
  setSliderValue(ui->xPositionSlider,ui->xPositionEdit,x);
}

void PingmatorWindow::setYPos(float y)
{
  setSliderValue(ui->yPositionSlider,ui->yPositionEdit,y);
}

void PingmatorWindow::setZPos(float z)
{
  setSliderValue(ui->zPositionSlider,ui->zPositionEdit,z);
}

float PingmatorWindow::getXPos()
{
  return ui->xPositionSlider->value() / m_precision;
}

float PingmatorWindow::getYPos()
{
  return ui->yPositionSlider->value() / m_precision;
}

float PingmatorWindow::getZPos()
{
  return ui->zPositionSlider->value() / m_precision;
}

void PingmatorWindow::enableProps(bool state)
{
  ui->propPositionGroup->setEnabled(state);
  ui->propScaleGroup->setEnabled(state);
  ui->propRotationGroup->setEnabled(state);
  ui->attachToLabel->setEnabled(state);
  ui->attachToComboBox->setEnabled(state);
}

void PingmatorWindow::enableAvatarRotation(bool state)
{
  ui->rotationGroupBox->setEnabled(state);
}

void PingmatorWindow::enableAvatarPosition(bool state)
{
  ui->positionGroupBox->setEnabled(state);
}

void PingmatorWindow::updatePropSpins(const QVector3D& position, const QVector3D& rotation, const QVector3D& scale)
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

void PingmatorWindow::onNewProp(const QString& propName)
{
  ui->propNameCombo->addItem(propName);
  ui->propNameCombo->setCurrentIndex(ui->propNameCombo->count() - 1);
  selectProp();
  ui->attachToComboBox->setCurrentIndex(0);
}

void PingmatorWindow::onSelectProp(bool isSelected, int attachmentPoint)
{
  ui->propNameCombo->setEnabled(isSelected);
  ui->deletePropButton->setEnabled(isSelected);
  if (isSelected)
  {
    ui->attachToComboBox->setCurrentIndex(attachmentPoint);
  }
  ui->animationView->selectProp();
}

void PingmatorWindow::onDeleteProp(const QString propName)
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

void PingmatorWindow::onClearProps()
{
  ui->propNameCombo->clear();
}

void PingmatorWindow::selectProp()
{
  emit selectProp(ui->propNameCombo->currentIndex());
}

// helper function to prevent feedback between the two widgets
void PingmatorWindow::setSliderValue(QSlider* slider,QLineEdit* edit,float value)
{
  slider->blockSignals(true);
  edit->blockSignals(true);
  slider->setValue((int)(value * m_precision));
  edit->setText(QString::number(value));
  edit->blockSignals(false);
  slider->blockSignals(false);
}

void PingmatorWindow::updateFps()
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
void PingmatorWindow::addToOpenFiles(const QString& fileName)
{
    openFiles.append(fileName);

    QString fixedName=fileName;
    fixedName.remove(QRegularExpression(".*/"));
    fixedName.remove(QRegularExpression("(\\.bvh|\\.avm)"));

    ui->selectAnimationCombo->addItem(fixedName);
}

// asks what to do with unsaved animations: Save (default), Discard or Cancel.
// returns true when it is fine to go on (saved or discarded)
bool PingmatorWindow::askToSaveChanges()
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
bool PingmatorWindow::clearOpenFiles()
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
void PingmatorWindow::setCurrentFile(const QString& fileName)
{
  currentFile=fileName;
  updateWindowTitle();
}

// this slot gets called from Animation::setFrame(int)
void PingmatorWindow::setCurrentFrame(int frame)
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
void PingmatorWindow::animationChanged(int which)
{
  qDebug("pingmator::animationChanged(%d)",which);

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
void PingmatorWindow::selectAnimation(Animation* animation)
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
void PingmatorWindow::setLoopInPoint(int inFrame)
{
  Animation* anim=scene->getAnimation();

  // FIXME: Temporary fix for a seg fault caused
  //  by adding then deleting a prop immediately
  //  before closing the Pingmator window
  if (!anim) return;

  int numOfFrames=anim->getNumberOfFrames();
  int outFrame=anim->getLoopOutPoint();
  qDebug("pingmator::setLoopInPoint(%d) (%d frames)",inFrame,numOfFrames);

  if(inFrame>numOfFrames) inFrame=numOfFrames;
  if(inFrame>outFrame) inFrame=outFrame+1;
  if(inFrame<1) inFrame=1;

  anim->setLoopInPoint(inFrame-1);

  ui->loopInSpinBox->setValue(inFrame);

  ui->loopInPercentLabel->setText(QString("(%1%)").arg(inFrame*100/numOfFrames));
}

// set loop out point (user view, so always +1)
void PingmatorWindow::setLoopOutPoint(int outFrame)
{
  Animation* anim=scene->getAnimation();
  int numOfFrames=anim->getNumberOfFrames();
  int inFrame=anim->getLoopInPoint();
  qDebug("pingmator::setLoopOutPoint(%d) (%d frames)",outFrame,numOfFrames);

  if(outFrame>numOfFrames) outFrame=numOfFrames;
  if((outFrame-1)<inFrame) outFrame=inFrame+1;
  if(outFrame<1) outFrame=1;

  anim->setLoopOutPoint(outFrame-1);

  ui->loopOutSpinBox->setValue(outFrame);

  ui->loopOutPercentLabel->setText(QString("(%1%)").arg(outFrame*100/numOfFrames));
}

void PingmatorWindow::setPlaystate(PlayState state)
{
//  qDebug("pingmator::setPlaystate(): setting playstate %d",(int) state);
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
    qDebug("pingmator::setPlaystate(): unknown playstate %d",(int) state);
}

// prevent closing of main window if there are unsaved changes
// options, window size and panel layout, remembered for next time
void PingmatorWindow::saveUserSettings()
{
  UserSettings::setSkeleton(ui->optionsSkeletonAction->isChecked());
  UserSettings::setJointLimits(ui->optionsJointLimitsAction->isChecked());
  UserSettings::setProtectFirstFrame(ui->optionsProtectFirstFrameAction->isChecked());
  UserSettings::setShowTimelinePanel(ui->optionsShowTimelineAction->isChecked());

  UserSettings::setMainWindowWidth(size().width());
  UserSettings::setMainWindowHeight(size().height());
  UserSettings::setWindowState(saveState());

  UserSettings::writeSettings();
}

void PingmatorWindow::closeEvent(QCloseEvent* event)
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
double PingmatorWindow::calculateLongestRunningTime()
{
  qDebug("pingmator::calculateLongestRunningTime()");
  longestRunningTime=0.0;
  for(unsigned int index=0;index< (unsigned int) animationIds.count();index++)
  {
    Animation* anim=animationIds.at(index);
    double time=anim->getNumberOfFrames()/((double) anim->fps());
    if(time>longestRunningTime) longestRunningTime=time;
  }
  qDebug("pingmator::calculateLongestRunningTime(): longestRunningTime now: %f seconds",longestRunningTime);
  return longestRunningTime;
}

// -------------------------------------------------------------------------
// autoconnection from designer UI

// ------- Menu Action Slots --------

void PingmatorWindow::on_fileNewAction_triggered()
{
  fileNew();
}

void PingmatorWindow::on_fileOpenAction_triggered()
{
  fileOpen();
}

void PingmatorWindow::on_fileAddAction_triggered()
{
  fileAdd();
}

void PingmatorWindow::on_fileSaveAction_triggered()
{
  fileSave();
}

void PingmatorWindow::on_fileSaveAsAction_triggered()
{
  fileSaveAs();
}

void PingmatorWindow::on_fileExportForSecondLifeAction_triggered()
{
  fileExportForSecondLife();
}

void PingmatorWindow::on_fileLoadPropsAction_triggered()
{
  fileLoadProps();
}

void PingmatorWindow::on_fileSavePropsAction_triggered()
{
  fileSaveProps();
}

void PingmatorWindow::on_fileExitAction_triggered()
{
  fileExit();
}

void PingmatorWindow::on_editCutAction_triggered()
{
  editCut();
}

void PingmatorWindow::on_editCopyAction_triggered()
{
  editCopy();
}

void PingmatorWindow::on_editPasteAction_triggered()
{
  editPaste();
}

void PingmatorWindow::on_toolsOptimizeBVHAction_triggered()
{
  toolsOptimizeBVH();
}

void PingmatorWindow::on_toolsMirrorAction_triggered()
{
  Animation* anim=scene->getAnimation();
  anim->mirror(currentPart);
  updateInputs();
}

void PingmatorWindow::on_optionsSkeletonAction_toggled(bool on)
{
  setSkeleton(on);
}

void PingmatorWindow::on_optionsJointLimitsAction_toggled(bool on)
{
  setJointLimits(on);
}

void PingmatorWindow::on_optionsLoopAction_toggled(bool on)
{
  setLoop(on);
}

void PingmatorWindow::on_optionsProtectFirstFrameAction_toggled(bool on)
{
  setProtectFirstFrame(on);
}

void PingmatorWindow::on_optionsShowTimelineAction_toggled(bool on)
{
  showTimeline(on);
}

void PingmatorWindow::on_optionsConfigurePingmatorAction_triggered()
{
  configure();
}

void PingmatorWindow::on_helpAboutAction_triggered()
{
  helpAbout();
}

void PingmatorWindow::on_helpWhatsNewAction_triggered()
{
  helpWhatsNew();
}

// ------- Additional Toolbar Element Slots --------

void PingmatorWindow::on_resetCameraAction_triggered()
{
  emit resetCamera();
}

// ------- UI Element Slots --------

void PingmatorWindow::on_selectAnimationCombo_activated(int which)
{
  animationChanged(which);
}

void PingmatorWindow::on_figureCombo_activated(int newShape)
{
  setAvatarShape(newShape);
}

void PingmatorWindow::on_scaleSpin_valueChanged(int newValue)
{
  setAvatarScale(newValue);
}

void PingmatorWindow::on_xRotationEdit_editingFinished()
{
  rotationValue();
}

void PingmatorWindow::on_xRotationSlider_valueChanged(int)
{
  rotationSlider(sender());
}

void PingmatorWindow::on_yRotationEdit_editingFinished()
{
  rotationValue();
}

void PingmatorWindow::on_yRotationSlider_valueChanged(int)
{
  rotationSlider(sender());
}

void PingmatorWindow::on_zRotationEdit_editingFinished()
{
  rotationValue();
}

void PingmatorWindow::on_zRotationSlider_valueChanged(int)
{
  rotationSlider(sender());
}

void PingmatorWindow::on_xPositionEdit_editingFinished()
{
  positionValue();
}

void PingmatorWindow::on_xPositionSlider_valueChanged(int)
{
  positionSlider(sender());
}

void PingmatorWindow::on_yPositionEdit_editingFinished()
{
  positionValue();
}

void PingmatorWindow::on_yPositionSlider_valueChanged(int)
{
  positionSlider(sender());
}

void PingmatorWindow::on_zPositionEdit_editingFinished()
{
  positionValue();
}

void PingmatorWindow::on_zPositionSlider_valueChanged(int)
{
  positionSlider(sender());
}

void PingmatorWindow::on_easeInCheck_stateChanged(int newState)
{
  easeInChanged(newState);
}

void PingmatorWindow::on_easeOutCheck_stateChanged(int newState)
{
  easeOutChanged(newState);
}

void PingmatorWindow::on_newBoxPropButton_clicked()
{
  emit newProp(Prop::Box, cubeMesh);
}

void PingmatorWindow::on_newSpherePropButton_clicked()
{
  emit newProp(Prop::Sphere, sphereMesh);
}

void PingmatorWindow::on_newConePropButton_clicked()
{
  emit newProp(Prop::Cone, coneMesh);
}

void PingmatorWindow::on_newTorusPropButton_clicked()
{
  emit newProp(Prop::Torus, torusMesh);
}

void PingmatorWindow::on_propNameCombo_activated(int index)
{
  selectProp();
}

void PingmatorWindow::on_deletePropButton_clicked()
{
  emit deleteProp();
}

void PingmatorWindow::on_attachToComboBox_activated(int attachmentPoint)
{
  // FIXME: find better solution for filtering endpoint for joints
  if (ui->attachToComboBox->currentText() == "-")
  {
    attachmentPoint = 0;
  }
  emit attachProp(attachmentPoint);
}

void PingmatorWindow::on_propXPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PingmatorWindow::on_propYPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PingmatorWindow::on_propZPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PingmatorWindow::on_propXScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PingmatorWindow::on_propYScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PingmatorWindow::on_propZScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PingmatorWindow::on_propXRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

void PingmatorWindow::on_propYRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

void PingmatorWindow::on_propZRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

void PingmatorWindow::on_currentFrameSlider_valueChanged(int newValue)
{
  frameSlider(newValue);
}

void PingmatorWindow::on_playButton_clicked()
{
  nextPlaystate();
}

void PingmatorWindow::on_keyframeButton_toggled(bool on)
{
  qDebug("on_keyframeButton_toggled(%d)",(int) on);
  scene->getAnimation()->toggleKeyFrame(currentPart); // (animationView->getSelectedPart());
  ui->animationView->update();
}

void PingmatorWindow::on_loopInSpinBox_editingFinished()
{
  setLoopInPoint(ui->loopInSpinBox->value());
}

void PingmatorWindow::on_loopOutSpinBox_editingFinished()
{
  setLoopOutPoint(ui->loopOutSpinBox->value());
}

void PingmatorWindow::on_framesSpin_valueChanged(int newValue)
{
  numFramesChanged(newValue);
}

void PingmatorWindow::on_fpsSpin_valueChanged(int newValue)
{
  setFPS(newValue);
}

// end autoconnection from designer UI
// -------------------------------------------------------------------------
