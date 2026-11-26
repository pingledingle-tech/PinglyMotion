/*
 * PinglyMotion - tests of the RLV reference answers (package B): the file reader, the comparison with the
 * sandbox, and the list of known differences. See src/scriptlab/rlvreference.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "scriptlab/rlvreference.h"

namespace
{
  const QString HEADER = QStringLiteral("viewer test viewer\ndate 2026-11-26\nsource recorded\n");

  RlvReference::Reference read(const QString& body)
  {
    const RlvReference::Result r = RlvReference::parse(HEADER + body);
    if (!r.ok()) qWarning().noquote() << r.error;
    return r.value.value_or(RlvReference::Reference());
  }

  QString refusal(const QString& body, bool withHeader = true)
  {
    return RlvReference::parse((withHeader ? HEADER : QString()) + body).error;
  }
}

class TstRlvReference : public QObject
{
  Q_OBJECT

  private slots:
    void parseAndWriteRoundTrip()
    {
      const QString text = HEADER + QStringLiteral(
        "\ncase a\nsend @sendim=n\nsend @getstatus=2222\nanswer 2222 /sendim\nprefix 2223 RestrainedLife viewer v\nnumber 2224\nany 2225\n"
        "\ncase b\nmode rlv\nobject collar\nsend @getstatus=2222\nanswer 2222 \nsend @getsitid=2222\nsilent\n");
      const RlvReference::Result r = RlvReference::parse(text);
      QVERIFY2(r.ok(), qPrintable(r.error));
      QCOMPARE(r.value->cases.size(), 2);
      QCOMPARE(r.value->viewer, QStringLiteral("test viewer"));
      QVERIFY(!r.value->fromSpecification());
      QCOMPARE(r.value->cases.at(1).mode, Rlv::Mode::Rlv);
      QCOMPARE(r.value->cases.at(1).lines.at(0).object, QStringLiteral("collar"));
      const QString written = RlvReference::write(*r.value);
      const RlvReference::Result again = RlvReference::parse(written);
      QVERIFY2(again.ok(), qPrintable(again.error));
      QCOMPARE(RlvReference::write(*again.value), written);
    }

    void parseRefusals()
    {
      QCOMPARE(refusal(QStringLiteral("case a\nsend @sendim=n\nsilent\n"), false),
               QStringLiteral("Line 1: the header needs \"viewer\", \"date\" and \"source\" before the first case"));
      QCOMPARE(refusal(QString(), true), QStringLiteral("The file has no case."));
      QCOMPARE(refusal(QStringLiteral("viewer again\n")), QStringLiteral("Line 4: \"viewer\" is given twice"));
      QCOMPARE(refusal(QStringLiteral("send @a=n\n")),
               QStringLiteral("Line 4: \"send\" comes before the first case; the header is \"viewer\", \"date\" and \"source\""));
      QCOMPARE(refusal(QStringLiteral("case a\ncase a\n")), QStringLiteral("Line 5: the case \"a\" is there twice"));
      QCOMPARE(refusal(QStringLiteral("case a b\n")), QStringLiteral("Line 4: a case name uses only letters, digits, \"_\", \"-\" and \".\""));
      QCOMPARE(refusal(QStringLiteral("case a\nsend sendim=n\n")), QStringLiteral("Line 5: \"send\" needs an RLV line that starts with \"@\""));
      QCOMPARE(refusal(QStringLiteral("case a\nanswer 2222 x\n")), QStringLiteral("Line 5: \"answer\" must come below a \"send\" line"));
      QCOMPARE(refusal(QStringLiteral("case a\nsend @getsitid=2222\nanswer two x\n")),
               QStringLiteral("Line 6: \"answer\" needs a channel (a whole number) and then the text"));
      QCOMPARE(refusal(QStringLiteral("case a\nsend @getsitid=2222\nprefix 2222\n")),
               QStringLiteral("Line 6: \"prefix\" needs the start of the answer after the channel"));
      QCOMPARE(refusal(QStringLiteral("case a\nsend @getsitid=2222\nnumber 2222 x\n")), QStringLiteral("Line 6: \"number\" takes only the channel"));
      QCOMPARE(refusal(QStringLiteral("case a\nsend @getsitid=2222\nsilent\nsilent\n")),
               QStringLiteral("Line 7: \"silent\" stands alone, directly below a \"send\" line"));
      QCOMPARE(refusal(QStringLiteral("case a\nmode x\n")), QStringLiteral("Line 5: \"mode\" is \"rlva\" or \"rlv\""));
      QCOMPARE(refusal(QStringLiteral("case a\nobject \n")),
               QStringLiteral("Line 5: \"\" is not an object name (1 to 64 letters, digits, \"-\", \"_\", \".\" or space)"));
      QCOMPARE(refusal(QStringLiteral("case a\nfly\n")),
               QStringLiteral("Line 5: \"fly\" is not a known word (case, mode, object, send, answer, prefix, number, any, silent)"));
      QCOMPARE(refusal(QStringLiteral("case a\nsend @sendim=n\n")),
               QStringLiteral("Line 4: the case \"a\" has no \"answer\" or \"silent\" line, so it tests nothing"));
      QCOMPARE(refusal(QStringLiteral("viewer \n"), false), QStringLiteral("Line 1: \"viewer\" needs a value"));
      QCOMPARE(refusal(QStringLiteral("source guess\n"), false),
               QStringLiteral("Line 1: \"source\" is \"recorded\" (in Second Life) or \"specification\", not \"guess\""));
      QVERIFY(refusal(QStringLiteral("case a\nsend @x=") + QString(RlvReference::MAX_LINE_CHARS, QLatin1Char('x')) + QLatin1Char('\n'))
                  .contains(QStringLiteral("longer than")));
    }

    void norwegianLettersAndLongAnswers()
    {
      const QString answer = QString(300, QChar(0x00E6)) + QStringLiteral("øå");
      const RlvReference::Reference ref = read(QStringLiteral("case a\nsend @showhovertextworld=n\nsend @getstatus:;") + answer.left(5) +
                                               QStringLiteral("=2222\nanswer 2222 ") + answer.left(5) + QStringLiteral("showhovertextworld\n"));
      QCOMPARE(ref.cases.size(), 1);
      const QVector<RlvReference::CaseResult> result = RlvReference::run(ref);
      QVERIFY(result.at(0).same());
    }

    void runSameAndDifferent()
    {
      const RlvReference::Reference ref = read(QStringLiteral(
        "case same\nsend @sendim=n\nsend @getstatus=2222\nanswer 2222 /sendim\n"
        "case wrong_text\nsend @getsitid=2222\nanswer 2222 x\n"
        "case wrong_channel\nsend @getsitid=2222\nanswer 2223 00000000-0000-0000-0000-000000000000\n"
        "case too_few\nsend @getsitid=2222\nanswer 2222 00000000-0000-0000-0000-000000000000\nanswer 2222 more\n"
        "case too_many\nsend @getsitid=2222\nsilent\n"
        "case prefix_ok\nsend @version=2222\nprefix 2222 RestrainedLife viewer v\nsend @versionnum=2222\nnumber 2222\nsend @getgroup=2222\nany 2222\n"
        "case prefix_wrong\nsend @version=2222\nprefix 2222 Something else\nsend @getsitid=2222\nnumber 2222\n"));
      const QVector<RlvReference::CaseResult> r = RlvReference::run(ref);
      QCOMPARE(r.size(), 7);
      QVERIFY(r.at(0).same());
      QCOMPARE(r.at(1).differences.size(), 1);
      QCOMPARE(r.at(1).differences.at(0).text(),
               QStringLiteral("Line 10: expected 2222: \"x\", but the sandbox gave 2222: \"00000000-0000-0000-0000-000000000000\"."));
      QCOMPARE(r.at(2).differences.size(), 1);
      QCOMPARE(r.at(2).differences.at(0).got, QStringLiteral("2222: \"00000000-0000-0000-0000-000000000000\""));
      QCOMPARE(r.at(3).differences.size(), 1);
      QCOMPARE(r.at(3).differences.at(0).got, QStringLiteral("no answer"));
      QCOMPARE(r.at(4).differences.size(), 1);
      QCOMPARE(r.at(4).differences.at(0).expected, QStringLiteral("no answer"));
      QVERIFY(r.at(5).same());
      QCOMPARE(r.at(6).differences.size(), 2);
    }

    void aSendWithoutAnswersIsNotChecked()
    {
      const RlvReference::Reference ref = read(QStringLiteral("case a\nsend @notify:2222=add\nsend @sendim=n\nsend @getsitid=2222\nanswer 2222 00000000-0000-0000-0000-000000000000\n"));
      QVERIFY(RlvReference::run(ref).at(0).same());
    }

    void leftOverAnswersAreADifference()
    {
      const RlvReference::Reference ref = read(QStringLiteral("case a\nsend @notify:2222=add\nsend @sendim=n\nanswer 2222 /sendim=n\nsend @fly=n\nsend @getsitid=2222\nanswer 2222 00000000-0000-0000-0000-000000000000\n"));
      QVERIFY(RlvReference::run(ref).at(0).same());
      const RlvReference::Reference more = read(QStringLiteral("case a\nsend @notify:2222=add\nsend @sendim=n\nsilent\n"));
      const RlvReference::CaseResult r = RlvReference::run(more).at(0);
      QCOMPARE(r.differences.size(), 1);
      QCOMPARE(r.differences.at(0).expected, QStringLiteral("no answer"));
    }

    void modeAndObjects()
    {
      // plain RLV does not know the RLVa-only behaviours, so the sandbox gives no answer there
      const RlvReference::Reference ref = read(QStringLiteral(
        "case plain\nmode rlv\nsend @getattachnames=2222\nsilent\n"
        "case rlva\nsend @getattachnames=2222\nanswer 2222 \n"
        "case two_objects\nobject a\nsend @sendim=n\nobject b\nsend @fly=n\nsend @getstatusall=2222\nanswer 2222 /sendim/fly\n"));
      const QVector<RlvReference::CaseResult> r = RlvReference::run(ref);
      QVERIFY(r.at(0).same());
      QVERIFY(r.at(1).same());
      QVERIFY(r.at(2).same());
    }

    void casesDoNotShareState()
    {
      const RlvReference::Reference ref = read(QStringLiteral(
        "case first\nsend @sendim=n\nsend @getstatus=2222\nanswer 2222 /sendim\n"
        "case second\nsend @getstatus=2222\nanswer 2222 \n"));
      const QVector<RlvReference::CaseResult> r = RlvReference::run(ref);
      QVERIFY(r.at(0).same());
      QVERIFY(r.at(1).same());
    }

    void knownDifferences()
    {
      const RlvReference::KnownResult k = RlvReference::parseKnown(QStringLiteral(
        "# note\n\nintended a/one The sandbox has no avatar.\nunclear a/two Not judged.\nfixed b/three Was wrong.\n"));
      QVERIFY2(k.ok(), qPrintable(k.error));
      QCOMPARE(k.list.size(), 3);
      QCOMPARE(RlvReference::stateName(k.list.at(0).state), QStringLiteral("intended"));
      QCOMPARE(RlvReference::stateName(k.list.at(1).state), QStringLiteral("unclear"));
      QCOMPARE(RlvReference::stateName(k.list.at(2).state), QStringLiteral("fixed"));
      QCOMPARE(k.list.at(0).reason, QStringLiteral("The sandbox has no avatar."));
      QCOMPARE(k.list.at(2).line, 5);
      QCOMPARE(RlvReference::parseKnown(QStringLiteral("maybe a/b why\n")).error,
               QStringLiteral("Line 1: the state is \"fixed\", \"intended\" or \"unclear\", not \"maybe\""));
      QCOMPARE(RlvReference::parseKnown(QStringLiteral("fixed ab why\n")).error, QStringLiteral("Line 1: the second word is \"<file>/<case>\""));
      QCOMPARE(RlvReference::parseKnown(QStringLiteral("fixed a/b\n")).error, QStringLiteral("Line 1: a known difference needs a reason in words"));
      QCOMPARE(RlvReference::parseKnown(QStringLiteral("fixed a/b x\nunclear a/b y\n")).error, QStringLiteral("Line 2: \"a/b\" is there twice"));
    }
};

QTEST_APPLESS_MAIN(TstRlvReference)
#include "tst_rlvreference.moc"
