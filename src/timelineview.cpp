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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QPainter>

#include "animation.h"
#include "bvhnode.h"
#include "constants.h"
#include "easingtiles.h"
#include "jointgroups.h"
#include "timeline.h"

#include "timelineview.h"

// rows shown in the timeline: joints of hidden groups may be left out
static int timelineRows()
{
  const int rows=JointGroups::rowCount();
  return rows>0 ? rows : Constants::numParts();
}

// scroll area that asks for the same height the timeline had with the
// classic skeleton, so the 3D view keeps its size
class TrackScrollArea : public QScrollArea
{
  public:
    explicit TrackScrollArea(QWidget* parent) : QScrollArea(parent) {}
    QSize sizeHint() const override
    {
      return QSize(QScrollArea::sizeHint().width(), 24 * Constants::lineHeight() + 2);
    }
};

TimelineView::TimelineView(QWidget* parent,Qt::WindowFlags f) : QFrame(parent,f)
{
  // the tracks above, the easing tile row below (issue #53 step 5)
  QVBoxLayout* layout=new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);

  // Bento has far more tracks than fit on screen, so track names and key
  // frames scroll up and down together in an outer scroll area
  QScrollArea* outer=new TrackScrollArea(this);
  outer->setWidgetResizable(true);
  outer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  outer->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  outer->setFrameShape(QFrame::NoFrame);
  outerView_=outer;

  QWidget* content=new QWidget(outer);
  QHBoxLayout* contentLayout=new QHBoxLayout(content);
  contentLayout->setContentsMargins(0,0,0,0);

  timelineTracks=new TimelineTracks(content);
  timeline=new Timeline(0);
  view=new QScrollArea(content);

  view->setBackgroundRole(QPalette::Dark);

  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  view->setWidget(timeline);
  // tall enough for all tracks, the outer area scrolls vertically
  view->setMinimumHeight((timelineRows() + 1) * Constants::lineHeight());

  timeline->setBackgroundRole(QPalette::Base);

  contentLayout->addWidget(timelineTracks,0);
  contentLayout->addWidget(view);
  outer->setWidget(content);

  layout->addWidget(outer,1);

  easingTiles_=new EasingTiles(this);
  easingTiles_->setObjectName(QStringLiteral("easingTiles"));
  layout->addWidget(easingTiles_,0);
  // the custom curve (issue #11) is handled here, where the selected joint and
  // the animation are known
  easingTiles_->setCustomCheck([this]()
  {
    BVHNode* joint=nullptr;
    int startFrame=0,endFrame=0;
    return currentSegment(joint,startFrame,endFrame) && EasingChoice::customCurveOf(joint,startFrame).has_value();
  });
  connect(easingTiles_,&EasingTiles::customCurveRequested,this,&TimelineView::editCustomCurve);

  connect(timeline, &Timeline::animationChanged, this, &TimelineView::setAnimation);
  connect(timeline, &Timeline::trackClicked, this, &TimelineView::selectTrack);
}

TimelineView::~TimelineView()
{
}

// absolute x pixel position
void TimelineView::scrollTo(int x)
{
  QSize size=view->size();
  view->ensureVisible(x,0,size.width()/4,size.height());
}

void TimelineView::setAnimation(Animation* anim)
{
  // the number of tracks is known once the skeleton is loaded
  view->setMinimumHeight((timelineRows() + 1) * Constants::lineHeight());
  timelineTracks->setAnimation(anim);
  animation_=anim;
}

bool TimelineView::currentSegment(BVHNode*& joint,int& startFrame,int& endFrame) const
{
  if(!animation_ || selectedTrack_<0) return false;
  joint=animation_->getNode(selectedTrack_);
  endFrame=animation_->getFrame();
  return joint && EasingChoice::segmentEndingAt(joint,endFrame,startFrame);
}

void TimelineView::editCustomCurve()
{
  BVHNode* joint=nullptr;
  int startFrame=0,endFrame=0;
  if(!currentSegment(joint,startFrame,endFrame)) return;

  CurveEditorDialog dialog(EasingChoice::editorStart(joint,startFrame,endFrame),this);
  if(dialog.exec()!=QDialog::Accepted) return;

  EasingChoice::setCustomCurve(animation_,joint,startFrame,endFrame,dialog.curve());
  // show the new state on the row (the check above marks it as custom)
  easingTiles_->setChoice(EasingChoice::choiceOf(joint,startFrame,endFrame));
}

Timeline* TimelineView::getTimeline() const
{
  return timeline;
}

EasingTiles* TimelineView::easingTiles() const
{
  return easingTiles_;
}

void TimelineView::selectTrack(int track)
{
  selectedTrack_=track;
  timeline->selectTrack(track);
  timelineTracks->selectTrack(track);

  // scroll the row into view; a joint without a row (hidden group) is left alone
  const int row=(track<0) ? -1 : JointGroups::rowOfTrack(track);
  if(row>=0 && outerView_)
  {
    const int y=row*Constants::lineHeight()+2;
    outerView_->ensureVisible(0,y+Constants::lineHeight()/2,0,Constants::lineHeight()/2+2);
  }
}

void TimelineView::layoutChanged()
{
  view->setMinimumHeight((timelineRows() + 1) * Constants::lineHeight());
  timeline->layoutChanged();
  timelineTracks->updateGeometry();
  timelineTracks->update();
}

void TimelineView::backgroundClicked()
{
  selectedTrack_=-1;
  timeline->selectTrack(-1);
  timelineTracks->selectTrack(-1);
}

// --------------

TimelineTracks::TimelineTracks(QWidget* parent,Qt::WindowFlags f) : QWidget(parent,f)
{
  resize(sizeHint());
}

TimelineTracks::~TimelineTracks()
{
  selectedTrack=-1;
}

QSize TimelineTracks::sizeHint() const
{
  return QSize(Constants::leftStrut(), (timelineRows() - 1) * Constants::lineHeight() + 2);
}

void TimelineTracks::paintEvent(QPaintEvent*)
{
  resize(sizeHint());
  if(!animation) return;
  {
    QPainter p(this);
    p.eraseRect(rect());
  }

  for (int part = 0; part < Constants::numParts(); part++)
  {
    drawTrack(part);
  }
}

void TimelineTracks::drawTrack(int track)
{
  if(track==-1) return;

  QString trackName=(animation) ? animation->getPartName(track) : "";
  if(trackName!="Site")
  {
    const int row=JointGroups::rowOfTrack(track);
    if(row<0) return;
    QPainter p(this);
    QPalette::ColorRole textColor=QPalette::WindowText;
    // joints of hidden groups are listed in a dimmed colour
    const QPalette::ColorGroup colorGroup=JointGroups::trackDimmed(track) ? QPalette::Disabled : QPalette::Active;

    int y = row * Constants::lineHeight() + 2;

    if(track==selectedTrack)
    {
      p.fillRect(0, y, width(), Constants::lineHeight(), palette().color(QPalette::Active, QPalette::Highlight));
#ifdef Q_OS_WIN32
      // on windows systems use contrast color to track highlight color
      textColor=QPalette::HighlightedText;
#endif
    }
    else
      p.eraseRect(0, y, width(), Constants::lineHeight());

    // draw track name
    p.setPen(palette().color(colorGroup,textColor));
    p.drawText(0, y + Constants::keyHeight(), trackName);
  }
}

void TimelineTracks::setAnimation(Animation* anim)
{
  animation=anim;
}

void TimelineTracks::selectTrack(int track)
{
  selectedTrack=track;
  repaint();
}
