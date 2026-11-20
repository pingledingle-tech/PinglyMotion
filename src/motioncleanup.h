/*
 * PinglyMotion – clean-up of imported motion (issues #17 and #18)
 *
 * Motion from video capture (XR Animator and similar) and from text-to-motion
 * models arrives at odd frame rates, shakes a little, starts in motion,
 * walks away from the start and does not loop. These functions fix that on
 * the data model of the parser (src/animfile.h), with no window and no disk:
 *
 *  - resample()          to another frame rate (30 fps), same length
 *  - insertStartPose()   a reference pose at frame 1, the motion from frame 2
 *  - inPlace()           remove the forward travel of the hip, keep the sway
 *  - blendLoopSeam()     ease the end into the first frame, so a loop has no jump
 *  - smooth()            light, zero-phase smoothing of the turns and positions
 *
 * Turns are handled as quaternions (interpolation, smoothing, blending), so a
 * joint never takes the long way round, whatever its rotation order. Every
 * function takes a whole animation and returns a whole new one or an error,
 * never half a result; the input is not changed. Only per-frame data is
 * handled (Format::Bvh): an AVM or .dingle file with key frames is refused.
 *
 * Suggested order for an import: resample, smooth, inPlace, blendLoopSeam,
 * then insertStartPose. Foot lock (planted feet that do not slide) is not
 * built yet.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef MOTIONCLEANUP_H
#define MOTIONCLEANUP_H

#include "animfile.h"

#include <QQuaternion>
#include <QString>

#include <optional>

namespace MotionCleanup
{
  constexpr double DEFAULT_FPS=30.0;
  constexpr int MAX_SMOOTH_RADIUS=10;     // frames on each side
  constexpr int DEFAULT_SMOOTH_RADIUS=1;   // weights 1 2 1: light
  constexpr int DEFAULT_LOOP_BLEND=8;     // frames

  struct Result
  {
    std::optional<AnimFile::Animation> animation;
    QString error;    // empty when it worked

    bool ok() const { return animation.has_value(); }
  };

  // the same motion at another frame rate (1 to 120 fps). The length
  // (frames-1)*frameTime stays the same within half a new frame; turns are
  // interpolated along the shortest way, positions in a straight line.
  Result resample(const AnimFile::Animation& anim,double fps=DEFAULT_FPS);

  // one frame more at the start: frame 1 (the file's frame 0) holds the start
  // pose, and the motion begins at frame 2. The start pose has every turn at
  // zero (the rest pose of the skeleton) and the positions of the first frame
  // of the motion. Second Life compares every frame with frame 1 and does not
  // play it, so this makes every joint that moves part of the animation.
  Result insertStartPose(const AnimFile::Animation& anim);

  // remove the travel of the root over the ground (X and Z; Y is up): the
  // straight line from the first to the last frame is taken away, so the last
  // frame stands where the first does and the sway in between stays. The
  // height is not changed. Frames before firstFrame (a start pose) are kept.
  Result inPlace(const AnimFile::Animation& anim,int firstFrame=0);

  // ease the last blendFrames frames into the pose of frame firstFrame, so
  // the last frame equals it and a loop has no jump. Turns of every joint and
  // the height of the root are blended; the travel of the root over the
  // ground is not (use inPlace() first for a walk in place).
  Result blendLoopSeam(const AnimFile::Animation& anim,int blendFrames=DEFAULT_LOOP_BLEND,int firstFrame=0);

  // light smoothing against the shake of video capture: a symmetric
  // (zero-phase, nothing is moved in time) binomial filter over 2*radius+1
  // frames on the turns (as quaternions) and positions. radius 1 to
  // MAX_SMOOTH_RADIUS; the window shrinks near the ends, so the first and the
  // last frame are kept as they are (a loop seam or start pose stays).
  // Frames before firstFrame are kept too.
  Result smooth(const AnimFile::Animation& anim,int radius=DEFAULT_SMOOTH_RADIUS,int firstFrame=0);

  // helpers, also for the tests: the turn of a joint in one frame (identity
  // when the joint has not three rotation channels) and the angle between two
  // turns in degrees (0 to 180)
  QQuaternion turn(const AnimFile::Animation& anim,int joint,int frame);
  float angleBetween(const QQuaternion& a,const QQuaternion& b);
}

#endif
