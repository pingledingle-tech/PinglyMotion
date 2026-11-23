/*
 * PinglyMotion - copy a stretch of the timeline and paste it again (see timelineclip.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "timelineclip.h"

#include "animation.h"
#include "constants.h"
#include "menuspec.h"

int TimelineClip::trackOf(Animation* anim,const QString& name)
{
  if(!anim) return -1;
  for(int track=0;track<Constants::numParts();track++)
    if(anim->getPartName(track)==name) return track;
  return -1;
}

int TimelineClip::mirrorTrack(Animation* anim,int track)
{
  if(!anim || track<=0) return track;
  BVHNode* joint=anim->getNode(track);
  if(!joint || !joint->getMirror()) return track;
  const int partner=anim->getPartIndex(joint->getMirror());
  return partner>0 ? partner : track;
}

FrameData TimelineClip::mirrored(const FrameData& data,bool position)
{
  FrameData out=data;
  if(position)
  {
    QVector3D p=data.position();
    p.setX(-p.x());
    out.setPosition(p);
  }
  else
  {
    QVector3D r=data.rotation();
    r.setY(-r.y());
    r.setZ(-r.z());
    out.setRotation(r);
  }
  return out;
}

int TimelineClip::copy(Animation* anim,const TimelineRange& range)
{
  clear();
  if(!anim || range.isEmpty()) return 0;
  const int frames=anim->getNumberOfFrames();
  const int first=qBound(0,range.firstFrame,frames-1);
  const int last=qBound(first,range.lastFrame,frames-1);
  length_=last-first+1;

  for(int track : range.tracks)
  {
    BVHNode* node=anim->getNode(track);
    if(!node || node->type==BVH_END) continue;
    Joint joint;
    joint.name=anim->getPartName(track);
    joint.position=(node->type==BVH_POS);
    // the pose at the first frame, the keys inside, the pose at the last frame
    auto keep=[&](int frame)
    {
      Key key;
      key.offset=frame-first;
      key.data=node->frameData(frame);
      key.data.setFrameNumber(key.offset);
      joint.keys.append(key);
    };
    keep(first);
    const QList<int> keyFrames=node->keyframeList();
    for(int frame : keyFrames)
      if(frame>first && frame<last) keep(frame);
    if(last>first) keep(last);
    joints_.append(joint);
  }
  if(joints_.isEmpty()) length_=0;
  return keyCount();
}

int TimelineClip::removeKeys(Animation* anim,const TimelineRange& range)
{
  if(!anim || range.isEmpty()) return 0;
  int removed=0;
  for(int track : range.tracks)
  {
    BVHNode* node=anim->getNode(track);
    if(!node || node->type==BVH_END) continue;
    const QList<int> keyFrames=node->keyframeList();
    for(int frame : keyFrames)
    {
      // every joint keeps its key on frame 0
      if(frame<=0 || frame<range.firstFrame || frame>range.lastFrame) continue;
      node->deleteKeyframe(frame);
      removed++;
    }
    emit anim->redrawTrack(track);
  }
  if(removed)
  {
    anim->setDirty(true);
    emit anim->frameChanged();
  }
  return removed;
}

TimelineClip::PasteResult TimelineClip::paste(Animation* anim,int atFrame,bool mirror) const
{
  PasteResult result;
  if(!anim || isEmpty() || atFrame<0) return result;
  const int frames=anim->getNumberOfFrames();

  for(const Joint& joint : joints_)
  {
    int track=trackOf(anim,joint.name);
    if(track<0) continue;
    if(mirror) track=mirrorTrack(anim,track);
    BVHNode* node=anim->getNode(track);
    if(!node || node->type==BVH_END) continue;

    int set=0;
    for(const Key& key : joint.keys)
    {
      const int frame=atFrame+key.offset;
      if(frame>=frames)
      {
        result.leftOut++;
        continue;
      }
      const FrameData data=mirror ? mirrored(key.data,joint.position) : key.data;
      node->addKeyframe(frame,data.position(),data.rotation());
      node->setEaseIn(frame,data.easeIn());
      node->setEaseOut(frame,data.easeOut());
      node->setCurve(frame,data.curve());
      node->setCustomCurve(frame,data.customCurve());
      set++;
    }
    if(set)
    {
      result.keys+=set;
      if(!result.tracks.contains(track)) result.tracks.append(track);
      emit anim->redrawTrack(track);
    }
  }
  if(result.keys)
  {
    anim->setDirty(true);
    emit anim->frameChanged();
  }
  return result;
}

int TimelineClip::keyCount() const
{
  int count=0;
  for(const Joint& joint : joints_) count+=joint.keys.count();
  return count;
}

namespace ClipSpec
{
  QString text(const char* source)
  {
    return QCoreApplication::translate("TimelineView",source);
  }

  QString jointCount(int count)
  {
    return count==1 ? text(oneJoint) : text(joints).arg(count);
  }

  QString selectedMessage(const TimelineRange& range)
  {
    if(range.isEmpty()) return QString();
    return text(selectedRange).arg(range.firstFrame+1).arg(range.lastFrame+1).arg(jointCount(range.tracks.count()));
  }

  QString copyMessage(const TimelineRange& range,bool cut)
  {
    if(range.isEmpty()) return QString();
    const QString joints=jointCount(range.tracks.count());
    if(range.firstFrame==range.lastFrame)
      return text(cut ? cutFrame : copiedFrame).arg(range.firstFrame+1).arg(joints);
    return text(cut ? cutRange : copiedRange).arg(range.firstFrame+1).arg(range.lastFrame+1).arg(joints);
  }

  QString pasteMessage(const TimelineClip::PasteResult& result,int atFrame,bool mirrored)
  {
    QString message=text(mirrored ? pastedMirrored : pasted).arg(atFrame+1).arg(jointCount(result.tracks.count()));
    if(result.leftOut==1) message+=QLatin1Char(' ')+text(leftOutOne);
    else if(result.leftOut>1) message+=QLatin1Char(' ')+text(leftOutMany).arg(result.leftOut);
    return message;
  }

  QString pasteBlocked(bool animationOpen,bool clipEmpty,bool playing,bool firstFrameLocked,int frame)
  {
    auto menu=[](const char* source) { return QCoreApplication::translate("MenuSpec",source); };
    if(!animationOpen) return menu(MenuSpec::openFirst);
    if(clipEmpty) return text(copyFirst);
    if(playing) return menu(MenuSpec::setKeyStopPlaying);
    if(firstFrameLocked && frame==0) return menu(MenuSpec::setKeyLocked);
    return QString();
  }

  QString copyBlocked(bool animationOpen,bool rowsEmpty,bool playing)
  {
    auto menu=[](const char* source) { return QCoreApplication::translate("MenuSpec",source); };
    if(!animationOpen) return menu(MenuSpec::openFirst);
    if(playing) return menu(MenuSpec::setKeyStopPlaying);
    if(rowsEmpty) return text(nothingToCopy);
    return QString();
  }
}
