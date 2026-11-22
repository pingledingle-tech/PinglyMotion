/*
 * PinglyMotion - tests for the touch phases of the Script Lab sandbox (gap list 2, step 1)
 *
 * A touch raises touch_start, then touch while it is held (once for a click, then every 0.1 simulated seconds),
 * then touch_end when it is let go, as in Second Life. Every expected value is exact and was worked out by hand
 * from the rules in docs/CONTROL.md ("Touches"): a touch at t held h seconds gives touch_start and touch at t,
 * touch at t + 0.1, t + 0.2, ... while that is before t + h, and touch_end at t + h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const GUEST = "11111111-2222-3333-4444-555555555555";

  // says every phase with the time, the number detected and the toucher
  const char* const ALL_PHASES = R"lsl(
    report(string phase, integer n)
    {
      llOwnerSay(phase + " " + (string)n + " " + llDetectedName(0) + " " + (string)llDetectedKey(0) + " "
                 + (string)llDetectedOwner(0) + " " + (string)llDetectedLinkNumber(0));
    }
    default
    {
      touch_start(integer n) { report("start", n); }
      touch(integer n) { report("touch", n); }
      touch_end(integer n) { report("end", n); }
    }
  )lsl";
}

class LslTouchTest : public QObject
{
  Q_OBJECT

  private slots:
    void clickRaisesAllThree();
    void heldTouchRepeats();
    void releaseOnARepeat();
    void anotherToucher();
    void onlyHandledEvents();
    void touchGoesToThePrimWithAnyTouchHandler();
    void touchEventsInLslEvent();
    void stateChangeWhileHeld();
    void refusesBadHold();
    void checkerKnowsTheEvents();

  private:
    static QJsonObject runSource(const QString& source, const QJsonArray& inputs, double seconds = 2, RlvState* rlv = nullptr,
                                 Session* session = nullptr);
    static QStringList events(const QJsonObject& answer, const QString& type = "owner_say");
};

QJsonObject LslTouchTest::runSource(const QString& source, const QJsonArray& inputs, double seconds, RlvState* rlv, Session* session)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"inputs", inputs}, {"seconds", seconds}}, rlv, session);
}

// "t text" for every entry of the type ("t name" for event entries), with the script name when there is one
QStringList LslTouchTest::events(const QJsonObject& answer, const QString& type)
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

void LslTouchTest::clickRaisesAllThree()
{
  // a click (no hold): touch_start, touch and touch_end at the same time, one toucher each (the owner)
  const QJsonObject r = runSource(ALL_PHASES, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}});
  const QString who = QString("Owner %1 %1 0").arg(OWNER);
  QCOMPARE(events(r), QStringList({"1 start 1 " + who, "1 touch 1 " + who, "1 end 1 " + who}));
  QCOMPARE(events(r, "event"), QStringList({"1 touch_start", "1 touch", "1 touch_end"}));
  // one input entry for the touch, without "hold"
  QStringList inputs;
  for (const QJsonValue& v : r["log"].toArray())
    if (v.toObject()["type"].toString() == "input") inputs << QString::fromUtf8(QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact));
  QCOMPARE(inputs, QStringList({R"({"input":"touch","name":"Owner","t":1,"type":"input"})"}));
  QCOMPARE(r["warnings"].toArray().size(), 0);
}

void LslTouchTest::heldTouchRepeats()
{
  // held 0.35 s from t = 1: touch at 1, 1.1, 1.2 and 1.3, touch_end at 1.35
  const QJsonObject r = runSource(ALL_PHASES, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"hold", 0.35}}});
  QCOMPARE(events(r, "event"), QStringList({"1 touch_start", "1 touch", "1.1 touch", "1.2 touch", "1.3 touch", "1.35 touch_end"}));
  QStringList inputs;
  for (const QJsonValue& v : r["log"].toArray())
    if (v.toObject()["type"].toString() == "input") inputs << QString::fromUtf8(QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact));
  QCOMPARE(inputs, QStringList({R"({"hold":0.35,"input":"touch","name":"Owner","t":1,"type":"input"})"}));
  // a held touch counts the touches with llGetTime
  const QJsonObject c = runSource(R"lsl(
    integer touches;
    default
    {
      touch_start(integer n) { touches = 0; }
      touch(integer n) { ++touches; }
      touch_end(integer n) { llOwnerSay((string)touches + " touches, held " + llGetSubString((string)llGetTime(), 0, 3)); }
    }
  )lsl", QJsonArray{QJsonObject{{"at", 0.5}, {"type", "touch"}, {"hold", 2}}}, 3);
  QCOMPARE(events(c), QStringList({"2.5 20 touches, held 2.50"}));
}

void LslTouchTest::releaseOnARepeat()
{
  // held 0.2 s: the repeat at 1.2 is the release, so touch_end comes there and no touch
  const QJsonObject r = runSource(ALL_PHASES, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"hold", 0.2}}});
  QCOMPARE(events(r, "event"), QStringList({"1 touch_start", "1 touch", "1.1 touch", "1.2 touch_end"}));
  // a touch let go after the end of the run: touch_end does not come in this run
  const QJsonObject late = runSource(ALL_PHASES, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"hold", 5}}}, 1.15);
  QCOMPARE(events(late, "event"), QStringList({"1 touch_start", "1 touch", "1.1 touch"}));
}

void LslTouchTest::anotherToucher()
{
  // another avatar touches: llDetectedName, Key and Owner are that avatar in every phase
  const QJsonObject r = runSource(ALL_PHASES, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"name", "Guest Resident"}, {"id", GUEST}, {"hold", 0.1}}});
  const QString who = QString("Guest Resident %1 %1 0").arg(GUEST);
  QCOMPARE(events(r), QStringList({"1 start 1 " + who, "1 touch 1 " + who, "1.1 end 1 " + who}));
}

void LslTouchTest::onlyHandledEvents()
{
  // a script with only touch_start works as before: one event per touch, held or not
  const QJsonObject r = runSource("default { touch_start(integer n) { llOwnerSay(\"start\"); } }",
                                  QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}, QJsonObject{{"at", 2}, {"type", "touch"}, {"hold", 0.5}}}, 3);
  QCOMPARE(events(r), QStringList({"1 start", "2 start"}));
  QCOMPARE(events(r, "event"), QStringList({"1 touch_start", "2 touch_start"}));
  // only touch_end
  const QJsonObject e = runSource("default { touch_end(integer n) { llOwnerSay(\"end \" + (string)n); } }",
                                  QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"hold", 0.5}}});
  QCOMPARE(events(e), QStringList({"1.5 end 1"}));
}

void LslTouchTest::touchGoesToThePrimWithAnyTouchHandler()
{
  RlvState rlv;
  auto script = [](const QString& name, const QString& source, int link) { return QJsonObject{{"name", name}, {"source", source}, {"link", link}}; };
  const QString start = "default { touch_start(integer n) { llOwnerSay(\"start \" + (string)llDetectedLinkNumber(0)); } }";
  const QString end = "default { touch_end(integer n) { llOwnerSay(\"end \" + (string)llDetectedLinkNumber(0)); } }";
  const QString held = "default { touch(integer n) { llOwnerSay(\"touch \" + (string)llDetectedLinkNumber(0)); } }";
  const QString quiet = "default { state_entry() { } }";
  // prim 2 has only a touch_end handler: it keeps its touches (the root gets nothing); prim 3 has a touch handler;
  // prim 4 has none, so its touches go on to the root
  const QJsonObject o{{"name", "box"}, {"prims", 4},
                      {"scripts", QJsonArray{script("root", start, 1), script("second", end, 2), script("third", held, 3), script("fourth", quiet, 4)}}};
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{o}}, {"seconds", 4},
                                                 {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"link", 2}},
                                                                       QJsonObject{{"at", 2}, {"type", "touch"}, {"link", 3}, {"hold", 0.15}},
                                                                       QJsonObject{{"at", 3}, {"type", "touch"}, {"link", 4}}}}},
                                     &rlv);
  QCOMPARE(events(r), QStringList({"1 second end 2", "2 third touch 3", "2.1 third touch 3", "3 root start 4"}));
}

void LslTouchTest::touchEventsInLslEvent()
{
  RlvState rlv;
  Session session;
  const QJsonObject r = runSource(ALL_PHASES, QJsonArray(), 1, &rlv, &session);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  // a click with lsl_event
  QJsonObject e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}}, &session);
  QCOMPARE(events(e, "event"), QStringList({"1 touch_start", "1 touch", "1 touch_end"}));
  QCOMPARE(e["time"].toDouble(), 2.0);
  // held 1.25 s without "seconds": it runs on until the touch is let go
  e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"hold", 1.25}}, &session);
  QStringList expected({"2 touch_start", "2 touch"});
  for (int i = 1; i <= 12; ++i) expected << QString::number(2.0 + i / 10.0) + " touch";
  expected << "3.25 touch_end";
  QCOMPARE(events(e, "event"), expected);
  QCOMPARE(e["time"].toDouble(), 3.25);
  // held longer than "seconds": touch_end comes with the next lsl_event
  e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"hold", 0.3}, {"seconds", 0.15}}, &session);
  QCOMPARE(events(e, "event"), QStringList({"3.25 touch_start", "3.25 touch", "3.35 touch"}));
  e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "chat"}, {"text", "x"}, {"seconds", 1}}, &session);
  QCOMPARE(events(e, "event"), QStringList({"3.45 touch", "3.55 touch_end"}));
}

void LslTouchTest::stateChangeWhileHeld()
{
  // touch_start changes state; the new state has only touch_end, so the touches while held are not handled there
  const QJsonObject r = runSource(R"lsl(
    default { touch_start(integer n) { state held; } }
    state held { touch_end(integer n) { llOwnerSay("let go"); state default; } }
  )lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"hold", 0.3}}});
  QCOMPARE(events(r), QStringList({"1.3 let go"}));
  QCOMPARE(events(r, "event"), QStringList({"1 touch_start", "1.3 touch_end"}));
}

void LslTouchTest::refusesBadHold()
{
  auto error = [](const QJsonObject& input) { return runSource(ALL_PHASES, QJsonArray{input})["error"].toString(); };
  QCOMPARE(error(QJsonObject{{"type", "chat"}, {"hold", 1}}), QString("Input 1: \"hold\" is only for touch (the seconds the touch is held)."));
  const QString range = "Input 1: \"hold\" must be a number of seconds from 0 to 60 (how long the touch is held; 0 is a click).";
  QCOMPARE(error(QJsonObject{{"type", "touch"}, {"hold", -1}}), range);
  QCOMPARE(error(QJsonObject{{"type", "touch"}, {"hold", 60.5}}), range);
  QCOMPARE(error(QJsonObject{{"type", "touch"}, {"hold", "1"}}), range);
  RlvState rlv;
  Session session;
  runSource(ALL_PHASES, QJsonArray(), 1, &rlv, &session);
  QCOMPARE(Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "dialog"}, {"button", "OK"}, {"hold", 1}}, &session)["error"].toString(),
           QString("\"hold\" is only for touch (the seconds the touch is held)."));
}

void LslTouchTest::checkerKnowsTheEvents()
{
  // touch and touch_end are raised now, so the checker does not warn about them
  QVERIFY(Parser::eventIsRaised("touch"));
  QVERIFY(Parser::eventIsRaised("touch_end"));
  const Parser::Result r = Parser::parse(ALL_PHASES);
  QVERIFY(r.ok());
  QCOMPARE(int(r.warnings.size()), 0);
}

QTEST_GUILESS_MAIN(LslTouchTest)
#include "tst_lsltouch.moc"
