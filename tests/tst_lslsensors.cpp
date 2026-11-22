/*
 * PinglyMotion - tests for sensors (Script Lab step 9b-7)
 *
 * llSensor, llSensorRepeat, llSensorRemove, the sensor and no_sensor events and the llDetected*
 * functions, against avatars and objects with positions given in the input. Every expected value
 * is exact: distances are whole metres along the axes, so the order and the limits are known.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";

  QJsonObject avatar(const QString& name, double x, double y, double z)
  {
    return QJsonObject{{"name", name}, {"position", QJsonArray{x, y, z}}};
  }
}

class LslSensorsTest : public QObject
{
  Q_OBJECT

  private slots:
    void rangeAndOrder();
    void filters();
    void rangeIsCappedAt96();
    void arcAroundTheForwardDirection();
    void rotatedObjectFacesElsewhere();
    void repeatEveryRate();
    void repeatEndsWithAStateChangeOrARateOfZero();
    void objectsAreFoundToo();
    void detectedData();
    void atMostSixteenResults();
    void avatarsAreChecked();
    void checkerHasNoWarnings();
    void constantsMatchSecondLife();

  private:
    static QJsonObject runWith(const QString& source, const QJsonArray& avatars, double seconds = 1);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QStringList times(const QJsonObject& answer, const QString& type);
    // a script that senses once with the given arguments and says "name@distance" of each find, or "none"
    static QString oneShot(const QString& args);
    static QJsonArray people();
};

QJsonObject LslSensorsTest::runWith(const QString& source, const QJsonArray& avatars, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"avatars", avatars}, {"seconds", seconds}});
}

QStringList LslSensorsTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslSensorsTest::entries(const QJsonObject& answer, const QString& type)
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

QStringList LslSensorsTest::times(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == type) out << QString::number(v.toObject()["t"].toDouble());
  return out;
}

QString LslSensorsTest::oneShot(const QString& args)
{
  return "default { state_entry() { llSensor(" + args + "); }\n"
         "sensor(integer n) { integer i; string s; for (i = 0; i < n; ++i) s += llDetectedName(i) + \"@\" + (string)((integer)llVecDist(llGetPos(), llDetectedPos(i))) + \" \"; llOwnerSay(s); }\n"
         "no_sensor() { llOwnerSay(\"none\"); } }";
}

// the sensing object stands at 128, 128, 20 (where the owner stands), facing +x
QJsonArray LslSensorsTest::people()
{
  return QJsonArray{avatar("Ann", 128, 133, 20), avatar("Bob", 128, 128, 30), avatar("Cy", 200, 128, 20), avatar("Dee", 128, 128, 150)};
}

void LslSensorsTest::rangeAndOrder()
{
  // the owner is there too, at distance 0; the nearest come first; Dee, 130 m away, is out of range
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 100.0, PI"), people())), QStringList({"Owner@0 Ann@5 Bob@10 Cy@72 "}));
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 6.0, PI"), people())), QStringList({"Owner@0 Ann@5 "}));
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 10.0, PI"), people())), QStringList({"Owner@0 Ann@5 Bob@10 "}));   // exactly 10 m is in
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 0.0, PI"), people())), QStringList({"none"}));
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, -5.0, PI"), people())), QStringList({"none"}));
  // no avatars given: only the owner
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 50.0, PI"), QJsonArray())), QStringList({"Owner@0 "}));
  const QJsonObject a = runWith(oneShot("\"\", NULL_KEY, AGENT, 6.0, PI"), people());
  QCOMPARE(entries(a, "sensor"), QStringList({R"({"found":2,"names":["Owner","Ann"],"type":"sensor"})"}));
}

void LslSensorsTest::filters()
{
  QCOMPARE(said(runWith(oneShot("\"Bob\", NULL_KEY, AGENT, 100.0, PI"), people())), QStringList({"Bob@10 "}));
  QCOMPARE(said(runWith(oneShot("\"bob\", NULL_KEY, AGENT, 100.0, PI"), people())), QStringList({"none"}));   // the name must match exactly
  QJsonArray withKey{QJsonObject{{"name", "Ann"}, {"key", "11111111-2222-3333-4444-555555555555"}, {"position", QJsonArray{128, 133, 20}}}, avatar("Bob", 128, 128, 30)};
  QCOMPARE(said(runWith(oneShot("\"\", \"11111111-2222-3333-4444-555555555555\", AGENT, 100.0, PI"), withKey)), QStringList({"Ann@5 "}));
  // the type mask: avatars are AGENT only; ACTIVE, PASSIVE and a mask of 0 find none of them
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, ACTIVE | PASSIVE, 100.0, PI"), people())), QStringList({"none"}));
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, 0, 100.0, PI"), people())), QStringList({"none"}));
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT | SCRIPTED, 6.0, PI"), people())), QStringList({"Owner@0 Ann@5 "}));
}

void LslSensorsTest::rangeIsCappedAt96()
{
  // Cy is 72 m away (in), Dee 130 m (out), whatever the range asked for
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 5000.0, PI"), people())), QStringList({"Owner@0 Ann@5 Bob@10 Cy@72 "}));
  QCOMPARE(said(runWith(oneShot("\"Cy\", NULL_KEY, AGENT, 71.9, PI"), people())), QStringList({"none"}));
}

void LslSensorsTest::arcAroundTheForwardDirection()
{
  const QJsonArray ring{avatar("Front", 138, 128, 20), avatar("Side", 128, 138, 20), avatar("Back", 118, 128, 20)};
  // the owner stands on the object (distance 0) and is always found
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 20.0, 0.5"), ring)), QStringList({"Owner@0 Front@10 "}));
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 20.0, 1.6"), ring)), QStringList({"Owner@0 Front@10 Side@10 "}));
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 20.0, PI"), ring)), QStringList({"Owner@0 Front@10 Side@10 Back@10 "}));
  QCOMPARE(said(runWith(oneShot("\"\", NULL_KEY, AGENT, 20.0, 10.0"), ring)), QStringList({"Owner@0 Front@10 Side@10 Back@10 "}));   // an arc over PI is PI
}

void LslSensorsTest::rotatedObjectFacesElsewhere()
{
  const QJsonArray ring{avatar("Front", 138, 128, 20), avatar("Side", 128, 138, 20), avatar("Back", 118, 128, 20)};
  // turned a quarter turn about z, the object faces +y
  const QString source = "default { state_entry() { llSetRot(llEuler2Rot(<0, 0, PI_BY_TWO>)); llSensor(\"\", NULL_KEY, AGENT, 20.0, 0.5); }\n"
                         "sensor(integer n) { integer i; string s; for (i = 0; i < n; ++i) s += llDetectedName(i) + \" \"; llOwnerSay(s); } }";
  QCOMPARE(said(runWith(source, ring)), QStringList({"Owner Side "}));
}

void LslSensorsTest::repeatEveryRate()
{
  const QString source = R"lsl(
    default {
      state_entry() { llSensorRepeat("Ann", NULL_KEY, AGENT, 20.0, PI, 2.0); }
      sensor(integer n) { llOwnerSay("found " + llDetectedName(0)); }
      no_sensor() { llOwnerSay("none"); }
    }
  )lsl";
  const QJsonObject a = runWith(source, QJsonArray{avatar("Ann", 128, 133, 20)}, 7);
  QCOMPARE(said(a), QStringList({"found Ann", "found Ann", "found Ann"}));
  QCOMPARE(times(a, "sensor"), QStringList({"2", "4", "6"}));   // the first scan comes one rate after the call
  QCOMPARE(entries(a, "sensor_set"), QStringList({
    R"({"arc":3.14159,"id":"00000000-0000-0000-0000-000000000000","name":"Ann","range":20,"rate":2,"sense":1,"type":"sensor_set"})"}));
  // nobody there: no_sensor each time
  const QJsonObject b = runWith(source, QJsonArray(), 5);
  QCOMPARE(said(b), QStringList({"none", "none"}));
}

void LslSensorsTest::repeatEndsWithAStateChangeOrARateOfZero()
{
  const QJsonObject removed = runWith(R"lsl(
    default {
      state_entry() { llSensorRepeat("", NULL_KEY, AGENT, 20.0, PI, 1.0); llSetTimerEvent(2.5); }
      timer() { llSensorRemove(); llSensorRemove(); llSetTimerEvent(0.0); }
      sensor(integer n) { llOwnerSay("sensor"); }
    }
  )lsl", QJsonArray(), 6);
  QCOMPARE(said(removed), QStringList({"sensor", "sensor"}));
  QCOMPARE(entries(removed, "sensor_remove"), QStringList({R"({"type":"sensor_remove"})"}));
  const QJsonObject zero = runWith(R"lsl(
    default {
      state_entry() { llSensorRepeat("", NULL_KEY, AGENT, 20.0, PI, 1.0); llSensorRepeat("", NULL_KEY, AGENT, 20.0, PI, 0.0); }
      sensor(integer n) { llOwnerSay("sensor"); }
    }
  )lsl", QJsonArray(), 3);
  QCOMPARE(said(zero), QStringList());
  const QJsonObject state = runWith(R"lsl(
    default {
      state_entry() { llSensorRepeat("", NULL_KEY, AGENT, 20.0, PI, 1.0); llSetTimerEvent(1.5); }
      timer() { state other; }
      sensor(integer n) { llOwnerSay("sensor"); }
    }
    state other { state_entry() { llOwnerSay("other"); } sensor(integer n) { llOwnerSay("sensor in other"); } }
  )lsl", QJsonArray(), 6);
  QCOMPARE(said(state), QStringList({"sensor", "other"}));
}

void LslSensorsTest::objectsAreFoundToo()
{
  const QJsonObject request{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{
    QJsonObject{{"name", "Scanner"}, {"source", R"lsl(
      default {
        state_entry() { llSensor("", NULL_KEY, SCRIPTED, 10.0, PI); llSensor("", NULL_KEY, AGENT, 10.0, PI); }
        sensor(integer n) {
          llOwnerSay(llDetectedName(0) + " type " + (string)llDetectedType(0) + " key " + (string)llDetectedKey(0) + " owner " + (string)llDetectedOwner(0)
                     + " pos " + (string)llDetectedPos(0) + " group " + (string)llDetectedGroup(0));
        }
        no_sensor() { llOwnerSay("none"); }
      }
    )lsl"}},
    QJsonObject{{"name", "Crate"}, {"key", "cccccccc-0000-4000-8000-000000000009"}, {"position", QJsonArray{128, 130, 20}},
                {"source", "default { state_entry() {} }"}}}}};
  const QJsonObject a = Control::run(request);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QStringList said;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") said << v.toObject()["text"].toString();
  QCOMPARE(said, QStringList({
    QString("Crate type 8 key cccccccc-0000-4000-8000-000000000009 owner %1 pos <128.00000, 130.00000, 20.00000> group 0").arg(OWNER),
    QString("Owner type 1 key %1 owner %1 pos <128.00000, 128.00000, 20.00000> group 0").arg(OWNER)}));
}

void LslSensorsTest::detectedData()
{
  const QJsonArray list{QJsonObject{{"name", "Ann"}, {"position", QJsonArray{128, 133, 20}}, {"velocity", QJsonArray{1, 2, 3}},
                                    {"rotation", QJsonArray{0, 0, 0.5, 0.5}}, {"in_group", true}}};
  const QJsonObject a = runWith(R"lsl(
    default {
      state_entry() { llSensor("Ann", NULL_KEY, AGENT, 20.0, PI); }
      sensor(integer n) {
        llOwnerSay((string)n + " " + (string)llDetectedVel(0) + " " + (string)llDetectedRot(0) + " group " + (string)llDetectedGroup(0));
        llOwnerSay((string)llDetectedKey(1) + " [" + llDetectedName(1) + "] " + (string)llDetectedPos(1) + " " + (string)llDetectedType(1) + " " + (string)llDetectedKey(-1));
        llOwnerSay((string)llDetectedRot(7) + " " + (string)llDetectedVel(7) + " " + (string)llDetectedOwner(7) + " " + (string)llDetectedGroup(7));
      }
    }
  )lsl", list);
  QCOMPARE(said(a), QStringList({
    "1 <1.00000, 2.00000, 3.00000> <0.00000, 0.00000, 0.50000, 0.50000> group 1",
    "00000000-0000-0000-0000-000000000000 [] <0.00000, 0.00000, 0.00000> 0 00000000-0000-0000-0000-000000000000",
    "<0.00000, 0.00000, 0.00000, 1.00000> <0.00000, 0.00000, 0.00000> 00000000-0000-0000-0000-000000000000 0"}));
  // a touch still has one detected avatar
  const QJsonObject t = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { touch_start(integer n) { llOwnerSay(llDetectedName(0) + \" \" + (string)llDetectedType(0) + \" \" + llDetectedName(1)); } }"},
                                                 {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}}}, {"seconds", 2}});
  QCOMPARE(said(t), QStringList({"Owner 1 "}));
}

void LslSensorsTest::atMostSixteenResults()
{
  QJsonArray crowd;
  for (int i = 0; i < 20; ++i) crowd.append(avatar(QString("P%1").arg(i, 2, 10, QChar('0')), 128, 129 + i, 20));
  const QString source = "default { state_entry() { llSensor(\"\", NULL_KEY, AGENT, 50.0, PI); } sensor(integer n) { llOwnerSay((string)n + \" \" + llDetectedName(0) + \" \" + llDetectedName(15) + \" [\" + llDetectedName(16) + \"]\"); } }";
  // the owner (distance 0) and the 15 nearest others
  QCOMPARE(said(runWith(source, crowd)), QStringList({"16 Owner P14 []"}));
}

void LslSensorsTest::avatarsAreChecked()
{
  auto problem = [](const QJsonValue& avatars)
  {
    const QJsonObject a = runWith("default { state_entry() {} }", avatars.toArray());
    return a["ok"].toBool() ? QString("ok") : a["error"].toString();
  };
  QCOMPARE(problem(QJsonArray{avatar("Ann", 1, 2, 3)}), QString("ok"));
  QVERIFY(problem(QJsonArray{5}).startsWith("Avatar 1: must be an object"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"position", QJsonArray{1, 2, 3}}}}).contains("\"name\" must be text of 1 to 63 characters."));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "A"}, {"color", 1}}}).contains("unknown field \"color\""));
  QVERIFY(problem(QJsonArray{avatar("A", 300, 0, 0)}).contains("\"position\" must be [x, y, z]"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "A"}, {"velocity", QJsonArray{1, 2}}}}).contains("\"velocity\" must be [x, y, z]"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "A"}, {"rotation", QJsonArray{1, 2, 3}}}}).contains("\"rotation\" must be [x, y, z, s]"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "A"}, {"in_group", "yes"}}}).contains("\"in_group\" must be true or false."));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "A"}, {"key", "bad key"}}}).contains("\"key\" must be a key"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "A"}, {"key", "k1"}}, QJsonObject{{"name", "B"}, {"key", "k1"}}}).contains("Avatar 2: Two avatars have the key k1."));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "A"}, {"key", "00000000-0000-0000-0000-000000000000"}}}).contains("NULL_KEY"));
  QJsonArray many;
  for (int i = 0; i < 65; ++i) many.append(avatar("A", 1, 1, 1));
  QCOMPARE(problem(many), QString("\"avatars\" holds at most 64 avatars."));
  const QJsonObject notList = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() {} }"}, {"avatars", "x"}});
  QVERIFY(notList["error"].toString().startsWith("\"avatars\" must be a list of avatars"));
}

void LslSensorsTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default {
      state_entry() { llSensor("", NULL_KEY, AGENT, 1.0, PI); llSensorRepeat("", NULL_KEY, AGENT, 1.0, PI, 1.0); llSensorRemove();
        llDetectedKey(0); llDetectedName(0); llDetectedOwner(0); llDetectedType(0); llDetectedPos(0); llDetectedRot(0); llDetectedVel(0); llDetectedGroup(0); }
      sensor(integer n) {}
      no_sensor() {}
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  for (const char* f : {"llSensor", "llSensorRepeat", "llSensorRemove", "llDetectedPos", "llDetectedRot", "llDetectedVel", "llDetectedType", "llDetectedGroup", "llDetectedOwner"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

void LslSensorsTest::constantsMatchSecondLife()
{
  const QList<QPair<QString, int>> expected = {{"AGENT", 1}, {"ACTIVE", 2}, {"PASSIVE", 4}, {"SCRIPTED", 8}};
  for (const auto& e : expected)
  {
    const BuiltinConstant* c = Builtins::constant(e.first);
    QVERIFY2(c != nullptr, qPrintable(e.first));
    QCOMPARE(c->value.i, e.second);
  }
}

QTEST_GUILESS_MAIN(LslSensorsTest)
#include "tst_lslsensors.moc"
