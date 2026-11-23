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
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "prop.h"
#include "scriptlab/labfolder.h"
#include "scriptlab/labtint.h"

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

QTEST_MAIN(TestExamples)
#include "tst_examples.moc"
