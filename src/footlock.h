/*
 * PinglyMotion – foot lock for imported motion (issue #17)
 *
 * Motion from video capture slides: a foot that stands on the floor drifts
 * a few centimetres while it should stay put. Foot lock fixes that on the
 * data model of the parser (src/animfile.h), with no window and no disk:
 *
 *  1. findContacts()  the frames where each foot is planted: its ankle is
 *                     less than CONTACT_HEIGHT above the floor and moves
 *                     slower than CONTACT_SPEED. The floor is the lowest
 *                     ankle height of both feet in the clip (as in
 *                     describe_motion). Runs shorter than
 *                     MIN_CONTACT_FRAMES are not a contact.
 *  2. lockFeet()      pins the ankle at one world position for each contact
 *                     (the mean of where it was), solves the leg (thigh and
 *                     shin) with an exact two-bone IK so it reaches the pin,
 *                     and keeps the foot's own turn in the world, so the
 *                     sole stays as it was. The knee stays in the plane it
 *                     had in the clip, so it keeps pointing the same way.
 *                     The pin is blended in and out over blendFrames frames
 *                     on each side. Every other joint, and every frame
 *                     outside the contacts and their blends, is unchanged.
 *
 * The two-bone IK here is analytic and works on the file's channels; the
 * program's IK solver (src/iktree.*) works on the joint tree of the 3D view
 * and needs a loaded scene, which an import has not got yet.
 *
 * Joint limits: given a limits table (bin/data/SL.lim, see parseLimits()),
 * every changed angle of the thigh, shin and foot stays inside its limits,
 * or where the clip itself was already outside, no further out than the clip.
 *
 * The legs are found by name (the classic lThigh/lShin/lFoot or the Second
 * Life mHipLeft/mKneeLeft/mAnkleLeft, see Retarget::slChain()). Only
 * per-frame data is handled (Format::Bvh), like src/motioncleanup.*.
 *
 * Suggested place in an import: after MotionCleanup::resample() and
 * smooth(), before inPlace() (a walk in place has feet that slide by design).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef FOOTLOCK_H
#define FOOTLOCK_H

#include "animfile.h"
#include "motioncleanup.h"

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>
#include <QVector3D>

namespace FootLock
{
  // all thresholds in one place (metres and seconds; converted with metersPerUnit)
  // ankle above the floor, m: 2 inches, the contact height of describe_motion
  // and check_animation (src/motiondescribe.cpp, src/animcheck.cpp)
  constexpr double CONTACT_HEIGHT=2.0*0.0254;
  constexpr double CONTACT_SPEED=0.2;      // ankle speed, m/s (below check_animation's 0.5 m/s slide)
  constexpr double METERS_PER_UNIT=0.0254; // Second Life BVH files are in inches
  constexpr int MIN_CONTACT_FRAMES=3;
  constexpr int DEFAULT_BLEND_FRAMES=4;    // on each side of a contact
  constexpr int MAX_BLEND_FRAMES=30;

  // the limits of one joint in degrees, index 0, 1, 2 = X, Y, Z rotation
  struct Limits
  {
    float minimum[3]={-180.0f,-180.0f,-180.0f};
    float maximum[3]={180.0f,180.0f,180.0f};
  };
  using LimitTable=QHash<QString,Limits>;

  // the text of a limits file (bin/data/SL.lim: name, weight, then three
  // times axis, minimum, maximum); lines that do not fit are skipped
  LimitTable parseLimits(const QByteArray& text);

  struct Options
  {
    double metersPerUnit=METERS_PER_UNIT;
    double contactHeight=CONTACT_HEIGHT;
    double contactSpeed=CONTACT_SPEED;
    int minContactFrames=MIN_CONTACT_FRAMES;
    int blendFrames=DEFAULT_BLEND_FRAMES;   // 0 to MAX_BLEND_FRAMES
    int firstFrame=0;                       // frames before it (a start pose) are kept
    LimitTable limits;                      // empty: no limits
  };

  // one planted stretch of one foot
  struct Contact
  {
    int first=0;          // first and last frame, both inside the contact
    int last=0;
    QVector3D pin;        // where the ankle is held, in world units of the file
  };

  struct Contacts
  {
    QVector<Contact> left,right;
    double floor=0.0;     // the floor height (world Y) in units of the file
    QString error;        // empty when it worked

    bool ok() const { return error.isEmpty(); }
  };

  Contacts findContacts(const AnimFile::Animation& anim,const Options& options=Options());

  // the whole clean-up step: find the contacts and lock the feet. A clip with
  // no contact comes back unchanged.
  MotionCleanup::Result lockFeet(const AnimFile::Animation& anim,const Options& options=Options());

  // helper, also for the tests: where a joint is in the world in one frame
  QVector3D worldPosition(const AnimFile::Animation& anim,int joint,int frame);
}

#endif
