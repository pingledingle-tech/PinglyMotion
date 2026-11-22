/*
 * PinglyMotion - tests for the own-object movement and physics stubs
 * (second gap list, step 6, part 1)
 *
 * llMoveToTarget, llStopMoveToTarget, llPushObject, llApplyImpulse, llSetBuoyancy, llRotLookAt and
 * llVolumeDetect only write a recorded_only entry with their arguments, the same pattern as the
 * step 9b-8 stubs: no motion model runs and nothing moves. Every expected value is exact.
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
    llApplyImpulse(<0, 1, 0>, FALSE);
    llSetBuoyancy(0.75);
    llRotLookAt(ZERO_ROTATION, 1.0, 0.5);
    llVolumeDetect(TRUE);
  )lsl");
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QStringList expected({
    rec("llMoveToTarget", "[[1,2,3],0.5]"),
    rec("llStopMoveToTarget", "[]"),
    rec("llPushObject", "[\"00000000-0000-0000-0000-000000000000\",[1,0,0],[0,0,0],1]"),
    rec("llApplyImpulse", "[[0,1,0],0]"),
    rec("llSetBuoyancy", "[0.75]"),
    rec("llRotLookAt", "[[0,0,0,1],1,0.5]"),
    rec("llVolumeDetect", "[1]")});
  QCOMPARE(entries(a, "recorded_only"), expected);
}

void LslMovementTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() {
      llMoveToTarget(ZERO_VECTOR, 1.0); llStopMoveToTarget(); llPushObject(NULL_KEY, ZERO_VECTOR, ZERO_VECTOR, FALSE);
      llApplyImpulse(ZERO_VECTOR, FALSE); llSetBuoyancy(0.0); llRotLookAt(ZERO_ROTATION, 1.0, 1.0); llVolumeDetect(FALSE);
    } }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  for (const char* f : {"llMoveToTarget", "llStopMoveToTarget", "llPushObject", "llApplyImpulse", "llSetBuoyancy",
                        "llRotLookAt", "llVolumeDetect"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

QTEST_GUILESS_MAIN(LslMovementTest)
#include "tst_lslmovement.moc"
