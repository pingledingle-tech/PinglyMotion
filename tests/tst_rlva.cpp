/*
 * PinglyMotion - tests for the Script Lab's RLVa-only behaviours and the mode
 * switch between plain RLV and RLVa (issue 297)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QSignalSpy>
#include <QtTest>

#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"

class TestRlva : public QObject
{
  Q_OBJECT

  private slots:
    void tableIsMarked();
    void rlvaNamesFromTheDictionary();
    void defaultModeIsPlainRlv();
    void plainModeAnswersUnknown_data();
    void plainModeAnswersUnknown();
    void rlvaModeAccepts_data();
    void rlvaModeAccepts();
    void plainBehavioursWorkInBothModes();
    void restrictionsBlockActions();
    void requiredOption();
    void parameterTypes();
    void cameraIsRecordedOnly();
    void environmentIsRecordedOnly();
    void forceOutsideEffectIsRecorded();
    void replies();
    void getCommandFollowsTheMode();
    void switchingModeStartsClean();
    void modeSignal();
    void parseModeText();
    void recordedIsBounded();

  private:
    static RlvState::Outcome one(RlvState& state, const QString& line)
    {
      const RlvState::ApplyResult r = state.apply("obj", line);
      return r.outcomes.value(0);
    }
};

// RLVa-only names, read from RLVa's behaviour dictionary (BHVR_EXTENDED and BHVR_EXPERIMENTAL entries)
static const QStringList interactionOnly = {"interact", "touchhud", "sendgesture", "showself", "showselfhead",
                                            "shownearby", "viewtransparent", "viewwireframe", "allowidle"};
static const QStringList teleportOnly = {"accepttprequest", "tprequest", "tplocal", "sendchannel_except"};
static const QStringList sharedOnly = {"sharedwear", "sharedunwear"};
static const QStringList cameraOnly = {"setcam_avdistmin", "setcam_avdistmax", "setcam_origindistmin",
                                       "setcam_origindistmax", "setcam_focus", "setcam_fov", "setcam_mode"};
static const QStringList replyOnly = {"findfolders", "getcommand", "getheightoffset", "getaddattachnames",
                                      "getaddoutfitnames", "getattachnames", "getoutfitnames", "getremattachnames",
                                      "getremoutfitnames", "getcam_avdist", "getcam_avdistmin", "getcam_avdistmax",
                                      "getcam_fov", "getcam_fovmin", "getcam_fovmax", "getcam_textures"};

static QStringList allRlvaOnly()
{
  return interactionOnly + teleportOnly + sharedOnly + cameraOnly + replyOnly;
}

void TestRlva::tableIsMarked()
{
  QSet<QString> seen;
  for (const Rlv::Behaviour& behaviour : Rlv::behaviours())
  {
    QVERIFY2(!seen.contains(behaviour.name), qPrintable(behaviour.name));   // no name twice
    seen.insert(behaviour.name);
    QVERIFY(!behaviour.description.isEmpty());
    // an outside effect always says what the sandbox does instead
    if (behaviour.external) QVERIFY2(!behaviour.note.isEmpty(), qPrintable(behaviour.name));
  }
  // the original RLV core stays plain RLV
  for (const QString name : {"detach", "sendim", "fly", "version", "getstatus", "share"})
    QCOMPARE(Rlv::findBehaviour(name)->dialect, Rlv::Dialect::Rlv);
}

void TestRlva::rlvaNamesFromTheDictionary()
{
  QStringList found;
  for (const Rlv::Behaviour& behaviour : Rlv::behaviours())
    if (behaviour.dialect == Rlv::Dialect::RlvaOnly) found << behaviour.name;
  QStringList expected = allRlvaOnly();
  found.sort();
  expected.sort();
  QCOMPARE(found, expected);
}

void TestRlva::defaultModeIsPlainRlv()
{
  RlvState state;
  QCOMPARE(state.mode(), Rlv::Mode::Rlv);
  QCOMPARE(Rlv::modeName(state.mode()), QString("rlv"));
}

void TestRlva::plainModeAnswersUnknown_data()
{
  QTest::addColumn<QString>("name");
  for (const QString& name : allRlvaOnly()) QTest::newRow(qPrintable(name)) << name;
}

void TestRlva::plainModeAnswersUnknown()
{
  QFETCH(QString, name);
  RlvState state;
  const Rlv::Behaviour* behaviour = Rlv::findBehaviour(name);
  QVERIFY(behaviour);
  QString line;
  if (behaviour->params & Rlv::AllowAddRemove) line = "@" + name + (behaviour->option == Rlv::OptionUse::Required ? ":1" : "") + "=n";
  else if (behaviour->params & Rlv::AllowForce) line = "@" + name + ":1=force";
  else line = "@" + name + "=2222";
  const RlvState::Outcome outcome = one(state, line);
  QCOMPARE(outcome.result, Rlv::Result::Failed);
  QVERIFY2(outcome.reason.contains("unknown behaviour"), qPrintable(outcome.reason));
  QVERIFY(state.restrictions().isEmpty());
  QVERIFY(!Rlv::findBehaviour(name, Rlv::Mode::Rlv));
  QVERIFY(Rlv::findBehaviour(name, Rlv::Mode::Rlva));
}

void TestRlva::rlvaModeAccepts_data()
{
  QTest::addColumn<QString>("line");
  QTest::addColumn<int>("result");
  const int ok = static_cast<int>(Rlv::Result::Success);
  const int kept = static_cast<int>(Rlv::Result::Retained);
  // one row per behaviour group
  QTest::newRow("interaction") << "@interact=n" << ok;
  QTest::newRow("touchhud") << "@touchhud=n" << ok;
  QTest::newRow("touchhud exception") << "@touchhud:c2d4ef12-3a45-4b67-89ab-0123456789ab=add" << ok;
  QTest::newRow("self view") << "@showself=n,showselfhead=n" << ok;
  QTest::newRow("view modes") << "@viewwireframe=n" << ok;
  QTest::newRow("teleport") << "@tprequest=n" << ok;
  QTest::newRow("accepttprequest") << "@accepttprequest=add" << ok;
  QTest::newRow("tplocal") << "@tplocal:10=n" << ok;
  QTest::newRow("sendchannel_except") << "@sendchannel_except:5=n" << ok;
  QTest::newRow("shared folder") << "@sharedwear=n" << ok;
  QTest::newRow("camera limit") << "@setcam_avdistmin:2.5=n" << ok;
  QTest::newRow("camera origin") << "@setcam_origindistmax:20=n" << ok;
  QTest::newRow("camera force") << "@setcam_fov:1.2=force" << kept;
  QTest::newRow("camera mode") << "@setcam_mode:mouselook=force" << kept;
  QTest::newRow("camera focus") << "@setcam_focus:abc;0/0/1=force" << kept;
  QTest::newRow("reply") << "@getcommand=2222" << ok;
  QTest::newRow("reply names") << "@getattachnames=2222" << ok;
  QTest::newRow("reply camera") << "@getcam_fov=2222" << ok;
}

void TestRlva::rlvaModeAccepts()
{
  QFETCH(QString, line);
  QFETCH(int, result);
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  const RlvState::ApplyResult r = state.apply("obj", line);
  QVERIFY(r.ok());
  QVERIFY(!r.outcomes.isEmpty());
  for (const RlvState::Outcome& outcome : r.outcomes)
    QCOMPARE(static_cast<int>(outcome.result), result);
}

void TestRlva::plainBehavioursWorkInBothModes()
{
  for (const Rlv::Mode mode : {Rlv::Mode::Rlv, Rlv::Mode::Rlva})
  {
    RlvState state;
    state.setMode(mode);
    QCOMPARE(one(state, "@fly=n").result, Rlv::Result::Success);
    QCOMPARE(one(state, "@sendim=n").result, Rlv::Result::Success);
    QVERIFY(state.has("fly"));
    QCOMPARE(state.actions().value("fly"), false);
    QCOMPARE(state.apply("obj", "@version=2222").replies.value(0).text, Rlv::versionText(true));
    // the camera and sky behaviours of plain RLV are known in both modes
    QCOMPARE(one(state, "@setenv=n").result, Rlv::Result::Success);
    QCOMPARE(one(state, "@setcam=n").result, Rlv::Result::Success);
  }
}

void TestRlva::restrictionsBlockActions()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  state.apply("obj", "@interact=n,tprequest=n,sharedwear=n,showself=n");
  const QMap<QString, bool> actions = state.actions();
  QCOMPARE(actions.value("interact"), false);
  QCOMPARE(actions.value("tprequest"), false);
  QCOMPARE(actions.value("sharedwear"), false);
  QCOMPARE(actions.value("showself"), false);
  QCOMPARE(actions.value("sharedunwear"), true);
  state.apply("obj", "@interact=y");
  QCOMPARE(state.actions().value("interact"), true);
  // with an option it is an exception, not a block
  state.apply("obj", "@touchhud:abc=add");
  QCOMPARE(state.actions().value("touchhud"), true);
  state.apply("obj", "@touchhud=n");
  QCOMPARE(state.actions().value("touchhud"), false);
}

void TestRlva::requiredOption()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  RlvState::Outcome outcome = one(state, "@setcam_avdistmin=n");
  QCOMPARE(outcome.result, Rlv::Result::Failed);
  QVERIFY(outcome.reason.contains("needs an option"));
  QCOMPARE(one(state, "@setcam_avdistmin:3=n").result, Rlv::Result::Success);
  QCOMPARE(one(state, "@setcam_avdistmin:3=y").result, Rlv::Result::Success);
  // plain RLV @setcam_avdist also needs its distance
  QCOMPARE(one(state, "@setcam_avdist=n").result, Rlv::Result::Failed);
  QCOMPARE(one(state, "@setcam_avdist:4=n").result, Rlv::Result::Success);
  // a behaviour without options refuses one
  outcome = one(state, "@interact:1=n");
  QCOMPARE(outcome.result, Rlv::Result::Failed);
  QVERIFY(outcome.reason.contains("takes no option"));
}

void TestRlva::parameterTypes()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  // force-only behaviours refuse n and y
  QCOMPARE(one(state, "@setcam_focus:abc=n").result, Rlv::Result::Failed);
  QCOMPARE(one(state, "@setcam_mode:thirdview=add").result, Rlv::Result::Failed);
  // restrictions refuse force
  QCOMPARE(one(state, "@interact=force").result, Rlv::Result::Failed);
  QCOMPARE(one(state, "@sharedwear=force").result, Rlv::Result::Failed);
  // reply-only behaviours refuse n
  QCOMPARE(one(state, "@getcommand=n").result, Rlv::Result::Failed);
  QCOMPARE(one(state, "@getcam_fov=force").result, Rlv::Result::Failed);
  QVERIFY(state.restrictions().isEmpty());
}

void TestRlva::cameraIsRecordedOnly()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  const RlvState::Outcome outcome = one(state, "@setcam_avdistmax:15=n");
  QCOMPARE(outcome.result, Rlv::Result::Success);
  QVERIFY2(outcome.reason.contains("recorded only"), qPrintable(outcome.reason));   // shown in the log
  QVERIFY(state.has("setcam_avdistmax", "15"));
  QCOMPARE(state.count("setcam_avdistmax", "15"), 1);
  // the camera group is all marked as outside effects
  for (const Rlv::Behaviour& behaviour : Rlv::behaviours())
    if (behaviour.group == Rlv::Group::Camera && behaviour.params != Rlv::AllowReply) QVERIFY2(behaviour.external, qPrintable(behaviour.name));
}

void TestRlva::environmentIsRecordedOnly()
{
  RlvState state;   // plain RLV
  const RlvState::Outcome sky = one(state, "@setenv=n");
  QCOMPARE(sky.result, Rlv::Result::Success);
  QVERIFY(sky.reason.contains("recorded only"));
  const RlvState::Outcome debug = one(state, "@setdebug=n");
  QCOMPARE(debug.result, Rlv::Result::Success);
  QVERIFY(debug.reason.contains("recorded only"));
  QVERIFY(state.has("setenv"));
  QCOMPARE(state.actions().value("setenv"), false);
  QCOMPARE(state.actions().value("setdebug"), false);
  // an ordinary restriction has no such note
  QVERIFY(one(state, "@fly=n").reason.isEmpty());
}

void TestRlva::forceOutsideEffectIsRecorded()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  QVERIFY(state.recorded().isEmpty());
  RlvState::Outcome outcome = one(state, "@setcam_fov:1.5=force");
  QCOMPARE(outcome.result, Rlv::Result::Retained);
  QVERIFY(outcome.reason.contains("recorded only"));
  QCOMPARE(state.recorded().value("setcam_fov"), QString("1.5"));
  one(state, "@setcam_fov:0.8=force");
  QCOMPARE(state.recorded().value("setcam_fov"), QString("0.8"));   // the last command wins
  QCOMPARE(state.recorded().size(), 1);
  one(state, "@setcam_eyeoffset:0/0/1=force");
  QCOMPARE(state.recorded().size(), 2);
  // a force command that acts inside the sandbox only is retained without a record
  one(state, "@unsit=force");
  QCOMPARE(state.recorded().size(), 2);
  state.reset();
  QVERIFY(state.recorded().isEmpty());
}

void TestRlva::replies()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  // no inventory, avatar or camera in the sandbox: empty answers on the channel
  for (const QString name : {"getattachnames", "getoutfitnames", "getremattachnames", "findfolders", "getcam_avdist", "getcam_fov", "getcam_textures"})
  {
    const RlvState::ApplyResult r = state.apply("obj", "@" + name + "=4321");
    QCOMPARE(r.outcomes.value(0).result, Rlv::Result::Success);
    QCOMPARE(r.replies.size(), 1);
    QCOMPARE(r.replies[0].channel, 4321);
    QCOMPARE(r.replies[0].text, QString());
  }
  const RlvState::ApplyResult height = state.apply("obj", "@getheightoffset=4321");
  QCOMPARE(height.replies.value(0).text, QString("0"));
}

void TestRlva::getCommandFollowsTheMode()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  RlvState::ApplyResult r = state.apply("obj", "@getcommand:setcam_avdist=1234");
  QCOMPARE(r.replies.size(), 1);
  QVERIFY(r.replies[0].text.startsWith("/setcam_avdist"));
  QVERIFY(r.replies[0].text.contains("/setcam_avdistmin"));
  QVERIFY(r.replies[0].text.contains("/setcam_avdistmax"));
  r = state.apply("obj", "@getcommand:tprequest=1234");
  // the filter matches anywhere in the name
  QVERIFY(r.replies[0].text.contains("/tprequest"));
  QVERIFY(r.replies[0].text.contains("/accepttprequest"));
  QVERIFY(!r.replies[0].text.contains("/fly"));
  // plain RLV does not list RLVa-only names; everything else is listed
  state.setMode(Rlv::Mode::Rlv);
  r = state.apply("obj", "@getcommand:tprequest=1234");
  QVERIFY(r.outcomes[0].reason.contains("unknown behaviour"));
  QVERIFY(r.replies.isEmpty());
}

void TestRlva::switchingModeStartsClean()
{
  RlvState state;
  state.apply("obj", "@fly=n,sendim=n");
  QVERIFY(state.has("fly"));
  state.setMode(Rlv::Mode::Rlva);
  QCOMPARE(state.mode(), Rlv::Mode::Rlva);
  QVERIFY(state.restrictions().isEmpty());
  QVERIFY(state.objects().isEmpty());
  state.apply("obj", "@interact=n,setcam_fov:1=force");
  QVERIFY(state.has("interact"));
  QVERIFY(!state.recorded().isEmpty());
  // back to plain RLV: nothing RLVa-only survives
  state.setMode(Rlv::Mode::Rlv);
  QVERIFY(state.restrictions().isEmpty());
  QVERIFY(state.recorded().isEmpty());
  QVERIFY(!state.has("interact"));
  QCOMPARE(one(state, "@interact=n").result, Rlv::Result::Failed);
  // switching to the mode it is already in changes nothing
  state.apply("obj", "@fly=n");
  state.setMode(Rlv::Mode::Rlv);
  QVERIFY(state.has("fly"));
}

void TestRlva::modeSignal()
{
  RlvState state;
  QSignalSpy spy(&state, &RlvState::modeChanged);
  state.setMode(Rlv::Mode::Rlv);   // already plain RLV
  QCOMPARE(spy.count(), 0);
  state.setMode(Rlv::Mode::Rlva);
  QCOMPARE(spy.count(), 1);
  // a restriction held before the switch is gone, and the change is announced
  RlvState other;
  other.apply("obj", "@fly=n");
  QSignalSpy changed(&other, &RlvState::changed);
  other.setMode(Rlv::Mode::Rlva);
  QCOMPARE(changed.count(), 1);
}

void TestRlva::parseModeText()
{
  Rlv::Mode mode = Rlv::Mode::Rlva;
  QVERIFY(Rlv::parseMode("rlv", mode));
  QCOMPARE(mode, Rlv::Mode::Rlv);
  QVERIFY(Rlv::parseMode(" RLVa ", mode));
  QCOMPARE(mode, Rlv::Mode::Rlva);
  QVERIFY(!Rlv::parseMode("", mode));
  QVERIFY(!Rlv::parseMode("rlvaa", mode));
  QVERIFY(!Rlv::parseMode("firestorm", mode));
  QCOMPARE(mode, Rlv::Mode::Rlva);   // a refused text leaves the value alone
}

void TestRlva::recordedIsBounded()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  // a long option is cut, and the record never holds more entries than there are behaviours
  state.apply("obj", "@setcam_fov:" + QString(Rlv::MAX_OPTION_LENGTH, 'a') + "=force");
  QCOMPARE(state.recorded().value("setcam_fov").length(), RlvState::MAX_RECORDED_LENGTH);
  for (int i = 0; i < 200; i++) state.apply("obj", QString("@setcam_fov:%1=force,setcam_mode:%1=force").arg(i));
  QVERIFY(state.recorded().size() <= Rlv::behaviours().size());
}

QTEST_GUILESS_MAIN(TestRlva)
#include "tst_rlva.moc"
