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

#ifdef __APPLE__
#include <QApplication.h>
#endif


#include <QHash>

#include <iostream>
#include <stdio.h>
#include <string.h>

// #include "main.h"
#include "logging.h"
#include "bvh.h"
#include "constants.h"
#include "usersettings.h"

#include "animation.h"

Animation::Animation(BVH* newBVH, const QString& bvhFile, const QByteArray* data)
  : frame(0),
    totalFrames(0),
    mirrored(false)
{

  // IK starts off for every limb (setIK() below compares with the old value)
  for(int i=0;i<NUM_IK;i++) ikOn[i]=false;

  bvh=newBVH;
  if(!bvh)
  {
    qCWarning(lcAnimation,"Animation(): BVH initialisation failed.");
    return;
  }

  QString fileName;

  // load BVH that defines motion
  if (!bvhFile.isEmpty())
    fileName=bvhFile;
  else
    fileName=Constants::defaultPosePath();

  loadBVH(fileName,bvhFile.isEmpty() ? nullptr : data);
  calcPartMirrors();
  setNumberOfFrames(bvh->lastLoadedNumberOfFrames);
  setAvatarScale(bvh->lastLoadedAvatarScale);
  setFigureType(bvh->lastLoadedFigureType);
  setLoopInPoint(bvh->lastLoadedLoopIn);
  setLoopOutPoint(bvh->lastLoadedLoopOut);
  setFrameTime(bvh->lastLoadedFrameTime);
  positionNode=bvh->lastLoadedPositionNode;
  m_extensions=bvh->lastLoadedExtensions;
  addKeyFrameAllJoints();

  ikTree.set(frames);
  setIK(IK_LHAND, false);
  setIK(IK_RHAND, false);
  setIK(IK_LFOOT, false);
  setIK(IK_RFOOT, false);

  setDirty(false);

  setPlaystate(PLAYSTATE_STOPPED);
}

Animation::~Animation()
{

  // remove rotation keyframes
  if(frames) bvh->bvhDelete(frames);
  // remove position keyframes
  if(positionNode) delete positionNode;
}

void Animation::loadBVH(const QString& bvhFile,const QByteArray* data)
{

  // read the Bento reference skeleton first, as reading a file resets the
  // lastLoaded... values that are used after loading the animation
  BVHNode* reference=bvh->animRead(Constants::femaleBvhPath(),Constants::limitsFilePath());
  if(!bvh->lastLimitsProblem.title.isEmpty()) m_loadProblems << bvh->lastLimitsProblem;
  delete bvh->lastLoadedPositionNode;
  bvh->lastLoadedPositionNode=NULL;

  if(!reference)
    qWarning("Animation::loadBVH(): reference skeleton %s: %s",Constants::femaleBvhPath().toLatin1().constData(),
             bvh->lastError.toLatin1().constData());

  BVHNode* loaded=bvh->animRead(bvhFile,Constants::limitsFilePath(),data);
  if(!bvh->lastLimitsProblem.title.isEmpty()) m_loadProblems << bvh->lastLimitsProblem;
  // files the user opens are checked before (BVH::checkFile); this is for the program's own data files
  if(!loaded)
    m_loadProblems << LoadProblem{QObject::tr("Animation file"),
                                  QObject::tr("<qt>%1 could not be read:<br>%2</qt>").arg(bvhFile.toHtmlEscaped(),bvh->lastError.toHtmlEscaped())};

  // put the animation on the full skeleton, so old files get the Bento joints too
  frames=bvh->conformToSkeleton(loaded,reference);
  if(frames!=loaded) bvh->bvhDelete(loaded);
  bvh->bvhDelete(reference);

  setFrame(0);
}

bool Animation::saveBVH(const QString& bvhFile, QString* error)
{
  QString message;
  if(!bvh->animWrite(this,bvhFile,message))
  {
    if(error) *error=message;
    return false;
  }
  setDirty(false);
  return true;
}

QString Animation::poseBvhText(int frameNumber)
{
  if(!frames) return QString();
  return bvh->bvhPoseText(this,frameNumber);
}

QJsonObject Animation::poseQuaternions(int frameNumber)
{
  if(!frames) return QJsonObject();
  return bvh->bvhPoseQuaternions(this,frameNumber);
}

int Animation::fps() const
{
  if(frames)
    return framesPerSecond;
  else
    return 0;
}

void Animation::setFPS(int fps)
{
  if(frames)
  {
    framesPerSecond=fps;
    setDirty(true);
  }
}

// convenience
void Animation::setFrameTime(double frameTime)
{
  framesPerSecond=qRound(1.0/frameTime);
}

// convenience
double Animation::frameTime() const
{
  return 1.0/framesPerSecond;
}

int Animation::getNumberOfFrames()
{
  return totalFrames;
}

void Animation::setNumberOfFrames(int num)
{
  const int oldLast=totalFrames-1;
  totalFrames=num;
  // a loop that ended on the last frame keeps ending on the last frame, and
  // loop points past the new last frame move back onto it (issue #189);
  // oldLast is -1 while the constructor reads the file, before any loop is set
  if(oldLast>=0 && loopOutPoint>=0 && num>0)
  {
    if(loopOutPoint>=oldLast || loopOutPoint>num-1) loopOutPoint=num-1;
    if(loopInPoint>loopOutPoint) loopInPoint=loopOutPoint;
  }
  setDirty(true);
  emit numberOfFrames(num);
}

int Animation::getFrame()
{
  return frame;
}

void Animation::setFrame(int frameNumber)
{
  if(frameNumber>=0 && frameNumber<totalFrames && frame!=frameNumber)
  {
    frame=frameNumber;
    solveIKIfOn();

    emit currentFrame(frame);
    emit frameChanged();
  }
}

void Animation::setMirrored(bool mirror)
{
  mirrored=mirror;
}

bool Animation::getMirrored()
{
  return mirrored;
}

// get next frame and take care of looping
int Animation::stepForward()
{
  if(frames)
  {
    int nextFrame=(frame+1) % totalFrames;

    if(playstate == PLAYSTATE_LOOPING)
    {
      if(!nextFrame || nextFrame>loopOutPoint) nextFrame=loopInPoint;
    }
    else
    {
      // do not wrap to the beginning when not looping
      if(!nextFrame) nextFrame=frame;
    }
    setFrame(nextFrame);

    return nextFrame;
  }
  return 0;
}

void Animation::setEaseIn(BVHNode* node,int frameNum,bool state)
{
  if(!node)
  {
    qCWarning(lcAnimation,"setEaseIn(): node==0!");
    return;
  }

  if(node->isKeyframe(frameNum))
  {
    setDirty(true);
    node->setEaseIn(frameNum,state);
    // tell main class that the keyframe has changed
    emit redrawTrack(getPartIndex(node));
  }
}

void Animation::setEaseOut(BVHNode* node,int frameNum,bool state)
{
  if(!node)
  {
    qCWarning(lcAnimation,"setEaseOut(): node==0!");
    return;
  }

  if(node->isKeyframe(frameNum))
  {
    setDirty(true);
    node->setEaseOut(frameNum,state);
    // tell main class that the keyframe has changed
    emit redrawTrack(getPartIndex(node));
  }
}

void Animation::setCurve(BVHNode* node,int frameNum,const std::optional<Easing::Preset>& curve)
{
  if(!node)
  {
    qCWarning(lcAnimation,"setCurve(): node==0!");
    return;
  }

  if(node->isKeyframe(frameNum) && node->curve(frameNum)!=curve)
  {
    setDirty(true);
    node->setCurve(frameNum,curve);
    // tell main class that the keyframe has changed
    emit redrawTrack(getPartIndex(node));
  }
}

std::optional<Easing::Preset> Animation::curve(BVHNode* node,int frameNum)
{
  if(!node) return std::nullopt;
  return node->curve(frameNum);
}

void Animation::setCustomCurve(BVHNode* node,int frameNum,const std::optional<Easing::Bezier>& curve)
{
  if(!node)
  {
    qCWarning(lcAnimation,"setCustomCurve(): node==0!");
    return;
  }

  const std::optional<Easing::Bezier> stored=curve ? std::optional<Easing::Bezier>(Easing::sanitized(*curve)) : std::nullopt;
  if(node->isKeyframe(frameNum) && node->customCurve(frameNum)!=stored)
  {
    setDirty(true);
    node->setCustomCurve(frameNum,stored);
    // tell main class that the keyframe has changed
    emit redrawTrack(getPartIndex(node));
  }
}

std::optional<Easing::Bezier> Animation::customCurve(BVHNode* node,int frameNum)
{
  if(!node) return std::nullopt;
  return node->customCurve(frameNum);
}

bool Animation::easeIn(BVHNode* node,int frameNum)
{
  if(!node)
  {
    qCWarning(lcAnimation,"easeIn(): node==0!");
    return false;
  }

  if(node->isKeyframe(frameNum))
  {
    return node->easeIn(frameNum);
  }
  qCWarning(lcAnimation,"easeIn(%s): requested easeIn for non-keyframe!",node->name().toLatin1().constData());
  return false;
}

bool Animation::easeOut(BVHNode* node,int frameNum)
{
  if(!node)
  {
    qCWarning(lcAnimation,"easeOut(): node==0!");
    return false;
  }

  if(node->isKeyframe(frameNum))
  {
    return node->easeOut(frameNum);
  }
  qCWarning(lcAnimation,"easeOut(%s): requested easeOut for non-keyframe!",node->name().toLatin1().constData());
  return false;
}

void Animation::setLoopInPoint(int inFrame)
{
  loopInPoint=inFrame;
  setDirty(true);
}

int Animation::getLoopInPoint()
{
  return loopInPoint;
}

void Animation::setLoopOutPoint(int outFrame)
{
  loopOutPoint=outFrame;
  setDirty(true);
}

int Animation::getLoopOutPoint()
{
  return loopOutPoint;
}

void Animation::applyIK(const QString& name)
{
  BVHNode* node=bvh->bvhFindNode(frames,name);

  QVector3D rotation = node->frameData(frame).rotation();

  if(node)
  {
    rotation += node->ikRotation;


    node->ikRotation.setX(0);
    node->ikRotation.setY(0);
    node->ikRotation.setZ(0);

    setDirty(true);
    addKeyFrame(node);
    node->setKeyframeRotation(frame, rotation);
    emit redrawTrack(getPartIndex(node));
//    }
  }
}

void Animation::setIK(IKPartType part,bool flag)
{
  if(ikOn[part]==flag) return;

  ikOn[part]=flag;

  if(flag)
  {
    switch(part)
    {
      case IK_LHAND: ikTree.setGoal(frame,"lHand"); break;
      case IK_RHAND: ikTree.setGoal(frame,"rHand"); break;
      case IK_LFOOT: ikTree.setGoal(frame,"lFoot"); break;
      case IK_RFOOT: ikTree.setGoal(frame,"rFoot"); break;
      default: break;
    }
  }
  else
  {
    switch(part)
    {
      case IK_LHAND:
        applyIK("lHand");
        applyIK("lForeArm");
        applyIK("lShldr");
        applyIK("lCollar");
        if(!ikOn[IK_RHAND])
        {
          applyIK("chest");
          applyIK("abdomen");
        }
        break;
      case IK_RHAND:
        applyIK("rHand");
        applyIK("rForeArm");
        applyIK("rShldr");
        applyIK("rCollar");
        if (!ikOn[IK_LHAND])
        {
          applyIK("chest");
          applyIK("abdomen");
        }
        break;
      case IK_LFOOT:
        applyIK("lThigh");
        applyIK("lShin");
        applyIK("lFoot");
        break;
      case IK_RFOOT:
        applyIK("rThigh");
        applyIK("rShin");
        applyIK("rFoot");
        break;
      default:
        break;
    }
  }
}

void Animation::setIK(BVHNode* node,bool flag)
{
  QString jointName=node->name();

  if(jointName=="lHand" ||
     jointName=="lForeArm" ||
     jointName=="lShldr" ||
     jointName=="lCollar")
  {
    setIK(IK_LHAND, flag);
  }
  else if(jointName=="rHand" ||
          jointName=="rForeArm" ||
          jointName=="rShldr" ||
          jointName=="rCollar")
  {
    setIK(IK_RHAND, flag);
  }
  else if(jointName=="lThigh" ||
          jointName=="lShin" ||
          jointName=="lFoot")
  {
    setIK(IK_LFOOT, flag);
  }
  else if(jointName=="rThigh" ||
          jointName=="rShin" ||
          jointName=="rFoot")
  {
    setIK(IK_RFOOT, flag);
  }
}

bool Animation::getIK(BVHNode* node)
{
  QString jointName=node->name();

  if(jointName=="lHand" ||
     jointName=="lForeArm" ||
     jointName=="lShldr" ||
     jointName=="lCollar")
  {
    return getIK(IK_LHAND);
  }
  else if(jointName=="rHand" ||
          jointName=="rForeArm" ||
          jointName=="rShldr" ||
          jointName=="rCollar")
  {
    return getIK(IK_RHAND);
  }
  else if(jointName=="lThigh" ||
          jointName=="lShin" ||
          jointName=="lFoot")
  {
    return getIK(IK_LFOOT);
  }
  else if(jointName=="rThigh" ||
          jointName=="rShin" ||
          jointName=="rFoot")
  {
    return getIK(IK_RFOOT);
  }
  return false;
}

bool Animation::getIK(IKPartType part)
{
  return ikOn[part];
}

// solves IK when it is on for any hand or foot
void Animation::solveIKIfOn()
{
  for(int i=0;i<NUM_IK;i++)
  {
    if(ikOn[i])
    {
      solveIK();
      return;
    }
  }
}

void Animation::solveIK()
{
  bvh->bvhResetIK(frames);
  if(ikOn[IK_LFOOT]) getEndSite("lFoot")->ikOn=true;
  if(ikOn[IK_RFOOT]) getEndSite("rFoot")->ikOn=true;
  if(ikOn[IK_LHAND]) getEndSite("lHand")->ikOn=true;
  if(ikOn[IK_RHAND]) getEndSite("rHand")->ikOn=true;

  ikTree.solve(frame);
}

void Animation::setRotation(BVHNode* node,double x,double y,double z)
{
  if (node)
  {

    if(node->isKeyframe(frame))
      node->setKeyframeRotation(frame, QVector3D(x, y, z));
    else
    {
      node->addKeyframe(frame,node->frameData(frame).position(), QVector3D(x, y, z));
      setEaseIn(node,frame,UserSettings::easeIn());
      setEaseOut(node,frame,UserSettings::easeOut());
    }

    BVHNode* mirrorNode=node->getMirror();
    if(mirrored && mirrorNode)
    {
      // new keyframe system
      if(mirrorNode->isKeyframe(frame))
        mirrorNode->setKeyframeRotation(frame, QVector3D(x, -y, -z));
      else
      {
        mirrorNode->addKeyframe(frame,node->frameData(frame).position(), QVector3D(x, -y, -z));
        setEaseIn(mirrorNode,frame,UserSettings::easeIn());
        setEaseOut(mirrorNode,frame,UserSettings::easeOut());
      }

      // tell timeline that this mirrored keyframe has changed (added or changed is the same here)
      emit redrawTrack(getPartIndex(mirrorNode));
    }
    // solve IK only now, for the rotation just stored (#128)
    solveIKIfOn();
    setDirty(true);
    // tell timeline that this keyframe has changed (added or changed is the same here)
    emit redrawTrack(getPartIndex(node));
    emit frameChanged();
  }
  else
  {
    qCWarning(lcAnimation,"setRotation(): node==0!");
  }
}

QVector3D Animation::getRotation(BVHNode* node)
{
  if(node)
  {
    return node->frameData(frame).rotation();
  }
  qCWarning(lcAnimation,"getRotation(): node==0!");
  return QVector3D();
}

RotationLimits Animation::getRotationLimits(BVHNode* node)
{
  if(node)
  {
    double xMin,yMin,zMin,xMax,yMax,zMax;

    if(node->type==BVH_POS)
    {
      xMin=yMin=zMin=xMax=yMax=zMax=0;
    }
    else if(UserSettings::jointLimits())
    {
      bvh->bvhGetChannelLimits(node,BVH_XROT,&xMin,&xMax);
      bvh->bvhGetChannelLimits(node,BVH_YROT,&yMin,&yMax);
      bvh->bvhGetChannelLimits(node,BVH_ZROT,&zMin,&zMax);
    }
    else
    {
      xMin=yMin=zMin=-180;
      xMax=yMax=zMax=180;
    }

    RotationLimits rotLimit;
    rotLimit.minimum = QVector3D(xMin, yMin, zMin);
    rotLimit.maximum = QVector3D(xMax, yMax, zMax);
    return rotLimit;
  }
  qCWarning(lcAnimation,"getRotationLimits(): node==0!");
  return RotationLimits();
}

void Animation::setPosition(double x,double y,double z)
{
  // new keyframe system
  if(positionNode->isKeyframe(frame))
    positionNode->setKeyframePosition(frame, QVector3D(x, y, z));
  else
  {
    positionNode->addKeyframe(frame, QVector3D(x, y, z), QVector3D());
    setEaseIn(positionNode,frame,UserSettings::easeIn());
    setEaseOut(positionNode,frame,UserSettings::easeOut());
  }
  // solve IK only now, for the position just stored (#128)
  solveIKIfOn();
  setDirty(true);
  // tell timeline that this keyframe has changed (added or changed is the same here)
  emit redrawTrack(0);
  emit frameChanged();
}

QVector3D Animation::getPosition()
{
  return positionNode->frameData(frame).position();
}

float Animation::standingLift()
{
  return StandingHipHeight-float(positionNode->frameData(0).position().y());
}

const QString Animation::getPartName(int index) const
{
  //  exception fot position pseudonode
  if(index==0) return positionNode->name();
  return bvh->bvhGetName(frames,index);
}

int Animation::getPartIndex(BVHNode* node)
{
  if(node==positionNode) return 0;
  return bvh->bvhGetIndex(frames,node->name());
}

BVHNode* Animation::getMotion()
{
  return frames;
}

BVHNode* Animation::getEndSite(const QString& rootName)
{
  BVHNode* node=bvh->bvhFindNode(frames,rootName);
  while(node && node->numChildren()>0)
  {
    node=node->child(0);
  }
  return node;
}

void Animation::recursiveAddKeyFrame(BVHNode* joint)
{
  if(joint->type!=BVH_END)
    addKeyFrame(joint);

  for(int i=0;i<joint->numChildren();i++)
    recursiveAddKeyFrame(joint->child(i));

  setDirty(true);
}

void Animation::addKeyFrameAllJoints()
{
  addKeyFrame(getNode(0));
  recursiveAddKeyFrame(frames);
}

void Animation::addKeyFrame(BVHNode* joint)
{
  // a key frame that is set again keeps its stored easing choice (issue #53):
  // opening a file sets frame 0 of every joint again
  // (its ease switches too: they hold the choice of the four old-style tiles, so
  // a file opened and saved again must not lose them at frame 0)
  const bool existing=joint->isKeyframe(frame);
  const bool easeIn=existing ? joint->easeIn(frame) : UserSettings::easeIn();
  const bool easeOut=existing ? joint->easeOut(frame) : UserSettings::easeOut();
  const std::optional<Easing::Preset> curve=joint->curve(frame);
  const std::optional<Easing::Bezier> customCurve=joint->customCurve(frame);
  joint->addKeyframe(frame,getPosition(),getRotation(joint));

  setEaseIn(joint,frame,easeIn);
  setEaseOut(joint,frame,easeOut);
  joint->setCurve(frame,curve);
  joint->setCustomCurve(frame,customCurve);

  setDirty(true);

  emit redrawTrack(getPartIndex(joint));
  emit frameChanged();
}

bool Animation::isKeyFrameHelper(BVHNode *joint)
{
  if(joint->isKeyframe(frame))
    return true;

  for (int i=0;i<joint->numChildren();i++)
  {
    if(isKeyFrameHelper(joint->child(i)))
    {
      return true;
    }
  }

  return false;
}

bool Animation::isKeyFrame(const QString& jointName)
{
  if(jointName.isEmpty())
     return isKeyFrame();
  else
  {
    BVHNode* node=bvh->bvhFindNode(frames,jointName);
    return node->isKeyframe(frame);
  }
  qCWarning(lcAnimation,"isKeyFrame('%s'): no node found.",jointName.toLatin1().constData());
}

bool Animation::isKeyFrame()
{
  if(positionNode->isKeyframe(frame)) return true;
  return isKeyFrameHelper(frames);
}

bool Animation::isKeyFrame(int jointNumber,int frameNumber)
{
  const BVHNode* joint=getNode(jointNumber);
  return joint->isKeyframe(frameNumber);
}

void Animation::deleteKeyFrame(BVHNode* joint,int frameNum,bool silent)
{
  // never delete first keyframe
  if(frameNum)
  {
    joint->deleteKeyframe(frameNum);
    setDirty(true);
  }

  // if silent is true then only send a signal to the timeline but not to the animation view
  if(!silent) emit frameChanged();
  emit redrawTrack(getPartIndex(joint));
}

void Animation::deleteKeyFrame(int jointNumber,int frameNum)
{
  // frames should always be current frame, but better play safe for future enhancements
  setFrame(frameNum);
  if(jointNumber)
  {
     BVHNode* joint=getNode(jointNumber);
     if(joint->isKeyframe(frameNum)) deleteKeyFrame(joint,frameNum);
  }
  else if(isKeyFrame()) deleteKeyFrameAllJoints();

  // tell main class that the keyframe has changed
  emit currentFrame(frameNum);
}

void Animation::recursiveDeleteKeyFrame(BVHNode* joint)
{
  deleteKeyFrame(joint,frame);

  for(int i=0;i<joint->numChildren();i++)
    recursiveDeleteKeyFrame(joint->child(i));
}

void Animation::deleteKeyFrameAllJoints()
{
  // never delete the first keyframe
  if(frame==0) return;
  deleteKeyFrame(getNode(0),frame);
  recursiveDeleteKeyFrame(frames);
}

bool Animation::toggleKeyFrame(BVHNode* node)
{
  if(!node)
    return toggleKeyFrameAllJoints();
  else
  {
    if (node->isKeyframe(frame))
    {
      deleteKeyFrame(node,frame);
      return false;
    }
    else
    {
      addKeyFrame(node);
      return true;
    }
  }
}

// returns TRUE if frame is now a keyframe for entire animation, FALSE if not
bool Animation::toggleKeyFrameAllJoints()
{
  if(frame==0)
    return true;  // first frame will always stay keyframe

  if(isKeyFrame())
  {
    deleteKeyFrameAllJoints();
    return false;
  }
  else
  {
    addKeyFrameAllJoints();
    return true;
  }
}

void Animation::cutFrame()
{
  // copy frame data into copy buffer
  copyFrame();
  // always delete frame from all tracks
  deleteFrame(0,frame);
}

void Animation::copyFrame()
{
  bvh->bvhGetFrameData(frames,frame);
}

void Animation::pasteFrame()
{
  bvh->bvhSetFrameData(frames,frame);
  addKeyFrameAllJoints();
}

void Animation::calcPartMirrors()
{
  QString name;
  QString n;

  // start at node index 1
  int i=1;
  // go through all nodes by index, stop if a node has no name (=does not exist =end of list)
  while(!(n=bvh->bvhGetName(frames,i)).isEmpty())
  {
    // find node by name
    BVHNode* node=bvh->bvhFindNode(frames,n);
    // copy the name
    name=n;
    // Bento bones: ...Left <-> ...Right (mHandIndex1Left, mFaceEar2Right)
    if(n.endsWith("Left")) name=n.left(n.length()-4)+"Right";
    else if(n.endsWith("Right")) name=n.left(n.length()-5)+"Left";
    // classic joints: first letter r becomes l, otherwise first letter becomes r
    else if(n.startsWith("r")) name[0]='l';
    else name[0]='r';

    // check if mirrored name is valid, get the node with that name
    BVHNode* m=bvh->bvhFindNode(frames,name);
    if(m)
    {
      // name was valid, record this node as mirror of the current node
      // keep the index number for une in AnimationView later
      node->setMirror(m,bvh->bvhGetIndex(frames,m->name()));
    }
    // next node
    i++;
  }
}

// copies the position and rotation data of one body part to another key frame position
void Animation::copyKeyFrame(int jointNumber,int from,int to)
{
  // move keyframe in copy mode
  moveKeyFrame(jointNumber,from,to,true);
}

// moves the position and rotation data of one body part to another key frame position
void Animation::moveKeyFrame(int jointNumber,int from,int to,bool copy)
{

  // make sure we don't drag a trail of mirror keys behind
  setMirrored(false);

  // set frame pointer to source frame position
  setFrame(from);

  // get the joint structure
  BVHNode* joint=getNode(jointNumber);
  const FrameData& frameData=joint->frameData(from);

  // block all further signals to avoid flickering
  blockSignals(true);

  // silently (true) delete key frame if not copy mode
  // we do copy mode here to avoid code duplication
  if(!copy) deleteKeyFrame(joint,from,true);

  setFrame(to);

  // move rotation or position of the body part
  if(joint->type==BVH_POS)
  {
    QVector3D position = frameData.position();
    setPosition(position.x(), position.y(), position.z());
  }
  else
  {
    QVector3D rotation = frameData.rotation();
    setRotation(joint, rotation.x(), rotation.y(), rotation.z());
  }
  // only now set ease in/out, because setRotation/setPosition sets to default when the
  // target position has no keyframe yet
  joint->setEaseIn(to,frameData.easeIn());
  joint->setEaseOut(to,frameData.easeOut());
  joint->setCurve(to,frameData.curve());
  joint->setCustomCurve(to,frameData.customCurve());
  // now re-enable signals so we get updates on screen
  blockSignals(false);
  // the change mark (and Undo, src/undohistory.h) hear of the move only now
  setDirty(true);

  // tell timeline where we are now
  emit currentFrame(frame);
}

bool Animation::compareFrames(const BVHNode* node,int key1,int key2) const
{
  if(node) return node->compareFrames(key1,key2);
  qCWarning(lcAnimation,"compareFrames(node): node==0!");
  return false;
}

const FrameData Animation::keyframeDataByIndex(int jointNumber,int index)
{
  BVHNode* joint=getNode(jointNumber);
  return joint->keyframeDataByIndex(index);
}

BVHNode* Animation::getNode(int jointNumber)
{
  //####
  // joint number 0 needs to return the hip position pseudonode
  if(jointNumber==0) return positionNode;
  // get the joint structure
  return bvh->bvhFindNode(frames,getPartName(jointNumber));
}

void Animation::insertFrameHelper(BVHNode* joint,int frameNumber)
{
  joint->insertFrame(frameNumber);
  for(int i=0;i<joint->numChildren();i++)
    insertFrameHelper(joint->child(i),frameNumber);
}

void Animation::insertFrame(int track,int pos)
{
  if(track==-1)
  {
    // insert positional frame
    BVHNode* joint=getNode(0);
    if(joint) joint->insertFrame(pos);
    // insert all rotational frames
    insertFrameHelper(frames,pos);
  }
  else
  {
    BVHNode* joint=getNode(track);
    if(joint) joint->insertFrame(frame);
  }
  setDirty(true);
  emit frameChanged();
}

// recursively remove frames from joint and all its children
void Animation::deleteFrameHelper(BVHNode* joint,int frameNumber)
{
  joint->deleteFrame(frameNumber);
  for(int i=0;i<joint->numChildren();i++)
    deleteFrameHelper(joint->child(i),frameNumber);
  emit redrawTrack(getPartIndex(joint));
}

// delete frame from a joint, if track==0 recursively delete from all joints
void Animation::deleteFrame(int track,int pos)
{

  if(track==-1)
  {
    // delete positional frame
    BVHNode* joint=getNode(0);
    if(joint) joint->deleteFrame(pos);
    // delete all rotational frames
    deleteFrameHelper(frames,pos);
  }
  else
  {
    BVHNode* joint=getNode(track);
    if(joint) joint->deleteFrame(pos);
  }
  setDirty(true);
  emit frameChanged();
}

void Animation::optimizeHelper(BVHNode* joint)
{
  if(joint->type!=BVH_END)
  {
    joint->optimize();
    emit redrawTrack(getPartIndex(joint));
  }

  for(int i=0;i<joint->numChildren();i++)
    optimizeHelper(joint->child(i));
}

void Animation::optimize()
{
  optimizeHelper(positionNode);
  optimizeHelper(frames);
  setDirty(true);
}

void Animation::mirrorHelper(BVHNode* joint)
{
  // make sure only to mirror one side of l/r joints, and joints that have no mirror node
  if(!joint->name().startsWith("l"))
  {
    joint->mirror();
    emit redrawTrack(getPartIndex(joint));
    if(joint->getMirror())
      emit redrawTrack(joint->getMirrorIndex());
  }

  for(int i=0;i<joint->numChildren();i++)
    mirrorHelper(joint->child(i));
}

void Animation::mirror(BVHNode* joint)
{
  if(!joint)
  {
    positionNode->mirror();
    mirrorHelper(frames);
  }
  else
  {
    joint->mirror();
    emit redrawTrack(getPartIndex(joint));
    if(joint->getMirror())
      emit redrawTrack(joint->getMirrorIndex());
  }
  setDirty(true);
}

void Animation::forEachJoint(BVHNode* joint,const std::function<void(BVHNode*)>& fn)
{
  if(joint->type!=BVH_END) fn(joint);

  for(int i=0;i<joint->numChildren();i++)
    forEachJoint(joint->child(i),fn);
}

void Animation::flipPose()
{
  // read every joint's current rotation first: a joint's target can be its
  // mirror partner's rotation, which must still be the one before any joint
  // in this walk was changed
  QHash<BVHNode*,QVector3D> rotations;
  forEachJoint(frames,[&](BVHNode* joint) { rotations.insert(joint,getRotation(joint)); });

  for(auto it=rotations.constBegin();it!=rotations.constEnd();++it)
  {
    BVHNode* joint=it.key();
    BVHNode* mirrorNode=joint->getMirror();
    const QVector3D source=mirrorNode ? rotations.value(mirrorNode) : it.value();
    // a joint that never moved from the rest pose stays that way (#53-style
    // "only what changed"), instead of a new keyframe on every joint
    if(source.isNull()) continue;
    setRotation(joint,source.x(),-source.y(),-source.z());
  }
  setDirty(true);
}

void Animation::tPose()
{
  forEachJoint(frames,[&](BVHNode* joint) { setRotation(joint,0.0,0.0,0.0); });
  setDirty(true);
}

void Animation::resetAllToFrameZero()
{
  forEachJoint(frames,[&](BVHNode* joint)
  {
    const QVector3D rest=joint->frameData(0).rotation();
    setRotation(joint,rest.x(),rest.y(),rest.z());
  });
  setDirty(true);
}

QList<BVHNode*> Animation::partNodes() const
{
  QList<BVHNode*> nodes;
  if(positionNode) nodes.append(positionNode);
  std::function<void(BVHNode*)> walk=[&](BVHNode* node)
  {
    if(!node) return;
    nodes.append(node);
    for(int i=0;i<node->numChildren();i++) walk(node->child(i));
  };
  walk(frames);
  return nodes;
}

Animation::Snapshot Animation::snapshot() const
{
  Snapshot state;
  for(const BVHNode* node : partNodes()) state.keys.append(node->keyframeMap());
  state.frames=totalFrames;
  state.fps=framesPerSecond;
  state.loopIn=loopInPoint;
  state.loopOut=loopOutPoint;
  return state;
}

void Animation::restore(const Snapshot& state)
{
  const QList<BVHNode*> nodes=partNodes();
  // a snapshot of another skeleton cannot be put back
  if(nodes.count()!=state.keys.count()) return;
  QList<int> changed;
  for(int i=0;i<nodes.count();i++)
  {
    if(nodes.at(i)->keyframeMap()==state.keys.at(i)) continue;
    nodes.at(i)->setKeyframeMap(state.keys.at(i));
    changed.append(i);
  }
  const bool framesChanged=totalFrames!=state.frames;
  totalFrames=state.frames;
  framesPerSecond=state.fps;
  loopInPoint=state.loopIn;
  loopOutPoint=state.loopOut;
  if(frame>=totalFrames) frame=qMax(0,totalFrames-1);

  if(framesChanged) emit numberOfFrames(totalFrames);
  for(int track : changed) emit redrawTrack(track);
  emit frameChanged();
}

bool Animation::dirty() const
{
  return isDirty;
}

void Animation::setDirty(bool state)
{
  isDirty=state;
  emit animationDirty(state);
}

float Animation::getAvatarScale()
{
  return avatarScale;
}

void Animation::setAvatarScale(float newScale)
{
  avatarScale=newScale;
}

Animation::FigureType Animation::getFigureType()
{
  return figureType;
}

void Animation::setFigureType(FigureType type)
{
  // safety check if figure is valid
  if(type>=0 && type<NUM_FIGURES)
    figureType=type;
}

void Animation::nextPlaystate()
{
}

void Animation::setPlaystate(PlayState state)
{
  playstate=state;
}
