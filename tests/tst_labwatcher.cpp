/*
 * PinglyMotion - tests of the Script Lab folder watch (issue 292)
 *
 * Exact answers only, in temporary folders: a saved script is run within the
 * debounce and its log is exact; a script with an error gives the line and
 * column; saving again runs again; Stop watching stops the runs; the
 * refusals (relative path, missing folder, a file, a network path, bad run
 * options); the limits (file size, hidden folders, depth, links out of the
 * folder); a script that loops forever is stopped by the interpreter's
 * limits and the watch keeps working.
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
};

void TestLabWatcher::refusals()
{
  LabWatcher w;
  QTemporaryDir dir;
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
  LabWatcher w;
  QTemporaryDir dir;
  QCOMPARE(w.watch(dir.path(), QJsonObject{{"seconds", -1}}).error,
           QString("\"seconds\" must be between 0 and 3600 (simulated seconds)."));
  QCOMPARE(w.watch(dir.path(), QJsonObject{{"speed", 2}}).error, QString("Unknown run option \"speed\". Options: seconds, inputs."));
  QCOMPARE(w.watch(dir.path(), QJsonObject{{"inputs", QJsonArray{QJsonObject{{"type", "wave"}}}}}).error,
           QString("Input 1: \"type\" must be \"chat\" or \"touch\"."));
  QVERIFY(!w.watching());
  QVERIFY(w.watch(dir.path(), QJsonObject{{"seconds", 5}}).ok());
  QCOMPARE(w.runOptions().value("seconds").toInt(), 5);
}

void TestLabWatcher::emptyFolder()
{
  LabWatcher w;
  QTemporaryDir dir;
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
  LabWatcher w;
  QTemporaryDir dir;
  QVERIFY(w.watch(dir.path()).ok());
  QVERIFY(writeFile(dir.filePath("hello.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("notes.txt"), "not a script"));
  // the debounce is 300 ms; give the file system watcher some slack
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 1, 5000);
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
  LabWatcher w;
  QTemporaryDir dir;
  QVERIFY(w.watch(dir.path()).ok());
  QVERIFY(writeFile(dir.filePath("broken.lsl"), Broken));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 1, 5000);
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
  LabWatcher w;
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("s.lsl"), Hello));
  QVERIFY(w.watch(dir.path()).ok());
  QCOMPARE(w.runCount(), 1);   // the newest script runs at once
  nextSecond();
  QVERIFY(writeFile(dir.filePath("s.lsl"), Bye));
  QTRY_COMPARE_WITH_TIMEOUT(w.runCount(), 2, 5000);
  QCOMPARE(ownerSays(w.lastResult()), QStringList{"bye"});
  QCOMPARE(w.lastResult().value("run").toInt(), 2);
  // an editor that saves by writing a new file and renaming it over the old one
  nextSecond();
  QVERIFY(writeFile(dir.filePath("s.lsl.tmp"), Hello));
  QFile::remove(dir.filePath("s.lsl"));
  QVERIFY(QFile::rename(dir.filePath("s.lsl.tmp"), dir.filePath("s.lsl")));
  QTRY_COMPARE_WITH_TIMEOUT(ownerSays(w.lastResult()), QStringList{"hello"}, 5000);
  QCOMPARE(w.scripts(), QStringList{"s.lsl"});
}

void TestLabWatcher::unwatchStopsReloading()
{
  LabWatcher w;
  QTemporaryDir dir;
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
  LabWatcher w;
  QTemporaryDir dir;
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
  LabWatcher w;
  QTemporaryDir dir;
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
  LabWatcher w;
  QTemporaryDir dir;
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
  LabWatcher w;
  QTemporaryDir dir;
  QDir root(dir.path());
  QVERIFY(root.mkpath(".git/hooks"));
  QVERIFY(writeFile(dir.filePath(".git/hooks/x.lsl"), Hello));
  QVERIFY(root.mkpath("a/b/c/d/e"));
  QVERIFY(writeFile(dir.filePath("a/one.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("a/b/c/d/four.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("a/b/c/d/e/five.lsl"), Hello));
  QVERIFY(writeFile(dir.filePath("UPPER.LSL"), Hello));
  QVERIFY(w.watch(dir.path()).ok());
  QCOMPARE(w.scripts(), (QStringList{"UPPER.LSL", "a/b/c/d/four.lsl", "a/one.lsl"}));
  QCOMPARE(w.scanTruncated(), false);
  // a script saved in a subfolder is run too
  QVERIFY(writeFile(dir.filePath("a/b/two.lsl"), Bye));
  QTRY_COMPARE_WITH_TIMEOUT(w.lastResult().value("file").toString(), QString("a/b/two.lsl"), 5000);
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
  LabWatcher w;
  QTemporaryDir dir;
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
  QTRY_COMPARE_WITH_TIMEOUT(ownerSays(w.lastResult()), QStringList{"hello"}, 5000);
}

QTEST_GUILESS_MAIN(TestLabWatcher)
#include "tst_labwatcher.moc"
