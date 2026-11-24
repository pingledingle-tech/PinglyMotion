/***************************************************************************
 *   Copyright (C) 2006 by Zi Ree   *
 *   Zi Ree @ SecondLife   *
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

#include <QPainter>
#include <QPaintEvent>

#include "animation.h"
#include "logging.h"
#include "constants.h"
#include "jointgroups.h"

#include "timeline.h"
#include "timelinelook.h"

Timeline::Timeline(QWidget* parent, Qt::WindowFlags)
  : QFrame(parent)
{

  // init offscreen pixmap as 0 so the first change of number of frames will not
  // try to copy old pixmap data
  offscreen=0;

  resize(50,50);

  fullRepaint=false;

  animation=0;
  // tracks can be drawn before the first paint event sets these
  numOfFrames=0;
  firstVisibleKeyX=0;
  visibleKeysX=0;
  frameSelected=0;
  drawn=false;
  setCurrentFrame(0);
  setWindowTitle(tr("Timeline"));

  leftMouseButton=false;
  shift=false;

  dragging=0;
  trackSelected=-1;

  setFocusPolicy(Qt::StrongFocus);
}

Timeline::~Timeline()
{
}

// the set of shown rows changed (joint groups shown or hidden, or the theme):
// work the rows out again and draw everything again from a clean pixmap
void Timeline::layoutChanged()
{
  const bool moved=rows_.update(animation,trackSelected);
  fitSize();
  if(offscreen) offscreen->fill(TimelineLook::row(false,Theme::isDark()));
  fullRepaint=true;
  update();
  if(moved) emit rowsChanged();
}

void Timeline::setRowFilter(int area,bool allJoints)
{
  if(rows_.filterArea()==area && rows_.allJoints()==allJoints) return;
  rows_.setFilter(area,allJoints);
  // a selected stretch may lose rows: it goes, as with another animation
  clearSelection();
  fitSize();
  if(offscreen) offscreen->fill(TimelineLook::row(false,Theme::isDark()));
  fullRepaint=true;
  update();
  // also when the rows stayed the same: the line over them may change
  emit rowsChanged();
}

void Timeline::fitSize()
{
  // as wide as the frames, as high as the rows (no rows: no height)
  const QSize newSize(numOfFrames * Constants::keyWidth(), rows_.rowCount() * Constants::lineHeight());
  if(newSize!=size())
  {
    resize(newSize);
    emit resized(newSize);
  }
}

void Timeline::rowsMoved()
{
  fitSize();
  if(offscreen) offscreen->fill(TimelineLook::row(false,Theme::isDark()));
  fullRepaint=true;
  update();
  emit rowsChanged();
}

void Timeline::paintEvent(QPaintEvent* /* e */)
{
  if(!animation) return;
  if(isHidden()) return;


  fitSize();
  // clip keyframe drawing to "dirty" area of redraw
  firstVisibleKeyX=0;
  visibleKeysX=numOfFrames;

  // full repaint only rarely needed
  if(fullRepaint)
  {
    // clip keyframe drawing to "dirty" area of redraw
    firstVisibleKeyX=0;
    visibleKeysX=numOfFrames;

    for (int part = 0; part < Constants::numParts(); part++)
    {
      drawTrack(part);
    } // for
    fullRepaint=false;
  }

  // reset keyframe clipping to have new or updated transition lines painted properly
  firstVisibleKeyX=0;
  visibleKeysX=numOfFrames;

  int xpos = frameSelected * Constants::keyWidth();

  QPainter p(this);
  p.drawPixmap(0,0,*offscreen);

  // the selected stretch (UX phase 3b): a box in the selection colour over its frames and rows
  const TimelineRange range=selection();
  if(!range.isEmpty())
  {
    int top=rows_.rowCount();
    int bottom=-1;
    for(int track : range.tracks)
    {
      top=qMin(top,rows_.rowOfTrack(track));
      bottom=qMax(bottom,rows_.rowOfTrack(track));
    }
    const QRect box(range.firstFrame*Constants::keyWidth(),top*Constants::lineHeight(),
                    range.frameCount()*Constants::keyWidth(),(bottom-top+1)*Constants::lineHeight());
    p.fillRect(box,TimelineLook::selectionBox(Theme::isDark()));
    p.setPen(TimelineLook::selectionBoxBorder(Theme::isDark()));
    p.setBrush(Qt::NoBrush);
    p.drawRect(box.adjusted(0,0,-1,-1));
  }

  // draw current frame marker
  p.fillRect(xpos + Constants::keyWidth() / 2 - 1, 0, 2, height(), TimelineLook::currentFrame(Theme::isDark()));

}

// the theme was switched: the rows are kept in a pixmap, so draw them all again
void Timeline::changeEvent(QEvent* event)
{
  QFrame::changeEvent(event);
  if(event->type()==QEvent::ApplicationPaletteChange || event->type()==QEvent::PaletteChange) layoutChanged();
  // the text size changed: the rows are taller or lower now (Constants::lineHeight)
  else if(event->type()==QEvent::FontChange && offscreen) setNumberOfFrames(numOfFrames);
}

void Timeline::setNumberOfFrames(int frames)
{

  numOfFrames=frames;
  // a joint keyed on every frame counts as imported, so the length matters
  const bool moved=rows_.update(animation,trackSelected);

  QPixmap* newOffscreen = new QPixmap(numOfFrames * Constants::keyWidth(), Constants::lineHeight() * (Constants::numParts() + 1));
  newOffscreen->fill(TimelineLook::row(false,Theme::isDark()));

  // copy old offscreen pixmap to new pixmap, if there is one
  if(offscreen)
  {
    QPainter p(newOffscreen);
    p.drawPixmap(0,0,*offscreen);
    delete offscreen;
  }

  offscreen=newOffscreen;

  fitSize();
  fullRepaint=true;
  repaint();
  if(moved) emit rowsChanged();
}

void Timeline::setCurrentFrame(int frame)
{
  frameSelected=frame;
  emit positionCenter(frameSelected * Constants::keyWidth());
}

void Timeline::setAnimation(Animation* anim)
{
  // don't do anything if this is the same animation as before
  if(anim==animation) return;

  if(animation)
  {
    disconnect(animation, &Animation::numberOfFrames, this, &Timeline::setNumberOfFrames);
    disconnect(animation, &Animation::redrawTrack, this, &Timeline::redrawTrackImmediately);
    disconnect(this, &Timeline::deleteFrame, animation, &Animation::deleteFrame);
    disconnect(this, &Timeline::insertFrame, animation, &Animation::insertFrame);
  }
  animation=anim;
  // a stretch of another animation means nothing here
  clearSelection();

  if(animation)
  {
    connect(animation, &Animation::numberOfFrames, this, &Timeline::setNumberOfFrames);
    connect(animation, &Animation::redrawTrack, this, &Timeline::redrawTrackImmediately);
    connect(this, &Timeline::deleteFrame, animation, &Animation::deleteFrame);
    connect(this, &Timeline::insertFrame, animation, &Animation::insertFrame);
    // as long as this animation, also when another one was shown before (two figures,
    // docs/UX_DECISIONS.md 6m): before, only the window's frame count field told it
    setNumberOfFrames(animation->getNumberOfFrames());
  }

  rows_.update(animation,trackSelected);
  fitSize();
  if(offscreen) offscreen->fill(TimelineLook::row(false,Theme::isDark()));
  emit animationChanged(anim);

  fullRepaint=true;
  repaint();
  emit rowsChanged();
}

void Timeline::redrawTrackImmediately(int track)
{
  // a joint that changed (or went back to its start pose) gets (or loses) its row
  if(rows_.updateTrack(animation,track))
  {
    rowsMoved();
    return;
  }
  drawTrack(track);
  update();
}

void Timeline::redrawTrack(int track)
{
  drawTrack(track);
}

void Timeline::drawKeyframe(int track,int frame)
{

  if(track==-1) return;
  const int row=rows_.rowOfTrack(track);
  if(row<0) return;

  int ypos = row * Constants::lineHeight();

  QPainter p(offscreen);
  // calculate x position
  int xpos = frame * Constants::keyWidth();

  // text colour, on_accent on the selected row (the token pair that has contrast)
  const bool dark=Theme::isDark();
  QColor color(TimelineLook::key(trackSelected==track && animation->getPartName(track)!="Site",areaOf(track),dark));
  // draw the key frame
  if(frame==0)
  {
    p.fillRect(xpos, ypos, Constants::keyWidth() / 2, Constants::keyHeight(), QBrush(color));
    p.fillRect(xpos, ypos + Constants::lineHeight() / 2 - 2, Constants::keyWidth(), 4, QBrush(color));
  }
  else if(frame==(numOfFrames-1))
  {
    p.fillRect(xpos + Constants::keyWidth() / 2, ypos, Constants::keyWidth() / 2, Constants::keyHeight(), QBrush(color));
    p.fillRect(xpos, ypos + Constants::lineHeight() / 2 - 2, Constants::keyWidth(), 4, QBrush(color));
  }
  else
  {
    // find out if we should paint in highlight or normal color
    if(frame && dragging==frame && trackSelected==track) color=TimelineLook::keyDragged(dark);

    // get the list of key frames
    const BVHNode* joint=animation->getNode(track);

    bool ps=animation->compareFrames(joint,joint->getKeyframeBefore(frame).frameNumber(),frame);
    bool ns=animation->compareFrames(joint,joint->getNextKeyframe(frame).frameNumber(),frame);

    p.setPen(color);
    p.setBrush(QBrush(color));

    if(ps && ns)
      p.drawEllipse(xpos, ypos, Constants::keyWidth() - 1, Constants::lineHeight() - 1);

    else if(ns)
    {
      QPolygon triangle(3);
      triangle.putPoints(0, 3, xpos, ypos + (Constants::lineHeight() - 1) / 2, xpos + Constants::keyWidth() - 1, ypos, xpos + Constants::keyWidth() - 1, ypos + Constants::lineHeight() - 1);

      p.drawPolygon(triangle);
    }
    else if(ps)
    {
      QPolygon triangle(3);
      triangle.putPoints(0, 3, xpos + Constants::keyWidth() - 1, ypos + (Constants::lineHeight() - 1) / 2, xpos, ypos, xpos, ypos + Constants::lineHeight() - 1);

      p.drawPolygon(triangle);
    }
    else
      p.fillRect(xpos, ypos, Constants::keyWidth() - 1, Constants::keyHeight(), QBrush(color));
  }
}

void Timeline::selectTrack(int track)
{
  int oldTrack=trackSelected;
  trackSelected=track;
  // the selected joint always has a row: a joint without keys gets one at the end
  if(rows_.setSelected(track))
  {
    rowsMoved();
    repaint();
    return;
  }
  // if the user clicked on a different track
  if(oldTrack!=trackSelected)
  {
    // draw old track with old color
    drawTrack(oldTrack);
    // draw new track
    drawTrack(trackSelected);
  }
  repaint();
}

void Timeline::drawTrack(int track)
{
  if(track==-1) return;
  if(!offscreen) return;

  // selected tracks get the selection colour, unless it's a "Site" track (end of limbs group);
  // every colour comes from the design tokens (src/timelinelook.h)
  const bool dark=Theme::isDark();
  QString trackName=animation->getPartName(track);
  const bool selected=(track==trackSelected && trackName!="Site");

  const int row=rows_.rowOfTrack(track);
  if(row<0) return;
  const int area=areaOf(track);

  int light=0;
  int y = row * Constants::lineHeight();

  // temporary painter, must be destroyed before entering main keyframe loop,
  // or we clash with drawKeyframe() painter
  QPainter* p=new QPainter(offscreen);

  // find widget coordinates of clipped area and decide if we paint light or dark background
  int firstGreyShade=(firstVisibleKeyX/10);
  light=firstGreyShade % 2;
  firstGreyShade *= Constants::keyWidth() * 10;

  // draw alternatingly light / dark background
  for(int x = firstGreyShade; x < (firstGreyShade + visibleKeysX + 10) * Constants::keyWidth(); x += 10 * Constants::keyWidth())
  {
    // draw a filled rectangle in the row colour, every other block toward the background
    p->fillRect(x, y, x + 10 * Constants::keyWidth(), Constants::lineHeight(), light ? TimelineLook::rowAlternate(selected,area,dark) : TimelineLook::row(selected,area,dark));
    light=1-light;
  }

  // if this is a "Site end" marker (end of group of limbs) do nothing more
  if(trackName=="Site")
  {
    delete p;
    return;
  }

  // draw straight line as track marker within "dirty" redraw region
  p->fillRect(firstVisibleKeyX * Constants::keyWidth(), y + Constants::lineHeight() / 2, (visibleKeysX + 1) * Constants::keyWidth(), 1, TimelineLook::midLine(selected,area,dark));
  p->fillRect(firstVisibleKeyX * Constants::keyWidth(), y + Constants::lineHeight() / 2 + 1, (visibleKeysX + 1) * Constants::keyWidth(), 1, TimelineLook::midLineShade(selected,area,dark));

  delete p;

  // get the list of key frames
  const BVHNode* joint=animation->getNode(track);
  // start drawing rest of key frames
  if(joint->numKeyframes())
  {
    // reset previous frame marker
    int oldFrame=0;
    // iterate through list of key frames
    for(int key=0;key<joint->numKeyframes();key++)
    {
      // get frame number of key frame
      const FrameData& frameData=joint->keyframeDataByIndex(key);
      int frameNum=frameData.frameNumber();

      // if key frame is not out of animation length
      if(frameNum<numOfFrames)
      {
        // if this key frame is not at the first animation frame
        if(frameNum>0)
        {
          // create new painter here, will be out of scope before drawKeyframe() gets called
          QPainter painter(offscreen);
          // check if it differs from the previous frame, if it does, draw a line there
          // but only if this line is inside the "dirty" redraw region
          if(
              (frameNum>firstVisibleKeyX && oldFrame<(firstVisibleKeyX+visibleKeysX)) &&
              !animation->compareFrames(joint,frameNum,oldFrame)
            )
          {
            const FrameData& oldFrameData=joint->keyframeDataByIndex(key-1);
            int frameDiff = (frameNum - oldFrame + 1) * Constants::keyWidth() / 2;

            QColor pen1(TimelineLook::key(selected,area,dark));
            QColor pen2(TimelineLook::midLineShade(selected,area,dark));

            if(oldFrameData.easeOut())
            {
              QPoint oldPos(oldFrame * Constants::keyWidth() + Constants::keyWidth() - 1, y + Constants::lineHeight() / 2);
              for (int x = 0; x < frameDiff - Constants::keyWidth() + 1; x++)
              {
                QPoint newPos(x + oldFrame * Constants::keyWidth() + Constants::keyWidth() - 1,
                              (int)(y + sin(x * 6.283 / (frameDiff - Constants::keyWidth() + 1)) * (Constants::lineHeight() / 2 - 1) + Constants::lineHeight() / 2)
                             );
                painter.setPen(pen1);
                painter.drawLine(oldPos,newPos);
                painter.setPen(pen2);
                painter.drawLine(oldPos+QPoint(0,1),newPos+QPoint(0,1));
                oldPos=newPos;
              }
            }
            else
            {
              // we use fillRect instead of drawLine because for some reason the windows
              // version did not draw properly
              painter.fillRect(oldFrame * Constants::keyWidth() + Constants::keyWidth() - 1,
                         y + Constants::keyHeight() / 2,
                         frameDiff - Constants::keyWidth(),
                         1, QBrush(pen1));
              painter.fillRect(oldFrame * Constants::keyWidth() + Constants::keyWidth() - 1,
                         y + Constants::keyHeight() / 2 + 1,
                         frameDiff,
                         1, QBrush(pen2));
            }

            if(frameData.easeIn())
            {
              QPoint oldPos(oldFrame * Constants::keyWidth() + frameDiff, y + Constants::lineHeight() / 2);
              for (int x = 0; x < frameDiff - Constants::keyWidth() + 1; x++)
              {
                QPoint newPos(x + oldFrame * Constants::keyWidth() + frameDiff - 1,
                              (int)(y + sin(x * 6.283 / (frameDiff - Constants::keyWidth() + 1)) * (Constants::lineHeight() / 2 - 1) + Constants::lineHeight() / 2)
                             );
                painter.setPen(pen1);
                painter.drawLine(oldPos,newPos);
                painter.setPen(pen2);
                painter.drawLine(oldPos+QPoint(0,1),newPos+QPoint(0,1));
                oldPos=newPos;
              }
            }
            else
            {
              painter.fillRect(oldFrame * Constants::keyWidth() + frameDiff - 1,
                         y + Constants::keyHeight() / 2,
                         frameDiff,
                         1, QBrush(pen1));
              painter.fillRect(oldFrame * Constants::keyWidth() + frameDiff - 1,
                         y + Constants::keyHeight() / 2 + 1,
                         frameDiff,
                         1, QBrush(pen2));
            }
          }
        }
        // only draw keyframes if they are in "dirty" clipping region
        if(isDirty(frameNum))
          // draw the key frame
          drawKeyframe(track,frameNum);
      }
      // remember this frame number as previous key frame
      oldFrame=frameNum;
    } // for

    // last frame is always a key frame, so draw it if it's inside the clipping region
    if(isDirty(numOfFrames-1)) drawKeyframe(track,numOfFrames-1);
  }
}

// returns true if frame is inside "dirty" redraw region
bool Timeline::isDirty(int frame)
{
  return (frame>=firstVisibleKeyX && frame<=(firstVisibleKeyX+visibleKeysX));
}

void Timeline::mousePressEvent(QMouseEvent* e)
{
  // get track and frame based on mouse coordinates
  const int track=rows_.trackOfRow(e->position().toPoint().y() / Constants::lineHeight());
  if(track<0) return;
  const int frame=qBound(0,int(e->position().toPoint().x() / Constants::keyWidth()),qMax(0,numOfFrames-1));

  // UX phase 3b: Shift and a press on an empty place selects a stretch (a
  // second Shift click moves its far corner); Shift on a key still copies the
  // key by dragging it, as before
  const bool onKey=frame>0 && frame<(numOfFrames-1) && animation->isKeyFrame(track,frame);
  if((e->modifiers() & Qt::ShiftModifier) && !onKey && e->button()==Qt::LeftButton)
  {
    if(hasSelection()) extendSelection(track,frame);
    else setSelection(track,frame,track,frame);
    frameSelected=frame;
    animation->setFrame(frame);
    selecting_=true;
    leftMouseButton=true;
    return;
  }
  // a plain click leaves the stretch
  clearSelection();

  selectTrack(track);
  frameSelected = e->position().toPoint().x() / Constants::keyWidth();

  // set animation frame to where we clicked
  animation->setFrame(frameSelected);
  // check if we clicked on a key frame which is not the first or last one in animation
  if(frameSelected>0 && frameSelected<(numOfFrames-1) && animation->isKeyFrame(trackSelected,frameSelected))
  {
    // first switch on dragging state, so drawKeyframe() will work as expected
    dragging=frameSelected;
    // highlight keyframe
    drawKeyframe(trackSelected,frameSelected);
  }
  emit trackClicked(trackSelected);
  repaint();
  // remember mouse button state
  leftMouseButton=true;
}

void Timeline::mouseReleaseEvent(QMouseEvent*)
{
  leftMouseButton=false;
  selecting_=false;
  if(dragging)
  {
    // first switch off dragging state, so drawKeyframe() will work as expected
    dragging=0;
    // remove highlight from keyframe
    drawKeyframe(trackSelected,frameSelected);
    repaint();
  }
}

void Timeline::mouseMoveEvent(QMouseEvent* e)
{
  // calculate new position (in frames)
  int frame = e->position().toPoint().x() / Constants::keyWidth();

  // a Shift drag that selects: the far corner follows the pointer, kept inside the rows and frames
  if(selecting_)
  {
    if(rows_.rowCount()==0 || numOfFrames<=0) return;
    const int row=qBound(0,int(e->position().toPoint().y() / Constants::lineHeight()),rows_.rowCount()-1);
    extendSelection(rows_.trackOfRow(row),frame);
    return;
  }

  // if frame position has not changed, do nothing
  if(frame==frameSelected) return;

  // special treatment if user is dragging a keyframe
  if(dragging)
  {
    // if user is dragging a keyframe, move it within more restrictive bounds
    if(frame>0 && frame<(numOfFrames-1))
    {
      // update dragging position
      dragging=frame;

      // make sure we don't overwrite already existing frames
      if(!animation->isKeyFrame(trackSelected,frame))
      {
        // check if the user holds the shift key
        if(shift)
        {
          // reset shift key status so we don't constantly create copies
          shift=false;
          // copy selected frame to new place
          animation->copyKeyFrame(trackSelected,frameSelected,frame);
        }
        else
          // move selected frame to new place
          animation->moveKeyFrame(trackSelected,frameSelected,frame);

        redrawTrack(trackSelected);
        // remember new position
        frameSelected=frame;
      }
    }
  }
  // no dragging so check if new position would be out of bounds
  else if(frame>=0 && frame<numOfFrames)
  {
    // remember new position
    frameSelected=frame;
    // set the new position
    animation->setFrame(frame);
  }
}

void Timeline::wheelEvent(QWheelEvent* e)
{
  // get curernt frame position
  int newFrame=frameSelected;

  // check if we should move forward or backwards
  if(e->angleDelta().y()>0) newFrame-=5;
  else newFrame+=5;

  // check if new position would be out of bounds
  if(newFrame>=numOfFrames) newFrame=numOfFrames-1;
  else if(newFrame<0) newFrame=0;

  // remember new position
  frameSelected=newFrame;
  // set the new position
  animation->setFrame(newFrame);
}

void Timeline::keyPressEvent(QKeyEvent* e)
{
  // UX phase 3b: Shift and the arrow keys grow or shrink the selected stretch
  // (it starts at the picked joint and the current frame); Escape leaves it
  if(e->modifiers() & Qt::ShiftModifier)
  {
    const int key=e->key();
    if(key==Qt::Key_Left || key==Qt::Key_Right || key==Qt::Key_Up || key==Qt::Key_Down)
    {
      int track=hasSelection() ? selEndTrack_ : trackSelected;
      if(!animation) { e->ignore(); return; }
      int frame=hasSelection() ? selEndFrame_ : animation->getFrame();
      if(rows_.rowOfTrack(track)<0) { e->accept(); return; }
      if(key==Qt::Key_Left) frame--;
      else if(key==Qt::Key_Right) frame++;
      else
      {
        const int next=rows_.trackOfRow(rows_.rowOfTrack(track)+(key==Qt::Key_Down ? 1 : -1));
        if(next>=0) track=next;
      }
      extendSelection(track,frame);
      e->accept();
      return;
    }
  }
  if(e->key()==Qt::Key_Escape && hasSelection())
  {
    clearSelection();
    e->accept();
    return;
  }

  switch(e->key())
  {
    case Qt::Key_Shift:
      shift=true;
      break;

    case Qt::Key_Delete:
      emit deleteFrame(trackSelected,frameSelected);

      if(trackSelected!=-1)
        drawTrack(trackSelected);
      else
        fullRepaint=true;

      repaint();
      break;

    // the row above or below becomes the selected joint (as a click on it)
    case Qt::Key_Up:
    case Qt::Key_Down:
    {
      int row=rows_.rowOfTrack(trackSelected);
      if(row<0) row=(e->key()==Qt::Key_Down) ? -1 : rows_.rowCount();
      row+=(e->key()==Qt::Key_Down) ? 1 : -1;
      const int track=rows_.trackOfRow(row);
      if(track<0) break;
      selectTrack(track);
      emit trackClicked(track);
      break;
    }

    case Qt::Key_Insert:
      emit insertFrame(trackSelected,frameSelected);

      if(trackSelected!=-1)
        drawTrack(trackSelected);
      else
        fullRepaint=true;

      repaint();
      break;

    default:
      e->ignore();
      return;
  }
  e->accept();
}

void Timeline::keyReleaseEvent(QKeyEvent* e)
{
  switch(e->key())
  {
    case Qt::Key_Shift:
      shift=false;
      break;
    default:
      e->ignore();
      return;
  }
  e->accept();
}

TimelineRange Timeline::selection() const
{
  TimelineRange range;
  if(!hasSelection() || numOfFrames<=0) return range;
  const int rowA=rows_.rowOfTrack(selAnchorTrack_);
  const int rowB=rows_.rowOfTrack(selEndTrack_);
  if(rowA<0 || rowB<0) return range;
  range.firstFrame=qBound(0,qMin(selAnchorFrame_,selEndFrame_),numOfFrames-1);
  range.lastFrame=qBound(0,qMax(selAnchorFrame_,selEndFrame_),numOfFrames-1);
  for(int row=qMin(rowA,rowB);row<=qMax(rowA,rowB);row++)
  {
    const int track=rows_.trackOfRow(row);
    if(track>=0) range.tracks.append(track);
  }
  return range;
}

void Timeline::setSelection(int anchorTrack,int anchorFrame,int endTrack,int endFrame)
{
  selAnchorTrack_=anchorTrack;
  selAnchorFrame_=anchorFrame;
  selEndTrack_=endTrack;
  selEndFrame_=endFrame;
  update();
  emit selectionChanged();
}

void Timeline::clearSelection()
{
  selecting_=false;
  if(!hasSelection()) return;
  selAnchorTrack_=selAnchorFrame_=selEndTrack_=selEndFrame_=-1;
  update();
  emit selectionChanged();
}

void Timeline::extendSelection(int track,int frame)
{
  if(!animation || numOfFrames<=0 || track<0) return;
  frame=qBound(0,frame,numOfFrames-1);
  if(!hasSelection())
  {
    // start at the picked joint (when it has a row) and the current frame
    const int anchor=(rows_.rowOfTrack(trackSelected)>=0) ? trackSelected : track;
    selAnchorTrack_=anchor;
    selAnchorFrame_=qBound(0,animation->getFrame(),numOfFrames-1);
  }
  if(track==selEndTrack_ && frame==selEndFrame_) return;
  selEndTrack_=track;
  selEndFrame_=frame;
  // the current frame follows the far corner
  frameSelected=frame;
  animation->setFrame(frame);
  update();
  emit selectionChanged();
}

TimelineRange Timeline::copyRange() const
{
  TimelineRange range=selection();
  if(!range.isEmpty() || !animation || numOfFrames<=0) return range;
  range.firstFrame=range.lastFrame=qBound(0,animation->getFrame(),numOfFrames-1);
  range.tracks=rows_.tracks().toList();
  return range;
}
