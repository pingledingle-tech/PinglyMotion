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
#include <QKeyEvent>
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
#include "transportbar.h"
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

// Start: play from the playhead. At the last frame it begins again at the
// loop-in frame (or the first frame), so Start always plays something.
void PinglyMotionWindow::startPlaying()
{
  if(playstate!=PLAYSTATE_STOPPED) return;
  Animation* anim=scene->getAnimation();
  if(!anim) return;

  anim->setFrame(Playback::startFrame(anim->getFrame(),anim->getNumberOfFrames(),anim->getLoopInPoint()));
  beginPlaying(anim);
}

// Stop: the playhead stays on the frame that is shown
void PinglyMotionWindow::stopPlaying()
{
  if(playstate==PLAYSTATE_STOPPED) return;
  timer.stop();

  // take care of locks, key frames ... (also shows the stopped state)
  frameSlider(ui->currentFrameSlider->value());
  if(Animation* anim=scene->getAnimation())
    anim->setPlaystate(PLAYSTATE_STOPPED);

  updateInputs();
}

// Replay from start: back to the first frame and play, also while playing
void PinglyMotionWindow::replayFromStart()
{
  Animation* anim=scene->getAnimation();
  if(!anim) return;
  timer.stop();

  frameSlider(Playback::replayFrame());
  anim->setPlaystate(PLAYSTATE_STOPPED);
  beginPlaying(anim);
}

// Frame back / Frame forward: stop playing, then one frame, the same way as
// moving the frame slider. At the first or the last frame nothing changes.
void PinglyMotionWindow::stepFrame(int delta)
{
  Animation* anim=scene->getAnimation();
  if(!anim) return;
  stopPlaying();

  const int frame=Playback::stepFrame(anim->getFrame(),anim->getNumberOfFrames(),delta);
  if(frame!=anim->getFrame()) frameSlider(frame);
}

// the left / right arrow keys step the frame (docs/POSING_UX.md, keys table).
// Fields, lists and the frame slider use the arrow keys themselves and keep
// them; the 3D view and the empty parts of the window pass them on to here.
void PinglyMotionWindow::keyPressEvent(QKeyEvent* event)
{
  if(event->modifiers()==Qt::NoModifier || event->modifiers()==Qt::KeypadModifier)
  {
    if(event->key()==Qt::Key_Left) { stepFrame(-1); event->accept(); return; }
    if(event->key()==Qt::Key_Right) { stepFrame(1); event->accept(); return; }
  }
  QMainWindow::keyPressEvent(event);
}

// plays from the current frame: repeating between the loop points when Loop
// is on, otherwise once to the last frame
void PinglyMotionWindow::beginPlaying(Animation* anim)
{
  const PlayState state=Playback::startState(UserSettings::loop(),anim->getFrame(),anim->getLoopOutPoint());
  setPlaystate(state);
  anim->setPlaystate(state);

  timer.start((int)(1.0/anim->fps()*1000.0));
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
  if(!anim) return;
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
  Animation* anim=scene->getAnimation();
  if(!anim) return;
  int fps=anim->fps();

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
  // as setLoopInPoint(): no animation, nothing to set
  if(!anim) return;
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

  // Start only while stopped, Stop only while playing
  if(m_transport) m_transport->setPlaystate(state);
  // the notes line shows the task only while stopped (UX phase 3c)
  ui->timelineView->setPlaying(state!=PLAYSTATE_STOPPED);
  updateEditActions();
}

void PinglyMotionWindow::on_currentFrameSlider_valueChanged(int newValue)
{
  frameSlider(newValue);
}

// Set key, the timeline's main button (UX phase 3a): sets a key, never takes one away
void PinglyMotionWindow::on_keyframeButton_clicked()
{
  setKeyFromMenu();
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
