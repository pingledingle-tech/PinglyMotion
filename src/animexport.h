/*
 * PinglyMotion – write a Second Life .anim file from an animation (issue #10)
 *
 * Takes the plain data model of the new parser (animfile.h, a BVH-like
 * animation: frame 0 is the reference frame, joints with Euler channels) and
 * gives the bytes of a Second Life internal animation file. The rules are in
 * docs/ANIM_FORMAT.md (sections 2 to 7); the reference is the Second Life
 * viewer, llcharacter/llkeyframemotion.cpp and llbvhloader.cpp (LGPL 2.1,
 * Linden Research). This is a function only: no menu, no dialog, no disk.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef ANIMEXPORT_H
#define ANIMEXPORT_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

#include "animfile.h"

namespace AnimExport
{
  // limits of the format and of the upload (docs/ANIM_FORMAT.md section 6)
  constexpr double MAX_DURATION=60.0;
  constexpr int MAX_JOINTS=216;
  constexpr int MAX_BYTES=250000;
  constexpr int MAX_PRIORITY=6;
  constexpr int MAX_HAND_POSE=13;
  constexpr double MAX_POSITION=5.0;       // metres, per axis
  constexpr double INCHES_TO_METERS=0.02540005;

  struct Options
  {
    int basePriority=2;                   // 0 to 6
    QHash<QString,int> jointPriority;     // by Second Life joint name; others use basePriority
    double easeIn=0.3;                    // seconds
    double easeOut=0.3;
    bool loop=false;
    double loopIn=0.0;                    // seconds; used when loop is on
    double loopOut=-1.0;                  // seconds; below 0 means "the duration"
    int handPose=1;                       // 0 to 13
    QString emote;                        // the library name, empty for none
    // joints that never move away from frame 0 are left out, as the viewer's
    // BVH importer does; the root joint is always kept
    bool keepStaticJoints=false;
    int maxBytes=MAX_BYTES;               // keys are thinned out when the file would be bigger
  };

  struct Result
  {
    QByteArray bytes;                     // empty when error is set
    QString error;
    QStringList warnings;
    bool ok() const { return error.isEmpty(); }
  };

  // the Second Life name for a BVH joint name ("hip" gives "mPelvis"); other names stay as they are
  QString slJointName(const QString& name);

  Result write(const AnimFile::Animation& anim,const Options& options=Options());

  // the quantising of the viewer (llmath/llquantize.h), exposed for tests
  quint16 floatToU16(float value,float lower,float upper);
  float u16ToFloat(quint16 value,float lower,float upper);
}

#endif
