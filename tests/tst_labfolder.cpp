/*
 * PinglyMotion - tests of the Script Lab folder model (lane 1, job B: the folder is the object)
 *
 * Exact answers only, in temporary folders: the prims a folder becomes (a
 * "Root" subfolder, the top folder's own files, children in name order), the
 * order of link messages between scripts of different prims, the import
 * check of all scripts at once (every error names file, script and prim, and
 * nothing runs), the source of every log line, notecards read from the
 * folder, the limits (path rules, number of scripts, file and total size,
 * path and name length, a name used twice, nested and linked folders) and
 * endless loops in several prims, which stop with their script and prim in
 * the log and do not hold the program.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>

#include "scriptlab/labfolder.h"

namespace
{
  bool writeFile(const QString& path, const QByteArray& text)
  {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  // the log entries of a run of a given type, as [text-or-name, script, prim, file]
  QJsonArray entries(const QJsonObject& run, const QString& type, const QString& field = "text")
  {
    QJsonArray out;
    for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
    {
      const QJsonObject e = v.toObject();
      if (e.value("type").toString() == type) out.append(QJsonArray{e.value(field), e.value("script"), e.value("prim"), e.value("file")});
    }
    return out;
  }

  // [link, name, folder] of every prim
  QJsonArray prims(const LabFolder::Linkset& ls)
  {
    QJsonArray out;
    for (const LabFolder::Prim& p : ls.prims) out.append(QJsonArray{p.link, p.name, p.folder});
    return out;
  }

  // [name, file, link] of every script
  QJsonArray scripts(const LabFolder::Linkset& ls)
  {
    QJsonArray out;
    for (const LabFolder::Script& s : ls.scripts) out.append(QJsonArray{s.name, s.file, s.link});
    return out;
  }

  // [message, file, prim, link] of every problem (or note)
  QJsonArray problems(const std::vector<LabFolder::Problem>& list)
  {
    QJsonArray out;
    for (const LabFolder::Problem& p : list) out.append(QJsonArray{p.message, p.file, p.prim, p.link});
    return out;
  }

  const QByteArray Quiet = "default { state_entry() { } }";
}

class TestLabFolder : public QObject
{
  Q_OBJECT

  private slots:
    void pathRules();
    void rootFolderAndChildPrims();
    void topFolderIsTheRootPrim();
    void linkMessagesReachEveryPrim();
    void singlePrimObject();
    void allScriptsAreCheckedAtOnce();
    void everyLogLineNamesItsSource();
    void notecardsComeFromTheFolder();
    void requestShape();
    void emptyFolderDoesNotRun();
    void tooManyScripts();
    void fileAndTotalSize();
    void pathAndNameLength();
    void nameUsedTwice();
    void nestedHiddenAndLinkedFolders();
    void endlessLoopsStopWithTheirSource();
    void slowLoopsShareOneRealTimeCap();
    void autoSecondsGrowWithScripts();
    void bigSetupGetsTimeToFinish();
    void foreverTimerStopsAtTheHardLimit();
    void runawayLoopStillStopsWithAutoTime();
    void anafinRunOptionsReachTheRun();
};

void TestLabFolder::pathRules()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(writeFile(dir.filePath("a.lsl"), Quiet));
  QCOMPARE(LabFolder::open("").error, QString("Give the folder to watch."));
  QCOMPARE(LabFolder::open("scripts").error, QString("Give the whole path of the folder, not a relative one."));
  QCOMPARE(LabFolder::open(dir.filePath("missing")).error, QString("The folder does not exist."));
  QCOMPARE(LabFolder::open(dir.filePath("a.lsl")).error, QString("That is a file, not a folder. Give the folder that holds the .lsl scripts."));
  QCOMPARE(LabFolder::open("//server/share").error, QString("Network folders cannot be watched. Use a folder on this computer."));
  QCOMPARE(LabFolder::open("/" + QString(5000, 'x')).error, QString("The folder path is too long (at most 4096 characters)."));
  const QJsonObject run = LabFolder::run(LabFolder::open("scripts"));
  QCOMPARE(run.value("ok").toBool(), false);
  QCOMPARE(run.value("error").toString(), QString("Give the whole path of the folder, not a relative one."));
  QVERIFY(!run.contains("result"));
}

void TestLabFolder::rootFolderAndChildPrims()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("MyObject");
  // the maker guide's layout, with a loose script in the top folder and children out of name order on disk
  QVERIFY(writeFile(top + "/Root/main.lsl", Quiet));
  QVERIFY(writeFile(top + "/Root/settings.txt", "colour=red\r\nsize=2\n"));
  QVERIFY(writeFile(top + "/Cuff_right/cuff_r.lsl", Quiet));
  QVERIFY(writeFile(top + "/cuff_left/cuff_l.lsl", Quiet));
  QVERIFY(writeFile(top + "/helper.lsl", Quiet));
  QVERIFY(writeFile(top + "/README.md", "not a script"));
  const LabFolder::Linkset ls = LabFolder::open(top);
  QVERIFY(ls.ok());
  QCOMPARE(ls.name, QString("MyObject"));
  QCOMPARE(ls.folder, QFileInfo(top).canonicalFilePath());
  QCOMPARE(prims(ls), QJsonArray({QJsonArray{1, "Root", "Root"}, QJsonArray{2, "cuff_left", "cuff_left"}, QJsonArray{3, "Cuff_right", "Cuff_right"}}));
  QCOMPARE(scripts(ls), QJsonArray({QJsonArray{"helper", "helper.lsl", 1}, QJsonArray{"main", "Root/main.lsl", 1},
                                    QJsonArray{"cuff_l", "cuff_left/cuff_l.lsl", 2}, QJsonArray{"cuff_r", "Cuff_right/cuff_r.lsl", 3}}));
  QCOMPARE(int(ls.notecards.size()), 1);
  QCOMPARE(ls.notecards[0].name, QString("settings"));
  QCOMPARE(ls.notecards[0].file, QString("Root/settings.txt"));
  QCOMPARE(ls.notecards[0].link, 1);
  QCOMPARE(ls.notecards[0].lines, QStringList({"colour=red", "size=2"}));
  QCOMPARE(ls.primName(3), QString("Cuff_right"));
  QCOMPARE(ls.primName(4), QString());

  const QJsonObject json = ls.toJson();
  QCOMPARE(json.value("object").toString(), QString("MyObject"));
  QCOMPARE(json.value("problems").toArray(), QJsonArray());
  QCOMPARE(json.value("notes").toArray(), QJsonArray());
  QCOMPARE(json.value("prims").toArray().at(0).toObject(),
           QJsonObject({{"link", 1}, {"name", "Root"}, {"folder", "Root"},
                        {"scripts", QJsonArray{QJsonObject{{"name", "helper"}, {"file", "helper.lsl"}}, QJsonObject{{"name", "main"}, {"file", "Root/main.lsl"}}}},
                        {"notecards", QJsonArray{QJsonObject{{"name", "settings"}, {"file", "Root/settings.txt"}, {"lines", 2}}}}}));
}

void TestLabFolder::topFolderIsTheRootPrim()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Collar");
  QVERIFY(writeFile(top + "/core.lsl", Quiet));
  QVERIFY(writeFile(top + "/Bell/bell.lsl", Quiet));
  QVERIFY(writeFile(top + "/Bell/sounds.notecard", "ding\n"));
  const LabFolder::Linkset ls = LabFolder::open(top);
  QVERIFY(ls.ok());
  QCOMPARE(prims(ls), QJsonArray({QJsonArray{1, "Collar", ""}, QJsonArray{2, "Bell", "Bell"}}));
  QCOMPARE(scripts(ls), QJsonArray({QJsonArray{"core", "core.lsl", 1}, QJsonArray{"bell", "Bell/bell.lsl", 2}}));
  QCOMPARE(ls.notecards[0].name, QString("sounds"));
  QCOMPARE(ls.notecards[0].link, 2);
}

void TestLabFolder::linkMessagesReachEveryPrim()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Cuffs");
  QVERIFY(writeFile(top + "/Root/main.lsl",
    "default {\n"
    "  state_entry() { llMessageLinked(LINK_ALL_OTHERS, 1, \"lock\", NULL_KEY); }\n"
    "  link_message(integer sender, integer num, string text, key id) { llOwnerSay(\"main heard \" + text + \" from link \" + (string)sender); }\n"
    "}\n"));
  QVERIFY(writeFile(top + "/Left/left.lsl",
    "default {\n"
    "  link_message(integer sender, integer num, string text, key id) {\n"
    "    llOwnerSay(\"left heard \" + text + \" in link \" + (string)llGetLinkNumber());\n"
    "    if (text == \"lock\") llMessageLinked(LINK_ROOT, 2, \"left locked\", NULL_KEY);\n"
    "  }\n"
    "}\n"));
  QVERIFY(writeFile(top + "/Right/right.lsl",
    "default {\n"
    "  link_message(integer sender, integer num, string text, key id) {\n"
    "    llOwnerSay(\"right heard \" + text + \" in link \" + (string)llGetLinkNumber());\n"
    "    if (text == \"lock\") llMessageLinked(LINK_ROOT, 2, \"right locked\", NULL_KEY);\n"
    "  }\n"
    "}\n"));
  const QJsonObject run = LabFolder::run(LabFolder::open(top), QJsonObject{{"seconds", 1}});
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(entries(run, "owner_say"),
           QJsonArray({QJsonArray{"left heard lock in link 2", "left", "Left", "Left/left.lsl"},
                       QJsonArray{"right heard lock in link 3", "right", "Right", "Right/right.lsl"},
                       QJsonArray{"main heard left locked from link 2", "main", "Root", "Root/main.lsl"},
                       QJsonArray{"main heard right locked from link 3", "main", "Root", "Root/main.lsl"}}));
  // the scripts of "objects" carry their file and prim too
  const QJsonArray list = run.value("result").toObject().value("objects").toArray().at(0).toObject().value("scripts").toArray();
  QCOMPARE(list.size(), 3);
  QCOMPARE(list.at(2).toObject().value("file").toString(), QString("Right/right.lsl"));
  QCOMPARE(list.at(2).toObject().value("prim").toString(), QString("Right"));
  QCOMPARE(list.at(2).toObject().value("link").toInt(), 3);
  QCOMPARE(run.value("result").toObject().value("objects").toArray().at(0).toObject().value("prims").toInt(), 3);
}

void TestLabFolder::singlePrimObject()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Ball");
  QVERIFY(writeFile(top + "/a.lsl", "default { state_entry() { llMessageLinked(LINK_THIS, 0, \"x\", NULL_KEY); llOwnerSay(\"a in link \" + (string)llGetLinkNumber()); } }"));
  QVERIFY(writeFile(top + "/b.lsl", "default { link_message(integer s, integer n, string t, key k) { llOwnerSay(\"b heard \" + t + \" from \" + (string)s); } }"));
  const QJsonObject run = LabFolder::run(LabFolder::open(top));
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(entries(run, "owner_say"), QJsonArray({QJsonArray{"a in link 0", "a", "Ball", "a.lsl"}, QJsonArray{"b heard x from 0", "b", "Ball", "b.lsl"}}));
}

void TestLabFolder::allScriptsAreCheckedAtOnce()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Broken");
  QVERIFY(writeFile(top + "/good.lsl", "default { state_entry() { llOwnerSay(\"never\"); } }"));
  QVERIFY(writeFile(top + "/Left/bad_type.lsl", "default { state_entry() { llOwnerSay(1); } }"));
  QVERIFY(writeFile(top + "/Right/bad_name.lsl", "default {\n  state_entry() {\n    nothere();\n  }\n}\n"));
  const LabFolder::Linkset ls = LabFolder::open(top);
  QVERIFY(ls.ok());
  const QJsonObject check = LabFolder::check(ls);
  QCOMPARE(check.value("valid").toBool(), false);
  const QJsonArray errors = check.value("errors").toArray();
  QCOMPARE(errors.size(), 2);
  QCOMPARE(errors.at(0).toObject(), QJsonObject({{"file", "Left/bad_type.lsl"}, {"script", "bad_type"}, {"prim", "Left"}, {"link", 2},
                                                 {"line", 1}, {"column", 38}, {"message", "Argument 1 of llOwnerSay must be a string, not an integer."}}));
  QCOMPARE(errors.at(1).toObject().value("file").toString(), QString("Right/bad_name.lsl"));
  QCOMPARE(errors.at(1).toObject().value("prim").toString(), QString("Right"));
  QCOMPARE(errors.at(1).toObject().value("link").toInt(), 3);
  QCOMPARE(errors.at(1).toObject().value("line").toInt(), 3);
  QCOMPARE(errors.at(1).toObject().value("column").toInt(), 5);
  QVERIFY(errors.at(1).toObject().value("message").toString().contains("nothere"));
  const QJsonArray perScript = check.value("scripts").toArray();
  QCOMPARE(perScript.size(), 3);
  QCOMPARE(perScript.at(0).toObject().value("file").toString(), QString("good.lsl"));
  QCOMPARE(perScript.at(0).toObject().value("valid").toBool(), true);
  QCOMPARE(perScript.at(1).toObject().value("valid").toBool(), false);
  QCOMPARE(perScript.at(2).toObject().value("valid").toBool(), false);

  const QJsonObject run = LabFolder::run(ls);
  QCOMPARE(run.value("ok").toBool(), false);
  QCOMPARE(run.value("error").toString(),
           QString("The folder was not run: 2 script(s) do not compile (Left/bad_type.lsl, Right/bad_name.lsl). Every error names its script and prim."));
  QCOMPARE(run.value("check").toObject(), check);
  QVERIFY(!run.contains("result"));   // nothing ran, so "never" was not said
}

void TestLabFolder::everyLogLineNamesItsSource()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Box");
  QVERIFY(writeFile(top + "/root.lsl", "default { state_entry() { llSetTimerEvent(0.5); } timer() { llSetTimerEvent(0); llOwnerSay(\"tick\"); } }"));
  QVERIFY(writeFile(top + "/Lid/lid.lsl", "default { touch_start(integer n) { llOwnerSay(\"lid touched\"); } }"));
  const QJsonObject run = LabFolder::run(LabFolder::open(top),
                                         QJsonObject{{"seconds", 2}, {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"link", 2}}}}});
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  const QJsonArray log = run.value("result").toObject().value("log").toArray();
  QVERIFY(log.size() > 5);
  int fromScripts = 0;
  int fromSandbox = 0;
  for (const QJsonValue& v : log)
  {
    const QJsonObject e = v.toObject();
    QVERIFY2(e.contains("prim"), qPrintable(e.value("type").toString()));
    if (e.contains("script"))
    {
      ++fromScripts;
      QVERIFY(e.contains("file"));
      QVERIFY(!e.contains("source"));
    }
    else
    {
      ++fromSandbox;
      QCOMPARE(e.value("source").toString(), QString("sandbox"));
    }
  }
  QVERIFY(fromScripts > 0);
  QCOMPARE(fromSandbox, 1);
  // the touch input names the prim it went to
  QJsonArray inputs;
  for (const QJsonValue& v : log)
    if (v.toObject().value("type").toString() == "input")
      inputs.append(QJsonArray{v.toObject().value("input"), v.toObject().value("prim"), v.toObject().value("source")});
  QCOMPARE(inputs, QJsonArray({QJsonArray{"touch", "Lid", "sandbox"}}));
  QCOMPARE(entries(run, "owner_say"), QJsonArray({QJsonArray{"tick", "root", "Box", "root.lsl"}, QJsonArray{"lid touched", "lid", "Lid", "Lid/lid.lsl"}}));
}

void TestLabFolder::notecardsComeFromTheFolder()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Reader");
  QVERIFY(writeFile(top + "/reader.lsl",
    "key q; integer n;\n"
    "default {\n"
    "  state_entry() { q = llGetNotecardLine(\"config\", 0); }\n"
    "  dataserver(key id, string data) {\n"
    "    if (data == EOF) { llOwnerSay(\"done\"); return; }\n"
    "    llOwnerSay(\"line \" + data);\n"
    "    q = llGetNotecardLine(\"config\", ++n);\n"
    "  }\n"
    "}\n"));
  QVERIFY(writeFile(top + "/config.txt", "first\nsecond\n"));
  const QJsonObject run = LabFolder::run(LabFolder::open(top), QJsonObject{{"seconds", 5}});
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(entries(run, "owner_say"), QJsonArray({QJsonArray{"line first", "reader", "Reader", "reader.lsl"},
                                                  QJsonArray{"line second", "reader", "Reader", "reader.lsl"},
                                                  QJsonArray{"done", "reader", "Reader", "reader.lsl"}}));
}

void TestLabFolder::requestShape()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Shape");
  QVERIFY(writeFile(top + "/a.lsl", Quiet));
  QVERIFY(writeFile(top + "/Arm/b.lsl", Quiet));
  QVERIFY(writeFile(top + "/Arm/note.txt", "x\n"));
  const QJsonObject req = LabFolder::request(LabFolder::open(top), QJsonObject{{"seconds", 3}, {"keep_rlv", true}, {"linkset", true}});
  QCOMPARE(req, QJsonObject({{"command", "lsl_run"}, {"seconds", 3}, {"keep_rlv", true},
                             {"objects", QJsonArray{QJsonObject{{"name", "Shape"}, {"prims", 2},
                                                                {"scripts", QJsonArray{QJsonObject{{"name", "a"}, {"source", QString(Quiet)}, {"link", 1}},
                                                                                       QJsonObject{{"name", "b"}, {"source", QString(Quiet)}, {"link", 2}}}},
                                                                {"inventory", QJsonArray{QJsonObject{{"name", "note"}, {"type", "notecard"}, {"lines", QJsonArray{"x"}}}}}}}}}));
}

void TestLabFolder::emptyFolderDoesNotRun()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(QDir(dir.path()).mkpath("Empty/Child"));
  const LabFolder::Linkset ls = LabFolder::open(dir.filePath("Empty"));
  QVERIFY(ls.ok());
  QCOMPARE(prims(ls), QJsonArray({QJsonArray{1, "Empty", ""}, QJsonArray{2, "Child", "Child"}}));
  const QJsonObject run = LabFolder::run(ls);
  QCOMPARE(run.value("ok").toBool(), false);
  QCOMPARE(run.value("error").toString(), QString("The folder has no .lsl script, so nothing was run."));
}

void TestLabFolder::tooManyScripts()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Many");
  for (int i = 0; i < LabFolder::MAX_SCRIPTS + 1; ++i) QVERIFY(writeFile(top + QString("/s%1.lsl").arg(i, 2, 10, QChar('0')), Quiet));
  const LabFolder::Linkset ls = LabFolder::open(top);
  QVERIFY(!ls.ok());
  QCOMPARE(problems(ls.problems), QJsonArray({QJsonArray{"The object has more than 32 scripts; the sandbox runs at most 32 in one object.", "", "", 0}}));
  const QJsonObject run = LabFolder::run(ls);
  QCOMPARE(run.value("ok").toBool(), false);
  QCOMPARE(run.value("error").toString(),
           QString("The folder was not run: it breaks 1 limit(s) or rule(s), listed in \"problems\" with the file and prim of each."));
  QCOMPARE(run.value("linkset").toObject().value("problems").toArray(),
           QJsonArray({QJsonObject{{"message", "The object has more than 32 scripts; the sandbox runs at most 32 in one object."}}}));
  QVERIFY(!run.contains("result"));
}

void TestLabFolder::fileAndTotalSize()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Big");
  // a comment fills a script up to an exact size
  auto padded = [](int bytes) { QByteArray b = Quiet + "\n//"; b.append(QByteArray(bytes - b.size(), 'x')); return b; };
  QVERIFY(writeFile(top + "/one.lsl", padded(200000)));
  QVERIFY(writeFile(top + "/Arm/two.lsl", padded(200000)));
  QVERIFY(writeFile(top + "/Leg/three.lsl", padded(200000)));
  QVERIFY(writeFile(top + "/Leg/four.lsl", padded(200000)));
  QVERIFY(writeFile(top + "/Leg/five.lsl", padded(200000)));
  QVERIFY(writeFile(top + "/Leg/six.lsl", padded(200000)));
  QVERIFY(writeFile(top + "/Leg/huge.lsl", padded(LabFolder::MAX_FILE_BYTES + 1)));
  const LabFolder::Linkset ls = LabFolder::open(top);
  QCOMPARE(problems(ls.problems), QJsonArray({QJsonArray{"The file is larger than 262144 bytes, so it was not read.", "Leg/huge.lsl", "Leg", 3},
                                              QJsonArray{"The scripts and notecards together are larger than 1048576 bytes.", "", "", 0}}));
}

void TestLabFolder::pathAndNameLength()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Long");
  const QString longName = QString(64, 'n');
  QVERIFY(writeFile(top + "/" + longName + ".lsl", Quiet));
  QVERIFY(writeFile(top + "/" + longName + "/x.lsl", Quiet));
  // a path inside the folder longer than 255 characters: four folders... prims are one level, so a long prim folder and file name
  const QString prim = QString(60, 'p');
  const QString file = QString(200, 'f') + ".lsl";
  const bool longPathMade = writeFile(top + "/" + prim + "/" + file, Quiet);
  const LabFolder::Linkset ls = LabFolder::open(top);
  QJsonArray expected{QJsonArray{"A prim's name is its folder's name; it must be 1 to 63 characters. Rename the folder.", longName, longName, 2}};
  // the files are read in link order: the root's script, then the long prim folder's, then the other prim's
  expected.append(QJsonArray{"The name (the file name without its extension) must be 1 to 63 characters.", longName + ".lsl", "Long", 1});
  if (longPathMade)
    expected.append(QJsonArray{"The path inside the folder is longer than 255 characters. Use shorter names.", prim + "/" + file, prim, 3});
  QCOMPARE(problems(ls.problems), expected);
}

void TestLabFolder::nameUsedTwice()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Twice");
  QVERIFY(writeFile(top + "/Left/main.lsl", Quiet));
  QVERIFY(writeFile(top + "/Right/main.lsl", Quiet));
  QVERIFY(writeFile(top + "/Right/config.lsl", Quiet));
  QVERIFY(writeFile(top + "/config.txt", "a\n"));
  const LabFolder::Linkset ls = LabFolder::open(top);
  QCOMPARE(problems(ls.problems),
           QJsonArray({QJsonArray{"The name \"config\" is used twice (config.txt and Right/config.lsl). The sandbox keeps one inventory per object, so every script and notecard needs its own name. Rename one.",
                                  "Right/config.lsl", "Right", 3},
                       QJsonArray{"The name \"main\" is used twice (Left/main.lsl and Right/main.lsl). The sandbox keeps one inventory per object, so every script and notecard needs its own name. Rename one.",
                                  "Right/main.lsl", "Right", 3}}));
}

void TestLabFolder::nestedHiddenAndLinkedFolders()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QTemporaryDir outside;
  QVERIFY(outside.isValid());
  QVERIFY(writeFile(outside.filePath("secret.lsl"), Quiet));
  const QString top = dir.filePath("Obj");
  QVERIFY(writeFile(top + "/main.lsl", Quiet));
  QVERIFY(writeFile(top + "/.git/hook.lsl", Quiet));
  QVERIFY(writeFile(top + "/Arm/arm.lsl", Quiet));
  QVERIFY(writeFile(top + "/Arm/Hand/hand.lsl", Quiet));
#ifndef Q_OS_WIN
  QVERIFY(QFile::link(outside.path(), top + "/Away"));
  QVERIFY(QFile::link(outside.filePath("secret.lsl"), top + "/Arm/secret.lsl"));
#endif
  const LabFolder::Linkset ls = LabFolder::open(top);
  QVERIFY(ls.ok());   // notes do not stop a run
  QCOMPARE(prims(ls), QJsonArray({QJsonArray{1, "Obj", ""}, QJsonArray{2, "Arm", "Arm"}}));
  QCOMPARE(scripts(ls), QJsonArray({QJsonArray{"main", "main.lsl", 1}, QJsonArray{"arm", "Arm/arm.lsl", 2}}));
  QJsonArray expected;
#ifndef Q_OS_WIN
  expected.append(QJsonArray{"\"Away\" is a link to a folder; links are not followed, so it is not a prim.", "Away", "", 0});
#endif
  expected.append(QJsonArray{"\"Arm/Hand\" is a folder inside a prim's folder. Prims are not nested, so it was not read.", "Arm/Hand", "Arm", 2});
#ifndef Q_OS_WIN
  expected.append(QJsonArray{"\"Arm/secret.lsl\" is a link that leads out of the folder, so it was not read.", "Arm/secret.lsl", "Arm", 2});
#endif
  QCOMPARE(problems(ls.notes), expected);
}

void TestLabFolder::endlessLoopsStopWithTheirSource()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Loops");
  QVERIFY(writeFile(top + "/a.lsl", "default { state_entry() { while (TRUE) { } } }"));
  QVERIFY(writeFile(top + "/B/b.lsl", "default { state_entry() { integer i; for (;;) { ++i; } } }"));
  QVERIFY(writeFile(top + "/C/c.lsl", "f() { f(); }\ndefault { state_entry() { f(); } }"));
  QVERIFY(writeFile(top + "/D/d.lsl", "default { state_entry() { llOwnerSay(\"still alive\"); } }"));
  QElapsedTimer clock;
  clock.start();
  const QJsonObject run = LabFolder::run(LabFolder::open(top), QJsonObject{{"seconds", 1}});
  QVERIFY2(clock.elapsed() < 10000, qPrintable(QString::number(clock.elapsed())));
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QJsonArray stops;
  for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
  {
    const QJsonObject e = v.toObject();
    if (e.value("type").toString() != "limit") continue;
    // a slow machine (a sanitizer build) may reach the shared real-time cap before the step limit
    QString message = e.value("message").toString();
    if (message == "The script ran longer than 2000 ms of real time and was stopped.") message = "The script ran more than 500000 steps and was stopped (an endless loop?).";
    stops.append(QJsonArray{e.value("script"), e.value("prim"), e.value("file"), message});
  }
  QCOMPARE(stops, QJsonArray({QJsonArray{"a", "Loops", "a.lsl", "The script ran more than 500000 steps and was stopped (an endless loop?)."},
                              QJsonArray{"b", "B", "B/b.lsl", "The script ran more than 500000 steps and was stopped (an endless loop?)."},
                              QJsonArray{"c", "C", "C/c.lsl", "Function calls nested deeper than 64 (endless recursion?); the script was stopped."}}));
  QCOMPARE(entries(run, "owner_say"), QJsonArray({QJsonArray{"still alive", "d", "D", "D/d.lsl"}}));
  QJsonArray stopped;
  for (const QJsonValue& v : run.value("result").toObject().value("objects").toArray().at(0).toObject().value("scripts").toArray())
    stopped.append(QJsonArray{v.toObject().value("name"), v.toObject().value("prim"), v.toObject().value("stopped")});
  QCOMPARE(stopped, QJsonArray({QJsonArray{"a", "Loops", true}, QJsonArray{"b", "B", true}, QJsonArray{"c", "C", true}, QJsonArray{"d", "D", false}}));
}

// loops that never yield and are slow per step reach the real-time cap, not the step limit; the cap counts
// from when the scripts are made, the same moment for all of them, so three such scripts together stop after
// about one cap (2000 ms), not three
void TestLabFolder::slowLoopsShareOneRealTimeCap()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Slow");
  const QByteArray slow = "default { state_entry() { string s = \"x\"; while (TRUE) { s = llGetSubString(s + s, 0, 30000); } } }";
  QVERIFY(writeFile(top + "/a.lsl", slow));
  QVERIFY(writeFile(top + "/B/b.lsl", slow));
  QVERIFY(writeFile(top + "/C/c.lsl", slow));
  QElapsedTimer clock;
  clock.start();
  const QJsonObject run = LabFolder::run(LabFolder::open(top), QJsonObject{{"seconds", 1}});
  const qint64 elapsed = clock.elapsed();
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QVERIFY2(elapsed < 5000, qPrintable(QString::number(elapsed)));
  QJsonArray stops;
  for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
  {
    const QJsonObject e = v.toObject();
    if (e.value("type").toString() == "limit") stops.append(QJsonArray{e.value("script"), e.value("prim"), e.value("message")});
  }
  const QString cap = "The script ran longer than 2000 ms of real time and was stopped.";
  QCOMPARE(stops, QJsonArray({QJsonArray{"a", "Slow", cap}, QJsonArray{"b", "B", cap}, QJsonArray{"c", "C", cap}}));
}

void TestLabFolder::autoSecondsGrowWithScripts()
{
  QCOMPARE(LabFolder::autoSeconds(0), 10);
  QCOMPARE(LabFolder::autoSeconds(1), 12);
  QCOMPARE(LabFolder::autoSeconds(2), 14);
  QCOMPARE(LabFolder::autoSeconds(10), 30);
  QCOMPARE(LabFolder::autoSeconds(25), 60);
  QCOMPARE(LabFolder::autoSeconds(32), 60);
  QCOMPARE(LabFolder::autoSeconds(1000), 60);
}

// a setup that needs 12 simulated seconds (a big collar): with no "seconds" the folder run gives it
// 10 + 2 per script, so it finishes; a flat 10 would have cut it off
void TestLabFolder::bigSetupGetsTimeToFinish()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Collar");
  QVERIFY(writeFile(top + "/a.lsl", "default { state_entry() { llSetTimerEvent(12.0); } timer() { llSetTimerEvent(0.0); llOwnerSay(\"setup done\"); } }"));
  QVERIFY(writeFile(top + "/B/b.lsl", Quiet));
  const QJsonObject run = LabFolder::run(LabFolder::open(top), QJsonObject());
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(run.value("seconds").toInt(), 14);
  QCOMPARE(run.value("time_note").toString(), QString("The setup ran 14 simulated seconds (10 + 2 for each of 2 scripts)."));
  QCOMPARE(entries(run, "owner_say"), QJsonArray({QJsonArray{"setup done", "a", "Collar", "a.lsl"}}));
  const QJsonObject flat = LabFolder::run(LabFolder::open(top), QJsonObject{{"seconds", 10}});
  QVERIFY2(flat.value("ok").toBool(), qPrintable(flat.value("error").toString()));
  QCOMPARE(flat.value("time_note").toString(), QString("The setup ran 10 simulated seconds, as asked."));
  QCOMPARE(entries(flat, "owner_say"), QJsonArray());
}

// a script that never settles (a timer for ever) is cut off at the hard limit, with the message saying so
void TestLabFolder::foreverTimerStopsAtTheHardLimit()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Busy");
  for (int i = 0; i < 26; ++i)
    QVERIFY(writeFile(top + QString("/s%1.lsl").arg(i, 2, 10, QChar('0')), i == 0 ? "default { state_entry() { llSetTimerEvent(1.0); } timer() { } }" : Quiet));
  const QJsonObject run = LabFolder::run(LabFolder::open(top), QJsonObject());
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(run.value("seconds").toInt(), 60);
  QCOMPARE(run.value("time_note").toString(),
           QString("The setup ran the longest automatic time, 60 simulated seconds (26 scripts). If it needs more, give \"seconds\" (at most 3600)."));
  QCOMPARE(run.value("result").toObject().value("time").toDouble(), 60.0);
}

// the safety limits are unchanged with the longer automatic time
void TestLabFolder::runawayLoopStillStopsWithAutoTime()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Runaway");
  QVERIFY(writeFile(top + "/a.lsl", "default { state_entry() { while (TRUE) { } } }"));
  QElapsedTimer clock;
  clock.start();
  const QJsonObject run = LabFolder::run(LabFolder::open(top), QJsonObject());
  QVERIFY2(clock.elapsed() < 10000, qPrintable(QString::number(clock.elapsed())));
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QString message;
  for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
    if (v.toObject().value("type").toString() == "limit") message = v.toObject().value("message").toString();
  if (message == "The script ran longer than 2000 ms of real time and was stopped.") message = "The script ran more than 500000 steps and was stopped (an endless loop?).";
  QCOMPARE(message, QString("The script ran more than 500000 steps and was stopped (an endless loop?)."));
}

// Anafin HUD test: a folder watched as one object takes avatars, transfers, order, seed, permissions and replies, as lsl_run does
void TestLabFolder::anafinRunOptionsReachTheRun()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString top = dir.filePath("Hud");
  // the root asks for permissions and counts the avatars; the child asks the RLV viewer for its version
  QVERIFY(writeFile(top + "/main.lsl", "default { state_entry() { llOwnerSay(\"main \" + (string)llGetListLength(llGetAgentList(AGENT_LIST_REGION, []))); "
                                       "llRequestPermissions(llGetOwner(), PERMISSION_DEBIT); } "
                                       "run_time_permissions(integer p) { llOwnerSay(\"perm \" + (string)p); } }"));
  QVERIFY(writeFile(top + "/Arm/arm.lsl", "default { state_entry() { llOwnerSay(\"arm\"); llListen(5, \"\", NULL_KEY, \"\"); llOwnerSay(\"@version=5\"); } "
                                          "listen(integer c, string n, key i, string m) { llOwnerSay(\"heard\"); } }"));
  const QJsonArray avatars{QJsonObject{{"name", "Bob"}, {"online", false}}, QJsonObject{{"name", "Ann"}}};
  const QJsonArray transfers{QJsonObject{{"result", "none"}}};
  const QJsonArray permissions{QJsonObject{{"result", "deny"}}};
  const QJsonObject replies{{"rlv", QJsonObject{{"result", "none"}}}};
  const QJsonObject options{{"seconds", 2}, {"avatars", avatars}, {"transfers", transfers}, {"order", "shuffle"}, {"seed", 3},
                            {"permissions", permissions}, {"replies", replies}, {"linkset", true}};
  const QJsonObject req = LabFolder::request(LabFolder::open(top), options);
  QCOMPARE(req.value("avatars").toArray(), avatars);
  QCOMPARE(req.value("transfers").toArray(), transfers);
  QCOMPARE(req.value("order").toString(), QString("shuffle"));
  QCOMPARE(req.value("seed").toInt(), 3);
  QCOMPARE(req.value("permissions").toArray(), permissions);
  QCOMPARE(req.value("replies").toObject(), replies);
  QVERIFY(!req.contains("linkset"));
  const QJsonObject run = LabFolder::run(LabFolder::open(top), options);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  // the owner and Ann are online (Bob is not), the owner denies, the viewer never answers; seed 3 runs the arm first
  QCOMPARE(entries(run, "owner_say"), QJsonArray({QJsonArray{"arm", "arm", "Arm", "Arm/arm.lsl"}, QJsonArray{"main 2", "main", "Hud", "main.lsl"},
                                                  QJsonArray{"perm 0", "main", "Hud", "main.lsl"}}));
  QCOMPARE(run.value("result").toObject().value("order").toObject().value("scripts").toArray(), QJsonArray({"Hud/arm", "Hud/main"}));
  // without the options: everyone is online, the owner grants, the viewer answers, list order
  const QJsonObject plain = LabFolder::run(LabFolder::open(top), QJsonObject{{"seconds", 2}});
  QCOMPARE(entries(plain, "owner_say"), QJsonArray({QJsonArray{"main 1", "main", "Hud", "main.lsl"}, QJsonArray{"arm", "arm", "Arm", "Arm/arm.lsl"},
                                                    QJsonArray{"perm 2", "main", "Hud", "main.lsl"}, QJsonArray{"heard", "arm", "Arm", "Arm/arm.lsl"}}));
}

QTEST_GUILESS_MAIN(TestLabFolder)
#include "tst_labfolder.moc"
