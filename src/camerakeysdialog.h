/*
 * PinglyMotion - the Camera and keys dialog (docs/UX_DECISIONS.md, section 6c;
 * docs/mockup/index.html, #dlgCamera), UX phase 1d
 *
 * Pick the mouse and key habit you know (PinglyMotion, Maya, Blender or
 * Second Life, src/camerahabitspec.h); PinglyMotion remembers the choice
 * (UserSettings::cameraHabit()) and shows its five gestures in a table,
 * then the keys that work the same in every habit (CameraHabitSpec::sharedRows).
 * Since UX phase 4a the 3D view follows the choice at once.
 * Reachable from Help > Camera and keys and from the Setup dialog's
 * "Show all mouse and keys" button.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef CAMERAKEYSDIALOG_H
#define CAMERAKEYSDIALOG_H

#include <QCoreApplication>
#include <QDialog>

class QRadioButton;
class QTableWidget;

namespace CameraKeysText
{
  inline constexpr const char* title = QT_TRANSLATE_NOOP("CameraKeysDialog", "Camera and keys");
  inline constexpr const char* intro = QT_TRANSLATE_NOOP("CameraKeysDialog",
    "Pick the habits you know. PinglyMotion remembers your choice.");
  inline constexpr const char* doneButton = QT_TRANSLATE_NOOP("CameraKeysDialog", "Done");
  inline constexpr const char* actionHeader = QT_TRANSLATE_NOOP("CameraKeysDialog", "What it does");
  inline constexpr const char* gestureHeader = QT_TRANSLATE_NOOP("CameraKeysDialog", "Mouse or key");
}

class CameraKeysDialog : public QDialog
{
  Q_OBJECT

  public:
    explicit CameraKeysDialog(QWidget* parent = nullptr);

    QString habitChecked() const;
    // the gesture table's current rows (action, gesture), for the checked habit
    QTableWidget* table() const { return m_table; }

  signals:
    void cameraHabitRequested(const QString& habit);

  private:
    void fillTable(const QString& habitId);

    QList<QRadioButton*> m_radios;
    QTableWidget* m_table;
};

#endif
