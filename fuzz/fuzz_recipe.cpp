/*
 * PinglyMotion - fuzz target for the Script Lab recipe (job A3, record and replay)
 *
 * Any bytes, read as UTF-8, go to LabRecipe::parse (the recipe.rlvtest reader). A recipe that is read
 * is written again and read back, which must give the same steps; then it is replayed on a small object
 * held in memory (a HUD that shows a menu and a cuff that locks), as the program does after a save. The
 * replay must stay inside its limits, its facts must survive the result file (writeResult, then
 * parseResult) and compare equal to themselves. The input is also read as a result file
 * (LabRecipe::parseResult) and compared with the replay's facts. Nothing may crash.
 * Job B3 (the player): the input is also read as the moments' fingerprints (parseFingerprints); the
 * replay keeps one moment after the start and after every step, in order, inside the log; the
 * fingerprints written to the result file read back the same, and firstDifference stays inside the
 * moments.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdlib>

#include <QString>

#include "scriptlab/labrecipe.h"
#include "scriptlab/lsl/world.h"

static void check(bool ok)
{
  if (!ok) abort();
}

static LabFolder::Linkset cuffs()
{
  LabFolder::Linkset ls;
  ls.name = QStringLiteral("Cuffs");
  ls.folder = QStringLiteral("/fuzz/Cuffs");
  ls.prims.push_back({1, QStringLiteral("Root"), QStringLiteral("Root")});
  ls.prims.push_back({2, QStringLiteral("Cuff"), QStringLiteral("Cuff")});
  ls.scripts.push_back({QStringLiteral("hud"), QStringLiteral("Root/hud.lsl"), 1, QStringLiteral(
    "default { state_entry() { llListen(-77, \"\", llGetOwner(), \"\"); llOwnerSay(\"Ready.\"); }\n"
    "touch_start(integer n) { llDialog(llDetectedKey(0), \"Cuffs\", [\"Lock\", \"Free\"], -77); }\n"
    "listen(integer c, string name, key id, string m) { llMessageLinked(LINK_ALL_OTHERS, 1, m, NULL_KEY); } }")});
  ls.scripts.push_back({QStringLiteral("cuff"), QStringLiteral("Cuff/cuff.lsl"), 2, QStringLiteral(
    "default { state_entry() { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); }\n"
    "link_message(integer s, integer n, string m, key id) {\n"
    "  if (m == \"Lock\") { llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES); llOwnerSay(\"@detach=n\"); llOwnerSay(\"Locked.\"); }\n"
    "  else { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); llOwnerSay(\"@detach=y\"); llSetTimerEvent(5.0); } }\n"
    "timer() { llOwnerSay(\"tick\"); } }")});
  return ls;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  if (size > 8192) return 0;
  const QString text = QString::fromUtf8(reinterpret_cast<const char*>(data), int(size));

  // the input as a result file: any lines, never a crash
  const QVector<LabRecipe::Fact> asResult = LabRecipe::parseResult(text);
  check(asResult.size() <= LabRecipe::MAX_FACTS);
  const QStringList keptMoments = LabRecipe::parseFingerprints(text);
  check(keptMoments.size() <= LabRecipe::MAX_STEPS + 1);

  const LabRecipe::Result parsed = LabRecipe::parse(text);
  check(parsed.ok() == parsed.value.has_value());
  if (!parsed.ok())
  {
    check(!parsed.error.isEmpty());
    return 0;
  }
  const LabRecipe::Recipe& recipe = *parsed.value;
  check(recipe.steps.size() <= LabRecipe::MAX_STEPS);
  for (const LabRecipe::Step& s : recipe.steps)
  {
    check(!LabRecipe::stepLine(s).isEmpty());
    check(!LabRecipe::stepWords(s).isEmpty());
  }
  // write and read again: the same steps
  const LabRecipe::Result again = LabRecipe::parse(LabRecipe::write(recipe));
  check(again.ok());
  check(again.value->steps == recipe.steps);

  // at most a few steps replayed, so one input stays fast
  LabRecipe::Recipe shortRecipe = recipe;
  if (shortRecipe.steps.size() > 12) shortRecipe.steps.resize(12);
  const LabRecipe::Replay replay = LabRecipe::replay(cuffs(), QJsonObject{{"seconds", 1}}, shortRecipe, Rlv::Mode::Rlva);
  check(replay.ran);
  check(replay.steps == shortRecipe.steps.size());
  check(replay.done >= 0 && replay.done <= replay.steps);
  check(replay.facts.size() <= LabRecipe::MAX_FACTS);
  check(replay.log.size() <= lsl::World::MAX_LOG_ENTRIES);
  for (const LabRecipe::Fact& f : replay.facts)
  {
    check(!f.key.isEmpty() && !f.subject.isEmpty() && !f.state.isEmpty());
    check(!f.absent().isEmpty());
  }
  check(LabRecipe::compare(replay.facts, replay.facts).isEmpty());
  const QStringList changes = LabRecipe::compare(asResult, replay.facts);
  const QString file = LabRecipe::writeResult(replay, changes, false);
  check(LabRecipe::compare(LabRecipe::parseResult(file), replay.facts).isEmpty());

  // the player's moments: the start and every step, in order, each inside the log
  check(replay.frames.size() == replay.steps + 1);
  int lastEnd = 0;
  for (int i = 0; i < replay.frames.size(); ++i)
  {
    const LabRecipe::Frame& f = replay.frames.at(i);
    check(f.step == i);
    check(f.logEnd >= lastEnd && f.logEnd <= replay.log.size());
    check(f.facts.size() <= LabRecipe::MAX_FACTS);
    lastEnd = f.logEnd;
  }
  check(replay.frames.last().facts == replay.facts);
  QStringList fingerprints;
  for (const LabRecipe::Frame& f : replay.frames) fingerprints << LabRecipe::frameFingerprint(f);
  check(LabRecipe::parseFingerprints(file) == fingerprints);
  check(LabRecipe::firstDifference(fingerprints, replay.frames) == -1);
  const int first = LabRecipe::firstDifference(keptMoments, replay.frames);
  check(first >= -1 && first < replay.frames.size());
  LabRecipe::toJson(replay, changes, false, first);
  return 0;
}
