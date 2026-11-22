/*
 * PinglyMotion - tests for the simple motion model (second gap list, step 6, part 3)
 *
 * A small, deliberately scaled-down slice of the motion model: the "step" input advances velocity by
 * simple gravity scaled by llSetBuoyancy, and position by velocity; llApplyImpulse changes the velocity
 * at once (impulse / mass); llGetVel answers it. moving_start/moving_end now also fire on their own from
 * "step" when speed crosses the threshold, on top of the manual "moving" input from part 2. No model yet
 * for llMoveToTarget/at_target, real collisions from motion, or vehicles: a later job. Every expected
 * value is exact.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslMotionTest : public QObject
{
  Q_OBJECT

  private slots:
    void gravityScaledByBuoyancy();
    void impulseChangesVelocityAtOnce();
    void movingStartHandlerCancelsVelocitySoMovingEndFollows();
    void wornObjectIgnoresStep();
    void refusesBadFields();

  private:
    static QJsonObject runSource(const QString& source, const QJsonArray& inputs, double seconds);
    static QStringList events(const QJsonObject& answer, const QString& type = "owner_say");
    static QJsonObject entry(const QJsonObject& answer, const QString& type, int occurrence = 0);
};

QJsonObject LslMotionTest::runSource(const QString& source, const QJsonArray& inputs, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"inputs", inputs}, {"seconds", seconds}});
}

QStringList LslMotionTest::events(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != type) continue;
    QString line = QString::number(e["t"].toDouble()) + " ";
    line += type == "event" ? e["name"].toString() : e["text"].toString();
    out << line;
  }
  return out;
}

QJsonObject LslMotionTest::entry(const QJsonObject& answer, const QString& type, int occurrence)
{
  int seen = 0;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    QJsonObject e = v.toObject();
    if (e["type"].toString() != type) continue;
    if (seen++ == occurrence) { e.remove("t"); return e; }
  }
  return QJsonObject();
}

// default object: scale <0.5, 0.5, 0.5>, one prim, so llGetMass gives 0.5*0.5*0.5*10 = 1.25; position starts
// at <128, 128, 20>. touch_start reads llGetVel and llGetPos back exactly, as a probe.
void LslMotionTest::gravityScaledByBuoyancy()
{
  const QString source = R"lsl(
    default
    {
      touch_start(integer n) { llOwnerSay((string)llGetVel() + " " + (string)llGetPos()); }
    }
  )lsl";
  const QJsonObject r = runSource(source, QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}},
                                                      QJsonObject{{"at", 2}, {"type", "touch"}}}, 2);
  QCOMPARE(events(r), QStringList({"2 <0.00000, 0.00000, -9.80000> <128.00000, 128.00000, 10.20000>"}));
  const QJsonObject step = entry(r, "input");
  QCOMPARE(step.value("input").toString(), QString("step"));
  QCOMPARE(step.value("velocity").toArray(), QJsonArray({0.0, 0.0, -9.8}));
  QCOMPARE(step.value("position").toArray(), QJsonArray({128.0, 128.0, 10.2}));
}

void LslMotionTest::impulseChangesVelocityAtOnce()
{
  const QString source = R"lsl(
    default
    {
      state_entry() { llApplyImpulse(<0, 0, 0.5>, FALSE); }
      touch_start(integer n) { llOwnerSay((string)llGetVel()); }
    }
  )lsl";
  const QJsonObject r = runSource(source, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}}, 1);
  // mass 1.25: velocity = <0, 0, 0.5> / 1.25 = <0, 0, 0.4>, at once, before any "step"
  QCOMPARE(events(r), QStringList({"1 <0.00000, 0.00000, 0.40000>"}));
  const QJsonObject v = entry(r, "velocity");
  QCOMPARE(v.value("op").toString(), QString("impulse"));
  QCOMPARE(v.value("mass").toDouble(), 1.25);
  QCOMPARE(v.value("velocity").toArray(), QJsonArray({0.0, 0.0, 0.4}));
}

void LslMotionTest::movingStartHandlerCancelsVelocitySoMovingEndFollows()
{
  // buoyancy 1.0: no gravity, so "step" alone never changes the velocity the impulse gave it. moving_start's own
  // handler applies the exact opposite impulse, so the object is at rest again before the next "step".
  const QString source = R"lsl(
    default
    {
      state_entry() { llSetBuoyancy(1.0); llApplyImpulse(<0, 0, 0.5>, FALSE); }
      moving_start() { llOwnerSay("start"); llApplyImpulse(<0, 0, -0.5>, FALSE); }
      moving_end() { llOwnerSay("end"); }
    }
  )lsl";
  const QJsonObject r = runSource(source, QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}},
                                                      QJsonObject{{"at", 2}, {"type", "step"}, {"dt", 1}}}, 2);
  QCOMPARE(events(r), QStringList({"1 start", "2 end"}));
  QCOMPARE(events(r, "event"), QStringList({"0 state_entry", "1 moving_start", "2 moving_end"}));
}

void LslMotionTest::wornObjectIgnoresStep()
{
  const QString source = "default { touch_start(integer n) { llOwnerSay((string)llGetVel()); } }";
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"},
                                                 {"objects", QJsonArray{QJsonObject{{"name", "Object"}, {"attached", 1}, {"source", source}}}},
                                                 {"seconds", 2},
                                                 {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 1}},
                                                                       QJsonObject{{"at", 2}, {"type", "touch"}}}}});
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  QCOMPARE(events(r), QStringList({"2 <0.00000, 0.00000, 0.00000>"}));
  const QJsonObject step = entry(r, "input");
  QCOMPARE(step.value("note").toString(), QString("Worn: the motion model does not apply to an attachment."));
}

void LslMotionTest::refusesBadFields()
{
  auto error = [](const QJsonObject& input) { return runSource("default { state_entry() { } }", QJsonArray{input}, 1)["error"].toString(); };
  QCOMPARE(error(QJsonObject{{"type", "step"}}), QString("Input 1: a step input needs \"dt\": the seconds the motion model advances."));
  QCOMPARE(error(QJsonObject{{"type", "step"}, {"dt", -1}}), QString("Input 1: \"dt\" must be a number of seconds from 0 to 60."));
  QCOMPARE(error(QJsonObject{{"type", "chat"}, {"dt", 1}}), QString("Input 1: \"dt\" is only for step (the seconds the motion model advances)."));
  QCOMPARE(error(QJsonObject{{"type", "step"}, {"dt", 1}, {"phase", "start"}}), QString("Input 1: \"phase\" is only for collision, land_collision and moving."));
}

QTEST_GUILESS_MAIN(LslMotionTest)
#include "tst_lslmotion.moc"
