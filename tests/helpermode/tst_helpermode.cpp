/*
 * PinglyMotion - test of helper mode (job UX-C, docs/UX_DECISIONS.md, 6y)
 *
 * The Helper switch: its texts, tip, icon and bold label, and its state saved and restored (it
 * is the hints setting). The helper panel shows and hides with it and its close button asks to
 * turn it off. The hint cards live in the panel and the ring stands around the part the card
 * talks about. The ring follows its control and goes into a floating window with it. A lesson
 * step goes on only when its action happens, is skipped when already done, and the lesson ends
 * with "Lesson done"; Stop, Esc, the helper switched off and a closed animation end it too. A
 * lesson that cannot start is greyed with its reason. The panel moves no control and stays inside
 * the window, also a small one. The panel is drawn in the design system in both themes at every
 * text size. The window's wiring is checked in its source.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>
#include <QtTest>

#include "constants.h"
#include "guidancespec.h"
#include "helpermode.h"
#include "helperspec.h"
#include "hintcard.h"
#include "theme.h"
#include "uirender.h"
#include "usersettings.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  // a stand-in window with the controls the cards and the lessons talk about; small enough for
  // the offscreen screen of the checks
  struct Host
  {
    QWidget window;
    QToolBar* bar;
    QWidget* middle;
    QWidget* view;
    QWidget* selected;
    QWidget* setKey;
    QWidget* start;
    QWidget* tiles;
    QWidget* joints;
    QWidget* setup;
    QWidget* library;
    QWidget* resetLayout;
    QWidget* scriptSwitch;
    QWidget* animateSwitch;
    QSet<QString> already;
    HintCards* cards = nullptr;
    HelperMode* helper = nullptr;

    Host()
    {
      window.resize(760, 560);
      auto make = [this](const QString& name, const QRect& r)
      {
        QWidget* w = new QWidget(&window);
        w->setObjectName(name);
        w->setGeometry(r);
        return w;
      };
      bar = new QToolBar(&window);
      bar->setGeometry(0, 0, 760, 36);
      scriptSwitch = make(QStringLiteral("scriptSwitch"), QRect(90, 40, 70, 24));
      animateSwitch = make(QStringLiteral("animateSwitch"), QRect(10, 40, 70, 24));
      library = make(QStringLiteral("library"), QRect(500, 40, 70, 24));
      resetLayout = make(QStringLiteral("resetLayout"), QRect(580, 40, 90, 24));
      setup = make(QStringLiteral("setup"), QRect(680, 40, 70, 24));
      joints = make(QStringLiteral("joints"), QRect(0, 70, 150, 330));
      middle = make(QStringLiteral("middle"), QRect(156, 70, 400, 330));
      view = new QWidget(middle);
      view->setObjectName(QStringLiteral("view"));
      view->setGeometry(0, 0, 400, 330);
      selected = make(QStringLiteral("selected"), QRect(562, 70, 198, 330));
      start = make(QStringLiteral("start"), QRect(10, 410, 70, 28));
      setKey = make(QStringLiteral("setKey"), QRect(600, 410, 70, 28));
      tiles = make(QStringLiteral("tiles"), QRect(100, 450, 400, 50));
      const QHash<QString, QWidget*> targets = {
        {"view", view}, {"selected", selected}, {"setKey", setKey}, {"start", start}, {"tiles", tiles},
        {"joints", joints}, {"setup", setup}, {"library", library}, {"resetLayout", resetLayout},
        {"scriptSwitch", scriptSwitch}, {"animateSwitch", animateSwitch}};
      auto targetFor = [targets](const QString& name) -> QWidget* { return targets.value(name, nullptr); };
      cards = new HintCards(&window, targetFor);
      helper = new HelperMode(&window, cards, targetFor,
                              [this](const QString& event) { return already.contains(event); },
                              [this]() { return middle->geometry(); });
      helper->addToBar(bar, nullptr);
      // as the window does: the switch and the close button set the one setting
      QObject::connect(helper, &HelperMode::switchClicked, helper, [this](bool on) { setOn(on); });
      QObject::connect(helper, &HelperMode::offRequested, helper, [this]() { setOn(false); });
      QObject::connect(cards, &HintCards::hintsOffRequested, helper, [this]() { setOn(false); });
      window.show();
    }

    void setOn(bool on)
    {
      UserSettings::setTipsOn(on);
      cards->setHintsOn(on);
      helper->setOn(on);
    }

    void open(bool open)
    {
      cards->setAnimationOpen(open);
      helper->setAnimationOpen(open);
    }

    QToolButton* switchButton() const { return qobject_cast<QToolButton*>(bar->widgetForAction(helper->switchAction())); }
    HelperPanel* panel() const { return helper->panel(); }
  };
}

class TestHelperMode : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void textsAndLessonsAreExact();
    void switchStateIsSavedAndRestored();
    void switchShowsAndHidesTheHelper();
    void closeButtonTurnsTheHelperOff();
    void hintCardsAreInTheHelper();
    void highlightStandsAroundTheRightControl();
    void highlightFollowsItsControl();
    void lessonStepGoesOnWhenTheActionHappens();
    void lessonSkipsWhatIsAlreadyDone();
    void lessonsNeedingAnAnimationAreGreyedWithTheReason();
    void lessonStopsWithStopEscAndHelperOff();
    void stepSaysWhenItsControlIsNotOnScreen();
    void otherLessonsGoOnWithTheirActions();
    void panelMovesNothingAndStaysInside();
    void cardInThePanelIsNeverCut();
    void panelIsDrawnInTheDesignSystemAtEveryTextSize();
    void windowWiringIsInPlace();

  private:
    QTemporaryDir m_dir;
};

void TestHelperMode::initTestCase()
{
  QVERIFY(m_dir.isValid());
  // never touch the real settings of the person running the tests
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void TestHelperMode::init()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
  UserSettings::readSettings();
  UserSettings::setTipsOn(true);
  UserSettings::setHintCard(0);
  UserSettings::setTextSizePercent(100);
}

void TestHelperMode::textsAndLessonsAreExact()
{
  QCOMPARE(QString::fromLatin1(HelperSpec::switchText), QStringLiteral("Helper"));
  QCOMPARE(QString::fromLatin1(HelperSpec::switchTipOn),
           QStringLiteral("Helper is on: tips and short lessons beside the tool. Click to turn it off."));
  QCOMPARE(QString::fromLatin1(HelperSpec::switchTipOff),
           QStringLiteral("Helper is off: a clean tool. Click to turn on tips and short lessons."));
  QCOMPARE(QString::fromLatin1(HelperSpec::offMessage),
           QStringLiteral("Helper off. The Helper button at the top turns it on again."));
  QCOMPARE(HelperSpec::lessonCount, 3);
  const QStringList titles = {"Make your first move", "Arrange the windows", "Try the Script Lab"};
  const QList<QStringList> events = {{"picked", "turned", "keySet", "played"}, {"libraryShown", "layoutReset"},
                                     {"scriptSide", "animateSide"}};
  const QList<QStringList> targets = {{"view", "selected", "setKey", "start"}, {"library", "resetLayout"},
                                      {"scriptSwitch", "animateSwitch"}};
  for (int i = 0; i < HelperSpec::lessonCount; ++i)
  {
    const HelperSpec::Lesson& lesson = HelperSpec::lessons[i];
    QCOMPARE(QString::fromLatin1(lesson.title), titles.at(i));
    QCOMPARE(lesson.stepCount, int(events.at(i).size()));
    for (int s = 0; s < lesson.stepCount; ++s)
    {
      QCOMPARE(QString::fromLatin1(lesson.steps[s].event), events.at(i).at(s));
      QCOMPARE(QString::fromLatin1(lesson.steps[s].target), targets.at(i).at(s));
      QVERIFY(!QString::fromLatin1(lesson.steps[s].text).isEmpty());
    }
  }
  QVERIFY(HelperSpec::lessons[0].needsAnimation);
  QVERIFY(!HelperSpec::lessons[1].needsAnimation);
  QVERIFY(!HelperSpec::lessons[2].needsAnimation);
}

void TestHelperMode::switchStateIsSavedAndRestored()
{
  // the switch is the hints setting: off is remembered after a restart, and so is on
  UserSettings::setTipsOn(false);
  UserSettings::writeSettings();
  UserSettings::setTipsOn(true);
  UserSettings::readSettings();
  QVERIFY(!UserSettings::tipsOn());
  {
    Host host;
    host.setOn(UserSettings::tipsOn());
    QVERIFY(!host.helper->isOn());
    QVERIFY(!host.helper->switchAction()->isChecked());
    QVERIFY(!host.helper->panelShown());
    // clicking the switch turns it on and the setting with it
    host.switchButton()->click();
    QVERIFY(host.helper->isOn());
    QVERIFY(UserSettings::tipsOn());
    QVERIFY(host.helper->panelShown());
  }
  UserSettings::writeSettings();
  UserSettings::setTipsOn(false);
  UserSettings::readSettings();
  QVERIFY(UserSettings::tipsOn());
  Host host;
  host.setOn(UserSettings::tipsOn());
  QVERIFY(host.helper->switchAction()->isChecked());
  QVERIFY(host.helper->panelShown());
}

void TestHelperMode::switchShowsAndHidesTheHelper()
{
  Host host;
  QAction* action = host.helper->switchAction();
  QCOMPARE(action->objectName(), QStringLiteral("helperSwitchAction"));
  QCOMPARE(action->text(), QStringLiteral("Helper"));
  QVERIFY(action->isCheckable());
  QToolButton* button = host.switchButton();
  QVERIFY(button);
  QCOMPARE(button->objectName(), QStringLiteral("helperSwitchActionButton"));
  QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
  QCOMPARE(button->focusPolicy(), Qt::StrongFocus);
  const QString key = QKeySequence(QStringLiteral("Shift+H")).toString(QKeySequence::NativeText);

  host.setOn(true);
  QVERIFY(host.helper->panelShown());
  QVERIFY(action->isChecked());
  QVERIFY(button->font().bold());
  QCOMPARE(action->toolTip(), QStringLiteral("Helper is on: tips and short lessons beside the tool. Click to turn it off.  (")
                                + key + QLatin1Char(')'));
  QCOMPARE(action->statusTip(), QStringLiteral("Helper is on: tips and short lessons beside the tool. Click to turn it off."));
  QCOMPARE(button->accessibleDescription(), action->statusTip());
  const QImage onIcon = action->icon().pixmap(16, 16).toImage();

  QSignalSpy clicked(host.helper, &HelperMode::switchClicked);
  button->click();
  QCOMPARE(clicked.count(), 1);
  QCOMPARE(clicked.at(0).at(0).toBool(), false);
  QVERIFY(!host.helper->panelShown());
  QVERIFY(!action->isChecked());
  QVERIFY(!button->font().bold());
  QCOMPARE(action->statusTip(), QStringLiteral("Helper is off: a clean tool. Click to turn on tips and short lessons."));
  // a different icon when off (a tick when on, the lightbulb when off): not by colour alone
  QVERIFY(action->icon().pixmap(16, 16).toImage() != onIcon);
  // off: no ring either
  QVERIFY(!host.helper->highlight()->isVisible());
  button->click();
  QVERIFY(host.helper->panelShown());
}

void TestHelperMode::closeButtonTurnsTheHelperOff()
{
  Host host;
  host.setOn(true);
  HelperPanel* panel = host.panel();
  QCOMPARE(panel->titleLabel()->text(), QStringLiteral("Helper"));
  QCOMPARE(panel->closeButton()->toolTip(), QStringLiteral("Turn the helper off"));
  QCOMPARE(panel->closeButton()->accessibleName(), QStringLiteral("Turn the helper off"));
  QVERIFY(!panel->closeButton()->icon().isNull());
  QSignalSpy off(host.helper, &HelperMode::offRequested);
  panel->closeButton()->click();
  QCOMPARE(off.count(), 1);
  QVERIFY(!host.helper->panelShown());
  QVERIFY(!UserSettings::tipsOn());
  QVERIFY(!host.helper->switchAction()->isChecked());
}

void TestHelperMode::hintCardsAreInTheHelper()
{
  Host host;
  QVERIFY(host.cards->embedded());
  QVERIFY(host.panel()->cardHolder()->isAncestorOf(host.cards->card()));
  host.setOn(true);
  // no animation: no card, the lessons only, and no ring
  QVERIFY(!host.cards->cardShown());
  QVERIFY(!host.helper->highlight()->isVisible());
  host.open(true);
  QVERIFY(host.cards->cardShown());
  QVERIFY(host.cards->card()->isVisible());
  QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Pick a joint"));
  // the card does not float by itself any more: it is inside the panel
  QVERIFY(host.panel()->geometry().contains(host.cards->card()->mapTo(&host.window, QPoint(0, 0))));
  QCOMPARE(host.cards->currentTarget(), host.view);
  QCOMPARE(host.helper->highlight()->target(), host.view);
  QVERIFY(host.helper->highlight()->isVisible());
  // Next: the ring goes to the part of the next card that has another part
  host.cards->next();
  host.cards->next();
  QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Turn it"));
  QCOMPARE(host.helper->highlight()->target(), host.selected);
  host.cards->next();
  host.cards->next();
  QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Save the pose"));
  QCOMPARE(host.helper->highlight()->target(), host.setKey);
  // the card's own close: no card, no ring, the lessons stay
  host.cards->close();
  QVERIFY(!host.cards->cardShown());
  QVERIFY(!host.helper->highlight()->isVisible());
  QVERIFY(host.helper->panelShown());
  // "Turn hints off" on the card turns the whole helper off
  host.setOn(false);
  host.setOn(true);
  QVERIFY(host.cards->cardShown());
  host.cards->card()->hintsOffButton()->click();
  QVERIFY(!host.helper->panelShown());
  QVERIFY(!host.cards->cardShown());
  QVERIFY(!UserSettings::tipsOn());
}

void TestHelperMode::highlightStandsAroundTheRightControl()
{
  Host host;
  host.setOn(true);
  host.open(true);
  HelperHighlight* ring = host.helper->highlight();
  const int pad = Theme::Size::focusRingOffset + Theme::Size::focusRingWidth;
  // the view is inside the middle part: its place in the window, with the ring's margin
  QCOMPARE(ring->target(), host.view);
  QCOMPARE(ring->parentWidget(), &host.window);
  QCOMPARE(ring->geometry(), QRect(156 - pad, 70 - pad, 400 + 2 * pad, 330 + 2 * pad));
  // it lets the mouse through and never takes the keys
  QVERIFY(ring->testAttribute(Qt::WA_TransparentForMouseEvents));
  QCOMPARE(ring->focusPolicy(), Qt::NoFocus);
  // a ring at the edge of the window is cut at the edge
  ring->setTarget(host.joints);
  QCOMPARE(ring->geometry(), QRect(0, 70 - pad, 150 + pad, 330 + 2 * pad));
}

void TestHelperMode::highlightFollowsItsControl()
{
  Host host;
  HelperHighlight* ring = host.helper->highlight();
  const int pad = Theme::Size::focusRingOffset + Theme::Size::focusRingWidth;
  ring->setTarget(host.view);
  QVERIFY(ring->isVisible());
  // the part above it moves: the ring moves with it
  host.middle->move(160, 80);
  QCOMPARE(ring->geometry(), QRect(160 - pad, 80 - pad, 400 + 2 * pad, 330 + 2 * pad));
  host.view->resize(300, 200);
  QCOMPARE(ring->geometry(), QRect(160 - pad, 80 - pad, 300 + 2 * pad, 200 + 2 * pad));
  // hidden with its control, back with it
  host.middle->hide();
  QVERIFY(!ring->isVisible());
  host.middle->show();
  QVERIFY(ring->isVisible());
  // a control in a floating window: the ring goes into that window
  QWidget floating;
  floating.resize(300, 200);
  QWidget* inside = new QWidget(&floating);
  inside->setGeometry(20, 30, 100, 40);
  floating.show();
  ring->setTarget(inside);
  QCOMPARE(ring->parentWidget(), &floating);
  QCOMPARE(ring->geometry(), QRect(20 - pad, 30 - pad, 100 + 2 * pad, 40 + 2 * pad));
  ring->setTarget(nullptr);
  QVERIFY(!ring->isVisible());
}

void TestHelperMode::lessonStepGoesOnWhenTheActionHappens()
{
  Host host;
  host.setOn(true);
  host.open(true);
  HelperPanel* panel = host.panel();
  QCOMPARE(panel->lessonButtons().size(), 3);
  QCOMPARE(panel->lessonButtons().at(0)->text(), QStringLiteral("Make your first move"));
  QSignalSpy changed(host.helper, &HelperMode::lessonChanged);
  panel->lessonButtons().at(0)->click();
  QCOMPARE(host.helper->lesson(), 0);
  QCOMPARE(panel->pages()->currentWidget(), panel->lessonPage());
  QCOMPARE(panel->lessonTitle()->text(), QStringLiteral("Make your first move"));
  // one thing at a time: the hint card is not shown during a lesson
  QVERIFY(!host.cards->cardShown());

  struct Expect { const char* progress; const char* text; QWidget* target; const char* event; };
  const QList<Expect> steps = {
    {"Step 1 of 4", "Click a joint on the figure to pick it.", host.view, HelperSpec::picked},
    {"Step 2 of 4", "Turn the picked joint: drag one of the coloured rings around it, or move a slider in Selected joint.",
     host.selected, HelperSpec::turned},
    {"Step 3 of 4", "Press Set key. It saves the pose you see now as a key.", host.setKey, HelperSpec::keySet},
    {"Step 4 of 4", "Press Start to watch your animation.", host.start, HelperSpec::played},
  };
  for (int i = 0; i < steps.size(); ++i)
  {
    QCOMPARE(host.helper->step(), i);
    QCOMPARE(panel->progressLabel()->text(), QString::fromLatin1(steps.at(i).progress));
    QCOMPARE(panel->stepLabel()->text(), QString::fromLatin1(steps.at(i).text));
    QCOMPARE(host.helper->highlight()->target(), steps.at(i).target);
    QVERIFY(host.helper->highlight()->isVisible());
    QVERIFY(panel->stopButton()->isVisible());
    QVERIFY(!panel->backButton()->isVisible());
    // something else happening does not move the step on
    host.helper->notify(QLatin1String(i == 0 ? HelperSpec::played : HelperSpec::picked));
    QCOMPARE(host.helper->step(), i);
    host.helper->notify(QLatin1String(steps.at(i).event));
  }
  QVERIFY(host.helper->lessonDone());
  QCOMPARE(panel->progressLabel()->text(), QStringLiteral("Lesson done"));
  QCOMPARE(panel->stepLabel()->text(),
           QStringLiteral("Your first key is saved. Go to another frame and make more keys to build a move."));
  QVERIFY(!host.helper->highlight()->isVisible());
  QVERIFY(!panel->stopButton()->isVisible());
  QVERIFY(panel->backButton()->isVisible());
  QCOMPARE(changed.count(), 5);   // started, three steps on, done
  // a done lesson stays done
  host.helper->notify(QLatin1String(HelperSpec::played));
  QVERIFY(host.helper->lessonDone());
  // Back to lessons: the lesson list, and the hint card again
  panel->backButton()->click();
  QCOMPARE(host.helper->lesson(), -1);
  QCOMPARE(panel->pages()->currentWidget(), panel->homePage());
  QVERIFY(host.cards->cardShown());
  QCOMPARE(host.helper->highlight()->target(), host.view);
}

void TestHelperMode::lessonSkipsWhatIsAlreadyDone()
{
  Host host;
  host.setOn(true);
  host.open(true);
  // a joint is already picked: the lesson starts with turning it
  host.already.insert(QLatin1String(HelperSpec::picked));
  host.helper->startLesson(0);
  QCOMPARE(host.helper->step(), 1);
  QCOMPARE(host.panel()->progressLabel()->text(), QStringLiteral("Step 2 of 4"));
  QCOMPARE(host.helper->highlight()->target(), host.selected);
  host.helper->stopLesson();
  // the Script side is already shown: the lesson asks for Animate at once
  host.already.insert(QLatin1String(HelperSpec::scriptSide));
  host.helper->startLesson(2);
  QCOMPARE(host.helper->step(), 1);
  QCOMPARE(host.helper->highlight()->target(), host.animateSwitch);
  host.helper->stopLesson();
  // everything already so: the lesson is done at once and says so
  host.already.insert(QLatin1String(HelperSpec::libraryShown));
  host.already.insert(QLatin1String(HelperSpec::layoutReset));
  host.helper->startLesson(1);
  QVERIFY(host.helper->lessonDone());
  QCOMPARE(host.panel()->progressLabel()->text(), QStringLiteral("Lesson done"));
}

void TestHelperMode::lessonsNeedingAnAnimationAreGreyedWithTheReason()
{
  Host host;
  host.setOn(true);
  QToolButton* first = host.panel()->lessonButtons().at(0);
  QVERIFY(!first->isEnabled());
  QCOMPARE(first->toolTip(), QStringLiteral("Pick a joint, turn it, save a key and watch it. Open an animation first."));
  QCOMPARE(first->statusTip(), QStringLiteral("Open an animation first."));
  QCOMPARE(host.helper->blockedReason(0), QStringLiteral("Open an animation first."));
  host.helper->startLesson(0);
  QCOMPARE(host.helper->lesson(), -1);
  // the others work without an animation
  for (int i = 1; i < 3; ++i)
  {
    QVERIFY(host.panel()->lessonButtons().at(i)->isEnabled());
    QCOMPARE(host.panel()->lessonButtons().at(i)->toolTip(), QString::fromLatin1(HelperSpec::lessons[i].summary));
  }
  host.open(true);
  QVERIFY(first->isEnabled());
  QCOMPARE(first->toolTip(), QStringLiteral("Pick a joint, turn it, save a key and watch it."));
  // the animation closes during the lesson: the lesson ends
  host.helper->startLesson(0);
  QCOMPARE(host.helper->lesson(), 0);
  host.open(false);
  QCOMPARE(host.helper->lesson(), -1);
  QVERIFY(!first->isEnabled());
  // a lesson cannot start while the helper is off
  host.setOn(false);
  host.helper->startLesson(1);
  QCOMPARE(host.helper->lesson(), -1);
}

void TestHelperMode::lessonStopsWithStopEscAndHelperOff()
{
  Host host;
  host.setOn(true);
  host.helper->startLesson(1);
  QCOMPARE(host.helper->lesson(), 1);
  QCOMPARE(host.panel()->stopButton()->text(), QStringLiteral("Stop lesson"));
  host.panel()->stopButton()->click();
  QCOMPARE(host.helper->lesson(), -1);
  QCOMPARE(host.panel()->pages()->currentWidget(), host.panel()->homePage());
  // Esc in the panel stops the lesson (the panel has the keys after a lesson starts)
  host.panel()->lessonButtons().at(1)->click();
  QCOMPARE(host.helper->lesson(), 1);
  QTest::keyClick(host.panel(), Qt::Key_Escape);
  QCOMPARE(host.helper->lesson(), -1);
  // Esc on a done lesson goes back to the lessons
  host.helper->startLesson(2);
  host.helper->notify(QLatin1String(HelperSpec::scriptSide));
  host.helper->notify(QLatin1String(HelperSpec::animateSide));
  QVERIFY(host.helper->lessonDone());
  QTest::keyClick(host.panel(), Qt::Key_Escape);
  QCOMPARE(host.helper->lesson(), -1);
  // the helper switched off ends the lesson, and on again shows the lessons
  host.helper->startLesson(1);
  host.setOn(false);
  QCOMPARE(host.helper->lesson(), -1);
  QVERIFY(!host.helper->highlight()->isVisible());
  host.setOn(true);
  QCOMPARE(host.panel()->pages()->currentWidget(), host.panel()->homePage());
  // the action of a step does nothing while no lesson runs
  host.helper->notify(QLatin1String(HelperSpec::libraryShown));
  QCOMPARE(host.helper->lesson(), -1);
}

void TestHelperMode::stepSaysWhenItsControlIsNotOnScreen()
{
  Host host;
  host.setOn(true);
  host.helper->startLesson(1);
  QVERIFY(!host.panel()->notOnScreenLabel()->isVisible());
  host.library->hide();
  host.helper->refresh();
  QVERIFY(host.panel()->notOnScreenLabel()->isVisible());
  QCOMPARE(host.panel()->notOnScreenLabel()->text(),
           QStringLiteral("This part is not on screen now. View > Reset layout brings it back."));
  QVERIFY(!host.helper->highlight()->isVisible());
  host.library->show();
  host.helper->refresh();
  QVERIFY(!host.panel()->notOnScreenLabel()->isVisible());
  QVERIFY(host.helper->highlight()->isVisible());
}

void TestHelperMode::otherLessonsGoOnWithTheirActions()
{
  Host host;
  host.setOn(true);
  host.helper->startLesson(1);
  QCOMPARE(host.panel()->stepLabel()->text(), QStringLiteral("Press Library at the top. The ready-made moves open in a window."));
  QCOMPARE(host.helper->highlight()->target(), host.library);
  host.helper->notify(QLatin1String(HelperSpec::libraryShown));
  QCOMPARE(host.panel()->stepLabel()->text(),
           QStringLiteral("Press Reset layout at the top. Every window goes back to its first place and the Library closes."));
  QCOMPARE(host.helper->highlight()->target(), host.resetLayout);
  host.helper->notify(QLatin1String(HelperSpec::layoutReset));
  QVERIFY(host.helper->lessonDone());
  QCOMPARE(host.panel()->stepLabel()->text(), QStringLiteral("You can move and resize every window. Reset layout always puts them back."));
  host.panel()->backButton()->click();

  host.helper->startLesson(2);
  QCOMPARE(host.panel()->stepLabel()->text(), QStringLiteral("Press Script at the top (F8). The Script Lab gets most of the window."));
  QCOMPARE(host.helper->highlight()->target(), host.scriptSwitch);
  host.helper->notify(QLatin1String(HelperSpec::scriptSide));
  QCOMPARE(host.panel()->stepLabel()->text(), QStringLiteral("Press Animate (F7). Your animation panels come back as you left them."));
  QCOMPARE(host.helper->highlight()->target(), host.animateSwitch);
  host.helper->notify(QLatin1String(HelperSpec::animateSide));
  QVERIFY(host.helper->lessonDone());
  QCOMPARE(host.panel()->stepLabel()->text(), QStringLiteral("Switch any time. Each side keeps its own layout."));
}

void TestHelperMode::panelMovesNothingAndStaysInside()
{
  Host host;
  const QList<QWidget*> others = {host.bar, host.middle, host.view, host.selected, host.setKey, host.start, host.tiles,
                                  host.joints, host.setup, host.library, host.resetLayout};
  QList<QRect> before;
  for (QWidget* w : others) before << w->geometry();
  host.setOn(true);
  host.open(true);
  QCoreApplication::processEvents();
  const QRect panel = host.panel()->geometry();
  const QRect hostRect(QPoint(0, 0), host.window.size());
  QVERIFY(hostRect.contains(panel));
  // beside the tool: at the bottom right of the middle part, not inside a panel
  const QRect middle = host.middle->geometry();
  QCOMPARE(panel.right(), middle.right() - Theme::Size::spacingL);
  QVERIFY(panel.bottom() <= middle.bottom());
  QCOMPARE(panel.width(), host.panel()->panelWidth());
  for (int j = 0; j < others.size(); ++j) QCOMPARE(others.at(j)->geometry(), before.at(j));
  // a lesson and back: nothing moved
  host.helper->startLesson(0);
  host.helper->stopLesson();
  for (int j = 0; j < others.size(); ++j) QCOMPARE(others.at(j)->geometry(), before.at(j));
  host.setOn(false);
  for (int j = 0; j < others.size(); ++j) QCOMPARE(others.at(j)->geometry(), before.at(j));
  host.setOn(true);
  // a small window keeps the whole panel inside it
  host.window.resize(420, 300);
  QCoreApplication::processEvents();
  QVERIFY(QRect(QPoint(0, 0), host.window.size()).contains(host.panel()->geometry()));
  // and at 200 percent text
  UserSettings::setTextSizePercent(200);
  host.helper->refresh();
  QVERIFY(QRect(QPoint(0, 0), host.window.size()).contains(host.panel()->geometry()));
  UserSettings::setTextSizePercent(100);
}

// the longest hint card fits in the panel, also after the card changed while the panel was shown
void TestHelperMode::cardInThePanelIsNeverCut()
{
  Host host;
  host.setOn(true);
  host.open(true);
  QCoreApplication::processEvents();
  for (int i = 0; i < 3; ++i)
  {
    HintCard* card = host.cards->card();
    QCoreApplication::processEvents();
    const int needs = card->heightForWidth(card->width());
    QVERIFY2(card->height() >= needs, qPrintable(QStringLiteral("card %1 is %2 high, needs %3").arg(i).arg(card->height()).arg(needs)));
    QVERIFY(host.panel()->height() >= host.panel()->wantedHeight());
    host.cards->next();
  }
  // and nothing in the panel needs the scroll bar while the window has room: the lesson pages too
  QScrollArea* scroll = host.panel()->findChild<QScrollArea*>(QStringLiteral("helperScroll"));
  QVERIFY(scroll);
  QVERIFY(!scroll->verticalScrollBar()->isVisible());
  for (int i = 0; i < HelperSpec::lessonCount; ++i)
  {
    host.helper->startLesson(i);
    QCoreApplication::processEvents();
    QVERIFY2(!scroll->verticalScrollBar()->isVisible(), HelperSpec::lessons[i].id);
    host.helper->stopLesson();
  }
}

void TestHelperMode::panelIsDrawnInTheDesignSystemAtEveryTextSize()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    HelperPanel panel;
    const QImage image = UiRender::render(&panel);
    UiRender::save(image, QStringLiteral("helperpanel"), v);
    QVERIFY(!image.isNull());
    // the fill is surface, the edge border, a lesson button surface_raised
    QString why = UiRender::expectColour(image, &panel, &panel, QPoint(panel.width() / 2, 0),
                                         Theme::color(Theme::Token::Border, v.dark), QStringLiteral("panel border"), 2);
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + why));
    why = UiRender::expectColour(image, &panel, &panel, QPoint(panel.width() / 2, 4),
                                 Theme::color(Theme::Token::Surface, v.dark), QStringLiteral("panel fill"), 2);
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + why));
    QToolButton* lesson = panel.lessonButtons().at(1);
    why = UiRender::expectColour(image, &panel, lesson, QPoint(lesson->width() - 4, lesson->height() / 2),
                                 Theme::color(Theme::Token::SurfaceRaised, v.dark), QStringLiteral("lesson button fill"), 2);
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + why));
    // every lesson button shows its whole title
    for (QToolButton* button : panel.lessonButtons())
      QVERIFY2(button->width() >= button->sizeHint().width(),
               qPrintable(v.name() + QStringLiteral(": ") + button->text() + QStringLiteral(" is cut")));
    QVERIFY(panel.height() >= panel.wantedHeight());
  }
}

// The window itself is built in the program only, so the wiring is checked in its source, as
// tests/guidance does.
void TestHelperMode::windowWiringIsInPlace()
{
  const QString source = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp"));
  const QString playback = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindowplayback.cpp"));
  QVERIFY(!source.isEmpty());
  QVERIFY(!playback.isEmpty());
  // the helper is built after the hint cards and follows the one hints setting
  QVERIFY(source.contains(QStringLiteral("buildHintCards();\n  buildHelper();")));
  QVERIFY(source.contains(QStringLiteral("if(m_helper) m_helper->setOn(on);")));
  QVERIFY(source.contains(QStringLiteral("m_helper->setOn(UserSettings::tipsOn());")));
  QVERIFY(source.contains(QStringLiteral("if(m_helper) m_helper->setAnimationOpen(!empty);")));
  // the switch sets Help > Show hints, which sets everything else (as the status bar switch and Setup do)
  QCOMPARE(source.count(QStringLiteral("if(m_hintsAction) m_hintsAction->setChecked(on);")), 3);
  QVERIFY(source.contains(QStringLiteral("connect(m_helper,&HelperMode::switchClicked,this,[this](bool on)")));
  QVERIFY(source.contains(QStringLiteral("connect(m_helper,&HelperMode::offRequested,this,[this]()")));
  // the switch stands right of Animate / Script
  QVERIFY(source.contains(QStringLiteral("m_helper->addToBar(ui->toolBar,barActions.value(barActions.indexOf(m_workMode->scriptAction())+1));")));
  // every event of the lessons is sent where it happens
  for (const char* event : {"picked", "turned", "keySet", "libraryShown", "layoutReset", "scriptSide", "animateSide"})
  {
    const QString id = QString::fromLatin1(event);
    QVERIFY2(source.contains(QStringLiteral("m_helper->notify(QLatin1String(HelperSpec::%1));").arg(id)), qPrintable(id));
  }
  QVERIFY(playback.contains(QStringLiteral("if(m_helper) m_helper->notify(QLatin1String(HelperSpec::played));")));
  // every target of the lessons and the cards is answered by the window
  for (int i = 0; i < HelperSpec::lessonCount; ++i)
    for (int s = 0; s < HelperSpec::lessons[i].stepCount; ++s)
    {
      const QString target = QString::fromLatin1(HelperSpec::lessons[i].steps[s].target);
      QVERIFY2(source.contains(QStringLiteral("if(name==QLatin1String(\"%1\")").arg(target)), qPrintable(target));
    }
  for (const GuidanceSpec::Card& card : GuidanceSpec::cards)
    QVERIFY(source.contains(QStringLiteral("if(name==QLatin1String(\"%1\")").arg(QString::fromLatin1(card.target))));
  // the panel follows the theme
  QVERIFY(source.contains(QStringLiteral("if(m_helper) m_helper->applyLook();")));
}

QTEST_MAIN(TestHelperMode)
#include "tst_helpermode.moc"
