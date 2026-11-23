/*
 * PinglyMotion - tests for step 18 of the Script Lab gap list: physics reads and settings, rotation targets, llBreakLink, ground
 * and region reads (llGetAccel, llGetCenterOfMass, llGetGeometricCenter, llGetMassMKS, llGetOmega, llGetPhysicsMaterial,
 * llSetPhysicsMaterial, llGetTorque, llSetTorque, llSetForceAndTorque, llSetAngularVelocity, llSetHoverHeight, llStopHover,
 * llRotTarget, llRotTargetRemove, llBreakLink, llDetectedGrab, llCollisionSprite, llEdgeOfWorld, llGroundContour,
 * llGroundNormal, llGroundSlope, llGetRegionFlags, llWind, llScriptDanger, llGetSPMaxMemory)
 *
 * The expected answers were worked out by hand from the Second Life wiki. Where the wiki is silent or unclear the sandbox
 * chooses; every choice is written here and in docs/CONTROL.md:
 *  - llGetAccel: the wiki gives ZERO_VECTOR for an attachment and for a child prim. For the root prim the sandbox answers how
 *    much the motion model's last "step" (with a time above 0) changed the velocity, per second (units are not in the wiki);
 *    llApplyImpulse and llSetVelocity do not count, and before any step it is ZERO_VECTOR.
 *  - llGetOmega, llGetTorque: only what llSetAngularVelocity, llSetTorque and llSetForceAndTorque stored (a local vector is
 *    turned by the object's rotation, as llSetForce does); the motion model neither turns the object nor uses the torque.
 *  - llGetMassMKS is llGetMass times 100 (the wiki: the kilogram value seems to be that).
 *  - llGetCenterOfMass: a physical root prim gives the centre of mass of the linkset (every prim a box of the same density);
 *    anything else gives the prim's own position (the wiki: llGetPos() for a non-physical object, a child's own centre).
 *  - llGetGeometricCenter: the average of the prim centres relative to the root prim, in the root's axes (the wiki).
 *  - llGet/llSetPhysicsMaterial: the start values are gravity 1.0, restitution 0.3, friction 0.5, density 1000.0 (the wiki
 *    gives the density default only). A value outside its range is cut to it (the wiki gives the ranges only), a NaN is left
 *    out, an attachment changes nothing. The material belongs to the object, not to a prim. The density does not change llGetMass.
 *  - llSetHoverHeight, llStopHover and llCollisionSprite are record only.
 *  - llRotTarget handles count 1, 2, 3 per script, as llTarget does; the error is the angle between the two rotations in
 *    radians; after each "step" input at_rot_target(handle, targetrot, ourrot) comes for every target within its error and
 *    not_at_rot_target() once when there are targets and none is reached.
 *  - llBreakLink separates the whole object that holds the prim with that link number (objects that llCreateLink joined are
 *    the only ones that can leave); any other number only logs a note.
 *  - llDetectedGrab is always ZERO_VECTOR (a touch is no grab). llEdgeOfWorld is TRUE (the region has no neighbour).
 *    The ground is flat: slope and contour are ZERO_VECTOR, the normal is <0,0,1>. llWind is ZERO_VECTOR. llGetRegionFlags is
 *    REGION_FLAG_ALLOW_DIRECT_TELEPORT. llScriptDanger is FALSE on the sandbox's parcel and off the region; TRUE on a parcel
 *    with a script danger, in a region with REGION_FLAG_SANDBOX, or when the parcel does not let everybody run scripts or build.
 *    llGetSPMaxMemory is 0 until llScriptProfiler profiled the script (step 19).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"
#include "scriptlab/lsl/sandbox.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

class LslSteps18Test : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void accelAfterSteps();
    void accelOfNoTimeAndBuoyancy();
    void accelInChildPrim();
    void omegaTorqueForce();
    void localVectorsAreTurned();
    void massAndCentres();
    void physicsMaterial();
    void recordOnly();
    void rotTargets();
    void rotTargetFollowsRotation();
    void breakLinkOfJoinedObject();
    void breakLinkRefusals();
    void breakLinkNeedsPermission();
    void groundAndRegionReads();
    void regionFlagsAndDanger();
    void touchGrab();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& source, const QJsonArray& inputs = QJsonArray(), double seconds = 3);
    static QJsonObject runObjects(const QJsonArray& objects, const QJsonArray& inputs = QJsonArray(), double seconds = 3);
    static QStringList said(const QJsonObject& answer);
    static QJsonArray ofType(const QJsonObject& answer, const QString& type);
};

namespace
{
  const char* const KEY_A = "aaaaaaaa-0000-4000-8000-000000000001";
  const char* const KEY_B = "bbbbbbbb-0000-4000-8000-000000000002";

  QJsonObject step(double at, double dt) { return QJsonObject{{"at", at}, {"type", "step"}, {"dt", dt}}; }
  QString body(const QString& statements) { return "default { state_entry() {\n" + statements + "\n} }"; }
}

QJsonObject LslSteps18Test::run(const QString& source, const QJsonArray& inputs, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request);
}

QJsonObject LslSteps18Test::runObjects(const QJsonArray& objects, const QJsonArray& inputs, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"objects", objects}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request);
}

QStringList LslSteps18Test::said(const QJsonObject& answer)
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

QJsonArray LslSteps18Test::ofType(const QJsonObject& answer, const QString& type)
{
  QJsonArray out;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == type) out.append(v);
  return out;
}

void LslSteps18Test::supportedNow()
{
  for (const char* name : {"llGetAccel", "llGetCenterOfMass", "llGetGeometricCenter", "llGetMassMKS", "llGetOmega", "llGetPhysicsMaterial",
                           "llSetPhysicsMaterial", "llGetTorque", "llSetTorque", "llSetForceAndTorque", "llSetAngularVelocity",
                           "llSetHoverHeight", "llStopHover", "llRotTarget", "llRotTargetRemove", "llBreakLink", "llDetectedGrab",
                           "llCollisionSprite", "llEdgeOfWorld", "llGroundContour", "llGroundNormal", "llGroundSlope",
                           "llGetRegionFlags", "llWind", "llScriptDanger", "llGetSPMaxMemory"})
  {
    QVERIFY2(Builtins::function(name) != nullptr, name);
    QVERIFY2(Builtins::function(name)->supported, name);
  }
  // the examples of "not supported yet" in the other tests
  for (const char* name : {"llNavigateTo"})
    QVERIFY2(Builtins::function(name) && !Builtins::function(name)->supported, name);
}

// gravity only: velocity 0 -> -9.8 in a step of 1 s (accel -9.8), then to -14.7 in a step of 0.5 s (accel -9.8 again)
void LslSteps18Test::accelAfterSteps()
{
  const QJsonObject a = run(R"lsl(integer n;
  default {
    state_entry() { llOwnerSay("start " + (string)llGetAccel()); llSetTimerEvent(1.5); }
    timer() { llOwnerSay((string)(++n) + " " + (string)llGetAccel()); if (n == 2) llSetTimerEvent(0); }
  })lsl", QJsonArray{step(1, 1), step(2, 0.5)}, 4);
  QCOMPARE(said(a), QStringList({"start <0.00000, 0.00000, 0.00000>", "1 <0.00000, 0.00000, -9.80000>", "2 <0.00000, 0.00000, -9.80000>"}));
}

// a step of no time keeps the last answer; with buoyancy 1 gravity does not change the velocity, so the acceleration is 0
void LslSteps18Test::accelOfNoTimeAndBuoyancy()
{
  const QString source = R"lsl(default {
    state_entry() { llSetTimerEvent(3); }
    timer() { llOwnerSay((string)llGetAccel() + " " + (string)llGetVel()); llSetTimerEvent(0); }
  })lsl";
  QCOMPARE(said(run(source, QJsonArray{step(1, 1), step(2, 0)})), QStringList({"<0.00000, 0.00000, -9.80000> <0.00000, 0.00000, -9.80000>"}));
  const QString floating = R"lsl(default {
    state_entry() { llSetBuoyancy(1.0); llSetTimerEvent(3); }
    timer() { llOwnerSay((string)llGetAccel()); llSetTimerEvent(0); }
  })lsl";
  QCOMPARE(said(run(floating, QJsonArray{step(1, 1)})), QStringList({"<0.00000, 0.00000, 0.00000>"}));
}

// the wiki: a child prim gets ZERO_VECTOR whatever the linkset does
void LslSteps18Test::accelInChildPrim()
{
  const QString report = "default { state_entry() { llSetTimerEvent(3); } timer() { llOwnerSay((string)llGetAccel()); llSetTimerEvent(0); } }";
  const QJsonArray objects{QJsonObject{{"name", "A"}, {"prims", 2}, {"scripts", QJsonArray{
      QJsonObject{{"name", "Root"}, {"link", 1}, {"source", report}}, QJsonObject{{"name", "Kid"}, {"link", 2}, {"source", report}}}}}};
  const QJsonObject a = runObjects(objects, QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}, {"object", "A"}}});
  QCOMPARE(said(a), QStringList({"A/Root: <0.00000, 0.00000, -9.80000>", "A/Kid: <0.00000, 0.00000, 0.00000>"}));
}

void LslSteps18Test::omegaTorqueForce()
{
  const QJsonObject a = run(body(R"lsl(
    llOwnerSay((string)llGetOmega() + " " + (string)llGetTorque());
    llSetAngularVelocity(<0, 0, 1.5>, FALSE);
    llSetTorque(<1, 2, 3>, FALSE);
    llOwnerSay((string)llGetOmega() + " " + (string)llGetTorque() + " " + (string)llGetForce());
    llSetForceAndTorque(<4, 5, 6>, <7, 8, 9>, FALSE);
    llOwnerSay((string)llGetOmega() + " " + (string)llGetTorque() + " " + (string)llGetForce());
  )lsl"));
  QCOMPARE(said(a), QStringList({"<0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000>",
                                 "<0.00000, 0.00000, 1.50000> <1.00000, 2.00000, 3.00000> <0.00000, 0.00000, 0.00000>",
                                 "<0.00000, 0.00000, 1.50000> <7.00000, 8.00000, 9.00000> <4.00000, 5.00000, 6.00000>"}));
  QJsonArray ops;
  for (const QJsonValue& v : ofType(a, "velocity")) ops.append(v.toObject()["op"]);
  QCOMPARE(ops, QJsonArray({"angular_velocity", "torque", "force_torque"}));
  const QJsonObject both = ofType(a, "velocity")[2].toObject();
  QCOMPARE(both["torque"].toArray(), QJsonArray({7.0, 8.0, 9.0}));
  QCOMPARE(both["force"].toArray(), QJsonArray({4.0, 5.0, 6.0}));
}

// turned 90 degrees about z, the object's own x axis points along the region's y axis (rounded to thousandths: no -0 or 1e-8)
void LslSteps18Test::localVectorsAreTurned()
{
  const QJsonObject a = run(body(R"lsl(
    llSetRot(llEuler2Rot(<0, 0, PI_BY_TWO>));
    llSetAngularVelocity(<1, 0, 0>, TRUE);
    llSetTorque(<0, 1, 0>, TRUE);
    vector w = llGetOmega();
    vector t = llGetTorque();
    llOwnerSay((string)llRound(w.x * 1000) + " " + (string)llRound(w.y * 1000) + " " + (string)llRound(w.z * 1000));
    llOwnerSay((string)llRound(t.x * 1000) + " " + (string)llRound(t.y * 1000) + " " + (string)llRound(t.z * 1000));
  )lsl"));
  QCOMPARE(said(a), QStringList({"0 1000 0", "-1000 0 0"}));
}

// default object: scale 0.5, mass 1.25 lindograms = 125 kg. Three prims: the root (0.5, volume 0.125), a child of 1 m at 3 m in x (volume 1)
// and another 0.5 child at 6 m in y (volume 0.125); the mass is 12.5 lindograms = 1250 kg. The centre of mass of the physical linkset is
// x = 3 * 1 / 1.25 = 2.4 and y = 6 * 0.125 / 1.25 = 0.6 from the root; the geometric centre (the average of the prim centres) is <1, 2, 0>
void LslSteps18Test::massAndCentres()
{
  QCOMPARE(said(run(body(R"lsl(
    llOwnerSay((string)llGetMass() + " " + (string)llGetMassMKS());
    llOwnerSay((string)llGetCenterOfMass() + " " + (string)llGetGeometricCenter());
    llSetStatus(STATUS_PHYSICS, TRUE);
    llOwnerSay((string)llGetCenterOfMass());
  )lsl"))), QStringList({"1.250000 125.000000", "<128.00000, 128.00000, 20.00000> <0.00000, 0.00000, 0.00000>", "<128.00000, 128.00000, 20.00000>"}));
  const QString setup = R"lsl(default {
    state_entry()
    {
      llSetLinkPrimitiveParamsFast(2, [PRIM_POSITION, llGetPos() + <3, 0, 0>, PRIM_SIZE, <1, 1, 1>]);
      llSetLinkPrimitiveParamsFast(3, [PRIM_POSITION, llGetPos() + <0, 6, 0>]);
      llOwnerSay((string)llGetCenterOfMass() + " " + (string)llGetGeometricCenter() + " " + (string)llGetMassMKS());
      llSetStatus(STATUS_PHYSICS, TRUE);
      vector c = llGetCenterOfMass();
      llOwnerSay((string)llRound(c.x * 1000) + " " + (string)llRound(c.y * 1000) + " " + (string)llRound(c.z * 1000));
    }
  })lsl";
  const QString kid = "default { state_entry() { llSetTimerEvent(1); } timer() { llOwnerSay((string)llGetCenterOfMass() + \" \" + (string)llGetGeometricCenter()); llSetTimerEvent(0); } }";
  const QJsonArray objects{QJsonObject{{"name", "Three"}, {"prims", 3}, {"scripts", QJsonArray{
      QJsonObject{{"name", "Root"}, {"link", 1}, {"source", setup}}, QJsonObject{{"name", "Kid"}, {"link", 2}, {"source", kid}}}}}};
  const QJsonObject a = runObjects(objects);
  QCOMPARE(said(a), QStringList({"Three/Root: <128.00000, 128.00000, 20.00000> <1.00000, 2.00000, 0.00000> 1250.000000",
                                 "Three/Root: 130400 128600 20000",
                                 "Three/Kid: <131.00000, 128.00000, 20.00000> <1.00000, 2.00000, 0.00000>"}));
}

void LslSteps18Test::physicsMaterial()
{
  const QJsonObject a = run(body(R"lsl(
    llOwnerSay(llList2CSV(llGetPhysicsMaterial()));
    llSetPhysicsMaterial(GRAVITY_MULTIPLIER | FRICTION, 2.0, 0.9, 0.8, 5000.0);
    llOwnerSay(llList2CSV(llGetPhysicsMaterial()));
    llSetPhysicsMaterial(DENSITY | RESTITUTION, 99.0, 0.25, 99.0, 2000.0);
    llOwnerSay(llList2CSV(llGetPhysicsMaterial()));
    llSetPhysicsMaterial(GRAVITY_MULTIPLIER | RESTITUTION | FRICTION | DENSITY, -5.0, 2.0, 300.0, 0.0);
    llOwnerSay(llList2CSV(llGetPhysicsMaterial()));
    llSetPhysicsMaterial(0, 5.0, 0.5, 5.0, 5.0);
    llOwnerSay(llList2CSV(llGetPhysicsMaterial()));
  )lsl"));
  QCOMPARE(said(a), QStringList({"1.000000, 0.300000, 0.500000, 1000.000000",
                                 "2.000000, 0.300000, 0.800000, 1000.000000",
                                 "2.000000, 0.250000, 0.800000, 2000.000000",
                                 "-1.000000, 1.000000, 255.000000, 1.000000",
                                 "-1.000000, 1.000000, 255.000000, 1.000000"}));
  const QJsonObject e = ofType(a, "physics")[0].toObject();
  QCOMPARE(e["op"].toString(), QString("material"));
  QCOMPARE(e["mask"].toInt(), 10);
  QCOMPARE(e["material"].toArray(), QJsonArray({2.0, 0.3, 0.8, 1000.0}));
  // an attachment changes nothing (the wiki: silently fails)
  const QJsonArray objects{QJsonObject{{"name", "Worn"}, {"attached", 1}, {"source", body(
      "llSetPhysicsMaterial(DENSITY, 1.0, 1.0, 1.0, 5.0); llOwnerSay(llList2CSV(llGetPhysicsMaterial()));")}}};
  QCOMPARE(said(runObjects(objects)), QStringList({"Worn: 1.000000, 0.300000, 0.500000, 1000.000000"}));
}

void LslSteps18Test::recordOnly()
{
  const QJsonObject a = run(body(R"lsl(
    llSetHoverHeight(10.0, TRUE, 0.5);
    llStopHover();
    llCollisionSprite("spark");
    llOwnerSay("done");
  )lsl"));
  QCOMPARE(said(a), QStringList({"done"}));
  const QJsonArray rec = ofType(a, "recorded_only");
  QCOMPARE(int(rec.size()), 3);
  QCOMPARE(rec[0].toObject()["function"].toString(), QString("llSetHoverHeight"));
  QCOMPARE(rec[0].toObject()["args"].toArray().size(), 3);
  QCOMPARE(rec[1].toObject()["function"].toString(), QString("llStopHover"));
  QCOMPARE(rec[2].toObject()["function"].toString(), QString("llCollisionSprite"));
  QCOMPARE(rec[2].toObject()["args"].toArray()[0].toString(), QString("spark"));
}

// the object stays at ZERO_ROTATION. A turn of 0.1 rad about z is within an error of 0.2 (reached), one of 1.0 rad is not.
// The reached target is removed by the script; the next step (of no time) finds only the far one: not_at_rot_target.
void LslSteps18Test::rotTargets()
{
  const QJsonObject a = run(R"lsl(default {
    state_entry()
    {
      integer near = llRotTarget(llEuler2Rot(<0, 0, 0.1>), 0.2);
      integer far = llRotTarget(llEuler2Rot(<0, 0, 1.0>), 0.2);
      llOwnerSay("handles " + (string)near + " " + (string)far);
      llRotTargetRemove(99);
    }
    at_rot_target(integer n, rotation target, rotation ours) { llOwnerSay("at " + (string)n + " " + (string)target + " " + (string)ours); llRotTargetRemove(n); }
    not_at_rot_target() { llOwnerSay("not at rot target"); }
  })lsl", QJsonArray{step(1, 1), step(2, 0)});
  QCOMPARE(said(a), QStringList({"handles 1 2",
                                 "at 1 <0.00000, 0.00000, 0.04998, 0.99875> <0.00000, 0.00000, 0.00000, 1.00000>",
                                 "not at rot target"}));
}

// llSetRot to 0.5 rad; targets at 0.45 rad: the angle is 0.05, so an error of 0.04 is not enough and 0.06 is. A removed target is gone.
void LslSteps18Test::rotTargetFollowsRotation()
{
  const QJsonObject a = run(R"lsl(default {
    state_entry()
    {
      llSetRot(llEuler2Rot(<0, 0, 0.5>));
      integer tight = llRotTarget(llEuler2Rot(<0, 0, 0.45>), 0.04);
      integer loose = llRotTarget(llEuler2Rot(<0, 0, 0.45>), 0.06);
      integer gone = llRotTarget(llEuler2Rot(<0, 0, 0.45>), 0.06);
      llRotTargetRemove(gone);
      llOwnerSay("handles " + (string)tight + " " + (string)loose + " " + (string)gone);
    }
    at_rot_target(integer n, rotation target, rotation ours) { llOwnerSay("at " + (string)n); }
    not_at_rot_target() { llOwnerSay("not"); }
  })lsl", QJsonArray{step(1, 1)});
  QCOMPARE(said(a), QStringList({"handles 1 2 3", "at 2"}));
}

namespace
{
  // an object with a script in every link given; a touch makes every script report its prim count and link number
  QJsonObject linkObject(const char* name, const char* key, int prims, const QString& rootExtra = QString())
  {
    const QString report = R"lsl(
      touch_start(integer n) { llOwnerSay("prims " + (string)llGetNumberOfPrims() + " link " + (string)llGetLinkNumber()); }
      changed(integer c) { if (c & CHANGED_LINK) llOwnerSay("changed link"); }
    )lsl";
    QJsonArray scripts;
    scripts.append(QJsonObject{{"name", "Root"}, {"link", 1}, {"source", "default {\n" + rootExtra + report + "\n}"}});
    for (int link = 2; link <= prims; ++link)
      scripts.append(QJsonObject{{"name", QString("Prim%1").arg(link)}, {"link", link}, {"source", "default {\n" + report + "\n}"}});
    return QJsonObject{{"name", name}, {"key", key}, {"prims", prims}, {"scripts", scripts}};
  }
  QJsonObject touchOf(double at, const char* object, int link = 1) { return QJsonObject{{"at", at}, {"type", "touch"}, {"object", object}, {"link", link}}; }
}

// A (2 prims) is the root and B (3 prims) joins at link 2: A1, B1, B2, B3, A2. llBreakLink(3) is a prim of B, so all of B leaves.
void LslSteps18Test::breakLinkOfJoinedObject()
{
  const QJsonArray objects{
    linkObject("A", KEY_A, 2, QString(R"lsl(
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
      run_time_permissions(integer p) { llCreateLink("%1", TRUE); llSetTimerEvent(1); }
      timer() { llSetTimerEvent(0); llBreakLink(3); }
    )lsl").arg(KEY_B)),
    linkObject("B", KEY_B, 3)};
  const QJsonObject a = runObjects(objects, QJsonArray{touchOf(0.5, "A", 1), touchOf(0.5, "B", 2), touchOf(2, "A", 1), touchOf(2, "A", 2),
                                                        touchOf(2, "B", 1), touchOf(2, "B", 2), touchOf(2, "B", 3)}, 3);
  QCOMPARE(said(a), QStringList({
      // llCreateLink: A has two scripts, B three
      "A/Root: changed link", "A/Prim2: changed link", "B/Root: changed link", "B/Prim2: changed link", "B/Prim3: changed link",
      // the touches at 0.5 s: five prims, B's second prim is link 3
      "A/Root: prims 5 link 1", "B/Prim2: prims 5 link 3",
      // llBreakLink(3): every script of both objects hears it
      "A/Root: changed link", "A/Prim2: changed link", "B/Root: changed link", "B/Prim2: changed link", "B/Prim3: changed link",
      // A is back to 2 prims, B to its own 3
      "A/Root: prims 2 link 1", "A/Prim2: prims 2 link 2", "B/Root: prims 3 link 1", "B/Prim2: prims 3 link 2", "B/Prim3: prims 3 link 3"}));
  const QJsonArray links = ofType(a, "link");
  QCOMPARE(int(links.size()), 2);
  const QJsonObject br = links[1].toObject();
  QCOMPARE(br["op"].toString(), QString("break"));
  QCOMPARE(br["link"].toInt(), 3);
  QCOMPARE(br["released"].toBool(), true);
  QCOMPARE(br["root"].toString(), QString(KEY_A));
  QCOMPARE(br["separated"].toString(), QString(KEY_B));
  QCOMPARE(br["prims"].toInt(), 2);
}

void LslSteps18Test::breakLinkRefusals()
{
  // nothing linked yet: 2 is no joined prim; after B (links 2 to 4) joined, 1 (the root), 5 (A's own second prim) and 9 are not joined prims either
  const QJsonArray objects{
    linkObject("A", KEY_A, 2, QString(R"lsl(
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
      run_time_permissions(integer p)
      {
        llBreakLink(2);
        llCreateLink("%1", TRUE);
        llBreakLink(1); llBreakLink(5); llBreakLink(9); llBreakLink(-1); llBreakLink(0);
      }
    )lsl").arg(KEY_B)),
    linkObject("B", KEY_B, 3)};
  const QJsonObject a = runObjects(objects, QJsonArray(), 1);
  QStringList notes;
  for (const QJsonValue& v : ofType(a, "link"))
  {
    const QJsonObject e = v.toObject();
    if (e["op"].toString() == "break") notes << QString::number(e["link"].toInt()) + ": " + (e["released"].toBool() ? "released" : e["note"].toString());
  }
  const QString none = "Nothing was linked with llCreateLink; the prims an object has from the start stay together in the sandbox.";
  const QString notJoined = "That link number is not a prim of an object that llCreateLink joined.";
  QCOMPARE(notes, QStringList({"2: " + none, "1: " + notJoined, "5: " + notJoined, "9: " + notJoined, "-1: " + notJoined, "0: " + notJoined}));
}

void LslSteps18Test::breakLinkNeedsPermission()
{
  const QJsonArray objects{linkObject("A", KEY_A, 2, "state_entry() { llBreakLink(2); }")};
  const QJsonObject a = runObjects(objects, QJsonArray(), 1);
  const QJsonArray debug = ofType(a, "debug");
  QCOMPARE(int(debug.size()), 1);
  QCOMPARE(debug[0].toObject()["text"].toString(), QString("Script trying to change links but PERMISSION_CHANGE_LINKS permission not set!"));
  QCOMPARE(int(ofType(a, "link").size()), 0);
  // the plain single-object run has nobody to link: with the permission the call logs a note
  const QJsonObject b = run(R"lsl(default {
    state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
    run_time_permissions(integer p) { llBreakLink(2); }
  })lsl");
  QCOMPARE(int(ofType(b, "link").size()), 1);
  QCOMPARE(ofType(b, "link")[0].toObject()["released"].toBool(), false);
}

void LslSteps18Test::groundAndRegionReads()
{
  const QJsonObject a = run(body(R"lsl(
    llOwnerSay((string)llGroundSlope(<1, 1, 0>) + " " + (string)llGroundContour(<1, 1, 0>) + " " + (string)llGroundNormal(<1, 1, 0>));
    llOwnerSay((string)llEdgeOfWorld(llGetPos(), <1, 0, 0>) + (string)llEdgeOfWorld(<10, 10, 10>, <0, 0, 1>) + (string)llEdgeOfWorld(<10, 10, 10>, <-1, -1, 0>));
    llOwnerSay((string)llGetRegionFlags() + " " + (string)((llGetRegionFlags() & REGION_FLAG_ALLOW_DIRECT_TELEPORT) != 0));
    llOwnerSay((string)llWind(<0, 0, 0>) + " " + (string)llWind(<5, 5, 5>));
    llOwnerSay((string)llScriptDanger(llGetPos()) + (string)llScriptDanger(<300, 10, 10>) + (string)llScriptDanger(<-1, 10, 10>));
    llOwnerSay((string)llGetSPMaxMemory());
  )lsl"));
  QCOMPARE(said(a), QStringList({"<0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 1.00000>",
                                 "111",
                                 "1048576 1",
                                 "<0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000>",
                                 "000",
                                 "0"}));
}

// C++ only: no lsl_run option sets the region's flags, wind or the parcel yet
void LslSteps18Test::regionFlagsAndDanger()
{
  struct Case { const char* name; int regionFlags; int parcelFlags; int danger; QString expected; };
  const int normal = 0x0800004B;
  const Case cases[] = {
    {"normal", 0x100000, normal, 0, "1048576|<1.00000, 2.00000, 3.00000>|0|0"},
    {"sandbox region", 0x100100, normal, 0, "1048832|<1.00000, 2.00000, 3.00000>|1|0"},
    {"scripts off", 0x100000, normal & ~0x2, 0, "1048576|<1.00000, 2.00000, 3.00000>|1|0"},
    {"building off", 0x100000, normal & ~0x40, 0, "1048576|<1.00000, 2.00000, 3.00000>|1|0"},
    {"script danger", 0x100000, normal, 1, "1048576|<1.00000, 2.00000, 3.00000>|1|0"},
  };
  for (const Case& c : cases)
  {
    RlvState rlv;
    Sandbox sandbox(rlv, true);
    RegionSettings region;
    region.regionFlags = c.regionFlags;
    region.parcelFlags = c.parcelFlags;
    region.parcelScriptDanger = c.danger;
    region.wind[0] = 1.0f; region.wind[1] = 2.0f; region.wind[2] = 3.0f;
    sandbox.setRegion(region);
    ObjectSpec spec;
    spec.name = "o";
    Parser::Result parsed = Parser::parse(R"lsl(
      default { state_entry() {
        llOwnerSay((string)llGetRegionFlags() + "|" + (string)llWind(<9, 9, 9>) + "|" + (string)llScriptDanger(<10, 10, 10>) + "|" + (string)llScriptDanger(<999, 10, 10>));
      } }
    )lsl");
    QVERIFY(parsed.ok());
    QCOMPARE(sandbox.addObject(spec, std::shared_ptr<const Script>(parsed.script.release())), QString());
    sandbox.start();
    sandbox.runUntil(1.0);
    QStringList said;
    for (const QJsonValue& v : sandbox.log())
      if (v.toObject()["type"].toString() == "owner_say") said << v.toObject()["text"].toString();
    QVERIFY2(said == QStringList({c.expected}), c.name);
  }
}

void LslSteps18Test::touchGrab()
{
  const QJsonObject a = run(R"lsl(default {
    touch_start(integer n) { llOwnerSay((string)llDetectedGrab(0) + " " + (string)llDetectedGrab(5)); }
  })lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}}, 2);
  QCOMPARE(said(a), QStringList({"<0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000>"}));
}

void LslSteps18Test::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default {
      state_entry() {
        llOwnerSay((string)llGetAccel() + (string)llGetCenterOfMass() + (string)llGetGeometricCenter() + (string)llGetMassMKS() + (string)llGetOmega());
        llSetPhysicsMaterial(DENSITY, 1.0, 0.5, 0.5, 1000.0);
        llOwnerSay(llList2CSV(llGetPhysicsMaterial()) + (string)llGetTorque());
        llSetTorque(<0, 0, 1>, TRUE);
        llSetForceAndTorque(<0, 0, 1>, <0, 0, 1>, FALSE);
        llSetAngularVelocity(<0, 0, 1>, TRUE);
        llSetHoverHeight(10.0, FALSE, 1.0);
        llStopHover();
        llCollisionSprite("");
        integer t = llRotTarget(ZERO_ROTATION, 0.1);
        llRotTargetRemove(t);
        llBreakLink(2);
        llOwnerSay((string)llEdgeOfWorld(<1, 1, 1>, <1, 0, 0>) + (string)llGroundContour(ZERO_VECTOR) + (string)llGroundNormal(ZERO_VECTOR) + (string)llGroundSlope(ZERO_VECTOR));
        llOwnerSay((string)llGetRegionFlags() + (string)llWind(ZERO_VECTOR) + (string)llScriptDanger(<1, 1, 1>) + (string)llGetSPMaxMemory());
      }
      touch_start(integer n) { llOwnerSay((string)llDetectedGrab(0)); }
      at_rot_target(integer n, rotation t, rotation o) { llRotTargetRemove(n); }
      not_at_rot_target() { llOwnerSay("not"); }
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QVERIFY2(a["warnings"].toArray().isEmpty(), qPrintable(QString::fromUtf8(QJsonDocument(a["warnings"].toArray()).toJson(QJsonDocument::Compact))));
}

QTEST_GUILESS_MAIN(LslSteps18Test)

#include "tst_lslsteps18.moc"
