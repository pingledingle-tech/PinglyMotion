/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The look of the transport buttons under the timeline: a green Start, a red
 * Stop, and Replay from start with a round arrow. The other buttons in the
 * bar get the same rounded shape in the theme's own colours. The icons are
 * drawn here, so there is no picture file to keep in step with the theme:
 * when the theme changes, the style sheet is made again from the new colours
 * and Replay's arrow is drawn again in the new text colour.
 *
 * Only the look lives here; what the buttons do is in transportbar.*.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef TRANSPORTLOOK_H
#define TRANSPORTLOOK_H

#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QString>

class QPushButton;
class QWidget;

namespace TransportLook
{
  // the colours of the two coloured buttons
  QColor startColour();
  QColor stopColour();

  // white triangle, white square, round arrow in the given colour
  QIcon startIcon();
  QIcon stopIcon();
  QIcon replayIcon(const QColor& colour);

  // the style sheet of the buttons in these theme colours (Start and Stop by
  // object name)
  QString styleSheet(const QPalette& palette);

  // gives the buttons in the bar the look, and makes it again when the
  // theme of the program changes
  void apply(QWidget* bar,QPushButton* start,QPushButton* stop,QPushButton* replay);
}

#endif
