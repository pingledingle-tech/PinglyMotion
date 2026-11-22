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
  // the key being dragged
  inline QColor keyDragged(bool dark) { return Theme::color(Token::Danger,dark); }
  // the line at the current frame
  inline QColor currentFrame(bool dark) { return Theme::color(Token::Danger,dark); }
  // track names: text, dimmed for joints of hidden groups, on_accent when selected
  inline QColor name(bool selected,bool dimmed,bool dark)
  {
    return Theme::color(selected ? Token::OnAccent : dimmed ? Token::TextDisabled : Token::Text,dark);
  }
}

#endif
