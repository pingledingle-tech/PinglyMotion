/*
 * PinglyMotion - describe_motion: the motion as computed text (issue #129)
 *
 * describe_pose (src/posedescribe.*) tells an AI assistant what one frame
 * looks like. Movement over time is where a picture fails worst: a model
 * cannot judge speed, order, foot contact or a loop seam from stills. This
 * module turns a frame range of an open animation into text: for every
 * classic joint that moves, what it does over time (its main axis increases,
 * holds or decreases; how far; how fast; when it starts); the paths of the
 * hands and feet (straight, arc, or out and back; how far; top speed); foot
 * contact and where the weight is; the hip's height and travel; the rhythm
 * (the repeat period, beats per minute, legs and arms in opposition or
 * together, feet alternating); the loop seam; what changes between
 * consecutive keys; and a one-line summary.
 *
 * Every word is chosen by a fixed threshold, listed at the top of
 * motiondescribe.cpp and in docs/CONTROL.md. Positions are in the program's
 * own units (inches) like describe_pose; speeds also in metres per second.
 * The floor for foot contact is the lowest ankle height in the range, so a
 * file whose figure stands higher or lower than the start pose still gets
 * the right contacts. Classic joints only, like describe_pose.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef MOTIONDESCRIBE_H
#define MOTIONDESCRIBE_H

#include <vector>

#include <QHash>
#include <QJsonObject>
#include <QQuaternion>
#include <QStringList>
#include <QVector3D>

class Animation;

namespace MotionDescribe
{
  // "from" and "to" are 0-based frames, from < to, both inside the
  // animation; the current frame of "animation" is not changed. "animation"
  // must have an open file (a skeleton with the classic joints).
  QJsonObject describe(Animation* animation, int from, int to);

  // shared with check_animation (src/animcheck.*): the frames from..to
  // (0-based) sampled once: per classic joint its rotation (each axis without
  // the 360-degree jumps) and the same turn as one rotation; world positions
  // of the hip, head, hands and feet (inches); the hip's own forward and left
  // directions, flat on the floor. Index 0 is frame "from".
  struct Samples
  {
    int from = 0, to = 0, count = 0;
    double frameTime = 0;
    QStringList joints;
    QHash<QString, std::vector<QVector3D>> rotation;
    QHash<QString, std::vector<QQuaternion>> orientation;
    QHash<QString, std::vector<QVector3D>> position;
    std::vector<QVector3D> hipForward, hipLeft;
  };
  // the animation must have the classic joints hip, head, lHand, rHand, lFoot and rFoot
  Samples sample(Animation* animation, int from, int to);

  // the loop seam as describe_motion's "loop" answer gives it
  QJsonObject loopSeam(Animation* animation);
}

#endif
