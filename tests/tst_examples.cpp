/*
 * PinglyMotion - tests of the Script Lab example folders (job E1, docs/EXAMPLES.md)
 *
 * Every example in examples/objects is one object (the folder model,
 * src/scriptlab/labfolder.h). For each one, exact answers only:
 * - the folder opens as the object we expect (prims, scripts, links), and
 *   every script passes the import check;
 * - a run with the inputs the guide asks for (touch, a dialog button, chat)
 *   gives exactly the expected owner lines and RLV restrictions;
 * - the props in example.json have the names of the prims, and with the
 *   colours of the run (src/scriptlab/labtint.h) on props of those names, as
 *   the main window gives them, every prop has the expected colour.
 * The guide texts and the rest of example.json are checked by
 * tests/test_examples.py.
 *
 * The cuff set in examples/scriptlab/cuffs (job 24) is tested here too: its
 * menu, a chain between every pair of points, hanging from the ceiling ring,
 * the leash, the RLV lines per cuff and the state after a reset, after
 * wearing it again and after a save (the recipe replayed).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "dingle.h"
#include "prop.h"
#include "scriptlab/labchains.h"
#include "scriptlab/labfolder.h"
#include "scriptlab/labrecipe.h"
#include "scriptlab/labtint.h"
#include "scriptlab/labwatcher.h"

namespace
{
  QString exampleFolder(const QString& name) { return QStringLiteral(EXAMPLES_DIR) + QLatin1Char('/') + name; }

  QJsonObject exampleJson(const QString& name)
  {
    QFile file(exampleFolder(name) + QStringLiteral("/example.json"));
    if (!file.open(QIODevice::ReadOnly)) return QJsonObject();
    return QJsonDocument::fromJson(file.readAll()).object();
  }

  // [link, name] of every prim
  QJsonArray prims(const LabFolder::Linkset& ls)
  {
    QJsonArray out;
    for (const LabFolder::Prim& p : ls.prims) out.append(QJsonArray{p.link, p.name});
    return out;
  }

  // [file, link] of every script
  QJsonArray scripts(const LabFolder::Linkset& ls)
  {
    QJsonArray out;
    for (const LabFolder::Script& s : ls.scripts) out.append(QJsonArray{s.file, s.link});
    return out;
  }

  // the owner lines of a run, in order, as "prim: text"
  QStringList ownerLines(const QJsonObject& run)
  {
    QStringList out;
    for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
    {
      const QJsonObject e = v.toObject();
      if (e.value("type").toString() == QLatin1String("owner_say"))
        out << e.value("prim").toString() + QStringLiteral(": ") + e.value("text").toString();
    }
    return out;
  }

  // the restrictions held at the end of a run, sorted (llOwnerSay lines that start with @ go to the
  // RLV state, not to the owner lines)
  QStringList restrictions(const QJsonObject& run)
  {
    QStringList out;
    for (const QJsonValue& v : run.value("result").toObject().value("rlv").toObject().value("state").toObject().value("restrictions").toArray())
    {
      out << v.toObject().value("behaviour").toString();
    }
    out.sort();
    return out;
  }

  // The props of example.json with the colours of the run, as the main window gives them
  // (src/pinglymotionwindow.cpp, propTintsChanged): "name: r g b a" with values from 0 to 1,
  // rounded to two places.
  QStringList propColours(const QJsonObject& example, const QJsonObject& run)
  {
    const QList<LabTint::Tint> tints = LabTint::fromFolder(run);
    QSharedPointer<Mesh> mesh(new Mesh(QString(), Mesh::Shape::SPHERE));
    QStringList out;
    unsigned int id = 1;
    for (const QJsonValue& v : example.value("props").toArray())
    {
      Prop prop(id++, Prop::Box, v.toObject().value("name").toString(), mesh);
      for (const LabTint::Tint& tint : tints)
      {
        if (tint.prim != prop.name()) continue;
        const QColor colour = tint.hasColour ? QColor::fromRgbF(tint.colour[0], tint.colour[1], tint.colour[2]) : prop.drawColour();
        prop.setTint(colour, tint.hasAlpha ? tint.alpha : 1.0f);
      }
      const QColor c = prop.drawColour();
      out << QStringLiteral("%1: %2 %3 %4 %5").arg(prop.name()).arg(c.redF(), 0, 'f', 2).arg(c.greenF(), 0, 'f', 2)
                                                   .arg(c.blueF(), 0, 'f', 2).arg(prop.drawAlpha(), 0, 'f', 2);
    }
    return out;
  }

  QJsonObject touch(double at, int link = 1) { return QJsonObject{{"at", at}, {"type", "touch"}, {"link", link}}; }
  QJsonObject dialog(double at, const QString& button) { return QJsonObject{{"at", at}, {"type", "dialog"}, {"button", button}}; }

  QJsonObject runFolder(const QString& name, const QJsonArray& inputs, double seconds)
  {
    const LabFolder::Linkset ls = LabFolder::open(exampleFolder(name));
    return LabFolder::run(ls, QJsonObject{{"seconds", seconds}, {"inputs", inputs}});
  }

  // prints what a run gave, to write the expected answers (PINGLYMOTION_EXAMPLES_DUMP=1)
  void dump(const QString& name, const QJsonObject& run)
  {
    if (qEnvironmentVariableIsEmpty("PINGLYMOTION_EXAMPLES_DUMP")) return;
    qWarning().noquote() << name << "error:" << run.value("error").toString();
    for (const QString& l : ownerLines(run)) qWarning().noquote() << "  say" << l;
    for (const QString& r : restrictions(run)) qWarning().noquote() << "  rlv" << r;
    for (const QString& p : propColours(exampleJson(name), run)) qWarning().noquote() << "  prop" << p;
    for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
    {
      const QString type = v.toObject().value("type").toString();
      if (type == "debug" || type == "error" || type == "region_say_to" || type == "input")
        qWarning().noquote() << "  log" << QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact);
    }
  }

  // --- the cuff set (job 24): examples/scriptlab/cuffs, one object of ten prims

  QString cuffsFolder() { return QDir::cleanPath(QStringLiteral(EXAMPLES_DIR) + QStringLiteral("/../scriptlab/cuffs")); }

  QJsonObject cuffsJson()
  {
    QFile file(cuffsFolder() + QStringLiteral("/example.json"));
    if (!file.open(QIODevice::ReadOnly)) return QJsonObject();
    return QJsonDocument::fromJson(file.readAll()).object();
  }

  // the menu answers of one avatar, one second apart, after a touch on a prim (link 9 is Wrist_left)
  QJsonArray clicks(const QStringList& buttons, int link = 9, const QString& id = QString(), const QString& name = QString())
  {
    QJsonObject t = touch(1, link);
    if (!id.isEmpty()) { t["id"] = id; t["name"] = name; }
    QJsonArray inputs{t};
    double at = 2;
    for (const QString& b : buttons)
    {
      QJsonObject d = dialog(at++, b);
      if (!id.isEmpty()) { d["id"] = id; d["name"] = name; }
      inputs << d;
    }
    return inputs;
  }

  QJsonObject runCuffs(const QJsonArray& inputs, double seconds = -1)
  {
    const LabFolder::Linkset ls = LabFolder::open(cuffsFolder());
    if (seconds < 0) seconds = inputs.size() + 1;
    // with Second Life's memory limit for a Mono script on: the script must fit in 64 KB
    return LabFolder::run(ls, QJsonObject{{"seconds", seconds}, {"inputs", inputs}, {"script_memory", 65536}});
  }

  // the restrictions held at the end, with their option: "detach:l forearm", sorted
  QStringList rlvLines(const QJsonObject& run)
  {
    QStringList out;
    for (const QJsonValue& v : run.value("result").toObject().value("rlv").toObject().value("state").toObject().value("restrictions").toArray())
    {
      const QString option = v.toObject().value("option").toString();
      out << v.toObject().value("behaviour").toString() + (option.isEmpty() ? QString() : QStringLiteral(":") + option);
    }
    out.sort();
    return out;
  }

  // the chain lines the 3D view draws at the end (src/scriptlab/labchains.h): "from -> to"
  QStringList chainLines(const QJsonObject& run)
  {
    QStringList out;
    for (const LabChains::Chain& c : LabChains::fromFolderResult(run, false)) out << c.from + QStringLiteral(" -> ") + c.to;
    return out;
  }

  // the newest menu: its message, then its buttons
  QStringList lastMenu(const QJsonObject& run)
  {
    QStringList out;
    for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
    {
      const QJsonObject e = v.toObject();
      if (e.value("type").toString() != QLatin1String("dialog")) continue;
      out = QStringList{e.value("message").toString()};
      for (const QJsonValue& b : e.value("buttons").toArray()) out << b.toString();
    }
    return out;
  }

  QStringList entries(const QJsonObject& run, const QString& type, const QString& field)
  {
    QStringList out;
    for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
      if (v.toObject().value("type").toString() == type) out << v.toObject().value(field).toVariant().toString();
    return out;
  }

  // debug lines (a refused llDialog, a bad call) and errors: there must be none
  QStringList problems(const QJsonObject& run)
  {
    QStringList out;
    for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
    {
      const QString type = v.toObject().value("type").toString();
      if (type == QLatin1String("debug") || type == QLatin1String("error") || type == QLatin1String("limit"))
        out << QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact);
    }
    return out;
  }

  // the owner lines after the first one (the ready line)
  QStringList saidAfterReady(const QJsonObject& run)
  {
    QStringList out = ownerLines(run);
    if (!out.isEmpty()) out.removeFirst();
    return out;
  }

  const QString Ready = QStringLiteral("Root: Cuff set ready. Touch a cuff or the box.");
  const QStringList MainButtons = {"Lock", "Unlock", "Chain", "Unchain", "Leash", "Unleash", "Hang", "Release", "Free all"};
}

class TestExamples : public QObject
{
  Q_OBJECT

private slots:
  void allExamplesOpenAndCompile_data();
  void allExamplesOpenAndCompile();
  void propsArePrims_data();
  void propsArePrims();
  void firstColour();
  void buttonHud();
  void gearSetLockAll();
  void gearSetFreeAll();
  void timedLock();
  void rlvRelay();
  void workSystem();
  void cuffSetOpens();
  void cuffSetStartAndMenu();
  void cuffSetChainEveryPair_data();
  void cuffSetChainEveryPair();
  void cuffSetAllPairsAtOnce();
  void cuffSetHangAndRelease();
  void cuffSetLockPerCuff();
  void cuffSetLeashByAnotherAvatar();
  void cuffSetUnchainAndFreeAll();
  void cuffSetKeepsStateAfterResetAndRewear();
  void cuffSetRecipeAfterSave();
  void cuffSetClicksInTheWatchedFolder();
};

void TestExamples::allExamplesOpenAndCompile_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<QJsonArray>("expectedPrims");
  QTest::addColumn<QJsonArray>("expectedScripts");
  QTest::newRow("First_colour") << "First_colour"
    << QJsonArray{QJsonArray{1, "First_colour"}, QJsonArray{2, "Box 0"}}
    << QJsonArray{QJsonArray{"Box 0/colour.lsl", 2}};
  QTest::newRow("Button_HUD") << "Button_HUD"
    << QJsonArray{QJsonArray{1, "Button_HUD"}}
    << QJsonArray{QJsonArray{"button_hud.lsl", 1}};
  QTest::newRow("Gear_set") << "Gear_set"
    << QJsonArray{QJsonArray{1, "Root"}, QJsonArray{2, "Ankle_left"}, QJsonArray{3, "Ankle_right"}, QJsonArray{4, "Blindfold"},
                  QJsonArray{5, "Collar"}, QJsonArray{6, "Gag"}, QJsonArray{7, "Hood"}, QJsonArray{8, "Wrist_left"},
                  QJsonArray{9, "Wrist_right"}}
    << QJsonArray{QJsonArray{"Root/hud.lsl", 1}, QJsonArray{"Ankle_left/ankle_left.lsl", 2}, QJsonArray{"Ankle_right/ankle_right.lsl", 3},
                  QJsonArray{"Blindfold/blindfold.lsl", 4}, QJsonArray{"Collar/collar.lsl", 5}, QJsonArray{"Gag/gag.lsl", 6},
                  QJsonArray{"Hood/hood.lsl", 7}, QJsonArray{"Wrist_left/wrist_left.lsl", 8}, QJsonArray{"Wrist_right/wrist_right.lsl", 9}};
  QTest::newRow("Timed_lock") << "Timed_lock"
    << QJsonArray{QJsonArray{1, "Timed_lock"}}
    << QJsonArray{QJsonArray{"timed_lock.lsl", 1}};
  QTest::newRow("RLV_relay") << "RLV_relay"
    << QJsonArray{QJsonArray{1, "RLV_relay"}}
    << QJsonArray{QJsonArray{"relay.lsl", 1}};
  QTest::newRow("Work_system") << "Work_system"
    << QJsonArray{QJsonArray{1, "Root"}, QJsonArray{2, "Bottle"}, QJsonArray{3, "Score_board"}}
    << QJsonArray{QJsonArray{"Root/work_hud.lsl", 1}, QJsonArray{"Bottle/bottle.lsl", 2}, QJsonArray{"Score_board/score_board.lsl", 3}};
}

void TestExamples::allExamplesOpenAndCompile()
{
  QFETCH(QString, name);
  QFETCH(QJsonArray, expectedPrims);
  QFETCH(QJsonArray, expectedScripts);
  const LabFolder::Linkset ls = LabFolder::open(exampleFolder(name));
  QVERIFY2(ls.ok(), qPrintable(ls.error + QJsonDocument(ls.toJson()).toJson(QJsonDocument::Compact)));
  QCOMPARE(ls.name, name);
  QCOMPARE(prims(ls), expectedPrims);
  QCOMPARE(scripts(ls), expectedScripts);
  QVERIFY(ls.notes.empty());
  QVERIFY(ls.notecards.empty());
  // the import check: every script compiles, with no warning
  const QJsonObject check = LabFolder::check(ls);
  QVERIFY2(check.value("valid").toBool(), QJsonDocument(check).toJson(QJsonDocument::Compact).constData());
  QCOMPARE(check.value("errors").toArray(), QJsonArray());
  QCOMPARE(check.value("warnings").toArray(), QJsonArray());
}

void TestExamples::propsArePrims_data()
{
  QTest::addColumn<QString>("name");
  for (const char* n : {"First_colour", "Button_HUD", "Gear_set", "Timed_lock", "RLV_relay", "Work_system"})
    QTest::newRow(n) << QString(n);
}

void TestExamples::propsArePrims()
{
  // every prop is named like a prim of the object, and every prim with a script that colours it has a prop
  QFETCH(QString, name);
  const QJsonObject example = exampleJson(name);
  QVERIFY(!example.isEmpty());
  QCOMPARE(example.value("object").toString(), name);
  const LabFolder::Linkset ls = LabFolder::open(exampleFolder(name));
  QStringList primNames;
  for (const LabFolder::Prim& p : ls.prims) primNames << p.name;
  QStringList propNames;
  for (const QJsonValue& v : example.value("props").toArray())
  {
    const QString prop = v.toObject().value("name").toString();
    QVERIFY2(primNames.contains(prop), qPrintable(prop));
    QVERIFY2(!propNames.contains(prop), qPrintable(prop));
    propNames << prop;
  }
  for (const LabFolder::Script& s : ls.scripts)
    if (s.source.contains(QLatin1String("llSetColor")) || s.source.contains(QLatin1String("llSetAlpha")))
      QVERIFY2(propNames.contains(ls.primName(s.link)), qPrintable(s.file));
}

void TestExamples::firstColour()
{
  const QJsonObject run = runFolder("First_colour", QJsonArray(), 1);
  dump("First_colour", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(ownerLines(run), QStringList({"Box 0: Box 0 is red now."}));
  QCOMPARE(propColours(exampleJson("First_colour"), run), QStringList({"Box 0: 1.00 0.00 0.00 1.00"}));
}

void TestExamples::buttonHud()
{
  const QJsonObject run = runFolder("Button_HUD", QJsonArray{touch(1), dialog(2, "Red")}, 3);
  dump("Button_HUD", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(ownerLines(run), QStringList({"Button_HUD: Touch me to pick a colour.", "Button_HUD: Now red."}));
  QCOMPARE(propColours(exampleJson("Button_HUD"), run), QStringList({"Button_HUD: 1.00 0.00 0.00 1.00"}));
  // the second button
  const QJsonObject green = runFolder("Button_HUD", QJsonArray{touch(1), dialog(2, "Green")}, 3);
  QCOMPARE(ownerLines(green), QStringList({"Button_HUD: Touch me to pick a colour.", "Button_HUD: Now green."}));
  QCOMPARE(propColours(exampleJson("Button_HUD"), green), QStringList({"Button_HUD: 0.00 1.00 0.00 1.00"}));
  // before a touch the HUD is white
  QCOMPARE(propColours(exampleJson("Button_HUD"), runFolder("Button_HUD", QJsonArray(), 1)),
           QStringList({"Button_HUD: 1.00 1.00 1.00 1.00"}));
}

void TestExamples::gearSetLockAll()
{
  const QJsonObject start = runFolder("Gear_set", QJsonArray(), 1);
  dump("Gear_set start", start);
  QVERIFY2(start.value("ok").toBool(), qPrintable(start.value("error").toString()));
  QCOMPARE(ownerLines(start), QStringList({"Root: HUD ready. Touch me."}));
  QCOMPARE(restrictions(start), QStringList());
  QCOMPARE(propColours(exampleJson("Gear_set"), start),
           QStringList({"Root: 1.00 1.00 1.00 1.00", "Wrist_left: 0.00 1.00 0.00 1.00", "Wrist_right: 0.00 1.00 0.00 1.00",
                        "Ankle_left: 0.00 1.00 0.00 1.00", "Ankle_right: 0.00 1.00 0.00 1.00", "Collar: 0.00 1.00 0.00 1.00",
                        "Hood: 0.00 1.00 0.00 1.00", "Gag: 0.00 1.00 0.00 1.00", "Blindfold: 0.00 1.00 0.00 1.00"}));

  const QJsonObject run = runFolder("Gear_set", QJsonArray{touch(1), dialog(2, "Lock all")}, 3);
  dump("Gear_set lock", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(ownerLines(run), QStringList({"Root: HUD ready. Touch me.", "Root: All 8 pieces are locked."}));
  QCOMPARE(restrictions(run), QStringList({"detach", "sendchat"}));
  QCOMPARE(propColours(exampleJson("Gear_set"), run),
           QStringList({"Root: 1.00 0.00 0.00 1.00", "Wrist_left: 1.00 0.00 0.00 1.00", "Wrist_right: 1.00 0.00 0.00 1.00",
                        "Ankle_left: 1.00 0.00 0.00 1.00", "Ankle_right: 1.00 0.00 0.00 1.00", "Collar: 1.00 0.00 0.00 1.00",
                        "Hood: 1.00 0.00 0.00 1.00", "Gag: 1.00 0.00 0.00 1.00", "Blindfold: 1.00 0.00 0.00 1.00"}));
}

void TestExamples::gearSetFreeAll()
{
  const QJsonObject run = runFolder("Gear_set", QJsonArray{touch(1), dialog(2, "Lock all"), dialog(3, "Free all")}, 4);
  dump("Gear_set free", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(ownerLines(run), QStringList({"Root: HUD ready. Touch me.", "Root: All 8 pieces are locked.", "Root: All 8 pieces are free."}));
  QCOMPARE(restrictions(run), QStringList());
  for (const QString& line : propColours(exampleJson("Gear_set"), run))
    QVERIFY2(line.endsWith(QLatin1String(": 0.00 1.00 0.00 1.00")), qPrintable(line));
}

void TestExamples::timedLock()
{
  // locked at 1 second, the 5 seconds count down, free again after them
  const QJsonObject locked = runFolder("Timed_lock", QJsonArray{touch(1)}, 3.5);
  dump("Timed_lock locked", locked);
  QVERIFY2(locked.value("ok").toBool(), qPrintable(locked.value("error").toString()));
  QCOMPARE(ownerLines(locked), QStringList({"Timed_lock: Free. Touch me to lock for 5 seconds.", "Timed_lock: Locked for 5 seconds.",
                                            "Timed_lock: 4 seconds left.", "Timed_lock: 3 seconds left."}));
  QCOMPARE(restrictions(locked), QStringList({"detach"}));
  QCOMPARE(propColours(exampleJson("Timed_lock"), locked), QStringList({"Timed_lock: 1.00 0.00 0.00 1.00"}));

  const QJsonObject run = runFolder("Timed_lock", QJsonArray{touch(1)}, 8);
  dump("Timed_lock", run);
  QCOMPARE(ownerLines(run), QStringList({"Timed_lock: Free. Touch me to lock for 5 seconds.", "Timed_lock: Locked for 5 seconds.",
                                         "Timed_lock: 4 seconds left.", "Timed_lock: 3 seconds left.", "Timed_lock: 2 seconds left.",
                                         "Timed_lock: 1 second left.", "Timed_lock: Free. Touch me to lock for 5 seconds."}));
  QCOMPARE(restrictions(run), QStringList());
  QCOMPARE(propColours(exampleJson("Timed_lock"), run), QStringList({"Timed_lock: 0.00 1.00 0.00 1.00"}));
}

void TestExamples::rlvRelay()
{
  const QString everyone = QStringLiteral("ffffffff-ffff-ffff-ffff-ffffffffffff");
  auto chat = [](double at, const QString& text)
  { return QJsonObject{{"at", at}, {"type", "chat"}, {"channel", -1812221819}, {"text", text}}; };
  const QJsonObject run = runFolder("RLV_relay", QJsonArray{chat(1, "trap," + everyone + ",!version|@detach=n|@sendchat=n"),
                                                            chat(2, "trap,not-this-relay,@fly=n")}, 3);
  dump("RLV_relay", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(ownerLines(run), QStringList({"RLV_relay: Relay ready on channel -1812221819."}));
  QCOMPARE(restrictions(run), QStringList({"detach", "sendchat"}));
  QCOMPARE(propColours(exampleJson("RLV_relay"), run), QStringList({"RLV_relay: 1.00 0.00 0.00 1.00"}));
  // the answers, one per command, to the sender
  QStringList answers;
  for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
    if (v.toObject().value("type").toString() == QLatin1String("region_say_to")) answers << v.toObject().value("text").toString();
  QCOMPARE(answers.size(), 3);
  QVERIFY2(answers.at(0).startsWith(QLatin1String("trap,")) && answers.at(0).endsWith(QLatin1String(",!version,1100")), qPrintable(answers.at(0)));
  QVERIFY2(answers.at(1).endsWith(QLatin1String(",@detach=n,ok")), qPrintable(answers.at(1)));
  QVERIFY2(answers.at(2).endsWith(QLatin1String(",@sendchat=n,ok")), qPrintable(answers.at(2)));

  // !release lets go of everything and the relay is green again
  const QJsonObject released = runFolder("RLV_relay", QJsonArray{chat(1, "trap," + everyone + ",@detach=n"),
                                                                 chat(2, "trap," + everyone + ",!release")}, 3);
  QCOMPARE(ownerLines(released), QStringList({"RLV_relay: Relay ready on channel -1812221819."}));
  QCOMPARE(restrictions(released), QStringList());
  QCOMPARE(propColours(exampleJson("RLV_relay"), released), QStringList({"RLV_relay: 0.00 1.00 0.00 1.00"}));
}

void TestExamples::workSystem()
{
  QJsonArray inputs;
  for (int i = 0; i < 5; ++i) inputs << touch(1 + 2 * i) << dialog(2 + 2 * i, "Work");
  const QJsonObject run = runFolder("Work_system", inputs, 11.5);
  dump("Work_system", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(ownerLines(run), QStringList({"Root: Touch me and press Work.", "Bottle: Bottle 25% full.", "Bottle: Bottle 50% full.",
                                         "Bottle: Bottle 75% full.", "Bottle: Bottle 100% full.", "Score_board: Score: 1",
                                         "Bottle: Bottle 25% full."}));
  QCOMPARE(propColours(exampleJson("Work_system"), run),
           QStringList({"Root: 0.30 0.40 1.00 1.00", "Bottle: 0.20 0.60 1.00 0.40", "Score_board: 1.00 1.00 1.00 1.00"}));
  // one quarter: the bottle is less clear, nothing scored yet
  const QJsonObject one = runFolder("Work_system", QJsonArray{touch(1), dialog(2, "Work")}, 3);
  QCOMPARE(propColours(exampleJson("Work_system"), one),
           QStringList({"Root: 0.30 0.40 1.00 1.00", "Bottle: 0.20 0.60 1.00 0.40", "Score_board: 1.00 1.00 1.00 1.00"}));
}

// --- the cuff set (job 24)

void TestExamples::cuffSetOpens()
{
  const LabFolder::Linkset ls = LabFolder::open(cuffsFolder());
  QVERIFY2(ls.ok(), qPrintable(ls.error + QJsonDocument(ls.toJson()).toJson(QJsonDocument::Compact)));
  QCOMPARE(ls.name, QStringLiteral("cuffs"));
  QCOMPARE(prims(ls), (QJsonArray{QJsonArray{1, "Root"}, QJsonArray{2, "Ankle_left"}, QJsonArray{3, "Ankle_right"},
                                  QJsonArray{4, "Ceiling_ring"}, QJsonArray{5, "Leash_ankle_left"}, QJsonArray{6, "Leash_ankle_right"},
                                  QJsonArray{7, "Leash_wrist_left"}, QJsonArray{8, "Leash_wrist_right"}, QJsonArray{9, "Wrist_left"},
                                  QJsonArray{10, "Wrist_right"}}));
  QCOMPARE(scripts(ls), (QJsonArray{QJsonArray{"Root/cuffs_main.lsl", 1}}));
  QCOMPARE(int(ls.notecards.size()), 9);
  QVERIFY(ls.notes.empty());
  const QJsonObject check = LabFolder::check(ls);
  QVERIFY2(check.value("valid").toBool(), QJsonDocument(check).toJson(QJsonDocument::Compact).constData());
  QCOMPARE(check.value("errors").toArray(), QJsonArray());
  QCOMPARE(check.value("warnings").toArray(), QJsonArray());
  // every prim has a prop of its name, once
  QStringList props;
  for (const QJsonValue& v : cuffsJson().value("props").toArray()) props << v.toObject().value("name").toString();
  QStringList names;
  for (const LabFolder::Prim& p : ls.prims) names << p.name;
  props.sort();
  names.sort();
  QCOMPARE(props, names);
  // the hang pose the script plays lies next to it
  QFile hang(cuffsFolder() + QStringLiteral("/Root/cuffs_hang.dingle"));
  QVERIFY(hang.open(QIODevice::ReadOnly));
  const Dingle::Result pose = Dingle::parse(hang.readAll());
  QVERIFY2(pose.ok(), qPrintable(pose.error));
  QCOMPARE(pose.ignoredKeys, 0);
}

void TestExamples::cuffSetStartAndMenu()
{
  const QJsonObject start = runCuffs(QJsonArray(), 1);
  dump("cuffs start", start);
  QVERIFY2(start.value("ok").toBool(), qPrintable(start.value("error").toString()));
  QCOMPARE(ownerLines(start), QStringList({Ready}));
  QCOMPARE(rlvLines(start), QStringList());
  QCOMPARE(chainLines(start), QStringList());
  QCOMPARE(propColours(cuffsJson(), start),
           QStringList({"Root: 0.00 1.00 0.00 1.00", "Wrist_left: 0.00 1.00 0.00 1.00", "Wrist_right: 0.00 1.00 0.00 1.00",
                        "Ankle_left: 0.00 1.00 0.00 1.00", "Ankle_right: 0.00 1.00 0.00 1.00", "Leash_wrist_left: 0.60 0.60 0.60 1.00",
                        "Leash_wrist_right: 0.60 0.60 0.60 1.00", "Leash_ankle_left: 0.60 0.60 0.60 1.00",
                        "Leash_ankle_right: 0.60 0.60 0.60 1.00", "Ceiling_ring: 0.60 0.60 0.60 1.00"}));

  // a click on any part shows the main menu: at most 12 buttons, none over 24 characters
  for (int link = 1; link <= 10; ++link)
  {
    const QJsonObject run = runCuffs(QJsonArray{touch(1, link)}, 2);
    QCOMPARE(lastMenu(run), QStringList({"Cuff set\nLocked: none. Chains: none. Leash: none. Hanging: no."}) + MainButtons);
    QCOMPARE(problems(run), QStringList());
  }
  // every sub-menu
  const QList<QPair<QStringList, QStringList>> menus = {
    {{"Lock"}, {"Lock which cuff?", "Wrist L", "Wrist R", "Ankle L", "Ankle R", "All", "Back"}},
    {{"Unlock"}, {"Unlock which cuff?", "Wrist L", "Wrist R", "Ankle L", "Ankle R", "All", "Back"}},
    {{"Chain"}, {"Chain from where?", "Wrist L", "Wrist R", "Ankle L", "Ankle R", "Ceiling", "Back"}},
    {{"Chain", "Ankle L"}, {"Chain Ankle L to where?", "Wrist L", "Wrist R", "Ankle R", "Ceiling", "Back"}},
    {{"Chain", "Ceiling"}, {"Chain Ceiling to where?", "Wrist L", "Wrist R", "Ankle L", "Ankle R", "Back"}},
    {{"Leash"}, {"Put the leash on which cuff? You hold it.", "Wrist L", "Wrist R", "Ankle L", "Ankle R", "Back"}},
    {{"Lock", "Back"}, QStringList({"Cuff set\nLocked: none. Chains: none. Leash: none. Hanging: no."}) + MainButtons},
  };
  for (const auto& m : menus)
  {
    const QJsonObject run = runCuffs(clicks(m.first));
    QCOMPARE(lastMenu(run), m.second);
    QCOMPARE(problems(run), QStringList());
  }
  // nothing to take off: said, and the main menu again
  const QJsonObject none = runCuffs(clicks({"Unchain", "Unleash", "Release"}));
  QCOMPARE(saidAfterReady(none), QStringList({"Root: There are no chains.", "Root: There is no leash.", "Root: Nothing hangs from the ceiling."}));
  QCOMPARE(lastMenu(none).mid(1), MainButtons);
}

void TestExamples::cuffSetChainEveryPair_data()
{
  QTest::addColumn<QString>("from");
  QTest::addColumn<QString>("to");
  QTest::addColumn<QString>("line");
  QTest::addColumn<QString>("said");
  QTest::addColumn<QStringList>("rlv");
  const QStringList hands = {"edit", "rez"};
  const QStringList hogtie = {"edit", "fly", "rez"};
  const QStringList hang = {"fly", "sittp", "tplm", "tploc", "tplure"};
  QTest::newRow("wrist to wrist") << "Wrist L" << "Wrist R" << "Wrist_left -> Wrist_right" << "Chained Wrist L - Wrist R." << hands;
  QTest::newRow("ankle to ankle") << "Ankle L" << "Ankle R" << "Ankle_left -> Ankle_right" << "Chained Ankle L - Ankle R." << QStringList({"fly"});
  QTest::newRow("left wrist to left ankle") << "Wrist L" << "Ankle L" << "Wrist_left -> Ankle_left" << "Chained Wrist L - Ankle L." << hogtie;
  QTest::newRow("left wrist to right ankle") << "Wrist L" << "Ankle R" << "Wrist_left -> Ankle_right" << "Chained Wrist L - Ankle R." << hogtie;
  QTest::newRow("right wrist to left ankle") << "Wrist R" << "Ankle L" << "Wrist_right -> Ankle_left" << "Chained Wrist R - Ankle L." << hogtie;
  QTest::newRow("right wrist to right ankle") << "Wrist R" << "Ankle R" << "Wrist_right -> Ankle_right" << "Chained Wrist R - Ankle R." << hogtie;
  QTest::newRow("picked the other way round") << "Ankle R" << "Wrist L" << "Wrist_left -> Ankle_right" << "Chained Wrist L - Ankle R." << hogtie;
  QTest::newRow("left wrist to ceiling") << "Wrist L" << "Ceiling" << "Wrist_left -> Ceiling_ring" << "Chained Wrist L - Ceiling." << hang;
  QTest::newRow("right wrist to ceiling") << "Wrist R" << "Ceiling" << "Wrist_right -> Ceiling_ring" << "Chained Wrist R - Ceiling." << hang;
  QTest::newRow("left ankle to ceiling") << "Ankle L" << "Ceiling" << "Ankle_left -> Ceiling_ring" << "Chained Ankle L - Ceiling." << hang;
  QTest::newRow("ceiling to right ankle") << "Ceiling" << "Ankle R" << "Ankle_right -> Ceiling_ring" << "Chained Ankle R - Ceiling." << hang;
}

void TestExamples::cuffSetChainEveryPair()
{
  QFETCH(QString, from);
  QFETCH(QString, to);
  QFETCH(QString, line);
  QFETCH(QString, said);
  QFETCH(QStringList, rlv);
  const QJsonObject run = runCuffs(clicks({"Chain", from, to}));
  dump("cuffs chain " + from + " " + to, run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(saidAfterReady(run), QStringList({"Root: " + said}));
  QCOMPARE(chainLines(run), QStringList({line}));
  QCOMPARE(rlvLines(run), rlv);
  QCOMPARE(problems(run), QStringList());
  // the menu comes back and says the chain
  QVERIFY2(lastMenu(run).value(0).contains(QStringLiteral("Chains: ") + said.mid(8, said.size() - 9) + QStringLiteral(".")),
           qPrintable(lastMenu(run).value(0)));
  // the same chain twice is refused
  const QJsonObject twice = runCuffs(clicks({"Chain", from, to, "Chain", from, to}));
  QCOMPARE(saidAfterReady(twice).value(1), "Root: " + said.mid(8, said.size() - 9) + QStringLiteral(" is chained already."));
  QCOMPARE(chainLines(twice), QStringList({line}));
}

void TestExamples::cuffSetAllPairsAtOnce()
{
  // every cuff chained to every other cuff at the same time, then the ceiling: two chain points per cuff and one
  // in the ceiling ring, so nine lines at most
  const QJsonObject run = runCuffs(clicks({"Chain", "Wrist L", "Wrist R", "Chain", "Wrist L", "Ankle L", "Chain", "Wrist L", "Ankle R",
                                           "Chain", "Wrist R", "Ankle L", "Chain", "Wrist R", "Ankle R", "Chain", "Ankle L", "Ankle R",
                                           "Hang", "Chain", "Ankle L", "Ceiling", "Chain", "Ankle R", "Ceiling", "Leash", "Wrist L"}));
  dump("cuffs all pairs", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(chainLines(run), QStringList({"Ankle_left -> Ankle_right", "Ankle_right -> Wrist_left", "Ceiling_ring -> Wrist_left",
                                         "Leash_ankle_left -> Ceiling_ring", "Leash_ankle_right -> Ceiling_ring",
                                         "Leash_wrist_left -> Ankle_left", "Leash_wrist_right -> Ankle_right", "Wrist_left -> Wrist_right",
                                         "Wrist_right -> Ankle_left"}));
  QCOMPARE(saidAfterReady(run).mid(6), QStringList({"Root: Hanging by the wrists from the ceiling.", "Root: Chained Ankle L - Ceiling.",
                                                    "Root: Chained Ankle R - Ceiling.",
                                                    "Root: No free chain point on Wrist L. Unchain something first."}));
  QCOMPARE(rlvLines(run), QStringList({"edit", "fly", "rez", "sittp", "tplm", "tploc", "tplure"}));
  QCOMPARE(problems(run), QStringList());
}

void TestExamples::cuffSetHangAndRelease()
{
  const QJsonObject hung = runCuffs(clicks({"Hang"}));
  dump("cuffs hang", hung);
  QVERIFY2(hung.value("ok").toBool(), qPrintable(hung.value("error").toString()));
  QCOMPARE(saidAfterReady(hung), QStringList({"Root: Hanging by the wrists from the ceiling."}));
  QCOMPARE(chainLines(hung), QStringList({"Wrist_left -> Ceiling_ring", "Wrist_right -> Ceiling_ring"}));
  QCOMPARE(entries(hung, "animation_start", "name"), QStringList({"cuffs_hang"}));
  QCOMPARE(rlvLines(hung), QStringList({"fly", "sittp", "tplm", "tploc", "tplure"}));
  QCOMPARE(propColours(cuffsJson(), hung).last(), QStringLiteral("Ceiling_ring: 1.00 0.00 0.00 1.00"));
  QVERIFY(lastMenu(hung).value(0).endsWith(QLatin1String("Hanging: yes.")));

  const QJsonObject released = runCuffs(clicks({"Hang", "Release"}));
  dump("cuffs release", released);
  QCOMPARE(saidAfterReady(released), QStringList({"Root: Hanging by the wrists from the ceiling.", "Root: Released from the ceiling."}));
  QCOMPARE(chainLines(released), QStringList());
  QCOMPARE(entries(released, "animation_stop", "name"), QStringList({"cuffs_hang"}));
  QCOMPARE(rlvLines(released), QStringList());
  QCOMPARE(propColours(cuffsJson(), released).last(), QStringLiteral("Ceiling_ring: 0.60 0.60 0.60 1.00"));

  // Release keeps the chains between the cuffs; an ankle on the ceiling hangs without the arms-up pose
  const QJsonObject kept = runCuffs(clicks({"Chain", "Wrist L", "Wrist R", "Chain", "Ankle L", "Ceiling", "Release"}));
  QCOMPARE(chainLines(kept), QStringList({"Wrist_left -> Wrist_right"}));
  QCOMPARE(entries(kept, "animation_start", "name"), QStringList());
  QCOMPARE(rlvLines(kept), QStringList({"edit", "rez"}));
}

void TestExamples::cuffSetLockPerCuff()
{
  const QJsonObject run = runCuffs(clicks({"Lock", "Wrist L", "Lock", "Ankle R"}));
  dump("cuffs lock", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(saidAfterReady(run), QStringList({"Root: Wrist L is locked.", "Root: Ankle R is locked."}));
  QCOMPARE(rlvLines(run), QStringList({"detach", "detach:l forearm", "detach:r lower leg"}));
  QCOMPARE(propColours(cuffsJson(), run).mid(0, 5),
           QStringList({"Root: 1.00 0.00 0.00 1.00", "Wrist_left: 1.00 0.00 0.00 1.00", "Wrist_right: 0.00 1.00 0.00 1.00",
                        "Ankle_left: 0.00 1.00 0.00 1.00", "Ankle_right: 1.00 0.00 0.00 1.00"}));
  QVERIFY(lastMenu(run).value(0).contains(QLatin1String("Locked: Wrist L, Ankle R.")));

  // unlocking one keeps the other; the chain lines turn Locked while any cuff is locked
  const QJsonObject one = runCuffs(clicks({"Lock", "Wrist L", "Lock", "Ankle R", "Unlock", "Wrist L", "Chain", "Wrist L", "Wrist R"}));
  QCOMPARE(rlvLines(one), QStringList({"detach", "detach:r lower leg", "edit", "rez"}));
  QCOMPARE(LabChains::fromFolderResult(one, rlvLines(one).contains("detach")).value(0).word(), QStringLiteral("Locked"));

  const QJsonObject all = runCuffs(clicks({"Lock", "All"}));
  QCOMPARE(saidAfterReady(all), QStringList({"Root: All cuffs are locked."}));
  QCOMPARE(rlvLines(all), QStringList({"detach", "detach:l forearm", "detach:l lower leg", "detach:r forearm", "detach:r lower leg"}));
  const QJsonObject open = runCuffs(clicks({"Lock", "All", "Unlock", "All"}));
  QCOMPARE(saidAfterReady(open), QStringList({"Root: All cuffs are locked.", "Root: All cuffs are unlocked."}));
  QCOMPARE(rlvLines(open), QStringList());
  QCOMPARE(problems(open), QStringList());
}

void TestExamples::cuffSetLeashByAnotherAvatar()
{
  const QString helper = QStringLiteral("a1b2c3d4-0000-4000-8000-00000000c0ff");
  QJsonArray inputs = clicks({"Leash", "Wrist L"}, 7, helper, QStringLiteral("Test Helper"));
  inputs << dialog(4, "Free all");   // the owner's answer to the helper's menu is not heard
  const QJsonObject run = runCuffs(inputs, 5);
  dump("cuffs leash", run);
  QVERIFY2(run.value("ok").toBool(), qPrintable(run.value("error").toString()));
  QCOMPARE(saidAfterReady(run), QStringList({"Root: Leash on Wrist L, held by Test Helper."}));
  QCOMPARE(entries(run, "region_say_to", "text"), QStringList({"You hold the leash on Wrist L."}));
  QCOMPARE(entries(run, "region_say_to", "to"), QStringList({helper}));
  // the leash point emits to the helper; the 3D view draws no line to an avatar (docs/SCRIPT_LAB.md, chain lines)
  QStringList streams;
  for (const QJsonValue& v : run.value("result").toObject().value("log").toArray())
  {
    const QJsonObject e = v.toObject();
    if (e.value("function").toString() == QLatin1String("llLinkParticleSystem"))
      streams << QString::number(e.value("emitter_link").toInt()) + QStringLiteral(" ") + e.value("target").toString();
  }
  QCOMPARE(streams, QStringList({"7 " + helper}));
  QCOMPARE(chainLines(run), QStringList());
  QCOMPARE(rlvLines(run), QStringList({"tplm", "tploc", "tplure"}));
  QCOMPARE(propColours(cuffsJson(), run).at(5), QStringLiteral("Leash_wrist_left: 1.00 0.80 0.00 1.00"));
  QVERIFY(lastMenu(run).value(0).contains(QLatin1String("Leash: Wrist L (Test Helper).")));

  // Unleash
  const QJsonObject off = runCuffs(clicks({"Leash", "Wrist L", "Unleash", "Wrist L"}, 7, helper, QStringLiteral("Test Helper")));
  QCOMPARE(saidAfterReady(off), QStringList({"Root: Leash on Wrist L, held by Test Helper.", "Root: Leash off Wrist L."}));
  QCOMPARE(rlvLines(off), QStringList());
  QCOMPARE(propColours(cuffsJson(), off).at(5), QStringLiteral("Leash_wrist_left: 0.60 0.60 0.60 1.00"));
}

void TestExamples::cuffSetUnchainAndFreeAll()
{
  const QJsonObject menu = runCuffs(clicks({"Chain", "Wrist L", "Wrist R", "Chain", "Ankle L", "Ankle R", "Unchain"}));
  QCOMPARE(lastMenu(menu), QStringList({"Take off which chain?", "Wrist L - Wrist R", "Ankle L - Ankle R", "All chains", "Back"}));
  const QJsonObject one = runCuffs(clicks({"Chain", "Wrist L", "Wrist R", "Chain", "Ankle L", "Ankle R", "Unchain", "Wrist L - Wrist R"}));
  QCOMPARE(saidAfterReady(one).last(), QStringLiteral("Root: Unchained Wrist L - Wrist R."));
  QCOMPARE(chainLines(one), QStringList({"Ankle_left -> Ankle_right"}));
  QCOMPARE(rlvLines(one), QStringList({"fly"}));

  const QJsonObject freed = runCuffs(clicks({"Lock", "All", "Chain", "Wrist L", "Ankle R", "Hang", "Leash", "Ankle L", "Free all"}));
  dump("cuffs free all", freed);
  QCOMPARE(saidAfterReady(freed).last(), QStringLiteral("Root: Everything is free."));
  QCOMPARE(chainLines(freed), QStringList());
  QCOMPARE(rlvLines(freed), QStringList());
  QCOMPARE(entries(freed, "animation_stop", "name"), QStringList({"cuffs_hang"}));
  QCOMPARE(propColours(cuffsJson(), freed),
           QStringList({"Root: 0.00 1.00 0.00 1.00", "Wrist_left: 0.00 1.00 0.00 1.00", "Wrist_right: 0.00 1.00 0.00 1.00",
                        "Ankle_left: 0.00 1.00 0.00 1.00", "Ankle_right: 0.00 1.00 0.00 1.00", "Leash_wrist_left: 0.60 0.60 0.60 1.00",
                        "Leash_wrist_right: 0.60 0.60 0.60 1.00", "Leash_ankle_left: 0.60 0.60 0.60 1.00",
                        "Leash_ankle_right: 0.60 0.60 0.60 1.00", "Ceiling_ring: 0.60 0.60 0.60 1.00"}));
  QCOMPARE(problems(freed), QStringList());
}

void TestExamples::cuffSetKeepsStateAfterResetAndRewear()
{
  const QStringList setUp = {"Lock", "All", "Chain", "Wrist L", "Wrist R", "Hang", "Leash", "Ankle L"};
  const QStringList rlv = {"detach", "detach:l forearm", "detach:l lower leg", "detach:r forearm", "detach:r lower leg",
                           "edit", "fly", "rez", "sittp", "tplm", "tploc", "tplure"};
  // the leash on Ankle L goes to the owner, an avatar: the 3D view draws no line for it
  const QStringList lines = {"Leash_wrist_left -> Ceiling_ring", "Wrist_left -> Wrist_right", "Wrist_right -> Ceiling_ring"};
  const QJsonObject before = runCuffs(clicks(setUp));
  dump("cuffs set up", before);
  QCOMPARE(rlvLines(before), rlv);
  QCOMPARE(chainLines(before), lines);

  // a reset (saving the script in Second Life resets it): the state comes back from Linkset Data
  LabFolder::Linkset ls = LabFolder::open(cuffsFolder());
  LabFolder::Script resetter;
  resetter.name = QStringLiteral("test_reset");
  resetter.file = QStringLiteral("Root/test_reset.lsl");
  resetter.link = 1;
  resetter.source = QStringLiteral("default { state_entry() { llListen(5, \"\", llGetOwner(), \"reset\"); }\n"
                                   "  listen(integer c, string n, key id, string m) { llResetOtherScript(\"cuffs_main\"); } }\n");
  ls.scripts.push_back(resetter);
  QJsonArray inputs = clicks(setUp);
  inputs << QJsonObject{{"at", 11}, {"channel", 5}, {"text", "reset"}};
  const QJsonObject reset = LabFolder::run(ls, QJsonObject{{"seconds", 12}, {"inputs", inputs}});
  dump("cuffs reset", reset);
  QVERIFY2(reset.value("ok").toBool(), qPrintable(reset.value("error").toString()));
  QCOMPARE(ownerLines(reset).mid(ownerLines(reset).size() - 2),
           QStringList({"Root: Restored: Locked: Wrist L, Wrist R, Ankle L, Ankle R. Chains: Wrist L - Wrist R, Wrist L - Ceiling, "
                        "Wrist R - Ceiling. Leash: Ankle L (Owner). Hanging: yes.", Ready}));
  QCOMPARE(rlvLines(reset), rlv);
  QCOMPARE(chainLines(reset), lines);
  QCOMPARE(problems(reset), QStringList());

  // worn again (as after a relog): every restriction is sent again
  QJsonArray worn = clicks(setUp);
  worn << QJsonObject{{"at", 11}, {"type", "attach"}, {"point", 30}} << QJsonObject{{"at", 12}, {"type", "detach"}}
       << QJsonObject{{"at", 13}, {"type", "attach"}, {"point", 30}};
  const QJsonObject rewear = runCuffs(worn, 14);
  dump("cuffs rewear", rewear);
  QVERIFY2(rewear.value("ok").toBool(), qPrintable(rewear.value("error").toString()));
  QCOMPARE(rlvLines(rewear), rlv);
  QCOMPARE(chainLines(rewear), lines);
}

void TestExamples::cuffSetRecipeAfterSave()
{
  // after a save Script Lab runs the folder from the start and replays the recorded steps (job A3)
  const LabRecipe::Result recipe = LabRecipe::parse(QStringLiteral(
    "touch Wrist_left\n"
    "menu Lock\n"
    "menu All\n"
    "expect lock:cuffs\n"
    "expect colour:Wrist_left red\n"
    "menu Chain\n"
    "menu Wrist L\n"
    "menu Ankle R\n"
    "expect blocked:fly\n"
    "menu Hang\n"
    "expect blocked:tplm\n"
    "expect colour:Ceiling_ring red\n"
    "menu Release\n"
    "expect not blocked:tplm\n"
    "expect blocked:fly\n"
    "menu Free all\n"
    "expect not lock:cuffs\n"
    "expect not blocked:fly\n"
    "expect colour:Wrist_left green\n"));
  QVERIFY2(recipe.ok(), qPrintable(recipe.error));
  const LabRecipe::Replay replay = LabRecipe::replay(LabFolder::open(cuffsFolder()), QJsonObject(), *recipe.value, Rlv::Mode::Rlv);
  QStringList failures;
  for (const LabRecipe::Failure& f : replay.failures) failures << f.text;
  QVERIFY2(replay.ran, qPrintable(replay.error));
  if (!qEnvironmentVariableIsEmpty("PINGLYMOTION_EXAMPLES_DUMP"))
    for (const LabRecipe::Fact& f : replay.facts) qWarning().noquote() << "  fact" << f.key << f.state;
  QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
  QCOMPARE(replay.expects, 10);
  QCOMPARE(replay.done, replay.steps);
  QVERIFY(replay.passed());
}

void TestExamples::cuffSetClicksInTheWatchedFolder()
{
  // the way the 3D view does it: Watch folder, a click on a prop is a touch, a button of the menu box answers
  LabWatcher w;
  QVERIFY(w.watch(cuffsFolder(), QJsonObject{{"linkset", true}}).ok());
  QCOMPARE(w.linkOf(QStringLiteral("Wrist_left")), 9);
  QVERIFY(w.touch(w.linkOf(QStringLiteral("Ceiling_ring"))).ok());
  QCOMPARE(w.dialog().value("message").toString(), QStringLiteral("Cuff set\nLocked: none. Chains: none. Leash: none. Hanging: no."));
  for (const char* button : {"Chain", "Wrist L", "Ankle R", "Lock", "All", "Hang"})
    QVERIFY2(w.answerDialog(QString::fromLatin1(button)).ok(), button);
  QCOMPARE(chainLines(w.folderResult()),
           QStringList({"Leash_wrist_left -> Ceiling_ring", "Wrist_left -> Ankle_right", "Wrist_right -> Ceiling_ring"}));
  QCOMPARE(w.dialog().value("buttons").toArray().size(), MainButtons.size());
  QVERIFY(w.dialog().value("message").toString().endsWith(QLatin1String("Hanging: yes.")));
  QCOMPARE(entries(w.folderResult(), "animation_start", "name"), QStringList({"cuffs_hang"}));
  // the menu stays usable: Free all takes every line away
  QVERIFY(w.answerDialog(QStringLiteral("Free all")).ok());
  QCOMPARE(chainLines(w.folderResult()), QStringList());
  w.unwatch();
}

QTEST_MAIN(TestExamples)
#include "tst_examples.moc"
