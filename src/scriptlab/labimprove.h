/*
 * PinglyMotion Script Lab - what can be better, and can this run here (queue 1, item 5, part c)
 *
 * Two reports on a watched object, in plain words, made from what the program already knows: the
 * scripts' text (parsed, and the static warnings of lsl_lint), the folder run's check and log, and the
 * weight (labweight.h).
 *
 * Suggestions, each with the script and line it is about when there is one:
 *  - a long list or text in a global variable: keep it in Linkset Data or a notecard instead;
 *  - messages that the script getting them did nothing with: send them only where they are needed;
 *  - small scripts (join some of them: every script fewer is 64 KB less) and a script near its memory
 *    limit (split it);
 *  - llSetMemoryLimit on scripts whose values use little (measure in Second Life first: the memory the
 *    code itself takes is not counted here);
 *  - global variables and functions that are never used.
 * The thresholds are this program's own rules of thumb, written below, not Second Life's.
 *
 * «Can this run here?»: one report per script: does not compile, stops (a function the sandbox does not
 * run, an error or a limit), runs with notes (calls the sandbox only records, web requests that get no
 * answer, a script that removes itself) or runs; with what works and what is missing.
 *
 * QtCore only. Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABIMPROVE_H
#define LABIMPROVE_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "labfolder.h"
#include "labweight.h"

namespace LabImprove
{
  // the rules of thumb (PinglyMotion's own, not Second Life's)
  static constexpr int LONG_LIST_ENTRIES = 20;        // a global list written with this many entries or more
  static constexpr int LONG_TEXT_CHARS = 256;         // a global string written with this many characters or more
  static constexpr qint64 SMALL_SCRIPT_BYTES = 4096;  // values under 4 KB: a candidate to join with another
  static constexpr qint64 FEW_VALUES_BYTES = 16384;   // values under 16 KB: llSetMemoryLimit could keep less
  static constexpr int FULL_PERCENT = 75;             // values over 75 percent of the limit: split the script
  static constexpr int MAX_NAMES = 6;                 // names listed in one sentence, then "and N more"

  struct Suggestion
  {
    QString kind;      // "long-list", "long-text", "messages", "join", "split", "memory-limit", "unused"
    QString script;    // the script it is about ("" for several)
    QString file;      // its file in the folder ("" for several)
    int line = 0;      // the line it is about (0: none)
    QString text;      // one or two plain sentences
    bool operator==(const Suggestion& other) const
    { return kind == other.kind && script == other.script && file == other.file && line == other.line && text == other.text; }
  };

  // What can be better, in the order of the list above, scripts in the folder's order. Empty when the
  // weight is not valid (nothing ran) and no script parses.
  QVector<Suggestion> suggest(const LabFolder::Linkset& linkset, const LabWeight::Weight& weight);

  enum class Verdict { Runs, RunsWithNotes, Stops, DoesNotCompile, NotRun };

  struct Report
  {
    QString script;
    QString file;
    QString prim;
    Verdict verdict = Verdict::Runs;
    QStringList works;     // plain sentences: what runs here
    QStringList missing;   // plain sentences: what does not, or only in part
  };

  // One report per script of the folder run (LabFolder::run's answer: "check" and "result"), in the folder's
  // order. A full log is said once in the reports' notes (later stops may be missing from it).
  QVector<Report> canRun(const LabFolder::Linkset& linkset, const QJsonObject& folderAnswer);
  // "Runs here.", "Runs here, with notes.", "Stops here.", "Does not compile.", "Did not run."
  QString verdictWords(Verdict verdict);

  // {"suggestions":[{kind, script, file, line, text}], "can_run":[{script, file, prim, verdict, works, missing}]}
  QJsonObject toJson(const QVector<Suggestion>& suggestions, const QVector<Report>& reports);
}

#endif
