/*
 * PinglyMotion Script Lab - record and replay (job A3)
 *
 * A recipe is what the user did with a watched object, step by step: a click
 * on a prop (a touch), a press on a menu button, an RLV line typed in the
 * panel, a short wait. It is a plain text file in the object folder,
 * recipe.rlvtest, in the scenario format proposed in docs/RLV_TOOLS_REPO.md
 * (lines starting with # are notes, "object" and "send" as there; "touch",
 * "menu" and "wait" are added here):
 *
 *   # PinglyMotion recipe ...
 *   touch Root
 *   menu Lock all
 *   object panel
 *   send @sendim=n
 *   wait 2
 *
 * replay() runs the folder from the start in a world of its own (its own RLV
 * state, so the panel's state is never touched) and then each step, every one
 * one simulated second after the one before, as a click in the program does.
 * The sandbox is deterministic, so the same scripts and the same recipe give
 * the same facts. facts() says in plain words what is true at the end ("Collar
 * is red.", "Gear_set is locked."), and compare() says what changed since the
 * facts kept from last time ("Before: Gear_set was locked. Now: it is not
 * locked."). The facts and the changes are kept next to the recipe in
 * recipe.rlvresult.
 *
 * QtCore only. Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABRECIPE_H
#define LABRECIPE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

#include "labfolder.h"
#include "rlvbehaviours.h"

namespace LabRecipe
{
  static constexpr int MAX_STEPS = 200;              // as lsl_run's inputs
  static constexpr int MAX_LINE_CHARS = 1024;
  static constexpr qint64 MAX_FILE_BYTES = 65536;
  static constexpr int MAX_TEXT_CHARS = 512;         // a menu answer (llTextBox takes up to 512)
  static constexpr double MAX_WAIT_SECONDS = 60.0;
  static constexpr int MAX_FACTS = 400;
  static constexpr int MAX_SAID_CHARS = 80;

  // the file names in the object folder
  QString recipeFile();    // "recipe.rlvtest"
  QString resultFile();    // "recipe.rlvresult"

  struct Step
  {
    enum class Kind { Touch, Menu, Send, Wait };
    Kind kind = Kind::Touch;
    QString text;              // the prim (touch), the button text (menu), the RLV line (send)
    QString object;            // send: the object the line comes from ("panel" unless "object" said else)
    double seconds = 0.0;      // wait
    int line = 0;              // the line in the file, from 1 (0 when made in the program)
    bool operator==(const Step& other) const;
  };

  struct Recipe
  {
    QVector<Step> steps;
  };

  struct Result
  {
    std::optional<Recipe> value;
    QString error;             // for the user, empty when it worked
    bool ok() const { return error.isEmpty(); }
  };

  // Reads a recipe. Refused with the line and the reason for an unknown word, a missing or too long
  // text, a wait outside 0.1 to 60 seconds, more than MAX_STEPS steps or a line over MAX_LINE_CHARS.
  Result parse(const QString& text);
  // The recipe as the file holds it: the notes on top, then one line per step ("object" lines only
  // where the object of a send changes). parse(write(r)) gives r back (without the line numbers).
  QString write(const Recipe& recipe);
  // one step as its line in the file: "touch Collar", "menu Lock all", "send @sendim=n", "wait 2"
  QString stepLine(const Step& step);
  // one step in plain words: "Click Collar.", "Press "Lock all" in the menu.",
  // "panel sends @sendim=n.", "Wait 2 seconds."
  QString stepWords(const Step& step);

  // One thing that is true at the end of a replay. The sentence is "<subject> is <state>.";
  // absent() is the state when the fact is not there any more (for the comparison).
  struct Fact
  {
    QString key;       // "lock:Gear_set", "colour:Collar", "blocked:sendim", "said:Root", "menu", "run", "stopped", "step:2"
    QString subject;   // "Gear_set", "Collar", "Send instant messages", "Root", "the menu", "the object", "step 2"
    QString state;     // "locked", "red", "blocked", "saying "HUD ready."", "open: ...", "not running: ...", "not done: ..."
    QString sentence() const;
    QString absent() const;
    bool operator==(const Fact& other) const { return key == other.key && subject == other.subject && state == other.state; }
  };

  // One moment of a replay, for the player (job B3): what was true right after the start (step 0) or
  // right after step n. The sandbox is deterministic, so the moments are taken once, while the replay
  // runs (docs/PLAYER_CHECK.md, recommendation (b)), and the player steps between them.
  struct Frame
  {
    int step = 0;              // 0: after the start; n: after step n (from 1)
    double t = 0.0;            // simulated seconds since the start
    int logEnd = 0;            // the log entries [0, logEnd) of Replay::log happened by this moment
    bool done = true;          // the step was done (false: it was skipped, see the fact "step:<n>")
    QVector<Fact> facts;       // the facts at this moment, as facts at the end are made
  };

  struct Replay
  {
    bool ran = false;          // the folder ran (every script compiled, no limit broken)
    QString error;             // why the folder did not run
    int steps = 0;             // steps in the recipe
    int done = 0;              // steps that were done (the others are facts "step:<n>")
    QVector<Fact> facts;       // the facts at the end (the last frame's facts)
    QJsonArray log;            // the whole replay's event log, a "step" line before each step
    bool logTruncated = false;
    QVector<Frame> frames;     // the start and every step, in order (empty when the folder did not run)
    QStringList stepTexts;     // every step in plain words (stepWords), for the player's list
  };

  // Runs the linkset from the start with options (as lsl_run: seconds, inputs) in a world of its own,
  // with an RLV state of its own in that mode, then every step one simulated second apart.
  // A step that cannot be done (no prim of that name, no menu open, every script stopped, an RLV line
  // refused) is skipped and becomes a fact; the rest go on.
  Replay replay(const LabFolder::Linkset& linkset, const QJsonObject& options, const Recipe& recipe, Rlv::Mode mode);

  // What changed from before to now, one plain sentence each, in the order of now's facts and then the
  // facts that went away: "Before: Collar was green. Now: it is red." Empty when nothing changed.
  QStringList compare(const QVector<Fact>& before, const QVector<Fact>& now);

  // The result file: notes, the facts (one "[key] sentence" line each), the fingerprint of every
  // moment (one "@<moment> <fingerprint>" line each, job B3), then the changes as notes.
  // first: there was nothing to compare with.
  QString writeResult(const Replay& replay, const QStringList& changes, bool first);
  // The facts of a result file (lines that start with "["); other lines are skipped. A line that cannot
  // be read is skipped too: the file is the program's own memory of last time, not something to refuse.
  QVector<Fact> parseResult(const QString& text);

  // --- the player (job B3): which moment first differed from the kept result
  // A short fingerprint of a moment: 16 hex digits of the SHA-1 of its step and its facts. Two moments
  // with the same fingerprint show the same facts.
  QString frameFingerprint(const Frame& frame);
  // The fingerprints kept in a result file (lines "@<moment> <fingerprint>", written by writeResult),
  // by moment; a moment without a line is an empty string. Lines that cannot be read are skipped, and
  // at most MAX_STEPS + 1 moments are read. Empty for a file from before the player.
  QStringList parseFingerprints(const QString& text);
  // The first moment whose facts differ from the kept fingerprints: the first moment where both have a
  // fingerprint and they differ; or, when all shared moments agree, the first moment only one of them
  // has (the last moment of now when the kept recipe was longer). -1 when nothing differs or nothing
  // was kept.
  int firstDifference(const QStringList& kept, const QVector<Frame>& now);

  // the plain name of a colour from red, green and blue between 0 and 1 ("red", "light blue" ...)
  QString colourName(double red, double green, double blue);

  // The replay as data for lab_replay and lab_result: steps, done, ran, error, facts (sentences),
  // changes, first, log, and for the player (job B3) frames (step, t, log_end, done, text, facts as
  // objects with key and text) and first_difference (a moment, or null).
  QJsonObject toJson(const Replay& replay, const QStringList& changes, bool first, int firstDifference = -1);
}

#endif
