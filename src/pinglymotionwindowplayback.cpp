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
#include "logging.h"
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
#include "easingtiles.h"
#include "propfile.h"
#include "constants.h"
#include "controlapi.h"
#include "controlserver.h"
#include "controlindicator.h"
#include "auroralink.h"
#include "sliderscale.h"
#include <QStatusBar>
#include "prop.h"
#include "jointgroups.h"
#include "jointinfo.h"
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
#include "playback.h"
#include "theme.h"
#include "ui_pinglymotionwindow.h"
#include "whatsnewdialog.h"
#include "wheelguard.h"

// What the remote control commands do in the window: the same steps as clicking.

// Part of PinglyMotionWindow: frames, loop points and playback.
// (Split out of pinglymotionwindow.cpp; nothing here changes what the window does.)

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
        anim->setPlaystate(PLAYSTATE_STOPPED);
      }

      updateInputs();
      return;
    }
  }
}

void PinglyMotionWindow::nextPlaystate()
{

  Animation* anim=scene->getAnimation();

  switch(playstate)
  {
    case PLAYSTATE_STOPPED:
    {
      // the playhead stays on the last frame after a one-shot play: start again
      anim->setFrame(Playback::startFrame(anim->getFrame(),anim->getNumberOfFrames(),anim->getLoopInPoint()));

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
      qCWarning(lcView,"nextPlaystate(): unknown playstate %d",(int) playstate);
  }

  updateInputs();
}

void PinglyMotionWindow::setFPS(int fps)
{

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

  if(outFrame>numOfFrames) outFrame=numOfFrames;
  if((outFrame-1)<inFrame) outFrame=inFrame+1;
  if(outFrame<1) outFrame=1;

  anim->setLoopOutPoint(outFrame-1);

  ui->loopOutSpinBox->setValue(outFrame);

  ui->loopOutPercentLabel->setText(QString("(%1%)").arg(outFrame*100/numOfFrames));
}

void PinglyMotionWindow::setPlaystate(PlayState state)
{
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
    qCWarning(lcView,"setPlaystate(): unknown playstate %d",(int) state);
}

void PinglyMotionWindow::on_currentFrameSlider_valueChanged(int newValue)
{
  frameSlider(newValue);
}

void PinglyMotionWindow::on_playButton_clicked()
{
  nextPlaystate();
}

void PinglyMotionWindow::on_keyframeButton_toggled(bool)
{
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
