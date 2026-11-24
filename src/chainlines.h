/*
 * PinglyMotion - chain and ribbon lines in the 3D view (Script Lab, job A4, part 2)
 *
 * What the 3D view needs to draw a chain a script made between two props
 * (scriptlab/labchains.h): the pieces of the straight line on screen, its
 * colour and the word beside it. Locked: one solid line in the danger colour
 * and the word Locked. Free: a dashed line in the success colour and the word
 * Free. The word stands in a small tag in the middle of the line, with the
 * line's colour as text and border on the surface colour, so the meaning
 * never rests on colour alone (design charter P10). No physics: a straight
 * line between the props' centres.
 *
 * Header only, so a test can check it without the 3D view.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef CHAINLINES_H
#define CHAINLINES_H

#include <cmath>

#include <QColor>
#include <QList>
#include <QPointF>
#include <QString>
#include <QVector>

#include "theme.h"

namespace ChainLines
{
  struct Line
  {
    QString from;      // the prop the chain starts at
    QString to;        // the prop it goes to
    bool locked = false;
    QString word;      // "Locked" or "Free"
  };

  // dashes of a free chain, in pixels at 100 percent (scaled with the text size)
  constexpr double DashOn = 8.0;
  constexpr double DashOff = 6.0;
  // the line's width in pixels at 100 percent
  constexpr double Width = 3.0;

  // the pieces of the line from a to b on screen, as pairs of points: one piece when locked,
  // dashes when free
  inline QVector<QPointF> pieces(const QPointF& a, const QPointF& b, bool locked, double scale)
  {
    QVector<QPointF> out;
    const double length = std::hypot(b.x() - a.x(), b.y() - a.y());
    if (locked || length <= 0.0)
    {
      out << a << b;
      return out;
    }
    const double on = DashOn * scale, off = DashOff * scale;
    for (double at = 0.0; at < length; at += on + off)
    {
      const double end = std::min(length, at + on);
      out << a + (b - a) * (at / length) << a + (b - a) * (end / length);
    }
    return out;
  }

  // the line's colour: danger when locked, success when free (theme tokens)
  inline QColor colour(bool locked, bool dark)
  {
    return Theme::color(locked ? Theme::Token::Danger : Theme::Token::Success, dark);
  }

  // the word's tag: the line's colour as text and border, on surface, radius.small
  inline QString tagStyle(bool locked, bool dark, int radius)
  {
    const QColor c = colour(locked, dark);
    return QStringLiteral("QLabel { background: %1; color: %2; border: 1px solid %2; border-radius: %3px; padding: 0px 4px; }")
      .arg(Theme::color(Theme::Token::Surface, dark).name(), c.name()).arg(radius);
  }

  // where the word stands: the middle of the line
  inline QPointF middle(const QPointF& a, const QPointF& b) { return (a + b) / 2.0; }
}

#endif
