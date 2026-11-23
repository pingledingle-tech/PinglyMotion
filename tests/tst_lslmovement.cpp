/*
 * PinglyMotion - tests for the own-object movement and physics stubs
 * (second gap list, step 6, part 1) and the vehicle function stubs (step 7)
 *
 * llMoveToTarget, llStopMoveToTarget, llPushObject, llRotLookAt and llVolumeDetect only write a
 * recorded_only entry with their arguments, the same pattern as the step 9b-8 stubs: no motion model
 * runs and nothing moves. llApplyImpulse and llSetBuoyancy are real now (second gap list, step 6, part
 * 3, tst_lslmotion.cpp); they stay in checkerHasNoWarnings() below, which only checks that lsl_check
 * finds them supported, not their behaviour. The vehicle functions (llSetVehicleType,
 * llSetVehicleFloatParam, llSetVehicleVectorParam, llSetVehicleRotationParam, llSetVehicleFlags,
 * llRemoveVehicleFlags, second gap list, step 7) are record only the same way: no vehicle physics is
 * simulated. Every expected value is exact.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslMovementTest : public QObject
{
  Q_OBJECT

  private slots:
    void recordedOnly();
    void vehicleFunctionsRecordedOnly();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& body);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QString rec(const QString& function, const QString& args)
    {
      return QString(R"({"args":%1,"function":"%2","note":"Recorded only: the sandbox plays and shows nothing for this.","type":"recorded_only"})").arg(args, function);
    }
};

QJsonObject LslMovementTest::run(const QString& body)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() {\n" + body + "\n} }"}, {"seconds", 1}});
}

QStringList LslMovementTest::entries(const QJsonObject& answer, const QString& type)
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

void LslMovementTest::recordedOnly()
{
  const QJsonObject a = run(R"lsl(
    llMoveToTarget(<1, 2, 3>, 0.5); llStopMoveToTarget();
    llPushObject(NULL_KEY, <1, 0, 0>, ZERO_VECTOR, TRUE);
    llRotLookAt(ZERO_ROTATION, 1.0, 0.5);
    llVolumeDetect(TRUE);
  )lsl");
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QStringList expected({
    rec("llMoveToTarget", "[[1,2,3],0.5]"),
    rec("llStopMoveToTarget", "[]"),
    rec("llPushObject", "[\"00000000-0000-0000-0000-000000000000\",[1,0,0],[0,0,0],1]"),
    rec("llRotLookAt", "[[0,0,0,1],1,0.5]"),
    rec("llVolumeDetect", "[1]")});
  QCOMPARE(entries(a, "recorded_only"), expected);
}

void LslMovementTest::vehicleFunctionsRecordedOnly()
{
  const QJsonObject a = run(R"lsl(
    llSetVehicleType(3);
    llSetVehicleFloatParam(16, 0.5);
    llSetVehicleVectorParam(17, <1, 0, 0>);
    llSetVehicleRotationParam(20, ZERO_ROTATION);
    llSetVehicleFlags(1);
    llRemoveVehicleFlags(1);
  )lsl");
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QStringList expected({
    rec("llSetVehicleType", "[3]"),
    rec("llSetVehicleFloatParam", "[16,0.5]"),
    rec("llSetVehicleVectorParam", "[17,[1,0,0]]"),
    rec("llSetVehicleRotationParam", "[20,[0,0,0,1]]"),
    rec("llSetVehicleFlags", "[1]"),
    rec("llRemoveVehicleFlags", "[1]")});
  QCOMPARE(entries(a, "recorded_only"), expected);
}

void LslMovementTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llMoveToTarget(ZERO_VECTOR, 1.0); llStopMoveToTarget(); llPushObject(NULL_KEY, ZERO_VECTOR, ZERO_VECTOR, FALSE);
      llApplyImpulse(ZERO_VECTOR, FALSE); llSetBuoyancy(0.0); llRotLookAt(ZERO_ROTATION, 1.0, 1.0); llVolumeDetect(FALSE);
      llSetVehicleType(0); llSetVehicleFloatParam(0, 0.0); llSetVehicleVectorParam(0, ZERO_VECTOR);
      llSetVehicleRotationParam(0, ZERO_ROTATION); llSetVehicleFlags(0); llRemoveVehicleFlags(0);
    } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  for (const char* f : {"llMoveToTarget", "llStopMoveToTarget", "llPushObject", "llApplyImpulse", "llSetBuoyancy",
                        "llRotLookAt", "llVolumeDetect", "llSetVehicleType", "llSetVehicleFloatParam",
                        "llSetVehicleVectorParam", "llSetVehicleRotationParam", "llSetVehicleFlags", "llRemoveVehicleFlags"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

QTEST_GUILESS_MAIN(LslMovementTest)
#include "tst_lslmovement.moc"
