/*
 * PinglyMotion - tests for llCastRay, llModifyLand, llRequestInventoryData, the texture reads and transforms, the
 * scale factor functions and the object animation functions (Script Lab, third gap list run, steps 13 to 15)
 *
 * The expected answers were worked out by hand from the Second Life wiki. Where the wiki is silent or unclear the
 * sandbox chooses; every choice is written here and in docs/CONTROL.md:
 *  - llCastRay: an object is one box (the box round all its prims along the region's axes, as llGetBoundingBox), an
 *    avatar a box of 0.45 x 0.6 x its height, the ground the flat plane at llGround inside the region. A ray that starts
 *    inside a box does not hit it (the wiki says so for the script's own prim). Hits come nearest first, at most
 *    RC_MAX_HITS (1 to 256; the default is 1). A hit on an object gives its root key (also without RC_GET_ROOT_KEY), the
 *    link number 0 for a one-prim object, an avatar and the ground and 1 for the root of a linkset (a guess), and the
 *    normal of the face the ray enters. The ground gives NULL_KEY. Worn objects are not hit. Phantom objects are hit
 *    only with RC_DETECT_PHANTOM. The list ends with the number of hits; no hit gives [0]. A list of options that is not
 *    pairs of integers, or has an unknown option, gives [RCERR_UNKNOWN] (a guess). Rejecting every type (15) stops the
 *    script with an error (the wiki: runtime error). The forced delay is not modelled.
 *  - llModifyLand is record only: one "land_edit" entry with action, brush, the object's place and "ok", "bad_value"
 *    (an action not 0 to 5 or a brush not 0 to 2) or "not_owner" (the parcel belongs to somebody else). The ground does
 *    not change, so llGround stays the same.
 *  - llRequestInventoryData works for a landmark only (the wiki: other item types are not supported): the dataserver
 *    event gives its place as a vector in text. A landmark without a "position" has <0,0,0> (a guess). Anything else, and a
 *    missing item, gives NULL_KEY and a line on DEBUG_CHANNEL (the text is a guess).
 *  - llOffsetTexture, llScaleTexture and llRotateTexture keep their values on the faces of the script's own prim (so the
 *    reads give them back) and are still recorded. llGetTextureOffset, llGetTextureRot and llGetTextureScale: ALL_SIDES
 *    gives face 0 (the wiki says so for llGetTextureOffset; a guess for the other two); a face the prim does not have
 *    gives <0,0,0> or 0.0.
 *  - llScaleByFactor multiplies every prim's size and every child's offset and gives TRUE; FALSE (nothing changes) when the
 *    factor is not above 0, a prim side would leave 0.01 to 64 m, a child would be more than 54 m from the root, the object
 *    is physical or in keyframed motion. A limit is reached with a tolerance of 1e-5 (relative), so a factor read from
 *    llGetMinScaleFactor or llGetMaxScaleFactor works. llGetMinScaleFactor is 0.01 over the smallest side of any prim;
 *    llGetMaxScaleFactor is the smaller of 64 over the largest side and 54 over the farthest child (a guess: the wiki
 *    gives only the limits).
 *  - Object animations are kept per script (the wiki says per prim) and only for animations of the inventory (type
 *    animation). A name twice is started once. llStopObjectAnimation takes the name or the key of the animation.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"

using namespace lsl;

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";
  const char* const KEY_T = "dddddddd-0000-4000-8000-000000000004";
}

class LslSteps131415Test : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void rayHitsAnObject();
    void rayFlagsAndLinkNumber();
    void rayGroundAndHitOrder();
    void rayRejects();
    void rayAvatar();
    void rayPhantom();
    void rayStartsInside();
    void rayBadOptionsAndErrors();
    void rayHitCount();
    void modifyLandRecords();
    void inventoryDataLandmark();
    void inventoryDataRefused();
    void landmarkPositionIsChecked();
    void textureReads();
    void textureTransformsAreRecordedToo();
    void scaleFactors();
    void scaleByFactorRefusals();
    void scaleByFactorLinkset();
    void objectAnimations();
    void checkerHasNoWarnings();

  private:
    static QJsonObject runOne(const QString& body, const QJsonArray& inputs = QJsonArray(), double seconds = 1, const QJsonObject& extra = QJsonObject());
    static QJsonObject runObjects(const QJsonArray& objects, double seconds = 2);
    // the caster at <10,10,50> runs body in a timer; the target at <50,10,50> is a 2 m cube (x 49 to 51, y 9 to 11, z 49 to 51)
    static QJsonObject castWith(const QString& body, const QString& targetScript = QString(), int targetPrims = 1);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type, const QString& field);
};

QJsonObject LslSteps131415Test::runOne(const QString& body, const QJsonArray& inputs, double seconds, const QJsonObject& extra)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", "default { state_entry() {\n" + body + "\n} }"}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  for (auto it = extra.begin(); it != extra.end(); ++it) request[it.key()] = it.value();
  return Control::run(request);
}

QJsonObject LslSteps131415Test::runObjects(const QJsonArray& objects, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", seconds}, {"objects", objects}});
}

QJsonObject LslSteps131415Test::castWith(const QString& body, const QString& targetScript, int targetPrims)
{
  const QString caster = "default { state_entry() { llSetTimerEvent(1.0); } timer() { llSetTimerEvent(0.0);\n" + body + "\n} }";
  const QString target = targetScript.isEmpty() ? QString("default { state_entry() { } }") : "default { state_entry() { " + targetScript + " } }";
  return runObjects(QJsonArray{
      QJsonObject{{"name", "Caster"}, {"source", caster}, {"position", QJsonArray{10, 10, 50}}},
      QJsonObject{{"name", "Target"}, {"key", KEY_T}, {"source", target}, {"position", QJsonArray{50, 10, 50}},
                  {"scale", QJsonArray{2, 2, 2}}, {"prims", targetPrims}}});
}

QStringList LslSteps131415Test::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslSteps131415Test::entries(const QJsonObject& answer, const QString& type, const QString& field)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != type || !e.contains(field)) continue;
    out << (e[field].isString() ? e[field].toString() : QString::fromUtf8(QJsonDocument(QJsonArray{e[field]}).toJson(QJsonDocument::Compact)));
  }
  return out;
}

void LslSteps131415Test::supportedNow()
{
  for (const char* name : {"llCastRay", "llModifyLand", "llRequestInventoryData", "llGetTextureOffset", "llGetTextureRot", "llGetTextureScale",
                           "llGetMinScaleFactor", "llGetMaxScaleFactor", "llScaleByFactor", "llGetObjectAnimationNames",
                           "llStartObjectAnimation", "llStopObjectAnimation"})
  {
    QVERIFY2(Builtins::function(name) != nullptr, name);
    QVERIFY2(Builtins::function(name)->supported, name);
  }
  // llNavigateTo is the unsupported example; llWarp2Pos is no LSL function
  QVERIFY(!Builtins::function("llNavigateTo")->supported);
  QVERIFY(Builtins::function("llWarp2Pos") == nullptr);
}

void LslSteps131415Test::rayHitsAnObject()
{
  // from x 20 to x 80 along y 10, z 50: the cube's face x = 49 is met at 29 of 60 metres; one hit, so the list is key, place, 1
  const QJsonObject a = castWith(R"lsl(
    list r = llCastRay(<20,10,50>, <80,10,50>, []);
    llOwnerSay(llDumpList2String(r, "|"));
    llOwnerSay((string)llGetListLength(r));
  )lsl");
  QCOMPARE(said(a), QStringList({QString("%1|<49.00000, 10.00000, 50.00000>|1").arg(KEY_T), "3"}));
}

void LslSteps131415Test::rayFlagsAndLinkNumber()
{
  // RC_GET_NORMAL (1) adds the normal of the face hit (the ray comes from -x, so <-1,0,0>), RC_GET_LINK_NUM (4) the link number
  // (0 for a single prim), the two together in the wiki's order: key, link, place, normal
  const QJsonObject a = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_DATA_FLAGS, RC_GET_NORMAL]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_DATA_FLAGS, RC_GET_LINK_NUM]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_DATA_FLAGS, RC_GET_NORMAL | RC_GET_LINK_NUM | RC_GET_ROOT_KEY]), "|"));
    // from the other side and from above
    llOwnerSay(llDumpList2String(llCastRay(<80,10,50>, <20,10,50>, [RC_DATA_FLAGS, RC_GET_NORMAL]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<50,10,70>, <50,10,40>, [RC_DATA_FLAGS, RC_GET_NORMAL]), "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({QString("%1|<49.00000, 10.00000, 50.00000>|<-1.00000, 0.00000, 0.00000>|1").arg(KEY_T),
                                 QString("%1|0|<49.00000, 10.00000, 50.00000>|1").arg(KEY_T),
                                 QString("%1|0|<49.00000, 10.00000, 50.00000>|<-1.00000, 0.00000, 0.00000>|1").arg(KEY_T),
                                 QString("%1|<51.00000, 10.00000, 50.00000>|<1.00000, 0.00000, 0.00000>|1").arg(KEY_T),
                                 QString("%1|<50.00000, 10.00000, 51.00000>|<0.00000, 0.00000, 1.00000>|1").arg(KEY_T)}));
  // the root of a linkset has link number 1
  const QJsonObject b = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_DATA_FLAGS, RC_GET_LINK_NUM]), "|"));
  )lsl", QString(), 3);
  QCOMPARE(said(b), QStringList({QString("%1|1|<49.00000, 10.00000, 50.00000>|1").arg(KEY_T)}));
}

void LslSteps131415Test::rayGroundAndHitOrder()
{
  // straight down at x 50: the cube's top at z 51 (9 of 60 m), then the ground at llGround = 20 (40 of 60 m); nearest first
  const QJsonObject a = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, 3, RC_DATA_FLAGS, RC_GET_NORMAL]), "|"));
    // one hit is the default: the nearest
    llOwnerSay(llDumpList2String(llCastRay(<50,10,60>, <50,10,0>, []), "|"));
    // beside the cube: the ground only, with the null key
    llOwnerSay(llDumpList2String(llCastRay(<20,10,60>, <20,10,0>, [RC_DATA_FLAGS, RC_GET_LINK_NUM]), "|"));
    // a ray that stops above the ground, and one that is level
    llOwnerSay(llDumpList2String(llCastRay(<20,10,60>, <20,10,30>, []), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,20>, <80,10,20>, []), "|"));
    // outside the region there is no ground
    llOwnerSay(llDumpList2String(llCastRay(<300,10,60>, <300,10,0>, []), "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({QString("%1|<50.00000, 10.00000, 51.00000>|<0.00000, 0.00000, 1.00000>|%2|<50.00000, 10.00000, 20.00000>|<0.00000, 0.00000, 1.00000>|2").arg(KEY_T, NULL_KEY),
                                 QString("%1|<50.00000, 10.00000, 51.00000>|1").arg(KEY_T),
                                 QString("%1|0|<20.00000, 10.00000, 20.00000>|1").arg(NULL_KEY),
                                 "0", "0", "0"}));
}

void LslSteps131415Test::rayRejects()
{
  // RC_REJECT_LAND (8) leaves only the cube; RC_REJECT_NONPHYSICAL (4) takes the cube out (it is not physical), leaving the ground;
  // RC_REJECT_PHYSICAL (2) does not touch it
  const QJsonObject a = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, 3, RC_REJECT_TYPES, RC_REJECT_LAND]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, 3, RC_REJECT_TYPES, RC_REJECT_NONPHYSICAL]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, 3, RC_REJECT_TYPES, RC_REJECT_PHYSICAL]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, 3, RC_REJECT_TYPES, RC_REJECT_LAND | RC_REJECT_NONPHYSICAL]), "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({QString("%1|<50.00000, 10.00000, 51.00000>|1").arg(KEY_T),
                                 QString("%1|<50.00000, 10.00000, 20.00000>|1").arg(NULL_KEY),
                                 QString("%1|<50.00000, 10.00000, 51.00000>|%2|<50.00000, 10.00000, 20.00000>|2").arg(KEY_T, NULL_KEY),
                                 "0"}));
  // a physical cube is rejected by RC_REJECT_PHYSICAL and kept by RC_REJECT_NONPHYSICAL
  const QJsonObject b = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<50,10,60>, <50,10,0>, [RC_REJECT_TYPES, RC_REJECT_PHYSICAL | RC_REJECT_LAND]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<50,10,60>, <50,10,0>, [RC_REJECT_TYPES, RC_REJECT_NONPHYSICAL | RC_REJECT_LAND]), "|"));
  )lsl", "llSetStatus(STATUS_PHYSICS, TRUE);");
  QCOMPARE(said(b), QStringList({"0", QString("%1|<50.00000, 10.00000, 51.00000>|1").arg(KEY_T)}));
}

void LslSteps131415Test::rayAvatar()
{
  // the owner stands at <128,128,20>: a box 0.45 by 0.6 by 1.8; the ray along x meets x = 128 - 0.225
  const QJsonObject a = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<100,128,20.5>, <160,128,20.5>, [RC_DATA_FLAGS, RC_GET_LINK_NUM]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<100,128,20.5>, <160,128,20.5>, [RC_REJECT_TYPES, RC_REJECT_AGENTS]), "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({QString("%1|0|<127.77500, 128.00000, 20.50000>|1").arg(OWNER), "0"}));
}

void LslSteps131415Test::rayPhantom()
{
  // a phantom object is hit only with RC_DETECT_PHANTOM (also when RC_REJECT_NONPHYSICAL would reject it)
  const QJsonObject a = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, []), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_DETECT_PHANTOM, TRUE]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_DETECT_PHANTOM, TRUE, RC_REJECT_TYPES, RC_REJECT_NONPHYSICAL]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_DETECT_PHANTOM, FALSE]), "|"));
  )lsl", "llSetStatus(STATUS_PHANTOM, TRUE);");
  QCOMPARE(said(a), QStringList({"0", QString("%1|<49.00000, 10.00000, 50.00000>|1").arg(KEY_T),
                                 QString("%1|<49.00000, 10.00000, 50.00000>|1").arg(KEY_T), "0"}));
}

void LslSteps131415Test::rayStartsInside()
{
  // from inside the cube the cube is not hit (the wiki); the ground below is
  const QJsonObject a = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<50,10,50>, <80,10,50>, []), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<50,10,50>, <50,10,0>, []), "|"));
    // a ray that stops before the cube, and a ray of no length
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <40,10,50>, []), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <20,10,50>, []), "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({"0", QString("%1|<50.00000, 10.00000, 20.00000>|1").arg(NULL_KEY), "0", "0"}));
}

void LslSteps131415Test::rayBadOptionsAndErrors()
{
  // [RCERR_UNKNOWN]: an odd list, a name that is no option, a value that is no integer, an option that is no integer
  const QJsonObject a = castWith(R"lsl(
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_MAX_HITS]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [9, 1]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, [RC_MAX_HITS, 1.5]), "|"));
    llOwnerSay(llDumpList2String(llCastRay(<20,10,50>, <80,10,50>, ["a", 1]), "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({"-1", "-1", "-1", "-1"}));
  // rejecting every type (1 + 2 + 4 + 8) is a runtime error
  const QJsonObject b = castWith(R"lsl(
    llOwnerSay("before");
    llCastRay(<20,10,50>, <80,10,50>, [RC_REJECT_TYPES, RC_REJECT_AGENTS | RC_REJECT_PHYSICAL | RC_REJECT_NONPHYSICAL | RC_REJECT_LAND]);
    llOwnerSay("after");
  )lsl");
  QCOMPARE(said(b), QStringList({"before"}));
  QCOMPARE(entries(b, "error", "message"),
           QStringList({"llCastRay: every hit type is rejected (RC_REJECT_TYPES 15), so nothing can be hit. Remove one of the RC_REJECT_* flags."}));
}

void LslSteps131415Test::rayHitCount()
{
  // RC_MAX_HITS is 1 to 256: 0 and negative numbers count as 1, 1000 as 256. Only the cube and the ground can be
  // hit here, so two hits at most; two hits are four list entries and the count
  const QJsonObject a = castWith(R"lsl(
    list r = llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, 0]);
    llOwnerSay((string)llList2Integer(r, -1));
    r = llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, -5]);
    llOwnerSay((string)llList2Integer(r, -1));
    r = llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, 1000]);
    llOwnerSay((string)llList2Integer(r, -1));
    r = llCastRay(<50,10,60>, <50,10,0>, [RC_MAX_HITS, 2]);
    llOwnerSay((string)llList2Integer(r, -1) + " " + (string)llGetListLength(r));
  )lsl");
  QCOMPARE(said(a), QStringList({"1", "1", "2", "2 5"}));
}

void LslSteps131415Test::modifyLandRecords()
{
  const QJsonObject a = runOne(R"lsl(
    float before = llGround(<0,0,0>);
    llModifyLand(LAND_RAISE, LAND_BRUSH_LARGE);
    llModifyLand(LAND_LEVEL, LAND_BRUSH_SMALL);
    llModifyLand(LAND_REVERT, LAND_BRUSH_MEDIUM);
    llModifyLand(6, LAND_BRUSH_SMALL);
    llModifyLand(LAND_NOISE, 3);
    llOwnerSay((string)(llGround(<0,0,0>) - before));
  )lsl");
  QCOMPARE(said(a), QStringList({"0.000000"}));
  QCOMPARE(entries(a, "land_edit", "action"), QStringList({"[1]", "[0]", "[5]", "[6]", "[4]"}));
  QCOMPARE(entries(a, "land_edit", "brush"), QStringList({"[2]", "[0]", "[1]", "[0]", "[3]"}));
  QCOMPARE(entries(a, "land_edit", "name"), QStringList({"raise", "level", "revert", "noise"}));
  QCOMPARE(entries(a, "land_edit", "size_m"), QStringList({"[8]", "[2]", "[4]", "[2]"}));
  QCOMPARE(entries(a, "land_edit", "result"), QStringList({"ok", "ok", "ok", "bad_value", "bad_value"}));
  QCOMPARE(entries(a, "land_edit", "position"), QStringList({"[[128,128,20]]", "[[128,128,20]]", "[[128,128,20]]", "[[128,128,20]]", "[[128,128,20]]"}));
}

void LslSteps131415Test::inventoryDataLandmark()
{
  const QJsonArray inventory{QJsonObject{{"name", "Home"}, {"type", "landmark"}, {"position", QJsonArray{10, 20.5, 30}}},
                             QJsonObject{{"name", "Nowhere"}, {"type", "landmark"}}};
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 3}, {"inventory", inventory}, {"source", R"lsl(
    key gHome; key gNone;
    default {
      state_entry() {
        gHome = llRequestInventoryData("Home");
        gNone = llRequestInventoryData("Nowhere");
        llOwnerSay("asked " + (string)(gHome != NULL_KEY) + (string)(gNone != NULL_KEY) + (string)(gHome != gNone));
      }
      dataserver(key q, string data) {
        vector v = (vector)data;
        string who = "?";
        if (q == gHome) who = "Home";
        if (q == gNone) who = "Nowhere";
        llOwnerSay(who + " " + data + " x=" + (string)v.x);
      }
    }
  )lsl"}});
  QCOMPARE(said(r), QStringList({"asked 111", "Home <10.00000, 20.50000, 30.00000> x=10.000000", "Nowhere <0.00000, 0.00000, 0.00000> x=0.000000"}));
  QCOMPARE(entries(r, "dataserver_request", "function"), QStringList({"llRequestInventoryData", "llRequestInventoryData"}));
  QCOMPARE(entries(r, "dataserver_request", "item"), QStringList({"Home", "Nowhere"}));
}

void LslSteps131415Test::inventoryDataRefused()
{
  const QJsonArray inventory{QJsonObject{{"name", "Card"}, {"type", "notecard"}, {"lines", QJsonArray{"hello"}}},
                             QJsonObject{{"name", "Sound"}, {"type", "sound"}}};
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 2}, {"inventory", inventory}, {"source", R"lsl(
    default {
      state_entry() {
        llOwnerSay((string)llRequestInventoryData("Card"));
        llOwnerSay((string)llRequestInventoryData("Sound"));
        llOwnerSay((string)llRequestInventoryData("Missing"));
      }
      dataserver(key q, string data) { llOwnerSay("unexpected"); }
    }
  )lsl"}});
  const QString none = NULL_KEY;
  QCOMPARE(said(r), QStringList({none, none, none}));
  QCOMPARE(entries(r, "debug", "text"), QStringList({"llRequestInventoryData: only landmarks are supported, 'Card' is not one",
                                                     "llRequestInventoryData: only landmarks are supported, 'Sound' is not one",
                                                     "Couldn't find item 'Missing'"}));
}

void LslSteps131415Test::landmarkPositionIsChecked()
{
  auto run = [](const QJsonObject& item)
  {
    return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { } }"}, {"inventory", QJsonArray{item}}});
  };
  QVERIFY(run(QJsonObject{{"name", "L"}, {"type", "landmark"}, {"position", QJsonArray{1, 2, 3}}})["ok"].toBool());
  const QString only = "inventory item 1: \"position\" is only for a landmark.";
  QVERIFY2(run(QJsonObject{{"name", "L"}, {"type", "sound"}, {"position", QJsonArray{1, 2, 3}}})["error"].toString().contains(only), "sound");
  const QString shape = "\"position\" must be a list of three numbers (x, y, z).";
  QVERIFY(run(QJsonObject{{"name", "L"}, {"type", "landmark"}, {"position", QJsonArray{1, 2}}})["error"].toString().contains(shape));
  QVERIFY(run(QJsonObject{{"name", "L"}, {"type", "landmark"}, {"position", QJsonArray{1, 2, "x"}}})["error"].toString().contains(shape));
  QVERIFY(run(QJsonObject{{"name", "L"}, {"type", "landmark"}, {"position", "here"}})["error"].toString().contains(shape));
  QVERIFY(run(QJsonObject{{"name", "L"}, {"type", "landmark"}, {"position", QJsonArray{1, 2, 1.0e9}}})["error"].toString().contains(shape));
}

void LslSteps131415Test::textureReads()
{
  const QJsonObject a = runOne(R"lsl(
    // a new prim: offset <0,0>, repeats <1,1>, rotation 0
    llOwnerSay((string)llGetTextureOffset(0) + " " + (string)llGetTextureScale(0) + " " + (string)llGetTextureRot(0));
    llSetPrimitiveParams([PRIM_TEXTURE, 2, "tex", <3.0, 4.0, 0.0>, <0.25, -0.5, 0.0>, 1.5]);
    llOwnerSay((string)llGetTextureOffset(2) + " " + (string)llGetTextureScale(2) + " " + (string)llGetTextureRot(2));
    // the other faces are untouched; ALL_SIDES reads face 0
    llOwnerSay((string)llGetTextureOffset(1) + " " + (string)llGetTextureOffset(ALL_SIDES));
    llSetPrimitiveParams([PRIM_TEXTURE, ALL_SIDES, "tex", <2.0, 2.0, 0.0>, <0.5, 0.5, 0.0>, 0.5]);
    llOwnerSay((string)llGetTextureOffset(ALL_SIDES) + " " + (string)llGetTextureScale(ALL_SIDES) + " " + (string)llGetTextureRot(ALL_SIDES));
    // a face the prim does not have
    llOwnerSay((string)llGetTextureOffset(6) + " " + (string)llGetTextureScale(6) + " " + (string)llGetTextureRot(6));
    llOwnerSay((string)llGetTextureOffset(-2) + " " + (string)llGetTextureScale(-2) + " " + (string)llGetTextureRot(-2));
  )lsl");
  QCOMPARE(said(a), QStringList({"<0.00000, 0.00000, 0.00000> <1.00000, 1.00000, 0.00000> 0.000000",
                                 "<0.25000, -0.50000, 0.00000> <3.00000, 4.00000, 0.00000> 1.500000",
                                 "<0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000>",
                                 "<0.50000, 0.50000, 0.00000> <2.00000, 2.00000, 0.00000> 0.500000",
                                 "<0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000> 0.000000",
                                 "<0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000> 0.000000"}));
}

void LslSteps131415Test::textureTransformsAreRecordedToo()
{
  const QJsonObject a = runOne(R"lsl(
    llOffsetTexture(0.5, -0.5, 2);
    llScaleTexture(2.0, 3.0, 2);
    llRotateTexture(1.25, 2);
    llOwnerSay((string)llGetTextureOffset(2) + " " + (string)llGetTextureScale(2) + " " + (string)llGetTextureRot(2));
    llOwnerSay((string)llGetTextureOffset(3) + " " + (string)llGetTextureScale(3) + " " + (string)llGetTextureRot(3));
    // ALL_SIDES sets every face
    llOffsetTexture(0.125, 0.0, ALL_SIDES);
    llRotateTexture(0.5, ALL_SIDES);
    llOwnerSay((string)llGetTextureOffset(5) + " " + (string)llGetTextureRot(0));
    // a face the prim does not have changes nothing
    llScaleTexture(9.0, 9.0, 6);
    llOwnerSay((string)llGetTextureScale(0));
  )lsl");
  QCOMPARE(said(a), QStringList({"<0.50000, -0.50000, 0.00000> <2.00000, 3.00000, 0.00000> 1.250000",
                                 "<0.00000, 0.00000, 0.00000> <1.00000, 1.00000, 0.00000> 0.000000",
                                 "<0.12500, 0.00000, 0.00000> 0.500000", "<1.00000, 1.00000, 0.00000>"}));
  // still recorded, as before
  QCOMPARE(entries(a, "recorded_only", "function"), QStringList({"llOffsetTexture", "llScaleTexture", "llRotateTexture", "llOffsetTexture", "llRotateTexture", "llScaleTexture"}));
}

void LslSteps131415Test::scaleFactors()
{
  // the default prim is 0.5 m: smallest factor 0.01 / 0.5 = 0.02, largest 64 / 0.5 = 128
  const QJsonObject a = runOne(R"lsl(
    llOwnerSay((string)llGetMinScaleFactor() + " " + (string)llGetMaxScaleFactor());
    llOwnerSay((string)llGetScale());
    llOwnerSay((string)llScaleByFactor(2.0));
    llOwnerSay((string)llGetScale());
    llOwnerSay((string)llGetMinScaleFactor() + " " + (string)llGetMaxScaleFactor());
    // the exact limits work: 64 for a 1 m prim, then 0.01 / 64 for the 64 m prim
    llOwnerSay((string)llScaleByFactor(llGetMaxScaleFactor()));
    llOwnerSay((string)llGetScale());
    llOwnerSay((string)llScaleByFactor(llGetMinScaleFactor()));
    llOwnerSay((string)llGetScale());
  )lsl");
  QCOMPARE(said(a), QStringList({"0.020000 128.000000", "<0.50000, 0.50000, 0.50000>", "1", "<1.00000, 1.00000, 1.00000>",
                                 "0.010000 64.000000", "1", "<64.00000, 64.00000, 64.00000>", "1", "<0.01000, 0.01000, 0.01000>"}));
  QCOMPARE(entries(a, "prim", "factor").size(), 3);
}

void LslSteps131415Test::scaleByFactorRefusals()
{
  const QJsonObject a = runOne(R"lsl(
    // not above 0
    llOwnerSay((string)llScaleByFactor(0.0) + (string)llScaleByFactor(-1.0));
    // too small: 0.5 * 0.01 = 0.005; too large: 0.5 * 200 = 100
    llOwnerSay((string)llScaleByFactor(0.01) + (string)llScaleByFactor(200.0));
    llOwnerSay((string)llGetScale());
    // physical
    llSetStatus(STATUS_PHYSICS, TRUE);
    llOwnerSay((string)llScaleByFactor(2.0));
    llOwnerSay((string)llGetScale());
  )lsl");
  QCOMPARE(said(a), QStringList({"00", "00", "<0.50000, 0.50000, 0.50000>", "0", "<0.50000, 0.50000, 0.50000>"}));
  QCOMPARE(entries(a, "prim", "factor").size(), 0);
}

void LslSteps131415Test::scaleByFactorLinkset()
{
  // root 0.5 m; the child 1 m, 10 m away: smallest side 0.5 -> 0.02; largest side 1 -> 64, farthest child 10 -> 54 / 10 = 5.4
  const QJsonObject a = runObjects(QJsonArray{QJsonObject{{"name", "Set"}, {"prims", 2}, {"source", R"lsl(
    default {
      state_entry() {
        llSetLinkPrimitiveParamsFast(2, [PRIM_SIZE, <1,1,1>, PRIM_POS_LOCAL, <10,0,0>]);
        llOwnerSay((string)llGetMinScaleFactor() + " " + (string)llGetMaxScaleFactor());
        llOwnerSay((string)llScaleByFactor(6.0));
        llOwnerSay((string)llScaleByFactor(2.0));
        llOwnerSay(llList2String(llGetLinkPrimitiveParams(2, [PRIM_SIZE]), 0) + " " + llList2String(llGetLinkPrimitiveParams(2, [PRIM_POS_LOCAL]), 0));
        llOwnerSay((string)llGetScale());
        llOwnerSay((string)llGetMaxScaleFactor());
      }
    }
  )lsl"}}});
  // 6 would put the child 60 m away: refused. 2 gives root 1, child 2 m at 20 m. The largest factor is now 54 / 20 = 2.7
  QCOMPARE(said(a), QStringList({"0.020000 5.400000", "0", "1", "<2.00000, 2.00000, 2.00000> <20.00000, 0.00000, 0.00000>", "<1.00000, 1.00000, 1.00000>", "2.700000"}));
  QCOMPARE(entries(a, "prim", "factor"), QStringList({"[2]", "[2]"}));
}

void LslSteps131415Test::objectAnimations()
{
  const QJsonArray inventory{QJsonObject{{"name", "walk"}, {"type", "animation"}, {"key", "11111111-1111-4111-8111-111111111111"}},
                             QJsonObject{{"name", "wave"}, {"type", "animation"}},
                             QJsonObject{{"name", "Card"}, {"type", "notecard"}}};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"inventory", inventory}, {"source", R"lsl(
    default {
      state_entry() {
        llOwnerSay("[" + llDumpList2String(llGetObjectAnimationNames(), ",") + "]");
        llStartObjectAnimation("wave");
        llStartObjectAnimation("walk");
        llStartObjectAnimation("wave");
        llOwnerSay("[" + llDumpList2String(llGetObjectAnimationNames(), ",") + "]");
        // not an animation in the inventory
        llStartObjectAnimation("Card");
        llStartObjectAnimation("nothing");
        llOwnerSay((string)llGetListLength(llGetObjectAnimationNames()));
        llStopObjectAnimation("wave");
        llOwnerSay("[" + llDumpList2String(llGetObjectAnimationNames(), ",") + "]");
        // an animation that is not playing, a name that is none
        llStopObjectAnimation("wave");
        llStopObjectAnimation("nothing");
        // by key
        llStopObjectAnimation("11111111-1111-4111-8111-111111111111");
        llOwnerSay("[" + llDumpList2String(llGetObjectAnimationNames(), ",") + "]");
        // and again: a stopped animation can be started
        llStartObjectAnimation("walk");
        llOwnerSay("[" + llDumpList2String(llGetObjectAnimationNames(), ",") + "]");
      }
    }
  )lsl"}});
  QCOMPARE(said(a), QStringList({"[]", "[wave,walk]", "2", "[walk]", "[]", "[walk]"}));
  QCOMPARE(entries(a, "object_animation", "op"), QStringList({"start", "start", "stop", "stop", "start"}));
  QCOMPARE(entries(a, "object_animation", "animation"), QStringList({"wave", "walk", "wave", "walk", "walk"}));
  QCOMPARE(entries(a, "debug", "text"), QStringList({"Couldn't find animation 'Card'", "Couldn't find animation 'nothing'"}));
}

void LslSteps131415Test::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default {
      state_entry() {
        list r = llCastRay(<0,0,10>, <0,0,0>, [RC_MAX_HITS, 2]);
        llModifyLand(LAND_LEVEL, LAND_BRUSH_SMALL);
        key k = llRequestInventoryData("lm");
        vector o = llGetTextureOffset(0); float t = llGetTextureRot(0); vector s = llGetTextureScale(0);
        float lo = llGetMinScaleFactor(); float hi = llGetMaxScaleFactor(); integer ok = llScaleByFactor(1.0);
        llStartObjectAnimation("a"); llStopObjectAnimation("a"); list n = llGetObjectAnimationNames();
      }
      dataserver(key q, string d) { }
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

QTEST_GUILESS_MAIN(LslSteps131415Test)
#include "tst_lslsteps131415.moc"
