/*
 * PinglyMotion - tests for several LSL objects in one world (Script Lab step 5, issue 291)
 *
 * Two objects talking on a channel, chat ranges, timer order on the simulated
 * clock, injected events (lsl_event: touch, dialog, attach, detach), per-object
 * RLV restrictions and limits. Every expected value is exact.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"
#include "scriptlab/lsl/sandbox.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

class LslWorldTest : public QObject
{
  Q_OBJECT

  private slots:
    void twoObjectsTalkOnAChannel();
    void whisperOutOfRangeIsNotHeard();
    void timerOrderIsDeterministic();
    void touchEventInjected();
    void dialogAnswerReachesListen();
    void attachAndDetach();
    void sameRestrictionTwoObjectsClearOnlyOwn();
    void limitStopsOneObjectOthersRun();
    void regionSayToAndPosition();
    void avatarChatRange();
    void refusesBadInput();
    void eventNeedsAWorld();
    void singleScriptCanTakeEvents();
    void examplesRun();

  private:
    static QJsonObject object(const QString& name, const QString& source, const QJsonObject& more = QJsonObject());
    static QJsonObject run(const QJsonArray& objects, double seconds, RlvState& rlv, Session* session = nullptr,
                           const QJsonArray& inputs = QJsonArray());
    // "t object type detail" lines for the entries of the given types, so whole sequences compare at once
    static QStringList lines(const QJsonObject& answer, const QStringList& types);
    static QString example(const QString& name);
};

QJsonObject LslWorldTest::object(const QString& name, const QString& source, const QJsonObject& more)
{
  QJsonObject o{{"name", name}, {"source", source}};
  for (auto it = more.begin(); it != more.end(); ++it) o[it.key()] = it.value();
  return o;
}

QJsonObject LslWorldTest::run(const QJsonArray& objects, double seconds, RlvState& rlv, Session* session, const QJsonArray& inputs)
{
  QJsonObject request{{"command", "lsl_run"}, {"objects", objects}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request, &rlv, session);
}

QStringList LslWorldTest::lines(const QJsonObject& answer, const QStringList& types)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    const QString type = e["type"].toString();
    if (!types.contains(type)) continue;
    QString detail;
    if (type == "event")
    {
      detail = e["name"].toString();
      QStringList args;
      for (const QJsonValue& a : e["args"].toArray()) args << a.toString();
      if (!args.isEmpty()) detail += "(" + args.join(",") + ")";
    }
    else if (e.contains("text")) detail = e["text"].toString();
    out << QString("%1 %2 %3 %4").arg(e["t"].toDouble()).arg(e["object"].toString(), type, detail);
  }
  return out;
}

QString LslWorldTest::example(const QString& name)
{
  QFile file(QString(PINGLYMOTION_EXAMPLES) + "/scriptlab/" + name);
  if (!file.open(QIODevice::ReadOnly)) return QString();
  return QString::fromUtf8(file.readAll());
}

void LslWorldTest::twoObjectsTalkOnAChannel()
{
  RlvState rlv;
  const QString hud = "default { state_entry() { llListen(5, \"\", NULL_KEY, \"\"); llSay(5, \"lock\"); }"
                      " listen(integer c, string n, key id, string m) { llOwnerSay(\"hud heard \" + m + \" from \" + n); } }";
  const QString cuff = "default { state_entry() { llListen(5, \"hud\", NULL_KEY, \"\"); }"
                       " listen(integer c, string n, key id, string m) { if (m == \"lock\") { llOwnerSay(\"@detach=n\"); llSay(5, \"locked\"); } } }";
  // every state_entry runs in list order, so the cuff listens before the hud speaks
  const QJsonObject a = run({object("cuff", cuff), object("hud", hud)}, 2, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(lines(a, {"say", "event", "owner_say"}),
           QStringList({"0 cuff event state_entry", "0 hud event state_entry", "0 hud say lock",
                        "0 cuff event listen(5,hud,c0ffee00-0000-4000-8000-000000000003,lock)", "0 cuff say locked",
                        "0 hud event listen(5,cuff,c0ffee00-0000-4000-8000-000000000002,locked)", "0 hud owner_say hud heard locked from cuff"}));
  // the cuff's restriction is held under the cuff's key
  QCOMPARE(rlv.objects().keys(), QStringList({"c0ffee00-0000-4000-8000-000000000002"}));
  QCOMPARE(rlv.objects().value("c0ffee00-0000-4000-8000-000000000002"), QStringList({"detach"}));
  const QJsonArray objects = a["objects"].toArray();
  QCOMPARE(objects.size(), 2);
  QCOMPARE(objects[0].toObject()["restrictions"].toArray(), QJsonArray({"detach"}));
  QCOMPARE(objects[1].toObject()["restrictions"].toArray(), QJsonArray());
  QCOMPARE(a["time"].toDouble(), 2.0);
  QCOMPARE(a["stopped"].toBool(), false);
}

void LslWorldTest::whisperOutOfRangeIsNotHeard()
{
  RlvState rlv;
  const QString talker = "default { state_entry() { llWhisper(1, \"w\"); llSay(1, \"s\"); llShout(1, \"sh\"); llRegionSay(1, \"r\"); } }";
  const QString ear = "default { state_entry() { llListen(1, \"\", NULL_KEY, \"\"); } listen(integer c, string n, key id, string m) { llOwnerSay(m); } }";
  const QJsonObject a = run({object("near", ear, {{"position", QJsonArray{128, 143, 20}}}),     // 15 m
                             object("far", ear, {{"position", QJsonArray{128, 178, 20}}}),      // 50 m
                             object("away", ear, {{"position", QJsonArray{10, 10, 20}}}),       // about 167 m
                             object("talker", talker, {{"position", QJsonArray{128, 128, 20}}})},
                            1, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(lines(a, {"owner_say"}), QStringList({"0 near owner_say s", "0 near owner_say sh", "0 near owner_say r",
                                                 "0 far owner_say sh", "0 far owner_say r", "0 away owner_say r"}));
}

void LslWorldTest::timerOrderIsDeterministic()
{
  RlvState rlv;
  const QJsonArray objects{object("a", "default { state_entry() { llSetTimerEvent(1.0); } timer() { } }"),
                           object("b", "default { state_entry() { llSetTimerEvent(0.5); } timer() { } }")};
  const QJsonObject first = run(objects, 2, rlv);
  QVERIFY2(first["ok"].toBool(), qPrintable(first["error"].toString()));
  QStringList timers;
  for (const QString& line : lines(first, {"event"}))
    if (line.endsWith("timer")) timers << line;
  QCOMPARE(timers, QStringList({"0.5 b event timer", "1 a event timer", "1 b event timer", "1.5 b event timer",
                                "2 a event timer", "2 b event timer"}));
  // the same request gives the same log
  const QJsonObject second = run(objects, 2, rlv);
  QCOMPARE(QJsonDocument(second["log"].toArray()).toJson(), QJsonDocument(first["log"].toArray()).toJson());
}

void LslWorldTest::touchEventInjected()
{
  RlvState rlv;
  Session session;
  int changes = 0;
  connect(&session, &Session::changed, [&changes]() { ++changes; });
  const QString hud = "default { touch_start(integer n) { llOwnerSay(\"touched by \" + llDetectedName(0)); llSay(7, \"go\"); } }";
  const QString door = "default { state_entry() { llListen(7, \"\", NULL_KEY, \"go\"); } listen(integer c, string n, key id, string m) { llOwnerSay(\"door opens\"); } }";
  QJsonObject a = run({object("hud", hud), object("door", door)}, 3, rlv, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(changes, 1);
  QVERIFY(session.sandbox());

  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"object", "hud"}, {"seconds", 2}}, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(changes, 2);
  QCOMPARE(lines(a, {"input", "event", "owner_say", "say"}),
           QStringList({"3 hud input ", "3 hud event touch_start(1)", "3 hud owner_say touched by Owner", "3 hud say go",
                        "3 door event listen(7,hud,c0ffee00-0000-4000-8000-000000000002,go)", "3 door owner_say door opens"}));
  const QJsonObject input = a["log"].toArray().first().toObject();
  QCOMPARE(input["input"].toString(), QString("touch"));
  QCOMPARE(input["name"].toString(), QString("Owner"));
  QCOMPARE(a["time"].toDouble(), 5.0);

  // a touch by another avatar
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"object", "hud"}, {"name", "Visitor"},
                                 {"id", "11111111-2222-3333-4444-555555555555"}}, &session);
  QCOMPARE(lines(a, {"owner_say"}), QStringList({"5 hud owner_say touched by Visitor", "5 door owner_say door opens"}));
  QCOMPARE(a["time"].toDouble(), 6.0);
}

void LslWorldTest::dialogAnswerReachesListen()
{
  RlvState rlv;
  Session session;
  const QString hud = "default { state_entry() { llListen(-99, \"\", llGetOwner(), \"\"); }"
                      " touch_start(integer n) { llDialog(llDetectedKey(0), \"Lock the cuffs?\", [\"Yes\", \"No\"], -99); }"
                      " listen(integer c, string n, key id, string m) { llOwnerSay(\"answer \" + m + \" from \" + n); } }";
  QJsonObject a = run({object("hud", hud, {{"attached", 35}})}, 1, rlv, &session,
                      {QJsonObject{{"at", 0.5}, {"type", "touch"}}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QJsonObject dialog = a["dialog"].toObject();
  QCOMPARE(dialog, QJsonObject({{"t", 0.5}, {"object", "hud"}, {"to", "a2e76fcd-9360-4f6d-a924-000000000001"},
                                {"message", "Lock the cuffs?"}, {"buttons", QJsonArray{"Yes", "No"}}, {"channel", -99}}));
  QCOMPARE(lines(a, {"dialog"}), QStringList({"0.5 hud dialog "}));

  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "dialog"}, {"object", "hud"}, {"button", "Yes"}}, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(lines(a, {"input", "owner_say"}), QStringList({"1 hud input Yes", "1 hud owner_say answer Yes from Owner"}));

  // a refused dialog says why on the debug channel, as Second Life does
  RlvState rlv2;
  a = run({object("bad", "default { state_entry() { llDialog(llGetOwner(), \"\", [], 1); llDialog(llGetOwner(), \"m\", [\"a\",\"b\",\"c\",\"d\",\"e\",\"f\",\"g\",\"h\",\"i\",\"j\",\"k\",\"l\",\"m\"], 1); llDialog(llGetOwner(), \"m\", [], 1); } }")}, 1, rlv2);
  QStringList debug;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"].toString() == "debug") debug << v.toObject()["text"].toString();
  QCOMPARE(debug, QStringList({"llDialog: must supply a message", "llDialog: button list too long, must be 12 or fewer entries"}));
  QCOMPARE(a["dialog"].toObject()["buttons"].toArray(), QJsonArray({"OK"}));

  // no dialog yet: the input says so
  a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { } }"}, {"seconds", 1},
                               {"inputs", QJsonArray{QJsonObject{{"type", "dialog"}, {"button", "Yes"}}}}}, &rlv2);
  QCOMPARE(a["log"].toArray().last().toObject()["error"].toString(), QString("The object has not shown a dialog, so there is no button to press."));
}

void LslWorldTest::attachAndDetach()
{
  RlvState rlv;
  Session session;
  const QString cuff = "default { state_entry() { llOwnerSay(\"entry \" + (string)llGetAttached()); }"
                       " attach(key id) { if (id) llOwnerSay(\"attached to \" + llKey2Name(id) + \" at \" + (string)llGetAttached()); else llOwnerSay(\"detached\"); } }";
  QJsonObject a = run({object("cuff", cuff, {{"attached", 30}})}, 1, rlv, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(lines(a, {"owner_say"}), QStringList({"0 cuff owner_say entry 30", "0 cuff owner_say attached to Owner at 30"}));
  QCOMPARE(a["objects"].toArray()[0].toObject()["attached"].toInt(), 30);

  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "detach"}}, &session);
  QCOMPARE(lines(a, {"owner_say"}), QStringList({"1 cuff owner_say detached"}));
  QCOMPARE(a["objects"].toArray()[0].toObject()["attached"].toInt(), 0);
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "detach"}}, &session);
  QCOMPARE(a["log"].toArray()[0].toObject()["error"].toString(), QString("The object is not attached."));
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "attach"}, {"point", 12}}, &session);
  QCOMPARE(lines(a, {"owner_say"}), QStringList({"3 cuff owner_say attached to Owner at 12"}));
}

void LslWorldTest::sameRestrictionTwoObjectsClearOnlyOwn()
{
  RlvState rlv;
  const QJsonObject a = run({object("collar", "default { state_entry() { llOwnerSay(\"@detach=n\"); } }"),
                             object("cuffs", "default { state_entry() { llOwnerSay(\"@detach=n\"); llSetTimerEvent(1.0); }"
                                             " timer() { llSetTimerEvent(0); llOwnerSay(\"@clear\"); } }")},
                            2, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(rlv.count("detach", QString()), 1);
  QCOMPARE(rlv.objects().keys(), QStringList({"c0ffee00-0000-4000-8000-000000000002"}));
  const QJsonObject state = a["rlv"].toObject()["state"].toObject();
  QCOMPARE(state["counts"].toObject()["detach"].toInt(), 1);
  QCOMPARE(state["blocked"].toArray().contains("detach"), true);
  QCOMPARE(a["rlv"].toObject()["lines"].toInt(), 3);
  QStringList rlvLines;
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "rlv_command")
      rlvLines << QString("%1 %2 %3 %4").arg(e["t"].toDouble()).arg(e["object"].toString(), e["text"].toString()).arg(e["restrictions"].toInt());
  }
  QCOMPARE(rlvLines, QStringList({"0 collar @detach=n 1", "0 cuffs @detach=n 2", "1 cuffs @clear 1"}));
}

void LslWorldTest::limitStopsOneObjectOthersRun()
{
  RlvState rlv;
  const QJsonObject a = run({object("loop", "default { state_entry() { while (TRUE) { } } }"),
                             object("clock", "default { state_entry() { llSetTimerEvent(1.0); } timer() { llOwnerSay(\"tick\"); } }")},
                            3, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QJsonArray objects = a["objects"].toArray();
  QCOMPARE(objects[0].toObject()["stopped"].toBool(), true);
  QCOMPARE(objects[0].toObject()["stop_reason"].toString(), QString("The script ran more than 500000 steps and was stopped (an endless loop?)."));
  QCOMPARE(objects[1].toObject()["stopped"].toBool(), false);
  QCOMPARE(lines(a, {"owner_say", "limit"}),
           QStringList({"0 loop limit ", "1 clock owner_say tick", "2 clock owner_say tick", "3 clock owner_say tick"}));
  QCOMPARE(a["stopped"].toBool(), false);

  // an endless ping-pong at one moment ends at the event limit of each script, not in a hang. Run with a
  // smaller event limit and a long real-time limit, so the event limit ends it on a slow machine too
  // (sanitizer builds); lsl_run uses the same Sandbox with the default limits
  Limits limits;
  limits.maxEvents = 2000;
  limits.maxWallMs = 600000;
  const QString ping = "default { state_entry() { llListen(3, \"\", NULL_KEY, \"\"); llSay(3, \"x\"); } listen(integer c, string n, key id, string m) { llSay(3, m); } }";
  std::shared_ptr<const Script> script(Parser::parse(ping).script.release());
  QVERIFY(script);
  RlvState rlv2;
  Sandbox sandbox(rlv2, true);
  ObjectSpec first, second;
  first.name = "ping";
  second.name = "pong";
  QCOMPARE(sandbox.addObject(first, script, limits), QString());
  QCOMPARE(sandbox.addObject(second, script, limits), QString());
  sandbox.start();
  sandbox.runUntil(1.0);
  QCOMPARE(sandbox.truncated(), true);
  // ping reaches the limit first; pong then hears nothing more and keeps waiting
  QCOMPARE(sandbox.interpreter(0).stopReason(), QString("The script handled more than 2000 events and was stopped."));
  QCOMPARE(sandbox.interpreter(1).stopped(), false);
  QCOMPARE(sandbox.allStopped(), false);
  QCOMPARE(sandbox.log().last().toObject()["type"].toString(), QString("limit"));

  // lsl_event refuses a world where every script stopped
  Session session;
  RlvState rlv3;
  run({object("loop", "default { state_entry() { while (TRUE) { } } }")}, 1, rlv3, &session);
  const QJsonObject c = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}}, &session);
  QCOMPARE(c["error"].toString(), QString("Every script in the world of the last lsl_run has stopped; run lsl_run again."));
}

void LslWorldTest::regionSayToAndPosition()
{
  RlvState rlv;
  const QString sender = "default { state_entry() { llRegionSayTo(\"c0ffee00-0000-4000-8000-000000000002\", 9, (string)llGetPos()); } }";
  const QString receiver = "default { state_entry() { llListen(9, \"\", NULL_KEY, \"\"); }"
                           " listen(integer c, string n, key id, string m) { llOwnerSay(n + \" at \" + m + \" owned by \" + (string)llGetOwnerKey(id)); } }";
  const QJsonObject a = run({object("receiver", receiver, {{"position", QJsonArray{250, 250, 3000}}}),
                             object("other", receiver, {{"position", QJsonArray{1, 2, 3}}}),
                             object("sender", sender, {{"position", QJsonArray{1, 2, 3}}})},
                            1, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(lines(a, {"owner_say", "region_say_to"}),
           QStringList({"0 sender region_say_to <1.00000, 2.00000, 3.00000>",
                        "0 receiver owner_say sender at <1.00000, 2.00000, 3.00000> owned by a2e76fcd-9360-4f6d-a924-000000000001"}));
}

void LslWorldTest::avatarChatRange()
{
  RlvState rlv;
  const QString ear = "default { state_entry() { llListen(0, \"\", NULL_KEY, \"\"); } listen(integer c, string n, key id, string m) { llOwnerSay(m); } }";
  const QJsonObject a = run({object("worn", ear, {{"attached", 2}}), object("far", ear, {{"position", QJsonArray{128, 178, 20}}})}, 2, rlv,
                            nullptr, {QJsonObject{{"at", 1}, {"text", "hello"}}, QJsonObject{{"at", 1}, {"text", "HEY"}, {"kind", "shout"}}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(lines(a, {"owner_say"}), QStringList({"1 worn owner_say hello", "1 worn owner_say HEY", "1 far owner_say HEY"}));
}

void LslWorldTest::refusesBadInput()
{
  RlvState rlv;
  auto error = [&rlv](const QJsonObject& request) { return Control::run(request, &rlv)["error"].toString(); };
  const QString ok = "default { state_entry() { } }";
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"objects", QJsonArray{}}}),
           QString("Give either \"source\" (one script) or \"objects\" (several scripts), not both."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{}}}), QString("\"objects\" must hold 1 to 16 objects."));
  QJsonArray many;
  for (int i = 0; i < 17; ++i) many.append(object(QString("o%1").arg(i), ok));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", many}}), QString("\"objects\" must hold 1 to 16 objects."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok), object("a", ok)}}}),
           QString("Object 2: two objects are named \"a\"; each needs its own name."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{QJsonObject{{"name", "a"}}}}}),
           QString("Object 1: missing \"source\": the object's LSL script as text (or \"scripts\" for several)."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok, {{"position", QJsonArray{1, 2}}})}}}),
           QString("Object 1: \"position\" must be [x, y, z] in metres: x and y 0 to 256, z 0 to 4096."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok, {{"attached", 56}})}}}),
           QString("Object 1: \"attached\" must be an attachment point from 1 to 55, or 0 (not attached)."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok, {{"key", "not a key!"}})}}}),
           QString("Object 1: \"key\" must be a key: 1 to 36 letters, digits or \"-\"."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok, {{"colour", 1}})}}}),
           QString("Object 1: unknown field \"colour\". Fields: name, source, scripts, prims, key, owner, owner_name, position, attached, description, scale, start_parameter, inventory."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok)}},
                             {"inputs", QJsonArray{QJsonObject{{"type", "touch"}, {"object", "b"}}}}}),
           QString("Input 1: there is no object named \"b\"."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok)}},
                             {"inputs", QJsonArray{QJsonObject{{"type", "dialog"}}}}}),
           QString("Input 1: a dialog answer needs \"button\": the text of the button pressed."));
  QCOMPARE(error(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok)}},
                             {"inputs", QJsonArray{QJsonObject{{"type", "touch"}, {"kind", "shout"}}}}}),
           QString("Input 1: \"kind\" is only for chat."));
  const QJsonObject bad = Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{object("a", ok), object("b", "default { x }")}}}, &rlv);
  QCOMPARE(bad["error"].toString(), QString("The script of object \"b\" does not compile, so nothing was run. The errors are listed (the same as lsl_check)."));
  QCOMPARE(bad["object"].toString(), QString("b"));
  QVERIFY(!bad["errors"].toArray().isEmpty());
}

void LslWorldTest::eventNeedsAWorld()
{
  Session session;
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}}, &session)["error"].toString(),
           QString("There is no running world to send the event to. Start one with lsl_run first (with \"source\" or \"objects\")."));
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}}, &session)["error"].toString(),
           QString("Missing \"type\": chat, touch, dialog, attach, detach, changed, sit, stand, control, money, http_request, email, collision, land_collision, "
                   "moving, at_target, not_at_target or step."));
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "fly"}}, &session)["error"].toString(),
           QString("\"type\" must be \"chat\", \"touch\", \"dialog\", \"attach\", \"detach\", \"changed\", \"sit\", \"stand\", \"control\", \"money\", \"http_request\", "
                   "\"email\", \"collision\", \"land_collision\", \"moving\", \"at_target\", \"not_at_target\" or \"step\"."));
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"at", 1}}, &session)["error"].toString(),
           QString("Unknown option \"at\". Fields: type, object, channel, kind, name, id, text, button, point, link, hold, change, levels, edges, amount, url, method, body, headers, "
                   "address, subject, seconds, phase, pos, tnum, targetpos, ourpos, dt."));
  RlvState rlv;
  run({object("a", "default { touch_start(integer n) { } }")}, 1, rlv, &session);
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"object", "zz"}}, &session)["error"].toString(),
           QString("There is no object named \"zz\" in the world of the last lsl_run."));
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"seconds", 4000}}, &session)["error"].toString(),
           QString("\"seconds\" must be between 0 and 3600 (simulated seconds)."));
}

void LslWorldTest::singleScriptCanTakeEvents()
{
  RlvState rlv;
  Session session;
  QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { llListen(4, \"\", NULL_KEY, \"\"); } listen(integer c, string n, key id, string m) { llOwnerSay(m); } }"},
                                           {"seconds", 1}}, &rlv, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  // the single form keeps its log without "object"
  QVERIFY(!a["log"].toArray().first().toObject().contains("object"));
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "chat"}, {"channel", 4}, {"text", "hi"}}, &session);
  QCOMPARE(lines(a, {"input", "owner_say"}), QStringList({"1  input hi", "1  owner_say hi"}));
}

void LslWorldTest::examplesRun()
{
  RlvState rlv;
  const QJsonObject a = run({object("hud", example("hud_cuffs_hud.lsl"), {{"attached", 35}}),
                             object("cuffs", example("hud_cuffs_cuffs.lsl"), {{"attached", 30}})},
                            2, rlv, nullptr, {QJsonObject{{"at", 1}, {"type", "touch"}, {"object", "hud"}},
                                              QJsonObject{{"at", 1.5}, {"type", "dialog"}, {"object", "hud"}, {"button", "Lock"}}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(a["warnings"].toArray().size(), 0);
  QCOMPARE(a["stopped"].toBool(), false);
  QCOMPARE(rlv.objects().value("c0ffee00-0000-4000-8000-000000000003"), QStringList({"detach"}));
  QCOMPARE(a["dialog"].toObject()["buttons"].toArray(), QJsonArray({"Lock", "Unlock"}));
  QCOMPARE(lines(a, {"owner_say"}), QStringList({"0 cuffs owner_say Cuffs worn.", "1.5 hud owner_say Cuffs: locked"}));
}

QTEST_APPLESS_MAIN(LslWorldTest)
#include "tst_lslworld.moc"
