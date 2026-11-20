/*
 * PinglyMotion - the clean-up choices of the import dialog (issue #17)
 *
 * The BVH and glTF import dialog (src/retargetdialog.*) lets the user switch
 * the clean-up steps of src/motioncleanup.* on and off. This file holds the
 * choices, what each choice gives when it is applied, their plain-words
 * labels and tips, and how they are remembered between imports. It has no
 * window, so all of it can be tested.
 *
 * Foot lock (planted feet that do not slide) is built as a function by
 * another job. When it exists, add ONE bool `footLock` to CleanupOptions
 * (default off), one line in apply() between inPlace and blendLoopSeam, one
 * entry in rows(), and its key in load() and save(). The dialog builds its
 * check boxes from rows(), so it needs no other change.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef IMPORTOPTIONS_H
#define IMPORTOPTIONS_H

#include "animfile.h"
#include "motioncleanup.h"

#include <QList>
#include <QString>

class QSettings;

namespace ImportOptions
{
  // Defaults: resample and start pose ON. Second Life plays 30 frames a
  // second, and it compares every frame with frame 1, so these two never
  // change how the motion looks and make it work in Second Life. The other
  // three change the motion itself (the walk stays in one place, the end is
  // bent towards the start, small movements are blurred), so they are OFF
  // until the user chooses them.
  struct CleanupOptions
  {
    bool resample=true;      // to 30 frames a second
    bool startPose=true;     // a start pose at frame 1
    bool inPlace=false;      // remove the forward travel of the hip
    bool loopSeam=false;     // ease the end into the first frame
    bool smooth=false;       // light smoothing against shake
    // footLock goes here (see the top of this file)

    bool operator==(const CleanupOptions& other) const;
    bool operator!=(const CleanupOptions& other) const { return !(*this==other); }
  };

  // one check box of the dialog
  struct Row
  {
    QString key;       // the name in the settings file
    QString label;     // plain words, short
    QString tip;       // one or two short sentences
    bool CleanupOptions::* field;
  };

  // the check boxes of the dialog, in the order they are shown
  const QList<Row>& rows();

  // run the chosen steps on a finished animation in the suggested order:
  // resample, smooth, in place, loop seam, start pose. With nothing chosen
  // the animation is returned unchanged. On the first step that fails the
  // result holds an error that says what to do, never half a result.
  MotionCleanup::Result apply(const AnimFile::Animation& anim,const CleanupOptions& options);

  // the options saved in the settings (the defaults for a key that is missing
  // or holds something that is not true or false)
  CleanupOptions load(QSettings& settings);
  void save(QSettings& settings,const CleanupOptions& options);

  // the settings file of the program (the same one UserSettings uses)
  CleanupOptions load();
  void save(const CleanupOptions& options);
}

#endif
