/*
 * PinglyMotion - tests for the money, web server and mail stubs (Script Lab, gap list 2, step 4)
 *
 * llGiveMoney, llEmail, llGetNextEmail, llRequestURL, llRequestSecureURL, llReleaseURL, llGetFreeURLs, llHTTPResponse
 * and llGetHTTPHeader, and the money, http_request and email inputs of lsl_run and lsl_event. Nothing goes on the
 * network, no L$ moves and no mail is sent: every value here is exact and worked out by hand from the rules in
 * docs/CONTROL.md ("Money, web server and mail"). A URL is http://sandbox.invalid:12046/cap/<key> (https and port 12043
 * for a secure one), where <key> is the MD5 of "<object key>/<script name>/url <n>" written as a key, n counting the
 * URLs that script was given; for the single object of a run with "source" (key c0ffee00-0000-4000-8000-000000000002,
 * script "Script") the first two are the keys in URL1 and URL2 below.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const OBJECT = "c0ffee00-0000-4000-8000-000000000002";
  const char* const GUEST = "11111111-2222-3333-4444-555555555555";
  const char* const URL1 = "http://sandbox.invalid:12046/cap/6f0cbd9d-f152-d7f5-0b12-9df9c12137c8";
  const char* const URL2 = "http://sandbox.invalid:12046/cap/6bd14677-4f8f-f9f4-f8ba-b6d9c07537ba";
  const char* const SECURE1 = "https://sandbox.invalid:12043/cap/6f0cbd9d-f152-d7f5-0b12-9df9c12137c8";

  // a web server script: says the URL it gets, and answers every request with "ok <body>" after saying what came in
  const char* const SERVER = R"lsl(default {
    state_entry() { llRequestURL(); }
    http_request(key id, string method, string body) {
      if (method == URL_REQUEST_GRANTED) { llOwnerSay("url " + body + " free " + (string)llGetFreeURLs()); return; }
      if (method == URL_REQUEST_DENIED) { llOwnerSay("denied"); return; }
      llOwnerSay(method + "|" + body + "|" + llGetHTTPHeader(id, "x-script-url") + "|" + llGetHTTPHeader(id, "x-path-info") + "|"
                 + llGetHTTPHeader(id, "x-query-string") + "|" + llGetHTTPHeader(id, "x-remote-ip") + "|"
                 + llGetHTTPHeader(id, "user-agent") + "|" + llGetHTTPHeader(id, "User-Agent") + "|" + llGetHTTPHeader(id, "x-custom"));
      llHTTPResponse(id, 200, "ok " + body);
    }
  })lsl";
}

class LslMoneyTest : public QObject
{
  Q_OBJECT

  private slots:
    void giveMoney();
    void giveMoneyWithoutPermission();
    void emailIsRecordedAndSleeps();
    void emailTooLong();
    void nextEmail();
    void mailboxFull();
    void urlAndIncomingRequest();
    void secureUrlAndRelease();
    void urlPoolAndDenied();
    void resetFreesUrls();
    void regionChangeFreesUrls();
    void queryNeedsSlash();
    void responseTimeoutAndUnknown();
    void pendingLimit();
    void bodyIsTruncated();
    void money();
    void moneyGoesToThePrimPaid();
    void severalObjects();
    void eventsInLslEvent();
    void inputErrors_data();
    void inputErrors();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& source, const QJsonArray& inputs = QJsonArray(), double seconds = 2);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type, const QString& input = QString());
};

// moc's preprocessor does not understand raw strings in macro arguments (QCOMPARE), so it skips the test bodies
#ifndef Q_MOC_RUN

QJsonObject LslMoneyTest::run(const QString& source, const QJsonArray& inputs, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request);
}

QStringList LslMoneyTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

// the entries of a type (for "input", only those of that input), without "t", as compact JSON
QStringList LslMoneyTest::entries(const QJsonObject& answer, const QString& type, const QString& input)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    QJsonObject e = v.toObject();
    if (e["type"].toString() != type || (!input.isEmpty() && e["input"].toString() != input)) continue;
    e.remove("t");
    out << QString::fromUtf8(QJsonDocument(e).toJson(QJsonDocument::Compact));
  }
  return out;
}

void LslMoneyTest::giveMoney()
{
  // the owner grants PERMISSION_DEBIT at once; llGiveMoney always answers 0
  const QJsonObject a = run(QString(R"lsl(default {
    state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_DEBIT); }
    run_time_permissions(integer p) {
      llOwnerSay((string)llGiveMoney("%1", 25));
      llOwnerSay((string)llGiveMoney("%1", 0));
      llOwnerSay((string)llGiveMoney("not a key", 5));
      llOwnerSay((string)llGiveMoney("%2", 5));
      llOwnerSay((string)llGiveMoney(llGetOwner(), 1));
    }
  })lsl").arg(GUEST, OBJECT));
  QCOMPARE(said(a), QStringList({"0", "0", "0", "0", "0"}));
  const QString note = R"("note":"Simulated: no L$ moved. The sandbox owner always has enough L$.")";
  QCOMPARE(entries(a, "give_money"), QStringList({
    QString(R"({"amount":25,%1,"result":"ok","to":"%2","type":"give_money"})").arg(note, GUEST),
    QString(R"({"amount":0,%1,"result":"INVALID_AMOUNT","to":"%2","type":"give_money"})").arg(note, GUEST),
    QString(R"({"amount":5,%1,"result":"INVALID_DESTINATION","to":"not a key","type":"give_money"})").arg(note),
    QString(R"({"amount":5,%1,"result":"LINDENDOLLAR_ENTITYDOESNOTEXIST","to":"%2","type":"give_money"})").arg(note, OBJECT),
    QString(R"({"amount":1,%1,"result":"ok","to":"%2","type":"give_money"})").arg(note, OWNER)}));
}

void LslMoneyTest::giveMoneyWithoutPermission()
{
  const QJsonObject a = run(QString(R"lsl(default { state_entry() { llOwnerSay((string)llGiveMoney("%1", 10)); } })lsl").arg(GUEST));
  QCOMPARE(said(a), QStringList({"0"}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"Script trying to pay L$ but PERMISSION_DEBIT permission not set","type":"debug"})"}));
  QCOMPARE(entries(a, "give_money"), QStringList({
    QString(R"({"amount":10,"note":"Simulated: no L$ moved. The sandbox owner always has enough L$.","result":"MISSING_PERMISSION_DEBIT","to":"%1","type":"give_money"})").arg(GUEST)}));
}

void LslMoneyTest::emailIsRecordedAndSleeps()
{
  // the header Second Life puts in front: "Object-Name: Object\n" (20 bytes), "Region: Sandbox (256000, 256000)\n" (33) and
  // "Local-Position: (128, 128, 20)\n\n" (32), 85 bytes; with "mail1@sandbox" (13), "Hi" (2) and "Hello" (5): 105 bytes
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llEmail("mail1@sandbox", "Hi", "Hello");
    llOwnerSay((string)llGetTime());
  } })lsl", QJsonArray(), 30);
  QCOMPARE(said(a), QStringList({"20.000000"}));
  QCOMPARE(entries(a, "email_sent"), QStringList({
    R"({"address":"mail1@sandbox","bytes":105,"message":"Hello","note":"Simulated: no email was sent.","result":"recorded","subject":"Hi","type":"email_sent"})"}));
  QCOMPARE(a["log"].toArray()[2].toObject()["t"].toDouble(), 0.0);   // state, event state_entry, then email_sent at 0
}

void LslMoneyTest::emailTooLong()
{
  // 13 + 2 + 85 + 4000 = 4100 bytes, more than 4096: not sent, but the script still sleeps 20 seconds
  const QJsonObject a = run(R"lsl(default { state_entry() {
    string m = "0123456789";
    integer i;
    string text;
    for (i = 0; i < 400; ++i) text += m;
    llEmail("mail1@sandbox", "Hi", text);
    llOwnerSay((string)llGetTime());
  } })lsl", QJsonArray(), 30);
  QCOMPARE(said(a), QStringList({"20.000000"}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"llEmail: the address, subject and message are 4100 bytes with the header, more than 4096, so the email was not sent.","type":"debug"})"}));
  const QStringList sent = entries(a, "email_sent");
  QCOMPARE(sent.size(), 1);
  QVERIFY2(sent[0].contains(R"("bytes":4100)") && sent[0].contains(R"("result":"too_long")"), qPrintable(sent[0]));
}

void LslMoneyTest::nextEmail()
{
  // two emails arrive at 1 and 2 seconds; at 3 the script takes "hi" (any sender, upper and lower case alike), then
  // mail from bob (any subject), then anything (none is left). The time is the UTC time it arrived: 1767225600 + 2.
  const QJsonObject a = run(R"lsl(default {
    state_entry() { llSetTimerEvent(3); }
    timer() {
      llSetTimerEvent(0);
      llGetNextEmail("", "hi");
      llGetNextEmail("bob@sandbox", "");
      llGetNextEmail("", "");
    }
    email(string time, string address, string subject, string message, integer left) {
      llOwnerSay(time + "|" + address + "|" + subject + "|" + message + "|" + (string)left);
    }
  })lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "email"}, {"address", "Bob@Sandbox"}, {"subject", "Order"}, {"body", "two apples"}},
                     QJsonObject{{"at", 2.5}, {"type", "email"}, {"address", "carol@sandbox"}, {"subject", "Hi"}, {"body", "x"}}}, 4);
  QCOMPARE(said(a), QStringList({"1767225602|carol@sandbox|Hi|x|1", "1767225601|Bob@Sandbox|Order|two apples|0"}));
  QCOMPARE(entries(a, "input", "email"), QStringList({
    R"({"address":"Bob@Sandbox","input":"email","subject":"Order","type":"input","waiting":1})",
    R"({"address":"carol@sandbox","input":"email","subject":"Hi","type":"input","waiting":2})"}));
  QCOMPARE(entries(a, "email_check"), QStringList({
    R"({"address":"","found":true,"left":1,"subject":"hi","type":"email_check"})",
    R"({"address":"bob@sandbox","found":true,"left":0,"subject":"","type":"email_check"})",
    R"({"address":"","found":false,"subject":"","type":"email_check"})"}));

  // the default sender, an empty subject and message
  const QJsonObject b = run(R"lsl(default {
    state_entry() { llSetTimerEvent(1); }
    timer() { llSetTimerEvent(0); llGetNextEmail("", ""); }
    email(string time, string address, string subject, string message, integer left) {
      llOwnerSay(time + "|" + address + "|" + subject + "|" + message + "|" + (string)left);
    }
  })lsl", QJsonArray{QJsonObject{{"at", 0.5}, {"type", "email"}}});
  QCOMPARE(said(b), QStringList({"1767225600|sender@sandbox|||0"}));
}

void LslMoneyTest::mailboxFull()
{
  QJsonArray inputs;
  for (int i = 0; i < 101; ++i) inputs.append(QJsonObject{{"at", 1}, {"type", "email"}, {"subject", QString::number(i)}});
  const QJsonObject a = run("default { state_entry() { } }", inputs);
  const QStringList e = entries(a, "input", "email");
  QCOMPARE(e.size(), 101);
  QCOMPARE(e[99], QString(R"({"address":"sender@sandbox","input":"email","subject":"99","type":"input","waiting":100})"));
  QCOMPARE(e[100], QString(R"({"address":"sender@sandbox","error":"The object's mailbox is full (100 emails); a script takes them out with llGetNextEmail.","input":"email","subject":"100","type":"input"})"));
}

void LslMoneyTest::urlAndIncomingRequest()
{
  const QJsonObject a = run(SERVER, QJsonArray{
    QJsonObject{{"at", 1}, {"type", "http_request"}, {"method", "post"}, {"url", QString(URL1) + "/items/7?color=red&size=2"}, {"body", "hello"},
                {"headers", QJsonObject{{"User-Agent", "tester/1.0"}, {"X-Custom", "yes"}, {"x-remote-ip", "10.0.0.1"}}}},
    QJsonObject{{"at", 2}, {"type", "http_request"}}}, 3);
  QCOMPARE(said(a), QStringList({
    QString("url %1 free 37").arg(URL1),
    QString("POST|hello|%1|/items/7|color=red&size=2|192.0.2.10|tester/1.0||yes").arg(URL1),
    QString("GET||%1|||192.0.2.10|||").arg(URL1)}));
  QCOMPARE(entries(a, "url_request"), QStringList({
    QString(R"({"request":"5c11ab00-0000-4000-8000-000000000001","result":"granted","secure":false,"type":"url_request","url":"%1"})").arg(URL1)}));
  QCOMPARE(entries(a, "input", "http_request"), QStringList({
    QString(R"({"body":"hello","input":"http_request","method":"POST","request":"c0de0000-0000-4000-8000-000000000001","type":"input","url":"%1/items/7?color=red&size=2"})").arg(URL1),
    QString(R"({"input":"http_request","method":"GET","request":"c0de0000-0000-4000-8000-000000000002","type":"input","url":"%1"})").arg(URL1)}));
  QCOMPARE(entries(a, "http_server_response"), QStringList({
    R"({"body":"ok hello","request":"c0de0000-0000-4000-8000-000000000001","result":"sent","status":200,"type":"http_server_response"})",
    R"({"body":"ok ","request":"c0de0000-0000-4000-8000-000000000002","result":"sent","status":200,"type":"http_server_response"})"}));
}

void LslMoneyTest::secureUrlAndRelease()
{
  const QJsonObject a = run(R"lsl(default {
    state_entry() { llRequestSecureURL(); }
    http_request(key id, string method, string body) {
      if (method != URL_REQUEST_GRANTED) { llOwnerSay("request " + method); return; }
      llOwnerSay(body + " " + (string)llGetFreeURLs());
      llReleaseURL(body);
      llReleaseURL(body);
      llOwnerSay((string)llGetFreeURLs());
    }
  })lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "http_request"}, {"url", SECURE1}}});
  QCOMPARE(said(a), QStringList({QString(SECURE1) + " 37", "38"}));
  QCOMPARE(entries(a, "url_release"), QStringList({
    QString(R"({"result":"released","type":"url_release","url":"%1"})").arg(SECURE1),
    QString(R"({"result":"not_found","type":"url_release","url":"%1"})").arg(SECURE1)}));
  QCOMPARE(entries(a, "input", "http_request"), QStringList({
    QString(R"({"error":"No script holds this URL: it was never granted, or it was released.","input":"http_request","method":"GET","status":404,"type":"input","url":"%1"})").arg(SECURE1)}));
}

void LslMoneyTest::urlPoolAndDenied()
{
  // 38 URLs in the whole world: the 39th request is denied, with an empty body
  const QJsonObject a = run(R"lsl(integer granted; integer denied; string last = "-";
  default {
    state_entry() { integer i; for (i = 0; i < 39; ++i) llRequestURL(); llSetTimerEvent(1); }
    http_request(key id, string method, string body) {
      if (method == URL_REQUEST_GRANTED) ++granted;
      if (method == URL_REQUEST_DENIED) { ++denied; last = body; }
    }
    timer() { llSetTimerEvent(0); llOwnerSay((string)granted + " " + (string)denied + " [" + last + "] " + (string)llGetFreeURLs()); }
  })lsl");
  QCOMPARE(said(a), QStringList({"38 1 [] 0"}));
  const QStringList requests = entries(a, "url_request");
  QCOMPARE(requests.size(), 39);
  QCOMPARE(requests.last(), QString(R"({"request":"5c11ab00-0000-4000-8000-000000000039","result":"denied","secure":false,"type":"url_request"})"));
}

void LslMoneyTest::resetFreesUrls()
{
  // a script reset frees the script's URL and forgets waiting requests; the next URL is a new one
  const QJsonObject a = run(R"lsl(default {
    state_entry() { llRequestURL(); llListen(0, "", NULL_KEY, ""); }
    listen(integer c, string n, key k, string m) { llResetScript(); }
    http_request(key id, string method, string body) {
      if (method == URL_REQUEST_GRANTED) llOwnerSay(body + " " + (string)llGetFreeURLs());
      else llOwnerSay("request " + (string)id);
    }
  })lsl", QJsonArray{QJsonObject{{"at", 1}, {"text", "reset"}}, QJsonObject{{"at", 2}, {"type", "http_request"}, {"url", URL1}}}, 3);
  QCOMPARE(said(a), QStringList({QString(URL1) + " 37", QString(URL2) + " 37"}));
  QCOMPARE(entries(a, "reset"), QStringList({R"({"type":"reset","urls_released":1})"}));
  QCOMPARE(entries(a, "input", "http_request"), QStringList({
    QString(R"({"error":"No script holds this URL: it was never granted, or it was released.","input":"http_request","method":"GET","status":404,"type":"input","url":"%1"})").arg(URL1)}));
}

void LslMoneyTest::regionChangeFreesUrls()
{
  const QJsonObject a = run(R"lsl(default {
    state_entry() { llRequestURL(); }
    changed(integer c) { llOwnerSay((string)c + " " + (string)llGetFreeURLs()); }
  })lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "changed"}, {"change", "inventory"}},
                     QJsonObject{{"at", 2}, {"type", "changed"}, {"change", "region_start"}},
                     QJsonObject{{"at", 3}, {"type", "http_request"}}}, 4);
  QCOMPARE(said(a), QStringList({"1 37", "1024 38"}));
  QCOMPARE(entries(a, "input", "changed"), QStringList({
    R"({"change":1,"input":"changed","name":"Owner","type":"input"})",
    R"({"change":1024,"input":"changed","name":"Owner","type":"input","urls_released":1})"}));
  QCOMPARE(entries(a, "input", "http_request"), QStringList({
    R"({"error":"No script of the object holds a URL; a script gets one with llRequestURL or llRequestSecureURL.","input":"http_request","method":"GET","status":404,"type":"input","url":""})"}));
}

void LslMoneyTest::queryNeedsSlash()
{
  const QJsonObject a = run(SERVER, QJsonArray{QJsonObject{{"at", 1}, {"type", "http_request"}, {"url", QString(URL1) + "?a=1"}},
                                               QJsonObject{{"at", 2}, {"type", "http_request"}, {"url", QString(URL1) + "/?a=1"}}});
  QCOMPARE(said(a), QStringList({QString("url %1 free 37").arg(URL1), QString("GET||%1|/|a=1|192.0.2.10|||").arg(URL1)}));
  QCOMPARE(entries(a, "input", "http_request").first(), QString(
    R"({"error":"A query needs a slash after the URL (%1/?a=1), as in Second Life.","input":"http_request","method":"GET","status":500,"type":"input","url":"%1?a=1"})").arg(URL1));
}

void LslMoneyTest::responseTimeoutAndUnknown()
{
  // the first request is answered 25 s after it came (still in time), the second 26 s after (too late); headers are
  // readable up to 30 s; an unknown key and a second answer to the same request are unknown_request
  const QJsonObject a = run(R"lsl(key first; key second; integer ticks; integer got;
  default {
    state_entry() { llRequestURL(); }
    http_request(key id, string method, string body) {
      if (method == URL_REQUEST_GRANTED) return;
      if (++got == 1) { first = id; llSetTimerEvent(25); }
      else second = id;
    }
    timer() {
      ++ticks;
      if (ticks == 1) {
        llHTTPResponse(first, 200, "a");
        llHTTPResponse(first, 200, "again");
        llHTTPResponse(NULL_KEY, 404, "x");
        llOwnerSay("[" + llGetHTTPHeader(second, "x-path-info") + "]");
        llSetTimerEvent(1);
      } else if (ticks == 2) {
        llHTTPResponse(second, 200, "b");
        llSetTimerEvent(0);
      }
    }
  })lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "http_request"}, {"url", QString(URL1) + "/one"}},
                     QJsonObject{{"at", 1}, {"type", "http_request"}, {"url", QString(URL1) + "/two"}}}, 30);
  QCOMPARE(said(a), QStringList({"[/two]"}));
  QCOMPARE(entries(a, "http_server_response"), QStringList({
    R"({"body":"a","request":"c0de0000-0000-4000-8000-000000000001","result":"sent","status":200,"type":"http_server_response"})",
    R"({"body":"again","request":"c0de0000-0000-4000-8000-000000000001","result":"unknown_request","status":200,"type":"http_server_response"})",
    R"({"body":"x","request":"00000000-0000-0000-0000-000000000000","result":"unknown_request","status":404,"type":"http_server_response"})",
    R"({"body":"b","request":"c0de0000-0000-4000-8000-000000000002","result":"timed_out","status":200,"type":"http_server_response"})"}));

  // headers are gone 30 s after the request came
  const QJsonObject b = run(R"lsl(key req;
  default {
    state_entry() { llRequestURL(); }
    http_request(key id, string method, string body) { if (method != URL_REQUEST_GRANTED) { req = id; llSetTimerEvent(30); } }
    timer() { llOwnerSay("[" + llGetHTTPHeader(req, "x-script-url") + "]"); llSetTimerEvent(0.5); }
  })lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "http_request"}}}, 31.6);
  QCOMPARE(said(b), QStringList({QString("[%1]").arg(URL1), "[]"}));
}

void LslMoneyTest::pendingLimit()
{
  // a script that never answers: 64 requests wait, the 65th gets 503 and no event
  QJsonArray inputs;
  for (int i = 0; i < 65; ++i) inputs.append(QJsonObject{{"at", 1}, {"type", "http_request"}});
  const QJsonObject a = run(R"lsl(integer n;
  default {
    state_entry() { llRequestURL(); llSetTimerEvent(2); }
    http_request(key id, string method, string body) { if (method == "GET") ++n; }
    timer() { llSetTimerEvent(0); llOwnerSay((string)n); }
  })lsl", inputs, 3);
  QCOMPARE(said(a), QStringList({"64"}));
  const QStringList e = entries(a, "input", "http_request");
  QCOMPARE(e.size(), 65);
  QCOMPARE(e[63], QString(R"({"input":"http_request","method":"GET","request":"c0de0000-0000-4000-8000-000000000064","type":"input","url":"%1"})").arg(URL1));
  QCOMPARE(e[64], QString(R"({"error":"The script already has 64 requests waiting for llHTTPResponse.","input":"http_request","method":"GET","status":503,"type":"input","url":"%1"})").arg(URL1));
}

void LslMoneyTest::bodyIsTruncated()
{
  // the body is cut to 2048 bytes: 2100 "a" give 2048 characters; 1100 "é" (2 bytes each) give 1024
  const QString script = R"lsl(default {
    state_entry() { llRequestURL(); }
    http_request(key id, string method, string body) { if (method == "PUT") llOwnerSay((string)llStringLength(body)); }
  })lsl";
  const QJsonObject a = run(script, QJsonArray{QJsonObject{{"at", 1}, {"type", "http_request"}, {"method", "PUT"}, {"body", QString(2100, QChar('a'))}},
                                               QJsonObject{{"at", 2}, {"type", "http_request"}, {"method", "PUT"}, {"body", QString(1100, QChar(0xE9))}}}, 3);
  QCOMPARE(said(a), QStringList({"2048", "1024"}));
  const QStringList e = entries(a, "input", "http_request");
  QCOMPARE(e.size(), 2);
  QVERIFY(e[0].contains(R"("body_truncated_at":2048)"));
  QVERIFY(e[1].contains(R"("body_truncated_at":2048)"));
}

void LslMoneyTest::money()
{
  const QJsonObject a = run(R"lsl(default { money(key id, integer amount) { llOwnerSay((string)amount + " from " + (string)id); } })lsl",
                            QJsonArray{QJsonObject{{"at", 1}, {"type", "money"}, {"amount", 50}},
                                       QJsonObject{{"at", 2}, {"type", "money"}, {"amount", 5}, {"id", GUEST}, {"name", "Guest"}}}, 3);
  QCOMPARE(said(a), QStringList({QString("50 from %1").arg(OWNER), QString("5 from %1").arg(GUEST)}));
  QCOMPARE(entries(a, "input", "money"), QStringList({
    QString(R"({"amount":50,"id":"%1","input":"money","name":"Owner","note":"Simulated: no L$ moved.","type":"input"})").arg(OWNER),
    QString(R"({"amount":5,"id":"%1","input":"money","name":"Guest","note":"Simulated: no L$ moved.","type":"input"})").arg(GUEST)}));

  // an object without a money event cannot be paid
  const QJsonObject b = run("default { state_entry() { } }", QJsonArray{QJsonObject{{"at", 1}, {"type", "money"}, {"amount", 1}}});
  QCOMPARE(entries(b, "input", "money"), QStringList({
    QString(R"({"amount":1,"error":"No script of the object has a money event, so it cannot be paid.","id":"%1","input":"money","name":"Owner","type":"input"})").arg(OWNER)}));
}

void LslMoneyTest::moneyGoesToThePrimPaid()
{
  // prim 2 has a money event and keeps its payments; prim 3 has none, so its payment goes to the root
  auto script = [](const QString& name, int link)
  {
    return QJsonObject{{"name", name}, {"link", link},
                       {"source", QString("default { money(key id, integer amount) { llOwnerSay(\"%1 \" + (string)amount); } }").arg(name)}};
  };
  const QJsonObject box{{"name", "box"}, {"prims", 3},
                        {"scripts", QJsonArray{script("root", 1), script("second", 2),
                                               QJsonObject{{"name", "third"}, {"link", 3}, {"source", "default { state_entry() { } }"}}}}};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{box}}, {"seconds", 3},
                                                 {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "money"}, {"amount", 7}, {"link", 2}},
                                                                       QJsonObject{{"at", 2}, {"type", "money"}, {"amount", 9}, {"link", 3}}}}});
  QCOMPARE(said(a), QStringList({"second 7", "root 9"}));
  QCOMPARE(Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{box}},
                                    {"inputs", QJsonArray{QJsonObject{{"type", "money"}, {"amount", 1}, {"link", 4}}}}})["error"].toString(),
           QString("Input 1: the object \"box\" has 3 prim(s); there is no link 4."));
}

void LslMoneyTest::severalObjects()
{
  // the URLs are counted in the whole world; a request without url goes to the newest URL of the object named; llDie
  // frees the object's URLs
  auto object = [](const QString& name, const QString& source) { return QJsonObject{{"name", name}, {"source", source}}; };
  const QString server = R"lsl(default {
    state_entry() { llRequestURL(); llListen(0, "", NULL_KEY, "die"); }
    listen(integer c, string n, key k, string m) { llDie(); }
    http_request(key id, string method, string body) { if (method == "GET") llOwnerSay("got " + llGetHTTPHeader(id, "x-path-info")); }
  })lsl";
  const QString counter = R"lsl(default {
    state_entry() { llSetTimerEvent(0.5); }
    timer() { llOwnerSay("free " + (string)llGetFreeURLs()); llSetTimerEvent(2); }
  })lsl";
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 3},
    {"objects", QJsonArray{object("A", server), object("B", server), object("C", counter)}},
    {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "http_request"}, {"object", "B"}},
                          QJsonObject{{"at", 1.5}, {"text", "die"}},
                          QJsonObject{{"at", 2}, {"type", "http_request"}, {"object", "A"}}}}});
  QStringList got;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") got << v.toObject()["object"].toString() + " " + v.toObject()["text"].toString();
  QCOMPARE(got, QStringList({"C free 36", "B got ", "C free 38"}));
  // B's URL is made from B's key (c0ffee00-...-000000000003) and its script "Script"
  const QStringList e = entries(a, "input", "http_request");
  QCOMPARE(e.size(), 2);
  QCOMPARE(e[0], QString(R"({"input":"http_request","method":"GET","object":"B","request":"c0de0000-0000-4000-8000-000000000001","type":"input","url":"http://sandbox.invalid:12046/cap/d56ed36e-0f46-08bb-90ca-62db1f53770e"})"));
  QCOMPARE(e[1], QString(R"({"error":"No script of the object holds a URL; a script gets one with llRequestURL or llRequestSecureURL.","input":"http_request","method":"GET","status":404,"type":"input","url":""})"));
}

void LslMoneyTest::eventsInLslEvent()
{
  RlvState rlv;
  Session session;
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"source", QString(SERVER).replace(
    "state_entry() { llRequestURL(); }",
    "state_entry() { llRequestURL(); }\n    money(key id, integer amount) { llOwnerSay(\"paid \" + (string)amount); }\n"
    "    timer() { llSetTimerEvent(0); llGetNextEmail(\"\", \"\"); }\n"
    "    email(string t, string a, string s, string m, integer n) { llOwnerSay(\"mail \" + s + \" \" + m); }")}}, &rlv, &session);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  QJsonObject e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "money"}, {"amount", 12}}, &session);
  QCOMPARE(said(e), QStringList({"paid 12"}));
  e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "http_request"}, {"method", "DELETE"}, {"url", QString(URL1) + "/x"}}, &session);
  QCOMPARE(said(e), QStringList({QString("DELETE||%1|/x||192.0.2.10|||").arg(URL1)}));
  QCOMPARE(entries(e, "http_server_response"), QStringList({
    R"({"body":"ok ","request":"c0de0000-0000-4000-8000-000000000001","result":"sent","status":200,"type":"http_server_response"})"}));
  e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "email"}, {"subject", "S"}, {"body", "M"}}, &session);
  QCOMPARE(entries(e, "input", "email"), QStringList({R"({"address":"sender@sandbox","input":"email","subject":"S","type":"input","waiting":1})"}));
  e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "chat"}, {"text", "x"}}, &session);
  QCOMPARE(said(e), QStringList());
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "money"}}, &session)["error"].toString(),
           QString("A money input needs \"amount\": the L$ paid, a whole number from 1 to 2147483647."));
}

void LslMoneyTest::inputErrors_data()
{
  QTest::addColumn<QJsonObject>("input");
  QTest::addColumn<QString>("error");
  QTest::newRow("no amount") << QJsonObject{{"type", "money"}} << "Input 1: a money input needs \"amount\": the L$ paid, a whole number from 1 to 2147483647.";
  QTest::newRow("zero amount") << QJsonObject{{"type", "money"}, {"amount", 0}} << "Input 1: a money input needs \"amount\": the L$ paid, a whole number from 1 to 2147483647.";
  QTest::newRow("half amount") << QJsonObject{{"type", "money"}, {"amount", 1.5}} << "Input 1: a money input needs \"amount\": the L$ paid, a whole number from 1 to 2147483647.";
  QTest::newRow("amount on chat") << QJsonObject{{"amount", 3}} << "Input 1: \"amount\" is only for money.";
  QTest::newRow("link on email") << QJsonObject{{"type", "email"}, {"link", 1}}
                                 << "Input 1: \"link\" is only for touch (the prim touched), sit (the prim sat on), money (the prim paid) and collision (the prim hit).";
  QTest::newRow("url on touch") << QJsonObject{{"type", "touch"}, {"url", "x"}} << "Input 1: \"url\" is only for http_request.";
  QTest::newRow("headers on money") << QJsonObject{{"type", "money"}, {"amount", 1}, {"headers", QJsonObject()}} << "Input 1: \"headers\" is only for http_request.";
  QTest::newRow("empty url") << QJsonObject{{"type", "http_request"}, {"url", ""}}
                             << "Input 1: \"url\" must be text of 1 to 2048 characters: a URL a script got, with any path and query (leave it out for the object's newest URL).";
  QTest::newRow("url not text") << QJsonObject{{"type", "http_request"}, {"url", 5}} << "Input 1: \"url\" must be text.";
  QTest::newRow("bad method") << QJsonObject{{"type", "http_request"}, {"method", "PATCH"}} << "Input 1: \"method\" must be GET, POST, PUT or DELETE (default GET).";
  QTest::newRow("body on GET") << QJsonObject{{"type", "http_request"}, {"body", "x"}} << "Input 1: \"body\" is only for POST and PUT requests, as in Second Life.";
  QTest::newRow("bad header name") << QJsonObject{{"type", "http_request"}, {"headers", QJsonObject{{"a b", "x"}}}}
                                   << "Input 1: \"headers\" must be an object of at most 32 headers, each name (letters, digits and \"-\", 1 to 64) with a text value of at most 1024 characters.";
  QTest::newRow("header not text") << QJsonObject{{"type", "http_request"}, {"headers", QJsonObject{{"a", 1}}}}
                                   << "Input 1: \"headers\" must be an object of at most 32 headers, each name (letters, digits and \"-\", 1 to 64) with a text value of at most 1024 characters.";
  QTest::newRow("address on chat") << QJsonObject{{"address", "a@b.c"}} << "Input 1: \"address\" is only for email.";
  QTest::newRow("body on chat") << QJsonObject{{"body", "x"}} << "Input 1: \"body\" is only for http_request and email.";
  QTest::newRow("long subject") << QJsonObject{{"type", "email"}, {"subject", QString(256, 's')}} << "Input 1: \"subject\" must be text of at most 255 characters.";
  QTest::newRow("unknown type") << QJsonObject{{"type", "pay"}}
                                << "Input 1: \"type\" must be \"chat\", \"touch\", \"dialog\", \"attach\", \"detach\", \"changed\", \"sit\", \"stand\", \"control\", \"money\", \"http_request\", "
                                   "\"email\", \"collision\", \"land_collision\", \"moving\", \"at_target\" or \"not_at_target\".";
}

void LslMoneyTest::inputErrors()
{
  QFETCH(QJsonObject, input);
  QFETCH(QString, error);
  const QJsonObject a = run("default { state_entry() { } }", QJsonArray{input});
  QVERIFY(!a["ok"].toBool());
  QCOMPARE(a["error"].toString(), error);
}

void LslMoneyTest::checkerHasNoWarnings()
{
  const Parser::Result r = Parser::parse(R"lsl(default {
    state_entry() {
      llGiveMoney(llGetOwner(), 1); llEmail("a@b.c", "s", "m"); llGetNextEmail("", "");
      key k = llRequestURL(); k = llRequestSecureURL(); llReleaseURL(""); integer n = llGetFreeURLs();
      llHTTPResponse(k, 200, ""); string h = llGetHTTPHeader(k, "x-path-info");
    }
    money(key id, integer amount) { }
    http_request(key id, string method, string body) { }
    email(string time, string address, string subject, string message, integer left) { }
  })lsl");
  QVERIFY(r.ok());
  QStringList warnings;
  for (const Diagnostic& d : r.warnings) warnings << d.message;
  QCOMPARE(warnings, QStringList());
  for (const char* name : {"llGiveMoney", "llEmail", "llGetNextEmail", "llRequestURL", "llRequestSecureURL", "llReleaseURL", "llGetFreeURLs",
                           "llHTTPResponse", "llGetHTTPHeader"})
    QVERIFY2(Builtins::function(name)->supported, name);
}

#endif // Q_MOC_RUN

QTEST_GUILESS_MAIN(LslMoneyTest)
#include "tst_lslmoney.moc"
