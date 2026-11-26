/*
 * PinglyMotion - tests for the answers from outside that can be slow, refused or lost (Script Lab, Anafin HUD test,
 * gaps 4 and 6)
 *
 * The lsl_run options "permissions" (the owner's answer to llRequestPermissions: grant, deny or none, with a delay),
 * "replies" (an RLV viewer's replies and dataserver replies: ok or none, with a delay; one answer for every reply or a
 * list in call order) and the avatar fields "online", "login" and "logout" (an avatar that is offline, or logs in or out
 * at a simulated time). Every value here is exact and worked out by hand from the rules in docs/CONTROL.md.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const BOB = "b0b00000-0000-4000-8000-000000000001";

  // asks the owner for PERMISSION_TRIGGER_ANIMATION (16) at the start and says what it has after one second and in
  // run_time_permissions, each line with the simulated time
  const char* const ASKER = R"lsl(string t() { return llGetSubString((string)llGetTime(), 0, 3); }
    default {
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION); llSetTimerEvent(1.0); }
      timer() { llSetTimerEvent(0.0); llOwnerSay(t() + " timer " + (string)llGetPermissions()); }
      run_time_permissions(integer p) {
        llOwnerSay(t() + " answer " + (string)p + " " + (string)llGetPermissions() + " " + (string)(llGetPermissionsKey() == llGetOwner()));
      }
    })lsl";

  // asks the RLV viewer for its version on channel 4711 and gives up after 5 seconds
  const char* const VERSION = R"lsl(string t() { return llGetSubString((string)llGetTime(), 0, 3); }
    integer handle;
    default {
      state_entry() { handle = llListen(4711, "", llGetOwner(), ""); llOwnerSay("@version=4711"); llSetTimerEvent(5.0); }
      listen(integer c, string n, key id, string m) { llOwnerSay(t() + " heard"); }
      timer() { llSetTimerEvent(0.0); llListenRemove(handle); llOwnerSay(t() + " gave up"); }
      touch_start(integer n) { handle = llListen(4711, "", llGetOwner(), ""); llOwnerSay("@version=4711"); }
    })lsl";
}

class LslAnswersTest : public QObject
{
  Q_OBJECT

  private slots:
    void permissionsDefaultIsUnchanged();
    void permissionDenied();
    void permissionDeniedLater();
    void permissionGrantedLater();
    void permissionNotAnswered();
    void permissionsInCallOrder();
    void resetForgetsAWaitingAnswer();
    void permissionsInLslEvent();
    void permissionsErrors_data();
    void permissionsErrors();
    void offlineAvatar();
    void avatarLogsInAndOut();
    void offlineOwnerIsNotAsked();
    void avatarErrors_data();
    void avatarErrors();
    void rlvReplyLate();
    void rlvReplyLost();
    void rlvReplyAfterTheListenIsGone();
    void rlvRepliesInCallOrder();
    void dataserverReplies();
    void dataserverReplyForEveryKind();
    void repliesErrors_data();
    void repliesErrors();

  private:
    static QJsonObject run(const QString& source, const QJsonObject& options, double seconds = 10);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
};

// moc's preprocessor does not understand raw strings in macro arguments (QCOMPARE), so it skips the test bodies
#ifndef Q_MOC_RUN

QJsonObject LslAnswersTest::run(const QString& source, const QJsonObject& options, double seconds)
{
  QJsonObject request = options;
  request["command"] = "lsl_run";
  request["source"] = source;
  request["seconds"] = seconds;
  return Control::run(request);
}

QStringList LslAnswersTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

// the entries of a type, with "t", as compact JSON
QStringList LslAnswersTest::entries(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == type) out << QString::fromUtf8(QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact));
  return out;
}

void LslAnswersTest::permissionsDefaultIsUnchanged()
{
  // without the option (and with an empty list) the owner grants at once, as before
  for (const QJsonObject& options : {QJsonObject(), QJsonObject{{"permissions", QJsonArray()}}})
  {
    const QJsonObject a = run(ASKER, options, 2);
    QCOMPARE(said(a), QStringList({"0.00 answer 16 16 1", "1.00 timer 16"}));
    QCOMPARE(entries(a, "permissions"), QStringList({
      R"({"granted":true,"mask":16,"note":"The sandbox grants the owner's permissions at once.","t":0,"type":"permissions"})"}));
  }
}

void LslAnswersTest::permissionDenied()
{
  const QJsonObject a = run(ASKER, QJsonObject{{"permissions", QJsonArray{QJsonObject{{"result", "deny"}}}}}, 2);
  // run_time_permissions comes at once with 0; llGetPermissionsKey is the owner who denied
  QCOMPARE(said(a), QStringList({"0.00 answer 0 0 1", "1.00 timer 0"}));
  QCOMPARE(entries(a, "permissions"), QStringList({
    R"({"answer":"deny","granted":false,"mask":16,"note":"The owner denies (the permissions option); run_time_permissions comes with 0.","t":0,"type":"permissions"})"}));
}

void LslAnswersTest::permissionDeniedLater()
{
  const QJsonObject a = run(ASKER, QJsonObject{{"permissions", QJsonArray{QJsonObject{{"result", "deny"}, {"delay", 2.5}}}}}, 4);
  QCOMPARE(said(a), QStringList({"1.00 timer 0", "2.50 answer 0 0 1"}));
  QCOMPARE(entries(a, "permissions"), QStringList({
    R"({"answer":"deny","delay":2.5,"granted":false,"mask":16,"note":"The owner denies (the permissions option); run_time_permissions comes with 0.","t":0,"type":"permissions"})"}));
}

void LslAnswersTest::permissionGrantedLater()
{
  // the permissions change when the owner answers, not before: the timer at 1 s still sees 0
  const QJsonObject a = run(ASKER, QJsonObject{{"permissions", QJsonArray{QJsonObject{{"result", "grant"}, {"delay", 3}}}}}, 4);
  QCOMPARE(said(a), QStringList({"1.00 timer 0", "3.00 answer 16 16 1"}));
  QCOMPARE(entries(a, "permissions"), QStringList({
    R"({"answer":"grant","delay":3,"granted":true,"mask":16,"note":"The owner grants later (the permissions option); run_time_permissions comes then.","t":0,"type":"permissions"})"}));
  // "grant" is the default result
  const QJsonObject b = run(ASKER, QJsonObject{{"permissions", QJsonArray{QJsonObject{{"delay", 3}}}}}, 4);
  QCOMPARE(said(b), QStringList({"1.00 timer 0", "3.00 answer 16 16 1"}));
}

void LslAnswersTest::permissionNotAnswered()
{
  const QJsonObject a = run(ASKER, QJsonObject{{"permissions", QJsonArray{QJsonObject{{"result", "none"}}}}}, 10);
  QCOMPARE(said(a), QStringList({"1.00 timer 0"}));
  QCOMPARE(entries(a, "permissions"), QStringList({
    R"({"answer":"none","granted":false,"mask":16,"note":"The owner does not answer (the permissions option); no run_time_permissions comes.","t":0,"type":"permissions"})"}));
}

void LslAnswersTest::permissionsInCallOrder()
{
  // each touch asks again: deny, no answer, grant after 1 s, then (after the list) grant at once
  const QString source = R"lsl(string t() { return llGetSubString((string)llGetTime(), 0, 3); }
    default {
      touch_start(integer n) { llRequestPermissions(llGetOwner(), PERMISSION_DEBIT); }
      run_time_permissions(integer p) { llOwnerSay(t() + " " + (string)p); }
    })lsl";
  QJsonArray touches;
  for (int at : {1, 2, 3, 6}) touches.append(QJsonObject{{"at", at}, {"type", "touch"}});
  const QJsonObject a = run(source, QJsonObject{{"inputs", touches}, {"permissions", QJsonArray{QJsonObject{{"result", "deny"}}, QJsonObject{{"result", "none"}},
                                                                                                 QJsonObject{{"delay", 1}}}}}, 8);
  QCOMPARE(said(a), QStringList({"1.00 0", "4.00 2", "6.00 2"}));
}

void LslAnswersTest::resetForgetsAWaitingAnswer()
{
  // the owner answers after 3 s, but the script resets at 1 s: the answer to the old request never comes; the request
  // after the reset takes the next answer (after the list: grant at once)
  const QString source = R"lsl(string t() { return llGetSubString((string)llGetTime(), 0, 3); }
    default {
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_DEBIT); }
      touch_start(integer n) { llResetScript(); }
      run_time_permissions(integer p) { llOwnerSay("answer " + (string)p); }
    })lsl";
  const QJsonObject a = run(source, QJsonObject{{"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}}},
                                                {"permissions", QJsonArray{QJsonObject{{"delay", 3}}}}}, 5);
  QCOMPARE(said(a), QStringList({"answer 2"}));
  QCOMPARE(entries(a, "permissions").size(), 2);
}

void LslAnswersTest::permissionsInLslEvent()
{
  // the list belongs to the world: a request in lsl_event takes the next answer
  RlvState rlv;
  Session session;
  const QString source = R"lsl(default {
      touch_start(integer n) { llRequestPermissions(llGetOwner(), PERMISSION_DEBIT); }
      run_time_permissions(integer p) { llOwnerSay((string)p); }
    })lsl";
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"source", source},
      {"permissions", QJsonArray{QJsonObject{{"result", "deny"}}, QJsonObject{{"result", "grant"}, {"delay", 0.5}}}},
      {"inputs", QJsonArray{QJsonObject{{"at", 0.5}, {"type", "touch"}}}}}, &rlv, &session);
  QCOMPARE(said(a), QStringList({"0"}));
  const QJsonObject b = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"seconds", 1}}, &session);
  QCOMPARE(said(b), QStringList({"2"}));
}

void LslAnswersTest::permissionsErrors_data()
{
  QTest::addColumn<QJsonValue>("permissions");
  QTest::addColumn<QString>("error");
  QTest::newRow("not a list") << QJsonValue(QJsonObject{{"result", "deny"}})
                              << "\"permissions\" must be a list of the owner's answers to llRequestPermissions, each {\"result\", \"delay\"}.";
  QTest::newRow("not an object") << QJsonValue(QJsonArray{"deny"}) << "Permission answer 1: must be an object with result and/or delay.";
  QTest::newRow("unknown field") << QJsonValue(QJsonArray{QJsonObject{{"mask", 16}}}) << "Permission answer 1: unknown field \"mask\". Fields: result, delay.";
  QTest::newRow("unknown result") << QJsonValue(QJsonArray{QJsonObject(), QJsonObject{{"result", "yes"}}})
                                  << "Permission answer 2: \"result\" must be one of grant, deny, none (default grant; none: no answer comes).";
  QTest::newRow("negative delay") << QJsonValue(QJsonArray{QJsonObject{{"delay", -0.5}}})
                                  << "Permission answer 1: \"delay\" must be the simulated seconds until the owner answers, 0 to 300 (default 0).";
  QTest::newRow("long delay") << QJsonValue(QJsonArray{QJsonObject{{"delay", 300.5}}})
                              << "Permission answer 1: \"delay\" must be the simulated seconds until the owner answers, 0 to 300 (default 0).";
  QTest::newRow("delay as text") << QJsonValue(QJsonArray{QJsonObject{{"delay", "1"}}})
                                 << "Permission answer 1: \"delay\" must be the simulated seconds until the owner answers, 0 to 300 (default 0).";
  QTest::newRow("delay with none") << QJsonValue(QJsonArray{QJsonObject{{"result", "none"}, {"delay", 1}}})
                                   << "Permission answer 1: \"delay\" cannot be used with result none (no answer comes).";
  QJsonArray many;
  for (int i = 0; i < 65; ++i) many.append(QJsonObject());
  QTest::newRow("too many") << QJsonValue(many) << "\"permissions\" holds at most 64 answers.";
}

void LslAnswersTest::permissionsErrors()
{
  QFETCH(QJsonValue, permissions);
  QFETCH(QString, error);
  const QJsonObject a = run("default { state_entry() { } }", QJsonObject{{"permissions", permissions}});
  QVERIFY(!a["ok"].toBool());
  QCOMPARE(a["error"].toString(), error);
}

void LslAnswersTest::offlineAvatar()
{
  // Bob is offline the whole run: not in the region, but llRequestAgentData still answers (DATA_ONLINE "0")
  const QString source = QString(R"lsl(key bob = "%1";
    default {
      state_entry() {
        llOwnerSay(llList2CSV(llGetAgentList(AGENT_LIST_REGION, [])));
        llOwnerSay("name [" + llKey2Name(bob) + "] info " + (string)llGetAgentInfo(bob) + " size " + (string)llGetAgentSize(bob));
        llOwnerSay("details " + (string)llGetListLength(llGetObjectDetails(bob, [OBJECT_NAME])));
        llRequestAgentData(bob, DATA_ONLINE);
        llRequestAgentData(bob, DATA_NAME);
        llSensor("", "", AGENT, 96.0, PI);
      }
      dataserver(key q, string data) { llOwnerSay("data " + data); }
      sensor(integer n) { llOwnerSay("found " + llDetectedName(0) + " of " + (string)n); }
      no_sensor() { llOwnerSay("nobody"); }
    })lsl").arg(BOB);
  const QJsonObject avatar{{"name", "Bob Resident"}, {"key", BOB}, {"position", QJsonArray{130, 128, 20}}};
  QJsonObject offline = avatar;
  offline["online"] = false;
  const QJsonObject a = run(source, QJsonObject{{"avatars", QJsonArray{offline}}}, 1);
  QCOMPARE(said(a), QStringList({OWNER, "name [] info 0 size <0.00000, 0.00000, 0.00000>", "details 0", "data 0", "data Bob Resident",
                                 "found Owner of 1"}));
  // online (the default): the same script finds him
  const QJsonObject b = run(source, QJsonObject{{"avatars", QJsonArray{avatar}}}, 1);
  QCOMPARE(said(b), QStringList({QString("%1, %2").arg(OWNER, BOB), "name [Bob Resident] info 0 size <0.45000, 0.60000, 1.80000>", "details 1",
                                 "data 1", "data Bob Resident", "found Owner of 2"}));
}

void LslAnswersTest::avatarLogsInAndOut()
{
  // Bob logs in at 2 s and out at 5 s; Carol is online until she logs out at 3 s and back in at 4 s. A timer counts the
  // avatars every second; the log has an avatar entry at each change (no event: Second Life tells scripts nothing)
  const QString source = QString(R"lsl(default {
      state_entry() { llSetTimerEvent(1.0); }
      timer() { llOwnerSay((string)llGetTime() + " " + (string)llGetListLength(llGetAgentList(AGENT_LIST_REGION, [])) + " " + (string)(llGetAgentSize("%1") != ZERO_VECTOR)); }
    })lsl").arg(BOB);
  const QJsonObject a = run(source, QJsonObject{{"avatars", QJsonArray{
      QJsonObject{{"name", "Bob Resident"}, {"key", BOB}, {"login", 2}, {"logout", 5}},
      QJsonObject{{"name", "Carol Resident"}, {"logout", 3}, {"login", 4}}}}}, 6.5);
  QCOMPARE(said(a), QStringList({"1.000000 2 0", "2.000000 3 1", "3.000000 2 1", "4.000000 3 1", "5.000000 2 0", "6.000000 2 0"}));
  QCOMPARE(entries(a, "avatar"), QStringList({
    QString(R"({"avatar":"%1","name":"Bob Resident","note":"The avatar logged in (the avatars option).","online":true,"t":2,"type":"avatar"})").arg(BOB),
    R"({"avatar":"a0000000-0000-4000-8000-000000000002","name":"Carol Resident","note":"The avatar logged out (the avatars option).","online":false,"t":3,"type":"avatar"})",
    R"({"avatar":"a0000000-0000-4000-8000-000000000002","name":"Carol Resident","note":"The avatar logged in (the avatars option).","online":true,"t":4,"type":"avatar"})",
    QString(R"({"avatar":"%1","name":"Bob Resident","note":"The avatar logged out (the avatars option).","online":false,"t":5,"type":"avatar"})").arg(BOB)}));
}

void LslAnswersTest::offlineOwnerIsNotAsked()
{
  // the owner listed as an offline avatar: nobody is asked, so no answer comes; the permissions list is not used
  const QJsonObject a = run(ASKER, QJsonObject{{"avatars", QJsonArray{QJsonObject{{"name", "Owner"}, {"key", OWNER}, {"online", false}}}},
                                               {"permissions", QJsonArray{QJsonObject{{"result", "deny"}}}}}, 2);
  QCOMPARE(said(a), QStringList({"1.00 timer 0"}));
  QCOMPARE(entries(a, "permissions"), QStringList({
    R"({"granted":false,"mask":16,"note":"The owner is offline (the avatars option); nobody is asked and no run_time_permissions comes.","t":0,"type":"permissions"})"}));
}

void LslAnswersTest::avatarErrors_data()
{
  QTest::addColumn<QJsonObject>("avatar");
  QTest::addColumn<QString>("error");
  const QJsonObject bob{{"name", "Bob"}};
  auto with = [&bob](const QJsonObject& more) {
    QJsonObject o = bob;
    for (auto it = more.begin(); it != more.end(); ++it) o[it.key()] = it.value();
    return o;
  };
  QTest::newRow("unknown field") << with({{"away", true}})
                                 << "Avatar 1: unknown field \"away\". Fields: name, key, position, velocity, rotation, in_group, friend, height, online, login, logout.";
  QTest::newRow("online as text") << with({{"online", "no"}}) << "Avatar 1: \"online\" must be true or false (whether the avatar is online at the start).";
  QTest::newRow("login negative") << with({{"login", -1}}) << "Avatar 1: \"login\" must be the simulated second the avatar logs in, 0 to 3600.";
  QTest::newRow("logout too late") << with({{"logout", 3601}}) << "Avatar 1: \"logout\" must be the simulated second the avatar logs out, 0 to 3600.";
  QTest::newRow("login as text") << with({{"login", "2"}}) << "Avatar 1: \"login\" must be the simulated second the avatar logs in, 0 to 3600.";
  QTest::newRow("same time") << with({{"login", 2}, {"logout", 2}}) << "Avatar 1: \"login\" and \"logout\" must be different times.";
  QTest::newRow("online logs in") << with({{"online", true}, {"login", 2}})
                                  << "Avatar 1: an avatar that is online at the start cannot log in before it logs out.";
  QTest::newRow("offline logs out") << with({{"online", false}, {"logout", 2}, {"login", 3}})
                                    << "Avatar 1: an avatar that is offline at the start cannot log out before it logs in.";
}

void LslAnswersTest::avatarErrors()
{
  QFETCH(QJsonObject, avatar);
  QFETCH(QString, error);
  const QJsonObject a = run("default { state_entry() { } }", QJsonObject{{"avatars", QJsonArray{avatar}}});
  QVERIFY(!a["ok"].toBool());
  QCOMPARE(a["error"].toString(), error);
}

void LslAnswersTest::rlvReplyLate()
{
  const QJsonObject a = run(VERSION, QJsonObject{{"replies", QJsonObject{{"rlv", QJsonArray{QJsonObject{{"delay", 2}}}}}}}, 6);
  QCOMPARE(said(a), QStringList({"2.00 heard", "5.00 gave up"}));
  const QJsonArray replies = QJsonDocument::fromJson(entries(a, "rlv_command").at(0).toUtf8()).object()["replies"].toArray();
  QCOMPARE(replies.size(), 1);
  QCOMPARE(replies[0].toObject()["channel"].toInt(), 4711);
  QCOMPARE(replies[0].toObject()["delay"].toDouble(), 2.0);
  QVERIFY(!replies[0].toObject().contains("lost"));
  // without the option the reply comes at once and the entry is as before (no delay, no lost)
  const QJsonObject b = run(VERSION, QJsonObject(), 6);
  QCOMPARE(said(b), QStringList({"0.00 heard", "5.00 gave up"}));
  QCOMPARE(QJsonDocument::fromJson(entries(b, "rlv_command").at(0).toUtf8()).object()["replies"].toArray()[0].toObject().keys(),
           QStringList({"channel", "text"}));
}

void LslAnswersTest::rlvReplyLost()
{
  // one answer for every reply: a viewer without RLV never answers, so the script gives up
  const QJsonObject a = run(VERSION, QJsonObject{{"replies", QJsonObject{{"rlv", QJsonObject{{"result", "none"}}}}}}, 6);
  QCOMPARE(said(a), QStringList({"5.00 gave up"}));
  const QJsonArray replies = QJsonDocument::fromJson(entries(a, "rlv_command").at(0).toUtf8()).object()["replies"].toArray();
  QCOMPARE(replies[0].toObject()["lost"].toBool(), true);
  QCOMPARE(entries(a, "input").size(), 0);   // nothing was heard
}

void LslAnswersTest::rlvReplyAfterTheListenIsGone()
{
  // the reply comes after 7 s, but the script stopped listening at 5 s: it is not heard
  const QJsonObject a = run(VERSION, QJsonObject{{"replies", QJsonObject{{"rlv", QJsonObject{{"delay", 7}}}}}}, 8);
  QCOMPARE(said(a), QStringList({"5.00 gave up"}));
  // the reply still arrives (an input entry at 7 s), only no listen is open for it
  QCOMPARE(entries(a, "input").size(), 1);
  QCOMPARE(QJsonDocument::fromJson(entries(a, "input").at(0).toUtf8()).object()["t"].toDouble(), 7.0);
}

void LslAnswersTest::rlvRepliesInCallOrder()
{
  // the first reply is lost, the second comes (after the list: at once); a touch at 6 s asks again
  const QJsonObject a = run(VERSION, QJsonObject{{"replies", QJsonObject{{"rlv", QJsonArray{QJsonObject{{"result", "none"}}}}}},
                                                 {"inputs", QJsonArray{QJsonObject{{"at", 6}, {"type", "touch"}}}}}, 8);
  QCOMPARE(said(a), QStringList({"5.00 gave up", "6.00 heard"}));
}

void LslAnswersTest::dataserverReplies()
{
  // three notecard lines: the first reply is lost, the second comes after 1.5 s, the third (after the list) at once
  const QString source = R"lsl(default {
      state_entry() { llGetNotecardLine("card", 0); llGetNotecardLine("card", 1); llGetNotecardLine("card", 2); }
      dataserver(key q, string data) { llOwnerSay(llGetSubString((string)llGetTime(), 0, 3) + " " + data); }
    })lsl";
  const QJsonObject card{{"name", "card"}, {"type", "notecard"}, {"lines", QJsonArray{"one", "two", "three"}}};
  const QJsonObject a = run(source, QJsonObject{{"inventory", QJsonArray{card}},
                                                {"replies", QJsonObject{{"dataserver", QJsonArray{QJsonObject{{"result", "none"}}, QJsonObject{{"delay", 1.5}}}}}}}, 3);
  QCOMPARE(said(a), QStringList({"0.00 three", "1.50 two"}));
  QCOMPARE(entries(a, "reply_lost").size(), 1);
  QCOMPARE(entries(a, "reply_delayed").size(), 1);
  QJsonObject lost = QJsonDocument::fromJson(entries(a, "reply_lost").at(0).toUtf8()).object();
  QJsonObject late = QJsonDocument::fromJson(entries(a, "reply_delayed").at(0).toUtf8()).object();
  // the query keys are those of the first and second dataserver_request
  const QStringList requests = entries(a, "dataserver_request");
  QCOMPARE(lost["query"].toString(), QJsonDocument::fromJson(requests.at(0).toUtf8()).object()["query"].toString());
  QCOMPARE(late["query"].toString(), QJsonDocument::fromJson(requests.at(1).toUtf8()).object()["query"].toString());
  lost.remove("query");
  late.remove("query");
  QCOMPARE(QString::fromUtf8(QJsonDocument(lost).toJson(QJsonDocument::Compact)),
           QString(R"({"kind":"dataserver","note":"Simulated: this dataserver reply never comes (the replies option).","t":0,"type":"reply_lost"})"));
  QCOMPARE(QString::fromUtf8(QJsonDocument(late).toJson(QJsonDocument::Compact)),
           QString(R"({"delay":1.5,"kind":"dataserver","note":"Simulated: this dataserver reply comes later (the replies option).","t":0,"type":"reply_delayed"})"));
}

void LslAnswersTest::dataserverReplyForEveryKind()
{
  // one answer for every dataserver reply, also llRequestAgentData; the RLV replies are not touched
  const QString source = R"lsl(default {
      state_entry() { llRequestAgentData(llGetOwner(), DATA_NAME); llRequestAgentData(llGetOwner(), DATA_ONLINE); }
      dataserver(key q, string data) { llOwnerSay(llGetSubString((string)llGetTime(), 0, 3) + " " + data); }
    })lsl";
  const QJsonObject a = run(source, QJsonObject{{"replies", QJsonObject{{"dataserver", QJsonObject{{"delay", 4}}}}}}, 5);
  QCOMPARE(said(a), QStringList({"4.00 Owner", "4.00 1"}));
  const QJsonObject b = run(VERSION, QJsonObject{{"replies", QJsonObject{{"dataserver", QJsonObject{{"result", "none"}}}}}}, 1);
  QCOMPARE(said(b), QStringList({"0.00 heard"}));
}

void LslAnswersTest::repliesErrors_data()
{
  QTest::addColumn<QJsonValue>("replies");
  QTest::addColumn<QString>("error");
  QTest::newRow("not an object") << QJsonValue(QJsonArray()) << "\"replies\" must be an object with rlv and/or dataserver.";
  QTest::newRow("unknown kind") << QJsonValue(QJsonObject{{"http", QJsonArray()}}) << "\"replies\": unknown kind \"http\". Kinds: rlv, dataserver.";
  QTest::newRow("kind as text") << QJsonValue(QJsonObject{{"rlv", "slow"}})
                                << "\"replies\" rlv: must be one answer {\"result\", \"delay\"} for every reply, or a list of answers in call order.";
  QTest::newRow("unknown result") << QJsonValue(QJsonObject{{"rlv", QJsonObject{{"result", "deny"}}}})
                                  << "\"replies\" rlv: \"result\" must be one of ok, none (default ok; none: no answer comes).";
  QTest::newRow("unknown field") << QJsonValue(QJsonObject{{"dataserver", QJsonArray{QJsonObject{{"wait", 1}}}}})
                                 << "\"replies\" dataserver, answer 1: unknown field \"wait\". Fields: result, delay.";
  QTest::newRow("entry not an object") << QJsonValue(QJsonObject{{"dataserver", QJsonArray{QJsonObject(), 3}}})
                                       << "\"replies\" dataserver, answer 2: must be an object with result and/or delay.";
  QTest::newRow("long delay rlv") << QJsonValue(QJsonObject{{"rlv", QJsonObject{{"delay", 301}}}})
                                  << "\"replies\" rlv: \"delay\" must be the simulated seconds until the RLV viewer's reply comes, 0 to 300 (default 0).";
  QTest::newRow("negative delay dataserver") << QJsonValue(QJsonObject{{"dataserver", QJsonArray{QJsonObject{{"delay", -1}}}}})
                                             << "\"replies\" dataserver, answer 1: \"delay\" must be the simulated seconds until the dataserver reply comes, 0 to 300 (default 0).";
  QTest::newRow("delay with none") << QJsonValue(QJsonObject{{"rlv", QJsonObject{{"result", "none"}, {"delay", 2}}}})
                                   << "\"replies\" rlv: \"delay\" cannot be used with result none (no answer comes).";
  QJsonArray many;
  for (int i = 0; i < 65; ++i) many.append(QJsonObject());
  QTest::newRow("too many") << QJsonValue(QJsonObject{{"rlv", many}}) << "\"replies\" rlv: holds at most 64 answers.";
}

void LslAnswersTest::repliesErrors()
{
  QFETCH(QJsonValue, replies);
  QFETCH(QString, error);
  const QJsonObject a = run("default { state_entry() { } }", QJsonObject{{"replies", replies}});
  QVERIFY(!a["ok"].toBool());
  QCOMPARE(a["error"].toString(), error);
}

#endif // Q_MOC_RUN

QTEST_GUILESS_MAIN(LslAnswersTest)
#include "tst_lslanswers.moc"
