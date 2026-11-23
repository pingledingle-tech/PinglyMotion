/*
 * PinglyMotion - tests for the land and parcel functions (Script Lab, second gap list)
 *
 * The sandbox has no parcel model: the whole 256 by 256 m region is one parcel with canned values (RegionSettings in
 * world.h). Ban list, pass list, map, eject and return functions only write a recorded_only entry; the reads give the
 * canned answers. Every expected value was worked out by hand from the rules in docs/CONTROL.md ("Land and parcel").
 * Where the Second Life wiki is unclear the function gives its documented return type with a harmless value:
 *  - llReturnObjectsByOwner and llReturnObjectsByID return 0 (no error); the sandbox holds nothing of anyone else's.
 *  - A position outside the region is on no parcel: NULL_KEY, 0, 0, [] (the wiki does not say what the grid gives).
 *  - An unknown PARCEL_DETAILS_* number is skipped, the way llGetObjectDetails skips an unknown value.
 *  - PARCEL_DETAILS_TP_ROUTING is 2 (teleports land anywhere) and the landing point and look-at are ZERO_VECTOR.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/world.h"

using namespace lsl;

class LslLandTest : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void recordedOnly();
    void landOwnerAndOverMyLand();
    void parcelFlags();
    void parcelCountsAndOwners();
    void parcelDetails();
    void parcelDetailsOffRegion();
    void parcelMusic();
    void returnObjects();
    void settingsChangeTheAnswers();
    void asinAndLog10();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& body);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QString rec(const QString& function, const QString& args)
    {
      return QString(R"({"args":%1,"function":"%2","note":"Recorded only: the sandbox plays and shows nothing for this.","type":"recorded_only"})").arg(args, function);
    }
};

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";
  const char* const GROUP = "9a0c0000-0000-4000-8000-000000000001";
  const char* const PARCEL_ID = "5a7d0000-0000-4000-8000-000000000001";
}

QJsonObject LslLandTest::run(const QString& body)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() {\n" + body + "\n} }"}, {"seconds", 1}});
}

QStringList LslLandTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslLandTest::entries(const QJsonObject& answer, const QString& type)
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

void LslLandTest::supportedNow()
{
  for (const char* f : {"llAddToLandBanList", "llRemoveFromLandBanList", "llResetLandBanList", "llAddToLandPassList", "llRemoveFromLandPassList",
                        "llResetLandPassList", "llGetLandOwnerAt", "llOverMyLand", "llMapDestination", "llGetParcelFlags", "llGetParcelPrimCount",
                        "llGetParcelMaxPrims", "llGetParcelPrimOwners", "llGetParcelDetails", "llSetParcelMusicURL", "llGetParcelMusicURL",
                        "llReturnObjectsByOwner", "llReturnObjectsByID", "llEjectFromLand"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

void LslLandTest::recordedOnly()
{
  const QJsonObject a = run(R"lsl(
    llAddToLandBanList("11111111-2222-3333-4444-555555555555", 2.5);
    llRemoveFromLandBanList("11111111-2222-3333-4444-555555555555");
    llResetLandBanList();
    llAddToLandPassList("66666666-7777-8888-9999-000000000000", 0.0);
    llRemoveFromLandPassList("66666666-7777-8888-9999-000000000000");
    llResetLandPassList();
    llMapDestination("Somewhere", <128, 64, 30>, <0, 1, 0>);
    llEjectFromLand("11111111-2222-3333-4444-555555555555");
    llOwnerSay("done");
  )lsl");
  QCOMPARE(said(a), QStringList{"done"});
  const QStringList expected({
    rec("llAddToLandBanList", R"(["11111111-2222-3333-4444-555555555555",2.5])"),
    rec("llRemoveFromLandBanList", R"(["11111111-2222-3333-4444-555555555555"])"),
    rec("llResetLandBanList", "[]"),
    rec("llAddToLandPassList", R"(["66666666-7777-8888-9999-000000000000",0])"),
    rec("llRemoveFromLandPassList", R"(["66666666-7777-8888-9999-000000000000"])"),
    rec("llResetLandPassList", "[]"),
    rec("llMapDestination", R"(["Somewhere",[128,64,30],[0,1,0]])"),
    rec("llEjectFromLand", R"(["11111111-2222-3333-4444-555555555555"])")});
  QCOMPARE(entries(a, "recorded_only"), expected);
}

void LslLandTest::landOwnerAndOverMyLand()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay(llGetLandOwnerAt(<10, 20, 30>));
    llOwnerSay(llGetLandOwnerAt(<255.5, 0, 0>));
    llOwnerSay(llGetLandOwnerAt(<256, 10, 0>));
    llOwnerSay(llGetLandOwnerAt(<10, -0.5, 0>));
    llOwnerSay((string)llOverMyLand(llGetKey()));
    llOwnerSay((string)llOverMyLand(llGetOwner()));
    llOwnerSay((string)llOverMyLand("11111111-2222-3333-4444-555555555555"));
  )lsl");
  QCOMPARE(said(a), QStringList({OWNER, OWNER, NULL_KEY, NULL_KEY, "1", "1", "0"}));
}

void LslLandTest::parcelFlags()
{
  // fly 0x1 + scripts 0x2 + landmark 0x8 + create objects 0x40 + all object entry 0x08000000 = 0x0800004B = 134217803
  const QJsonObject a = run(R"lsl(
    integer f = llGetParcelFlags(<1, 1, 1>);
    llOwnerSay((string)f);
    llOwnerSay((string)((f & PARCEL_FLAG_ALLOW_FLY) != 0) + (string)((f & PARCEL_FLAG_ALLOW_SCRIPTS) != 0)
      + (string)((f & PARCEL_FLAG_ALLOW_DAMAGE) != 0) + (string)((f & PARCEL_FLAG_USE_BAN_LIST) != 0));
    llOwnerSay((string)llGetParcelFlags(<300, 1, 1>));
  )lsl");
  QCOMPARE(said(a), QStringList({"134217803", "1100", "0"}));
}

void LslLandTest::parcelCountsAndOwners()
{
  // one object with one prim: the count is 1 for TOTAL and OWNER and 0 for the other categories; 15000 prims in a region
  const QJsonObject a = run(R"lsl(
    string c = ",";
    llOwnerSay(llDumpList2String([llGetParcelPrimCount(<1, 1, 1>, PARCEL_COUNT_TOTAL, FALSE),
      llGetParcelPrimCount(<1, 1, 1>, PARCEL_COUNT_OWNER, FALSE), llGetParcelPrimCount(<1, 1, 1>, PARCEL_COUNT_GROUP, FALSE),
      llGetParcelPrimCount(<1, 1, 1>, PARCEL_COUNT_OTHER, FALSE), llGetParcelPrimCount(<1, 1, 1>, PARCEL_COUNT_SELECTED, FALSE),
      llGetParcelPrimCount(<1, 1, 1>, PARCEL_COUNT_TEMP, FALSE)], c));
    llOwnerSay(llDumpList2String([llGetParcelPrimCount(<500, 1, 1>, PARCEL_COUNT_TOTAL, FALSE),
      llGetParcelPrimCount(<500, 1, 1>, PARCEL_COUNT_TOTAL, TRUE)], c));
    llOwnerSay(llDumpList2String([llGetParcelMaxPrims(<1, 1, 1>, FALSE), llGetParcelMaxPrims(<1, 1, 1>, TRUE),
      llGetParcelMaxPrims(<500, 1, 1>, FALSE), llGetParcelMaxPrims(<500, 1, 1>, TRUE)], c));
    list o = llGetParcelPrimOwners(<1, 1, 1>);
    llOwnerSay((string)llGetListLength(o) + " " + llList2String(o, 0) + " " + (string)llList2Integer(o, 1));
    llOwnerSay((string)llGetListLength(llGetParcelPrimOwners(<500, 1, 1>)));
  )lsl");
  QCOMPARE(said(a), QStringList({"1,1,0,0,0,0", "0,1", "15000,15000,0,15000", QString("2 %1 1").arg(OWNER), "0"}));
}

void LslLandTest::parcelDetails()
{
  const QJsonObject a = run(R"lsl(
    string c = "|";
    vector p = <1, 1, 1>;
    llOwnerSay(llDumpList2String(llGetParcelDetails(p, [PARCEL_DETAILS_NAME, PARCEL_DETAILS_DESC, PARCEL_DETAILS_OWNER, PARCEL_DETAILS_GROUP]), "|"));
    llOwnerSay(llDumpList2String(llGetParcelDetails(p, [PARCEL_DETAILS_AREA, PARCEL_DETAILS_ID, PARCEL_DETAILS_SEE_AVATARS]), "|"));
    llOwnerSay(llDumpList2String(llGetParcelDetails(p, [PARCEL_DETAILS_PRIM_CAPACITY, PARCEL_DETAILS_PRIM_USED, PARCEL_DETAILS_LANDING_POINT,
      PARCEL_DETAILS_LANDING_LOOKAT, PARCEL_DETAILS_TP_ROUTING, PARCEL_DETAILS_FLAGS, PARCEL_DETAILS_SCRIPT_DANGER]), "|"));
    llOwnerSay((string)llGetListLength(llGetParcelDetails(p, [PARCEL_DETAILS_NAME, 99, -1, PARCEL_DETAILS_AREA])));
    llOwnerSay((string)llGetListLength(llGetParcelDetails(p, [])));
  )lsl");
  const QStringList expected({
    QString("Sandbox Parcel||%1|%2").arg(OWNER, GROUP),
    QString("65536|%1|1").arg(PARCEL_ID),
    "15000|1|<0.00000, 0.00000, 0.00000>|<0.00000, 0.00000, 0.00000>|2|134217803|0",
    "2", "0"});
  QCOMPARE(said(a), expected);
}

void LslLandTest::parcelDetailsOffRegion()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetListLength(llGetParcelDetails(<256, 0, 0>, [PARCEL_DETAILS_NAME, PARCEL_DETAILS_AREA])));
    llOwnerSay((string)llGetListLength(llGetParcelDetails(<0, 0, 0>, [PARCEL_DETAILS_NAME, PARCEL_DETAILS_AREA])));
  )lsl");
  QCOMPARE(said(a), QStringList({"0", "2"}));
}

void LslLandTest::parcelMusic()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay("[" + llGetParcelMusicURL() + "]");
    llSetParcelMusicURL("http://example.org/stream");
    llOwnerSay("[" + llGetParcelMusicURL() + "]");
    llSetParcelMusicURL("");
    llOwnerSay("[" + llGetParcelMusicURL() + "]");
  )lsl");
  QCOMPARE(said(a), QStringList({"[]", "[http://example.org/stream]", "[]"}));
  QCOMPARE(entries(a, "recorded_only"), QStringList({rec("llSetParcelMusicURL", "[\"http://example.org/stream\"]"), rec("llSetParcelMusicURL", "[\"\"]")}));
}

void LslLandTest::returnObjects()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llReturnObjectsByOwner("11111111-2222-3333-4444-555555555555", OBJECT_RETURN_PARCEL));
    llOwnerSay((string)llReturnObjectsByID(["c0ffee00-0000-4000-8000-000000000009"]));
  )lsl");
  QCOMPARE(said(a), QStringList({"0", "0"}));
  QCOMPARE(entries(a, "recorded_only"), QStringList({rec("llReturnObjectsByOwner", R"(["11111111-2222-3333-4444-555555555555",1])"),
                                                     rec("llReturnObjectsByID", "[1]")}));
}

void LslLandTest::settingsChangeTheAnswers()
{
  // C++ only: a world whose parcel belongs to somebody else (no lsl_run option sets this yet)
  RegionSettings r;
  QCOMPARE(r.parcelOwner, QString());
  QCOMPARE(r.parcelArea, 65536);
  QCOMPARE(r.parcelMaxPrims, 15000);
  QCOMPARE(r.parcelFlags, 0x0800004B);
  QCOMPARE(RegionSettings::SIZE, 256.0f);
  World w;
  w.setRegion(r);
  QCOMPARE(w.region().parcelOwner, QString());
  r.parcelOwner = "99999999-9999-4999-8999-999999999999";
  w.setRegion(r);
  QCOMPARE(w.region().parcelOwner, QString("99999999-9999-4999-8999-999999999999"));
  w.setParcelMusicUrl("x");
  QCOMPARE(w.parcelMusicUrl(), QString("x"));
}

void LslLandTest::asinAndLog10()
{
  // llAsin(1) is pi / 2; llLog10 of 1000 is 3 and of 0 or less is 0 (as llLog)
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llAsin(1.0) + " " + (string)llAsin(0.0) + " " + (string)llAsin(-1.0));
    llOwnerSay((string)llLog10(1000.0) + " " + (string)llLog10(1.0) + " " + (string)llLog10(0.0) + " " + (string)llLog10(-5.0));
  )lsl");
  QCOMPARE(said(a), QStringList({"1.570796 0.000000 -1.570796", "3.000000 0.000000 0.000000 0.000000"}));
}

void LslLandTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llAddToLandBanList(NULL_KEY, 1.0); llRemoveFromLandBanList(NULL_KEY); llResetLandBanList();
      llAddToLandPassList(NULL_KEY, 1.0); llRemoveFromLandPassList(NULL_KEY); llResetLandPassList();
      llMapDestination("r", ZERO_VECTOR, ZERO_VECTOR); llEjectFromLand(NULL_KEY);
      llOwnerSay((string)llGetLandOwnerAt(ZERO_VECTOR) + (string)llOverMyLand(NULL_KEY) + (string)llGetParcelFlags(ZERO_VECTOR)
        + (string)llGetParcelPrimCount(ZERO_VECTOR, 0, FALSE) + (string)llGetParcelMaxPrims(ZERO_VECTOR, FALSE)
        + llDumpList2String(llGetParcelPrimOwners(ZERO_VECTOR), " ") + llDumpList2String(llGetParcelDetails(ZERO_VECTOR, [0]), " ")
        + llGetParcelMusicURL());
      llSetParcelMusicURL("u");
      llOwnerSay((string)llReturnObjectsByOwner(NULL_KEY, OBJECT_RETURN_PARCEL) + (string)llReturnObjectsByID([NULL_KEY]));
    } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

QTEST_GUILESS_MAIN(LslLandTest)

#include "tst_lslland.moc"
