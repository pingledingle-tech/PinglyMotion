/*
 * Pingmator – write a Second Life .anim file from an animation (issue #10)
 *
 * See animexport.h and docs/ANIM_FORMAT.md. The numbers follow the viewer:
 *  - axes: BVH x, y, z become Second Life y, z, x (section 7.2)
 *  - the root joint (the pelvis) is stored relative to frame 0, in metres
 *    (section 7.3); frame 0 is a reference frame that is not played (7.4)
 *  - quaternions are stored as x, y, z of the unit quaternion with w >= 0,
 *    each part quantised to 16 bits (section 4)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animexport.h"

#include <QObject>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace AnimExport
{

namespace
{
  struct Quat
  {
    double x=0.0,y=0.0,z=0.0,w=1.0;
  };

  // the usual product: a*b applies b first, then a
  Quat mul(const Quat& a,const Quat& b)
  {
    Quat r;
    r.w=a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z;
    r.x=a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y;
    r.y=a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x;
    r.z=a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w;
    return r;
  }

  Quat conjugate(const Quat& q) { Quat r; r.x=-q.x; r.y=-q.y; r.z=-q.z; r.w=q.w; return r; }

  void normalize(Quat& q)
  {
    const double length=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    if(length<1e-12) { q=Quat(); return; }
    q.x/=length; q.y/=length; q.z/=length; q.w/=length;
  }

  Quat fromAxisDegrees(int axis,double degrees)
  {
    const double half=degrees*M_PI/360.0;
    Quat q;
    q.w=std::cos(half);
    const double s=std::sin(half);
    if(axis==0) q.x=s; else if(axis==1) q.y=s; else q.z=s;
    return q;
  }

  // Rotate a vector by the quaternion (q v q^-1)
  void rotateVector(const Quat& q,const double in[3],double out[3])
  {
    Quat v; v.x=in[0]; v.y=in[1]; v.z=in[2]; v.w=0.0;
    const Quat r=mul(mul(q,v),conjugate(q));
    out[0]=r.x; out[1]=r.y; out[2]=r.z;
  }

  // the angle between two rotations in radians
  double angleBetween(const Quat& a,const Quat& b)
  {
    const double dot=std::fabs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w);
    return 2.0*std::acos(std::min(1.0,dot));
  }

  struct RotKey { double time; Quat q; };
  struct PosKey { double time; double v[3]; };

  struct JointData
  {
    QString name;
    int priority=2;
    std::vector<RotKey> rot;
    std::vector<PosKey> pos;
  };

  bool isRotation(AnimFile::Channel c)
  {
    return c==AnimFile::Channel::XRotation || c==AnimFile::Channel::YRotation || c==AnimFile::Channel::ZRotation;
  }

  bool isPosition(AnimFile::Channel c)
  {
    return c==AnimFile::Channel::XPosition || c==AnimFile::Channel::YPosition || c==AnimFile::Channel::ZPosition;
  }

  // the rotation channels of a joint in one frame, in BVH axes: the first channel is the leftmost factor
  Quat jointRotation(const AnimFile::Animation& anim,int joint,int frame)
  {
    const AnimFile::Joint& j=anim.joints[joint];
    Quat q;
    for(int i=0;i<j.channels.size();i++)
    {
      const AnimFile::Channel c=j.channels[i];
      if(!isRotation(c)) continue;
      const double degrees=anim.value(frame,j.firstValue+i);
      const int axis=c==AnimFile::Channel::XRotation ? 0 : (c==AnimFile::Channel::YRotation ? 1 : 2);
      q=mul(q,fromAxisDegrees(axis,degrees));
    }
    normalize(q);
    return q;
  }

  // the position channels of a joint in one frame, in BVH units; missing channels are 0
  void jointPosition(const AnimFile::Animation& anim,int joint,int frame,double out[3])
  {
    const AnimFile::Joint& j=anim.joints[joint];
    out[0]=out[1]=out[2]=0.0;
    for(int i=0;i<j.channels.size();i++)
    {
      const AnimFile::Channel c=j.channels[i];
      const double value=anim.value(frame,j.firstValue+i);
      if(c==AnimFile::Channel::XPosition) out[0]=value;
      else if(c==AnimFile::Channel::YPosition) out[1]=value;
      else if(c==AnimFile::Channel::ZPosition) out[2]=value;
    }
  }

  bool hasRotationChannels(const AnimFile::Joint& j)
  {
    for(AnimFile::Channel c : j.channels) if(isRotation(c)) return true;
    return false;
  }

  bool hasPositionChannels(const AnimFile::Joint& j)
  {
    for(AnimFile::Channel c : j.channels) if(isPosition(c)) return true;
    return false;
  }

  // BVH axes (x, y, z) to Second Life axes: x = z, y = x, z = y
  Quat toSecondLifeAxes(const Quat& q)
  {
    Quat r; r.x=q.z; r.y=q.x; r.z=q.y; r.w=q.w;
    return r;
  }

  void appendU16(QByteArray& b,quint16 v) { char t[2]; qToLittleEndian<quint16>(v,t); b.append(t,2); }
  void appendS32(QByteArray& b,qint32 v) { char t[4]; qToLittleEndian<qint32>(v,t); b.append(t,4); }
  void appendU32(QByteArray& b,quint32 v) { char t[4]; qToLittleEndian<quint32>(v,t); b.append(t,4); }
  void appendF32(QByteArray& b,float v)
  {
    quint32 bits; std::memcpy(&bits,&v,4);
    appendU32(b,bits);
  }
  void appendString(QByteArray& b,const QByteArray& s) { b.append(s); b.append('\0'); }

  // LLVector3::quantize16(): quantise, decode, and the writer quantises once more
  quint16 quantizeTwice(float value,float lower,float upper)
  {
    const float once=u16ToFloat(floatToU16(value,lower,upper),lower,upper);
    return floatToU16(once,lower,upper);
  }

  // keep the first and last key and every key that a straight line between the kept
  // neighbours does not explain within the tolerances
  std::vector<RotKey> thinRotations(const std::vector<RotKey>& keys,double tolerance)
  {
    const size_t n=keys.size();
    if(n<3) return keys;
    std::vector<RotKey> kept;
    kept.push_back(keys[0]);
    size_t i=0;
    while(i<n-1)
    {
      size_t best=i+1;
      for(size_t j=i+2;j<n;j++)
      {
        bool ok=true;
        for(size_t m=i+1;m<j && ok;m++)
        {
          const double u=(keys[m].time-keys[i].time)/(keys[j].time-keys[i].time);
          Quat b=keys[j].q;
          const Quat& a=keys[i].q;
          if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0.0) { b.x=-b.x; b.y=-b.y; b.z=-b.z; b.w=-b.w; }
          Quat l; l.x=a.x+(b.x-a.x)*u; l.y=a.y+(b.y-a.y)*u; l.z=a.z+(b.z-a.z)*u; l.w=a.w+(b.w-a.w)*u;
          normalize(l);
          ok=angleBetween(l,keys[m].q)<=tolerance;
        }
        if(!ok) break;
        best=j;
      }
      kept.push_back(keys[best]);
      i=best;
    }
    return kept;
  }

  std::vector<PosKey> thinPositions(const std::vector<PosKey>& keys,double tolerance)
  {
    const size_t n=keys.size();
    if(n<3) return keys;
    std::vector<PosKey> kept;
    kept.push_back(keys[0]);
    size_t i=0;
    while(i<n-1)
    {
      size_t best=i+1;
      for(size_t j=i+2;j<n;j++)
      {
        bool ok=true;
        for(size_t m=i+1;m<j && ok;m++)
        {
          const double u=(keys[m].time-keys[i].time)/(keys[j].time-keys[i].time);
          double d2=0.0;
          for(int k=0;k<3;k++)
          {
            const double l=keys[i].v[k]+(keys[j].v[k]-keys[i].v[k])*u;
            d2+=(l-keys[m].v[k])*(l-keys[m].v[k]);
          }
          ok=std::sqrt(d2)<=tolerance;
        }
        if(!ok) break;
        best=j;
      }
      kept.push_back(keys[best]);
      i=best;
    }
    return kept;
  }

  struct Header
  {
    float duration=1.0f;
    int basePriority=2;
    QByteArray emote;
    float loopIn=0.0f;
    float loopOut=1.0f;
    bool loop=false;
    float easeIn=0.3f;
    float easeOut=0.3f;
    int handPose=1;
  };

  QByteArray serialize(const Header& h,const std::vector<JointData>& joints,double thinning)
  {
    QByteArray out;
    appendU16(out,1);                                   // version
    appendU16(out,0);                                   // sub_version
    appendS32(out,h.basePriority);
    appendF32(out,h.duration);
    appendString(out,h.emote);
    appendF32(out,h.loopIn);
    appendF32(out,h.loopOut);
    appendS32(out,h.loop ? 1 : 0);
    appendF32(out,h.easeIn);
    appendF32(out,h.easeOut);
    appendU32(out,static_cast<quint32>(h.handPose));
    appendU32(out,static_cast<quint32>(joints.size()));

    for(const JointData& j : joints)
    {
      const std::vector<RotKey> rot=thinning>0.0 ? thinRotations(j.rot,thinning) : j.rot;
      const std::vector<PosKey> pos=thinning>0.0 ? thinPositions(j.pos,thinning*0.5) : j.pos;

      appendString(out,j.name.toLatin1());
      appendS32(out,j.priority);

      // keys that quantise to the same time are one key; the later one wins
      std::vector<std::pair<quint16,Quat>> rq;
      for(const RotKey& k : rot)
      {
        Quat q=k.q;
        normalize(q);
        if(q.w<0.0) { q.x=-q.x; q.y=-q.y; q.z=-q.z; q.w=-q.w; }
        const quint16 t=floatToU16(static_cast<float>(k.time),0.0f,h.duration);
        if(!rq.empty() && rq.back().first==t) rq.back().second=q; else rq.push_back({t,q});
      }
      appendS32(out,static_cast<qint32>(rq.size()));
      for(const auto& k : rq)
      {
        appendU16(out,k.first);
        appendU16(out,quantizeTwice(static_cast<float>(k.second.x),-1.0f,1.0f));
        appendU16(out,quantizeTwice(static_cast<float>(k.second.y),-1.0f,1.0f));
        appendU16(out,quantizeTwice(static_cast<float>(k.second.z),-1.0f,1.0f));
      }

      std::vector<std::pair<quint16,const PosKey*>> pq;
      for(const PosKey& k : pos)
      {
        const quint16 t=floatToU16(static_cast<float>(k.time),0.0f,h.duration);
        if(!pq.empty() && pq.back().first==t) pq.back().second=&k; else pq.push_back({t,&k});
      }
      appendS32(out,static_cast<qint32>(pq.size()));
      const float p=static_cast<float>(MAX_POSITION);
      for(const auto& k : pq)
      {
        appendU16(out,k.first);
        for(int a=0;a<3;a++) appendU16(out,quantizeTwice(static_cast<float>(k.second->v[a]),-p,p));
      }
    }

    appendS32(out,0);                                   // no constraints
    return out;
  }
}

quint16 floatToU16(float value,float lower,float upper)
{
  value=std::min(std::max(value,lower),upper);
  value-=lower;
  value/=(upper-lower);
  value*=65535.0f;
  const float f=std::floor(value);
  if(f<=0.0f) return 0;
  if(f>=65535.0f) return 65535;
  return static_cast<quint16>(f);
}

float u16ToFloat(quint16 value,float lower,float upper)
{
  const float oneOver=1.0f/65535.0f;
  float temp=static_cast<float>(value);
  temp*=oneOver;
  temp*=(upper-lower);
  temp+=lower;
  const float maxError=(upper-lower)*oneOver;
  if(std::fabs(temp)<maxError) temp=0.0f;               // zero comes through as zero
  return temp;
}

QString slJointName(const QString& name)
{
  static const QHash<QString,QString> aliases={
    {"hip","mPelvis"},{"abdomen","mTorso"},{"chest","mChest"},{"neck","mNeck"},{"head","mHead"},
    {"lCollar","mCollarLeft"},{"rCollar","mCollarRight"},
    {"lShldr","mShoulderLeft"},{"rShldr","mShoulderRight"},
    {"lForeArm","mElbowLeft"},{"rForeArm","mElbowRight"},
    {"lHand","mWristLeft"},{"rHand","mWristRight"},
    {"lThigh","mHipLeft"},{"rThigh","mHipRight"},
    {"lShin","mKneeLeft"},{"rShin","mKneeRight"},
    {"lFoot","mAnkleLeft"},{"rFoot","mAnkleRight"}};
  return aliases.value(name,name);
}

Result write(const AnimFile::Animation& anim,const Options& options)
{
  Result result;
  auto fail=[&result](const QString& message){ result.error=message; return result; };

  if(anim.joints.isEmpty() || anim.frames<1 || anim.valuesPerFrame<1 ||
     anim.values.size()<static_cast<size_t>(anim.frames)*static_cast<size_t>(anim.valuesPerFrame))
    return fail(QObject::tr("There is no animation to export."));

  // time: frame 0 is a reference frame and is not played when there are more frames
  const int firstFrame=anim.frames>1 ? 1 : 0;
  const double duration=std::max(anim.frames-2,1)*anim.frameTime;
  if(!(duration>0.0)) return fail(QObject::tr("The animation has no length."));
  if(duration>MAX_DURATION)
    return fail(QObject::tr("The animation is %1 seconds long. Second Life accepts at most %2 seconds.")
                .arg(duration,0,'f',1).arg(MAX_DURATION,0,'f',0));

  Header header;
  header.duration=static_cast<float>(duration);
  header.basePriority=std::min(std::max(options.basePriority,0),MAX_PRIORITY);
  if(header.basePriority!=options.basePriority)
    result.warnings << QObject::tr("The priority was changed to %1 (the range is 0 to %2).").arg(header.basePriority).arg(MAX_PRIORITY);
  if(options.handPose<0 || options.handPose>MAX_HAND_POSE)
    return fail(QObject::tr("The hand pose must be between 0 and %1.").arg(MAX_HAND_POSE));
  header.handPose=options.handPose;
  header.emote=options.emote.toLatin1();
  if(header.emote.contains('\0')) return fail(QObject::tr("The emote name is not valid."));

  header.loop=options.loop;
  if(options.loop)
  {
    double out=options.loopOut<0.0 ? duration : options.loopOut;
    double in=options.loopIn;
    out=std::min(std::max(out,0.0),duration);
    in=std::min(std::max(in,0.0),out);
    header.loopIn=static_cast<float>(in);
    header.loopOut=static_cast<float>(out);
  }
  else
  {
    header.loopIn=0.0f;
    header.loopOut=header.duration;
  }

  double easeIn=std::max(options.easeIn,0.0);
  double easeOut=std::max(options.easeOut,0.0);
  if(!options.loop && easeIn+easeOut>duration && easeIn+easeOut>0.0)
  {
    const double scale=duration/(easeIn+easeOut);
    easeIn*=scale; easeOut*=scale;
    result.warnings << QObject::tr("The ease times were shortened to fit the animation.");
  }
  header.easeIn=static_cast<float>(easeIn);
  header.easeOut=static_cast<float>(easeOut);

  std::vector<JointData> joints;
  QStringList seen;

  for(int ji=0;ji<anim.joints.size();ji++)
  {
    const AnimFile::Joint& j=anim.joints[ji];
    if(j.kind==AnimFile::JointKind::EndSite) continue;

    const QString name=slJointName(j.name);
    if(name=="mRoot" || name=="mScreen")
    {
      result.warnings << QObject::tr("The joint %1 was left out: Second Life does not accept it.").arg(name);
      continue;
    }
    if(seen.contains(name)) return fail(QObject::tr("The joint %1 is in the animation twice.").arg(name));
    seen << name;

    const bool isRoot=ji==0;
    const bool rotates=hasRotationChannels(j);
    const bool moves=hasPositionChannels(j);
    if(!rotates && !moves && !isRoot) continue;

    // is there any motion away from frame 0?
    bool moving=false;
    {
      const Quat q0=jointRotation(anim,ji,0);
      double p0[3]; jointPosition(anim,ji,0,p0);
      for(int f=1;f<anim.frames && !moving;f++)
      {
        if(rotates && angleBetween(q0,jointRotation(anim,ji,f))>0.001) moving=true;
        if(moves)
        {
          double p[3]; jointPosition(anim,ji,f,p);
          const double d2=(p[0]-p0[0])*(p[0]-p0[0])+(p[1]-p0[1])*(p[1]-p0[1])+(p[2]-p0[2])*(p[2]-p0[2]);
          if(d2>0.001) moving=true;
        }
      }
    }
    if(!moving && !isRoot && !options.keepStaticJoints) continue;

    JointData data;
    data.name=name;
    data.priority=std::min(std::max(options.jointPriority.value(name,header.basePriority),0),MAX_PRIORITY);

    const Quat q0=jointRotation(anim,ji,0);
    const Quat q0inverse=conjugate(q0);
    double p0[3]; jointPosition(anim,ji,0,p0);

    for(int f=firstFrame;f<anim.frames;f++)
    {
      const double time=std::max(f-1,0)*anim.frameTime;
      if(rotates)
      {
        Quat q=jointRotation(anim,ji,f);
        if(isRoot) q=mul(q0inverse,q);                  // the pelvis is relative to frame 0
        RotKey key; key.time=time; key.q=toSecondLifeAxes(q);
        data.rot.push_back(key);
      }
      if(moves)
      {
        double p[3]; jointPosition(anim,ji,f,p);
        double v[3]={p[0],p[1],p[2]};
        if(isRoot)
        {
          const double delta[3]={p[0]-p0[0],p[1]-p0[1],p[2]-p0[2]};
          rotateVector(q0inverse,delta,v);
        }
        PosKey key; key.time=time;
        // BVH units are taken as inches; BVH x, y, z become Second Life y, z, x
        key.v[0]=std::min(std::max(v[2]*INCHES_TO_METERS,-MAX_POSITION),MAX_POSITION);
        key.v[1]=std::min(std::max(v[0]*INCHES_TO_METERS,-MAX_POSITION),MAX_POSITION);
        key.v[2]=std::min(std::max(v[1]*INCHES_TO_METERS,-MAX_POSITION),MAX_POSITION);
        data.pos.push_back(key);
      }
    }

    const bool slName=name.size()>1 && name[0]=='m' && name[1].isUpper();
    if(!slName)
      result.warnings << QObject::tr("The joint %1 is not a Second Life joint name. Second Life will refuse the file.").arg(name);

    joints.push_back(data);
  }

  if(joints.empty()) return fail(QObject::tr("There is no joint to export."));
  if(joints.size()>static_cast<size_t>(MAX_JOINTS))
    return fail(QObject::tr("The animation has %1 joints. Second Life accepts at most %2.").arg(joints.size()).arg(MAX_JOINTS));

  QByteArray bytes=serialize(header,joints,0.0);
  if(bytes.size()>options.maxBytes)
  {
    // thin out keys that a straight line explains, a little more each time
    double tolerance=0.0005;
    for(int round=0;round<10 && bytes.size()>options.maxBytes;round++,tolerance*=2.0)
      bytes=serialize(header,joints,tolerance);
    if(bytes.size()>options.maxBytes)
      return fail(QObject::tr("The file would be %1 bytes, more than the %2 bytes Second Life accepts, even with fewer keys.")
                  .arg(bytes.size()).arg(options.maxBytes));
    result.warnings << QObject::tr("Keys were removed to keep the file under %1 bytes.").arg(options.maxBytes);
  }

  result.bytes=bytes;
  return result;
}

}
