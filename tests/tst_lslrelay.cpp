/*
 * PinglyMotion - tests for the Script Lab's built-in, simulated RLV relay (job 23, src/scriptlab/lsl/relay.*).
 *
 * The relay answers the relay protocol (1.100, with the ORG core x-tension 0003) on channel -1812221819. Every
 * reply is checked exactly: "<ident>,<controller key>,<command>,<answer>" for each command, in order, then the
 * viewer's answers to queries; ask, accept and decline; restrictions per controller in the RLV core; !release;
 * the release and ping inputs; messages that are ignored; the option's refusals. The controller's key is written
 * as ME in the expected lines (the trap prints what it hears with its own key replaced).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/relay.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

namespace
{
  // a trap: listens on the relay channel and on 2222, prints what it hears with its own key as ME, and sends `first`
  // (a relay message after "<ident>,<owner key>,") when it starts; a touch sends `touched`, chat on 5 sends that text
  QString trap(const QString& first, const QString& touched = QString(), const QString& how = "llSay(RELAY, ")
  {
    return QString(R"lsl(
integer RELAY = -1812221819;
send(string commands) { %3"trap," + (string)llGetOwner() + "," + commands); }
default
{
  state_entry()
  {
    llListen(RELAY, "", NULL_KEY, "");
    llListen(2222, "", NULL_KEY, "");
    llListen(5, "", NULL_KEY, "");
    if ("%1" != "") send("%1");
  }
  touch_start(integer n) { send("%2"); }
  listen(integer c, string n, key id, string m)
  {
    if (c == 5) { llSay(RELAY, m); return; }
    llOwnerSay((string)c + " " + n + " " + llReplaceSubString(m, (string)llGetKey(), "ME", 0));
  }
}
)lsl").arg(first, touched, how);
  }

  QStringList said(const QJsonObject& a)
  {
    QStringList out;
    if (!a["ok"].toBool()) return QStringList{"NOT OK: " + a["error"].toString()};
    for (const QJsonValue& v : a["log"].toArray())
      if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
    return out;
  }

  QJsonArray relayEntries(const QJsonObject& a)
  {
    QJsonArray out;
    for (const QJsonValue& v : a["log"].toArray())
      if (v.toObject()["type"].toString() == "relay") out.append(v);
    return out;
  }

  QJsonObject run(const QString& source, const QJsonValue& relay, const QJsonArray& inputs = QJsonArray(), double seconds = 2)
  {
    QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}, {"relay", relay}};
    if (!inputs.isEmpty()) request["inputs"] = inputs;
    return Control::run(request);
  }

  QString controllerKey(const QJsonObject& a)
  {
    return relayEntries(a).isEmpty() ? QString() : relayEntries(a).first().toObject()["from"].toString();
  }
}

class LslRelayTest : public QObject
{
  Q_OBJECT

  private slots:
    void metaCommands();
    void rlvCommandsAndViewerAnswer();
    void restrictionsPerController();
    void releaseByController();
    void askAccept();
    void askDecline();
    void askRunsOut();
    void denyMode();
    void liftingNeverAsks();
    void regionSayToReachesTheRelay();
    void ignoredMessages();
    void wildcardKey();
    void pingFreesFarControllers();
    void outOfReach();
    void releaseInput();
    void pingInput();
    void pongIsNotAnswered();
    void noRelayByDefault();
    void refusedOptions();
    void relayExampleStillWorks();
};

void LslRelayTest::metaCommands()
{
  const QJsonObject a = run(trap("!version|!implversion|!x-orgversions|!handover|!who|!x-who|hello"), true);
  QCOMPARE(said(a), QStringList({
    "-1812221819 RLV Relay trap,ME,!version,1100",
    "-1812221819 RLV Relay trap,ME,!implversion,PinglyMotion Script Lab relay 1",
    "-1812221819 RLV Relay trap,ME,!x-orgversions,ORG=0003",
    "-1812221819 RLV Relay trap,ME,!handover,ko",
    "-1812221819 RLV Relay trap,ME,!who,ko",
    "-1812221819 RLV Relay trap,ME,!x-who,ko",
    "-1812221819 RLV Relay trap,ME,hello,ko"}));
  // nothing that needs asking was sent: the controller is not kept
  QCOMPARE(a["relay"].toObject()["controllers"].toArray().size(), 0);
  QCOMPARE(a["relay"].toObject()["mode"].toString(), QString("auto"));
  QCOMPARE(a["relay"].toObject()["key"].toString(), QString(RlvRelay::KEY));
}

void LslRelayTest::rlvCommandsAndViewerAnswer()
{
  const QJsonObject a = run(trap("@detach=n|@sendim=n|@version=2222|@nosuchthing=n"), true);
  QCOMPARE(said(a), QStringList({
    "-1812221819 RLV Relay trap,ME,@detach=n,ok",
    "-1812221819 RLV Relay trap,ME,@sendim=n,ok",
    "-1812221819 RLV Relay trap,ME,@version=2222,ok",
    "-1812221819 RLV Relay trap,ME,@nosuchthing=n,ko",
    "2222 Owner RestrainedLife viewer v3.4.3 (PinglyMotion Script Lab)"}));
  const QString me = controllerKey(a);
  QVERIFY(!me.isEmpty());
  // the RLV core holds the restrictions under the controller's key
  const QJsonObject objects = a["rlv"].toObject()["state"].toObject()["objects"].toObject();
  QCOMPARE(objects[me].toArray(), QJsonArray({"detach", "sendim"}));
  const QJsonArray controllers = a["relay"].toObject()["controllers"].toArray();
  QCOMPARE(controllers.size(), 1);
  QCOMPARE(controllers[0].toObject()["key"].toString(), me);
  QCOMPARE(controllers[0].toObject()["trust"].toString(), QString("accepted"));
  QCOMPARE(controllers[0].toObject()["restrictions"].toArray(), QJsonArray({"detach", "sendim"}));
  // the log entry: one result per command
  const QJsonObject entry = relayEntries(a).first().toObject();
  QCOMPARE(entry["text"].toString(), "trap,a2e76fcd-9360-4f6d-a924-000000000001,@detach=n|@sendim=n|@version=2222|@nosuchthing=n");
  QCOMPARE(entry["commands"].toArray().size(), 4);
  QCOMPARE(entry["commands"].toArray()[0].toObject()["reply"].toString(), QString("ok"));
  QCOMPARE(entry["commands"].toArray()[0].toObject()["answer"].toString(), QString("accept"));
  QCOMPARE(entry["commands"].toArray()[3].toObject()["reply"].toString(), QString("ko"));
  QCOMPARE(entry["replies"].toArray().last().toObject()["viewer"].toBool(), true);
  QCOMPARE(entry["restrictions"].toArray(), QJsonArray({"detach", "sendim"}));
}

void LslRelayTest::restrictionsPerController()
{
  // two traps: each holds its own restrictions; Trap One releasing (on a touch) leaves Trap Two's
  const QString k1 = "70000000-0000-4000-8000-000000000001", k2 = "70000000-0000-4000-8000-000000000002";
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 2}, {"relay", true}, {"objects", QJsonArray{
    QJsonObject{{"name", "Trap One"}, {"key", k1}, {"source", trap("@detach=n|@fly=n", "!release")}},
    QJsonObject{{"name", "Trap Two"}, {"key", k2}, {"source", trap("@fly=n|@tplm=n")}}}},
    {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"object", "Trap One"}}}}};
  const QJsonObject a = Control::run(request);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QJsonObject objects = a["rlv"].toObject()["state"].toObject()["objects"].toObject();
  QVERIFY(!objects.contains(k1));
  QCOMPARE(objects[k2].toArray(), QJsonArray({"fly", "tplm"}));
  const QJsonArray blocked = a["rlv"].toObject()["state"].toObject()["blocked"].toArray();
  QVERIFY(blocked.contains("fly"));
  QVERIFY(!blocked.contains("detach"));
  QCOMPARE(a["relay"].toObject()["controllers"].toArray().size(), 1);
  QCOMPARE(a["relay"].toObject()["controllers"].toArray()[0].toObject()["name"].toString(), QString("Trap Two"));
}

void LslRelayTest::releaseByController()
{
  const QJsonObject a = run(trap("@detach=n", "!release"), true, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}});
  QCOMPARE(said(a), QStringList({"-1812221819 RLV Relay trap,ME,@detach=n,ok", "-1812221819 RLV Relay trap,ME,!release,ok"}));
  QVERIFY(a["rlv"].toObject()["state"].toObject()["objects"].toObject().isEmpty());
  QCOMPARE(a["relay"].toObject()["controllers"].toArray().size(), 0);
}

void LslRelayTest::askAccept()
{
  const QJsonObject a = run(trap("!version|@detach=n|@fly=n"), QJsonObject{{"mode", "ask"}, {"answers", QJsonArray{"accept"}}});
  QCOMPARE(said(a), QStringList({"-1812221819 RLV Relay trap,ME,!version,1100", "-1812221819 RLV Relay trap,ME,@detach=n,ok",
                                 "-1812221819 RLV Relay trap,ME,@fly=n,ok"}));
  const QJsonArray commands = relayEntries(a).first().toObject()["commands"].toArray();
  QVERIFY(!commands[0].toObject().contains("asked"));
  QCOMPARE(commands[1].toObject()["asked"].toBool(), true);
  QCOMPARE(commands[1].toObject()["answer"].toString(), QString("accept"));
  QVERIFY(!commands[2].toObject().contains("asked"));   // asked once per controller
  QCOMPARE(a["relay"].toObject()["answers_left"].toInt(), 0);
}

void LslRelayTest::askDecline()
{
  // declined: every command that needs a yes gets ko, a version query still works; the decline holds for later messages
  const QJsonObject a = run(trap("@detach=n|@versionnum=2222", "@fly=n|@getstatus=2222"), QJsonObject{{"mode", "ask"}, {"answers", QJsonArray{"decline", "accept"}}},
                            QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}});
  QCOMPARE(said(a), QStringList({
    "-1812221819 RLV Relay trap,ME,@detach=n,ko",
    "-1812221819 RLV Relay trap,ME,@versionnum=2222,ok",
    "2222 Owner 3040300",
    "-1812221819 RLV Relay trap,ME,@fly=n,ko",
    "-1812221819 RLV Relay trap,ME,@getstatus=2222,ko"}));
  QVERIFY(a["rlv"].toObject()["state"].toObject()["objects"].toObject().isEmpty());
  QCOMPARE(a["relay"].toObject()["answers_left"].toInt(), 1);
  QCOMPARE(a["relay"].toObject()["controllers"].toArray()[0].toObject()["trust"].toString(), QString("declined"));
}

void LslRelayTest::askRunsOut()
{
  const QJsonObject a = run(trap("@detach=n"), QJsonObject{{"mode", "ask"}});
  QCOMPARE(said(a), QStringList({"-1812221819 RLV Relay trap,ME,@detach=n,ko"}));
  QCOMPARE(relayEntries(a).first().toObject()["commands"].toArray()[0].toObject()["answer"].toString(), QString("decline"));
}

void LslRelayTest::denyMode()
{
  const QJsonObject a = run(trap("!version|@detach=n|@version=2222"), QJsonObject{{"mode", "deny"}});
  QCOMPARE(said(a), QStringList({"-1812221819 RLV Relay trap,ME,!version,1100", "-1812221819 RLV Relay trap,ME,@detach=n,ko",
                                 "-1812221819 RLV Relay trap,ME,@version=2222,ok", "2222 Owner RestrainedLife viewer v3.4.3 (PinglyMotion Script Lab)"}));
}

void LslRelayTest::liftingNeverAsks()
{
  const QJsonObject a = run(trap("@detach=y|@clear"), QJsonObject{{"mode", "deny"}});
  QCOMPARE(said(a), QStringList({"-1812221819 RLV Relay trap,ME,@detach=y,ok", "-1812221819 RLV Relay trap,ME,@clear,ok"}));
}

void LslRelayTest::regionSayToReachesTheRelay()
{
  // llRegionSayTo to the avatar reaches what she wears, wherever the trap stands
  const QString source = trap("@detach=n", QString(), "llRegionSayTo(llGetOwner(), RELAY, ");
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 2}, {"relay", true},
                      {"objects", QJsonArray{QJsonObject{{"name", "Far Trap"}, {"position", QJsonArray{250, 250, 20}}, {"source", source}}}}};
  QCOMPARE(said(Control::run(request)), QStringList({"-1812221819 RLV Relay trap,ME,@detach=n,ok"}));
}

void LslRelayTest::ignoredMessages()
{
  // chat on 5 is sent on the relay channel as it is
  const QJsonArray inputs{
    QJsonObject{{"at", 0.5}, {"channel", 5}, {"text", "trap,6a7e0c3d-0000-4000-8000-0000000000a1,@detach=n"}},
    QJsonObject{{"at", 0.6}, {"channel", 5}, {"text", "trap,a2e76fcd-9360-4f6d-a924-000000000001"}},
    QJsonObject{{"at", 0.7}, {"channel", 5}, {"text", "trap,a2e76fcd-9360-4f6d-a924-000000000001,@detach=n,@fly=n"}}};
  const QJsonObject a = run(trap(""), true, inputs);
  QCOMPARE(said(a), QStringList());
  const QJsonArray entries = relayEntries(a);
  QCOMPARE(entries.size(), 3);
  QCOMPARE(entries[0].toObject()["ignored"].toString(), QString("The message is for another avatar."));
  QCOMPARE(entries[1].toObject()["ignored"].toString(), QString("Not a relay message: it must be <ident>,<avatar key>,<commands>, with exactly two commas."));
  QCOMPARE(entries[2].toObject()["ignored"].toString(), QString("Not a relay message: it must be <ident>,<avatar key>,<commands>, with exactly two commas."));
  QVERIFY(a["rlv"].toObject()["state"].toObject()["objects"].toObject().isEmpty());
}

void LslRelayTest::wildcardKey()
{
  // ORG core: the wildcard key counts as the avatar's
  const QJsonArray inputs{QJsonObject{{"at", 0.5}, {"channel", 5}, {"text", "scan,ffffffff-ffff-ffff-ffff-ffffffffffff,!version|@detach=n"}}};
  const QJsonObject a = run(trap(""), true, inputs);
  QCOMPARE(said(a), QStringList({"-1812221819 RLV Relay scan,ME,!version,1100", "-1812221819 RLV Relay scan,ME,@detach=n,ok"}));
  QCOMPARE(a["rlv"].toObject()["state"].toObject()["objects"].toObject()[controllerKey(a)].toArray(), QJsonArray({"detach"}));
}

void LslRelayTest::pingFreesFarControllers()
{
  // a controller that holds a restriction and stands beyond shout range (100 metres) is not reachable: the ping frees it
  const QString k1 = "70000000-0000-4000-8000-000000000001", k2 = "70000000-0000-4000-8000-000000000002";
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 2}, {"relay", true}, {"objects", QJsonArray{
    QJsonObject{{"name", "Far Trap"}, {"key", k1}, {"position", QJsonArray{250, 250, 20}}, {"source", trap("@detach=n", QString(), "llRegionSayTo(llGetOwner(), RELAY, ")}},
    QJsonObject{{"name", "Near Trap"}, {"key", k2}, {"position", QJsonArray{128, 200, 20}}, {"source", trap("@fly=n", QString(), "llShout(RELAY, ")}}}},
    {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "relay_ping"}}}}};
  const QJsonObject a = Control::run(request);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QJsonObject entry = relayEntries(a).last().toObject();
  QCOMPARE(entry["pinged"].toArray(), QJsonArray({k2}));
  QCOMPARE(entry["freed"].toArray(), QJsonArray({k1}));
  const QJsonObject objects = a["rlv"].toObject()["state"].toObject()["objects"].toObject();
  QVERIFY(!objects.contains(k1));
  QCOMPARE(objects[k2].toArray(), QJsonArray({"fly"}));
}

void LslRelayTest::outOfReach()
{
  // llSay reaches 20 metres; the avatar stands at 128, 128, 20
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 2}, {"relay", true},
                      {"objects", QJsonArray{QJsonObject{{"name", "Far Trap"}, {"position", QJsonArray{128, 150, 20}}, {"source", trap("@detach=n")}}}}};
  const QJsonObject a = Control::run(request);
  QCOMPARE(said(a), QStringList());
  QCOMPARE(relayEntries(a).size(), 0);
  request["objects"] = QJsonArray{QJsonObject{{"name", "Near Trap"}, {"position", QJsonArray{128, 147, 20}}, {"source", trap("@detach=n")}}};
  QCOMPARE(said(Control::run(request)), QStringList({"-1812221819 RLV Relay trap,ME,@detach=n,ok"}));
}

void LslRelayTest::releaseInput()
{
  const QJsonObject a = run(trap("@detach=n|@fly=n"), true, QJsonArray{QJsonObject{{"at", 1}, {"type", "relay_release"}}});
  QCOMPARE(said(a), QStringList({"-1812221819 RLV Relay trap,ME,@detach=n,ok", "-1812221819 RLV Relay trap,ME,@fly=n,ok",
                                 "-1812221819 RLV Relay release,ME,!release,ok"}));
  QVERIFY(a["rlv"].toObject()["state"].toObject()["objects"].toObject().isEmpty());
  const QJsonObject entry = relayEntries(a).last().toObject();
  QCOMPARE(entry["action"].toString(), QString("release"));
  QCOMPARE(entry["freed"].toArray().size(), 1);
}

void LslRelayTest::pingInput()
{
  // two traps hold restrictions; one deletes itself; the ping frees it and pings the other
  const QString k1 = "70000000-0000-4000-8000-000000000001", k2 = "70000000-0000-4000-8000-000000000002";
  const QString dying = QString(trap("@detach=n")).replace("touch_start(integer n) { send(\"\"); }", "touch_start(integer n) { llDie(); }");
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 3}, {"relay", true}, {"objects", QJsonArray{
    QJsonObject{{"name", "Gone Trap"}, {"key", k1}, {"source", dying}},
    QJsonObject{{"name", "Live Trap"}, {"key", k2}, {"source", trap("@fly=n")}}}},
    {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"object", "Gone Trap"}}, QJsonObject{{"at", 2}, {"type", "relay_ping"}}}}};
  const QJsonObject a = Control::run(request);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QVERIFY(said(a).contains("-1812221819 RLV Relay ping,ME,ping,ping"));
  const QJsonObject entry = relayEntries(a).last().toObject();
  QCOMPARE(entry["action"].toString(), QString("ping"));
  QCOMPARE(entry["pinged"].toArray(), QJsonArray({k2}));
  QCOMPARE(entry["freed"].toArray(), QJsonArray({k1}));
  const QJsonObject objects = a["rlv"].toObject()["state"].toObject()["objects"].toObject();
  QVERIFY(!objects.contains(k1));
  QCOMPARE(objects[k2].toArray(), QJsonArray({"fly"}));
}

void LslRelayTest::pongIsNotAnswered()
{
  const QJsonObject a = run(trap("@detach=n|!pong"), true);
  QCOMPARE(said(a), QStringList({"-1812221819 RLV Relay trap,ME,@detach=n,ok"}));
  QCOMPARE(relayEntries(a).first().toObject()["commands"].toArray()[1].toObject()["note"].toString(), QString("The controller answered the relay's ping."));
}

void LslRelayTest::noRelayByDefault()
{
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 2}, {"source", trap("@detach=n")}};
  const QJsonObject a = Control::run(request);
  QCOMPARE(said(a), QStringList());
  QVERIFY(!a.contains("relay"));
  QCOMPARE(said(run(trap("@detach=n"), false)), QStringList());
  // a relay input without a relay says so
  request["inputs"] = QJsonArray{QJsonObject{{"at", 1}, {"type", "relay_ping"}}};
  bool found = false;
  for (const QJsonValue& v : Control::run(request)["log"].toArray())
    if (v.toObject()["input"].toString() == "relay_ping")
    {
      QCOMPARE(v.toObject()["error"].toString(), QString("No relay is worn: give lsl_run the relay option."));
      found = true;
    }
  QVERIFY(found);
}

void LslRelayTest::refusedOptions()
{
  auto error = [](const QJsonValue& relay) { return run("default { state_entry() { } }", relay)["error"].toString(); };
  QCOMPARE(error(3), QString("\"relay\" must be true, false or {\"mode\": \"auto\", \"ask\" or \"deny\", \"answers\": [\"accept\" or \"decline\", ...]}."));
  QCOMPARE(error(QJsonObject{{"mode", "maybe"}}),
           QString("Relay: \"mode\" must be \"auto\" (accept every object), \"ask\" (ask the avatar; the answers list answers) or \"deny\" (refuse every object)."));
  QCOMPARE(error(QJsonObject{{"answers", QJsonArray{"accept"}}}), QString("Relay: \"answers\" is only for the mode \"ask\"."));
  QCOMPARE(error(QJsonObject{{"mode", "ask"}, {"answers", QJsonArray{"yes"}}}),
           QString("Relay: \"answers\" must be a list of at most 64 entries, each \"accept\" or \"decline\"."));
  QCOMPARE(error(QJsonObject{{"wear", true}}), QString("Relay: unknown field \"wear\". Fields: mode, answers."));
}

void LslRelayTest::relayExampleStillWorks()
{
  // the repository's own simple relay script keeps working next to (and without) the built-in relay
  QFile file(QString(PINGLYMOTION_EXAMPLES) + "/objects/RLV_relay/relay.lsl");
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QString source = QString::fromUtf8(file.readAll());
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 1}, {"source", source}};
  QVERIFY(Control::run(request)["ok"].toBool());
  request["relay"] = true;
  QVERIFY(Control::run(request)["ok"].toBool());
}

QTEST_GUILESS_MAIN(LslRelayTest)
#include "tst_lslrelay.moc"
