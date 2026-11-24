/*
 * PinglyMotion - tests of the Script Lab folder watch (issue 292)
 *
 * Exact answers only, in temporary folders: a saved script is run within the
 * debounce and its log is exact; a script with an error gives the line and
 * column; saving again runs again; Stop watching stops the runs; the
 * refusals (relative path, missing folder, a file, a network path, bad run
 * options); the limits (file size, hidden folders, depth, links out of the
 * folder); a script that loops forever is stopped by the interpreter's
 * limits and the watch keeps working. Linkset mode (lane 1, job B): the
 * folder runs as one object, a saved script or notecard runs the whole
 * folder again, and every script's result has its own errors and the shared
 * run or the reason the folder did not run.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "scriptlab/labwatcher.h"
#include "scriptlab/rlvstate.h"

namespace
{
  const char* const Hello = "default\n{\n    state_entry()\n    {\n        llOwnerSay(\"hello\");\n    }\n}\n";
  const char* const Bye = "default\n{\n    state_entry()\n    {\n        llOwnerSay(\"bye\");\n    }\n}\n";
  const char* const Broken = "default { state_entry() { llOwnerSay(1); } }";

  bool writeFile(const QString& path, const QByteArray& text)
  {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  // the owner_say texts of a result's log
  QStringList ownerSays(const QJsonObject& result)
  {
    QStringList out;
    for (const QJsonValue& v : result.value("result").toObject().value("log").toArray())
      if (v.toObject().value("type").toString() == "owner_say") out << v.toObject().value("text").toString();
    return out;
  }

  // waits for file times to move on, so a second write is seen as a change
  void nextSecond() { QTest::qWait(1100); }
}

class TestLabWatcher : public QObject
{
  Q_OBJECT

  private slots:
    void refusals();
    void runOptionsAreChecked();
    void emptyFolder();
    void savedScriptRunsWithinDebounce();
    void errorHasLineAndColumn();
    void savingAgainRunsAgain();
    void unwatchStopsReloading();
    void newestScriptRunsAtWatch();
    void inputsReachTheScript();
    void tooLargeFileIsNotRead();
    void hiddenAndDeepFoldersAreSkipped();
    void linksDoNotLeaveTheFolder();
    void endlessLoopIsStopped();
    void rlvLinesReachTheSharedState();
    void linksetRunsTheWholeFolder();
    void linksetBrokenScriptStopsTheRun();
    void touchGoesToThePrim();
};

void TestLabWatcher::refusals()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(dir.isValid());
  QCOMPARE(w.watch("").error, QString("Give the folder to watch."));
  QCOMPARE(w.watch("scripts").error, QString("Give the whole path of the folder, not a relative one."));
  QCOMPARE(w.watch(dir.filePath("missing")).error, QString("The folder does not exist."));
  QVERIFY(writeFile(dir.filePath("a.lsl"), Hello));
  QCOMPARE(w.watch(dir.filePath("a.lsl")).error, QString("That is a file, not a folder. Give the folder that holds the .lsl scripts."));
  QCOMPARE(w.watch("//server/share").error, QString("Network folders cannot be watched. Use a folder on this computer."));
  QCOMPARE(w.watch("\\\\server\\share").error, QString("Network folders cannot be watched. Use a folder on this computer."));
  QCOMPARE(w.watch("/" + QString(5000, 'x')).error, QString("The folder path is too long (at most 4096 characters)."));
  QVERIFY(!w.watching());
  QCOMPARE(w.runCount(), 0);
}

void TestLabWatcher::runOptionsAreChecked()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QCOMPARE(w.watch(dir.path(), QJsonObject{{"seconds", -1}}).error,
           QString("\"seconds\" must be between 0 and 3600 (simulated seconds)."));
  QCOMPARE(w.watch(dir.path(), QJsonObject{{"speed", 2}}).error, QString("Unknown run option \"speed\". Options: seconds, inputs, keep_rlv, linkset."));
  QCOMPARE(w.watch(dir.path(), QJsonObject{{"linkset", "yes"}}).error,
           QString("\"linkset\" must be true (the folder is one object) or false (every script on its own)."));
  QCOMPARE(w.watch(dir.path(), QJsonObject{{"inputs", QJsonArray{QJsonObject{{"type", "wave"}}}}}).error,
           QString("Input 1: \"type\" must be \"chat\", \"touch\", \"dialog\", \"attach\", \"detach\", \"changed\", \"sit\", \"stand\", \"control\", \"money\", \"http_request\", "
                   "\"email\", \"remote_data\", \"collision\", \"land_collision\", \"moving\", \"at_target\", \"not_at_target\", \"step\", \"drop\" or \"experience\"."));
  QVERIFY(!w.watching());
  QVERIFY(w.watch(dir.path(), QJsonObject{{"seconds", 5}}).ok());
  QCOMPARE(w.runOptions().value("seconds").toInt(), 5);
}

void TestLabWatcher::emptyFolder()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QSignalSpy changed(&w, &LabWatcher::watchChanged);
  QVERIFY(w.watch(dir.path()).ok());
  QVERIFY(w.watching());
  QCOMPARE(w.folder(), QFileInfo(dir.path()).canonicalFilePath());
  QCOMPARE(w.scripts(), QStringList());
  QCOMPARE(w.runCount(), 0);
  QVERIFY(w.lastResult().isEmpty());
  QCOMPARE(changed.count(), 1);
}

void TestLabWatcher::savedScriptRunsWithinDebounce()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(w.watch(dir.path()).ok());
  QVERIFY(writeFile(dir.filePath("hello.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("notes.txt"), "not a script"));
  // the debounce is 300 ms; give the file system watcher some slack
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 1, 10000);
  QCOMPARE(w.scripts(), QStringList{"hello.lsl"});
  const QJsonObject last = w.lastResult();
  QCOMPARE(last.value("file").toString(), QString("hello.lsl"));
  QCOMPARE(last.value("run").toInt(), 1);
  QCOMPARE(last.value("valid").toBool(), true);
  QCOMPARE(last.value("errors").toArray().size(), 0);
  const QJsonObject run = last.value("result").toObject();
  QCOMPARE(run.value("ok").toBool(), true);
  QCOMPARE(run.value("stopped").toBool(), false);
  QCOMPARE(run.value("state").toString(), QString("default"));
  QCOMPARE(ownerSays(last), QStringList{"hello"});
  QCOMPARE(w.result("hello.lsl"), last);
  QCOMPARE(w.absolutePath("hello.lsl"), QDir(w.folder()).filePath("hello.lsl"));
  QCOMPARE(w.absolutePath("notes.txt"), QString());
  // no more runs without a change
  QTest::qWait(800);
  QCOMPARE(w.runCount(), 1);
}

void TestLabWatcher::errorHasLineAndColumn()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(w.watch(dir.path()).ok());
  QVERIFY(writeFile(dir.filePath("broken.lsl"), Broken));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 1, 10000);
  const QJsonObject last = w.lastResult();
  QCOMPARE(last.value("valid").toBool(), false);
  QVERIFY(!last.contains("result"));   // not run
  const QJsonArray errors = last.value("errors").toArray();
  QCOMPARE(errors.size(), 1);
  QCOMPARE(errors[0].toObject().value("line").toInt(), 1);
  QCOMPARE(errors[0].toObject().value("column").toInt(), 38);
  QCOMPARE(errors[0].toObject().value("message").toString(), QString("Argument 1 of llOwnerSay must be a string, not an integer."));
}

void TestLabWatcher::savingAgainRunsAgain()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(writeFile(dir.filePath("s.lsl"), Hello));
  QVERIFY(w.watch(dir.path()).ok());
  QCOMPARE(w.runCount(), 1);   // the newest script runs at once
  nextSecond();
  QVERIFY(writeFile(dir.filePath("s.lsl"), Bye));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 2, 10000);
  QCOMPARE(ownerSays(w.lastResult()), QStringList{"bye"});
  QCOMPARE(w.lastResult().value("run").toInt(), 2);
  // an editor that saves by writing a new file and renaming it over the old one
  nextSecond();
  QVERIFY(writeFile(dir.filePath("s.lsl.tmp"), Hello));
  QFile::remove(dir.filePath("s.lsl"));
  QVERIFY(QFile::rename(dir.filePath("s.lsl.tmp"), dir.filePath("s.lsl")));
  QTRY_COMPARE_WITH_TIMEOUT(ownerSays(w.lastResult()), QStringList{"hello"}, 10000);
  QCOMPARE(w.scripts(), QStringList{"s.lsl"});
}

void TestLabWatcher::unwatchStopsReloading()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(writeFile(dir.filePath("s.lsl"), Hello));
  QVERIFY(w.watch(dir.path()).ok());
  QCOMPARE(w.runCount(), 1);
  QVERIFY(w.unwatch());
  QVERIFY(!w.unwatch());
  QVERIFY(!w.watching());
  QCOMPARE(w.folder(), QString());
  QVERIFY(w.lastResult().isEmpty());
  nextSecond();
  QVERIFY(writeFile(dir.filePath("s.lsl"), Bye));
  QVERIFY(writeFile(dir.filePath("t.lsl"), Bye));
  QTest::qWait(1500);
  QCOMPARE(w.runCount(), 1);
}

void TestLabWatcher::newestScriptRunsAtWatch()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(writeFile(dir.filePath("old.lsl"), Hello));
  nextSecond();
  QVERIFY(writeFile(dir.filePath("new.lsl"), Bye));
  QVERIFY(w.watch(dir.path()).ok());
  QCOMPARE(w.scripts(), (QStringList{"new.lsl", "old.lsl"}));
  QCOMPARE(w.runCount(), 1);
  QCOMPARE(w.lastResult().value("file").toString(), QString("new.lsl"));
  QVERIFY(w.result("old.lsl").isEmpty());
}

void TestLabWatcher::inputsReachTheScript()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(writeFile(dir.filePath("echo.lsl"),
                    "default { state_entry() { llListen(7, \"\", NULL_KEY, \"\"); }\n"
                    "  listen(integer c, string n, key id, string m) { llOwnerSay(\"heard \" + m); } }"));
  const QJsonObject options{{"seconds", 3}, {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"channel", 7}, {"text", "dance"}}}}};
  QVERIFY(w.watch(dir.path(), options).ok());
  QCOMPARE(ownerSays(w.lastResult()), QStringList{"heard dance"});
  QCOMPARE(w.lastResult().value("result").toObject().value("time").toDouble(), 3.0);
}

void TestLabWatcher::tooLargeFileIsNotRead()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QByteArray big = QByteArray(Hello) + "\n// " + QByteArray(LabWatcher::MAX_FILE_BYTES, 'x') + "\n";
  QVERIFY(writeFile(dir.filePath("big.lsl"), big));
  QVERIFY(w.watch(dir.path()).ok());
  const QJsonObject last = w.lastResult();
  QCOMPARE(last.value("file").toString(), QString("big.lsl"));
  QCOMPARE(last.value("valid").toBool(), false);
  QCOMPARE(last.value("read_error").toString(), QString("The file is larger than 262144 bytes, so it was not read."));
  QVERIFY(!last.contains("result"));
}

void TestLabWatcher::hiddenAndDeepFoldersAreSkipped()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QDir root(dir.path());
  QVERIFY(root.mkpath(".git/hooks"));
  QVERIFY(writeFile(dir.filePath(".git/hooks/x.lsl"), Hello));
  QVERIFY(root.mkpath("a/b/c/d/e"));
  QVERIFY(writeFile(dir.filePath("a/one.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("a/b/c/d/four.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("a/b/c/d/e/five.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("UPPER.LSL"), Hello));
  QVERIFY(writeFile(dir.filePath(".hidden.lsl"), Hello));
  QVERIFY(w.watch(dir.path()).ok());
  QCOMPARE(w.scripts(), (QStringList{"UPPER.LSL", "a/b/c/d/four.lsl", "a/one.lsl"}));
  QCOMPARE(w.scanTruncated(), false);
  // a script saved in a subfolder is run too
  QVERIFY(writeFile(dir.filePath("a/b/two.lsl"), Bye));
  QTRY_COMPARE_WITH_TIMEOUT(w.lastResult().value("file").toString(), QString("a/b/two.lsl"), 10000);
}

void TestLabWatcher::linksDoNotLeaveTheFolder()
{
#ifdef Q_OS_WIN
  QSKIP("QFile::link makes .lnk shortcuts on Windows, not file system links.");
#endif
  LabWatcher w;
  QTemporaryDir outside;
  QTemporaryDir dir;
  QVERIFY(writeFile(outside.filePath("secret.lsl"), Hello));
  QVERIFY(QDir(outside.path()).mkpath("more"));
  QVERIFY(writeFile(outside.filePath("more/other.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("inside.lsl"), Hello));
  if (!QFile::link(outside.filePath("secret.lsl"), dir.filePath("link_out.lsl")))
    QSKIP("This system does not let the test make links.");
  QVERIFY(QFile::link(outside.filePath("more"), dir.filePath("linked_folder")));
  QVERIFY(QFile::link(dir.filePath("inside.lsl"), dir.filePath("link_in.lsl")));
  QVERIFY(QDir(dir.path()).mkpath("sub"));
  QVERIFY(QFile::link(dir.path(), dir.filePath("sub/loop")));
  QVERIFY(w.watch(dir.path()).ok());
  // a link to a file inside counts; a link out of the folder and linked folders do not
  QCOMPARE(w.scripts(), (QStringList{"inside.lsl", "link_in.lsl"}));
}

void TestLabWatcher::endlessLoopIsStopped()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(writeFile(dir.filePath("loop.lsl"), "default { state_entry() { while (TRUE) { } } }"));
  QElapsedTimer clock;
  clock.start();
  QVERIFY(w.watch(dir.path()).ok());
  QVERIFY(clock.elapsed() < 5000);
  const QJsonObject run = w.lastResult().value("result").toObject();
  QCOMPARE(run.value("stopped").toBool(), true);
  QVERIFY2(run.value("stop_reason").toString().contains("steps") || run.value("stop_reason").toString().contains("real time"),
           qPrintable(run.value("stop_reason").toString()));
  // the watch still works after it
  nextSecond();
  QVERIFY(writeFile(dir.filePath("loop.lsl"), Hello));
  QTRY_COMPARE_WITH_TIMEOUT(ownerSays(w.lastResult()), QStringList{"hello"}, 10000);
}

// llOwnerSay("@...") in a watched script goes to the RLV state the panel shows; each run starts empty
void TestLabWatcher::rlvLinesReachTheSharedState()
{
  LabWatcher w;
  RlvState rlv;
  w.setRlvState(&rlv);
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("lock.lsl"), "default { state_entry() { llOwnerSay(\"@detach=n,sendim=n\"); } }"));
  QVERIFY(w.watch(dir.path()).ok());
  QVERIFY(rlv.has("detach"));
  QVERIFY(rlv.has("sendim"));
  const QJsonObject state = w.lastResult().value("result").toObject().value("rlv").toObject().value("state").toObject();
  QCOMPARE(state.value("blocked").toArray(), (QJsonArray{"detach", "sendim"}));
  // the next run starts from an empty state
  nextSecond();
  QVERIFY(writeFile(dir.filePath("lock.lsl"), "default { state_entry() { llOwnerSay(\"@detach=n\"); } }"));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 2, 10000);
  QVERIFY(rlv.has("detach"));
  QVERIFY(!rlv.has("sendim"));
  // keep_rlv is checked like lsl_run checks it
  QCOMPARE(w.watch(dir.path(), QJsonObject{{"keep_rlv", "yes"}}).error, QString("\"keep_rlv\" must be true or false."));
}

// lane 1, job B: "linkset": true makes the folder one object; every change runs all its scripts together
void TestLabWatcher::linksetRunsTheWholeFolder()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(writeFile(dir.filePath("main.lsl"),
                    "default { state_entry() { llMessageLinked(LINK_ALL_OTHERS, 0, \"hello\", NULL_KEY); } }"));
  QVERIFY(QDir(dir.path()).mkpath("Arm"));
  QVERIFY(writeFile(dir.filePath("Arm/arm.lsl"),
                    "key q;\ndefault { link_message(integer s, integer n, string t, key k) { llOwnerSay(\"arm \" + t); q = llGetNotecardLine(\"words\", 0); }\n"
                    "  dataserver(key id, string data) { llOwnerSay(\"note \" + data); } }"));
  QVERIFY(writeFile(dir.filePath("Arm/words.txt"), "one\n"));
  QVERIFY(w.watch(dir.path(), QJsonObject{{"linkset", true}, {"seconds", 2}}).ok());
  QCOMPARE(w.linkset(), true);
  QVERIFY(!w.runOptions().contains("linkset"));
  QCOMPARE(w.scripts(), QStringList({"Arm/arm.lsl", "main.lsl"}));
  QCOMPARE(w.runCount(), 1);   // one run of the whole folder
  QCOMPARE(ownerSays(w.result("Arm/arm.lsl")), QStringList({"arm hello", "note one"}));
  QCOMPARE(w.result("main.lsl").value("result"), w.result("Arm/arm.lsl").value("result"));
  QCOMPARE(w.result("Arm/arm.lsl").value("prim").toString(), QString("Arm"));
  QCOMPARE(w.result("Arm/arm.lsl").value("link").toInt(), 2);
  QCOMPARE(w.result("Arm/arm.lsl").value("script").toString(), QString("arm"));
  QCOMPARE(w.result("Arm/arm.lsl").value("linkset").toBool(), true);
  QCOMPARE(w.folderResult().value("ok").toBool(), true);
  QCOMPARE(w.folderResult().value("run").toInt(), 1);

  // saving one script runs the whole folder again, once
  nextSecond();
  QVERIFY(writeFile(dir.filePath("main.lsl"),
                    "default { state_entry() { llMessageLinked(LINK_ALL_OTHERS, 0, \"again\", NULL_KEY); } }"));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 2, 10000);
  QCOMPARE(w.lastResult().value("file").toString(), QString("main.lsl"));
  QCOMPARE(ownerSays(w.lastResult()), QStringList({"arm again", "note one"}));
  QCOMPARE(w.folderResult().value("file").toString(), QString("main.lsl"));

  // saving a notecard runs it again too
  nextSecond();
  QVERIFY(writeFile(dir.filePath("Arm/words.txt"), "two\n"));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 3, 10000);
  QCOMPARE(w.folderResult().value("file").toString(), QString("Arm/words.txt"));
  QCOMPARE(ownerSays(w.result("Arm/arm.lsl")), QStringList({"arm again", "note two"}));
  QTest::qWait(1000);
  QCOMPARE(w.runCount(), 3);   // nothing else changed

  QVERIFY(w.unwatch());
  QCOMPARE(w.linkset(), false);
  QCOMPARE(w.folderResult(), QJsonObject());
}

void TestLabWatcher::linksetBrokenScriptStopsTheRun()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(writeFile(dir.filePath("good.lsl"), Hello));
  QVERIFY(QDir(dir.path()).mkpath("Lid/Deeper"));
  QVERIFY(writeFile(dir.filePath("Lid/bad.lsl"), Broken));
  QVERIFY(writeFile(dir.filePath("Lid/Deeper/lost.lsl"), Hello));
  QVERIFY(w.watch(dir.path(), QJsonObject{{"linkset", true}}).ok());
  QCOMPARE(w.runCount(), 1);
  const QString notRun = "The folder was not run: 1 script(s) do not compile (Lid/bad.lsl). Every error names its script and prim.";
  const QJsonObject bad = w.result("Lid/bad.lsl");
  QCOMPARE(bad.value("valid").toBool(), false);
  QCOMPARE(bad.value("errors").toArray().size(), 1);
  QCOMPARE(bad.value("errors").toArray().at(0).toObject().value("prim").toString(), QString("Lid"));
  QCOMPARE(bad.value("errors").toArray().at(0).toObject().value("column").toInt(), 38);
  QCOMPARE(bad.value("not_run").toString(), notRun);
  QVERIFY(!bad.contains("result"));
  const QJsonObject good = w.result("good.lsl");
  QCOMPARE(good.value("valid").toBool(), true);
  QCOMPARE(good.value("not_run").toString(), notRun);
  QVERIFY(!good.contains("result"));
  // a script below a prim's folder is not part of the object, and says why
  const QJsonObject lost = w.result("Lid/Deeper/lost.lsl");
  QCOMPARE(lost.value("valid").toBool(), false);
  QCOMPARE(lost.value("read_error").toString(), QString("\"Lid/Deeper\" is a folder inside a prim's folder. Prims are not nested, so it was not read."));
  QCOMPARE(w.folderResult().value("error").toString(), notRun);

  // fixing the script runs the folder
  nextSecond();
  QVERIFY(writeFile(dir.filePath("Lid/bad.lsl"), Bye));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 2, 10000);
  QCOMPARE(ownerSays(w.lastResult()), QStringList({"hello", "bye"}));
  QCOMPARE(w.lastResult().value("file").toString(), QString("Lid/bad.lsl"));
}

// job A1: a click on a prop sends a touch to the scripts in that prim of the last folder run; the world
// is kept between clicks, and the log says which prop was clicked
void TestLabWatcher::touchGoesToThePrim()
{
  QTemporaryDir dir;   // made first, so the watch ends before the folder is removed
  LabWatcher w;
  QVERIFY(QDir(dir.path()).mkpath("Root"));
  QVERIFY(QDir(dir.path()).mkpath("Lid"));
  QVERIFY(writeFile(dir.filePath("Root/main.lsl"), "default { touch_start(integer n) { llOwnerSay(\"root touched\"); } }"));
  QVERIFY(writeFile(dir.filePath("Lid/lid.lsl"),
                    "integer count;\ndefault { touch_start(integer n) { count++; llOwnerSay(\"lid touched \" + (string)count); } }"));
  QCOMPARE(w.touch(2).error, QString("No folder is watched as one object."));
  QVERIFY(w.watch(dir.path(), QJsonObject{{"linkset", true}}).ok());
  QCOMPARE(w.linkOf("Root"), 1);
  QCOMPARE(w.linkOf("Lid"), 2);
  QCOMPARE(w.linkOf("Nope"), 0);
  QVERIFY(ownerSays(w.lastResult()).isEmpty());

  QSignalSpy changed(&w, &LabWatcher::resultChanged);
  QVERIFY(w.touch(2).ok());
  QCOMPARE(changed.count(), 1);
  QCOMPARE(ownerSays(w.lastResult()), QStringList({"lid touched 1"}));
  // the same log in every script's result and in the folder's answer
  QCOMPARE(w.result("Root/main.lsl").value("result"), w.folderResult().value("result"));
  QCOMPARE(w.result("Lid/lid.lsl").value("result"), w.folderResult().value("result"));
  const QJsonArray log = w.folderResult().value("result").toObject().value("log").toArray();
  int clicked = -1, said = -1;
  for (int i = 0; i < log.size(); ++i)
  {
    const QJsonObject e = log[i].toObject();
    if (e.value("type").toString() == "clicked")
    {
      clicked = i;
      QCOMPARE(e.value("prim").toString(), QString("Lid"));
      QCOMPARE(e.value("link").toInt(), 2);
      QCOMPARE(e.value("source").toString(), QString("sandbox"));
    }
    if (e.value("type").toString() == "owner_say")
    {
      said = i;
      QCOMPARE(e.value("script").toString(), QString("lid"));
      QCOMPARE(e.value("prim").toString(), QString("Lid"));
      QCOMPARE(e.value("file").toString(), QString("Lid/lid.lsl"));
    }
  }
  QVERIFY(clicked >= 0);
  QVERIFY(said > clicked);

  // the world goes on: the second click counts on, a click on the root reaches the root's script only
  QVERIFY(w.touch(2).ok());
  QCOMPARE(ownerSays(w.lastResult()), QStringList({"lid touched 1", "lid touched 2"}));
  QVERIFY(w.touch(1).ok());
  QCOMPARE(ownerSays(w.lastResult()), QStringList({"lid touched 1", "lid touched 2", "root touched"}));
  QCOMPARE(w.touch(9).error, QString("The object has no prim with link 9."));
  QCOMPARE(w.touch(0).error, QString("The object has no prim with link 0."));

  // a save runs the folder again from the start
  nextSecond();
  QVERIFY(writeFile(dir.filePath("Lid/lid.lsl"),
                    "integer count;\ndefault { touch_start(integer n) { count++; llOwnerSay(\"lid again \" + (string)count); } }"));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 2, 10000);
  QVERIFY(ownerSays(w.lastResult()).isEmpty());
  QVERIFY(w.touch(2).ok());
  QCOMPARE(ownerSays(w.lastResult()), QStringList({"lid again 1"}));

  // a folder that does not compile cannot be touched
  nextSecond();
  QVERIFY(writeFile(dir.filePath("Lid/lid.lsl"), "default { touch_start(integer n) { llOwnerSay(\"x\") } }"));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 3, 10000);
  QCOMPARE(w.touch(2).error, QString("The object did not run, so it cannot be touched. Fix it and save."));

  QVERIFY(w.unwatch());
  QCOMPARE(w.touch(2).error, QString("No folder is watched as one object."));
}

QTEST_GUILESS_MAIN(TestLabWatcher)
#include "tst_labwatcher.moc"
