/*
 * PinglyMotion - the empty state, UX phase 1b
 *
 * What the 3D view shows while no animation is open (design charter P4 and
 * P6; docs/mockup/index.html, "No animation open"): a title, one line, and
 * the ways to start. The start choices are Standing, Sitting and Dance
 * example as on the mockup's welcome card; each opens a clip of the
 * reference library (library/index.json) by the same route as the control
 * command load_reference, so the file is checked against the index first.
 * A plain "New animation" button stays for a blank start. The buttons are tool
 * buttons on purpose: the transport look paints every push button of the program
 * (src/transportlook.cpp), and these draw their own look from the theme tokens.
 *
 * One table (EmptySpec) for the widget and for tests/emptystate, which
 * checks every text and every clip id exactly.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef EMPTYSTATE_H
#define EMPTYSTATE_H

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QWidget>

class QLabel;
class QToolButton;

namespace EmptySpec
{
  struct Choice
  {
    const char* id;       // object name of the button
    const char* text;     // the choice
    const char* small;    // one line under it
    const char* clip;     // id in library/index.json
    bool main;            // the one main (accent) button
  };

  inline constexpr const char* heading = QT_TRANSLATE_NOOP("EmptyState", "No animation open");
  inline constexpr const char* line = QT_TRANSLATE_NOOP("EmptyState", "Pick a pose to start. You can change it later.");
  inline constexpr const char* newText = QT_TRANSLATE_NOOP("EmptyState", "New animation");

  inline constexpr Choice standing = {"startStandingButton", QT_TRANSLATE_NOOP("EmptyState", "Standing"),
    QT_TRANSLATE_NOOP("EmptyState", "A calm standing pose. A good start for most moves."), "m2m-idle-a", false};
  inline constexpr Choice sitting = {"startSittingButton", QT_TRANSLATE_NOOP("EmptyState", "Sitting"),
    QT_TRANSLATE_NOOP("EmptyState", "A calm seated pose. Good for a quiet scene."), "m2m-sitting-idle", false};
  inline constexpr Choice dance = {"startDanceButton", QT_TRANSLATE_NOOP("EmptyState", "Dance example"),
    QT_TRANSLATE_NOOP("EmptyState", "A short, simple dance. Change it to learn."), "m2m-dance-simple", true};

  inline constexpr const Choice* choices[] = {&standing, &sitting, &dance};
}

class EmptyState : public QWidget
{
  Q_OBJECT

  public:
    explicit EmptyState(QWidget* parent = nullptr);

    QLabel* headingLabel() const { return m_heading; }
    QLabel* lineLabel() const { return m_line; }
    // the three start choices, in the order of EmptySpec::choices
    QList<QToolButton*> choiceButtons() const { return m_choices; }
    QToolButton* newButton() const { return m_new; }

    // colours and icon again, after the theme changed
    void applyLook();

  signals:
    // a start choice was clicked: the library id of its clip
    void startChosen(const QString& clipId);
    void newChosen();

  protected:
    // covers the whole parent, whatever its size
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    QLabel* m_icon;
    QLabel* m_heading;
    QLabel* m_line;
    QList<QToolButton*> m_choices;
    QToolButton* m_new;
};

#endif
