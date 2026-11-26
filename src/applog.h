/*
 * PinglyMotion
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The program's local error log (issue 239).
 *
 * One logger for all code: it is Qt's message handler, so every qCDebug, qCInfo,
 * qCWarning, qCCritical and qFatal (with the categories in logging.h) lands here,
 * and is also passed on to the console as before. On top of that it catches
 * uncaught C++ exceptions (std::set_terminate) and, best effort, fatal signals.
 *
 * - The log stays on the user's computer, in the application data folder
 *   (GenericDataLocation/PinglyMotion/logs/pinglymotion.log). Nothing is sent anywhere.
 * - Every line is written and flushed at once, so it survives a crash.
 * - At most maxBytes (5 MB); when full, the oldest lines are dropped.
 * - The home folder is replaced by "<home>" in every line. Log messages never hold
 *   animation content (joint values, key data); write what happened, not the data.
 * - A marker file exists while the program runs and is removed on a clean exit.
 *   If it is still there at the next start, the last session crashed (or was
 *   killed) and the log says so.
 */
#ifndef APPLOG_H
#define APPLOG_H

#include <QByteArray>
#include <QString>

namespace AppLog
{
// 5 MB, the most the log file may hold
constexpr qint64 defaultMaxBytes = 5 * 1024 * 1024;

// the placeholder that replaces the home folder in the log
inline const char* homePlaceholder() { return "<home>"; }

struct Options
{
  // folder of the log file and the marker; empty: defaultDirectory()
  QString directory;
  // the folder to hide from the log; empty: QDir::homePath()
  QString homePath;
  qint64 maxBytes = defaultMaxBytes;
  // std::set_terminate and the fatal signal handlers (tests may leave them out)
  bool crashHandlers = true;
  // pass messages on to the previous handler (the console)
  bool forwardToConsole = true;
};

// GenericDataLocation/PinglyMotion/logs
QString defaultDirectory();

// Opens the log, notes a crashed last session, writes a start line (version,
// operating system) and installs the handlers. Returns false when the log file
// cannot be opened (the program then runs as before, console only).
bool install(const Options& options = Options());

// Writes the clean-exit line and removes the marker. Call when the program ends normally.
void markCleanExit();

// Restores the previous handlers and closes the log (tests; the program does not need it).
void uninstall();

// The log file in use (empty when not installed).
QString filePath();

// True when the marker of the last session was still there at install().
bool lastSessionCrashed();

// Empties the log file. Returns false when it is not installed or cannot be emptied.
bool clear();

// --- pure helpers, used inside and tested on their own ---

// Replaces the home folder (with / or \ as separator; case-insensitive on Windows)
// by homePlaceholder().
QString scrubPersonalData(const QString& text, const QString& homePath);

// The newest whole lines of content that fit in maxBytes (oldest lines dropped).
QByteArray trimToLimit(const QByteArray& content, qint64 maxBytes);
}

#endif
