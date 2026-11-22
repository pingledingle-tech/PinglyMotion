/*
 * PinglyMotion - icons from Lucide (https://lucide.dev, ISC licence, the
 * licence text is in src/icons/lucide/LICENSE)
 *
 * The one icon set of the design system (docs/DESIGN_SYSTEM.md, "Icons"):
 * outline icons on a 24 pixel grid, stroke 2, round ends and corners. The
 * path data of each icon is copied unchanged from Lucide's SVG file and drawn
 * here with QPainter, so the program needs no SVG module and no picture
 * files, and an icon takes any colour (the text colour next to it).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef LUCIDEICONS_H
#define LUCIDEICONS_H

#include <QColor>
#include <QIcon>
#include <QPainterPath>
#include <QString>
#include <QStringList>

namespace LucideIcons
{
  // the Lucide release the path data was copied from
  const char* version();

  // the icons copied so far, by their Lucide names ("play", "square",
  // "rotate-ccw")
  QStringList names();

  // the icon's outline on the 24 x 24 grid; empty for an unknown name
  QPainterPath path(const QString& name);

  // an SVG path ("M3 12a9 9 0 1 0 9-9 ...") as a painter path; knows the
  // commands Lucide uses: M L H V A C Z and their lower-case forms
  QPainterPath fromSvgPath(const QString& data);

  // the icon drawn in colour (Normal and Active) and in disabledColour
  // (Disabled) at 16, 20, 24, 32, 48 and 64 pixels (the larger ones
  // for sharp screens); a null icon for an unknown name
  QIcon icon(const QString& name,const QColor& colour,const QColor& disabledColour);
}

#endif
