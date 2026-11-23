/*
 * PinglyMotion - the Keys and mouse dialog (docs/UX_DECISIONS.md, section 6c;
 * docs/mockup/index.html, #dlgKeys), UX phase 1d
 *
 * A page of the mouse and keyboard actions that work in the program today,
 * reachable from Help > Keys and mouse; the mouse rows follow the chosen camera
 * habit (UX phase 4a). Difference from the mockup,
 * on purpose: the mockup's table (KEYROWS_ALL) also lists dragging a joint,
 * the rotation rings and the keyboard pull, which wait for the project
 * owner's yes before they are built (docs/UX_DECISIONS.md, 6a and 6b); this
 * page shows only what PinglyMotion can really do now, so it never claims
 * what is not true (docs/TEACHING_TEXTS.md, voice guide, rule 6). The rows
 * come from src/keysmousedialog.cpp's own table (KeysMouseSpec), which
 * tests/setupdialog checks against the real menu keys in src/menuspec.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef KEYSMOUSEDIALOG_H
#define KEYSMOUSEDIALOG_H

#include <QCoreApplication>
#include <QDialog>
#include <QList>
#include <QPair>

#include "camerahabitspec.h"

class QTableWidget;

namespace KeysMouseText
{
  inline constexpr const char* title = QT_TRANSLATE_NOOP("KeysMouseDialog", "Keys and mouse");
  inline constexpr const char* closeButton = QT_TRANSLATE_NOOP("KeysMouseDialog", "Close");
  inline constexpr const char* actionHeader = QT_TRANSLATE_NOOP("KeysMouseDialog", "What it does");
  inline constexpr const char* gestureHeader = QT_TRANSLATE_NOOP("KeysMouseDialog", "Mouse or key");
}

namespace KeysMouseSpec
{
  struct Row
  {
    const char* action;
    const char* gesture;
  };

  // The keyboard shortcuts that are built, in the order of the mockup where it still applies.
  // The dialog puts the chosen camera habit's mouse rows (src/camerahabitspec.h: look around,
  // move the view, zoom, focus) right after the first row (tableRows() below), since the
  // camera follows the habit (UX phase 4a).
  inline constexpr Row rows[] = {
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Pick a joint"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Click it, or its row in the Joints panel")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Frame the whole figure"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Shift+F")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Look around with keys"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Alt + arrow keys in the 3D view")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Zoom with keys"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Page Up, Page Down")},
    // UX phase 4b: dragging a joint (part 2), the rings around the picked joint and the view toolbar's tools (part 1)
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Pull a limb"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Press a joint and drag. Let go to set a key. Esc cancels.")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Turn only that joint"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Hold Shift when you press the joint (or choose Turn only this joint)")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Turn on one axis"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Drag the red X, green Y or blue Z ring around the picked joint")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Pull with the keyboard"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Enter on the picked joint, arrow keys, Enter to set a key, Esc cancels")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "View tools: pick, look around, move the view, zoom"), QT_TRANSLATE_NOOP("KeysMouseDialog", "V, O, M, Z")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Front view"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Numpad 1")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Side view"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Numpad 3")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Top view"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Numpad 7")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Home view"), QT_TRANSLATE_NOOP("KeysMouseDialog", "0")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Step one frame back or forward"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Left, Right arrow keys")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Set key"), QT_TRANSLATE_NOOP("KeysMouseDialog", "K")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Switch theme"), QT_TRANSLATE_NOOP("KeysMouseDialog", "T")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Expert mode"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Shift+E")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Show hints"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Shift+H")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Text size"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Ctrl+= bigger, Ctrl+- smaller, Ctrl+0 back to 100%")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Hide panels"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Tab")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Script Lab"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Ctrl+Shift+L")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Library"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Ctrl+Shift+B")},
  };

  inline QString action(const Row& row) { return QCoreApplication::translate("KeysMouseDialog", row.action); }
  inline QString gesture(const Row& row) { return QCoreApplication::translate("KeysMouseDialog", row.gesture); }

  // the whole table for this camera habit: "Pick a joint", the habit's look around, move the
  // view, zoom and focus rows, then the other rows above
  inline QList<QPair<QString, QString>> tableRows(const CameraHabitSpec::Habit& habit)
  {
    QList<QPair<QString, QString>> table;
    table.append({action(rows[0]), gesture(rows[0])});
    for (int i : {0, 1, 2, 4})
      table.append({CameraHabitSpec::action(habit.rows[i]), CameraHabitSpec::gesture(habit.rows[i])});
    for (size_t i = 1; i < sizeof(rows) / sizeof(rows[0]); ++i)
      table.append({action(rows[i]), gesture(rows[i])});
    return table;
  }
}

class KeysMouseDialog : public QDialog
{
  Q_OBJECT

  public:
    // the mouse rows of the habit UserSettings::cameraHabit() names
    explicit KeysMouseDialog(QWidget* parent = nullptr);

    QTableWidget* table() const { return m_table; }

  private:
    QTableWidget* m_table;
};

#endif
