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

#ifndef TIMELINE_H
#define TIMELINE_H

#include <QFrame>

#include "timelineclip.h"
#include "timelinerows.h"

/**
  @author Zi Ree
*/

class Animation;

class Timeline : public QFrame
{
  Q_OBJECT

  public:
    Timeline(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~Timeline();

    void setAnimation(Animation* anim);
    void selectTrack(int track);
    // the highlighted track, -1 when none
    int selectedTrack() const { return trackSelected; }
    // joint groups were shown or hidden (JointGroups)
    void layoutChanged();
    // the joints that have a row (UX phase 3a: changed joints and the selected one)
    const TimelineRows& rows() const { return rows_; }
    // the row filter (UX job U2, TimelineRows::setFilter): area -1 is every area
    void setRowFilter(int area,bool allJoints);

    // UX phase 3b: a stretch of the timeline (frames and rows) is selected
    // with Shift and a drag or click, or Shift and the arrow keys; Copy, Cut
    // and Paste mirrored use it. The corners are a joint and a frame each.
    bool hasSelection() const { return selAnchorTrack_>=0 && selEndTrack_>=0; }
    // the frames and the joints (in row order) between the two corners; empty
    // when nothing is selected or a corner's joint has no row any more
    TimelineRange selection() const;
    void setSelection(int anchorTrack,int anchorFrame,int endTrack,int endFrame);
    void clearSelection();
    // what Copy and Cut take: the selected stretch, or without one the current
    // frame on every joint that has a row
    TimelineRange copyRange() const;

  signals:
    void positionCenter(int pos);
    void trackClicked(int track);
    void resized(const QSize& newSize);
    void animationChanged(Animation* anim);
    void deleteFrame(int track,int frame);
    void insertFrame(int track,int frame);
    // other joints have a row now (a joint changed, or another one was selected)
    void rowsChanged();
    // the selected stretch changed (or went away)
    void selectionChanged();

  public slots:
    void setCurrentFrame(int frame);

  protected slots:
    void redrawTrack(int track);
    void redrawTrackImmediately(int track); // does an immediate repaint()
    void setNumberOfFrames(int frames);

  protected:
    virtual void paintEvent(QPaintEvent* event);
    void changeEvent(QEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void wheelEvent(QWheelEvent* e);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    void drawKeyframe(int track,int frame);
    void drawTrack(int track);
    bool isDirty(int frame);      // returns true if frame is inside "dirty" redraw region
    // the rows moved: draw them all again and tell the panel
    void rowsMoved();
    // as wide as the frames and as high as the rows
    void fitSize();
    // the body area of a joint's row (TimelineRows::areaOfTrack)
    int areaOf(int track) const { return rows_.areaOfTrack(track); }

    TimelineRows rows_;

    Animation* animation;

    int numOfFrames;
    int currentFrame;

    // keyframe drawing clip area
    int firstVisibleKeyX;
    int visibleKeysX;

    bool drawn;

    bool leftMouseButton;
    bool shift;

    bool fullRepaint;

    QPixmap* offscreen;

    int trackSelected;
    int frameSelected;
    int dragging;         // contains the current animation frame where dragging occurs

    // the selected stretch (UX phase 3b): corners as joint and frame, -1 when none
    int selAnchorTrack_=-1;
    int selAnchorFrame_=-1;
    int selEndTrack_=-1;
    int selEndFrame_=-1;
    // a Shift drag that selects is going on
    bool selecting_=false;
    // moves the corner that is not the anchor (starting a stretch at the
    // picked joint and the current frame when there is none)
    void extendSelection(int track,int frame);
};

#endif
