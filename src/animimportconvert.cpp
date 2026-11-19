/*
 * PinglyMotion - turn a read Second Life .anim file into an animation of the
 * program (issue #22, menu step). See animimportconvert.h.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <QHash>
#include <QObject>
#include <QQuaternion>
#include <QVector3D>

#include <algorithm>
#include <cmath>

#include "animexport.h"
#include "animimportconvert.h"
#include "retarget.h"

namespace AnimImportConvert
{
  namespace
  {
    bool isRotation(AnimFile::Channel c)
    {
      return c==AnimFile::Channel::XRotation || c==AnimFile::Channel::YRotation || c==AnimFile::Channel::ZRotation;
    }

    // the unit quaternion at a time: the first key before the first, the last key after the last
    QQuaternion sampleRotation(const QVector<AnimImport::RotationKey>& keys,double time)
    {
      auto quat=[](const AnimImport::RotationKey& k){ return QQuaternion(k.w,k.x,k.y,k.z).normalized(); };
      if(keys.isEmpty()) return QQuaternion();
      if(time<=keys.first().time) return quat(keys.first());
      if(time>=keys.last().time) return quat(keys.last());
      int hi=1;
      while(hi<keys.size()-1 && keys[hi].time<time) hi++;
      const AnimImport::RotationKey& a=keys[hi-1];
      const AnimImport::RotationKey& b=keys[hi];
      const double span=double(b.time)-double(a.time);
      const float u=span>0.0 ? float((time-a.time)/span) : 0.0f;
      return QQuaternion::slerp(quat(a),quat(b),u).normalized();
    }

    QVector3D samplePosition(const QVector<AnimImport::PositionKey>& keys,double time)
    {
      auto vec=[](const AnimImport::PositionKey& k){ return QVector3D(k.x,k.y,k.z); };
      if(keys.isEmpty()) return QVector3D();
      if(time<=keys.first().time) return vec(keys.first());
      if(time>=keys.last().time) return vec(keys.last());
      int hi=1;
      while(hi<keys.size()-1 && keys[hi].time<time) hi++;
      const AnimImport::PositionKey& a=keys[hi-1];
      const AnimImport::PositionKey& b=keys[hi];
      const double span=double(b.time)-double(a.time);
      const float u=span>0.0 ? float((time-a.time)/span) : 0.0f;
      return vec(a)*(1.0f-u)+vec(b)*u;
    }

    // the turn of a joint in one frame of the target, as a quaternion (BVH axes)
    QQuaternion frameRotation(const AnimFile::Animation& anim,int joint,int frame)
    {
      const AnimFile::Joint& j=anim.joints[joint];
      float angles[3]={0,0,0};
      int k=0;
      for(int i=0;i<j.channels.size() && k<3;i++)
        if(isRotation(j.channels[i])) angles[k++]=anim.value(frame,j.firstValue+i);
      return Retarget::eulerToQuaternion(angles,j.rotationOrder());
    }

    // the keys of the file use Second Life axes; the program uses BVH axes (x = y, y = z, z = x)
    QQuaternion toBvhAxes(const QQuaternion& sl)
    {
      return QQuaternion(sl.scalar(),sl.y(),sl.z(),sl.x());
    }
  }

  QString listNames(const QStringList& names)
  {
    QStringList shown=names.mid(0,MAX_LISTED);
    if(names.size()>MAX_LISTED)
      return QObject::tr("%1 and %2 more").arg(shown.join(QStringLiteral(", "))).arg(names.size()-MAX_LISTED);
    if(shown.size()>1)
      return QObject::tr("%1 and %2").arg(shown.mid(0,shown.size()-1).join(QStringLiteral(", ")),shown.last());
    return shown.join(QString());
  }

  Result convert(const AnimImport::Motion& motion,const AnimFile::Animation& target)
  {
    Result result;

    const QString targetProblem=Retarget::validate(target,false);
    if(!targetProblem.isEmpty() || target.frames<1)
    {
      result.error=QObject::tr("The skeleton of the program could not be used.\nReinstall the program, then try again.");
      return result;
    }

    // the joints of the target by their Second Life names
    QHash<QString,int> byName;
    for(int i=0;i<target.joints.size();i++)
      if(target.joints[i].kind!=AnimFile::JointKind::EndSite)
        byName.insert(AnimExport::slJointName(target.joints[i].name),i);

    const double duration=motion.duration;
    const int played=std::max(int(std::lround(duration/FRAME_TIME)),1);
    const int frames=played+2;                      // frame 0 is the reference pose
    if(frames>AnimFile::MAX_FRAMES)
    {
      result.error=QObject::tr("This animation is too long to open.\nShorten it in the program that made it, then try again.");
      return result;
    }

    AnimFile::Animation anim=target;
    anim.format=AnimFile::Format::Bvh;
    anim.frames=frames;
    anim.frameTime=FRAME_TIME;
    anim.avatarScale=1.0f;
    anim.loopIn=-1;
    anim.loopOut=0;
    anim.hasPositionKeys=false;
    anim.positionKeys.clear();
    for(AnimFile::Joint& joint : anim.joints) joint.keys.clear();

    // every frame starts as the rest pose of the target (its frame 0)
    const size_t perFrame=static_cast<size_t>(target.valuesPerFrame);
    anim.values.assign(static_cast<size_t>(frames)*perFrame,0.0f);
    for(int f=0;f<frames;f++)
      std::copy(target.values.begin(),target.values.begin()+perFrame,anim.values.begin()+static_cast<size_t>(f)*perFrame);

    QStringList unknown;       // joints that are not on the skeleton
    QStringList noPosition;    // joints with moves that cannot be kept
    QStringList used;
    for(const AnimImport::Joint& fileJoint : motion.joints)
    {
      const QString name=AnimExport::slJointName(fileJoint.name);
      const int index=byName.value(name,-1);
      if(index<0 || used.contains(name))
      {
        if(!unknown.contains(fileJoint.name)) unknown << fileJoint.name;
        continue;
      }
      used << name;
      const AnimFile::Joint& joint=target.joints[index];
      const bool isRoot=index==0;

      QVector<int> rotationChannels;
      QVector<int> positionChannels(3,-1);
      for(int i=0;i<joint.channels.size();i++)
      {
        const AnimFile::Channel c=joint.channels[i];
        if(isRotation(c)) rotationChannels.append(i);
        else if(c==AnimFile::Channel::XPosition) positionChannels[0]=i;
        else if(c==AnimFile::Channel::YPosition) positionChannels[1]=i;
        else if(c==AnimFile::Channel::ZPosition) positionChannels[2]=i;
      }

      const QQuaternion rest=frameRotation(target,index,0);
      QVector3D restPosition;
      if(positionChannels[0]>=0) restPosition.setX(target.value(0,joint.firstValue+positionChannels[0]));
      if(positionChannels[1]>=0) restPosition.setY(target.value(0,joint.firstValue+positionChannels[1]));
      if(positionChannels[2]>=0) restPosition.setZ(target.value(0,joint.firstValue+positionChannels[2]));

      const bool turns=!fileJoint.rotations.isEmpty() && rotationChannels.size()==3;
      const bool moves=!fileJoint.positions.isEmpty();
      const bool canMove=positionChannels[0]>=0 && positionChannels[1]>=0 && positionChannels[2]>=0;
      if(moves && !canMove && !noPosition.contains(fileJoint.name)) noPosition << fileJoint.name;

      for(int f=1;f<frames;f++)
      {
        const double time=std::min(double(f-1)*FRAME_TIME,duration);
        float* row=anim.values.data()+static_cast<size_t>(f)*perFrame+joint.firstValue;
        if(turns)
        {
          QQuaternion q=toBvhAxes(sampleRotation(fileJoint.rotations,time));
          if(isRoot) q=(rest*q).normalized();           // the pelvis is stored relative to frame 0
          float angles[3];
          Retarget::quaternionToEuler(q,joint.rotationOrder(),angles);
          for(int k=0;k<3;k++) row[rotationChannels[k]]=angles[k];
        }
        if(moves && canMove)
        {
          const QVector3D sl=samplePosition(fileJoint.positions,time);
          // Second Life x, y, z are BVH z, x, y; BVH units are inches
          QVector3D v(float(sl.y()/AnimExport::INCHES_TO_METERS),
                      float(sl.z()/AnimExport::INCHES_TO_METERS),
                      float(sl.x()/AnimExport::INCHES_TO_METERS));
          if(isRoot) v=rest.rotatedVector(v);
          const QVector3D p=restPosition+v;
          row[positionChannels[0]]=p.x();
          row[positionChannels[1]]=p.y();
          row[positionChannels[2]]=p.z();
        }
      }
    }

    if(used.isEmpty())
    {
      result.error=QObject::tr("No joint in this file fits the skeleton of the program.\n"
                               "Check that it is an animation for a Second Life avatar.");
      return result;
    }

    if(!unknown.isEmpty())
      result.notes << QObject::tr("These joints are not on the skeleton and were left out: %1.").arg(listNames(unknown));
    if(!noPosition.isEmpty())
      result.notes << QObject::tr("Only the hip can move. The moves of %1 were left out.").arg(listNames(noPosition));
    if(!motion.constraints.isEmpty())
      result.notes << QObject::tr("The holds that pin a joint to something were left out.");
    result.notes << QObject::tr("Priority, slow start and finish, and hands are not kept.");

    if(motion.loop)
    {
      // the window counts frames from 1, and frame 1 is the first one that plays
      const int in=int(std::lround(motion.loopIn/FRAME_TIME))+2;
      const int out=int(std::lround(motion.loopOut/FRAME_TIME))+2;
      result.loopOut=std::min(std::max(out,2),frames);
      result.loopIn=std::min(std::max(in,2),result.loopOut);
    }

    result.animation=std::move(anim);
    return result;
  }

  Result fromBytes(const QByteArray& bytes,const AnimFile::Animation& target)
  {
    const AnimImport::Result read=AnimImport::parse(bytes);
    if(!read.ok())
    {
      Result result;
      result.error=QObject::tr("This file could not be opened.\n%1\nCheck that it is a Second Life .anim file.").arg(read.error);
      return result;
    }
    Result result=convert(*read.motion,target);
    if(result.ok() && !read.warnings.isEmpty())
      result.notes=read.warnings+result.notes;
    return result;
  }
}
