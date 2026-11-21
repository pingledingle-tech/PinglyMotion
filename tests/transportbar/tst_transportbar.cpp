/*
 * PinglyMotion - test of the transport buttons under the timeline
 *
 * docs/UX_DECISIONS.md section 1, build order step 1: three buttons Start,
 * Stop and Replay from start, and a Loop switch that is on by default and
 * remembered between sessions. Start can be clicked only while stopped, Stop
 * only while playing, Replay from start always.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCheckBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "constants.h"
#include "transportbar.h"
#include "usersettings.h"

class TestTransportBar : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void buttonTexts();
    void stoppedStartAndReplayOnly();
    void playingStopAndReplayOnly();
    void clicksAreReported();
    void loopIsOnByDefault();
    void setLoopSendsNoSignal();
    void loopClickIsReported();

    void loopSettingDefaultsToOn();
    void loopSettingDefaultsToOnInAnOlderFile();
    void loopSettingIsRemembered();

  private:
    QTemporaryDir m_dir;
};

static const QString group = QStringLiteral("user_settings/1.0/");

void TestTransportBar::initTestCase()
{
  QVERIFY(m_dir.isValid());
  // never touch the real settings of the person running the tests
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void TestTransportBar::init()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
}

void TestTransportBar::buttonTexts()
{
  TransportBar bar;
  QCOMPARE(bar.startButton()->text(), QStringLiteral("Start"));
  QCOMPARE(bar.stopButton()->text(), QStringLiteral("Stop"));
  QCOMPARE(bar.replayButton()->text(), QStringLiteral("Replay from start"));
  QCOMPARE(bar.loopCheckBox()->text(), QStringLiteral("Loop"));
  // three separate buttons, none of them a toggle that changes its meaning
  QVERIFY(!bar.startButton()->isCheckable());
  QVERIFY(!bar.stopButton()->isCheckable());
  QVERIFY(!bar.replayButton()->isCheckable());
}

void TestTransportBar::stoppedStartAndReplayOnly()
{
  TransportBar bar;
  // a new bar shows the stopped state
  QVERIFY(bar.startButton()->isEnabled());
  QVERIFY(!bar.stopButton()->isEnabled());
  QVERIFY(bar.replayButton()->isEnabled());

  bar.setPlaystate(PLAYSTATE_LOOPING);
  bar.setPlaystate(PLAYSTATE_STOPPED);
  QVERIFY(bar.startButton()->isEnabled());
  QVERIFY(!bar.stopButton()->isEnabled());
  QVERIFY(bar.replayButton()->isEnabled());
}

void TestTransportBar::playingStopAndReplayOnly()
{
  TransportBar bar;
  for (PlayState state : {PLAYSTATE_LOOPING, PLAYSTATE_PLAYING})
  {
    bar.setPlaystate(state);
    QVERIFY(!bar.startButton()->isEnabled());
    QVERIFY(bar.stopButton()->isEnabled());
    QVERIFY(bar.replayButton()->isEnabled());
  }
}

void TestTransportBar::clicksAreReported()
{
  TransportBar bar;
  QSignalSpy start(&bar, &TransportBar::startClicked);
  QSignalSpy stop(&bar, &TransportBar::stopClicked);
  QSignalSpy replay(&bar, &TransportBar::replayClicked);

  bar.startButton()->click();
  QCOMPARE(start.count(), 1);
  QCOMPARE(stop.count(), 0);
  QCOMPARE(replay.count(), 0);

  bar.setPlaystate(PLAYSTATE_LOOPING);
  bar.stopButton()->click();
  QCOMPARE(stop.count(), 1);

  bar.replayButton()->click();
  QCOMPARE(replay.count(), 1);

  // a button that cannot be clicked sends nothing
  bar.setPlaystate(PLAYSTATE_STOPPED);
  bar.stopButton()->click();
  QCOMPARE(stop.count(), 1);
}

void TestTransportBar::loopIsOnByDefault()
{
  TransportBar bar;
  QVERIFY(bar.loop());
  QVERIFY(bar.loopCheckBox()->isChecked());
}

void TestTransportBar::setLoopSendsNoSignal()
{
  TransportBar bar;
  QSignalSpy toggled(&bar, &TransportBar::loopToggled);
  bar.setLoop(false);
  QVERIFY(!bar.loop());
  bar.setLoop(true);
  QVERIFY(bar.loop());
  QCOMPARE(toggled.count(), 0);
}

void TestTransportBar::loopClickIsReported()
{
  TransportBar bar;
  QSignalSpy toggled(&bar, &TransportBar::loopToggled);
  bar.loopCheckBox()->click();
  QCOMPARE(toggled.count(), 1);
  QCOMPARE(toggled.at(0).at(0).toBool(), false);
  bar.loopCheckBox()->click();
  QCOMPARE(toggled.count(), 2);
  QCOMPARE(toggled.at(1).at(0).toBool(), true);
}

void TestTransportBar::loopSettingDefaultsToOn()
{
  // a first start has no settings file: Loop is on
  UserSettings::setLoop(true);
  UserSettings::readSettings();
  QVERIFY(UserSettings::loop());
}

void TestTransportBar::loopSettingDefaultsToOnInAnOlderFile()
{
  // a settings file with other values but no "loop" value: Loop is on
  {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       Constants::organizationName(), Constants::applicationName());
    settings.setValue(group + QStringLiteral("skeleton"), false);
    settings.sync();
  }
  UserSettings::setLoop(false);
  UserSettings::readSettings();
  QVERIFY(UserSettings::loop());
}

void TestTransportBar::loopSettingIsRemembered()
{
  for (bool on : {false, true, false})
  {
    UserSettings::setLoop(on);
    UserSettings::writeSettings();
    UserSettings::setLoop(!on);
    UserSettings::readSettings();
    QCOMPARE(UserSettings::loop(), on);
  }
}

QTEST_MAIN(TestTransportBar)
#include "tst_transportbar.moc"
