/*
 * PinglyMotion - colours of the 3D view's own drawing, from the design tokens
 *
 * The background, the avatar's body, the rotation rings, the axis lines of a
 * prop, the joint rings, the bones and the effect markers are drawn in these
 * colours (src/animationview.cpp).
 * Every colour is a token of docs/design_tokens.json in the current theme or
 * the theme given; nothing is written by hand (design charter P1, P7).
 * tests/uxcolours checks each answer against the token for both themes.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef VIEWCOLOURS_H
#define VIEWCOLOURS_H

#include <QColor>
#include <QVector4D>

#include "theme.h"

namespace ViewColours
{
  // the colour as the 4 floats the view's shaders take
  inline QVector4D vector(const QColor& colour)
  {
    return QVector4D(colour.redF(),colour.greenF(),colour.blueF(),1.0f);
  }

  // axis 0 = X (axis_x, red), 1 = Y (axis_y, green), 2 = Z (axis_z, blue)
  inline QColor axis(int axis,bool dark)
  {
    using Theme::Token;
    return Theme::color(axis==0 ? Token::AxisX : axis==1 ? Token::AxisY : Token::AxisZ,dark);
  }
  inline QColor axis(int axisIndex) { return axis(axisIndex,Theme::isDark()); }

  // a rotation ring: the axis colour; a ring that is not the one in use is
  // 20 percent toward the background (the States rule of docs/DESIGN_SYSTEM.md)
  inline QColor ring(int axisIndex,bool strong,bool dark)
  {
    const QColor colour=axis(axisIndex,dark);
    return strong ? colour : Theme::mix(colour,Theme::color(Theme::Token::Background,dark),0.2);
  }
  inline QColor ring(int axisIndex,bool strong) { return ring(axisIndex,strong,Theme::isDark()); }

  // the ring on a joint (src/jointrings.h, the mockup's .jt): the edge is
  // accent_text under the mouse, on_accent on the selected joint, else text
  inline QColor jointRingEdge(bool selected,bool hovered,bool dark)
  {
    using Theme::Token;
    return Theme::color(hovered ? Token::AccentText : selected ? Token::OnAccent : Token::Text,dark);
  }
  // the inside: selection on the selected joint, the body area's colour on a
  // changed joint (area 0 Body, 1 Face, 2 Hands, 3 Other), else none: an
  // invalid colour, the ring is hollow
  inline QColor jointRingFill(bool selected,bool changed,int area,bool dark)
  {
    using Theme::Token;
    if(selected) return Theme::color(Token::Selection,dark);
    if(!changed) return QColor();
    static const Token areas[]={Token::AreaBody,Token::AreaFace,Token::AreaHands,Token::AreaOther};
    return Theme::color(areas[(area>=0 && area<4) ? area : 3],dark);
  }

  // the line from a joint to its parent (the mockup's .bone): text_muted;
  // strong (to the selected joint, or in a pulled limb) accent_text
  inline QColor bone(bool strong,bool dark)
  {
    return Theme::color(strong ? Theme::Token::AccentText : Theme::Token::TextMuted,dark);
  }
  inline QColor bone(bool strong) { return bone(strong,Theme::isDark()); }

  // the 3D view's background, and the colour the fog fades toward: the
  // background token, the same as the window behind the panels (the mockup's
  // .view lies on the page's background)
  inline QColor background(bool dark) { return Theme::color(Theme::Token::Background,dark); }
  inline QColor background() { return background(Theme::isDark()); }

  // the floor's two kinds of squares (src/floor.cpp): a quiet checkerboard made from the theme
  // itself, so it reads in both themes without shouting: background with 12 percent (the
  // fainter square) or 28 percent (the stronger) text_muted mixed in. While the frame is
  // protected (the first frame) the same two shares of danger_fill are mixed into the
  // background instead. Asked for at every picture, so a theme switch shows at once.
  inline QColor floorTile(bool strong,bool protectedFrame,bool dark)
  {
    using Theme::Token;
    const QColor from=Theme::color(protectedFrame ? Token::DangerFill : Token::TextMuted,dark);
    return Theme::mix(Theme::color(Token::Background,dark),from,strong ? 0.28 : 0.12);
  }
  inline QColor floorTile(bool strong,bool protectedFrame) { return floorTile(strong,protectedFrame,Theme::isDark()); }

  // the avatar's body in the 3D view (the base colour the shader lights; the
  // light itself and Options > Light... are the same in both themes):
  // Normal text_muted (as the bones, at least 4.5:1 on the background in both
  // themes), Hovered accent_text (as a joint ring under the mouse), Selected
  // selection, MirrorPartner (the other side while Mirror is on) halfway from
  // selection to the body colour, Pulled (a limb being pulled) accent_text.
  // A joint moved by IK has 30 percent accent mixed in.
  enum class BodyPart { Normal, Hovered, Selected, MirrorPartner, Pulled };
  inline QColor body(BodyPart part,bool ik,bool dark)
  {
    using Theme::Token;
    QColor colour;
    switch(part)
    {
      case BodyPart::Hovered: colour=Theme::color(Token::AccentText,dark); break;
      case BodyPart::Selected: colour=Theme::color(Token::Selection,dark); break;
      case BodyPart::MirrorPartner:
        colour=Theme::mix(Theme::color(Token::Selection,dark),Theme::color(Token::TextMuted,dark),0.5);
        break;
      case BodyPart::Pulled: colour=Theme::color(Token::AccentText,dark); break;
      case BodyPart::Normal: default: colour=Theme::color(Token::TextMuted,dark); break;
    }
    return ik ? Theme::mix(colour,Theme::color(Token::Accent,dark),0.3) : colour;
  }
  inline QColor body(BodyPart part,bool ik) { return body(part,ik,Theme::isDark()); }

  // effect preview markers: a box for an object, a ball for a lock
  inline QColor objectMarker(bool dark) { return Theme::color(Theme::Token::Accent,dark); }
  inline QColor objectMarker() { return objectMarker(Theme::isDark()); }
  inline QColor lockMarker(bool dark) { return Theme::color(Theme::Token::DangerFill,dark); }
  inline QColor lockMarker() { return lockMarker(Theme::isDark()); }
}

#endif
