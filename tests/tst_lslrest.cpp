/*
 * PinglyMotion - tests for the leftover reads and media functions (Script Lab, second gap list)
 *
 * Display names, simulator data, llGetEnv, llGetGMTclock, llSetContentType, the link media family and the touch face
 * data of touch events, plus OBJECT_HEALTH to OBJECT_VOLUME_DETECT (50 to 56) of llGetObjectDetails. Nothing is fetched
 * or played and nothing is shown: the answers are canned. Every expected value was worked out by hand from the rules
 * in docs/CONTROL.md ("Leftover reads and media"). Where the Second Life wiki is unclear the function gives its
 * documented return type with a harmless value:
 *  - llGetDisplayName gives the name without the last name Resident, "" for a key that is no avatar of the sandbox.
 *  - llRequestDisplayName and llRequestUsername give no dataserver answer for a key that is no avatar of the sandbox
 *    (the wiki does not say what the grid does); llRequestUsername was not in the job list but is the same family.
 *  - llRequestSimulatorData: DATA_SIM_POS for another region gets no answer (the wiki gives no position), any other
 *    DATA_SIM_* number gets none; status and rating of another region are "unknown" and "UNKNOWN", as the wiki says.
 *  - llGetEnv gives "" for a key the wiki does not list; region_name is not on the wiki's list, so it gives "".
 *  - llGetLinkMedia with a set of links (LINK_SET and the like) gives the empty list, as llGetLinkPrimitiveParams.
 *  - The touch data (face 0, UV and ST <0.5, 0.5, 0>, normal <0, 0, 1>, binormal <1, 0, 0>, the prim's position) is
 *    the sandbox's choice: it does not simulate where on the face a click lands.
 *  - OBJECT_HEALTH, OBJECT_DAMAGE, OBJECT_DAMAGE_TYPE, OBJECT_LOCKED and OBJECT_VOLUME_DETECT are 0 (no damage,
 *    nothing locked, llVolumeDetect is record only).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/world.h"

using namespace lsl;

class LslRestTest : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void displayNames();
    void displayNameRequests();
    void simulatorData();
    void gmtClock();
    void environment();
    void contentTypeIsRecorded();
    void linkMediaSinglePrim();
    void linkMediaSeveralPrims();
    void linkMediaErrors();
    void linkMediaWhitelist();
    void linkMediaRecorded();
    void touchFaceData();
    void touchDataOutsideTouch();
    void objectDetails50to56();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& body, const QJsonObject& extra = QJsonObject(), double seconds = 2);
    static QJsonObject runSource(const QString& source, const QJsonObject& extra = QJsonObject(), double seconds = 2);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
};

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const OBJECT = "c0ffee00-0000-4000-8000-000000000002";
  const char* const GUEST = "11111111-2222-3333-4444-555555555555";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";
}

QJsonObject LslRestTest::runSource(const QString& source, const QJsonObject& extra, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}};
  for (auto it = extra.begin(); it != extra.end(); ++it) request[it.key()] = it.value();
  return Control::run(request);
}

QJsonObject LslRestTest::run(const QString& body, const QJsonObject& extra, double seconds)
{
  return runSource("default { state_entry() {\n" + body + "\n} }", extra, seconds);
}

QStringList LslRestTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslRestTest::entries(const QJsonObject& answer, const QString& type)
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

void LslRestTest::supportedNow()
{
  for (const char* f : {"llGetDisplayName", "llRequestDisplayName", "llRequestUsername", "llRequestSimulatorData", "llGetGMTclock", "llGetEnv",
                        "llSetContentType", "llSetLinkMedia", "llGetLinkMedia", "llClearLinkMedia", "llDetectedTouchFace", "llDetectedTouchUV",
                        "llDetectedTouchST", "llDetectedTouchNormal", "llDetectedTouchBinormal", "llDetectedTouchPos"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

void LslRestTest::displayNames()
{
  const QJsonObject a = run(QString(R"lsl(
    llOwnerSay(llGetDisplayName("%1"));
    llOwnerSay(llGetDisplayName("%2"));
    llOwnerSay(llGetDisplayName("%3"));
    llOwnerSay("[" + llGetDisplayName("%4") + "]");
    llOwnerSay("[" + llGetDisplayName(NULL_KEY) + "]");
  )lsl").arg(OWNER, GUEST, "66666666-7777-8888-9999-000000000000", OBJECT),
    QJsonObject{{"avatars", QJsonArray{QJsonObject{{"name", "Guest Resident"}, {"key", GUEST}},
                                       QJsonObject{{"name", "Ann Smith"}, {"key", "66666666-7777-8888-9999-000000000000"}}}}});
  QCOMPARE(said(a), QStringList({"Owner", "Guest", "Ann Smith", "[]", "[]"}));
}

void LslRestTest::displayNameRequests()
{
  const QString source = QString(R"lsl(
    list asked;
    default {
      state_entry() {
        asked = [llRequestDisplayName("%1"), llRequestUsername("%2"), llRequestDisplayName("%2"), llRequestUsername("%3"),
                 llRequestDisplayName(NULL_KEY)];
        llOwnerSay((string)llGetListLength(asked));
      }
      dataserver(key id, string data) { llOwnerSay((string)llListFindList(asked, [id]) + " " + data); }
    })lsl").arg(OWNER, GUEST, OBJECT);
  const QJsonObject b = runSource(source, QJsonObject{{"avatars", QJsonArray{QJsonObject{{"name", "Guest Resident"}, {"key", GUEST}}}}});
  // the object key and NULL_KEY get no answer; the owner "Owner" has the user name "owner"
  QCOMPARE(said(b), QStringList({"5", "0 Owner", "1 guest", "2 Guest"}));
  QStringList functions;
  for (const QJsonValue& v : b["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != "dataserver_request") continue;
    functions << e["function"].toString() + " " + e["avatar"].toString();
    QCOMPARE(e["query"].toString().size(), 36);
  }
  QCOMPARE(functions, QStringList({QString("llRequestDisplayName ") + OWNER, QString("llRequestUsername ") + GUEST,
                                   QString("llRequestDisplayName ") + GUEST, QString("llRequestUsername ") + OBJECT,
                                   QString("llRequestDisplayName ") + NULL_KEY}));
}

void LslRestTest::simulatorData()
{
  const QString source = R"lsl(
    list asked;
    default {
      state_entry() {
        asked = [llRequestSimulatorData("Sandbox", DATA_SIM_POS), llRequestSimulatorData("sandbox", DATA_SIM_STATUS),
                 llRequestSimulatorData("SANDBOX", DATA_SIM_RATING), llRequestSimulatorData("Elsewhere", DATA_SIM_POS),
                 llRequestSimulatorData("Elsewhere", DATA_SIM_STATUS), llRequestSimulatorData("Elsewhere", DATA_SIM_RATING),
                 llRequestSimulatorData("Sandbox", 99), llRequestSimulatorData("Sandbox", 128)];
        llOwnerSay((string)llGetListLength(asked));
      }
      dataserver(key id, string data) { llOwnerSay((string)llListFindList(asked, [id]) + " " + data); }
    })lsl";
  const QJsonObject a = runSource(source);
  // the position of Elsewhere and the numbers 99 and 128 get no answer
  QCOMPARE(said(a), QStringList({"8", "0 <256000.00000, 256000.00000, 0.00000>", "1 up", "2 PG", "4 unknown", "5 UNKNOWN"}));
  // another region name and corner: the asked name "Sandbox" is now another region, so status and rating say unknown
  const QJsonObject b = runSource(source, QJsonObject{{"region", QJsonObject{{"name", "Hovly"}, {"corner", QJsonArray{256, 512, 0}}}}});
  QCOMPARE(said(b), QStringList({"8", "1 unknown", "2 UNKNOWN", "4 unknown", "5 UNKNOWN"}));
  const QJsonObject c = runSource(R"lsl(default {
      state_entry() { llRequestSimulatorData("Hovly", DATA_SIM_POS); }
      dataserver(key id, string data) { llOwnerSay(data); }
    })lsl", QJsonObject{{"region", QJsonObject{{"name", "Hovly"}, {"corner", QJsonArray{256, 512, 0}}}}});
  QCOMPARE(said(c), QStringList({"<256.00000, 512.00000, 0.00000>"}));
  // the request entry in full
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != "dataserver_request") continue;
    QCOMPARE(e["function"].toString(), QString("llRequestSimulatorData"));
    QCOMPARE(e["region"].toString(), QString("Sandbox"));
    QCOMPARE(e["data"].toInt(), 5);
    break;
  }
}

void LslRestTest::gmtClock()
{
  // the start time of the default region is 2026-01-01 00:00:00 UTC: the clock starts at 0 and counts whole seconds
  QCOMPARE(said(run(R"lsl(
    llOwnerSay((string)llGetGMTclock());
    llSleep(2.5);
    llOwnerSay((string)llGetGMTclock());
  )lsl")), QStringList({"0.000000", "2.000000"}));
  // a start 3 h 25 min 7 s after midnight is 12307 s; a start at 23:59:59 plus 3 s wraps over midnight to 2
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetGMTclock());
    llSleep(3.0);
    llOwnerSay((string)llGetGMTclock());
  )lsl", QJsonObject{{"region", QJsonObject{{"unix_start", 1767225600 + 12307}}}});
  QCOMPARE(said(a), QStringList({"12307.000000", "12310.000000"}));
  const QJsonObject b = run(R"lsl(
    llSleep(3.0);
    llOwnerSay((string)llGetGMTclock());
  )lsl", QJsonObject{{"region", QJsonObject{{"unix_start", 1767225600 + 86399}}}});
  QCOMPARE(said(b), QStringList({"2.000000"}));
}

void LslRestTest::environment()
{
  const QJsonObject a = run(R"lsl(
    list keys = ["sim_channel", "sim_version", "dynamic_pathfinding", "estate_id", "estate_name", "agent_limit", "region_idle",
                 "region_cpu_ratio", "region_product_name", "region_rating", "region_start_time", "frame_number", "simulator_hostname",
                 "whisper_range", "chat_range", "shout_range", "death_action", "region_name", "nonsense", ""];
    integer i;
    for (i = 0; i < llGetListLength(keys); ++i) llOwnerSay(llList2String(keys, i) + "=[" + llGetEnv(llList2String(keys, i)) + "]");
    llSleep(1.0);
    llOwnerSay(llGetEnv("frame_number"));
  )lsl");
  QCOMPARE(said(a), QStringList({
    "sim_channel=[Second Life Server]", "sim_version=[10.11.30.215699]", "dynamic_pathfinding=[enabled]", "estate_id=[1]",
    "estate_name=[mainland]", "agent_limit=[40]", "region_idle=[0]", "region_cpu_ratio=[1]", "region_product_name=[Estate / Full Region]",
    "region_rating=[PG]", "region_start_time=[1767225600]", "frame_number=[1]", "simulator_hostname=[sandbox.invalid]",
    "whisper_range=[10]", "chat_range=[20]", "shout_range=[100]", "death_action=[0]", "region_name=[]", "nonsense=[]", "=[]",
    "46"}));
  // the start time follows the region setting
  QCOMPARE(said(run("llOwnerSay(llGetEnv(\"region_start_time\"));", QJsonObject{{"region", QJsonObject{{"unix_start", 1800000000}}}})),
           QStringList({"1800000000"}));
}

void LslRestTest::contentTypeIsRecorded()
{
  const QJsonObject a = run(R"lsl(
    llSetContentType("11111111-2222-3333-4444-555555555555", CONTENT_TYPE_JSON);
    llOwnerSay("done");
  )lsl");
  QCOMPARE(said(a), QStringList({"done"}));
  QCOMPARE(entries(a, "recorded_only"), QStringList({
    R"({"args":["11111111-2222-3333-4444-555555555555",5],"function":"llSetContentType","note":"Recorded only: the sandbox plays and shows nothing for this.","type":"recorded_only"})"}));
}

void LslRestTest::linkMediaSinglePrim()
{
  // a single prim: link 0 and LINK_THIS reach it; what llSetLinkMedia sets, llGetLinkMedia and llGetPrimMediaParams read back
  const QJsonObject a = run(R"lsl(
    llOwnerSay("none " + llDumpList2String(llGetLinkMedia(0, 2, [PRIM_MEDIA_WIDTH_PIXELS]), ","));
    llOwnerSay("set " + (string)llSetLinkMedia(LINK_THIS, 2, [PRIM_MEDIA_CURRENT_URL, "https://example.com/a", PRIM_MEDIA_WIDTH_PIXELS, 640, PRIM_MEDIA_AUTO_PLAY, 5]));
    llOwnerSay("get " + llDumpList2String(llGetLinkMedia(LINK_THIS, 2, [PRIM_MEDIA_WIDTH_PIXELS, PRIM_MEDIA_CURRENT_URL, PRIM_MEDIA_AUTO_PLAY, PRIM_MEDIA_CONTROLS]), ","));
    llOwnerSay("same " + llDumpList2String(llGetPrimMediaParams(2, [PRIM_MEDIA_WIDTH_PIXELS, PRIM_MEDIA_CURRENT_URL]), ","));
    llOwnerSay("other face " + (string)llGetListLength(llGetLinkMedia(0, 3, [PRIM_MEDIA_WIDTH_PIXELS])));
    llOwnerSay("prim set " + (string)llSetPrimMediaParams(4, [PRIM_MEDIA_HOME_URL, "https://example.com/h"]));
    llOwnerSay("link get " + llDumpList2String(llGetLinkMedia(1, 4, [PRIM_MEDIA_HOME_URL]), ","));
    llOwnerSay("clear " + (string)llClearLinkMedia(LINK_THIS, 2));
    llOwnerSay("after clear " + (string)llGetListLength(llGetLinkMedia(0, 2, [PRIM_MEDIA_WIDTH_PIXELS])));
    llOwnerSay("still " + (string)llGetListLength(llGetPrimMediaParams(4, [PRIM_MEDIA_HOME_URL])));
  )lsl");
  QCOMPARE(said(a), QStringList({
    "none ", "set 0", "get 640,https://example.com/a,1,0", "same 640,https://example.com/a", "other face 0", "prim set 0",
    "link get https://example.com/h", "clear 0", "after clear 0", "still 1"}));
}

void LslRestTest::linkMediaSeveralPrims()
{
  // three prims: LINK_SET and ALL_SIDES reach every prim and face; links and faces keep their own media
  const QString source = R"lsl(default { state_entry() {
    llOwnerSay("set " + (string)llSetLinkMedia(2, 1, [PRIM_MEDIA_HOME_URL, "https://example.com/two"]));
    llOwnerSay("two " + llDumpList2String(llGetLinkMedia(2, 1, [PRIM_MEDIA_HOME_URL]), ","));
    llOwnerSay("root " + (string)llGetListLength(llGetLinkMedia(LINK_ROOT, 1, [PRIM_MEDIA_HOME_URL])));
    llOwnerSay("others " + (string)llSetLinkMedia(LINK_ALL_OTHERS, ALL_SIDES, [PRIM_MEDIA_WIDTH_PIXELS, 100]));
    llOwnerSay("this " + (string)llGetListLength(llGetLinkMedia(LINK_THIS, 0, [PRIM_MEDIA_WIDTH_PIXELS])));
    llOwnerSay("three " + llDumpList2String(llGetLinkMedia(3, 5, [PRIM_MEDIA_WIDTH_PIXELS]), ","));
    llOwnerSay("two again " + llDumpList2String(llGetLinkMedia(2, 1, [PRIM_MEDIA_HOME_URL, PRIM_MEDIA_WIDTH_PIXELS]), ","));
    llOwnerSay("set all " + (string)llSetLinkMedia(LINK_SET, ALL_SIDES, [PRIM_MEDIA_HEIGHT_PIXELS, 50]));
    llOwnerSay("root " + llDumpList2String(llGetLinkMedia(1, 0, [PRIM_MEDIA_WIDTH_PIXELS, PRIM_MEDIA_HEIGHT_PIXELS]), ","));
    llOwnerSay("set group " + llDumpList2String(llGetLinkMedia(LINK_SET, 0, [PRIM_MEDIA_WIDTH_PIXELS]), "") + "|");
    llOwnerSay("clear children " + (string)llClearLinkMedia(LINK_ALL_CHILDREN, ALL_SIDES));
    llOwnerSay("child gone " + (string)llGetListLength(llGetLinkMedia(2, 1, [PRIM_MEDIA_HOME_URL])) + (string)llGetListLength(llGetLinkMedia(3, 5, [PRIM_MEDIA_WIDTH_PIXELS])));
    llOwnerSay("root kept " + (string)llGetListLength(llGetLinkMedia(1, 5, [PRIM_MEDIA_WIDTH_PIXELS])));
  } })lsl";
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1},
    {"objects", QJsonArray{QJsonObject{{"name", "box"}, {"prims", 3}, {"source", source}}}}});
  QCOMPARE(said(r), QStringList({
    "set 0", "two https://example.com/two", "root 0", "others 0", "this 0", "three 100", "two again https://example.com/two,100",
    "set all 0", "root 0,50", "set group |", "clear children 0", "child gone 00", "root kept 1"}));
}

void LslRestTest::linkMediaErrors()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay("bad face " + (string)llSetLinkMedia(0, 6, [PRIM_MEDIA_WIDTH_PIXELS, 1]));
    llOwnerSay("bad face clear " + (string)llClearLinkMedia(0, -2));
    llOwnerSay("no link " + (string)llSetLinkMedia(5, 0, [PRIM_MEDIA_WIDTH_PIXELS, 1]));
    llOwnerSay("no link clear " + (string)llClearLinkMedia(5, 0));
    llOwnerSay("rule " + (string)llSetLinkMedia(0, 0, [PRIM_MEDIA_WIDTH_PIXELS, "wide"]));
    llOwnerSay("unknown rule " + (string)llSetLinkMedia(0, 0, [99, 1]));
    llOwnerSay("too big " + (string)llSetLinkMedia(0, 0, [PRIM_MEDIA_WIDTH_PIXELS, 2049]));
    llOwnerSay("none made " + (string)llGetListLength(llGetLinkMedia(0, 0, [PRIM_MEDIA_WIDTH_PIXELS])));
    llSetLinkMedia(0, 0, [PRIM_MEDIA_WIDTH_PIXELS, 7]);
    llOwnerSay("get bad face " + llDumpList2String(llGetLinkMedia(0, 6, [PRIM_MEDIA_WIDTH_PIXELS]), ","));
    llOwnerSay("get all sides " + llDumpList2String(llGetLinkMedia(0, ALL_SIDES, [PRIM_MEDIA_WIDTH_PIXELS]), ","));
    llOwnerSay("get no link " + llDumpList2String(llGetLinkMedia(5, 0, [PRIM_MEDIA_WIDTH_PIXELS]), ","));
    llOwnerSay("get bad type " + llDumpList2String(llGetLinkMedia(0, 0, ["x"]), ","));
    llOwnerSay("get bad id " + llDumpList2String(llGetLinkMedia(0, 0, [15]), ","));
    llOwnerSay("get set " + (string)llGetListLength(llGetLinkMedia(LINK_SET, 0, [PRIM_MEDIA_WIDTH_PIXELS])));
  )lsl");
  // STATUS_BOUNDS_ERROR 1002, STATUS_NOT_FOUND 1003, STATUS_TYPE_MISMATCH 1001, STATUS_MALFORMED_PARAMS 1000
  QCOMPARE(said(a), QStringList({
    "bad face 1002", "bad face clear 1002", "no link 1003", "no link clear 1003", "rule 1001", "unknown rule 1000", "too big 1002",
    "none made 0", "get bad face 1002", "get all sides 1002", "get no link 1003", "get bad type 1001", "get bad id 1000", "get set 0"}));
}

void LslRestTest::linkMediaWhitelist()
{
  // a whitelist that refuses the address: STATUS_WHITELIST_FAILED, nothing changes on any face
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llSetLinkMedia(0, ALL_SIDES, [PRIM_MEDIA_WHITELIST_ENABLE, TRUE, PRIM_MEDIA_WHITELIST, "example.com", PRIM_MEDIA_CURRENT_URL, "https://other.org/x"]));
    llOwnerSay((string)llGetListLength(llGetLinkMedia(0, 0, [PRIM_MEDIA_CURRENT_URL])) + (string)llGetListLength(llGetLinkMedia(0, 5, [PRIM_MEDIA_CURRENT_URL])));
    llOwnerSay((string)llSetLinkMedia(0, ALL_SIDES, [PRIM_MEDIA_WHITELIST_ENABLE, TRUE, PRIM_MEDIA_WHITELIST, "example.com", PRIM_MEDIA_CURRENT_URL, "https://www.example.com/x"]));
    llOwnerSay(llList2String(llGetLinkMedia(0, 5, [PRIM_MEDIA_CURRENT_URL]), 0));
  )lsl");
  QCOMPARE(said(a), QStringList({"2001", "00", "0", "https://www.example.com/x"}));
}

void LslRestTest::linkMediaRecorded()
{
  const QJsonObject a = run(R"lsl(
    llSetLinkMedia(LINK_SET, 1, [PRIM_MEDIA_WIDTH_PIXELS, 640]);
    llGetLinkMedia(0, 1, [PRIM_MEDIA_WIDTH_PIXELS]);
    llGetLinkMedia(0, 2, [PRIM_MEDIA_WIDTH_PIXELS]);
    llGetLinkMedia(LINK_SET, 1, []);
    llSetLinkMedia(7, 1, []);
    llClearLinkMedia(LINK_THIS, ALL_SIDES);
  )lsl");
  QCOMPARE(entries(a, "prim_media"), QStringList({
    R"({"face":1,"link":-1,"note":"Simulated: nothing is fetched or played.","op":"set","params":[9,640],"result":"STATUS_OK","type":"prim_media"})",
    R"({"face":1,"link":0,"op":"get","params":[9],"result":"ok","type":"prim_media"})",
    R"({"face":2,"link":0,"op":"get","params":[9],"result":"no_media","type":"prim_media"})",
    R"({"face":1,"link":-1,"op":"get","params":[],"result":"link_set","type":"prim_media"})",
    R"({"face":1,"link":7,"note":"Simulated: nothing is fetched or played.","op":"set","params":[],"result":"STATUS_NOT_FOUND","type":"prim_media"})",
    R"({"face":-1,"link":-4,"note":"Simulated: no media was ever loaded.","op":"clear","result":"STATUS_OK","type":"prim_media"})"}));
}

void LslRestTest::touchFaceData()
{
  // inside every touch phase: face 0, UV and ST in the middle, normal up, binormal along x, position of the prim; an index
  // with nobody behind it (here 1, -1 and 5) gives the TOUCH_INVALID_* values
  const QString source = R"lsl(default {
    touch_start(integer n) {
      llOwnerSay("start " + (string)llDetectedTouchFace(0) + " " + (string)llDetectedTouchUV(0) + " " + (string)llDetectedTouchST(0) + " "
        + (string)llDetectedTouchNormal(0) + " " + (string)llDetectedTouchBinormal(0));
      llOwnerSay("pos " + (string)(llDetectedTouchPos(0) == llGetPos()));
      llOwnerSay("nobody " + (string)llDetectedTouchFace(1) + " " + (string)llDetectedTouchUV(1) + " " + (string)llDetectedTouchST(-1) + " "
        + (string)llDetectedTouchNormal(5) + " " + (string)llDetectedTouchBinormal(1) + " " + (string)llDetectedTouchPos(1));
      llOwnerSay("invalid " + (string)(llDetectedTouchFace(1) == TOUCH_INVALID_FACE) + (string)(llDetectedTouchUV(1) == TOUCH_INVALID_TEXCOORD)
        + (string)(llDetectedTouchNormal(1) == TOUCH_INVALID_VECTOR));
    }
    touch(integer n) { llOwnerSay("touch " + (string)llDetectedTouchFace(0)); }
    touch_end(integer n) { llOwnerSay("end " + (string)llDetectedTouchFace(0) + " " + (string)llDetectedTouchUV(0)); }
  })lsl";
  const QJsonObject a = runSource(source, QJsonObject{{"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}}}});
  const QString none = "<-1.00000, -1.00000, 0.00000>";
  const QString zero = "<0.00000, 0.00000, 0.00000>";
  QCOMPARE(said(a), QStringList({
    "start 0 <0.50000, 0.50000, 0.00000> <0.50000, 0.50000, 0.00000> <0.00000, 0.00000, 1.00000> <1.00000, 0.00000, 0.00000>",
    "pos 1",
    "nobody -1 " + none + " " + none + " " + zero + " " + zero + " " + zero,
    "invalid 111",
    "touch 0",
    "end 0 <0.50000, 0.50000, 0.00000>"}));
  // a touch by another avatar gives the same data
  const QJsonObject b = runSource(R"lsl(default { touch_start(integer n) { llOwnerSay(llDetectedName(0) + " " + (string)llDetectedTouchFace(0) + " " + (string)llDetectedTouchST(0)); } })lsl",
    QJsonObject{{"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"name", "Guest Resident"}, {"id", GUEST}}}}});
  QCOMPARE(said(b), QStringList({"Guest Resident 0 <0.50000, 0.50000, 0.00000>"}));
}

void LslRestTest::touchDataOutsideTouch()
{
  // not in a touch event (state_entry, a sensor): TOUCH_INVALID_FACE, TOUCH_INVALID_TEXCOORD and TOUCH_INVALID_VECTOR
  const QString none = "<-1.00000, -1.00000, 0.00000>";
  const QString zero = "<0.00000, 0.00000, 0.00000>";
  const QString expected = "-1 " + none + " " + none + " " + zero + " " + zero + " " + zero;
  const QJsonObject a = runSource(R"lsl(
    string all(integer i) {
      return (string)llDetectedTouchFace(i) + " " + (string)llDetectedTouchUV(i) + " " + (string)llDetectedTouchST(i) + " "
        + (string)llDetectedTouchNormal(i) + " " + (string)llDetectedTouchBinormal(i) + " " + (string)llDetectedTouchPos(i);
    }
    default {
      state_entry() { llOwnerSay("entry " + all(0)); llSensor("", NULL_KEY, AGENT, 96.0, PI); }
      sensor(integer n) { llOwnerSay("sensor " + llDetectedName(0) + " " + all(0)); }
      no_sensor() { llOwnerSay("no sensor"); }
    })lsl");
  QCOMPARE(said(a), QStringList({"entry " + expected, "sensor Owner " + expected}));
}

void LslRestTest::objectDetails50to56()
{
  // 53 and 54 are five integers each: base, owner, group, everyone, next owner (the same as llGetObjectPermMask:
  // PERM_ALL 2147483647 for base and owner, nothing for group and everyone, PERM_MOVE | PERM_TRANSFER 532480 for the next owner)
  const QJsonObject a = run(QString(R"lsl(
    llOwnerSay("health " + llList2CSV(llGetObjectDetails(llGetKey(), [OBJECT_HEALTH, OBJECT_DAMAGE, OBJECT_DAMAGE_TYPE])));
    llOwnerSay("perms " + llList2CSV(llGetObjectDetails(llGetKey(), [OBJECT_PERMS])));
    llOwnerSay("combined " + llList2CSV(llGetObjectDetails(llGetKey(), [OBJECT_PERMS_COMBINED])));
    llOwnerSay("flags " + llList2CSV(llGetObjectDetails(llGetKey(), [OBJECT_LOCKED, OBJECT_VOLUME_DETECT])));
    llOwnerSay("order " + llList2CSV(llGetObjectDetails(llGetKey(), [OBJECT_LOCKED, OBJECT_PERMS, OBJECT_NAME, 57])));
    llOwnerSay("mask " + llList2CSV([llGetObjectPermMask(MASK_BASE), llGetObjectPermMask(MASK_OWNER), llGetObjectPermMask(MASK_GROUP),
                                      llGetObjectPermMask(MASK_EVERYONE), llGetObjectPermMask(MASK_NEXT)]));
    llOwnerSay("avatar " + llList2CSV(llGetObjectDetails("%1", [OBJECT_HEALTH, OBJECT_DAMAGE, OBJECT_DAMAGE_TYPE, OBJECT_PERMS, OBJECT_LOCKED, OBJECT_VOLUME_DETECT])));
    llOwnerSay("nobody " + (string)llGetListLength(llGetObjectDetails(NULL_KEY, [OBJECT_HEALTH, OBJECT_PERMS])));
  )lsl").arg(OWNER));
  const QString perms = "2147483647, 2147483647, 0, 0, 532480";
  QCOMPARE(said(a), QStringList({
    "health 0.000000, 0.000000, 0",
    "perms " + perms,
    "combined " + perms,
    "flags 0, 0",
    "order 0, " + perms + ", Object, -1",
    "mask " + perms,
    "avatar 0.000000, 0.000000, 0, 0, 0, 0, 0, 0, 0, 0",
    "nobody 0"}));
}

void LslRestTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default {
      state_entry() {
        llOwnerSay(llGetDisplayName(llGetOwner()) + (string)llGetGMTclock() + llGetEnv("sim_channel"));
        key a = llRequestDisplayName(llGetOwner()); key b = llRequestUsername(llGetOwner()); key c = llRequestSimulatorData("r", DATA_SIM_RATING);
        llSetContentType(a, CONTENT_TYPE_TEXT);
        integer s = llSetLinkMedia(LINK_THIS, ALL_SIDES, [PRIM_MEDIA_WIDTH_PIXELS, 1]);
        list m = llGetLinkMedia(LINK_THIS, 0, [PRIM_MEDIA_WIDTH_PIXELS]);
        s = llClearLinkMedia(LINK_THIS, ALL_SIDES);
        list d = llGetObjectDetails(llGetKey(), [OBJECT_HEALTH, OBJECT_PERMS, OBJECT_LOCKED]);
      }
      touch_start(integer n) {
        llOwnerSay((string)llDetectedTouchFace(0) + (string)llDetectedTouchUV(0) + (string)llDetectedTouchST(0)
          + (string)llDetectedTouchNormal(0) + (string)llDetectedTouchBinormal(0) + (string)llDetectedTouchPos(0));
      }
      dataserver(key id, string data) { }
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

QTEST_GUILESS_MAIN(LslRestTest)

#include "tst_lslrest.moc"
