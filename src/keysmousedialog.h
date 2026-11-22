/*
 * PinglyMotion - the Keys and mouse dialog (docs/UX_DECISIONS.md, section 6c;
 * docs/mockup/index.html, #dlgKeys), UX phase 1d
 *
 * A static page of the mouse and keyboard actions that work in the program
 * today, reachable from Help > Keys and mouse. Difference from the mockup,
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

  // mouse gestures (the PinglyMotion habit, src/camerahabitspec.h; the camera does not read a
  // different habit yet) and the keyboard shortcuts that are built, in the order of the mockup
  // where it still applies
  inline constexpr Row rows[] = {
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Pick a joint"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Click it, or its row in the Joints panel")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Look around (orbit)"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Drag empty space")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Move the view (pan)"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Right-drag")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Zoom"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Mouse wheel")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Front view"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Numpad 1")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Side view"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Numpad 3")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Top view"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Numpad 7")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Home view"), QT_TRANSLATE_NOOP("KeysMouseDialog", "0")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Step one frame back or forward"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Left, Right arrow keys")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Set key"), QT_TRANSLATE_NOOP("KeysMouseDialog", "K")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Switch theme"), QT_TRANSLATE_NOOP("KeysMouseDialog", "T")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Expert mode"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Shift+E")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Show hints"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Shift+H")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Text size"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Ctrl+=")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Hide panels"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Tab")},
    {QT_TRANSLATE_NOOP("KeysMouseDialog", "Script Lab"), QT_TRANSLATE_NOOP("KeysMouseDialog", "Ctrl+Shift+L")},
  };

  inline QString action(const Row& row) { return QCoreApplication::translate("KeysMouseDialog", row.action); }
  inline QString gesture(const Row& row) { return QCoreApplication::translate("KeysMouseDialog", row.gesture); }
}

class KeysMouseDialog : public QDialog
{
  Q_OBJECT

  public:
    explicit KeysMouseDialog(QWidget* parent = nullptr);

    QTableWidget* table() const { return m_table; }

  private:
    QTableWidget* m_table;
};

#endif
