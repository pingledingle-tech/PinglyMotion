/*
 * PinglyMotion - tests for the status bar items and the "Tips on" setting (#88)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

#include "controlindicator.h"
#include "constants.h"
#include "usersettings.h"

class TestStatusBar : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();

    void auroraStates();
    void auroraTexts();
    void auroraWidgetFollowsTheLink();
    void auroraDotAsksToToggle();

    void tipsSwitchStartsOn();
    void tipsSwitchClick();
    void tipsSwitchSetOnIsQuiet();

    void tipsSettingDefaultsToOn();
    void tipsSettingIsSaved();
    void tipsSettingMissingInOldFile();

    void itemsSitOnPills();

  private:
    void removeSettingsFile();

    QTemporaryDir m_dir;
};

void TestStatusBar::initTestCase()
{
  QVERIFY(m_dir.isValid());
  // never touch the real settings of the person running the tests
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
  removeSettingsFile();
}

void TestStatusBar::removeSettingsFile()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
}

void TestStatusBar::auroraStates()
{
  typedef AuroraIndicator AI;
  QCOMPARE(AI::stateFor(false, false), AI::Off);
  QCOMPARE(AI::stateFor(false, true), AI::Off);   // switched off wins
  QCOMPARE(AI::stateFor(true, false), AI::Connecting);
  QCOMPARE(AI::stateFor(true, true), AI::Live);
}

void TestStatusBar::auroraTexts()
{
  typedef AuroraIndicator AI;
  QCOMPARE(AI::textFor(AI::Off), QStringLiteral("Aurora off"));
  QCOMPARE(AI::textFor(AI::Connecting), QStringLiteral("Aurora · connecting…"));
  QCOMPARE(AI::textFor(AI::Live), QStringLiteral("Aurora · live"));
}

void TestStatusBar::auroraWidgetFollowsTheLink()
{
  AuroraIndicator indicator;
  QCOMPARE(indicator.state(), AuroraIndicator::Off);
  QCOMPARE(indicator.text(), AuroraIndicator::textFor(AuroraIndicator::Off));

  indicator.setStatus(true, false);
  QCOMPARE(indicator.state(), AuroraIndicator::Connecting);
  QCOMPARE(indicator.text(), AuroraIndicator::textFor(AuroraIndicator::Connecting));

  indicator.setStatus(true, true);
  QCOMPARE(indicator.state(), AuroraIndicator::Live);
  QCOMPARE(indicator.text(), AuroraIndicator::textFor(AuroraIndicator::Live));

  // Aurora went away, the link keeps trying
  indicator.setStatus(true, false);
  QCOMPARE(indicator.state(), AuroraIndicator::Connecting);

  indicator.setStatus(false, false);
  QCOMPARE(indicator.state(), AuroraIndicator::Off);
}

void TestStatusBar::auroraDotAsksToToggle()
{
  AuroraIndicator indicator;
  QSignalSpy spy(&indicator, &AuroraIndicator::toggleRequested);
  QToolButton* dot = indicator.findChild<QToolButton*>();
  QVERIFY(dot);
  QVERIFY(!dot->toolTip().isEmpty());
  dot->click();
  QCOMPARE(spy.count(), 1);
  // only the window changes the link; the indicator waits for setStatus()
  QCOMPARE(indicator.state(), AuroraIndicator::Off);
}

void TestStatusBar::tipsSwitchStartsOn()
{
  TipsSwitch tips;
  QVERIFY(tips.isOn());
  QCOMPARE(tips.text(), QStringLiteral("Tips on"));
  QCOMPARE(TipsSwitch::textFor(false), QStringLiteral("Tips off"));
}

void TestStatusBar::tipsSwitchClick()
{
  TipsSwitch tips;
  QSignalSpy spy(&tips, &TipsSwitch::toggled);
  QToolButton* button = tips.findChild<QToolButton*>();
  QVERIFY(button);
  QVERIFY(!button->icon().isNull());

  button->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), false);
  QVERIFY(!tips.isOn());
  QCOMPARE(tips.text(), QStringLiteral("Tips off"));

  button->click();
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).at(0).toBool(), true);
  QCOMPARE(tips.text(), QStringLiteral("Tips on"));
}

void TestStatusBar::tipsSwitchSetOnIsQuiet()
{
  TipsSwitch tips;
  QSignalSpy spy(&tips, &TipsSwitch::toggled);
  tips.setOn(false);
  QVERIFY(!tips.isOn());
  QCOMPARE(tips.text(), QStringLiteral("Tips off"));
  tips.setOn(true);
  QCOMPARE(tips.text(), QStringLiteral("Tips on"));
  QCOMPARE(spy.count(), 0);
}

void TestStatusBar::tipsSettingDefaultsToOn()
{
  removeSettingsFile();
  UserSettings::setTipsOn(true);
  UserSettings::readSettings();   // no settings file yet: first start
  QVERIFY(UserSettings::tipsOn());
}

void TestStatusBar::tipsSettingIsSaved()
{
  removeSettingsFile();
  UserSettings::setTipsOn(false);
  UserSettings::writeSettings();
  UserSettings::setTipsOn(true);
  UserSettings::readSettings();
  QVERIFY(!UserSettings::tipsOn());

  UserSettings::setTipsOn(true);
  UserSettings::writeSettings();
  UserSettings::setTipsOn(false);
  UserSettings::readSettings();
  QVERIFY(UserSettings::tipsOn());
}

void TestStatusBar::tipsSettingMissingInOldFile()
{
  // a settings file from an older PinglyMotion has no tips_on key: tips are on
  removeSettingsFile();
  {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       Constants::organizationName(), Constants::applicationName());
    settings.setValue(QStringLiteral("user_settings/1.0/loop"), true);
    settings.sync();
  }
  UserSettings::setTipsOn(false);
  UserSettings::readSettings();
  QVERIFY(UserSettings::loop());
  QVERIFY(UserSettings::tipsOn());
}

void TestStatusBar::itemsSitOnPills()
{
  ControlIndicator control;
  AuroraIndicator aurora;
  TipsSwitch tips;
  QVERIFY(qobject_cast<StatusPill*>(&control));
  QVERIFY(qobject_cast<StatusPill*>(&aurora));
  QVERIFY(qobject_cast<StatusPill*>(&tips));
  // the remote-control indicator keeps its texts from #43
  QCOMPARE(control.text(), ControlIndicator::textFor(ControlIndicator::Off, QString(), QString(), -1));
}

QTEST_MAIN(TestStatusBar)
#include "tst_statusbar.moc"
