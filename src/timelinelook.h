/*
 * PinglyMotion - colours of the timeline and its track names, from the tokens
 *
 * The timeline (src/timeline.cpp) and the track name column
 * (src/timelineview.cpp) ask these functions for every colour, so both follow
 * the dark and the light theme (design charter P1, P7). tests/uxcolours draws
 * the timeline in both themes and checks the pixels against these answers.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef TIMELINELOOK_H
#define TIMELINELOOK_H

#include <QColor>

#include "theme.h"

namespace TimelineLook
{
  using Theme::Token;

  // a row: surface, or selection for the selected joint
  inline QColor row(bool selected,bool dark)
  {
    return Theme::color(selected ? Token::Selection : Token::Surface,dark);
  }
  // every other block of ten frames is 35 percent toward the background
  inline QColor rowAlternate(bool selected,bool dark)
  {
    return Theme::mix(row(selected,dark),Theme::color(Token::Background,dark),0.35);
  }
  // the thin line through the middle of a row, and the line under it
  inline QColor midLine(bool selected,bool dark)
  {
    return Theme::mix(row(selected,dark),Theme::color(Token::Background,dark),0.7);
  }
  inline QColor midLineShade(bool selected,bool dark)
  {
    return Theme::mix(row(selected,dark),Theme::color(selected ? Token::OnAccent : Token::Text,dark),0.15);
  }
  // key frames and the lines between them: text, on_accent on the selected row
  inline QColor key(bool selected,bool dark)
  {
    return Theme::color(selected ? Token::OnAccent : Token::Text,dark);
  }
  // UX phase 3b: the selected stretch of the timeline, selection at 22 percent
  // (as the mockup's pressed view tools) with a 1 pixel selection border
  inline QColor selectionBox(bool dark)
  {
    QColor c=Theme::color(Token::Selection,dark);
    c.setAlphaF(0.22);
    return c;
  }
  inline QColor selectionBoxBorder(bool dark) { return Theme::color(Token::Selection,dark); }
  // the key being dragged
  inline QColor keyDragged(bool dark) { return Theme::color(Token::Danger,dark); }
  // the line at the current frame
  inline QColor currentFrame(bool dark) { return Theme::color(Token::Danger,dark); }
  // track names: text, dimmed for joints of hidden groups, on_accent when selected
  inline QColor name(bool selected,bool dimmed,bool dark)
  {
    return Theme::color(selected ? Token::OnAccent : dimmed ? Token::TextDisabled : Token::Text,dark);
  }

  // UX phase 3a: each row in the colour of its body area (JointGroups::Tab:
  // Body, Face, Hands, Other), the same tokens as the Joints panel's tabs
  inline QColor area(int area,bool dark)
  {
    static const Token tokens[]={Token::AreaBody,Token::AreaFace,Token::AreaHands,Token::AreaOther};
    return Theme::color(tokens[(area>=0 && area<4) ? area : 3],dark);
  }
  // the key frame row of a joint: surface with 5 percent of its area colour
  // (the mockup's .trow), selection for the selected joint
  inline QColor row(bool selected,int areaId,bool dark)
  {
    return selected ? row(true,dark) : Theme::mix(row(false,dark),area(areaId,dark),0.05);
  }
  inline QColor rowAlternate(bool selected,int areaId,bool dark)
  {
    return Theme::mix(row(selected,areaId,dark),Theme::color(Token::Background,dark),0.35);
  }
  inline QColor midLine(bool selected,int areaId,bool dark)
  {
    return Theme::mix(row(selected,areaId,dark),Theme::color(Token::Background,dark),0.7);
  }
  inline QColor midLineShade(bool selected,int areaId,bool dark)
  {
    return Theme::mix(row(selected,areaId,dark),Theme::color(selected ? Token::OnAccent : Token::Text,dark),0.15);
  }
  // the name of a joint: surface with 9 percent of its area colour (the
  // mockup's .tname), selection for the selected joint
  inline QColor nameRow(bool selected,int areaId,bool dark)
  {
    return selected ? row(true,dark) : Theme::mix(row(false,dark),area(areaId,dark),0.09);
  }
  // the stripe at the left of a joint's name: its area colour (border-left 4px)
  inline int areaStripeWidth() { return 4; }
  // the area's name at the right of a joint's name: text_muted (on_accent when selected)
  inline QColor areaName(bool selected,bool dark)
  {
    return Theme::color(selected ? Token::OnAccent : Token::TextMuted,dark);
  }
  // key frames: the area colour, on_accent on the selected row (the mockup's .key)
  inline QColor key(bool selected,int areaId,bool dark)
  {
    return selected ? key(true,dark) : area(areaId,dark);
  }
}

#endif
