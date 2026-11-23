/*
 * PinglyMotion - tests for llSetRegionPos, the pure maths and string functions and the force, impulse and velocity
 * records (Script Lab, third gap list run, steps 4 to 6)
 *
 * The expected numbers of llListStatistics, llModPow and llXorBase64* were worked out independently with Python
 * (statistics, pow, base64), the llRot2Angle ones from the wiki's formula. Where the Second Life wiki is unclear the
 * function gives its documented return type with a harmless canned value:
 *  - llWarp2Pos is no LSL function (neither the viewer's keyword list nor Second Life's definitions know it), so it stays
 *    unknown and a script that calls it gets "not supported yet".
 *  - llSetRegionPos has no 10 m limit (the wiki documents none, unlike llSetPos); TRUE and the move for x and y from -10
 *    to 266 and z from 0 to 4096, FALSE and no move outside, for a physical object and for an attachment. The ground snap
 *    for a target below the terrain is not modelled.
 *  - llListStatistics: the median of an even count is the mean of the two middle values, STD_DEV is the sample standard
 *    deviation (n - 1; 0.0 for one value), GEOMETRIC_MEAN is 0.0 for a negative product or a zero (the wiki contradicts
 *    itself there), an empty list gives 0.0.
 *  - llXorBase64Strings behaves like llXorBase64 (the wiki does not describe its old bug); llXorBase64StringsCorrect also
 *    ends the key at its first NUL byte, as the wiki says. An empty key returns the first string unchanged.
 *  - llModPow reads its three arguments as unsigned 32-bit values, the answer as a signed integer; modulus 0 gives 0.
 *  - llSetForce stores the force (turned into region axes when local), llGetForce reads it back; the motion model does not
 *    use it. llSetVelocity sets the velocity (local turned by the object's rotation) with no physics check, like
 *    llApplyImpulse. llApplyRotationalImpulse and llGroundRepel are recorded only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslSteps456Test : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void warp2PosStaysUnknown();
    void regionPosMoves();
    void regionPosFarAndOutside();
    void regionPosPhysical();
    void statisticsWikiExample();
    void statisticsEdgeCases();
    void rot2Angle();
    void xorBase64();
    void xorBase64Edge();
    void modPow();
    void forceRecordAndRead();
    void forceLocal();
    void velocitySet();
    void rotationalImpulseAndGroundRepel();
    void checkerHasNoWarnings();
    void oldXorFunctionsAreDeprecated();

  private:
    static QJsonObject run(const QString& body);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
};

QJsonObject LslSteps456Test::run(const QString& body)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() {\n" + body + "\n} }"}, {"seconds", 1}});
}

QStringList LslSteps456Test::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "owner_say") out << e["text"].toString();
  }
  return out;
}

QStringList LslSteps456Test::entries(const QJsonObject& answer, const QString& type)
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

void LslSteps456Test::supportedNow()
{
  for (const char* f : {"llSetRegionPos", "llListStatistics", "llRot2Angle", "llXorBase64", "llXorBase64Strings",
                        "llXorBase64StringsCorrect", "llModPow", "llSetForce", "llGetForce", "llSetVelocity",
                        "llApplyRotationalImpulse", "llGroundRepel"})
    QVERIFY2(Builtins::supportedFunctions().contains(QString::fromLatin1(f)), f);
}

void LslSteps456Test::warp2PosStaysUnknown()
{
  QVERIFY(Builtins::function("llWarp2Pos") == nullptr);
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", "default { state_entry() { llWarp2Pos(<1,1,1>); } }"}});
  QVERIFY(!a["warnings"].toArray().isEmpty() || !a["valid"].toBool());
}

void LslSteps456Test::regionPosMoves()
{
  const QJsonObject a = run(R"lsl(
    integer r = llSetRegionPos(<100, 120, 30>);
    llOwnerSay((string)r + " " + (string)llGetPos());
  )lsl");
  QCOMPARE(said(a), QStringList({"1 <100.00000, 120.00000, 30.00000>"}));
  QCOMPARE(entries(a, "prim"), QStringList({R"({"link":0,"op":"position","position":[100,120,30],"type":"prim"})"}));
}

void LslSteps456Test::regionPosFarAndOutside()
{
  // no 10 m limit: 150 m in one call
  const QJsonObject a = run(R"lsl(
    llSetRegionPos(<10, 10, 20>);
    integer far = llSetRegionPos(<160, 10, 20>);
    llOwnerSay((string)far + " " + (string)llGetPos());
    integer edge = llSetRegionPos(<266, -10, 4096>);
    llOwnerSay((string)edge + " " + (string)llGetPos());
    integer a1 = llSetRegionPos(<266.5, 10, 20>);
    llOwnerSay((string)a1 + " " + (string)llGetPos());
    integer a2 = llSetRegionPos(<10, -10.5, 20>);
    integer a3 = llSetRegionPos(<10, 10, -1>);
    integer a4 = llSetRegionPos(<10, 10, 4096.5>);
    llOwnerSay((string)a2 + (string)a3 + (string)a4 + " " + (string)llGetPos());
  )lsl");
  QCOMPARE(said(a), QStringList({"1 <160.00000, 10.00000, 20.00000>", "1 <266.00000, -10.00000, 4096.00000>",
                                 "0 <266.00000, -10.00000, 4096.00000>", "000 <266.00000, -10.00000, 4096.00000>"}));
}

void LslSteps456Test::regionPosPhysical()
{
  const QJsonObject a = run(R"lsl(
    llSetRegionPos(<50, 50, 50>);
    llSetStatus(STATUS_PHYSICS, TRUE);
    integer r = llSetRegionPos(<60, 60, 60>);
    llOwnerSay((string)r + " " + (string)llGetPos());
  )lsl");
  QCOMPARE(said(a), QStringList({"0 <50.00000, 50.00000, 50.00000>"}));
}

void LslSteps456Test::statisticsWikiExample()
{
  // the wiki's list: [-1, 3.33, 17, "object", -4.5, 0.8675]; expected values from Python's statistics module
  const QJsonObject a = run(R"lsl(
    list l = [-1, 3.33, 17, "object", -4.5, 0.8675];
    integer i;
    for (i = 0; i <= 9; ++i) llOwnerSay((string)i + " " + (string)llListStatistics(i, l));
  )lsl");
  QCOMPARE(said(a), QStringList({"0 21.500000", "1 -4.500000", "2 17.000000", "3 3.139500", "4 0.867500", "5 8.258468",
                                 "6 15.697500", "7 322.091461", "8 5.000000", "9 2.943574"}));
}

void LslSteps456Test::statisticsEdgeCases()
{
  const QJsonObject a = run(R"lsl(
    list even = [4, 1, 3, 2];
    llOwnerSay((string)llListStatistics(LIST_STAT_MEDIAN, even) + " " + (string)llListStatistics(LIST_STAT_STD_DEV, even));
    llOwnerSay((string)llListStatistics(LIST_STAT_GEOMETRIC_MEAN, even) + " " + (string)llListStatistics(LIST_STAT_SUM_SQUARES, even));
    llOwnerSay((string)llListStatistics(LIST_STAT_GEOMETRIC_MEAN, [2.0, 8.0]));
    llOwnerSay((string)llListStatistics(LIST_STAT_GEOMETRIC_MEAN, [-2, -8]));
    llOwnerSay((string)llListStatistics(LIST_STAT_GEOMETRIC_MEAN, [0, 5]));
    integer i;
    for (i = 0; i <= 9; ++i) llOwnerSay((string)llListStatistics(i, []) + " " + (string)llListStatistics(i, ["a", <1,2,3>, ZERO_ROTATION]));
    llOwnerSay((string)llListStatistics(LIST_STAT_STD_DEV, [7]) + " " + (string)llListStatistics(LIST_STAT_RANGE, [7]) + " " + (string)llListStatistics(LIST_STAT_NUM_COUNT, [7, 1.5, "x"]));
    llOwnerSay((string)llListStatistics(99, [1, 2]));
  )lsl");
  QStringList expected({"2.500000 1.290994", "2.213364 30.000000", "4.000000", "4.000000", "0.000000"});
  for (int i = 0; i < 10; ++i) expected << "0.000000 0.000000";
  expected << "0.000000 0.000000 2.000000" << "0.000000";
  QCOMPARE(said(a), expected);
}

void LslSteps456Test::rot2Angle()
{
  // 2 * acos(0.5) = 2.0943951 (120 degrees); a rotation of 270 degrees gives the unsigned minimum angle pi / 2
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llRot2Angle(ZERO_ROTATION));
    llOwnerSay((string)llRot2Angle(<0, 0, 0, 1>) + " " + (string)llRot2Angle(<0, 0, 0, 0>));
    llOwnerSay((string)llRot2Angle(<0.8660254, 0, 0, 0.5>));
    llOwnerSay((string)llRot2Angle(<0, 0.7071068, 0, 0.7071068>));
    llOwnerSay((string)llRot2Angle(<1, 0, 0, 0>));
    llOwnerSay((string)llRot2Angle(llAxisAngle2Rot(<0, 0, 1>, 1.0)));
    llOwnerSay((string)llRot2Angle(llAxisAngle2Rot(<0, 0, 1>, 4.712389)));
    llOwnerSay((string)llRot2Angle(<0, 0, 2, 0>));
  )lsl");
  QCOMPARE(said(a), QStringList({"0.000000", "0.000000 0.000000", "2.094395", "1.570796", "3.141593", "1.000000", "1.570796", "3.141593"}));
}

void LslSteps456Test::xorBase64()
{
  // "Hello, World" = SGVsbG8sIFdvcmxk; key "key" = a2V5; result worked out with Python (base64 + bytes xor)
  const QJsonObject a = run(R"lsl(
    string text = llStringToBase64("Hello, World");
    string pass = llStringToBase64("key");
    string x = llXorBase64(text, pass);
    llOwnerSay(x);
    llOwnerSay(llXorBase64Strings(text, pass));
    llOwnerSay(llXorBase64StringsCorrect(text, pass));
    llOwnerSay(llBase64ToString(llXorBase64(x, pass)));
  )lsl");
  QCOMPARE(said(a), QStringList({"IwAVBwpVSzIWGQkd", "IwAVBwpVSzIWGQkd", "IwAVBwpVSzIWGQkd", "Hello, World"}));
}

void LslSteps456Test::xorBase64Edge()
{
  // key "ab\0cd" (YWIAY2Q=): llXorBase64StringsCorrect ends it at the NUL (key "ab"), llXorBase64 uses all five bytes
  const QJsonObject a = run(R"lsl(
    string text = llStringToBase64("Hello, World");
    llOwnerSay(llXorBase64(text, "YWIAY2Q="));
    llOwnerSay(llXorBase64StringsCorrect(text, "YWIAY2Q="));
    llOwnerSay(llXorBase64(text, "") + "|" + llXorBase64StringsCorrect(text, "") + "|" + llXorBase64Strings(text, ""));
    llOwnerSay(llXorBase64("", "a2V5") + "|");
  )lsl");
  QCOMPARE(said(a), QStringList({"KQdsDwtNQlcMFg0G", "KQcNDg5OQTUOEA0G", "SGVsbG8sIFdvcmxk|SGVsbG8sIFdvcmxk|SGVsbG8sIFdvcmxk", "|"}));
}

void LslSteps456Test::modPow()
{
  // 13743^14200 mod 10000 = 1, the wiki's example; the second wiki example is read as unsigned 32-bit (2813638566)
  // and answered as the signed integer -1481328730; 0xFFFFFFFF as -1 is the unsigned 4294967295
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llModPow(13743, 14200, 10000));
    llOwnerSay((string)llModPow(0xc734abfa, 0x8371b314, 0xffffffff));
    llOwnerSay((string)llModPow(2, 10, 1000) + " " + (string)llModPow(3, 0, 7) + " " + (string)llModPow(5, 3, 1) + " " + (string)llModPow(5, 3, 0));
    llOwnerSay((string)llModPow(2, 31, -1));
    llOwnerSay((string)llModPow(-1, 3, 100));
  )lsl");
  QCOMPARE(said(a), QStringList({"1", "-1481328730", "24 1 0 0", "-2147483648", "75"}));
}

void LslSteps456Test::forceRecordAndRead()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetForce());
    llSetForce(<1, 2, 3>, FALSE);
    llOwnerSay((string)llGetForce());
    llSetForce(ZERO_VECTOR, FALSE);
    llOwnerSay((string)llGetForce());
  )lsl");
  QCOMPARE(said(a), QStringList({"<0.00000, 0.00000, 0.00000>", "<1.00000, 2.00000, 3.00000>", "<0.00000, 0.00000, 0.00000>"}));
  QCOMPARE(entries(a, "velocity"), QStringList({R"({"force":[1,2,3],"op":"force","type":"velocity"})", R"({"force":[0,0,0],"op":"force","type":"velocity"})"}));
}

void LslSteps456Test::forceLocal()
{
  // local: turned by the root rotation (90 degrees about z: <1,0,0> becomes <0,1,0>)
  const QJsonObject a = run(R"lsl(
    llSetRot(llAxisAngle2Rot(<0, 0, 1>, PI_BY_TWO));
    llSetForce(<1, 0, 0>, TRUE);
    vector f = llGetForce();   // the turn leaves float noise (about -4e-8), so print rounded hundredths
    llOwnerSay((string)llRound(f.x * 100) + " " + (string)llRound(f.y * 100) + " " + (string)llRound(f.z * 100));
  )lsl");
  QCOMPARE(said(a), QStringList({"0 100 0"}));
}

void LslSteps456Test::velocitySet()
{
  const QJsonObject a = run(R"lsl(
    llSetVelocity(<1, 2, 3>, FALSE);
    llOwnerSay((string)llGetVel());
    llSetRot(llAxisAngle2Rot(<0, 0, 1>, PI_BY_TWO));
    llSetVelocity(<2, 0, 0>, TRUE);
    vector v = llGetVel();
    llOwnerSay((string)llRound(v.x * 100) + " " + (string)llRound(v.y * 100) + " " + (string)llRound(v.z * 100));
  )lsl");
  QCOMPARE(said(a), QStringList({"<1.00000, 2.00000, 3.00000>", "0 200 0"}));
  QCOMPARE(entries(a, "velocity").first(), QString(R"({"op":"set","type":"velocity","velocity":[1,2,3]})"));
}

void LslSteps456Test::rotationalImpulseAndGroundRepel()
{
  const QJsonObject a = run(R"lsl(
    llApplyRotationalImpulse(<1, 0, 0>, TRUE);
    llGroundRepel(1.5, TRUE, 0.5);
  )lsl");
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QString note = "Recorded only: the sandbox plays and shows nothing for this.";
  QCOMPARE(entries(a, "recorded_only"),
           QStringList({QString(R"({"args":[[1,0,0],1],"function":"llApplyRotationalImpulse","note":"%1","type":"recorded_only"})").arg(note),
                        QString(R"({"args":[1.5,1,0.5],"function":"llGroundRepel","note":"%1","type":"recorded_only"})").arg(note)}));
}

void LslSteps456Test::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llOwnerSay((string)llSetRegionPos(<1, 1, 1>) + (string)llListStatistics(LIST_STAT_MEAN, [1, 2]) + (string)llRot2Angle(ZERO_ROTATION));
      llOwnerSay(llXorBase64("", "") + (string)llModPow(2, 3, 5));
      llSetForce(<1, 0, 0>, TRUE);
      llOwnerSay((string)llGetForce());
      llSetVelocity(<1, 0, 0>, FALSE);
      llApplyRotationalImpulse(<1, 0, 0>, FALSE);
      llGroundRepel(1.0, TRUE, 0.1);
    } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

void LslSteps456Test::oldXorFunctionsAreDeprecated()
{
  // the lint warns about the two old names (the viewer's keyword list marks them deprecated); both still run
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() { llOwnerSay(llXorBase64Strings("", "") + llXorBase64StringsCorrect("", "")); } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QStringList messages;
  for (const QJsonValue& w : a["warnings"].toArray()) messages << w.toObject()["message"].toString();
  QCOMPARE(messages, QStringList({"llXorBase64Strings is deprecated in Second Life.", "llXorBase64StringsCorrect is deprecated in Second Life."}));
}

QTEST_GUILESS_MAIN(LslSteps456Test)

#include "tst_lslsteps456.moc"
