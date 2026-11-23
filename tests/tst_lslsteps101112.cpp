/*
 * PinglyMotion - tests for llParcelMediaCommandList, llParcelMediaQuery, the continuous collision event and
 * llReleaseCamera (Script Lab, third gap list run, steps 10 to 12)
 *
 * The expected answers were worked out by hand from the Second Life wiki. Where the wiki is silent or unclear the
 * sandbox chooses; every choice is written here and in docs/CONTROL.md:
 *  - Parcel media is record only (nothing is fetched or played). The sandbox has one parcel with one media state:
 *    URL, TEXTURE (a key, NULL_KEY at the start), TYPE, DESC (strings, empty at the start), SIZE (two integers, 0 and 0),
 *    AUTO_ALIGN (0), LOOP_SET, TIME, AGENT and the status that STOP, PAUSE, PLAY, LOOP and UNLOAD leave.
 *  - llParcelMediaCommandList takes the wiki's command forms: STOP, PAUSE, PLAY, LOOP and UNLOAD alone; URL, TYPE, DESC
 *    with a string, TEXTURE and AGENT with a key, SIZE with two integers, AUTO_ALIGN with an integer, TIME and LOOP_SET
 *    with a float. A list that has an entry that is no command number, no known command, or a command whose value is
 *    missing or of the wrong type stops there (a guess); what came before stays done.
 *  - UNLOAD stops the media and clears URL and TEXTURE (a guess; the wiki only says it unloads the media). STOP also
 *    sets TIME back to 0 (a guess).
 *  - llParcelMediaQuery answers in the order asked: URL, DESC, TYPE as strings, TEXTURE as a key, SIZE as two
 *    integers, AUTO_ALIGN as an integer. Every other command number is skipped. Both functions only work for a
 *    script whose owner owns the parcel (the wiki); otherwise nothing happens and a query gives [].
 *  - The wiki's forced delay of 2 seconds for both functions is not modelled (no function's forced delay is).
 *  - collision is the event while colliding; the "collision" input without "phase" raises it with the same data as
 *    collision_start, in the scripts of the prim hit that have a collision handler, else the root's.
 *  - llReleaseCamera needs PERMISSION_CONTROL_CAMERA (2048) granted to the avatar given; otherwise a line on
 *    DEBUG_CHANNEL (the text is the sandbox's guess) and nothing happens. An avatar that did not grant it is ignored
 *    (a guess). The function is deprecated; nothing in the sandbox has a camera to give back, so it is recorded.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"
#include "scriptlab/lsl/sandbox.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const GUEST = "11111111-2222-3333-4444-555555555555";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";
}

class LslSteps101112Test : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void mediaStartsEmpty();
    void mediaCommandsAndQuery();
    void mediaStatusFollowsCommands();
    void mediaUnloadAndStop();
    void mediaBadListStopsThere();
    void mediaQuerySkipsWhatCannotBeAsked();
    void mediaOnSomebodyElsesParcel();
    void collisionRepeatsBetweenStartAndEnd();
    void collisionGoesToThePrimHit();
    void collisionPhaseErrorsMentionTheContinuousEvent();
    void releaseCameraNeedsPermission();
    void releaseCameraWithPermission();
    void releaseCameraOtherAvatar();
    void checkerHasNoWarnings();

  private:
    static QJsonObject runOne(const QString& body, const QJsonArray& inputs = QJsonArray(), double seconds = 1);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type, const QString& field);
};

QJsonObject LslSteps101112Test::runOne(const QString& body, const QJsonArray& inputs, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() {\n" + body + "\n} }"}, {"inputs", inputs}, {"seconds", seconds}});
}

QStringList LslSteps101112Test::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslSteps101112Test::entries(const QJsonObject& answer, const QString& type, const QString& field)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != type || !e.contains(field)) continue;
    out << (e[field].isString() ? e[field].toString() : QString::fromUtf8(QJsonDocument(QJsonArray{e[field]}).toJson(QJsonDocument::Compact)));
  }
  return out;
}

void LslSteps101112Test::supportedNow()
{
  for (const char* name : {"llParcelMediaCommandList", "llParcelMediaQuery", "llReleaseCamera"})
  {
    QVERIFY2(Builtins::function(name) != nullptr, name);
    QVERIFY2(Builtins::function(name)->supported, name);
  }
  // llNavigateTo is the unsupported example; llWarp2Pos is no LSL function
  QVERIFY(!Builtins::function("llNavigateTo")->supported);
  QVERIFY(Builtins::function("llWarp2Pos") == nullptr);
}

void LslSteps101112Test::mediaStartsEmpty()
{
  const QJsonObject a = runOne(R"lsl(
    list r = llParcelMediaQuery([PARCEL_MEDIA_COMMAND_URL, PARCEL_MEDIA_COMMAND_DESC, PARCEL_MEDIA_COMMAND_TYPE,
                                 PARCEL_MEDIA_COMMAND_TEXTURE, PARCEL_MEDIA_COMMAND_SIZE, PARCEL_MEDIA_COMMAND_AUTO_ALIGN]);
    llOwnerSay((string)llGetListLength(r));
    llOwnerSay(llDumpList2String(r, "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({"7", QString("|||%1|0|0|0").arg(NULL_KEY)}));
}

void LslSteps101112Test::mediaCommandsAndQuery()
{
  const QJsonObject a = runOne(R"lsl(
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_URL, "http://example.com/a.mp4", PARCEL_MEDIA_COMMAND_TYPE, "video/mp4",
                              PARCEL_MEDIA_COMMAND_DESC, "Demo", PARCEL_MEDIA_COMMAND_SIZE, 640, 480,
                              PARCEL_MEDIA_COMMAND_AUTO_ALIGN, 5,
                              PARCEL_MEDIA_COMMAND_TEXTURE, (key)"c0ffee00-0000-4000-8000-000000000009"]);
    list r = llParcelMediaQuery([PARCEL_MEDIA_COMMAND_URL, PARCEL_MEDIA_COMMAND_TYPE, PARCEL_MEDIA_COMMAND_DESC,
                                 PARCEL_MEDIA_COMMAND_SIZE, PARCEL_MEDIA_COMMAND_AUTO_ALIGN, PARCEL_MEDIA_COMMAND_TEXTURE]);
    llOwnerSay(llDumpList2String(r, "|"));
    llOwnerSay((string)llGetListEntryType(r, 3) + " " + (string)llGetListEntryType(r, 4) + " " + (string)llGetListEntryType(r, 6) + " " + (string)llGetListEntryType(r, 0));
    // order asked is order answered
    llOwnerSay(llDumpList2String(llParcelMediaQuery([PARCEL_MEDIA_COMMAND_DESC, PARCEL_MEDIA_COMMAND_URL]), "|"));
  )lsl");
  // AUTO_ALIGN is TRUE or FALSE (5 is TRUE: 1); TYPE_INTEGER 1, TYPE_KEY 4, TYPE_STRING 3
  QCOMPARE(said(a), QStringList({"http://example.com/a.mp4|video/mp4|Demo|640|480|1|c0ffee00-0000-4000-8000-000000000009",
                                 "1 1 4 3", "Demo|http://example.com/a.mp4"}));
}

void LslSteps101112Test::mediaStatusFollowsCommands()
{
  const QJsonObject a = runOne(R"lsl(
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_PLAY]);
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_PAUSE]);
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_LOOP]);
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_TIME, 12.5, PARCEL_MEDIA_COMMAND_LOOP_SET, 3.0, PARCEL_MEDIA_COMMAND_AGENT, (key)"11111111-2222-3333-4444-555555555555", PARCEL_MEDIA_COMMAND_PLAY]);
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_STOP]);
  )lsl");
  QCOMPARE(entries(a, "parcel_media", "status"), QStringList({"play", "pause", "loop", "play", "stop"}));
  QCOMPARE(entries(a, "parcel_media", "op"), QStringList({"command", "command", "command", "command", "command"}));
}

void LslSteps101112Test::mediaUnloadAndStop()
{
  const QJsonObject a = runOne(R"lsl(
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_URL, "http://example.com/a.mp4", PARCEL_MEDIA_COMMAND_TYPE, "video/mp4",
                              PARCEL_MEDIA_COMMAND_TEXTURE, (key)"c0ffee00-0000-4000-8000-000000000009", PARCEL_MEDIA_COMMAND_PLAY]);
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_STOP]);
    llOwnerSay(llDumpList2String(llParcelMediaQuery([PARCEL_MEDIA_COMMAND_URL, PARCEL_MEDIA_COMMAND_TYPE]), "|"));
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_UNLOAD]);
    llOwnerSay(llDumpList2String(llParcelMediaQuery([PARCEL_MEDIA_COMMAND_URL, PARCEL_MEDIA_COMMAND_TYPE, PARCEL_MEDIA_COMMAND_TEXTURE]), "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({"http://example.com/a.mp4|video/mp4", QString("|video/mp4|%1").arg(NULL_KEY)}));
  QCOMPARE(entries(a, "parcel_media", "status"), QStringList({"play", "stop", "stop", "stop", "stop"}));
}

void LslSteps101112Test::mediaBadListStopsThere()
{
  const QJsonObject a = runOne(R"lsl(
    // TIME wants a float: the integer stops the list; URL before it stays, TYPE after it is not reached
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_URL, "u", PARCEL_MEDIA_COMMAND_TIME, 1, PARCEL_MEDIA_COMMAND_TYPE, "t"]);
    llOwnerSay(llDumpList2String(llParcelMediaQuery([PARCEL_MEDIA_COMMAND_URL, PARCEL_MEDIA_COMMAND_TYPE]), "|"));
    // SIZE with one number only
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_SIZE, 10]);
    // a string where a command number belongs, and a number that is no command
    llParcelMediaCommandList(["x", PARCEL_MEDIA_COMMAND_DESC, "d"]);
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_DESC, "d1", 99, PARCEL_MEDIA_COMMAND_DESC, "d2"]);
    llOwnerSay(llDumpList2String(llParcelMediaQuery([PARCEL_MEDIA_COMMAND_DESC, PARCEL_MEDIA_COMMAND_SIZE]), "|"));
  )lsl");
  QCOMPARE(said(a), QStringList({"u|", "d1|0|0"}));
  QCOMPARE(entries(a, "parcel_media", "stopped"),
           QStringList({"TIME needs a float", "SIZE needs two integers", "entry 0 is not a command number", "99 is not a parcel media command"}));
}

void LslSteps101112Test::mediaQuerySkipsWhatCannotBeAsked()
{
  const QJsonObject a = runOne(R"lsl(
    llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_DESC, "d"]);
    list r = llParcelMediaQuery([PARCEL_MEDIA_COMMAND_PLAY, PARCEL_MEDIA_COMMAND_TIME, PARCEL_MEDIA_COMMAND_DESC, PARCEL_MEDIA_COMMAND_LOOP_SET]);
    llOwnerSay((string)llGetListLength(r) + " " + llList2String(r, 0));
    llOwnerSay((string)llGetListLength(llParcelMediaQuery([])));
  )lsl");
  QCOMPARE(said(a), QStringList({"1 d", "0"}));
}

void LslSteps101112Test::mediaOnSomebodyElsesParcel()
{
  // C++ only: no lsl_run option makes the parcel somebody else's yet
  RlvState rlv;
  Sandbox sandbox(rlv, true);
  RegionSettings region;
  region.parcelOwner = "99999999-9999-4999-8999-999999999999";
  sandbox.setRegion(region);
  ObjectSpec spec;
  spec.name = "o";
  Parser::Result parsed = Parser::parse(R"lsl(
    default { state_entry() {
      llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_DESC, "d", PARCEL_MEDIA_COMMAND_PLAY]);
      llOwnerSay("[" + llDumpList2String(llParcelMediaQuery([PARCEL_MEDIA_COMMAND_DESC]), "|") + "]");
    } }
  )lsl");
  QVERIFY(parsed.ok());
  QCOMPARE(sandbox.addObject(spec, std::shared_ptr<const Script>(parsed.script.release())), QString());
  sandbox.start();
  sandbox.runUntil(1.0);
  QStringList said, status;
  for (const QJsonValue& v : sandbox.log())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "owner_say") said << e["text"].toString();
    if (e["type"].toString() == "parcel_media") status << QString("%1 %2 %3").arg(e["op"].toString()).arg(e["allowed"].toBool()).arg(e["status"].toString());
  }
  QCOMPARE(said, QStringList({"[]"}));
  QCOMPARE(status, QStringList({"command 0 stop", "query 0 stop"}));
}

void LslSteps101112Test::collisionRepeatsBetweenStartAndEnd()
{
  const QString source = R"lsl(
    default
    {
      collision_start(integer n) { llOwnerSay("start " + (string)n + " " + llDetectedName(0)); }
      collision(integer n) { llOwnerSay("while " + (string)n + " " + llDetectedName(0) + " " + (string)llDetectedKey(0) + " " + (string)llDetectedLinkNumber(0)); }
      collision_end(integer n) { llOwnerSay("end " + (string)n); }
    }
  )lsl";
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", 5},
      {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "collision"}, {"phase", "start"}, {"name", "Guest Resident"}, {"id", GUEST}},
                            QJsonObject{{"at", 2}, {"type", "collision"}, {"name", "Guest Resident"}, {"id", GUEST}},
                            QJsonObject{{"at", 3}, {"type", "collision"}, {"name", "Guest Resident"}, {"id", GUEST}},
                            QJsonObject{{"at", 4}, {"type", "collision"}, {"phase", "end"}, {"name", "Guest Resident"}, {"id", GUEST}}}}});
  QCOMPARE(said(r), QStringList({"start 1 Guest Resident",
                                 QString("while 1 Guest Resident %1 0").arg(GUEST), QString("while 1 Guest Resident %1 0").arg(GUEST), "end 1"}));
  QCOMPARE(entries(r, "event", "name"), QStringList({"collision_start", "collision", "collision", "collision_end"}));
}

void LslSteps101112Test::collisionGoesToThePrimHit()
{
  auto script = [](const QString& name, const QString& source, int link) { return QJsonObject{{"name", name}, {"source", source}, {"link", link}}; };
  const QString both = "default { collision(integer n) { llOwnerSay(\"hit \" + (string)llDetectedLinkNumber(0)); } }";
  const QString quiet = "default { state_entry() { } }";
  // link 2 has no collision handler of its own here, so the root's gets it; link number 2 is still what is detected
  QJsonObject o{{"name", "box"}, {"prims", 2}, {"scripts", QJsonArray{script("root", both, 1), script("second", quiet, 2)}}};
  QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{o}}, {"seconds", 1},
                                           {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "collision"}, {"link", 2}}}}});
  QCOMPARE(said(r), QStringList({"hit 2"}));
  // a handler on the prim hit gets it, and the root does not
  o = QJsonObject{{"name", "box"}, {"prims", 2}, {"scripts", QJsonArray{script("root", both, 1), script("second", both, 2)}}};
  r = Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{o}}, {"seconds", 1},
                               {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "collision"}, {"link", 2}}}}});
  QCOMPARE(said(r), QStringList({"hit 2"}));
  // a script that only handles collision_start is not woken by the continuous event
  r = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { collision_start(integer n) { llOwnerSay(\"start\"); } }"}, {"seconds", 1},
                               {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "collision"}}}}});
  QCOMPARE(said(r), QStringList());
}

void LslSteps101112Test::collisionPhaseErrorsMentionTheContinuousEvent()
{
  auto error = [](const QJsonObject& input) { return runOne("", QJsonArray{input})["error"].toString(); };
  QCOMPARE(error(QJsonObject{{"type", "collision"}, {"phase", "sideways"}}),
           QString("Input 1: \"phase\" must be \"start\" or \"end\" (leave it out for the continuous collision)."));
  QCOMPARE(error(QJsonObject{{"type", "moving"}}), QString("Input 1: a moving input needs \"phase\": \"start\" or \"end\"."));
  QCOMPARE(error(QJsonObject{{"type", "collision"}}), QString());
}

void LslSteps101112Test::releaseCameraNeedsPermission()
{
  const QJsonObject a = runOne(QString("llReleaseCamera((key)\"%1\"); llOwnerSay(\"after\");").arg(OWNER));
  QCOMPARE(said(a), QStringList({"after"}));
  QCOMPARE(entries(a, "debug", "text"), QStringList({"Script trying to control camera but PERMISSION_CONTROL_CAMERA permission not set"}));
  QCOMPARE(entries(a, "recorded_only", "name").size(), 0);
  // another permission is not enough
  const QJsonObject b = runOne(QString("llRequestPermissions((key)\"%1\", PERMISSION_TRACK_CAMERA); llReleaseCamera((key)\"%1\");").arg(OWNER));
  QCOMPARE(entries(b, "debug", "text").size(), 1);
}

void LslSteps101112Test::releaseCameraWithPermission()
{
  const QJsonObject a = runOne(QString("llRequestPermissions((key)\"%1\", PERMISSION_CONTROL_CAMERA);\n llReleaseCamera((key)\"%1\"); llOwnerSay(\"done\");").arg(OWNER));
  QCOMPARE(said(a), QStringList({"done"}));
  QCOMPARE(entries(a, "debug", "text").size(), 0);
  QCOMPARE(entries(a, "recorded_only", "function"), QStringList({"llReleaseCamera"}));
}

void LslSteps101112Test::releaseCameraOtherAvatar()
{
  const QJsonObject a = runOne(QString("llRequestPermissions((key)\"%1\", PERMISSION_CONTROL_CAMERA);\n llReleaseCamera((key)\"%2\"); llOwnerSay(\"done\");").arg(OWNER, GUEST));
  QCOMPARE(said(a), QStringList({"done"}));
  QCOMPARE(entries(a, "debug", "text").size(), 0);
  QCOMPARE(entries(a, "recorded_only", "function").size(), 0);
  QCOMPARE(entries(a, "camera", "released"), QStringList({"[false]"}));
}

void LslSteps101112Test::checkerHasNoWarnings()
{
  QVERIFY(Parser::eventIsRaised("collision"));
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default {
      state_entry() {
        llParcelMediaCommandList([PARCEL_MEDIA_COMMAND_PLAY]);
        list q = llParcelMediaQuery([PARCEL_MEDIA_COMMAND_URL]);
      }
      collision(integer n) { }
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  // llReleaseCamera is deprecated in Second Life; the checker may say so, but never that it is unsupported
  const QJsonObject b = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", "default { state_entry() { llReleaseCamera(NULL_KEY); } }"}});
  QVERIFY(b["valid"].toBool());
  for (const QJsonValue& w : b["warnings"].toArray()) QVERIFY(!w.toObject()["message"].toString().contains("not supported"));
}

QTEST_GUILESS_MAIN(LslSteps101112Test)
#include "tst_lslsteps101112.moc"
