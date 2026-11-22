/*
 * PinglyMotion - the Setup dialog (docs/UX_DECISIONS.md, section 6c;
 * docs/mockup/index.html, #dlgSetup), UX phase 1d
 *
 * Beginner or experienced, mouse and keys habit, theme, text size and hints
 * in one dialog, reachable from the gear button in the top bar, View > Setup
 * and Help > Setup, so the welcome choices can be found and changed at any
 * time. Every choice applies at once and is remembered (src/usersettings.*);
 * nothing here moves the panels. The dialog only shows and changes the
 * settings; PinglyMotionWindow does the actual work (theme, text size,
 * hints switch, status messages) through the same code path as its own
 * menu actions, so the two stay in sync.
 *
 * "Show all mouse and keys" opens the Camera and keys dialog
 * (src/camerakeysdialog.*); Help > Keys and mouse opens the general keys and
 * mouse page (src/keysmousedialog.*) on its own.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef SETUPDIALOG_H
#define SETUPDIALOG_H

#include <QCoreApplication>
#include <QDialog>

class QRadioButton;
class TipsSwitch;

namespace SetupText
{
  inline constexpr const char* title = QT_TRANSLATE_NOOP("SetupDialog", "Setup");
  inline constexpr const char* intro = QT_TRANSLATE_NOOP("SetupDialog",
    "Change these any time. PinglyMotion remembers them. Nothing here moves the panels.");
  inline constexpr const char* experienceLabel = QT_TRANSLATE_NOOP("SetupDialog", "Have you made animations before?");
  inline constexpr const char* beginnerChoice = QT_TRANSLATE_NOOP("SetupDialog", "New to animation (Beginner mode)");
  inline constexpr const char* expertChoice = QT_TRANSLATE_NOOP("SetupDialog", "Experienced (Expert mode)");
  inline constexpr const char* experienceCaption = QT_TRANSLATE_NOOP("SetupDialog",
    "Expert mode shows more controls (curves, Paste mirrored). Beginner mode keeps the screen calm.");
  inline constexpr const char* cameraLabel = QT_TRANSLATE_NOOP("SetupDialog", "Mouse and keys like");
  inline constexpr const char* showAllKeys = QT_TRANSLATE_NOOP("SetupDialog", "Show all mouse and keys");
  inline constexpr const char* themeLabel = QT_TRANSLATE_NOOP("SetupDialog", "Theme");
  inline constexpr const char* darkChoice = QT_TRANSLATE_NOOP("SetupDialog", "Dark");
  inline constexpr const char* lightChoice = QT_TRANSLATE_NOOP("SetupDialog", "Light");
  inline constexpr const char* textSizeLabel = QT_TRANSLATE_NOOP("SetupDialog", "Text size");
  inline constexpr const char* guidanceLabel = QT_TRANSLATE_NOOP("SetupDialog", "Guidance");
  inline constexpr const char* doneButton = QT_TRANSLATE_NOOP("SetupDialog", "Done");

  // the five text size choices (docs/UX_DECISIONS.md, section 8: 100 to 200, steps of 25)
  inline constexpr int sizes[] = {100, 125, 150, 175, 200};
}

class SetupDialog : public QDialog
{
  Q_OBJECT

  public:
    explicit SetupDialog(QWidget* parent = nullptr);

    // read back for tests: the controls show what UserSettings held when the dialog opened
    bool expertChecked() const;
    QString cameraHabitChecked() const;
    bool darkChecked() const;
    int textSizeChecked() const;
    bool hintsChecked() const;

  signals:
    // the user changed a choice; PinglyMotionWindow stores it and applies it (same path as its
    // own menu actions, so the window and the dialog never disagree)
    void expertModeRequested(bool expert);
    void cameraHabitRequested(const QString& habit);
    void darkThemeRequested(bool dark);
    void textSizeRequested(int percent);
    void hintsRequested(bool on);
    void showCameraKeysRequested();

  private:
    QRadioButton* m_beginnerRadio;
    QRadioButton* m_expertRadio;
    QList<QRadioButton*> m_cameraRadios;
    QRadioButton* m_darkRadio;
    QRadioButton* m_lightRadio;
    QList<QRadioButton*> m_textSizeRadios;
    TipsSwitch* m_hintsSwitch;
};

#endif
