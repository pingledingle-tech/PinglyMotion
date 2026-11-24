/*
 * PinglyMotion Script Lab - record and replay (job A3): exact tests
 *
 * The recipe format (parse, write, refusals), recording clicks, menu answers and typed RLV lines
 * through the watcher, the replay's facts, the plain-words comparison after a changed script, and
 * the messages when there is no recipe.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "scriptlab/labrecipe.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/rlvstate.h"

namespace
{
  bool writeFile(const QString& path, const QString& text)
  {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return f.write(text.toUtf8()) >= 0;
  }

  QString readFile(const QString& path)
  {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(f.readAll());
  }

  const char* Hud =
    "default {\n"
    "  state_entry() { llListen(-77, \"\", llGetOwner(), \"\"); llOwnerSay(\"Ready.\"); }\n"
    "  touch_start(integer n) { llDialog(llDetectedKey(0), \"Cuffs\", [\"Lock\", \"Free\"], -77); }\n"
    "  listen(integer c, string name, key id, string m) { llMessageLinked(LINK_ALL_OTHERS, 1, m, NULL_KEY); }\n"
    "}\n";

  QString cuff(bool lock)
  {
    return QString("default {\n"
                   "  state_entry() { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); }\n"
                   "  link_message(integer s, integer n, string m, key id) {\n"
                   "    if (m == \"Lock\") { llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES); %1llOwnerSay(\"Locked.\"); }\n"
                   "    else { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); llOwnerSay(\"@detach=y\"); llOwnerSay(\"Free.\"); }\n"
                   "  }\n"
                   "}\n").arg(lock ? "llOwnerSay(\"@detach=n\"); " : "");
  }

  // the object "Cuffs": a HUD in the root that shows a menu, and a cuff that locks
  QString makeCuffs(const QTemporaryDir& dir, bool lock = true)
  {
    const QString object = dir.filePath("Cuffs");
    QDir().mkpath(object + "/Root");
    QDir().mkpath(object + "/Cuff");
    writeFile(object + "/Root/hud.lsl", Hud);
    writeFile(object + "/Cuff/cuff.lsl", cuff(lock));
    return object;
  }

  const QString RecipeText =
    "# PinglyMotion recipe: what you did, one step per line.\n"
    "# The program replays it after every save of a script.\n"
    "# Steps: touch <part>, menu <button>, send <RLV line>, wait <seconds>.\n"
    "# \"object <name>\" says who sends the next RLV lines. Lines with # are notes.\n"
    "touch Root\n"
    "menu Lock\n"
    "send @sendim=n\n";

  const QStringList CuffFacts = {
    "Cuffs is locked.",
    "Send instant messages is blocked.",
    "Cuff is red.",
    "Root is saying \"Ready.\".",
    "Cuff is saying \"Locked.\".",
  };

  QStringList strings(const QJsonValue& v)
  {
    QStringList out;
    for (const QJsonValue& s : v.toArray()) out << s.toString();
    return out;
  }
}

class TestLabRecipe : public QObject
{
  Q_OBJECT

  private slots:
    void parseAndWrite();
    void parseRefusals();
    void stepWords();
    void colourNames();
    void recordClicksMenuAndLines();
    void replayAfterAChangedScript();
    void replayIsTheSameEveryTime();
    void stepThatCannotBeDone();
    void noRecipe();
    void compareWords();
    void resultFileRoundTrip();
    void objectThatDoesNotRun();
};

void TestLabRecipe::parseAndWrite()
{
  const LabRecipe::Result r = LabRecipe::parse("# a note\n\ntouch Collar\r\nmenu Lock all\nobject cuffs\nsend @detach=n\nwait 2.5\nsend @clear\n");
  QVERIFY2(r.ok(), qPrintable(r.error));
  QCOMPARE(r.value->steps.size(), 5);
  QCOMPARE(int(r.value->steps[0].kind), int(LabRecipe::Step::Kind::Touch));
  QCOMPARE(r.value->steps[0].text, QString("Collar"));
  QCOMPARE(r.value->steps[0].line, 3);
  QCOMPARE(r.value->steps[1].text, QString("Lock all"));
  QCOMPARE(r.value->steps[2].object, QString("cuffs"));
  QCOMPARE(r.value->steps[2].text, QString("@detach=n"));
  QCOMPARE(r.value->steps[3].seconds, 2.5);
  QCOMPARE(r.value->steps[4].object, QString("cuffs"));
  QCOMPARE(LabRecipe::write(*r.value),
           QString("# PinglyMotion recipe: what you did, one step per line.\n"
                   "# The program replays it after every save of a script.\n"
                   "# Steps: touch <part>, menu <button>, send <RLV line>, wait <seconds>.\n"
                   "# \"object <name>\" says who sends the next RLV lines. Lines with # are notes.\n"
                   "touch Collar\n"
                   "menu Lock all\n"
                   "object cuffs\n"
                   "send @detach=n\n"
                   "wait 2.5\n"
                   "send @clear\n"));
  // write then parse gives the same steps
  const LabRecipe::Result again = LabRecipe::parse(LabRecipe::write(*r.value));
  QVERIFY(again.ok());
  QCOMPARE(again.value->steps, r.value->steps);
  // a send without "object" comes from panel
  QCOMPARE(LabRecipe::parse("send @sendim=n").value->steps[0].object, QString("panel"));
}

void TestLabRecipe::parseRefusals()
{
  QCOMPARE(LabRecipe::parse("touch Lid\njump high\n").error, QString("Line 2: unknown word \"jump\". Use touch, menu, send, object or wait."));
  QCOMPARE(LabRecipe::parse("expect blocked detach").error, QString("Line 1: \"expect\" lines are not built yet. Remove the line."));
  QCOMPARE(LabRecipe::parse("touch").error, QString("Line 1: \"touch\" needs the name of a part, at most 63 characters."));
  QCOMPARE(LabRecipe::parse("touch " + QString(64, 'x')).error, QString("Line 1: \"touch\" needs the name of a part, at most 63 characters."));
  QCOMPARE(LabRecipe::parse("menu").error, QString("Line 1: \"menu\" needs the button's text, at most 512 characters."));
  QCOMPARE(LabRecipe::parse("send detach=n").error, QString("Line 1: \"send\" needs an RLV line that starts with @."));
  QCOMPARE(LabRecipe::parse("wait 0").error, QString("Line 1: \"wait\" needs seconds from 0.1 to 60."));
  QCOMPARE(LabRecipe::parse("wait 61").error, QString("Line 1: \"wait\" needs seconds from 0.1 to 60."));
  QCOMPARE(LabRecipe::parse("wait nan").error, QString("Line 1: \"wait\" needs seconds from 0.1 to 60."));
  QCOMPARE(LabRecipe::parse("object bad/name").error, QString("Line 1: \"object\" needs a name of 1 to 64 letters, digits, spaces, dots, - or _."));
  QCOMPARE(LabRecipe::parse("touch " + QString(1100, 'x')).error, QString("Line 1 is longer than 1024 characters."));
  QString many;
  for (int i = 0; i < 201; ++i) many += "touch Lid\n";
  QCOMPARE(LabRecipe::parse(many).error, QString("The recipe has more than 200 steps."));
  QCOMPARE(LabRecipe::parse(QString(70000, '#')).error, QString("The recipe is larger than 65536 characters."));
  // an empty recipe is fine: it has no steps
  QVERIFY(LabRecipe::parse("").ok());
  QVERIFY(LabRecipe::parse("").value->steps.isEmpty());
}

void TestLabRecipe::stepWords()
{
  const LabRecipe::Recipe r = *LabRecipe::parse("touch Collar\nmenu Lock all\nobject cuffs\nsend @detach=n\nwait 2\n").value;
  QCOMPARE(LabRecipe::stepWords(r.steps[0]), QString("Click Collar."));
  QCOMPARE(LabRecipe::stepWords(r.steps[1]), QString("Press \"Lock all\" in the menu."));
  QCOMPARE(LabRecipe::stepWords(r.steps[2]), QString("cuffs sends @detach=n."));
  QCOMPARE(LabRecipe::stepWords(r.steps[3]), QString("Wait 2 seconds."));
  QCOMPARE(LabRecipe::stepLine(r.steps[3]), QString("wait 2"));
}

void TestLabRecipe::colourNames()
{
  QCOMPARE(LabRecipe::colourName(1, 0, 0), QString("red"));
  QCOMPARE(LabRecipe::colourName(0, 1, 0), QString("green"));
  QCOMPARE(LabRecipe::colourName(0.9, 0.1, 0.05), QString("red"));
  QCOMPARE(LabRecipe::colourName(1, 1, 1), QString("white"));
  QCOMPARE(LabRecipe::colourName(0.45, 0.5, 0.55), QString("grey"));
  QCOMPARE(LabRecipe::colourName(1, 0.55, 0), QString("orange"));
  QCOMPARE(LabRecipe::colourName(0.5, 0.75, 1), QString("light blue"));
}

void TestLabRecipe::recordClicksMenuAndLines()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  const QString object = makeCuffs(dir);
  RlvState rlv;
  LabWatcher w;
  w.setRlvState(&rlv);
  QCOMPARE(w.startRecording().error, QString("No folder is watched as one object."));
  QVERIFY(w.watch(object, QJsonObject{{"linkset", true}}).ok());
  QVERIFY(!w.hasRecipe());
  QVERIFY(w.lastReplay().isEmpty());
  QSignalSpy changed(&w, &LabWatcher::recipeChanged);

  QVERIFY(w.startRecording().ok());
  QVERIFY(w.recording());
  // starting writes nothing yet: the old recipe (none here) stays until the first step
  QVERIFY(!w.hasRecipe());
  QVERIFY(w.touch(w.linkOf("Root")).ok());
  QVERIFY(w.hasRecipe());
  QVERIFY(w.answerDialog("Lock").ok());
  w.recordLine("panel", "@sendim=n");
  w.recordLine("panel", "not an RLV line");     // left out
  w.recordLine("bad/name", "@fly=n");           // left out: not an object name
  QCOMPARE(w.recordedSteps().size(), 3);
  QCOMPARE(readFile(object + "/recipe.rlvtest"), RecipeText);
  QVERIFY(changed.count() >= 4);

  // stopping replays the new steps once: the first result, nothing to compare with
  QVERIFY(w.stopRecording());
  QVERIFY(!w.recording());
  QVERIFY(!w.stopRecording());
  const QJsonObject replay = w.lastReplay();
  QCOMPARE(replay.value("ran").toBool(), true);
  QCOMPARE(replay.value("steps").toInt(), 3);
  QCOMPARE(replay.value("done").toInt(), 3);
  QCOMPARE(replay.value("first").toBool(), true);
  QCOMPARE(strings(replay.value("changes")), QStringList());
  QCOMPARE(strings(replay.value("facts")), CuffFacts);
  QCOMPARE(strings(replay.value("steps_text")), QStringList({"touch Root", "menu Lock", "send @sendim=n"}));
  QCOMPARE(replay.value("recipe").toString(), QString("recipe.rlvtest"));
  QCOMPARE(readFile(object + "/recipe.rlvresult"),
           QString("# PinglyMotion: what the recipe gave the last time it was replayed.\n"
                   "# The program writes this file after every replay. Do not edit it.\n"
                   "# 3 of 3 steps done.\n"
                   "[lock:Cuffs] Cuffs is locked.\n"
                   "[blocked:sendim] Send instant messages is blocked.\n"
                   "[colour:Cuff] Cuff is red.\n"
                   "[said:Root] Root is saying \"Ready.\".\n"
                   "[said:Cuff] Cuff is saying \"Locked.\".\n"
                   "# This was the first replay. There was nothing to compare with.\n"));
  // the replay has its own world and RLV state: the panel's state is what the clicks made, no more
  QVERIFY(!rlv.has("sendim"));
  QVERIFY(rlv.has("detach"));
  // the replay log marks each step
  int stepLines = 0;
  for (const QJsonValue& v : replay.value("log").toArray())
    if (v.toObject().value("type").toString() == "step") ++stepLines;
  QCOMPARE(stepLines, 3);

  // starting again and stopping without a step keeps the old recipe
  QVERIFY(w.startRecording().ok());
  QVERIFY(w.stopRecording());
  QCOMPARE(readFile(object + "/recipe.rlvtest"), RecipeText);
}

void TestLabRecipe::replayAfterAChangedScript()
{
  QTemporaryDir dir;
  const QString object = makeCuffs(dir);
  QVERIFY(writeFile(object + "/recipe.rlvtest", RecipeText));
  RlvState rlv;
  LabWatcher w;
  w.setRlvState(&rlv);
  // watching runs the folder once, and that run replays the recipe: the first time, nothing to compare
  QVERIFY(w.watch(object, QJsonObject{{"linkset", true}}).ok());
  QCOMPARE(w.lastReplay().value("first").toBool(), true);
  QCOMPARE(strings(w.lastReplay().value("facts")), CuffFacts);

  // the cuff no longer locks: a save runs the folder, the recipe is replayed and compared
  QVERIFY(writeFile(object + "/Cuff/cuff.lsl", cuff(false)));
  w.runScript("Cuff/cuff.lsl");
  const QJsonObject replay = w.lastReplay();
  QCOMPARE(replay.value("first").toBool(), false);
  QCOMPARE(replay.value("run").toInt(), 2);
  QCOMPARE(strings(replay.value("changes")), QStringList({"Before: Cuffs was locked. Now: it is not locked."}));
  QVERIFY(readFile(object + "/recipe.rlvresult").endsWith("# Changes since the time before:\n"
                                                          "# Before: Cuffs was locked. Now: it is not locked.\n"));

  // and back: the comparison is with the result of the run before
  QVERIFY(writeFile(object + "/Cuff/cuff.lsl", cuff(true)));
  w.runScript("Cuff/cuff.lsl");
  QCOMPARE(strings(w.lastReplay().value("changes")), QStringList({"Before: Cuffs was not locked. Now: it is locked."}));
}

void TestLabRecipe::replayIsTheSameEveryTime()
{
  QTemporaryDir dir;
  const QString object = makeCuffs(dir);
  QVERIFY(writeFile(object + "/recipe.rlvtest", RecipeText));
  LabWatcher w;
  QVERIFY(w.watch(object, QJsonObject{{"linkset", true}}).ok());
  const QString first = readFile(object + "/recipe.rlvresult");
  for (int i = 0; i < 3; ++i)
  {
    QVERIFY(w.replayRecipe().ok());
    QCOMPARE(w.lastReplay().value("first").toBool(), false);
    QCOMPARE(strings(w.lastReplay().value("changes")), QStringList());
    QCOMPARE(strings(w.lastReplay().value("facts")), CuffFacts);
  }
  QVERIFY(readFile(object + "/recipe.rlvresult").endsWith("# Nothing changed since the time before.\n"));
  QCOMPARE(readFile(object + "/recipe.rlvresult").section('\n', 0, 7), first.section('\n', 0, 7));
}

void TestLabRecipe::stepThatCannotBeDone()
{
  QTemporaryDir dir;
  const QString object = makeCuffs(dir);
  QVERIFY(writeFile(object + "/recipe.rlvtest", "menu Lock\ntouch Lid\ntouch Root\nmenu Free\n"));
  LabWatcher w;
  QVERIFY(w.watch(object, QJsonObject{{"linkset", true}}).ok());
  const QJsonObject replay = w.lastReplay();
  QCOMPARE(replay.value("done").toInt(), 2);
  QCOMPARE(strings(replay.value("facts")),
           QStringList({"Step 1 (menu Lock) is not done: no menu was open.",
                        "Step 2 (touch Lid) is not done: there is no part called Lid.",
                        "Cuff is green.",
                        "Root is saying \"Ready.\".",
                        "Cuff is saying \"Free.\"."}));
}

void TestLabRecipe::noRecipe()
{
  QTemporaryDir dir;
  const QString object = makeCuffs(dir);
  LabWatcher w;
  QCOMPARE(w.replayRecipe().error, QString("No folder is watched as one object."));
  QVERIFY(w.watch(object, QJsonObject{{"linkset", true}}).ok());
  QCOMPARE(w.replayRecipe().error, QString("There is no recipe yet. Record what you do first."));
  QVERIFY(writeFile(object + "/recipe.rlvtest", "# only notes\n"));
  QCOMPARE(w.replayRecipe().error, QString("The recipe has no steps. Record what you do first."));
  QVERIFY(writeFile(object + "/recipe.rlvtest", "dance\n"));
  QCOMPARE(w.replayRecipe().error, QString("The recipe has a mistake. Line 1: unknown word \"dance\". Use touch, menu, send, object or wait."));
  // a save with a broken recipe still runs the folder, and keeps no replay
  w.runScript("Cuff/cuff.lsl");
  QVERIFY(w.lastReplay().isEmpty());
  QVERIFY(!QFile::exists(object + "/recipe.rlvresult"));
  // a folder that is not one object has no recipe
  LabWatcher single;
  QVERIFY(single.watch(object + "/Cuff").ok());
  QCOMPARE(single.recipePath(), QString());
  QCOMPARE(single.replayRecipe().error, QString("No folder is watched as one object."));
}

void TestLabRecipe::compareWords()
{
  using F = LabRecipe::Fact;
  const QVector<F> before = {{"lock:Cuffs", "Cuffs", "locked"}, {"colour:Cuff", "Cuff", "red"}, {"menu", "the menu", "open: \"Cuffs\" with Lock, Free"}};
  const QVector<F> now = {{"colour:Cuff", "Cuff", "green"}, {"blocked:sendim", "Send instant messages", "blocked"}, {"said:Root", "Root", "saying \"Hi\""}};
  QCOMPARE(LabRecipe::compare(before, now),
           QStringList({"Before: Cuff was red. Now: it is green.",
                        "Before: Send instant messages was allowed. Now: it is blocked.",
                        "Before: Root was silent. Now: it is saying \"Hi\".",
                        "Before: Cuffs was locked. Now: it is not locked.",
                        "Before: the menu was open: \"Cuffs\" with Lock, Free. Now: it is closed."}));
  QCOMPARE(LabRecipe::compare(now, now), QStringList());
  // the object did not run: one line, not a list of everything that went
  const QVector<F> broken = {{"run", "the object", "not running: Cuff/cuff.lsl does not compile"}};
  QCOMPARE(LabRecipe::compare(now, broken), QStringList({"Before: the object ran. Now: it is not running: Cuff/cuff.lsl does not compile."}));
  QCOMPARE(LabRecipe::compare(broken, now), QStringList({"Before: the object did not run. Now: it runs again."}));
  QCOMPARE(LabRecipe::compare(broken, broken), QStringList());
}

void TestLabRecipe::resultFileRoundTrip()
{
  LabRecipe::Replay r;
  r.ran = true;
  r.steps = 2;
  r.done = 2;
  r.facts = {{"lock:Cuffs", "Cuffs", "locked"}, {"menu", "the menu", "open: \"Is it on?\" with Yes, No"}, {"said:Root", "Root", "saying \"This is it.\""}};
  const QString text = LabRecipe::writeResult(r, QStringList(), false);
  QCOMPARE(LabRecipe::parseResult(text), r.facts);
  QCOMPARE(LabRecipe::compare(LabRecipe::parseResult(text), r.facts), QStringList());
  // a line the program did not write is skipped
  QCOMPARE(LabRecipe::parseResult("[x]\n[] y is z.\nhello\n[lock:A] A is locked\n").size(), 0);
}

void TestLabRecipe::objectThatDoesNotRun()
{
  QTemporaryDir dir;
  const QString object = makeCuffs(dir);
  QVERIFY(writeFile(object + "/recipe.rlvtest", RecipeText));
  LabWatcher w;
  QVERIFY(w.watch(object, QJsonObject{{"linkset", true}}).ok());
  QVERIFY(writeFile(object + "/Cuff/cuff.lsl", "default { state_entry() { llSay(0, ) } }"));
  w.runScript("Cuff/cuff.lsl");
  const QJsonObject replay = w.lastReplay();
  QCOMPARE(replay.value("ran").toBool(), false);
  QCOMPARE(strings(replay.value("changes")).size(), 1);
  QVERIFY(strings(replay.value("changes")).first().startsWith("Before: the object ran. Now: it is not running: "));
}

QTEST_GUILESS_MAIN(TestLabRecipe)
#include "tst_labrecipe.moc"
