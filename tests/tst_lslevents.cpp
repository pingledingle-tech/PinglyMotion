/*
 * PinglyMotion - tests for the events on_rez and changed (Script Lab step 9b-2, issue 314)
 *
 * on_rez with the start parameter (order after state_entry and before attach, llResetScript,
 * state change in state_entry), changed with the CHANGED_* flags from an input of type
 * "changed" (names, lists, numbers; every script of the object; lsl_event; refused input),
 * and the checker no longer warning that the two events are never raised. Every expected
 * value is exact; the CHANGED_* values are those of Second Life (0x1 inventory, 0x80 owner,
 * 0x20 link, 0x400 region start).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

class LslEventsTest : public QObject
{
  Q_OBJECT

  private slots:
    void onRezAtStart();
    void onRezStartParameter();
    void onRezBeforeAttach();
    void resetScriptDoesNotRaiseOnRez();
    void stateChangeInStateEntryDropsOnRez();
    void changedFlags();
    void changedNamesListsAndNumbers();
    void changedReachesEveryScriptOfTheObject();
    void changedWithLslEvent();
    void changedIsRefusedWhenWrong();
    void checkerKnowsBothEvents();

  private:
    static QJsonObject run(const QJsonObject& request, RlvState& rlv, Session* session = nullptr);
    static QJsonObject object(const QString& name, const QString& source, const QJsonObject& extra = QJsonObject());
    static QStringList said(const QJsonObject& answer);
};

QJsonObject LslEventsTest::run(const QJsonObject& request, RlvState& rlv, Session* session)
{
  return Control::run(request, &rlv, session);
}

QJsonObject LslEventsTest::object(const QString& name, const QString& source, const QJsonObject& extra)
{
  QJsonObject o = extra;
  o["name"] = name;
  o["source"] = source;
  return o;
}

QStringList LslEventsTest::said(const QJsonObject& answer)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "owner_say") out << QString("%1 %2").arg(e["t"].toDouble()).arg(e["text"].toString());
  }
  return out;
}

static const char* const REZ_SCRIPT =
  "default { state_entry() { llOwnerSay(\"entry\"); } on_rez(integer p) { llOwnerSay(\"rez \" + (string)p); } }";

void LslEventsTest::onRezAtStart()
{
  RlvState rlv;
  const QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"source", REZ_SCRIPT}, {"seconds", 1}}, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"0 entry", "0 rez 0"}));
  // the event entry has the start parameter as its argument
  QJsonObject rez;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"] == "event" && v.toObject()["name"] == "on_rez") rez = v.toObject();
  QCOMPARE(rez["args"].toArray().size(), 1);
  QCOMPARE(rez["args"].toArray()[0].toString(), QString("0"));
}

void LslEventsTest::onRezStartParameter()
{
  RlvState rlv;
  QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"source", REZ_SCRIPT}, {"start_parameter", 42}, {"seconds", 1}}, rlv);
  QCOMPARE(said(a), QStringList({"0 entry", "0 rez 42"}));
  a = run(QJsonObject{{"command", "lsl_run"}, {"source", REZ_SCRIPT}, {"start_parameter", -2147483648.0}, {"seconds", 1}}, rlv);
  QCOMPARE(said(a), QStringList({"0 entry", "0 rez -2147483648"}));
  // two objects, each with its own parameter
  a = run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{
    object("a", REZ_SCRIPT, QJsonObject{{"start_parameter", 7}}), object("b", REZ_SCRIPT)}}}, rlv);
  QCOMPARE(said(a), QStringList({"0 entry", "0 rez 7", "0 entry", "0 rez 0"}));
}

void LslEventsTest::onRezBeforeAttach()
{
  RlvState rlv;
  const QString script = "default { state_entry() { llOwnerSay(\"entry\"); } on_rez(integer p) { llOwnerSay(\"rez \" + (string)p); }"
                         " attach(key id) { llOwnerSay(\"attach\"); } }";
  const QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{
    object("worn", script, QJsonObject{{"attached", 30}, {"start_parameter", 3}})}}}, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"0 entry", "0 rez 3", "0 attach"}));
}

void LslEventsTest::resetScriptDoesNotRaiseOnRez()
{
  // as in Second Life, llResetScript runs state_entry again, but the object is not rezzed again
  RlvState rlv;
  const QString script = "default { state_entry() { llOwnerSay(\"entry\"); } on_rez(integer p) { llOwnerSay(\"rez \" + (string)p); llResetScript(); } }";
  const QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"source", script}, {"start_parameter", 5}, {"seconds", 2}}, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"0 entry", "0 rez 5", "0 entry"}));
}

void LslEventsTest::stateChangeInStateEntryDropsOnRez()
{
  // a state change empties the event queue, so the waiting on_rez is gone, as in Second Life
  RlvState rlv;
  const QString script = "default { state_entry() { llOwnerSay(\"entry\"); state other; } on_rez(integer p) { llOwnerSay(\"rez\"); } }"
                         " state other { state_entry() { llOwnerSay(\"other\"); } on_rez(integer p) { llOwnerSay(\"other rez\"); } }";
  const QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"source", script}, {"seconds", 1}}, rlv);
  QCOMPARE(said(a), QStringList({"0 entry", "0 other"}));
}

void LslEventsTest::changedFlags()
{
  RlvState rlv;
  const QString script = "default { changed(integer c) { llOwnerSay((string)c + \":\" + (string)((c & CHANGED_INVENTORY) != 0) + (string)((c & CHANGED_OWNER) != 0)"
                         " + (string)((c & CHANGED_LINK) != 0) + (string)((c & CHANGED_REGION_START) != 0)); } }";
  const QJsonArray inputs{
    QJsonObject{{"at", 1}, {"type", "changed"}, {"change", "inventory"}},
    QJsonObject{{"at", 2}, {"type", "changed"}, {"change", "owner"}},
    QJsonObject{{"at", 3}, {"type", "changed"}, {"change", "link"}},
    QJsonObject{{"at", 4}, {"type", "changed"}, {"change", "region_start"}}};
  const QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"source", script}, {"inputs", inputs}, {"seconds", 5}}, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"1 1:1000", "2 128:0100", "3 32:0010", "4 1024:0001"}));
  // the input is logged with its flags
  QStringList inputsLogged;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"] == "input") inputsLogged << QString("%1 %2 %3").arg(v.toObject()["t"].toDouble()).arg(v.toObject()["input"].toString()).arg(v.toObject()["change"].toInt());
  QCOMPARE(inputsLogged, QStringList({"1 changed 1", "2 changed 128", "3 changed 32", "4 changed 1024"}));
}

void LslEventsTest::changedNamesListsAndNumbers()
{
  RlvState rlv;
  const QString script = "default { changed(integer c) { llOwnerSay((string)c); } }";
  const QJsonArray inputs{
    QJsonObject{{"at", 1}, {"type", "changed"}, {"change", "CHANGED_OWNER"}},
    QJsonObject{{"at", 2}, {"type", "changed"}, {"change", "Allowed_Drop"}},
    QJsonObject{{"at", 3}, {"type", "changed"}, {"change", QJsonArray{"link", "region_start"}}},
    QJsonObject{{"at", 4}, {"type", "changed"}, {"change", 3}},
    QJsonObject{{"at", 5}, {"type", "changed"}, {"change", "color"}},
    QJsonObject{{"at", 6}, {"type", "changed"}, {"change", "shape"}},
    QJsonObject{{"at", 7}, {"type", "changed"}, {"change", "scale"}},
    QJsonObject{{"at", 8}, {"type", "changed"}, {"change", "texture"}},
    QJsonObject{{"at", 9}, {"type", "changed"}, {"change", "region"}},
    QJsonObject{{"at", 10}, {"type", "changed"}, {"change", "teleport"}},
    QJsonObject{{"at", 11}, {"type", "changed"}, {"change", "media"}}};
  const QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"source", script}, {"inputs", inputs}, {"seconds", 12}}, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"1 128", "2 64", "3 1056", "4 3", "5 2", "6 4", "7 8", "8 16", "9 256", "10 512", "11 2048"}));
}

void LslEventsTest::changedReachesEveryScriptOfTheObject()
{
  RlvState rlv;
  const QString mine = "default { changed(integer c) { llOwnerSay(llGetScriptName() + \" \" + (string)c); } }";
  const QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"seconds", 2}, {"inputs", QJsonArray{
      QJsonObject{{"at", 1}, {"type", "changed"}, {"object", "box"}, {"change", "inventory"}}}},
    {"objects", QJsonArray{
      QJsonObject{{"name", "box"}, {"prims", 2}, {"scripts", QJsonArray{
        QJsonObject{{"name", "main"}, {"source", mine}, {"link", 1}}, QJsonObject{{"name", "helper"}, {"source", mine}, {"link", 2}},
        QJsonObject{{"name", "deaf"}, {"source", "default { state_entry() { } }"}}}}},
      object("other", "default { changed(integer c) { llOwnerSay(\"other heard\"); } }")}}}, rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"1 main 1", "1 helper 1"}));
}

void LslEventsTest::changedWithLslEvent()
{
  RlvState rlv;
  Session session;
  QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"source",
    "default { changed(integer c) { llOwnerSay(\"changed \" + (string)c); } }"}}, rlv, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  a = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "changed"}, {"change", QJsonArray{"inventory", "owner"}}, {"seconds", 1}}, &session);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"1 changed 129"}));
}

void LslEventsTest::changedIsRefusedWhenWrong()
{
  RlvState rlv;
  const QString src = "default { changed(integer c) { } }";
  auto error = [&](const QJsonObject& input)
  {
    const QJsonObject a = run(QJsonObject{{"command", "lsl_run"}, {"source", src}, {"inputs", QJsonArray{input}}}, rlv);
    return a["ok"].toBool() ? QString() : a["error"].toString();
  };
  QVERIFY2(error(QJsonObject{{"type", "changed"}}).contains("needs \"change\""), "missing change");
  QVERIFY(error(QJsonObject{{"type", "changed"}, {"change", "weather"}}).contains("\"change\" must be a name"));
  QVERIFY(error(QJsonObject{{"type", "changed"}, {"change", 0}}).contains("\"change\" must be a name"));
  QVERIFY(error(QJsonObject{{"type", "changed"}, {"change", 65536}}).contains("\"change\" must be a name"));
  QVERIFY(error(QJsonObject{{"type", "changed"}, {"change", QJsonArray()}}).contains("\"change\" must be a name"));
  QVERIFY(error(QJsonObject{{"type", "changed"}, {"change", QJsonArray{"owner", 5}}}).contains("\"change\" must be a name"));
  QVERIFY(error(QJsonObject{{"type", "touch"}, {"change", "owner"}}).contains("\"change\" is only for changed"));
  QVERIFY(error(QJsonObject{{"type", "changed"}, {"change", "owner"}, {"object", "ghost"}}).contains("there is no object named \"ghost\""));
  QCOMPARE(error(QJsonObject{{"type", "changed"}, {"change", "owner"}}), QString());
  // the type list in the message
  QVERIFY(error(QJsonObject{{"type", "weather"}}).contains("\"sit\", \"stand\", \"control\", \"money\", \"http_request\", "
                                                            "\"email\", \"remote_data\", \"collision\", \"land_collision\", \"moving\", \"at_target\", \"not_at_target\", \"step\", \"drop\", \"experience\", "));
}

void LslEventsTest::checkerKnowsBothEvents()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source",
    "default { on_rez(integer p) { } changed(integer c) { } }"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  // an event that is still never raised keeps its warning (path_update: pathfinding, not part of the second gap list)
  const QJsonObject b = Control::check(QJsonObject{{"command", "lsl_check"},
    {"source", "default { path_update(integer type, list reserved) { } }"}});
  QCOMPARE(b["warnings"].toArray().size(), 1);
}

QTEST_GUILESS_MAIN(LslEventsTest)
#include "tst_lslevents.moc"
