/*
 * PinglyMotion - the welcome card (UX phase 6a, docs/UX_DECISIONS.md, 6k;
 * docs/mockup/index.html, #dlgWelcome)
 *
 * Shown on the first start only ("New to animation, or experienced?"),
 * and again from Help > Show the welcome card again or the Setup dialog.
 * It asks for Beginner or Expert, and for the camera habit when the user
 * is experienced, then offers the three start choices of the empty state
 * (same clips) and Skip. Closing it any other way (Esc, the window's close
 * button) counts as Skip. The dialog only collects the answer; apply()
 * remembers it (UserSettings) and the window opens the clip.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef WELCOMEDIALOG_H
#define WELCOMEDIALOG_H

#include <QDialog>
#include <QList>
#include <QString>

class QLabel;
class QRadioButton;
class QToolButton;
class QWidget;

class WelcomeDialog : public QDialog
{
  Q_OBJECT

  public:
    struct Answer
    {
      bool skipped = true;        // Skip, Esc or the close button
      QString clipId;             // library id of the chosen start clip; empty when skipped
      bool expert = false;        // "Experienced"
      QString cameraHabit;        // chosen camera habit; empty unless experienced
    };

    explicit WelcomeDialog(QWidget* parent = nullptr);

    // the card shows by itself only until it was answered once
    static bool due();
    // remember the answer: Beginner or Expert, the camera habit (only when experienced),
    // and that the card was answered (so it never shows by itself again)
    static void apply(const Answer& answer);

    Answer answer() const { return m_answer; }

    // the controls, for tests
    QList<QToolButton*> startButtons() const { return m_starts; }
    QToolButton* skipButton() const { return m_skip; }
    QRadioButton* newRadio() const { return m_new; }
    QRadioButton* proRadio() const { return m_pro; }
    QList<QRadioButton*> cameraRadios() const { return m_cameras; }
    QWidget* cameraRow() const { return m_cameraRow; }
    QLabel* introLabel() const { return m_intro; }

  protected:
    void reject() override;

  private:
    void collect(const QString& clipId);
    void finishWith(const QString& clipId);

    Answer m_answer;
    QLabel* m_intro;
    QRadioButton* m_new;
    QRadioButton* m_pro;
    QWidget* m_cameraRow;
    QList<QRadioButton*> m_cameras;
    QList<QToolButton*> m_starts;
    QToolButton* m_skip;
};

#endif
