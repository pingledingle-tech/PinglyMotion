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

// for the look of the buttons (transportlook.h)
#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QPainterPath>
#include <QVBoxLayout>

#include "constants.h"
#include "lucideicons.h"
#include "theme.h"
#include "playback.h"
#include "transportbar.h"
#include "transportlook.h"
#include "usersettings.h"

// the widgets of a layout and of the layouts inside it, in reading order
static QList<QWidget*> widgetsInOrder(QLayout* layout)
{
  QList<QWidget*> widgets;
  for (int i = 0; i < layout->count(); ++i)
  {
    QLayoutItem* item = layout->itemAt(i);
    if (item->widget())
      widgets << item->widget();
    else if (item->layout())
      widgets << widgetsInOrder(item->layout());
  }
  return widgets;
}

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

    void lookTokensMatchTheFile();
    void lookButtonKinds();
    void lookColoursAreTokens();
    void lookDrawnInTheTokenColours();
    void lookLucidePaths();
    void lookIconsInTextColour();
    void lookFollowsTheTheme();
    void lookButtonsAddedLaterGetTheLook();
    void lookDisabledSaysWhy();
    void lookShapeAndSize();
    void lookLeavesOtherButtonsAlone();
    void lookFocusRingAndKeyboard();
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
  // (in two parts since UX phase 0c, so they can break into two lines)
  TransportBar bar;
  QVERIFY(bar.layout());
  const QList<QWidget*> order = widgetsInOrder(bar.layout());
  QCOMPARE(order, QList<QWidget*>({bar.startButton(), bar.stopButton(), bar.replayButton(), bar.backButton(),
                                   bar.forwardButton(), bar.loopCheckBox()}));
  // on one line unless asked otherwise, in that order from left to right
  QVERIFY(!bar.isWrapped());
  bar.resize(bar.sizeHint());
  bar.layout()->activate();
  for (int i = 1; i < order.size(); ++i)
    QVERIFY(order.at(i)->x() > order.at(i - 1)->x());
  QCOMPARE(bar.oneLineWidth(), bar.minimumSizeHint().width());
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

// the look in the design system (docs/DESIGN_SYSTEM.md, transportlook.h)

// the tokens in docs/design_tokens.json
static QJsonObject tokenFile()
{
  QFile file(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/design_tokens.json"));
  if (!file.open(QIODevice::ReadOnly))
    return QJsonObject();
  return QJsonDocument::fromJson(file.readAll()).object();
}

void TestTransportBar::lookTokensMatchTheFile()
{
  const QJsonObject file = tokenFile();
  QVERIFY(!file.isEmpty());
  for (const char* themeName : {"dark", "light"})
  {
    const QJsonObject theme = file.value(QStringLiteral("themes")).toObject().value(QLatin1String(themeName)).toObject();
    QCOMPARE(theme.size(), Theme::tokens().size());
    for (Theme::Token token : Theme::tokens())
    {
      const QString name = QLatin1String(Theme::tokenName(token));
      QVERIFY2(theme.contains(name), qPrintable(name));
      QCOMPARE(Theme::color(token, QLatin1String(themeName) == QLatin1String("dark")).name(),
               theme.value(name).toString().toLower());
    }
  }
  QCOMPARE(file.value("radius").toObject().value("control").toInt(), Theme::Size::radiusControl);
  QCOMPARE(file.value("border_width").toInt(), Theme::Size::borderWidth);
  QCOMPARE(file.value("focus_ring").toObject().value("width").toInt(), Theme::Size::focusRingWidth);
  QCOMPARE(file.value("focus_ring").toObject().value("offset").toInt(), Theme::Size::focusRingOffset);
  QCOMPARE(file.value("control_height").toObject().value("compact").toInt(), Theme::Size::controlCompact);
  QCOMPARE(file.value("spacing").toObject().value("s").toInt(), Theme::Size::spacingS);
  QCOMPARE(file.value("icon").toObject().value("grid").toInt(), Theme::Size::iconGrid);
  QCOMPARE(file.value("icon").toObject().value("stroke").toInt(), Theme::Size::iconStroke);
  QCOMPARE(file.value("icon").toObject().value("sizes").toArray().first().toInt(), Theme::Size::iconSmall);
  QCOMPARE(file.value("icon").toObject().value("set").toString(), QStringLiteral("Lucide"));
  QCOMPARE(file.value("type").toObject().value("label").toObject().value("weight").toInt(), Theme::Size::labelWeight);
}

static double share(const QIcon& icon, const QColor& colour, QIcon::Mode mode = QIcon::Normal);

// the colour of the pixel in the middle of the left edge of the button,
// inside the border (the fill)
static QColor fillPixel(QPushButton* button)
{
  const QImage image = button->grab().toImage();
  const qreal ratio = image.devicePixelRatio();
  const int margin = TransportLook::ringMargin();
  return image.pixelColor(int((margin + 3) * ratio), int(button->height() / 2 * ratio));
}

void TestTransportBar::lookButtonKinds()
{
  TransportBar bar;
  QCOMPARE(TransportLook::kind(bar.startButton()), TransportLook::Kind::Main);
  QCOMPARE(TransportLook::kind(bar.stopButton()), TransportLook::Kind::Danger);
  QCOMPARE(TransportLook::kind(bar.replayButton()), TransportLook::Kind::Normal);
  // no style sheet anywhere: the old one is gone, and a style sheet would
  // stop the buttons from following the text size
  QVERIFY(bar.styleSheet().isEmpty());
  QVERIFY(bar.loopCheckBox()->styleSheet().isEmpty());
  for (QPushButton* button : {bar.startButton(), bar.stopButton(), bar.replayButton()})
    QVERIFY(button->styleSheet().isEmpty());
  // the object names the look uses are still the buttons' names
  QCOMPARE(bar.startButton()->objectName(), QStringLiteral("transportStartButton"));
  QCOMPARE(bar.stopButton()->objectName(), QStringLiteral("transportStopButton"));
  QCOMPARE(bar.replayButton()->objectName(), QStringLiteral("transportReplayButton"));
}

void TestTransportBar::lookColoursAreTokens()
{
  using Theme::Token;
  using TransportLook::Kind;
  using TransportLook::State;
  for (bool dark : {true, false})
  {
    auto c = [dark](Token t) { return Theme::color(t, dark); };
    const QColor text = c(Token::Text);
    const QColor onAccent = c(Token::OnAccent);
    struct Expect
    {
      Kind kind;
      QColor fill, border, colour, hover, pressed;
    };
    const Expect expects[] = {
      {Kind::Main, c(Token::Accent), c(Token::Accent), onAccent, Theme::mix(c(Token::Accent), onAccent, 0.12),
       Theme::mix(c(Token::Accent), text, 0.08)},
      {Kind::Danger, c(Token::DangerFill), c(Token::DangerFill), onAccent,
       Theme::mix(c(Token::DangerFill), onAccent, 0.12), Theme::mix(c(Token::DangerFill), text, 0.08)},
      {Kind::Normal, c(Token::SurfaceRaised), c(Token::Border), text, Theme::mix(c(Token::SurfaceRaised), text, 0.08),
       Theme::mix(c(Token::SurfaceRaised), text, 0.16)},
    };
    for (const Expect& e : expects)
    {
      const TransportLook::Colours normal = TransportLook::colours(e.kind, State::Normal, dark);
      QCOMPARE(normal.fill, e.fill);
      QCOMPARE(normal.border, e.border);
      QCOMPARE(normal.text, e.colour);
      QCOMPARE(TransportLook::colours(e.kind, State::Hover, dark).fill, e.hover);
      QCOMPARE(TransportLook::colours(e.kind, State::Pressed, dark).fill, e.pressed);
      QCOMPARE(TransportLook::colours(e.kind, State::Hover, dark).text, e.colour);
      // cannot be clicked: text_disabled on the normal fill, border at 50 percent
      const TransportLook::Colours off = TransportLook::colours(e.kind, State::Disabled, dark);
      QCOMPARE(off.fill, c(Token::SurfaceRaised));
      QCOMPARE(off.text, c(Token::TextDisabled));
      QCOMPARE(off.border.rgb(), c(Token::Border).rgb());
      QCOMPARE(off.border.alpha(), 128);
    }
  }
  // the mix of the "States" table: 8 percent white into black
  QCOMPARE(Theme::mix(Qt::black, Qt::white, 0.08), QColor(20, 20, 20));
}

void TestTransportBar::lookDrawnInTheTokenColours()
{
  using Theme::Token;
  const bool before = Theme::isDark();
  for (bool dark : {true, false})
  {
    Theme::apply(dark);
    QCoreApplication::processEvents();
    TransportBar bar;
    // away from the mouse, so no button shows its hover colour
    QCursor::setPos(0, 0);
    bar.move(400, 400);
    bar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bar));
    QCOMPARE(TransportLook::state(bar.startButton()), TransportLook::State::Normal);
    // stopped: Start filled with accent, Replay with surface_raised, Stop
    // pale; playing: Stop filled with danger_fill
    QCOMPARE(fillPixel(bar.startButton()).name(), Theme::color(Token::Accent, dark).name());
    QCOMPARE(fillPixel(bar.replayButton()).name(), Theme::color(Token::SurfaceRaised, dark).name());
    QCOMPARE(fillPixel(bar.stopButton()).name(), Theme::color(Token::SurfaceRaised, dark).name());
    bar.setPlaystate(PLAYSTATE_LOOPING);
    QCOMPARE(fillPixel(bar.stopButton()).name(), Theme::color(Token::DangerFill, dark).name());
    QCOMPARE(fillPixel(bar.startButton()).name(), Theme::color(Token::SurfaceRaised, dark).name());
    // the replay icon in the text colour of this theme
    QVERIFY(share(bar.replayButton()->icon(), Theme::color(Token::Text, dark)) > 0.05);
  }
  Theme::apply(before);
  QCoreApplication::processEvents();
}

void TestTransportBar::lookLucidePaths()
{
  QCOMPARE(QString::fromLatin1(LucideIcons::version()), QStringLiteral("1.77.0"));
  // every icon the program has (the list comes from src/lucideicons.cpp, not from a copy here, so
  // two jobs that add an icon no longer conflict): a unique name, an outline, and a drawing
  const QStringList iconNames = LucideIcons::names();
  QVERIFY(!iconNames.isEmpty());
  QStringList unique = iconNames;
  unique.removeDuplicates();
  QCOMPARE(unique.size(), iconNames.size());
  for (const QString& iconName : iconNames)
  {
    QVERIFY2(!iconName.isEmpty(), "an icon without a name");
    QVERIFY2(!LucideIcons::path(iconName).isEmpty(), qPrintable(iconName));
    const QIcon drawn = LucideIcons::icon(iconName, Qt::white, Qt::gray);
    QVERIFY2(!drawn.isNull(), qPrintable(iconName));
    QVERIFY2(share(drawn, Qt::white) > 0.0, qPrintable(iconName));
  }
  // the icons the transport buttons and the tests below use
  for (const char* used : {"play", "square", "rotate-ccw", "user-plus", "user-minus", "search", "library-big"})
    QVERIFY2(iconNames.contains(QLatin1String(used)), used);
  // two figures (docs/UX_DECISIONS.md 6m): the head is the circle round 9,7 with radius 4, the
  // shoulders reach from x 2 to 16 at the bottom; user-plus has its cross out to x 22
  const QRectF plus = LucideIcons::path(QStringLiteral("user-plus")).boundingRect();
  QVERIFY(qAbs(plus.left() - 2) < 0.05 && qAbs(plus.top() - 3) < 0.05 && qAbs(plus.right() - 22) < 0.05 && qAbs(plus.bottom() - 21) < 0.05);
  const QRectF minus = LucideIcons::path(QStringLiteral("user-minus")).boundingRect();
  QVERIFY(qAbs(minus.left() - 2) < 0.05 && qAbs(minus.top() - 3) < 0.05 && qAbs(minus.right() - 22) < 0.05 && qAbs(minus.bottom() - 21) < 0.05);
  QVERIFY(!LucideIcons::path(QStringLiteral("user-plus")).contains(QPointF(19, 3)));
  QVERIFY(LucideIcons::path(QStringLiteral("no-such-icon")).isEmpty());
  // the Library panel's icons (from the approved mockup): the search lens is the circle round
  // 11,11 with radius 8 and the handle to 21,21; the book stands from x 3
  const QRectF lens = LucideIcons::path(QStringLiteral("search")).boundingRect();
  QVERIFY(qAbs(lens.left() - 3) < 0.05 && qAbs(lens.top() - 3) < 0.05 && qAbs(lens.right() - 21) < 0.05 && qAbs(lens.bottom() - 21) < 0.05);
  QVERIFY(qAbs(LucideIcons::path(QStringLiteral("library-big")).boundingRect().left() - 3) < 0.05);
  QVERIFY(LucideIcons::icon(QStringLiteral("no-such-icon"), Qt::white, Qt::gray).isNull());

  // the rounded square: x 3, y 3, 18 by 18
  const QRectF square = LucideIcons::path(QStringLiteral("square")).boundingRect();
  QVERIFY2(qAbs(square.left() - 3) < 0.01 && qAbs(square.top() - 3) < 0.01 && qAbs(square.width() - 18) < 0.01 &&
             qAbs(square.height() - 18) < 0.01,
           qPrintable(QStringLiteral("%1 %2 %3 %4").arg(square.left()).arg(square.top()).arg(square.width()).arg(square.height())));

  // the play triangle points right: its right end is the middle row
  const QPainterPath play = LucideIcons::path(QStringLiteral("play"));
  const QRectF playBox = play.boundingRect();
  QVERIFY(playBox.left() > 4.9 && playBox.left() < 5.1);
  QVERIFY(playBox.right() > 20.5 && playBox.right() < 21.5);
  QVERIFY(playBox.top() > 2.5 && playBox.top() < 3.5);
  QVERIFY(playBox.bottom() > 20.5 && playBox.bottom() < 21.5);
  QVERIFY(play.contains(QPointF(10, 12)));
  QVERIFY(!play.contains(QPointF(20, 5)));

  // the arc of rotate-ccw: a circle of radius 9 round the middle, from the
  // left round the bottom and the right to the top
  const QPainterPath arc = LucideIcons::fromSvgPath(QStringLiteral("M3 12a9 9 0 1 0 9-9"));
  QCOMPARE(arc.currentPosition(), QPointF(12, 3));
  const QRectF arcBox = arc.boundingRect();
  QVERIFY(qAbs(arcBox.left() - 3) < 0.05 && qAbs(arcBox.right() - 21) < 0.05 && qAbs(arcBox.bottom() - 21) < 0.05);
  QVERIFY(arc.pointAtPercent(0.5).y() > 12);   // goes round the bottom first
  // relative and absolute commands, H and V
  const QPainterPath corner = LucideIcons::fromSvgPath(QStringLiteral("M3 3v5h5"));
  QCOMPARE(corner.currentPosition(), QPointF(8, 8));
  QCOMPARE(corner.elementCount(), 3);
  const QRectF ccw = LucideIcons::path(QStringLiteral("rotate-ccw")).boundingRect();
  QVERIFY(qAbs(ccw.left() - 3) < 0.05 && qAbs(ccw.top() - 3) < 0.05 && qAbs(ccw.right() - 21) < 0.05);
}

// share of the pixels of a 32 x 32 picture that are close to this colour
static double share(const QIcon& icon, const QColor& colour, QIcon::Mode mode)
{
  const QImage image = icon.pixmap(32, 32, mode).toImage().convertToFormat(QImage::Format_ARGB32);
  int count = 0;
  for (int y = 0; y < image.height(); y++)
    for (int x = 0; x < image.width(); x++)
    {
      const QColor c = image.pixelColor(x, y);
      if (c.alpha() > 200 && qAbs(c.red() - colour.red()) < 24 && qAbs(c.green() - colour.green()) < 24 &&
          qAbs(c.blue() - colour.blue()) < 24)
        count++;
    }
  return double(count) / (image.width() * image.height());
}

void TestTransportBar::lookIconsInTextColour()
{
  using Theme::Token;
  TransportBar bar;
  QCOMPARE(TransportLook::iconName(bar.startButton()), QStringLiteral("play"));
  QCOMPARE(TransportLook::iconName(bar.stopButton()), QStringLiteral("square"));
  QCOMPARE(TransportLook::iconName(bar.replayButton()), QStringLiteral("rotate-ccw"));
  for (bool dark : {true, false})
  {
    for (QPushButton* button : {bar.startButton(), bar.stopButton(), bar.replayButton()})
    {
      const QIcon icon = TransportLook::icon(button, dark);
      const QColor colour = TransportLook::textColour(TransportLook::kind(button), dark);
      const QColor disabled = Theme::color(Token::TextDisabled, dark);
      // an outline: some pixels, not a filled shape, and an empty middle for the arrow
      const double line = share(icon, colour);
      QVERIFY2(line > 0.05 && line < 0.45, qPrintable(button->objectName() + QString::number(line)));
      QVERIFY(share(icon, disabled, QIcon::Disabled) > 0.05);
      QCOMPARE(share(icon, colour, QIcon::Disabled), 0.0);
    }
    QCOMPARE(TransportLook::textColour(TransportLook::Kind::Main, dark), Theme::color(Token::OnAccent, dark));
    QCOMPARE(TransportLook::textColour(TransportLook::Kind::Danger, dark), Theme::color(Token::OnAccent, dark));
    QCOMPARE(TransportLook::textColour(TransportLook::Kind::Normal, dark), Theme::color(Token::Text, dark));
  }
  // the outline of the square has an empty middle (not a filled block)
  const QImage square = TransportLook::icon(bar.stopButton(), true).pixmap(32, 32).toImage();
  QCOMPARE(square.pixelColor(16, 16).alpha(), 0);
  for (QPushButton* button : {bar.startButton(), bar.stopButton(), bar.replayButton()})
    QCOMPARE(button->iconSize(), QSize(16, 16));
}

void TestTransportBar::lookFollowsTheTheme()
{
  // as View > Dark theme does: Theme::apply sets the palette of the program
  TransportBar bar;
  const bool before = Theme::isDark();
  for (bool dark : {false, true, false})
  {
    Theme::apply(dark);
    QCoreApplication::processEvents();
    const QColor text = Theme::color(Theme::Token::Text, dark);
    QVERIFY(share(bar.replayButton()->icon(), text) > 0.05);
    QCOMPARE(share(bar.replayButton()->icon(), Theme::color(Theme::Token::Text, !dark)), 0.0);
  }
  Theme::apply(before);
  QCoreApplication::processEvents();
}

void TestTransportBar::lookButtonsAddedLaterGetTheLook()
{
  // for example the frame step buttons next to the first three
  TransportBar bar;
  QPushButton* later = new QPushButton(QStringLiteral("Later"), &bar);
  QCoreApplication::processEvents();
  QCOMPARE(TransportLook::kind(later), TransportLook::Kind::Normal);
  QVERIFY(later->icon().isNull());
  QCOMPARE(later->font().weight(), QFont::DemiBold);
  QCOMPARE(later->minimumSize(), TransportLook::minimumSize(later));
}

void TestTransportBar::lookDisabledSaysWhy()
{
  TransportBar bar;
  // stopped: Stop cannot be clicked and says why
  QCOMPARE(bar.stopButton()->toolTip(), QStringLiteral("It is not playing. Click Start to play it."));
  QCOMPARE(bar.startButton()->toolTip(), QStringLiteral("Play the animation from where the playhead is"));
  bar.setPlaystate(PLAYSTATE_LOOPING);
  QCOMPARE(bar.startButton()->toolTip(), QStringLiteral("It is already playing. Click Stop to stop it."));
  QCOMPARE(bar.stopButton()->toolTip(), QStringLiteral("Stop playing; the playhead stays on this frame"));
  bar.setPlaystate(PLAYSTATE_STOPPED);
  QCOMPARE(bar.startButton()->toolTip(), QStringLiteral("Play the animation from where the playhead is"));
  // the same words in the status line and for screen readers
  for (QPushButton* button : {bar.startButton(), bar.stopButton(), bar.replayButton()})
  {
    QCOMPARE(button->statusTip(), button->toolTip());
    QCOMPARE(button->accessibleDescription(), button->toolTip());
  }
}

void TestTransportBar::lookShapeAndSize()
{
  TransportBar bar;
  bar.show();
  QVERIFY(QTest::qWaitForWindowExposed(&bar));
  const int margin = TransportLook::ringMargin();
  QCOMPARE(margin, 4);
  for (QPushButton* button : {bar.startButton(), bar.stopButton(), bar.replayButton()})
  {
    // the button inside the ring margin is at least control_height.compact high
    QVERIFY2(button->height() - 2 * margin >= 24, qPrintable(QString::number(button->height())));
    QVERIFY(button->width() >= button->fontMetrics().horizontalAdvance(button->text()) + 16 + 8 + 2 * 8 + 2 * margin);
    QCOMPARE(button->minimumSize(), TransportLook::minimumSize(button));
  }
  // twice the text size: the buttons grow instead of cutting the text off
  const int smallHeight = bar.startButton()->minimumHeight();
  QFont font = bar.font();
  font.setPointSizeF(font.pointSizeF() * 2.0);
  bar.setFont(font);
  QCoreApplication::processEvents();
  QCOMPARE(bar.startButton()->font().weight(), QFont::DemiBold);
  QCOMPARE(bar.startButton()->font().pointSizeF(), font.pointSizeF());
  for (QPushButton* button : {bar.startButton(), bar.stopButton(), bar.replayButton()})
  {
    QCOMPARE(button->minimumSize(), TransportLook::minimumSize(button));
    QVERIFY(button->minimumWidth() >= button->fontMetrics().horizontalAdvance(button->text()) + 16 + 8 + 2 * 8 + 2 * margin);
    QVERIFY(button->minimumHeight() >= button->fontMetrics().height() + 2 * margin);
  }
  QVERIFY(bar.startButton()->minimumHeight() > smallHeight);
}

void TestTransportBar::lookFocusRingAndKeyboard()
{
  using Theme::Token;
  QWidget window;
  QVBoxLayout* layout = new QVBoxLayout(&window);
  TransportBar* bar = new TransportBar(&window);
  layout->addWidget(bar);
  window.show();
  window.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&window));

  // all of the bar can be reached with Tab, in screen order
  QCOMPARE(bar->startButton()->focusPolicy() & Qt::TabFocus, Qt::TabFocus);
  QCOMPARE(bar->replayButton()->focusPolicy() & Qt::TabFocus, Qt::TabFocus);
  QCOMPARE(bar->startButton()->nextInFocusChain(), static_cast<QWidget*>(bar->stopButton()));
  QCOMPARE(bar->stopButton()->nextInFocusChain(), static_cast<QWidget*>(bar->replayButton()));
  // after Replay come the bar's other controls (such as frame step buttons)
  // and then the Loop switch
  QWidget* next = bar->replayButton()->nextInFocusChain();
  for (int i = 0; i < 8 && next != bar->loopCheckBox(); i++)
  {
    QCOMPARE(next->parentWidget(), static_cast<QWidget*>(bar));
    next = next->nextInFocusChain();
  }
  QCOMPARE(next, static_cast<QWidget*>(bar->loopCheckBox()));

  // focus from the keyboard shows the ring, in focus_ring, round the button
  bar->replayButton()->setFocus(Qt::MouseFocusReason);
  bar->startButton()->setFocus(Qt::TabFocusReason);
  QVERIFY(bar->startButton()->hasFocus());
  QVERIFY(TransportLook::showsFocusRing(bar->startButton()));
  const QImage picture = bar->startButton()->grab().toImage();
  const QColor ring = Theme::color(Token::FocusRing);
  const QColor edge = picture.pixelColor(picture.width() / 2, 1);
  QVERIFY2(qAbs(edge.red() - ring.red()) < 30 && qAbs(edge.green() - ring.green()) < 30 && qAbs(edge.blue() - ring.blue()) < 30,
           qPrintable(edge.name()));

  // Space on Start: it plays (the window would call setPlaystate), and the
  // focus goes on to Stop with the ring, so Space again stops it
  QSignalSpy start(bar, &TransportBar::startClicked);
  QSignalSpy stop(bar, &TransportBar::stopClicked);
  QTest::keyClick(bar->startButton(), Qt::Key_Space);
  QCOMPARE(start.count(), 1);
  bar->setPlaystate(PLAYSTATE_LOOPING);
  QVERIFY(bar->stopButton()->hasFocus());
  QVERIFY(TransportLook::showsFocusRing(bar->stopButton()));
  QTest::keyClick(bar->stopButton(), Qt::Key_Space);
  QCOMPARE(stop.count(), 1);
  bar->setPlaystate(PLAYSTATE_STOPPED);
  QVERIFY(bar->startButton()->hasFocus());
  QVERIFY(TransportLook::showsFocusRing(bar->startButton()));

  // a click with the mouse gives focus without the ring, also after the turn
  bar->replayButton()->setFocus(Qt::MouseFocusReason);
  QVERIFY(!TransportLook::showsFocusRing(bar->replayButton()));
  bar->startButton()->setFocus(Qt::MouseFocusReason);
  QVERIFY(!TransportLook::showsFocusRing(bar->startButton()));
  bar->setPlaystate(PLAYSTATE_PLAYING);
  QVERIFY(bar->stopButton()->hasFocus());
  QVERIFY(!TransportLook::showsFocusRing(bar->stopButton()));
  bar->setPlaystate(PLAYSTATE_STOPPED);
}

void TestTransportBar::lookKeepsTextsAndOrder()
{
  // the look changes nothing the project owner approved: texts, tips, order
  TransportBar bar;
  QCOMPARE(bar.startButton()->text(), QStringLiteral("Start"));
  QCOMPARE(bar.stopButton()->text(), QStringLiteral("Stop"));
  QCOMPARE(bar.replayButton()->text(), QStringLiteral("Replay from start"));
  QCOMPARE(bar.replayButton()->toolTip(), QStringLiteral("Go back to the first frame and play"));
  QVERIFY(bar.layout());
  const QList<QWidget*> order = widgetsInOrder(bar.layout());
  QCOMPARE(order.value(0), bar.startButton());
  QCOMPARE(order.value(1), bar.stopButton());
  QCOMPARE(order.value(2), bar.replayButton());
}

// the look is for the bar's own buttons only: a button elsewhere in the program keeps its size and is
// drawn by its own style, although the look also listens to the whole application (job B3 found this)
void TestTransportBar::lookLeavesOtherButtonsAlone()
{
  QWidget window;
  QVBoxLayout* layout = new QVBoxLayout(&window);
  TransportBar* bar = new TransportBar(&window);
  layout->addWidget(bar);
  QPushButton* other = new QPushButton("Watch folder...", &window);
  other->setMinimumHeight(40);
  layout->addWidget(other);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QCoreApplication::processEvents();
  QCOMPARE(other->minimumHeight(), 40);
  QFont font = window.font();
  font.setPointSizeF(font.pointSizeF() * 1.5);
  window.setFont(font);
  QCoreApplication::processEvents();
  QCOMPARE(other->minimumHeight(), 40);
  QVERIFY(other->font().weight() != QFont::DemiBold);
  // the bar's own buttons still get the look
  QCOMPARE(bar->startButton()->minimumSize(), TransportLook::minimumSize(bar->startButton()));
}

QTEST_MAIN(TestTransportBar)
#include "tst_transportbar.moc"
