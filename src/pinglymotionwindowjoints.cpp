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

// Part of PinglyMotionWindow: the joint picker, the Selected joint panel, rotation and position fields, and the designer auto-connected slots.
// (Split out of pinglymotionwindow.cpp; nothing here changes what the window does.)

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

    // showing the animation's own frame count and speed must not write them back: with two
    // figures that marked the figure that became active as changed (docs/UX_DECISIONS.md 6m)
    {
      const QSignalBlocker blockFrames(ui->framesSpin);
      ui->framesSpin->setValue(anim->getNumberOfFrames());
    }
    ui->currentFrameSlider->setMaximum(anim->getNumberOfFrames()-1);
    // Frame back / Frame forward: not past the first or the last frame
    if(m_transport) m_transport->setFrame(anim->getFrame(),anim->getNumberOfFrames());
    {
      const QSignalBlocker blockFps(ui->fpsSpin);
      ui->fpsSpin->setValue(anim->fps());
    }

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

