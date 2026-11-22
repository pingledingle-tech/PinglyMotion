/*
 * PinglyMotion - tests for the Script Lab's RLV restriction state (issue 287)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QSignalSpy>
#include <QtTest>

#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"

class TestRlvState : public QObject
{
  Q_OBJECT

  private slots:
    void addAndRemove();
    void duplicate();
    void clearAll();
    void clearWithFilter();
    void unknownBehaviour();
    void badSyntaxInALine();
    void multipleCommandsInOneLine();
    void replyOnChannel();
    void version();
    void getStatus();
    void options();
    void twoObjectsSameRestriction();
    void forceIsRetained();
    void reset();
    void limits();
    void changedSignal();

  private:
    static QList<Rlv::Result> results(const RlvState::ApplyResult& r)
    {
      QList<Rlv::Result> list;
      for (const RlvState::Outcome& o : r.outcomes) list << o.result;
      return list;
    }
};

void TestRlvState::addAndRemove()
{
  RlvState state;
  RlvState::ApplyResult r = state.apply("collar", "@detach=n");
  QVERIFY(r.ok());
  QCOMPARE(results(r), QList<Rlv::Result>{Rlv::Result::Success});
  QVERIFY(state.has("detach"));
  QCOMPARE(state.actions().value("detach"), false);
  QCOMPARE(state.actions().value("fly"), true);

  r = state.apply("collar", "@detach=y");
  QCOMPARE(results(r), QList<Rlv::Result>{Rlv::Result::Success});
  QVERIFY(!state.has("detach"));
  QCOMPARE(state.actions().value("detach"), true);
  QVERIFY(state.objects().isEmpty());   // an object without restrictions is dropped

  // removing what is not set is a success of its own kind
  r = state.apply("collar", "@detach=y");
  QCOMPARE(results(r), QList<Rlv::Result>{Rlv::Result::Unset});
}

void TestRlvState::duplicate()
{
  RlvState state;
  state.apply("collar", "@fly=n");
  const RlvState::ApplyResult r = state.apply("collar", "@fly=add");
  QCOMPARE(results(r), QList<Rlv::Result>{Rlv::Result::Duplicate});
  QCOMPARE(state.count("fly"), 1);
  QCOMPARE(state.restrictions().size(), 1);
}

void TestRlvState::clearAll()
{
  RlvState state;
  state.apply("collar", "@detach=n,sendim=n");
  state.apply("hud", "@fly=n");
  const RlvState::ApplyResult r = state.apply("collar", "@clear");
  QCOMPARE(results(r), QList<Rlv::Result>{Rlv::Result::Success});
  // only the sending object's restrictions go
  QVERIFY(!state.has("detach"));
  QVERIFY(!state.has("sendim"));
  QVERIFY(state.has("fly"));
  QCOMPARE(state.objects().keys(), QStringList{"hud"});
  // @clear from an object without restrictions is fine
  QCOMPARE(results(state.apply("nobody", "@clear")), QList<Rlv::Result>{Rlv::Result::Success});
}

void TestRlvState::clearWithFilter()
{
  RlvState state;
  state.apply("collar", "@tplm=n,tploc=n,tplure=n,fly=n,sendim:abc=n");
  state.apply("collar", "@clear=tp");
  QVERIFY(!state.has("tplm"));
  QVERIFY(!state.has("tploc"));
  QVERIFY(!state.has("tplure"));
  QVERIFY(state.has("fly"));
  // the filter also matches the option part
  state.apply("collar", "@clear=abc");
  QVERIFY(!state.has("sendim", "abc"));
  QCOMPARE(state.objects().value("collar"), QStringList{"fly"});
}

void TestRlvState::unknownBehaviour()
{
  RlvState state;
  const RlvState::ApplyResult r = state.apply("collar", "@dance=n");
  QVERIFY(r.ok());   // the line itself is fine
  QCOMPARE(results(r), QList<Rlv::Result>{Rlv::Result::Failed});
  QVERIFY(r.outcomes[0].reason.contains("unknown behaviour"));
  QVERIFY(state.restrictions().isEmpty());
}

void TestRlvState::badSyntaxInALine()
{
  RlvState state;
  RlvState::ApplyResult r = state.apply("collar", "@detach,fly=n,sendim=maybe");
  QVERIFY(r.ok());
  QCOMPARE(results(r), (QList<Rlv::Result>{Rlv::Result::Failed, Rlv::Result::Success, Rlv::Result::Failed}));
  QVERIFY(!r.outcomes[0].reason.isEmpty());
  QVERIFY(!r.outcomes[2].reason.isEmpty());
  QCOMPARE(state.objects().value("collar"), QStringList{"fly"});

  // a line without "@" is refused as a whole
  r = state.apply("collar", "detach=n");
  QVERIFY(!r.ok());
  QVERIFY(r.outcomes.isEmpty());
}

void TestRlvState::multipleCommandsInOneLine()
{
  RlvState state;
  const RlvState::ApplyResult r = state.apply("collar", "@detach=n,sendim=n");
  QCOMPARE(results(r), (QList<Rlv::Result>{Rlv::Result::Success, Rlv::Result::Success}));
  QCOMPARE(state.restrictions().size(), 2);
  QCOMPARE(state.objects().value("collar"), (QStringList{"detach", "sendim"}));
  QCOMPARE(state.actions().value("detach"), false);
  QCOMPARE(state.actions().value("sendim"), false);
}

void TestRlvState::replyOnChannel()
{
  RlvState state;
  const RlvState::ApplyResult r = state.apply("collar", "@versionnum=12345,fly=n,versionnum=7");
  QCOMPARE(r.replies.size(), 2);
  QCOMPARE(r.replies[0].channel, 12345);
  QCOMPARE(r.replies[0].text, QString("3040300"));
  QCOMPARE(r.replies[1].channel, 7);
  QVERIFY(state.has("fly"));
}

void TestRlvState::version()
{
  RlvState state;
  const RlvState::ApplyResult r = state.apply("collar", "@version=1,versionnew=2,versionnum=3");
  QCOMPARE(r.replies.size(), 3);
  QCOMPARE(r.replies[0].text, QString("RestrainedLife viewer v3.4.3 (PinglyMotion Script Lab)"));
  QCOMPARE(r.replies[1].text, QString("RestrainedLove viewer v3.4.3 (PinglyMotion Script Lab)"));
  QCOMPARE(r.replies[2].text, QString("3040300"));
  QVERIFY(state.restrictions().isEmpty());   // questions hold nothing
}

void TestRlvState::getStatus()
{
  RlvState state;
  state.apply("collar", "@detach=n,sendim=n,sendim:abc=n");
  state.apply("hud", "@fly=n");

  RlvState::ApplyResult r = state.apply("collar", "@getstatus=2222");
  QCOMPARE(r.replies.size(), 1);
  QCOMPARE(r.replies[0].channel, 2222);
  QCOMPARE(r.replies[0].text, QString("/detach/sendim/sendim:abc"));

  r = state.apply("collar", "@getstatus:send=2222");
  QCOMPARE(r.replies[0].text, QString("/sendim/sendim:abc"));

  r = state.apply("collar", "@getstatus:send;|=2222");
  QCOMPARE(r.replies.size(), 1);
  QCOMPARE(r.replies[0].text, QString("|sendim|sendim:abc"));

  r = state.apply("collar", "@getstatusall=5");
  QCOMPARE(r.replies[0].text, QString("/detach/sendim/sendim:abc/fly"));

  // an object without restrictions gets an empty answer
  r = state.apply("other", "@getstatus=5");
  QCOMPARE(r.replies.size(), 1);
  QVERIFY(r.replies[0].text.isEmpty());
}

void TestRlvState::options()
{
  RlvState state;
  const QString uuid = "00000000-0000-0000-0000-000000000001";
  state.apply("collar", "@sendim:" + uuid + "=n");
  QVERIFY(state.has("sendim", uuid));
  QVERIFY(!state.has("sendim"));
  // an exception alone does not block sending IMs
  QCOMPARE(state.actions().value("sendim"), true);

  state.apply("collar", "@sendim=n");
  QCOMPARE(state.actions().value("sendim"), false);
  QCOMPARE(state.restrictions().size(), 2);
  QCOMPARE(state.restrictions()[0].option, uuid);

  state.apply("collar", "@sendim:" + uuid + "=y");
  QVERIFY(!state.has("sendim", uuid));
  QVERIFY(state.has("sendim"));
}

void TestRlvState::twoObjectsSameRestriction()
{
  RlvState state;
  state.apply("collar", "@detach=n");
  state.apply("cuffs", "@detach=n");
  QCOMPARE(state.count("detach"), 2);
  QCOMPARE(state.restrictions().size(), 2);

  state.apply("collar", "@detach=y");
  QCOMPARE(state.count("detach"), 1);
  QVERIFY(state.has("detach"));   // the cuffs still hold it
  QCOMPARE(state.actions().value("detach"), false);

  state.apply("cuffs", "@clear");
  QCOMPARE(state.count("detach"), 0);
  QCOMPARE(state.actions().value("detach"), true);
}

void TestRlvState::forceIsRetained()
{
  RlvState state;
  const RlvState::ApplyResult r = state.apply("collar", "@unsit=force,fly=force");
  QCOMPARE(results(r), (QList<Rlv::Result>{Rlv::Result::Retained, Rlv::Result::Failed}));
  QVERIFY(!r.outcomes[0].reason.isEmpty());
  QVERIFY(state.restrictions().isEmpty());
}

void TestRlvState::reset()
{
  RlvState state;
  state.apply("collar", "@detach=n,sendim=n");
  state.apply("hud", "@fly=n");
  state.reset();
  QVERIFY(state.restrictions().isEmpty());
  QVERIFY(state.objects().isEmpty());
  QCOMPARE(state.count("detach"), 0);
  for (bool allowed : state.actions()) QVERIFY(allowed);
}

void TestRlvState::limits()
{
  RlvState state;
  QVERIFY(!state.apply("", "@fly=n").ok());
  QVERIFY(!state.apply(QString(RlvState::MAX_OBJECT_ID_LENGTH + 1, 'a'), "@fly=n").ok());
  QVERIFY(!state.apply("bad/id", "@fly=n").ok());
  QVERIFY(!state.apply(" collar", "@fly=n").ok());
  QVERIFY(state.apply("Collar 2.0_x-y", "@fly=n").ok());
  state.reset();

  for (int i = 0; i < RlvState::MAX_OBJECTS; i++)
    QCOMPARE(results(state.apply(QString("obj%1").arg(i), "@fly=n")), QList<Rlv::Result>{Rlv::Result::Success});
  const RlvState::ApplyResult r = state.apply("one-too-many", "@fly=n");
  QCOMPARE(results(r), QList<Rlv::Result>{Rlv::Result::Failed});
  QCOMPARE(state.objects().size(), RlvState::MAX_OBJECTS);
  state.reset();

  for (int i = 0; i < RlvState::MAX_RESTRICTIONS_PER_OBJECT; i++)
    state.apply("collar", QString("@sendim:%1=n").arg(i));
  QCOMPARE(state.restrictions().size(), RlvState::MAX_RESTRICTIONS_PER_OBJECT);
  QCOMPARE(results(state.apply("collar", "@fly=n")), QList<Rlv::Result>{Rlv::Result::Failed});

  // a long answer is cut to one chat message
  const RlvState::ApplyResult status = state.apply("collar", "@getstatus=9");
  QCOMPARE(status.replies[0].text.length(), 1023);
}

void TestRlvState::changedSignal()
{
  RlvState state;
  QSignalSpy spy(&state, &RlvState::changed);
  state.apply("collar", "@detach=n,sendim=n");
  QCOMPARE(spy.count(), 1);   // once per line
  state.apply("collar", "@version=5");
  QCOMPARE(spy.count(), 1);   // a question changes nothing
  state.apply("collar", "@detach=n");
  QCOMPARE(spy.count(), 1);   // a duplicate changes nothing
  state.apply("collar", "@clear");
  QCOMPARE(spy.count(), 2);
  state.reset();
  QCOMPARE(spy.count(), 2);   // nothing to reset
}

QTEST_GUILESS_MAIN(TestRlvState)
#include "tst_rlvstate.moc"
