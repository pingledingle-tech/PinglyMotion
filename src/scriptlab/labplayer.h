/*
 * PinglyMotion Script Lab - the player (job B3, plan "Script Lab: see and understand")
 *
 * The player shows the replay of the recipe (job A3, labrecipe.h) as a film. The replay keeps one
 * moment right after the start and right after every step (LabRecipe::Frame); the player steps
 * between them with the animation timeline's own play buttons (TransportBar, playback.h): Start
 * plays one moment per second, Stop stops, Replay from start goes to the start and plays, Step back
 * and Step forward move one moment. A list of the moments jumps to any of them, and "Go to first
 * difference" jumps to the step where this replay first differed from the one before.
 *
 * At the chosen moment the player shows what was true then: who is locked (red, "Locked") or free
 * again (green, "Free"), which actions are blocked or allowed again, the colours, the open menu, what
 * each part said and a step that could not be done; rows that changed at this step are bold. Expert
 * mode adds the event log up to the moment. While the user looks at a moment ("showing"), the props in
 * the 3D view show the colours and places of that moment (folderAt); "Show now" goes back to now.
 *
 * The moments are taken once, while the replay runs (docs/PLAYER_CHECK.md, recommendation (b)):
 * nothing is run again when the user steps.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABPLAYER_H
#define LABPLAYER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class TransportBar;

// what the player needs, without a window (tests/tst_labplayer.cpp)
namespace LabPlayerModel
{
  struct Fact
  {
    QString key;     // "lock:Cuffs", "blocked:sendim", "colour:Cuff", "menu", "said:Root", "step:2", ...
    QString text;    // "Cuffs is locked."
    bool operator==(const Fact& other) const { return key == other.key && text == other.text; }
  };

  struct Moment
  {
    int step = 0;        // 0: the start; n: right after step n
    double t = 0.0;      // simulated seconds
    int logEnd = 0;      // the replay's log entries [0, logEnd) happened by now
    bool done = true;    // the step was done
    QString text;        // "Start." or the step in plain words ("Click Root.")
    QVector<Fact> facts;
  };

  // the moments of a replay (LabRecipe::toJson, LabWatcher::lastReplay); empty when there are none
  QVector<Moment> moments(const QJsonObject& replay);
  // the moment the replay first differed from the one before; -1 when none
  int firstDifference(const QJsonObject& replay);

  // The folder run (LabWatcher::folderResult) as it was at a moment of the replay: its linkset, and as
  // its log the replay's log up to logEnd. The props read their colours (LabTint::fromFolder) and places
  // (LabPrimProps::movesFromFolder) from it. An empty object when the folder did not run.
  QJsonObject folderAt(const QJsonObject& folder, const QJsonArray& log, int logEnd);

  // the line in the list of moments: "Start" or "Step 2: Press "Lock" in the menu." (plus " Not done.")
  QString momentTitle(const Moment& moment);
  // the line over the list: "Step 2 of 3, at 12 s: Press "Lock" in the menu." or "Start, at 10 s."
  QString positionLine(const QVector<Moment>& moments, int index);

  enum class Look
  {
    Locked,     // red, "Locked: Cuffs"
    Free,       // green, "Free: Cuffs" (locked at another moment of the replay, not now)
    Blocked,    // red, "Blocked: Send instant messages"
    Allowed,    // green, "Allowed: Send instant messages" (blocked at another moment, not now)
    Problem,    // a step that could not be done, the scripts stopped
    Plain       // colours, the menu, what was said
  };

  struct Row
  {
    QString text;
    Look look = Look::Plain;
    bool changed = false;    // not there, or different, at the moment before
    bool operator==(const Row& other) const { return text == other.text && look == other.look && changed == other.changed; }
  };

  // What is true at moment index, in this order: locks and free parts, blocked and allowed actions,
  // problems, then the rest, each in the order of the facts. A part or action that is locked or
  // blocked at any moment of the replay and not now is shown as free or allowed. One plain row when
  // there is nothing to show.
  QVector<Row> rows(const QVector<Moment>& moments, int index);

  // the replay's log up to the moment, one line each (ScriptLabPanel::logLine), a step as
  // "--- Step 2: menu Lock ---"
  QStringList logLines(const QJsonArray& log, int logEnd);
}

class LabPlayer : public QWidget
{
  Q_OBJECT

  public:
    // one moment per second while playing
    static constexpr int PLAY_INTERVAL_MS = 1000;

    explicit LabPlayer(QWidget* parent = nullptr);

    // A new replay (LabWatcher::lastReplay): the last moment is chosen, the 3D view shows now and
    // playing stops. The same replay again changes nothing.
    void setReplay(const QJsonObject& replay);
    // Expert mode adds the event log up to the moment (charter P5)
    void setExpertMode(bool expert);

    int momentCount() const { return m_moments.size(); }
    int current() const { return m_current; }
    // the user looks at a moment: the props in the 3D view show it (until Show now or a new replay)
    bool showing() const { return m_showing; }
    bool playing() const { return m_timer.isActive(); }
    const QVector<LabPlayerModel::Moment>& moments() const { return m_moments; }

    // jumps to a moment (kept between the first and the last) and shows it in the 3D view
    void showMoment(int index);
    // one moment back (-1) or forward (+1), as Step back and Step forward
    void stepBy(int delta);
    // as Start: plays from this moment (from the start when on the last one), one moment a second
    void play();
    // as Stop
    void stop();
    // as Replay from start
    void replayFromStart();
    // as Go to first difference; false when this replay did not differ from the one before
    bool goToFirstDifference();
    // as Show now: the 3D view shows now again; the chosen moment stays chosen in the list
    void showNow();

    // the folder run as at the chosen moment (LabPlayerModel::folderAt), for the props
    QJsonObject folderAt(const QJsonObject& folder) const;

    TransportBar* transportBar() const { return m_bar; }
    QLabel* positionLabel() const { return m_position; }
    QListWidget* momentList() const { return m_list; }
    QListWidget* stateList() const { return m_state; }
    QPushButton* firstDifferenceButton() const { return m_firstDifference; }
    QPushButton* nowButton() const { return m_now; }
    QLabel* viewLabel() const { return m_view; }
    QLabel* logTitle() const { return m_logTitle; }
    QPlainTextEdit* logView() const { return m_log; }

  signals:
    // the chosen moment or showing() changed
    void momentChanged();

  protected:
    void changeEvent(QEvent* event) override;
    // the play buttons in two lines when the panel is narrow (TransportBar::setWrapped)
    void resizeEvent(QResizeEvent* event) override;

  private:
    void setCurrent(int index, bool show);
    void refresh();
    void refreshButtons();
    void refreshState();
    void applyStyle();
    void tick();

    QJsonObject m_replay;
    QVector<LabPlayerModel::Moment> m_moments;
    QJsonArray m_logEntries;
    int m_current = 0;
    int m_firstDiff = -1;
    bool m_showing = false;
    bool m_expert = false;
    bool m_styling = false;
    QTimer m_timer;

    QLabel* m_title = nullptr;
    QLabel* m_position = nullptr;
    TransportBar* m_bar = nullptr;
    QListWidget* m_list = nullptr;
    QPushButton* m_firstDifference = nullptr;
    QPushButton* m_now = nullptr;
    QLabel* m_view = nullptr;
    QLabel* m_stateTitle = nullptr;
    QListWidget* m_state = nullptr;
    QLabel* m_logTitle = nullptr;
    QPlainTextEdit* m_log = nullptr;
};

#endif
