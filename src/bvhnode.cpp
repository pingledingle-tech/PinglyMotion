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

#include <algorithm>
#include <math.h>

#include "bvhnode.h"
#include "logging.h"
#include "easing.h"

BVHNode::BVHNode(const QString& name)
{
  setName(name);

  // clean out lists
  keyframes.clear();
  children.clear();

  setMirror(NULL,0);

  // every field gets a defined value; the readers set the real ones
  type=BVH_JOINT;
  numChannels=0;
  channelOrder=Rotation::Order::XYZ;
  for(int i=0;i<3;i++) offset[i]=0.0;
  for(int i=0;i<6;i++)
  {
    channelType[i]=BVH_XROT;
    channelMin[i]=-10000;
    channelMax[i]=10000;
  }
  ikOn=false;
  ikWeight=0.0;

  ikRotation.setX(0);
  ikRotation.setY(0);
  ikRotation.setZ(0);

  ikGoalPos[0]=0;
  ikGoalPos[1]=0;
  ikGoalPos[2]=0;

  ikGoalDir[0]=0;
  ikGoalDir[1]=1;
  ikGoalDir[2]=0;
}

BVHNode::~BVHNode()
{
}

const QString& BVHNode::name() const
{
  return m_name;
}

void BVHNode::setName(const QString& newName)
{
  m_name=newName;
}

int BVHNode::numChildren() const
{
  return children.count();
}

BVHNode* BVHNode::child(int num)
{
  return children.at(num);
}

void BVHNode::addChild(BVHNode* newChild)
{
  children.append(newChild);
}

void BVHNode::insertChild(BVHNode* newChild,int index)
{
  children.insert(index,newChild);
}

void BVHNode::removeChild(BVHNode* child)
{
  children.removeAll(child);
}

void BVHNode::addKeyframe(int frame, QVector3D position, QVector3D rotation)
{
  keyframes[frame] = FrameData(frame, position, rotation);
}

void BVHNode::setKeyframePosition(int frame, const QVector3D& position)
{
  if(!isKeyframe(frame)) qCWarning(lcAnimation,"setKeyframePosition(%d): not a keyframe!",frame);
  else
  {
    FrameData& key=keyframes[frame];
    key.setPosition(position);
  }
}

void BVHNode::setKeyframeRotation(int frame, const QVector3D& rotation)
{
  if(!isKeyframe(frame)) qCWarning(lcAnimation,"setKeyframeRotation(%d): not a keyframe!",frame);
  else
  {
    FrameData& key=keyframes[frame];
    key.setRotation(rotation);
  }
}

void BVHNode::deleteKeyframe(int frame)
{
  keyframes.remove(frame);
}

void BVHNode::insertFrame(int frame)
{
  // every key at this frame or later moves one frame further. Built as a new list: the old
  // loop copied from one fixed frame and erased the key its iterator stood on, which read freed
  // memory and mixed keys up when two or more keys came after the frame (found by job B1's test)
  QMap<int,FrameData> moved;
  for(auto it=keyframes.cbegin();it!=keyframes.cend();++it)
  {
    FrameData data=it.value();
    int number=it.key();
    if(number>=frame)
    {
      number++;
      data.setFrameNumber(number);
    }
    moved.insert(number,data);
  }
  keyframes=moved;
}

// delete a frame and move all keys back one frame
void BVHNode::deleteFrame(int frame)
{
  // a key on this frame goes, every later key moves one frame back (built as a new list,
  // for the same reason as insertFrame())
  QMap<int,FrameData> moved;
  for(auto it=keyframes.cbegin();it!=keyframes.cend();++it)
  {
    int number=it.key();
    if(number==frame) continue;
    FrameData data=it.value();
    if(number>frame)
    {
      number--;
      data.setFrameNumber(number);
    }
    moved.insert(number,data);
  }
  keyframes=moved;
}

bool BVHNode::isKeyframe(int frame) const
{
  return keyframes.contains(frame);
}

double BVHNode::interpolate(double from,double to,int steps,int pos,bool easeOut,bool easeIn) const
{
  // the start key's easeOut and the target key's easeIn pick one of the four
  // old-style easing choices; the easing module computes the value exactly as
  // this function always did (issue #53 step 2, docs/EASING.md)
  return Easing::segmentValue(Easing::presetFromSwitches(easeOut,easeIn),from,to,steps,pos);
}

double BVHNode::interpolate(double from,double to,int steps,int pos,Easing::Preset preset) const
{
  return Easing::segmentValue(preset,from,to,steps,pos);
}

QVector3D BVHNode::clampOvershoot(const QVector3D& rotation,const QVector3D& from,const QVector3D& to) const
{
  // the three rotation angles in the order of QVector3D: x, y, z
  const BVHChannelType axes[3]={BVH_XROT,BVH_YROT,BVH_ZROT};
  QVector3D result=rotation;
  for(int axis=0;axis<3;axis++)
  {
    for(int i=0;i<numChannels && i<6;i++)
    {
      if(channelType[i]!=axes[axis]) continue;
      double low=channelMin[i];
      double high=channelMax[i];
      // no limits loaded for this channel (BVHNode() and the readers set
      // -10000..10000, and SL.lim writes the same for a free joint)
      if(low<=-10000.0 && high>=10000.0) break;
      if(low>high) std::swap(low,high);
      // never tighter than the keys themselves
      low=std::min(low,std::min(static_cast<double>(from[axis]),static_cast<double>(to[axis])));
      high=std::max(high,std::max(static_cast<double>(from[axis]),static_cast<double>(to[axis])));
      const double value=result[axis];
      if(value<low) result[axis]=static_cast<float>(low);
      else if(value>high) result[axis]=static_cast<float>(high);
      break;
    }
  }
  return result;
}

const FrameData BVHNode::frameData(int frame) const
{
  // return empty frame data on end site nodes
  if(type==BVH_END) return FrameData();
  // if the keyframe exists, return the data
  if(isKeyframe(frame)) return keyframes[frame];

  // get keyframes before and after desired frame
  FrameData before=getKeyframeBefore(frame);
  FrameData after=getNextKeyframe(before.frameNumber());

  int frameBefore=before.frameNumber();
  int frameAfter=after.frameNumber();

  // if before and after frames are the same, there are no more keyframes left, so
  // we return the last keyframe data
  if(frameBefore==frameAfter) return before;

  QVector3D rotationBefore = before.rotation();
  QVector3D rotationAfter = after.rotation();
  QVector3D positionBefore = before.position();
  QVector3D positionAfter = after.position();

  QVector3D iRotation;
  QVector3D iPosition;

  const int steps=frameAfter-frameBefore;
  const int pos=frame-frameBefore;

  // a custom curve on the start key wins over everything (issue #11); it may
  // pass either pose, so the joint's limits stop the rotation as for the
  // overshoot choice. Positions are never clamped.
  if(const std::optional<Easing::Bezier> custom=before.customCurve())
  {
    iRotation.setX(Easing::customSegmentValue(*custom,rotationBefore.x(),rotationAfter.x(),steps,pos));
    iRotation.setY(Easing::customSegmentValue(*custom,rotationBefore.y(),rotationAfter.y(),steps,pos));
    iRotation.setZ(Easing::customSegmentValue(*custom,rotationBefore.z(),rotationAfter.z(),steps,pos));
    iRotation=clampOvershoot(iRotation,rotationBefore,rotationAfter);
    iPosition.setX(Easing::customSegmentValue(*custom,positionBefore.x(),positionAfter.x(),steps,pos));
    iPosition.setY(Easing::customSegmentValue(*custom,positionBefore.y(),positionAfter.y(),steps,pos));
    iPosition.setZ(Easing::customSegmentValue(*custom,positionBefore.z(),positionAfter.z(),steps,pos));
    return FrameData(frame, iPosition, iRotation);
  }

  // a choice stored on the start key wins, otherwise the two ease switches (issue #53)
  const Easing::Preset preset=Easing::segmentPreset(before.curve(),before.easeOut(),after.easeIn());

  iRotation.setX(interpolate(rotationBefore.x(), rotationAfter.x(), frameAfter - frameBefore, frame - frameBefore, preset));
  iRotation.setY(interpolate(rotationBefore.y(), rotationAfter.y(), frameAfter - frameBefore, frame - frameBefore, preset));
  iRotation.setZ(interpolate(rotationBefore.z(), rotationAfter.z(), frameAfter - frameBefore, frame - frameBefore, preset));

  // the overshoot may pass the target key; the joint's limits stop it there
  // (issue #53 proposal 4). The other choices never leave the span between
  // the keys, so they are not touched. Positions are never clamped.
  if(preset==Easing::Preset::OvershootThenSettle)
    iRotation=clampOvershoot(iRotation,rotationBefore,rotationAfter);

  iPosition.setX(interpolate(positionBefore.x(), positionAfter.x(), frameAfter - frameBefore, frame - frameBefore, preset));
  iPosition.setY(interpolate(positionBefore.y(), positionAfter.y(), frameAfter - frameBefore, frame - frameBefore, preset));
  iPosition.setZ(interpolate(positionBefore.z(), positionAfter.z(), frameAfter - frameBefore, frame - frameBefore, preset));


  // return interpolated frame data here
  return FrameData(frame, iPosition, iRotation);
}

const FrameData BVHNode::getKeyframeBefore(int frame) const
{
  if(frame<=0)
  {
    // should never happen
    qCDebug(lcAnimation,"getKeyframeBefore(int frame): frame==0!");
    return keyframes[0];
  }
  return frameData(getKeyframeNumberBefore(frame));
}

const FrameData BVHNode::getNextKeyframe(int frame) const
{
  QMap<int,FrameData>::const_iterator itCurrent=keyframes.find(frame);
  if(itCurrent != keyframes.cend())
  {
    ++itCurrent;
  }
  // if we are asked for a keyframe past the last one, return the last one
  if(itCurrent == keyframes.cend())
  {
    --itCurrent;
  }
  return (*itCurrent);
}

int BVHNode::getKeyframeNumberBefore(int frame) const
{
  // frame 0 is always a key frame; below it there is nothing to find
  if(frame<=0)
  {
    // should never happen
    qCDebug(lcAnimation,"getKeyframeNumberBefore(int frame): frame==0!");
    return 0;
  }

  // find previous key
  while(--frame && !isKeyframe(frame)) {};

  return frame;
}

int BVHNode::getKeyframeNumberAfter(int frame) const
{
  // get a list of all keyframe numbers
  QList<int> keys=keyframeList();

  // past the end? return -1
  if(frame>(int) keys[keyframes.count()-1])
    return -1;

  // find next key
  while(++frame && !isKeyframe(frame)) {};

  return frame;
}

const FrameData BVHNode::keyframeDataByIndex(int index) const
{
  // get a list of all keyframe numbers
  QList<int> keys=keyframeList();
  // get frame number of keyframe at given index
  int number=keys[index];
  // return keyframe data
  return keyframes[number];
}

const QList<int> BVHNode::keyframeList() const
{
  return keyframes.keys();
}

int BVHNode::numKeyframes() const
{
  return keyframes.count();
}

void BVHNode::setEaseIn(int frame,bool state)
{
  (*keyframes.find(frame)).setEaseIn(state);
}

void BVHNode::setEaseOut(int frame,bool state)
{
  (*keyframes.find(frame)).setEaseOut(state);
}

bool BVHNode::easeIn(int frame)
{
  if(keyframes.contains(frame))
    return (*keyframes.find(frame)).easeIn();

  qCWarning(lcAnimation,"easeIn(): asked on non-keyframe!");
  return false;
}

bool BVHNode::easeOut(int frame)
{
  if(keyframes.contains(frame))
    return (*keyframes.find(frame)).easeOut();

  qCWarning(lcAnimation,"easeOut(): asked on non-keyframe!");
  return false;
}

void BVHNode::setCurve(int frame,const std::optional<Easing::Preset>& curve)
{
  if(keyframes.contains(frame))
    (*keyframes.find(frame)).setCurve(curve);
}

std::optional<Easing::Preset> BVHNode::curve(int frame) const
{
  if(keyframes.contains(frame))
    return keyframes[frame].curve();
  return std::nullopt;
}

void BVHNode::setCustomCurve(int frame,const std::optional<Easing::Bezier>& curve)
{
  if(keyframes.contains(frame))
    (*keyframes.find(frame)).setCustomCurve(curve);
}

std::optional<Easing::Bezier> BVHNode::customCurve(int frame) const
{
  if(keyframes.contains(frame))
    return keyframes[frame].customCurve();
  return std::nullopt;
}

bool BVHNode::compareFrames(int key1,int key2) const
{
  if(type==BVH_POS)
  {
    const QVector3D position1 = frameData(key1).position();
    const QVector3D position2 = frameData(key2).position();

    if(position1.x() != position2.x()) return false;
    if(position1.y() != position2.y()) return false;
    if(position1.z() != position2.z()) return false;
  }
  else
  {
    const QVector3D rotation1 = frameData(key1).rotation();
    const QVector3D rotation2 = frameData(key2).rotation();

    if(rotation1.x() != rotation2.x()) return false;
    if(rotation1.y() != rotation2.y()) return false;
    if(rotation1.z() != rotation2.z()) return false;
  }

  return true;
}

void BVHNode::optimize()
{
  // PASS 1 - remove identical keyframes

  // get a list of all keyframe numbers
  QList<int> keys=keyframeList();
  QList<int> keysToDelete;

  // build a list of all identical keyframes to delete
  for(unsigned int i=1;i< (unsigned int) keys.count();i++)
  {
    // if we're comparing the last keyframe, it only makes sense to check for the one before
    if(i==(unsigned int) keys.count()-1)
    {
      if(compareFrames(keys[i],keys[i-1]))
        keysToDelete.append(keys[i]);
    }
    // otherwise check for the one before and the one after
    else if(compareFrames(keys[i],keys[i-1]) && compareFrames(keys[i],keys[i+1]))
      keysToDelete.append(keys[i]);
  }

  // delete keyframes on the delete list
  for(unsigned int i=0;i< (unsigned int) keysToDelete.count();i++)
    deleteKeyframe(keysToDelete[i]);

  // PASS 2 - remove keyframes that are superfluous due to linear interpolation

  QVector3D oldRotationDifference;
  QVector3D oldPositionDifference;

  // get first frame to compare - we even compare frame 1 here because we need
  // the initial "distance" and "difference" values. The first keyframe will
  // never be deleted, though
  QMap<int,FrameData>::const_iterator itBefore=keyframes.begin();

  if(itBefore==keyframes.end()) return;

  // make "current" frame one frame after "before" frame
  QMap<int,FrameData>::const_iterator itCurrent=itBefore;
  itCurrent++;

  if(itCurrent==keyframes.end()) return;

  // defines how much difference from anticipated change is acceptable for optimizing
  double tolerance=0.01;

  // loop as long as there are keyframes left
  while(itCurrent!=keyframes.end())
  {
    int distance=itCurrent.key()-itBefore.key();

    // optimize positions if this is the position node
    if(type==BVH_POS)
    {
      QVector3D positionDifference = (*itCurrent).position() - (*itBefore).position();

      positionDifference /= distance;

      if(fabs(positionDifference.x() - oldPositionDifference.x()) < tolerance &&
         fabs(positionDifference.y() - oldPositionDifference.y()) < tolerance &&
         fabs(positionDifference.z() - oldPositionDifference.z()) < tolerance)
      {
        // never delete the key in the first frame
        if(itBefore.key()!=0) keyframes.remove(itBefore.key());
      }

      oldPositionDifference = positionDifference;
    }
    // otherwise optimize rotations
    else
    {
      QVector3D rotationDifference = (*itCurrent).rotation() - (*itBefore).rotation();

      rotationDifference /= distance;

      if(fabs(rotationDifference.x() - oldRotationDifference.x()) < tolerance &&
         fabs(rotationDifference.y() - oldRotationDifference.y()) < tolerance &&
         fabs(rotationDifference.z() - oldRotationDifference.z()) < tolerance)
      {
          // never delete the key in the first frame
          if(itBefore.key()!=0) keyframes.remove(itBefore.key());
      }

      oldRotationDifference = rotationDifference;
    }

    itBefore=itCurrent;
    itCurrent++;
  } // while
}

BVHNode* BVHNode::getMirror() const
{
  return mirrorPart;
}

unsigned int BVHNode::getMirrorIndex() const
{
  return mirrorIndex;
}

void BVHNode::setMirror(BVHNode* mirror,unsigned int index)
{
  mirrorPart=mirror;
  mirrorIndex=index;
}

void BVHNode::mirrorKeys()
{
  QList<int> keys=keyframeList();
  for(unsigned int index=0;index< (unsigned int) keys.count();index++)
  {
    int frame=keys[index];
    if(type==BVH_POS)
    {
      QVector3D position = frameData(frame).position();
      position.setX(-position.x());
      setKeyframePosition(frame, position);
    }
    else
    {
      QVector3D rotation = frameData(frame).rotation();
      rotation.setY(-rotation.y());
      rotation.setZ(-rotation.z());
      setKeyframeRotation(frame, rotation);
    }
  }
}

void BVHNode::mirror()
{
  mirrorKeys();

  BVHNode* node2=getMirror();

  // if a mirror node is given, swap the keyframes, too
  if(node2)
  {
    node2->mirrorKeys();
    QMap<int,FrameData> temp=keyframes;
    keyframes=node2->keyframes;
    node2->keyframes=temp;
  }
}

// ************************************************************************

FrameData::FrameData()
{
  m_frameNumber=0;
  m_easeIn=false;
  m_easeOut=false;
}

FrameData::FrameData(int num, QVector3D position, QVector3D rotation)
{
  m_frameNumber=num;
  m_rotation = rotation;
  m_position = position;
  m_easeIn=false;
  m_easeOut=false;
}

bool FrameData::operator==(const FrameData& other) const
{
  return m_frameNumber==other.m_frameNumber && m_rotation==other.m_rotation && m_position==other.m_position &&
         m_easeIn==other.m_easeIn && m_easeOut==other.m_easeOut && m_curve==other.m_curve &&
         m_customCurve==other.m_customCurve;
}

int FrameData::frameNumber() const
{
  return m_frameNumber;
}

void FrameData::setFrameNumber(int frame)
{
  m_frameNumber=frame;
}

QVector3D FrameData::position() const
{
  return m_position;
}

QVector3D FrameData::rotation() const
{
  return m_rotation;
}

void FrameData::setEaseIn(bool state)
{
  m_easeIn=state;
}

void FrameData::setEaseOut(bool state)
{
  m_easeOut=state;
}

std::optional<Easing::Preset> FrameData::curve() const
{
  return m_curve;
}

void FrameData::setCurve(const std::optional<Easing::Preset>& curve)
{
  m_curve=curve;
}

std::optional<Easing::Bezier> FrameData::customCurve() const
{
  return m_customCurve;
}

void FrameData::setCustomCurve(const std::optional<Easing::Bezier>& curve)
{
  // stored inside the editor's limits, so playback, save and export agree
  if(curve) m_customCurve=Easing::sanitized(*curve);
  else m_customCurve.reset();
}

bool FrameData::easeIn() const
{
  return m_easeIn;
}

bool FrameData::easeOut() const
{
  return m_easeOut;
}

void FrameData::setPosition(const QVector3D& position)
{
  m_position = position;
}

void FrameData::setRotation(const QVector3D& rotation)
{
  m_rotation = rotation;
}

FrameData::~FrameData()
{
}
