/*
 * PinglyMotion Script Lab - the weight of a setup (queue 1, item 5, part b: «improve a finished setup»)
 *
 * How heavy a watched object is, read from the sandbox after a run or a replay of the recording:
 *
 *  - memory: Second Life keeps 64 KB for every script, less for a script that called llSetMemoryLimit
 *    with a smaller number, so the object keeps the sum of the scripts' limits. The memory a script's
 *    values use at the end of the run is the Live tree's Memory column (a rough count; code is not
 *    counted: that waits for a measurement in Second Life);
 *  - messages: per script, the link messages and chat it sent, the link_message and listen events it
 *    got, how many of those it did nothing with (its handler wrote no global variable, changed no state
 *    and called nothing that acts) and how many of its own messages the script that got them did nothing
 *    with. The counts come from the sandbox itself, so a full event log does not cut them short.
 *
 * Before and after: the weight at the end of a replay is kept in recipe.rlvresult ("=weight" lines that
 * older versions skip) and the next replay says what changed, one plain sentence each.
 *
 * QtCore only. Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABWEIGHT_H
#define LABWEIGHT_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "labfolder.h"

namespace LabWeight
{
  static constexpr qint64 SCRIPT_MEMORY = 65536;      // what Second Life keeps for a script (Mono), the most llSetMemoryLimit allows
  static constexpr int MAX_SCRIPTS = 64;              // lsl::Sandbox::MAX_SCRIPTS: rows read from a result file

  struct Script
  {
    QString name;          // llGetScriptName
    QString prim;          // the prim's name in the folder model
    QString file;          // relative to the object folder ("" when read from a result file)
    qint64 limit = SCRIPT_MEMORY;   // llGetMemoryLimit at the end
    qint64 used = 0;       // the memory of its values at the end (the Memory column)
    qint64 code = -1;      // a rough estimate of the code's size (lsl::Interpreter::codeBytes); -1 when not known (not kept in result files)
    // code and values together, the number to hold against the limit; -1 when the code is not known
    qint64 estimate() const { return code < 0 ? -1 : code + used; }
    qint64 sent = 0, received = 0, unused = 0, ignored = 0;
    bool stopped = false;
    bool operator==(const Script& other) const;
  };

  struct Weight
  {
    bool valid = false;            // false: nothing ran, so nothing was measured
    QVector<Script> scripts;       // in the folder's order (link order, then file name)
    int count() const { return scripts.size(); }
    qint64 reserved() const;       // the sum of the limits
    int limited() const;           // scripts with a limit below SCRIPT_MEMORY
    qint64 sent() const;
    qint64 received() const;
    qint64 unused() const;
    bool operator==(const Weight& other) const { return valid == other.valid && scripts == other.scripts; }
  };

  // The weight of the linkset's object from the "objects" of an lsl_run answer (or Sandbox::objectsJson()):
  // the entry whose scripts belong to it. Not valid when there is none.
  Weight measure(const LabFolder::Linkset& linkset, const QJsonArray& objects);

  // KB as the panel writes it: 1 KB = 1024 bytes, one decimal below 10 KB ("6.2 KB", "64 KB", "1472 KB")
  QString kb(qint64 bytes);

  // The plain lines of the weight: "23 scripts. Second Life keeps 64 KB for each: 1472 KB.", the line for the
  // scripts that set a smaller limit, and the messages: "1814 messages got, 1500 of them not used."
  QStringList summary(const Weight& weight);

  // What changed from before to now, one plain sentence each ("Before: 1472 KB kept for 23 scripts. Now: 1408 KB
  // for 22 scripts."), first the whole object, then each script whose numbers changed, in now's order, then the
  // scripts that are gone. Empty when nothing changed or one of them is not valid.
  QStringList compare(const Weight& before, const Weight& now);

  // The lines kept in recipe.rlvresult: a note, "=weight <count>", then one "=script <limit> <used> <sent> <received>
  // <unused> <ignored> <stopped> <name>" per script. Nothing when not valid.
  QString writeLines(const Weight& weight);
  // The weight in a result file; lines that cannot be read are skipped, at most MAX_SCRIPTS scripts are read.
  // Not valid when the file has no "=weight" line (a file from before the weight view) or the count does not match.
  Weight parseLines(const QString& text);

  // {"valid", "scripts", "reserved", "limited", "sent", "received", "unused", "rows":[{name, prim, limit, used,
  // sent, received, unused, ignored, stopped}]}: for lab_result and the tests
  QJsonObject toJson(const Weight& weight);
}

#endif
