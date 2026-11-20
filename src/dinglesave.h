/*
 * PinglyMotion - file name rules for saving .dingle files (issue #26, jobs 5 and 6)
 *
 * Save always writes .dingle. A new animation is called pingle.dingle. An
 * opened .avm file is replaced: it is saved as .dingle with the same name, and
 * only after that worked the old .avm goes to the recycle bin. An opened .bvh
 * file is left alone. These functions have no window and no disk access
 * (except looking at paths), so they can be tested.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef DINGLESAVE_H
#define DINGLESAVE_H

#include <QString>

#include "animfile.h"

namespace DingleSave
{
  // the name of a new animation
  QString defaultName();

  // true when Save has to ask for a name: a new animation, or an opened .avm or .bvh file
  bool needsName(const QString& currentFile,const QString& untitledName);

  // the name to offer in the Save dialog: the open file's name with .dingle,
  // or pingle.dingle for a new animation
  QString suggestedName(const QString& currentFile,const QString& untitledName);

  // the file name with .dingle added when it has another ending or none
  QString withDingleEnding(const QString& fileName);

  // true when saving oldFile's animation as newFile replaces the old .avm file:
  // oldFile is an .avm and newFile is the .dingle with the same folder and name
  bool replacesOldFile(const QString& oldFile,const QString& newFile);

  // the check before an old .avm goes to the recycle bin: the saved file, read
  // back, is the animation in memory. Same joints, timing, key frames, loop,
  // figure and scale; values may differ by the tolerance (the text of the
  // file holds a few digits). The position key list is not compared.
  bool sameAnimation(const AnimFile::Animation& saved,const AnimFile::Animation& memory,float tolerance=1.0e-3f);
  // the first thing that differs, for a test message; empty when sameAnimation() is true
  QString firstDifference(const AnimFile::Animation& saved,const AnimFile::Animation& memory,float tolerance=1.0e-3f);
}

#endif
