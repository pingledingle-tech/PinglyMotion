/*
 * PinglyMotion Script Lab - the folder is the object (lane 1, job B)
 *
 * Reads one folder as one Second Life object (one linkset) and runs all its
 * scripts together in one simulated object, so llMessageLinked and
 * link_message reach each other (docs/SCRIPT_LAB.md, "The folder model").
 *
 *   MyObject/            the top folder: one object, named after the folder
 *     main.lsl           scripts and notecards right in the top folder belong to the root prim
 *     Root/              a subfolder named "Root" (any case) is the root prim too
 *       settings.txt     a notecard (.txt or .notecard; its name is the file name without the extension)
 *     Cuff_left/         every other subfolder is one child prim, links 2, 3, ... in name order
 *       cuff.lsl         a script's name is its file name without ".lsl"
 *
 * All scripts are checked at once; every problem names its file, script and
 * prim. A run only starts when the folder breaks no limit and every script
 * compiles. Every log line of a run names its source: "prim" and "file" are
 * added to each entry of a script (next to "script"), and an entry that no
 * script made (an input, the sandbox itself) gets "prim" and "source":"sandbox".
 *
 * Limits (each one is reported as a problem, nothing runs): the folder path
 * rules of Live, at most MAX_ENTRIES entries, MAX_SCRIPTS scripts (the
 * sandbox's limit for one object), MAX_PRIMS prims, MAX_FILE_BYTES per file,
 * MAX_TOTAL_BYTES for all scripts and notecards together, MAX_RELATIVE_PATH
 * characters for a path inside the folder and MAX_NAME characters for a name.
 * Endless loops: every script has the interpreter's limits; the real-time
 * cap counts from when the scripts are made, which is the same moment for
 * all scripts of a run, so a whole folder of endless loops stops after about
 * Limits::maxWallMs and each stop is in the log with its script and prim.
 *
 * Read only: the folder is never written. Hidden names (.git) are skipped
 * and links are never followed out of the folder. QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABFOLDER_H
#define LABFOLDER_H

#include <vector>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class RlvState;
namespace lsl { class Session; }

namespace LabFolder
{
  static constexpr int MAX_ENTRIES = 5000;
  static constexpr int MAX_SCRIPTS = 32;              // lsl::Sandbox::MAX_SCRIPTS_PER_OBJECT
  static constexpr int MAX_PRIMS = 256;               // lsl::Sandbox::MAX_PRIMS
  static constexpr int MAX_NOTECARDS = 224;           // lsl::Inventory::MAX_ITEMS less the scripts
  static constexpr qint64 MAX_FILE_BYTES = 262144;    // as Live and lsl::Lexer::MAX_SOURCE_CHARS
  static constexpr qint64 MAX_TOTAL_BYTES = 1048576;  // as lsl::Control::MAX_TOTAL_SOURCE_CHARS
  static constexpr int MAX_RELATIVE_PATH = 255;
  static constexpr int MAX_NAME = 63;                 // object, prim, script and notecard names, as in Second Life
  static constexpr int MAX_PATH_CHARS = 4096;
  // Simulated time of a folder run when the options give no "seconds": a big setup (a collar with
  // many scripts) needs more than the flat 10 seconds of lsl_run. AUTO_BASE_SECONDS plus
  // AUTO_PER_SCRIPT_SECONDS for each script, never more than AUTO_MAX_SECONDS. Real-time and step
  // limits of the interpreter are unchanged, so a runaway script still stops at those.
  static constexpr int AUTO_BASE_SECONDS = 10;
  static constexpr int AUTO_PER_SCRIPT_SECONDS = 2;
  static constexpr int AUTO_MAX_SECONDS = 60;
  // the automatic simulated seconds for a folder with this many scripts
  int autoSeconds(int scripts);

  struct Prim
  {
    int link = 1;
    QString name;        // the folder's name; the root prim without a "Root" folder has the object's name
    QString folder;      // relative to the top folder: "" for the top folder itself
  };

  struct Script
  {
    QString name;        // llGetScriptName: the file name without ".lsl"
    QString file;        // relative to the top folder, with "/"
    int link = 1;
    QString source;
  };

  struct Notecard
  {
    QString name;
    QString file;
    int link = 1;
    QStringList lines;
  };

  // one limit or rule a folder breaks, or a note about something left out
  struct Problem
  {
    QString message;
    QString file;        // relative path, "" when it is about the whole folder
    QString prim;        // "" when it is about the whole folder
    int link = 0;        // 0 when it is about the whole folder
    QJsonObject toJson() const;
  };

  struct Linkset
  {
    QString error;                 // the folder itself cannot be read (path rules); nothing else is filled then
    QString folder;                // absolute, without links
    QString name;                  // the object's name: the top folder's name
    std::vector<Prim> prims;       // link 1 first
    std::vector<Script> scripts;   // in link order, then by file name
    std::vector<Notecard> notecards;
    std::vector<Problem> problems; // limits and rules broken: nothing runs while there is one
    std::vector<Problem> notes;    // things left out that do not stop a run (a nested folder, a link)
    bool truncated = false;        // the scan stopped at MAX_ENTRIES
    bool ok() const { return error.isEmpty() && problems.empty(); }
    // the prim of a link ("" when there is none)
    QString primName(int link) const;
    // {"object", "folder", "prims":[{link, name, folder, scripts:[...], notecards:[...]}], "problems", "notes"}
    QJsonObject toJson() const;
  };

  // The folder path rules of Live: "" when the path may be opened, else why not. canonical gets the
  // absolute path without links.
  QString pathError(const QString& folder, QString* canonical = nullptr);

  // Reads the folder as one object. Never runs anything and never writes.
  Linkset open(const QString& folder);

  // Checks every script at once (as lsl_check). {"valid", "scripts":[{file, script, prim, link, valid,
  // errors, warnings}], "errors":[...], "warnings":[...]}: every error and warning has file, script, prim,
  // link, line, column and message.
  QJsonObject check(const Linkset& linkset);

  // The lsl_run request that runs the linkset as one object (options: seconds, inputs, keep_rlv).
  QJsonObject request(const Linkset& linkset, const QJsonObject& options = QJsonObject());

  // Checks and, when the folder breaks no limit and every script compiles, runs it as one object.
  // {"ok", "linkset" (toJson), "check" (as check()), "result" (the lsl_run answer, with every log entry
  // tagged with its source) only when it ran, "error" when it did not}. rlv: as lsl::Control::run.
  // With rlv and session the world of the run is kept in session, so a touch can be sent to it later
  // (job A1: a click on a prop); a folder that did not run leaves session as it was.
  QJsonObject run(const Linkset& linkset, const QJsonObject& options = QJsonObject(), RlvState* rlv = nullptr,
                  lsl::Session* session = nullptr);

  // Adds the source of every log entry of an lsl_run answer of this linkset (used by run()).
  void tagLog(const Linkset& linkset, QJsonObject& answer);
}

#endif
