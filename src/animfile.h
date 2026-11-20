/*
 * PinglyMotion – BVH and AVM files read into a plain data model (issue #62)
 *
 * This is the parser and writer for animation files; bvh.cpp builds the joint
 * tree from it and saves through it. It reads from memory, never touches the
 * disk or the window, and either returns the whole animation or an error
 * message – never half a result and never a crash. Every number and index that comes from the file is checked
 * against the limits below (see docs/SECURITY_AUDIT.md, sections 2, 7 and 8).
 *
 * The data model keeps what is in the file, in file order. It does not know
 * the Second Life skeleton: dropping unknown joints, joint name aliases and
 * joint limits stay with the code that builds an Animation from it.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef ANIMFILE_H
#define ANIMFILE_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QVector3D>

#include <optional>
#include <vector>

#include "easing.h"

namespace AnimFile
{
  // limits for files that are read
  constexpr qsizetype MAX_FILE_BYTES=64*1024*1024;
  constexpr int MAX_DEPTH=64;          // nesting of joints; the root is depth 0
  constexpr int MAX_JOINTS=256;        // joints and End Sites together; the SL skeleton has 133 joints
  constexpr int MAX_CHANNELS=6;        // per joint: 3 positions and 3 rotations
  constexpr int MAX_FRAMES=10000;
  constexpr double MIN_FRAME_TIME=0.001;
  constexpr double MAX_FRAME_TIME=10.0;
  constexpr float MAX_AVATAR_SCALE=10.0f;

  // values of the AVM "Figure:" line (the same as Animation::FigureType)
  constexpr int FIGURE_MALE=0;
  constexpr int FIGURE_FEMALE=1;

  enum class Format { Bvh, Avm };

  // in the same order as BVHChannelType in bvhnode.h
  enum class Channel : quint8 { XPosition, YPosition, ZPosition, XRotation, YRotation, ZRotation };

  enum class JointKind : quint8 { Root, Joint, EndSite };

  // the order of the rotation channels of a joint; the same values as BVHOrderType in bvhnode.h
  enum class RotationOrder : quint8 { XYZ=1, ZYX, XZY, YZX, YXZ, ZXY };

  // a key frame of an AVM or .dingle file
  struct Key
  {
    int frame=0;
    bool easeIn=false;
    bool easeOut=false;
    // .dingle only: the easing choice of the segment that starts at this key
    // (the key's "curve", issue #53 step 3). Empty: easeOut of this key and
    // easeIn of the next key pick the choice, as always (Easing::segmentPreset).
    std::optional<Easing::Preset> curve;
    // .dingle only: a custom curve of that segment (a "curve" of type "bezier",
    // issue #11). When set it wins over `curve` and the ease flags.
    std::optional<Easing::Bezier> customCurve;

    bool operator==(const Key& other) const
    { return frame==other.frame && easeIn==other.easeIn && easeOut==other.easeOut && curve==other.curve &&
             customCurve==other.customCurve; }
    bool operator!=(const Key& other) const { return !(*this==other); }
  };

  struct Joint
  {
    QString name;                 // as written in the file; "Site" for most End Sites
    JointKind kind=JointKind::Joint;
    int parent=-1;                // index into Animation::joints, -1 for the root
    int depth=0;                  // 0 for the root
    double offset[3]={0.0,0.0,0.0};
    QVector<Channel> channels;    // in file order (also the order the rotations are applied in)
    int firstValue=0;             // where this joint's channels start in a frame of Animation::values
    // AVM only: sorted, no frame twice, frame 0 always there, all frames < Animation::frames.
    // In a BVH file every frame is a key frame and this list is empty.
    QVector<Key> keys;

    // the rotation order from the rotation channels; XYZ when there are not three different ones
    RotationOrder rotationOrder() const;
  };

  struct Animation
  {
    Format format=Format::Bvh;

    // depth first, in file order: a parent always comes before its children.
    // joints[0] is the root.
    QVector<Joint> joints;

    int frames=0;
    double frameTime=0.0;
    int valuesPerFrame=0;         // all channels of all joints
    std::vector<float> values;    // frames × valuesPerFrame, frame by frame

    // AVM only (a BVH file keeps the defaults)
    float avatarScale=1.0f;
    int figure=FIGURE_FEMALE;
    int loopIn=-1;                // -1 means "no loop points set"
    int loopOut=0;                // the number of frames when the file has none
    // true when the file has its own "Positions:" list. Without it, the
    // position keys are the root joint's key frames (older AVM files).
    bool hasPositionKeys=false;
    QVector<Key> positionKeys;    // like Joint::keys; empty for a BVH file

    float value(int frame,int channel) const
    { return values[static_cast<size_t>(frame)*valuesPerFrame+channel]; }

    // the channels of a joint in one frame; missing channels are 0, like in bvh.cpp
    QVector3D rotation(int joint,int frame) const;
    QVector3D position(int joint,int frame) const;

    QVector<int> children(int joint) const;
    // index of the first joint with this name, or -1
    int findJoint(const QString& name) const;
  };

  struct Result
  {
    // set when the file was read; then error is empty
    std::optional<Animation> animation;
    // why the file cannot be read, for the user; line is 0 when it is not about one line
    QString error;
    int line=0;

    bool ok() const { return animation.has_value(); }
  };

  Result parse(const QByteArray& data,Format format);
  Result parseBvh(const QByteArray& data);
  Result parseAvm(const QByteArray& data);

  // the format from the file name ending (.bvh or .avm, any case)
  std::optional<Format> formatForFileName(const QString& fileName);

  QString channelName(Channel channel);

  // The text of the file for this animation, in the format it has (BVH or
  // AVM). The animation must
  // follow the rules of a parsed one (see Joint and Animation above); for an
  // AVM file every joint has its key frames, frame 0 first, and
  // hasPositionKeys is set.
  QByteArray write(const Animation& anim);
}

#endif
