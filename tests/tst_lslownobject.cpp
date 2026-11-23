/*
 * PinglyMotion - tests for the cheap reads and state of the object itself (Script Lab, gap list 2, step 2)
 *
 * llDie, llGetVel, llGetMass, llSetScale, llSetLocalRot, llGetLinkName, llGetLinkKey, llDetectedLinkNumber,
 * llGetObjectPermMask, llGetInventoryPermMask, llGetObjectDetails, llSameGroup, llSetScriptState,
 * llGetScriptState, llResetOtherScript, llGetCreator and llGetUsername. Every expected value is exact and was
 * worked out by hand from the rules in docs/CONTROL.md ("Reads and state of the object itself"): the mass is
 * x * y * z * 10 per prim, the masks are fixed numbers, and the key of child prim 2 of the object
 * c0ffee00-0000-4000-8000-000000000002 is the MD5 of "c0ffee00-0000-4000-8000-000000000002/link/2", computed
 * outside the program.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const OBJECT = "c0ffee00-0000-4000-8000-000000000002";
  const char* const CHILD2 = "553857e6-0a71-ceb3-1b8c-3efe18b3d789";
  const char* const GUEST = "11111111-2222-3333-4444-555555555555";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";
  const char* const GROUP = "9a0c0000-0000-4000-8000-000000000001";
}

class LslOwnObjectTest : public QObject
{
  Q_OBJECT

  private slots:
    void velocityMassAndScale();
    void localRotation();
    void linkNamesAndKeys();
    void detectedLinkNumber();
    void permissionMasks();
    void creatorUsernameAndGroup();
    void objectDetailsOfItself();
    void objectDetailsOfOthers();
    void scriptState();
    void resetOtherScript();
    void dieStopsTheObject();
    void dieInAnAttachmentDoesNothing();
    void checkerHasNoWarnings();
    void constantsMatchSecondLife();

  private:
    static QJsonObject runSource(const QString& source, const QJsonArray& inputs = QJsonArray(), double seconds = 2);
    static QJsonObject runObjects(const QJsonArray& objects, const QJsonArray& inputs = QJsonArray(), double seconds = 2);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
};

QJsonObject LslOwnObjectTest::runSource(const QString& source, const QJsonArray& inputs, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"inputs", inputs}, {"seconds", seconds}});
}

QJsonObject LslOwnObjectTest::runObjects(const QJsonArray& objects, const QJsonArray& inputs, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", objects}, {"inputs", inputs}, {"seconds", seconds}});
}

QStringList LslOwnObjectTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslOwnObjectTest::entries(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    QJsonObject e = v.toObject();
    if (e["type"].toString() != type) continue;
    e.remove("t");
    out << QString::fromUtf8(QJsonDocument(e).toJson(QJsonDocument::Compact));
  }
  return out;
}

void LslOwnObjectTest::velocityMassAndScale()
{
  const QJsonObject a = runSource(R"lsl(
    default { state_entry() {
      llOwnerSay((string)llGetVel() + " " + (string)llGetMass());
      llSetScale(<1, 2, 0.001>);
      llOwnerSay((string)llGetScale() + " " + (string)llGetMass());
      llSetScale(<100, 1, 1>);
      llOwnerSay((string)llGetScale() + " " + (string)llList2Vector(llGetPrimitiveParams([PRIM_SIZE]), 0));
      llSetStatus(STATUS_PHYSICS, TRUE);
      llSetScale(<2, 2, 2>);
      llOwnerSay((string)llGetScale());
    } }
  )lsl");
  const QStringList expected1({
    "<0.00000, 0.00000, 0.00000> 1.250000",
    "<1.00000, 2.00000, 0.01000> 0.200000",
    "<64.00000, 1.00000, 1.00000> <64.00000, 1.00000, 1.00000>",
    "<64.00000, 1.00000, 1.00000>"});
  QCOMPARE(said(a), expected1);
  const QStringList expected2({
    R"({"link":0,"op":"size","size":[1,2,0.01],"type":"prim"})",
    R"({"link":0,"op":"size","size":[64,1,1],"type":"prim"})",
    R"({"link":0,"op":"status","status":15,"type":"prim"})"});
  QCOMPARE(entries(a, "prim"), expected2);

  // three prims of 0.5 m: 3 * 1.25; a child's size counts with its own size
  const QJsonObject b = runObjects(QJsonArray{QJsonObject{{"name", "box"}, {"prims", 3}, {"scripts", QJsonArray{
    QJsonObject{{"name", "main"}, {"source", R"lsl(default { state_entry() {
      llOwnerSay((string)llGetMass());
      llSetLinkPrimitiveParamsFast(2, [PRIM_SIZE, <1, 1, 1>]);
      llOwnerSay((string)llGetMass());
    } })lsl"}}}}}});
  const QStringList expected3({"3.750000", "12.500000"});
  QCOMPARE(said(b), expected3);
}

void LslOwnObjectTest::localRotation()
{
  const QJsonObject a = runObjects(QJsonArray{QJsonObject{{"name", "box"}, {"prims", 2}, {"scripts", QJsonArray{
    QJsonObject{{"name", "child"}, {"link", 2}, {"source", R"lsl(default { state_entry() {
      llSetLocalRot(llEuler2Rot(<0, 0, PI_BY_TWO>));
      llOwnerSay((string)llGetLocalRot() + " " + (string)llGetRot());
    } })lsl"}},
    QJsonObject{{"name", "root"}, {"link", 1}, {"source", R"lsl(default { state_entry() {
      llSetLocalRot(llEuler2Rot(<PI_BY_TWO, 0, 0>));
      llOwnerSay((string)llGetRot());
    } })lsl"}}}}}});
  // the child's local rotation is set; its region rotation is local * root (the root still has none when it runs)
  // the root's llSetLocalRot is the object's rotation, as llSetRot
  const QStringList expected4({
    "<0.00000, 0.00000, 0.70711, 0.70711> <0.00000, 0.00000, 0.70711, 0.70711>",
    "<0.70711, 0.00000, 0.00000, 0.70711>"});
  QCOMPARE(said(a), expected4);
  const QStringList expected5({
    R"({"link":2,"object":"box","op":"rotation","rotation":[0,0,0.70711,0.70711],"script":"child","type":"prim"})",
    R"({"link":1,"object":"box","op":"rotation","rotation":[0.70711,0,0,0.70711],"script":"root","type":"prim"})"});
  QCOMPARE(entries(a, "prim"), expected5);
}

void LslOwnObjectTest::linkNamesAndKeys()
{
  // a single prim: 0, 1 and LINK_THIS are the prim; anything else is NULL_KEY, for llGetLinkName too
  const QJsonObject a = runSource(R"lsl(default { state_entry() {
    llOwnerSay(llGetLinkName(0) + "|" + llGetLinkName(1) + "|" + llGetLinkName(LINK_THIS) + "|" + llGetLinkName(2) + "|" + llGetLinkName(LINK_SET));
    llOwnerSay((string)llGetLinkKey(0) + "|" + (string)llGetLinkKey(LINK_THIS) + "|" + (string)llGetLinkKey(2));
  } })lsl");
  const QStringList expected6({
    QString("Object|Object|Object|%1|%1").arg(NULL_KEY),
    QString("%1|%1|%2").arg(OBJECT, NULL_KEY)});
  QCOMPARE(said(a), expected6);

  // three prims and an avatar sitting: the avatar is link 4, after the prims
  const QString main = R"lsl(default {
    state_entry() {
      llSetLinkPrimitiveParamsFast(2, [PRIM_NAME, "door"]);
      llOwnerSay(llGetLinkName(1) + "|" + llGetLinkName(2) + "|" + llGetLinkName(3) + "|" + llGetLinkName(4));
      llOwnerSay((string)llGetLinkKey(1) + "|" + (string)llGetLinkKey(2) + "|" + (string)llGetLinkKey(0));
    }
    changed(integer c) {
      llOwnerSay((string)llGetNumberOfPrims() + "|" + llGetLinkName(4) + "|" + (string)llGetLinkKey(4) + "|" + llGetLinkName(5));
    }
  })lsl";
  const QJsonObject b = runObjects(QJsonArray{QJsonObject{{"name", "box"}, {"prims", 3}, {"scripts", QJsonArray{
    QJsonObject{{"name", "main"}, {"source", main}}}}}},
    QJsonArray{QJsonObject{{"at", 1}, {"type", "sit"}, {"object", "box"}, {"id", GUEST}, {"name", "Guest"}}});
  const QStringList expected7({
    QString("box|door|Object|%1").arg(NULL_KEY),
    QString("%1|%2|%3").arg(OBJECT, CHILD2, NULL_KEY),
    QString("4|Guest|%1|%2").arg(GUEST, NULL_KEY)});
  QCOMPARE(said(b), expected7);
}

void LslOwnObjectTest::detectedLinkNumber()
{
  const QString report = R"lsl(default { touch_start(integer n) {
    llOwnerSay(llGetScriptName() + " " + (string)llDetectedLinkNumber(0) + " " + (string)llDetectedLinkNumber(1) + " " + (string)llDetectedLinkNumber(-1));
  } })lsl";
  // a touch on the child with a script: that script; a touch on a prim without one goes to the root, with the prim touched
  const QJsonObject a = runObjects(QJsonArray{QJsonObject{{"name", "box"}, {"prims", 3}, {"scripts", QJsonArray{
    QJsonObject{{"name", "root"}, {"link", 1}, {"source", report}}, QJsonObject{{"name", "child"}, {"link", 2}, {"source", report}}}}}},
    QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"object", "box"}, {"link", 2}},
               QJsonObject{{"at", 2}, {"type", "touch"}, {"object", "box"}, {"link", 3}},
               QJsonObject{{"at", 3}, {"type", "touch"}, {"object", "box"}}}, 4);
  const QStringList expected8({"child 2 0 0", "root 3 0 0", "root 1 0 0"});
  QCOMPARE(said(a), expected8);
  // a single prim is link 0
  const QJsonObject b = runSource(report, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}});
  const QStringList expected9({"Script 0 0 0"});
  QCOMPARE(said(b), expected9);
  // outside a touch (a sensor has no link) it is 0
  const QJsonObject c = runSource(R"lsl(default {
    state_entry() { llSensor("", NULL_KEY, AGENT, 10, PI); }
    sensor(integer n) { llOwnerSay((string)n + " " + (string)llDetectedLinkNumber(0)); }
  })lsl");
  const QStringList expected10({"1 0"});
  QCOMPARE(said(c), expected10);
}

void LslOwnObjectTest::permissionMasks()
{
  const QJsonObject a = runSource(R"lsl(default { state_entry() {
    llOwnerSay(llList2CSV([llGetObjectPermMask(MASK_BASE), llGetObjectPermMask(MASK_OWNER), llGetObjectPermMask(MASK_GROUP),
      llGetObjectPermMask(MASK_EVERYONE), llGetObjectPermMask(MASK_NEXT), llGetObjectPermMask(MASK_OWNER | MASK_COMBINED),
      llGetObjectPermMask(7)]));
    llOwnerSay(llList2CSV([llGetInventoryPermMask("Script", MASK_OWNER), llGetInventoryPermMask("Script", MASK_NEXT),
      llGetInventoryPermMask("card", MASK_EVERYONE), llGetInventoryPermMask("nothing", MASK_OWNER)]));
    llOwnerSay((string)((llGetObjectPermMask(MASK_NEXT) & PERM_TRANSFER) != 0) + (string)((llGetObjectPermMask(MASK_NEXT) & PERM_COPY) != 0));
  } })lsl");
  const QStringList expected11({
    "2147483647, 2147483647, 0, 0, 532480, 2147483647, 0",
    "2147483647, 532480, 0, 0",
    "10"});
  QCOMPARE(said(a), expected11);
  const QStringList expected12({R"({"channel":2147483647,"text":"No item named 'card'","type":"debug"})",
                                             R"({"channel":2147483647,"text":"No item named 'nothing'","type":"debug"})"});
  QCOMPARE(entries(a, "debug"), expected12);
  // an item given in the inventory counts too
  const QJsonObject b = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1},
    {"inventory", QJsonArray{QJsonObject{{"name", "card"}, {"lines", QJsonArray{"a"}}}}},
    {"source", R"lsl(default { state_entry() { llOwnerSay((string)llGetInventoryPermMask("card", MASK_BASE)); } })lsl"}});
  const QStringList expected13({"2147483647"});
  QCOMPARE(said(b), expected13);
  QCOMPARE(entries(b, "debug"), QStringList());
}

void LslOwnObjectTest::creatorUsernameAndGroup()
{
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1},
    {"avatars", QJsonArray{QJsonObject{{"name", "Ann Smith"}, {"key", GUEST}, {"in_group", true}},
                           QJsonObject{{"name", "Bob Resident"}, {"key", "22222222-2222-4222-8222-222222222222"}}}},
    {"source", QString(
      "default { state_entry() {\n"
      "  llOwnerSay((string)llGetCreator());\n"
      "  llOwnerSay(llList2CSV([llGetUsername(llGetOwner()), llGetUsername(\"%1\"), llGetUsername(\"22222222-2222-4222-8222-222222222222\"),\n"
      "    \"[\" + llGetUsername(llGetKey()) + \"]\", \"[\" + llGetUsername(NULL_KEY) + \"]\"]));\n"
      "  llOwnerSay(llList2CSV([llSameGroup(NULL_KEY), llSameGroup(llGetKey()), llSameGroup(llGetOwner()), llSameGroup(\"%1\"),\n"
      "    llSameGroup(\"22222222-2222-4222-8222-222222222222\"), llSameGroup(\"%2\"), llSameGroup(\"12345678-1234-1234-1234-123456789012\")]));\n"
      "} }\n").arg(GUEST, GROUP)}});
  const QStringList expected{OWNER, "owner, ann.smith, bob, [], []", "0, 1, 0, 1, 0, 1, 0"};
  QCOMPARE(said(a), expected);
}

void LslOwnObjectTest::objectDetailsOfItself()
{
  const QJsonObject a = runSource(R"lsl(default { state_entry() {
    list all;
    integer i;
    for (i = 1; i <= 49; ++i) all += i;
    // 50 (health) and 56 (volume detect) have canned values now; 99 is unknown
    all += [50, 56, 99, "x", -1, 1.0];
    list d = llGetObjectDetails(llGetKey(), all);
    llOwnerSay((string)llGetListLength(d));
    llOwnerSay(llDumpList2String(llList2List(d, 0, 24), "|"));
    llOwnerSay(llDumpList2String(llList2List(d, 25, -1), "|"));
    llOwnerSay(llList2CSV([llGetListEntryType(d, 2), llGetListEntryType(d, 3), llGetListEntryType(d, 11), llGetListEntryType(d, 12), llGetListEntryType(d, 6)]));
  } })lsl");
  const QStringList expected14({
    "53",
    QString("Object||<128.00000, 128.00000, 20.00000>|<0.00000, 0.00000, 0.00000, 1.00000>|<0.00000, 0.00000, 0.00000>|%1|%2|%1|1|1|65536"
            "|0.000000|1|1.000000|1.000000|1.000000|0.000000|%3|0|0|0|0|0|0|0.000000").arg(OWNER, GROUP, OBJECT),
    QString("-1.000000|%1|0|<0.00000, 0.00000, 0.00000>|1|1|%2||0|0|2026-01-01T00:00:00.000000Z|0|0|0|0|-1|3|1.250000|"
            "|2026-01-01T00:00:00.000000Z|0|<0.50000, 0.50000, 0.50000>|<1.00000, 1.00000, 1.00000>|1.000000|0.000000|0|-1|-1").arg(NULL_KEY, OWNER),
    "5, 6, 2, 1, 4"});
  QCOMPARE(said(a), expected14);
}

void LslOwnObjectTest::objectDetailsOfOthers()
{
  const QString ask = QString(R"lsl(default { state_entry() {
    llSetTimerEvent(1);
  }
  timer() {
    llSetTimerEvent(0);
    list flags = [OBJECT_NAME, OBJECT_DESC, OBJECT_POS, OBJECT_OWNER, OBJECT_GROUP, OBJECT_CREATOR, OBJECT_ROOT, OBJECT_ATTACHED_POINT,
      OBJECT_PATHFINDING_TYPE, OBJECT_RUNNING_SCRIPT_COUNT, OBJECT_TOTAL_SCRIPT_COUNT, OBJECT_PRIM_COUNT, OBJECT_SIT_COUNT,
      OBJECT_RENDER_WEIGHT, OBJECT_BODY_SHAPE_TYPE, OBJECT_ATTACHED_SLOTS_AVAILABLE, OBJECT_ACCOUNT_LEVEL, OBJECT_SCALE,
      OBJECT_LINK_NUMBER, OBJECT_MASS, OBJECT_TEXT];
    llOwnerSay(llDumpList2String(llGetObjectDetails("c0ffee00-0000-4000-8000-000000000003", flags), "|"));
    llOwnerSay(llDumpList2String(llGetObjectDetails(llGetOwner(), flags), "|"));
    llOwnerSay(llDumpList2String(llGetObjectDetails("%1", flags), "|"));
    llOwnerSay((string)llGetListLength(llGetObjectDetails(NULL_KEY, flags)) + " " + (string)llGetListLength(llGetObjectDetails("12345678-1234-1234-1234-123456789012", flags)));
  } })lsl").arg(GUEST);
  const QString other = R"lsl(default { state_entry() { llSetText("hi", <1, 0, 0>, 0.5); } })lsl";
  const QJsonObject a = runObjects(QJsonArray{
    QJsonObject{{"name", "asker"}, {"source", ask}},
    QJsonObject{{"name", "chair"}, {"description", "a chair"}, {"position", QJsonArray{130, 128, 20}}, {"prims", 2}, {"scripts", QJsonArray{
      QJsonObject{{"name", "a"}, {"source", other}}, QJsonObject{{"name", "b"}, {"link", 2}, {"source", "default { state_entry() { llSetScriptState(\"b\", FALSE); } }"}}}}},
    QJsonObject{{"name", "cuff"}, {"attached", 30}, {"source", "default { state_entry() { } }"}}},
    QJsonArray{QJsonObject{{"at", 0.5}, {"type", "sit"}, {"object", "chair"}, {"id", GUEST}, {"name", "Guest"}}});
  const QStringList expected15({
    QString("chair|a chair|<130.00000, 128.00000, 20.00000>|%1|%2|%1|c0ffee00-0000-4000-8000-000000000003|0|0|1|2|2|1|0|-1.000000|0|-1"
            "|<0.50000, 0.50000, 0.50000>|1|2.500000|hi").arg(OWNER, GROUP),
    QString("Owner||<128.00000, 128.00000, 20.00000>|%1|%2|%2|%1|0|1|1|1|0|0|-1|0.000000|37|0|<0.00000, 0.00000, 0.00000>|0|0.000000|")
      .arg(OWNER, NULL_KEY),
    QString("Guest||<128.00000, 128.00000, 20.00000>|%1|%2|%2|c0ffee00-0000-4000-8000-000000000003|0|1|0|0|0|0|-1|0.000000|38|0|<0.00000, 0.00000, 0.00000>|0|0.000000|")
      .arg(GUEST, NULL_KEY),
    "0 0"});
  QCOMPARE(said(a), expected15);
}

void LslOwnObjectTest::scriptState()
{
  const QString main = R"lsl(default {
    state_entry() {
      llOwnerSay("states " + (string)llGetScriptState("main") + (string)llGetScriptState("ticker") + (string)llGetScriptState("child") + (string)llGetScriptState("nobody"));
      llSetTimerEvent(2.5);
    }
    timer() {
      llSetTimerEvent(0);
      llSetScriptState("ticker", FALSE);
      llSetScriptState("ticker", FALSE);
      llOwnerSay("paused " + (string)llGetScriptState("ticker"));
      llMessageLinked(LINK_THIS, 1, "lost", "");
      llSetScriptState("ticker", TRUE);
      llOwnerSay("running " + (string)llGetScriptState("ticker"));
      llSetScriptState("nobody", TRUE);
    }
  })lsl";
  const QString ticker = R"lsl(integer n;
  default {
    state_entry() { llSetTimerEvent(1); }
    timer() { ++n; llOwnerSay("tick " + (string)n); }
    link_message(integer s, integer num, string str, key id) { llOwnerSay("heard " + str); }
  })lsl";
  const QJsonObject a = runObjects(QJsonArray{QJsonObject{{"name", "box"}, {"prims", 2}, {"scripts", QJsonArray{
    QJsonObject{{"name", "main"}, {"source", main}}, QJsonObject{{"name", "ticker"}, {"source", ticker}},
    QJsonObject{{"name", "child"}, {"link", 2}, {"source", "default { state_entry() { } }"}}}}}}, QJsonArray(), 5);
  // the right side of + runs first in LSL, so "nobody" is asked before "child"
  // ticks at 1 and 2; paused and resumed at 2.5 (the link message sent while paused is lost); its timer counts again
  // from 2.5, so the next ticks come at 3.5 and 4.5, and the count (its memory) was kept
  const QStringList expected16({"states 1100", "tick 1", "tick 2", "paused 0", "running 1", "tick 3", "tick 4"});
  QCOMPARE(said(a), expected16);
  const QStringList expected17({
    R"({"object":"box","running":false,"script":"main","target":"ticker","type":"script_state"})",
    R"({"object":"box","running":true,"script":"main","target":"ticker","type":"script_state"})"});
  QCOMPARE(entries(a, "script_state"), expected17);
  const QStringList expected18({
    R"({"channel":2147483647,"object":"box","script":"main","text":"llGetScriptState: there is no script named \"nobody\" in this prim.","type":"debug"})",
    R"({"channel":2147483647,"object":"box","script":"main","text":"llGetScriptState: there is no script named \"child\" in this prim.","type":"debug"})",
    R"({"channel":2147483647,"object":"box","script":"main","text":"llSetScriptState: there is no script named \"nobody\" in this prim.","type":"debug"})"});
  QCOMPARE(entries(a, "debug"), expected18);

  // a script stops itself: the rest of the event runs, then nothing more (its timer does not fire); the answer says so
  const QJsonObject b = runSource(R"lsl(default {
    state_entry() { llSetTimerEvent(1); llSetScriptState(llGetScriptName(), FALSE); llOwnerSay("after " + (string)llGetScriptState(llGetScriptName())); }
    timer() { llOwnerSay("tick"); }
    touch_start(integer n) { llOwnerSay("touched"); }
  })lsl", QJsonArray{QJsonObject{{"at", 1.5}, {"type", "touch"}}}, 3);
  const QStringList expected19({"after 0"});
  QCOMPARE(said(b), expected19);
  const QStringList expected20({R"({"running":false,"target":"Script","type":"script_state"})"});
  QCOMPARE(entries(b, "script_state"), expected20);
  const QJsonObject o = b["objects"].toArray()[0].toObject();
  QCOMPARE(o["running"].toBool(true), false);
  QCOMPARE(o["stopped"].toBool(), false);
}

void LslOwnObjectTest::resetOtherScript()
{
  const QString main = R"lsl(default {
    state_entry() { llSetTimerEvent(1); }
    timer() {
      if (llGetTime() < 1.5) { llResetOtherScript("counter"); return; }
      llSetTimerEvent(0);
      llSetScriptState("counter", FALSE);
      llResetOtherScript("counter");
      llResetOtherScript("missing");
    }
  })lsl";
  const QString counter = R"lsl(integer n = 5;
  default { state_entry() { ++n; llOwnerSay("counter " + (string)n); } })lsl";
  const QJsonObject a = runObjects(QJsonArray{QJsonObject{{"name", "box"}, {"scripts", QJsonArray{
    QJsonObject{{"name", "main"}, {"source", main}}, QJsonObject{{"name", "counter"}, {"source", counter}}}}}}, QJsonArray(), 3);
  // the reset runs the counter's state_entry again with its globals set back; a script that is not running is not reset
  const QStringList expected21({"counter 6", "counter 6"});
  QCOMPARE(said(a), expected21);
  const QStringList expected22({R"({"object":"box","script":"main","target":"counter","type":"reset_other"})"});
  QCOMPARE(entries(a, "reset_other"), expected22);
  const QStringList expected23({R"({"object":"box","script":"counter","type":"reset"})"});
  QCOMPARE(entries(a, "reset"), expected23);
  const QStringList expected24({
    R"({"channel":2147483647,"object":"box","script":"main","text":"llResetOtherScript: there is no script named \"missing\" in this prim.","type":"debug"})"});
  QCOMPARE(entries(a, "debug"), expected24);

  // a script can reset itself with it
  const QJsonObject b = runSource(R"lsl(integer n;
    default { state_entry() { ++n; llOwnerSay((string)n); if (llGetUnixTime() == 1767225600) { llSleep(1); llResetOtherScript(llGetScriptName()); } } })lsl");
  const QStringList expected25({"1", "1"});
  QCOMPARE(said(b), expected25);
  const QStringList expected26({R"({"type":"reset"})"});
  QCOMPARE(entries(b, "reset"), expected26);
}

void LslOwnObjectTest::dieStopsTheObject()
{
  RlvState rlv;
  Session session;
  const QString main = R"lsl(default {
    state_entry() { llSetTimerEvent(1); }
    timer() { llOwnerSay("bye"); llDie(); llOwnerSay("never"); }
  })lsl";
  const QString finder = R"lsl(default {
    state_entry() { llSetTimerEvent(2); }
    timer() { llSetTimerEvent(0); llSensor("", NULL_KEY, SCRIPTED, 20, PI); llOwnerSay("[" + llKey2Name("c0ffee00-0000-4000-8000-000000000002") + "] " + (string)llGetListLength(llGetObjectDetails("c0ffee00-0000-4000-8000-000000000002", [OBJECT_NAME]))); }
    sensor(integer n) { llOwnerSay("found " + llDetectedName(0)); }
    no_sensor() { llOwnerSay("nothing"); }
  })lsl";
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 3}, {"objects", QJsonArray{
    QJsonObject{{"name", "box"}, {"scripts", QJsonArray{QJsonObject{{"name", "main"}, {"source", main}},
                                                        QJsonObject{{"name", "other"}, {"source", "default { state_entry() { llSetTimerEvent(0.5); } timer() { } }"}}}}},
    QJsonObject{{"name", "finder"}, {"source", finder}}}}}, &rlv, &session);
  const QStringList expected27({"bye", "[] 0", "nothing"});
  QCOMPARE(said(a), expected27);
  const QStringList expected28({
    R"({"column":34,"line":3,"message":"The object was deleted with llDie.","object":"box","script":"main","type":"die"})",
    R"({"message":"Another script of the object called llDie, so the object was deleted.","object":"box","script":"other","type":"die"})"});
  QCOMPARE(entries(a, "die"), expected28);
  const QJsonObject box = a["objects"].toArray()[0].toObject();
  QCOMPARE(box["deleted"].toBool(), true);
  QCOMPARE(box["stopped"].toBool(), true);
  QCOMPARE(box["stop_reason"].toString(), QString("main: The object was deleted with llDie. other: Another script of the object called llDie, so the object was deleted."));
  QVERIFY(!a["objects"].toArray()[1].toObject().contains("deleted"));
  // an input for the deleted object says so
  const QJsonObject e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"object", "box"}}, &session);
  const QStringList expected29({R"({"error":"The object was deleted with llDie.","input":"touch","name":"Owner","object":"box","type":"input"})"});
  QCOMPARE(entries(e, "input"), expected29);
}

void LslOwnObjectTest::dieInAnAttachmentDoesNothing()
{
  const QJsonObject a = runObjects(QJsonArray{QJsonObject{{"name", "cuff"}, {"attached", 30},
    {"source", R"lsl(default { state_entry() { llDie(); llOwnerSay("still here"); } })lsl"}}});
  const QStringList expected30({"still here"});
  QCOMPARE(said(a), expected30);
  QCOMPARE(entries(a, "die"), QStringList());
  QVERIFY(!a["stopped"].toBool());
  QVERIFY(!a["objects"].toArray()[0].toObject().contains("deleted"));
}

void LslOwnObjectTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llGetVel(); llGetMass(); llSetScale(<1,1,1>); llSetLocalRot(ZERO_ROTATION); llGetLinkName(1); llGetLinkKey(1);
      llGetObjectPermMask(MASK_OWNER); llGetInventoryPermMask("x", MASK_OWNER); llGetObjectDetails(NULL_KEY, [OBJECT_NAME]);
      llSameGroup(NULL_KEY); llSetScriptState("x", TRUE); llGetScriptState("x"); llResetOtherScript("x"); llGetCreator();
      llGetUsername(NULL_KEY); llDie();
    } touch_start(integer n) { llDetectedLinkNumber(0); } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  for (const char* f : {"llDie", "llGetVel", "llGetMass", "llSetScale", "llSetLocalRot", "llGetLinkName", "llGetLinkKey",
                        "llDetectedLinkNumber", "llGetObjectPermMask", "llGetInventoryPermMask", "llGetObjectDetails", "llSameGroup",
                        "llSetScriptState", "llGetScriptState", "llResetOtherScript", "llGetCreator", "llGetUsername"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

void LslOwnObjectTest::constantsMatchSecondLife()
{
  const QList<QPair<QString, int>> expected = {
    {"MASK_BASE", 0}, {"MASK_OWNER", 1}, {"MASK_GROUP", 2}, {"MASK_EVERYONE", 3}, {"MASK_NEXT", 4}, {"MASK_COMBINED", 0x10},
    {"PERM_ALL", 0x7FFFFFFF}, {"PERM_MOVE", 0x80000}, {"PERM_TRANSFER", 0x2000},
    {"OBJECT_NAME", 1}, {"OBJECT_TEXT_ALPHA", 49}, {"OBJECT_HEALTH", 50}, {"OBJECT_UNKNOWN_DETAIL", -1},
    {"OPT_LEGACY_LINKSET", 0}, {"OPT_AVATAR", 1}, {"OPT_OTHER", -1}, {"PRIM_MATERIAL_WOOD", 3}};
  for (const auto& e : expected)
  {
    const BuiltinConstant* c = Builtins::constant(e.first);
    QVERIFY2(c != nullptr, qPrintable(e.first));
    QCOMPARE(c->value.i, e.second);
  }
}

QTEST_GUILESS_MAIN(LslOwnObjectTest)
#include "tst_lslownobject.moc"
