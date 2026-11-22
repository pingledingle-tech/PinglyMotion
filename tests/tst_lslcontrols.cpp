/*
 * PinglyMotion - tests for controls (Script Lab step 9b-6)
 *
 * llTakeControls, llReleaseControls, the control input and the control event, and the
 * permission PERMISSION_TAKE_CONTROLS. Every expected value is exact.
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
  const char* const GUEST = "11111111-2222-3333-4444-555555555555";

  // an owner who takes forward and left (accept, pass on), then releases them on a touch
  const char* const DRIVER = R"lsl(
    default {
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_TAKE_CONTROLS); }
      run_time_permissions(integer p) { if (p & PERMISSION_TAKE_CONTROLS) llTakeControls(CONTROL_FWD | CONTROL_LEFT, TRUE, FALSE); }
      control(key id, integer level, integer edge) { llOwnerSay("control " + (string)id + " level " + (string)level + " edge " + (string)edge); }
      touch_start(integer n) { llReleaseControls(); }
    }
  )lsl";
}

class LslControlsTest : public QObject
{
  Q_OBJECT

  private slots:
    void controlEventIsCutToTheControlsTaken();
    void releaseStopsTheEvents();
    void needsPermission();
    void onlyTheAvatarWithPermissionIsHeard();
    void sittingAvatarTakesControls();
    void inputIsChecked();
    void checkerHasNoWarnings();
    void constantsMatchSecondLife();

  private:
    static QJsonObject runWith(const QString& source, const QJsonArray& inputs, double seconds = 5);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QJsonObject control(double at, const QJsonValue& levels, const QJsonValue& edges, const QString& id = QString())
    {
      QJsonObject o{{"at", at}, {"type", "control"}, {"levels", levels}, {"edges", edges}};
      if (!id.isEmpty()) o["id"] = id;
      return o;
    }
};

QJsonObject LslControlsTest::runWith(const QString& source, const QJsonArray& inputs, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"inputs", inputs}, {"seconds", seconds}});
}

QStringList LslControlsTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslControlsTest::entries(const QJsonObject& answer, const QString& type)
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

void LslControlsTest::controlEventIsCutToTheControlsTaken()
{
  const QString owner = "a2e76fcd-9360-4f6d-a924-000000000001";
  const QJsonObject a = runWith(DRIVER, QJsonArray{
    control(1, QJsonArray{"forward", "back"}, "forward"),   // forward is taken: level 1, edge 1 (back is not taken)
    control(2, "right", "right"),                           // right is not taken: no event
    control(3, QJsonArray{"left", "CONTROL_FWD"}, 4),       // level 5 (fwd + left), edge 4 (left)
    control(4, 0x10000000, 0x10000000)});                   // the left mouse button is not taken
  QCOMPARE(said(a), QStringList({
    "control " + owner + " level 1 edge 1",
    "control " + owner + " level 5 edge 4"}));
  QCOMPARE(entries(a, "controls"), QStringList({
    R"({"accept":true,"avatar":"a2e76fcd-9360-4f6d-a924-000000000001","controls":5,"op":"take","pass_on":false,"type":"controls"})"}));
  QCOMPARE(entries(a, "input").size(), 4);
  QCOMPARE(entries(a, "input")[0], QString(R"({"edges":1,"id":"a2e76fcd-9360-4f6d-a924-000000000001","input":"control","levels":3,"name":"Owner","type":"input"})"));
}

void LslControlsTest::releaseStopsTheEvents()
{
  const QJsonObject a = runWith(DRIVER, QJsonArray{control(1, "forward", "forward"), QJsonObject{{"at", 2}, {"type", "touch"}},
                                                   control(3, "forward", "forward")});
  QCOMPARE(said(a).size(), 1);
  QCOMPARE(entries(a, "controls"), QStringList({
    R"({"accept":true,"avatar":"a2e76fcd-9360-4f6d-a924-000000000001","controls":5,"op":"take","pass_on":false,"type":"controls"})",
    R"({"avatar":"a2e76fcd-9360-4f6d-a924-000000000001","op":"release","type":"controls"})"}));
}

void LslControlsTest::needsPermission()
{
  const QJsonObject a = runWith(R"lsl(
    default {
      state_entry() { llTakeControls(CONTROL_FWD, TRUE, TRUE); llReleaseControls(); }
      control(key id, integer level, integer edge) { llOwnerSay("unexpected"); }
    }
  )lsl", QJsonArray{control(1, "forward", "forward")});
  QCOMPARE(said(a), QStringList());
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"Script trying to take controls but PERMISSION_TAKE_CONTROLS permission not set","type":"debug"})"}));
  QCOMPARE(entries(a, "controls"), QStringList());
}

void LslControlsTest::onlyTheAvatarWithPermissionIsHeard()
{
  const QJsonObject a = runWith(DRIVER, QJsonArray{control(1, "forward", "forward", GUEST)});
  QCOMPARE(said(a), QStringList());
  QCOMPARE(entries(a, "input").size(), 1);
}

void LslControlsTest::sittingAvatarTakesControls()
{
  const QString script = R"lsl(
    default {
      state_entry() { llSitTarget(<0, 0, 1>, ZERO_ROTATION); }
      changed(integer c) {
        key av = llAvatarOnSitTarget();
        if (av != NULL_KEY) llRequestPermissions(av, PERMISSION_TAKE_CONTROLS);
      }
      run_time_permissions(integer p) { llTakeControls(CONTROL_FWD | CONTROL_BACK, TRUE, TRUE); }
      control(key id, integer level, integer edge) { llOwnerSay((string)id + " " + (string)level + " " + (string)edge); }
    }
  )lsl";
  const QJsonObject a = runWith(script, QJsonArray{QJsonObject{{"at", 1}, {"type", "sit"}, {"id", GUEST}, {"name", "Guest"}},
                                                   control(2, QJsonArray{"back", "up"}, "back", GUEST)});
  QCOMPARE(said(a), QStringList({"11111111-2222-3333-4444-555555555555 2 2"}));
}

void LslControlsTest::inputIsChecked()
{
  auto problem = [](const QJsonObject& input)
  {
    const QJsonObject a = runWith("default { state_entry() {} }", QJsonArray{input});
    return a["ok"].toBool() ? QString("ok") : a["error"].toString();
  };
  QCOMPARE(problem(control(1, "forward", 0)), QString("ok"));
  QCOMPARE(problem(QJsonObject{{"type", "control"}, {"edges", 0x5000033F}}), QString("ok"));
  QVERIFY(problem(QJsonObject{{"type", "control"}}).contains("a control input needs \"levels\""));
  QVERIFY(problem(QJsonObject{{"type", "control"}, {"levels", "sideways"}}).contains("\"levels\" must be a name (forward, back, left"));
  QVERIFY(problem(QJsonObject{{"type", "control"}, {"edges", QJsonArray()}}).contains("\"edges\" must be a name"));
  QVERIFY(problem(QJsonObject{{"type", "control"}, {"edges", 2}, {"levels", 0x40}}).contains("\"levels\" must be a name"));
  QCOMPARE(problem(QJsonObject{{"type", "touch"}, {"levels", 1}}), QString("Input 1: \"levels\" and \"edges\" are only for control."));
}

void LslControlsTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default { state_entry() { llTakeControls(CONTROL_FWD, TRUE, TRUE); llReleaseControls(); }
              control(key id, integer level, integer edge) {} }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  QVERIFY(Builtins::function("llTakeControls")->supported && Builtins::function("llReleaseControls")->supported);
}

void LslControlsTest::constantsMatchSecondLife()
{
  const QList<QPair<QString, int>> expected = {
    {"CONTROL_FWD", 1}, {"CONTROL_BACK", 2}, {"CONTROL_LEFT", 4}, {"CONTROL_RIGHT", 8}, {"CONTROL_UP", 0x10}, {"CONTROL_DOWN", 0x20},
    {"CONTROL_ROT_LEFT", 0x100}, {"CONTROL_ROT_RIGHT", 0x200}, {"CONTROL_LBUTTON", 0x10000000}, {"CONTROL_ML_LBUTTON", 0x40000000}};
  for (const auto& e : expected)
  {
    const BuiltinConstant* c = Builtins::constant(e.first);
    QVERIFY2(c != nullptr, qPrintable(e.first));
    QCOMPARE(c->value.i, e.second);
  }
}

QTEST_GUILESS_MAIN(LslControlsTest)
#include "tst_lslcontrols.moc"
