/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The look of the transport buttons under the timeline, in the design system
 * (docs/DESIGN_CHARTER.md, docs/DESIGN_SYSTEM.md): every button has the same
 * shape (radius.control, border_width, at least control_height.compact high),
 * Start is the main button (accent), Stop a danger button (danger_fill) and
 * every other button in the bar a normal button (surface_raised). The icons
 * are Lucide's play, square and rotate-ccw in the colour of the button's text.
 * All colours are theme tokens (src/theme.*); when View > Dark theme changes
 * the theme, the style sheets and icons are made again.
 *
 * The buttons are drawn here, not with a style sheet: a style sheet on a
 * button stops it from following a change of the text size, and it cannot
 * draw the focus ring outside the button. Keyboard focus shows a focus_ring
 * ring, 2 pixels wide and 2 pixels outside the button, following its radius;
 * the button keeps a margin of 4 pixels for it. A click with the mouse gives
 * focus without the ring. The size follows the text: the button grows with
 * the system text size and the program's scale instead of cutting text off.
 *
 * Only the look lives here; what the buttons do is in transportbar.*.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef TRANSPORTLOOK_H
#define TRANSPORTLOOK_H

#include <QColor>
#include <QIcon>
#include <QSize>
#include <QString>

class QPushButton;
class QWidget;

namespace TransportLook
{
  // the three kinds of button in docs/DESIGN_SYSTEM.md, "Shape"
  enum class Kind
  {
    Main,
    Normal,
    Danger
  };

  // Start is the main button, Stop the danger button, anything else normal
  Kind kind(const QPushButton* button);

  // the Lucide icon of the button, empty for a button the look gives none
  QString iconName(const QPushButton* button);

  // colour of the text and the icon of an enabled button of this kind
  QColor textColour(Kind kind,bool dark);

  // the margin around a button that the focus ring is drawn in
  // (focus_ring.offset + focus_ring.width)
  int ringMargin();

  // the states of docs/DESIGN_SYSTEM.md, "States", that change the colours
  // (focus adds the ring on top)
  enum class State
  {
    Normal,
    Hover,
    Pressed,
    Disabled
  };

  struct Colours
  {
    QColor fill;
    QColor border;   // at 50 percent opacity when disabled
    QColor text;     // also the icon
  };
  Colours colours(Kind kind,State state,bool dark);
  // the state the button is in now
  State state(const QPushButton* button);

  // the smallest size that shows the icon and the whole text with the
  // padding, at least control_height.compact high, plus the ring margin
  QSize minimumSize(const QPushButton* button);

  // the icon of the button in its colours (null when iconName is empty)
  QIcon icon(const QPushButton* button,bool dark);

  // whether the button shows the keyboard focus ring now, and setting it
  // (when the bar moves the focus to another button itself)
  bool showsFocusRing(const QPushButton* button);
  void setFocusRing(QPushButton* button,bool on);

  // gives every button in the bar its look and focus ring, also buttons added
  // later, and makes the look again when the theme of the program changes
  void apply(QWidget* bar);
}

#endif
