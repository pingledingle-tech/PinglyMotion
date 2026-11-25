/*
 * PinglyMotion Script Lab - «improve a finished setup», parts b and c (queue 1, item 5 in the jobs list, issue 74)
 *
 * The weight of a watched object (labweight.h): memory kept and messages per script, counted by the
 * sandbox itself; the result file lines and the before-and-after sentences of a replay; the suggestions
 * and «Can this run here?» (labimprove.h), all with exact answers; and the same on the OpenCollar collar.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "scriptlab/rlvstate.h"
#include "scriptlab/labfolder.h"
#include "scriptlab/labimprove.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/labweight.h"

class LabWeightTest : public QObject
{
    Q_OBJECT
  private slots:
    void countsMessagesAndMemory();
    void summaryAndKb();
    void resultLinesRoundTrip();
    void compareSaysWhatChanged();
    void replayKeepsWeightAndComparesIt();
    void suggestions();
    void canRunReports();
    void canRunWhenAScriptDoesNotCompile();
    void openCollar();

  private:
    static bool writeFile(const QString& path, const QByteArray& text);
    static QString makeSetup(const QTemporaryDir& dir, bool lighter = false);
    static LabWeight::Weight run(const QString& folder, QJsonObject* answer = nullptr);
};

bool LabWeightTest::writeFile(const QString& path, const QByteArray& text)
{
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  return f.open(QIODevice::WriteOnly) && f.write(text) == text.size();
}

// A small setup: hub sends three link messages to every script; user uses them and keeps 16 KB; idle and idle2
// do nothing with them; idle holds a long list and things it never uses. lighter: hub sends to the prim User only.
QString LabWeightTest::makeSetup(const QTemporaryDir& dir, bool lighter)
{
  const QString top = dir.filePath("Setup");
  writeFile(top + "/Root/hub.lsl", QByteArray("default { state_entry() {\n"
                                              "  integer i;\n"
                                              "  for (i = 1; i <= 3; ++i) llMessageLinked(") + (lighter ? "2" : "LINK_SET") + ", i, \"go\", \"\");\n"
                                              "} }\n");
  writeFile(top + "/Root/idle.lsl", "list names = [\"a\", \"b\", \"c\", \"d\", \"e\", \"f\", \"g\", \"h\", \"i\", \"j\",\n"
                                    "              \"k\", \"l\", \"m\", \"n\", \"o\", \"p\", \"q\", \"r\", \"s\", \"t\"];\n"
                                    "integer spare;\n"
                                    "noop() { }\n"
                                    "default { link_message(integer s, integer n, string m, key id) { if (n == 99) llOwnerSay(llList2String(names, 0)); } }\n");
  writeFile(top + "/Root/idle2.lsl", "default { link_message(integer s, integer n, string m, key id) { if (n == 99) llOwnerSay(m); } }\n");
  writeFile(top + "/User/user.lsl", "integer got;\n"
                                    "default { state_entry() { llSetMemoryLimit(16384); }\n"
                                    "  link_message(integer s, integer n, string m, key id) { got = got + n; if (got == 6) llOwnerSay(\"all\"); } }\n");
  return top;
}

LabWeight::Weight LabWeightTest::run(const QString& folder, QJsonObject* answer)
{
  const LabFolder::Linkset ls = LabFolder::open(folder);
  const QJsonObject a = LabFolder::run(ls, QJsonObject{{"seconds", 2}});
  if (answer) *answer = a;
  return LabWeight::measure(ls, a.value("result").toObject().value("objects").toArray());
}

static QStringList rows(const LabWeight::Weight& w)
{
  QStringList out;
  for (const LabWeight::Script& s : w.scripts)
    out << QString("%1 %2 limit %3 used %4 sent %5 got %6 unused %7 ignored %8%9").arg(s.name, s.prim).arg(s.limit).arg(s.used)
             .arg(s.sent).arg(s.received).arg(s.unused).arg(s.ignored).arg(s.stopped ? " stopped" : "");
  return out;
}

void LabWeightTest::countsMessagesAndMemory()
{
  QTemporaryDir dir;
  const LabWeight::Weight w = run(makeSetup(dir));
  QVERIFY(w.valid);
  // in the folder's order: the root prim's scripts by file name, then the prim User. hub's three messages reach all four
  // scripts; hub has no link_message event, user uses them, idle and idle2 do nothing with them (6 times in all)
  QCOMPARE(rows(w), QStringList({"hub Root limit 65536 used 0 sent 3 got 0 unused 0 ignored 6",
                                 "idle Root limit 65536 used 392 sent 0 got 3 unused 3 ignored 0",
                                 "idle2 Root limit 65536 used 0 sent 0 got 3 unused 3 ignored 0",
                                 "user User limit 16384 used 16 sent 0 got 3 unused 0 ignored 0"}));
  QCOMPARE(w.count(), 4);
  QCOMPARE(w.reserved(), qint64(3 * 65536 + 16384));
  QCOMPARE(w.limited(), 1);
  QCOMPARE(w.sent(), qint64(3));
  QCOMPARE(w.received(), qint64(9));
  QCOMPARE(w.unused(), qint64(6));
  const QJsonObject json = LabWeight::toJson(w);
  QCOMPARE(json.value("reserved").toDouble(), double(3 * 65536 + 16384));
  QCOMPARE(json.value("rows").toArray().at(3).toObject(),
           QJsonObject({{"name", "user"}, {"prim", "User"}, {"limit", 16384}, {"used", 16}, {"sent", 0}, {"received", 3},
                        {"unused", 0}, {"ignored", 0}, {"stopped", false}}));
  // a run that did not happen measures nothing
  QVERIFY(!LabWeight::measure(LabFolder::open(dir.filePath("Setup")), QJsonArray()).valid);
}

void LabWeightTest::summaryAndKb()
{
  QCOMPARE(LabWeight::kb(65536), QString("64 KB"));
  QCOMPARE(LabWeight::kb(6348), QString("6.2 KB"));
  QCOMPARE(LabWeight::kb(1024), QString("1 KB"));
  QCOMPARE(LabWeight::kb(0), QString("0 KB"));
  QCOMPARE(LabWeight::kb(1507328), QString("1472 KB"));
  QTemporaryDir dir;
  QCOMPARE(LabWeight::summary(run(makeSetup(dir))), QStringList({
    "4 scripts. Second Life keeps 64 KB for each, less for a script with a smaller limit: 208 KB in all.",
    "1 script sets a smaller limit with llSetMemoryLimit.",
    "Messages: 3 sent, 9 got, 6 not used (the script that got it did nothing with it)."}));
  LabWeight::Weight one;
  one.valid = true;
  one.scripts.append(LabWeight::Script());
  one.scripts[0].name = "only";
  QCOMPARE(LabWeight::summary(one).first(), QString("1 script. Second Life keeps 64 KB for each: 64 KB in all."));
  QCOMPARE(LabWeight::summary(LabWeight::Weight()), QStringList());
}

void LabWeightTest::resultLinesRoundTrip()
{
  QTemporaryDir dir;
  const LabWeight::Weight w = run(makeSetup(dir));
  const QString lines = LabWeight::writeLines(w);
  QCOMPARE(lines, QString("# The weight at the end (memory kept and messages per script), for Improve a setup:\n"
                          "=weight 4\n"
                          "=script 65536 0 3 0 0 6 0 hub\n"
                          "=script 65536 392 0 3 3 0 0 idle\n"
                          "=script 65536 0 0 3 3 0 0 idle2\n"
                          "=script 16384 16 0 3 0 0 0 user\n"));
  const LabWeight::Weight back = LabWeight::parseLines("# notes\n[run] the object is running.\r\n" + lines + "@0 0123456789abcdef\n");
  QVERIFY(back.valid);
  QCOMPARE(back.count(), 4);
  QVERIFY(LabWeight::compare(back, w).isEmpty());
  QCOMPARE(back.scripts[3].limit, qint64(16384));
  // a file from before the weight view, a count that does not match, junk: not valid, never a crash
  QVERIFY(!LabWeight::parseLines("[run] the object is running.\n").valid);
  QVERIFY(!LabWeight::parseLines("=weight 2\n=script 65536 0 0 0 0 0 0 one\n").valid);
  QVERIFY(!LabWeight::parseLines("=weight 1\n=script 65536 0 0 0 0 0 2 one\n").valid);
  QVERIFY(!LabWeight::parseLines("=script 65536 0 0 0 0 0 0 one\n=weight 1\n").valid);
  QVERIFY(!LabWeight::parseLines("=weight 999\n").valid);
  QVERIFY(!LabWeight::parseLines(QString(70000, QLatin1Char('='))).valid);
  // a bigger limit than Second Life allows, more unused than got: cut to what can be
  const LabWeight::Weight cut = LabWeight::parseLines("=weight 1\n=script 999999 1 2 3 9 4 1 name with spaces\n");
  QVERIFY(cut.valid);
  QCOMPARE(cut.scripts[0].limit, LabWeight::SCRIPT_MEMORY);
  QCOMPARE(cut.scripts[0].unused, qint64(3));
  QCOMPARE(cut.scripts[0].name, QString("name with spaces"));
  QVERIFY(cut.scripts[0].stopped);
  QCOMPARE(LabWeight::writeLines(LabWeight::Weight()), QString());
}

void LabWeightTest::compareSaysWhatChanged()
{
  QTemporaryDir dir;
  const LabWeight::Weight before = run(makeSetup(dir));
  QTemporaryDir dir2;
  const QString lighter = makeSetup(dir2, true);
  QVERIFY(QFile::remove(lighter + "/Root/idle2.lsl"));
  const LabWeight::Weight now = run(lighter);
  QCOMPARE(LabWeight::compare(before, now), QStringList({
    "Before: 208 KB kept for 4 scripts. Now: 144 KB for 3 scripts.",
    "Before: 9 messages got, 6 not used. Now: 3 got, 0 not used.",
    "idle: did nothing with 3, now 0.",
    "Gone: idle2 (it kept 64 KB)."}));
  QCOMPARE(LabWeight::compare(now, before), QStringList({
    "Before: 144 KB kept for 3 scripts. Now: 208 KB for 4 scripts.",
    "Before: 3 messages got, 0 not used. Now: 9 got, 6 not used.",
    "idle: did nothing with 0, now 3.",
    "New: idle2 (64 KB kept)."}));
  QVERIFY(LabWeight::compare(before, before).isEmpty());
  QVERIFY(LabWeight::compare(LabWeight::Weight(), now).isEmpty());
}

void LabWeightTest::replayKeepsWeightAndComparesIt()
{
  QTemporaryDir dir;
  const QString top = makeSetup(dir);
  QVERIFY(writeFile(top + "/recipe.rlvtest", "wait 1\n"));
  RlvState rlv;
  LabWatcher w;
  w.setRlvState(&rlv);
  QVERIFY(!w.weight().valid);
  // watching runs the folder, and the run replays the recipe: the weight is kept, nothing to compare yet
  QVERIFY(w.watch(top, QJsonObject{{"linkset", true}}).ok());
  QVERIFY(w.weight().valid);
  QCOMPARE(w.weight().reserved(), qint64(3 * 65536 + 16384));
  QJsonObject replay = w.lastReplay();
  QCOMPARE(replay.value("weight").toObject().value("reserved").toDouble(), double(3 * 65536 + 16384));
  QCOMPARE(replay.value("weight_compared").toBool(), false);
  QCOMPARE(replay.value("weight_changes").toArray(), QJsonArray());
  // hub sends to the prim that needs it: the next save replays, and the weight says what changed
  QVERIFY(writeFile(top + "/Root/hub.lsl", "default { state_entry() {\n  integer i;\n  for (i = 1; i <= 3; ++i) llMessageLinked(2, i, \"go\", \"\");\n} }\n"));
  w.runScript("Root/hub.lsl");
  replay = w.lastReplay();
  QCOMPARE(replay.value("weight_compared").toBool(), true);
  QCOMPARE(replay.value("weight_changes").toArray(), QJsonArray({"Before: 9 messages got, 6 not used. Now: 3 got, 0 not used.",
                                                                 "idle: did nothing with 3, now 0.",
                                                                 "idle2: did nothing with 3, now 0."}));
  // the same again: nothing changed
  w.runScript("Root/hub.lsl");
  QCOMPARE(w.lastReplay().value("weight_compared").toBool(), true);
  QCOMPARE(w.lastReplay().value("weight_changes").toArray(), QJsonArray());
}

void LabWeightTest::suggestions()
{
  QTemporaryDir dir;
  const QString top = makeSetup(dir);
  const LabWeight::Weight w = run(top);
  QStringList got;
  for (const LabImprove::Suggestion& x : LabImprove::suggest(LabFolder::open(top), w))
    got << QString("%1 | %2 | %3 | %4 | %5").arg(x.kind, x.script, x.file).arg(x.line).arg(x.text);
  QCOMPARE(got, QStringList({
    "long-list | idle | Root/idle.lsl | 1 | idle: the list names is written with 20 entries. A long list fills the script's memory all the time. "
      "Keep it in Linkset Data or a notecard and read the part you need.",
    "messages | hub | Root/hub.lsl | 0 | Messages that the script getting them did nothing with: hub (3 sent, 6 not used). Send each message only "
      "where it is needed: to one prim's link number instead of LINK_SET, or to fewer scripts that have a link_message event.",
    "join |  |  | 0 | 4 scripts use less than 4 KB each for their values: hub, idle, idle2 and user. Second Life keeps 64 KB for each script, "
      "so joining some of them saves 64 KB for every script fewer.",
    "memory-limit |  |  | 0 | 3 scripts use less than 16 KB each for their values, but Second Life keeps 64 KB for each: hub, idle and idle2. "
      "llSetMemoryLimit can keep less. The memory the code itself takes is not counted here, so measure in Second Life (llGetUsedMemory) "
      "before you set a limit.",
    "unused | idle | Root/idle.lsl | 3 | idle never uses the global variable spare and the function noop. Remove what is not used: it takes "
      "memory for nothing."}));
  // a script near its limit: split it; one small script alone: no join, the memory limit names it
  LabWeight::Weight full;
  full.valid = true;
  LabWeight::Script big;
  big.name = "big";
  big.file = "big.lsl";
  big.limit = 32768;
  big.used = 30000;
  full.scripts.append(big);
  LabWeight::Script small;
  small.name = "small";
  small.used = 100;
  full.scripts.append(small);
  QStringList texts;
  for (const LabImprove::Suggestion& x : LabImprove::suggest(LabFolder::Linkset(), full)) texts << x.text;
  QCOMPARE(texts, QStringList({"big uses 29 KB of its 32 KB for its values. Split it in two before it runs out of memory.",
                               "small uses less than 16 KB for its values, but Second Life keeps 64 KB for it. llSetMemoryLimit can keep less. "
                               "The memory the code itself takes is not counted here, so measure in Second Life (llGetUsedMemory) before you set a limit."}));
  QVERIFY(LabImprove::suggest(LabFolder::Linkset(), LabWeight::Weight()).isEmpty());
}

void LabWeightTest::canRunReports()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Odd");
  QVERIFY(writeFile(top + "/camera.lsl", "default { state_entry() { llSetCameraParams([]); llHTTPRequest(\"https://example.com\", [], \"\"); } }\n"));
  QVERIFY(writeFile(top + "/sky.lsl", "default { state_entry() { llOwnerSay(\"before\"); llGetEnvironment(ZERO_VECTOR, []); } }\n"));
  QVERIFY(writeFile(top + "/plain.lsl", "default { state_entry() { llOwnerSay(\"hi\"); } }\n"));
  QVERIFY(writeFile(top + "/gone.lsl", "default { state_entry() { llRemoveInventory(llGetScriptName()); } }\n"));
  const LabFolder::Linkset ls = LabFolder::open(top);
  const QJsonObject answer = LabFolder::run(ls, QJsonObject{{"seconds", 1}});
  QVERIFY(answer.value("ok").toBool());
  QStringList got;
  for (const LabImprove::Report& r : LabImprove::canRun(ls, answer))
    got << r.script + " | " + r.prim + " | " + LabImprove::verdictWords(r.verdict) + " | " + r.works.join(" / ") + " | " + r.missing.join(" / ");
  QCOMPARE(got, QStringList({
    "camera | Odd | Runs here, with notes. | 2 of the 2 built-in functions it calls run here. / It handled these events: state_entry. | "
      "Only recorded, nothing is shown: llSetCameraParams. / Its web requests get no answer: the sandbox never goes on the internet.",
    "gone | Odd | Runs here. | 2 of the 2 built-in functions it calls run here. / It handled these events: state_entry. / "
      "It removed itself from the object, as it does in Second Life. | ",
    "plain | Odd | Runs here. | 1 of the 1 built-in functions it calls run here. / It handled these events: state_entry. | ",
    "sky | Odd | Stops here. | 1 of the 2 built-in functions it calls run here. / It handled these events: state_entry. | "
      "Not in the sandbox yet: llGetEnvironment. The script stops if it calls one. / "
      "It stopped: llGetEnvironment is not supported yet in the Script Lab sandbox, so the script was stopped here."}));
  const QJsonObject json = LabImprove::toJson({}, LabImprove::canRun(ls, answer));
  QCOMPARE(json.value("can_run").toArray().at(3).toObject().value("verdict").toString(), QString("stops"));
  QCOMPARE(json.value("can_run").toArray().at(0).toObject().value("verdict").toString(), QString("runs_with_notes"));
  QCOMPARE(json.value("suggestions").toArray(), QJsonArray());
}

void LabWeightTest::canRunWhenAScriptDoesNotCompile()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Broken");
  QVERIFY(writeFile(top + "/good.lsl", "default { state_entry() { llOwnerSay(\"hi\"); } }\n"));
  QVERIFY(writeFile(top + "/bad.lsl", "default { state_entry() { llOwnerSay(1, 2); } }\n"));
  const LabFolder::Linkset ls = LabFolder::open(top);
  const QJsonObject answer = LabFolder::run(ls, QJsonObject{{"seconds", 1}});
  QVERIFY(!answer.value("ok").toBool());
  const QVector<LabImprove::Report> r = LabImprove::canRun(ls, answer);
  QCOMPARE(r.size(), 2);
  QCOMPARE(r[0].script, QString("bad"));
  QCOMPARE(r[0].verdict, LabImprove::Verdict::DoesNotCompile);
  QCOMPARE(r[0].missing.size(), 1);
  QVERIFY2(r[0].missing.first().startsWith("Line 1: "), qPrintable(r[0].missing.first()));
  QCOMPARE(r[1].script, QString("good"));
  QCOMPARE(r[1].verdict, LabImprove::Verdict::NotRun);
  QCOMPARE(r[1].missing.last(), QString("The object did not run: ") + answer.value("error").toString());
  QCOMPARE(LabImprove::verdictWords(LabImprove::Verdict::DoesNotCompile), QString("Does not compile."));
  QCOMPARE(LabImprove::verdictWords(LabImprove::Verdict::NotRun), QString("Did not run."));
}

void LabWeightTest::openCollar()
{
  // the touchstone: the whole collar, started and clicked
  const LabFolder::Linkset ls = LabFolder::open(QString(PINGLYMOTION_EXAMPLES) + "/improve/OpenCollar");
  const QJsonObject answer = LabFolder::run(ls, QJsonObject{{"seconds", 21}, {"inputs", QJsonArray{QJsonObject{{"at", 20}, {"type", "touch"}}}}});
  QVERIFY(answer.value("ok").toBool());
  const LabWeight::Weight w = LabWeight::measure(ls, answer.value("result").toObject().value("objects").toArray());
  QVERIFY(w.valid);
  // 23 scripts, two remove themselves at the start; oc_titler sets a limit of 40000 bytes
  QCOMPARE(w.count(), 21);
  QCOMPARE(w.limited(), 1);
  QCOMPARE(w.reserved(), qint64(20 * 65536 + 40000));
  QCOMPARE(LabWeight::summary(w).first(), QString("21 scripts. Second Life keeps 64 KB for each, less for a script with a smaller limit: 1319 KB in all."));
  // the counts come from the sandbox, not the log (which is full long before the click)
  QVERIFY(answer.value("result").toObject().value("log_truncated").toBool());
  QCOMPARE(w.sent(), qint64(148));
  QCOMPARE(w.received(), qint64(2669));
  QCOMPARE(w.unused(), qint64(2488));
  // what can be better: the long lists and text, the messages, joining and memory limits, unused globals
  QStringList kinds;
  for (const LabImprove::Suggestion& s : LabImprove::suggest(ls, w)) kinds << s.kind;
  QCOMPARE(kinds.count("long-list"), 4);
  QCOMPARE(kinds.count("long-text"), 1);
  QCOMPARE(kinds.count("messages"), 1);
  QCOMPARE(kinds.count("join"), 1);
  QCOMPARE(kinds.count("memory-limit"), 1);
  QCOMPARE(kinds.count("unused"), 18);
  // can this run here: every script runs, none stops or fails to compile; seven with notes
  int notes = 0;
  for (const LabImprove::Report& r : LabImprove::canRun(ls, answer))
  {
    QVERIFY2(r.verdict == LabImprove::Verdict::Runs || r.verdict == LabImprove::Verdict::RunsWithNotes, qPrintable(r.script));
    notes += r.verdict == LabImprove::Verdict::RunsWithNotes ? 1 : 0;
  }
  QCOMPARE(notes, 6);
}

QTEST_GUILESS_MAIN(LabWeightTest)
#include "tst_labweight.moc"
