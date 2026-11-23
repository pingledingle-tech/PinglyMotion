/*
 * PinglyMotion - tests for prim state without effects, L$ logic and record-only stubs
 * (Script Lab step 9b-3, issue 317)
 *
 * llSetText, llSetAlpha, llSetColor, llSetTexture, llSetPrimitiveParams and the link forms,
 * llGetLinkPrimitiveParams, position, rotation and status, llSetSitText, llTextBox,
 * llTransferLindenDollars with PERMISSION_DEBIT, and the stubs that only record. Every
 * expected value is exact (the log entries as the sandbox writes them, and what scripts read
 * back); rotations are those of 90 degrees about z, whose values are known.
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

class LslPrimsTest : public QObject
{
  Q_OBJECT

  private slots:
    void textAndReadBack();
    void faceColourAlphaTexture();
    void primitiveParamsRules();
    void badRulesAreDebugLines();
    void unsupportedRuleStops();
    void linkForms();
    void positionAndRotationOfARootPrim();
    void childPrimPositionAndRotation();
    void statusFlags();
    void sitText();
    void textBox();
    void lindenDollars();
    void recordOnlyStubs();
    void checkerHasNoWarnings();
    void constantsMatchSecondLife();

  private:
    static QJsonObject run(const QString& body, double seconds = 1, const QJsonObject& extra = QJsonObject());
    static QStringList said(const QJsonObject& answer);
    // the log entries of one type, each as compact JSON without "t"
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QStringList stops(const QJsonObject& answer);
};

QJsonObject LslPrimsTest::run(const QString& body, double seconds, const QJsonObject& extra)
{
  const QString source = "say(string s) { llOwnerSay(s); }\ndefault { state_entry() {\n" + body + "\n} }";
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}};
  for (auto it = extra.begin(); it != extra.end(); ++it) request[it.key()] = it.value();
  return Control::run(request);
}

QStringList LslPrimsTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslPrimsTest::entries(const QJsonObject& answer, const QString& type)
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

QStringList LslPrimsTest::stops(const QJsonObject& answer)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QString type = v.toObject()["type"].toString();
    if (type == "unsupported" || type == "error" || type == "limit") out << v.toObject()["message"].toString();
  }
  return out;
}

void LslPrimsTest::textAndReadBack()
{
  const QJsonObject a = run(R"lsl(
    llSetText("Hello", <1, 0.5, 0.25>, 0.75);
    say(llList2CSV(llGetPrimitiveParams([PRIM_TEXT])));
    llSetText("Clamped", <2, -1, 0.5>, 3.0);
    say(llList2CSV(llGetPrimitiveParams([PRIM_TEXT])));
    llSetText("", ZERO_VECTOR, 0.0);
  )lsl");
  QCOMPARE(said(a), QStringList({"Hello, <1.00000, 0.50000, 0.25000>, 0.750000", "Clamped, <1.00000, 0.00000, 0.50000>, 1.000000"}));
  QCOMPARE(entries(a, "prim"), QStringList({
    R"({"alpha":0.75,"color":[1,0.5,0.25],"link":0,"op":"text","text":"Hello","type":"prim"})",
    R"({"alpha":1,"color":[1,0,0.5],"link":0,"op":"text","text":"Clamped","type":"prim"})",
    R"({"alpha":0,"color":[0,0,0],"link":0,"op":"text","text":"","type":"prim"})"}));
  // the text is cut at 254 characters
  const QJsonObject b = run("llSetText(\"" + QString(300, 'x') + "\", <1,1,1>, 1.0); say((string)llStringLength(llList2String(llGetPrimitiveParams([PRIM_TEXT]), 0)));");
  QCOMPARE(said(b), QStringList({"254"}));
}

void LslPrimsTest::faceColourAlphaTexture()
{
  const QJsonObject a = run(R"lsl(
    say((string)llGetAlpha(0) + " " + (string)llGetColor(0) + " " + llGetTexture(0));
    llSetAlpha(0.5, ALL_SIDES);
    say((string)llGetAlpha(0) + " " + (string)llGetAlpha(ALL_SIDES));
    llSetColor(<1, 0, 0>, 2);
    say((string)llGetColor(2) + " " + (string)llGetColor(ALL_SIDES));
    llSetTexture("my-texture", 3);
    say(llGetTexture(3) + " " + llGetTexture(0));
    llSetColor(<0, 0, 0>, 9);
    say((string)llGetColor(9) + " " + (string)llGetColor(5));
    llSetAlpha(7.5, 1);
    say((string)llGetAlpha(1));
  )lsl");
  QCOMPARE(said(a), QStringList({
    "1.000000 <1.00000, 1.00000, 1.00000> 89556747-24cb-43ed-920b-47caed15465f",
    "0.500000 0.500000",
    "<1.00000, 0.00000, 0.00000> <1.00000, 0.83333, 0.83333>",
    "my-texture 89556747-24cb-43ed-920b-47caed15465f",
    "<0.00000, 0.00000, 0.00000> <1.00000, 1.00000, 1.00000>",
    "1.000000"}));
  QCOMPARE(entries(a, "prim"), QStringList({
    R"({"alpha":0.5,"face":-1,"link":0,"op":"alpha","type":"prim"})",
    R"({"color":[1,0,0],"face":2,"link":0,"op":"color","type":"prim"})",
    R"({"face":3,"link":0,"op":"texture","texture":"my-texture","type":"prim"})",
    R"({"color":[0,0,0],"face":9,"link":0,"op":"color","type":"prim"})",
    R"({"alpha":1,"face":1,"link":0,"op":"alpha","type":"prim"})"}));
}

void LslPrimsTest::primitiveParamsRules()
{
  const QJsonObject a = run(R"lsl(
    llSetPrimitiveParams([PRIM_COLOR, 1, <0, 1, 0>, 0.25, PRIM_FULLBRIGHT, ALL_SIDES, TRUE, PRIM_GLOW, 0, 0.5,
                          PRIM_SIZE, <1, 2, 3>, PRIM_NAME, "Nm", PRIM_DESC, "Ds", PRIM_PHYSICS, TRUE,
                          PRIM_TEXTURE, 4, "tex", <2, 3, 0>, <0.5, 0.25, 0>, 1.5]);
    say(llList2CSV(llGetPrimitiveParams([PRIM_COLOR, 1, PRIM_FULLBRIGHT, 2, PRIM_GLOW, 0, PRIM_SIZE, PRIM_NAME, PRIM_DESC, PRIM_PHYSICS])));
    say(llList2CSV(llGetPrimitiveParams([PRIM_TEXTURE, 4])));
    say(llGetObjectName() + "|" + llGetObjectDesc() + "|" + (string)llGetScale());
  )lsl");
  QCOMPARE(said(a), QStringList({
    "<0.00000, 1.00000, 0.00000>, 0.250000, 1, 0.500000, <1.00000, 2.00000, 3.00000>, Nm, Ds, 1",
    "tex, <2.00000, 3.00000, 0.00000>, <0.50000, 0.25000, 0.00000>, 1.500000",
    "Nm|Ds|<1.00000, 2.00000, 3.00000>"}));
  const QStringList log = entries(a, "prim");
  QCOMPARE(log.size(), 8);
  QCOMPARE(log[0], QString(R"({"alpha":0.25,"color":[0,1,0],"face":1,"link":0,"op":"color","type":"prim"})"));
  QCOMPARE(log[1], QString(R"({"face":-1,"link":0,"op":"fullbright","type":"prim","value":true})"));
  QCOMPARE(log[2], QString(R"({"face":0,"link":0,"op":"glow","type":"prim","value":0.5})"));
  QCOMPARE(log[3], QString(R"({"link":0,"op":"size","size":[1,2,3],"type":"prim"})"));
  QCOMPARE(log[4], QString(R"({"link":0,"op":"name","text":"Nm","type":"prim"})"));
  QCOMPARE(log[5], QString(R"({"link":0,"op":"description","text":"Ds","type":"prim"})"));
  QCOMPARE(log[6], QString(R"({"link":0,"op":"physics","type":"prim","value":true})"));
  QCOMPARE(log[7], QString(R"({"face":4,"link":0,"offsets":[0.5,0.25],"op":"texture","repeats":[2,3],"rotation":1.5,"texture":"tex","type":"prim"})"));
}

void LslPrimsTest::badRulesAreDebugLines()
{
  // an integer where the alpha float belongs: the earlier rule is kept, this one and the rest are skipped, a debug line says so
  const QJsonObject a = run(R"lsl(
    llSetPrimitiveParams([PRIM_GLOW, 0, 0.75, PRIM_COLOR, 0, <1, 0, 0>, 1, PRIM_GLOW, 1, 0.5]);
    say(llList2CSV(llGetPrimitiveParams([PRIM_GLOW, 0, PRIM_GLOW, 1, PRIM_COLOR, 0])));
    llSetPrimitiveParams([5.5]);
    llSetPrimitiveParams([PRIM_POSITION]);
  )lsl");
  QCOMPARE(said(a), QStringList({"0.750000, 0.000000, <1.00000, 1.00000, 1.00000>, 1.000000"}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"llSetPrimitiveParams: bad PRIM_COLOR rule at list position 3: needs face, color vector and alpha float","type":"debug"})",
    R"({"channel":2147483647,"text":"llSetPrimitiveParams: bad primitive rule at list position 0: a rule starts with a PRIM_* integer","type":"debug"})",
    R"({"channel":2147483647,"text":"llSetPrimitiveParams: bad PRIM_POSITION rule at list position 0: needs a vector","type":"debug"})"}));
}

void LslPrimsTest::unsupportedRuleStops()
{
  QJsonObject a = run("llSetPrimitiveParams([PRIM_GLOW, 0, 0.5, PRIM_TYPE, 1]); say(\"not reached\");");
  QCOMPARE(stops(a), QStringList({"PRIM_TYPE in llSetPrimitiveParams is not supported yet in the Script Lab sandbox, so the script was stopped here."}));
  QCOMPARE(said(a), QStringList());
  a = run("llGetLinkPrimitiveParams(LINK_ROOT, [PRIM_BUMP_SHINY, 0]); say(\"not reached\");");
  QCOMPARE(stops(a), QStringList({"PRIM_BUMP_SHINY in llGetLinkPrimitiveParams is not supported yet in the Script Lab sandbox, so the script was stopped here."}));
  a = run("llSetPrimitiveParams([123, 1]);");
  QCOMPARE(stops(a), QStringList({"The primitive rule 123 in llSetPrimitiveParams is not supported yet in the Script Lab sandbox, so the script was stopped here."}));
  // a function that is only named
  a = run("vector v = llGetAccel();");
  QVERIFY(stops(a).size() == 1 && stops(a)[0].contains("llGetAccel is not supported yet"));
}

void LslPrimsTest::linkForms()
{
  const QString source = R"lsl(
    default { state_entry() {
      llSetLinkPrimitiveParams(2, [PRIM_COLOR, ALL_SIDES, <1, 0, 0>, 1.0]);
      llSetLinkColor(LINK_ALL_CHILDREN, <0, 0, 1>, 1);
      llSetLinkAlpha(3, 0.5, 1);
      llSetLinkPrimitiveParamsFast(LINK_THIS, [PRIM_TEXT, "root", <1, 1, 1>, 1.0]);
      llSetLinkPrimitiveParams(LINK_ROOT, [PRIM_LINK_TARGET, 3, PRIM_TEXT, "three", <0, 1, 0>, 0.5, PRIM_LINK_TARGET, 1, PRIM_NAME, "newroot"]);
      llSetLinkTexture(LINK_ALL_OTHERS, "other-tex", 0);
      llOwnerSay(llList2CSV(llGetLinkPrimitiveParams(2, [PRIM_COLOR, 0, PRIM_COLOR, 1])));
      llOwnerSay(llList2CSV(llGetLinkPrimitiveParams(3, [PRIM_COLOR, 1, PRIM_TEXT])));
      llOwnerSay(llList2CSV(llGetLinkPrimitiveParams(LINK_ROOT, [PRIM_TEXT, PRIM_NAME, PRIM_COLOR, 0])));
      llOwnerSay(llList2CSV(llGetLinkPrimitiveParams(LINK_SET, [PRIM_TEXT])));
      llOwnerSay(llList2CSV(llGetLinkPrimitiveParams(9, [PRIM_TEXT])));
      llOwnerSay(llGetTexture(0) + " " + llList2String(llGetLinkPrimitiveParams(3, [PRIM_TEXTURE, 0]), 0));
    } }
  )lsl";
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{
    QJsonObject{{"name", "box"}, {"prims", 3}, {"source", source}}}}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({
    "<1.00000, 0.00000, 0.00000>, 1.000000, <0.00000, 0.00000, 1.00000>, 1.000000",
    "<0.00000, 0.00000, 1.00000>, 0.500000, three, <0.00000, 1.00000, 0.00000>, 0.500000",
    "root, <1.00000, 1.00000, 1.00000>, 1.000000, newroot, <1.00000, 1.00000, 1.00000>, 1.000000",
    "",
    "",
    "89556747-24cb-43ed-920b-47caed15465f other-tex"}));
  QStringList links;
  for (const QString& e : entries(a, "prim")) links << QJsonDocument::fromJson(e.toUtf8()).object()["op"].toString() + QString::number(QJsonDocument::fromJson(e.toUtf8()).object()["link"].toInt());
  QCOMPARE(links, QStringList({"color2", "color2", "color3", "alpha3", "text1", "text3", "name1", "texture2", "texture3"}));
}

void LslPrimsTest::positionAndRotationOfARootPrim()
{
  const QJsonObject a = run(R"lsl(
    say((string)llGetPos());
    llSetPos(<130, 128, 20>);
    say((string)llGetPos());
    llSetPos(<158, 128, 20>);
    say((string)llGetPos());
    llSetPos(<300, 128, 20>);
    say((string)llGetPos());
    say((string)llGetRot() + " " + (string)llGetLocalRot() + " " + (string)llGetRootRotation());
    llSetRot(llEuler2Rot(<0, 0, PI_BY_TWO>));
    say((string)llGetRot() + " " + (string)llGetLocalRot());
    llSetPrimitiveParams([PRIM_POSITION, <131, 128, 20>]);
    say((string)llGetRootPosition() + " " + (string)llGetLocalPos());
  )lsl");
  QCOMPARE(said(a), QStringList({
    "<128.00000, 128.00000, 20.00000>",
    "<130.00000, 128.00000, 20.00000>",
    "<140.00000, 128.00000, 20.00000>",
    "<140.00000, 128.00000, 20.00000>",
    "<0.00000, 0.00000, 0.00000, 1.00000> <0.00000, 0.00000, 0.00000, 1.00000> <0.00000, 0.00000, 0.00000, 1.00000>",
    "<0.00000, 0.00000, 0.70711, 0.70711> <0.00000, 0.00000, 0.70711, 0.70711>",
    "<131.00000, 128.00000, 20.00000> <131.00000, 128.00000, 20.00000>"}));
  QCOMPARE(entries(a, "prim"), QStringList({
    R"({"link":0,"op":"position","position":[130,128,20],"type":"prim"})",
    R"({"link":0,"note":"A rezzed object moves at most 10 m per call; it moved 10 m towards the target.","op":"position","position":[140,128,20],"type":"prim"})",
    R"({"link":0,"note":"The target is outside the region: the object did not move.","op":"position","position":[300,128,20],"type":"prim"})",
    R"({"link":0,"op":"rotation","rotation":[0,0,0.70711,0.70711],"type":"prim"})",
    R"({"link":0,"op":"position","position":[131,128,20],"type":"prim"})"}));
  // the object's place in the answer follows
  QCOMPARE(a["objects"].toArray()[0].toObject()["position"].toArray()[0].toDouble(), 131.0);
}

void LslPrimsTest::childPrimPositionAndRotation()
{
  const QString mainScript = "default { state_entry() { llSetRot(llEuler2Rot(<0, 0, PI_BY_TWO>)); } }";
  const QString kid = R"lsl(
    default { state_entry() { llSetTimerEvent(1.0); }
      timer() { llSetTimerEvent(0);
        llSetPos(<1, 0, 0>);
        llOwnerSay((string)llGetLocalPos() + " " + (string)llGetPos() + " " + (string)llGetRootPosition());
        llSetRot(ZERO_ROTATION);
        llOwnerSay((string)llGetLocalRot() + " " + (string)llGetRootRotation());
        llOwnerSay((string)llVecDist(llGetPos(), <128, 129, 20>));
        llSetRot(llEuler2Rot(<0, 0, PI_BY_TWO>));
        llOwnerSay((string)llGetLocalRot());
      } }
  )lsl";
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 3}, {"objects", QJsonArray{
    QJsonObject{{"name", "box"}, {"prims", 2}, {"scripts", QJsonArray{
      QJsonObject{{"name", "main"}, {"source", mainScript}, {"link", 1}}, QJsonObject{{"name", "kid"}, {"source", kid}, {"link", 2}}}}}}}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QStringList got = said(a);
  QCOMPARE(got.size(), 4);
  QCOMPARE(got[0], QString("<1.00000, 0.00000, 0.00000> <128.00000, 129.00000, 20.00000> <128.00000, 128.00000, 20.00000>"));
  QCOMPARE(got[1], QString("<0.00000, 0.00000, -0.70711, 0.70711> <0.00000, 0.00000, 0.70711, 0.70711>"));
  QVERIFY2(got[2].startsWith("0.00000"), qPrintable(got[2]));
  // the z component may print as -0.00000 on some platforms
  QVERIFY2(got[3] == "<0.00000, 0.00000, 0.00000, 1.00000>" || got[3] == "<0.00000, 0.00000, -0.00000, 1.00000>", qPrintable(got[3]));
}

void LslPrimsTest::statusFlags()
{
  const QJsonObject a = run(R"lsl(
    say((string)llGetStatus(STATUS_ROTATE_X) + (string)llGetStatus(STATUS_ROTATE_Y) + (string)llGetStatus(STATUS_ROTATE_Z)
        + (string)llGetStatus(STATUS_PHYSICS) + (string)llGetStatus(STATUS_PHANTOM));
    llSetStatus(STATUS_PHANTOM | STATUS_BLOCK_GRAB, TRUE);
    say((string)llGetStatus(STATUS_PHANTOM) + (string)llGetStatus(STATUS_BLOCK_GRAB) + (string)llGetStatus(STATUS_PHANTOM | STATUS_BLOCK_GRAB)
        + (string)llGetStatus(STATUS_PHANTOM | STATUS_PHYSICS) + (string)llGetStatus(0));
    llSetStatus(STATUS_ROTATE_X | STATUS_ROTATE_Y, FALSE);
    llSetStatus(STATUS_PHANTOM, FALSE);
    say((string)llGetStatus(STATUS_ROTATE_X) + (string)llGetStatus(STATUS_ROTATE_Z) + (string)llGetStatus(STATUS_PHANTOM) + (string)llGetStatus(STATUS_BLOCK_GRAB));
  )lsl");
  QCOMPARE(said(a), QStringList({"11100", "11100", "0101"}));
  QCOMPARE(entries(a, "prim"), QStringList({
    R"({"link":0,"op":"status","status":94,"type":"prim"})",
    R"({"link":0,"op":"status","status":88,"type":"prim"})",
    R"({"link":0,"op":"status","status":72,"type":"prim"})"}));
}

void LslPrimsTest::sitText()
{
  const QJsonObject a = run("llSetSitText(\"Take a seat\");");
  QCOMPARE(entries(a, "prim"), QStringList({R"({"link":0,"op":"sit_text","text":"Take a seat","type":"prim"})"}));
}

void LslPrimsTest::textBox()
{
  // llTextBox and llDialog with the button !!llTextBox!! are the same: a dialog entry without buttons, marked text_box
  const QString source = R"lsl(
    default { state_entry() { llListen(11, "", NULL_KEY, ""); llTextBox(llGetOwner(), "Name?", 11); llDialog(llGetOwner(), "Again?", ["!!llTextBox!!"], 12); }
      listen(integer c, string n, key id, string m) { llOwnerSay("heard " + (string)c + " " + m); } }
  )lsl";
  RlvState rlv;
  Session session;
  QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"source", source}}, &rlv, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QStringList dialogs = entries(a, "dialog");
  QCOMPARE(dialogs.size(), 2);
  QCOMPARE(dialogs[0], QString(R"({"buttons":[],"channel":11,"message":"Name?","text_box":true,"to":"a2e76fcd-9360-4f6d-a924-000000000001","type":"dialog"})"));
  QCOMPARE(dialogs[1], QString(R"({"buttons":[],"channel":12,"message":"Again?","text_box":true,"to":"a2e76fcd-9360-4f6d-a924-000000000001","type":"dialog"})"));
  QVERIFY(a["dialog"].toObject()["text_box"].toBool());
  QCOMPARE(a["dialog"].toObject()["buttons"].toArray().size(), 0);
  // the avatar types an answer: the dialog input says it on the dialog's newest channel (12), where the script does not listen
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "dialog"}, {"button", "free text"}, {"seconds", 1}}, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(entries(a, "input").size(), 1);
  QCOMPARE(QJsonDocument::fromJson(entries(a, "input")[0].toUtf8()).object()["text"].toString(), QString("free text"));
  QCOMPARE(QJsonDocument::fromJson(entries(a, "input")[0].toUtf8()).object()["channel"].toInt(), 12);
}

void LslPrimsTest::lindenDollars()
{
  const QString avatar = "11111111-2222-4333-8444-555555555555";
  const QString source = R"lsl(
    key last;
    default {
      state_entry() {
        last = llTransferLindenDollars(")lsl" + avatar + R"lsl(", 25);
        llOwnerSay("no permission: " + (string)(last != NULL_KEY));
        llRequestPermissions(llGetOwner(), PERMISSION_DEBIT);
      }
      run_time_permissions(integer p) {
        if (p & PERMISSION_DEBIT) {
          last = llTransferLindenDollars(")lsl" + avatar + R"lsl(", 40);
          llTransferLindenDollars(")lsl" + avatar + R"lsl(", 0);
          llTransferLindenDollars(")lsl" + avatar + R"lsl(", -5);
          llTransferLindenDollars("not-a-key", 5);
          llTransferLindenDollars(llGetOwner(), 7);
        }
      }
      transaction_result(key id, integer success, string data) {
        llOwnerSay("result " + (string)success + " " + data + " " + (string)(id == last));
      }
    }
  )lsl";
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"source", source}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({
    "no permission: 1",
    "result 0 MISSING_PERMISSION_DEBIT 1",
    "result 1 " + avatar + ",40 1",
    "result 0 INVALID_AMOUNT 0",
    "result 0 INVALID_AMOUNT 0",
    "result 0 INVALID_DESTINATION 0",
    "result 1 a2e76fcd-9360-4f6d-a924-000000000001,7 0"}));
  QStringList results;
  for (const QString& e : entries(a, "linden_dollars"))
  {
    const QJsonObject o = QJsonDocument::fromJson(e.toUtf8()).object();
    results << QString("%1 %2 %3").arg(o["to"].toString().left(8)).arg(o["amount"].toInt()).arg(o["result"].toString());
  }
  QCOMPARE(results, QStringList({"11111111 25 MISSING_PERMISSION_DEBIT", "11111111 40 ok", "11111111 0 INVALID_AMOUNT", "11111111 -5 INVALID_AMOUNT",
                                 "not-a-ke 5 INVALID_DESTINATION", "a2e76fcd 7 ok"}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"Script trying to transfer L$ but PERMISSION_DEBIT permission not set","type":"debug"})"}));
  // paying an object is refused
  const QJsonObject b = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{
    QJsonObject{{"name", "payer"}, {"source", "default { state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_DEBIT); }"
        " run_time_permissions(integer p) { llTransferLindenDollars(\"c0ffee00-0000-4000-8000-000000000003\", 5); }"
        " transaction_result(key id, integer ok, string d) { llOwnerSay(d); } }"}},
    QJsonObject{{"name", "shop"}, {"source", "default { state_entry() { } }"}}}}});
  QCOMPARE(said(b), QStringList({"LINDENDOLLAR_ENTITYDOESNOTEXIST"}));
}

void LslPrimsTest::recordOnlyStubs()
{
  const QJsonObject a = run(R"lsl(
    llParticleSystem([PSYS_PART_FLAGS, 1, PSYS_SRC_PATTERN, 2]);
    llLinkParticleSystem(LINK_SET, []);
    llTriggerSound("ding", 1.0);
    llPlaySound("bell", 0.5);
    llLoopSound("hum", 0.25);
    llStopSound();
    llPreloadSound("later");
    llTargetOmega(<0, 0, 1>, 3.0, 1.0);
    llLoadURL(llGetOwner(), "Visit", "https://example.invalid/page");
    say("done");
  )lsl");
  QCOMPARE(said(a), QStringList({"done"}));
  const QString note = R"("note":"Recorded only: the sandbox plays and shows nothing for this.")";
  QCOMPARE(entries(a, "recorded_only"), QStringList({
    R"({"args":[4],"function":"llParticleSystem",)" + note + R"(,"type":"recorded_only"})",
    R"({"args":[-1,0],"function":"llLinkParticleSystem",)" + note + R"(,"type":"recorded_only"})",
    R"({"args":["ding",1],"function":"llTriggerSound",)" + note + R"(,"type":"recorded_only"})",
    R"({"args":["bell",0.5],"function":"llPlaySound",)" + note + R"(,"type":"recorded_only"})",
    R"({"args":["hum",0.25],"function":"llLoopSound",)" + note + R"(,"type":"recorded_only"})",
    R"({"args":[],"function":"llStopSound",)" + note + R"(,"type":"recorded_only"})",
    R"({"args":["later"],"function":"llPreloadSound",)" + note + R"(,"type":"recorded_only"})",
    R"({"args":[[0,0,1],3,1],"function":"llTargetOmega",)" + note + R"(,"type":"recorded_only"})",
    R"({"args":["a2e76fcd-9360-4f6d-a924-000000000001","Visit","https://example.invalid/page"],"function":"llLoadURL",)" + note + R"(,"type":"recorded_only"})"}));
}

void LslPrimsTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llSetText("a", <1,1,1>, 1.0); llSetAlpha(1.0, ALL_SIDES); llSetColor(<1,1,1>, ALL_SIDES); llSetTexture("t", 0);
      llSetPrimitiveParams([]); llSetLinkPrimitiveParams(1, []); llSetLinkPrimitiveParamsFast(1, []); llGetLinkPrimitiveParams(1, []);
      llSetPos(<1,1,1>); llSetRot(ZERO_ROTATION); llGetRot(); llGetLocalRot(); llSetStatus(1, 1); llGetStatus(1); llSetSitText("x");
      llTextBox(NULL_KEY, "a", 1); llTransferLindenDollars(NULL_KEY, 1); llParticleSystem([]); llLinkParticleSystem(1, []);
      llTriggerSound("a", 1.0); llPlaySound("a", 1.0); llLoopSound("a", 1.0); llStopSound(); llPreloadSound("a"); llTargetOmega(ZERO_VECTOR, 0.0, 0.0);
      llLoadURL(NULL_KEY, "a", "b");
    } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

void LslPrimsTest::constantsMatchSecondLife()
{
  // the numbers the sandbox uses for LINK_*, ALL_SIDES and PRIM_* rules are the table's (Second Life's)
  const QList<QPair<const char*, int>> expected = {
    {"LINK_SET", -1}, {"LINK_ALL_OTHERS", -2}, {"LINK_ALL_CHILDREN", -3}, {"LINK_THIS", -4}, {"ALL_SIDES", -1},
    {"PRIM_PHYSICS", 3}, {"PRIM_TEMP_ON_REZ", 4}, {"PRIM_PHANTOM", 5}, {"PRIM_POSITION", 6}, {"PRIM_SIZE", 7}, {"PRIM_ROTATION", 8},
    {"PRIM_TEXTURE", 17}, {"PRIM_COLOR", 18}, {"PRIM_FULLBRIGHT", 20}, {"PRIM_GLOW", 25}, {"PRIM_TEXT", 26}, {"PRIM_NAME", 27},
    {"PRIM_DESC", 28}, {"PRIM_ROT_LOCAL", 29}, {"PRIM_POS_LOCAL", 33}, {"PRIM_LINK_TARGET", 34}, {"PERMISSION_DEBIT", 2},
    {"PRIM_TYPE", 9}, {"PRIM_BUMP_SHINY", 19}, {"PRIM_FLEXIBLE", 21}, {"PRIM_TEXGEN", 22}, {"PRIM_POINT_LIGHT", 23},
    {"PRIM_OMEGA", 32}, {"PRIM_MATERIAL", 2}};
  for (const auto& e : expected)
  {
    const BuiltinConstant* c = Builtins::constant(e.first);
    QVERIFY2(c != nullptr, e.first);
    QCOMPARE(c->value.i, e.second);
  }
}

QTEST_GUILESS_MAIN(LslPrimsTest)
#include "tst_lslprims.moc"
