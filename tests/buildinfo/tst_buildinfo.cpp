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
};

void TestBuildInfo::labelStartsWithTheVersion()
{
  QVERIFY(Constants::buildLabel().startsWith(Constants::versionNumberString()));
}

void TestBuildInfo::developmentBuildNamesSourceAndCommit()
{
  // the tests are built without UPDATE_CHANNEL=2, so this is not a release label
  const QStringList parts = Constants::buildLabel().split(QStringLiteral(" · "));
  QCOMPARE(parts.size(), 3);
  QCOMPARE(parts.at(0), Constants::versionNumberString());
  QVERIFY(!parts.at(1).isEmpty());                 // "dev", "PR #12" or a branch name
  QVERIFY(parts.at(2).size() >= 1 && parts.at(2).size() <= 7);   // short commit or "unknown"
}

QTEST_MAIN(TestBuildInfo)
#include "tst_buildinfo.moc"
