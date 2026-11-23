/*
 * PinglyMotion - the four camera and key habits (docs/UX_DECISIONS.md,
 * section 8, and docs/mockup/index.html, SCHEMES)
 *
 * One table for the Camera and keys dialog (and the Setup dialog's "Mouse
 * and keys like" choice): the id UserSettings::cameraHabit() stores, the
 * name shown in the radio list, and the five gesture rows of the mockup's
 * clickable camera table. Since UX phase 4a the 3D view follows the chosen
 * habit (Camera::gestureFor(), src/camera.h); this table holds the texts the
 * dialogs show for it. tests/setupdialog checks every row against the mockup
 * exactly, tests/camerahabits checks that each row is what the camera does.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef CAMERAHABITSPEC_H
#define CAMERAHABITSPEC_H

#include <QCoreApplication>
#include <QString>

namespace CameraHabitSpec
{
  struct Row
  {
    const char* action;   // what the gesture does
    const char* gesture;  // the mouse or key gesture
  };

  struct Habit
  {
    const char* id;    // one of UserSettings::cameraHabits()
    const char* name;  // shown in the radio list
    Row rows[5];
  };

  inline constexpr Habit pinglymotion = {"pinglymotion", QT_TRANSLATE_NOOP("CameraHabitSpec", "PinglyMotion"), {
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Look around (orbit)"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Drag empty space")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Move the view (pan)"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Right-drag")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Zoom"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Mouse wheel")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Pick a joint"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Click it, or its row")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Focus on the picked joint"), QT_TRANSLATE_NOOP("CameraHabitSpec", "F, or double-click it")}}};

  inline constexpr Habit maya = {"maya", QT_TRANSLATE_NOOP("CameraHabitSpec", "Maya"), {
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Look around (orbit)"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Alt + left-drag")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Move the view (pan)"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Alt + middle-drag")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Zoom"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Alt + right-drag, or wheel")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Pick a joint"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Click")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Focus on the picked joint"), QT_TRANSLATE_NOOP("CameraHabitSpec", "F")}}};

  inline constexpr Habit blender = {"blender", QT_TRANSLATE_NOOP("CameraHabitSpec", "Blender"), {
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Look around (orbit)"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Middle-drag")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Move the view (pan)"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Shift + middle-drag")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Zoom"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Ctrl + middle-drag, or wheel")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Pick a joint"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Click")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Focus on the picked joint"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Numpad .")}}};

  inline constexpr Habit sl = {"sl", QT_TRANSLATE_NOOP("CameraHabitSpec", "Second Life"), {
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Look around (orbit)"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Alt + drag (sideways), Ctrl + Alt + drag")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Move the view (pan)"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Ctrl + Alt + Shift + drag")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Zoom"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Alt + drag up or down, or wheel")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Pick a joint"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Click")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Focus on the picked joint"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Alt + click")}}};

  // The keys that work the same in every habit (the mockup's KEYROWS_ALL, the rows that are
  // built: dragging a joint, the rings, the keyboard pull and the view tools come with UX phase 4b).
  // The Camera and keys dialog shows them under the habit's five rows.
  inline constexpr Row sharedRows[] = {
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Frame the picked joint, the whole figure, home view"), QT_TRANSLATE_NOOP("CameraHabitSpec", "F, Shift+F, 0")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Front, side, top view"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Numpad 1, 3, 7 (or 1, 3, 7 in the 3D view)")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Look around with keys"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Alt + arrow keys in the 3D view")},
    {QT_TRANSLATE_NOOP("CameraHabitSpec", "Zoom with keys"), QT_TRANSLATE_NOOP("CameraHabitSpec", "Page Up, Page Down")}};

  inline constexpr int sharedRowCount = int(sizeof(sharedRows) / sizeof(sharedRows[0]));

  // the status line after a habit is chosen (the mockup's setScheme() feedback)
  inline constexpr const char* chosen = QT_TRANSLATE_NOOP("CameraHabitSpec", "Camera and keys like %1.");

  // in the order of the mockup's radio list and UserSettings::cameraHabits()
  inline constexpr const Habit* all[] = {&pinglymotion, &maya, &blender, &sl};

  inline QString name(const Habit& habit) { return QCoreApplication::translate("CameraHabitSpec", habit.name); }
  inline QString action(const Row& row) { return QCoreApplication::translate("CameraHabitSpec", row.action); }
  inline QString gesture(const Row& row) { return QCoreApplication::translate("CameraHabitSpec", row.gesture); }
  inline QString chosenMessage(const Habit& habit) { return QCoreApplication::translate("CameraHabitSpec", chosen).arg(name(habit)); }

  // the habit of this id, or PinglyMotion's own habit (the default) for an unknown or empty id,
  // the same rule as UserSettings::cameraHabit()
  inline const Habit& find(const QString& id)
  {
    for (const Habit* habit : all)
      if (id == QLatin1String(habit->id)) return *habit;
    return pinglymotion;
  }
}

#endif
