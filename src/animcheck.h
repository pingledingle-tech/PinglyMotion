/*
 * PinglyMotion - check_animation: exact checks for Second Life (issue #129)
 *
 * describe_pose and describe_motion tell an AI assistant what the animation
 * looks like; this module tells it what is wrong with it, by fixed rules, so
 * the assistant can fix a problem before the user uploads the file. Each
 * check gives an error (Second Life refuses the file or loses part of it), a
 * warning (it plays, but likely looks wrong) or a note (good to know), with
 * the frames and joints it is about:
 *
 *   length            longer than Second Life's 60 seconds (error)
 *   nothing_moves     no joint moves away from frame 1 (error)
 *   hip_offset        the hip more than 5 m from frame 1 on an axis (error)
 *   frame1_posed      posed joints that keep the frame-1 angle all the
 *                     time, so Second Life drops them and their pose (warning)
 *   joint_limits      a key past the joint's natural limit (warning)
 *   foot_under_floor  an ankle below the floor Second Life stands it on (warning)
 *   floating          both feet off that floor for a long time (warning)
 *   foot_sliding      a foot that is down slides along the floor (warning)
 *   jerk              a joint or the hip jumps in one frame (warning)
 *   dead_start        nothing moves at the start (warning)
 *   loop_seam         the loop jumps from its last frame to its first (warning or note)
 *   dropped_joints    classic joints that never move, left out on export (note)
 *   identical_keys    keys that change nothing (note)
 *
 * The thresholds are fixed, at the top of animcheck.cpp and in
 * docs/CONTROL.md. Lengths are in the program's units (inches).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef ANIMCHECK_H
#define ANIMCHECK_H

#include <QJsonObject>

class Animation;

namespace AnimCheck
{
  // the whole animation; the current frame of "animation" is not changed
  QJsonObject check(Animation* animation);
}

#endif
