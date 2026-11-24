/*
 * PinglyMotion - test of the guidance (UX phase 6a, docs/UX_DECISIONS.md, 6k)
 *
 * The welcome card: every text, the three start choices (the same clips as the
 * empty state), Beginner or Expert and the camera habit, Skip and Esc, shows
 * only once and the choice is remembered after a restart. The hint cards: every
 * title and text word for word as the mockup's CARDS, the order, skipping what
 * is done, Next, Turn hints off, following the hints setting, remembered place,
 * the card never moves another control. Texts are also looked up in the
 * mockup file itself (docs/mockup/index.html).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

#include "camerahabitspec.h"
#include "constants.h"
#include "emptystate.h"
#include "guidancespec.h"
#include "hintcard.h"
#include "theme.h"
#include "uirender.h"
#include "usersettings.h"
#include "welcomedialog.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  QString mockup() { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/mockup/index.html")); }

  QString tr2(const char* text, const char* context) { return QCoreApplication::translate(context, text); }

  // the mockup's CARDS: title, text and target in the order of the file
  struct MockCard { QString title, text, target; };
  QList<MockCard> mockupCards()
  {
    const QString html = mockup();
    const int start = html.indexOf(QStringLiteral("const CARDS = ["));
    const int end = html.indexOf(QStringLiteral("];"), start);
    const QString block = html.mid(start, end - start);
    QList<MockCard> cards;
    const QRegularExpression row(QStringLiteral("t: \"([^\"]*)\", p: \"([^\"]*)\", at: \"#([A-Za-z]+)\""));
    auto it = row.globalMatch(block);
    while (it.hasNext())
    {
      const QRegularExpressionMatch m = it.next();
      cards.append({m.captured(1), m.captured(2), m.captured(3)});
    }
    return cards;
  }

  // a stand-in window with the parts the cards stand by
  struct Host
  {
    QWidget window;
    QWidget* view;
    QWidget* selected;
    QWidget* setKey;
    QWidget* start;
    QWidget* tiles;
    QWidget* joints;
    QWidget* setup;
    HintCards* cards = nullptr;

    Host()
    {
      window.resize(1400, 900);
      auto make = [this](const QString& name, const QRect& r)
      {
        QWidget* w = new QWidget(&window);
        w->setObjectName(name);
        w->setGeometry(r);
        return w;
      };
      joints = make(QStringLiteral("joints"), QRect(0, 60, 260, 600));
      view = make(QStringLiteral("view"), QRect(266, 60, 800, 600));
      selected = make(QStringLiteral("selected"), QRect(1080, 60, 320, 600));
      setKey = make(QStringLiteral("setKey"), QRect(1000, 760, 90, 32));
      start = make(QStringLiteral("start"), QRect(20, 760, 90, 32));
      tiles = make(QStringLiteral("tiles"), QRect(200, 820, 500, 60));
      setup = make(QStringLiteral("setup"), QRect(1300, 4, 80, 32));
      cards = new HintCards(&window, [this](const QString& name) -> QWidget*
      {
        if (name == QLatin1String("view")) return view;
        if (name == QLatin1String("selected")) return selected;
        if (name == QLatin1String("setKey")) return setKey;
        if (name == QLatin1String("start")) return start;
        if (name == QLatin1String("tiles")) return tiles;
        if (name == QLatin1String("joints")) return joints;
        if (name == QLatin1String("setup")) return setup;
        return nullptr;
      });
      window.show();
    }
  };
}

class TestGuidance : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    // the welcome card
    void welcomeTextsAreTheMockups();
    void welcomeStartChoicesAreTheEmptyStatesClips();
    void welcomeBeginnerAndExpert();
    void welcomeSkipAndEscAreSkip();
    void welcomeShowsOnlyOnceAndRemembersTheChoice();
    void welcomeCanBeAnsweredAgain();

    // the hint cards
    void cardTextsAreTheMockupsWordForWord();
    void cardsFollowTheHintsSetting();
    void cardsGoInOrderAndAreRemembered();
    void cardsSkipWhatIsDone();
    void cardButtons();
    void cardCloseStops();
    void cardStaysByItsPartAndMovesNothing();
    void cardIsDrawnInTheDesignSystemAtEveryTextSize();

    // Beginner and Expert
    void expertControlsAreTheMockupsAndNothingElse();
    void windowWiringIsInPlace();

  private:
    QTemporaryDir m_dir;
};

void TestGuidance::initTestCase()
{
  QVERIFY(m_dir.isValid());
  // never touch the real settings of the person running the tests
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void TestGuidance::init()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
  UserSettings::readSettings();
  // the values this test looks at, at their defaults (reading a missing file keeps what was set)
  UserSettings::setWelcomeSeen(false);
  UserSettings::setExpertMode(false);
  UserSettings::setCameraHabit(QStringLiteral("pinglymotion"));
  UserSettings::setTipsOn(true);
  UserSettings::setHintCard(0);
  UserSettings::setTextSizePercent(100);
}

// ---------------------------------------------------------------- the welcome card

void TestGuidance::welcomeTextsAreTheMockups()
{
  const QString html = mockup();
  QVERIFY(!html.isEmpty());
  using namespace GuidanceSpec;
  QCOMPARE(QString::fromLatin1(welcomeTitle), QStringLiteral("Make your first animation"));
  QCOMPARE(QString::fromLatin1(welcomeIntro),
           QStringLiteral("Pick a pose to start. You can change everything later in <b>Setup</b> (the gear button at the top right)."));
  QCOMPARE(QString::fromLatin1(experienceLabel), QStringLiteral("Have you made animations before?"));
  QCOMPARE(QString::fromLatin1(newChoice), QStringLiteral("New to animation"));
  QCOMPARE(QString::fromLatin1(proChoice), QStringLiteral("Experienced"));
  QCOMPARE(QString::fromLatin1(cameraLabel), QStringLiteral("Camera and keys like"));
  QCOMPARE(QString::fromLatin1(skipButton), QStringLiteral("Skip"));
  QCOMPARE(QString::fromLatin1(skipMessage), QStringLiteral("Welcome card closed. Setup (top right) or Help brings it back."));
  QCOMPARE(QString::fromLatin1(readyMessage), QStringLiteral("Ready. Click a joint on the figure to pick it."));
  // every one of them is in the mockup, word for word
  for (const QString& text : {QString::fromLatin1(welcomeTitle), QString::fromLatin1(welcomeIntro),
                              QString::fromLatin1(experienceLabel), QString::fromLatin1(cameraLabel),
                              QString::fromLatin1(skipMessage)})
    QVERIFY2(html.contains(text), qPrintable(text));
  QVERIFY(html.contains(QStringLiteral(">New to animation</button>")));
  QVERIFY(html.contains(QStringLiteral(">Experienced</button>")));
  QVERIFY(html.contains(QStringLiteral(">Skip</button>")));
  QVERIFY(html.contains(QStringLiteral("\"Ready. \" + TXT[\"NEXT-01\"]")));
  QVERIFY(html.contains(QStringLiteral("\"NEXT-01\": \"Click a joint on the figure to pick it.\"")));

  WelcomeDialog dialog;
  QCOMPARE(dialog.windowTitle(), QStringLiteral("Make your first animation"));
  QCOMPARE(dialog.introLabel()->text(), QString::fromLatin1(welcomeIntro));
  QCOMPARE(dialog.newRadio()->text(), QStringLiteral("New to animation"));
  QCOMPARE(dialog.proRadio()->text(), QStringLiteral("Experienced"));
  QCOMPARE(dialog.skipButton()->text(), QStringLiteral("Skip"));
  // the three choices say the empty state's words (the mockup's lines claim notes the clips lack)
  QCOMPARE(dialog.startButtons().size(), 3);
  QCOMPARE(dialog.startButtons().at(0)->text(),
           QStringLiteral("Standing\nA calm standing pose. A good start for most moves."));
  QCOMPARE(dialog.startButtons().at(1)->text(), QStringLiteral("Sitting\nA calm seated pose. Good for a quiet scene."));
  QCOMPARE(dialog.startButtons().at(2)->text(), QStringLiteral("Dance example\nA short, simple dance. Change it to learn."));
  for (const QString& title : {QStringLiteral(">Standing</span>"), QStringLiteral(">Sitting</span>"), QStringLiteral(">Dance example</span>")})
    QVERIFY2(html.contains(title), qPrintable(title));
}

void TestGuidance::welcomeStartChoicesAreTheEmptyStatesClips()
{
  const char* clips[] = {"m2m-idle-a", "m2m-sitting-idle", "m2m-dance-simple"};
  for (int i = 0; i < 3; ++i)
  {
    WelcomeDialog dialog;
    dialog.startButtons().at(i)->click();
    const WelcomeDialog::Answer answer = dialog.answer();
    QVERIFY(!answer.skipped);
    QCOMPARE(answer.clipId, QString::fromLatin1(clips[i]));
    QCOMPARE(answer.clipId, QString::fromLatin1(EmptySpec::choices[i]->clip));
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
  }
}

void TestGuidance::welcomeBeginnerAndExpert()
{
  // the card opens as a first start: New to animation, no camera row
  {
    WelcomeDialog dialog;
    dialog.show();
    QVERIFY(dialog.newRadio()->isChecked());
    QVERIFY(!dialog.proRadio()->isChecked());
    QVERIFY(!dialog.cameraRow()->isVisibleTo(&dialog));
    dialog.startButtons().at(0)->click();
    const WelcomeDialog::Answer answer = dialog.answer();
    QVERIFY(!answer.expert);
    QVERIFY(answer.cameraHabit.isEmpty());
  }
  // Experienced shows the camera row; the chosen habit comes with the answer
  {
    WelcomeDialog dialog;
    dialog.show();
    dialog.proRadio()->click();
    QVERIFY(dialog.cameraRow()->isVisibleTo(&dialog));
    QCOMPARE(dialog.cameraRadios().size(), 4);
    QStringList names;
    for (QRadioButton* radio : dialog.cameraRadios()) names << radio->text();
    QCOMPARE(names, (QStringList{"PinglyMotion", "Maya", "Blender", "Second Life"}));
    QVERIFY(dialog.cameraRadios().at(0)->isChecked());   // the habit now
    dialog.cameraRadios().at(1)->click();
    dialog.startButtons().at(1)->click();
    const WelcomeDialog::Answer answer = dialog.answer();
    QVERIFY(answer.expert);
    QCOMPARE(answer.cameraHabit, QStringLiteral("maya"));
    // going back to New to animation hides the row again, and the answer has no habit
  }
  {
    WelcomeDialog dialog;
    dialog.show();
    dialog.proRadio()->click();
    dialog.cameraRadios().at(2)->click();
    dialog.newRadio()->click();
    QVERIFY(!dialog.cameraRow()->isVisibleTo(&dialog));
    dialog.skipButton()->click();
    QVERIFY(!dialog.answer().expert);
    QVERIFY(dialog.answer().cameraHabit.isEmpty());
  }
  // the card opens on what Setup says now
  UserSettings::setExpertMode(true);
  UserSettings::setCameraHabit(QStringLiteral("sl"));
  WelcomeDialog again;
  again.show();
  QVERIFY(again.proRadio()->isChecked());
  QVERIFY(again.cameraRow()->isVisibleTo(&again));
  QVERIFY(again.cameraRadios().at(3)->isChecked());
}

void TestGuidance::welcomeSkipAndEscAreSkip()
{
  {
    WelcomeDialog dialog;
    dialog.skipButton()->click();
    QVERIFY(dialog.answer().skipped);
    QVERIFY(dialog.answer().clipId.isEmpty());
  }
  {
    WelcomeDialog dialog;
    dialog.show();
    dialog.proRadio()->click();
    QTest::keyClick(&dialog, Qt::Key_Escape);
    QVERIFY(!dialog.isVisible());
    QVERIFY(dialog.answer().skipped);
    QVERIFY(dialog.answer().clipId.isEmpty());
    QVERIFY(dialog.answer().expert);   // what was chosen still counts, as in the mockup
    QCOMPARE(dialog.result(), int(QDialog::Rejected));
  }
  {
    // the window's close button is the same as Esc
    WelcomeDialog dialog;
    dialog.show();
    dialog.close();
    QVERIFY(dialog.answer().skipped);
  }
}

void TestGuidance::welcomeShowsOnlyOnceAndRemembersTheChoice()
{
  QVERIFY(WelcomeDialog::due());   // first start

  WelcomeDialog dialog;
  dialog.show();
  dialog.proRadio()->click();
  dialog.cameraRadios().at(2)->click();
  dialog.startButtons().at(0)->click();
  WelcomeDialog::apply(dialog.answer());
  QVERIFY(!WelcomeDialog::due());
  QVERIFY(UserSettings::welcomeSeen());
  QVERIFY(UserSettings::expertMode());
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("blender"));

  // a restart: written, forgotten and read again
  UserSettings::writeSettings();
  UserSettings::setWelcomeSeen(false);
  UserSettings::setExpertMode(false);
  UserSettings::setCameraHabit(QStringLiteral("pinglymotion"));
  UserSettings::readSettings();
  QVERIFY(!WelcomeDialog::due());
  QVERIFY(UserSettings::expertMode());
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("blender"));
}

void TestGuidance::welcomeCanBeAnsweredAgain()
{
  // Skip is an answer too: it never shows by itself again, and the choice is remembered
  WelcomeDialog first;
  first.skipButton()->click();
  WelcomeDialog::apply(first.answer());
  QVERIFY(!WelcomeDialog::due());
  QVERIFY(!UserSettings::expertMode());

  // Help > Show the welcome card again: a new card opens on the old choice and can change it
  WelcomeDialog second;
  second.show();
  QVERIFY(second.newRadio()->isChecked());
  second.proRadio()->click();
  second.startButtons().at(2)->click();
  WelcomeDialog::apply(second.answer());
  QVERIFY(UserSettings::expertMode());
  QVERIFY(!WelcomeDialog::due());
  // a camera habit is only changed by an experienced answer
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("pinglymotion"));

  // a settings file that says something else than true or 1 means the card was not answered
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, Constants::organizationName(), Constants::applicationName());
  settings.setValue(QStringLiteral("user_settings/1.0/welcome_seen"), QStringLiteral("maybe"));
  settings.sync();
  UserSettings::readSettings();
  QVERIFY(WelcomeDialog::due());
}

// ---------------------------------------------------------------- the hint cards

void TestGuidance::cardTextsAreTheMockupsWordForWord()
{
  const QList<MockCard> mock = mockupCards();
  QCOMPARE(mock.size(), 9);
  QCOMPARE(GuidanceSpec::cardCount, 9);
  const QHash<QString, QString> targets = {{"view", "view"}, {"selected", "selected"}, {"bKey", "setKey"},
    {"bStart", "start"}, {"tiles", "tiles"}, {"joints", "joints"}, {"setupBtn", "setup"}};
  for (int i = 0; i < 9; ++i)
  {
    const GuidanceSpec::Card& card = GuidanceSpec::cards[i];
    QCOMPARE(QString::fromLatin1(card.title), mock.at(i).title);
    QCOMPARE(QString::fromLatin1(card.text), mock.at(i).text);
    QCOMPARE(QString::fromLatin1(card.target), targets.value(mock.at(i).target));
  }
  // the first titles in the order the owner sees them
  QCOMPARE(QString::fromLatin1(GuidanceSpec::cards[0].title), QStringLiteral("Pick a joint"));
  QCOMPARE(QString::fromLatin1(GuidanceSpec::cards[1].title), QStringLiteral("Pull a limb"));
  // the buttons and the feedback line
  const QString html = mockup();
  QVERIFY(html.contains(QStringLiteral(">Turn hints off</button>")));
  QVERIFY(html.contains(QStringLiteral("data-name=\"Turn all hints off (Help > Show hints)\"")));
  QVERIFY(html.contains(QStringLiteral("data-name=\"Close this hint\"")));
  QVERIFY(html.contains(QStringLiteral("data-name=\"Next hint\">Next</button>")));
  QVERIFY(html.contains(QStringLiteral("`Hint ${cardIdx + 1} of ${CARDS.length}`")));
  QVERIFY(html.contains(QString::fromLatin1(GuidanceSpec::hintsOffMessage)));
  QVERIFY(html.contains(QString::fromLatin1(GuidanceSpec::expertOnMessage)));
  QVERIFY(html.contains(QString::fromLatin1(GuidanceSpec::expertOffMessage)));
}

void TestGuidance::cardsFollowTheHintsSetting()
{
  Host host;
  HintCards* cards = host.cards;
  // no animation open: no card
  cards->setHintsOn(true);
  QVERIFY(!cards->cardShown());
  cards->setAnimationOpen(true);
  QVERIFY(cards->cardShown());
  QCOMPARE(cards->card()->titleLabel()->text(), QStringLiteral("Pick a joint"));
  QCOMPARE(cards->card()->textLabel()->text(),
           QStringLiteral("Click a joint on the figure to pick it. You can also use the Joints list."));
  QCOMPARE(cards->card()->dots()->accessibleName(), QStringLiteral("Hint 1 of 9"));
  // hints off: gone; on again: the same card is back
  cards->setHintsOn(false);
  QVERIFY(!cards->cardShown());
  cards->setHintsOn(true);
  QVERIFY(cards->cardShown());
  QCOMPARE(cards->card()->titleLabel()->text(), QStringLiteral("Pick a joint"));
  // an animation that is closed takes the card away
  cards->setAnimationOpen(false);
  QVERIFY(!cards->cardShown());
  cards->setAnimationOpen(true);
  QVERIFY(cards->cardShown());
  // a part that is not on screen has no card
  host.view->hide();
  cards->refresh();
  QVERIFY(!cards->cardShown());
  host.view->show();
  cards->refresh();
  QVERIFY(cards->cardShown());
}

void TestGuidance::cardsGoInOrderAndAreRemembered()
{
  const QList<MockCard> mock = mockupCards();
  {
    Host host;
    host.cards->setHintsOn(true);
    host.cards->setAnimationOpen(true);
    for (int i = 0; i < 9; ++i)
    {
      QVERIFY2(host.cards->cardShown(), qPrintable(QString::number(i)));
      QCOMPARE(host.cards->index(), i);
      QCOMPARE(host.cards->card()->titleLabel()->text(), mock.at(i).title);
      QCOMPARE(host.cards->card()->textLabel()->text(), mock.at(i).text);
      QCOMPARE(host.cards->card()->dots()->accessibleName(), QStringLiteral("Hint %1 of 9").arg(i + 1));
      if (i == 3)
      {
        QCOMPARE(UserSettings::hintCard(), 3);   // remembered as it goes
        break;
      }
      host.cards->card()->nextButton()->click();
    }
  }
  // a restart in the middle: the next card is the one the user had not seen
  UserSettings::writeSettings();
  UserSettings::setHintCard(0);
  UserSettings::readSettings();
  QCOMPARE(UserSettings::hintCard(), 3);
  {
    Host host;
    host.cards->setHintsOn(true);
    host.cards->setAnimationOpen(true);
    QCOMPARE(host.cards->index(), 3);
    QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Red, green, blue = X, Y, Z"));
    for (int i = 3; i < 9; ++i) host.cards->next();
    // after the last one there is no card, and it stays so
    QVERIFY(!host.cards->cardShown());
    QCOMPARE(host.cards->index(), 9);
    host.cards->next();
    QCOMPARE(host.cards->index(), 9);
    QCOMPARE(UserSettings::hintCard(), 9);
  }
  {
    Host host;
    host.cards->setHintsOn(true);
    host.cards->setAnimationOpen(true);
    QVERIFY(!host.cards->cardShown());
    host.cards->restart();
    QVERIFY(host.cards->cardShown());
    QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Pick a joint"));
  }
  // an odd number in the settings gives a usable one
  UserSettings::setHintCard(-4);
  QCOMPARE(UserSettings::hintCard(), 0);
  UserSettings::setHintCard(5000);
  QCOMPARE(UserSettings::hintCard(), 99);
  UserSettings::setHintCard(0);
}

void TestGuidance::cardsSkipWhatIsDone()
{
  Host host;
  host.cards->setHintsOn(true);
  host.cards->setAnimationOpen(true);
  QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Pick a joint"));
  host.cards->setPicked(true);
  QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Pull a limb"));
  host.cards->markPulled();
  QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Turn it"));
  host.cards->markTurned();
  QCOMPARE(host.cards->card()->titleLabel()->text(), QStringLiteral("Red, green, blue = X, Y, Z"));
  QCOMPARE(host.cards->index(), 3);
  // done things never bring a card back
  host.cards->setPicked(false);
  QCOMPARE(host.cards->index(), 3);

  // pulling first skips the earlier card that is still waiting only when it is current:
  // a user who turns a joint first still gets "Pick a joint" first
  UserSettings::setHintCard(0);
  Host second;
  second.cards->setHintsOn(true);
  second.cards->setAnimationOpen(true);
  second.cards->markTurned();
  QCOMPARE(second.cards->card()->titleLabel()->text(), QStringLiteral("Pick a joint"));
  second.cards->setPicked(true);
  second.cards->markPulled();
  // picked, pulled and turned: all three are done, so the fourth shows
  QCOMPARE(second.cards->card()->titleLabel()->text(), QStringLiteral("Red, green, blue = X, Y, Z"));
}

void TestGuidance::cardButtons()
{
  Host host;
  host.cards->setHintsOn(true);
  host.cards->setAnimationOpen(true);
  HintCard* card = host.cards->card();
  QCOMPARE(card->nextButton()->text(), QStringLiteral("Next"));
  QCOMPARE(card->nextButton()->toolTip(), QStringLiteral("Next hint"));
  QCOMPARE(card->hintsOffButton()->text(), QStringLiteral("Turn hints off"));
  QCOMPARE(card->hintsOffButton()->toolTip(), QStringLiteral("Turn all hints off (Help > Show hints)"));
  QCOMPARE(card->closeButton()->toolTip(), QStringLiteral("Close this hint"));
  QVERIFY(!card->closeButton()->icon().isNull());
  // Next goes to the next card, which shows at once
  card->nextButton()->click();
  QCOMPARE(host.cards->index(), 1);
  QVERIFY(host.cards->cardShown());
  QCOMPARE(card->titleLabel()->text(), QStringLiteral("Pull a limb"));
  // Turn hints off tells the window; the window turns the setting off, which hides the card
  QSignalSpy spy(host.cards, &HintCards::hintsOffRequested);
  card->hintsOffButton()->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(host.cards->index(), 1);   // it does not use up a card
  host.cards->setHintsOn(false);
  QVERIFY(!host.cards->cardShown());
}

// the close button closes the card and stops: no next card shows by itself (the project owner,
// 24 Nov 2026). Esc on the card does the same. Turning hints off and on shows the next card.
void TestGuidance::cardCloseStops()
{
  Host host;
  host.cards->setHintsOn(true);
  host.cards->setAnimationOpen(true);
  HintCard* card = host.cards->card();
  QSignalSpy closed(host.cards, &HintCards::cardClosed);
  QVERIFY(host.cards->cardShown());
  QCOMPARE(card->titleLabel()->text(), QStringLiteral("Pick a joint"));
  card->closeButton()->click();
  QVERIFY(!host.cards->cardShown());
  QVERIFY(host.cards->closed());
  QCOMPARE(closed.count(), 1);
  // the closed card counts as seen
  QCOMPARE(host.cards->index(), 1);
  QCOMPARE(UserSettings::hintCard(), 1);
  // nothing the user does brings a card back by itself
  host.cards->setPicked(true);
  host.cards->markPulled();
  host.cards->setAnimationOpen(false);
  host.cards->setAnimationOpen(true);
  host.cards->refresh();
  QVERIFY(!host.cards->cardShown());
  // hints off and on (Help > Show hints twice): the next card waiting shows
  host.cards->setHintsOn(false);
  host.cards->setHintsOn(true);
  QVERIFY(host.cards->cardShown());
  QCOMPARE(card->titleLabel()->text(), QStringLiteral("Turn it"));
  QVERIFY(!host.cards->closed());
  // Esc on the card closes it too
  card->setFocus();
  QTest::keyClick(card, Qt::Key_Escape);
  QVERIFY(!host.cards->cardShown());
  QCOMPARE(host.cards->index(), 3);
  QCOMPARE(closed.count(), 2);
  // the status line text
  QCOMPARE(QString::fromLatin1(GuidanceSpec::closedMessage),
           QStringLiteral("Hint closed. Turn Help > Show hints off and on for the next one."));
  UserSettings::setHintCard(0);
}

void TestGuidance::cardStaysByItsPartAndMovesNothing()
{
  Host host;
  const QList<QWidget*> others = {host.view, host.selected, host.setKey, host.start, host.tiles, host.joints, host.setup};
  QList<QRect> before;
  for (QWidget* w : others) before << w->geometry();
  host.cards->setHintsOn(true);
  host.cards->setAnimationOpen(true);
  const QRect hostRect(QPoint(0, 0), host.window.size());
  const QList<QWidget*> targets = {host.view, host.view, host.selected, host.selected, host.setKey, host.start,
                                   host.tiles, host.joints, host.setup};
  for (int i = 0; i < 9; ++i)
  {
    QVERIFY(host.cards->cardShown());
    QCOMPARE(host.cards->index(), i);
    const QRect card = host.cards->card()->geometry();
    QVERIFY2(hostRect.contains(card), qPrintable(QStringLiteral("card %1 sticks out: %2,%3 %4x%5").arg(i)
             .arg(card.x()).arg(card.y()).arg(card.width()).arg(card.height())));
    // the mockup's places: bottom right of the view, left of the Selected joint panel, above Set key, ...
    const QRect part = targets.at(i)->geometry();
    switch (i)
    {
      case 0: case 1:
        QCOMPARE(card.right(), part.right() - Theme::Size::spacingL - 1);
        QVERIFY(card.bottom() <= part.bottom());
        break;
      case 2: case 3:
        QVERIFY(card.right() < part.left());
        QCOMPARE(card.top(), part.top() + 40);
        break;
      case 4: case 5: case 6:
        QCOMPARE(card.left(), part.left());
        QVERIFY(card.bottom() < part.top());
        break;
      case 7:
        QCOMPARE(card.left(), part.right() + Theme::Size::spacingM);
        break;
      case 8:
        QCOMPARE(card.right(), part.right() - 1);
        QVERIFY(card.top() > part.bottom());
        break;
    }
    // no other control moved or changed its size
    for (int j = 0; j < others.size(); ++j) QCOMPARE(others.at(j)->geometry(), before.at(j));
    host.cards->next();
  }
  QVERIFY(!host.cards->cardShown());
  for (int j = 0; j < others.size(); ++j) QCOMPARE(others.at(j)->geometry(), before.at(j));

  // a smaller window keeps the card inside it
  UserSettings::setHintCard(0);
  host.cards->restart();
  host.window.resize(700, 500);
  QCoreApplication::processEvents();
  QVERIFY(host.cards->cardShown());
  QVERIFY(QRect(QPoint(0, 0), host.window.size()).contains(host.cards->card()->geometry()));
}

void TestGuidance::cardIsDrawnInTheDesignSystemAtEveryTextSize()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    HintCard card;
    // the longest text of all
    const GuidanceSpec::Card& spec = GuidanceSpec::cards[1];
    card.setCard(1, GuidanceSpec::cardCount, QString::fromLatin1(spec.title), QString::fromLatin1(spec.text));
    const QImage image = UiRender::render(&card);
    UiRender::save(image, QStringLiteral("hintcard"), v);
    QVERIFY(!image.isNull());
    // the edge is the accent_text colour, the fill surface_raised, Next the accent
    QString why = UiRender::expectColour(image, &card, &card, QPoint(card.width() / 2, 0),
                                         Theme::color(Theme::Token::AccentText, v.dark), QStringLiteral("card border"), 2);
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + why));
    why = UiRender::expectColour(image, &card, &card, QPoint(card.width() / 2, 6),
                                 Theme::color(Theme::Token::SurfaceRaised, v.dark), QStringLiteral("card fill"), 2);
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + why));
    why = UiRender::expectColour(image, &card, card.nextButton(), QPoint(3, card.nextButton()->height() / 2),
                                 Theme::color(Theme::Token::Accent, v.dark), QStringLiteral("Next fill"), 2);
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + why));
    // nothing is clipped and the three footer parts do not overlap
    // a text that wraps is checked by the height it needs for its width (the helper compares it with
    // the unwrapped size hint)
    QStringList clipped;
    for (const QString& line : UiRender::clipping(&card))
      if (!line.startsWith(QStringLiteral("QLabel 'hintCardText'")) && !line.startsWith(QStringLiteral("QLabel 'hintCardTitle'")))
        clipped << line;
    for (QLabel* label : {card.textLabel(), card.titleLabel()})
      if (label->height() < label->heightForWidth(label->width()))
        clipped << QStringLiteral("%1 is %2 high, its text needs %3").arg(label->objectName()).arg(label->height())
                       .arg(label->heightForWidth(label->width()));
    QVERIFY2(clipped.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + clipped.join('\n')));
    QVERIFY(card.hintsOffButton()->geometry().right() < card.dots()->geometry().left());
    QVERIFY(card.dots()->geometry().right() < card.nextButton()->geometry().left());
  }
}

// ---------------------------------------------------------------- Beginner and Expert

void TestGuidance::expertControlsAreTheMockupsAndNothingElse()
{
  // the mockup marks exactly two controls as Expert mode only (the hands grid is built in code there)
  const QString html = mockup();
  const QRegularExpression expertOnly(QStringLiteral("class=\"btn expert-only\" data-cmd=\"[a-z]+\" data-name=\"([^\"]+)\""));
  QStringList names;
  auto it = expertOnly.globalMatch(html);
  while (it.hasNext()) names << it.next().captured(1);
  QCOMPARE(names, (QStringList{"Paste mirrored", "Edit curve"}));
  QVERIFY(html.contains(QStringLiteral("if (expert) h += `<section data-p=\"P5 P10\"><h3 class=\"sech\" id=\"hgL\">Each joint, in degrees</h3>")));

  // the program reads the mode for each of them (Paste mirrored and the hands grid here, Edit curve in
  // tests/easingtiles, the grid in tests/handstab), and a change is remembered
  UserSettings::setExpertMode(true);
  UserSettings::writeSettings();
  UserSettings::setExpertMode(false);
  UserSettings::readSettings();
  QVERIFY(UserSettings::expertMode());
  UserSettings::setExpertMode(false);
  UserSettings::writeSettings();
  UserSettings::setExpertMode(true);
  UserSettings::readSettings();
  QVERIFY(!UserSettings::expertMode());
}

// The window itself is built in the program only, so the wiring is checked in its source, as
// tests/menutexts does for the menus.
void TestGuidance::windowWiringIsInPlace()
{
  const QString source = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp"));
  QVERIFY(!source.isEmpty());
  // the welcome card: first start only, never with a file or --control, and from Help
  QVERIFY(source.contains(QStringLiteral("if(WelcomeDialog::due() && !m_startedWithFileOrControl) showWelcomeCard();")));
  QVERIFY(source.contains(QStringLiteral("m_startedWithFileOrControl=startWithControl || !fileFromCommandLine.isEmpty();")));
  QVERIFY(source.contains(QStringLiteral("connect(helpWelcomeAction,&QAction::triggered,this,&PinglyMotionWindow::showWelcomeCard);")));
  QVERIFY(source.contains(QStringLiteral("WelcomeDialog::apply(answer);")));
  QVERIFY(source.contains(QStringLiteral("startFromLibrary(answer.clipId);")));
  QVERIFY(source.contains(QStringLiteral("connect(&dialog,&SetupDialog::welcomeRequested")));
  // the hint cards follow the hints setting from both switches, the open animation, and what the user did
  QCOMPARE(source.count(QStringLiteral("m_hintCards->setHintsOn(on);")), 2);
  QVERIFY(source.contains(QStringLiteral("m_hintCards->setAnimationOpen(!empty);")));
  QVERIFY(source.contains(QStringLiteral("m_hintCards->setPicked(true);")));
  QVERIFY(source.contains(QStringLiteral("m_hintCards->markPulled();")));
  QVERIFY(source.contains(QStringLiteral("m_hintCards->markTurned();")));
  QVERIFY(source.contains(QStringLiteral("if(m_hintsAction) m_hintsAction->setChecked(false);")));
  // a closed card says so in the status line (UX small fixes)
  QVERIFY(source.contains(QStringLiteral("connect(m_hintCards,&HintCards::cardClosed,this,[this]()")));
  QVERIFY(source.contains(QStringLiteral("GuidanceSpec::closedMessage")));
  // the panels follow the text size, so Ctrl+0 brings back their size at 100 percent (UX small fixes)
  QVERIFY(source.contains(QStringLiteral("DialogFit::measurePanels(this,panelDocks())")));
  QVERIFY(source.contains(QStringLiteral("DialogFit::followTextSize(this,panels)")));
  // Expert mode shows Paste mirrored and says the mockup's sentence
  QVERIFY(source.contains(QStringLiteral("m_panels->pasteMirrored->setVisible(on);")));
  QVERIFY(source.contains(QStringLiteral("GuidanceSpec::expertOnMessage")));
  QVERIFY(source.contains(QStringLiteral("GuidanceSpec::expertOffMessage")));
}

QTEST_MAIN(TestGuidance)
#include "tst_guidance.moc"
