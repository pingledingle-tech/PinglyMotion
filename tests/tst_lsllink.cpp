/*
 * PinglyMotion - tests for several scripts in one object (Script Lab step 9a, issue 305)
 *
 * Link messages (round trips, LINK_* targets in single-prim and multi-prim
 * objects, the order they run in), the 64-event queue of each script, a
 * Stack-Heap Collision from a link message too large for the receiver, chat
 * between prims of one object, endless loops between scripts, Linkset Data
 * (limits, UTF-8 sizes, events, survival of a script reset and of
 * lsl_event) and the "scripts" shape of lsl_run's objects. Every expected
 * value is exact.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"
#include "scriptlab/lsl/sandbox.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

class LslLinkTest : public QObject
{
  Q_OBJECT

  private slots:
    void roundTripInOnePrim();
    void linkTargetsSinglePrim();
    void linkTargetsSeveralPrims();
    void queueOf64DropsTheRest();
    void stackHeapCollision();
    void primDoesNotHearItself();
    void endlessLoopBetweenScripts();
    void linksetDataStore();
    void linksetDataInScripts();
    void linksetDataSurvivesResetAndEvent();
    void scriptsShapeOfObjects();
    void refusesBadScripts();
    void touchGoesToThePrim();
    void notSupportedYetIsClear();

  private:
    static QJsonObject script(const QString& name, const QString& source, int link = 0);
    static QJsonObject run(const QJsonArray& objects, double seconds, RlvState& rlv, Session* session = nullptr,
                           const QJsonArray& inputs = QJsonArray());
    // "t script type detail" for the entries of the given types
    static QStringList lines(const QJsonArray& log, const QStringList& types);
};

QJsonObject LslLinkTest::script(const QString& name, const QString& source, int link)
{
  QJsonObject o{{"name", name}, {"source", source}};
  if (link > 0) o["link"] = link;
  return o;
}

QJsonObject LslLinkTest::run(const QJsonArray& objects, double seconds, RlvState& rlv, Session* session, const QJsonArray& inputs)
{
  QJsonObject request{{"command", "lsl_run"}, {"objects", objects}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request, &rlv, session);
}

QStringList LslLinkTest::lines(const QJsonArray& log, const QStringList& types)
{
  QStringList out;
  for (const QJsonValue& v : log)
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
    else if (type == "message_linked")
      detail = QString("%1 %2 %3 -> %4").arg(e["link"].toInt()).arg(e["num"].toInt()).arg(e["text"].toString()).arg(e["scripts"].toInt());
    else if (type == "linkset_data")
      detail = QString("%1 %2 %3 %4").arg(e["op"].toString(), e["name"].toString(), e["result"].toString()).arg(e["used"].toInt());
    else if (e.contains("text")) detail = e["text"].toString();
    else if (e.contains("message")) detail = e["message"].toString();
    out << QString("%1 %2 %3 %4").arg(e["t"].toDouble()).arg(e["script"].toString(), type, detail);
  }
  return out;
}

void LslLinkTest::roundTripInOnePrim()
{
  RlvState rlv;
  // a asks with LINK_SET (which reaches a itself too), b answers the sender's link number
  const QString a = "default { state_entry() { llMessageLinked(LINK_SET, 7, \"hi\", \"k\"); }"
                    " link_message(integer s, integer n, string m, key id) { llOwnerSay(\"a got \" + (string)n + \" \" + m + \" from \" + (string)s); } }";
  const QString b = "default { link_message(integer s, integer n, string m, key id) {"
                    " if (n == 7) llMessageLinked(s, 8, \"re:\" + m, id); } }";
  const QJsonObject r = run({QJsonObject{{"name", "box"}, {"scripts", QJsonArray{script("a", a), script("b", b)}}}}, 1, rlv);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  // fixed order: a runs everything in its queue (its state_entry, then its own message), then b (next in the
  // list) runs its queue; b's answer goes to link 0, the single prim, so it reaches both scripts (b ignores 8);
  // then a runs again
  QCOMPARE(lines(r["log"].toArray(), {"event", "message_linked", "owner_say"}),
           QStringList({"0 a event state_entry", "0 a message_linked -1 7 hi -> 2",
                        "0 a event link_message(0,7,hi,k)", "0 a owner_say a got 7 hi from 0",
                        "0 b event link_message(0,7,hi,k)", "0 b message_linked 0 8 re:hi -> 2",
                        "0 b event link_message(0,8,re:hi,k)",
                        "0 a event link_message(0,8,re:hi,k)", "0 a owner_say a got 8 re:hi from 0"}));
  // every entry names its object and its script
  for (const QJsonValue& v : r["log"].toArray())
  {
    QCOMPARE(v.toObject()["object"].toString(), QString("box"));
    QVERIFY(v.toObject().contains("script"));
  }
  // the same request gives the same log
  RlvState rlv2;
  const QJsonObject again = run({QJsonObject{{"name", "box"}, {"scripts", QJsonArray{script("a", a), script("b", b)}}}}, 1, rlv2);
  QCOMPARE(QJsonDocument(again["log"].toArray()).toJson(), QJsonDocument(r["log"].toArray()).toJson());
}

void LslLinkTest::linkTargetsSinglePrim()
{
  RlvState rlv;
  const QString sender = "default { state_entry() { llOwnerSay((string)llGetLinkNumber());"
                         " llMessageLinked(LINK_ROOT, 0, \"root\", \"\"); llMessageLinked(0, 0, \"zero\", \"\");"
                         " llMessageLinked(LINK_THIS, 0, \"this\", \"\"); llMessageLinked(LINK_ALL_OTHERS, 0, \"others\", \"\");"
                         " llMessageLinked(LINK_ALL_CHILDREN, 0, \"children\", \"\"); llMessageLinked(LINK_SET, 0, \"set\", \"\");"
                         " llMessageLinked(2, 0, \"two\", \"\"); } }";
  const QString ear = "default { link_message(integer s, integer n, string m, key id) { llOwnerSay(m); } }";
  const QJsonObject r = run({QJsonObject{{"name", "one"}, {"scripts", QJsonArray{script("send", sender), script("ear", ear)}}}}, 1, rlv);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  // a single prim is link 0: LINK_ROOT (1), LINK_ALL_OTHERS, LINK_ALL_CHILDREN and 2 reach nothing
  QCOMPARE(lines(r["log"].toArray(), {"message_linked", "owner_say"}),
           QStringList({"0 send owner_say 0", "0 send message_linked 1 0 root -> 0", "0 send message_linked 0 0 zero -> 2",
                        "0 send message_linked -4 0 this -> 2", "0 send message_linked -2 0 others -> 0",
                        "0 send message_linked -3 0 children -> 0", "0 send message_linked -1 0 set -> 2",
                        "0 send message_linked 2 0 two -> 0",
                        "0 ear owner_say zero", "0 ear owner_say this", "0 ear owner_say set"}));
}

void LslLinkTest::linkTargetsSeveralPrims()
{
  RlvState rlv;
  const QString sender = "default { state_entry() {"
                         " llMessageLinked(LINK_ROOT, 0, \"root\", \"\"); llMessageLinked(LINK_ALL_OTHERS, 0, \"others\", \"\");"
                         " llMessageLinked(LINK_ALL_CHILDREN, 0, \"children\", \"\"); llMessageLinked(LINK_THIS, 0, \"this\", \"\");"
                         " llMessageLinked(3, 0, \"three\", \"\"); llMessageLinked(LINK_SET, 0, \"set\", \"\");"
                         " llMessageLinked(0, 0, \"zero\", \"\"); llMessageLinked(9, 0, \"nine\", \"\"); } }";
  const QString ear = "default { link_message(integer s, integer n, string m, key id) {"
                      " llOwnerSay(m + \" in \" + (string)llGetLinkNumber() + \" from \" + (string)s); } }";
  // the sender is in prim 2; the root is 1 in an object of several prims. "scripts" counts every script in
  // the prims the link reaches, also one without a link_message handler (the sender here)
  const QJsonObject o{{"name", "set"}, {"prims", 3},
                      {"scripts", QJsonArray{script("root", ear, 1), script("send", sender, 2), script("third", ear, 3)}}};
  const QJsonObject r = run({o}, 1, rlv);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  QCOMPARE(lines(r["log"].toArray(), {"message_linked"}),
           QStringList({"0 send message_linked 1 0 root -> 1", "0 send message_linked -2 0 others -> 2",
                        "0 send message_linked -3 0 children -> 2", "0 send message_linked -4 0 this -> 1",
                        "0 send message_linked 3 0 three -> 1", "0 send message_linked -1 0 set -> 3",
                        "0 send message_linked 0 0 zero -> 0", "0 send message_linked 9 0 nine -> 0"}));
  // "third" comes after "send" in the list, so it runs its messages in the same round; "root" in the next
  QCOMPARE(lines(r["log"].toArray(), {"owner_say"}),
           QStringList({"0 third owner_say others in 3 from 2", "0 third owner_say children in 3 from 2",
                        "0 third owner_say three in 3 from 2", "0 third owner_say set in 3 from 2",
                        "0 root owner_say root in 1 from 2", "0 root owner_say others in 1 from 2",
                        "0 root owner_say set in 1 from 2"}));
  const QJsonArray scripts = r["objects"].toArray()[0].toObject()["scripts"].toArray();
  QCOMPARE(scripts.size(), 3);
  QCOMPARE(scripts[0].toObject()["link"].toInt(), 1);
  QCOMPARE(scripts[1].toObject()["link"].toInt(), 2);
  QCOMPARE(r["objects"].toArray()[0].toObject()["prims"].toInt(), 3);
}

void LslLinkTest::queueOf64DropsTheRest()
{
  RlvState rlv;
  // "flood" sends 70 messages in its state_entry; "slow" still has its own state_entry waiting, so 63 fit
  // in its queue of 64 and 7 are dropped (Second Life drops them silently; the sandbox logs each one)
  const QString flood = "default { state_entry() { integer i; for (i = 0; i < 70; ++i) llMessageLinked(LINK_SET, i, \"\", \"\"); } }";
  const QString slow = "integer got; default { state_entry() { } link_message(integer s, integer n, string m, key id) { ++got;"
                       " if (n == 62) llOwnerSay((string)got); } }";
  const QJsonObject r = run({QJsonObject{{"name", "q"}, {"scripts", QJsonArray{script("flood", flood), script("slow", slow)}}}}, 1, rlv);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  const QStringList dropped = lines(r["log"].toArray(), {"event_dropped"});
  QCOMPARE(dropped.size(), 7);
  QCOMPARE(dropped.first(), QString("0 slow event_dropped More than 64 events were waiting; this one was dropped, as in Second Life."));
  QCOMPARE(lines(r["log"].toArray(), {"owner_say"}), QStringList({"0 slow owner_say 63"}));
}

void LslLinkTest::stackHeapCollision()
{
  // small memory so the sizes are easy to count: an integer, a key or an empty string costs 16 bytes,
  // a string 16 plus 2 per character (the sandbox's rough count)
  Limits limits;
  limits.maxMemoryBytes = 1000;
  const QString big = QString(450, QChar('x'));
  auto parse = [](const QString& source) { return std::shared_ptr<const Script>(Parser::parse(source).script.release()); };
  RlvState rlv;
  Sandbox sandbox(rlv, true);
  ObjectSpec spec;
  spec.name = "o";
  ScriptEntry sender;
  sender.name = "sender";
  sender.script = parse("default { state_entry() { llMessageLinked(LINK_SET, 1, \"" + big + "\", \"\"); } }");
  ScriptEntry full;
  full.name = "full";   // holds 100 characters already: 216 bytes
  full.script = parse("string s = \"" + QString(100, QChar('y')) + "\"; default { link_message(integer a, integer b, string c, key d) { llOwnerSay(\"full\"); } }");
  ScriptEntry empty;
  empty.name = "empty";
  empty.script = parse("default { link_message(integer a, integer b, string c, key d) { llOwnerSay((string)llStringLength(c)); } }");
  QVERIFY(sender.script && full.script && empty.script);
  QCOMPARE(sandbox.addObject(spec, std::vector<ScriptEntry>{sender, full, empty}, limits), QString());
  sandbox.start();
  sandbox.runUntil(1.0);
  // 16 + 16 + (16 + 900) + 16 = 964 bytes arrive; 216 + 964 > 1000 crashes "full", 0 + 964 fits in "empty"
  QCOMPARE(lines(sandbox.log(), {"error", "owner_say"}),
           QStringList({"0 full error Stack-Heap Collision: the link_message event brought 964 bytes, more than the script's free memory (216 of 1000 bytes used), so the script crashed, as in Second Life.",
                        "0 empty owner_say 450"}));
  QCOMPARE(sandbox.scriptInterpreter(1).stopped(), true);
  QCOMPARE(sandbox.scriptInterpreter(2).stopped(), false);
}

void LslLinkTest::primDoesNotHearItself()
{
  RlvState rlv;
  const QString talker = "default { state_entry() { llSay(4, \"hello\"); } }";
  const QString ear = "default { state_entry() { llListen(4, \"\", NULL_KEY, \"\"); }"
                      " listen(integer c, string n, key id, string m) { llOwnerSay(llGetScriptName() + \" heard \" + m + \" from \" + n); } }";
  // the ears come first in the list so their listens are open when the talker speaks
  const QJsonObject o{{"name", "pair"}, {"prims", 2},
                      {"scripts", QJsonArray{script("same", ear, 1), script("other", ear, 2), script("talker", talker, 1)}}};
  const QJsonObject r = run({o}, 1, rlv);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  // the talker's own prim does not hear it; the other prim of the same object does
  QCOMPARE(lines(r["log"].toArray(), {"owner_say"}), QStringList({"0 other owner_say other heard hello from pair"}));
}

void LslLinkTest::endlessLoopBetweenScripts()
{
  // the event limit, not the real-time limit, must end it, so the test does not depend on the machine's speed
  Limits limits;
  limits.maxEvents = 1000;
  limits.maxWallMs = 600000;
  const QString ping = "default { state_entry() { if (llGetLinkNumber() == 1) llMessageLinked(LINK_ALL_OTHERS, 0, \"x\", \"\"); }"
                       " link_message(integer s, integer n, string m, key id) { llMessageLinked(s, n + 1, m, id); } }";
  std::shared_ptr<const Script> script(Parser::parse(ping).script.release());
  QVERIFY(script);
  RlvState rlv;
  Sandbox sandbox(rlv, true);
  ObjectSpec spec;
  spec.name = "loop";
  spec.prims = 2;
  ScriptEntry a, b;
  a.name = "ping";
  a.script = script;
  b.name = "pong";
  b.link = 2;
  b.script = script;
  QCOMPARE(sandbox.addObject(spec, std::vector<ScriptEntry>{a, b}, limits), QString());
  sandbox.start();
  sandbox.runUntil(5.0);
  // the scripts take turns at one moment; pong handled its state_entry and the first message in the first
  // round, so it is one event ahead and reaches the event limit first; ping then waits for an answer forever
  QCOMPARE(sandbox.scriptInterpreter(1).stopReason(), QString("The script handled more than 1000 events and was stopped."));
  QCOMPARE(sandbox.scriptInterpreter(0).stopped(), false);
  // about 2 entries per event (the event and its message_linked): the log is full at 2000 entries, and the
  // entry that says why a script stopped always comes last
  QCOMPARE(sandbox.truncated(), true);
  const QJsonArray log = sandbox.log();
  QCOMPARE(log.size(), 2001);
  QCOMPARE(log.last().toObject()["type"].toString(), QString("limit"));
  QCOMPARE(log.last().toObject()["script"].toString(), QString("pong"));
  QCOMPARE(sandbox.now(), 5.0);
}

void LslLinkTest::linksetDataStore()
{
  LinksetData d;
  QCOMPARE(d.available(), 131072);
  QCOMPARE(d.write("", "x"), int(LinksetData::ENoKey));
  QCOMPARE(d.write("b", "2"), int(LinksetData::Ok));
  QCOMPARE(d.write("b", "2"), int(LinksetData::NoUpdate));
  QCOMPARE(d.write("a", "1"), int(LinksetData::Ok));
  QCOMPARE(d.write("c", ""), int(LinksetData::NotFound));
  // a pair costs the UTF-8 bytes of name and value: "ø" is 2 bytes
  QCOMPARE(d.write("ø", "øø"), int(LinksetData::Ok));
  QCOMPARE(d.used(), 2 + 2 + 6);
  QCOMPARE(d.keys(0, 0), QStringList({"a", "b", "ø"}));
  QCOMPARE(d.keys(1, 1), QStringList({"b"}));
  QCOMPARE(d.keys(-5, 2), QStringList({"a", "b"}));
  QCOMPARE(d.keys(3, 0), QStringList());
  QCOMPARE(d.write("a", ""), int(LinksetData::Ok));   // an empty value deletes
  QCOMPARE(d.read("a"), QString());
  QCOMPARE(d.remove("a"), int(LinksetData::NotFound));
  QCOMPARE(d.count(), 2);
  // 128 KiB in all
  d.reset();
  QCOMPARE(d.write("big", QString(131069, QChar('v'))), int(LinksetData::Ok));
  QCOMPARE(d.available(), 0);
  QCOMPARE(d.write("x", "y"), int(LinksetData::EMemory));
  QCOMPARE(d.write("big", QString(131070, QChar('v'))), int(LinksetData::EMemory));
  QCOMPARE(d.write("big", QString(131068, QChar('v'))), int(LinksetData::Ok));   // a smaller value replaces it
  QCOMPARE(d.available(), 1);
  QCOMPARE(LinksetData::resultName(LinksetData::NoUpdate), QString("LINKSETDATA_NOUPDATE"));
}

void LslLinkTest::linksetDataInScripts()
{
  RlvState rlv;
  const QString writer = "default { state_entry() {"
                         " llOwnerSay((string)llLinksetDataWrite(\"color\", \"red\"));"
                         " llOwnerSay((string)llLinksetDataWrite(\"color\", \"red\"));"
                         " llOwnerSay((string)llLinksetDataWrite(\"\", \"x\"));"
                         " llOwnerSay((string)llLinksetDataWrite(\"alpha\", \"1\"));"
                         " llOwnerSay(llDumpList2String(llLinksetDataListKeys(0, 0), \",\"));"
                         " llOwnerSay((string)llLinksetDataCountKeys() + \" \" + (string)llLinksetDataAvailable());"
                         " llOwnerSay((string)llLinksetDataWrite(\"alpha\", \"\"));"
                         " llOwnerSay((string)llLinksetDataDelete(\"alpha\"));"
                         " llLinksetDataReset(); } }";
  const QString reader = "default { linkset_data(integer action, string name, string value) {"
                         " llOwnerSay((string)action + \" \" + name + \"=\" + value + \" read \" + llLinksetDataRead(\"color\")); } }";
  const QJsonObject r = run({QJsonObject{{"name", "store"}, {"scripts", QJsonArray{script("writer", writer), script("reader", reader)}}}}, 1, rlv);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  QCOMPARE(lines(r["log"].toArray(), {"owner_say", "linkset_data"}),
           QStringList({"0 writer linkset_data write color LINKSETDATA_OK 8", "0 writer owner_say 0",
                        "0 writer linkset_data write color LINKSETDATA_NOUPDATE 8", "0 writer owner_say 5",
                        "0 writer linkset_data write  LINKSETDATA_ENOKEY 8", "0 writer owner_say 2",
                        "0 writer linkset_data write alpha LINKSETDATA_OK 14", "0 writer owner_say 0",
                        "0 writer owner_say alpha,color", "0 writer owner_say 2 131058",
                        "0 writer linkset_data delete alpha LINKSETDATA_OK 8", "0 writer owner_say 0",
                        "0 writer linkset_data delete alpha LINKSETDATA_NOTFOUND 8", "0 writer owner_say 4",
                        "0 writer linkset_data reset   0",
                        // the reader runs after the writer's event; the store is already empty then
                        "0 reader owner_say 1 color=red read ", "0 reader owner_say 1 alpha=1 read ",
                        "0 reader owner_say 2 alpha= read ", "0 reader owner_say 0 = read "}));
}

void LslLinkTest::linksetDataSurvivesResetAndEvent()
{
  RlvState rlv;
  Session session;
  // the data belongs to the object: a script reset (llResetScript) keeps it, its globals are gone
  const QString s = "integer runs; default { state_entry() { ++runs; llOwnerSay((string)runs + \" \" + llLinksetDataRead(\"n\"));"
                    " llLinksetDataWrite(\"n\", llLinksetDataRead(\"n\") + \"x\"); }"
                    " touch_start(integer t) { llResetScript(); } }";
  const QJsonObject r = run({QJsonObject{{"name", "keep"}, {"scripts", QJsonArray{script("s", s)}}}}, 1, rlv, &session,
                            QJsonArray{QJsonObject{{"at", 0.5}, {"type", "touch"}}});
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  QCOMPARE(lines(r["log"].toArray(), {"owner_say", "reset"}), QStringList({"0 s owner_say 1 ", "0.5 s reset ", "0.5 s owner_say 1 x"}));
  // and lsl_event runs on in the same world, with the same data
  const QJsonObject e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}}, &session);
  QVERIFY2(e["ok"].toBool(), qPrintable(e["error"].toString()));
  QCOMPARE(lines(e["log"].toArray(), {"owner_say"}), QStringList({"1 s owner_say 1 xx"}));
  const QJsonObject data = e["objects"].toArray()[0].toObject()["linkset_data"].toObject();
  QCOMPARE(data["count"].toInt(), 1);
  QCOMPARE(data["used"].toInt(), 4);
  QCOMPARE(data["available"].toInt(), 131068);
  QCOMPARE(data["pairs"].toObject(), QJsonObject({{"n", "xxx"}}));
}

void LslLinkTest::scriptsShapeOfObjects()
{
  RlvState rlv;
  const QString idle = "default { state_entry() { llOwnerSay(llGetScriptName() + \" \" + llGetObjectName()); } }";
  const QJsonObject r = run({QJsonObject{{"name", "multi"}, {"scripts", QJsonArray{script("one", idle), script("two", idle)}}},
                             QJsonObject{{"name", "old"}, {"source", idle}}}, 1, rlv);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  QCOMPARE(lines(r["log"].toArray(), {"owner_say"}), QStringList({"0 one owner_say one multi", "0 two owner_say two multi", "0  owner_say Script old"}));
  const QJsonObject multi = r["objects"].toArray()[0].toObject();
  QCOMPARE(multi["prims"].toInt(), 1);
  QCOMPARE(multi["stopped"].toBool(), false);
  QCOMPARE(multi["scripts"].toArray().size(), 2);
  const QJsonObject one = multi["scripts"].toArray()[0].toObject();
  QCOMPARE(one.keys(), QStringList({"link", "memory", "memory_code", "memory_estimate", "memory_limit", "messages", "name", "open_listens", "state", "steps", "stopped", "timer"}));
  QCOMPARE(one["link"].toInt(), 0);
  QCOMPARE(one["state"].toString(), QString("default"));
  QCOMPARE(multi["linkset_data"].toObject(), QJsonObject({{"count", 0}, {"used", 0}, {"available", 131072}, {"pairs", QJsonObject()}}));
  // the old shape has no scripts list and no "script" in its log entries
  const QJsonObject old = r["objects"].toArray()[1].toObject();
  QVERIFY(!old.contains("scripts"));
  QVERIFY(!old.contains("linkset_data"));
  // an object stops only when all its scripts stopped
  RlvState rlv2;
  const QJsonObject s = run({QJsonObject{{"name", "half"}, {"scripts", QJsonArray{script("bad", "default { state_entry() { integer i = 1 / 0; } }"),
                                                                                    script("good", idle)}}}}, 1, rlv2);
  QCOMPARE(s["objects"].toArray()[0].toObject()["stopped"].toBool(), false);
  QCOMPARE(s["objects"].toArray()[0].toObject()["scripts"].toArray()[0].toObject()["stopped"].toBool(), true);
}

void LslLinkTest::refusesBadScripts()
{
  RlvState rlv;
  auto error = [&](const QJsonObject& object) { return run({object}, 1, rlv)["error"].toString(); };
  const QString ok = "default { state_entry() { } }";
  QCOMPARE(error(QJsonObject{{"name", "a"}, {"source", ok}, {"scripts", QJsonArray{script("s", ok)}}}),
           QString("Object 1: give either \"source\" (one script) or \"scripts\" (several scripts), not both."));
  QCOMPARE(error(QJsonObject{{"name", "a"}, {"scripts", QJsonArray()}}), QString("Object 1: \"scripts\" must hold 1 to 32 scripts."));
  QCOMPARE(error(QJsonObject{{"name", "a"}, {"scripts", QJsonArray{script("s", ok), script("s", ok)}}}),
           QString("Object 1: script 2: two scripts are named \"s\"; each script of an object needs its own name."));
  QCOMPARE(error(QJsonObject{{"name", "a"}, {"prims", 2}, {"scripts", QJsonArray{script("s", ok, 3)}}}),
           QString("Object 1: a script is in link 3, but \"prims\" is 2."));
  QCOMPARE(error(QJsonObject{{"name", "a"}, {"scripts", QJsonArray{QJsonObject{{"name", "s"}, {"source", ok}, {"memory", 1}}}}}),
           QString("Object 1: script 1: unknown field \"memory\". Fields: name, source, link."));
  QCOMPARE(error(QJsonObject{{"name", "a"}, {"prims", 0}, {"source", ok}}),
           QString("Object 1: \"prims\" must be a whole number from 1 to 256 (how many prims the object has)."));
  const QJsonObject bad = run({QJsonObject{{"name", "a"}, {"scripts", QJsonArray{script("good", ok), script("broken", "default {")}}}}, 1, rlv);
  QCOMPARE(bad["error"].toString(), QString("The script \"broken\" of object \"a\" does not compile, so nothing was run. The errors are listed (the same as lsl_check)."));
  QCOMPARE(bad["script"].toString(), QString("broken"));
  // 64 scripts in all objects together
  QJsonArray many;
  for (int i = 0; i < 32; ++i) many.append(script(QString("s%1").arg(i), ok));
  QCOMPARE(run({QJsonObject{{"name", "a"}, {"scripts", many}}, QJsonObject{{"name", "b"}, {"scripts", many}},
                QJsonObject{{"name", "c"}, {"source", ok}}}, 1, rlv)["error"].toString(),
           QString("At most 64 scripts in all objects together."));
  // a touch on a prim the object does not have
  QCOMPARE(run({QJsonObject{{"name", "a"}, {"source", ok}}}, 1, rlv, nullptr, QJsonArray{QJsonObject{{"type", "touch"}, {"link", 2}}})["error"].toString(),
           QString("Input 1: the object \"a\" has 1 prim(s); there is no link 2."));
  QCOMPARE(run({QJsonObject{{"name", "a"}, {"source", ok}}}, 1, rlv, nullptr, QJsonArray{QJsonObject{{"type", "chat"}, {"link", 1}}})["error"].toString(),
           QString("Input 1: \"link\" is only for touch (the prim touched), sit (the prim sat on), money (the prim paid) and collision (the prim hit)."));
}

void LslLinkTest::touchGoesToThePrim()
{
  RlvState rlv;
  Session session;
  const QString toucher = "default { touch_start(integer n) { llOwnerSay(llGetScriptName() + \" touched\"); } }";
  const QString quiet = "default { state_entry() { } }";
  // prim 2 handles touches itself; prim 3 has no touch handler, so its touches go on to the root
  const QJsonObject o{{"name", "t"}, {"prims", 3},
                      {"scripts", QJsonArray{script("root", toucher, 1), script("second", toucher, 2), script("third", quiet, 3)}}};
  const QJsonObject r = run({o}, 1, rlv, &session, QJsonArray{QJsonObject{{"at", 0.1}, {"type", "touch"}, {"link", 2}},
                                                              QJsonObject{{"at", 0.2}, {"type", "touch"}, {"link", 3}},
                                                              QJsonObject{{"at", 0.3}, {"type", "touch"}}});
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  QCOMPARE(lines(r["log"].toArray(), {"owner_say"}),
           QStringList({"0.1 second owner_say second touched", "0.2 root owner_say root touched", "0.3 root owner_say root touched"}));
  const QJsonObject e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"link", 2}}, &session);
  QVERIFY2(e["ok"].toBool(), qPrintable(e["error"].toString()));
  QCOMPARE(lines(e["log"].toArray(), {"owner_say"}), QStringList({"1 second owner_say second touched"}));
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"link", 4}}, &session)["error"].toString(),
           QString("The object has 3 prim(s); there is no link 4."));
}

void LslLinkTest::notSupportedYetIsClear()
{
  // known to the checker (a warning), and the run stops there with "not supported yet"
  const QString source = "default { state_entry() { llLinksetDataWriteProtected(\"a\", \"b\", \"pass\"); } }";
  const QJsonObject check = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", source}});
  QCOMPARE(check["valid"].toBool(), true);
  QCOMPARE(check["warnings"].toArray().size(), 1);
  RlvState rlv;
  const QJsonObject r = run({QJsonObject{{"name", "p"}, {"scripts", QJsonArray{script("s", source)}}}}, 1, rlv);
  QCOMPARE(lines(r["log"].toArray(), {"unsupported"}),
           QStringList({"0 s unsupported llLinksetDataWriteProtected is not supported yet in the Script Lab sandbox, so the script was stopped here."}));
  // the new events are raised now, so lsl_check does not warn about them
  const QJsonObject events = Control::check(QJsonObject{{"command", "lsl_check"}, {"source",
    "default { link_message(integer s, integer n, string m, key id) { } linkset_data(integer a, string n, string v) { } }"}});
  QCOMPARE(events["valid"].toBool(), true);
  QCOMPARE(events["warnings"].toArray(), QJsonArray());
}

QTEST_GUILESS_MAIN(LslLinkTest)
#include "tst_lsllink.moc"
