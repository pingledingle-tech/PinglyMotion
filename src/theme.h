/*
 * PinglyMotion - light and dark look for the whole program
 *
 * The design tokens of docs/DESIGN_SYSTEM.md (docs/design_tokens.json) live
 * here: widgets ask the theme for a colour or a size and never write one by
 * hand (design charter P1, P7). tests/transportbar checks that the values
 * here are the ones in docs/design_tokens.json.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QList>

namespace Theme
{
  // Fusion style with a dark or light palette; also sets the theme that
  // color(Token) answers for
  void apply(bool dark);

  // the theme set by the last apply() (dark before the first one, as the
  // program starts in the dark theme)
  bool isDark();

  // the colour tokens, in the order of docs/design_tokens.json
  enum class Token
  {
    Background,
    Surface,
    SurfaceRaised,
    Border,
    Text,
    TextMuted,
    TextDisabled,
    Accent,
    OnAccent,
    AccentText,
    FocusRing,
    Danger,
    DangerFill,
    Success,
    Warning,
    Selection,
    AreaBody,
    AreaFace,
    AreaHands,
    AreaOther,
    AxisX,
    AxisY,
    AxisZ,
    OnAxis,
    GoFill,
    ReplayFill
  };

  QList<Token> tokens();
  // the token's name in docs/design_tokens.json, for example "surface_raised"
  const char* tokenName(Token token);
  QColor color(Token token,bool dark);
  // in the current theme
  QColor color(Token token);

  // a mixed with share t (0 to 1) of b, as the "States" table says
  // ("mixes 8 percent text in")
  QColor mix(const QColor& a,const QColor& b,double t);

  // sizes in logical pixels at 100 percent (docs/design_tokens.json)
  namespace Size
  {
    constexpr int radiusSmall=4;
    constexpr int radiusControl=6;
    constexpr int radiusPanel=8;
    constexpr int borderWidth=1;
    constexpr int focusRingWidth=2;
    constexpr int focusRingOffset=2;
    constexpr int controlCompact=24;
    constexpr int controlDefault=32;
    constexpr int controlLarge=40;
    constexpr int spacingXs=4;
    constexpr int spacingS=8;
    constexpr int spacingM=12;
    constexpr int spacingL=16;
    constexpr int spacingXl=24;
    constexpr int spacingXxl=32;
    constexpr int iconGrid=24;
    constexpr int iconStroke=2;
    constexpr int iconSmall=16;
    constexpr int labelWeight=600;
    // the line between two docked panels (and a splitter), wide enough to grab
    // easily: spacing s (UX fixes 1, docs/UX_DECISIONS.md 6c)
    constexpr int panelGrab=spacingS;
    // the edge of a floating panel that resizes it: spacing xs
    constexpr int floatingEdge=spacingXs;
  }

  // the two sizes above at the current text size (TextSize::scaled); the style
  // that Theme::apply() sets answers them for PM_DockWidgetSeparatorExtent,
  // PM_SplitterWidth and PM_DockWidgetFrameWidth
  int panelGrabWidth();
  int floatingEdgeWidth();
}

#endif
