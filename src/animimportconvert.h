/*
 * PinglyMotion - turn a read Second Life .anim file into an animation of the
 * program (issue #22, menu step)
 *
 * The reader (src/animimport.*) keeps what is in the file: joints with turn
 * and position keys in seconds, metres and quaternions. This module puts those
 * keys onto the Second Life skeleton of the program (a BVH style animation,
 * sampled at 30 frames per second, frame 0 is the reference pose). It is the
 * opposite of src/animexport.* (same axes, same units) and says in plain words
 * what could not be imported. No window, no disk.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef ANIMIMPORTCONVERT_H
#define ANIMIMPORTCONVERT_H

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <optional>

#include "animfile.h"
#include "animimport.h"

namespace AnimImportConvert
{
  constexpr double FRAME_TIME=1.0/30.0;
  constexpr int MAX_LISTED=8;          // names listed in a note before "and N more"

  struct Result
  {
    std::optional<AnimFile::Animation> animation;   // BVH format, on the target skeleton
    QString error;                                   // why nothing was imported, with what to do
    QStringList notes;                               // what could not be imported, in plain words
    // loop points in the numbers the window uses (-1 when the file does not loop)
    int loopIn=-1;
    int loopOut=-1;

    bool ok() const { return animation.has_value(); }
  };

  // put the motion onto the skeleton; the target is a Second Life skeleton
  // with its rest pose in frame 0 (for example bin/data/SLFemale.bvh)
  Result convert(const AnimImport::Motion& motion,const AnimFile::Animation& target);

  // read the bytes of a .anim file and convert them
  Result fromBytes(const QByteArray& bytes,const AnimFile::Animation& target);

  // "a, b and 3 more" (at most MAX_LISTED names)
  QString listNames(const QStringList& names);
}

#endif
