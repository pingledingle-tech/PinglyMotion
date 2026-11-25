/*
 * PinglyMotion Script Lab - "improve a finished setup", part (a) (queue 1, item 5 in the jobs list, issue 74)
 *
 * The sandbox fixes a real, large setup needs, and the touchstone: the public OpenCollar collar
 * (examples/improve/OpenCollar, GPL v2, copied unchanged) loads as one object, every script compiles, it
 * starts and its main menu comes on a click.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "scriptlab/labfolder.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/lexer.h"

class LslImproveTest : public QObject
{
    Q_OBJECT
  private slots:
    void dialogButtons();
    void linksetDataFindAndDeleteFound();
    void listFindListNext();
    void setAgentRot();
    void gltfOverridesAreRecorded();
    void scriptTextLimitHoldsACollar();
    void openCollarLoads();
    void openCollarRunsAndOpensItsMenu();

  private:
    static QJsonObject run(const QString& body, const QString& events = QString());
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QString collar() { return QString(PINGLYMOTION_EXAMPLES) + "/improve/OpenCollar"; }
};

QJsonObject LslImproveTest::run(const QString& body, const QString& events)
{
  const QString source = "say(string s) { llOwnerSay(s); }\ndefault {\n state_entry() {\n" + body + "\n }\n" + events + "\n}\n";
  return lsl::Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", 2}});
}

QStringList LslImproveTest::said(const QJsonObject& answer)
{
  QStringList out;
  for (const QJsonValue& v : answer.value("log").toArray())
    if (v.toObject().value("type").toString() == "owner_say") out << v.toObject().value("text").toString();
  return out;
}

QStringList LslImproveTest::entries(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  for (const QJsonValue& v : answer.value("log").toArray())
  {
    QJsonObject e = v.toObject();
    if (e.value("type").toString() != type) continue;
    e.remove("t");
    out << QString::fromUtf8(QJsonDocument(e).toJson(QJsonDocument::Compact));
  }
  return out;
}

void LslImproveTest::dialogButtons()
{
  // a button that is only a space is a real button, as in Second Life (OpenCollar fills its menus with " ")
  QJsonObject a = run(R"lsl(llDialog(llGetOwner(), "Pick", ["A", " ", "B"], 7);)lsl");
  QCOMPARE(a.value("dialog").toObject().value("buttons").toArray(), QJsonArray({"A", " ", "B"}));
  QCOMPARE(entries(a, "debug"), QStringList());
  // an empty label, and an entry that is not a string, are refused and no dialog is shown
  const QString refused = R"({"channel":2147483647,"text":"llDialog: all buttons must have label strings","type":"debug"})";
  for (const QString& list : {QString(R"(["A", ""])"), QString(R"(["A", 5])"), QString(R"(["A", 1.5])"), QString(R"x([(key)"x"])x"),
                              QString(R"([<1, 2, 3>])")})
  {
    a = run("llDialog(llGetOwner(), \"Pick\", " + list + ", 7);");
    QVERIFY2(!a.contains("dialog"), qPrintable(list));
    QCOMPARE(entries(a, "debug"), QStringList({refused}));
  }
  // the length rule still counts a long label of spaces
  a = run(R"lsl(llDialog(llGetOwner(), "Pick", ["                         "], 7);)lsl");
  QCOMPARE(entries(a, "debug"), QStringList({R"({"channel":2147483647,"text":"llDialog: button labels must be 24 or fewer characters long","type":"debug"})"}));
}

void LslImproveTest::linksetDataFindAndDeleteFound()
{
  // the wiki's example names: matches anywhere in the name, alphabetical, start from 0, count below 1 means all
  const QJsonObject a = run(R"lsl(
    list names = ["ThingC_data", "ThingA_config", "ThingB_data", "ThingA_data", "ThingC_config", "ThingB_config"];
    integer i;
    for (i = 0; i < 6; ++i) llLinksetDataWrite(llList2String(names, i), "v");
    say(llList2CSV(llLinksetDataFindKeys("Thing", 0, 1)));
    say(llList2CSV(llLinksetDataFindKeys("Thing", 3, 1)));
    say(llList2CSV(llLinksetDataFindKeys("_data", 1, 10)));
    say(llList2CSV(llLinksetDataFindKeys("^thinga", 0, 0)));
    say(llList2CSV(llLinksetDataFindKeys("(?i)^thinga", 0, 0)));
    say(llList2CSV(llLinksetDataFindKeys("Thing", 9, 0)));
    say(llList2CSV(llLinksetDataFindKeys("(", 0, 0)));
    say(llList2CSV(llLinksetDataDeleteFound("_config$", "")));
    say(llList2CSV(llLinksetDataListKeys(0, 0)));
    say(llList2CSV(llLinksetDataDeleteFound("nothing", "")));
  )lsl", R"lsl(linkset_data(integer action, string name, string value) { if (action == LINKSETDATA_MULTIDELETE) say("multi " + name + " [" + value + "]"); })lsl");
  QCOMPARE(said(a), QStringList({"ThingA_config", "ThingB_data", "ThingB_data, ThingC_data", "", "ThingA_config, ThingA_data", "", "",
                                 "3, 0", "ThingA_data, ThingB_data, ThingC_data", "0, 0",
                                 "multi ThingA_config,ThingB_config,ThingC_config []"}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"llLinksetDataFindKeys: the pattern is not a regular expression of at most 800 characters, so nothing matched.","type":"debug"})"}));
  QStringList found;
  for (const QString& e : entries(a, "linkset_data"))
    if (e.contains("delete_found")) found << e;
  QCOMPARE(found, QStringList({R"({"deleted":3,"op":"delete_found","pattern":"_config$","type":"linkset_data","used":36})",
                               R"({"deleted":0,"op":"delete_found","pattern":"nothing","type":"linkset_data","used":36})"}));
}

void LslImproveTest::listFindListNext()
{
  // the wiki's example list and answers
  const QJsonObject a = run(R"lsl(
    list src = ["A", 0, "B", 1, "C", 2, "A", 0, "A", 1, "A", "A", 0, <1, 2, 3>, "c"];
    say(llList2CSV([llListFindListNext(src, ["A", 0], 0), llListFindListNext(src, ["A", 0], 1), llListFindListNext(src, ["A", 0], 2),
                    llListFindListNext(src, ["A", 0], 3), llListFindListNext(src, ["A", 0], -1), llListFindListNext(src, ["A", 0], -2),
                    llListFindListNext(src, ["A", 0], -3), llListFindListNext(src, ["A", 0], -4)]));
    say(llList2CSV([llListFindListNext(src, [3], 0), llListFindListNext(src, [<1, 2, 3>], -1), llListFindListNext(src, ["c"], 0),
                    llListFindListNext(src, ["c"], -1), llListFindListNext(src, [], 0), llListFindListNext([], ["A"], 0),
                    llListFindListNext(src, ["a"], 0), llListFindListNext(src, ["A", 0.0], 0)]));
  )lsl");
  QCOMPARE(said(a), QStringList({"0, 6, 11, -1, 11, 6, 0, -1", "-1, 13, 14, 14, 0, -1, -1, -1"}));
}

void LslImproveTest::setAgentRot()
{
  // without PERMISSION_TRIGGER_ANIMATION: a debug line, nothing is turned
  QJsonObject a = run("llSetAgentRot(llEuler2Rot(<0, 0, 90> * DEG_TO_RAD), 0);");
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"Script trying to turn the avatar but PERMISSION_TRIGGER_ANIMATION permission not set","type":"debug"})"}));
  QCOMPARE(entries(a, "agent_rot"), QStringList());
  // with it: only the turn about the vertical axis is kept (the roll of 30 degrees is dropped)
  a = run("llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION);",
          "run_time_permissions(integer p) { if (p & PERMISSION_TRIGGER_ANIMATION) llSetAgentRot(llEuler2Rot(<30, 0, 90> * DEG_TO_RAD), 0); }");
  QCOMPARE(entries(a, "agent_rot"), QStringList({
    R"({"avatar":"a2e76fcd-9360-4f6d-a924-000000000001","degrees":90,"rotation":[0,0,0.70711,0.70711],"type":"agent_rot"})"}));
  QCOMPARE(entries(a, "unsupported"), QStringList());
}

void LslImproveTest::gltfOverridesAreRecorded()
{
  // GLTF material overrides are not drawn: the call is recorded and the script runs on
  const QJsonObject a = run(R"lsl(llSetLinkGLTFOverrides(LINK_THIS, ALL_SIDES, [OVERRIDE_GLTF_BASE_ALPHA, 0.5]); say("after");)lsl");
  QCOMPARE(said(a), QStringList({"after"}));
  QCOMPARE(entries(a, "unsupported"), QStringList());
  QCOMPARE(a.value("warnings").toArray().size(), 0);
  const QStringList recorded = entries(a, "recorded_only");
  QCOMPARE(recorded.size(), 1);
  QVERIFY2(recorded[0].contains("\"function\":\"llSetLinkGLTFOverrides\""), qPrintable(recorded[0]));
}

void LslImproveTest::scriptTextLimitHoldsACollar()
{
  // one run takes 1 MiB of script text in all (a whole collar is about 0.7 MB); each script still at most 256 KiB
  QCOMPARE(lsl::Control::MAX_TOTAL_SOURCE_CHARS, 1048576);
  QCOMPARE(LabFolder::MAX_TOTAL_BYTES, qint64(1048576));
  QCOMPARE(lsl::Lexer::MAX_SOURCE_CHARS, 262144);
  const QString pad = "default { state_entry() { } }\n//" + QString(200000, 'x');
  QJsonArray scripts;
  for (int i = 0; i < 5; ++i) scripts.append(QJsonObject{{"name", QString("s%1").arg(i)}, {"source", pad}});
  QJsonObject a = lsl::Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1},
                                                {"objects", QJsonArray{QJsonObject{{"name", "Big"}, {"scripts", scripts}}}}});
  QVERIFY2(a.value("ok").toBool(), qPrintable(a.value("error").toString()));
  scripts.append(QJsonObject{{"name", "s5"}, {"source", pad}});
  a = lsl::Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1},
                                    {"objects", QJsonArray{QJsonObject{{"name", "Big"}, {"scripts", scripts}}}}});
  QVERIFY(!a.value("ok").toBool());
  QCOMPARE(a.value("error").toString(), QString("The scripts together are too long (at most 1048576 characters)."));
}

void LslImproveTest::openCollarLoads()
{
  const LabFolder::Linkset ls = LabFolder::open(collar());
  QVERIFY2(ls.ok(), qPrintable(QJsonDocument(ls.toJson()).toJson(QJsonDocument::Compact)));
  QCOMPARE(ls.name, QString("OpenCollar"));
  QCOMPARE(int(ls.prims.size()), 1);
  QStringList names;
  qint64 bytes = 0;
  for (const LabFolder::Script& s : ls.scripts) { names << s.name; bytes += s.source.toUtf8().size(); }
  QCOMPARE(names, QStringList({"oc_addons", "oc_anim", "oc_api", "oc_auth", "oc_bookmarks", "oc_core", "oc_couples", "oc_dialog",
                               "oc_folders", "oc_folders_locks", "oc_label", "oc_leash", "oc_meshlabel", "oc_particle", "oc_relay",
                               "oc_resizer", "oc_rlvextension", "oc_rlvsuite", "oc_rlvsys", "oc_settings", "oc_states", "oc_themes",
                               "oc_titler"}));
  // more than the old limit of 512 KiB, less than the new one
  QVERIFY2(bytes > 524288 && bytes < LabFolder::MAX_TOTAL_BYTES, qPrintable(QString::number(bytes)));
  QCOMPARE(int(ls.notecards.size()), 0);   // LICENSE has no notecard extension: it is not part of the object
  // every script compiles, with no warning: every function it calls is known and runs in the sandbox
  const QJsonObject check = LabFolder::check(ls);
  QCOMPARE(check.value("errors").toArray(), QJsonArray());
  QCOMPARE(check.value("warnings").toArray(), QJsonArray());
  QVERIFY(check.value("valid").toBool());
}

void LslImproveTest::openCollarRunsAndOpensItsMenu()
{
  // the collar starts (about 12 simulated seconds), the owner clicks it at 20 s and answers the menu at 22 s
  const QJsonArray inputs{QJsonObject{{"at", 20}, {"type", "touch"}},
                          QJsonObject{{"at", 22}, {"type", "dialog"}, {"button", "Access"}}};
  const QJsonObject answer = LabFolder::run(LabFolder::open(collar()), QJsonObject{{"seconds", 21}, {"inputs", inputs}});
  QVERIFY2(answer.value("ok").toBool(), qPrintable(answer.value("error").toString()));
  const QJsonObject result = answer.value("result").toObject();
  // nothing stopped for a missing feature, an error or a limit; two scripts remove themselves, as they do in Second Life
  QStringList stops;
  for (const QJsonValue& v : result.value("log").toArray())
  {
    const QJsonObject e = v.toObject();
    const QString type = e.value("type").toString();
    if (type == "unsupported" || type == "error" || type == "limit" || type == "removed")
      stops << type + " " + e.value("script").toString();
  }
  QCOMPARE(stops, QStringList({"removed oc_auth", "removed oc_label"}));
  // the main menu, to the owner who clicked
  QJsonObject dialog = result.value("dialog").toObject();
  QCOMPARE(dialog.value("t").toDouble(), 20.01);
  QCOMPARE(dialog.value("message").toString(), QString("\nOpenCollar 8.4.0000"));
  QCOMPARE(dialog.value("to").toString(), QString("a2e76fcd-9360-4f6d-a924-000000000001"));
  QCOMPARE(dialog.value("buttons").toArray(),
           QJsonArray({"Help/About", " ", " ", "Apps", "Access", "Settings", QString::fromUtf8("□ Lock"), "Animations", "Addons"}));
  QVERIFY(dialog.value("channel").toInt() != 0);

  // run on to 23 s: the answer "Access" opens the access menu
  const QJsonObject later = LabFolder::run(LabFolder::open(collar()), QJsonObject{{"seconds", 23}, {"inputs", inputs}});
  dialog = later.value("result").toObject().value("dialog").toObject();
  QCOMPARE(dialog.value("t").toDouble(), 22.01);
  QVERIFY2(dialog.value("message").toString().startsWith("\nOpenCollar Access Controls\n"), qPrintable(dialog.value("message").toString()));
  QCOMPARE(dialog.value("buttons").toArray(),
           QJsonArray({"Runaway", "Access List", "BACK", QString::fromUtf8("\u25a1 Group"), QString::fromUtf8("\u25a1 Public"),
                       QString::fromUtf8("\u25a1 Wearer Trust"), "- Owner", "- Trust", "- Block", "+ Owner", "+ Trust", "+ Block"}));
}

QTEST_GUILESS_MAIN(LslImproveTest)
#include "tst_lslimprove.moc"
