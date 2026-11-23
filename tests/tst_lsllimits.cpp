/*
 * PinglyMotion - tests for the limit reads, llTarget and the region reads (Script Lab, third gap list run, steps 1 to 3)
 *
 * Every expected value was worked out by hand from the rules in docs/CONTROL.md ("Limits, targets and region reads").
 * Where the Second Life wiki is unclear the function gives its documented return type with a harmless canned value:
 *  - llGetMemoryLimit starts at 65536 (a Mono script). llSetMemoryLimit gives TRUE and stores the limit when it is from the
 *    memory the script uses now up to 65536, else FALSE and nothing changes (the wiki gives no exact rule for the low end).
 *  - llGetNumberOfSides is 6: every prim the sandbox simulates is a box. llGetLinkNumberOfSides is 6 for a link the object
 *    has (0 and 1 for a single prim) and for LINK_THIS, else 0.
 *  - llGetObjectMass of an avatar is a canned 50.0 (the sandbox has no avatar body); of an unknown key 0.0.
 *  - llGetBoundingBox of an avatar is the sandbox's avatar box (0.45 by 0.6 by the height), relative to the avatar.
 *  - llGetEnergy is 1.0, llGetRegionTimeDilation 1.0, llGetRegionFPS 45.0, llGetSimulatorHostname "sim-sandbox.invalid".
 *  - llTarget numbers its targets 1, 2, 3 and so on (the wiki does not say where the handles start).
 *  - at_target and not_at_target come from the "step" input of the motion model (the only thing that moves an object), once
 *    per step; llSetPos alone raises none.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslLimitsTest : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void memoryLimit();
    void memoryLimitResets();
    void sidesAndEnergy();
    void minEventDelayRecorded();
    void objectReadsOfOtherObjects();
    void boundingBoxOfLinkset();
    void boundingBoxTurned();
    void ownMassIsLlGetMass();
    void targetReached();
    void targetNotReached();
    void targetRemove();
    void targetsOfSeveralScripts();
    void manualAtTargetStillWorks();
    void regionReads();
    void agentCount();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& body, const QJsonArray& inputs = QJsonArray(), double seconds = 3, const QJsonObject& extra = QJsonObject());
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
};

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
}

QJsonObject LslLimitsTest::run(const QString& body, const QJsonArray& inputs, double seconds, const QJsonObject& extra)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", "default { state_entry() {\n" + body + "\n} }"}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  for (auto it = extra.begin(); it != extra.end(); ++it) request[it.key()] = it.value();
  return Control::run(request);
}

QStringList LslLimitsTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != "owner_say") continue;
    QString who = e["object"].toString();
    if (e.contains("script")) who += "/" + e["script"].toString();
    out << (who.isEmpty() ? QString() : who + ": ") + e["text"].toString();
  }
  return out;
}

QStringList LslLimitsTest::entries(const QJsonObject& answer, const QString& type)
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

void LslLimitsTest::supportedNow()
{
  for (const char* f : {"llGetMemoryLimit", "llSetMemoryLimit", "llMinEventDelay", "llGetNumberOfSides", "llGetLinkNumberOfSides",
                        "llGetObjectPrimCount", "llGetObjectMass", "llGetBoundingBox", "llGetEnergy", "llTarget", "llTargetRemove",
                        "llGetRegionTimeDilation", "llGetRegionFPS", "llGetRegionAgentCount", "llGetSimulatorHostname"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

void LslLimitsTest::memoryLimit()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetMemoryLimit());
    integer r = llSetMemoryLimit(32768);
    llOwnerSay((string)r + " " + (string)llGetMemoryLimit());
    r = llSetMemoryLimit(65537);
    llOwnerSay((string)r + " " + (string)llGetMemoryLimit());
    r = llSetMemoryLimit(-1);
    llOwnerSay((string)r + " " + (string)llGetMemoryLimit());
    r = llSetMemoryLimit(0);
    llOwnerSay((string)r + " " + (string)llGetMemoryLimit());
    r = llSetMemoryLimit(65536);
    llOwnerSay((string)r + " " + (string)llGetMemoryLimit());
  )lsl");
  QCOMPARE(said(a), QStringList({"65536", "1 32768", "0 32768", "0 32768", "0 32768", "1 65536"}));
}

void LslLimitsTest::memoryLimitResets()
{
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 2}, {"source", R"lsl(
    default
    {
      state_entry()
      {
        llOwnerSay("limit " + (string)llGetMemoryLimit());
        if (llLinksetDataRead("done") == "") { llLinksetDataWrite("done", "1"); llSetMemoryLimit(40000); llResetScript(); }
      }
    }
  )lsl"}});
  QCOMPARE(said(a), QStringList({"limit 65536", "limit 65536"}));
}

void LslLimitsTest::sidesAndEnergy()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetNumberOfSides());
    llOwnerSay((string)llGetLinkNumberOfSides(0) + " " + (string)llGetLinkNumberOfSides(1) + " " + (string)llGetLinkNumberOfSides(2));
    llOwnerSay((string)llGetLinkNumberOfSides(LINK_THIS) + " " + (string)llGetLinkNumberOfSides(LINK_SET) + " " + (string)llGetLinkNumberOfSides(-9));
    llOwnerSay((string)llGetEnergy());
  )lsl");
  QCOMPARE(said(a), QStringList({"6", "6 6 0", "6 0 0", "1.000000"}));
}

void LslLimitsTest::minEventDelayRecorded()
{
  const QJsonObject a = run("llMinEventDelay(0.5); llOwnerSay(\"done\");");
  QCOMPARE(said(a), QStringList({"done"}));
  QCOMPARE(entries(a, "recorded_only"),
           QStringList({R"({"args":[0.5],"function":"llMinEventDelay","note":"Recorded only: the sandbox plays and shows nothing for this.","type":"recorded_only"})"}));
}

// "Big": two prims, root scale <2, 1, 1> (mass 20) and a child of the default 0.5 size (mass 1.25), key "big-1".
void LslLimitsTest::objectReadsOfOtherObjects()
{
  const QString probe = R"lsl(
    default
    {
      state_entry()
      {
        llOwnerSay((string)llGetObjectPrimCount("big-1") + " " + (string)llGetObjectPrimCount("nobody") + " " + (string)llGetObjectPrimCount(llGetOwner()));
        llOwnerSay((string)llGetObjectMass("big-1") + " " + (string)llGetObjectMass("nobody") + " " + (string)llGetObjectMass(llGetOwner()));
        llOwnerSay(llList2CSV(llGetBoundingBox("big-1")));
        llOwnerSay((string)llGetListLength(llGetBoundingBox("nobody")));
        llOwnerSay(llList2CSV(llGetBoundingBox(llGetOwner())));
      }
    }
  )lsl";
  const QJsonArray objects{QJsonObject{{"name", "Big"}, {"key", "big-1"}, {"prims", 2}, {"scale", QJsonArray{2, 1, 1}},
                                        {"source", "default { state_entry() { } }"}},
                           QJsonObject{{"name", "Probe"}, {"source", probe}}};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", objects}});
  QCOMPARE(said(a), QStringList({"Probe: 2 0 0",
                                 "Probe: 21.250000 0.000000 50.000000",
                                 "Probe: <-1.00000, -0.50000, -0.50000>, <1.00000, 0.50000, 0.50000>",
                                 "Probe: 0",
                                 "Probe: <-0.22500, -0.30000, -0.90000>, <0.22500, 0.30000, 0.90000>"}));
}

// the child is moved to 3 m beside the root (PRIM_POSITION is a region position) with size <1, 1, 1>; the root keeps the default 0.5: x from -0.25 to 3.5
void LslLimitsTest::boundingBoxOfLinkset()
{
  const QJsonArray objects{QJsonObject{{"name", "Two"}, {"prims", 2}, {"source", R"lsl(
    default
    {
      state_entry()
      {
        llSetLinkPrimitiveParamsFast(2, [PRIM_POSITION, llGetPos() + <3, 0, 0>, PRIM_SIZE, <1, 1, 1>]);
        llOwnerSay(llList2CSV(llGetBoundingBox(llGetKey())));
      }
    }
  )lsl"}}};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", objects}});
  QCOMPARE(said(a), QStringList({"Two: <-0.25000, -0.50000, -0.50000>, <3.50000, 0.50000, 0.50000>"}));
}

// root <2, 1, 1> turned 90 degrees about z: the box is now 1 wide in x and 2 in y
void LslLimitsTest::boundingBoxTurned()
{
  const QJsonArray objects{QJsonObject{{"name", "Turned"}, {"scale", QJsonArray{2, 1, 1}}, {"source", R"lsl(
    default
    {
      state_entry()
      {
        llSetRot(llEuler2Rot(<0, 0, PI_BY_TWO>));
        list b = llGetBoundingBox(llGetKey());
        vector lo = llList2Vector(b, 0);
        vector hi = llList2Vector(b, 1);
        llOwnerSay((string)llRound(lo.x * 1000) + " " + (string)llRound(lo.y * 1000) + " " + (string)llRound(lo.z * 1000) + " "
                   + (string)llRound(hi.x * 1000) + " " + (string)llRound(hi.y * 1000) + " " + (string)llRound(hi.z * 1000));
      }
    }
  )lsl"}}};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", objects}});
  QCOMPARE(said(a), QStringList({"Turned: -500 -1000 -500 500 1000 500"}));
}

void LslLimitsTest::ownMassIsLlGetMass()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetMass() + " " + (string)llGetObjectMass(llGetKey()) + " " + (string)llGetObjectPrimCount(llGetKey()));
  )lsl");
  QCOMPARE(said(a), QStringList({"1.250000 1.250000 1"}));
}

namespace
{
  // a script that sets the targets in state_entry and reports every at_target and not_at_target
  const char* const TARGET_SCRIPT = R"lsl(
    default
    {
      state_entry()
      {
        integer first = llTarget(<128, 128, 10.2>, 0.5);
        integer second = llTarget(<128, 128, 100>, 1.0);
        llOwnerSay("handles " + (string)first + " " + (string)second);
      }
      at_target(integer n, vector t, vector o)
      {
        llOwnerSay("at " + (string)n + " " + (string)t + " " + (string)o);
        llTargetRemove(n);
      }
      not_at_target() { llOwnerSay("not at target"); }
    }
  )lsl";
}

// gravity takes the object from z 20 to 10.2 in one second: inside the first target (range 0.5), far from the second.
// The reached target is removed by the script; the next step finds only the far one: not_at_target.
void LslLimitsTest::targetReached()
{
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", TARGET_SCRIPT}, {"seconds", 3},
    {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}}, QJsonObject{{"at", 2}, {"type", "step"}, {"dt", 0}}}}});
  QCOMPARE(said(a), QStringList({"handles 1 2", "at 1 <128.00000, 128.00000, 10.20000> <128.00000, 128.00000, 10.20000>", "not at target"}));
}

void LslLimitsTest::targetNotReached()
{
  const QJsonObject a = run(R"lsl(
    llTarget(<128, 128, 100>, 1.0);
  )lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}}});
  QCOMPARE(entries(a, "input").size(), 1);
  // no at_target or not_at_target handler in this script: the event is dropped without a trace; check with a handler
  const QJsonObject b = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 3}, {"source", R"lsl(
    default
    {
      state_entry() { llTarget(<128, 128, 100>, 1.0); }
      at_target(integer n, vector t, vector o) { llOwnerSay("at"); }
      not_at_target() { llOwnerSay("not " + (string)llGetPos()); }
    }
  )lsl"}, {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}}}}});
  QCOMPARE(said(b), QStringList({"not <128.00000, 128.00000, 10.20000>"}));
}

void LslLimitsTest::targetRemove()
{
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 3}, {"source", R"lsl(
    default
    {
      state_entry()
      {
        integer t = llTarget(<128, 128, 10.2>, 0.5);
        llTargetRemove(t);
        llTargetRemove(99);
        llOwnerSay("removed " + (string)t);
      }
      at_target(integer n, vector t, vector o) { llOwnerSay("at"); }
      not_at_target() { llOwnerSay("not"); }
    }
  )lsl"}, {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}}}}});
  QCOMPARE(said(a), QStringList({"removed 1"}));
}

// each script keeps its own targets and handles; the second script's first handle is 1 as well
void LslLimitsTest::targetsOfSeveralScripts()
{
  const QJsonArray objects{QJsonObject{{"name", "Mover"}, {"scripts", QJsonArray{
    QJsonObject{{"name", "near"}, {"source", R"lsl(
      default { state_entry() { llOwnerSay("handle " + (string)llTarget(<128, 128, 10.2>, 0.5)); }
                at_target(integer n, vector t, vector o) { llOwnerSay("near at " + (string)n); llTargetRemove(n); } }
    )lsl"}},
    QJsonObject{{"name", "far"}, {"source", R"lsl(
      default { state_entry() { llOwnerSay("handle " + (string)llTarget(<0, 0, 0>, 1.0)); }
                not_at_target() { llOwnerSay("far not at"); } }
    )lsl"}}}}}};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 3}, {"objects", objects},
    {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}}}}});
  QCOMPARE(said(a), QStringList({"Mover/near: handle 1", "Mover/far: handle 1", "Mover/near: near at 1", "Mover/far: far not at"}));
}

void LslLimitsTest::manualAtTargetStillWorks()
{
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 3}, {"source", R"lsl(
    default { at_target(integer n, vector t, vector o) { llOwnerSay("at " + (string)n + " " + (string)t + " " + (string)o); } }
  )lsl"}, {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "at_target"}, {"tnum", 7}, {"targetpos", QJsonArray{1, 2, 3}}, {"ourpos", QJsonArray{4, 5, 6}}}}}});
  QCOMPARE(said(a), QStringList({"at 7 <1.00000, 2.00000, 3.00000> <4.00000, 5.00000, 6.00000>"}));
}

void LslLimitsTest::regionReads()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetRegionTimeDilation() + " " + (string)llGetRegionFPS());
    llOwnerSay(llGetSimulatorHostname());
  )lsl");
  QCOMPARE(said(a), QStringList({"1.000000 45.000000", "sim-sandbox.invalid"}));
}

void LslLimitsTest::agentCount()
{
  QCOMPARE(said(run("llOwnerSay((string)llGetRegionAgentCount());")), QStringList({"1"}));
  const QJsonArray avatars{QJsonObject{{"name", "Visitor One"}, {"key", "visitor-1"}}, QJsonObject{{"name", "Visitor Two"}, {"key", "visitor-2"}}};
  QCOMPARE(said(run("llOwnerSay((string)llGetRegionAgentCount());", QJsonArray(), 3, QJsonObject{{"avatars", avatars}})), QStringList({"3"}));
  // an avatar given with the owner's key is the owner, not one more
  const QJsonArray withOwner{QJsonObject{{"name", "Owner"}, {"key", OWNER}}, QJsonObject{{"name", "Visitor One"}, {"key", "visitor-1"}}};
  QCOMPARE(said(run("llOwnerSay((string)llGetRegionAgentCount());", QJsonArray(), 3, QJsonObject{{"avatars", withOwner}})), QStringList({"2"}));
}

void LslLimitsTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llOwnerSay((string)llGetMemoryLimit() + (string)llSetMemoryLimit(32768));
      llMinEventDelay(0.1);
      llOwnerSay((string)llGetNumberOfSides() + (string)llGetLinkNumberOfSides(1) + (string)llGetEnergy());
      llOwnerSay((string)llGetObjectPrimCount(llGetKey()) + (string)llGetObjectMass(llGetKey()) + llList2CSV(llGetBoundingBox(llGetKey())));
      integer t = llTarget(<1, 2, 3>, 1.0);
      llTargetRemove(t);
      llOwnerSay((string)llGetRegionTimeDilation() + (string)llGetRegionFPS() + (string)llGetRegionAgentCount() + llGetSimulatorHostname());
    }
    at_target(integer n, vector t, vector o) { llTargetRemove(n); }
    not_at_target() { llOwnerSay("not"); } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

QTEST_GUILESS_MAIN(LslLimitsTest)

#include "tst_lsllimits.moc"
