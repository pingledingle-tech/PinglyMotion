/*
 * PinglyMotion - which joints have a row in the timeline (see timelinerows.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "timelinerows.h"

#include <QtMath>

#include "animation.h"
#include "bvhnode.h"
#include "constants.h"
#include "menuspec.h"

namespace
{
  // turns and places closer than this count as the same (file rounding)
  constexpr double same=0.005;

  bool differs(const QVector3D& a,const QVector3D& b)
  {
    return qAbs(a.x()-b.x())>same || qAbs(a.y()-b.y())>same || qAbs(a.z()-b.z())>same;
  }
}

namespace TimelineSpec
{
  QString text(const char* source)
  {
    return QCoreApplication::translate(context,source);
  }

  QString areaName(int area)
  {
    return text(areas[(area>=0 && area<int(JointGroups::TAB_COUNT)) ? area : int(JointGroups::OtherTab)]);
  }

  bool addKey(Animation* anim,BVHNode* joint)
  {
    if(!anim || !joint) return false;
    if(joint->isKeyframe(anim->getFrame())) return false;
    anim->addKeyFrame(joint);
    return true;
  }

  QString setKeyMessage(bool added,int frame,const QString& jointName)
  {
    return added ? text(keySet).arg(frame+1).arg(jointName) : text(keyThere);
  }

  QString setKeyBlocked(bool animationOpen,bool jointPicked,bool playing,bool firstFrameLocked)
  {
    auto menu=[](const char* source) { return QCoreApplication::translate("MenuSpec",source); };
    if(!animationOpen) return menu(MenuSpec::openFirst);
    if(!jointPicked) return menu(MenuSpec::setKeyPickJoint);
    if(playing) return menu(MenuSpec::setKeyStopPlaying);
    if(firstFrameLocked) return menu(MenuSpec::setKeyLocked);
    return QString();
  }

  QString areaMessage(int area)
  {
    if(area<0 || area>=int(JointGroups::TAB_COUNT)) return text(showsAllAreas);
    return text(showsArea).arg(areaName(area));
  }

  QString changedOnlyMessage(bool changedOnly)
  {
    return text(changedOnly ? showsChanged : showsEvery);
  }

  QString areaEmptyTitle(int area)
  {
    if(area<0 || area>=int(JointGroups::TAB_COUNT)) return QString();
    return text(areaEmpty).arg(areaName(area));
  }

  QString areaKey(int area)
  {
    if(area<0 || area>=int(JointGroups::TAB_COUNT)) return QStringLiteral("all");
    return QLatin1String(areaKeys[area]);
  }

  int areaFromKey(const QString& key)
  {
    for(int area=0;area<int(JointGroups::TAB_COUNT);area++)
      if(key==QLatin1String(areaKeys[area])) return area;
    return -1;
  }
}

int TimelineRows::areaOf(const QString& name)
{
  // the Avatar Position ("position", JointList::POSITION_TRACK) is listed on the Body tab of the Joints panel
  if(name==QLatin1String("position")) return JointGroups::BodyTab;
  return JointGroups::tabOf(name);
}

bool TimelineRows::jointChanged(Animation* anim,int track)
{
  if(!anim || track<0) return false;
  if(anim->getPartName(track)==QLatin1String("Site")) return false;
  const BVHNode* joint=anim->getNode(track);
  if(!joint) return false;
  const int keys=joint->numKeyframes();
  if(keys==0) return false;

  const bool position=(track==0);
  const FrameData first=joint->keyframeDataByIndex(0);
  // the Avatar Position starts at the middle of the floor at the figure's own
  // height, which differs from figure to figure: a move away from the middle,
  // or a key in another place than the first key, counts
  if(position && (qAbs(first.position().x())>same || qAbs(first.position().z())>same)) return true;
  bool afterFirstFrame=false;
  for(int key=0;key<keys;key++)
  {
    const FrameData data=joint->keyframeDataByIndex(key);
    // a pose other than the start pose of a new animation
    if(position ? differs(data.position(),first.position()) : differs(data.rotation(),QVector3D()))
      return true;
    if(data.frameNumber()>0) afterFirstFrame=true;
  }
  // a key set by hand after the first frame; a joint keyed on every frame
  // (an imported file) that never leaves its start pose has not changed
  return afterFirstFrame && keys<anim->getNumberOfFrames();
}

void TimelineRows::clear()
{
  m_changed.clear();
  m_area.clear();
  m_listed.clear();
  m_rows.clear();
  m_rowOfTrack.clear();
}

bool TimelineRows::update(Animation* anim,int selectedTrack)
{
  const QVector<int> before=m_rows;
  m_selected=selectedTrack;
  m_changed.clear();
  m_area.clear();
  m_listed.clear();
  if(anim)
  {
    const int parts=Constants::numParts();
    for(int track=0;track<parts;track++)
    {
      const QString name=anim->getPartName(track);
      const bool site=(name==QLatin1String("Site"));
      m_changed << jointChanged(anim,track);
      m_area << areaOf(name);
      // End Sites and joints left out of the lists never have a row
      m_listed << (!site && JointGroups::rowOfTrack(track)>=0);
    }
  }
  build();
  return before!=m_rows;
}

bool TimelineRows::updateTrack(Animation* anim,int track)
{
  if(!anim || track<0 || track>=m_changed.count()) return false;
  const bool now=jointChanged(anim,track);
  if(now==m_changed[track]) return false;
  // the selected joint keeps its row, but "No keys yet" may come or go
  m_changed[track]=now;
  build();
  return true;
}

bool TimelineRows::setFilter(int area,bool allJoints)
{
  if(area<0 || area>=int(JointGroups::TAB_COUNT)) area=-1;
  if(area==m_filterArea && allJoints==m_allJoints) return false;
  m_filterArea=area;
  m_allJoints=allJoints;
  const QVector<int> before=m_rows;
  build();
  return before!=m_rows;
}

bool TimelineRows::areaHidesChanges() const
{
  if(m_filterArea<0 || m_allJoints || !anyChanged()) return false;
  for(int track : m_rows)
    if(m_changed[track]) return false;
  return true;
}

bool TimelineRows::setSelected(int selectedTrack)
{
  if(selectedTrack==m_selected) return false;
  m_selected=selectedTrack;
  const QVector<int> before=m_rows;
  build();
  return before!=m_rows;
}

void TimelineRows::build()
{
  m_rows.clear();
  m_rowOfTrack.fill(-1,m_changed.count());
  // the changed joints in the order of the skeleton, then the selected joint
  // when it has not changed (the mockup's editedTracks()); the filter (U2) cuts
  // them to one body area, or gives every joint a row
  for(int track=0;track<m_changed.count();track++)
  {
    if(!m_listed[track] || !(m_changed[track] || m_allJoints)) continue;
    if(m_filterArea>=0 && m_area[track]!=m_filterArea) continue;
    m_rowOfTrack[track]=m_rows.count();
    m_rows << track;
  }
  if(m_selected>=0 && m_selected<m_changed.count() && m_listed[m_selected] && m_rowOfTrack[m_selected]<0)
  {
    m_rowOfTrack[m_selected]=m_rows.count();
    m_rows << m_selected;
  }
}

int TimelineRows::rowOfTrack(int track) const
{
  return (track>=0 && track<m_rowOfTrack.count()) ? m_rowOfTrack[track] : -1;
}

int TimelineRows::trackOfRow(int row) const
{
  return (row>=0 && row<m_rows.count()) ? m_rows[row] : -1;
}

bool TimelineRows::anyChanged() const
{
  for(int track=0;track<m_changed.count();track++)
    if(m_changed[track] && m_listed[track]) return true;
  return false;
}

bool TimelineRows::changed(int track) const
{
  return track>=0 && track<m_changed.count() && m_changed[track];
}

int TimelineRows::areaOfTrack(int track) const
{
  return (track>=0 && track<m_area.count()) ? m_area[track] : int(JointGroups::OtherTab);
}
