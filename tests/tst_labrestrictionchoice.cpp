/*
 * PinglyMotion Script Lab - choose a restriction from the list (job A2): exact lines and words
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "scriptlab/labrestrictionchoice.h"
#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"

using namespace LabRestrictionChoice;

class TestLabRestrictionChoice : public QObject
{
  Q_OBJECT

  private slots:
    void sendim();
    void everyRowBlocksItsAction_data();
    void everyRowBlocksItsAction();
    void distanceField();
    void channelField();
    void allowLiftsOnlyYourOwn();
    void modes();
    void wordsAreShort();
    void scriptLineAndPlace();
};

void TestLabRestrictionChoice::sendim()
{
  const Choice c = forAction("sendim", Rlv::Mode::Rlva);
  QVERIFY(c.ok());
  QCOMPARE(c.label, QString("Send instant messages"));
  QCOMPARE(c.behaviour, QString("sendim"));
  QCOMPARE(int(c.value), int(ValueKind::None));
  QCOMPARE(c.example, Rlv::findHelp("sendim")->example);
  QCOMPARE(c.sentence, Rlv::findHelp("sendim")->sentence);
  QCOMPARE(blockLine(c, QString()).text, QString("@sendim=n"));
  QCOMPARE(blockLine(c, "anything").text, QString("@sendim=n"));   // no field: a value is not used
  QCOMPARE(explainBlock(c, QString()),
           QStringList({"This line blocks: Send instant messages.", Rlv::findHelp("sendim")->sentence,
                        "@ starts an RLV line, and sendim is the command.", "=n means no: the avatar may not do this."}));
  QCOMPARE(explainAllow(c), QStringList({"This line lifts the block again.", "=y means yes: the avatar may do this again."}));
}

void TestLabRestrictionChoice::everyRowBlocksItsAction_data()
{
  QTest::addColumn<QString>("action");
  for (const Rlv::Action& a : Rlv::actions()) QTest::newRow(qPrintable(a.id)) << a.id;
}

// every row of the list: Blocked runs a line that really blocks that row, Allowed lifts it again
void TestLabRestrictionChoice::everyRowBlocksItsAction()
{
  QFETCH(QString, action);
  bool someMode = false;
  for (Rlv::Mode mode : {Rlv::Mode::Rlva, Rlv::Mode::Rlv})
  {
    const Choice c = forAction(action, mode);
    if (!c.ok())
    {
      QVERIFY(!c.reason.isEmpty());
      continue;
    }
    someMode = true;
    RlvState state;
    state.setMode(mode);
    const Line line = blockLine(c, QString());
    QVERIFY2(line.ok(), qPrintable(line.error));
    const RlvState::ApplyResult applied = state.apply("panel", line.text);
    QVERIFY2(applied.ok(), qPrintable(applied.error));
    QCOMPARE(applied.outcomes.size(), 1);
    QCOMPARE(int(applied.outcomes[0].result), int(Rlv::Result::Success));
    QCOMPARE(state.actions().value(action, true), false);
    const QString lift = allowLine(c, state.restrictions(), "panel");
    QCOMPARE(lift, QStringLiteral("@%1=y").arg(c.behaviour));
    QVERIFY(state.apply("panel", lift).ok());
    QCOMPARE(state.actions().value(action, false), true);
    QVERIFY(state.restrictions().isEmpty());
  }
  QVERIFY2(someMode, "no command blocks this row in either mode");
}

void TestLabRestrictionChoice::distanceField()
{
  const Choice c = forAction("touchfar", Rlv::Mode::Rlva);
  QCOMPARE(c.behaviour, QString("touchfar"));
  QCOMPARE(int(c.value), int(ValueKind::Distance));
  QCOMPARE(blockLine(c, QString()).text, QString("@touchfar=n"));
  QCOMPARE(blockLine(c, " 3 ").text, QString("@touchfar:3=n"));
  QCOMPARE(blockLine(c, "2.5").text, QString("@touchfar:2.5=n"));
  QCOMPARE(blockLine(c, "far").error, QString("Type a distance in metres, such as 1.5."));
  QCOMPARE(blockLine(c, "-1").error, QString("Type a distance in metres, such as 1.5."));
  QCOMPARE(blockLine(c, "0").error, QString("Type a distance in metres, such as 1.5."));
  QCOMPARE(explainBlock(c, "3")[3], QString(":3 is the distance in metres."));
  QCOMPARE(int(forAction("sittp", Rlv::Mode::Rlva).value), int(ValueKind::Distance));
  QCOMPARE(int(forAction("tplocal", Rlv::Mode::Rlva).value), int(ValueKind::Distance));
  RlvState state;
  QVERIFY(state.apply("panel", "@touchfar:3=n").ok());
  QCOMPARE(heldDistance(c, state.restrictions()), QString("3"));
  QCOMPARE(allowLine(c, state.restrictions(), "panel"), QString("@touchfar:3=y"));
  QVERIFY(state.apply("panel", "@touchfar:3=y").ok());
  QVERIFY(state.restrictions().isEmpty());
  QCOMPARE(heldDistance(c, state.restrictions()), QString());
}

void TestLabRestrictionChoice::channelField()
{
  const Choice c = forAction("sendchannel", Rlv::Mode::Rlva);
  QCOMPARE(c.behaviour, QString("sendchannel"));
  QCOMPARE(int(c.value), int(ValueKind::ExceptChannel));
  QCOMPARE(blockLine(c, "5").text, QString("@sendchannel=n,sendchannel:5=add"));
  QCOMPARE(blockLine(c, "0").error, QString("Type a channel number from 1, such as 5."));
  QCOMPARE(blockLine(c, "x").error, QString("Type a channel number from 1, such as 5."));
  QCOMPARE(explainBlock(c, "5").last(), QString("sendchannel:5=add keeps channel 5 open."));
  RlvState state;
  QVERIFY(state.apply("panel", "@sendchannel=n,sendchannel:5=add").ok());
  QCOMPARE(state.actions().value("sendchannel"), false);
  QCOMPARE(allowLine(c, state.restrictions(), "panel"), QString("@sendchannel=y,sendchannel:5=rem"));
  QVERIFY(state.apply("panel", allowLine(c, state.restrictions(), "panel")).ok());
  QVERIFY(state.restrictions().isEmpty());
  QCOMPARE(explainAllow(c).last(), QString("=rem closes a channel that was kept open."));
}

void TestLabRestrictionChoice::allowLiftsOnlyYourOwn()
{
  const Choice c = forAction("detach", Rlv::Mode::Rlva);
  RlvState state;
  QVERIFY(state.apply("collar", "@detach=n").ok());
  QCOMPARE(allowLine(c, state.restrictions(), "panel"), QString());   // the panel holds nothing
  QCOMPARE(otherHolders(c, state.restrictions(), "panel"), QStringList({"collar"}));
  QVERIFY(state.apply("panel", "@detach=n").ok());
  QCOMPARE(allowLine(c, state.restrictions(), "panel"), QString("@detach=y"));
  QCOMPARE(otherHolders(c, state.restrictions(), "panel"), QStringList({"collar"}));
  QCOMPARE(otherHolders(c, state.restrictions(), "collar"), QStringList({"panel"}));
}

void TestLabRestrictionChoice::modes()
{
  // jump is RLVa only: in plain RLV mode the row says why it has no line
  QVERIFY(forAction("jump", Rlv::Mode::Rlva).ok());
  const Choice plain = forAction("jump", Rlv::Mode::Rlv);
  QVERIFY(!plain.ok());
  QCOMPARE(plain.reason, QString("Only RLVa has this. Switch Viewer rules to RLVa."));
  QCOMPARE(blockLine(plain, QString()).error, plain.reason);
  QCOMPARE(forAction("nothing", Rlv::Mode::Rlva).reason, QString("The sandbox does not know this action."));
}

// every sentence a row can show has at most 12 words (docs/TEACHING.md)
void TestLabRestrictionChoice::wordsAreShort()
{
  for (Rlv::Mode mode : {Rlv::Mode::Rlva, Rlv::Mode::Rlv})
    for (const Rlv::Action& a : Rlv::actions())
    {
      const Choice c = forAction(a.id, mode);
      QStringList texts = explainBlock(c, "12.5") + explainAllow(c);
      if (!c.ok()) texts << c.reason;
      for (const QString& t : texts)
        QVERIFY2(t.split(' ', Qt::SkipEmptyParts).size() <= 12, qPrintable(a.id + ": " + t));
    }
}

void TestLabRestrictionChoice::scriptLineAndPlace()
{
  QCOMPARE(scriptLine("@sendim=n"), QString("llOwnerSay(\"@sendim=n\");"));
  const QString code = scriptLine("@sendim=n");

  // the cursor outside every event: into state_entry
  const QString script = "default\n{\n    state_entry()\n    {\n        llOwnerSay(\"hi\");\n    }\n\n    touch_start(integer n)\n    {\n    }\n}\n";
  Insertion in = insert(script, 0, code);
  QCOMPARE(in.text, QString("default\n{\n    state_entry()\n    {\n        llOwnerSay(\"@sendim=n\");\n        llOwnerSay(\"hi\");\n    }\n\n    touch_start(integer n)\n    {\n    }\n}\n"));
  QCOMPARE(in.line, 5);
  QCOMPARE(in.text.mid(in.position, code.size()), code);

  // the cursor inside touch_start's body: after the cursor's line
  const int inTouch = script.indexOf("    {\n    }\n}") + 5;   // just after touch_start's "{"
  in = insert(script, inTouch, code);
  QCOMPARE(in.text, QString("default\n{\n    state_entry()\n    {\n        llOwnerSay(\"hi\");\n    }\n\n    touch_start(integer n)\n    {\n        llOwnerSay(\"@sendim=n\");\n    }\n}\n"));
  QCOMPARE(in.line, 10);

  // the cursor on the line with llOwnerSay("hi"): the new line comes under it, same indent
  in = insert(script, script.indexOf("llOwnerSay"), code);
  QCOMPARE(in.line, 6);
  QVERIFY(in.text.contains("        llOwnerSay(\"hi\");\n        llOwnerSay(\"@sendim=n\");\n"));

  // no state_entry: the first event; braces in strings and comments do not count
  const QString tricky = "// { not code\ndefault { touch_start(integer n) { llSay(0, \"}{\"); } }";
  in = insert(tricky, 0, code);
  QCOMPARE(in.text, QString("// { not code\ndefault { touch_start(integer n) {\n    llOwnerSay(\"@sendim=n\"); llSay(0, \"}{\"); } }"));
  QCOMPARE(in.line, 3);

  // no event at all: at the cursor
  in = insert("", 0, code);
  QCOMPARE(in.text, QString("\n") + code);
  QCOMPARE(in.line, 2);
}

QTEST_GUILESS_MAIN(TestLabRestrictionChoice)
#include "tst_labrestrictionchoice.moc"
