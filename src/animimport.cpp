/*
 * PinglyMotion – read a Second Life .anim file (issue #22, step 1)
 *
 * See animimport.h and docs/ANIM_FORMAT.md. The reader never searches for a
 * string end outside the bytes that are left, and every count is checked
 * against the bytes that are left before anything is allocated for it.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animimport.h"

#include <QObject>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace AnimImport
{

namespace
{
  // the decoding of the viewer (llmath/llquantize.h): zero comes through as zero
  float u16ToFloat(quint16 value,float lower,float upper)
  {
    const float oneOver=1.0f/65535.0f;
    float temp=static_cast<float>(value);
    temp*=oneOver;
    temp*=(upper-lower);
    temp+=lower;
    if(std::fabs(temp)<(upper-lower)*oneOver) temp=0.0f;
    return temp;
  }

  class Cursor
  {
  public:
    explicit Cursor(const QByteArray& d) : data(d) {}
    qsizetype left() const { return data.size()-pos; }
    bool has(qsizetype n) const { return n>=0 && n<=left(); }

    bool u8(quint8& v) { if(!has(1)) return false; v=static_cast<quint8>(data[pos]); pos+=1; return true; }
    bool u16(quint16& v) { if(!has(2)) return false; v=qFromLittleEndian<quint16>(data.constData()+pos); pos+=2; return true; }
    bool s32(qint32& v) { if(!has(4)) return false; v=qFromLittleEndian<qint32>(data.constData()+pos); pos+=4; return true; }
    bool u32(quint32& v) { if(!has(4)) return false; v=qFromLittleEndian<quint32>(data.constData()+pos); pos+=4; return true; }
    bool f32(float& v)
    {
      quint32 bits;
      if(!u32(bits)) return false;
      std::memcpy(&v,&bits,4);
      return true;
    }

    // a zero-terminated string; the end is only searched inside the bytes that are left
    bool string(QString& s,int maxBytes,bool& tooLong)
    {
      tooLong=false;
      const qsizetype limit=std::min<qsizetype>(left(),static_cast<qsizetype>(maxBytes)+1);
      qsizetype i=0;
      while(i<limit && data[pos+i]!='\0') i++;
      if(i>=limit) { tooLong=left()>maxBytes; return false; }
      s=QString::fromLatin1(data.constData()+pos,i);
      pos+=i+1;
      return true;
    }

    // a fixed field of n bytes read as a string that ends at the first zero byte
    bool fixedString(QString& s,int n)
    {
      if(!has(n)) return false;
      qsizetype len=0;
      while(len<n && data[pos+len]!='\0') len++;
      s=QString::fromLatin1(data.constData()+pos,len);
      pos+=n;
      return true;
    }

    qsizetype pos=0;
  private:
    const QByteArray& data;
  };

  // sorted by time; keys at the same time are one key and the later one in the file wins,
  // as in the viewer (it keeps them in a map)
  template<class Key> void sortAndMerge(QVector<Key>& keys)
  {
    std::stable_sort(keys.begin(),keys.end(),[](const Key& a,const Key& b){ return a.time<b.time; });
    QVector<Key> merged;
    merged.reserve(keys.size());
    for(const Key& k : keys)
    {
      if(!merged.isEmpty() && merged.last().time==k.time) merged.last()=k;
      else merged.push_back(k);
    }
    keys=merged;
  }

  bool finite(float v) { return std::isfinite(v); }

  Result fail(const QString& message)
  {
    Result r;
    r.error=message;
    return r;
  }

  QString cutOff() { return QObject::tr("The file is cut off or damaged."); }
}

Result parse(const QByteArray& data)
{
  if(data.isEmpty()) return fail(QObject::tr("The file is empty."));
  if(data.size()>MAX_FILE_BYTES) return fail(QObject::tr("The file is too big for an animation."));

  Cursor in(data);
  Motion m;
  QStringList warnings;

  quint16 version=0,subVersion=0;
  if(!in.u16(version) || !in.u16(subVersion)) return fail(cutOff());
  if(version==0 && subVersion==1)
    return fail(QObject::tr("This is an old animation format (version 0.1) that is not supported."));
  if(version!=1 || subVersion!=0)
    return fail(QObject::tr("This is not a Second Life animation file (version %1.%2).").arg(version).arg(subVersion));

  qint32 basePriority=0;
  float duration=0.0f;
  if(!in.s32(basePriority) || !in.f32(duration)) return fail(cutOff());
  if(basePriority<-1) return fail(QObject::tr("The animation has a priority that is not valid."));
  if(basePriority<0) basePriority=0;
  if(basePriority>MAX_PRIORITY)
  {
    warnings << QObject::tr("The priority %1 was changed to %2.").arg(basePriority).arg(MAX_PRIORITY);
    basePriority=MAX_PRIORITY;
  }
  m.basePriority=basePriority;
  if(!finite(duration) || !(duration>0.0f) || duration>MAX_DURATION)
    return fail(QObject::tr("The animation length is not valid. It must be above 0 and at most %1 seconds.").arg(MAX_DURATION,0,'f',0));
  m.duration=duration;

  bool tooLong=false;
  if(!in.string(m.emote,MAX_NAME_BYTES,tooLong))
    return fail(tooLong ? QObject::tr("The emote name is too long.") : cutOff());

  qint32 loop=0;
  float loopIn=0,loopOut=0,easeIn=0,easeOut=0;
  quint32 handPose=0,numJoints=0;
  if(!in.f32(loopIn) || !in.f32(loopOut) || !in.s32(loop) || !in.f32(easeIn) || !in.f32(easeOut) ||
     !in.u32(handPose) || !in.u32(numJoints))
    return fail(cutOff());

  if(!finite(loopIn) || !finite(loopOut) || !finite(easeIn) || !finite(easeOut))
    return fail(QObject::tr("The loop or ease times are not valid numbers."));
  m.loop=loop!=0;
  if(loopIn<0.0f || loopOut>duration || loopIn>loopOut)
  {
    warnings << QObject::tr("The loop points were moved inside the animation.");
    loopOut=std::min(std::max(loopOut,0.0f),duration);
    loopIn=std::min(std::max(loopIn,0.0f),loopOut);
  }
  m.loopIn=loopIn; m.loopOut=loopOut;
  if(easeIn<0.0f || easeOut<0.0f)
  {
    warnings << QObject::tr("A negative ease time was changed to 0.");
    easeIn=std::max(easeIn,0.0f); easeOut=std::max(easeOut,0.0f);
  }
  m.easeIn=easeIn; m.easeOut=easeOut;
  if(handPose>static_cast<quint32>(MAX_HAND_POSE))
    return fail(QObject::tr("The hand pose %1 is not valid.").arg(handPose));
  m.handPose=static_cast<int>(handPose);

  if(numJoints==0 || numJoints>static_cast<quint32>(MAX_JOINTS))
    return fail(QObject::tr("The animation has %1 joints. It must have between 1 and %2.").arg(numJoints).arg(MAX_JOINTS));

  // a joint is at least a one-letter name, its end, priority and two key counts
  if(!in.has(static_cast<qsizetype>(numJoints)*14)) return fail(cutOff());

  QStringList names;
  for(quint32 ji=0;ji<numJoints;ji++)
  {
    Joint j;
    if(!in.string(j.name,MAX_NAME_BYTES,tooLong))
      return fail(tooLong ? QObject::tr("A joint name is too long.") : cutOff());
    if(j.name.isEmpty()) return fail(QObject::tr("A joint has no name."));
    if(j.name=="mRoot" || j.name=="mScreen")
      return fail(QObject::tr("The joint %1 cannot be animated.").arg(j.name));
    if(names.contains(j.name)) return fail(QObject::tr("The joint %1 is in the file twice.").arg(j.name));
    names << j.name;

    qint32 priority=0;
    if(!in.s32(priority)) return fail(cutOff());
    if(priority<-1) return fail(QObject::tr("The joint %1 has a priority that is not valid.").arg(j.name));
    if(priority>MAX_PRIORITY)
    {
      warnings << QObject::tr("The priority of %1 was changed to %2.").arg(j.name).arg(MAX_PRIORITY);
      priority=MAX_PRIORITY;
    }
    j.priority=priority;

    qint32 numRot=0;
    if(!in.s32(numRot)) return fail(cutOff());
    if(numRot<0 || !in.has(static_cast<qsizetype>(numRot)*8))
      return fail(numRot<0 ? QObject::tr("The joint %1 has a key count that is not valid.").arg(j.name) : cutOff());
    j.rotations.reserve(numRot);
    for(qint32 k=0;k<numRot;k++)
    {
      quint16 t,x,y,z;
      in.u16(t); in.u16(x); in.u16(y); in.u16(z);
      RotationKey key;
      key.time=u16ToFloat(t,0.0f,duration);
      key.x=u16ToFloat(x,-1.0f,1.0f);
      key.y=u16ToFloat(y,-1.0f,1.0f);
      key.z=u16ToFloat(z,-1.0f,1.0f);
      const float w2=1.0f-key.x*key.x-key.y*key.y-key.z*key.z;
      key.w=w2>0.0f ? std::sqrt(w2) : 0.0f;
      j.rotations.push_back(key);
    }
    sortAndMerge(j.rotations);
    if(j.rotations.size()!=numRot)
      warnings << QObject::tr("The joint %1 had keys at the same time; only the last of each was kept.").arg(j.name);

    qint32 numPos=0;
    if(!in.s32(numPos)) return fail(cutOff());
    if(numPos<0 || !in.has(static_cast<qsizetype>(numPos)*8))
      return fail(numPos<0 ? QObject::tr("The joint %1 has a key count that is not valid.").arg(j.name) : cutOff());
    j.positions.reserve(numPos);
    const float p=static_cast<float>(MAX_POSITION);
    for(qint32 k=0;k<numPos;k++)
    {
      quint16 t,x,y,z;
      in.u16(t); in.u16(x); in.u16(y); in.u16(z);
      PositionKey key;
      key.time=u16ToFloat(t,0.0f,duration);
      key.x=u16ToFloat(x,-p,p);
      key.y=u16ToFloat(y,-p,p);
      key.z=u16ToFloat(z,-p,p);
      j.positions.push_back(key);
    }
    sortAndMerge(j.positions);
    if(j.positions.size()!=numPos)
      warnings << QObject::tr("The joint %1 had position keys at the same time; only the last of each was kept.").arg(j.name);

    m.joints.push_back(j);
  }

  // constraints: a bad count means "ignore them" in the viewer, and the file is still good
  qint32 numConstraints=0;
  if(!in.s32(numConstraints)) return fail(cutOff());
  if(numConstraints<0 || numConstraints>MAX_CONSTRAINTS)
  {
    warnings << QObject::tr("The constraints in the file were ignored.");
  }
  else
  {
    for(qint32 c=0;c<numConstraints;c++)
    {
      Constraint k;
      quint8 chain=0,type=0;
      if(!in.u8(chain) || !in.u8(type)) return fail(cutOff());
      if(chain>numJoints) return fail(QObject::tr("A constraint is not valid."));
      if(type>1) return fail(QObject::tr("A constraint is not valid."));
      k.chainLength=chain; k.type=type;
      if(!in.fixedString(k.sourceVolume,16)) return fail(cutOff());
      for(int i=0;i<3;i++) if(!in.f32(k.sourceOffset[i])) return fail(cutOff());
      if(!in.fixedString(k.targetVolume,16)) return fail(cutOff());
      for(int i=0;i<3;i++) if(!in.f32(k.targetOffset[i])) return fail(cutOff());
      for(int i=0;i<3;i++) if(!in.f32(k.targetDirection[i])) return fail(cutOff());
      if(!in.f32(k.easeInStart) || !in.f32(k.easeInStop) || !in.f32(k.easeOutStart) || !in.f32(k.easeOutStop))
        return fail(cutOff());
      bool good=true;
      for(int i=0;i<3;i++) good=good && finite(k.sourceOffset[i]) && finite(k.targetOffset[i]) && finite(k.targetDirection[i]);
      good=good && finite(k.easeInStart) && finite(k.easeInStop) && finite(k.easeOutStart) && finite(k.easeOutStop);
      if(!good) return fail(QObject::tr("A constraint is not valid."));
      m.constraints.push_back(k);
    }
  }

  if(in.left()>0) warnings << QObject::tr("%1 bytes after the end of the animation were ignored.").arg(in.left());

  Result r;
  r.motion=m;
  r.warnings=warnings;
  return r;
}

}
