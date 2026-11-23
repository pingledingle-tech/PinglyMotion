/*
 * PinglyMotion - tests for the last steps of the Script Lab gap list: the obsolete functions llSound, llTakeCamera and
 * llCloud (step 16) and remote data, the deprecated XML-RPC functions (step 17)
 *
 * The expected answers were worked out by hand from the Second Life wiki. Where the wiki is silent or unclear the
 * sandbox chooses; every choice is written here and in docs/CONTROL.md:
 *  - llSound is record only (one "recorded_only" entry, like llPlaySound); llTakeCamera does nothing (the wiki: never
 *    implemented) and is recorded only; llCloud gives 0.0 (the wiki: since 13.02.08.270166 it only returns 0.0). The
 *    deprecated warning of the checker comes from the viewer's keyword list, which marks llSound and llTakeCamera but not llCloud.
 *  - llOpenRemoteDataChannel gives a script one channel, announced by remote_data(REMOTE_DATA_CHANNEL, channel,
 *    NULL_KEY, "", 0, ""). A second call keeps the same channel and announces it again (the wiki says the same ID is kept;
 *    the repeat is a guess). Nothing is opened on the network. llCloseRemoteDataChannel closes it; another key is "not_found".
 *  - A request comes from an input {"type": "remote_data"} (kind "request", the default): remote_data(REMOTE_DATA_REQUEST,
 *    channel, message, sender, idata, sdata) with "id" the message (default: made up), "address" the sender (default
 *    192.0.2.10, a documentation address), "idata", "text" as sdata and "channel_key" the channel (default: the object's
 *    open channel). Without an open channel there is no event and the input entry has "error". Which parameter holds what
 *    is a guess: the wiki lists the parameters of the event but not their meaning.
 *  - llRemoteDataReply is only recorded: "sent" when the channel is the script's and the message was asked, else
 *    "unknown_channel" or "unknown_message" (a guess). A message is answered once.
 *  - llSendRemoteData is only recorded and gives the message key. Input kind "reply" answers it:
 *    remote_data(REMOTE_DATA_REPLY, channel as given to llSendRemoteData, message, sender, idata, sdata); "id" is the
 *    message (default: the newest one sent). A message is answered once.
 *  - llRemoteDataSetRegion is recorded. No forced delay is simulated (the wiki: 1 s for open and close, 3 s for send and reply).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/lint.h"
#include "scriptlab/lsl/parser.h"

using namespace lsl;

namespace
{
  // the first key a script of the run makes with llGenerateKey-style counting is 5c11ab00-0000-4000-8000-000000000001
  const char* const CH1 = "5c11ab00-0000-4000-8000-000000000001";
  const char* const MSG2 = "5c11ab00-0000-4000-8000-000000000002";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";

  // opens a channel and says everything remote_data brings
  const char* const SERVER = R"lsl(default {
    state_entry() { llOpenRemoteDataChannel(); }
    remote_data(integer type, key channel, key message, string sender, integer idata, string sdata) {
      llOwnerSay((string)type + "|" + (string)channel + "|" + (string)message + "|" + sender + "|" + (string)idata + "|" + sdata);
      if (type == REMOTE_DATA_REQUEST) llRemoteDataReply(channel, message, "re " + sdata, idata + 1);
    }
  })lsl";
}

class LslSteps1617Test : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void obsoleteFunctions();
    void openChannel();
    void openTwiceKeepsTheChannel();
    void requestAndReply();
    void requestFields();
    void requestWithoutChannel();
    void unknownMessageAndChannel();
    void answeredOnce();
    void closeChannel();
    void sendAndAnswer();
    void sendNewestAndUnknown();
    void setRegion();
    void constantsAndEvent();
    void inputErrors_data();
    void inputErrors();
    void deprecatedLint();
    void checkerHasNoWarnings();

  private:
    static QJsonObject runSource(const QString& source, const QJsonArray& inputs = QJsonArray(), double seconds = 2);
    static QStringList said(const QJsonObject& answer);
    static QJsonArray ofType(const QJsonObject& answer, const QString& type, const QString& op = QString());
};

QJsonObject LslSteps1617Test::runSource(const QString& source, const QJsonArray& inputs, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request);
}

QStringList LslSteps1617Test::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QJsonArray LslSteps1617Test::ofType(const QJsonObject& answer, const QString& type, const QString& op)
{
  QJsonArray out;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == type && (op.isEmpty() || v.toObject()["op"].toString() == op)) out.append(v);
  return out;
}

void LslSteps1617Test::supportedNow()
{
  for (const char* name : {"llSound", "llTakeCamera", "llCloud", "llOpenRemoteDataChannel", "llCloseRemoteDataChannel",
                           "llRemoteDataSetRegion", "llSendRemoteData", "llRemoteDataReply"})
  {
    QVERIFY2(Builtins::function(name) != nullptr, name);
    QVERIFY2(Builtins::function(name)->supported, name);
  }
  // llGetSimStats is the unsupported example; llWarp2Pos is no LSL function
  QVERIFY(!Builtins::function("llGetSimStats")->supported);
  QVERIFY(Builtins::function("llWarp2Pos") == nullptr);
}

void LslSteps1617Test::obsoleteFunctions()
{
  const QJsonObject a = runSource(R"lsl(default { state_entry() {
    llSound("bang", 0.5, TRUE, FALSE);
    llTakeCamera(llGetOwner());
    llOwnerSay((string)llCloud(<1.0, 2.0, 3.0>));
  } })lsl");
  QCOMPARE(said(a), QStringList{"0.000000"});
  const QJsonArray rec = ofType(a, "recorded_only");
  QCOMPARE(int(rec.size()), 2);
  QCOMPARE(rec[0].toObject()["function"].toString(), QString("llSound"));
  QCOMPARE(rec[0].toObject()["args"].toArray().size(), 4);
  QCOMPARE(rec[0].toObject()["args"].toArray()[0].toString(), QString("bang"));
  QCOMPARE(rec[1].toObject()["function"].toString(), QString("llTakeCamera"));
}

void LslSteps1617Test::openChannel()
{
  const QJsonObject a = runSource(SERVER);
  QCOMPARE(said(a), QStringList{QString("1|%1|%2||0|").arg(CH1, NULL_KEY)});
  const QJsonArray open = ofType(a, "remote_data", "open");
  QCOMPARE(int(open.size()), 1);
  QCOMPARE(open[0].toObject()["channel"].toString(), QString(CH1));
  QVERIFY(open[0].toObject()["new"].toBool());
}

void LslSteps1617Test::openTwiceKeepsTheChannel()
{
  const QJsonObject a = runSource(R"lsl(default {
    state_entry() { llOpenRemoteDataChannel(); llOpenRemoteDataChannel(); }
    remote_data(integer type, key channel, key message, string sender, integer idata, string sdata) { llOwnerSay((string)type + " " + (string)channel); }
  })lsl");
  QCOMPARE(said(a), (QStringList{QString("1 %1").arg(CH1), QString("1 %1").arg(CH1)}));
  const QJsonArray open = ofType(a, "remote_data", "open");
  QCOMPARE(int(open.size()), 2);
  QVERIFY(open[0].toObject()["new"].toBool());
  QVERIFY(!open[1].toObject()["new"].toBool());
}

void LslSteps1617Test::requestAndReply()
{
  const QString msg = "aaaaaaaa-0000-4000-8000-000000000001";
  const QJsonObject a = runSource(SERVER, QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"id", msg}, {"address", "203.0.113.5"},
                                                                  {"idata", 41}, {"text", "ping"}}});
  QCOMPARE(said(a), (QStringList{QString("1|%1|%2||0|").arg(CH1, NULL_KEY), QString("2|%1|%2|203.0.113.5|41|ping").arg(CH1, msg)}));
  const QJsonArray reply = ofType(a, "remote_data", "reply");
  QCOMPARE(int(reply.size()), 1);
  const QJsonObject r = reply[0].toObject();
  QCOMPARE(r["channel"].toString(), QString(CH1));
  QCOMPARE(r["message"].toString(), msg);
  QCOMPARE(r["sdata"].toString(), QString("re ping"));
  QCOMPARE(r["idata"].toInt(), 42);
  QCOMPARE(r["result"].toString(), QString("sent"));
  // the input is in the log too
  const QJsonArray in = ofType(a, "input");
  QCOMPARE(int(in.size()), 1);
  QCOMPARE(in[0].toObject()["input"].toString(), QString("remote_data"));
  QCOMPARE(in[0].toObject()["kind"].toString(), QString("request"));
  QCOMPARE(in[0].toObject()["message"].toString(), msg);
  QVERIFY(!in[0].toObject().contains("error"));
}

void LslSteps1617Test::requestFields()
{
  // defaults: the sender is the documentation address, idata 0, sdata empty, a made-up message key
  const QJsonObject a = runSource(SERVER, QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"channel_key", CH1}}});
  const QStringList got = said(a);
  QCOMPARE(got.size(), 2);
  const QStringList parts = got[1].split('|');
  QCOMPARE(parts.size(), 6);
  QCOMPARE(parts[0], QString("2"));
  QCOMPARE(parts[1], QString(CH1));
  QCOMPARE(parts[2], QString("c0de0000-0000-4000-8000-000000000001"));
  QCOMPARE(parts[3], QString("192.0.2.10"));
  QCOMPARE(parts[4], QString("0"));
  QCOMPARE(parts[5], QString());
}

void LslSteps1617Test::requestWithoutChannel()
{
  const QJsonObject a = runSource("default { state_entry() { } remote_data(integer t, key c, key m, string s, integer i, string d) { llOwnerSay(\"got\"); } }",
                                  QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"text", "x"}}});
  QVERIFY(said(a).isEmpty());
  const QJsonArray in = ofType(a, "input");
  QCOMPARE(int(in.size()), 1);
  QCOMPARE(in[0].toObject()["error"].toString(), QString("No script of the object has an open channel; a script opens one with llOpenRemoteDataChannel."));
  // a channel key that is not the script's
  const QJsonObject b = runSource(SERVER, QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"channel_key", "nope"}}});
  QCOMPARE(said(b).size(), 1);
  QCOMPARE(ofType(b, "input")[0].toObject()["error"].toString(), QString("No script of the object has this channel open."));
}

void LslSteps1617Test::unknownMessageAndChannel()
{
  const QJsonObject b = runSource(R"lsl(default {
    state_entry() {
      llOpenRemoteDataChannel();
      llRemoteDataReply((key)"5c11ab00-0000-4000-8000-000000000001", (key)"zzz", "s", 1);
      llRemoteDataReply((key)"other", (key)"zzz", "s", 2);
    }
  })lsl");
  const QJsonArray replies = ofType(b, "remote_data", "reply");
  QCOMPARE(int(replies.size()), 2);
  QCOMPARE(replies[0].toObject()["result"].toString(), QString("unknown_message"));
  QCOMPARE(replies[1].toObject()["result"].toString(), QString("unknown_channel"));
}

void LslSteps1617Test::answeredOnce()
{
  const QString msg = "aaaaaaaa-0000-4000-8000-000000000002";
  const QJsonObject a = runSource(R"lsl(default {
    state_entry() { llOpenRemoteDataChannel(); }
    remote_data(integer type, key channel, key message, string sender, integer idata, string sdata) {
      if (type == REMOTE_DATA_REQUEST) { llRemoteDataReply(channel, message, "a", 1); llRemoteDataReply(channel, message, "b", 2); }
    }
  })lsl", QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"id", msg}}});
  const QJsonArray replies = ofType(a, "remote_data", "reply");
  QCOMPARE(int(replies.size()), 2);
  QCOMPARE(replies[0].toObject()["result"].toString(), QString("sent"));
  QCOMPARE(replies[1].toObject()["result"].toString(), QString("unknown_message"));
}

void LslSteps1617Test::closeChannel()
{
  const QJsonObject a = runSource(R"lsl(default {
    state_entry() { llOpenRemoteDataChannel(); }
    remote_data(integer type, key channel, key message, string sender, integer idata, string sdata) {
      llOwnerSay("event " + (string)type);
      if (type == REMOTE_DATA_CHANNEL) { llCloseRemoteDataChannel((key)"other"); llCloseRemoteDataChannel(channel); }
    }
  })lsl", QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"text", "late"}}});
  // the request after the close finds no channel
  QCOMPARE(said(a), QStringList{"event 1"});
  const QJsonArray close = ofType(a, "remote_data", "close");
  QCOMPARE(int(close.size()), 2);
  QCOMPARE(close[0].toObject()["result"].toString(), QString("not_found"));
  QCOMPARE(close[1].toObject()["result"].toString(), QString("closed"));
  QVERIFY(ofType(a, "input")[0].toObject().contains("error"));
}

void LslSteps1617Test::sendAndAnswer()
{
  const QString src = R"lsl(default {
    state_entry() { key m = llSendRemoteData((key)"remote-chan", "198.51.100.7", 7, "hello"); llOwnerSay("sent " + (string)m); }
    remote_data(integer type, key channel, key message, string sender, integer idata, string sdata) {
      llOwnerSay((string)type + "|" + (string)channel + "|" + (string)message + "|" + sender + "|" + (string)idata + "|" + sdata);
    }
  })lsl";
  const QJsonObject a = runSource(src, QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"kind", "reply"}, {"idata", 9}, {"text", "world"}}});
  QCOMPARE(said(a), (QStringList{QString("sent %1").arg(CH1), QString("3|remote-chan|%1|192.0.2.10|9|world").arg(CH1)}));
  const QJsonArray send = ofType(a, "remote_data", "send");
  QCOMPARE(int(send.size()), 1);
  const QJsonObject s = send[0].toObject();
  QCOMPARE(s["channel"].toString(), QString("remote-chan"));
  QCOMPARE(s["dest"].toString(), QString("198.51.100.7"));
  QCOMPARE(s["idata"].toInt(), 7);
  QCOMPARE(s["sdata"].toString(), QString("hello"));
  QCOMPARE(s["message"].toString(), QString(CH1));
  // answered once: a second reply finds nothing
  const QJsonObject b = runSource(src, QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"kind", "reply"}},
                                                  QJsonObject{{"at", 1.5}, {"type", "remote_data"}, {"kind", "reply"}}});
  QCOMPARE(said(b).size(), 2);
  QCOMPARE(ofType(b, "input")[1].toObject()["error"].toString(), QString("No script of the object has sent a message with llSendRemoteData."));
}

void LslSteps1617Test::sendNewestAndUnknown()
{
  const QString src = R"lsl(default {
    state_entry() { llSendRemoteData((key)"c", "d", 1, "one"); llSendRemoteData((key)"c", "d", 2, "two"); }
    remote_data(integer type, key channel, key message, string sender, integer idata, string sdata) { llOwnerSay(sdata + " " + (string)message); }
  })lsl";
  // no id: the newest message (the second, key ...002) is answered
  const QJsonObject a = runSource(src, QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"kind", "reply"}, {"text", "A"}}});
  QCOMPARE(said(a), QStringList{QString("A %1").arg(MSG2)});
  // the first message by its id
  const QJsonObject b = runSource(src, QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"kind", "reply"}, {"text", "B"}, {"id", CH1}}});
  QCOMPARE(said(b), QStringList{QString("B %1").arg(CH1)});
  // an id no script sent
  const QJsonObject c = runSource(src, QJsonArray{QJsonObject{{"at", 1.0}, {"type", "remote_data"}, {"kind", "reply"}, {"id", "nobody"}}});
  QVERIFY(said(c).isEmpty());
  QCOMPARE(ofType(c, "input")[0].toObject()["error"].toString(), QString("No script of the object sent this message with llSendRemoteData."));
}

void LslSteps1617Test::setRegion()
{
  const QJsonObject a = runSource("default { state_entry() { llOpenRemoteDataChannel(); llRemoteDataSetRegion(); } remote_data(integer t, key c, key m, string s, integer i, string d) { } }");
  const QJsonArray r = ofType(a, "remote_data", "set_region");
  QCOMPARE(int(r.size()), 1);
  QCOMPARE(r[0].toObject()["channel"].toString(), QString(CH1));
}

void LslSteps1617Test::constantsAndEvent()
{
  QCOMPARE(Builtins::constant("REMOTE_DATA_CHANNEL")->value.i, 1);
  QCOMPARE(Builtins::constant("REMOTE_DATA_REQUEST")->value.i, 2);
  QCOMPARE(Builtins::constant("REMOTE_DATA_REPLY")->value.i, 3);
  QVERIFY(Builtins::event("remote_data") != nullptr);
  QVERIFY(Parser::eventIsRaised("remote_data"));
}

void LslSteps1617Test::inputErrors_data()
{
  QTest::addColumn<QString>("input");
  QTest::addColumn<QString>("message");
  QTest::newRow("kind") << "{\"type\":\"remote_data\",\"kind\":\"say\"}" << "Input 1: \"kind\" must be \"request\" or \"reply\".";
  QTest::newRow("kind elsewhere") << "{\"type\":\"touch\",\"kind\":\"request\"}" << "Input 1: \"kind\" is only for chat and remote_data.";
  QTest::newRow("idata elsewhere") << "{\"type\":\"chat\",\"idata\":1}" << "Input 1: \"idata\" is only for remote_data.";
  QTest::newRow("idata text") << "{\"type\":\"remote_data\",\"idata\":\"x\"}" << "Input 1: \"idata\" must be a whole number.";
  QTest::newRow("channel_key elsewhere") << "{\"type\":\"touch\",\"channel_key\":\"abc\"}" << "Input 1: \"channel_key\" is only for remote_data.";
  QTest::newRow("channel_key bad") << "{\"type\":\"remote_data\",\"channel_key\":\"a b\"}" << "Input 1: \"channel_key\" must be a key: 1 to 36 letters, digits or \"-\".";
  QTest::newRow("channel_key reply") << "{\"type\":\"remote_data\",\"kind\":\"reply\",\"channel_key\":\"abc\"}" << "Input 1: \"channel_key\" is only for a remote_data request.";
  QTest::newRow("address elsewhere") << "{\"type\":\"touch\",\"address\":\"x\"}" << "Input 1: \"address\" is only for email and remote_data.";
  QTest::newRow("subject") << "{\"type\":\"remote_data\",\"subject\":\"x\"}" << "Input 1: \"subject\" is only for email.";
}

void LslSteps1617Test::inputErrors()
{
  QFETCH(QString, input);
  QFETCH(QString, message);
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { } }"},
                                                 {"inputs", QJsonArray{QJsonDocument::fromJson(input.toUtf8()).object()}}});
  QVERIFY(!a["ok"].toBool());
  QCOMPARE(a["error"].toString(), message);
}

void LslSteps1617Test::deprecatedLint()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", "default { state_entry() { llSound(\"s\", 1.0, FALSE, FALSE); llTakeCamera(llGetOwner()); llOpenRemoteDataChannel(); } }"}});
  QVERIFY(a["valid"].toBool());
  QStringList messages;
  for (const QJsonValue& w : a["warnings"].toArray()) messages << w.toObject()["message"].toString();
  QVERIFY2(messages.contains("llSound is deprecated in Second Life."), qPrintable(messages.join("; ")));
  QVERIFY2(messages.contains("llTakeCamera is deprecated in Second Life."), qPrintable(messages.join("; ")));
  QVERIFY2(messages.contains("llOpenRemoteDataChannel is deprecated in Second Life."), qPrintable(messages.join("; ")));
  QVERIFY2(!messages.join(" ").contains("not supported"), qPrintable(messages.join("; ")));
}

void LslSteps1617Test::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", SERVER}});
  QVERIFY(a["valid"].toBool());
  for (const QJsonValue& w : a["warnings"].toArray())
    QVERIFY2(w.toObject()["message"].toString().contains("deprecated"), qPrintable(w.toObject()["message"].toString()));
}

QTEST_GUILESS_MAIN(LslSteps1617Test)
#include "tst_lslsteps1617.moc"
