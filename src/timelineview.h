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

#include <functional>

class QLabel;
class QPushButton;
class QScrollArea;
class Timeline;
class Animation;
class EasingTiles;
class BVHNode;
class TimelineRows;
class NotesLine;

class TimelineTracks : public QWidget
{
  Q_OBJECT

  public:
    TimelineTracks(QWidget* parent=0,Qt::WindowFlags f = Qt::WindowFlags());
    ~TimelineTracks();

    virtual QSize sizeHint() const;

    void drawTrack(int track);
    void setAnimation(Animation* anim);
    // the rows to draw (the timeline's, UX phase 3a)
    void setRows(const TimelineRows* rows) { rows_=rows; }
    // the name shown for a joint (its friendly name, see TimelineView::setNameOf)
    void setNameOf(const std::function<QString(const QString&)>& nameOf) { nameOf_=nameOf; updateGeometry(); }
    QString shownName(int track) const;
    // as wide as the widest name with its area name, at least Constants::leftStrut
    int neededWidth() const;

    void selectTrack(int track);
    // the highlighted track name row, -1 when none
    int selectedTrackNumber() const { return selectedTrack; }

  protected:
    // nothing until the window sets an animation (setAnimation)
    Animation* animation=nullptr;
    virtual void paintEvent(QPaintEvent* event);

    int selectedTrack=-1;
    const TimelineRows* rows_=nullptr;
    std::function<QString(const QString&)> nameOf_;
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
    // UX phase 3a: the joints that have a row (the timeline's)
    const TimelineRows& rows() const;
    // the names shown for the joints: their friendly names (the window knows them)
    void setNameOf(const std::function<QString(const QString&)>& nameOf);
    // the empty states: "No keys yet" with "Set the first key", and the line
    // shown while no animation is open
    QWidget* noKeys() const { return noKeys_; }
    QLabel* noKeysTitle() const { return noKeysTitle_; }
    QLabel* noKeysLine() const { return noKeysLine_; }
    QPushButton* firstKeyButton() const { return firstKeyButton_; }
    QLabel* noAnimationLine() const { return noAnimation_; }
    // no animation is open (the window's empty state): the rows give way to one line
    void setNoAnimation(bool none);
    // Set the first key is greyed with this reason; empty: it can be clicked
    void setFirstKeyBlocked(const QString& why);
    // UX phase 3c: the notes line under the easing tiles, with the note of
    // the current frame (src/timelinenotes.*)
    NotesLine* notesLine() const { return notesLine_; }
    // Help > Show hints: the notes line shows only while it is on
    void setHintsOn(bool on);
    // the window plays or stops: stopped at the last frame the line shows the task
    void setPlaying(bool playing);

  signals:
    // "Set the first key" was clicked: the window sets a key like Set key
    void setKeyRequested();
    // Hide notes was clicked on the notes line
    void notesHidden();

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
    // other joints have a row now: sizes, the empty state, and the selected row in view
    void rowsChanged();
    void updateEmptyState();
    // scroll by the least amount that shows the selected joint's row
    void scrollToSelected();
    // the row area is as high as its rows; laid out at once, so scrolling knows the height
    void fitRowHeight();

    QWidget* noKeys_=nullptr;
    QLabel* noKeysTitle_=nullptr;
    QLabel* noKeysLine_=nullptr;
    QPushButton* firstKeyButton_=nullptr;
    QLabel* noAnimation_=nullptr;
    QWidget* rowsArea_=nullptr;
    NotesLine* notesLine_=nullptr;
    QMetaObject::Connection notesFrame_;
    bool playing_=false;
    // the notes of the open animation, its frame and frames, to the notes line
    void updateNotes();
    bool none_=false;

    // the selected joint, the pose change ending at the current frame:
    // false when there is none
    bool currentSegment(BVHNode*& joint,int& startFrame,int& endFrame) const;

    Animation* animation_=nullptr;
    int selectedTrack_=-1;
};

#endif
