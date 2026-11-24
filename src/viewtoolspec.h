/*
 * PinglyMotion - the texts of the view toolbar and the view-direction widget in the 3D view
 * (UX phase 4b, docs/UX_DECISIONS.md 6b, docs/mockup/index.html: the "vtools" toolbar, GIZ,
 * setTool(), the Show menu and the hint bar's ring line)
 *
 * One table for the toolbar (src/viewtoolbar.*), the widget (src/viewgizmo.*), the window and
 * tests/viewtools, which checks every text exactly against the mockup. Where the program says
 * something else than the mockup on purpose, the comment says why.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef VIEWTOOLSPEC_H
#define VIEWTOOLSPEC_H

#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include "camera.h"

namespace ViewToolSpec
{
  inline QString tr(const char* text) { return QCoreApplication::translate("ViewToolSpec", text); }

  // the toolbar's name for screen readers
  inline constexpr const char* toolbarName = QT_TRANSLATE_NOOP("ViewToolSpec", "View tools");

  // a button of the toolbar: its label, the first line of its tip, its key and the last line of its tip
  struct Button
  {
    const char* id;        // object name
    const char* label;     // on the button
    const char* name;      // first line of the tip
    const char* key;       // shown on the button and in the tip ("" for none)
    const char* tipline;   // last line of the tip ("" for none)
    const char* icon;      // Lucide name
  };

  // What the left mouse button does. Pick and pull: the mockup's text without its last sentence
  // ("Drag empty space to look around."), which is true only in the PinglyMotion habit; in the
  // others a plain drag on empty space does nothing (Camera::gestureFor()).
  inline constexpr Button pick = {"viewToolPick", QT_TRANSLATE_NOOP("ViewToolSpec", "Pick and pull"), QT_TRANSLATE_NOOP("ViewToolSpec", "Pick and pull"), "V",
    QT_TRANSLATE_NOOP("ViewToolSpec", "Click a joint to pick it, drag it to pull the limb."), "mouse-pointer-2"};
  inline constexpr Button orbit = {"viewToolOrbit", QT_TRANSLATE_NOOP("ViewToolSpec", "Look around"), QT_TRANSLATE_NOOP("ViewToolSpec", "Look around"), "O",
    QT_TRANSLATE_NOOP("ViewToolSpec", "Drag anywhere to turn the view around the figure."), "orbit"};
  inline constexpr Button pan = {"viewToolPan", QT_TRANSLATE_NOOP("ViewToolSpec", "Move the view"), QT_TRANSLATE_NOOP("ViewToolSpec", "Move the view"), "M",
    QT_TRANSLATE_NOOP("ViewToolSpec", "Drag to slide the view sideways or up and down."), "hand"};
  inline constexpr Button zoom = {"viewToolZoom", QT_TRANSLATE_NOOP("ViewToolSpec", "Zoom"), QT_TRANSLATE_NOOP("ViewToolSpec", "Zoom"), "Z",
    QT_TRANSLATE_NOOP("ViewToolSpec", "Drag up to come closer, down to go back. The mouse wheel zooms too."), "zoom-in"};
  // in the order of ViewTool
  inline constexpr const Button* tools[] = {&pick, &orbit, &pan, &zoom};
  inline const Button& tool(ViewTool t) { return *tools[int(t)]; }

  // the status line after a tool is chosen (the mockup's setTool() feedback)
  inline constexpr const char* toolChosen[] = {
    QT_TRANSLATE_NOOP("ViewToolSpec", "Pick and pull: click a joint to pick it, drag it to pull the limb."),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Look around: drag anywhere to turn the view."),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Move the view: drag to slide the view."),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Zoom: drag up to come closer, down to go back.")};
  inline QString toolChosenMessage(ViewTool t) { return tr(toolChosen[int(t)]); }

  // the hint while the mouse is over the 3D view with a tool other than Pick (the mockup's hint bar)
  inline constexpr const char* toolHover[] = {
    "",
    QT_TRANSLATE_NOOP("ViewToolSpec", "Look around: drag anywhere. V goes back to picking joints."),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Move the view: drag. V goes back to picking joints."),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Zoom: drag up or down. V goes back to picking joints.")};
  inline QString toolHoverHint(ViewTool t) { return t == ViewTool::Pick ? QString() : tr(toolHover[int(t)]); }

  // framing
  inline constexpr Button focus = {"viewFocus", QT_TRANSLATE_NOOP("ViewToolSpec", "Picked joint"), QT_TRANSLATE_NOOP("ViewToolSpec", "Focus on the picked joint"), "F", "", "crosshair"};
  inline constexpr Button wholeFigure = {"viewWholeFigure", QT_TRANSLATE_NOOP("ViewToolSpec", "Whole figure"), QT_TRANSLATE_NOOP("ViewToolSpec", "Frame the whole figure"), "Shift+F", "", "maximize"};
  inline constexpr Button home = {"viewHome", QT_TRANSLATE_NOOP("ViewToolSpec", "Home view"), QT_TRANSLATE_NOOP("ViewToolSpec", "Home view: front, normal size"), "0", "", "house"};
  inline constexpr const char* homeDone = QT_TRANSLATE_NOOP("ViewToolSpec", "Home view: front, normal size.");
  inline constexpr const char* wholeFigureDone = QT_TRANSLATE_NOOP("ViewToolSpec", "The whole figure fits the view.");
  inline constexpr const char* focusDone = QT_TRANSLATE_NOOP("ViewToolSpec", "Camera turns around %1 now.");
  inline constexpr const char* focusNoJoint = QT_TRANSLATE_NOOP("ViewToolSpec", "Focus: Pick a joint first.");

  // the two menus
  inline constexpr Button lookFrom = {"viewLookFrom", QT_TRANSLATE_NOOP("ViewToolSpec", "Look from"), QT_TRANSLATE_NOOP("ViewToolSpec", "Look from: front, side, top or back"), "Numpad 1, 3, 7", "", "axis-3d"};
  inline constexpr Button show = {"viewShow", QT_TRANSLATE_NOOP("ViewToolSpec", "Show"), QT_TRANSLATE_NOOP("ViewToolSpec", "Show or hide parts of the 3D view"), "", "", "eye"};

  // Look from: the mockup's four items and their keys (the 3D view's own keys, Camera::viewForKey())
  struct Look
  {
    const char* text;
    const char* key;
    float height;
    float angle;
  };
  inline constexpr Look looks[] = {
    {QT_TRANSLATE_NOOP("ViewToolSpec", "Front"), "1", 0, 0},
    {QT_TRANSLATE_NOOP("ViewToolSpec", "Side"), "3", 0, 90},
    {QT_TRANSLATE_NOOP("ViewToolSpec", "Top"), "7", 90, 0},
    {QT_TRANSLATE_NOOP("ViewToolSpec", "Back"), "Ctrl+1", 0, 180}};
  inline constexpr int lookCount = int(sizeof(looks) / sizeof(looks[0]));

  // Show
  enum class Part
  {
    Floor,      // "show_floor"
    Rings,      // "show_rings"
    JointDots,  // the same setting as Options > Show skeleton
    ToolNames   // "view_tool_names"
  };
  inline constexpr const char* partText[] = {
    QT_TRANSLATE_NOOP("ViewToolSpec", "Floor grid"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Rotation rings"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Joint dots"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Tool names in this toolbar")};
  inline constexpr Part parts[] = {Part::Floor, Part::Rings, Part::JointDots, Part::ToolNames};
  inline QString partName(Part part) { return tr(partText[int(part)]); }
  // the status line after a Show item is ticked or unticked: "Showing: floor grid." (the mockup)
  inline constexpr const char* showing = QT_TRANSLATE_NOOP("ViewToolSpec", "Showing: %1.");
  inline constexpr const char* hiding = QT_TRANSLATE_NOOP("ViewToolSpec", "Hiding: %1.");
  inline QString shownMessage(Part part, bool on) { return tr(on ? showing : hiding).arg(partName(part).toLower()); }

  // the view-direction widget
  inline constexpr const char* widgetName = QT_TRANSLATE_NOOP("ViewToolSpec", "View direction: click an axis end to look from there");
  inline constexpr const char* widgetDrag = QT_TRANSLATE_NOOP("ViewToolSpec", "Drag to look around");

  // its six ends, in the order the arrow keys go through them (the mockup's GIZ)
  enum class End
  {
    PlusX,
    MinusX,
    PlusY,
    MinusY,
    PlusZ,
    MinusZ
  };
  struct EndSpec
  {
    End end;
    const char* letter;  // on the end: the letter on the + ends, a minus and the letter on the - ends
    const char* name;    // its tip and its name for screen readers
    float height;        // the view it gives, as Camera::lookFrom() takes it
    float angle;
  };
  // "−" is the minus sign the mockup writes (U+2212)
  inline constexpr EndSpec ends[] = {
    {End::PlusX, "X", QT_TRANSLATE_NOOP("ViewToolSpec", "Look from the left side (+X)"), 0, 270},
    {End::MinusX, "−X", QT_TRANSLATE_NOOP("ViewToolSpec", "Look from the right side (−X)"), 0, 90},
    {End::PlusY, "Y", QT_TRANSLATE_NOOP("ViewToolSpec", "Look from the top (+Y)"), 90, 0},
    {End::MinusY, "−Y", QT_TRANSLATE_NOOP("ViewToolSpec", "Look from below (−Y)"), -90, 0},
    {End::PlusZ, "Z", QT_TRANSLATE_NOOP("ViewToolSpec", "Look from the front (+Z)"), 0, 0},
    {End::MinusZ, "−Z", QT_TRANSLATE_NOOP("ViewToolSpec", "Look from the back (−Z)"), 0, 180}};
  inline constexpr int endCount = 6;
  inline const EndSpec& endSpec(End end) { return ends[int(end)]; }
  // the status line after an end is chosen: "Looking from: Left side." (the mockup)
  inline constexpr const char* lookingFrom = QT_TRANSLATE_NOOP("ViewToolSpec", "Looking from: %1.");

  // the view's name under the widget, in the order of ViewName
  inline constexpr const char* viewNames[] = {
    QT_TRANSLATE_NOOP("ViewToolSpec", "Front"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Back"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Left side"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Right side"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Top"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Below"),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Free")};
  inline QString viewName(ViewName name) { return tr(viewNames[int(name)]); }
  inline QString lookingFromMessage(ViewName name) { return tr(lookingFrom).arg(viewName(name)); }

  // the hint while the mouse is over a rotation ring (the mockup's hint bar line, with the
  // Selected joint panel's axis names)
  inline constexpr const char* ringHover[] = {
    QT_TRANSLATE_NOOP("ViewToolSpec", "Drag the red X ring to turn the joint: tilt forward or back."),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Drag the green Y ring to turn the joint: turn left or right."),
    QT_TRANSLATE_NOOP("ViewToolSpec", "Drag the blue Z ring to turn the joint: lean to the side.")};
  inline QString ringHoverHint(int axis) { return axis >= 0 && axis <= 2 ? tr(ringHover[axis]) : QString(); }

  // the hint while the mouse is over a joint with Pick and pull (the mockup's HINT-02)
  inline constexpr const char* jointHover = QT_TRANSLATE_NOOP("ViewToolSpec", "Click to pick this joint. Press and drag to pull the limb.");

  // Dragging a joint (UX phase 4b part 2, docs/UX_DECISIONS.md 6a; the mockup's showDragTip(),
  // finishJointDrag(), cancelJointDrag() and startKeyGrab()). The label next to the pointer:
  inline constexpr const char* pulling = QT_TRANSLATE_NOOP("ViewToolSpec", "Pulling %1");
  inline constexpr const char* turningOnly = QT_TRANSLATE_NOOP("ViewToolSpec", "Turning only %1");
  inline constexpr const char* turningHeld = QT_TRANSLATE_NOOP("ViewToolSpec", "Turning only %1 (it holds the body)");
  inline constexpr const char* pullHowMouse = QT_TRANSLATE_NOOP("ViewToolSpec", "Release: set key · Esc: cancel");
  inline constexpr const char* pullHowKeys = QT_TRANSLATE_NOOP("ViewToolSpec", "Arrow keys move it · Enter: set key · Esc: cancel");
  inline constexpr const char* pullShift = QT_TRANSLATE_NOOP("ViewToolSpec", "Shift when you press: turn only this joint");
  // the status line when it ends, with the mockup's "Ctrl+Z undoes it." (Undo works since job B1:
  // the whole pull is one step)
  inline constexpr const char* pulled = QT_TRANSLATE_NOOP("ViewToolSpec", "Pulled %1. Key set at frame %2 for %3 and %1. Ctrl+Z undoes it.");
  inline constexpr const char* turned = QT_TRANSLATE_NOOP("ViewToolSpec", "Turned %1. Key set at frame %2 for %1. Ctrl+Z undoes it.");
  inline constexpr const char* pullCancelled = QT_TRANSLATE_NOOP("ViewToolSpec", "Pull cancelled. Nothing changed.");
  // why a pull cannot start or go on
  inline constexpr const char* pullBlocked = QT_TRANSLATE_NOOP("ViewToolSpec", "Pull a joint: %1");
  inline constexpr const char* keysBlocked = QT_TRANSLATE_NOOP("ViewToolSpec", "Pull with keys: %1");
  inline constexpr const char* keysNoJoint = QT_TRANSLATE_NOOP("ViewToolSpec", "Pull with keys: Pick a joint first. Click it on the figure or in the Joints list.");
  inline constexpr const char* finishFirst = QT_TRANSLATE_NOOP("ViewToolSpec", "Finish or cancel the pull first: Esc cancels.");
  // how far one arrow key moves the pulled joint on the screen (pixels; Shift: small steps), as the mockup
  inline constexpr int keyStep = 12;
  inline constexpr int keyStepSmall = 3;

  // the label's lines: what happens, how to end it, and (a mouse pull of a limb) the Shift choice
  inline QStringList pullLines(const QString& joint, bool turnOnly, bool held, bool viaKeys)
  {
    QStringList lines;
    lines << tr(turnOnly ? (held ? turningHeld : turningOnly) : pulling).arg(joint);
    lines << tr(viaKeys ? pullHowKeys : pullHowMouse);
    if (!viaKeys && !turnOnly) lines << tr(pullShift);
    return lines;
  }
  // "Pulled Right Hand. Key set at frame 5 for Right Forearm, Right Upper Arm and Right Hand.";
  // driven: the joints that bent, nearest first; frame: as the frame counter shows it (from 1)
  inline QString pullDoneMessage(const QString& joint, const QStringList& driven, int frame, bool turnOnly)
  {
    if (turnOnly || driven.isEmpty()) return tr(turned).arg(joint).arg(frame);
    return tr(pulled).arg(joint).arg(frame).arg(driven.join(QStringLiteral(", ")));
  }

  // the tip of a button: name, then the key, then the tip line, one per line
  inline QString tip(const Button& button)
  {
    QString text = tr(button.name);
    if (button.key[0]) text += QStringLiteral(" (%1)").arg(QLatin1String(button.key));
    if (button.tipline[0]) text += QLatin1Char('\n') + tr(button.tipline);
    return text;
  }
}

#endif
