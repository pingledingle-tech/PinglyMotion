/*
 * PinglyMotion - tests for llHTTPRequest with canned replies (Script Lab step 9b-9)
 *
 * The sandbox never goes on the network: the replies come from the "http" option of lsl_run.
 * A request is checked (URL, HTTP_* parameters, throttle), logged as an http_request entry and
 * answered at once with http_response in every script of the requesting prim. Every expected
 * value is exact.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslHttpTest : public QObject
{
  Q_OBJECT

  private slots:
    void cannedReply();
    void postWithParameters();
    void noReplyGives499();
    void lateReply();
    void replyThatNeverComes();
    void matchingOrder();
    void bodyIsTruncated();
    void refusedRequests_data();
    void refusedRequests();
    void throttle();
    void throttleQuiet();
    void everyScriptInThePrim();
    void inputErrors_data();
    void inputErrors();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& body, const QJsonArray& http, double seconds = 1);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    // a script that prints every http_response as "status|metadata|body"
    static QString script(const QString& stateEntry)
    {
      return "default {\n state_entry() {\n" + stateEntry + "\n }\n"
             " http_response(key id, integer status, list meta, string body) {\n"
             "  llOwnerSay((string)status + \"|\" + llList2CSV(meta) + \"|\" + body);\n }\n}";
    }
};

QJsonObject LslHttpTest::run(const QString& body, const QJsonArray& http, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", script(body)}, {"seconds", seconds}};
  if (!http.isEmpty()) request["http"] = http;
  return Control::run(request);
}

QStringList LslHttpTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslHttpTest::entries(const QJsonObject& answer, const QString& type)
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

void LslHttpTest::cannedReply()
{
  const QJsonObject a = run(R"lsl(
    key id = llHTTPRequest("https://example.com/api?x=1", [], "");
    llOwnerSay((string)id);
  )lsl", QJsonArray{QJsonObject{{"url", "https://example.com/api?x=1"}, {"body", "{\"ok\":true}"}}});
  QCOMPARE(said(a), (QStringList{"5c11ab00-0000-4000-8000-000000000001", "200||{\"ok\":true}"}));
  const QString want1 = R"({"method":"GET","mimetype":"text/plain;charset=utf-8","request":"5c11ab00-0000-4000-8000-000000000001","result":"canned","status":200,"type":"http_request","url":"https://example.com/api?x=1"})";
  QCOMPARE(entries(a, "http_request"), QStringList{want1});
}

void LslHttpTest::postWithParameters()
{
  const QJsonObject a = run(R"lsl(
    llHTTPRequest("http://example.com/save", [HTTP_METHOD, "POST", HTTP_MIMETYPE, "application/json", HTTP_CUSTOM_HEADER, "X-Token", "abc",
                  HTTP_VERIFY_CERT, FALSE, HTTP_PRAGMA_NO_CACHE, FALSE, HTTP_USER_AGENT, "Tool/1", HTTP_ACCEPT, "application/json",
                  HTTP_EXTENDED_ERROR, FALSE], "n=1");
  )lsl", QJsonArray{QJsonObject{{"url", "http://example.com/save"}, {"method", "get"}, {"body", "wrong"}},
                    QJsonObject{{"url", "http://example.com/save"}, {"method", "POST"}, {"status", 201}, {"body", "saved"}}});
  QCOMPARE(said(a), QStringList{"201||saved"});
  const QString want2 = R"({"body":"n=1","headers":[["X-Token","abc"]],"method":"POST","mimetype":"application/json","request":"5c11ab00-0000-4000-8000-000000000001","result":"canned","status":201,"type":"http_request","url":"http://example.com/save"})";
  QCOMPARE(entries(a, "http_request"), QStringList{want2});
}

void LslHttpTest::noReplyGives499()
{
  const QJsonObject a = run(R"lsl(llHTTPRequest("https://example.com/other", [], "");)lsl",
                            QJsonArray{QJsonObject{{"url", "https://example.com/api"}}});
  QCOMPARE(said(a), QStringList{"499||"});
  const QString want3 = R"({"method":"GET","mimetype":"text/plain;charset=utf-8","request":"5c11ab00-0000-4000-8000-000000000001","result":"no_reply","status":499,"type":"http_request","url":"https://example.com/other"})";
  QCOMPARE(entries(a, "http_request"), QStringList{want3});
  // no "http" option at all: the same
  const QJsonObject none = run(R"lsl(llHTTPRequest("https://example.com/", [], "");)lsl", QJsonArray());
  QCOMPARE(said(none), QStringList{"499||"});
}

void LslHttpTest::lateReply()
{
  // job 23: the reply comes 5 simulated seconds after the request, so a script that gave up after 2 seconds sees it late
  const QString source = "default {\n state_entry() { llHTTPRequest(\"https://example.com/slow\", [], \"\"); llSetTimerEvent(2.0); }\n"
    " timer() { llSetTimerEvent(0.0); llOwnerSay(\"waited \" + (string)llRound(llGetTime())); }\n"
    " http_response(key id, integer status, list meta, string body) { llOwnerSay((string)llRound(llGetTime()) + \" \" + (string)status + \" \" + body); }\n}";
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", 10},
                      {"http", QJsonArray{QJsonObject{{"url", "https://example.com/slow"}, {"body", "late"}, {"delay", 5}}}}};
  const QJsonObject a = Control::run(request);
  QCOMPARE(said(a), (QStringList{"waited 2", "5 200 late"}));
  const QString want = R"({"delay":5,"method":"GET","mimetype":"text/plain;charset=utf-8","request":"5c11ab00-0000-4000-8000-000000000001","result":"canned","status":200,"type":"http_request","url":"https://example.com/slow"})";
  QCOMPARE(entries(a, "http_request"), QStringList{want});
  // a run that ends before the reply comes does not see it
  request["seconds"] = 4;
  QCOMPARE(said(Control::run(request)), QStringList{"waited 2"});
}

void LslHttpTest::replyThatNeverComes()
{
  // job 23: a delay of 60 seconds or more never comes: status 499 and an empty body after 60 seconds, as in Second Life
  const QString source = "default {\n state_entry() { llHTTPRequest(\"https://example.com/dead\", [], \"\"); }\n"
    " http_response(key id, integer status, list meta, string body) { llOwnerSay((string)llRound(llGetTime()) + \" \" + (string)status + \" [\" + body + \"]\"); }\n}";
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", 100},
                      {"http", QJsonArray{QJsonObject{{"url", "https://example.com/dead"}, {"body", "never"}, {"delay", 300}}}}};
  const QJsonObject a = Control::run(request);
  QCOMPARE(said(a), (QStringList{"60 499 []"}));
  const QString want = R"({"delay":60,"method":"GET","mimetype":"text/plain;charset=utf-8","request":"5c11ab00-0000-4000-8000-000000000001","result":"timed_out","status":499,"type":"http_request","url":"https://example.com/dead"})";
  QCOMPARE(entries(a, "http_request"), QStringList{want});
  request["http"] = QJsonArray{QJsonObject{{"url", "https://example.com/dead"}, {"delay", 301}}};
  QCOMPARE(Control::run(request)["error"].toString(),
           QString("HTTP reply 1: \"delay\" must be the simulated seconds until the reply comes, 0 to 300 (from 60 on it never comes and the request gets status 499 after 60 seconds)."));
}

void LslHttpTest::matchingOrder()
{
  const QJsonArray http{QJsonObject{{"url", "https://example.com/a/b"}, {"body", "exact"}},
                        QJsonObject{{"url", "https://example.com/a/*"}, {"method", "PUT"}, {"body", "put prefix"}},
                        QJsonObject{{"url", "https://example.com/a/*"}, {"body", "prefix"}},
                        QJsonObject{{"url", "*"}, {"status", 404}, {"body", "anything else"}}};
  const QJsonObject a = run(R"lsl(
    llHTTPRequest("https://example.com/a/b", [], "");
    llHTTPRequest("https://example.com/a/b", [HTTP_METHOD, "PUT"], "");
    llHTTPRequest("https://example.com/a/c", [HTTP_METHOD, "PUT"], "");
    llHTTPRequest("https://example.com/a/c", [HTTP_METHOD, "DELETE"], "");
    llHTTPRequest("https://example.com/a", [], "");
    llHTTPRequest("HTTPS://other.example/", [], "");
  )lsl", http);
  QCOMPARE(said(a), (QStringList{"200||exact", "200||exact", "200||put prefix", "200||prefix", "404||anything else", "404||anything else"}));
}

void LslHttpTest::bodyIsTruncated()
{
  // the script says the status, the metadata, the body's length and (when short) the body: llOwnerSay cuts long text
  const QString source = R"lsl(default {
    state_entry() {
      llHTTPRequest("https://example.com/big", [], "");
      llHTTPRequest("https://example.com/small", [HTTP_BODY_MAXLENGTH, 5], "");
      llHTTPRequest("https://example.com/big", [HTTP_BODY_MAXLENGTH, 16384], "");
      llHTTPRequest("https://example.com/small", [HTTP_BODY_MAXLENGTH, 6], "");
      llHTTPRequest("https://example.com/small", [HTTP_BODY_MAXLENGTH, 0], "");
    }
    http_response(key id, integer status, list meta, string body) {
      string shown = body; if (llStringLength(body) > 100) shown = "...";
      llOwnerSay((string)status + "|" + llList2CSV(meta) + "|" + (string)llStringLength(body) + "|" + shown);
    } })lsl";
  const QJsonArray http{QJsonObject{{"url", "https://example.com/big"}, {"body", QString(3000, 'a')}},
                        QJsonObject{{"url", "https://example.com/small"}, {"body", QString::fromUtf8("\xc3\xa6\xc3\xb8\xc3\xa5\xc3\xa6")}}};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", 1}, {"http", http}});
  QCOMPARE(said(a), (QStringList{"200|0, 2048|2048|...", QString::fromUtf8("200|0, 5|2|\xc3\xa6\xc3\xb8"), "200||3000|...",
                                 QString::fromUtf8("200|0, 6|3|\xc3\xa6\xc3\xb8\xc3\xa5"), "200|0, 0|0|"}));
  const QStringList log = entries(a, "http_request");
  QCOMPARE(log.size(), 5);
  QVERIFY(log[0].contains("\"truncated_at\":2048"));
  QVERIFY(log[1].contains("\"truncated_at\":5"));
  QVERIFY(!log[2].contains("truncated_at"));
}

void LslHttpTest::refusedRequests_data()
{
  QTest::addColumn<QString>("call");
  QTest::addColumn<QString>("debug");
  QTest::newRow("ftp") << R"(llHTTPRequest("ftp://example.com/", [], ""))"
                       << "llHTTPRequest: the URL must start with http:// or https://, name a host and have no spaces or control characters.";
  QTest::newRow("space") << R"(llHTTPRequest("https://example.com/a b", [], ""))"
                         << "llHTTPRequest: the URL must start with http:// or https://, name a host and have no spaces or control characters.";
  QTest::newRow("no host") << R"(llHTTPRequest("https://", [], ""))"
                           << "llHTTPRequest: the URL must start with http:// or https://, name a host and have no spaces or control characters.";
  QTest::newRow("unknown flag") << R"(llHTTPRequest("https://example.com/", [42, 1], ""))"
                                << "llHTTPRequest: parameter 1 of the list is not one of the HTTP_* flags.";
  QTest::newRow("flag as text") << R"(llHTTPRequest("https://example.com/", ["HTTP_METHOD", "GET"], ""))"
                                << "llHTTPRequest: parameter 1 of the list is not one of the HTTP_* flags.";
  QTest::newRow("missing value") << R"(llHTTPRequest("https://example.com/", [HTTP_BODY_MAXLENGTH], ""))"
                                 << "llHTTPRequest: HTTP_BODY_MAXLENGTH is missing its value.";
  QTest::newRow("missing header value") << R"(llHTTPRequest("https://example.com/", [HTTP_CUSTOM_HEADER, "X-A"], ""))"
                                        << "llHTTPRequest: HTTP_CUSTOM_HEADER is missing its value.";
  QTest::newRow("wrong type") << R"(llHTTPRequest("https://example.com/", [HTTP_METHOD, 1], ""))"
                              << "llHTTPRequest: HTTP_METHOD needs a string.";
  QTest::newRow("wrong type int") << R"(llHTTPRequest("https://example.com/", [HTTP_BODY_MAXLENGTH, "5"], ""))"
                                  << "llHTTPRequest: HTTP_BODY_MAXLENGTH needs an integer.";
  QTest::newRow("method") << R"(llHTTPRequest("https://example.com/", [HTTP_METHOD, "PATCH"], ""))"
                          << "llHTTPRequest: HTTP_METHOD must be \"GET\", \"POST\", \"PUT\" or \"DELETE\".";
  QTest::newRow("method case") << R"(llHTTPRequest("https://example.com/", [HTTP_METHOD, "get"], ""))"
                               << "llHTTPRequest: HTTP_METHOD must be \"GET\", \"POST\", \"PUT\" or \"DELETE\".";
  QTest::newRow("max length") << R"(llHTTPRequest("https://example.com/", [HTTP_BODY_MAXLENGTH, 16385], ""))"
                              << "llHTTPRequest: HTTP_BODY_MAXLENGTH must be 0 to 16384.";
  QTest::newRow("content type") << R"(llHTTPRequest("https://example.com/", [HTTP_CUSTOM_HEADER, "content-type", "a"], ""))"
                                << "llHTTPRequest: set Content-Type with HTTP_MIMETYPE, not HTTP_CUSTOM_HEADER.";
  QTest::newRow("empty header") << R"(llHTTPRequest("https://example.com/", [HTTP_CUSTOM_HEADER, "", "a"], ""))"
                                << "llHTTPRequest: HTTP_CUSTOM_HEADER needs a header name.";
  QTest::newRow("long headers") << R"(llHTTPRequest("https://example.com/", [HTTP_CUSTOM_HEADER, "X-A", "", HTTP_CUSTOM_HEADER, "X-B", h], ""))"
                                << "llHTTPRequest: the custom headers together have more than 4096 characters.";
  QTest::newRow("user agent") << R"(llHTTPRequest("https://example.com/", [HTTP_USER_AGENT, "My Tool"], ""))"
                              << "llHTTPRequest: HTTP_USER_AGENT may not have spaces.";
}

void LslHttpTest::refusedRequests()
{
  QFETCH(QString, call);
  QFETCH(QString, debug);
  // h: 4094 characters, so "X-A" and "X-B" + h are 4100 characters, more than 4096
  const QString setup = "string h = \"x\"; while (llStringLength(h) < 4094) h += \"x\";\n";
  const QJsonObject a = run(setup + "key id = " + call + "; llOwnerSay((string)id);", QJsonArray{QJsonObject{{"url", "*"}}});
  QCOMPARE(said(a), QStringList{"00000000-0000-0000-0000-000000000000"});
  QCOMPARE(entries(a, "debug"), QStringList{
    QString::fromUtf8(QJsonDocument(QJsonObject{{"channel", 2147483647}, {"text", debug}, {"type", "debug"}}).toJson(QJsonDocument::Compact))});
  QCOMPARE(entries(a, "http_request"), QStringList());
}

void LslHttpTest::throttle()
{
  // 26 requests at once: the 26th is refused; 20 seconds after the first ones, requests go through again
  const QJsonObject a = run(R"lsl(
    integer i; integer ok;
    for (i = 0; i < 26; ++i) if (llHTTPRequest("https://example.com/", [], "") != NULL_KEY) ++ok;
    llOwnerSay("ok " + (string)ok);
    llSetTimerEvent(20.0);
  }
  timer() {
    llSetTimerEvent(0.0);
    llOwnerSay("later " + (string)(llHTTPRequest("https://example.com/", [], "") != NULL_KEY));
  )lsl", QJsonArray{QJsonObject{{"url", "*"}, {"status", 204}}}, 25);
  QStringList lines = said(a);
  QCOMPARE(lines.size(), 25 + 1 + 1 + 1);
  QCOMPARE(lines.count("204||"), 26);
  QCOMPARE(lines.indexOf("ok 25"), 0);
  QCOMPARE(lines.indexOf("later 1"), 26);
  const QString want4 = R"({"channel":2147483647,"text":"llHTTPRequest: too many requests (more than 25 in 20 seconds from this object), so this one was refused.","type":"debug"})";
  QCOMPARE(entries(a, "debug"), QStringList{want4});
  const QString throttled = R"({"method":"GET","mimetype":"text/plain;charset=utf-8","result":"throttled","type":"http_request","url":"https://example.com/"})";
  QCOMPARE(entries(a, "http_request")[25], throttled);
}

void LslHttpTest::throttleQuiet()
{
  const QJsonObject a = run(R"lsl(
    integer i; integer ok;
    for (i = 0; i < 30; ++i) if (llHTTPRequest("https://example.com/", [HTTP_VERBOSE_THROTTLE, FALSE], "") != NULL_KEY) ++ok;
    llOwnerSay("ok " + (string)ok);
  )lsl", QJsonArray());
  QCOMPARE(said(a).first(), QString("ok 25"));
  QCOMPARE(entries(a, "debug"), QStringList());
  QCOMPARE(entries(a, "http_request").size(), 30);
}

void LslHttpTest::everyScriptInThePrim()
{
  const QString asker = R"lsl(default { state_entry() { llHTTPRequest("https://example.com/", [], ""); }
    http_response(key id, integer status, list meta, string body) { llOwnerSay("asker " + body); } })lsl";
  const QString listener = R"lsl(default { http_response(key id, integer status, list meta, string body) { llOwnerSay(llGetScriptName() + " " + body); } })lsl";
  const QJsonObject a = Control::run(QJsonObject{
    {"command", "lsl_run"}, {"seconds", 1},
    {"http", QJsonArray{QJsonObject{{"url", "*"}, {"body", "hi"}}}},
    {"objects", QJsonArray{QJsonObject{{"name", "box"}, {"prims", 2},
                                       {"scripts", QJsonArray{QJsonObject{{"name", "asker"}, {"source", asker}},
                                                              QJsonObject{{"name", "same prim"}, {"source", listener}},
                                                              QJsonObject{{"name", "child"}, {"source", listener}, {"link", 2}}}}},
                           QJsonObject{{"name", "other"}, {"source", listener}}}}});
  QCOMPARE(said(a), (QStringList{"asker hi", "same prim hi"}));
}

void LslHttpTest::inputErrors_data()
{
  QTest::addColumn<QJsonValue>("http");
  QTest::addColumn<QString>("error");
  QTest::newRow("not a list") << QJsonValue("x") << "\"http\" must be a list of canned replies, each {\"url\", \"method\", \"status\", \"body\", \"delay\"}.";
  QJsonArray many;
  for (int i = 0; i < 65; ++i) many.append(QJsonObject{{"url", "*"}});
  QTest::newRow("too many") << QJsonValue(many) << "\"http\" holds at most 64 canned replies.";
  QTest::newRow("not an object") << QJsonValue(QJsonArray{1}) << "HTTP reply 1: must be an object with a url.";
  QTest::newRow("unknown field") << QJsonValue(QJsonArray{QJsonObject{{"url", "*"}, {"headers", 1}}})
                                 << "HTTP reply 1: unknown field \"headers\". Fields: url, method, status, body, delay.";
  QTest::newRow("no url") << QJsonValue(QJsonArray{QJsonObject{{"body", "x"}}})
                          << "HTTP reply 1: \"url\" must be text of 1 to 2048 characters: the exact URL, or a prefix ending in * (\"*\" alone answers every URL).";
  QTest::newRow("method") << QJsonValue(QJsonArray{QJsonObject{{"url", "*"}, {"method", "PATCH"}}})
                          << "HTTP reply 1: \"method\" must be GET, POST, PUT or DELETE (omit it to answer every method).";
  QTest::newRow("status") << QJsonValue(QJsonArray{QJsonObject{{"url", "*"}}, QJsonObject{{"url", "*"}, {"status", 600}}})
                          << "HTTP reply 2: \"status\" must be a whole number from 100 to 599 (default 200).";
  QTest::newRow("body") << QJsonValue(QJsonArray{QJsonObject{{"url", "*"}, {"body", 5}}})
                        << "HTTP reply 1: \"body\" must be text of at most 65536 characters.";
}

void LslHttpTest::inputErrors()
{
  QFETCH(QJsonValue, http);
  QFETCH(QString, error);
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", script("")}, {"http", http}});
  QCOMPARE(a["ok"].toBool(), false);
  QCOMPARE(a["error"].toString(), error);
}

void LslHttpTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", script(R"lsl(llHTTPRequest("https://example.com/", [], "");)lsl")}});
  QCOMPARE(a["valid"].toBool(), true);
  QCOMPARE(a["warnings"].toArray().size(), 0);
  QVERIFY(Builtins::function("llHTTPRequest")->supported);
}

QTEST_GUILESS_MAIN(LslHttpTest)
#include "tst_lslhttp.moc"
