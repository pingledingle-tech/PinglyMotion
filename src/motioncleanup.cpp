/*
 * PinglyMotion – clean-up of imported motion (issues #17 and #18)
 *
 * See motioncleanup.h.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "motioncleanup.h"

#include "retarget.h"

#include <QObject>
#include <QVector4D>

#include <algorithm>
#include <cmath>
#include <vector>

using AnimFile::Animation;
using AnimFile::Channel;

namespace MotionCleanup
{
namespace
{
  bool isRotation(Channel c) { return c==Channel::XRotation || c==Channel::YRotation || c==Channel::ZRotation; }

  // the value indices of a joint's rotation channels in file order, when it
  // has exactly three different ones
  bool rotationChannels(const AnimFile::Joint& j,int out[3])
  {
    int n=0;
    int seen=0;
    for(int c=0;c<j.channels.size();c++)
    {
      if(!isRotation(j.channels[c])) continue;
      const int bit=1<<int(j.channels[c]);
      if(n>=3 || (seen & bit)) return false;
      seen|=bit;
      out[n++]=j.firstValue+c;
    }
    return n==3;
  }

  QString check(const Animation& anim)
  {
    if(anim.format!=AnimFile::Format::Bvh)
      return QObject::tr("Only motion with a pose in every frame (BVH) can be cleaned up.");
    if(anim.joints.isEmpty() || anim.frames<1 || anim.valuesPerFrame<0 ||
       anim.values.size()!=static_cast<size_t>(anim.frames)*static_cast<size_t>(anim.valuesPerFrame))
      return QObject::tr("The animation has no frames.");
    if(!(anim.frameTime>=AnimFile::MIN_FRAME_TIME && anim.frameTime<=AnimFile::MAX_FRAME_TIME))
      return QObject::tr("The animation has no valid frame time.");
    for(float v : anim.values)
      if(!std::isfinite(v)) return QObject::tr("The animation holds a number that is not valid.");
    return QString();
  }

  Result fail(const QString& error) { Result r; r.error=error; return r; }
  Result done(Animation&& anim) { Result r; r.animation=std::move(anim); return r; }

  float* frameValues(Animation& a,int frame) { return a.values.data()+static_cast<size_t>(frame)*a.valuesPerFrame; }
  const float* frameValues(const Animation& a,int frame) { return a.values.data()+static_cast<size_t>(frame)*a.valuesPerFrame; }

  QQuaternion readTurn(const float* values,const int rot[3],AnimFile::RotationOrder order)
  {
    const float angles[3]={values[rot[0]],values[rot[1]],values[rot[2]]};
    return Retarget::eulerToQuaternion(angles,order);
  }

  void writeTurn(float* values,const int rot[3],AnimFile::RotationOrder order,const QQuaternion& q)
  {
    float angles[3];
    Retarget::quaternionToEuler(q.normalized(),order,angles);
    for(int i=0;i<3;i++) values[rot[i]]=angles[i];
  }

  // indices of all values that are not part of a three-channel turn
  std::vector<int> plainValues(const Animation& a)
  {
    std::vector<char> inTurn(static_cast<size_t>(a.valuesPerFrame),0);
    for(const AnimFile::Joint& j : a.joints)
    {
      int rot[3];
      if(rotationChannels(j,rot)) for(int i : rot) inTurn[static_cast<size_t>(i)]=1;
    }
    std::vector<int> out;
    for(int i=0;i<a.valuesPerFrame;i++) if(!inTurn[static_cast<size_t>(i)]) out.push_back(i);
    return out;
  }

  // the value indices of the root's X, Y and Z position channels, -1 if missing
  void rootPosition(const Animation& a,int pos[3])
  {
    pos[0]=pos[1]=pos[2]=-1;
    const AnimFile::Joint& root=a.joints[0];
    for(int c=0;c<root.channels.size();c++)
    {
      if(root.channels[c]==Channel::XPosition) pos[0]=root.firstValue+c;
      else if(root.channels[c]==Channel::YPosition) pos[1]=root.firstValue+c;
      else if(root.channels[c]==Channel::ZPosition) pos[2]=root.firstValue+c;
    }
  }

  // binomial weights for a window of 2m+1 frames (1 2 1, 1 4 6 4 1, ...),
  // they add up to 1
  double weight(int m,int k)
  {
    double c=1.0;
    for(int i=1;i<=m+k;i++) c=c*(2*m-i+1)/i;
    return c/std::pow(4.0,m);
  }

  float smoothstep(float x)
  {
    x=std::clamp(x,0.0f,1.0f);
    return x*x*(3.0f-2.0f*x);
  }
}

QQuaternion turn(const Animation& anim,int joint,int frame)
{
  if(joint<0 || joint>=anim.joints.size() || frame<0 || frame>=anim.frames) return QQuaternion();
  const AnimFile::Joint& j=anim.joints[joint];
  int rot[3];
  if(!rotationChannels(j,rot)) return QQuaternion();
  return readTurn(frameValues(anim,frame),rot,j.rotationOrder());
}

float angleBetween(const QQuaternion& a,const QQuaternion& b)
{
  // the turn from a to b; atan2 stays exact for small angles, where acos does not
  const QQuaternion r=a.normalized().conjugated()*b.normalized();
  const double v=std::sqrt(double(r.x())*r.x()+double(r.y())*r.y()+double(r.z())*r.z());
  return float(2.0*std::atan2(v,std::fabs(double(r.scalar())))*180.0/3.14159265358979323846);
}

Result resample(const Animation& anim,double fps)
{
  const QString error=check(anim);
  if(!error.isEmpty()) return fail(error);
  if(!(fps>=1.0 && fps<=120.0)) return fail(QObject::tr("The frame rate must be between 1 and 120 frames per second."));

  const double length=(anim.frames-1)*anim.frameTime;
  const double newTime=1.0/fps;
  const double count=std::floor(length/newTime+0.5)+1.0;
  if(count>AnimFile::MAX_FRAMES)
    return fail(QObject::tr("At %1 frames per second the animation would have more than %2 frames.").arg(fps).arg(AnimFile::MAX_FRAMES));

  Animation out=anim;
  out.frames=int(count);
  out.frameTime=newTime;
  out.values.assign(static_cast<size_t>(out.frames)*out.valuesPerFrame,0.0f);

  const std::vector<int> plain=plainValues(anim);
  for(int f=0;f<out.frames;f++)
  {
    double s=std::min(f*newTime/anim.frameTime,double(anim.frames-1));
    if(std::fabs(s-std::round(s))<1e-4) s=std::round(s);   // on an old frame: copy it exactly
    const int i0=std::min(int(std::floor(s)),anim.frames-1);
    const int i1=std::min(i0+1,anim.frames-1);
    const float t=float(s-i0);
    const float* a=frameValues(anim,i0);
    const float* b=frameValues(anim,i1);
    float* o=frameValues(out,f);
    for(int i : plain) o[i]=a[i]+(b[i]-a[i])*t;
    for(const AnimFile::Joint& j : anim.joints)
    {
      int rot[3];
      if(!rotationChannels(j,rot)) continue;
      const AnimFile::RotationOrder order=j.rotationOrder();
      if(t==0.0f) { for(int i : rot) o[i]=a[i]; continue; }
      writeTurn(o,rot,order,QQuaternion::slerp(readTurn(a,rot,order),readTurn(b,rot,order),t));
    }
  }
  return done(std::move(out));
}

Result insertStartPose(const Animation& anim)
{
  const QString error=check(anim);
  if(!error.isEmpty()) return fail(error);
  if(anim.frames+1>AnimFile::MAX_FRAMES)
    return fail(QObject::tr("The animation already has %1 frames, the most there can be.").arg(AnimFile::MAX_FRAMES));

  Animation out=anim;
  out.frames=anim.frames+1;
  out.values.resize(static_cast<size_t>(out.frames)*out.valuesPerFrame);
  std::copy(anim.values.begin(),anim.values.end(),out.values.begin()+out.valuesPerFrame);
  // the start pose: turns at zero, positions of the first frame of the motion
  float* start=frameValues(out,0);
  const float* first=frameValues(anim,0);
  for(const AnimFile::Joint& j : anim.joints)
    for(int c=0;c<j.channels.size();c++)
      start[j.firstValue+c]=isRotation(j.channels[c]) ? 0.0f : first[j.firstValue+c];
  return done(std::move(out));
}

Result inPlace(const Animation& anim,int firstFrame)
{
  const QString error=check(anim);
  if(!error.isEmpty()) return fail(error);
  if(firstFrame<0 || firstFrame>=anim.frames) return fail(QObject::tr("The first frame is not part of the animation."));
  int pos[3];
  rootPosition(anim,pos);
  Animation out=anim;
  const int last=anim.frames-1;
  if(last==firstFrame) return done(std::move(out));
  for(int axis : {0,2})
  {
    if(pos[axis]<0) continue;
    const float start=anim.value(firstFrame,pos[axis]);
    const float travel=anim.value(last,pos[axis])-start;
    for(int f=firstFrame;f<=last;f++)
      frameValues(out,f)[pos[axis]]-=travel*float(f-firstFrame)/float(last-firstFrame);
    frameValues(out,last)[pos[axis]]=start;   // exactly, without rounding
  }
  return done(std::move(out));
}

Result blendLoopSeam(const Animation& anim,int blendFrames,int firstFrame)
{
  const QString error=check(anim);
  if(!error.isEmpty()) return fail(error);
  if(firstFrame<0 || firstFrame>=anim.frames) return fail(QObject::tr("The first frame is not part of the animation."));
  if(blendFrames<1) return fail(QObject::tr("The blend needs at least one frame."));
  Animation out=anim;
  const int last=anim.frames-1;
  const int span=std::min(blendFrames,last-firstFrame);
  if(span<1) return done(std::move(out));

  int pos[3];
  rootPosition(anim,pos);
  std::vector<int> plain=plainValues(anim);
  // the travel over the ground is not blended
  plain.erase(std::remove_if(plain.begin(),plain.end(),[&](int i){ return i==pos[0] || i==pos[2]; }),plain.end());

  const float* target=frameValues(anim,firstFrame);
  for(int f=last-span+1;f<=last;f++)
  {
    const float w=smoothstep(float(f-(last-span))/float(span));
    float* o=frameValues(out,f);
    const float* a=frameValues(anim,f);
    for(int i : plain) o[i]=a[i]+(target[i]-a[i])*w;
    for(const AnimFile::Joint& j : anim.joints)
    {
      int rot[3];
      if(!rotationChannels(j,rot)) continue;
      const AnimFile::RotationOrder order=j.rotationOrder();
      if(f==last) { for(int i : rot) o[i]=target[i]; continue; }
      writeTurn(o,rot,order,QQuaternion::slerp(readTurn(a,rot,order),readTurn(target,rot,order),w));
    }
  }
  return done(std::move(out));
}

Result smooth(const Animation& anim,int radius,int firstFrame)
{
  const QString error=check(anim);
  if(!error.isEmpty()) return fail(error);
  if(firstFrame<0 || firstFrame>=anim.frames) return fail(QObject::tr("The first frame is not part of the animation."));
  if(radius<1 || radius>MAX_SMOOTH_RADIUS)
    return fail(QObject::tr("The smoothing must be between 1 and %1 frames.").arg(MAX_SMOOTH_RADIUS));

  Animation out=anim;
  const int count=anim.frames-firstFrame;
  // the window is symmetric and shrinks near the ends (a frame at the very end is kept)
  auto window=[&](int f){ return std::min({radius,f,count-1-f}); };
  const std::vector<int> plain=plainValues(anim);
  for(int i : plain)
    for(int f=0;f<count;f++)
    {
      const int m=window(f);
      if(m<1) continue;
      double sum=0.0;
      for(int k=-m;k<=m;k++) sum+=weight(m,k)*anim.value(firstFrame+f+k,i);
      frameValues(out,firstFrame+f)[i]=float(sum);
    }

  std::vector<QVector4D> q(static_cast<size_t>(count));
  for(const AnimFile::Joint& j : anim.joints)
  {
    int rot[3];
    if(!rotationChannels(j,rot)) continue;
    const AnimFile::RotationOrder order=j.rotationOrder();
    // the turns as 4D vectors, each on the same side as the one before
    for(int f=0;f<count;f++)
    {
      QVector4D v=readTurn(frameValues(anim,firstFrame+f),rot,order).toVector4D();
      if(f>0 && QVector4D::dotProduct(v,q[static_cast<size_t>(f-1)])<0.0f) v=-v;
      q[static_cast<size_t>(f)]=v;
    }
    for(int f=0;f<count;f++)
    {
      const int m=window(f);
      if(m<1) continue;
      QVector4D sum;
      for(int k=-m;k<=m;k++) sum+=float(weight(m,k))*q[static_cast<size_t>(f+k)];
      if(sum.lengthSquared()<1e-12f) continue;   // cannot happen with turns on one side; keep the frame
      writeTurn(frameValues(out,firstFrame+f),rot,order,QQuaternion(sum.normalized()));
    }
  }
  return done(std::move(out));
}

}
