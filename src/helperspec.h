/*
 * PinglyMotion - the texts and lessons of helper mode (job UX-C, docs/UX_DECISIONS.md, 6y)
 *
 * One table for the widgets (src/helpermode.*) and the test (tests/helpermode). A lesson is a
 * short row of steps. Each step names the real control it is about (its target, by the same
 * names the window answers in PinglyMotionWindow::helperTarget) and the thing the user does
 * there (its event, sent by the window with HelperMode::notify). A step goes on when that thing
 * happens, so every step visibly does something. Only controls that exist are used.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef HELPERSPEC_H
#define HELPERSPEC_H

#include <QCoreApplication>
#include <QString>

namespace HelperSpec
{
  // ---- the switch at the top ----
  inline constexpr const char* switchId = "helperSwitchAction";
  inline constexpr const char* switchText = QT_TRANSLATE_NOOP("HelperMode", "Helper");
  inline constexpr const char* switchTipOn = QT_TRANSLATE_NOOP("HelperMode",
    "Helper is on: tips and short lessons beside the tool. Click to turn it off.");
  inline constexpr const char* switchTipOff = QT_TRANSLATE_NOOP("HelperMode",
    "Helper is off: a clean tool. Click to turn on tips and short lessons.");
  // the same setting as Help > Show hints, so the switch shows that item's key
  inline constexpr const char* switchKey = "Shift+H";
  inline constexpr const char* onMessage = QT_TRANSLATE_NOOP("HelperMode",
    "Helper on. Tips and lessons are in the helper at the bottom right.");
  inline constexpr const char* offMessage = QT_TRANSLATE_NOOP("HelperMode",
    "Helper off. The Helper button at the top turns it on again.");

  // ---- the helper panel ----
  inline constexpr const char* panelTitle = QT_TRANSLATE_NOOP("HelperMode", "Helper");
  inline constexpr const char* closeName = QT_TRANSLATE_NOOP("HelperMode", "Turn the helper off");
  inline constexpr const char* lessonsHeading = QT_TRANSLATE_NOOP("HelperMode", "Lessons");
  inline constexpr const char* stepProgress = QT_TRANSLATE_NOOP("HelperMode", "Step %1 of %2");
  inline constexpr const char* stopLesson = QT_TRANSLATE_NOOP("HelperMode", "Stop lesson");
  inline constexpr const char* stopName = QT_TRANSLATE_NOOP("HelperMode", "Stop this lesson and go back to the lessons (Esc)");
  inline constexpr const char* doneTitle = QT_TRANSLATE_NOOP("HelperMode", "Lesson done");
  inline constexpr const char* backToLessons = QT_TRANSLATE_NOOP("HelperMode", "Back to lessons");
  inline constexpr const char* needsAnimation = QT_TRANSLATE_NOOP("HelperMode", "Open an animation first.");
  inline constexpr const char* notOnScreen = QT_TRANSLATE_NOOP("HelperMode",
    "This part is not on screen now. View > Reset layout brings it back.");

  // ---- what the user does (the window sends these with HelperMode::notify) ----
  inline constexpr const char* picked = "picked";              // a joint was picked
  inline constexpr const char* turned = "turned";              // a joint was turned with a ring or a slider
  inline constexpr const char* keySet = "keySet";              // Set key saved a key
  inline constexpr const char* played = "played";              // the animation started playing
  inline constexpr const char* libraryShown = "libraryShown";  // the Library window opened
  inline constexpr const char* layoutReset = "layoutReset";    // Reset layout was used
  inline constexpr const char* scriptSide = "scriptSide";      // the Script side of the switch was chosen
  inline constexpr const char* animateSide = "animateSide";    // the Animate side was chosen

  struct Step
  {
    const char* id;
    const char* text;
    const char* target;   // the control the highlight stands around
    const char* event;    // what makes the step go on
  };

  struct Lesson
  {
    const char* id;
    const char* title;
    const char* summary;
    bool needsAnimation;
    const Step* steps;
    int stepCount;
    const char* doneText;
  };

  inline constexpr Step firstMoveSteps[] = {
    {"pick", QT_TRANSLATE_NOOP("HelperMode", "Click a joint on the figure to pick it."), "view", picked},
    {"turn", QT_TRANSLATE_NOOP("HelperMode",
       "Turn the picked joint: drag one of the coloured rings around it, or move a slider in Selected joint."),
     "selected", turned},
    {"setKey", QT_TRANSLATE_NOOP("HelperMode", "Press Set key. It saves the pose you see now as a key."), "setKey", keySet},
    {"play", QT_TRANSLATE_NOOP("HelperMode", "Press Start to watch your animation."), "start", played},
  };
  inline constexpr Step windowsSteps[] = {
    {"library", QT_TRANSLATE_NOOP("HelperMode", "Press Library at the top. The ready-made moves open in a window."),
     "library", libraryShown},
    {"reset", QT_TRANSLATE_NOOP("HelperMode",
       "Press Reset layout at the top. Every window goes back to its first place and the Library closes."),
     "resetLayout", layoutReset},
  };
  inline constexpr Step scriptSteps[] = {
    {"script", QT_TRANSLATE_NOOP("HelperMode", "Press Script at the top (F8). The Script Lab gets most of the window."),
     "scriptSwitch", scriptSide},
    {"animate", QT_TRANSLATE_NOOP("HelperMode", "Press Animate (F7). Your animation panels come back as you left them."),
     "animateSwitch", animateSide},
  };

  inline constexpr Lesson lessons[] = {
    {"firstMove", QT_TRANSLATE_NOOP("HelperMode", "Make your first move"),
     QT_TRANSLATE_NOOP("HelperMode", "Pick a joint, turn it, save a key and watch it."), true,
     firstMoveSteps, int(sizeof(firstMoveSteps) / sizeof(firstMoveSteps[0])),
     QT_TRANSLATE_NOOP("HelperMode", "Your first key is saved. Go to another frame and make more keys to build a move.")},
    {"windows", QT_TRANSLATE_NOOP("HelperMode", "Arrange the windows"),
     QT_TRANSLATE_NOOP("HelperMode", "Open the Library, then put every window back."), false,
     windowsSteps, int(sizeof(windowsSteps) / sizeof(windowsSteps[0])),
     QT_TRANSLATE_NOOP("HelperMode", "You can move and resize every window. Reset layout always puts them back.")},
    {"script", QT_TRANSLATE_NOOP("HelperMode", "Try the Script Lab"),
     QT_TRANSLATE_NOOP("HelperMode", "Switch to Script and back to Animate."), false,
     scriptSteps, int(sizeof(scriptSteps) / sizeof(scriptSteps[0])),
     QT_TRANSLATE_NOOP("HelperMode", "Switch any time. Each side keeps its own layout.")},
  };
  inline constexpr int lessonCount = int(sizeof(lessons) / sizeof(lessons[0]));

  inline QString tr(const char* source) { return QCoreApplication::translate("HelperMode", source); }
}

#endif
