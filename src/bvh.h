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

#ifndef BVH_H
#define BVH_H

#include <QtCore>
#include <QTextStream>

#include "bvhnode.h"
#include "animation.h"
#include "animfile.h"

class BVHNode;
class Animation;
namespace AnimFile { struct Animation; }

class BVH
{
  public:

    BVH();
    ~BVH();

    void setChannelLimits(BVHNode* node,BVHChannelType type,double min,double max) const;
    void parseLimFile(BVHNode* root,const QString& limFile) const;

    void setNumFrames(int numFrames);
    int numFrames() const;
    void bvhIndent(QTextStream& out,int depth);
    void bvhWriteNode(BVHNode* node,QTextStream& out,int depth);
    void bvhPrintNode(BVHNode* n, int depth);

    // the write functions return false and set error when the file could not be written;
    // an existing file is then left as it was
    bool bvhWrite(Animation* root,const QString& file,QString& error);
    // what is written to a file: the animation as the plain data model of animfile.h
    AnimFile::Animation snapshot(Animation* anim,AnimFile::Format format);
    // one frame as a small BVH for the Aurora link: neutral frame, then the pose twice
    QString bvhPoseText(Animation* anim,int frame);
    // the pose of one frame for Aurora's "pose" message: {"rotations":{joint:[x,y,z,w]},
    // "positions":{"hip":[x,y,z]}}, in Second Life's avatar axes (X forward, Y left, Z up),
    // rotations relative to the rest pose, the hip offset from frame 0 in metres
    QJsonObject bvhPoseQuaternions(Animation* anim,int frame);
    void bvhPoseNodeQuaternions(BVHNode* node,int frame,QJsonObject& rotations);
    void bvhWritePoseFrame(BVHNode* node,QTextStream& out,int frame,bool neutral);
    BVHNode* bvhFindNode(BVHNode* root,const QString& name) const;

    void bvhGetChannelLimits(BVHNode* node, BVHChannelType type, double *min, double *max);
    void bvhResetIK(BVHNode* root);

    const QString bvhGetName(BVHNode* node,int index);
    int bvhGetIndex(BVHNode* node,const QString& name);
    void bvhCopyOffsets(BVHNode* dst,BVHNode* src);

    void bvhGetFrameData(BVHNode* node,int frame);
    void bvhSetFrameData(BVHNode* node,int frame);

    // opens a .bvh or .avm file with the new parser (src/animfile.*); returns
    // NULL with the reason in lastError when the file cannot be opened
    BVHNode* animRead(const QString& file,const QString& limFile);

    bool avmWrite(Animation* root,const QString& file,QString& error);
    bool animWrite(Animation* root,const QString& file,QString& error);
    void bvhDelete(BVHNode* node);

    // Bento: rebuild a loaded animation on the reference skeleton, so every
    // animation has the same joints in the same order
    BVHNode* conformToSkeleton(BVHNode* loaded,BVHNode* reference);
    // number of nodes (joints and End Sites) in a tree
    static int countNodes(BVHNode* node);

    // reads the whole file and returns false, with the reason in error, if it
    // is not an animation Pingmator can open; nothing is kept
    static bool checkFile(const QString& file,QString* error);

    // why the last animRead() returned NULL
    QString lastError;

    QStringList bvhTypeName;
    QStringList bvhChannelName;
    int lastLoadedNumberOfFrames;
    int lastLoadedLoopIn;
    int lastLoadedLoopOut;
    float lastLoadedAvatarScale;
    double lastLoadedFrameTime;
    Animation::FigureType lastLoadedFigureType;
    BVHNode* lastLoadedPositionNode;

    int nodeCount;

    QList<QVector3D> rotationCopyBuffer;
    QVector3D positionCopyBuffer;

  protected:
    // remember if the loaded animation is in old or new AVM format
    bool havePositionKeys;

    QStringList validNodes;
    // maps every known joint name or alias to the name used in Pingmator
    QHash<QString,QString> jointAliases;

    void loadBentoJoints();

    void startRead();
    BVHNode* finishRead(BVHNode* root,const QString& limFile);
    // the joint tree from the new parser's data model
    BVHNode* buildTree(const AnimFile::Animation& anim);
    BVHNode* copySkeletonNode(BVHNode* reference,BVHNode* loaded);
    BVHNode* findLoadedNode(BVHNode* loaded,const QString& name) const;
    BVHNode* positionNode;

    // remembers the first reason the file cannot be read; always returns false
    bool failed(const QString& reason);

    // removes all unknown nodes from the animation
    void removeNoSLNodes(BVHNode* root);

    const QString bvhGetNameHelper(BVHNode* node,int index);
    int bvhGetIndexHelper(BVHNode* node,const QString& name);

    void bvhGetFrameDataHelper(BVHNode* node,int frame);
    void bvhSetFrameDataHelper(BVHNode* node,int frame);

    int pasteIndex;
};

#endif
