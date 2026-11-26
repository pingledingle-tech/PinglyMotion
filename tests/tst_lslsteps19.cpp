/*
 * PinglyMotion - tests for step 19 of the Script Lab gap list: the key-value store (llCreateKeyValue, llReadKeyValue,
 * llUpdateKeyValue, llDeleteKeyValue, llDataSizeKeyValue, llKeyCountKeyValue, llKeysKeyValue), the experience functions
 * (llAgentInExperience, llGetExperienceDetails, llGetExperienceErrorMessage, llRequestExperiencePermissions), llGetSimStats,
 * llScriptProfiler and llManageEstateAccess
 *
 * The expected answers were worked out by hand from the Second Life wiki. Where the wiki is silent or unclear the sandbox
 * chooses; every choice is written here and in docs/CONTROL.md:
 *  - No "experience" in lsl_run: no script is in an experience. Every key-value call then answers "0,5" (XP_ERROR_NO_EXPERIENCE) in
 *    the dataserver event, llAgentInExperience is FALSE, llGetExperienceDetails is the empty list and a permission request is
 *    answered with experience_permissions_denied(agent, 5). With "experience" every script of the world is in that one experience.
 *  - The checks of a key-value call, in this order: an experience (5), its state ("disabled" 8, "suspended" 9), the store on
 *    ("store_enabled": false gives 12), the sizes (an empty key, a key over 1011 bytes or a value over 4095 bytes, UTF-8, give 3).
 *  - llCreateKeyValue of a key that exists gives 13 (the wiki). llReadKeyValue of a missing key gives 14. llDeleteKeyValue of a
 *    missing key gives 13 (the wiki's page for it; the other pages use 14). llUpdateKeyValue of a missing key creates it (the
 *    wiki); with checked TRUE it gives 15, as no stored value can match the original (the wiki is silent).
 *  - Success is "1,<value>" for create, read, update and delete (delete: the deleted value), "1,<used>,<quota>" for
 *    llDataSizeKeyValue (bytes of all keys and values, 134217728 unless "quota" says otherwise), "1,<count>" for llKeyCountKeyValue
 *    and "1,<key>,<key>..." for llKeysKeyValue (alphabetical, which is stable; at most 4096 characters after "1,"). A first below 0,
 *    a count below 1: "0,3" (the wiki is silent); a first past the last key: "0,14" (the wiki). Over the quota: "0,11" (the wiki is silent).
 *  - The dataserver event comes at once, in the same simulated second (there is no delay model).
 *  - llGetExperienceDetails: [name, owner, key, state, state message, group]; the state is XP_ERROR_NONE, 8 or 9 and the
 *    message llGetExperienceErrorMessage of it (the wiki gives no values; its bug note shows a code and its text). NULL_KEY or
 *    the experience's key answers; any other key the empty list.
 *  - llAgentInExperience is TRUE for an avatar of the region that is in "members" while the experience is enabled.
 *  - llRequestExperiencePermissions: a member gets experience_permissions at once; a blocked avatar gets
 *    experience_permissions_denied(agent, 4); an avatar that has not answered is asked, and the answer comes with an input
 *    {"type":"experience","kind":"allow"} or "deny" (the choice is kept, as the wiki says). There is no five-minute time-out.
 *    A disabled or suspended experience denies with its state (8 or 9). A key with no avatar in the sandbox asks nobody.
 *  - llGetSimStats: canned numbers of a healthy region (see builtins.cpp), the agent and script counts are the world's, and a
 *    number that is no SIM_STAT_* gives 0.0.
 *  - llScriptProfiler: PROFILE_SCRIPT_MEMORY (bit 0) starts, any other value stops; llGetSPMaxMemory is the most memory the script
 *    used since the start (measured at every statement), kept after PROFILE_NONE, 0 before any profiling.
 *  - llManageEstateAccess: checks in this order: the throttle (30 calls in 30 seconds, for the whole estate), the action (one of
 *    the six ESTATE_ACCESS_* values), the key (a UUID, not NULL_KEY), mainland (message on DEBUG_CHANNEL), the estate rights
 *    of the object's owner (message on DEBUG_CHANNEL; the estate owner is the owner of the world unless "estate" says
 *    otherwise). Adding what is there or removing what is not is TRUE and changes nothing. The lists have no size limit.
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

class LslSteps19Test : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void storeRoundTrip();
    void handleMatches();
    void noExperience();
    void stateAndStoreSwitches();
    void sizeLimits();
    void quota();
    void setupAndInputs();
    void inputThroughLslEvent();
    void errorMessages();
    void details();
    void agentInExperience();
    void permissionRequests();
    void permissionRequestRefusals();
    void simStats();
    void profiler();
    void estateAccess();
    void estateRights();
    void estateSilentAndLogged();
    void estateThrottle();
    void refusedOptions();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& source, const QJsonObject& extra = QJsonObject(), const QJsonArray& inputs = QJsonArray(), double seconds = 3);
    static QStringList said(const QJsonObject& answer);
    static QJsonArray ofType(const QJsonObject& answer, const QString& type);
};

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const KEY_A = "aaaaaaaa-0000-4000-8000-000000000001";
  const char* const KEY_B = "bbbbbbbb-0000-4000-8000-000000000002";
  const char* const KEY_C = "cccccccc-0000-4000-8000-000000000003";
  const char* const KEY_D = "dddddddd-0000-4000-8000-000000000004";
  const char* const EXPERIENCE_KEY = "e0000000-0000-4000-8000-0000000000e1";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";

  QString body(const QString& statements) { return "default { state_entry() {\n" + statements + "\n} }"; }

  // a script that does its calls one after the other: each dataserver answer is said and starts the next call
  QString chain(const QStringList& calls)
  {
    QString source = "integer n;\nnext() {\n";
    for (int i = 0; i < calls.size(); ++i) source += QString("  %1 (n == %2) { %3 }\n").arg(i == 0 ? "if" : "else if").arg(i).arg(calls[i]);
    source += QString("  ++n;\n}\ndefault {\n  state_entry() { next(); }\n  dataserver(key q, string d) { llOwnerSay(d); if (n < %1) next(); }\n}").arg(calls.size());
    return source;
  }

  QJsonObject experience(const QJsonObject& fields = QJsonObject()) { return QJsonObject{{"experience", fields}}; }
  QJsonObject avatar(const QString& name, const QString& key) { return QJsonObject{{"name", name}, {"key", key}}; }
}

QJsonObject LslSteps19Test::run(const QString& source, const QJsonObject& extra, const QJsonArray& inputs, double seconds)
{
  QJsonObject request = extra;
  request["command"] = "lsl_run";
  request["source"] = source;
  request["seconds"] = seconds;
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request);
}

QStringList LslSteps19Test::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != "owner_say") continue;
    out << e["text"].toString();
  }
  return out;
}

QJsonArray LslSteps19Test::ofType(const QJsonObject& answer, const QString& type)
{
  QJsonArray out;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == type) out.append(v);
  return out;
}

void LslSteps19Test::supportedNow()
{
  for (const char* name : {"llCreateKeyValue", "llReadKeyValue", "llUpdateKeyValue", "llDeleteKeyValue", "llDataSizeKeyValue", "llKeyCountKeyValue",
                           "llKeysKeyValue", "llAgentInExperience", "llGetExperienceDetails", "llGetExperienceErrorMessage",
                           "llRequestExperiencePermissions", "llGetSimStats", "llScriptProfiler", "llManageEstateAccess"})
  {
    QVERIFY2(Builtins::function(name) != nullptr, name);
    QVERIFY2(Builtins::function(name)->supported, name);
  }
  // pathfinding is not in this step: the example of "not supported yet" in the other tests
  QVERIFY(Builtins::function("llNavigateTo") && !Builtins::function("llNavigateTo")->supported);
  QVERIFY(Builtins::function("llCreateCharacter") && !Builtins::function("llCreateCharacter")->supported);
}

// every call in order: the answers of the dataserver event, and the store at the end
void LslSteps19Test::storeRoundTrip()
{
  const QString source = chain({
    R"(llCreateKeyValue("a", "x,y");)",              // 0: created
    R"(llCreateKeyValue("a", "again");)",            // 1: exists
    R"(llReadKeyValue("a");)",                       // 2
    R"(llReadKeyValue("nope");)",                    // 3: missing
    R"(llUpdateKeyValue("a", "z", FALSE, "");)",     // 4: unchecked update
    R"(llUpdateKeyValue("a", "q", TRUE, "wrong");)", // 5: checked, stale
    R"(llUpdateKeyValue("a", "w", TRUE, "z");)",     // 6: checked, right
    R"(llUpdateKeyValue("b", "v", FALSE, "");)",     // 7: missing, so created
    R"(llUpdateKeyValue("c", "u", TRUE, "u");)",     // 8: checked on a missing key
    R"(llKeyCountKeyValue();)",                      // 9
    R"(llDataSizeKeyValue();)",                      // 10: a w b v = 4 bytes
    R"(llKeysKeyValue(0, 10);)",                     // 11
    R"(llKeysKeyValue(1, 1);)",                      // 12
    R"(llKeysKeyValue(2, 1);)",                      // 13: past the end
    R"(llKeysKeyValue(-1, 1);)",                     // 14
    R"(llKeysKeyValue(0, 0);)",                      // 15
    R"(llDeleteKeyValue("a");)",                     // 16: the deleted value
    R"(llDeleteKeyValue("a");)",                     // 17: gone
    R"(llKeyCountKeyValue();)"                       // 18
  });
  const QJsonObject a = run(source, experience());
  QCOMPARE(said(a), QStringList({"1,x,y", "0,13", "1,x,y", "0,14", "1,z", "0,15", "1,w", "1,v", "0,15", "1,2", "1,4,134217728",
                                 "1,a,b", "1,b", "0,14", "0,3", "0,3", "1,w", "0,13", "1,1"}));
  const QJsonObject x = a["experience"].toObject();
  QCOMPARE(x["store"].toObject(), QJsonObject({{"b", "v"}}));
  QCOMPARE(x["store_bytes"].toDouble(), 2.0);
  QCOMPARE(x["key"].toString(), QString(EXPERIENCE_KEY));
  QCOMPARE(x["state"].toString(), QString("enabled"));
  QCOMPARE(ofType(a, "dataserver_request").size(), qsizetype(19));
  QCOMPARE(ofType(a, "dataserver_request")[0].toObject()["function"].toString(), QString("llCreateKeyValue"));
  QCOMPARE(ofType(a, "dataserver_request")[0].toObject()["result"].toString(), QString("1,x,y"));
}

void LslSteps19Test::handleMatches()
{
  const QJsonObject a = run(R"lsl(key h1; key h2;
    default {
      state_entry() { h1 = llCreateKeyValue("k", "v"); h2 = llReadKeyValue("k"); llOwnerSay((string)(h1 == h2) + (string)(h1 == NULL_KEY)); }
      dataserver(key q, string d) { llOwnerSay((string)(q == h1) + (string)(q == h2) + " " + d); }
    })lsl", experience());
  QCOMPARE(said(a), QStringList({"00", "10 1,v", "01 1,v"}));
}

void LslSteps19Test::noExperience()
{
  const QJsonObject a = run(R"lsl(default {
      state_entry() {
        llCreateKeyValue("a", "b");
        llOwnerSay((string)llAgentInExperience(llGetOwner()));
        llOwnerSay((string)llGetListLength(llGetExperienceDetails(NULL_KEY)));
        llRequestExperiencePermissions(llGetOwner(), "");
      }
      dataserver(key q, string d) { llOwnerSay(d); }
      experience_permissions(key id) { llOwnerSay("granted"); }
      experience_permissions_denied(key id, integer why) { llOwnerSay("denied " + (string)why + " " + llGetExperienceErrorMessage(why)); }
    })lsl");
  QCOMPARE(said(a), QStringList({"0", "0", "0,5", "denied 5 script not associated with an experience"}));
  QVERIFY(!a.contains("experience"));
  QVERIFY(!a.contains("estate"));
}

void LslSteps19Test::stateAndStoreSwitches()
{
  const QString source = chain({R"(llCreateKeyValue("a", "b");)", R"(llReadKeyValue("a");)", R"(llKeyCountKeyValue();)", R"(llDataSizeKeyValue();)", R"(llKeysKeyValue(0, 1);)"});
  QCOMPARE(said(run(source, experience({{"state", "disabled"}}))), QStringList({"0,8", "0,8", "0,8", "0,8", "0,8"}));
  QCOMPARE(said(run(source, experience({{"state", "suspended"}}))), QStringList({"0,9", "0,9", "0,9", "0,9", "0,9"}));
  QCOMPARE(said(run(source, experience({{"store_enabled", false}}))), QStringList({"0,12", "0,12", "0,12", "0,12", "0,12"}));
  // a disabled experience beats a store that is off
  QCOMPARE(said(run(source, experience({{"state", "disabled"}, {"store_enabled", false}}))), QStringList({"0,8", "0,8", "0,8", "0,8", "0,8"}));
}

void LslSteps19Test::sizeLimits()
{
  const QString prelude = R"lsl(string big = "0123456789";
    default {
      state_entry() {
        integer i;
        for (i = 0; i < 9; ++i) big += big;   // 5120 characters
        llCreateKeyValue(llGetSubString(big, 0, 1010), "v");   // a key of 1011 bytes
      }
      dataserver(key q, string d) {
        llOwnerSay(llGetSubString(d, 0, 1) + (string)llStringLength(d));
        ++n;
        if (n == 1) llCreateKeyValue(llGetSubString(big, 0, 1011), "v");           // 1012 bytes
        else if (n == 2) llCreateKeyValue("", "v");                                // no key
        else if (n == 3) llCreateKeyValue("k", llGetSubString(big, 0, 4094));      // 4095 bytes
        else if (n == 4) llCreateKeyValue("l", llGetSubString(big, 0, 4095));      // 4096 bytes
        else if (n == 5) llReadKeyValue(llGetSubString(big, 0, 1011));
        else if (n == 6) llCreateKeyValue("é", llGetSubString(big, 0, 4092) + "é"); // 4093 + 2 = 4095 bytes in UTF-8
        else if (n == 7) llCreateKeyValue("m", llGetSubString(big, 0, 4093) + "éé"); // 4098 bytes
      }
    })lsl";
  const QString source = "integer n;\n" + prelude;
  const QJsonObject a = run(source, experience());
  // "1," and the value; "0,3" is 3 characters
  QCOMPARE(said(a), QStringList({"1,3", "0,3", "0,3", "1,4097", "0,3", "0,3", "1,4096", "0,3"}));
}

void LslSteps19Test::quota()
{
  const QString source = chain({R"(llCreateKeyValue("ab", "cde");)",       // 5 bytes
                                R"(llCreateKeyValue("fg", "hijkl");)",     // 7 more would be 12 > 10
                                R"(llUpdateKeyValue("ab", "cdefghijk", FALSE, "");)",   // 2 + 9 = 11 > 10
                                R"(llUpdateKeyValue("ab", "cdefgh", FALSE, "");)",      // 8
                                R"(llDataSizeKeyValue();)"});
  const QJsonObject a = run(source, experience({{"quota", 10}}));
  QCOMPARE(said(a), QStringList({"1,cde", "0,11", "0,11", "1,cdefgh", "1,8,10"}));
  QCOMPARE(a["experience"].toObject()["quota"].toDouble(), 10.0);
}

// the store from the setup (bytes counted in UTF-8) and changed from outside with experience inputs
void LslSteps19Test::setupAndInputs()
{
  const QString source = R"lsl(default {
      state_entry() { llDataSizeKeyValue(); llSetTimerEvent(3); }
      timer() { llSetTimerEvent(0); llKeysKeyValue(0, 10); }
      dataserver(key q, string d) { llOwnerSay(d); }
    })lsl";
  const QJsonArray inputs{QJsonObject{{"at", 1}, {"type", "experience"}, {"kind", "set"}, {"key", "n"}, {"value", "5"}},
                          QJsonObject{{"at", 1.5}, {"type", "experience"}, {"kind", "set"}, {"key", "k"}, {"value", "w"}},
                          QJsonObject{{"at", 2}, {"type", "experience"}, {"kind", "delete"}, {"key", "é"}},
                          QJsonObject{{"at", 2.5}, {"type", "experience"}, {"kind", "delete"}, {"key", "ghost"}}};
  const QJsonObject a = run(source, experience({{"store", QJsonObject{{"k", "v"}, {"é", "ü"}}}}), inputs, 4);
  QCOMPARE(said(a), QStringList({"1,6,134217728", "1,k,n"}));
  QCOMPARE(a["experience"].toObject()["store"].toObject(), QJsonObject({{"k", "w"}, {"n", "5"}}));
  const QJsonArray in = ofType(a, "input");
  QCOMPARE(int(in.size()), 4);
  QVERIFY(!in[0].toObject().contains("error"));
  QCOMPARE(in[3].toObject()["error"].toString(), QString("key-value store communication failed"));
  // an input before there is an experience says so
  const QJsonObject none = run("default { timer() { } }", QJsonObject(), QJsonArray{QJsonObject{{"at", 1}, {"type", "experience"}, {"kind", "allow"}}}, 2);
  QVERIFY2(none["ok"].toBool(), qPrintable(none["error"].toString()));
  QVERIFY(ofType(none, "input")[0].toObject()["error"].toString().contains("No experience is set up"));
}

void LslSteps19Test::inputThroughLslEvent()
{
  RlvState rlv;
  Session session;
  const QString source = R"lsl(default {
      state_entry() { llRequestExperiencePermissions(llGetOwner(), ""); }
      experience_permissions(key id) { llOwnerSay("granted"); llReadKeyValue("k"); }
      experience_permissions_denied(key id, integer why) { llOwnerSay("denied " + (string)why); }
      dataserver(key q, string d) { llOwnerSay(d); }
    })lsl";
  QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", 1}, {"experience", QJsonObject{{"store", QJsonObject{{"k", "v"}}}}}}, &rlv, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList());
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "experience"}, {"kind", "set"}, {"key", "k"}, {"value", "new"}, {"seconds", 1}}, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(a["experience"].toObject()["store"].toObject(), QJsonObject({{"k", "new"}}));
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "experience"}, {"kind", "allow"}, {"seconds", 1}}, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"granted", "1,new"}));
  QCOMPARE(a["experience"].toObject()["members"].toArray(), QJsonArray({OWNER}));
}

void LslSteps19Test::errorMessages()
{
  const QJsonObject a = run(body(R"lsl(
    list out;
    integer i;
    for (i = 0; i <= 19; ++i) out += llGetExperienceErrorMessage(i);
    out += llGetExperienceErrorMessage(-1);
    out += llGetExperienceErrorMessage(99);
    llOwnerSay(llDumpList2String(out, "|"));
  )lsl"));
  QCOMPARE(said(a), QStringList({"no error|exceeded throttle|experiences are disabled|invalid parameters|operation not permitted|"
                                 "script not associated with an experience|not found|invalid experience|experience is disabled|"
                                 "experience is suspended|unknown error|experience data quota exceeded|key-value store is disabled|"
                                 "key-value store communication failed|key doesn't exist|retry update|experience content rating too high|"
                                 "not allowed to run on this land|experience permissions request timed out|unknown error|unknown error|unknown error"}));
  // the messages need no experience
  QVERIFY(!run("default { timer() { } }").contains("experience"));
}

void LslSteps19Test::details()
{
  // %1 is the key of the experience and %2 another key
  const QString source = QString(R"lsl(default { state_entry() {
    llOwnerSay(llList2CSV(llGetExperienceDetails(NULL_KEY)));
    llOwnerSay(llList2CSV(llGetExperienceDetails("%1")) + "|" + llList2CSV(llGetExperienceDetails("%2")));
    list d = llGetExperienceDetails(NULL_KEY);
    llOwnerSay((string)llGetListLength(d) + " " + (string)llGetListEntryType(d, 0) + (string)llGetListEntryType(d, 1) + (string)llGetListEntryType(d, 2) +
               (string)llGetListEntryType(d, 3) + (string)llGetListEntryType(d, 4) + (string)llGetListEntryType(d, 5));
  } })lsl");
  const QString standard = QString("Sandbox Experience, %1, %2, 0, no error, %3").arg(OWNER, EXPERIENCE_KEY, NULL_KEY);
  // name and message are strings (3), owner, id and group keys (4), the state an integer (1)
  QCOMPARE(said(run(source.arg(EXPERIENCE_KEY, KEY_D), experience())), QStringList({standard, standard + "|", "6 344134"}));
  const QString mine = QString("My Place, %1, %2, 9, experience is suspended, %3").arg(KEY_A, KEY_C, KEY_B);
  const QJsonObject custom = experience({{"key", KEY_C}, {"name", "My Place"}, {"owner", KEY_A}, {"group", KEY_B}, {"state", "suspended"}});
  QCOMPARE(said(run(source.arg(KEY_C, KEY_D), custom)), QStringList({mine, mine + "|", "6 344134"}));
  // the default key is no key of this experience any more
  QCOMPARE(said(run(source.arg(EXPERIENCE_KEY, KEY_D), custom)), QStringList({mine, "|", "6 344134"}));
}

void LslSteps19Test::agentInExperience()
{
  const QString source = body(QString(R"lsl(
    llOwnerSay((string)llAgentInExperience("%1") + (string)llAgentInExperience("%2") + (string)llAgentInExperience(llGetOwner()) + (string)llAgentInExperience("%3") + (string)llAgentInExperience("%4"));
  )lsl").arg(KEY_A, KEY_B, KEY_C, KEY_D));
  const QJsonObject world = QJsonObject{{"avatars", QJsonArray{avatar("Ann", KEY_A), avatar("Bob", KEY_B), avatar("Cy", KEY_C)}}};
  QJsonObject setup = world;
  setup["experience"] = QJsonObject{{"members", QJsonArray{KEY_A, KEY_C, KEY_D}}};
  // Ann and Cy are members and in the region; Bob is in the region but no member; the owner is no member; Dee is a member but not here
  QCOMPARE(said(run(source, setup)), QStringList({"10010"}));
  setup["experience"] = QJsonObject{{"members", QJsonArray{KEY_A}}, {"state", "disabled"}};
  QCOMPARE(said(run(source, setup)), QStringList({"00000"}));
  // with no experience at all
  QCOMPARE(said(run(source, world)), QStringList({"00000"}));
}

void LslSteps19Test::permissionRequests()
{
  const QString source = QString(R"lsl(default {
      state_entry() {
        llRequestExperiencePermissions("%1", "");   // a member
        llRequestExperiencePermissions("%2", "");   // blocked
        llRequestExperiencePermissions("%3", "");   // has not answered
        llRequestExperiencePermissions(llGetOwner(), "");   // has not answered
        llRequestExperiencePermissions("%4", "");   // nobody of that key
        llOwnerSay("asked");
        llSetTimerEvent(4);
      }
      timer() { llSetTimerEvent(0); llRequestExperiencePermissions("%3", ""); }
      experience_permissions(key id) { llOwnerSay("granted " + llGetSubString((string)id, 0, 7)); }
      experience_permissions_denied(key id, integer why) { llOwnerSay("denied " + llGetSubString((string)id, 0, 7) + " " + (string)why); }
    })lsl").arg(KEY_A, KEY_B, KEY_C, KEY_D);
  const QJsonObject setup{{"avatars", QJsonArray{avatar("Ann", KEY_A), avatar("Bob", KEY_B), avatar("Cy", KEY_C)}},
                          {"experience", QJsonObject{{"members", QJsonArray{KEY_A}}, {"blocked", QJsonArray{KEY_B}}}}};
  const QJsonArray inputs{QJsonObject{{"at", 1}, {"type", "experience"}, {"kind", "allow"}, {"id", KEY_C}},
                          QJsonObject{{"at", 2}, {"type", "experience"}, {"kind", "deny"}},
                          QJsonObject{{"at", 2.5}, {"type", "experience"}, {"kind", "allow"}, {"id", KEY_D}}};
  const QJsonObject a = run(source, setup, inputs, 6);
  QCOMPARE(said(a), QStringList({"asked", "granted aaaaaaaa", "denied bbbbbbbb 4", "granted cccccccc", "denied a2e76fcd 4", "granted cccccccc"}));
  const QJsonObject x = a["experience"].toObject();
  QCOMPARE(x["members"].toArray(), QJsonArray({KEY_A, KEY_C, KEY_D}));
  QCOMPARE(x["blocked"].toArray(), QJsonArray({KEY_B, OWNER}));
  QJsonArray results;
  for (const QJsonValue& v : ofType(a, "experience")) results.append(v.toObject()["result"]);
  QCOMPARE(results, QJsonArray({"granted", "denied", "asked", "asked", "nobody", "granted"}));
}

void LslSteps19Test::permissionRequestRefusals()
{
  const QString source = body(R"lsl(llRequestExperiencePermissions(llGetOwner(), "");)lsl")
      .replace("default {", R"lsl(default {
      experience_permissions(key id) { llOwnerSay("granted"); }
      experience_permissions_denied(key id, integer why) { llOwnerSay("denied " + (string)why + " " + llGetExperienceErrorMessage(why)); })lsl");
  QCOMPARE(said(run(source, experience({{"state", "disabled"}, {"members", QJsonArray{OWNER}}}))), QStringList({"denied 8 experience is disabled"}));
  QCOMPARE(said(run(source, experience({{"state", "suspended"}}))), QStringList({"denied 9 experience is suspended"}));
  QCOMPARE(said(run(source, experience({{"blocked", QJsonArray{OWNER}}}))), QStringList({"denied 4 operation not permitted"}));
  QCOMPARE(said(run(source, experience({{"members", QJsonArray{OWNER}}}))), QStringList({"granted"}));
  // an avatar who is asked and never answers: nothing comes, and the request is listed in the log
  const QJsonObject pending = run(source, experience());
  QCOMPARE(said(pending), QStringList());
  QCOMPARE(ofType(pending, "experience")[0].toObject()["result"].toString(), QString("asked"));
}

void LslSteps19Test::simStats()
{
  const QString source = body(R"lsl(
    llOwnerSay((string)llGetSimStats(SIM_STAT_AGENT_COUNT) + " " + (string)llGetSimStats(SIM_STAT_ACTIVE_SCRIPT_COUNT) + " " + (string)llGetSimStats(SIM_STAT_PHYSICS_FPS));
    llOwnerSay((string)llGetSimStats(SIM_STAT_PCT_CHARS_STEPPED) + " " + (string)llGetSimStats(SIM_STAT_SCRIPT_RUN_PCT) + " " + (string)llGetSimStats(SIM_STAT_CHILD_AGENT_COUNT));
    llOwnerSay((string)llGetSimStats(SIM_STAT_FRAME_MS) + " " + (string)llGetSimStats(SIM_STAT_AI_MS) + " " + (string)llGetSimStats(27) + " " + (string)llGetSimStats(-1));
  )lsl");
  QCOMPARE(said(run(source)), QStringList({"1.000000 1.000000 45.000000", "0.000000 100.000000 0.000000", "22.000000 0.000000 0.000000 0.000000"}));
  // three avatars (the owner and two more) and two scripts
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 1}, {"avatars", QJsonArray{avatar("Ann", KEY_A), avatar("Bob", KEY_B)}},
                      {"region", QJsonObject{{"name", "Quiet"}}},
                      {"objects", QJsonArray{QJsonObject{{"name", "One"}, {"source", body("llOwnerSay((string)llGetSimStats(SIM_STAT_AGENT_COUNT) + \" \" + (string)llGetSimStats(SIM_STAT_ACTIVE_SCRIPT_COUNT));")}},
                                             QJsonObject{{"name", "Two"}, {"source", "default { timer() { } }"}}}}};
  QCOMPARE(said(Control::run(request)), QStringList({"3.000000 2.000000"}));
}

void LslSteps19Test::profiler()
{
  const QString source = R"lsl(integer burn(integer n) {
      list l;
      integer i;
      for (i = 0; i < n; ++i) l += [i, "text"];
      return llGetListLength(l);
    }
    default {
      state_entry() {
        integer a = llGetSPMaxMemory();
        burn(20);
        integer b = llGetSPMaxMemory();           // not profiling: still 0
        llScriptProfiler(PROFILE_SCRIPT_MEMORY);
        integer c = llGetSPMaxMemory();           // the memory in use now
        burn(20);
        integer d = llGetSPMaxMemory();
        burn(200);
        integer e = llGetSPMaxMemory();
        llScriptProfiler(PROFILE_NONE);
        burn(400);
        integer f = llGetSPMaxMemory();           // the peak is kept, and not raised
        llOwnerSay((string)a + " " + (string)b + " " + (string)(c > 0) + " " + (string)(c <= llGetUsedMemory()) + " " + (string)(d > c) + " " + (string)(e > d) + " " + (string)(f == e));
        llScriptProfiler(PROFILE_SCRIPT_MEMORY);   // a new run starts from the memory in use now
        integer g = llGetSPMaxMemory();
        llOwnerSay((string)(g < e) + " " + (string)(g > 0));
      }
    })lsl";
  QCOMPARE(said(run(source)), QStringList({"0 0 1 1 1 1 1", "1 1"}));
}

void LslSteps19Test::estateAccess()
{
  const QString source = body(QString(R"lsl(
    list r;
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "%1");
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "%1");      // already there
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "%2");
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_REMOVE, "%1");
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_REMOVE, "%3");   // not there
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_GROUP_ADD, "%3");
    r += llManageEstateAccess(ESTATE_ACCESS_BANNED_AGENT_ADD, "%1");
    r += llManageEstateAccess(ESTATE_ACCESS_BANNED_AGENT_ADD, "%4");
    r += llManageEstateAccess(ESTATE_ACCESS_BANNED_AGENT_REMOVE, "%4");
    llOwnerSay(llList2CSV(r));
    r = [];
    r += llManageEstateAccess(3, "%1");                                    // no such action
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD | ESTATE_ACCESS_BANNED_AGENT_ADD, "%1");   // two at once
    r += llManageEstateAccess(0, "%1");
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, NULL_KEY);
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "");
    r += llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "not-a-key");
    llOwnerSay(llList2CSV(r));
  )lsl").arg(KEY_A, KEY_B, KEY_C, KEY_D));
  const QJsonObject a = run(source);
  QCOMPARE(said(a), QStringList({"1, 1, 1, 1, 1, 1, 1, 1, 1", "0, 0, 0, 0, 0, 0"}));
  const QJsonObject estate = a["estate"].toObject();
  QCOMPARE(estate["allowed_agents"].toArray(), QJsonArray({KEY_B}));
  QCOMPARE(estate["allowed_groups"].toArray(), QJsonArray({KEY_C}));
  QCOMPARE(estate["banned_agents"].toArray(), QJsonArray({KEY_A}));
  QCOMPARE(estate["mainland"].toBool(), false);
  // the log: the nine real changes or not
  QJsonArray changed;
  for (const QJsonValue& v : ofType(a, "estate_access")) changed.append(v.toObject()["changed"]);
  QCOMPARE(changed, QJsonArray({true, false, true, true, false, true, true, true, true}));
  QCOMPARE(ofType(a, "estate_access")[0].toObject()["action"].toInt(), 4);
  // the setup: lists that are given stay and are changed
  const QJsonObject b = run(body(QString(R"lsl(llOwnerSay((string)llManageEstateAccess(ESTATE_ACCESS_BANNED_AGENT_REMOVE, "%1"));)lsl").arg(KEY_A)),
                            QJsonObject{{"estate", QJsonObject{{"banned_agents", QJsonArray{KEY_A, KEY_B}}, {"allowed_agents", QJsonArray{KEY_C}}}}});
  QCOMPARE(said(b), QStringList({"1"}));
  QCOMPARE(b["estate"].toObject()["banned_agents"].toArray(), QJsonArray({KEY_B}));
  QCOMPARE(b["estate"].toObject()["allowed_agents"].toArray(), QJsonArray({KEY_C}));
}

void LslSteps19Test::estateRights()
{
  const QString source = body(QString(R"lsl(llOwnerSay((string)llManageEstateAccess(ESTATE_ACCESS_BANNED_AGENT_ADD, "%1"));)lsl").arg(KEY_A));
  // the estate belongs to somebody else: the owner of the object is no manager
  QJsonObject a = run(source, QJsonObject{{"estate", QJsonObject{{"owner", KEY_B}}}});
  QCOMPARE(said(a), QStringList({"0"}));
  QCOMPARE(ofType(a, "debug").size(), qsizetype(1));
  QCOMPARE(ofType(a, "debug")[0].toObject()["text"].toString(), QString("llManageEstateAccess object owner must manage estate."));
  QCOMPARE(ofType(a, "debug")[0].toObject()["channel"].toInt(), 0x7FFFFFFF);
  QCOMPARE(a["estate"].toObject()["banned_agents"].toArray(), QJsonArray());
  // a manager may
  a = run(source, QJsonObject{{"estate", QJsonObject{{"owner", KEY_B}, {"managers", QJsonArray{OWNER}}}}});
  QCOMPARE(said(a), QStringList({"1"}));
  QCOMPARE(ofType(a, "debug").size(), qsizetype(0));
  QCOMPARE(a["estate"].toObject()["banned_agents"].toArray(), QJsonArray({KEY_A}));
  // the estate owner may
  a = run(source, QJsonObject{{"estate", QJsonObject{{"owner", OWNER}}}});
  QCOMPARE(said(a), QStringList({"1"}));
  // mainland: nothing works, with its own message
  a = run(source, QJsonObject{{"estate", QJsonObject{{"mainland", true}}}});
  QCOMPARE(said(a), QStringList({"0"}));
  QCOMPARE(ofType(a, "debug")[0].toObject()["text"].toString(), QString("llManageEstateAccess does not work on mainland"));
  // a bad action or key is refused before the rights are looked at: no message
  a = run(body(R"lsl(llOwnerSay((string)llManageEstateAccess(3, NULL_KEY));)lsl"), QJsonObject{{"estate", QJsonObject{{"owner", KEY_B}}}});
  QCOMPARE(said(a), QStringList({"0"}));
  QCOMPARE(ofType(a, "debug").size(), qsizetype(0));
  // two objects, one owned by somebody else than the estate owner
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 1}, {"estate", QJsonObject{{"owner", OWNER}}},
                      {"objects", QJsonArray{QJsonObject{{"name", "Mine"}, {"source", "default { state_entry() { llOwnerSay(\"mine \" + (string)llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, \"" + QString(KEY_A) + "\")); } }"}},
                                             QJsonObject{{"name", "Theirs"}, {"owner", KEY_B}, {"source", "default { state_entry() { llOwnerSay(\"theirs \" + (string)llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, \"" + QString(KEY_C) + "\")); } }"}}}}};
  const QJsonObject two = Control::run(request);
  QCOMPARE(said(two), QStringList({"mine 1", "theirs 0"}));
}

void LslSteps19Test::estateSilentAndLogged()
{
  const QString notified = body(QString(R"lsl(llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "%1");)lsl").arg(KEY_A));
  QCOMPARE(ofType(run(notified), "estate_access")[0].toObject()["owner_notified"].toBool(), true);
  const QString silent = QString(R"lsl(default {
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_SILENT_ESTATE_MANAGEMENT); }
      run_time_permissions(integer p) { llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "%1"); }
    })lsl").arg(KEY_A);
  const QJsonObject a = run(silent);
  QCOMPARE(ofType(a, "estate_access").size(), qsizetype(1));
  QCOMPARE(ofType(a, "estate_access")[0].toObject()["owner_notified"].toBool(), false);
  QCOMPARE(ofType(a, "estate_access")[0].toObject()["id"].toString(), QString(KEY_A));
  // another permission does not silence it
  const QString other = QString(silent).replace("PERMISSION_SILENT_ESTATE_MANAGEMENT", "PERMISSION_DEBIT");
  QCOMPARE(ofType(run(other), "estate_access")[0].toObject()["owner_notified"].toBool(), true);
}

// 30 calls in 30 simulated seconds; the 31st is refused, and calls are allowed again once the first is 30 seconds old
void LslSteps19Test::estateThrottle()
{
  const QString source = QString(R"lsl(integer ok() { return llManageEstateAccess(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "%1"); }
    default {
      state_entry() {
        integer i; integer good;
        for (i = 0; i < 31; ++i) good += ok();
        llOwnerSay((string)good);
        llSetTimerEvent(29);
      }
      timer() { llOwnerSay("t" + (string)llGetTime() + " " + (string)ok()); if (llGetTime() < 40) llSetTimerEvent(2); else llSetTimerEvent(0); }
    })lsl").arg(KEY_A);
  const QJsonObject a = run(source, QJsonObject(), QJsonArray(), 40);
  const QStringList lines = said(a);
  QCOMPARE(lines.value(0), QString("30"));
  QCOMPARE(lines.value(1), QString("t29.000000 0"));   // 29 s: the first call is still inside the 30 s
  QCOMPARE(lines.value(2), QString("t31.000000 1"));   // 31 s: all of them are older than 30 s
}

void LslSteps19Test::refusedOptions()
{
  auto error = [&](const QJsonObject& extra, const QJsonArray& inputs = QJsonArray())
  {
    const QJsonObject a = run("default { timer() { } }", extra, inputs, 1);
    return a["ok"].toBool() ? QString() : a["error"].toString();
  };
  QVERIFY(error(QJsonObject{{"experience", 5}}).contains("\"experience\" must be an object"));
  QVERIFY(error(experience({{"flavour", 1}})).contains("unknown field \"flavour\""));
  QVERIFY(error(experience({{"key", ""}})).contains("\"key\" must be a key"));
  QVERIFY(error(experience({{"owner", 5}})).contains("\"owner\" must be a key"));
  QVERIFY(error(experience({{"name", ""}})).contains("\"name\" must be text"));
  QVERIFY(error(experience({{"state", "sleeping"}})).contains("\"state\" must be"));
  QVERIFY(error(experience({{"members", "everybody"}})).contains("\"members\" must be a list"));
  QVERIFY(error(experience({{"members", QJsonArray{5}}})).contains("\"members\" must be a list"));
  QVERIFY(error(experience({{"members", QJsonArray{KEY_A}}, {"blocked", QJsonArray{KEY_A}}})).contains("in both"));
  QVERIFY(error(experience({{"store_enabled", "yes"}})).contains("\"store_enabled\" must be true or false"));
  QVERIFY(error(experience({{"quota", 0}})).contains("\"quota\" must be"));
  QVERIFY(error(experience({{"quota", 134217729.0}})).contains("\"quota\" must be"));
  QVERIFY(error(experience({{"store", "x"}})).contains("\"store\" must be an object"));
  QVERIFY(error(experience({{"store", QJsonObject{{"k", 5}}}})).contains("must be text of at most 4095 bytes"));
  QVERIFY(error(experience({{"store", QJsonObject{{"", "v"}}}})).contains("a key of \"store\" must be 1 to 1011 bytes"));
  QVERIFY(error(experience({{"store", QJsonObject{{"k", "vvvvvv"}}}, {"quota", 3}})).contains("larger than \"quota\""));
  QVERIFY(error(QJsonObject{{"estate", "x"}}).contains("\"estate\" must be an object"));
  QVERIFY(error(QJsonObject{{"estate", QJsonObject{{"boss", 1}}}}).contains("unknown field \"boss\""));
  QVERIFY(error(QJsonObject{{"estate", QJsonObject{{"owner", ""}}}}).contains("\"owner\" must be a key"));
  QVERIFY(error(QJsonObject{{"estate", QJsonObject{{"managers", 3}}}}).contains("\"managers\" must be a list"));
  QVERIFY(error(QJsonObject{{"estate", QJsonObject{{"banned_agents", QJsonArray{""}}}}}).contains("\"banned_agents\" must be a list"));
  QVERIFY(error(QJsonObject{{"estate", QJsonObject{{"mainland", 1}}}}).contains("\"mainland\" must be true or false"));
  // the inputs
  auto input = [&](const QJsonObject& in) { return error(QJsonObject(), QJsonArray{in}); };
  QVERIFY(input(QJsonObject{{"type", "experience"}}).contains("needs \"kind\""));
  QVERIFY(input(QJsonObject{{"type", "experience"}, {"kind", "maybe"}}).contains("\"kind\" must be \"allow\", \"deny\", \"set\" or \"delete\""));
  QVERIFY(input(QJsonObject{{"type", "experience"}, {"kind", "allow"}, {"key", "k"}}).contains("only for the experience kinds set and delete"));
  QVERIFY(input(QJsonObject{{"type", "experience"}, {"kind", "set"}}).contains("set input needs \"key\""));
  QVERIFY(input(QJsonObject{{"type", "experience"}, {"kind", "set"}, {"key", "k"}}).contains("set input needs \"value\""));
  QVERIFY(input(QJsonObject{{"type", "experience"}, {"kind", "delete"}, {"key", "k"}, {"value", "v"}}).contains("\"value\" is only for the experience kind set"));
  QVERIFY(input(QJsonObject{{"type", "chat"}, {"key", "k"}}).contains("\"key\" is only for experience"));
  QVERIFY(input(QJsonObject{{"type", "touch"}, {"value", "v"}}).contains("\"value\" is only for experience"));
  QVERIFY(input(QJsonObject{{"type", "touch"}, {"kind", "allow"}}).contains("\"kind\" is only for chat, remote_data and experience"));
  QVERIFY(input(QJsonObject{{"type", "weather"}}).contains("\"experience\", \"relay_release\" or \"relay_ping\"."));
}

void LslSteps19Test::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default {
      state_entry() {
        key a = llCreateKeyValue("k", "v");
        key b = llReadKeyValue("k");
        key c = llUpdateKeyValue("k", "w", TRUE, "v");
        key d = llDeleteKeyValue("k");
        key e = llDataSizeKeyValue();
        key f = llKeyCountKeyValue();
        key g = llKeysKeyValue(0, 5);
        llOwnerSay((string)llAgentInExperience(llGetOwner()) + llList2CSV(llGetExperienceDetails(NULL_KEY)) + llGetExperienceErrorMessage(XP_ERROR_NONE));
        llRequestExperiencePermissions(llGetOwner(), "");
        llOwnerSay((string)llGetSimStats(SIM_STAT_AGENT_COUNT));
        llScriptProfiler(PROFILE_SCRIPT_MEMORY);
        llOwnerSay((string)llManageEstateAccess(ESTATE_ACCESS_BANNED_AGENT_ADD, llGetOwner()));
      }
      dataserver(key q, string d) { llOwnerSay(llGetSubString(d, 0, 0)); }
      experience_permissions(key id) { llOwnerSay("yes"); }
      experience_permissions_denied(key id, integer why) { llOwnerSay(llGetExperienceErrorMessage(why)); }
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QVERIFY2(a["warnings"].toArray().isEmpty(), qPrintable(QString::fromUtf8(QJsonDocument(a["warnings"].toArray()).toJson(QJsonDocument::Compact))));
}

QTEST_GUILESS_MAIN(LslSteps19Test)

#include "tst_lslsteps19.moc"
