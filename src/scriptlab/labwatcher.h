/*
 * PinglyMotion Script Lab - Live: watch a folder and run changed scripts (issue 292)
 *
 * Watches one folder the user picks (a cloned repository, for example) for
 * LSL scripts. When a .lsl file in it is saved, the watcher waits a short
 * moment (so an editor's several writes count as one change), reads the
 * file, checks it as lsl_check does and, when it compiles, runs it as
 * lsl_run does (with the RLV state the panel shows). The result is kept per file; the panel and the control
 * command lab_result show the latest one.
 *
 * Lost watch (job T2): Windows sometimes stops watching a folder ("Access is
 * denied", for example while something else deletes or locks a file in it) and
 * Qt then drops the folder and its files from the watch list. Each change
 * notice makes the watcher look at the folder again and build the watch list
 * anew; when the folder cannot be watched at all (it is gone, or the watch is
 * refused), the watcher looks at the folder itself every RETRY_MS until it can
 * watch it again. A notice for a file whose content is the same as the content
 * that last ran (the same save seen twice, or a dropped watch) does not run it
 * again; a real change always does, also when the time and size look the same.
 *
 * Safety: the folder is only read, never written. At most MAX_SCRIPTS
 * scripts of at most MAX_FILE_BYTES each are watched, the scan stops after
 * MAX_ENTRIES entries and MAX_DEPTH folder levels, hidden folders (.git) are
 * skipped, and links are never followed out of the folder (a linked folder
 * is not entered at all; a linked file counts only when its target is
 * inside the folder). Each run has the interpreter's limits, including its
 * real-time cap, and one batch runs at most MAX_RUNS_PER_BATCH scripts
 * before the program gets back to the user.
 *
 * Linkset mode (run option "linkset": true, lane 1 job B): the folder is one
 * object (LabFolder, labfolder.h). Any saved script or notecard runs the
 * whole folder again as one linkset; each script's result holds its own
 * check and the shared run, and folderResult() the whole folder's answer.
 *
 * Record and replay (job A3, labrecipe.h): the only files the watcher ever
 * writes are recipe.rlvtest (while the user records in the panel) and
 * recipe.rlvresult (after a replay), both in the top of the watched folder and
 * never through a link. A folder run started by a save replays the recipe when
 * there is one.
 *
 * QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABWATCHER_H
#define LABWATCHER_H

#include <QDateTime>
#include <QHash>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>

#include <memory>

#include "labfolder.h"
#include "labrecipe.h"

class RlvState;
namespace lsl { class Session; }

class LabWatcher : public QObject
{
  Q_OBJECT

  public:
    static constexpr int MAX_SCRIPTS = 200;
    static constexpr qint64 MAX_FILE_BYTES = 262144;
    static constexpr int MAX_ENTRIES = 5000;
    static constexpr int MAX_DEPTH = 4;
    static constexpr int MAX_RUNS_PER_BATCH = 4;
    static constexpr int DEBOUNCE_MS = 300;
    // how often the folder is looked at while it cannot be watched (job T2)
    static constexpr int RETRY_MS = 1000;
    static constexpr int MAX_PATH_CHARS = 4096;

    explicit LabWatcher(QObject* parent = nullptr);
    ~LabWatcher() override;

    struct Result
    {
      QString error;              // empty when it worked
      bool ok() const { return error.isEmpty(); }
    };

    // Starts watching a folder (absolute path to an existing folder). Stops
    // watching the folder before, if any. seconds and inputs are given to
    // every run, as in lsl_run (also keep_rlv); they are checked here. With
    // "linkset": true the folder is one object and every run runs all its
    // scripts together (LabFolder). Then the newest script in the folder is
    // run once (in linkset mode: the whole folder), so there is a result at once.
    Result watch(const QString& folder, const QJsonObject& runOptions = QJsonObject());
    // stops watching; false when nothing was watched
    bool unwatch();

    bool watching() const { return !m_folder.isEmpty(); }
    // the watched folder, as an absolute path without links ("" when none)
    QString folder() const { return m_folder; }
    // the scripts found, relative to the folder, sorted
    QStringList scripts() const { return m_snapshot.keys(); }
    // how many runs since the program started (also counts unreadable files)
    int runCount() const { return m_runs; }
    // the newest result (empty object when there is none yet)
    QJsonObject lastResult() const { return m_last; }
    // the newest result of one script, by its path relative to the folder
    QJsonObject result(const QString& relative) const { return m_results.value(relative); }
    // the absolute path of a watched script, "" when it is not one
    QString absolutePath(const QString& relative) const;
    // true when the scan stopped at a limit (too many entries or scripts)
    bool scanTruncated() const { return m_truncated; }
    // the folders and files the system watches for us now (absolute paths, sorted)
    QStringList watchedPaths() const;
    // true while the folder is watched but the system does not watch it for us, so the
    // watcher looks at it itself every RETRY_MS (job T2)
    bool retrying() const { return m_retry.isActive(); }
    // for the tests: does what Qt does when Windows stops watching a folder with "Access is
    // denied": the folder and its files are dropped from the watch list and reported removed
    void simulateLostWatchForTests();

    // the run options given to watch()
    QJsonObject runOptions() const { return m_options; }
    // true when the folder is watched as one object (run option "linkset")
    bool linkset() const { return m_linkset; }
    // linkset mode: the whole folder's newest answer (LabFolder::run plus "run" and "file", the file
    // whose change started it); an empty object otherwise
    QJsonObject folderResult() const { return m_folderResult; }

    // The RLV state the runs use (the one rlv_state and the panel show).
    // Not owned. Without it each run has a state of its own.
    void setRlvState(RlvState* state) { m_rlv = state; }

    // Linkset mode (job A1, a click on a prop): sends a touch to the prim of that link in the world of
    // the last folder run and runs the world on for one simulated second. A "clicked" line (with the
    // prim's name) and the touch's own log lines are added to the log of folderResult() and of every
    // script's result, and resultChanged() is emitted. Refused, with the reason, when no folder is
    // watched as one object, the folder did not run, the link is not a prim of it, or every script
    // has stopped. The next save runs the folder again from the start.
    Result touch(int link);
    // the link of the prim of that name in the last folder run (0 when there is none)
    int linkOf(const QString& prim) const;

    // Linkset mode (job A2, the menu box over the 3D view): the newest menu (llDialog or llTextBox)
    // a script of the last folder run showed, as lsl::Sandbox::lastDialog gives it (object, message,
    // buttons, channel, "text_box"); empty when there is none or the folder did not run.
    QJsonObject dialog() const;
    // Presses a button of that menu (or sends the typed text of a text box): the same lsl_event
    // "dialog" input the panel's menu sends, so the avatar says the text on the menu's channel, and
    // the world runs on for one simulated second. A "clicked" line with the button and the channel
    // comes first in the log. Refused, with the reason, when no menu is open.
    Result answerDialog(const QString& button);
    // the lsl_event input that answers this dialog with this button (shared with the panel's menu)
    static QJsonObject dialogAnswer(const QJsonObject& dialog, const QString& button);

    // --- record and replay (job A3) ----------------------------------------------------------
    // Starts recording in linkset mode: from now on a touch(), an answerDialog() and a recordLine() that
    // work are steps of a new recipe. The old recipe file is replaced when the first step comes, so
    // starting and stopping without a step keeps it. Refused when no folder is watched as one object.
    Result startRecording();
    // Stops recording. With steps, the recipe is replayed at once and its facts are kept as the first
    // result (nothing to compare with yet). False when it was not recording.
    bool stopRecording();
    bool recording() const { return m_recording; }
    // the steps recorded since startRecording()
    QVector<LabRecipe::Step> recordedSteps() const { return m_recorded; }
    // an RLV line the user typed or chose in the panel, from that object; a step only while recording
    void recordLine(const QString& object, const QString& line);
    // the recipe file of the watched folder ("" when no folder is watched as one object)
    QString recipePath() const;
    bool hasRecipe() const;
    // Replays the recipe file now (as after a save): the folder from the start in a world of its own,
    // then each step; compares the facts with the result file and writes the new result file.
    // first: do not compare (the steps are new). Refused, with the reason, when no folder is watched
    // as one object, there is no recipe, it cannot be read, or the result cannot be written.
    Result replayRecipe(bool first = false);
    // the newest replay: LabRecipe::toJson plus "run" (the folder run it followed), "recipe" (the file
    // name) and "steps_text" (the recipe's step lines); empty when there was none since watching
    QJsonObject lastReplay() const { return m_replay; }

    // Reads, checks and runs one script now (also used by the tests and the
    // first run after watch). relative is a path from scripts(). In linkset
    // mode it runs the whole folder, with relative as the file that changed.
    void runScript(const QString& relative);

    // looks at the folder again at once (the debounce timer calls it)
    void rescan();

  signals:
    // a script was run again; the panel reads lastResult()
    void resultChanged();
    // started or stopped watching, or the list of scripts changed
    void watchChanged();
    // recording started or stopped, a step was recorded, or a replay finished (job A3)
    void recipeChanged();

  private:
    struct Entry
    {
      QDateTime modified;
      qint64 size = 0;
    };

    QMap<QString, Entry> scan(QStringList* folders, QMap<QString, Entry>* notecards = nullptr);
    void runFolder(const QString& trigger);
    // sends one lsl_event input to the world of the last folder run, with "clicked" first in the log
    Result sendEvent(const QJsonObject& request, QJsonObject clicked);
    void scheduleRescan();
    void runPending();
    void updateWatchedPaths(const QStringList& folders);
    // a change notice for a watched file (also "removed", when the system dropped it)
    void fileNotice(const QString& path);
    // the SHA-1 of a file's content (at most MAX_FILE_BYTES + 1 bytes); empty when it cannot be read
    static QByteArray contentHash(const QString& path);

    QString m_folder;
    QJsonObject m_options;
    RlvState* m_rlv = nullptr;
    QFileSystemWatcher* m_watcher;
    QTimer m_debounce;
    QTimer m_retry;                    // job T2: looks at the folder while it cannot be watched
    QHash<QString, QByteArray> m_seen; // job T2: the content hash of each file when it last changed or ran
    QMap<QString, Entry> m_snapshot;
    QMap<QString, Entry> m_notecards;  // linkset mode: notecards, so a saved notecard runs the folder again
    bool m_linkset = false;
    QJsonObject m_folderResult;
    LabFolder::Linkset m_lastLinkset;   // linkset mode: the folder as the last run read it
    lsl::Session* m_world;              // linkset mode: the world of the last folder run, for touch()
    std::unique_ptr<RlvState> m_ownRlv; // the RLV state of that world when no state was given
    QStringList m_pending;             // changed scripts waiting to run
    QMap<QString, QJsonObject> m_results;
    QJsonObject m_last;
    int m_runs = 0;
    bool m_truncated = false;
    // job A3
    void recordStep(const LabRecipe::Step& step);
    bool m_recording = false;
    QVector<LabRecipe::Step> m_recorded;
    QJsonObject m_replay;
};

#endif
