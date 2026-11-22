/*
 * PinglyMotion - tests for sitting and pose balls (Script Lab step 9b-5)
 *
 * llSitTarget, llLinkSitTarget, llAvatarOnSitTarget, llAvatarOnLinkSitTarget, llUnSit,
 * llGetAnimationList, llGetAnimation, llGetAgentInfo, the sit and stand inputs, the changed
 * event with CHANGED_LINK, and the permissions of a sitting avatar. Every expected value is
 * exact; the animation key is the MD5 of "<object key>/animation/<name>", worked out outside
 * the program.
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
  const char* const GUEST = "11111111-2222-3333-4444-555555555555";
  const char* const GUEST2 = "66666666-7777-8888-9999-000000000000";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";

  const char* const POSE_BALL = R"lsl(
    default {
      state_entry() { llSitTarget(<0, 0, 0.5>, ZERO_ROTATION); llOwnerSay("on target [" + (string)llAvatarOnSitTarget() + "]"); }
      changed(integer c) {
        if (c & CHANGED_LINK) {
          key av = llAvatarOnSitTarget();
          if (av != NULL_KEY) { llOwnerSay("sat " + (string)av + " prims " + (string)llGetNumberOfPrims()); llRequestPermissions(av, PERMISSION_TRIGGER_ANIMATION); }
          else llOwnerSay("stood prims " + (string)llGetNumberOfPrims());
        }
      }
      run_time_permissions(integer p) {
        if (p & PERMISSION_TRIGGER_ANIMATION) { llStartAnimation("pose"); llOwnerSay("anim " + llList2CSV(llGetAnimationList(llGetPermissionsKey()))); }
      }
    }
  )lsl";
}

class LslSittingTest : public QObject
{
  Q_OBJECT

  private slots:
    void poseBall();
    void sitWithoutTargetAndSecondSitter();
    void unSit();
    void linkSitTarget();
    void agentInfoAndAnimation();
    void permissionsOfASitter();
    void sitAndStandThroughLslEvent();
    void inputIsChecked();
    void limits();
    void checkerHasNoWarnings();
    void constantsMatchSecondLife();

  private:
    static QJsonObject runWith(const QString& source, const QJsonArray& inputs, double seconds = 5);
    static QJsonObject sit(double at, const QString& id, const QString& name = "Guest") { return QJsonObject{{"at", at}, {"type", "sit"}, {"id", id}, {"name", name}}; }
    static QJsonObject stand(double at, const QString& id) { return QJsonObject{{"at", at}, {"type", "stand"}, {"id", id}}; }
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
};

QJsonObject LslSittingTest::runWith(const QString& source, const QJsonArray& inputs, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"inputs", inputs}, {"seconds", seconds}});
}

QStringList LslSittingTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslSittingTest::entries(const QJsonObject& answer, const QString& type)
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

void LslSittingTest::poseBall()
{
  const QJsonObject a = runWith(POSE_BALL, QJsonArray{sit(1, GUEST), stand(2, GUEST)});
  QCOMPARE(said(a), QStringList({
    "on target [00000000-0000-0000-0000-000000000000]",
    "sat 11111111-2222-3333-4444-555555555555 prims 2",
    "anim 47aae6f5-cd41-356b-cec9-f7014f3cbb12",
    "stood prims 1"}));
  QCOMPARE(entries(a, "input"), QStringList({
    R"({"id":"11111111-2222-3333-4444-555555555555","input":"sit","name":"Guest","on_target":true,"type":"input"})",
    R"({"id":"11111111-2222-3333-4444-555555555555","input":"stand","name":"Guest","type":"input"})"}));
  QCOMPARE(entries(a, "prim"), QStringList({R"({"link":0,"offset":[0,0,0.5],"op":"sit_target","rotation":[0,0,0,1],"set":true,"type":"prim"})"}));
  QCOMPARE(entries(a, "permissions"), QStringList({
    R"({"granted":true,"mask":16,"note":"The sandbox grants an avatar sitting on the object PERMISSION_TRIGGER_ANIMATION and PERMISSION_TAKE_CONTROLS at once, and no other permission.","type":"permissions"})"}));
  QCOMPARE(entries(a, "animation_start"), QStringList({R"({"name":"pose","type":"animation_start"})"}));
  QVERIFY(!a["stopped"].toBool());
  // the answer lists nobody sitting at the end
  QVERIFY(!a["objects"].toArray()[0].toObject().contains("sitters"));
}

void LslSittingTest::sitWithoutTargetAndSecondSitter()
{
  const QString script = R"lsl(
    default {
      changed(integer c) { llOwnerSay("changed " + (string)c + " target [" + (string)llAvatarOnSitTarget() + "] prims " + (string)llGetNumberOfPrims()); }
      state_entry() { llSitTarget(<0, 0, 1>, ZERO_ROTATION); }
    }
  )lsl";
  // the first avatar takes the sit target; the second sits on the prim without it and does not change the target
  const QJsonObject a = runWith(script, QJsonArray{sit(1, GUEST), sit(2, GUEST2, "Second"), stand(3, GUEST), stand(4, GUEST2)});
  QCOMPARE(said(a), QStringList({
    "changed 32 target [11111111-2222-3333-4444-555555555555] prims 2",
    "changed 32 target [11111111-2222-3333-4444-555555555555] prims 3",
    "changed 32 target [00000000-0000-0000-0000-000000000000] prims 2",
    "changed 32 target [00000000-0000-0000-0000-000000000000] prims 1"}));
  QCOMPARE(entries(a, "input"), QStringList({
    R"({"id":"11111111-2222-3333-4444-555555555555","input":"sit","name":"Guest","on_target":true,"type":"input"})",
    R"({"id":"66666666-7777-8888-9999-000000000000","input":"sit","name":"Second","on_target":false,"type":"input"})",
    R"({"id":"11111111-2222-3333-4444-555555555555","input":"stand","name":"Guest","type":"input"})",
    R"({"id":"66666666-7777-8888-9999-000000000000","input":"stand","name":"Second","type":"input"})"}));
  // without any sit target the avatar sits on the prim, changed still comes, and llAvatarOnSitTarget stays null
  const QJsonObject b = runWith(R"lsl(default { changed(integer c) { llOwnerSay("target [" + (string)llAvatarOnSitTarget() + "]"); } })lsl", QJsonArray{sit(1, GUEST)});
  QCOMPARE(said(b), QStringList({"target [00000000-0000-0000-0000-000000000000]"}));
  QCOMPARE(entries(b, "input"), QStringList({
    R"({"id":"11111111-2222-3333-4444-555555555555","input":"sit","name":"Guest","on_target":false,"type":"input"})"}));
  // the answer shows who is sitting at the end
  const QJsonObject c = runWith(R"lsl(default { state_entry() { llSitTarget(<0, 0, 1>, ZERO_ROTATION); } })lsl", QJsonArray{sit(1, GUEST)});
  const QJsonArray sitters = c["objects"].toArray()[0].toObject()["sitters"].toArray();
  QCOMPARE(sitters.size(), 1);
  QCOMPARE(QString::fromUtf8(QJsonDocument(sitters[0].toObject()).toJson(QJsonDocument::Compact)),
           QString(R"({"key":"11111111-2222-3333-4444-555555555555","link":1,"name":"Guest","on_target":true})"));
}

void LslSittingTest::unSit()
{
  const QJsonObject a = runWith(R"lsl(
    default {
      state_entry() { llSitTarget(<0, 0, 1>, ZERO_ROTATION); }
      touch_start(integer n) { llUnSit(NULL_KEY); llUnSit("11111111-2222-3333-4444-555555555555"); llUnSit(llAvatarOnSitTarget()); }
      changed(integer c) { llOwnerSay("changed [" + (string)llAvatarOnSitTarget() + "]"); }
    }
  )lsl", QJsonArray{sit(1, GUEST), QJsonObject{{"at", 2}, {"type", "touch"}}, QJsonObject{{"at", 3}, {"type", "touch"}}});
  QCOMPARE(said(a), QStringList({"changed [11111111-2222-3333-4444-555555555555]", "changed [00000000-0000-0000-0000-000000000000]"}));
  QCOMPARE(entries(a, "unsit"), QStringList({R"({"avatar":"11111111-2222-3333-4444-555555555555","by":"llUnSit","type":"unsit"})"}));
}

void LslSittingTest::linkSitTarget()
{
  const QJsonObject request{{"command", "lsl_run"}, {"seconds", 5}, {"objects", QJsonArray{QJsonObject{{"name", "Chair"}, {"prims", 2}, {"source", R"lsl(
      default {
        state_entry() {
          llLinkSitTarget(2, <0, 0, 1>, ZERO_ROTATION);
          llLinkSitTarget(1, <1, 2, 3>, <0, 0, 0.70711, 0.70711>);
          llLinkSitTarget(1, ZERO_VECTOR, ZERO_ROTATION);
        }
        changed(integer c) { llOwnerSay("2: " + (string)llAvatarOnLinkSitTarget(2) + " 1: " + (string)llAvatarOnLinkSitTarget(1) + " 5: " + (string)llAvatarOnLinkSitTarget(5)); }
      }
    )lsl"}}}}, {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "sit"}, {"id", GUEST}, {"name", "Guest"}, {"link", 2}}}}};
  const QJsonObject a = Control::run(request);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"2: 11111111-2222-3333-4444-555555555555 1: 00000000-0000-0000-0000-000000000000 5: 00000000-0000-0000-0000-000000000000"}));
  QCOMPARE(entries(a, "prim"), QStringList({
    R"({"link":2,"object":"Chair","offset":[0,0,1],"op":"sit_target","rotation":[0,0,0,1],"set":true,"type":"prim"})",
    R"({"link":1,"object":"Chair","offset":[1,2,3],"op":"sit_target","rotation":[0,0,0.70711,0.70711],"set":true,"type":"prim"})",
    R"({"link":1,"object":"Chair","offset":[0,0,0],"op":"sit_target","rotation":[0,0,0,1],"set":false,"type":"prim"})"}));
  QCOMPARE(entries(a, "input"), QStringList({
    R"({"id":"11111111-2222-3333-4444-555555555555","input":"sit","link":2,"name":"Guest","object":"Chair","on_target":true,"type":"input"})"}));
}

void LslSittingTest::agentInfoAndAnimation()
{
  const QString script = QString(R"lsl(
    string info(key k) { return "info " + (string)llGetAgentInfo(k) + " anim [" + llGetAnimation(k) + "]"; }
    default {
      state_entry() { llSitTarget(<0, 0, 1>, ZERO_ROTATION); }
      touch_start(integer n) { llOwnerSay("owner " + info(llGetOwner())); llOwnerSay("guest " + info("%1")); llOwnerSay("other " + info("%2")); }
    }
  )lsl").arg(GUEST, GUEST2);
  const QJsonObject touch{{"type", "touch"}};
  QJsonObject t1 = touch; t1["at"] = 1;
  QJsonObject t2 = touch; t2["at"] = 3;
  const QJsonObject a = runWith(script, QJsonArray{t1, sit(2, GUEST), t2});
  QCOMPARE(said(a), QStringList({
    "owner info 0 anim [Standing]", "guest info 0 anim []", "other info 0 anim []",
    "owner info 0 anim [Standing]", "guest info 48 anim [Sitting]", "other info 0 anim []"}));
  // an owner who wears an object of the world has AGENT_ATTACHMENTS (2)
  const QJsonObject request{{"command", "lsl_run"}, {"seconds", 2}, {"objects", QJsonArray{
    QJsonObject{{"name", "hud"}, {"attached", 35}, {"source", R"lsl(default { state_entry() { llOwnerSay((string)llGetAgentInfo(llGetOwner())); } })lsl"}}}}};
  QCOMPARE(said(Control::run(request)), QStringList({"2"}));
}

void LslSittingTest::permissionsOfASitter()
{
  // owner: everything at once; a sitting avatar: the animation and controls permissions only; a stranger: nothing
  const QString script = QString(R"lsl(
    default {
      state_entry() { llSitTarget(<0, 0, 1>, ZERO_ROTATION); }
      touch_start(integer n) {
        llRequestPermissions("%1", PERMISSION_DEBIT | PERMISSION_TRIGGER_ANIMATION | PERMISSION_TAKE_CONTROLS);
        llRequestPermissions("%2", PERMISSION_TRIGGER_ANIMATION);
      }
      run_time_permissions(integer p) { llOwnerSay("granted " + (string)p + " key " + (string)llGetPermissionsKey()); }
    }
  )lsl").arg(GUEST, GUEST2);
  const QJsonObject a = runWith(script, QJsonArray{sit(1, GUEST), QJsonObject{{"at", 2}, {"type", "touch"}}});
  QCOMPARE(said(a), QStringList({"granted 20 key 11111111-2222-3333-4444-555555555555"}));
  QCOMPARE(entries(a, "permissions").size(), 2);
  QVERIFY(entries(a, "permissions")[1].contains(R"("granted":false)"));
  // only a permission the sitter does not get automatically: nothing is granted, no event
  const QJsonObject b = runWith(QString(R"lsl(
    default {
      state_entry() { llSitTarget(<0, 0, 1>, ZERO_ROTATION); }
      touch_start(integer n) { llRequestPermissions("%1", PERMISSION_DEBIT); }
      run_time_permissions(integer p) { llOwnerSay("unexpected"); }
    }
  )lsl").arg(GUEST), QJsonArray{sit(1, GUEST), QJsonObject{{"at", 2}, {"type", "touch"}}});
  QCOMPARE(said(b), QStringList());
  QVERIFY(entries(b, "permissions")[0].contains(R"("granted":false)"));
  // the owner is granted everything at once, as before
  const QJsonObject c = runWith(R"lsl(
    default { state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_DEBIT | PERMISSION_TRIGGER_ANIMATION); }
              run_time_permissions(integer p) { llOwnerSay((string)p); } }
  )lsl", QJsonArray());
  QCOMPARE(said(c), QStringList({"18"}));   // PERMISSION_DEBIT (2) + PERMISSION_TRIGGER_ANIMATION (16)
}

void LslSittingTest::sitAndStandThroughLslEvent()
{
  RlvState rlv;
  Session session;
  QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", POSE_BALL}, {"seconds", 1}}, &rlv, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "sit"}, {"id", GUEST}, {"name", "Guest"}}, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"sat 11111111-2222-3333-4444-555555555555 prims 2", "anim 47aae6f5-cd41-356b-cec9-f7014f3cbb12"}));
  QCOMPARE(a["objects"].toArray()[0].toObject()["sitters"].toArray().size(), 1);
  // stand without naming an object finds the object the avatar sits on
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "stand"}, {"id", GUEST}}, &session);
  QCOMPARE(said(a), QStringList({"stood prims 1"}));
}

void LslSittingTest::inputIsChecked()
{
  const QString script = "default { state_entry() { llSitTarget(<0, 0, 1>, ZERO_ROTATION); } }";
  auto errors = [&](const QJsonArray& inputs)
  {
    QStringList out;
    const QJsonObject a = runWith(script, inputs);
    if (!a["ok"].toBool()) return QStringList{a["error"].toString()};
    for (const QJsonValue& v : a["log"].toArray())
      if (v.toObject()["type"].toString() == "input" && v.toObject().contains("error")) out << v.toObject()["error"].toString();
    return out;
  };
  QCOMPARE(errors(QJsonArray{sit(1, GUEST), sit(2, GUEST)}), QStringList({"The avatar is already sitting."}));
  QCOMPARE(errors(QJsonArray{stand(1, GUEST)}), QStringList({"The avatar is not sitting on the object."}));
  QCOMPARE(errors(QJsonArray{QJsonObject{{"type", "sit"}, {"link", 3}}}), QStringList({"Input 1: the object \"Object\" has 1 prim(s); there is no link 3."}));
  QCOMPARE(errors(QJsonArray{QJsonObject{{"type", "chat"}, {"link", 1}}}),
           QStringList({"Input 1: \"link\" is only for touch (the prim touched), sit (the prim sat on) and money (the prim paid)."}));
  // the owner can sit too (no id: the owner), and cannot sit twice
  QCOMPARE(errors(QJsonArray{QJsonObject{{"type", "sit"}}, QJsonObject{{"type", "sit"}}}), QStringList({"The avatar is already sitting."}));
}

void LslSittingTest::limits()
{
  // at most 16 avatars on one object; the 17th is refused
  QJsonArray inputs;
  for (int i = 0; i < 17; ++i) inputs.append(sit(1 + i, QString("11111111-2222-3333-4444-%1").arg(i, 12, 10, QChar('0')), QString("Guest %1").arg(i)));
  const QJsonObject a = runWith("default { state_entry() {} }", inputs, 30);
  QVERIFY(a["ok"].toBool());
  QStringList refused;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject().contains("error")) refused << v.toObject()["error"].toString();
  QCOMPARE(refused, QStringList({"At most 16 avatars sit on one object."}));
  QCOMPARE(a["objects"].toArray()[0].toObject()["sitters"].toArray().size(), 16);
  // many changed events at once keep the 64-event queue limit
  QJsonArray many;
  for (int i = 0; i < 16; ++i) many.append(sit(1, QString("11111111-2222-3333-4444-%1").arg(i, 12, 10, QChar('0'))));
  for (int i = 0; i < 16; ++i) many.append(stand(1, QString("11111111-2222-3333-4444-%1").arg(i, 12, 10, QChar('0'))));
  const QJsonObject b = runWith("integer n; default { changed(integer c) { ++n; } }", many, 5);
  QVERIFY(b["ok"].toBool());
  QVERIFY(!b["stopped"].toBool());
}

void LslSittingTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llSitTarget(<0,0,1>, ZERO_ROTATION); llLinkSitTarget(1, <0,0,1>, ZERO_ROTATION); llAvatarOnSitTarget(); llAvatarOnLinkSitTarget(1);
      llUnSit(NULL_KEY); llGetAnimationList(NULL_KEY); llGetAnimation(NULL_KEY); llGetAgentInfo(NULL_KEY);
    } changed(integer c) {} }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  for (const char* f : {"llSitTarget", "llLinkSitTarget", "llAvatarOnSitTarget", "llAvatarOnLinkSitTarget", "llUnSit", "llGetAnimationList", "llGetAnimation", "llGetAgentInfo"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

void LslSittingTest::constantsMatchSecondLife()
{
  const QList<QPair<QString, int>> expected = {
    {"CHANGED_LINK", 0x20}, {"AGENT_SITTING", 0x10}, {"AGENT_ON_OBJECT", 0x20}, {"AGENT_ATTACHMENTS", 0x2},
    {"PERMISSION_TRIGGER_ANIMATION", 0x10}, {"PERMISSION_TAKE_CONTROLS", 0x4}};
  for (const auto& e : expected)
  {
    const BuiltinConstant* c = Builtins::constant(e.first);
    QVERIFY2(c != nullptr, qPrintable(e.first));
    QCOMPARE(c->value.i, e.second);
  }
  Q_UNUSED(NULL_KEY);
}

QTEST_GUILESS_MAIN(LslSittingTest)
#include "tst_lslsitting.moc"
