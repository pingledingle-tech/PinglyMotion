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
    void everyActionOnce();
    void rowsOfThisPart_data();
    void rowsOfThisPart();
    void rlvOnlyMark();
    void forceCommandsAreRetained_data();
    void forceCommandsAreRetained();
    void exceptionsAreHeldPerFolder();
    void touchRowsBlockTheirOwnAction();
    void fixedAnswers_data();
    void fixedAnswers();
    void outfitAndAttachAnswers();
    void folderQuestionsAnswerEmpty_data();
    void folderQuestionsAnswerEmpty();
    void debugAndEnvFamilies();
    void familiesAreBounded();
    void movementRestrictions();
    void setrotIsRlvOnly();
    void recordedRows_data();
    void recordedRows();
    void sphereAndOverlayForce();
    void versionWithBlacklist();
    void blacklistAndZoomAnswers();
    void rlvOnlyNamesInRlvaMode_data();
    void rlvOnlyNamesInRlvaMode();
    void tableHasNoDuplicatesAndIsMarked();

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
  // the RLV wiki has the old name camavdist but not setcam_avdist, so only that pair differs in its mark
  if (alias != QLatin1String("camavdist")) QCOMPARE(a->dialect, t->dialect);
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
  const QString arg = name == "notify" ? "5" : (name.startsWith("redir") ? "7" : (name.startsWith("sendchannel") ? "9" : "c2d4ef12-3a45-4b67-89ab-0123456789ab"));
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
  const QString exception = name.startsWith("sendchannel") ? "9" : "c2d4ef12-3a45-4b67-89ab-0123456789ab";
  const RlvState::Outcome added = one(state, "@" + name + ":" + exception + "=add");
  if (Rlv::findBehaviour(name)->option != Rlv::OptionUse::None) QCOMPARE(added.result, Rlv::Result::Success);
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

// every behaviour added with the outfit, touch and view families: name, parameter types, option use, actions it blocks, mark
struct Spec { const char* name; int params; Rlv::OptionUse option; const char* blocks; Rlv::Dialect dialect; };
static const int AR = Rlv::AllowAddRemove;
static const int FO = Rlv::AllowForce;
static const Rlv::OptionUse NO = Rlv::OptionUse::None;
static const Rlv::OptionUse OP = Rlv::OptionUse::Optional;
static const Rlv::OptionUse RQ = Rlv::OptionUse::Required;
static const Rlv::Dialect PLAIN = Rlv::Dialect::Rlv;
static const Rlv::Dialect RLVAONLY = Rlv::Dialect::RlvaOnly;
static const Spec specs[] = {
  {"addattach", AR, OP, "addattach", PLAIN}, {"remattach", AR | FO, OP, "remattach", PLAIN},
  {"addoutfit", AR | FO, OP, "addoutfit", PLAIN}, {"remoutfit", AR | FO, OP, "remoutfit", PLAIN},
  {"detachme", FO, NO, "", PLAIN}, {"defaultwear", AR, NO, "", Rlv::Dialect::RlvOnly},
  {"unsharedwear", AR, NO, "unsharedwear", PLAIN}, {"unsharedunwear", AR, NO, "unsharedunwear", PLAIN},
  {"attach", FO, RQ, "", PLAIN}, {"attachover", FO, RQ, "", PLAIN}, {"attachoverorreplace", FO, RQ, "", PLAIN},
  {"attachall", FO, RQ, "", PLAIN}, {"attachallover", FO, RQ, "", PLAIN}, {"attachalloverorreplace", FO, RQ, "", PLAIN},
  {"detachall", FO, RQ, "", PLAIN},
  {"attachthis", AR | FO, OP, "attachthis", PLAIN}, {"attachthisover", FO, OP, "", PLAIN},
  {"attachthisoverorreplace", FO, OP, "", PLAIN}, {"attachallthis", AR | FO, OP, "attachallthis", PLAIN},
  {"attachallthisover", FO, OP, "", PLAIN}, {"attachallthisoverorreplace", FO, OP, "", PLAIN},
  {"detachthis", AR | FO, OP, "detachthis", PLAIN}, {"detachallthis", AR | FO, OP, "detachallthis", PLAIN},
  {"attachthis_except", AR, RQ, "", PLAIN}, {"attachallthis_except", AR, RQ, "", PLAIN},
  {"detachthis_except", AR, RQ, "", PLAIN}, {"detachallthis_except", AR, RQ, "", PLAIN},
  {"touchworld", AR, OP, "touchworld", PLAIN}, {"touchthis", AR, RQ, "", PLAIN}, {"touchme", AR, NO, "", PLAIN},
  {"touchattach", AR, NO, "touchattach", PLAIN}, {"touchattachself", AR, NO, "touchattachself", PLAIN},
  {"touchattachother", AR, OP, "touchattachother", PLAIN},
  {"showhovertextall", AR, NO, "showhovertextall", PLAIN}, {"showhovertext", AR, RQ, "", PLAIN},
  {"showhovertexthud", AR, NO, "showhovertexthud", PLAIN}, {"showhovertextworld", AR, NO, "showhovertextworld", PLAIN},
  {"shownames", AR, OP, "shownames", PLAIN}, {"shownametags", AR, OP, "shownametags", PLAIN},
  {"viewnote", AR, OP, "viewnote", PLAIN}, {"viewscript", AR, OP, "viewscript", PLAIN},
  {"viewtexture", AR, OP, "viewtexture", PLAIN},
  {"editobj", AR, RQ, "", PLAIN}, {"editworld", AR, NO, "editworld", PLAIN}, {"editattach", AR, NO, "editattach", PLAIN},
  {"buy", AR, NO, "buy", RLVAONLY}, {"pay", AR, OP, "pay", RLVAONLY},
  {"acceptpermission", AR, NO, "", PLAIN}, {"denypermission", AR, NO, "", Rlv::Dialect::RlvOnly},
  {"setgroup", AR | FO, OP, "setgroup", PLAIN}, {"temprun", AR, NO, "temprun", PLAIN}, {"alwaysrun", AR, NO, "alwaysrun", PLAIN},
  {"jump", AR, NO, "jump", RLVAONLY}, {"setrot", FO, RQ, "", Rlv::Dialect::RlvOnly}, {"adjustheight", FO, RQ, "", PLAIN},
  {"tpto", FO, RQ, "", PLAIN}, {"standtp", AR, NO, "", PLAIN}, {"accepttp", AR, OP, "", PLAIN}, {"sitground", FO, NO, "", PLAIN}};

void TestRlvTable::everyActionOnce()
{
  QSet<QString> seen;
  for (const Rlv::Action& action : Rlv::actions())
  {
    QVERIFY2(!seen.contains(action.id), qPrintable(action.id));
    QVERIFY(!action.label.isEmpty());
    seen.insert(action.id);
  }
  // every action a behaviour blocks exists
  for (const Rlv::Behaviour& behaviour : Rlv::behaviours())
    for (const QString& id : behaviour.blocks) QVERIFY2(seen.contains(id), qPrintable(behaviour.name + " blocks " + id));
}

void TestRlvTable::rowsOfThisPart_data()
{
  QTest::addColumn<int>("index");
  for (size_t i = 0; i < sizeof(specs) / sizeof(specs[0]); i++) QTest::newRow(specs[i].name) << static_cast<int>(i);
}

static QString argumentFor(const QString& name)
{
  // option values RLVa accepts (docs/RLV_OPTION_TYPES.md): a point, a layer, else a folder or a key
  if (name == "addattach" || name == "remattach") return "spine";
  if (name == "addoutfit" || name == "remoutfit") return "shirt";
  if (name.startsWith("attach") || name.startsWith("detach")) return "clothes/hat";
  return "c2d4ef12-3a45-4b67-89ab-0123456789ab";
}

void TestRlvTable::rowsOfThisPart()
{
  QFETCH(int, index);
  const Spec& spec = specs[index];
  const QString name = spec.name;
  const Rlv::Behaviour* b = Rlv::findBehaviour(name);
  QVERIFY(b);
  QCOMPARE(b->params, spec.params);
  QCOMPARE(static_cast<int>(b->option), static_cast<int>(spec.option));
  QCOMPARE(b->blocks.join(","), QString(spec.blocks));
  QCOMPARE(b->dialect, spec.dialect);
  QVERIFY(!b->description.isEmpty());
  QVERIFY(!b->external);

  // the mark decides in which mode it is known
  const Rlv::Mode home = spec.dialect == Rlv::Dialect::RlvOnly ? Rlv::Mode::Rlv : Rlv::Mode::Rlva;
  QCOMPARE(Rlv::findBehaviour(name, Rlv::Mode::Rlv) != nullptr, spec.dialect != Rlv::Dialect::RlvaOnly);
  QCOMPARE(Rlv::findBehaviour(name, Rlv::Mode::Rlva) != nullptr, spec.dialect != Rlv::Dialect::RlvOnly);

  RlvState state;
  state.setMode(home);
  const QString arg = argumentFor(name);
  const QString withOption = ":" + arg;
  // RLVa takes no option for these (docs/RLV_OPTION_TYPES.md); the rest checks the plain RLV form
  if (b->value.restriction == Rlv::ValueType::None && (spec.params & Rlv::AllowAddRemove) && home == Rlv::Mode::Rlva)
  {
    QCOMPARE(one(state, "@" + name + withOption + "=n").reason,
             QString("@%1%2=n: RLVa takes no option here. Remove \"%2\".").arg(name, withOption));
    QVERIFY(state.restrictions().isEmpty());
    if (spec.dialect == Rlv::Dialect::RlvaOnly)
    {
      // only RLVa knows it, so without the option it is a plain restriction
      QCOMPARE(one(state, "@" + name + "=n").result, Rlv::Result::Success);
      QCOMPARE(state.actions().value(b->blocks.value(0)), false);
      QCOMPARE(one(state, "@" + name + "=y").result, Rlv::Result::Success);
      QVERIFY(state.restrictions().isEmpty());
      return;
    }
    state.setMode(Rlv::Mode::Rlv);
  }
  if (spec.params & Rlv::AllowAddRemove)
  {
    const QString opt = spec.option == NO ? QString() : withOption;
    if (spec.option == RQ) QCOMPARE(one(state, "@" + name + "=n").reason, QString("@%1 needs an option").arg(name));
    if (spec.option == NO) QCOMPARE(one(state, "@" + name + withOption + "=n").reason, QString("@%1 takes no option").arg(name));
    QCOMPARE(one(state, "@" + name + opt + "=n").result, Rlv::Result::Success);
    QCOMPARE(one(state, "@" + name + opt + "=n").result, Rlv::Result::Duplicate);
    QVERIFY(state.has(name, spec.option == NO ? QString() : arg));
    // with an option it never blocks; without one it blocks exactly its own actions
    int blockedNow = 0;
    for (bool allowed : state.actions()) blockedNow += allowed ? 0 : 1;
    QCOMPARE(blockedNow, (spec.option == NO && !b->blocks.isEmpty()) ? static_cast<int>(b->blocks.size()) : 0);
    QCOMPARE(one(state, "@" + name + opt + "=y").result, Rlv::Result::Success);
    QVERIFY(state.restrictions().isEmpty());
    if (spec.option == OP)
    {
      one(state, "@" + name + "=n");
      if (!b->blocks.isEmpty()) QCOMPARE(state.actions().value(b->blocks.value(0)), false);
      QVERIFY(!state.has(name, arg));
    }
  }
  else
  {
    QVERIFY(one(state, "@" + name + (spec.option == NO ? QString() : withOption) + "=n").reason.contains("does not take"));
  }
  if (!(spec.params & FO))
    QVERIFY(one(state, "@" + name + withOption + "=force").reason.contains("does not take"));
}

void TestRlvTable::rlvOnlyMark()
{
  QCOMPARE(Rlv::findBehaviour("defaultwear")->dialect, Rlv::Dialect::RlvOnly);
  RlvState state;
  state.setMode(Rlv::Mode::Rlv);
  QCOMPARE(one(state, "@defaultwear=n").result, Rlv::Result::Success);
  QCOMPARE(one(state, "@denypermission=n").result, Rlv::Result::Success);
  QVERIFY(state.has("defaultwear"));
  QVERIFY(Rlv::behaviourNames(Rlv::Mode::Rlv).contains("defaultwear"));
  QVERIFY(!Rlv::behaviourNames(Rlv::Mode::Rlva).contains("defaultwear"));
  QVERIFY(!Rlv::behaviourNames(Rlv::Mode::Rlva).contains("denypermission"));
  QVERIFY(Rlv::behaviourNames(Rlv::Mode::Rlva).contains("interact"));
  QVERIFY(!Rlv::behaviourNames(Rlv::Mode::Rlv).contains("showself"));
  // in RLVa mode it is unknown, and switching the mode clears what was held
  state.setMode(Rlv::Mode::Rlva);
  QVERIFY(state.restrictions().isEmpty());
  const RlvState::Outcome outcome = one(state, "@defaultwear=n");
  QCOMPARE(outcome.result, Rlv::Result::Failed);
  QCOMPARE(outcome.reason, QString("unknown behaviour \"defaultwear\""));
  QCOMPARE(Rlv::checkCommand(Rlv::parseLine("@denypermission=n").commands.value(0), Rlv::Mode::Rlva), QString("unknown behaviour \"denypermission\""));
  QVERIFY(Rlv::checkCommand(Rlv::parseLine("@denypermission=n").commands.value(0), Rlv::Mode::Rlv).isEmpty());
  // getcommand follows the mode
  QCOMPARE(state.apply("obj", "@getcommand:permission=9").replies.value(0).text, QString("/acceptpermission"));
}

void TestRlvTable::forceCommandsAreRetained_data()
{
  QTest::addColumn<QString>("line");
  QTest::newRow("detachme") << "@detachme=force";
  QTest::newRow("remattach") << "@remattach:chest=force";
  QTest::newRow("remoutfit") << "@remoutfit:shirt=force";
  QTest::newRow("attach") << "@attach:Clothes/Hat=force";
  QTest::newRow("attachover") << "@attachover:Clothes/Hat=force";
  QTest::newRow("attachoverorreplace") << "@attachoverorreplace:Clothes/Hat=force";
  QTest::newRow("attachall") << "@attachall:Clothes=force";
  QTest::newRow("attachallover") << "@attachallover:Clothes=force";
  QTest::newRow("attachalloverorreplace") << "@attachalloverorreplace:Clothes=force";
  QTest::newRow("attachthis") << "@attachthis=force";
  QTest::newRow("attachthis folder") << "@attachthis:Clothes/Hat=force";
  QTest::newRow("attachthisover") << "@attachthisover=force";
  QTest::newRow("attachthisoverorreplace") << "@attachthisoverorreplace=force";
  QTest::newRow("attachallthis") << "@attachallthis=force";
  QTest::newRow("attachallthisover") << "@attachallthisover=force";
  QTest::newRow("attachallthisoverorreplace") << "@attachallthisoverorreplace=force";
  QTest::newRow("detachall") << "@detachall:Clothes=force";
  QTest::newRow("detachthis") << "@detachthis=force";
  QTest::newRow("detachallthis") << "@detachallthis=force";
  QTest::newRow("setgroup") << "@setgroup:none=force";
  QTest::newRow("setrot") << "@setrot:1.57=force";
  QTest::newRow("adjustheight") << "@adjustheight:0.1;1=force";
  QTest::newRow("tpto") << "@tpto:128/128/30=force";
  QTest::newRow("sitground") << "@sitground=force";
  // RLVa only: fly=force and the addoutfit names of the shared folder force commands
  QTest::newRow("fly") << "@fly=force";
  QTest::newRow("addoutfit") << "@addoutfit:Clothes/Hat=force";
  QTest::newRow("addoutfitall") << "@addoutfitall:Clothes=force";
  QTest::newRow("addoutfitover") << "@addoutfitover:Clothes/Hat=force";
  QTest::newRow("addoutfitallover") << "@addoutfitallover:Clothes=force";
  QTest::newRow("addoutfitthis") << "@addoutfitthis=force";
  QTest::newRow("addoutfitallthis") << "@addoutfitallthis=force";
  QTest::newRow("addoutfitthisover") << "@addoutfitthisover=force";
  QTest::newRow("addoutfitallthisover") << "@addoutfitallthisover=force";
}

void TestRlvTable::forceCommandsAreRetained()
{
  QFETCH(QString, line);
  RlvState state;
  // in the mode that knows it: RLVa, or plain RLV for an RLV-only name
  const Rlv::Behaviour* b = Rlv::findBehaviour(Rlv::parseLine(line).commands.value(0).behaviour);
  QVERIFY(b);
  if (b->dialect == Rlv::Dialect::RlvOnly) state.setMode(Rlv::Mode::Rlv);
  const RlvState::Outcome outcome = one(state, line);
  QCOMPARE(outcome.result, Rlv::Result::Retained);
  QCOMPARE(outcome.reason, QString("accepted; force commands are not carried out in the sandbox"));
  QVERIFY(state.restrictions().isEmpty());   // nothing is held, nothing was done
  QVERIFY(state.recorded().isEmpty());
}

void TestRlvTable::exceptionsAreHeldPerFolder()
{
  RlvState state;
  one(state, "@attachthis=n");
  QCOMPARE(state.actions().value("attachthis"), false);
  QCOMPARE(one(state, "@attachthis_except:Clothes/Hat=add").result, Rlv::Result::Success);
  QCOMPARE(one(state, "@attachthis_except:Clothes/Hat=add").result, Rlv::Result::Duplicate);
  QCOMPARE(one(state, "@attachthis_except:Shoes=add").result, Rlv::Result::Success);
  QCOMPARE(state.apply("obj", "@getstatus:except=7").replies.value(0).text, QString("/attachthis_except:clothes/hat/attachthis_except:shoes"));
  QCOMPARE(one(state, "@attachthis_except:clothes/hat=rem").result, Rlv::Result::Success);
  QCOMPARE(state.restrictions().size(), 2);
}

void TestRlvTable::touchRowsBlockTheirOwnAction()
{
  RlvState state;
  one(state, "@touchall=n");
  QCOMPARE(state.actions().value("touch"), false);
  QCOMPARE(state.actions().value("touchworld"), true);   // separate actions
  one(state, "@touchworld=n,touchattachself=n,touchme=add,touchthis:c2d4ef12-3a45-4b67-89ab-0123456789ab=add");
  QCOMPARE(state.actions().value("touchworld"), false);
  QCOMPARE(state.actions().value("touchattachself"), false);
  QCOMPARE(state.actions().value("touchattach"), true);
  QCOMPARE(state.actions().value("touchattachother"), true);
  QCOMPARE(state.restrictions().size(), 5);
}

void TestRlvTable::fixedAnswers_data()
{
  QTest::addColumn<QString>("line");
  QTest::addColumn<QString>("text");
  QTest::newRow("getsitid") << "@getsitid=5" << "00000000-0000-0000-0000-000000000000";
  QTest::newRow("getgroup") << "@getgroup=5" << "none";
}

void TestRlvTable::fixedAnswers()
{
  QFETCH(QString, line);
  QFETCH(QString, text);
  RlvState state;
  const RlvState::ApplyResult r = state.apply("obj", line);
  QCOMPARE(r.outcomes.value(0).result, Rlv::Result::Success);
  QCOMPARE(r.replies.size(), 1);
  QCOMPARE(r.replies[0].channel, 5);
  QCOMPARE(r.replies[0].text, text);
  // a question takes no n or force
  QVERIFY(state.apply("obj", line.left(line.indexOf('=')) + "=n").outcomes.value(0).reason.contains("does not take"));
}

void TestRlvTable::outfitAndAttachAnswers()
{
  RlvState state;
  QCOMPARE(Rlv::clothingLayers().size(), 16);
  QCOMPARE(Rlv::attachmentPoints().size(), 41);
  QCOMPARE(state.apply("obj", "@getoutfit=9").replies.value(0).text, QString(16, '0'));
  QCOMPARE(state.apply("obj", "@getattach=9").replies.value(0).text, QString(41, '0'));
  // one name: one digit; a name that is none of them: empty
  QCOMPARE(state.apply("obj", "@getoutfit:shirt=9").replies.value(0).text, QString("0"));
  QCOMPARE(state.apply("obj", "@getoutfit:physics=9").replies.value(0).text, QString("0"));
  QCOMPARE(state.apply("obj", "@getoutfit:hat=9").replies.value(0).text, QString());
  QCOMPARE(state.apply("obj", "@getattach:left hand=9").replies.value(0).text, QString("0"));
  QCOMPARE(state.apply("obj", "@getattach:root=9").replies.value(0).text, QString("0"));
  QCOMPARE(state.apply("obj", "@getattach:tail=9").replies.value(0).text, QString());
  QCOMPARE(state.apply("obj", "@getattach:tail=9").replies.size(), 1);   // still answers, with an empty line
  QCOMPARE(Rlv::attachmentPoints().first(), QString("none"));
  QCOMPARE(Rlv::attachmentPoints().last(), QString("root"));
  QCOMPARE(Rlv::clothingLayers().first(), QString("gloves"));
  QCOMPARE(Rlv::clothingLayers().last(), QString("physics"));
}

void TestRlvTable::folderQuestionsAnswerEmpty_data()
{
  QTest::addColumn<QString>("name");
  for (const QString name : {"getinv", "getinvworn", "findfolder", "getpath", "getpathnew"}) QTest::newRow(qPrintable(name)) << name;
}

void TestRlvTable::folderQuestionsAnswerEmpty()
{
  QFETCH(QString, name);
  RlvState state;
  for (const QString& line : {"@" + name + "=8", "@" + name + ":clothes=8"})
  {
    const RlvState::ApplyResult r = state.apply("obj", line);
    QCOMPARE(r.outcomes.value(0).result, Rlv::Result::Success);
    QCOMPARE(r.replies.size(), 1);
    QCOMPARE(r.replies[0].channel, 8);
    QCOMPARE(r.replies[0].text, QString());
  }
  QVERIFY(Rlv::findBehaviour(name)->description.contains("no inventory"));
}

void TestRlvTable::debugAndEnvFamilies()
{
  RlvState state;
  // a name after the prefix is needed; the prefix alone is no behaviour
  QVERIFY(Rlv::findBehaviour("setdebug_renderfarclip"));
  QVERIFY(Rlv::findBehaviour("getenv_sunmoonposition"));
  QVERIFY(!Rlv::findBehaviour("setdebug_"));
  QVERIFY(!Rlv::findBehaviour("setenvx"));
  QCOMPARE(Rlv::findBehaviour("setenv_daytime")->group, Rlv::Group::Environment);
  QCOMPARE(state.apply("obj", "@setdebug_=force").outcomes.value(0).reason, QString("unknown behaviour \"setdebug_\""));
  // nothing set yet: the answer is empty
  QCOMPARE(state.apply("obj", "@getdebug_renderfarclip=3").replies.value(0).text, QString());
  RlvState::Outcome outcome = one(state, "@setdebug_renderfarclip:64=force");
  QCOMPARE(outcome.result, Rlv::Result::Retained);
  QVERIFY2(outcome.reason.contains("recorded only"), qPrintable(outcome.reason));
  QCOMPARE(state.recorded().value("setdebug_renderfarclip"), QString("64"));
  QCOMPARE(state.apply("obj", "@getdebug_renderfarclip=3").replies.value(0).text, QString("64"));
  one(state, "@setenv_daytime:0.5=force");
  QCOMPARE(state.recorded().value("setenv_daytime"), QString("0.5"));
  QCOMPARE(state.apply("obj", "@getenv_daytime=3").replies.value(0).text, QString("0.5"));
  QCOMPARE(state.apply("obj", "@getenv_other=3").replies.value(0).text, QString());
  // the last value wins; debug and sky names do not mix
  one(state, "@setdebug_renderfarclip:128=force");
  QCOMPARE(state.apply("obj", "@getdebug_renderfarclip=3").replies.value(0).text, QString("128"));
  QCOMPARE(state.apply("obj", "@getenv_renderfarclip=3").replies.value(0).text, QString());
  QCOMPARE(state.recorded().size(), 2);
  // parameter types and options
  QVERIFY(one(state, "@setdebug_renderfarclip=force").reason.contains("needs an option"));
  QVERIFY(one(state, "@setdebug_renderfarclip:64=n").reason.contains("does not take"));
  QVERIFY(one(state, "@getdebug_renderfarclip:1=3").reason.contains("takes no option"));
  QVERIFY(state.apply("obj", "@getdebug_renderfarclip=n").outcomes.value(0).reason.contains("does not take"));
  // a reset forgets them
  state.reset();
  QVERIFY(state.recorded().isEmpty());
  QCOMPARE(state.apply("obj", "@getdebug_renderfarclip=3").replies.value(0).text, QString());
}

void TestRlvTable::familiesAreBounded()
{
  RlvState state;
  for (int i = 0; i < RlvState::MAX_RECORDED_ENTRIES + 20; i++)
    state.apply("obj", QString("@setenv_name%1:1=force").arg(i));
  QCOMPARE(state.recorded().size(), RlvState::MAX_RECORDED_ENTRIES);
  const RlvState::Outcome refused = one(state, "@setenv_onemore:1=force");
  QCOMPARE(refused.result, Rlv::Result::Failed);
  QCOMPARE(refused.reason, QString("256 outside effects are already recorded (the most allowed)"));
  // a name that is already recorded can still change
  QCOMPARE(one(state, "@setenv_name0:2=force").result, Rlv::Result::Retained);
  QCOMPARE(state.recorded().value("setenv_name0"), QString("2"));
  QCOMPARE(state.recorded().size(), RlvState::MAX_RECORDED_ENTRIES);
}

void TestRlvTable::movementRestrictions()
{
  RlvState state;
  one(state, "@temprun=n,alwaysrun=n,jump=n,setgroup=n");
  const QMap<QString, bool> a = state.actions();
  QCOMPARE(a.value("temprun"), false);
  QCOMPARE(a.value("alwaysrun"), false);
  QCOMPARE(a.value("jump"), false);
  QCOMPARE(a.value("setgroup"), false);
  QCOMPARE(a.value("fly"), true);
  QCOMPARE(state.apply("obj", "@getstatus=2").replies.value(0).text, QString("/temprun/alwaysrun/jump/setgroup"));
  // the group can still be named in an exception form, and force is accepted but not carried out
  QCOMPARE(one(state, "@setgroup:none=force").result, Rlv::Result::Retained);
}

void TestRlvTable::setrotIsRlvOnly()
{
  QCOMPARE(Rlv::findBehaviour("setrot")->dialect, Rlv::Dialect::RlvOnly);
  RlvState state;
  state.setMode(Rlv::Mode::Rlv);
  QCOMPARE(one(state, "@setrot:3.14=force").result, Rlv::Result::Retained);
  state.setMode(Rlv::Mode::Rlva);
  QCOMPARE(one(state, "@setrot:3.14=force").reason, QString("unknown behaviour \"setrot\""));
}

// the camera, sphere and overlay behaviours that are only recorded: name, parameter types, option use, mark
struct RecSpec { const char* name; int params; Rlv::OptionUse option; Rlv::Dialect dialect; };
static const Rlv::Dialect RLVONLY = Rlv::Dialect::RlvOnly;
static const RecSpec recSpecs[] = {
  {"camdrawmin", AR, RQ, RLVONLY}, {"camdrawmax", AR, RQ, RLVONLY}, {"camdrawalphamin", AR, RQ, RLVONLY},
  {"camdrawalphamax", AR, RQ, RLVONLY}, {"camdrawcolor", AR, RQ, RLVONLY},
  // RLVa only; RLVa sets the sphere and overlay values with =force only
  {"setsphere", AR, NO, RLVAONLY}, {"setsphere_mode", FO, OP, RLVAONLY}, {"setsphere_origin", FO, OP, RLVAONLY},
  {"setsphere_color", FO, OP, RLVAONLY}, {"setsphere_distmin", FO, OP, RLVAONLY},
  {"setsphere_distmax", FO, OP, RLVAONLY}, {"setsphere_distextend", FO, OP, RLVAONLY},
  {"setsphere_param", FO, OP, RLVAONLY}, {"setsphere_tween", FO, OP, RLVAONLY},
  {"setsphere_valuemin", FO, OP, RLVAONLY}, {"setsphere_valuemax", FO, OP, RLVAONLY},
  {"setoverlay", AR, NO, RLVAONLY}, {"setoverlay_alpha", FO, OP, RLVAONLY}, {"setoverlay_texture", FO, OP, RLVAONLY},
  {"setoverlay_tint", FO, OP, RLVAONLY}, {"setoverlay_tween", FO, OP, RLVAONLY}, {"setoverlay_touch", AR, NO, RLVAONLY}};

void TestRlvTable::recordedRows_data()
{
  QTest::addColumn<int>("index");
  for (size_t i = 0; i < sizeof(recSpecs) / sizeof(recSpecs[0]); i++) QTest::newRow(recSpecs[i].name) << static_cast<int>(i);
}

void TestRlvTable::recordedRows()
{
  QFETCH(int, index);
  const RecSpec& spec = recSpecs[index];
  const QString name = spec.name;
  const Rlv::Behaviour* b = Rlv::findBehaviour(name);
  QVERIFY(b);
  QCOMPARE(b->params, spec.params);
  QCOMPARE(static_cast<int>(b->option), static_cast<int>(spec.option));
  QCOMPARE(b->dialect, spec.dialect);
  QCOMPARE(b->group, Rlv::Group::Camera);
  QVERIFY(b->external);
  QVERIFY(b->blocks.isEmpty());
  QVERIFY(b->note.startsWith("recorded only"));
  QCOMPARE(Rlv::findBehaviour(name, Rlv::Mode::Rlv) != nullptr, spec.dialect != RLVAONLY);
  QCOMPARE(Rlv::findBehaviour(name, Rlv::Mode::Rlva) != nullptr, spec.dialect != RLVONLY);

  RlvState state;
  state.setMode(spec.dialect == RLVONLY ? Rlv::Mode::Rlv : Rlv::Mode::Rlva);
  // a force value of the type RLVa reads (docs/RLV_OPTION_TYPES.md)
  QString value = "2";
  if (b->value.force == Rlv::ValueType::Vector3) value = "1/0.5/0";
  if (b->value.force == Rlv::ValueType::Vector4) value = "1/0.5/0/1";
  if (b->value.force == Rlv::ValueType::TextureUuid) value = "c2d4ef12-3a45-4b67-89ab-0123456789ab";
  const QString opt = spec.option == NO ? QString() : ":1/0.5/0";
  if (!(spec.params & AR))
  {
    // a value set with =force only
    QCOMPARE(one(state, "@" + name + opt + "=n").reason, QString("@%1 does not take =n").arg(name));
    QVERIFY(state.restrictions().isEmpty());
    const RlvState::Outcome forced = one(state, "@" + name + ":" + value + "=force");
    QCOMPARE(forced.result, Rlv::Result::Retained);
    QVERIFY(forced.reason.contains("recorded only"));
    QCOMPARE(state.recorded().value(name), value);
    return;
  }
  const RlvState::Outcome added = one(state, "@" + name + opt + "=n");
  QCOMPARE(added.result, Rlv::Result::Success);
  QVERIFY2(added.reason.contains("recorded only"), qPrintable(added.reason));
  QVERIFY(state.has(name, spec.option == NO ? QString() : "1/0.5/0"));
  QCOMPARE(one(state, "@" + name + opt + "=n").result, Rlv::Result::Duplicate);
  for (bool allowed : state.actions()) QVERIFY(allowed);   // nothing in the sandbox is blocked
  if (spec.option == RQ) QCOMPARE(one(state, "@" + name + "=n").reason, QString("@%1 needs an option").arg(name));
  if (spec.option == NO) QCOMPARE(one(state, "@" + name + ":1=n").reason, QString("@%1 takes no option").arg(name));
  QCOMPARE(one(state, "@" + name + opt + "=y").result, Rlv::Result::Success);
  QVERIFY(state.restrictions().isEmpty());
  if (spec.params & FO)
  {
    const RlvState::Outcome forced = one(state, "@" + name + ":" + value + "=force");
    QCOMPARE(forced.result, Rlv::Result::Retained);
    QVERIFY(forced.reason.contains("recorded only"));
    QCOMPARE(state.recorded().value(name), value);
    QVERIFY(state.restrictions().isEmpty());
  }
  else
  {
    QVERIFY(one(state, "@" + name + opt + "=force").reason.contains("does not take"));
    QVERIFY(state.recorded().isEmpty());
  }
}

void TestRlvTable::sphereAndOverlayForce()
{
  RlvState state;
  one(state, "@setsphere_distmin:2=force");
  one(state, "@setoverlay_alpha:0.5=force");
  QCOMPARE(state.recorded().size(), 2);
  QCOMPARE(state.recorded().value("setsphere_distmin"), QString("2"));
  QCOMPARE(state.recorded().value("setoverlay_alpha"), QString("0.5"));
  // the last value wins and the record stays one entry per name
  one(state, "@setsphere_distmin:9=force");
  QCOMPARE(state.recorded().value("setsphere_distmin"), QString("9"));
  QCOMPARE(state.recorded().size(), 2);
}

void TestRlvTable::versionWithBlacklist()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlv);
  const RlvState::ApplyResult r = state.apply("obj", "@versionnumbl=1234");
  QCOMPARE(r.outcomes.value(0).result, Rlv::Result::Success);
  QCOMPARE(r.replies.size(), 1);
  QCOMPARE(r.replies[0].channel, 1234);
  QCOMPARE(r.replies[0].text, QString("3040300,"));
  // the plain version number is unchanged
  QCOMPARE(state.apply("obj", "@versionnum=1234").replies.value(0).text, QString("3040300"));
}

void TestRlvTable::blacklistAndZoomAnswers()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlv);
  RlvState::ApplyResult r = state.apply("obj", "@getblacklist=77");
  QCOMPARE(r.replies.size(), 1);
  QCOMPARE(r.replies[0].text, QString());
  r = state.apply("obj", "@getblacklist:sit=77");
  QCOMPARE(r.replies.value(0).text, QString());
  r = state.apply("obj", "@getcam_zoommin=77");
  QCOMPARE(r.replies.size(), 1);
  QCOMPARE(r.replies[0].channel, 77);
  QCOMPARE(r.replies[0].text, QString("1"));
  // the camera limit that was set does not change the sandbox answer
  one(state, "@camzoommin:3=n");
  QCOMPARE(state.apply("obj", "@getcam_zoommin=77").replies.value(0).text, QString("1"));
}

void TestRlvTable::rlvOnlyNamesInRlvaMode_data()
{
  QTest::addColumn<QString>("line");
  for (const QString line : {"@camdrawmin:5=n", "@camdrawmax:5=n", "@camdrawalphamin:1=n", "@camdrawalphamax:1=n",
                             "@camdrawcolor:1/0/0=n", "@versionnumbl=5", "@getblacklist=5", "@getcam_zoommin=5",
                             "@defaultwear=n", "@denypermission=n", "@setrot:1=force"})
    QTest::newRow(qPrintable(line)) << line;
}

void TestRlvTable::rlvOnlyNamesInRlvaMode()
{
  QFETCH(QString, line);
  RlvState state;
  state.setMode(Rlv::Mode::Rlv);
  const RlvState::Outcome known = one(state, line);
  QVERIFY2(known.result != Rlv::Result::Failed, qPrintable(line));   // known in plain RLV
  QVERIFY(!known.reason.contains("unknown behaviour"));
  state.setMode(Rlv::Mode::Rlva);
  const RlvState::Outcome outcome = one(state, line);
  QCOMPARE(outcome.result, Rlv::Result::Failed);
  QVERIFY2(outcome.reason.startsWith("unknown behaviour"), qPrintable(outcome.reason));
  QVERIFY(state.restrictions().isEmpty());
  QVERIFY(state.apply("obj", line).replies.isEmpty());
}

void TestRlvTable::tableHasNoDuplicatesAndIsMarked()
{
  QSet<QString> seen;
  int rlvOnly = 0;
  for (const Rlv::Behaviour& b : Rlv::behaviours())
  {
    QVERIFY2(!seen.contains(b.name), qPrintable(b.name));
    seen.insert(b.name);
    if (b.dialect == Rlv::Dialect::RlvOnly) rlvOnly++;
    if (!b.alias.isEmpty()) QVERIFY2(Rlv::findBehaviour(b.alias), qPrintable(b.name));   // an alias points at a real entry
  }
  // exactly the names that RLVa does not have
  QStringList names;
  for (const Rlv::Behaviour& b : Rlv::behaviours())
    if (b.dialect == Rlv::Dialect::RlvOnly) names << b.name;
  names.sort();
  QCOMPARE(names, QStringList({"camdrawalphamax", "camdrawalphamin", "camdrawcolor", "camdrawmax", "camdrawmin",
                               "defaultwear", "denypermission", "getblacklist", "getcam_zoommin", "setrot", "versionnumbl"}));
  QCOMPARE(rlvOnly, 11);
}

QTEST_GUILESS_MAIN(TestRlvTable)
#include "tst_rlvtable.moc"
