/*
 * PinglyMotion - tests for the channel map (Script Lab, job A4): which boxes and arrows a run of a
 * given folder gives, the plain-word error for a channel nobody listens on, the hub, the listen
 * filters, listens removed by llListenRemove and a state change, menus, RLV lines, web requests,
 * inputs from you, the script line each arrow came from, and the cap on arrows
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include "scriptlab/labchannelmap.h"
#include "scriptlab/labfolder.h"
#include "scriptlab/rlvstate.h"

using namespace LabChannelMap;

namespace
{
  bool writeFile(const QString& path, const QByteArray& text)
  {
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  // the arrows as plain lines: "from -> to | label | error | file:line:column"
  QStringList lines(const Map& map)
  {
    QStringList out;
    for (const Arrow& a : map.arrows)
    {
      QString line = map.boxes.value(a.from).label() + QStringLiteral(" -> ")
                     + (a.to < 0 ? QStringLiteral("nobody") : map.boxes.value(a.to).label()) + QStringLiteral(" | ") + a.label();
      if (!a.error.isEmpty()) line += QStringLiteral(" | ") + a.error;
      if (!a.file.isEmpty()) line += QStringLiteral(" | %1:%2:%3").arg(a.file).arg(a.line).arg(a.column);
      out << line;
    }
    return out;
  }

  QJsonObject entry(const QString& json)
  {
    return QJsonDocument::fromJson(json.toUtf8()).object();
  }

  QJsonArray log(const QStringList& entries)
  {
    QJsonArray out;
    for (const QString& e : entries) out.append(entry(e));
    return out;
  }

  // the hub folder: the root listens on -77 and passes what it hears on as a link message; the collar
  // says "lock" on -77, says hello on channel 7 (nobody listens), and on the link message sends an RLV
  // line and a web request
  const QByteArray HubScript =
    "integer CMD = -77;\n"                                        // 1
    "default\n"                                                   // 2
    "{\n"                                                         // 3
    "    state_entry()\n"                                         // 4
    "    {\n"                                                     // 5
    "        llListen(CMD, \"\", NULL_KEY, \"\");\n"              // 6
    "    }\n"                                                     // 7
    "    listen(integer channel, string name, key id, string message)\n"   // 8
    "    {\n"                                                     // 9
    "        // llSay(1, \"not this one\");\n"                    // 10
    "        llMessageLinked(LINK_ALL_OTHERS, 5, message, NULL_KEY);\n"     // 11
    "    }\n"                                                     // 12
    "}\n";
  const QByteArray CollarScript =
    "default\n"                                                   // 1
    "{\n"                                                         // 2
    "    state_entry()\n"                                         // 3
    "    {\n"                                                     // 4
    "        llSay(7, \"hello\");\n"                              // 5
    "        llSay(-77, \"lock\");\n"                             // 6
    "    }\n"                                                     // 7
    "    link_message(integer sender, integer num, string str, key id)\n"  // 8
    "    {\n"                                                     // 9
    "        llOwnerSay(\"@detach=n\");\n"                        // 10
    "        llHTTPRequest(\"https://example.com/log\", [], str);\n"        // 11
    "    }\n"                                                     // 12
    "}\n";
}

class TestLabChannelMap : public QObject
{
  Q_OBJECT

  private slots:
    void folderRun();
    void folderRunJson();
    void unheardChannel();
    void listenFilters();
    void listenRemovedAndState();
    void stoppedScriptHearsNothing();
    void menuAndAnswer();
    void youSayAndTouch();
    void linkMessageToNobody();
    void regionSayTo();
    void rlvRefusedAndReply();
    void countAndCap();
    void findCallSkipsComments();
    void emptyFolderResult();
    void labels();
    void hub();
};

// a real folder run: the boxes in link order, then your viewer and the web server, and every arrow
void TestLabChannelMap::folderRun()
{
  QTemporaryDir dir;
  const QString folder = dir.path() + "/Gear";
  QVERIFY(writeFile(folder + "/Root/hub.lsl", HubScript));
  QVERIFY(writeFile(folder + "/Collar/collar.lsl", CollarScript));
  RlvState rlv;
  const QJsonObject result = LabFolder::run(LabFolder::open(folder), QJsonObject(), &rlv);
  QVERIFY2(result.value("ok").toBool(), qPrintable(QJsonDocument(result).toJson()));
  QJsonObject folderResult = result;
  const Map map = fromFolderResult(folderResult, [&](const QString& file) { return readFile(folder + "/" + file); });

  QStringList boxes;
  for (const Box& b : map.boxes) boxes << b.label();
  QCOMPARE(boxes, QStringList({"Root", "Collar", "Your viewer (RLV)", "Web server"}));
  QCOMPARE(map.boxes[0].link, 1);
  QCOMPARE(map.boxes[1].link, 2);
  QVERIFY(!map.boxes[0].hub);   // it only answers the part that spoke to it
  QVERIFY(!map.boxes[1].hub);
  QCOMPARE(lines(map), QStringList({
    "Collar -> nobody | say on channel 7: \"hello\" | Nobody listens on channel 7. | Collar/collar.lsl:5:9",
    "Collar -> Root | say on channel -77: \"lock\" | Collar/collar.lsl:6:9",
    "Root -> Collar | link message 5: \"lock\" | Root/hub.lsl:11:9",
    "Collar -> Your viewer (RLV) | RLV line: \"@detach=n\" | Collar/collar.lsl:10:9",
    "Collar -> Web server | web request: \"GET https://example.com/log\" | Collar/collar.lsl:11:9",
  }));
  QCOMPARE(map.unheard(), 1);
  QCOMPARE(map.messages(), 5);
  QVERIFY(!map.truncated);
}

void TestLabChannelMap::folderRunJson()
{
  QTemporaryDir dir;
  const QString folder = dir.path() + "/Gear";
  QVERIFY(writeFile(folder + "/Root/hub.lsl", HubScript));
  QVERIFY(writeFile(folder + "/Collar/collar.lsl", CollarScript));
  RlvState rlv;
  const Map map = fromFolderResult(LabFolder::run(LabFolder::open(folder), QJsonObject(), &rlv));
  const QJsonObject json = map.toJson();
  const QJsonArray boxes = json.value("boxes").toArray();
  QCOMPARE(boxes.size(), 4);
  QCOMPARE(boxes[0].toObject(), entry(R"({"kind":"part","name":"Root","link":1})"));
  QCOMPARE(boxes[2].toObject(), entry(R"j({"kind":"viewer","name":"Your viewer (RLV)"})j"));
  QCOMPARE(boxes[3].toObject(), entry(R"({"kind":"web","name":"Web server"})"));
  const QJsonObject first = json.value("arrows").toArray().first().toObject();
  // without a source reader there is no line, but the file is known
  QCOMPARE(first, entry(R"({"kind":"chat","from":1,"to":-1,"how":"say","channel":7,"text":"hello","count":1,"t":0,)"
                        R"("error":"Nobody listens on channel 7.","file":"Collar/collar.lsl"})"));
}

void TestLabChannelMap::unheardChannel()
{
  const Map map = build(log({R"({"t":0,"type":"say","channel":7,"text":"hi","object":"Lamp"})",
                             R"({"t":1,"type":"shout","channel":-3,"text":"x","object":"Lamp"})",
                             R"({"t":2,"type":"region_say","channel":9,"text":"y","object":"Lamp"})",
                             R"({"t":3,"type":"say","channel":0,"text":"Hello, avatar.","object":"Lamp"})"}),
                        QJsonArray(), {});
  QCOMPARE(lines(map), QStringList({
    "Lamp -> nobody | say on channel 7: \"hi\" | Nobody listens on channel 7.",
    "Lamp -> nobody | shout on channel -3: \"x\" | Nobody listens on channel -3.",
    "Lamp -> nobody | region say on channel 9: \"y\" | Nobody listens on channel 9.",
    "Lamp -> You | say on channel 0: \"Hello, avatar.\"",   // open chat: you hear it
  }));
}

// name, key and message filters, as lsl::Interpreter::receive
void TestLabChannelMap::listenFilters()
{
  const QJsonArray objects{entry(R"({"name":"HUD","key":"k-hud","owner":"k-owner"})"),
                           entry(R"({"name":"Cuff","key":"k-cuff","owner":"k-owner"})")};
  const Map map = build(log({
    R"({"t":0,"type":"listen","handle":1,"channel":5,"name":"","id":"k-hud","message":"","object":"Cuff"})",
    R"({"t":0,"type":"listen","handle":2,"channel":6,"name":"Other","id":"","message":"","object":"Cuff"})",
    R"({"t":0,"type":"listen","handle":3,"channel":7,"name":"","id":"","message":"lock","object":"Cuff"})",
    R"({"t":0,"type":"listen","handle":4,"channel":8,"name":"","id":"k-owner","message":"","object":"Cuff"})",
    R"({"t":1,"type":"say","channel":5,"text":"a","object":"HUD"})",
    R"({"t":1,"type":"say","channel":6,"text":"b","object":"HUD"})",
    R"({"t":1,"type":"say","channel":7,"text":"free","object":"HUD"})",
    R"({"t":1,"type":"say","channel":7,"text":"lock","object":"HUD"})",
    R"({"t":1,"type":"say","channel":8,"text":"c","object":"HUD"})",
    R"({"t":2,"type":"input","input":"chat","channel":8,"name":"Owner","text":"d"})"}),
    objects, {});
  QCOMPARE(lines(map), QStringList({
    "HUD -> Cuff | say on channel 5: \"a\"",
    "HUD -> nobody | say on channel 6: \"b\" | Nobody listens on channel 6.",
    "HUD -> nobody | say on channel 7: \"free\" | Nobody listens on channel 7.",
    "HUD -> Cuff | say on channel 7: \"lock\"",
    "HUD -> nobody | say on channel 8: \"c\" | Nobody listens on channel 8.",
    "You -> Cuff | you say on channel 8: \"d\"",
  }));
}

void TestLabChannelMap::listenRemovedAndState()
{
  const Map map = build(log({
    R"({"t":0,"type":"listen","handle":1,"channel":5,"name":"","id":"","message":"","object":"B","script":"b"})",
    R"({"t":0,"type":"listen","handle":2,"channel":6,"name":"","id":"","message":"","object":"B","script":"b"})",
    R"({"t":1,"type":"say","channel":5,"text":"one","object":"A","script":"a"})",
    R"({"t":2,"type":"listen_remove","handle":1,"object":"B","script":"b"})",
    R"({"t":3,"type":"say","channel":5,"text":"two","object":"A","script":"a"})",
    R"({"t":3,"type":"say","channel":6,"text":"three","object":"A","script":"a"})",
    R"({"t":4,"type":"state","name":"locked","object":"B","script":"b"})",
    R"({"t":5,"type":"say","channel":6,"text":"four","object":"A","script":"a"})"}),
    QJsonArray(), {});
  QCOMPARE(lines(map), QStringList({
    "A -> B | say on channel 5: \"one\"",
    "A -> nobody | say on channel 5: \"two\" | Nobody listens on channel 5.",
    "A -> B | say on channel 6: \"three\"",
    "A -> nobody | say on channel 6: \"four\" | Nobody listens on channel 6.",
  }));
}

void TestLabChannelMap::stoppedScriptHearsNothing()
{
  const Map map = build(log({
    R"({"t":0,"type":"listen","handle":1,"channel":5,"name":"","id":"","message":"","object":"B"})",
    R"({"t":1,"type":"error","message":"Math error","line":3,"column":1,"object":"B"})",
    R"({"t":2,"type":"say","channel":5,"text":"x","object":"A"})"}),
    QJsonArray(), {});
  QCOMPARE(lines(map), QStringList({"A -> nobody | say on channel 5: \"x\" | Nobody listens on channel 5."}));
}

void TestLabChannelMap::menuAndAnswer()
{
  const Map map = build(log({
    R"({"t":0,"type":"dialog","to":"k","message":"Gear set","buttons":["Lock all"],"channel":-7001,"object":"HUD"})",
    R"({"t":1,"type":"input","input":"dialog","name":"Owner","channel":-7001,"text":"Lock all","object":"HUD"})",
    R"({"t":2,"type":"listen","handle":1,"channel":-7001,"name":"","id":"","message":"","object":"HUD"})",
    R"({"t":3,"type":"input","input":"dialog","name":"Owner","channel":-7001,"text":"Free all","object":"HUD"})"}),
    QJsonArray(), {});
  QCOMPARE(lines(map), QStringList({
    "HUD -> You | menu on channel -7001: \"Gear set\"",
    "You -> nobody | menu answer on channel -7001: \"Lock all\" | Nobody listens on channel -7001, so the menu answer is lost.",
    "You -> HUD | menu answer on channel -7001: \"Free all\"",
  }));
}

void TestLabChannelMap::youSayAndTouch()
{
  const QVector<Prim> prims{{1, "Root", true}, {2, "Lamp", true}};
  const Map map = build(log({
    R"({"t":0,"type":"input","input":"touch","name":"Owner","link":2,"prim":"Lamp","source":"sandbox","object":"Obj"})",
    R"({"t":0,"type":"input","input":"touch","name":"Owner","link":2,"prim":"Lamp","source":"sandbox","object":"Obj"})",
    R"({"t":1,"type":"input","input":"chat","name":"Owner","channel":3,"text":"on","prim":"Root","source":"sandbox"})",
    R"({"t":2,"type":"input","input":"touch","name":"Owner","error":"There is no object named \"X\"."})"}),
    QJsonArray(), prims);
  QCOMPARE(lines(map), QStringList({
    "You -> Lamp | click (2 times)",
    "You -> nobody | you say on channel 3: \"on\" | Nobody listens on channel 3.",
  }));
  QCOMPARE(map.boxes.size(), 3);   // Root, Lamp, You: a refused input draws nothing
}

void TestLabChannelMap::linkMessageToNobody()
{
  const QVector<Prim> prims{{1, "Root", true}, {2, "Cuff", false}, {3, "Lamp", true}};
  const Map map = build(log({
    R"({"t":0,"type":"message_linked","link":2,"num":4,"text":"x","id":"","scripts":0,"prim":"Root","script":"main","file":"main.lsl"})",
    R"({"t":0,"type":"message_linked","link":-1,"num":5,"text":"y","id":"","scripts":2,"prim":"Root","script":"main","file":"main.lsl"})",
    R"({"t":0,"type":"message_linked","link":-3,"num":6,"text":"z","id":"","scripts":1,"prim":"Lamp","script":"lamp","file":"Lamp/lamp.lsl"})"}),
    QJsonArray(), prims);
  QCOMPARE(lines(map), QStringList({
    "Root -> nobody | link message 4: \"x\" | No script gets link message 4. | main.lsl:0:0",
    "Root -> Root | link message 5: \"y\" | main.lsl:0:0",
    "Root -> Lamp | link message 5: \"y\" | main.lsl:0:0",
    "Lamp -> Lamp | link message 6: \"z\" | Lamp/lamp.lsl:0:0",
  }));
}

void TestLabChannelMap::regionSayTo()
{
  const QJsonArray objects{entry(R"({"name":"HUD","key":"k-hud","owner":"k-owner"})"),
                           entry(R"({"name":"Cuff","key":"k-cuff","owner":"k-owner"})")};
  const Map map = build(log({
    R"({"t":0,"type":"listen","handle":1,"channel":5,"name":"","id":"","message":"","object":"Cuff"})",
    R"({"t":0,"type":"listen","handle":1,"channel":5,"name":"","id":"","message":"","object":"HUD"})",
    R"({"t":1,"type":"region_say_to","to":"k-cuff","channel":5,"text":"a","object":"HUD"})",
    R"({"t":1,"type":"region_say_to","to":"k-other","channel":5,"text":"b","object":"HUD"})",
    R"({"t":1,"type":"region_say_to","to":"k-owner","channel":0,"text":"c","object":"HUD"})"}),
    objects, {});
  QCOMPARE(lines(map), QStringList({
    "HUD -> Cuff | say to one on channel 5: \"a\"",
    "HUD -> nobody | say to one on channel 5: \"b\" | Nobody with that key listens on channel 5.",
    "HUD -> You | say to one on channel 0: \"c\"",
  }));
}

void TestLabChannelMap::rlvRefusedAndReply()
{
  const Map map = build(log({
    R"({"t":0,"type":"rlv_command","text":"@version=2222","replies":[{"channel":2222,"text":"RestrainedLove 3"}],"object":"Collar"})",
    R"({"t":1,"type":"rlv_command","text":"@nope=n","error":"Unknown command","object":"Collar"})",
    R"({"t":2,"type":"owner_say","text":"Locked.","object":"Collar"})",
    R"({"t":3,"type":"instant_message","to":"k","text":"Hi","object":"Collar"})",
    R"({"t":4,"type":"input","input":"http_request","method":"POST","object":"Collar"})"}),
    QJsonArray(), {});
  QCOMPARE(lines(map), QStringList({
    "Collar -> Your viewer (RLV) | RLV line: \"@version=2222\"",
    "Your viewer (RLV) -> Collar | answer on channel 2222: \"RestrainedLove 3\"",
    "Collar -> nobody | RLV line: \"@nope=n\" | Your viewer refused this line: Unknown command",
    "Collar -> You | owner say: \"Locked.\"",
    "Collar -> You | message to you: \"Hi\"",
    "Web server -> Collar | web request in: \"POST\"",
  }));
}

void TestLabChannelMap::countAndCap()
{
  QStringList entries;
  for (int i = 0; i < 3; ++i) entries << R"({"t":0,"type":"say","channel":7,"text":"same","object":"A"})";
  Map map = build(log(entries), QJsonArray(), {});
  QCOMPARE(map.arrows.size(), 1);
  QCOMPARE(map.arrows[0].count, 3);
  QCOMPARE(map.arrows[0].label(), QString("say on channel 7: \"same\" (3 times)"));
  QCOMPARE(map.messages(), 3);
  entries.clear();
  for (int i = 0; i < MAX_ARROWS + 5; ++i)
    entries << QStringLiteral(R"({"t":0,"type":"say","channel":%1,"text":"x","object":"A"})").arg(i + 1);
  map = build(log(entries), QJsonArray(), {});
  QCOMPARE(map.arrows.size(), MAX_ARROWS);
  QVERIFY(map.truncated);
  QVERIFY(map.toJson().value("truncated").toBool());
}

void TestLabChannelMap::findCallSkipsComments()
{
  QCOMPARE(findCall(QString::fromUtf8(HubScript), {"llMessageLinked"}, "lock").line, 11);
  QCOMPARE(findCall(QString::fromUtf8(HubScript), {"llSay"}, "").line, 0);   // only in a comment
  const QString two = "default { state_entry() {\n llSay(1, \"a\");\n llSay(2, \"b\");\n } }\n";
  QCOMPARE(findCall(two, {"llSay"}, "b").line, 3);
  QCOMPARE(findCall(two, {"llSay"}, "c").line, 2);   // no line holds the text: the first call
  QCOMPARE(findCall(two, {"llSay"}, "b").column, 2);
  QCOMPARE(findCall("not lsl at all", {"llSay"}, "").line, 0);
}

void TestLabChannelMap::emptyFolderResult()
{
  QVERIFY(fromFolderResult(QJsonObject()).boxes.isEmpty());
  QVERIFY(fromFolderResult(entry(R"({"ok":false,"error":"x"})")).arrows.isEmpty());
}

void TestLabChannelMap::labels()
{
  Box b;
  b.name = "Root";
  QCOMPARE(b.label(), QString("Root"));
  b.hub = true;
  QCOMPARE(b.label(), QString("Root (hub)"));
  QCOMPARE(b.explain(), QString("This part listens and passes messages on, like a server."));
  b.kind = BoxKind::Web;
  QCOMPARE(b.explain(), QString("A web server outside Second Life. The sandbox never goes online."));
  Arrow a;
  a.kind = ArrowKind::Chat;
  a.how = "whisper";
  a.channel = 4;
  QCOMPARE(a.label(), QString("whisper on channel 4"));
  a.kind = ArrowKind::Touch;
  QCOMPARE(a.label(), QString("click"));
}

// the menu answer comes from you and the HUD passes it on to the cuffs: the HUD is a hub
void TestLabChannelMap::hub()
{
  const QVector<Prim> prims{{1, "HUD", true}, {2, "Cuff", true}, {3, "Collar", true}};
  const Map map = build(log({
    R"({"t":0,"type":"listen","handle":1,"channel":-5,"name":"","id":"","message":"","prim":"HUD","script":"hud"})",
    R"({"t":1,"type":"input","input":"dialog","name":"Owner","channel":-5,"text":"Lock","prim":"HUD","source":"sandbox"})",
    R"({"t":1,"type":"message_linked","link":-2,"num":7,"text":"lock","id":"","scripts":2,"prim":"HUD","script":"hud"})",
    R"({"t":1,"type":"message_linked","link":1,"num":8,"text":"locked","id":"","scripts":1,"prim":"Cuff","script":"cuff"})"}),
    QJsonArray(), prims);
  QCOMPARE(lines(map), QStringList({
    "You -> HUD (hub) | menu answer on channel -5: \"Lock\"",
    "HUD (hub) -> Cuff | link message 7: \"lock\"",
    "HUD (hub) -> Collar | link message 7: \"lock\"",
    "Cuff -> HUD (hub) | link message 8: \"locked\"",
  }));
  QVERIFY(map.boxes[0].hub);
  QVERIFY(!map.boxes[1].hub);
  QCOMPARE(map.boxes[0].explain(), QString("This part listens and passes messages on, like a server."));
}

QTEST_GUILESS_MAIN(TestLabChannelMap)
#include "tst_labchannelmap.moc"
