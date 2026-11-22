/*
 * PinglyMotion - exact-answer tests for the Script Lab's RLV behaviour table
 * (old names, chat and IM families, outfits, touch, replies ...), issue 294
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"

class TestRlvTable : public QObject
{
  Q_OBJECT

  private slots:
    void aliasRowsFollowTheirTarget_data();
    void aliasRowsFollowTheirTarget();
    void aliasSharesState_data();
    void aliasSharesState();
    void aliasAcrossObjects();
    void getstatusShowsTheNameUsed();
    void aliasBlocksTheSameAction();
    void aliasClearAndRemove();
    void zoomLimitsAreRecordedOnly();
    void touchFar();

  private:
    static RlvState::Outcome one(RlvState& state, const QString& line, const QString& object = "obj")
    {
      return state.apply(object, line).outcomes.value(0);
    }
};

static const QList<QPair<QString, QString>> aliases = {
  {"camdistmax", "setcam_avdistmax"}, {"camdistmin", "setcam_avdistmin"}, {"camavdist", "setcam_avdist"},
  {"camtextures", "setcam_textures"}, {"camunlock", "setcam_unlock"}, {"fartouch", "touchfar"}};

void TestRlvTable::aliasRowsFollowTheirTarget_data()
{
  QTest::addColumn<QString>("alias");
  QTest::addColumn<QString>("target");
  for (const auto& pair : aliases) QTest::newRow(qPrintable(pair.first)) << pair.first << pair.second;
}

void TestRlvTable::aliasRowsFollowTheirTarget()
{
  QFETCH(QString, alias);
  QFETCH(QString, target);
  const Rlv::Behaviour* a = Rlv::findBehaviour(alias);
  const Rlv::Behaviour* t = Rlv::findBehaviour(target);
  QVERIFY(a);
  QVERIFY(t);
  QCOMPARE(a->alias, target);
  QVERIFY(t->alias.isEmpty());
  QCOMPARE(a->params, t->params);
  QCOMPARE(a->option, t->option);
  QCOMPARE(a->dialect, t->dialect);
  QCOMPARE(a->group, t->group);
  QCOMPARE(a->external, t->external);
  QCOMPARE(Rlv::canonicalName(alias), target);
  QCOMPARE(Rlv::canonicalName(target), target);
  QVERIFY(Rlv::findBehaviour(alias, Rlv::Mode::Rlv));
  QVERIFY(Rlv::findBehaviour(alias, Rlv::Mode::Rlva));
}

void TestRlvTable::aliasSharesState_data()
{
  QTest::addColumn<QString>("alias");
  QTest::addColumn<QString>("target");
  QTest::addColumn<QString>("option");
  QTest::newRow("camdistmax") << "camdistmax" << "setcam_avdistmax" << "5";
  QTest::newRow("camdistmin") << "camdistmin" << "setcam_avdistmin" << "1.5";
  QTest::newRow("camavdist") << "camavdist" << "setcam_avdist" << "4";
  QTest::newRow("camtextures") << "camtextures" << "setcam_textures" << "c2d4ef12-3a45-4b67-89ab-0123456789ab";
  QTest::newRow("camunlock") << "camunlock" << "setcam_unlock" << "";
  QTest::newRow("fartouch") << "fartouch" << "touchfar" << "3";
}

void TestRlvTable::aliasSharesState()
{
  QFETCH(QString, alias);
  QFETCH(QString, target);
  QFETCH(QString, option);
  const QString suffix = option.isEmpty() ? QString() : ":" + option;
  RlvState state;
  QCOMPARE(one(state, "@" + alias + suffix + "=n").result, Rlv::Result::Success);
  // holding the old name is holding the new name
  QVERIFY(state.has(target, option));
  QVERIFY(state.has(alias, option));
  QCOMPARE(state.count(target, option), 1);
  // the same restriction under the other name is a duplicate and adds nothing
  QCOMPARE(one(state, "@" + target + suffix + "=n").result, Rlv::Result::Duplicate);
  QCOMPARE(state.count(target, option), 1);
  QCOMPARE(state.restrictions().size(), 1);
  // lifting it under the other name removes it
  QCOMPARE(one(state, "@" + target + suffix + "=y").result, Rlv::Result::Success);
  QVERIFY(!state.has(target, option));
  QVERIFY(state.restrictions().isEmpty());
  // and the other way round
  one(state, "@" + target + suffix + "=n");
  QCOMPARE(one(state, "@" + alias + suffix + "=y").result, Rlv::Result::Success);
  QVERIFY(state.restrictions().isEmpty());
  QCOMPARE(one(state, "@" + alias + suffix + "=y").result, Rlv::Result::Unset);
}

void TestRlvTable::aliasAcrossObjects()
{
  RlvState state;
  one(state, "@camdistmax:5=n", "a");
  one(state, "@setcam_avdistmax:5=n", "b");
  QCOMPARE(state.count("setcam_avdistmax", "5"), 2);
  one(state, "@setcam_avdistmax:5=y", "b");
  QCOMPARE(state.count("setcam_avdistmax", "5"), 1);
  one(state, "@clear", "a");
  QCOMPARE(state.count("setcam_avdistmax", "5"), 0);
  QVERIFY(state.restrictions().isEmpty());
}

void TestRlvTable::getstatusShowsTheNameUsed()
{
  RlvState state;
  state.apply("obj", "@camdistmax:5=n,setcam_avdistmin:2=n,fartouch=n");
  QCOMPARE(state.apply("obj", "@getstatus=7").replies.value(0).text, QString("/camdistmax:5/setcam_avdistmin:2/fartouch"));
  // the filter works on the name that was used
  QCOMPARE(state.apply("obj", "@getstatus:camdist=7").replies.value(0).text, QString("/camdistmax:5"));
  QCOMPARE(state.apply("obj", "@getstatus:avdist=7").replies.value(0).text, QString("/setcam_avdistmin:2"));
  QCOMPARE(state.objects().value("obj"), QStringList({"camdistmax:5", "setcam_avdistmin:2", "fartouch"}));
}

void TestRlvTable::aliasBlocksTheSameAction()
{
  RlvState state;
  QCOMPARE(state.actions().value("touchfar"), true);
  one(state, "@fartouch=n");
  QCOMPARE(state.actions().value("touchfar"), false);
  one(state, "@fartouch=y");
  QCOMPARE(state.actions().value("touchfar"), true);
  // with a distance it is a limit, not a block
  one(state, "@fartouch:3=n");
  QCOMPARE(state.actions().value("touchfar"), true);
}

void TestRlvTable::aliasClearAndRemove()
{
  RlvState state;
  state.apply("obj", "@camunlock=n,camtextures=n,camavdist:3=n");
  QCOMPARE(state.restrictions().size(), 3);
  state.apply("obj", "@clear=camtex");
  QCOMPARE(state.restrictions().size(), 2);
  QVERIFY(!state.has("setcam_textures"));
  state.apply("obj", "@clear");
  QVERIFY(state.restrictions().isEmpty());
  QVERIFY(!state.has("setcam_unlock"));
}

void TestRlvTable::zoomLimitsAreRecordedOnly()
{
  RlvState state;
  for (const QString name : {"camzoommax", "camzoommin"})
  {
    RlvState::Outcome outcome = one(state, "@" + name + "=n");
    QCOMPARE(outcome.result, Rlv::Result::Failed);
    QVERIFY(outcome.reason.contains("needs an option"));
    outcome = one(state, "@" + name + ":2.5=n");
    QCOMPARE(outcome.result, Rlv::Result::Success);
    QVERIFY2(outcome.reason.contains("recorded only"), qPrintable(outcome.reason));
    QVERIFY(state.has(name, "2.5"));
    QVERIFY(Rlv::findBehaviour(name)->alias.isEmpty());
  }
  // they are not the field of view limits
  QVERIFY(!state.has("setcam_fovmin"));
  QVERIFY(!state.has("setcam_fovmax"));
}

void TestRlvTable::touchFar()
{
  RlvState state;
  QCOMPARE(one(state, "@touchfar=n").result, Rlv::Result::Success);
  QCOMPARE(state.actions().value("touchfar"), false);
  QCOMPARE(one(state, "@touchfar=force").result, Rlv::Result::Failed);
  QVERIFY(one(state, "@touchfar=force").reason.contains("does not take"));
}

QTEST_GUILESS_MAIN(TestRlvTable)
#include "tst_rlvtable.moc"
