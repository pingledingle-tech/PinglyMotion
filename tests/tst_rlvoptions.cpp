/*
 * PinglyMotion - exact-answer tests for the Script Lab's RLV option value check
 * (lane 1, job C): every option value type, a value that fits and one that does not,
 * the exact log text, and the state after a refused command. The rules come from
 * RLVa's source; docs/RLV_OPTION_TYPES.md names the function behind each one.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"

namespace
{
  const char* const KEY = "c2d4ef12-3a45-4b67-89ab-0123456789ab";

  RlvState::Outcome one(RlvState& state, const QString& line, const QString& object = QStringLiteral("obj"))
  {
    const RlvState::ApplyResult r = state.apply(object, line);
    return r.outcomes.value(0);
  }

  int words(const QString& sentence)
  {
    return sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size();
  }
}

class TestRlvOptions : public QObject
{
  Q_OBJECT

  private slots:
    void fits_data();
    void fits();
    void restrictionRefused_data();
    void restrictionRefused();
    void removeWithBadValueIsUnset();
    void forceRefusedRecordsNothing();
    void forceFitsIsRecorded();
    void plainRlvModeDoesNotCheck();
    void oneBadCommandDoesNotStopTheLine();
    void valueTypesInTheTable_data();
    void valueTypesInTheTable();
    void everyTypeHasAWarning();
    void warningsAreShortSentences();
    void namesLists();
};

// optionFits for every type: what RLVa accepts and refuses
void TestRlvOptions::fits_data()
{
  QTest::addColumn<int>("type");
  QTest::addColumn<QString>("option");
  QTest::addColumn<bool>("fits");
  auto row = [](const char* name, Rlv::ValueType type, const QString& option, bool fits)
  { QTest::newRow(name) << static_cast<int>(type) << option << fits; };
  using V = Rlv::ValueType;
  row("empty always fits", V::Uuid, QString(), true);
  row("text anything", V::Unchecked, "any/folder path", true);
  row("none refuses", V::None, "x", false);
  row("uuid", V::Uuid, KEY, true);
  row("uuid upper", V::Uuid, "C2D4EF12-3A45-4B67-89AB-0123456789AB", true);
  row("uuid any separator", V::Uuid, "c2d4ef12x3a45x4b67x89abx0123456789ab", true);
  row("uuid 35 old form", V::Uuid, "c2d4ef12-3a45-4b67-89ab0123456789ab", true);
  row("uuid word", V::Uuid, "notauuid", false);
  row("uuid not hex", V::Uuid, "g2d4ef12-3a45-4b67-89ab-0123456789ab", false);
  row("uuid short", V::Uuid, "c2d4ef12-3a45-4b67-89ab-0123456789a", false);
  row("uuid space", V::Uuid, QString(" ") + KEY, false);
  row("texture uuid spaces", V::TextureUuid, QString(" ") + KEY + " ", true);
  row("texture uuid word", V::TextureUuid, "white", false);
  row("im uuid", V::UuidOrDistance, KEY, true);
  row("im distance", V::UuidOrDistance, "20", true);
  row("im two distances", V::UuidOrDistance, "5;20", true);
  row("im three parts", V::UuidOrDistance, "5;20;30", false);
  row("im negative", V::UuidOrDistance, "-1", false);
  row("im empty part", V::UuidOrDistance, "5;", false);
  row("im word", V::UuidOrDistance, "bob", false);
  row("tags uuid", V::UuidOrNumber, KEY, true);
  row("tags number", V::UuidOrNumber, "12.5", true);
  row("tags word", V::UuidOrNumber, "far", false);
  row("number", V::Number, "1.5", true);
  row("number negative", V::Number, "-2", true);
  row("number trailing text", V::Number, "3m", true);
  row("number leading space", V::Number, " 4", true);
  row("number word", V::Number, "far", false);
  row("number out of range", V::Number, "1e999", false);
  row("integer", V::Integer, "2", true);
  row("integer prefix", V::Integer, "2.7", true);
  row("integer word", V::Integer, "two", false);
  row("integer too big", V::Integer, "2147483648", false);
  row("channel", V::Channel, "5", true);
  row("channel zero", V::Channel, "0", false);
  row("channel negative", V::Channel, "-5", false);
  row("channel word", V::Channel, "five", false);
  row("reply channel", V::ReplyChannel, "7", true);
  row("reply channel highest", V::ReplyChannel, "2147483646", true);
  row("reply channel debug", V::ReplyChannel, "2147483647", false);
  row("reply channel zero", V::ReplyChannel, "0", false);
  row("vector", V::Vector3, "0/0/1", true);
  row("vector decimals", V::Vector3, "0.5/-1/2e1", true);
  row("vector two", V::Vector3, "0/1", false);
  row("vector commas", V::Vector3, "0,0,1", false);
  row("vector4", V::Vector4, "0/0/0/1", true);
  row("vector4 three", V::Vector4, "0/0/1", false);
  row("attach point", V::AttachPoint, "spine", true);
  row("attach point bento", V::AttachPoint, "tail base", true);
  row("attach point root", V::AttachPoint, "root", true);
  row("attach point bad", V::AttachPoint, "x", false);
  row("layer", V::Layer, "shirt", true);
  row("layer universal", V::Layer, "universal", true);
  row("layer bad", V::Layer, "hat", false);
  row("camera mode", V::CameraMode, "mouselook", true);
  row("camera mode bad", V::CameraMode, "firstperson", false);
  row("notify left to the state", V::Notify, "abc", true);
}

void TestRlvOptions::fits()
{
  QFETCH(int, type);
  QFETCH(QString, option);
  QFETCH(bool, fits);
  QCOMPARE(Rlv::optionFits(static_cast<Rlv::ValueType>(type), option), fits);
}

// a refused add: the exact warning, the command failed and nothing is held
void TestRlvOptions::restrictionRefused_data()
{
  QTest::addColumn<QString>("line");
  QTest::addColumn<QString>("behaviour");
  QTest::addColumn<QString>("good");
  QTest::addColumn<QString>("warning");
  QTest::newRow("uuid") << "@sendim:notauuid=n" << "sendim" << KEY
      << "@sendim:notauuid=n: \"notauuid\" is not a key or a distance. Use a key, or metres such as 20 or 5;20.";
  QTest::newRow("uuid required") << "@sendimto:bob=add" << "sendimto" << KEY
      << "@sendimto:bob=add: \"bob\" is not a valid key (UUID). Use a key of 36 characters, like 0f1e2d3c-4b5a-6978-8796-a5b4c3d2e1f0.";
  QTest::newRow("uuid optional") << "@recvchat:bob=add" << "recvchat" << KEY
      << "@recvchat:bob=add: \"bob\" is not a valid key (UUID). Use a key of 36 characters, like 0f1e2d3c-4b5a-6978-8796-a5b4c3d2e1f0.";
  QTest::newRow("texture") << "@setcam_textures:white=n" << "setcam_textures" << KEY
      << "@setcam_textures:white=n: \"white\" is not a valid key (UUID). Use a key of 36 characters, like 0f1e2d3c-4b5a-6978-8796-a5b4c3d2e1f0.";
  QTest::newRow("uuid or number") << "@shownametags:far=n" << "shownametags" << "12"
      << "@shownametags:far=n: \"far\" is not a key or a number. Use a key, or metres such as 20.";
  QTest::newRow("number") << "@sittp:far=n" << "sittp" << "2.5"
      << "@sittp:far=n: \"far\" is not a number. Use a number such as 1.5.";
  QTest::newRow("channel") << "@sendchannel:abc=add" << "sendchannel" << "5"
      << "@sendchannel:abc=add: \"abc\" is not a valid channel. Use a whole number from 1.";
  QTest::newRow("reply channel") << "@redirchat:0=add" << "redirchat" << "7"
      << "@redirchat:0=add: \"0\" is not a valid channel. Use a channel from 1 to 2147483646.";
  QTest::newRow("vector") << "@setcam_eyeoffset:1/2=n" << "setcam_eyeoffset" << "1/2/3"
      << "@setcam_eyeoffset:1/2=n: \"1/2\" is not three numbers x/y/z. Use three numbers such as 0/0/1.";
  QTest::newRow("attach point") << "@detach:x=n" << "detach" << "spine"
      << "@detach:x=n: \"x\" is not an attachment point. Use a name such as spine or left hand.";
  QTest::newRow("layer") << "@remoutfit:hat=n" << "remoutfit" << "shirt"
      << "@remoutfit:hat=n: \"hat\" is not a clothing layer. Use a name such as shirt or pants.";
  QTest::newRow("none") << "@viewnote:" + QString(KEY) + "=n" << "viewnote" << QString()
      << "@viewnote:" + QString(KEY) + "=n: RLVa takes no option here. Remove \":" + QString(KEY) + "\".";
}

void TestRlvOptions::restrictionRefused()
{
  QFETCH(QString, line);
  QFETCH(QString, behaviour);
  QFETCH(QString, good);
  QFETCH(QString, warning);
  RlvState state;
  QSignalSpy changed(&state, &RlvState::changed);
  const RlvState::ApplyResult r = state.apply("obj", line);
  QVERIFY(r.ok());
  QCOMPARE(r.outcomes.size(), 1);
  QCOMPARE(r.outcomes.at(0).result, Rlv::Result::Failed);
  QCOMPARE(r.outcomes.at(0).reason, warning);
  QVERIFY(state.restrictions().isEmpty());
  QVERIFY(state.objects().isEmpty());
  QCOMPARE(changed.count(), 0);
  // the value that fits is held
  const QString okLine = good.isEmpty() ? "@" + behaviour + "=n" : "@" + behaviour + ":" + good + "=n";
  const RlvState::Outcome ok = one(state, okLine);
  QCOMPARE(ok.result, Rlv::Result::Success);
  QVERIFY(!ok.reason.startsWith("@"));
  QVERIFY(state.has(behaviour, good));
  QCOMPARE(changed.count(), 1);
}

void TestRlvOptions::removeWithBadValueIsUnset()
{
  RlvState state;
  one(state, "@detach:spine=n");
  QSignalSpy changed(&state, &RlvState::changed);
  const RlvState::Outcome outcome = one(state, "@detach:x=y");
  QCOMPARE(outcome.result, Rlv::Result::Unset);
  QCOMPARE(outcome.reason, QString("@detach:x=y: \"x\" is not an attachment point. Use a name such as spine or left hand."));
  QCOMPARE(state.objects().value("obj"), QStringList{"detach:spine"});
  QCOMPARE(changed.count(), 0);
}

void TestRlvOptions::forceRefusedRecordsNothing()
{
  RlvState state;
  QSignalSpy changed(&state, &RlvState::changed);
  struct Row { const char* line; const char* warning; };
  const Row rows[] = {
    {"@setcam_fov:wide=force", "@setcam_fov:wide=force: \"wide\" is not a number. Use a number such as 1.5."},
    {"@setcam_mode:firstperson=force", "@setcam_mode:firstperson=force: \"firstperson\" is not a camera mode. Use mouselook, thirdperson or reset."},
    {"@setsphere_mode:soft=force", "@setsphere_mode:soft=force: \"soft\" is not a whole number. Use a whole number such as 2."},
    {"@setoverlay_tint:red=force", "@setoverlay_tint:red=force: \"red\" is not three numbers x/y/z. Use three numbers such as 0/0/1."},
    {"@sit:chair=force", "@sit:chair=force: \"chair\" is not a valid key (UUID). Use a key of 36 characters, like 0f1e2d3c-4b5a-6978-8796-a5b4c3d2e1f0."},
  };
  for (const Row& row : rows)
  {
    const RlvState::Outcome outcome = one(state, row.line);
    QCOMPARE(outcome.result, Rlv::Result::Failed);
    QCOMPARE(outcome.reason, QString(row.warning));
  }
  QVERIFY(state.recorded().isEmpty());
  QVERIFY(state.recordedBy().isEmpty());
  QCOMPARE(changed.count(), 0);
}

void TestRlvOptions::forceFitsIsRecorded()
{
  RlvState state;
  QCOMPARE(one(state, "@setcam_fov:1.2=force").result, Rlv::Result::Retained);
  QCOMPARE(state.recorded().value("setcam_fov"), QString("1.2"));
  QCOMPARE(one(state, "@setcam_mode:mouselook=force").result, Rlv::Result::Retained);
  QCOMPARE(one(state, "@sit:" + QString(KEY) + "=force").result, Rlv::Result::Retained);
  // no option where one may be left out is not a value to check
  QCOMPARE(one(state, "@setcam_fov=force").result, Rlv::Result::Retained);
  // force forms whose option RLVa reads from the inventory are not checked
  QCOMPARE(one(state, "@detach:my folder=force").result, Rlv::Result::Retained);
}

void TestRlvOptions::plainRlvModeDoesNotCheck()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlv);
  QCOMPARE(one(state, "@sendim:notauuid=n").result, Rlv::Result::Success);
  QCOMPARE(one(state, "@detach:x=n").result, Rlv::Result::Success);
  QVERIFY(state.has("sendim", "notauuid"));
  Rlv::Command command = Rlv::parseCommand("sendim:notauuid=n");
  QCOMPARE(Rlv::optionProblem(command, Rlv::Mode::Rlv), QString());
  QVERIFY(!Rlv::optionProblem(command, Rlv::Mode::Rlva).isEmpty());
}

void TestRlvOptions::oneBadCommandDoesNotStopTheLine()
{
  RlvState state;
  const RlvState::ApplyResult r = state.apply("obj", "@fly=n,sendim:bob=add,detach:spine=n");
  QVERIFY(r.ok());
  QCOMPARE(r.outcomes.size(), 3);
  QCOMPARE(r.outcomes.at(0).result, Rlv::Result::Success);
  QCOMPARE(r.outcomes.at(1).result, Rlv::Result::Failed);
  QCOMPARE(r.outcomes.at(2).result, Rlv::Result::Success);
  QCOMPARE(state.objects().value("obj"), (QStringList{"fly", "detach:spine"}));
}

// the value type of each form is a field of the behaviour table, checked here for a
// sample of rows (tests/tst_rlvsources.cpp checks every generic row against RLVa)
void TestRlvOptions::valueTypesInTheTable_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<int>("restriction");
  QTest::addColumn<int>("force");
  using V = Rlv::ValueType;
  auto row = [](const char* name, V r, V f) { QTest::newRow(name) << name << static_cast<int>(r) << static_cast<int>(f); };
  row("sendim", V::UuidOrDistance, V::Unchecked);
  row("sendimto", V::Uuid, V::Unchecked);
  row("sendchannel", V::Channel, V::Unchecked);
  row("redirchat", V::ReplyChannel, V::Unchecked);
  row("notify", V::Notify, V::Unchecked);
  row("detach", V::AttachPoint, V::Unchecked);
  row("addoutfit", V::Layer, V::Unchecked);
  row("sit", V::None, V::Uuid);
  row("setcam_eyeoffset", V::Vector3, V::Vector3);
  row("setcam_focusoffset", V::Unchecked, V::Vector3);
  row("setcam_mode", V::Unchecked, V::CameraMode);
  row("setsphere_param", V::Unchecked, V::Unchecked);   // a known difference, docs/RLV_OPTION_TYPES.md
  row("attachthis", V::Unchecked, V::Unchecked);
  row("fly", V::Unchecked, V::Unchecked);
}

void TestRlvOptions::valueTypesInTheTable()
{
  QFETCH(QString, name);
  QFETCH(int, restriction);
  QFETCH(int, force);
  const Rlv::Behaviour* b = Rlv::findBehaviour(name);
  QVERIFY(b);
  QCOMPARE(static_cast<int>(b->value.restriction), restriction);
  QCOMPARE(static_cast<int>(b->value.force), force);
  QCOMPARE(static_cast<int>(Rlv::valueType(*b, Rlv::ParamType::Add)), restriction);
  QCOMPARE(static_cast<int>(Rlv::valueType(*b, Rlv::ParamType::Remove)), restriction);
  QCOMPARE(static_cast<int>(Rlv::valueType(*b, Rlv::ParamType::Force)), force);
  QCOMPARE(static_cast<int>(Rlv::valueType(*b, Rlv::ParamType::Reply)), static_cast<int>(Rlv::ValueType::Unchecked));
}

// every checked type a table row uses gives a warning with a fix, and has a name
void TestRlvOptions::everyTypeHasAWarning()
{
  QSet<int> seen;
  for (const Rlv::Behaviour& b : Rlv::behaviours())
  {
    for (const Rlv::ValueType type : {b.value.restriction, b.value.force})
    {
      QVERIFY(!Rlv::valueTypeName(type).isEmpty());
      if (type == Rlv::ValueType::Unchecked || type == Rlv::ValueType::Notify || seen.contains(static_cast<int>(type))) continue;
      seen.insert(static_cast<int>(type));
      // the force form is checked for a type only force uses
      const bool force = type == b.value.force && type != b.value.restriction;
      const Rlv::Command command = Rlv::parseCommand(b.name + ":?bad=" + (force ? "force" : "n"));
      QVERIFY2(command.valid, qPrintable(b.name));
      const QString warning = Rlv::optionProblem(command, Rlv::Mode::Rlva);
      QVERIFY2(warning.startsWith("@" + command.text + ": "), qPrintable(b.name + ": " + warning));
      QVERIFY2(warning.contains("Use ") || warning.contains("Remove "), qPrintable(warning));
    }
  }
  QCOMPARE(seen.size(), 13);   // every type but Unchecked, Notify and Vector4 (no row uses it yet)
}

// plain words: at most 12 words in each sentence after the command (tools/check_texts.py's rule)
void TestRlvOptions::warningsAreShortSentences()
{
  for (const Rlv::Behaviour& b : Rlv::behaviours())
  {
    for (const QString& form : {QStringLiteral("n"), QStringLiteral("force")})
    {
      const Rlv::Command command = Rlv::parseCommand(b.name + ":?bad=" + form);
      QString warning = Rlv::optionProblem(command, Rlv::Mode::Rlva);
      if (warning.isEmpty()) continue;
      warning = warning.mid(command.text.size() + 3);   // "@" + text + ": "
      for (const QString& sentence : warning.split(QRegularExpression("\\.\\s"), Qt::SkipEmptyParts))
        QVERIFY2(words(sentence) <= 12, qPrintable(sentence));
    }
  }
}

void TestRlvOptions::namesLists()
{
  QCOMPARE(Rlv::optionProblem(Rlv::parseCommand("setsphere_param:1/2/3=force")), QString());
  QCOMPARE(Rlv::rlvaAttachPointNames().size(), 56);
  QCOMPARE(Rlv::rlvaLayerNames().size(), 17);
  // the sandbox's own lists for @getattach and @getoutfit are all known to RLVa (but "none")
  for (const QString& point : Rlv::attachmentPoints())
    if (point != QLatin1String("none")) QVERIFY2(Rlv::rlvaAttachPointNames().contains(point), qPrintable(point));
  for (const QString& layer : Rlv::clothingLayers()) QVERIFY2(Rlv::rlvaLayerNames().contains(layer), qPrintable(layer));
}

QTEST_GUILESS_MAIN(TestRlvOptions)
#include "tst_rlvoptions.moc"
