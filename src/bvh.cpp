/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Copyright (C) 2006 by Vinay Pulim.
 * All rights reserved.
 *
 */

#include <QCoreApplication>
#include <QQuaternion>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QHash>

#include <climits>

#include "animfile.h"
#include "logging.h"
#include "bvh.h"
#include "constants.h"
#include "dingle.h"
#include "skeletonwalk.h"

BVH::BVH()
{
  bvhTypeName.append("POS");
  bvhTypeName.append("ROOT");
  bvhTypeName.append("JOINT");
  bvhTypeName.append("End");
  bvhTypeName.append("NoSL");
  bvhChannelName.append("Xposition");
  bvhChannelName.append("Yposition");
  bvhChannelName.append("Zposition");
  bvhChannelName.append("Xrotation");
  bvhChannelName.append("Yrotation");
  bvhChannelName.append("Zrotation");

  validNodes << "hip" << "abdomen" << "chest" << "neck" << "head"
             << "lCollar" << "lShldr" << "lForeArm" << "lHand"
             << "rCollar" << "rShldr" << "rForeArm" << "rHand"
             << "lThigh" << "lShin" << "lFoot"
             << "rThigh" << "rShin" << "rFoot"
             << "Site";

  loadBentoJoints();
}

// read the list of Bento joints and their aliases (bin/data/bento_joints.txt)
void BVH::loadBentoJoints()
{
  QFile file(Constants::pinglyMotionDataPath()+"/bento_joints.txt");
  if(!file.open(QIODevice::ReadOnly))
  {
    qCWarning(lcFile,"loadBentoJoints(): bento_joints.txt not found, Bento joints disabled");
    return;
  }

  while(!file.atEnd())
  {
    const QString line=QString::fromLatin1(file.readLine()).trimmed();
    if(line.isEmpty() || line.startsWith('#')) continue;

    const QStringList names=line.split(' ',Qt::SkipEmptyParts);
    for(int i=0;i<names.count();i++)
    {
      if(!validNodes.contains(names[i])) validNodes << names[i];
      jointAliases.insert(names[i],names[0]);
    }
  }
}

int BVH::countNodes(BVHNode* node)
{
  if(!node) return 0;
  int count=1;
  for(int i=0;i<node->numChildren();i++)
    count+=countNodes(node->child(i));
  return count;
}

// find the joint in the loaded animation that belongs to a skeleton joint,
// also when the file uses another name for it (e.g. mPelvis instead of hip)
BVHNode* BVH::findLoadedNode(BVHNode* loaded,const QString& name) const
{
  if(!loaded) return NULL;
  if(loaded->type!=BVH_END)
  {
    const QString mapped=jointAliases.value(loaded->name(),loaded->name());
    if(mapped==name) return loaded;
  }
  for(int i=0;i<loaded->numChildren();i++)
  {
    BVHNode* found=findLoadedNode(loaded->child(i),name);
    if(found) return found;
  }
  return NULL;
}

BVHNode* BVH::copySkeletonNode(BVHNode* reference,BVHNode* loaded)
{
  BVHNode* node=new BVHNode(reference->name());
  node->type=reference->type;
  for(int i=0;i<3;i++) node->offset[i]=reference->offset[i];
  node->numChannels=reference->numChannels;
  for(int i=0;i<6;i++)
  {
    node->channelType[i]=reference->channelType[i];
    node->channelMin[i]=reference->channelMin[i];
    node->channelMax[i]=reference->channelMax[i];
  }
  node->channelOrder=reference->channelOrder;
  node->ikOn=false;
  node->ikWeight=reference->ikWeight;

  if(reference->type!=BVH_END)
  {
    // copy all key frames from the joint with the same name in the loaded file
    BVHNode* source=findLoadedNode(loaded,reference->name());
    if(source)
    {
      const QList<int> keys=source->keyframeList();
      for(int k=0;k<keys.count();k++)
      {
        const FrameData data=source->frameData(keys[k]);
        node->addKeyframe(keys[k],data.position(),data.rotation());
        node->setEaseIn(keys[k],data.easeIn());
        node->setEaseOut(keys[k],data.easeOut());
        node->setCurve(keys[k],data.curve());
        node->setCustomCurve(keys[k],data.customCurve());
      }
    }
    // joints the file does not have get a neutral key frame at the start
    if(node->numKeyframes()==0)
      node->addKeyframe(0,QVector3D(),QVector3D());
  }

  for(int i=0;i<reference->numChildren();i++)
    node->addChild(copySkeletonNode(reference->child(i),loaded));

  return node;
}

BVHNode* BVH::conformToSkeleton(BVHNode* loaded,BVHNode* reference)
{
  if(!loaded || !reference) return loaded;
  return copySkeletonNode(reference,loaded);
}

BVH::~ BVH()
{
}

bool BVH::failed(const QString& reason)
{
  // the first problem is the one worth telling
  if(lastError.isEmpty())
  {
    lastError=reason;
    qCWarning(lcFile,"BVH: cannot read the file: %s",reason.toLatin1().constData());
  }
  return false;
}

void BVH::removeNoSLNodes(BVHNode* root)
{
  // walk through list of all child nodes
  for(int i=0;i<root->numChildren();i++)
  {
    BVHNode* child=root->child(i);
    // if this is an unsupported node, remove it
    if(child->type==BVH_NO_SL)
    {
      qCDebug(lcFile,"removeNoSLNodes(): removing node '%s'",child->name().toLatin1().constData());
      // find all child joints of the unsupported child
      for(int j=0;j<child->numChildren();j++)
      {
        // move child nodes to the current parent joint
        root->insertChild(child->child(j),i);
      }
      // remove unsupported child node
      root->removeChild(child);

      // start checking for nodes at this point over again
      removeNoSLNodes(root);
      return;
    }
    // check next parent node
    removeNoSLNodes(root->child(i));
  }
}

void BVH::setChannelLimits(BVHNode *node,BVHChannelType type,double min,double max) const
{

  int i;
  if(!node) return;
  for(i=0;i<node->numChannels;i++)
  {
    if(node->channelType[i]==type)
    {
      node->channelMin[i]=min;
      node->channelMax[i]=max;
      return;
    }
  }
}

// read joint limits file
// Returns what went wrong (empty title: nothing); the caller shows it.
Animation::LoadProblem BVH::parseLimFile(BVHNode* root,const QString& limFile) const
{

  QFile limit(limFile);

  if(!limit.open(QIODevice::ReadOnly))
  {
    return {QObject::tr("Missing Limits File"),QObject::tr("<qt>Limits file not found at:<br>%1</qt>").arg(limFile)};
  }

  while(!limit.atEnd())
  {
    QString line=limit.readLine(4096).trimmed();
    if(line.isEmpty())
    {
      return {QObject::tr("Error reading limits file"),QObject::tr("Error reading limits file.")};
    }

    QStringList parameters=line.split(' ');
    // name, weight, then 3 x (axis, minimum, maximum)
    if(parameters.count()<11)
    {
      qWarning("BVH::parseLimFile(): skipping a line with %d values, expected 11",(int) parameters.count());
      continue;
    }
    QString name=parameters[0];
    double weight=parameters[1].toDouble();

    BVHNode* node=bvhFindNode(root,name);
    if(node)
    {
      node->ikWeight=weight;

      for(int i=0;i<3;i++)
      {
        QString channel=parameters[i*3+2];
        double min=parameters[i*3+3].toDouble();
        double max=parameters[i*3+4].toDouble();

        if     (channel.startsWith("X")) setChannelLimits(node,BVH_XROT,min,max);
        else if(channel.startsWith("Y")) setChannelLimits(node,BVH_YROT,min,max);
        else if(channel.startsWith("Z")) setChannelLimits(node,BVH_ZROT,min,max);
      } // for
    }
    else
      qCWarning(lcFile,"parseLimFile(): Node '%s' not in animation. This will lead to problems!",name.toLatin1().constData());
  }
  limit.close();
  return {};
}

/* .avm files look suspiciously like .bvh files, except
   with keyframe data tacked at the end -- Lex Neva */
// what every way of reading a file starts with: the position pseudo node and
// the defaults for everything a file may leave out
void BVH::startRead()
{
  lastLimitsProblem=Animation::LoadProblem();
  // positions pseudonode
  lastLoadedPositionNode=new BVHNode("position");
  lastLoadedPositionNode->type=BVH_POS;
  // default avatar scale for BVH and AVM files
  lastLoadedAvatarScale=1.0;
  // default figure type
  lastLoadedFigureType=Animation::FIGURE_FEMALE;
  // indicates "no loop points set"; loop out is set to the last frame below
  lastLoadedLoopIn=-1;
  lastLoadedLoopOut=-1;
  // reset the error state
  lastError.clear();

  // assume old style animation format for compatibility
  havePositionKeys=false;
}

// what every way of reading a file ends with, once the file's tree is built
BVHNode* BVH::finishRead(BVHNode* root,const QString& limFile)
{
  if(!root)
  {
    if(lastError.isEmpty()) failed(QObject::tr("The file could not be read."));
    return NULL;
  }

  // a BVH has no loop points, and an older AVM may not have them: loop out
  // is then the last frame (it was left over from an earlier file, #91)
  if(lastLoadedLoopOut<0 || lastLoadedLoopOut>lastLoadedNumberOfFrames)
    lastLoadedLoopOut=lastLoadedNumberOfFrames;

  lastLimitsProblem=Animation::LoadProblem();
  if(!limFile.isEmpty())
    lastLimitsProblem=parseLimFile(root,limFile);

  removeNoSLNodes(root);

  // old style animation or BVH format means we need to add position keyframes ourselves
  if(!havePositionKeys)
  {
    for(int index=0;index<root->numKeyframes();index++)
    {
      const FrameData& frameData=root->keyframeDataByIndex(index);
      int frameNum=frameData.frameNumber();
      lastLoadedPositionNode->addKeyframe(frameNum, frameData.position(), QVector3D());
      lastLoadedPositionNode->setEaseIn(frameNum,frameData.easeIn());
      lastLoadedPositionNode->setEaseOut(frameNum,frameData.easeOut());
      lastLoadedPositionNode->setCurve(frameNum,frameData.curve());
      lastLoadedPositionNode->setCustomCurve(frameNum,frameData.customCurve());
    }
  }

  return root;
}

// builds the joint tree from what the parser read (src/animfile.*)
BVHNode* BVH::buildTree(const AnimFile::Animation& anim,bool storedPositions)
{
  const bool avm=(anim.format==AnimFile::Format::Avm);
  QVector<BVHNode*> nodes(anim.joints.size(),nullptr);

  for(int i=0;i<anim.joints.size();i++)
  {
    const AnimFile::Joint& joint=anim.joints[i];
    BVHNode* node=new BVHNode(joint.name);
    nodes[i]=node;

    // joints that are not part of the Second Life skeleton are removed later
    if(!validNodes.contains(node->name()))
      node->type=BVH_NO_SL;
    else if(joint.kind==AnimFile::JointKind::Root)
      node->type=BVH_ROOT;
    else if(joint.kind==AnimFile::JointKind::EndSite)
      node->type=BVH_END;
    else
      node->type=BVH_JOINT;

    for(int k=0;k<3;k++) node->offset[k]=joint.offset[k];
    node->ikOn=false;
    node->ikWeight=0.0;

    // the parser allows at most AnimFile::MAX_CHANNELS (6) channels
    node->numChannels=qMin(int(joint.channels.size()),AnimFile::MAX_CHANNELS);
    for(int k=0;k<node->numChannels;k++)
    {
      node->channelType[k]=static_cast<BVHChannelType>(joint.channels[k]);
      node->channelMin[k]=-10000;
      node->channelMax[k]=10000;
    }
    node->channelOrder=joint.rotationOrder();

    if(joint.parent>=0 && joint.parent<i)
      nodes[joint.parent]->addChild(node);

    if(!avm)
    {
      // in a BVH file every frame is a key frame (End Sites have none)
      if(node->type!=BVH_END)
        for(int frame=0;frame<anim.frames;frame++)
          node->addKeyframe(frame,anim.position(i,frame),anim.rotation(i,frame));
      continue;
    }

    // AVM: every joint, End Sites too, gets its key frames; frame 0 is always one
    for(const AnimFile::Key& key : joint.keys)
      node->addKeyframe(key.frame,anim.position(i,key.frame),anim.rotation(i,key.frame));
    // the root's first frame is also the first position key frame
    if(node->type==BVH_ROOT)
      lastLoadedPositionNode->addKeyframe(0,anim.position(i,0),anim.rotation(i,0));
    for(const AnimFile::Key& key : joint.keys)
    {
      if(key.easeIn) node->setEaseIn(key.frame,true);
      if(key.easeOut) node->setEaseOut(key.frame,true);
      if(key.curve) node->setCurve(key.frame,key.curve);
      if(key.customCurve) node->setCustomCurve(key.frame,key.customCurve);
    }
  }

  BVHNode* root=nodes.isEmpty() ? nullptr : nodes[0];
  if(!root) return nullptr;

  if(avm && anim.hasPositionKeys)
  {
    // In a .dingle the position of a position key frame is the stored position
    // of that frame: the file has its own position track, so a position key
    // without a root rotation key at the same frame keeps its value (#188).
    // In an .avm it is taken from the root joint's key frames, interpolated
    // between them when the position key is not also a root key frame -
    // exactly like the old parser (open question in #62).
    for(const AnimFile::Key& key : anim.positionKeys)
      lastLoadedPositionNode->addKeyframe(key.frame,
                                          storedPositions ? anim.position(0,key.frame) : root->frameData(key.frame).position(),
                                          QVector3D());
    for(const AnimFile::Key& key : anim.positionKeys)
    {
      if(!lastLoadedPositionNode->isKeyframe(key.frame)) continue;
      if(key.easeIn) lastLoadedPositionNode->setEaseIn(key.frame,true);
      if(key.easeOut) lastLoadedPositionNode->setEaseOut(key.frame,true);
      if(key.curve) lastLoadedPositionNode->setCurve(key.frame,key.curve);
      if(key.customCurve) lastLoadedPositionNode->setCustomCurve(key.frame,key.customCurve);
    }
  }

  return root;
}

// opens a file with the parser in src/animfile.* (issue #62), or a .dingle
// file with the reader in src/dingle.* (issue #26)
BVHNode* BVH::animRead(const QString& file,const QString& limFile,const QByteArray* given)
{
  startRead();

  const bool dingle=file.endsWith(QLatin1String(".dingle"),Qt::CaseInsensitive);
  const std::optional<AnimFile::Format> format=AnimFile::formatForFileName(file);
  if(!format && !dingle)
  {
    failed(QObject::tr("Only .dingle, .bvh and .avm files can be opened."));
    return NULL;
  }
  const qsizetype maxBytes=dingle ? Dingle::MAX_FILE_BYTES : AnimFile::MAX_FILE_BYTES;

  QByteArray data;
  if(given)
    data=*given;
  else
  {
    QFile animationFile(file);
    if(!animationFile.open(QIODevice::ReadOnly))
    {
      failed(QObject::tr("The file could not be opened: %1").arg(animationFile.errorString()));
      return NULL;
    }
    if(animationFile.size()>maxBytes)
    {
      failed(QObject::tr("The file is larger than %1 MB.").arg(maxBytes/(1024*1024)));
      return NULL;
    }
    data=animationFile.read(maxBytes+1);
    if(animationFile.error()!=QFileDevice::NoError)
    {
      failed(QObject::tr("The file could not be read: %1").arg(animationFile.errorString()));
      return NULL;
    }
    animationFile.close();
  }
  if(data.size()>maxBytes)
  {
    failed(QObject::tr("The file is larger than %1 MB.").arg(maxBytes/(1024*1024)));
    return NULL;
  }

  AnimFile::Animation anim;
  int dingleIgnoredKeys=0;
  bool dingleNewerVersion=false;
  if(dingle)
  {
    // a parsed .dingle is an AVM-style animation (keys, position keys, loop)
    Dingle::Result result=Dingle::parse(data);
    if(!result.ok())
    {
      failed(result.error.isEmpty() ? QObject::tr("The file could not be read.") : result.error);
      return NULL;
    }
    dingleIgnoredKeys=result.ignoredKeys;
    dingleNewerVersion=result.newerVersion;
    anim=std::move(*result.animation);
  }
  else
  {
    AnimFile::Result result=AnimFile::parse(data,*format);
    if(!result.ok())
    {
      if(result.line>0)
        failed(QObject::tr("Line %1: %2").arg(result.line).arg(result.error));
      else
        failed(result.error);
      return NULL;
    }
    anim=std::move(*result.animation);
  }

  lastLoadedNumberOfFrames=anim.frames;
  lastLoadedFrameTime=anim.frameTime;
  lastLoadedIgnoredKeys=dingleIgnoredKeys;
  lastLoadedNewerVersion=dingleNewerVersion;
  if(anim.format==AnimFile::Format::Avm)
  {
    havePositionKeys=anim.hasPositionKeys;
    lastLoadedAvatarScale=anim.avatarScale;
    lastLoadedFigureType=static_cast<Animation::FigureType>(anim.figure);
    lastLoadedLoopIn=anim.loopIn;
    lastLoadedLoopOut=anim.loopOut;
  }

  return finishRead(buildTree(anim,dingle),limFile);
}

BVH::CheckResult BVH::checkFile(const QString& file,const QByteArray* data)
{
  BVH test;
  BVHNode* root=test.animRead(file,QString(),data);
  delete test.lastLoadedPositionNode;
  // a failed read always gives a reason, so ok() cannot say yes to a file that was refused
  if(!root) return {test.lastError.isEmpty() ? QCoreApplication::translate("BVH","The file could not be read.") : test.lastError};
  test.bvhDelete(root);
  return {};
}

// The pose of one frame for the live link, as a small BVH: the first frame is neutral
// (no rotation, the hip where frame 0 has it) so that the receiver sees every
// joint of the pose as a change, as Second Life does with a BVH upload; then the pose twice.
QString BVH::bvhPoseText(Animation* anim,int frame)
{
  AnimFile::Animation model=snapshot(anim,AnimFile::Format::Bvh);
  if(frame<0 || frame>=model.frames) return QString();

  const size_t perFrame=size_t(model.valuesPerFrame);
  std::vector<float> values;
  values.reserve(3*perFrame);
  // frame 0 without rotations
  for(const AnimFile::Joint& joint : model.joints)
  {
    for(int k=0;k<joint.channels.size();k++)
    {
      const AnimFile::Channel channel=joint.channels[k];
      const bool position=channel==AnimFile::Channel::XPosition || channel==AnimFile::Channel::YPosition ||
                          channel==AnimFile::Channel::ZPosition;
      values.push_back(position ? model.values[size_t(joint.firstValue+k)] : 0.0f);
    }
  }
  for(int i=0;i<2;i++)
    values.insert(values.end(),model.values.begin()+std::ptrdiff_t(size_t(frame)*perFrame),
                  model.values.begin()+std::ptrdiff_t((size_t(frame)+1)*perFrame));

  model.values=values;
  model.frames=3;
  model.frameTime=1.0;
  return QString::fromUtf8(AnimFile::write(model));
}

// One joint's rotation: the channels in file order, each new axis rotation multiplied
// on the right (same as the 3D view, the IK and the .anim export), then BVH axes (Y up, Z forward,
// X sideways) turned into Second Life's avatar axes (X forward, Y left, Z up).
void BVH::bvhPoseNodeQuaternions(BVHNode* node,int frame,QJsonObject& rotations)
{
  if(node->type!=BVH_END && node->numChannels>0)
  {
    const QVector3D rotation=node->frameData(frame).rotation();
    QQuaternion q;
    bool hasRotation=false;
    for(int i=0;i<node->numChannels;i++)
    {
      const BVHChannelType type=node->channelType[i];
      if(type==BVH_XROT) { q=q*QQuaternion::fromAxisAndAngle(1,0,0,rotation.x()); hasRotation=true; }
      else if(type==BVH_YROT) { q=q*QQuaternion::fromAxisAndAngle(0,1,0,rotation.y()); hasRotation=true; }
      else if(type==BVH_ZROT) { q=q*QQuaternion::fromAxisAndAngle(0,0,1,rotation.z()); hasRotation=true; }
    }
    if(hasRotation)
    {
      q.normalize();
      // vector part (x,y,z) in BVH axes is (z,x,y) in Second Life axes
      rotations[node->name()]=QJsonArray({q.z(),q.x(),q.y(),q.scalar()});
    }
  }
  for(int i=0;i<node->numChildren();i++)
    bvhPoseNodeQuaternions(node->child(i),frame,rotations);
}

QJsonObject BVH::bvhPoseQuaternions(Animation* anim,int frame)
{
  BVHNode* root=anim->getMotion();
  QJsonObject rotations;
  bvhPoseNodeQuaternions(root,frame,rotations);
  // the hip moves relative to frame 0, like the neutral frame of the pose BVH; inches -> metres
  BVHNode* position=anim->getNode(0);
  const QVector3D offset=(position->frameData(frame).position()-position->frameData(0).position())*0.0254f;
  QJsonObject positions;
  positions[root->name()]=QJsonArray({offset.z(),offset.x(),offset.y()});
  QJsonObject pose;
  pose["rotations"]=rotations;
  pose["positions"]=positions;
  return pose;
}

namespace
{
  // writes to a temporary file and only replaces the real one when everything was written
  bool openForWriting(QSaveFile& f, QString& error)
  {
    if(f.open(QIODevice::WriteOnly)) return true;
    error=QObject::tr("Could not write %1: %2").arg(QDir::toNativeSeparators(f.fileName()),f.errorString());
    return false;
  }
}

namespace
{
  // writes the bytes to a temporary file and only replaces the real one when
  // everything was written
  bool saveBytes(const QString& file,const QByteArray& bytes,QString& error)
  {
    QSaveFile f(file);
    if(!openForWriting(f,error)) return false;
    if(f.write(bytes)!=bytes.size() || !f.commit())
    {
      error=QObject::tr("Could not write %1: %2").arg(QDir::toNativeSeparators(file),f.errorString());
      f.cancelWriting();
      return false;
    }
    return true;
  }

  void collectJoints(BVHNode* node,int parent,int depth,QVector<BVHNode*>& nodes,AnimFile::Animation& anim)
  {
    AnimFile::Joint joint;
    joint.name=node->name();
    joint.kind=node->type==BVH_ROOT ? AnimFile::JointKind::Root
             : node->type==BVH_END ? AnimFile::JointKind::EndSite
             : AnimFile::JointKind::Joint;
    joint.parent=parent;
    joint.depth=depth;
    for(int k=0;k<3;k++) joint.offset[k]=node->offset[k];
    for(int k=0;k<node->numChannels;k++)
      joint.channels.append(static_cast<AnimFile::Channel>(node->channelType[k]));
    joint.firstValue=anim.valuesPerFrame;
    anim.valuesPerFrame+=int(joint.channels.size());

    const int index=int(anim.joints.size());
    anim.joints.append(joint);
    nodes.append(node);
    for(int i=0;i<node->numChildren();i++)
      collectJoints(node->child(i),index,depth+1,nodes,anim);
  }

  // key frames below the number of frames; frame 0 is kept as the first one
  QVector<AnimFile::Key> keysOf(BVHNode* node,int frames)
  {
    QVector<AnimFile::Key> keys;
    const QList<int> list=node->keyframeList();
    for(int i=0;i<list.size();i++)
    {
      if(i>0 && list[i]>=frames) break;
      AnimFile::Key key;
      key.frame=list[i];
      key.easeIn=node->keyframeDataByIndex(i).easeIn();
      key.easeOut=node->keyframeDataByIndex(i).easeOut();
      key.curve=node->keyframeDataByIndex(i).curve();
      key.customCurve=node->keyframeDataByIndex(i).customCurve();
      keys.append(key);
    }
    return keys;
  }
}

AnimFile::Animation BVH::snapshot(Animation* anim,AnimFile::Format format)
{
  AnimFile::Animation model;
  model.format=format;
  model.frames=anim->getNumberOfFrames();
  model.frameTime=anim->frameTime();

  BVHNode* root=anim->getMotion();
  BVHNode* position=anim->getNode(0);
  QVector<BVHNode*> nodes;
  collectJoints(root,-1,0,nodes,model);

  // the hip position is kept in a node of its own: every position channel gets it from there
  model.values.assign(size_t(model.frames)*size_t(model.valuesPerFrame),0.0f);
  for(int frame=0;frame<model.frames;frame++)
  {
    const QVector3D hip=position->frameData(frame).position();
    for(int i=0;i<nodes.size();i++)
    {
      const AnimFile::Joint& joint=model.joints[i];
      if(joint.channels.isEmpty()) continue;
      const QVector3D rotation=nodes[i]->frameData(frame).rotation();
      for(int k=0;k<joint.channels.size();k++)
      {
        float value=0.0f;
        switch(joint.channels[k])
        {
          case AnimFile::Channel::XPosition: value=hip.x(); break;
          case AnimFile::Channel::YPosition: value=hip.y(); break;
          case AnimFile::Channel::ZPosition: value=hip.z(); break;
          case AnimFile::Channel::XRotation: value=rotation.x(); break;
          case AnimFile::Channel::YRotation: value=rotation.y(); break;
          case AnimFile::Channel::ZRotation: value=rotation.z(); break;
        }
        model.values[size_t(frame)*size_t(model.valuesPerFrame)+size_t(joint.firstValue+k)]=value;
      }
    }
  }

  if(format==AnimFile::Format::Avm)
  {
    for(int i=0;i<nodes.size();i++)
      model.joints[i].keys=keysOf(nodes[i],model.frames);
    model.avatarScale=anim->getAvatarScale();
    model.figure=int(anim->getFigureType());
    model.loopIn=anim->getLoopInPoint();
    model.loopOut=anim->getLoopOutPoint();
    model.hasPositionKeys=true;
    model.positionKeys=keysOf(position,model.frames);
  }
  return model;
}

bool BVH::bvhWrite(Animation* anim,const QString& file,QString& error)
{
  return saveBytes(file,AnimFile::write(snapshot(anim,AnimFile::Format::Bvh)),error);
}

bool BVH::dingleWrite(Animation* anim,const QString& file,QString& error)
{
  return saveBytes(file,Dingle::write(snapshot(anim,AnimFile::Format::Avm)),error);
}

bool BVH::animWrite(Animation* anim,const QString& file,QString& error)
{
  // rudimentary file type identification from filename
  if(file.endsWith(".bvh",Qt::CaseInsensitive))
    return bvhWrite(anim,file,error);
  if(file.endsWith(".dingle",Qt::CaseInsensitive))
    return dingleWrite(anim,file,error);
  if(file.endsWith(".avm",Qt::CaseInsensitive))
  {
    error=QObject::tr("Old .avm files are not saved any more. Save as .dingle instead.");
    return false;
  }
  error=QObject::tr("The file name must end in .dingle or .bvh.");
  return false;
}

BVHNode* BVH::bvhFindNode(BVHNode* root,const QString& name) const
{
  return SkeletonWalk::findNode(root,name);
}

void BVH::bvhGetChannelLimits(BVHNode* node,BVHChannelType type,double* min, double* max)
{
  int i;
  if(!node)
  {
    *min=-10000;
    *max=10000;
    return;
  }
  for(i=0;i<node->numChannels;i++)
  {
    if(node->channelType[i]==type)
    {
      *min=node->channelMin[i];
      *max=node->channelMax[i];
    }
  }
}

void BVH::bvhResetIK(BVHNode* root)
{
  int i;
  if(root)
  {
    root->ikOn=false;
    for(i=0;i<root->numChildren();i++)
    {
      bvhResetIK(root->child(i));
    }
  }
}

const QString BVH::bvhGetNameHelper(BVHNode* node,int index)
{
  nodeCount++;
  if(nodeCount==index) return node->name();

  for(int i=0;i<node->numChildren();i++)
  {
    const QString val=bvhGetNameHelper(node->child(i),index);
    if(!val.isEmpty()) return val;
  }
  return QString();
}

const QString BVH::bvhGetName(BVHNode* node,int index)
{
  nodeCount=0;
  return bvhGetNameHelper(node, index);
}

int BVH::bvhGetIndexHelper(BVHNode* node,const QString& name)
{
  nodeCount++;
  if(node->name()==name) return nodeCount;

  for(int i=0;i<node->numChildren();i++)
  {
    int val;
    if((val=bvhGetIndexHelper(node->child(i),name)))
      return val;
  }
  return 0;
}

int BVH::bvhGetIndex(BVHNode* node,const QString& name)
{
  nodeCount=0;
  return bvhGetIndexHelper(node,name);
}

void BVH::bvhGetFrameDataHelper(BVHNode* node,int frame)
{
  if(node->type!=BVH_END)
  {
    rotationCopyBuffer.append(node->frameData(frame).rotation());

//    rotationCopyBuffer[rotationCopyBuffer.count()-1].bodyPart=node->name(); // not necessary but nice for debugging
  }


  for(int i=0;i<node->numChildren();i++)
    bvhGetFrameDataHelper(node->child(i),frame);
}

void BVH::bvhGetFrameData(BVHNode* node,int frame)
{
  if(!node) return;

  rotationCopyBuffer.clear();
  positionCopyBuffer=lastLoadedPositionNode->frameData(frame).position();
  bvhGetFrameDataHelper(node,frame);
}

void BVH::bvhSetFrameDataHelper(BVHNode* node,int frame)
{
  // if this node is not the end of a joint child structure
  if(node->type!=BVH_END)
  {
    // add the node as key frame
    node->addKeyframe(frame, QVector3D(), rotationCopyBuffer[pasteIndex]);
    // increment paste buffer counter
    pasteIndex++;
  }

  // traverse down the child list, call this function recursively
  for(int i=0;i<node->numChildren();i++)
    bvhSetFrameDataHelper(node->child(i),frame);
}

void BVH::bvhSetFrameData(BVHNode* node,int frame)
{
  if(!node) return;

  // reset paste buffer counter
  pasteIndex=0;
  lastLoadedPositionNode->addKeyframe(frame, positionCopyBuffer, QVector3D());
  // paste all keyframes for all joints
  bvhSetFrameDataHelper(node,frame);
}

void BVH::bvhDelete(BVHNode* node)
{
  if(node)
  {
    for(int i=0;i<node->numChildren();i++)
      bvhDelete(node->child(i));

    delete node;
  }
}
