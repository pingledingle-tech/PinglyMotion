/*
 * PinglyMotion - tests of the RLV help rows and warnings for the Script Lab editor (lane 1, job R3)
 *
 * Exact answers only: one help row per behaviour and family, in the table's order; every example
 * parses, names its own command and passes the table's checks and the option value check (job C);
 * every plain sentence has at most 12 words and no avoided word (docs/TEACHING.md); the warning
 * for a given wrong command, letter for letter; the nearest name for a typo; the forms in words.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QRegularExpression>

#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvcommand.h"

class TestRlvExamples : public QObject
{
  Q_OBJECT

  private slots:
    void everyCommandHasOneRow();
    void examplesAreValid();
    void sentencesArePlain();
    void familyMembersShareTheRow();
    void warnings_data();
    void warnings();
    void nearestName();
    void formsText();
};

void TestRlvExamples::everyCommandHasOneRow()
{
  QStringList expected;
  for (const Rlv::Behaviour& b : Rlv::behaviours()) expected << b.name;
  for (const Rlv::Behaviour& b : Rlv::familyBehaviours()) expected << b.name;
  QStringList got;
  for (const Rlv::Help& row : Rlv::helpRows()) got << row.name;
  QCOMPARE(got, expected);
  QCOMPARE(got.size(), 223);
}

void TestRlvExamples::examplesAreValid()
{
  for (const Rlv::Help& row : Rlv::helpRows())
  {
    const Rlv::ParseResult parsed = Rlv::parseLine(row.example);
    QVERIFY2(parsed.ok(), qPrintable(row.example + ": " + parsed.error));
    QCOMPARE(int(parsed.commands.size()), 1);
    const Rlv::Command& c = parsed.commands.first();
    if (row.name.endsWith(QLatin1Char('_')))
      QVERIFY2(c.behaviour.startsWith(row.name) && c.behaviour.size() > row.name.size(), qPrintable(row.example));
    else
      QCOMPARE(c.behaviour, row.name);
    const Rlv::Behaviour* b = Rlv::findBehaviour(c.behaviour);
    QVERIFY(b);
    // judged in the viewer family that knows it, as the static check does
    const Rlv::Mode mode = b->dialect == Rlv::Dialect::RlvOnly ? Rlv::Mode::Rlv : Rlv::Mode::Rlva;
    QVERIFY2(Rlv::checkCommand(c, mode).isEmpty(), qPrintable(row.example + ": " + Rlv::checkCommand(c, mode)));
    QVERIFY2(Rlv::optionProblem(c, Rlv::Mode::Rlva).isEmpty(), qPrintable(row.example + ": " + Rlv::optionProblem(c, Rlv::Mode::Rlva)));
    QVERIFY2(Rlv::plainProblem(c, mode).isEmpty(), qPrintable(row.example + ": " + Rlv::plainProblem(c, mode)));
    // a restriction-only command is shown in its restriction form, never forced
    if (!(b->params & Rlv::AllowForce)) QVERIFY2(c.type != Rlv::ParamType::Force, qPrintable(row.example));
  }
}

void TestRlvExamples::sentencesArePlain()
{
  // the rules of tools/check_texts.py: at most 12 words a sentence, no avoided word
  static const QRegularExpression avoided(QStringLiteral(
    "\\b(key\\s*-?frames?|rotat(e|es|ed|ing|ion|ions)|interpolat\\w*|ease[ -]in|ease[ -]out|inverse kinematics|joint limits?|quaternions?|bake[sd]?|baking)\\b"),
    QRegularExpression::CaseInsensitiveOption);
  static const QRegularExpression caseSensitive(QStringLiteral("\\b(IK|Euler)\\b"));
  static const QRegularExpression sentenceEnd(QStringLiteral("(?<=[.!?])\\s+"));
  for (const Rlv::Help& row : Rlv::helpRows())
  {
    QVERIFY2(!row.sentence.isEmpty() && row.sentence.endsWith(QLatin1Char('.')), qPrintable(row.name));
    QVERIFY2(!avoided.match(row.sentence).hasMatch() && !caseSensitive.match(row.sentence).hasMatch(), qPrintable(row.sentence));
    for (const QString& sentence : row.sentence.split(sentenceEnd, Qt::SkipEmptyParts))
    {
      const int words = int(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size());
      QVERIFY2(words <= 12, qPrintable(QString("%1: %2 words").arg(sentence).arg(words)));
    }
  }
}

void TestRlvExamples::familyMembersShareTheRow()
{
  const Rlv::Help* help = Rlv::findHelp("setenv_daytime");
  QVERIFY(help);
  QCOMPARE(help->name, QString("setenv_"));
  QCOMPARE(help->example, QString("@setenv_daytime:0.5=force"));
  QCOMPARE(Rlv::findHelp("detach")->sentence, QString("Locks this object on you, so it cannot be taken off."));
  QVERIFY(!Rlv::findHelp("nosuch"));
}

void TestRlvExamples::warnings_data()
{
  QTest::addColumn<QString>("command");
  QTest::addColumn<QString>("mode");
  QTest::addColumn<QString>("warning");
  QTest::newRow("bad form") << "detach=x" << "rlva" << "@detach=x: x is not valid. Use n, y or force.";
  QTest::newRow("bad form, restriction only") << "sendchat=x" << "rlva" << "@sendchat=x: x is not valid. Use n or y.";
  QTest::newRow("no =") << "detach" << "rlva" << "@detach: the = part is missing. End it with =n, =y or =force.";
  QTest::newRow("no = on a question") << "version" << "rlva" << "@version: the = part is missing. End it with =2222 (a channel number).";
  QTest::newRow("nothing after =") << "detach=" << "rlva" << "@detach=: nothing comes after =. Use n, y or force.";
  QTest::newRow("typo") << "detatch=n" << "rlva" << "@detatch=n: detatch is not an RLV command. Did you mean detach?";
  QTest::newRow("unknown") << "zzzzzz=n" << "rlva" << "@zzzzzz=n: zzzzzz is not an RLV command. Check the spelling.";
  QTest::newRow("wrong form") << "sendchat=force" << "rlva" << "@sendchat=force: sendchat does not take =force. Use n or y.";
  QTest::newRow("question form on a restriction") << "sendchat=2222" << "rlva" << "@sendchat=2222: sendchat does not take a channel. Use n or y.";
  QTest::newRow("restriction form on a question") << "version=n" << "rlva" << "@version=n: version does not take =n. Use a channel number such as 2222.";
  QTest::newRow("RLVa-only form in RLV") << "fly=force" << "rlv" << "@fly=force: fly with =force is RLVa only. Use n or y.";
  QTest::newRow("RLVa-only form in RLVa") << "fly=force" << "rlva" << "";
  QTest::newRow("RLVa only in RLV") << "setcam=n" << "rlv" << "@setcam=n: setcam is RLVa only. RLV viewers do not know it.";
  QTest::newRow("RLV only in RLVa") << "defaultwear=n" << "rlva" << "@defaultwear=n: defaultwear is RLV only. RLVa viewers do not know it.";
  QTest::newRow("no option") << "sendchat:x=n" << "rlva" << "@sendchat:x=n: sendchat takes no option. Remove \":x\".";
  QTest::newRow("needs an option") << "redirchat=n" << "rlva" << "@redirchat=n: redirchat needs an option after \":\". Example: @redirchat:2222=n";
  QTest::newRow("bad value (job C)") << "detach:x=n" << "rlva" << "@detach:x=n: \"x\" is not an attachment point. Use a name such as spine or left hand.";
  QTest::newRow("bad value not checked in RLV") << "detach:x=n" << "rlv" << "";
  QTest::newRow("fine") << "detach=n" << "rlva" << "";
  QTest::newRow("clear") << "clear" << "rlva" << "";
  QTest::newRow("family") << "setenv_daytime:0.5=force" << "rlva" << "";
}

void TestRlvExamples::warnings()
{
  QFETCH(QString, command);
  QFETCH(QString, mode);
  QFETCH(QString, warning);
  Rlv::Mode m = Rlv::Mode::Rlva;
  QVERIFY(Rlv::parseMode(mode, m));
  QCOMPARE(Rlv::plainProblem(Rlv::parseCommand(command), m), warning);
}

void TestRlvExamples::nearestName()
{
  QCOMPARE(Rlv::nearestName("detatch"), QString("detach"));
  QCOMPARE(Rlv::nearestName("sendcht"), QString("sendchat"));
  QCOMPARE(Rlv::nearestName("shwinv"), QString("showinv"));
  QCOMPARE(Rlv::nearestName("xyzxyzxyz"), QString());
  QCOMPARE(Rlv::nearestName(QString()), QString());
  QCOMPARE(Rlv::nearestName(QString(500, QLatin1Char('a'))), QString());
}

void TestRlvExamples::formsText()
{
  QCOMPARE(Rlv::formsText(*Rlv::findBehaviour("detach"), Rlv::Mode::Rlva), QString("n, y or force"));
  QCOMPARE(Rlv::formsText(*Rlv::findBehaviour("fly"), Rlv::Mode::Rlva), QString("n, y or force"));
  QCOMPARE(Rlv::formsText(*Rlv::findBehaviour("fly"), Rlv::Mode::Rlv), QString("n or y"));
  QCOMPARE(Rlv::formsText(*Rlv::findBehaviour("version"), Rlv::Mode::Rlva), QString("a channel number such as 2222"));
  QCOMPARE(Rlv::formsText(*Rlv::findBehaviour("detachme"), Rlv::Mode::Rlva, true), QString("=force"));
  QCOMPARE(Rlv::formsOf(*Rlv::findBehaviour("detach")), (QStringList{"n", "y", "add", "rem", "force"}));
  QCOMPARE(Rlv::formsOf(*Rlv::findBehaviour("getstatus")), (QStringList{"<channel>"}));
  QVERIFY(Rlv::formIsRlvaOnly(*Rlv::findBehaviour("fly"), "force"));
  QVERIFY(!Rlv::formIsRlvaOnly(*Rlv::findBehaviour("fly"), "n"));
}

QTEST_GUILESS_MAIN(TestRlvExamples)
#include "tst_rlvexamples.moc"
