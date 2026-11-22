/*
 * PinglyMotion - the four camera and key habits (docs/UX_DECISIONS.md,
 * section 8, and docs/mockup/index.html, SCHEMES)
 *
 * One table for the Camera and keys dialog (and the Setup dialog's "Mouse
 * and keys like" choice): the id UserSettings::cameraHabit() stores, the
 * name shown in the radio list, and the five gesture rows of the mockup's
 * clickable camera table. The camera itself does not read the habit yet
 * (docs/UX_DECISIONS.md, section 8; the mouse always uses the PinglyMotion
 * gestures until a later UX phase wires the others up), so this table only
 * feeds the two dialogs. tests/setupdialog checks every row against the
 * mockup exactly.
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

  // in the order of the mockup's radio list and UserSettings::cameraHabits()
  inline constexpr const Habit* all[] = {&pinglymotion, &maya, &blender, &sl};

  inline QString name(const Habit& habit) { return QCoreApplication::translate("CameraHabitSpec", habit.name); }
  inline QString action(const Row& row) { return QCoreApplication::translate("CameraHabitSpec", row.action); }
  inline QString gesture(const Row& row) { return QCoreApplication::translate("CameraHabitSpec", row.gesture); }

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
