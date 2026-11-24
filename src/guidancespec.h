/*
 * PinglyMotion - the texts of the guidance (UX phase 6a)
 *
 * The welcome card and the hint cards, word for word as in the mockup
 * (docs/mockup/index.html: #dlgWelcome, CARDS, #hintcard), checked against
 * the mockup file by tests/guidance. One table for the widgets and the test.
 *
 * Differences from the mockup, on purpose (docs/UX_DECISIONS.md, 6k): the
 * three start choices use the empty state's lines (src/emptystate.h), because
 * the mockup's lines claim a note on each key that the library clips do not
 * have; and the card "Turn it" says "the coloured rings" only because the
 * rings exist since UX phase 4b.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef GUIDANCESPEC_H
#define GUIDANCESPEC_H

#include <QCoreApplication>
#include <QString>

namespace GuidanceSpec
{
  // ---- the welcome card ----
  inline constexpr const char* welcomeTitle = QT_TRANSLATE_NOOP("WelcomeDialog", "Make your first animation");
  inline constexpr const char* welcomeIntro = QT_TRANSLATE_NOOP("WelcomeDialog",
    "Pick a pose to start. You can change everything later in <b>Setup</b> (the gear button at the top right).");
  inline constexpr const char* experienceLabel = QT_TRANSLATE_NOOP("WelcomeDialog", "Have you made animations before?");
  inline constexpr const char* newChoice = QT_TRANSLATE_NOOP("WelcomeDialog", "New to animation");
  inline constexpr const char* proChoice = QT_TRANSLATE_NOOP("WelcomeDialog", "Experienced");
  inline constexpr const char* cameraLabel = QT_TRANSLATE_NOOP("WelcomeDialog", "Camera and keys like");
  inline constexpr const char* skipButton = QT_TRANSLATE_NOOP("WelcomeDialog", "Skip");

  // what the status line says after the card (the mockup's feedback lines)
  inline constexpr const char* skipMessage = QT_TRANSLATE_NOOP("WelcomeDialog",
    "Welcome card closed. Setup (top right) or Help brings it back.");
  inline constexpr const char* readyMessage = QT_TRANSLATE_NOOP("WelcomeDialog",
    "Ready. Click a joint on the figure to pick it.");

  // ---- the hint cards, one at a time ----
  struct Card
  {
    const char* id;       // object name part and target name
    const char* title;
    const char* text;
    const char* target;   // which part of the window the card stands by
  };

  inline constexpr Card cards[] = {
    {"pickJoint", QT_TRANSLATE_NOOP("HintCard", "Pick a joint"),
     QT_TRANSLATE_NOOP("HintCard", "Click a joint on the figure to pick it. You can also use the Joints list."), "view"},
    {"pullLimb", QT_TRANSLATE_NOOP("HintCard", "Pull a limb"),
     QT_TRANSLATE_NOOP("HintCard", "Press a joint and drag. The limb follows your pointer. Let go to set a key. Esc cancels. Hold Shift when you press to turn only that joint. No mouse? Press Enter on the picked joint and use the arrow keys."), "view"},
    {"turnIt", QT_TRANSLATE_NOOP("HintCard", "Turn it"),
     QT_TRANSLATE_NOOP("HintCard", "Drag one of the coloured rings around the picked joint, or use the sliders on the right."), "selected"},
    {"axisColours", QT_TRANSLATE_NOOP("HintCard", "Red, green, blue = X, Y, Z"),
     QT_TRANSLATE_NOOP("HintCard", "Everywhere in PinglyMotion, X is red, Y is green and Z is blue: the rings, the sliders and the axis widget. The letters are always shown too."), "selected"},
    {"saveThePose", QT_TRANSLATE_NOOP("HintCard", "Save the pose"),
     QT_TRANSLATE_NOOP("HintCard", "Saves the pose you see now as a key. A key is a saved pose."), "setKey"},
    {"watchIt", QT_TRANSLATE_NOOP("HintCard", "Watch it"),
     QT_TRANSLATE_NOOP("HintCard", "Plays your animation from the start. Press it again to stop."), "start"},
    {"changeHowItMoves", QT_TRANSLATE_NOOP("HintCard", "Change how it moves"),
     QT_TRANSLATE_NOOP("HintCard", "Pick a key on the timeline, then an easing choice. Each tile draws the move: a curve from Start to End."), "tiles"},
    {"movePanels", QT_TRANSLATE_NOOP("HintCard", "Move the panels"),
     QT_TRANSLATE_NOOP("HintCard", "Every panel has a pop-out button in its title. Pop it out to move it anywhere, even to a second screen. View > Reset layout puts everything back."), "joints"},
    {"setupAnyTime", QT_TRANSLATE_NOOP("HintCard", "Setup any time"),
     QT_TRANSLATE_NOOP("HintCard", "Beginner or experienced, mouse habits, theme and text size are all in Setup, the gear button at the top right."), "setup"},
  };
  inline constexpr int cardCount = int(sizeof(cards) / sizeof(cards[0]));

  inline constexpr const char* hintsOffButton = QT_TRANSLATE_NOOP("HintCard", "Turn hints off");
  inline constexpr const char* hintsOffName = QT_TRANSLATE_NOOP("HintCard", "Turn all hints off (Help > Show hints)");
  inline constexpr const char* nextButton = QT_TRANSLATE_NOOP("HintCard", "Next");
  inline constexpr const char* nextName = QT_TRANSLATE_NOOP("HintCard", "Next hint");
  inline constexpr const char* closeName = QT_TRANSLATE_NOOP("HintCard", "Close this hint");
  inline constexpr const char* progress = QT_TRANSLATE_NOOP("HintCard", "Hint %1 of %2");
  inline constexpr const char* hintsOffMessage = QT_TRANSLATE_NOOP("HintCard",
    "Hints off. Turn them on again in Setup or Help > Show hints.");
  // the close button or Esc on a hint card (UX small fixes, docs/TEACHING.md HINT-CLOSED-01)
  inline constexpr const char* closedMessage = QT_TRANSLATE_NOOP("HintCard",
    "Hint closed. To see the next one, turn Help > Show hints off and on.");

  // Expert and Beginner (the mockup's feedback lines)
  inline constexpr const char* expertOnMessage = QT_TRANSLATE_NOOP("GuidanceSpec", "Expert mode: more controls, nothing moved.");
  inline constexpr const char* expertOffMessage = QT_TRANSLATE_NOOP("GuidanceSpec", "Beginner mode: expert controls hidden.");

  inline QString tr(const char* source, const char* context) { return QCoreApplication::translate(context, source); }
}

#endif
