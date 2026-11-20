/*
 * PinglyMotion – retargeting core: move a BVH animation made for any humanoid
 * skeleton onto the Second Life skeleton (issue #16, step 1)
 *
 * This is pure maths on the data model of the parser (src/animfile.h). It
 * never touches the disk or the window, and either returns a whole animation
 * or an error message. Nothing in the program uses it yet; the import menu
 * and the mapping dialog come in later jobs.
 *
 * Three parts:
 *  1. Bone name matching: a bone name is split into tokens, the side
 *     (left/right) is found when it stands alone, common rig prefixes are
 *     dropped, and the tokens are mapped to a canonical slot (pelvis, spine,
 *     neck, head, clavicle, upper arm, lower arm, hand, thigh, calf, foot,
 *     toe, five fingers). Twist, IK and control bones are refused.
 *     Ambiguous names are resolved from the tree ("shoulder" is the clavicle
 *     in some rigs and the upper arm in others).
 *  2. Chain pairing: chains of different length (a spine of 3 bones against
 *     2, fingers with 4 segments against 3) are paired.
 *  3. Retarget maths: the source turns are taken in world space, the
 *     difference between the two rest poses is corrected per bone
 *     (swing-twist), and the hip movement is scaled by leg length. The
 *     result is an AnimFile::Animation on the target (Second Life) skeleton.
 *
 * The design follows the retargeting pipeline of Mesh2Motion
 * (https://github.com/Mesh2Motion/mesh2motion-app, MIT licence): name
 * tokenizer, bone vocabulary, chain resolver, slot mapper and swing-twist
 * retarget. No code was copied; this is written from the idea, for Qt and
 * for the Second Life skeleton. Credit for the design goes to that project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef RETARGET_H
#define RETARGET_H

#include "animfile.h"

#include <QPair>
#include <QQuaternion>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

namespace Retarget
{
  // limits of the retargeting code (the parser's limits in animfile.h apply too)
  constexpr int MAX_NAME_LENGTH=256;   // longer bone names are not matched
  constexpr int MAX_PAIRS=AnimFile::MAX_JOINTS;

  enum class Side { None, Left, Right };

  // what a single bone name says, without looking at the tree
  struct NameInfo
  {
    QStringList tokens;   // lower case, prefixes and the side removed
    Side side=Side::None;
    // the canonical slot without side, for example "upperarm", "spine",
    // "finger.index"; empty when the name is unknown or refused.
    // "shoulder" and "hipside" are ambiguous and resolved from the tree.
    QString slot;
    bool refused=false;   // twist, IK, control or end bone
    QString reason;       // why it was refused or ignored, for the user
  };

  // split a bone name into lower case tokens: camelCase, digits and
  // separators (_ . - : | space) split it; a namespace before ':' or '|' is dropped
  QStringList tokenize(const QString& name);
  NameInfo analyseName(const QString& name);

  enum class ChainAlign
  {
    FromRoot,   // pair the first bones, drop the extra ones at the end (limbs, fingers)
    Spread      // spread evenly, first with first and last with last (spine)
  };

  // pairs (index in the source chain, index in the target chain) for chains
  // of different length; every index is used at most once
  QVector<QPair<int,int>> pairChain(int sourceCount,int targetCount,ChainAlign align);

  enum class MatchStatus { Matched, Ignored, Refused };

  // the result for one joint of the source skeleton
  struct BoneMatch
  {
    int joint=-1;          // index into the source Animation::joints
    QString chain;         // for example "upperarm.L", "spine", "finger.index.R"; empty if not matched
    int segment=0;         // position in the chain, from the root
    int target=-1;         // joint of the target skeleton, -1 if not paired
    MatchStatus status=MatchStatus::Ignored;
    QString reason;        // why it was ignored or refused
  };

  // skeletons the import knows by heart: their joints are mapped with a fixed
  // table instead of being guessed from the names (docs/RETARGET_MAPPING.md)
  enum class KnownSkeleton
  {
    None,
    Soma77     // NVIDIA Kimodo's SOMA 77-joint skeleton (text-to-motion, issue #18)
  };

  // the known skeleton a source is, from its joint names and tree; None when
  // it is none of them
  KnownSkeleton detectKnownSkeleton(const AnimFile::Animation& source);
  // a readable name for messages, empty for None
  QString knownSkeletonName(KnownSkeleton skeleton);

  struct Mapping
  {
    // (source joint, target joint); every joint at most once on each side
    QVector<QPair<int,int>> pairs;
    QVector<BoneMatch> bones;        // one entry per source joint, End Sites included
    QStringList missingRequired;     // readable names of required slots the source lacks
    QStringList missingTarget;       // required slots the target skeleton lacks
    bool direct=false;               // the source already uses Second Life joint names
    KnownSkeleton known=KnownSkeleton::None;   // mapped with the fixed table of a known skeleton

    bool ok() const { return missingRequired.isEmpty() && missingTarget.isEmpty() && !pairs.isEmpty(); }
  };

  // match the joints of the source to the target skeleton (a Second Life
  // skeleton, for example bin/data/TPose.bvh) by name and tree shape
  Mapping mapSkeletons(const AnimFile::Animation& source,const AnimFile::Animation& target);

  // fill missingRequired and missingTarget again from the pairs (after the
  // pairs were changed by hand, see src/retargetmapping.*)
  void updateMissing(Mapping& mapping,const AnimFile::Animation& target);

  // the canonical chain of a Second Life joint name ("mShoulderLeft" or the
  // old name "lShldr" give "upperarm.L"), empty if the joint takes no part in retargeting
  QString slChain(const QString& jointName);

  struct Options
  {
    // use the first frame of the source as its rest pose, not the header
    // (for files that start in a T-pose or A-pose)
    bool firstFrameAsRest=false;
  };

  struct Result
  {
    std::optional<AnimFile::Animation> animation;  // on the target skeleton, BVH format
    QString error;                                 // empty when it worked
    Mapping mapping;
    // per frame time and frame count of the source are kept
    float legScale=1.0f;                           // target leg length / source leg length

    bool ok() const { return animation.has_value(); }
  };

  // check that an animation follows the rules of a parsed one (joint and
  // frame limits of animfile.h, finite numbers); empty if it does
  QString validate(const AnimFile::Animation& anim,bool needFrames);

  // retarget with the automatic mapping
  Result retarget(const AnimFile::Animation& source,const AnimFile::Animation& target,
                  const Options& options=Options());
  // retarget with a given mapping (for a later mapping dialog); the mapping is checked
  Result retarget(const AnimFile::Animation& source,const AnimFile::Animation& target,
                  const Mapping& mapping,const Options& options=Options());

  // rotation helpers, angles in degrees, channels applied in the given order
  // (the same as the program: the first channel is the outermost turn)
  QQuaternion eulerToQuaternion(const float angles[3],AnimFile::RotationOrder order);
  // angles[0..2] are the values for the first, second and third channel of the order
  void quaternionToEuler(const QQuaternion& q,AnimFile::RotationOrder order,float angles[3]);
}

#endif
