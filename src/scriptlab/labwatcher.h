/*
 * PinglyMotion Script Lab - Live: watch a folder and run changed scripts (issue 292)
 *
 * Watches one folder the user picks (a cloned repository, for example) for
 * LSL scripts. When a .lsl file in it is saved, the watcher waits a short
 * moment (so an editor's several writes count as one change), reads the
 * file, checks it as lsl_check does and, when it compiles, runs it as
 * lsl_run does. The result is kept per file; the panel and the control
 * command lab_result show the latest one.
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
 * QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABWATCHER_H
#define LABWATCHER_H

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>

class RlvState;

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
    static constexpr int MAX_PATH_CHARS = 4096;

    explicit LabWatcher(QObject* parent = nullptr);

    struct Result
    {
      QString error;              // empty when it worked
      bool ok() const { return error.isEmpty(); }
    };

    // Starts watching a folder (absolute path to an existing folder). Stops
    // watching the folder before, if any. seconds and inputs are given to
    // every run, as in lsl_run; they are checked here. Then the newest script
    // in the folder is run once, so there is a result at once.
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

    // the run options given to watch()
    QJsonObject runOptions() const { return m_options; }

    // The RLV state the runs use (the one rlv_state and the panel show).
    // Not owned. Without it each run has a state of its own.
    void setRlvState(RlvState* state) { m_rlv = state; }

    // Reads, checks and runs one script now (also used by the tests and the
    // first run after watch). relative is a path from scripts().
    void runScript(const QString& relative);

    // looks at the folder again at once (the debounce timer calls it)
    void rescan();

  signals:
    // a script was run again; the panel reads lastResult()
    void resultChanged();
    // started or stopped watching, or the list of scripts changed
    void watchChanged();

  private:
    struct Entry
    {
      QDateTime modified;
      qint64 size = 0;
    };

    QMap<QString, Entry> scan(QStringList* folders);
    void scheduleRescan();
    void runPending();
    void updateWatchedPaths(const QStringList& folders);

    QString m_folder;
    QJsonObject m_options;
    RlvState* m_rlv = nullptr;
    QFileSystemWatcher* m_watcher;
    QTimer m_debounce;
    QMap<QString, Entry> m_snapshot;
    QStringList m_pending;             // changed scripts waiting to run
    QMap<QString, QJsonObject> m_results;
    QJsonObject m_last;
    int m_runs = 0;
    bool m_truncated = false;
};

#endif
