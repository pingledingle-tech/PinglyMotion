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

#ifndef ANIMATION_H
#define ANIMATION_H

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QTimer>
#include <QVector3D>

#include <functional>
#include <optional>

#include "easing.h"
#include "iktree.h"
#include "playstate.h"

class BVH;

struct RotationLimits
{
    QVector3D minimum;
    QVector3D maximum;
};

class Animation : public QObject
{
  Q_OBJECT

  public:
    typedef enum
    {
      IK_LHAND=0,
      IK_RHAND,
      IK_LFOOT,
      IK_RFOOT,
      NUM_IK
    } IKPartType;

    typedef enum
    {
      FIGURE_MALE=0,
      FIGURE_FEMALE,
      NUM_FIGURES
    } FigureType;

    // A problem with one of the program's own data files (skeleton, limits file):
    // the model does not show it, the window does (same title and text as always).
    struct LoadProblem
    {
      QString title;
      QString text;
    };

    // with data: those bytes are parsed instead of reading bvhFile (see BVH::animRead())
    Animation(BVH* bvh,const QString& bvhFile=QString(),const QByteArray* data=nullptr);
    ~Animation();

    void loadBVH(const QString& bvhFile,const QByteArray* data=nullptr);
    // what went wrong while the constructor read its files; empty when all was well
    const QList<LoadProblem>& loadProblems() const { return m_loadProblems; }
    // the "extensions" object of the .dingle file it was read from, written
    // back on save (the wave example's notes are in it, UX phase 3c); empty
    // for other files
    const QJsonObject& extensions() const { return m_extensions; }
    void setExtensions(const QJsonObject& extensions) { m_extensions=extensions; }
    // false and an error text when the file could not be written; unsaved changes stay marked
    bool saveBVH(const QString& bvhFile, QString* error=nullptr);
    // the pose of one frame as a small BVH, for the Aurora link
    QString poseBvhText(int frame);
    // the same pose as quaternions per joint, for Aurora's "pose" message
    QJsonObject poseQuaternions(int frame);
    int getNumberOfFrames();
    void setNumberOfFrames(int num);
    int getFrame();
    void setFrame(int frameNumber);
    int stepForward();
    void setLoopInPoint(int inFrame);
    int getLoopInPoint();
    void setLoopOutPoint(int outFrame);
    int getLoopOutPoint();
    void setIK(BVHNode* node, bool flag);
    bool getIK(BVHNode* node);
    const QString getPartName(int index) const;
    int getPartIndex(BVHNode* node);
    void setMirrored(bool mirror);
    bool getMirrored();

    // get and set frames per second
    void setFPS(int fps);
    int fps() const;

    // convenience functions
    void setFrameTime(double frameTime);
    double frameTime() const;

    float getAvatarScale();
    void setAvatarScale(float newScale);

    FigureType getFigureType();
    void setFigureType(FigureType type);

    BVHNode* getMotion();
    BVHNode* getEndSite(const QString& siteParentName);
    BVHNode* getNode(int jointNumber);

    void cutFrame();
    void copyFrame();
    void pasteFrame();

    bool dirty() const;
    void setDirty(bool state);

    void nextPlaystate();
    void setPlaystate(PlayState state);

    const FrameData keyframeDataByIndex(int jointNumber,int index);

    void setRotation(BVHNode* node,double x,double y,double z);
    QVector3D getRotation(BVHNode* node);
    RotationLimits getRotationLimits(BVHNode* node);
    void setPosition(double x,double y,double z);
    QVector3D getPosition();

    // The hip height of the start pose, a new animation's frame 1 (data/TPose.dingle), in
    // inches: with it the avatar's soles touch the floor of the 3D view.
    static constexpr float StandingHipHeight=43.5285187f;
    // How far the 3D view lifts the figure so its feet stand on the floor: Second Life
    // stands the hip of frame 1 at standing height whatever height the file gives it, so
    // the view does the same, for every animation in one place (UX fixes 1: clips whose
    // frame 1 hip is at 41 inches, like data/Relaxed.dingle and the examples, stood
    // 2.53 inches under the floor). StandingHipHeight minus the hip height of frame 1;
    // 0 for a new animation. check_animation uses the same floor (src/animcheck.cpp).
    float standingLift();
    void addKeyFrameAllJoints();
    void addKeyFrame(BVHNode* joint);
    bool isKeyFrame();
    bool isKeyFrame(const QString& jointName);
    bool isKeyFrame(int jointNumber,int frame);
    void deleteKeyFrameAllJoints();
    void deleteKeyFrame(BVHNode* joint,int frame,bool silent=false); // silent = only send signal to timeline
    bool toggleKeyFrameAllJoints();
    bool toggleKeyFrame(BVHNode* node);

    void setEaseIn(BVHNode* node,int frameNum,bool state);
    void setEaseOut(BVHNode* node,int frameNum,bool state);
    bool easeIn(BVHNode* node,int frameNum);
    bool easeOut(BVHNode* node,int frameNum);
    // the stored easing choice of a key frame (issue #53): the segment that
    // starts at the key plays with it; empty means the two switches decide
    void setCurve(BVHNode* node,int frameNum,const std::optional<Easing::Preset>& curve);
    std::optional<Easing::Preset> curve(BVHNode* node,int frameNum);
    // the custom curve of a key frame (issue #11); wins over curve() when set
    void setCustomCurve(BVHNode* node,int frameNum,const std::optional<Easing::Bezier>& curve);
    std::optional<Easing::Bezier> customCurve(BVHNode* node,int frameNum);

    void copyKeyFrame(int jointNumber,int from,int to);
    void moveKeyFrame(int jointNumber,int from,int to,bool copy=false);

    bool compareFrames(const BVHNode* joint,int key1,int key2) const;

    // mirror a joint or the whole animation, if joint==0
    void mirror(BVHNode* joint);

    // Whole pose actions of the current frame only (posing UX phase 2a):
    // every joint of the skeleton, not just the selected one.
    //
    // Mirrors the current frame left to right: a joint with a mirror partner
    // takes the partner's current rotation with Y and Z negated (the
    // convention BVHNode::mirror() already uses); a joint with no partner
    // keeps its own rotation with Y and Z negated in place. A joint whose
    // source rotation is all zero is left untouched, so it stays a
    // non-keyframe where it was one.
    void flipPose();
    // Sets every joint's rotation to zero. The program's own rest-pose
    // offsets (data/firestorm, TPose.dingle) put the figure in a T-pose when
    // every rotation is zero, so this needs no joint-by-joint special case.
    void tPose();
    // Copies every joint's frame 0 rotation onto the current frame ("Reset
    // all" in the Joints panel).
    void resetAllToFrameZero();

    void optimize();

    // Everything Undo and Redo bring back (src/undohistory.h): the key frames of
    // every joint and of the Avatar Position (with their easing and curves), the
    // number of frames, frames per second and the loop. keys[i] belongs to the part
    // with index i (0 is the Avatar Position, as getNode() counts).
    struct Snapshot
    {
      QList<QMap<int,FrameData>> keys;
      int frames=0;
      int fps=0;
      int loopIn=-1;
      int loopOut=-1;
      bool operator==(const Snapshot& other) const
      {
        return frames==other.frames && fps==other.fps && loopIn==other.loopIn && loopOut==other.loopOut &&
               keys==other.keys;
      }
      bool operator!=(const Snapshot& other) const { return !(*this==other); }
    };
    Snapshot snapshot() const;
    // puts a snapshot back: the joints stay the same objects, so pointers to them
    // stay valid. Tells the timeline (numberOfFrames, redrawTrack) and the view
    // (frameChanged); it does not change the dirty mark, the caller does.
    void restore(const Snapshot& state);

    enum { MAX_PARTS=512 };

  public slots:
    void deleteKeyFrame(int jointNumber,int frame);
    void insertFrame(int jointNumber,int frame);
    void deleteFrame(int jointNumber,int frame);

  signals:
    void numberOfFrames(int num);
    void currentFrame(int frame);
    void frameChanged();
    void redrawTrack(int track);
    void animationDirty(bool state);

  protected:
    BVH* bvh;
    BVHNode* frames;
    BVHNode* positionNode;

    FigureType figureType;

    // this flag shows if the animation has been worked on and not yet saved
    bool isDirty;

    // display the avatar at another scale (1.0 is default)
    float avatarScale;

    int frame;
    int totalFrames;
    int framesPerSecond;

    PlayState playstate;

    int loopInPoint;
    int loopOutPoint;

    bool mirrored;
    QList<LoadProblem> m_loadProblems;
    QJsonObject m_extensions;
    bool ikOn[NUM_IK];
    IKTree ikTree;

    void recursiveAddKeyFrame(BVHNode* joint);
    bool isKeyFrameHelper(BVHNode* joint);
    void recursiveDeleteKeyFrame(BVHNode* joint);
    void insertFrameHelper(BVHNode* joint,int frame);
    void deleteFrameHelper(BVHNode* joint,int frame);
    void optimizeHelper(BVHNode* joint);
    void mirrorHelper(BVHNode* joint);
    // calls fn once for every rotation joint of the skeleton (not End Sites,
    // not the position pseudo-track); used by flipPose(), tPose() and
    // resetAllToFrameZero()
    void forEachJoint(BVHNode* joint,const std::function<void(BVHNode*)>& fn);
    // every node in part index order (0 the Avatar Position, then the skeleton
    // depth first, End Sites included), for snapshot() and restore()
    QList<BVHNode*> partNodes() const;

    void calcPartMirrors();
    void setIK(IKPartType part, bool flag);
    bool getIK(IKPartType part);
    void applyIK(const QString& name);
    void solveIK();
    void solveIKIfOn();
};

#endif
