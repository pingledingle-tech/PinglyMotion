/*
 * PinglyMotion - the program's text size, 100 to 200 percent
 *
 * The text size setting (UserSettings::textSizePercent, docs/UX_DECISIONS.md
 * section 8) scales the application font, and with it every widget that
 * takes its size from the font. Sizes that are written as numbers (the
 * design tokens of docs/DESIGN_SYSTEM.md, the timeline's row height) are
 * scaled with scaled(). The design system says sizes are given at 100
 * percent and grow with the scale setting (charter P9).
 *
 * The font the program starts with (the system font) is the 100 percent
 * font; apply() always scales that one, so applying 150 and then 200 gives
 * 200 percent of the system font, not 300.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef TEXTSIZE_H
#define TEXTSIZE_H

#include <QFont>
#include <QKeySequence>
#include <QList>

namespace TextSize
{
  constexpr int Minimum=100;
  constexpr int Maximum=200;
  constexpr int Default=100;

  // a percent cut to Minimum..Maximum
  int bounded(int percent);

  // one step of 25 percent up (stops at 200, never wraps to 100) or down (stops at 100)
  int stepUp(int percent);
  int stepDown(int percent);

  // `base` made `percent` percent as big (cut to 100..200): the point size
  // when the font has one, otherwise the pixel size. Nothing else changes.
  QFont scaledFont(const QFont& base,int percent);

  // Makes the application font `percent` percent (cut to 100..200) of the
  // font the program started with, and remembers the percent for scaled().
  // Every widget gets a font change event when the size really changes.
  // Answers the percent that was applied.
  int apply(int percent);

  // the percent set by the last apply(), 100 before the first one
  int percent();

  // a size in logical pixels at 100 percent, at the current percent
  // (rounded to the nearest pixel, never smaller than at 100 percent)
  int scaled(int size);
  // the same at a given percent (cut to 100..200)
  int scaled(int size,int percent);

  // The keys of View > Text size, Smaller text and Text size back to 100%
  // (docs/UX_DECISIONS.md, 6c). The first key is the one the menu shows.
  // Bigger is Ctrl and the plus key: on Nordic and German layouts the plus
  // key stands next to 0 and "=" needs Shift (Ctrl+Shift+0 is taken by
  // Windows), on a US layout Ctrl+= is the same key without Shift. The
  // number pad's plus, minus and 0 work too. No two keys of one list are
  // the same key press, so Qt never sees an ambiguous shortcut.
  QList<QKeySequence> biggerKeys();
  QList<QKeySequence> smallerKeys();
  QList<QKeySequence> resetKeys();

  // forgets the start font and goes back to 100 percent (tests only)
  void resetForTests();
}

#endif
