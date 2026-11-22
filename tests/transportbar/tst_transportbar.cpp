/*
 * PinglyMotion - test of the transport buttons under the timeline
 *
 * docs/UX_DECISIONS.md section 1, build order steps 1 and 2: buttons Start,
 * Stop, Replay from start, Frame back and Frame forward, and a Loop switch
 * that is on by default and remembered between sessions. Start can be clicked
 * only while stopped, Stop only while playing, Replay from start always.
 * Frame back and Frame forward go one frame and stop at the first and the
 * last frame: Frame back cannot be clicked on the first frame, Frame forward
 * not on the last.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCheckBox>
#include <QLayout>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "constants.h"
#include "playback.h"
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

    void stepButtonTexts();
    void stepButtonOrder();
    void stepButtonsAtFirstMiddleAndLast();
    void stepButtonsWithOneFrame();
    void stepButtonsDoNotDependOnPlaying();
    void stepClicksAreReported();
    void stepGoesOneFrame();
    void stepStopsAtTheEnds();
    void stepThroughAWholeAnimation();
    void buttonsDoNotTakeTheKeyboard();

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

void TestTransportBar::stepButtonTexts()
{
  TransportBar bar;
  QCOMPARE(bar.backButton()->text(), QStringLiteral("Frame back"));
  QCOMPARE(bar.forwardButton()->text(), QStringLiteral("Frame forward"));
  QCOMPARE(bar.backButton()->toolTip(), QStringLiteral("Go one frame back (Left arrow key)"));
  QCOMPARE(bar.forwardButton()->toolTip(), QStringLiteral("Go one frame forward (Right arrow key)"));
  QVERIFY(!bar.backButton()->isCheckable());
  QVERIFY(!bar.forwardButton()->isCheckable());
}

void TestTransportBar::stepButtonOrder()
{
  // next to the other transport buttons: Start, Stop, Replay from start,
  // Frame back, Frame forward, then the Loop switch
  TransportBar bar;
  QLayout* layout = bar.layout();
  QVERIFY(layout);
  QCOMPARE(layout->count(), 6);
  QCOMPARE(layout->itemAt(0)->widget(), bar.startButton());
  QCOMPARE(layout->itemAt(1)->widget(), bar.stopButton());
  QCOMPARE(layout->itemAt(2)->widget(), bar.replayButton());
  QCOMPARE(layout->itemAt(3)->widget(), bar.backButton());
  QCOMPARE(layout->itemAt(4)->widget(), bar.forwardButton());
  QCOMPARE(layout->itemAt(5)->widget(), bar.loopCheckBox());
}

void TestTransportBar::stepButtonsAtFirstMiddleAndLast()
{
  TransportBar bar;
  // the example wave has 61 frames (0 to 60 inside the program)
  bar.setFrame(0, 61);
  QVERIFY(!bar.backButton()->isEnabled());
  QVERIFY(bar.forwardButton()->isEnabled());

  bar.setFrame(1, 61);
  QVERIFY(bar.backButton()->isEnabled());
  QVERIFY(bar.forwardButton()->isEnabled());

  bar.setFrame(30, 61);
  QVERIFY(bar.backButton()->isEnabled());
  QVERIFY(bar.forwardButton()->isEnabled());

  bar.setFrame(59, 61);
  QVERIFY(bar.backButton()->isEnabled());
  QVERIFY(bar.forwardButton()->isEnabled());

  bar.setFrame(60, 61);
  QVERIFY(bar.backButton()->isEnabled());
  QVERIFY(!bar.forwardButton()->isEnabled());
}

void TestTransportBar::stepButtonsWithOneFrame()
{
  // a new bar, and an animation of one frame: nowhere to step
  TransportBar bar;
  QVERIFY(!bar.backButton()->isEnabled());
  QVERIFY(!bar.forwardButton()->isEnabled());
  bar.setFrame(0, 1);
  QVERIFY(!bar.backButton()->isEnabled());
  QVERIFY(!bar.forwardButton()->isEnabled());
}

void TestTransportBar::stepButtonsDoNotDependOnPlaying()
{
  // while playing they can still be clicked (the window stops, then steps)
  TransportBar bar;
  bar.setFrame(5, 10);
  for (PlayState state : {PLAYSTATE_LOOPING, PLAYSTATE_PLAYING, PLAYSTATE_STOPPED})
  {
    bar.setPlaystate(state);
    QVERIFY(bar.backButton()->isEnabled());
    QVERIFY(bar.forwardButton()->isEnabled());
  }
}

void TestTransportBar::stepClicksAreReported()
{
  TransportBar bar;
  QSignalSpy back(&bar, &TransportBar::backClicked);
  QSignalSpy forward(&bar, &TransportBar::forwardClicked);
  bar.setFrame(5, 10);

  bar.forwardButton()->click();
  QCOMPARE(forward.count(), 1);
  QCOMPARE(back.count(), 0);
  bar.backButton()->click();
  QCOMPARE(back.count(), 1);
  QCOMPARE(forward.count(), 1);

  // on the first frame Frame back sends nothing, on the last Frame forward
  bar.setFrame(0, 10);
  bar.backButton()->click();
  QCOMPARE(back.count(), 1);
  bar.setFrame(9, 10);
  bar.forwardButton()->click();
  QCOMPARE(forward.count(), 1);
}

void TestTransportBar::stepGoesOneFrame()
{
  QCOMPARE(Playback::stepFrame(5, 10, 1), 6);
  QCOMPARE(Playback::stepFrame(5, 10, -1), 4);
  QCOMPARE(Playback::stepFrame(0, 10, 1), 1);
  QCOMPARE(Playback::stepFrame(9, 10, -1), 8);
}

void TestTransportBar::stepStopsAtTheEnds()
{
  // no wrap-around: the first frame stays the first, the last stays the last
  QCOMPARE(Playback::stepFrame(0, 10, -1), 0);
  QCOMPARE(Playback::stepFrame(9, 10, 1), 9);
  QCOMPARE(Playback::stepFrame(0, 1, 1), 0);
  QCOMPARE(Playback::stepFrame(0, 1, -1), 0);
}

void TestTransportBar::stepThroughAWholeAnimation()
{
  // forward from the first frame to the last, one frame at a time, and back
  const int total = 61;
  int frame = 0;
  for (int i = 1; i < total; ++i)
  {
    QVERIFY(Playback::stepButtons(frame, total).forward);
    frame = Playback::stepFrame(frame, total, 1);
    QCOMPARE(frame, i);
  }
  QVERIFY(!Playback::stepButtons(frame, total).forward);
  QCOMPARE(Playback::stepFrame(frame, total, 1), total - 1);

  for (int i = total - 2; i >= 0; --i)
  {
    QVERIFY(Playback::stepButtons(frame, total).back);
    frame = Playback::stepFrame(frame, total, -1);
    QCOMPARE(frame, i);
  }
  QVERIFY(!Playback::stepButtons(frame, total).back);
  QCOMPARE(Playback::stepFrame(frame, total, -1), 0);
}

void TestTransportBar::buttonsDoNotTakeTheKeyboard()
{
  // a click must not take the keyboard focus, so the left / right arrow keys
  // still step the frame afterwards
  TransportBar bar;
  for (QPushButton* button : {bar.startButton(), bar.stopButton(), bar.replayButton(),
                              bar.backButton(), bar.forwardButton()})
    QCOMPARE(button->focusPolicy(), Qt::TabFocus);
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
