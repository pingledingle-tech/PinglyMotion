/*
 * PinglyMotion - the beginner animation view (UX-E, docs/UX_DECISIONS.md, 6r)
 *
 * In Beginner mode the 3D figure gets most of the window: the Joints list (with the
 * Hands tab and its finger grid) is closed at first, and Edit curve stays behind the
 * Expert switch. The Selected joint panel (the three turn sliders) and the timeline
 * with Set key stay. Each hidden part has one visible way back: a plain button in the
 * Selected joint panel, and the View menu.
 *
 * The rules and the texts are here, without widgets, so the window and
 * tests/windowrules use the same ones.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef BEGINNERVIEW_H
#define BEGINNERVIEW_H

#include <QCoreApplication>
#include <QString>

namespace BeginnerView
{
  // the two buttons above the Selected joint panel, shown in Beginner mode only
  inline constexpr const char* showJointsButton = QT_TRANSLATE_NOOP("BeginnerView", "Show all joints");
  inline constexpr const char* showJointsTip = QT_TRANSLATE_NOOP("BeginnerView",
    "Open the Joints list with every joint, the Hands tab and the Avatar / Props switch. View > Joints does the same.");
  inline constexpr const char* showMoreButton = QT_TRANSLATE_NOOP("BeginnerView", "Show curves and finger grid");
  inline constexpr const char* showMoreTip = QT_TRANSLATE_NOOP("BeginnerView",
    "Turn on Expert mode: Edit curve, the finger grid and more. View > Expert mode turns it off again.");

  // what the status line adds when the mode changes the panels
  inline constexpr const char* beginnerLayoutMessage = QT_TRANSLATE_NOOP("BeginnerView",
    "The Joints list is closed, so the figure has more room. Show all joints opens it.");
  inline constexpr const char* expertLayoutMessage = QT_TRANSLATE_NOOP("BeginnerView",
    "The panels are as you left them in Expert mode.");
  inline constexpr const char* resetBeginnerMessage = QT_TRANSLATE_NOOP("BeginnerView",
    "Layout reset. The figure has most of the window; Show all joints opens the Joints list.");

  // The Joints list is part of the default layout in Expert mode only.
  inline bool jointsListInDefaultLayout(bool expert) { return expert; }

  // "Show all joints" is shown in Beginner mode while the Joints list is closed.
  inline bool showJointsButtonVisible(bool expert,bool jointsListOpen) { return !expert && !jointsListOpen; }

  // "Show curves and finger grid" is shown in Beginner mode, where those are hidden.
  inline bool showMoreButtonVisible(bool expert) { return !expert; }

  inline QString tr(const char* source) { return QCoreApplication::translate("BeginnerView",source); }
}

#endif
