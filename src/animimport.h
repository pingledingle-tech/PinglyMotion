/*
 * Pingmator – read a Second Life .anim file (issue #22, step 1)
 *
 * Turns the bytes of a .anim file into a plain data model: header, joints,
 * rotation and position keys in seconds, metres and quaternions. It reads from
 * memory, never touches the disk or the window, and either returns the whole
 * motion or an error message, never half a result and never a crash. Every
 * count, length and number from the file is checked before it is used (see
 * docs/ANIM_FORMAT.md, section 8, and docs/SECURITY_AUDIT.md).
 *
 * The data model keeps what is in the file. It does not know the Second Life
 * skeleton: turning the keys into a Pingmator animation is a later step.
 * The reference for the format is the Second Life viewer,
 * llcharacter/llkeyframemotion.cpp (LGPL 2.1, Linden Research).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef ANIMIMPORT_H
#define ANIMIMPORT_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

namespace AnimImport
{
  // limits of files that are read
  constexpr qsizetype MAX_FILE_BYTES=4*1024*1024;
  constexpr double MAX_DURATION=60.0;
  constexpr int MAX_JOINTS=216;
  constexpr int MAX_CONSTRAINTS=10;
  constexpr int MAX_NAME_BYTES=64;
  constexpr int MAX_PRIORITY=6;
  constexpr int MAX_HAND_POSE=13;
  constexpr double MAX_POSITION=5.0;     // metres per axis

  struct RotationKey
  {
    float time=0.0f;                     // seconds, 0 to duration
    float x=0.0f,y=0.0f,z=0.0f,w=1.0f;   // unit quaternion (w from x, y, z, never negative)
  };

  struct PositionKey
  {
    float time=0.0f;                     // seconds, 0 to duration
    float x=0.0f,y=0.0f,z=0.0f;          // metres
  };

  struct Joint
  {
    QString name;                        // as in the file (an alias is not changed)
    int priority=-1;                     // -1 means "use the motion's base priority"
    QVector<RotationKey> rotations;      // sorted by time, no time twice
    QVector<PositionKey> positions;      // sorted by time, no time twice
  };

  struct Constraint
  {
    int chainLength=0;
    int type=0;                          // 0 point, 1 plane
    QString sourceVolume;
    float sourceOffset[3]={0,0,0};
    QString targetVolume;
    float targetOffset[3]={0,0,0};
    float targetDirection[3]={0,0,0};
    float easeInStart=0,easeInStop=0,easeOutStart=0,easeOutStop=0;
  };

  struct Motion
  {
    int basePriority=0;                  // 0 to 6
    float duration=0.0f;                 // seconds, above 0 up to 60
    QString emote;                       // library name, empty for none
    bool loop=false;
    float loopIn=0.0f;                   // seconds, loopIn <= loopOut <= duration
    float loopOut=0.0f;
    float easeIn=0.0f;                   // seconds, not negative
    float easeOut=0.0f;
    int handPose=0;                      // 0 to 13
    QVector<Joint> joints;               // 1 to 216, in file order
    QVector<Constraint> constraints;     // 0 to 10
  };

  struct Result
  {
    std::optional<Motion> motion;        // set when the file was read; then error is empty
    QString error;                       // why the file cannot be read, for the user
    QStringList warnings;                // things that were changed or ignored
    bool ok() const { return motion.has_value(); }
  };

  Result parse(const QByteArray& data);
}

#endif
