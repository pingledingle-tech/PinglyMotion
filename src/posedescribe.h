/*
 * PinglyMotion - describe_pose: the pose as computed text (issue #129)
 *
 * An AI assistant misreads joint angles from a picture by 15 to 20 degrees
 * and mixes up the figure's left and right, but it can read exact numbers
 * and plain sentences. This module turns one frame of an open animation into
 * that text: world positions of head, hands, feet and hip; rule-based
 * sentences with a dead zone around their thresholds, so a tiny angle change
 * never flips the wording back and forth; the classic joints that changed
 * from frame 1, in friendly names, with "more" or "less"; left/right
 * symmetry notes; and a one-line summary.
 *
 * Reuses the conventions of docs/ASSISTANT_GUIDE.md and its calibration
 * table: classic joint names, the figure's own left and right, and
 * BVHNode::mirror()'s left/right sign rule (negate Y and Z). World positions
 * are in the program's own units (inches, like the position fields and
 * docs/ASSISTANT_GUIDE.md section 1 and 3) and in metres
 * (AnimExport::INCHES_TO_METERS). The floor is y = 0 in that space: the
 * start pose stands with the hip at 43.5 and the legs straight down to it.
 * The rules, changes and symmetry cover the classic skeleton only, the one
 * the guide's sections 2 and 5 talk about. "hands" (phase 2c) adds, for each
 * hand, which way the palm faces and where each Bento fingertip is, relative
 * to the wrist, in the character's own directions; the face, wings and tail
 * are not covered.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef POSEDESCRIBE_H
#define POSEDESCRIBE_H

#include <QHash>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QStringList>

class Animation;
class BVHNode;

namespace PoseDescribe
{
  // "frame" is 0-based, like Animation::getFrame(); the current frame of
  // "animation" is not changed. "animation" must have an open file (a
  // skeleton loaded with the classic joints).
  QJsonObject describe(Animation* animation, int frame);

  // shared with describe_motion (src/motiondescribe.*): the classic joints
  // this module covers, their everyday names, a joint by name (nullptr when
  // the skeleton has none), and the world matrix of every joint at a
  // 0-based frame, hip position track included
  const QStringList& classicJoints();
  QString friendlyName(const QString& joint);
  BVHNode* findJoint(Animation* animation, const QString& name);
  QHash<BVHNode*, QMatrix4x4> worldTransforms(Animation* animation, int frame);
}

#endif
