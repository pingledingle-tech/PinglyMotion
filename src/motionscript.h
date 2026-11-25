/*
 * PinglyMotion - the motion script: an animation as a small table in words (issue #129)
 *
 * An AI assistant plans movement well in words and numbers, so this is a plain
 * text form of a whole animation that it can write by hand and read back:
 *
 *   # PinglyMotion motion script 1
 *   fps 30
 *   frames 46
 *   loop 2 46
 *
 *   frame | joint    | x   | y   | z   | easing
 *   1     | position | 0   | 43.5| 0   |
 *   2     | rShldr   | -90 | 0   | -20 | ease-in-out
 *   0.5s  | rForeArm | 0   | 110 | 0   | ease-in-out
 *
 * Every table row is one key: the frame (a frame number from 1, or seconds
 * with an "s", which become frame = seconds x fps + 1), the joint (a name from
 * the joints command, or "position" for the hip position), the three angles in
 * degrees (or the hip position in the program's units, inches) and the easing
 * of the movement into this key from the joint's key before (one of the six
 * names; empty means ease-in-out, the program's default). Lines starting with
 * # are notes; empty lines are skipped; a markdown header row (first cell
 * "frame") and its |---| line are skipped too, so a markdown table works.
 *
 * write() turns an animation into this text, parse() reads the text and
 * apply() builds the animation from it. write(apply(parse(text))) gives the
 * same text, and an animation that went through write() and apply() plays
 * the same: the tests check both, frame by frame.
 *
 * Pure model code: no files, no window. The control commands motion_script
 * and load_motion_script (src/controlapi.cpp) use it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef MOTIONSCRIPT_H
#define MOTIONSCRIPT_H

#include <optional>

#include <QList>
#include <QString>
#include <QStringList>

#include "easing.h"

class Animation;

namespace MotionScript
{
  // limits, so a script from outside can never make the program slow or big
  constexpr int maxTextChars = 4 * 1024 * 1024;
  constexpr int maxRows = 200000;
  constexpr int maxFrames = 10000;
  constexpr int maxFps = 60;
  constexpr double maxAngle = 360.0;
  constexpr double maxPosition = 10000.0;
  // the header line write() puts first
  inline constexpr const char* firstLine = "# PinglyMotion motion script 1";
  // the name used in the joint column for the hip position
  inline constexpr const char* positionName = "position";

  struct Row
  {
    int line = 0;           // line number in the text, from 1, for error messages
    int frame = 0;          // 1-based
    QString joint;          // a joint name, or "position"
    double value[3] = {0, 0, 0};
    // the movement into this key; empty: the default (ease-in-out)
    std::optional<Easing::Preset> easing;
  };

  struct Script
  {
    int fps = 30;
    int frames = 0;         // 0 when the text did not say: the last row's frame
    int loopIn = 0;         // 0: no loop line (the default loop of a new animation)
    int loopOut = 0;
    QList<Row> rows;
  };

  struct Result
  {
    std::optional<Script> value;
    QString error;          // plain words for the user and the AI, with the line number
    bool ok() const { return error.isEmpty() && value.has_value(); }
  };

  // reads a motion script; checks everything that can be checked without a skeleton
  Result parse(const QString& text);

  struct Written
  {
    QString text;
    int rows = 0;
    // joints whose key uses a custom curve from the curve editor, written as
    // "custom"; such a script cannot be loaded again until they get a named easing
    QStringList custom;
  };

  // the open animation as a motion script: fps, frames, loop, then one row per
  // key, in frame order and skeleton order. Frame 1 rows are written for the
  // hip position and for every joint that is not at 0 there; later frames for
  // every key. A non-empty "only" keeps the rows of those joints ("position" for the hip).
  Written write(Animation* animation, const QStringList& only = QStringList());

  // the checks apply() makes against the skeleton of "animation", without
  // changing it: unknown joints (with a suggestion), rows outside the frames,
  // the same key twice, a loop that does not fit. Empty when all is well.
  QString check(Animation* animation, const Script& script);

  struct Applied
  {
    QString error;
    QStringList outsideLimits;   // joints with a key past the joint's limits (kept, not cut back)
    int keys = 0;
    bool ok() const { return error.isEmpty(); }
  };

  // replaces every key of "animation" by the script: frame count, fps, loop
  // and the keys of every joint and of the hip position. A joint the script
  // does not name keeps only a key at frame 1 with angle 0; the hip position
  // keeps its frame 1 value unless the script gives one. Checks everything
  // first (unknown joints with a suggestion, rows past the last frame, the
  // same key twice, "custom" easing); on an error nothing is changed.
  Applied apply(Animation* animation, const Script& script);
}

#endif
