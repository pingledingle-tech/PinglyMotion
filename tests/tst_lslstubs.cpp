/*
 * PinglyMotion - tests for more record-only functions and the animation override
 * (Script Lab step 9b-8)
 *
 * Sound, texture animation, camera, pay price, click action, look-at, old particle and filter
 * functions only write a recorded_only entry with their arguments; llSetAnimationOverride,
 * llResetAnimationOverride and llGetAnimationOverride keep a small state. Every expected value is
 * exact.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslStubsTest : public QObject
{
  Q_OBJECT

  private slots:
    void recordedOnly();
    void recordedOnlyKeepsItsLimits();
    void animationOverride();
    void animationOverrideNeedsPermission();
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

QJsonObject LslStubsTest::run(const QString& body)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() {\n" + body + "\n} }"}, {"seconds", 1}});
}

QStringList LslStubsTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslStubsTest::entries(const QJsonObject& answer, const QString& type)
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

void LslStubsTest::recordedOnly()
{
  const QJsonObject a = run(R"lsl(
    llTriggerSoundLimited("snd", 0.5, <1, 2, 3>, <4, 5, 6>);
    llLoopSoundMaster("m", 1.0); llLoopSoundSlave("s", 0.25); llPlaySoundSlave("p", 2.0); llSoundPreload("x");
    llAdjustSoundVolume(0.75); llSetSoundQueueing(TRUE); llSetSoundRadius(10.0); llCollisionSound("c", 0.5);
    llSetTextureAnim(ANIM_ON | LOOP, ALL_SIDES, 4, 4, 0.0, 16.0, 8.0);
    llSetLinkTextureAnim(LINK_THIS, 0, 1, 2, 2, 0.0, 4.0, 1.0);
    llOffsetTexture(0.5, -0.5, 2); llScaleTexture(2.0, 3.0, ALL_SIDES); llRotateTexture(1.5, 0);
    llSetCameraEyeOffset(<0, 0, 2>); llSetCameraAtOffset(<1, 0, 0>); llSetCameraParams([CAMERA_ACTIVE, 1, CAMERA_DISTANCE, 5.0]);
    llClearCameraParams(); llSetLinkCamera(LINK_THIS, <0, 0, 1>, <1, 0, 0>);
    llSetPayPrice(PAY_HIDE, [1, 5]); llSetClickAction(CLICK_ACTION_SIT); llSetTouchText("Go");
    llPointAt(<1, 1, 1>); llStopPointAt(); llLookAt(<0, 0, 1>, 2.0, 0.5); llStopLookAt();
    llMakeExplosion(10, 1.0, 2.0, 3.0, 4.0, "tex", <0, 0, 1>); llMakeSmoke(1, 2.0, 3.0, 4.0, 5.0, "t", <1, 2, 3>);
    llMakeFire(2, 1.0, 1.0, 1.0, 1.0, "f", ZERO_VECTOR); llMakeFountain(3, 1.0, 1.0, 1.0, 1.0, 1, "f", ZERO_VECTOR, 0.5);
    llCollisionFilter("n", NULL_KEY, TRUE); llPassTouches(1); llPassCollisions(0);
    llSetRemoteScriptAccessPin(1234); llSetParcelMusicURL("example.org/s"); llRefreshPrimURL(); llSetPrimURL("u");
  )lsl");
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QStringList expected({
    rec("llTriggerSoundLimited", "[\"snd\",0.5,[1,2,3],[4,5,6]]"),
    rec("llLoopSoundMaster", "[\"m\",1]"), rec("llLoopSoundSlave", "[\"s\",0.25]"), rec("llPlaySoundSlave", "[\"p\",2]"), rec("llSoundPreload", "[\"x\"]"),
    rec("llAdjustSoundVolume", "[0.75]"), rec("llSetSoundQueueing", "[1]"), rec("llSetSoundRadius", "[10]"), rec("llCollisionSound", "[\"c\",0.5]"),
    rec("llSetTextureAnim", "[3,-1,4,4,0,16,8]"),
    rec("llSetLinkTextureAnim", "[-4,0,1,2,2,0,4,1]"),
    rec("llOffsetTexture", "[0.5,-0.5,2]"), rec("llScaleTexture", "[2,3,-1]"), rec("llRotateTexture", "[1.5,0]"),
    rec("llSetCameraEyeOffset", "[[0,0,2]]"), rec("llSetCameraAtOffset", "[[1,0,0]]"), rec("llSetCameraParams", "[4]"),
    rec("llClearCameraParams", "[]"), rec("llSetLinkCamera", "[-4,[0,0,1],[1,0,0]]"),
    rec("llSetPayPrice", "[-1,2]"), rec("llSetClickAction", "[1]"), rec("llSetTouchText", "[\"Go\"]"),
    rec("llPointAt", "[[1,1,1]]"), rec("llStopPointAt", "[]"), rec("llLookAt", "[[0,0,1],2,0.5]"), rec("llStopLookAt", "[]"),
    rec("llMakeExplosion", "[10,1,2,3,4,\"tex\",[0,0,1]]"), rec("llMakeSmoke", "[1,2,3,4,5,\"t\",[1,2,3]]"),
    rec("llMakeFire", "[2,1,1,1,1,\"f\",[0,0,0]]"), rec("llMakeFountain", "[3,1,1,1,1,1,\"f\",[0,0,0],0.5]"),
    rec("llCollisionFilter", "[\"n\",\"00000000-0000-0000-0000-000000000000\",1]"), rec("llPassTouches", "[1]"), rec("llPassCollisions", "[0]"),
    rec("llSetRemoteScriptAccessPin", "[1234]"), rec("llSetParcelMusicURL", "[\"example.org/s\"]"), rec("llRefreshPrimURL", "[]"), rec("llSetPrimURL", "[\"u\"]")});
  QCOMPARE(entries(a, "recorded_only"), expected);
}

void LslStubsTest::recordedOnlyKeepsItsLimits()
{
  // a long text is cut like chat; a loop of calls fills the log only up to its cap and the run stays alive
  const QJsonObject a = run("llSetTouchText(\"" + QString("x").repeated(2000) + "\"); integer i; for (i = 0; i < 3000; ++i) llPassTouches(i);");
  QVERIFY(a["ok"].toBool());
  const QString quote(QChar(34));
  const QString expectedFirst = rec("llSetTouchText", "[" + quote + QString("x").repeated(1024) + quote + "]");
  QCOMPARE(entries(a, "recorded_only")[0], expectedFirst);
  const bool truncated = a["log_truncated"].toBool();
  const int logSize = a["log"].toArray().size();
  const bool stopped = a["stopped"].toBool();
  QVERIFY(truncated);
  const bool fits = logSize <= 2001;
  QVERIFY(fits);
  QVERIFY(!stopped);
}

void LslStubsTest::animationOverride()
{
  const QString source =
    "    default {\n"
    "      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_OVERRIDE_ANIMATIONS); }\n"
    "      run_time_permissions(integer p) {\n"
    "        llOwnerSay(\"[\" + llGetAnimationOverride(\"Standing\") + \"]\");\n"
    "        llSetAnimationOverride(\"Standing\", \"my stand\");\n"
    "        llSetAnimationOverride(\"Walking\", \"my walk\");\n"
    "        llOwnerSay(\"[\" + llGetAnimationOverride(\"Standing\") + \"] [\" + llGetAnimationOverride(\"Walking\") + \"] [\" + llGetAnimationOverride(\"Running\") + \"]\");\n"
    "        llSetAnimationOverride(\"Standing\", \"other stand\");\n"
    "        llResetAnimationOverride(\"Walking\");\n"
    "        llResetAnimationOverride(\"Running\");\n"
    "        llSetAnimationOverride(\"Hopping\", \"x\");\n"
    "        llOwnerSay(\"[\" + llGetAnimationOverride(\"Standing\") + \"] [\" + llGetAnimationOverride(\"Walking\") + \"]\");\n"
    "      }\n"
    "    }\n"
    "  \n";
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"source", source}});
  const QStringList expectedSaid({"[]", "[my stand] [my walk] []", "[other stand] []"});
  QCOMPARE(said(a), expectedSaid);
  QCOMPARE(entries(a, "animation_override"), QStringList({
    R"({"animation":"my stand","op":"set","state":"Standing","type":"animation_override"})",
    R"({"animation":"my walk","op":"set","state":"Walking","type":"animation_override"})",
    R"({"animation":"other stand","op":"set","state":"Standing","type":"animation_override"})",
    R"({"op":"reset","state":"Walking","type":"animation_override"})",
    R"({"op":"reset","state":"Running","type":"animation_override"})"}));
  const QString hopping = QString("{\"channel\":2147483647,\"text\":\"llSetAnimationOverride: unknown animation state \\\"Hopping\\\"\",\"type\":\"debug\"}");
  const QStringList expectedDebug({hopping});
  QCOMPARE(entries(a, "debug"), expectedDebug);
}

void LslStubsTest::animationOverrideNeedsPermission()
{
  const QJsonObject a = run(R"lsl(
    llSetAnimationOverride("Standing", "x"); llResetAnimationOverride("Standing"); llOwnerSay("[" + llGetAnimationOverride("Standing") + "]");
  )lsl");
  QCOMPARE(said(a), QStringList({"[]"}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"Script trying to override animations but PERMISSION_OVERRIDE_ANIMATIONS permission not set","type":"debug"})",
    R"({"channel":2147483647,"text":"Script trying to override animations but PERMISSION_OVERRIDE_ANIMATIONS permission not set","type":"debug"})"}));
  QCOMPARE(entries(a, "animation_override"), QStringList());
}

void LslStubsTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() { llSetClickAction(0); llSetCameraParams([]); llAdjustSoundVolume(1.0); llSetAnimationOverride("Standing", "a"); llGetAnimationOverride("Standing"); } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  for (const char* f : {"llTriggerSoundLimited", "llLoopSoundMaster", "llLoopSoundSlave", "llPlaySoundSlave", "llSoundPreload", "llAdjustSoundVolume",
                        "llSetSoundQueueing", "llSetSoundRadius", "llCollisionSound", "llSetTextureAnim", "llSetLinkTextureAnim", "llOffsetTexture",
                        "llScaleTexture", "llRotateTexture", "llSetCameraEyeOffset", "llSetCameraAtOffset", "llSetCameraParams", "llClearCameraParams",
                        "llSetLinkCamera", "llSetPayPrice", "llSetClickAction", "llSetTouchText", "llPointAt", "llStopPointAt", "llLookAt", "llStopLookAt",
                        "llMakeExplosion", "llMakeFountain", "llMakeSmoke", "llMakeFire", "llCollisionFilter", "llPassTouches", "llPassCollisions",
                        "llSetRemoteScriptAccessPin", "llSetParcelMusicURL", "llRefreshPrimURL", "llSetPrimURL", "llSetAnimationOverride",
                        "llResetAnimationOverride", "llGetAnimationOverride"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

QTEST_GUILESS_MAIN(LslStubsTest)
#include "tst_lslstubs.moc"
