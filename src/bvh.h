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

    BVHNode* bvhRead(const QString& file);

    void assignChannels(BVHNode* node, FILE* f, int frame);
    void setChannelLimits(BVHNode* node,BVHChannelType type,double min,double max) const;
    void parseLimFile(BVHNode* root,const QString& limFile) const;

    void setNumFrames(int numFrames);
    int numFrames() const;
    void bvhIndent(QTextStream& out,int depth);
    void bvhWriteNode(BVHNode* node,QTextStream& out,int depth);
    void bvhWriteFrame(BVHNode* node,QTextStream& out,int frame);
    void bvhPrintNode(BVHNode* n, int depth);

    // the write functions return false and set error when the file could not be written;
    // an existing file is then left as it was
    bool bvhWrite(Animation* root,const QString& file,QString& error);
    // the old writers, kept until the old parser code is removed (#62 part 4),
    // so that a test can compare their files with the ones the new writer makes
    bool legacyBvhWrite(Animation* root,const QString& file,QString& error);
    bool legacyAvmWrite(Animation* root,const QString& file,QString& error);
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
    // the same with the old token parser; not used for opening files any more
    // (issue #62), kept for the comparison tests until the old code is removed
    BVHNode* legacyAnimRead(const QString& file,const QString& limFile);

    // lex neva's stuff:
    BVHNode* avmRead(const QString& file);

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

    // limits for files that are read
    static const int MAX_FILE_BYTES=64*1024*1024;
    static const int MAX_DEPTH=64;
    static const int MAX_JOINTS=256;
    static const int MAX_FRAMES=10000;

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
    QString inputFile;
    QStringList tokens;
    int tokenPos;

    // remember if the loaded animation is in old or new AVM format
    bool havePositionKeys;

    QStringList validNodes;
    // maps every known joint name or alias to the name used in Pingmator
    QHash<QString,QString> jointAliases;

    void loadBentoJoints();

    // shared by animRead() and legacyAnimRead()
    void startRead();
    BVHNode* finishRead(BVHNode* root,const QString& limFile);
    // the joint tree from the new parser's data model
    BVHNode* buildTree(const AnimFile::Animation& anim);
    BVHNode* copySkeletonNode(BVHNode* reference,BVHNode* loaded);
    BVHNode* findLoadedNode(BVHNode* loaded,const QString& name) const;
    BVHNode* positionNode;

    // joints and channels read so far from the hierarchy of the current file
    int jointsRead;
    int channelsRead;

    QString token();
    bool expect_token(const QString& expect);
    // remembers the first reason the file cannot be read; always returns false
    bool failed(const QString& reason);
    bool readInt(int& value,int minimum,int maximum,const QString& what);
    bool readFloat(double& value,const QString& what);
    // number of tokens not read yet
    int tokensLeft() const;
    bool readFile(const QString& file);
    BVHNode* bvhReadNode(int depth);
    bool readMotion(BVHNode* root);

    void assignChannels(BVHNode* node,int frame);

    bool avmReadKeyFrame(BVHNode* root);
    bool avmReadKeyFrameProperties(BVHNode* root);

    void avmWriteKeyFrame(BVHNode* root, QTextStream& out, int numberOfFrames);
    void avmWriteKeyFrameProperties(BVHNode* root, QTextStream& out, int numberOfFrames);

    // removes all unknown nodes from the animation
    void removeNoSLNodes(BVHNode* root);

    // debugging function, dumps the node structure
    void dumpNodes(BVHNode* node,QString indent);

    void setAllKeyFramesHelper(BVHNode* node,int numberOfFrames) const;
    const QString bvhGetNameHelper(BVHNode* node,int index);
    int bvhGetIndexHelper(BVHNode* node,const QString& name);

    void bvhGetFrameDataHelper(BVHNode* node,int frame);
    void bvhSetFrameDataHelper(BVHNode* node,int frame);

    int pasteIndex;
};

#endif
