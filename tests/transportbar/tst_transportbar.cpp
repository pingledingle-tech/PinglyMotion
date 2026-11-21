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

// for the look of the buttons (transportlook.h)
#include <QApplication>
#include <QGuiApplication>
#include <QImage>
#include <QLayout>
#include <QPalette>

#include "constants.h"
#include "transportbar.h"
#include "transportlook.h"
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

    void lookStartGreenStopRed();
    void lookIconsOnAllThree();
    void lookIconsAreDrawn();
    void lookReplayIconFollowsTheTheme();
    void lookKeepsTextsAndOrder();

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

// the modern look: green Start, red Stop, a round arrow on Replay
void TestTransportBar::lookStartGreenStopRed()
{
  TransportBar bar;
  QCOMPARE(TransportLook::startColour(), QColor(0x2e, 0x9d, 0x4f));
  QCOMPARE(TransportLook::stopColour(), QColor(0xd6, 0x45, 0x45));
  const QString sheet = bar.startButton()->styleSheet();
  QCOMPARE(sheet, TransportLook::styleSheet(QGuiApplication::palette()));
  QCOMPARE(bar.stopButton()->styleSheet(), sheet);
  QCOMPARE(bar.replayButton()->styleSheet(), sheet);
  // the bar itself and the Loop switch keep following the theme
  QVERIFY(bar.styleSheet().isEmpty());
  QVERIFY(bar.loopCheckBox()->styleSheet().isEmpty());
  QVERIFY(sheet.contains(QStringLiteral("QPushButton#transportStartButton { background-color: #2e9d4f;")));
  QVERIFY(sheet.contains(QStringLiteral("QPushButton#transportStopButton { background-color: #d64545;")));
  // a rule for each state: under the mouse, pressed, cannot be clicked
  for (const char* name : {"transportStartButton", "transportStopButton"})
    for (const char* state : {":hover", ":pressed", ":disabled"})
      QVERIFY2(sheet.contains(QStringLiteral("QPushButton#%1%2 {").arg(QLatin1String(name), QLatin1String(state))),
               qPrintable(QStringLiteral("%1%2").arg(QLatin1String(name), QLatin1String(state))));
  // the object names the style sheet uses are still the buttons' names
  QCOMPARE(bar.startButton()->objectName(), QStringLiteral("transportStartButton"));
  QCOMPARE(bar.stopButton()->objectName(), QStringLiteral("transportStopButton"));
}

void TestTransportBar::lookIconsOnAllThree()
{
  TransportBar bar;
  for (QPushButton* button : {bar.startButton(), bar.stopButton(), bar.replayButton()})
  {
    QVERIFY2(!button->icon().isNull(), qPrintable(button->objectName()));
    QCOMPARE(button->iconSize(), QSize(16, 16));
    QVERIFY(!button->icon().pixmap(16, 16).isNull());
    QVERIFY(!button->icon().pixmap(16, 16, QIcon::Disabled).isNull());
  }
}

// share of the pixels of a 32 x 32 picture that are mostly this colour
static double share(const QIcon& icon, const QColor& colour, QIcon::Mode mode = QIcon::Normal)
{
  const QImage image = icon.pixmap(32, 32, mode).toImage().convertToFormat(QImage::Format_ARGB32);
  int count = 0;
  for (int y = 0; y < image.height(); y++)
    for (int x = 0; x < image.width(); x++)
    {
      const QColor c = image.pixelColor(x, y);
      if (c.alpha() > 200 && qAbs(c.red() - colour.red()) < 40 && qAbs(c.green() - colour.green()) < 40 &&
          qAbs(c.blue() - colour.blue()) < 40)
        count++;
    }
  return double(count) / (image.width() * image.height());
}

void TestTransportBar::lookIconsAreDrawn()
{
  // white triangle, white square: each covers a fair part of the picture,
  // the square more than the triangle; the disabled pictures are faded
  const double triangle = share(TransportLook::startIcon(), Qt::white);
  const double square = share(TransportLook::stopIcon(), Qt::white);
  QVERIFY2(triangle > 0.15 && triangle < 0.5, qPrintable(QString::number(triangle)));
  QVERIFY2(square > 0.2 && square < 0.5, qPrintable(QString::number(square)));
  QVERIFY(square > triangle);
  QCOMPARE(share(TransportLook::startIcon(), Qt::white, QIcon::Disabled), 0.0);

  // the round arrow is a thin line: less than the square, more than nothing,
  // and its middle is empty
  const QIcon arrow = TransportLook::replayIcon(Qt::black);
  const double line = share(arrow, Qt::black);
  QVERIFY2(line > 0.05 && line < square, qPrintable(QString::number(line)));
  const QImage image = arrow.pixmap(32, 32).toImage();
  QCOMPARE(image.pixelColor(16, 16).alpha(), 0);
}

void TestTransportBar::lookReplayIconFollowsTheTheme()
{
  // as View > Dark theme does: a new palette for the whole program
  TransportBar bar;
  const QPalette before = QGuiApplication::palette();
  QPalette palette = before;
  palette.setColor(QPalette::ButtonText, QColor(250, 250, 250));
  QApplication::setPalette(palette);
  // the sheets and the icon are made again once Qt has handed out the palette
  QCoreApplication::processEvents();
  QVERIFY(share(bar.replayButton()->icon(), QColor(250, 250, 250)) > 0.05);
  QVERIFY(bar.replayButton()->styleSheet().contains(QStringLiteral("color: #fafafa;")));

  palette.setColor(QPalette::ButtonText, QColor(10, 10, 10));
  QApplication::setPalette(palette);
  QCoreApplication::processEvents();
  QVERIFY(share(bar.replayButton()->icon(), QColor(10, 10, 10)) > 0.05);
  QCOMPARE(share(bar.replayButton()->icon(), QColor(250, 250, 250)), 0.0);
  // the plain buttons get the new text colour, Start and Stop keep theirs
  QCOMPARE(bar.replayButton()->styleSheet(), TransportLook::styleSheet(palette));
  QVERIFY(bar.replayButton()->styleSheet().contains(QStringLiteral("color: #0a0a0a;")));
  QVERIFY(bar.startButton()->styleSheet().contains(QStringLiteral("QPushButton#transportStartButton { background-color: #2e9d4f;")));
  QApplication::setPalette(before);
  QCoreApplication::processEvents();
}

void TestTransportBar::lookKeepsTextsAndOrder()
{
  // the look changes nothing the project owner approved: texts, tips, order
  TransportBar bar;
  QCOMPARE(bar.startButton()->text(), QStringLiteral("Start"));
  QCOMPARE(bar.stopButton()->text(), QStringLiteral("Stop"));
  QCOMPARE(bar.replayButton()->text(), QStringLiteral("Replay from start"));
  QCOMPARE(bar.replayButton()->toolTip(), QStringLiteral("Go back to the first frame and play"));
  QLayout* layout = bar.layout();
  QVERIFY(layout);
  QCOMPARE(layout->itemAt(0)->widget(), bar.startButton());
  QCOMPARE(layout->itemAt(1)->widget(), bar.stopButton());
  QCOMPARE(layout->itemAt(2)->widget(), bar.replayButton());
}

QTEST_MAIN(TestTransportBar)
#include "tst_transportbar.moc"
