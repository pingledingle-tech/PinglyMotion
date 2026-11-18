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

#include <QQuaternion>
#include <QJsonArray>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QHash>

#include <climits>

#include "animfile.h"
#include "bvh.h"
#include "constants.h"

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
  QFile file(Constants::pingmatorDataPath()+"/bento_joints.txt");
  if(!file.open(QIODevice::ReadOnly))
  {
    qDebug("BVH::loadBentoJoints(): bento_joints.txt not found, Bento joints disabled");
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

QString BVH::token()
{
  if(tokenPos>=tokens.size())
  {
    qDebug("BVH::token(): no more tokens at index %d",tokenPos);
    return QString();
  }
  return tokens[tokenPos++];
}

int BVH::tokensLeft() const
{
  return qMax(0,tokens.size()-tokenPos);
}

bool BVH::failed(const QString& reason)
{
  // the first problem is the one worth telling
  if(lastError.isEmpty())
  {
    lastError=reason;
    qDebug("BVH: cannot read the file: %s",reason.toLatin1().constData());
  }
  return false;
}

bool BVH::expect_token(const QString& name)
{
  const QString found=token();
  if(name!=found)
    return failed(QObject::tr("\"%1\" expected, found \"%2\"").arg(name,found.left(40)));
  return true;
}

bool BVH::readInt(int& value,int minimum,int maximum,const QString& what)
{
  bool ok=false;
  const QString text=token();
  value=text.toInt(&ok);
  if(!ok || value<minimum || value>maximum)
  {
    if(maximum==INT_MAX)
      return failed(QObject::tr("%1 must be a whole number of at least %2, found \"%3\"")
                      .arg(what).arg(minimum).arg(text.left(40)));
    return failed(QObject::tr("%1 must be a whole number from %2 to %3, found \"%4\"")
                    .arg(what).arg(minimum).arg(maximum).arg(text.left(40)));
  }
  return true;
}

bool BVH::readFloat(double& value,const QString& what)
{
  bool ok=false;
  const QString text=token();
  value=text.toDouble(&ok);
  if(!ok || !qIsFinite(value))
    return failed(QObject::tr("%1 must be a number, found \"%2\"").arg(what,text.left(40)));
  return true;
}

// reads the file into the token list
bool BVH::readFile(const QString& file)
{
  QFile animationFile(file);
  if(!animationFile.open(QIODevice::ReadOnly))
    return failed(QObject::tr("The file could not be opened: %1").arg(animationFile.errorString()));
  if(animationFile.size()>MAX_FILE_BYTES)
    return failed(QObject::tr("The file is larger than %1 MB.").arg(MAX_FILE_BYTES/(1024*1024)));

  inputFile=animationFile.readAll();
  animationFile.close();

  inputFile=inputFile.simplified();
  tokens=inputFile.split(' ');
  return true;
}

// returns NULL at the end of the child list ("}") and on errors; lastError
// tells the two apart
BVHNode* BVH::bvhReadNode(int depth)
{
  qDebug("BVH::bvhReadNode()");
  if(!lastError.isEmpty()) return NULL;

  QString type=token();
  if(type=="}") return NULL;

  // check for node type first
  BVHNodeType nodeType;
  if      (type=="ROOT")  nodeType=BVH_ROOT;
  else if (type=="JOINT") nodeType=BVH_JOINT;
  else if (type=="End")   nodeType=BVH_END;
  else
  {
    failed(QObject::tr("Unknown joint type \"%1\".").arg(type.left(40)));
    return NULL;
  }

  // a joint per nesting level, so this also limits the recursion
  if(depth>MAX_DEPTH || ++jointsRead>MAX_JOINTS)
  {
    failed(QObject::tr("The file has more than %1 joints, or they are nested more than %2 deep.").arg(MAX_JOINTS).arg(MAX_DEPTH));
    return NULL;
  }

  // add node with name
  BVHNode* node=new BVHNode(token());
  if(!validNodes.contains(node->name()))
    node->type=BVH_NO_SL;
  else
    // set node type
    node->type=nodeType;

  double offset[3];
  if(!expect_token("{") || !expect_token("OFFSET") ||
     !readFloat(offset[0],"OFFSET") || !readFloat(offset[1],"OFFSET") || !readFloat(offset[2],"OFFSET"))
  {
    delete node;
    return NULL;
  }
  for(int i=0;i<3;i++) node->offset[i]=offset[i];
  node->ikOn=false;
  node->ikWeight=0.0;
  if(node->type!=BVH_END)
  {
    // a node holds at most 6 channels (3 positions, 3 rotations)
    int count=0;
    if(!expect_token("CHANNELS") || !readInt(count,0,6,"CHANNELS"))
    {
      delete node;
      return NULL;
    }
    node->numChannels=count;

    // rotation order for this node
    QString order;
    for(int i=0;i<node->numChannels;i++)
    {
      node->channelMin[i] = -10000;
      node->channelMax[i] = 10000;
      type=token();
      if     (type=="Xposition") node->channelType[i]=BVH_XPOS;
      else if(type=="Yposition") node->channelType[i]=BVH_YPOS;
      else if(type=="Zposition") node->channelType[i]=BVH_ZPOS;
      else if(type=="Xrotation")
      {
        node->channelType[i]=BVH_XROT;
        order+='X';
      }
      else if(type=="Yrotation")
      {
        node->channelType[i]=BVH_YROT;
        order+='Y';
      }
      else if(type=="Zrotation")
      {
        node->channelType[i]=BVH_ZROT;
        order+='Z';
      }
      else
      {
        failed(QObject::tr("Unknown channel \"%1\".").arg(type.left(40)));
        delete node;
        return NULL;
      }
    }
    channelsRead+=node->numChannels;

    if     (order=="XYZ") node->channelOrder=BVH_XYZ;
    else if(order=="ZYX") node->channelOrder=BVH_ZYX;
    else if(order=="YZX") node->channelOrder=BVH_YZX;
    else if(order=="XZY") node->channelOrder=BVH_XZY;
    else if(order=="YXZ") node->channelOrder=BVH_YXZ;
    else if(order=="ZXY") node->channelOrder=BVH_ZXY;
  }

  BVHNode* child=NULL;
  while((child=bvhReadNode(depth+1)))
    node->addChild(child);

  if(!lastError.isEmpty())
  {
    bvhDelete(node);
    return NULL;
  }
  return node;
}

void BVH::removeNoSLNodes(BVHNode* root)
{
  qDebug("BVH::removeNoSLNodes()");
  // walk through list of all child nodes
  for(int i=0;i<root->numChildren();i++)
  {
    BVHNode* child=root->child(i);
    // if this is an unsupported node, remove it
    if(child->type==BVH_NO_SL)
    {
      qDebug("BVH::removeNoSLNodes(): removing node '%s'",child->name().toLatin1().constData());
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

void BVH::assignChannels(BVHNode* node,int frame)
{
//  qDebug("BVH::assignChannels()");
  if(!lastError.isEmpty()) return;

  // create new rotation and position objects
  QVector3D* rotation = new QVector3D();
  QVector3D* position = new QVector3D();

  for(int i=0;i<node->numChannels;i++)
  {
    double value=0.0;
    if(!readFloat(value,QObject::tr("A motion value"))) break;
    BVHChannelType type=node->channelType[i];
    if     (type==BVH_XPOS) position->setX(value);
    else if(type==BVH_YPOS) position->setY(value);
    else if(type==BVH_ZPOS) position->setZ(value);
    else if(type==BVH_XROT) rotation->setX(value);
    else if(type==BVH_YROT) rotation->setY(value);
    else if(type==BVH_ZROT) rotation->setZ(value);
    else qDebug("BVH::assignChannels(): unknown channel type %d",(int) type);
  }

  // put rotation and position into the node's cache for later keyframe referencing
  node->cacheRotation(rotation);
  node->cachePosition(position);

  for(int i=0;i<node->numChildren();i++)
    assignChannels(node->child(i),frame);
}

void BVH::setChannelLimits(BVHNode *node,BVHChannelType type,double min,double max) const
{
  qDebug("BVH::setChannelLimits()");

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
void BVH::parseLimFile(BVHNode* root,const QString& limFile) const
{
  qDebug("BVH::parseLimFile('%s')",limFile.toLatin1().constData());

  QFile limit(limFile);

  if(!limit.open(QIODevice::ReadOnly))
  {
    QMessageBox::critical(0,QObject::tr("Missing Limits File"),QObject::tr("<qt>Limits file not found at:<br>%1</qt>").arg(limFile));
    return;
  }

  while(!limit.atEnd())
  {
    QString line=limit.readLine(4096).trimmed();
    if(line.isEmpty())
    {
      QMessageBox::critical(0,QObject::tr("Error reading limits file"),QObject::tr("Error reading limits file."));
      return;
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
      qDebug("BVH::parseLimFile(): Node '%s' not in animation. This will lead to problems!",name.toLatin1().constData());
  }
  limit.close();
}

// in BVH files, this is necessary so that
// all frames but the start and last aren't
// blown away by interpolation
void BVH::setAllKeyFramesHelper(BVHNode* node,int numberOfFrames) const
{
//  qDebug("BVH::setAllKeyFramesHelper()");

  for(int i=0;i<numberOfFrames;i++)
  {
    const QVector3D* rotation = node->getCachedRotation(i);
    const QVector3D* position = node->getCachedPosition(i);

    if(node->type!=BVH_END)
      node->addKeyframe(i, QVector3D(position->x(), position->y(), position->z()), QVector3D(rotation->x(), rotation->y(), rotation->z()));
  }

  for(int i=0;i<node->numChildren();i++)
    setAllKeyFramesHelper(node->child(i),numberOfFrames);
}

// MOTION section: frame count, frame time and one value per channel per frame
bool BVH::readMotion(BVHNode* root)
{
  int totalFrames=0;
  double frameTime=0.0;
  if(!expect_token("MOTION") || !expect_token("Frames:") || !readInt(totalFrames,1,MAX_FRAMES,"Frames") ||
     !expect_token("Frame") || !expect_token("Time:") || !readFloat(frameTime,"Frame Time"))
    return false;
  if(frameTime<0.001 || frameTime>10.0)
    return failed(QObject::tr("Frame Time must be from 0.001 to 10 seconds."));
  // check that the values are there before making room for them
  if(static_cast<qint64>(totalFrames)*channelsRead>tokensLeft())
    return failed(QObject::tr("The file says it has %1 frames, but has fewer motion values.").arg(totalFrames));

  lastLoadedNumberOfFrames=totalFrames;
  lastLoadedFrameTime=frameTime;

  for(int i=0;i<totalFrames && lastError.isEmpty();i++)
    assignChannels(root,i);
  return lastError.isEmpty();
}

BVHNode* BVH::bvhRead(const QString& file)
{
  qDebug("BVH::bvhRead('%s')",file.toLatin1().constData());

  if(!readFile(file) || !expect_token("HIERARCHY")) return NULL;

  BVHNode* root=bvhReadNode(0);
  if(!root)
  {
    failed(QObject::tr("The file has no joints."));
    return NULL;
  }
  if(!readMotion(root))
  {
    bvhDelete(root);
    return NULL;
  }

  setAllKeyFramesHelper(root,lastLoadedNumberOfFrames);

  return(root);
}

bool BVH::avmReadKeyFrame(BVHNode* root)
{
  // NOTE: new system needs frame 0 as key frame
  // FIXME: find a better way without code duplication
  const QVector3D* rotation = root->getCachedRotation(0);
  const QVector3D* position = root->getCachedPosition(0);
  root->addKeyframe(0, QVector3D(position->x(), position->y(), position->z()), QVector3D(rotation->x(), rotation->y(), rotation->z()));
  if(root->type==BVH_ROOT)
    lastLoadedPositionNode->addKeyframe(0, QVector3D(position->x(), position->y(), position->z()), QVector3D(rotation->x(), rotation->y(), rotation->z()));

  int numKeyFrames=0;
  if(!readInt(numKeyFrames,0,tokensLeft(),QObject::tr("The number of key frames"))) return false;
  for(int i=0;i<numKeyFrames;i++)
  {
    int key=0;
    if(!readInt(key,0,INT_MAX,QObject::tr("A key frame number"))) return false;

    // key frames past the end are left out, as before
    if(key<lastLoadedNumberOfFrames)
    {
      const QVector3D* rotation = root->getCachedRotation(key);
      const QVector3D* position = root->getCachedPosition(key);
      root->addKeyframe(key, QVector3D(position->x(), position->y(), position->z()), QVector3D(rotation->x(), rotation->y(), rotation->z()));
    }
  }

  // all keyframes are found, flush the node's cache to free up memory
  root->flushFrameCache();

  for(int i=0;i<root->numChildren();i++)
    if(!avmReadKeyFrame(root->child(i))) return false;
  return true;
}

// reads ease in / out data
bool BVH::avmReadKeyFrameProperties(BVHNode* root)
{
  // NOTE: key frame properties save key 0, too, so numKeyFrames here will be one higher than before

  qDebug("BVH::avmReadKeyFrameProperties()");

  if(tokensLeft()==0) return true;

  int numKeyFrames=0;
  if(!readInt(numKeyFrames,0,tokensLeft(),QObject::tr("The number of key frame properties"))) return false;
  qDebug("BVH::avmReadKeyFrameProperties(): reading properties for %d key frames",numKeyFrames);

  for(int i=0;i<numKeyFrames;i++)
  {
    int key=0;
    if(!readInt(key,0,3,QObject::tr("A key frame property"))) return false;

    // properties for key frames the joint does not have are left out
    if(i>=root->numKeyframes()) continue;
    if(key & 1) root->setEaseIn(root->keyframeDataByIndex(i).frameNumber(),true);
    if(key & 2) root->setEaseOut(root->keyframeDataByIndex(i).frameNumber(),true);
  }

  for(int i=0;i<root->numChildren();i++)
    if(!avmReadKeyFrameProperties(root->child(i))) return false;
  return true;
}

// for debugging only
void BVH::dumpNodes(BVHNode* node,QString indent)
{
  qDebug("%s %s (%d)",indent.toLatin1().constData(),node->name().toLatin1().constData(),node->numChildren());
  indent+="+--";
  for(int i=0;i<node->numChildren();i++)
  {
    dumpNodes(node->child(i),indent);
  }
}

/* .avm files look suspiciously like .bvh files, except
   with keyframe data tacked at the end -- Lex Neva */
BVHNode* BVH::avmRead(const QString& file)
{
  qDebug("BVH::avmRead(%s)",file.toLatin1().constData());

  if(!readFile(file) || !expect_token("HIERARCHY")) return NULL;

  BVHNode* root=bvhReadNode(0);
  if(!root)
  {
    failed(QObject::tr("The file has no joints."));
    return NULL;
  }
  if(!readMotion(root) || !avmReadKeyFrame(root))
  {
    bvhDelete(root);
    return NULL;
  }
  lastLoadedLoopOut=lastLoadedNumberOfFrames;

  // older files have no properties
  if(tokensLeft()>0 && tokens[tokenPos]=="Properties")
  {
    tokenPos++;
    if(!avmReadKeyFrameProperties(root))
    {
      bvhDelete(root);
      return NULL;
    }
  }

  // read remaining properties
  QString propertyName;
  while(lastError.isEmpty() && !(propertyName=token()).isEmpty())
  {
      QString propertyValue=token();

      if(!propertyValue.isEmpty())
      {
        qDebug("BVH::avmRead(): Found extended property: '%s=%s'",propertyName.toLatin1().constData(),
                                                                  propertyValue.toLatin1().constData());
        bool ok=false;
        if(propertyName=="Scale:")
        {
          const float scale=propertyValue.toFloat(&ok);
          if(ok && scale>0.0f && scale<=10.0f) lastLoadedAvatarScale=scale;
        }
        else if(propertyName=="Figure:")
        {
          const int figure=propertyValue.toInt(&ok);
          if(ok && (figure==Animation::FIGURE_FEMALE || figure==Animation::FIGURE_MALE))
            lastLoadedFigureType=static_cast<Animation::FigureType>(figure);
        }
        else if(propertyName=="LoopIn:")
        {
          const int frame=propertyValue.toInt(&ok);
          // -1 means "no loop points set" and is kept (#91)
          if(ok) lastLoadedLoopIn=(frame==-1) ? -1 : qBound(0,frame,lastLoadedNumberOfFrames);
        }
        else if(propertyName=="LoopOut:")
        {
          const int frame=propertyValue.toInt(&ok);
          if(ok) lastLoadedLoopOut=qBound(0,frame,lastLoadedNumberOfFrames);
        }
        else if(propertyName=="Positions:")
        {
          // remember that this is a new animation that has seperate position keyframes
          havePositionKeys=true;

          int num=propertyValue.toInt(&ok);
          if(!ok || num<0 || num>tokensLeft())
          {
            failed(QObject::tr("Bad number of position key frames."));
            break;
          }
          qDebug("Reading %d Positions:",num);
          for(int index=0;index<num;index++)
          {
            int key=0;
            if(!readInt(key,0,INT_MAX,QObject::tr("A position key frame number"))) break;
            qDebug("Reading position frame %d",key);
            // key frames past the end are left out
            if(key>=lastLoadedNumberOfFrames) continue;
            const FrameData& frameData=root->frameData(key);
            lastLoadedPositionNode->addKeyframe(key, frameData.position(), QVector3D());
          } // for
        }
        else if(propertyName=="PositionsEase:")
        {
          int num=propertyValue.toInt(&ok);
          if(!ok || num<0 || num>tokensLeft())
          {
            failed(QObject::tr("Bad number of position key frame properties."));
            break;
          }
          qDebug("Reading %d PositionsEases:",num);
          for(int index=0;index<num;index++)
          {
            int key=0;
            if(!readInt(key,0,3,QObject::tr("A position key frame property"))) break;
            qDebug("Reading position ease for key index %d: %d",index,key);

            // properties for key frames that do not exist are left out
            if(index>=lastLoadedPositionNode->numKeyframes()) continue;
            if(key & 1) lastLoadedPositionNode->setEaseIn(lastLoadedPositionNode->keyframeDataByIndex(index).frameNumber(),true);
            if(key & 2) lastLoadedPositionNode->setEaseOut(lastLoadedPositionNode->keyframeDataByIndex(index).frameNumber(),true);

          } // for
        }
        else
          qDebug("BVH::avmRead(): Unknown extended property '%s' (%s), ignoring.",propertyName.toLatin1().constData(),
                                                                                  propertyValue.toLatin1().constData());
    }
  } // while

  if(!lastError.isEmpty())
  {
    bvhDelete(root);
    return NULL;
  }
  return(root);
}

// what every way of reading a file starts with: the position pseudo node and
// the defaults for everything a file may leave out
void BVH::startRead()
{
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
  // reset token position and error state
  tokenPos=0;
  tokens.clear();
  lastError.clear();
  jointsRead=0;
  channelsRead=0;

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

  if(!limFile.isEmpty())
    parseLimFile(root,limFile);

  removeNoSLNodes(root);
//  dumpNodes(root,QString());

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
    }
  }

  return root;
}

// builds the joint tree from what the new parser read (issue #62), the same
// tree the old token parser (bvhRead()/avmRead()) builds
BVHNode* BVH::buildTree(const AnimFile::Animation& anim)
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
    node->channelOrder=static_cast<BVHOrderType>(joint.rotationOrder());

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
    }
  }

  BVHNode* root=nodes.isEmpty() ? nullptr : nodes[0];
  if(!root) return nullptr;

  if(avm && anim.hasPositionKeys)
  {
    // The position of a position key frame is taken from the root joint's key
    // frames, interpolated between them when the position key is not also a
    // root key frame - exactly like the old parser (open question in #62:
    // the motion values of that frame could be used instead).
    for(const AnimFile::Key& key : anim.positionKeys)
      lastLoadedPositionNode->addKeyframe(key.frame,root->frameData(key.frame).position(),QVector3D());
    for(const AnimFile::Key& key : anim.positionKeys)
    {
      if(!lastLoadedPositionNode->isKeyframe(key.frame)) continue;
      if(key.easeIn) lastLoadedPositionNode->setEaseIn(key.frame,true);
      if(key.easeOut) lastLoadedPositionNode->setEaseOut(key.frame,true);
    }
  }

  return root;
}

// opens a file with the new parser (src/animfile.*, issue #62)
BVHNode* BVH::animRead(const QString& file,const QString& limFile)
{
  startRead();

  const std::optional<AnimFile::Format> format=AnimFile::formatForFileName(file);
  if(!format)
  {
    failed(QObject::tr("Only .bvh and .avm files can be opened."));
    return NULL;
  }

  QFile animationFile(file);
  if(!animationFile.open(QIODevice::ReadOnly))
  {
    failed(QObject::tr("The file could not be opened: %1").arg(animationFile.errorString()));
    return NULL;
  }
  if(animationFile.size()>AnimFile::MAX_FILE_BYTES)
  {
    failed(QObject::tr("The file is larger than %1 MB.").arg(AnimFile::MAX_FILE_BYTES/(1024*1024)));
    return NULL;
  }
  const QByteArray data=animationFile.read(AnimFile::MAX_FILE_BYTES+1);
  if(animationFile.error()!=QFileDevice::NoError)
  {
    failed(QObject::tr("The file could not be read: %1").arg(animationFile.errorString()));
    return NULL;
  }
  animationFile.close();

  const AnimFile::Result result=AnimFile::parse(data,*format);
  if(!result.ok())
  {
    if(result.line>0)
      failed(QObject::tr("Line %1: %2").arg(result.line).arg(result.error));
    else
      failed(result.error);
    return NULL;
  }

  const AnimFile::Animation& anim=*result.animation;
  lastLoadedNumberOfFrames=anim.frames;
  lastLoadedFrameTime=anim.frameTime;
  if(anim.format==AnimFile::Format::Avm)
  {
    havePositionKeys=anim.hasPositionKeys;
    lastLoadedAvatarScale=anim.avatarScale;
    lastLoadedFigureType=static_cast<Animation::FigureType>(anim.figure);
    lastLoadedLoopIn=anim.loopIn;
    lastLoadedLoopOut=anim.loopOut;
  }

  return finishRead(buildTree(anim),limFile);
}

// the old token parser; not used for opening files any more (issue #62),
// kept until part 4 so the tests can compare the two
BVHNode* BVH::legacyAnimRead(const QString& file,const QString& limFile)
{
  BVHNode* root;

  startRead();

  // rudimentary file type identification from filename
  if(file.endsWith(".bvh",Qt::CaseInsensitive))
    root = bvhRead(file);
  else if(file.endsWith(".avm",Qt::CaseInsensitive))
    root = avmRead(file);
  else
  {
    failed(QObject::tr("Only .bvh and .avm files can be opened."));
    return NULL;
  }

  // the token list is not needed any more
  tokens.clear();
  inputFile.clear();

  return finishRead(root,limFile);
}

bool BVH::checkFile(const QString& file,QString* error)
{
  BVH test;
  BVHNode* root=test.animRead(file,QString());
  delete test.lastLoadedPositionNode;
  if(!root)
  {
    if(error) *error=test.lastError;
    return false;
  }
  test.bvhDelete(root);
  return true;
}

void BVH::bvhIndent(QTextStream& out,int depth)
{
  for(int i=0;i<depth;i++)
    out << "\t";
}

void BVH::bvhWriteNode(BVHNode* node,QTextStream& out,int depth)
{
  bvhIndent(out,depth);
  out << bvhTypeName[node->type] << " " << node->name() << Qt::endl;

  bvhIndent(out, depth);
  out << "{" << Qt::endl;

  bvhIndent(out,depth+1);
  out << "OFFSET " << node->offset[0] << " " << node->offset[1] << " " << node->offset[2] << Qt::endl;

  if(node->type!=BVH_END)
  {
    bvhIndent(out,depth+1);
    out << "CHANNELS " << node->numChannels << " ";
    for (int i=0;i<node->numChannels;i++)
      out << bvhChannelName[node->channelType[i]] << " ";

    out << Qt::endl;
  }
  for(int i=0;i<node->numChildren();i++)
    bvhWriteNode(node->child(i),out,depth+1);

  bvhIndent(out,depth);
  out << "}" << Qt::endl;
}

void BVH::bvhWriteFrame(BVHNode* node,QTextStream& out,int frame)
{
  QVector3D rotation = node->frameData(frame).rotation();
  QVector3D position = positionNode->frameData(frame).position();

  // preserve channel order while writing
  for (int i=0; i<node->numChannels; i++)
  {
    float value=0.0;
    BVHChannelType type=node->channelType[i];

    if     (type==BVH_XPOS) value = position.x();
    else if(type==BVH_YPOS) value = position.y();
    else if(type==BVH_ZPOS) value = position.z();

    else if(type==BVH_XROT) value = rotation.x();
    else if(type==BVH_YROT) value = rotation.y();
    else if(type==BVH_ZROT) value = rotation.z();

    out << value << " ";
  }

  for(int i=0;i<node->numChildren();i++)
    bvhWriteFrame(node->child(i),out,frame);
}

// Write one frame of the pose for the live link: the first frame is neutral
// (no rotation, the hip where frame 0 has it) so that the receiver sees every
// joint of the pose as a change, as Second Life does with a BVH upload.
void BVH::bvhWritePoseFrame(BVHNode* node,QTextStream& out,int frame,bool neutral)
{
  const QVector3D rotation = neutral ? QVector3D() : node->frameData(frame).rotation();
  const QVector3D position = positionNode->frameData(neutral ? 0 : frame).position();

  for (int i=0; i<node->numChannels; i++)
  {
    float value=0.0;
    BVHChannelType type=node->channelType[i];

    if     (type==BVH_XPOS) value = position.x();
    else if(type==BVH_YPOS) value = position.y();
    else if(type==BVH_ZPOS) value = position.z();

    else if(type==BVH_XROT) value = rotation.x();
    else if(type==BVH_YROT) value = rotation.y();
    else if(type==BVH_ZROT) value = rotation.z();

    out << value << " ";
  }

  for(int i=0;i<node->numChildren();i++)
    bvhWritePoseFrame(node->child(i),out,frame,neutral);
}

QString BVH::bvhPoseText(Animation* anim,int frame)
{
  QString text;
  QTextStream out(&text);
  out.setNumberFlags(QTextStream::ForcePoint);
  out.setRealNumberPrecision(7);

  BVHNode* root=anim->getMotion();
  positionNode=anim->getNode(0);

  out << "HIERARCHY\n";
  bvhWriteNode(root,out,0);
  out << "MOTION\n";
  out << "Frames:\t3\n";
  out << "Frame Time:\t1.0\n";
  for(int i=0;i<3;i++)
  {
    bvhWritePoseFrame(root,out,frame,i==0);
    out << "\n";
  }
  out.flush();
  return text;
}

// One joint's rotation as Aurora reads a BVH: the channels applied in their order
// (each new axis rotation multiplied in front), then BVH axes (Y up, Z forward,
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
      if(type==BVH_XROT) { q=QQuaternion::fromAxisAndAngle(1,0,0,rotation.x())*q; hasRotation=true; }
      else if(type==BVH_YROT) { q=QQuaternion::fromAxisAndAngle(0,1,0,rotation.y())*q; hasRotation=true; }
      else if(type==BVH_ZROT) { q=QQuaternion::fromAxisAndAngle(0,0,1,rotation.z())*q; hasRotation=true; }
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

  bool finishWriting(QSaveFile& f, QTextStream& out, QString& error)
  {
    out.flush();
    if(out.status()!=QTextStream::Ok || !f.commit())
    {
      error=QObject::tr("Could not write %1: %2").arg(QDir::toNativeSeparators(f.fileName()),f.errorString());
      f.cancelWriting();
      return false;
    }
    return true;
  }
}

bool BVH::bvhWrite(Animation* anim, const QString& file, QString& error)
{
  int i;
  QSaveFile f(file);
  if(!openForWriting(f,error)) return false;
  QTextStream out(&f);
  out.setNumberFlags(QTextStream::ForcePoint);
  out.setRealNumberPrecision(7);

  BVHNode* root=anim->getMotion();
  positionNode=anim->getNode(0);

  out << "HIERARCHY" << Qt::endl;
  bvhWriteNode(root,out,0);

  out << "MOTION" << Qt::endl;

  out << "Frames:\t" << anim->getNumberOfFrames() << Qt::endl;
  out << "Frame Time:\t" << anim->frameTime() << Qt::endl;

  for(i=0;i<anim->getNumberOfFrames();i++)
  {
    bvhWriteFrame(root,out,i);
    out << Qt::endl;
  }

  return finishWriting(f,out,error);
}

void BVH::avmWriteKeyFrame(BVHNode* root, QTextStream& out, int numberOfFrames)
{
  const QList<int> keys=root->keyframeList();

  int actualKeyCount = keys.count();
  // Write only keys for which frame data has been written
  for (int i = 1; i < keys.count(); i++)
  {
    if (keys[i] >= numberOfFrames)
    {
      actualKeyCount = i;
      break;
    }
  }

  // no key frames (usually at joint ends), just write a 0\n line
  if (actualKeyCount == 0)
    out << "0" << Qt::endl;

  // write line of key files
  else
  {
    // write number of key files
    out << actualKeyCount - 1 << " ";

    // skip frame 0 (always key frame) while saving and write all keys in a row
    for (unsigned int i = 1; i < (unsigned int) actualKeyCount; i++)
      out << keys[i] << " ";

    out << Qt::endl;
  }

  for(int i=0;i<root->numChildren();i++)
    avmWriteKeyFrame(root->child(i), out, numberOfFrames);
}

// writes ease in / out data
void BVH::avmWriteKeyFrameProperties(BVHNode* root, QTextStream& out, int numberOfFrames)
{
  const QList<int> keys=root->keyframeList();

  int actualKeyCount = keys.count();
  // Write only keys for which frame data has been written
  for (int i = 1; i < keys.count(); i++)
  {
    if (keys[i] >= numberOfFrames)
    {
      actualKeyCount = i;
      break;
    }
  }

  // NOTE: remember, ease in/out data always takes first frame into account
  out << actualKeyCount << " ";

  for (unsigned int i = 0; i < (unsigned int) actualKeyCount; i++)
  {
    int type=0;

    if(root->keyframeDataByIndex(i).easeIn()) type|=1;
    if(root->keyframeDataByIndex(i).easeOut()) type|=2;

    out << type << " ";
  }
  out << Qt::endl;

  for(int i=0;i<root->numChildren();i++)
    avmWriteKeyFrameProperties(root->child(i), out, numberOfFrames);
}

bool BVH::avmWrite(Animation* anim,const QString& file,QString& error)
{
  QSaveFile f(file);
  if(!openForWriting(f,error)) return false;
  QTextStream out(&f);
  out.setNumberFlags(QTextStream::ForcePoint);
  out.setRealNumberPrecision(7);

  BVHNode* root=anim->getMotion();
  positionNode=anim->getNode(0);

  out << "HIERARCHY" << Qt::endl;
  bvhWriteNode(root,out,0);

  out << "MOTION" << Qt::endl;

  out << "Frames:\t" << anim->getNumberOfFrames() << Qt::endl;

  out << "Frame Time:\t" << anim->frameTime() << Qt::endl;
  for(int i=0;i<anim->getNumberOfFrames();i++)
  {
    bvhWriteFrame(root,out,i);
    out << Qt::endl;
  }

  avmWriteKeyFrame(root, out, anim->getNumberOfFrames());
  out << "Properties" << Qt::endl;

  avmWriteKeyFrameProperties(root, out, anim->getNumberOfFrames());

  // write remaining properties
  out << "Scale: " << anim->getAvatarScale() << Qt::endl;
  out << "Figure: " << anim->getFigureType() << Qt::endl;
  out << "LoopIn: " << anim->getLoopInPoint() << Qt::endl;
  out << "LoopOut: " << anim->getLoopOutPoint() << Qt::endl;

  // HACK: add-on for position key support - this needs to be revised badly
  // HACK: we need a new file format that makes it easier to add new features
  out << "Positions: ";
  avmWriteKeyFrame(positionNode, out, anim->getNumberOfFrames());

  out << "PositionsEase: ";
  avmWriteKeyFrameProperties(positionNode, out, anim->getNumberOfFrames());

  return finishWriting(f,out,error);
}

bool BVH::animWrite(Animation* anim,const QString& file,QString& error)
{
  // rudimentary file type identification from filename
  if(file.endsWith(".bvh",Qt::CaseInsensitive))
    return bvhWrite(anim,file,error);
  if(file.endsWith(".avm",Qt::CaseInsensitive))
    return avmWrite(anim,file,error);
  error=QObject::tr("The file name must end in .bvh or .avm.");
  return false;
}

void BVH::bvhPrintNode(BVHNode* n,int depth)
{
  int i;
  QString indent;
  for(i=0;i<depth;i++)
    indent+="    ";
  qDebug() << indent << n->name() << " (" << n->offset[0] << n->offset[1] << n->offset[2] << ")";
  for(i=0;i<n->numChildren();i++)
    bvhPrintNode(n->child(i), depth+1);
}

BVHNode* BVH::bvhFindNode(BVHNode* root,const QString& name) const
{
  BVHNode *node;
  if(!root) return NULL;
  if(root->name()==name) return root;

  for(int i=0;i<root->numChildren();i++)
  {
    if((node=bvhFindNode(root->child(i),name))) return node;
  }

  return NULL;
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

void BVH::bvhCopyOffsets(BVHNode* dst,BVHNode* src)
{
  int i;
  if(!dst || !src) return;
  dst->offset[0]=src->offset[0];
  dst->offset[1]=src->offset[1];
  dst->offset[2]=src->offset[2];
  for(i=0;i<src->numChildren();i++)
  {
    bvhCopyOffsets(dst->child(i),src->child(i));
  }
}

void BVH::bvhGetFrameDataHelper(BVHNode* node,int frame)
{
  if(node->type!=BVH_END)
  {
    rotationCopyBuffer.append(node->frameData(frame).rotation());

//    rotationCopyBuffer[rotationCopyBuffer.count()-1].bodyPart=node->name(); // not necessary but nice for debugging
//    qDebug(QString("copying frame data for %1 frame number %2 (%3)")
//          .arg(node->name()).arg(frame).arg(positionCopyBuffer[rotationCopyBuffer.count()-1].bodyPart));
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
//    qDebug(QString("pasting frame data for %1 frame number %2 (%3)").arg(node->name()).arg(frame).arg(rotationCopyBuffer[pasteIndex].bodyPart));
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
