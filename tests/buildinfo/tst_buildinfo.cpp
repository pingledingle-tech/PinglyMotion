/*
 * PinglyMotion - test of the build label in the window title (#21)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "constants.h"

class TestBuildInfo : public QObject
{
  Q_OBJECT

  private slots:
    void labelStartsWithTheVersion();
    void developmentBuildNamesSourceAndCommit();
    void pullRequestLabelKeepsTheNumberSign();
    void releaseLabelIsTheVersionOnly();
    void missingPartsAreLeftOut();
    void windowTitleHasLabelAndFile();
    void ciSourceArrivesInTheProgram();
};

void TestBuildInfo::labelStartsWithTheVersion()
{
  QVERIFY(Constants::buildLabel().startsWith(Constants::versionNumberString()));
}

void TestBuildInfo::developmentBuildNamesSourceAndCommit()
{
  // the tests are built without UPDATE_CHANNEL=2, so this is not a release label
  const QStringList parts = Constants::buildLabel().split(QStringLiteral(" · "));
  QVERIFY2(parts.size() == 3 || parts.size() == 4, qPrintable(Constants::buildLabel()));
  QCOMPARE(parts.at(0), Constants::versionNumberString());
  QVERIFY(!parts.at(1).isEmpty());                 // "dev", "PR #12" or a branch name
  QVERIFY(parts.at(2).size() >= 1 && parts.at(2).size() <= 7);   // short commit or "unknown"
  if (parts.size() == 4)
    QCOMPARE(parts.at(3), QStringLiteral("build ") + Constants::buildNumber());
}

void TestBuildInfo::pullRequestLabelKeepsTheNumberSign()
{
  QCOMPARE(Constants::composeBuildLabel("0.4.0", false, "PR #120", "5085c90", "345"),
           QStringLiteral("0.4.0 · PR #120 · 5085c90 · build 345"));
  QCOMPARE(Constants::composeBuildLabel("0.4.0", false, "fix/title", "5085c90", "345"),
           QStringLiteral("0.4.0 · fix/title · 5085c90 · build 345"));
}

void TestBuildInfo::releaseLabelIsTheVersionOnly()
{
  QCOMPARE(Constants::composeBuildLabel("0.4.0", true, "PR #120", "5085c90", "345"), QStringLiteral("0.4.0"));
}

void TestBuildInfo::missingPartsAreLeftOut()
{
  QCOMPARE(Constants::composeBuildLabel("0.4.0", false, "", "", "0"), QStringLiteral("0.4.0 · dev"));
  QCOMPARE(Constants::composeBuildLabel("0.4.0", false, "dev", "5085c90", ""), QStringLiteral("0.4.0 · dev · 5085c90"));
}

void TestBuildInfo::windowTitleHasLabelAndFile()
{
  QCOMPARE(Constants::windowTitle("walk.bvh"),
           QStringLiteral("PinglyMotion ") + Constants::buildLabel() + QStringLiteral(" [walk.bvh]"));
}

// The Linux check builds with the same source as the Windows zip ("PR #120") and
// sets PINGLYMOTION_EXPECT_BUILD_SOURCE, so a source that gets lost on the way fails here.
void TestBuildInfo::ciSourceArrivesInTheProgram()
{
  const QString expected = qEnvironmentVariable("PINGLYMOTION_EXPECT_BUILD_SOURCE");
  if (expected.isEmpty()) QSKIP("not a CI build");
  QCOMPARE(Constants::buildLabel().split(QStringLiteral(" · ")).value(1), expected);
}

QTEST_MAIN(TestBuildInfo)
#include "tst_buildinfo.moc"
