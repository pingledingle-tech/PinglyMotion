/*
 * PinglyMotion - tests for the avatar attach/detach and camera functions (Script Lab, gap list, group "avatar and camera")
 *
 * The sandbox has no avatar body, camera or teleport: attach, detach, force-mouselook and teleport calls only write a
 * recorded_only entry (the object's attachment point does not change), the reads give canned answers. Every expected
 * value was worked out by hand from the rules in docs/CONTROL.md ("Avatar and camera"). Where the Second Life wiki is
 * unclear the function gives its documented return type with a harmless canned value:
 *  - The wiki does not give the text of the DEBUG_CHANNEL errors; the sandbox says "Script trying to <do> but
 *    PERMISSION_<X> permission not set", the way the other permission checks of the interpreter do.
 *  - llGetCameraPos and llGetCameraRot without PERMISSION_TRACK_CAMERA fail (the wiki names no value): ZERO_VECTOR and
 *    ZERO_ROTATION. With it the sandbox, which has no camera, answers the owner's place <128, 128, 20> and ZERO_ROTATION.
 *  - llGetAgentLanguage answers "en" for every avatar of the sandbox and "" for any other key (the wiki says the string is
 *    empty when the avatar shares no language; it names no case for an unknown key).
 *  - llGetAttachedList lists the objects of the world that the avatar wears, HUDs left out (the wiki: "by design"); a key
 *    that is no avatar of the sandbox gives ["NOT FOUND"] as the wiki says.
 *  - llTeleportAgent, llTeleportAgentGlobalCoords and llTeleportAgentHome need PERMISSION_TELEPORT and only teleport the
 *    owner (the wiki: unless the script is part of an Experience, which the sandbox does not model); nothing moves.
 *  - llGetCameraAspect and llGetCameraFOV need PERMISSION_TRACK_CAMERA and give 0.0 without it (the wiki); with it they
 *    give canned values: 1.777778 (16:9) and 1.047198 radians (the viewer's default 60 degrees).
 *  - llSetCameraParams records only the length of its list (the way the interpreter records any list argument).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslAvatarTest : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void attachNeedsPermission();
    void attachRecorded();
    void detachRecorded();
    void attachedListAndLanguage();
    void attachedListOfTheWorld();
    void cameraReadsNeedPermission();
    void cameraReadsWithPermission();
    void cameraRecorded();
    void teleportNeedsPermission();
    void teleportRecorded();
    void teleportOnlyTheOwner();
    void cameraViewReads();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& body);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QStringList debugTexts(const QJsonObject& answer);
    static QString rec(const QString& function, const QString& args)
    {
      return QString(R"({"args":%1,"function":"%2","note":"Recorded only: the sandbox plays and shows nothing for this.","type":"recorded_only"})").arg(args, function);
    }
};

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const STRANGER = "11111111-2222-3333-4444-555555555555";
}

QJsonObject LslAvatarTest::run(const QString& body)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() {\n" + body + "\n} }"}, {"seconds", 1}});
}

QStringList LslAvatarTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslAvatarTest::entries(const QJsonObject& answer, const QString& type)
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

QStringList LslAvatarTest::debugTexts(const QJsonObject& answer)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "debug") out << v.toObject()["text"].toString();
  return out;
}

void LslAvatarTest::supportedNow()
{
  for (const char* f : {"llAttachToAvatar", "llAttachToAvatarTemp", "llDetachFromAvatar", "llGetAttached", "llGetAttachedList",
                        "llGetAgentLanguage", "llGetAgentInfo", "llForceMouselook", "llGetCameraPos", "llGetCameraRot",
                        "llSetCameraParams", "llClearCameraParams", "llSetCameraEyeOffset", "llSetCameraAtOffset",
                        "llGetCameraAspect", "llGetCameraFOV", "llTeleportAgentHome", "llTeleportAgent", "llTeleportAgentGlobalCoords"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

void LslAvatarTest::attachNeedsPermission()
{
  const QJsonObject a = run(R"lsl(
    llAttachToAvatar(ATTACH_HEAD);
    llAttachToAvatarTemp(ATTACH_LHAND);
    llDetachFromAvatar();
    llOwnerSay("done");
  )lsl");
  QCOMPARE(said(a), QStringList{"done"});
  QCOMPARE(entries(a, "recorded_only"), QStringList());
  QCOMPARE(debugTexts(a), QStringList(3, "Script trying to attach but PERMISSION_ATTACH permission not set"));
  // another permission is not enough
  const QJsonObject b = run(R"lsl(
    llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION | PERMISSION_TELEPORT);
    llAttachToAvatar(ATTACH_HEAD);
  )lsl");
  QCOMPARE(entries(b, "recorded_only"), QStringList());
  QCOMPARE(debugTexts(b), QStringList{"Script trying to attach but PERMISSION_ATTACH permission not set"});
}

void LslAvatarTest::attachRecorded()
{
  const QJsonObject a = run(R"lsl(
    llRequestPermissions(llGetOwner(), PERMISSION_ATTACH);
    llAttachToAvatar(ATTACH_HEAD);
    llAttachToAvatarTemp(ATTACH_HUD_CENTER_1);
    llOwnerSay((string)llGetAttached());
  )lsl");
  // record only: the object stays where it was rezzed
  QCOMPARE(said(a), QStringList{"0"});
  QCOMPARE(entries(a, "recorded_only"), QStringList({rec("llAttachToAvatar", "[2]"), rec("llAttachToAvatarTemp", "[35]")}));
  QCOMPARE(debugTexts(a), QStringList());
}

void LslAvatarTest::detachRecorded()
{
  const QJsonObject a = run(R"lsl(
    llRequestPermissions(llGetOwner(), PERMISSION_ATTACH);
    llDetachFromAvatar();
  )lsl");
  QCOMPARE(entries(a, "recorded_only"), QStringList{rec("llDetachFromAvatar", "[]")});
}

void LslAvatarTest::attachedListAndLanguage()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay(llDumpList2String(llGetAttachedList(llGetOwner()), ","));
    llOwnerSay((string)llGetListLength(llGetAttachedList(llGetOwner())));
    llOwnerSay(llDumpList2String(llGetAttachedList("11111111-2222-3333-4444-555555555555"), ","));
    llOwnerSay(llDumpList2String(llGetAttachedList(llGetKey()), ","));
    llOwnerSay("[" + llGetAgentLanguage(llGetOwner()) + "]");
    llOwnerSay("[" + llGetAgentLanguage("11111111-2222-3333-4444-555555555555") + "]");
    llOwnerSay("[" + llGetAgentLanguage(NULL_KEY) + "]");
  )lsl");
  QCOMPARE(said(a), QStringList({"", "0", "NOT FOUND", "NOT FOUND", "[en]", "[]", "[]"}));
}

void LslAvatarTest::attachedListOfTheWorld()
{
  // three objects: a head attachment, a HUD, and a free object that asks; a stranger's attachment is not the owner's
  const QJsonObject request{{"command", "lsl_run"}, {"seconds", 2}, {"objects", QJsonArray{
    QJsonObject{{"name", "Asker"}, {"key", "aaaaaaaa-0000-4000-8000-000000000001"}, {"source", R"lsl(
      default { state_entry() { llSetTimerEvent(1.0); }
        timer() { llOwnerSay(llDumpList2String(llGetAttachedList(llGetOwner()), ",")); llOwnerSay(llGetAgentLanguage(llGetOwner()));
                  llOwnerSay("hud " + (string)llGetAttached()); llSetTimerEvent(0.0); } }
    )lsl"}},
    QJsonObject{{"name", "Hat"}, {"key", "aaaaaaaa-0000-4000-8000-000000000002"}, {"attached", 2}, {"source", "default { state_entry() { llOwnerSay((string)llGetAttached()); } }"}},
    QJsonObject{{"name", "Hud"}, {"key", "aaaaaaaa-0000-4000-8000-000000000003"}, {"attached", 35}, {"source", "default { state_entry() { llOwnerSay((string)llGetAttached()); } }"}},
    QJsonObject{{"name", "Wing"}, {"key", "aaaaaaaa-0000-4000-8000-000000000004"}, {"attached", 45}, {"source", "default { state_entry() { llOwnerSay((string)llGetAttached()); } }"}},
    QJsonObject{{"name", "Theirs"}, {"key", "aaaaaaaa-0000-4000-8000-000000000005"}, {"attached", 3}, {"owner", STRANGER}, {"source", "default { state_entry() { llOwnerSay((string)llGetAttached()); } }"}}}}};
  const QJsonObject a = Control::run(request);
  QStringList timerLines;
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "owner_say" && e["object"].toString() == "Asker") timerLines << e["text"].toString();
  }
  QCOMPARE(timerLines, QStringList({"aaaaaaaa-0000-4000-8000-000000000002,aaaaaaaa-0000-4000-8000-000000000004", "en", "hud 0"}));
}

void LslAvatarTest::cameraReadsNeedPermission()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetCameraPos());
    llOwnerSay((string)llGetCameraRot());
    llRequestPermissions(llGetOwner(), PERMISSION_CONTROL_CAMERA);
    llOwnerSay((string)llGetCameraPos());
  )lsl");
  QCOMPARE(said(a), QStringList({"<0.00000, 0.00000, 0.00000>", "<0.00000, 0.00000, 0.00000, 1.00000>", "<0.00000, 0.00000, 0.00000>"}));
  QCOMPARE(debugTexts(a), QStringList(3, "Script trying to track camera but PERMISSION_TRACK_CAMERA permission not set"));
}

void LslAvatarTest::cameraReadsWithPermission()
{
  const QJsonObject a = run(R"lsl(
    llRequestPermissions(llGetOwner(), PERMISSION_TRACK_CAMERA);
    llOwnerSay((string)llGetCameraPos());
    llOwnerSay((string)llGetCameraRot());
    llOwnerSay((string)llGetPermissions());
  )lsl");
  QCOMPARE(said(a), QStringList({"<128.00000, 128.00000, 20.00000>", "<0.00000, 0.00000, 0.00000, 1.00000>", "1024"}));
  QCOMPARE(debugTexts(a), QStringList());
}

void LslAvatarTest::cameraRecorded()
{
  const QJsonObject a = run(R"lsl(
    llSetCameraParams([CAMERA_ACTIVE, 1, CAMERA_DISTANCE, 5.5]);
    llSetCameraEyeOffset(<0, 0, 1>);
    llSetCameraAtOffset(<2, 0, 0.5>);
    llClearCameraParams();
    llForceMouselook(TRUE);
    llForceMouselook(FALSE);
    llOwnerSay("done");
  )lsl");
  QCOMPARE(said(a), QStringList{"done"});
  QCOMPARE(entries(a, "recorded_only"), QStringList({rec("llSetCameraParams", "[4]"), rec("llSetCameraEyeOffset", "[[0,0,1]]"),
                                                     rec("llSetCameraAtOffset", "[[2,0,0.5]]"), rec("llClearCameraParams", "[]"),
                                                     rec("llForceMouselook", "[1]"), rec("llForceMouselook", "[0]")}));
}

void LslAvatarTest::teleportNeedsPermission()
{
  const QJsonObject a = run(R"lsl(
    llTeleportAgentHome(llGetOwner());
    llTeleportAgent(llGetOwner(), "", <10, 10, 30>, <0, 1, 0>);
    llTeleportAgentGlobalCoords(llGetOwner(), <256000, 256000, 0>, <1, 2, 3>, <0, 1, 0>);
    llOwnerSay("done");
  )lsl");
  QCOMPARE(said(a), QStringList{"done"});
  QCOMPARE(entries(a, "recorded_only"), QStringList());
  QCOMPARE(debugTexts(a), QStringList(3, "Script trying to teleport but PERMISSION_TELEPORT permission not set"));
}

void LslAvatarTest::teleportRecorded()
{
  const QJsonObject a = run(R"lsl(
    llRequestPermissions(llGetOwner(), PERMISSION_TELEPORT);
    llTeleportAgentHome(llGetOwner());
    llTeleportAgent(llGetOwner(), "Home landmark", <10, 10, 30>, <0, 1, 0>);
    llTeleportAgentGlobalCoords(llGetOwner(), <256000, 256000, 0>, <1, 2, 3>, <0, 1, 0>);
  )lsl");
  QCOMPARE(entries(a, "recorded_only"),
           QStringList({rec("llTeleportAgentHome", QString(R"(["%1"])").arg(OWNER)),
                        rec("llTeleportAgent", QString(R"(["%1","Home landmark",[10,10,30],[0,1,0]])").arg(OWNER)),
                        rec("llTeleportAgentGlobalCoords", QString(R"(["%1",[256000,256000,0],[1,2,3],[0,1,0]])").arg(OWNER))}));
  QCOMPARE(debugTexts(a), QStringList());
}

void LslAvatarTest::teleportOnlyTheOwner()
{
  const QJsonObject a = run(R"lsl(
    llRequestPermissions(llGetOwner(), PERMISSION_TELEPORT);
    llTeleportAgentHome("11111111-2222-3333-4444-555555555555");
    llTeleportAgent(NULL_KEY, "", <1, 1, 1>, ZERO_VECTOR);
  )lsl");
  QCOMPARE(entries(a, "recorded_only"), QStringList());
  QCOMPARE(debugTexts(a), QStringList({"llTeleportAgentHome: only the owner can be teleported", "llTeleportAgent: only the owner can be teleported"}));
}

void LslAvatarTest::cameraViewReads()
{
  const QJsonObject a = run(R"lsl(
    llOwnerSay((string)llGetCameraAspect() + " " + (string)llGetCameraFOV());
    llRequestPermissions(llGetOwner(), PERMISSION_TRACK_CAMERA);
    llOwnerSay((string)llGetCameraAspect() + " " + (string)llGetCameraFOV());
  )lsl");
  QCOMPARE(said(a), QStringList({"0.000000 0.000000", "1.777778 1.047198"}));
  QCOMPARE(debugTexts(a), QStringList(2, "Script trying to track camera but PERMISSION_TRACK_CAMERA permission not set"));
}

void LslAvatarTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llRequestPermissions(llGetOwner(), PERMISSION_ATTACH | PERMISSION_TRACK_CAMERA | PERMISSION_TELEPORT);
      llAttachToAvatar(ATTACH_HEAD); llAttachToAvatarTemp(ATTACH_HUD_CENTER_1); llDetachFromAvatar();
      llOwnerSay(llDumpList2String(llGetAttachedList(llGetOwner()), ",") + llGetAgentLanguage(llGetOwner()) + (string)llGetAttached());
      llForceMouselook(TRUE);
      llOwnerSay((string)llGetCameraPos() + (string)llGetCameraRot());
      llSetCameraParams([CAMERA_ACTIVE, 1]); llClearCameraParams(); llSetCameraEyeOffset(ZERO_VECTOR); llSetCameraAtOffset(ZERO_VECTOR);
      llOwnerSay((string)llGetCameraAspect() + (string)llGetCameraFOV());
      llTeleportAgentHome(llGetOwner()); llTeleportAgent(llGetOwner(), "", ZERO_VECTOR, ZERO_VECTOR);
      llTeleportAgentGlobalCoords(llGetOwner(), ZERO_VECTOR, ZERO_VECTOR, ZERO_VECTOR);
    } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

QTEST_GUILESS_MAIN(LslAvatarTest)

#include "tst_lslavatar.moc"
