/*
 * PinglyMotion - tests for the region and avatar reads (Script Lab, gap list 2, step 3)
 *
 * llGetRegionName, llGetRegionCorner, llGround, llWater, llGetAgentSize, llGetAgentList, llRequestAgentData,
 * llGetSunDirection, llGetTimeOfDay, llGetTimestamp, llGetWallclock and llGetDate, with the canned region of the
 * sandbox and the "region" option of lsl_run. Every expected value is exact and was worked out by hand from the
 * rules in docs/CONTROL.md ("The region and the avatars in it"): the run starts at 1767225600 (2026-01-01 00:00
 * UTC, which is 16:00 on 31 December in Second Life time, UTC-8, so llGetWallclock is 57600); in 2026 Second Life
 * summer time runs from 8 March 10:00 UTC (1772964000) to 1 November 09:00 UTC (1793523600); the sun direction is
 * <sin a, 0, -cos a> with a = 2 pi * time of day / day length. No test reads the computer's clock.
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
  const char* const SITTER = "66666666-7777-8888-9999-000000000000";
}

class LslRegionTest : public QObject
{
  Q_OBJECT

  private slots:
    void defaultRegion();
    void regionOption();
    void clockFollowsSimulatedTime();
    void summerTimeStartsAndEnds();
    void timeOfDayAndSun();
    void agentSize();
    void agentList();
    void requestAgentData();
    void regionKeptForLslEvent();
    void badRegionInput();
    void checkerHasNoWarnings();
    void constantsMatchSecondLife();

  private:
    static QJsonObject run(const QString& source, const QJsonObject& extra = QJsonObject(), double seconds = 2);
    static QStringList said(const QJsonObject& answer);
};

QJsonObject LslRegionTest::run(const QString& source, const QJsonObject& extra, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}};
  for (auto it = extra.begin(); it != extra.end(); ++it) request[it.key()] = it.value();
  return Control::run(request);
}

QStringList LslRegionTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

void LslRegionTest::defaultRegion()
{
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llOwnerSay(llGetRegionName());
    llOwnerSay((string)llGetRegionCorner());
    llOwnerSay((string)llGround(ZERO_VECTOR) + " " + (string)llGround(<300, -50, 10>));
    llOwnerSay((string)llWater(ZERO_VECTOR) + " " + (string)llWater(<1, 2, 3>));
    llOwnerSay(llGetDate() + " " + llGetTimestamp() + " " + (string)llGetUnixTime());
    llOwnerSay((string)llGetWallclock() + " " + (string)llGetTimeOfDay() + " " + (string)llGetSunDirection());
  } })lsl");
  const QStringList expected({
    "Sandbox",
    "<256000.00000, 256000.00000, 0.00000>",
    "20.000000 20.000000",
    "20.000000 20.000000",
    "2026-01-01 2026-01-01T00:00:00.000000Z 1767225600",
    "57600.000000 7200.000000 <0.00000, 0.00000, 1.00000>"});
  QCOMPARE(said(a), expected);
}

void LslRegionTest::regionOption()
{
  const QJsonObject region{{"name", "Test Region"}, {"corner", QJsonArray{512000, 768000, 0}}, {"ground", 23.5}, {"water", 21.25},
                           {"day_length", 3600}, {"time_of_day", 450}, {"unix_start", 1800000000}};
  const QJsonObject a = run(QString(R"lsl(default { state_entry() {
    llOwnerSay(llGetRegionName() + " " + (string)llGetRegionCorner());
    llOwnerSay((string)llGround(ZERO_VECTOR) + " " + (string)llWater(ZERO_VECTOR));
    llOwnerSay(llGetDate() + " " + llGetTimestamp() + " " + (string)llGetUnixTime());
    llOwnerSay((string)llGetTimeOfDay() + " " + (string)llGetSunDirection());
    llOwnerSay(llList2String(llGetObjectDetails("%1", [OBJECT_CREATION_TIME, OBJECT_REZ_TIME]), 1));
  } })lsl").arg(OBJECT), QJsonObject{{"region", region}});
  // 1800000000 is 2027-01-15 08:00:00 UTC; an eighth of the day: a = pi / 4
  const QStringList expected({
    "Test Region <512000.00000, 768000.00000, 0.00000>",
    "23.500000 21.250000",
    "2027-01-15 2027-01-15T08:00:00.000000Z 1800000000",
    "450.000000 <0.70711, 0.00000, -0.70711>",
    "2027-01-15T08:00:00.000000Z"});
  QCOMPARE(said(a), expected);
}

void LslRegionTest::clockFollowsSimulatedTime()
{
  // the time functions follow the simulated clock: a timer at 1.25 s, then a sleep of 0.000250 s
  const QJsonObject a = run(R"lsl(default {
    state_entry() { llSetTimerEvent(1.25); }
    timer() {
      llSetTimerEvent(0);
      llOwnerSay(llGetTimestamp() + " " + (string)llGetWallclock() + " " + (string)llGetTimeOfDay());
      llSleep(0.25);
      llOwnerSay(llGetTimestamp() + " " + llGetDate());
    }
  })lsl", QJsonObject(), 3);
  const QStringList expected({
    "2026-01-01T00:00:01.250000Z 57601.250000 7201.250000",
    "2026-01-01T00:00:01.500000Z 2026-01-01"});
  QCOMPARE(said(a), expected);

  // a run that starts one second before midnight UTC: the date changes during the run
  const QJsonObject b = run(R"lsl(default { state_entry() {
    llOwnerSay(llGetDate() + " " + llGetTimestamp());
    llSleep(1.0);
    llOwnerSay(llGetDate() + " " + llGetTimestamp());
  } })lsl", QJsonObject{{"region", QJsonObject{{"unix_start", 1767311999}}}});
  const QStringList expected2({
    "2026-01-01 2026-01-01T23:59:59.000000Z",
    "2026-01-02 2026-01-02T00:00:00.000000Z"});
  QCOMPARE(said(b), expected2);
}

void LslRegionTest::summerTimeStartsAndEnds()
{
  const QString source = R"lsl(default { state_entry() {
    llOwnerSay((string)llGetWallclock());
    llSleep(1.0);
    llOwnerSay((string)llGetWallclock());
  } })lsl";
  // one second before 8 March 2026, 10:00 UTC: 01:59:59 standard time, then 03:00:00 summer time
  const QStringList expected({"7199.000000", "10800.000000"});
  QCOMPARE(said(run(source, QJsonObject{{"region", QJsonObject{{"unix_start", 1772963999}}}})), expected);
  // one second before 1 November 2026, 09:00 UTC: 01:59:59 summer time, then 01:00:00 standard time
  const QStringList expected2({"7199.000000", "3600.000000"});
  QCOMPARE(said(run(source, QJsonObject{{"region", QJsonObject{{"unix_start", 1793523599}}}})), expected2);
  // a summer day: 1 July 2026, 00:00 UTC is 17:00 the day before in Second Life time
  const QStringList expected3({"61200.000000", "61201.000000"});
  QCOMPARE(said(run(source, QJsonObject{{"region", QJsonObject{{"unix_start", 1782864000}}}})), expected3);
}

void LslRegionTest::timeOfDayAndSun()
{
  const QString source = R"lsl(default { state_entry() {
    llOwnerSay((string)llGetTimeOfDay() + " " + (string)llGetSunDirection());
    llSleep(2.0);
    llOwnerSay((string)llGetTimeOfDay() + " " + (string)llGetSunDirection());
  } })lsl";
  // midnight, then two seconds later in a day of 60 seconds
  const QStringList expected({"0.000000 <0.00000, 0.00000, -1.00000>", "2.000000 <0.20791, 0.00000, -0.97815>"});
  QCOMPARE(said(run(source, QJsonObject{{"region", QJsonObject{{"day_length", 60}, {"time_of_day", 0}}}}, 3)), expected);
  // sunrise (east), then the day wraps round: 59 + 2 = 61, which is 1 second into the next day
  const QStringList expected2({"15.000000 <1.00000, 0.00000, 0.00000>", "17.000000 <0.97815, 0.00000, 0.20791>"});
  QCOMPARE(said(run(source, QJsonObject{{"region", QJsonObject{{"day_length", 60}, {"time_of_day", 15}}}}, 3)), expected2);
  const QStringList expected3({"59.000000 <-0.10453, 0.00000, -0.99452>", "1.000000 <0.10453, 0.00000, -0.99452>"});
  QCOMPARE(said(run(source, QJsonObject{{"region", QJsonObject{{"day_length", 60}, {"time_of_day", 59}}}}, 3)), expected3);
  // sunset (west)
  const QStringList expected4({"2700.000000 <-1.00000, 0.00000, 0.00000>", "2702.000000 <-0.99999, 0.00000, -0.00349>"});
  QCOMPARE(said(run(source, QJsonObject{{"region", QJsonObject{{"day_length", 3600}, {"time_of_day", 2700}}}}, 3)), expected4);
}

void LslRegionTest::agentSize()
{
  const QJsonObject a = run(QString(R"lsl(default { state_entry() {
    llOwnerSay((string)llGetAgentSize("%1"));
    llOwnerSay((string)llGetAgentSize("%2"));
    llOwnerSay((string)llGetAgentSize("%3"));
    llOwnerSay((string)llGetAgentSize(NULL_KEY) + " " + (string)llGetAgentSize("not a key"));
  } })lsl").arg(OWNER, GUEST, OBJECT),
    QJsonObject{{"avatars", QJsonArray{QJsonObject{{"name", "Guest Avatar"}, {"key", GUEST}, {"height", 2.1}}}}});
  const QStringList expected({
    "<0.45000, 0.60000, 1.80000>",
    "<0.45000, 0.60000, 2.10000>",
    "<0.00000, 0.00000, 0.00000>",
    "<0.00000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000>"});
  QCOMPARE(said(a), expected);
}

void LslRegionTest::agentList()
{
  const QString source = R"lsl(default {
    state_entry() { llOwnerSay(llList2CSV(llGetAgentList(AGENT_LIST_REGION, []))); }
    changed(integer c) {
      llOwnerSay(llList2CSV(llGetAgentList(AGENT_LIST_PARCEL, [])) + " | " + llList2CSV(llGetAgentList(AGENT_LIST_PARCEL_OWNER, [])));
      llOwnerSay(llList2CSV(llGetAgentList(0, [])) + " " + llList2CSV(llGetAgentList(3, [])) + " "
                 + (string)llGetListLength(llGetAgentList(AGENT_LIST_REGION, [1, 2])));
    }
  })lsl";
  // the owner alone
  QCOMPARE(said(run(source)), QStringList({QString(OWNER)}));
  // the owner first, then the listed avatars; an avatar that sits down joins at the end
  const QJsonObject a = run(source, QJsonObject{
    {"avatars", QJsonArray{QJsonObject{{"name", "Guest Avatar"}, {"key", GUEST}}}},
    {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "sit"}, {"id", SITTER}, {"name", "Sitting Avatar"}}}}});
  const QString three = QString("%1, %2, %3").arg(OWNER, GUEST, SITTER);
  const QStringList expected({QString("%1, %2").arg(OWNER, GUEST), three + " | " + three, "INVALID_SCOPE INVALID_SCOPE 3"});
  QCOMPARE(said(a), expected);
  // the owner where the avatars list puts it
  const QJsonObject b = run(source, QJsonObject{{"avatars", QJsonArray{QJsonObject{{"name", "Guest Avatar"}, {"key", GUEST}},
                                                                     QJsonObject{{"name", "Owner"}, {"key", OWNER}}}}});
  QCOMPARE(said(b), QStringList({QString("%1, %2").arg(GUEST, OWNER)}));
}

void LslRegionTest::requestAgentData()
{
  const QJsonObject a = run(QString(R"lsl(
    list asked;
    default {
      state_entry() {
        asked = [llRequestAgentData("%1", DATA_ONLINE), llRequestAgentData("%1", DATA_NAME), llRequestAgentData("%1", DATA_BORN),
                 llRequestAgentData("%1", DATA_RATING), llRequestAgentData("%1", DATA_PAYINFO), llRequestAgentData("%2", DATA_NAME),
                 llRequestAgentData("%3", DATA_NAME), llRequestAgentData("%1", 99)];
        llOwnerSay((string)llGetListLength(asked) + " " + (string)(llList2Key(asked, 6) != NULL_KEY));
      }
      dataserver(key id, string data) { llOwnerSay((string)llListFindList(asked, [id]) + " " + data); }
    })lsl").arg(OWNER, GUEST, OBJECT),
    QJsonObject{{"avatars", QJsonArray{QJsonObject{{"name", "Guest Avatar"}, {"key", GUEST}}}}});
  // the object key and DATA_* 99 get no answer
  const QStringList expected({"8 1", "0 1", "1 Owner", "2 2026-01-01", "3 0,0,0,0,0,0", "4 0", "5 Guest Avatar"});
  QCOMPARE(said(a), expected);
  int requests = 0;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"].toString() == "dataserver_request")
    {
      ++requests;
      QCOMPARE(v.toObject()["function"].toString(), QString("llRequestAgentData"));
    }
  QCOMPARE(requests, 8);
  // the first entry in full
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != "dataserver_request") continue;
    QCOMPARE(e["avatar"].toString(), QString(OWNER));
    QCOMPARE(e["data"].toInt(), 1);
    QCOMPARE(e["query"].toString().size(), 36);
    QCOMPARE(e["t"].toDouble(), 0.0);
    break;
  }
}

void LslRegionTest::regionKeptForLslEvent()
{
  RlvState rlv;
  Session session;
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"region", QJsonObject{{"name", "Kept"}, {"unix_start", 1800000000}}},
    {"source", R"lsl(default { touch_start(integer n) { llOwnerSay(llGetRegionName() + " " + llGetTimestamp()); } })lsl"}}, &rlv, &session);
  QVERIFY(a["ok"].toBool());
  const QJsonObject e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"seconds", 1}}, &session);
  QCOMPARE(said(e), QStringList({"Kept 2027-01-15T08:00:01.000000Z"}));
}

void LslRegionTest::badRegionInput()
{
  const QString source = "default { state_entry() { } }";
  const QList<QPair<QJsonObject, QString>> cases = {
    {QJsonObject{{"region", "Sandbox"}}, "\"region\" must be an object with any of name, corner, ground, water, day_length, time_of_day, unix_start."},
    {QJsonObject{{"region", QJsonObject{{"weather", 1}}}}, "Region: unknown field \"weather\". Fields: name, corner, ground, water, day_length, time_of_day, unix_start."},
    {QJsonObject{{"region", QJsonObject{{"name", QString(36, 'x')}}}}, "Region: \"name\" must be text of 1 to 35 characters."},
    {QJsonObject{{"region", QJsonObject{{"name", " "}}}}, "Region: \"name\" must be text of 1 to 35 characters."},
    {QJsonObject{{"region", QJsonObject{{"corner", QJsonArray{256000, 256000, 5}}}}}, "Region: \"corner\" must be [x, y, 0] in whole metres, x and y 0 to 16777216."},
    {QJsonObject{{"region", QJsonObject{{"corner", QJsonArray{0.5, 0, 0}}}}}, "Region: \"corner\" must be [x, y, 0] in whole metres, x and y 0 to 16777216."},
    {QJsonObject{{"region", QJsonObject{{"ground", -1}}}}, "Region: \"ground\" must be the ground height in metres, 0 to 4096."},
    {QJsonObject{{"region", QJsonObject{{"water", "20"}}}}, "Region: \"water\" must be the water height in metres, 0 to 4096."},
    {QJsonObject{{"region", QJsonObject{{"day_length", 0}}}}, "Region: \"day_length\" must be the seconds of one day, 60 to 2592000."},
    {QJsonObject{{"region", QJsonObject{{"time_of_day", 14400}}}},
     "Region: \"time_of_day\" must be the seconds since the region's midnight at the start, 0 up to (not including) day_length."},
    {QJsonObject{{"region", QJsonObject{{"unix_start", 1.5}}}},
     "Region: \"unix_start\" must be the UTC time at the start in whole seconds since 1970, 0 to 2000000000."},
    {QJsonObject{{"avatars", QJsonArray{QJsonObject{{"name", "Tall"}, {"height", 4}}}}},
     "Avatar 1: \"height\" must be the avatar's height in metres, 0.5 to 3 (default 1.8)."}};
  for (const auto& c : cases)
  {
    const QJsonObject a = run(source, c.first);
    QCOMPARE(a["ok"].toBool(), false);
    QCOMPARE(a["error"].toString(), c.second);
  }
}

void LslRegionTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llGetRegionName(); llGetRegionCorner(); llGround(ZERO_VECTOR); llWater(ZERO_VECTOR); llGetAgentSize(NULL_KEY);
      llGetAgentList(AGENT_LIST_REGION, []); llRequestAgentData(NULL_KEY, DATA_NAME); llGetSunDirection(); llGetTimeOfDay();
      llGetTimestamp(); llGetWallclock(); llGetDate();
    } transaction_result(key id, integer success, string data) { } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  for (const char* f : {"llGetRegionName", "llGetRegionCorner", "llGround", "llWater", "llGetAgentSize", "llGetAgentList",
                        "llRequestAgentData", "llGetSunDirection", "llGetTimeOfDay", "llGetTimestamp", "llGetWallclock", "llGetDate"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
  // the interpreter raises transaction_result (llTransferLindenDollars), so the checker must not warn about it
  QVERIFY(Parser::eventIsRaised("transaction_result"));
}

void LslRegionTest::constantsMatchSecondLife()
{
  const QList<QPair<QString, int>> expected = {
    {"AGENT_LIST_PARCEL", 1}, {"AGENT_LIST_PARCEL_OWNER", 2}, {"AGENT_LIST_REGION", 4},
    {"DATA_ONLINE", 1}, {"DATA_NAME", 2}, {"DATA_BORN", 3}, {"DATA_RATING", 4}, {"DATA_PAYINFO", 8}};
  for (const auto& e : expected)
  {
    const BuiltinConstant* c = Builtins::constant(e.first);
    QVERIFY2(c != nullptr, qPrintable(e.first));
    QCOMPARE(c->value.i, e.second);
  }
}

QTEST_GUILESS_MAIN(LslRegionTest)
#include "tst_lslregion.moc"
