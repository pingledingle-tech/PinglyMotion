/*
 * PinglyMotion - tests for the collision-family events (second gap list, step 6, part 2)
 *
 * collision_start, collision_end, land_collision_start, land_collision_end, land_collision, moving_start,
 * moving_end, at_target and not_at_target never happen on their own (there is no motion model); new "inputs"
 * types raise them for testing, with the llDetected* data or vector arguments exactly as given. Every expected
 * value is exact.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"

using namespace lsl;

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const GUEST = "11111111-2222-3333-4444-555555555555";
}

class LslCollisionTest : public QObject
{
  Q_OBJECT

  private slots:
    void collisionStartAndEnd();
    void collisionGoesToThePrimHit();
    void collisionHasTheAvatarsVelocity();
    void collisionNameFromTheAvatarsList();
    void attachmentHasTheWearersVelocity();
    void landCollisionPhases();
    void movingStartAndEnd();
    void atTargetAndNotAtTarget();
    void refusesBadFields();
    void checkerKnowsTheEvents();

  private:
    static QJsonObject runSource(const QString& source, const QJsonArray& inputs, double seconds = 1);
    static QStringList events(const QJsonObject& answer, const QString& type = "owner_say");
};

QJsonObject LslCollisionTest::runSource(const QString& source, const QJsonArray& inputs, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"inputs", inputs}, {"seconds", seconds}});
}

QStringList LslCollisionTest::events(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != type) continue;
    QString line = QString::number(e["t"].toDouble()) + " ";
    if (e.contains("script")) line += e["script"].toString() + " ";
    line += type == "event" ? e["name"].toString() : e["text"].toString();
    out << line;
  }
  return out;
}

void LslCollisionTest::collisionStartAndEnd()
{
  const QString source = R"lsl(
    default
    {
      collision_start(integer n) { llOwnerSay("start " + (string)n + " " + llDetectedName(0) + " " + (string)llDetectedKey(0)
                                               + " " + (string)llDetectedOwner(0) + " " + (string)llDetectedLinkNumber(0)); }
      collision_end(integer n) { llOwnerSay("end " + (string)n + " " + llDetectedName(0)); }
    }
  )lsl";
  const QJsonObject r = runSource(source, QJsonArray{QJsonObject{{"at", 1}, {"type", "collision"}, {"phase", "start"}, {"name", "Guest Resident"}, {"id", GUEST}},
                                                      QJsonObject{{"at", 2}, {"type", "collision"}, {"phase", "end"}}}, 2);
  const QString who = QString("Guest Resident %1 %1 0").arg(GUEST);
  QCOMPARE(events(r), QStringList({"1 start 1 " + who, "2 end 1 Owner"}));
  QCOMPARE(events(r, "event"), QStringList({"1 collision_start", "2 collision_end"}));
}

void LslCollisionTest::collisionGoesToThePrimHit()
{
  // the scripts of the prim hit that have collision_start get it; when none does, the root's (collisions pass on)
  auto script = [](const QString& name, const QString& source, int link) { return QJsonObject{{"name", name}, {"source", source}, {"link", link}}; };
  const QString start = "default { collision_start(integer n) { llOwnerSay(\"start \" + (string)llDetectedLinkNumber(0)); } }";
  const QString quiet = "default { state_entry() { } }";
  const QJsonObject o{{"name", "box"}, {"prims", 2}, {"scripts", QJsonArray{script("root", start, 1), script("second", quiet, 2)}}};
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{o}}, {"seconds", 1},
                                                 {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "collision"}, {"phase", "start"}, {"link", 2}}}}});
  QCOMPARE(events(r), QStringList({"1 root start 2"}));
}

// Anafin HUD test: a collision from an avatar of the avatars list gives that avatar's "velocity" in llDetectedVel, in
// collision_start, the continuous collision and collision_end; an avatar that is not listed (here the owner) stands still
void LslCollisionTest::collisionHasTheAvatarsVelocity()
{
  const QString source = R"lsl(
    default
    {
      collision_start(integer n) { llOwnerSay("start " + llDetectedName(0) + " " + (string)llDetectedVel(0)); }
      collision(integer n) { llOwnerSay("while " + llDetectedName(0) + " " + (string)llDetectedVel(0)); }
      collision_end(integer n) { llOwnerSay("end " + llDetectedName(0) + " " + (string)llDetectedVel(0)); }
    }
  )lsl";
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", 4},
      {"avatars", QJsonArray{QJsonObject{{"name", "Guest Resident"}, {"key", GUEST}, {"velocity", QJsonArray{1.5, 0, -0.25}}}}},
      {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "collision"}, {"phase", "start"}, {"id", GUEST}, {"name", "Guest Resident"}},
                            QJsonObject{{"at", 2}, {"type", "collision"}, {"id", GUEST}, {"name", "Guest Resident"}},
                            QJsonObject{{"at", 3}, {"type", "collision"}, {"phase", "end"}, {"id", GUEST}, {"name", "Guest Resident"}},
                            QJsonObject{{"at", 4}, {"type", "collision"}, {"phase", "start"}}}}});
  QCOMPARE(events(r), QStringList({"1 start Guest Resident <1.50000, 0.00000, -0.25000>",
                                   "2 while Guest Resident <1.50000, 0.00000, -0.25000>",
                                   "3 end Guest Resident <1.50000, 0.00000, -0.25000>",
                                   "4 start Owner <0.00000, 0.00000, 0.00000>"}));
}

// job 23 (leftover of the Anafin HUD test): an input with only the id of a listed avatar has that avatar's name in
// llDetectedName; an id that is not listed keeps the owner's name, and an input's own name always wins
void LslCollisionTest::collisionNameFromTheAvatarsList()
{
  const QString source = R"lsl(default { collision_start(integer n) { llOwnerSay(llDetectedName(0) + " " + (string)llDetectedKey(0)); }
                                         touch_start(integer n) { llOwnerSay("touch " + llDetectedName(0)); } })lsl";
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", 4},
      {"avatars", QJsonArray{QJsonObject{{"name", "Guest Resident"}, {"key", GUEST}}}},
      {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "collision"}, {"phase", "start"}, {"id", GUEST}},
                            QJsonObject{{"at", 2}, {"type", "collision"}, {"phase", "start"}, {"id", "6a7e0c3d-0000-4000-8000-0000000000ff"}},
                            QJsonObject{{"at", 3}, {"type", "collision"}, {"phase", "start"}, {"id", GUEST}, {"name", "Given Name"}},
                            QJsonObject{{"at", 4}, {"type", "touch"}, {"id", GUEST}}}}});
  QCOMPARE(events(r), QStringList({QString("1 Guest Resident ") + GUEST, "2 Owner 6a7e0c3d-0000-4000-8000-0000000000ff",
                                   QString("3 Given Name ") + GUEST, "4 touch Guest Resident"}));
}

// Anafin HUD test: llGetVel in a worn object answers the wearer's "velocity" (the owner's, from the avatars list; zero when
// the owner is not listed); a rezzed object keeps its own velocity (zero here) however the owner moves
void LslCollisionTest::attachmentHasTheWearersVelocity()
{
  const QString source = R"lsl(default { collision_start(integer n) { llOwnerSay((string)llDetectedVel(0) + " " + (string)llGetVel()); } })lsl";
  const QJsonObject walker{{"name", "Walker Resident"}, {"key", GUEST}, {"velocity", QJsonArray{1.5, 0, 0}}};
  const QJsonArray inputs{QJsonObject{{"at", 1}, {"type", "collision"}, {"phase", "start"}, {"object", "hud"}, {"id", GUEST}}};
  // the repro of the test: the HUD on point 35, the owner not listed
  const QJsonObject repro = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"inputs", inputs}, {"avatars", QJsonArray{walker}},
      {"objects", QJsonArray{QJsonObject{{"name", "hud"}, {"attached", 35}, {"source", source}}}}});
  QCOMPARE(events(repro), QStringList({"1 <1.50000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000>"}));
  // the owner listed and walking: the HUD moves with the owner
  const QJsonObject owner{{"name", "Owner"}, {"key", OWNER}, {"velocity", QJsonArray{0, 2, 0.5}}};
  const QJsonObject worn = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"inputs", inputs}, {"avatars", QJsonArray{walker, owner}},
      {"objects", QJsonArray{QJsonObject{{"name", "hud"}, {"attached", 35}, {"source", source}}}}});
  QCOMPARE(events(worn), QStringList({"1 <1.50000, 0.00000, 0.00000> <0.00000, 2.00000, 0.50000>"}));
  // the same object rezzed on the ground does not move with its owner
  const QJsonObject rezzed = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"inputs", inputs}, {"avatars", QJsonArray{walker, owner}},
      {"objects", QJsonArray{QJsonObject{{"name", "hud"}, {"source", source}}}}});
  QCOMPARE(events(rezzed), QStringList({"1 <1.50000, 0.00000, 0.00000> <0.00000, 0.00000, 0.00000>"}));
}

void LslCollisionTest::landCollisionPhases()
{
  const QString source = R"lsl(
    default
    {
      land_collision_start(vector p) { llOwnerSay("start " + (string)p); }
      land_collision(vector p) { llOwnerSay("hit " + (string)p); }
      land_collision_end(vector p) { llOwnerSay("end " + (string)p); }
    }
  )lsl";
  const QJsonObject r = runSource(source, QJsonArray{QJsonObject{{"at", 1}, {"type", "land_collision"}, {"phase", "start"}, {"pos", QJsonArray{1, 2, 3}}},
                                                      QJsonObject{{"at", 2}, {"type", "land_collision"}, {"pos", QJsonArray{4, 5, 6}}},
                                                      QJsonObject{{"at", 3}, {"type", "land_collision"}, {"phase", "end"}, {"pos", QJsonArray{7, 8, 9}}}}, 3);
  QCOMPARE(events(r), QStringList({"1 start <1.00000, 2.00000, 3.00000>", "2 hit <4.00000, 5.00000, 6.00000>", "3 end <7.00000, 8.00000, 9.00000>"}));
  QCOMPARE(events(r, "event"), QStringList({"1 land_collision_start", "2 land_collision", "3 land_collision_end"}));
}

void LslCollisionTest::movingStartAndEnd()
{
  const QString source = R"lsl(
    default
    {
      moving_start() { llOwnerSay("moving"); }
      moving_end() { llOwnerSay("stopped"); }
    }
  )lsl";
  const QJsonObject r = runSource(source, QJsonArray{QJsonObject{{"at", 1}, {"type", "moving"}, {"phase", "start"}},
                                                      QJsonObject{{"at", 2}, {"type", "moving"}, {"phase", "end"}}}, 2);
  QCOMPARE(events(r), QStringList({"1 moving", "2 stopped"}));
  QCOMPARE(events(r, "event"), QStringList({"1 moving_start", "2 moving_end"}));
}

void LslCollisionTest::atTargetAndNotAtTarget()
{
  const QString source = R"lsl(
    default
    {
      at_target(integer tnum, vector target, vector our) { llOwnerSay((string)tnum + " " + (string)target + " " + (string)our); }
      not_at_target() { llOwnerSay("not at target"); }
    }
  )lsl";
  const QJsonObject r = runSource(source, QJsonArray{QJsonObject{{"at", 1}, {"type", "at_target"}, {"tnum", 3}, {"targetpos", QJsonArray{1, 2, 3}}, {"ourpos", QJsonArray{0, 0, 0}}},
                                                      QJsonObject{{"at", 2}, {"type", "not_at_target"}}}, 2);
  QCOMPARE(events(r), QStringList({"1 3 <1.00000, 2.00000, 3.00000> <0.00000, 0.00000, 0.00000>", "2 not at target"}));
  QCOMPARE(events(r, "event"), QStringList({"1 at_target", "2 not_at_target"}));
}

void LslCollisionTest::refusesBadFields()
{
  auto error = [](const QJsonObject& input) { return runSource("default { state_entry() { } }", QJsonArray{input})["error"].toString(); };
  // a collision input without "phase" is the continuous collision event (steps 10 to 12 of the third run)
  QCOMPARE(error(QJsonObject{{"type", "collision"}}), QString());
  QCOMPARE(error(QJsonObject{{"type", "collision"}, {"phase", "sideways"}}),
           QString("Input 1: \"phase\" must be \"start\" or \"end\" (leave it out for the continuous collision)."));
  QCOMPARE(error(QJsonObject{{"type", "moving"}}), QString("Input 1: a moving input needs \"phase\": \"start\" or \"end\"."));
  QCOMPARE(error(QJsonObject{{"type", "chat"}, {"phase", "start"}}), QString("Input 1: \"phase\" is only for collision, land_collision and moving."));
  QCOMPARE(error(QJsonObject{{"type", "land_collision"}}), QString("Input 1: a land_collision input needs \"pos\": [x, y, z] (the event's own vector argument)."));
  QCOMPARE(error(QJsonObject{{"type", "land_collision"}, {"pos", QJsonArray{1, 2}}}),
           QString("Input 1: \"pos\" must be [x, y, z], three numbers (the land_collision event's own vector argument)."));
  QCOMPARE(error(QJsonObject{{"type", "chat"}, {"pos", QJsonArray{1, 2, 3}}}), QString("Input 1: \"pos\" is only for land_collision (the event's own vector argument)."));
  QCOMPARE(error(QJsonObject{{"type", "at_target"}}), QString("Input 1: an at_target input needs \"tnum\": the target number, a whole number from 0 (llTarget's return value)."));
  QCOMPARE(error(QJsonObject{{"type", "at_target"}, {"tnum", 1}}),
           QString("Input 1: an at_target input needs \"targetpos\": [x, y, z] (the target position given to llTarget)."));
  QCOMPARE(error(QJsonObject{{"type", "at_target"}, {"tnum", 1}, {"targetpos", QJsonArray{0, 0, 0}}}),
           QString("Input 1: an at_target input needs \"ourpos\": [x, y, z] (where the object is now)."));
  QCOMPARE(error(QJsonObject{{"type", "chat"}, {"tnum", 1}}), QString("Input 1: \"tnum\" is only for at_target."));
  QCOMPARE(error(QJsonObject{{"type", "collision"}, {"link", 1}}), QString());
  // land_collision with no phase is the continuous event; that is fine
  QCOMPARE(error(QJsonObject{{"type", "land_collision"}, {"pos", QJsonArray{0, 0, 0}}}), QString());
}

void LslCollisionTest::checkerKnowsTheEvents()
{
  for (const char* name : {"collision_start", "collision_end", "land_collision_start", "land_collision_end", "land_collision",
                            "moving_start", "moving_end", "at_target", "not_at_target", "transaction_result"})
    QVERIFY2(Parser::eventIsRaised(name), name);
  QVERIFY(Parser::eventIsRaised("collision"));    // the continuous collision (input without "phase"), steps 10 to 12 of the third run
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default {
      collision_start(integer n) { } collision_end(integer n) { }
      land_collision_start(vector p) { } land_collision_end(vector p) { } land_collision(vector p) { }
      moving_start() { } moving_end() { }
      at_target(integer t, vector a, vector b) { } not_at_target() { }
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

QTEST_GUILESS_MAIN(LslCollisionTest)
#include "tst_lslcollision.moc"
