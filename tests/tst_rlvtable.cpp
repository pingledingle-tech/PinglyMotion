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
    void chatImRows_data();
    void chatImRows();
    void chatImBlocks_data();
    void chatImBlocks();
    void secureFormsAreSeparateRestrictions();
    void notifyAddAndLift();
    void notifyFilter();
    void notifyRefusesBadChannels_data();
    void notifyRefusesBadChannels();
    void notifyOnClear();
    void notifyOnlyForItsOwnObject();
    void notifyIsBounded();
    void permissiveIsState();

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

struct ChatRow { const char* name; Rlv::OptionUse option; const char* blocks; };
static const ChatRow chatRows[] = {
  {"chatshout", Rlv::OptionUse::None, "chatshout"}, {"chatnormal", Rlv::OptionUse::None, "chatnormal"},
  {"chatwhisper", Rlv::OptionUse::None, "chatwhisper"}, {"redirchat", Rlv::OptionUse::Required, ""},
  {"rediremote", Rlv::OptionUse::Required, ""}, {"emote", Rlv::OptionUse::None, ""},
  {"recvchat_sec", Rlv::OptionUse::Optional, "recvchat"}, {"recvchatfrom", Rlv::OptionUse::Required, ""},
  {"recvemote", Rlv::OptionUse::Optional, "recvemote"}, {"recvemote_sec", Rlv::OptionUse::Optional, "recvemote"},
  {"recvemotefrom", Rlv::OptionUse::Required, ""}, {"sendchannel_sec", Rlv::OptionUse::Optional, "sendchannel"},
  {"sendim_sec", Rlv::OptionUse::Optional, "sendim"}, {"sendimto", Rlv::OptionUse::Required, ""},
  {"startim", Rlv::OptionUse::Optional, "startim"}, {"startimto", Rlv::OptionUse::Required, ""},
  {"recvim_sec", Rlv::OptionUse::Optional, "recvim"}, {"recvimfrom", Rlv::OptionUse::Required, ""},
  {"share_sec", Rlv::OptionUse::Optional, "share"}, {"tplure_sec", Rlv::OptionUse::Optional, "tplure"},
  {"tprequest_sec", Rlv::OptionUse::Optional, "tprequest"}, {"shownames_sec", Rlv::OptionUse::Optional, "shownames"},
  {"permissive", Rlv::OptionUse::None, ""}, {"notify", Rlv::OptionUse::Required, ""}};

void TestRlvTable::chatImRows_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<int>("option");
  QTest::addColumn<QString>("blocks");
  for (const ChatRow& row : chatRows) QTest::newRow(row.name) << row.name << static_cast<int>(row.option) << row.blocks;
}

void TestRlvTable::chatImRows()
{
  QFETCH(QString, name);
  QFETCH(int, option);
  QFETCH(QString, blocks);
  const Rlv::Behaviour* b = Rlv::findBehaviour(name);
  QVERIFY(b);
  QCOMPARE(b->dialect, Rlv::Dialect::Rlv);
  QCOMPARE(b->params, static_cast<int>(Rlv::AllowAddRemove));
  QCOMPARE(static_cast<int>(b->option), option);
  QCOMPARE(b->blocks.join(","), blocks);
  QVERIFY(!b->description.isEmpty());
  QVERIFY(Rlv::findBehaviour(name, Rlv::Mode::Rlv));   // plain RLV knows it
  RlvState state;
  const QString arg = name == "notify" ? "5" : (name.startsWith("redir") ? "7" : "c2d4ef12-3a45-4b67-89ab-0123456789ab");
  if (option == static_cast<int>(Rlv::OptionUse::None))
  {
    QCOMPARE(one(state, "@" + name + "=n").result, Rlv::Result::Success);
    QCOMPARE(one(state, "@" + name + "=n").result, Rlv::Result::Duplicate);
    QVERIFY(state.has(name));
    QCOMPARE(one(state, "@" + name + ":" + arg + "=n").reason, QString("@%1 takes no option").arg(name));
    QCOMPARE(one(state, "@" + name + "=y").result, Rlv::Result::Success);
    QVERIFY(!state.has(name));
  }
  else
  {
    if (option == static_cast<int>(Rlv::OptionUse::Required))
      QCOMPARE(one(state, "@" + name + "=n").reason, QString("@%1 needs an option").arg(name));
    else
      QCOMPARE(one(state, "@" + name + "=n").result, Rlv::Result::Success);
    state.reset();
    QCOMPARE(one(state, "@" + name + ":" + arg + "=add").result, Rlv::Result::Success);
    QVERIFY(state.has(name, arg));
    QCOMPARE(one(state, "@" + name + ":" + arg + "=rem").result, Rlv::Result::Success);
    QVERIFY(!state.has(name, arg));
  }
  QVERIFY(one(state, "@" + name + "=force").reason.contains("does not take"));
}

void TestRlvTable::chatImBlocks_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<QString>("action");
  QTest::newRow("chatshout") << "chatshout" << "chatshout";
  QTest::newRow("chatnormal") << "chatnormal" << "chatnormal";
  QTest::newRow("chatwhisper") << "chatwhisper" << "chatwhisper";
  QTest::newRow("recvchat_sec") << "recvchat_sec" << "recvchat";
  QTest::newRow("recvemote") << "recvemote" << "recvemote";
  QTest::newRow("recvemote_sec") << "recvemote_sec" << "recvemote";
  QTest::newRow("sendchannel_sec") << "sendchannel_sec" << "sendchannel";
  QTest::newRow("sendim_sec") << "sendim_sec" << "sendim";
  QTest::newRow("startim") << "startim" << "startim";
  QTest::newRow("recvim_sec") << "recvim_sec" << "recvim";
  QTest::newRow("share_sec") << "share_sec" << "share";
  QTest::newRow("tplure_sec") << "tplure_sec" << "tplure";
  QTest::newRow("tprequest_sec") << "tprequest_sec" << "tprequest";
  QTest::newRow("shownames_sec") << "shownames_sec" << "shownames";
}

void TestRlvTable::chatImBlocks()
{
  QFETCH(QString, name);
  QFETCH(QString, action);
  RlvState state;
  QVERIFY(Rlv::actions().size() > 0);
  QCOMPARE(state.actions().value(action), true);
  one(state, "@" + name + "=n");
  const QMap<QString, bool> blocked = state.actions();
  QCOMPARE(blocked.value(action), false);
  int falseCount = 0;
  for (bool allowed : blocked) falseCount += allowed ? 0 : 1;
  QCOMPARE(falseCount, 1);   // exactly this one action
  // with an option it is an exception, not a block
  state.reset();
  one(state, "@" + name + ":c2d4ef12-3a45-4b67-89ab-0123456789ab=add");
  QCOMPARE(state.actions().value(action), true);
}

void TestRlvTable::secureFormsAreSeparateRestrictions()
{
  RlvState state;
  one(state, "@sendim=n");
  QCOMPARE(one(state, "@sendim_sec=n").result, Rlv::Result::Success);   // another restriction, not a duplicate
  QCOMPARE(state.apply("obj", "@getstatus=7").replies.value(0).text, QString("/sendim/sendim_sec"));
  one(state, "@sendim=y");
  QCOMPARE(state.actions().value("sendim"), false);   // the secure form still blocks
  one(state, "@sendim_sec=y");
  QCOMPARE(state.actions().value("sendim"), true);
}

void TestRlvTable::notifyAddAndLift()
{
  RlvState state;
  RlvState::ApplyResult r = state.apply("obj", "@notify:2222=add");
  QCOMPARE(r.outcomes.value(0).result, Rlv::Result::Success);
  QVERIFY(r.replies.isEmpty());   // setting up a notification says nothing
  r = state.apply("obj", "@sendim=n,fly=n");
  QCOMPARE(r.replies.size(), 2);
  QCOMPARE(r.replies[0].channel, 2222);
  QCOMPARE(r.replies[0].text, QString("/sendim=n"));
  QCOMPARE(r.replies[1].text, QString("/fly=n"));
  // a duplicate changes nothing and says nothing
  QVERIFY(state.apply("obj", "@fly=n").replies.isEmpty());
  r = state.apply("obj", "@fly=y");
  QCOMPARE(r.replies.size(), 1);
  QCOMPARE(r.replies[0].text, QString("/fly=y"));
  // a restriction that was not held says nothing
  QVERIFY(state.apply("obj", "@fly=y").replies.isEmpty());
  // an option is part of the rule, the name used is the one shown
  r = state.apply("obj", "@camdistmax:5=n");
  QCOMPARE(r.replies.value(0).text, QString("/camdistmax:5=n"));
  // removing the notification stops the messages
  QCOMPARE(state.apply("obj", "@notify:2222=rem").outcomes.value(0).result, Rlv::Result::Success);
  QVERIFY(state.apply("obj", "@sit=n").replies.isEmpty());
  // two notifications on two channels both answer
  state.apply("obj", "@notify:11=add,notify:12=add");
  r = state.apply("obj", "@rez=n");
  QCOMPARE(r.replies.size(), 2);
  QCOMPARE(r.replies[0].channel, 11);
  QCOMPARE(r.replies[1].channel, 12);
}

void TestRlvTable::notifyFilter()
{
  RlvState state;
  state.apply("obj", "@notify:77;send=add");
  RlvState::ApplyResult r = state.apply("obj", "@sendim=n,recvim=n,sendchat=n,fly=n");
  QStringList texts;
  for (const RlvState::Reply& reply : r.replies) texts << QString::number(reply.channel) + " " + reply.text;
  QCOMPARE(texts, QStringList({"77 /sendim=n", "77 /sendchat=n"}));
  // an empty filter after the semicolon means every rule
  state.apply("obj", "@clear");
  state.apply("obj", "@notify:78;=add");
  QCOMPARE(state.apply("obj", "@fly=n").replies.value(0).text, QString("/fly=n"));
}

void TestRlvTable::notifyRefusesBadChannels_data()
{
  QTest::addColumn<QString>("line");
  QTest::newRow("channel 0") << "@notify:0=add";
  QTest::newRow("channel 0 filter") << "@notify:0;fly=add";
  QTest::newRow("negative") << "@notify:-5=add";
  QTest::newRow("text") << "@notify:abc=add";
  QTest::newRow("empty channel") << "@notify:;fly=add";
  QTest::newRow("too big") << "@notify:2147483648=add";
  QTest::newRow("decimal") << "@notify:1.5=add";
  QTest::newRow("no option") << "@notify=add";
}

void TestRlvTable::notifyRefusesBadChannels()
{
  QFETCH(QString, line);
  RlvState state;
  const RlvState::Outcome outcome = one(state, line);
  QCOMPARE(outcome.result, Rlv::Result::Failed);
  QVERIFY2(!outcome.reason.isEmpty(), qPrintable(line));
  QVERIFY(state.restrictions().isEmpty());
  QCOMPARE(one(state, "@notify:2147483647=add").result, Rlv::Result::Success);   // the highest channel is fine
}

void TestRlvTable::notifyOnClear()
{
  RlvState state;
  state.apply("obj", "@fly=n,sendim=n,notify:5=add");
  RlvState::ApplyResult r = state.apply("obj", "@clear=send");
  QCOMPARE(r.replies.size(), 1);
  QCOMPARE(r.replies[0].text, QString("/sendim=y"));
  // a full clear removes the notification too, so nobody hears about the rest
  r = state.apply("obj", "@clear");
  QVERIFY(r.replies.isEmpty());
  QVERIFY(state.restrictions().isEmpty());
}

void TestRlvTable::notifyOnlyForItsOwnObject()
{
  RlvState state;
  state.apply("a", "@notify:5=add");
  // another object's change is not announced to a, and a's change is not announced to b
  QVERIFY(state.apply("b", "@fly=n").replies.isEmpty());
  QCOMPARE(state.apply("a", "@sit=n").replies.size(), 1);
  QVERIFY(state.apply("b", "@rez=n").replies.isEmpty());
}

void TestRlvTable::notifyIsBounded()
{
  RlvState state;
  for (int i = 1; i <= 100; i++) state.apply("obj", QString("@notify:%1=add").arg(i));
  QCOMPARE(state.restrictions().size(), 100);
  // one change tells every notification, but never more than the limit of answers per line
  const RlvState::ApplyResult r = state.apply("obj", "@fly=n,sit=n,rez=n,sendim=n");
  QCOMPARE(r.replies.size(), 256);
  QCOMPARE(r.replies[0].channel, 1);
  QCOMPARE(r.replies[99].channel, 100);
  QCOMPARE(r.replies[100].channel, 1);
}

void TestRlvTable::permissiveIsState()
{
  RlvState state;
  QCOMPARE(one(state, "@permissive=n").result, Rlv::Result::Success);
  QVERIFY(state.has("permissive"));
  for (bool allowed : state.actions()) QVERIFY(allowed);   // blocks nothing
  QCOMPARE(state.apply("obj", "@getstatus=3").replies.value(0).text, QString("/permissive"));
}

QTEST_GUILESS_MAIN(TestRlvTable)
#include "tst_rlvtable.moc"
