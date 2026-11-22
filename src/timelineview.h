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

#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

/**
  @author Zi Ree
*/

#include <QWidget>
#include <QFrame>

class QScrollArea;
class Timeline;
class Animation;
class EasingTiles;
class BVHNode;

class TimelineTracks : public QWidget
{
  Q_OBJECT

  public:
    TimelineTracks(QWidget* parent=0,Qt::WindowFlags f = Qt::WindowFlags());
    ~TimelineTracks();

    virtual QSize sizeHint() const;

    void drawTrack(int track);
    void setAnimation(Animation* anim);

    void selectTrack(int track);
    // the highlighted track name row, -1 when none
    int selectedTrackNumber() const { return selectedTrack; }

  protected:
    // nothing until the window sets an animation (setAnimation)
    Animation* animation=nullptr;
    virtual void paintEvent(QPaintEvent* event);

    int selectedTrack=-1;
};

class TimelineView : public QFrame
{
  Q_OBJECT

  public:
    TimelineView(QWidget* parent=0,Qt::WindowFlags f = Qt::WindowFlags());
    ~TimelineView();

    Timeline* getTimeline() const;
    // the easing tile row under the tracks (issue #53 step 5)
    EasingTiles* easingTiles() const;
    // the joint that is selected (highlighted in the names and in the rows), -1 when none
    int selectedTrack() const { return selectedTrack_; }
    TimelineTracks* tracks() const { return timelineTracks; }
    // the outer area that scrolls the track rows up and down
    QScrollArea* rowScroller() const { return outerView_; }

  public slots:
    void selectTrack(int track);
    void backgroundClicked();
    // joint groups were shown or hidden (JointGroups)
    void layoutChanged();
    void scrollTo(int x);

  protected slots:
    void setAnimation(Animation* anim);
    // "Custom curve" was clicked on the tile row: edit the pose change that
    // ends at the current frame of the selected joint (issue #11)
    void editCustomCurve();

  protected:
    // the text size changed: the rows follow it (Constants::lineHeight)
    void changeEvent(QEvent* event) override;

    QScrollArea* view;
    // the outer area that scrolls the track rows up and down
    QScrollArea* outerView_=nullptr;
    Timeline* timeline;
    TimelineTracks* timelineTracks;
    EasingTiles* easingTiles_;

  private:
    // the selected joint, the pose change ending at the current frame:
    // false when there is none
    bool currentSegment(BVHNode*& joint,int& startFrame,int& endFrame) const;

    Animation* animation_=nullptr;
    int selectedTrack_=-1;
};

#endif
