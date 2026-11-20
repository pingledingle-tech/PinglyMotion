/*
 * PinglyMotion – foot lock for imported motion (issue #17)
 *
 * See footlock.h.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "footlock.h"

#include "retarget.h"

#include <QObject>
#include <QQuaternion>

#include <algorithm>
#include <cmath>
#include <vector>

using AnimFile::Animation;
using AnimFile::Channel;

namespace FootLock
{
namespace
{
  constexpr double PI=3.14159265358979323846;
  constexpr int MAX_PASSES=8;
  // a leg is held back from being quite straight (share of thigh plus shin)
  constexpr double REACH_SHARE=0.995;
  constexpr float REACHED=1e-3f;   // units of the file (inches: 0.025 mm)

  struct Leg
  {
    int thigh=-1;
    int shin=-1;
    int foot=-1;
  };

  bool isRotation(Channel c) { return c==Channel::XRotation || c==Channel::YRotation || c==Channel::ZRotation; }

  // the value indices of a joint's rotation channels in file order and their
  // axes (0 X, 1 Y, 2 Z), when it has exactly three different ones
  bool rotationChannels(const AnimFile::Joint& j,int index[3],int axis[3])
  {
    int n=0;
    int seen=0;
    for(int c=0;c<j.channels.size();c++)
    {
      if(!isRotation(j.channels[c])) continue;
      const int a=int(j.channels[c])-int(Channel::XRotation);
      if(n>=3 || (seen & (1<<a))) return false;
      seen|=1<<a;
      index[n]=j.firstValue+c;
      axis[n++]=a;
    }
    return n==3;
  }

  QQuaternion axisTurn(int axis,float degrees)
  {
    const QVector3D v(axis==0 ? 1.0f : 0.0f,axis==1 ? 1.0f : 0.0f,axis==2 ? 1.0f : 0.0f);
    return QQuaternion::fromAxisAndAngle(v,degrees);
  }

  // world turns and positions of all joints in one frame, channels in file
  // order (the same as the program and src/retarget.cpp)
  void forward(const Animation& anim,int frame,std::vector<QQuaternion>& world,std::vector<QVector3D>& pos)
  {
    const int n=anim.joints.size();
    world.assign(static_cast<size_t>(n),QQuaternion());
    pos.assign(static_cast<size_t>(n),QVector3D());
    for(int i=0;i<n;i++)
    {
      const AnimFile::Joint& j=anim.joints[i];
      QQuaternion r;
      QVector3D t(float(j.offset[0]),float(j.offset[1]),float(j.offset[2]));
      for(int c=0;c<j.channels.size();c++)
      {
        const float v=anim.value(frame,j.firstValue+c);
        switch(j.channels[c])
        {
          case Channel::XPosition: t[0]+=v; break;
          case Channel::YPosition: t[1]+=v; break;
          case Channel::ZPosition: t[2]+=v; break;
          case Channel::XRotation: r=r*axisTurn(0,v); break;
          case Channel::YRotation: r=r*axisTurn(1,v); break;
          case Channel::ZRotation: r=r*axisTurn(2,v); break;
        }
      }
      const size_t s=static_cast<size_t>(i);
      if(j.parent<0)
      {
        world[s]=r.normalized();
        pos[s]=t;
      }
      else
      {
        const size_t p=static_cast<size_t>(j.parent);
        world[s]=(world[p]*r).normalized();
        pos[s]=pos[p]+world[p].rotatedVector(t);
      }
    }
  }

  // the leg of one side ("L" or "R"), found by the Second Life chain names
  Leg findLeg(const Animation& anim,const QString& side)
  {
    Leg leg;
    for(int i=0;i<anim.joints.size();i++)
    {
      if(Retarget::slChain(anim.joints[i].name)!="foot."+side) continue;
      const int shin=anim.joints[i].parent;
      if(shin<0 || Retarget::slChain(anim.joints[shin].name)!="calf."+side) continue;
      const int thigh=anim.joints[shin].parent;
      if(thigh<0 || anim.joints[thigh].parent<0 || Retarget::slChain(anim.joints[thigh].name)!="thigh."+side) continue;
      int index[3],axis[3];
      if(!rotationChannels(anim.joints[thigh],index,axis) || !rotationChannels(anim.joints[shin],index,axis) ||
         !rotationChannels(anim.joints[i],index,axis)) continue;
      leg.thigh=thigh;
      leg.shin=shin;
      leg.foot=i;
      return leg;
    }
    return leg;
  }

  QString check(const Animation& anim,const Options& o)
  {
    if(anim.format!=AnimFile::Format::Bvh)
      return QObject::tr("Only motion with a pose in every frame (BVH) can be cleaned up.");
    const QString invalid=Retarget::validate(anim,true);
    if(!invalid.isEmpty()) return invalid;
    if(!(std::isfinite(o.metersPerUnit) && o.metersPerUnit>0.0) ||
       !(std::isfinite(o.contactHeight) && o.contactHeight>0.0) ||
       !(std::isfinite(o.contactSpeed) && o.contactSpeed>0.0))
      return QObject::tr("The foot lock thresholds must be above zero.");
    if(o.minContactFrames<1) return QObject::tr("A contact needs at least one frame.");
    if(o.blendFrames<0 || o.blendFrames>MAX_BLEND_FRAMES)
      return QObject::tr("The foot lock blend must be between 0 and %1 frames.").arg(MAX_BLEND_FRAMES);
    if(o.firstFrame<0 || o.firstFrame>=anim.frames) return QObject::tr("The first frame is not part of the animation.");
    return QString();
  }

  float smoothstep(float x)
  {
    x=std::clamp(x,0.0f,1.0f);
    return x*x*(3.0f-2.0f*x);
  }

  // the weight of the pin in a frame (1 inside a contact, easing to 0 over
  // the blend) and which contact it belongs to; -1 when no contact is near
  int pinFor(const QVector<Contact>& contacts,int frame,int blend,float& weight)
  {
    weight=0.0f;
    int best=-1;
    for(int c=0;c<contacts.size();c++)
    {
      const Contact& k=contacts[c];
      int d=0;
      if(frame<k.first) d=k.first-frame;
      else if(frame>k.last) d=frame-k.last;
      if(d>blend) continue;
      const float w=d==0 ? 1.0f : smoothstep(1.0f-float(d)/float(blend+1));
      if(w>weight) { weight=w; best=c; }
    }
    return best;
  }

  float angleDegrees(const QVector3D& u,const QVector3D& v)
  {
    const double c=QVector3D::dotProduct(u,v);
    const double s=QVector3D::crossProduct(u,v).length();
    return float(std::atan2(s,c)*180.0/PI);
  }

  // write a local turn to a joint's rotation channels: the angles nearest to
  // the clip's own (whole turns of 360 degrees added), inside the limits
  void writeTurn(Animation& out,const Animation& anim,int joint,int frame,const QQuaternion& q,const Options& o)
  {
    const AnimFile::Joint& j=anim.joints[joint];
    int index[3],axis[3];
    if(!rotationChannels(j,index,axis)) return;
    float angles[3];
    Retarget::quaternionToEuler(q.normalized(),j.rotationOrder(),angles);
    const auto limit=o.limits.constFind(j.name);
    float* values=out.values.data()+static_cast<size_t>(frame)*out.valuesPerFrame;
    for(int i=0;i<3;i++)
    {
      const float old=anim.value(frame,index[i]);
      float v=angles[i];
      v+=360.0f*std::round((old-v)/360.0f);
      if(limit!=o.limits.constEnd())
      {
        // a clip that is already outside its limits may stay there, not go further
        const float lo=std::min(limit->minimum[axis[i]],old);
        const float hi=std::max(limit->maximum[axis[i]],old);
        v=std::clamp(v,lo,hi);
      }
      values[index[i]]=v;
    }
  }

  // move one leg so its ankle reaches the target; the foot gets footWorld,
  // its world turn in the clip
  void solveLeg(Animation& out,const Animation& anim,const Leg& leg,int frame,const QVector3D& target,
                const QQuaternion& footWorld,const std::vector<QQuaternion>& world,const std::vector<QVector3D>& pos,const Options& o)
  {
    const size_t th=static_cast<size_t>(leg.thigh),sh=static_cast<size_t>(leg.shin),ft=static_cast<size_t>(leg.foot);
    const size_t parent=static_cast<size_t>(anim.joints[leg.thigh].parent);
    const QVector3D H=pos[th],K=pos[sh],A=pos[ft];
    const QVector3D toHip=H-K,toAnkle=A-K;
    const float a=toHip.length(),b=toAnkle.length();
    if(a<1e-6f || b<1e-6f) return;

    // 1. bend the knee in its own plane so that hip to ankle has the right length
    const float reach=(target-H).length();
    const float d=std::clamp(reach,std::fabs(a-b)+1e-4f,a+b-1e-4f);
    const double cosine=std::clamp((double(a)*a+double(b)*b-double(d)*d)/(2.0*a*b),-1.0,1.0);
    const float wanted=float(std::acos(cosine)*180.0/PI);
    QVector3D normal=QVector3D::crossProduct(toHip,toAnkle);
    if(normal.length()<1e-4f*a*b)
      normal=-world[sh].rotatedVector(QVector3D(1.0f,0.0f,0.0f));   // a straight leg bends about the knee's X axis
    const QQuaternion bend=QQuaternion::fromAxisAndAngle(normal.normalized(),wanted-angleDegrees(toHip,toAnkle));
    const QVector3D bentAnkle=K+bend.rotatedVector(toAnkle);

    // 2. swing the whole leg about the hip onto the target
    const QQuaternion swing=QQuaternion::rotationTo(bentAnkle-H,target-H);

    const QQuaternion thighWorld=(swing*world[th]).normalized();
    const QQuaternion shinWorld=(swing*bend*world[sh]).normalized();
    writeTurn(out,anim,leg.thigh,frame,world[parent].conjugated()*thighWorld,o);
    writeTurn(out,anim,leg.shin,frame,thighWorld.conjugated()*shinWorld,o);
    writeTurn(out,anim,leg.foot,frame,shinWorld.conjugated()*footWorld,o);
  }

  // the share of the way to the pin (at most weight) the leg can reach: a
  // pin out of reach (the hip has moved on at the end of a step) is followed
  // only as far as the leg is long, along the line to the pin, so the foot
  // never swings somewhere else
  float reachable(const Leg& leg,const std::vector<QVector3D>& pos,const QVector3D& toPin,float weight)
  {
    const QVector3D H=pos[static_cast<size_t>(leg.thigh)];
    const QVector3D K=pos[static_cast<size_t>(leg.shin)];
    const QVector3D A=pos[static_cast<size_t>(leg.foot)];
    const double reach=REACH_SHARE*((K-H).length()+(A-K).length());
    const QVector3D d=A-H;
    if((d+toPin*weight).length()<=reach) return weight;
    const double dd=QVector3D::dotProduct(d,d),de=QVector3D::dotProduct(d,toPin),ee=QVector3D::dotProduct(toPin,toPin);
    if(dd>=reach*reach || ee<1e-12) return 0.0f;
    const double t=(-de+std::sqrt(de*de-ee*(dd-reach*reach)))/ee;
    return float(std::clamp(t,0.0,double(weight)));
  }

  QVector<Contact> contactsOf(const std::vector<QVector3D>& p,int firstFrame,double floor,double frameTime,
                              double height,double speed,int minFrames)
  {
    QVector<Contact> out;
    const int n=int(p.size());
    std::vector<char> down(static_cast<size_t>(n),0);
    for(int i=0;i<n;i++)
    {
      const int i0=std::max(i-1,0),i1=std::min(i+1,n-1);
      const double v=i1>i0 ? double((p[static_cast<size_t>(i1)]-p[static_cast<size_t>(i0)]).length())/((i1-i0)*frameTime) : 0.0;
      down[static_cast<size_t>(i)]=(p[static_cast<size_t>(i)].y()-floor)<height && v<speed;
    }
    for(int i=0;i<n;)
    {
      if(!down[static_cast<size_t>(i)]) { i++; continue; }
      int e=i;
      while(e+1<n && down[static_cast<size_t>(e+1)]) e++;
      if(e-i+1>=minFrames)
      {
        Contact c;
        c.first=firstFrame+i;
        c.last=firstFrame+e;
        QVector3D sum;
        for(int k=i;k<=e;k++) sum+=p[static_cast<size_t>(k)];
        c.pin=sum/float(e-i+1);
        out << c;
      }
      i=e+1;
    }
    return out;
  }
}

LimitTable parseLimits(const QByteArray& text)
{
  LimitTable table;
  for(const QByteArray& raw : text.split('\n'))
  {
    const QList<QByteArray> p=raw.simplified().split(' ');
    if(p.size()<11) continue;
    Limits l;
    bool good=true;
    for(int i=0;i<3 && good;i++)
    {
      const QByteArray axis=p[2+i*3].toUpper();
      const int a=axis.startsWith('X') ? 0 : axis.startsWith('Y') ? 1 : axis.startsWith('Z') ? 2 : -1;
      bool ok1=false,ok2=false;
      const float lo=p[3+i*3].toFloat(&ok1);
      const float hi=p[4+i*3].toFloat(&ok2);
      if(a<0 || !ok1 || !ok2 || !std::isfinite(lo) || !std::isfinite(hi)) { good=false; break; }
      l.minimum[a]=std::min(lo,hi);
      l.maximum[a]=std::max(lo,hi);
    }
    if(good) table.insert(QString::fromUtf8(p[0]),l);
  }
  return table;
}

QVector3D worldPosition(const Animation& anim,int joint,int frame)
{
  if(joint<0 || joint>=anim.joints.size() || frame<0 || frame>=anim.frames) return QVector3D();
  std::vector<QQuaternion> world;
  std::vector<QVector3D> pos;
  forward(anim,frame,world,pos);
  return pos[static_cast<size_t>(joint)];
}

Contacts findContacts(const Animation& anim,const Options& o)
{
  Contacts result;
  result.error=check(anim,o);
  if(!result.ok()) return result;
  const Leg left=findLeg(anim,QStringLiteral("L"));
  const Leg right=findLeg(anim,QStringLiteral("R"));
  if(left.foot<0 || right.foot<0)
  {
    result.error=QObject::tr("The legs (thigh, shin and foot on both sides) were not found.");
    return result;
  }

  std::vector<QVector3D> pl,pr;
  std::vector<QQuaternion> world;
  std::vector<QVector3D> pos;
  for(int f=o.firstFrame;f<anim.frames;f++)
  {
    forward(anim,f,world,pos);
    pl.push_back(pos[static_cast<size_t>(left.foot)]);
    pr.push_back(pos[static_cast<size_t>(right.foot)]);
  }
  double floor=pl[0].y();
  for(const QVector3D& v : pl) floor=std::min(floor,double(v.y()));
  for(const QVector3D& v : pr) floor=std::min(floor,double(v.y()));
  result.floor=floor;

  const double height=o.contactHeight/o.metersPerUnit;
  const double speed=o.contactSpeed/o.metersPerUnit;
  result.left=contactsOf(pl,o.firstFrame,floor,anim.frameTime,height,speed,o.minContactFrames);
  result.right=contactsOf(pr,o.firstFrame,floor,anim.frameTime,height,speed,o.minContactFrames);
  return result;
}

MotionCleanup::Result lockFeet(const Animation& anim,const Options& o)
{
  MotionCleanup::Result r;
  const Contacts contacts=findContacts(anim,o);
  if(!contacts.ok()) { r.error=contacts.error; return r; }
  const Leg legs[2]={findLeg(anim,QStringLiteral("L")),findLeg(anim,QStringLiteral("R"))};
  const QVector<Contact>* lists[2]={&contacts.left,&contacts.right};

  Animation out=anim;
  std::vector<QQuaternion> world;
  std::vector<QVector3D> pos;
  for(int f=o.firstFrame;f<anim.frames;f++)
  {
    float weight[2];
    const int pin[2]={pinFor(*lists[0],f,o.blendFrames,weight[0]),pinFor(*lists[1],f,o.blendFrames,weight[1])};
    if(pin[0]<0 && pin[1]<0) continue;
    forward(anim,f,world,pos);
    QVector3D target[2];
    QQuaternion footWorld[2];
    for(int s=0;s<2;s++)
    {
      if(pin[s]<0) continue;
      footWorld[s]=world[static_cast<size_t>(legs[s].foot)];
      const QVector3D ankle=pos[static_cast<size_t>(legs[s].foot)];
      const QVector3D toPin=(*lists[s])[pin[s]].pin-ankle;
      target[s]=ankle+toPin*reachable(legs[s],pos,toPin,weight[s]);
    }
    // the legs do not depend on each other, so each pass serves both. One pass
    // is exact; when a joint limit holds a joint back, the next passes let the
    // other joints of the leg make up for it.
    for(int pass=0;pass<MAX_PASSES;pass++)
    {
      if(pass>0) forward(out,f,world,pos);
      bool reached=true;
      for(int s=0;s<2;s++)
      {
        if(pin[s]<0) continue;
        if((pos[static_cast<size_t>(legs[s].foot)]-target[s]).length()<=REACHED) continue;
        reached=false;
        solveLeg(out,anim,legs[s],f,target[s],footWorld[s],world,pos,o);
      }
      if(reached) break;
    }
  }
  r.animation=std::move(out);
  return r;
}

}
