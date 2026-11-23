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
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QPainter>

#include <algorithm>

#include "animation.h"
#include "bvhnode.h"
#include "constants.h"
#include "easingtiles.h"
#include "figures.h"
#include "jointgroups.h"
#include "timeline.h"
#include "timelinenotes.h"
#include "transportlook.h"

#include "timelineview.h"
#include "timelinelook.h"

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

  // the rows on top, the empty states under them (UX phase 3a)
  QWidget* content=new QWidget(outer);
  QVBoxLayout* contentColumn=new QVBoxLayout(content);
  contentColumn_=contentColumn;
  contentColumn->setContentsMargins(0,0,0,0);
  contentColumn->setSpacing(0);

  rowsArea_=new QWidget(content);
  QHBoxLayout* contentLayout=new QHBoxLayout(rowsArea_);
  contentLayout->setContentsMargins(0,0,0,0);

  timelineTracks=new TimelineTracks(rowsArea_);
  timeline=new Timeline(0);
  view=new QScrollArea(rowsArea_);

  view->setBackgroundRole(QPalette::Base);

  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  view->setWidget(timeline);
  timelineTracks->setRows(&timeline->rows());
  // tall enough for all rows, the outer area scrolls vertically
  fitRowHeight();

  timeline->setBackgroundRole(QPalette::Base);

  contentLayout->addWidget(timelineTracks,0);
  contentLayout->addWidget(view);
  contentColumn->addWidget(rowsArea_,0);

  // "No keys yet": the title, one line and Set the first key (the mockup's .tempty)
  noKeys_=new QWidget(content);
  noKeys_->setObjectName(QStringLiteral("timelineNoKeys"));
  QVBoxLayout* noKeysLayout=new QVBoxLayout(noKeys_);
  noKeysLayout->setContentsMargins(Theme::Size::spacingM,Theme::Size::spacingS,Theme::Size::spacingM,Theme::Size::spacingS);
  noKeysLayout->setSpacing(Theme::Size::spacingXs);
  noKeysTitle_=new QLabel(TimelineSpec::text(TimelineSpec::noKeysTitle),noKeys_);
  noKeysTitle_->setObjectName(QStringLiteral("timelineNoKeysTitle"));
  QFont titleFont=noKeysTitle_->font();
  titleFont.setWeight(QFont::DemiBold);
  noKeysTitle_->setFont(titleFont);
  noKeysLine_=new QLabel(TimelineSpec::text(TimelineSpec::noKeysLine),noKeys_);
  noKeysLine_->setObjectName(QStringLiteral("timelineNoKeysLine"));
  noKeysLine_->setForegroundRole(QPalette::PlaceholderText);
  // the button in a row of its own, drawn as the main button (src/transportlook.*)
  QWidget* buttonRow=new QWidget(noKeys_);
  QHBoxLayout* buttonLayout=new QHBoxLayout(buttonRow);
  buttonLayout->setContentsMargins(0,0,0,0);
  firstKeyButton_=new QPushButton(TimelineSpec::text(TimelineSpec::firstKey),buttonRow);
  firstKeyButton_->setObjectName(QStringLiteral("timelineFirstKeyButton"));
  buttonLayout->addWidget(firstKeyButton_);
  buttonLayout->addStretch(1);
  TransportLook::apply(buttonRow);
  noKeysLayout->addWidget(noKeysTitle_);
  noKeysLayout->addWidget(noKeysLine_);
  noKeysLayout->addWidget(buttonRow);
  contentColumn->addWidget(noKeys_,0);
  connect(firstKeyButton_,&QPushButton::clicked,this,&TimelineView::setKeyRequested);

  // no animation open: one line instead of the rows
  noAnimation_=new QLabel(TimelineSpec::text(TimelineSpec::noAnimation),content);
  noAnimation_->setObjectName(QStringLiteral("timelineNoAnimation"));
  noAnimation_->setWordWrap(true);
  noAnimation_->setForegroundRole(QPalette::PlaceholderText);
  noAnimation_->setContentsMargins(Theme::Size::spacingM,Theme::Size::spacingS,Theme::Size::spacingM,Theme::Size::spacingS);
  noAnimation_->hide();
  contentColumn->addWidget(noAnimation_,0);
  contentColumn->addStretch(1);

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
  connect(easingTiles_,&EasingTiles::editCurveRequested,this,&TimelineView::editCustomCurve);
  // why the row is greyed out (UX phase 5, the mockup's reasons): no animation,
  // no joint, or no key with a move before it
  easingTiles_->setNoKeyCheck([this]()
  {
    if(none_ || !animation_) return EasingTiles::NoKey::NoAnimation;
    if(selectedTrack_<0) return EasingTiles::NoKey::NoJoint;
    return EasingTiles::NoKey::NoMove;
  });

  // the notes line under the tiles (UX phase 3c, the mockup's .note)
  notesLine_=new NotesLine(this);
  layout->addWidget(notesLine_,0);
  connect(notesLine_,&NotesLine::hidden,this,&TimelineView::notesHidden);

  connect(timeline, &Timeline::animationChanged, this, &TimelineView::setAnimation);
  connect(timeline, &Timeline::trackClicked, this, &TimelineView::selectTrack);
  connect(timeline, &Timeline::rowsChanged, this, &TimelineView::rowsChanged);
  // the other figure's greyed keys move sideways with the active figure's keys
  connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() { syncFigureRows(); });
  updateEmptyState();
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
  timelineTracks->setAnimation(anim);
  animation_=anim;
  rowsChanged();

  // the notes line follows this animation's notes and frame
  disconnect(notesFrame_);
  notesLine_->setNotes(anim ? NotesSpec::fromExtensions(anim->extensions()) : NotesSpec::Notes());
  if(anim) notesFrame_=connect(anim,&Animation::currentFrame,this,[this]() { updateNotes(); });
  updateNotes();
}

void TimelineView::updateNotes()
{
  notesLine_->setAnimationOpen(animation_ && !none_);
  if(animation_) notesLine_->setFrame(animation_->getFrame(),animation_->getNumberOfFrames(),playing_);
}

void TimelineView::setHintsOn(bool on)
{
  notesLine_->setHintsOn(on);
}

void TimelineView::setPlaying(bool playing)
{
  playing_=playing;
  updateNotes();
}

const TimelineRows& TimelineView::rows() const
{
  return timeline->rows();
}

void TimelineView::setNameOf(const std::function<QString(const QString&)>& nameOf)
{
  nameOf_=nameOf;
  timelineTracks->setNameOf(nameOf);
  timelineTracks->update();
  for(FigureRows* rows : figureRows_) if(rows) rows->setNameOf(nameOf);
}

void TimelineView::setFigures(const QList<Animation*>& animations,const QStringList& paths,int active,bool expert)
{
  // the old parts go after this event (a header may be the sender of the click that got here)
  for(FigureHeader* header : figureHeaders_)
  {
    header->hide();
    header->deleteLater();
  }
  for(FigureRows* rows : figureRows_)
  {
    if(!rows) continue;
    rows->hide();
    rows->deleteLater();
  }
  figureHeaders_.clear();
  figureRows_.clear();

  contentColumn_->removeWidget(rowsArea_);
  contentColumn_->removeWidget(noKeys_);
  contentColumn_->removeWidget(noAnimation_);
  int at=0;
  const int count=int(std::min(animations.size(),paths.size()));
  if(count<2)
  {
    // one figure: exactly as before
    contentColumn_->insertWidget(at++,rowsArea_,0);
    contentColumn_->insertWidget(at++,noKeys_,0);
  }
  else
  {
    QWidget* content=outerView_->widget();
    for(int i=0;i<count;i++)
    {
      FigureHeader* header=new FigureHeader(content);
      header->setFigure(i+1,paths.at(i),i==active);
      header->setExpert(expert);
      connect(header,&FigureHeader::makeActiveClicked,this,[this,i]() { emit makeActiveRequested(i); });
      connect(header,&FigureHeader::removeClicked,this,[this,i]() { emit removeRequested(i); });
      figureHeaders_ << header;
      contentColumn_->insertWidget(at++,header,0);
      if(i==active)
      {
        figureRows_ << nullptr;
        contentColumn_->insertWidget(at++,rowsArea_,0);
        contentColumn_->insertWidget(at++,noKeys_,0);
        continue;
      }
      FigureRows* rows=new FigureRows(content);
      rows->setNameOf(nameOf_);
      rows->setAnimation(animations.at(i),i+1);
      connect(rows,&FigureRows::rowClicked,this,[this,i](int track) { emit otherJointClicked(i,track); });
      figureRows_ << rows;
      contentColumn_->insertWidget(at++,rows,0);
    }
  }
  contentColumn_->insertWidget(at++,noAnimation_,0);
  syncFigureRows();
  updateEmptyState();
  fitRowHeight();
}

void TimelineView::setFiguresExpert(bool expert)
{
  for(FigureHeader* header : figureHeaders_) header->setExpert(expert);
}

void TimelineView::syncFigureRows()
{
  const int namesWidth=timelineTracks->width();
  const int scroll=view->horizontalScrollBar()->value();
  for(FigureRows* rows : figureRows_)
  {
    if(!rows) continue;
    rows->setNamesWidth(namesWidth);
    rows->setScrollX(scroll);
  }
}

void TimelineView::fitRowHeight()
{
  const int rows=timeline->rows().rowCount();
  // no rows: no row area at all, the empty state stands on top
  view->setMinimumHeight(rows ? rows*Constants::lineHeight()+2 : 0);
  view->setMaximumHeight(rows ? QWIDGETSIZE_MAX : 0);
  timelineTracks->updateGeometry();
  if(outerView_ && outerView_->widget())
  {
    // lay the content out now, so the scroll bar knows the new height at once
    // (selectTrack scrolls to a row that was only just added)
    QWidget* content=outerView_->widget();
    rowsArea_->layout()->invalidate();
    rowsArea_->layout()->activate();
    rowsArea_->updateGeometry();
    content->layout()->invalidate();
    content->layout()->activate();
    const int height=std::max(outerView_->viewport()->height(),content->minimumSizeHint().height());
    content->resize(outerView_->viewport()->width(),height);
  }
}

void TimelineView::rowsChanged()
{
  fitRowHeight();
  syncFigureRows();
  timelineTracks->update();
  updateEmptyState();
  scrollToSelected();
}

void TimelineView::updateEmptyState()
{
  const bool noKeys=!none_ && animation_ && !timeline->rows().anyChanged();
  noKeys_->setVisible(noKeys);
  noAnimation_->setVisible(none_);
  rowsArea_->setVisible(!none_);
  if(outerView_ && outerView_->widget()) outerView_->widget()->layout()->activate();
  easingTiles_->refreshNoKey();
}

void TimelineView::setNoAnimation(bool none)
{
  none_=none;
  updateEmptyState();
  fitRowHeight();
  updateNotes();
}

void TimelineView::setFirstKeyBlocked(const QString& why)
{
  firstKeyButton_->setEnabled(why.isEmpty());
  firstKeyButton_->setToolTip(why);
  firstKeyButton_->setStatusTip(why);
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
  easingTiles_->refreshNoKey();
  timelineTracks->selectTrack(track);
  // a joint without keys gets its row here (rowsChanged follows)
  timeline->selectTrack(track);
  scrollToSelected();
}

void TimelineView::scrollToSelected()
{
  // scroll the row into view; a joint without a row (hidden group) is left alone
  const int track=selectedTrack_;
  const int row=(track<0) ? -1 : timeline->rows().rowOfTrack(track);
  if(row>=0 && outerView_)
  {
    // scroll by the least amount that shows the whole row; a row already
    // in view does not move the timeline
    // with two figures a header row (and the other figure's rows) may stand above
    const int top=rowsArea_->y()+row*Constants::lineHeight()+2;
    const int bottom=top+Constants::lineHeight();
    QScrollBar* bar=outerView_->verticalScrollBar();
    if(top<bar->value())
      bar->setValue(row==0 ? 0 : top);
    else if(bottom>bar->value()+bar->pageStep())
      bar->setValue(bottom-bar->pageStep());
  }
}

void TimelineView::layoutChanged()
{
  timeline->layoutChanged();
  fitRowHeight();
  timelineTracks->update();
  updateEmptyState();
}

void TimelineView::changeEvent(QEvent* event)
{
  QFrame::changeEvent(event);
  if(event->type()==QEvent::FontChange) layoutChanged();
}

void TimelineView::backgroundClicked()
{
  selectedTrack_=-1;
  easingTiles_->refreshNoKey();
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
  const int rows=rows_ ? rows_->rowCount() : 0;
  // no rows: no height (the row area is closed, the empty state stands on top)
  return QSize(neededWidth(), rows ? rows * Constants::lineHeight() + 2 : 0);
}

QString TimelineTracks::shownName(int track) const
{
  if(!animation) return QString();
  const QString name=animation->getPartName(track);
  if(nameOf_)
  {
    const QString shown=nameOf_(name);
    if(!shown.isEmpty()) return shown;
  }
  return name;
}

// the stripe, a gap, the name, a gap, the area name and a gap on the right
int TimelineTracks::neededWidth() const
{
  int widest=0;
  if(rows_ && animation)
  {
    const QFontMetrics metrics(font());
    const int gap=Theme::Size::spacingS;
    for(int track : rows_->tracks())
    {
      const int width=TimelineLook::areaStripeWidth()+gap+metrics.horizontalAdvance(shownName(track))
                      +gap+metrics.horizontalAdvance(TimelineSpec::areaName(rows_->areaOfTrack(track)))+gap;
      widest=std::max(widest,width);
    }
  }
  return std::max(Constants::leftStrut(),widest);
}

void TimelineTracks::paintEvent(QPaintEvent*)
{
  if(size()!=sizeHint()) resize(sizeHint());
  if(!animation || !rows_) return;
  {
    QPainter p(this);
    p.fillRect(rect(),TimelineLook::row(false,Theme::isDark()));
  }

  for (int track : rows_->tracks())
  {
    drawTrack(track);
  }
}

void TimelineTracks::drawTrack(int track)
{
  if(track==-1 || !rows_) return;

  const int row=rows_->rowOfTrack(track);
  if(row<0) return;
  QPainter p(this);
  // colours from the design tokens (src/timelinelook.h): the area colour as a
  // stripe and a tint, joints of hidden groups dimmed
  const bool dark=Theme::isDark();
  const bool selected=(track==selectedTrack);
  const int area=rows_->areaOfTrack(track);
  const int gap=Theme::Size::spacingS;

  int y = row * Constants::lineHeight() + 2;

  p.fillRect(0, y, width(), Constants::lineHeight(), TimelineLook::nameRow(selected,area,dark));
  p.fillRect(0, y, TimelineLook::areaStripeWidth(), Constants::lineHeight(), TimelineLook::area(area,dark));

  // the area's name on the right, the joint's name on the left
  const QFontMetrics metrics(font());
  const QString areaName=TimelineSpec::areaName(area);
  const int areaWidth=metrics.horizontalAdvance(areaName);
  const int nameLeft=TimelineLook::areaStripeWidth()+gap;
  const QRect line(0, y, width(), Constants::lineHeight());
  p.setPen(TimelineLook::areaName(selected,dark));
  p.drawText(QRect(width()-gap-areaWidth, y, areaWidth, Constants::lineHeight()), Qt::AlignVCenter|Qt::AlignLeft, areaName);
  p.setPen(TimelineLook::name(selected,JointGroups::trackDimmed(track),dark));
  p.drawText(QRect(nameLeft, line.top(), width()-nameLeft-gap-areaWidth-gap, line.height()), Qt::AlignVCenter|Qt::AlignLeft, shownName(track));
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
