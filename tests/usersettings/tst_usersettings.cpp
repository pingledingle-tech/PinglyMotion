/*
 * PinglyMotion - tests for the remembered settings of UX phase 0b:
 * Beginner/Expert mode, hints, camera habit, text size and theme.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QRect>
#include <QtTest>

#include "constants.h"
#include "usersettings.h"

class TestUserSettings : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void defaultsOnFirstStart();
    void defaultsWhenKeysAreMissing();
    void valuesSurviveSaveAndReload();
    void textSizeIsCutToTheRange();
    void badStoredValuesFallBack();
    void unknownCameraHabitBecomesDefault();
    void windowPlacesSurviveSaveAndReload();
    void badWindowPlacesAreLeftOut();
    void expertModeWatchers();
    void welcomeAndHintCardAreRemembered();

  private:
    void writeVersionGroup(const QVariantMap& values);
    void changeEverything();

    QTemporaryDir m_dir;
};

static const QString group = QStringLiteral("user_settings/1.0/");

void TestUserSettings::initTestCase()
{
  QVERIFY(m_dir.isValid());
  // never touch the real settings of the person running the tests
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void TestUserSettings::init()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
  changeEverything();
}

void TestUserSettings::changeEverything()
{
  UserSettings::setExpertMode(true);
  UserSettings::setTipsOn(false);
  UserSettings::setCameraHabit(QStringLiteral("blender"));
  UserSettings::setTextSizePercent(150);
  UserSettings::setDarkTheme(false);
}

void TestUserSettings::writeVersionGroup(const QVariantMap& values)
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.setValue(group + QStringLiteral("loop"), true);   // makes the group exist
  for (auto it = values.begin(); it != values.end(); ++it)
    settings.setValue(group + it.key(), it.value());
  settings.sync();
}

void TestUserSettings::defaultsOnFirstStart()
{
  // the values a new program has before anything is read or saved
  UserSettings::setExpertMode(false);
  UserSettings::setTipsOn(true);
  UserSettings::setCameraHabit(QString());
  UserSettings::setTextSizePercent(0);
  UserSettings::setDarkTheme(true);
  QCOMPARE(UserSettings::expertMode(), false);   // Beginner mode
  QCOMPARE(UserSettings::tipsOn(), true);
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("pinglymotion"));
  QCOMPARE(UserSettings::textSizePercent(), 100);
  QCOMPARE(UserSettings::darkTheme(), true);
}

void TestUserSettings::defaultsWhenKeysAreMissing()
{
  // a settings file of an older version has none of the new keys
  writeVersionGroup({});
  UserSettings::readSettings();
  QCOMPARE(UserSettings::expertMode(), false);
  QCOMPARE(UserSettings::tipsOn(), true);
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("pinglymotion"));
  QCOMPARE(UserSettings::textSizePercent(), 100);
  QCOMPARE(UserSettings::darkTheme(), true);
}

void TestUserSettings::valuesSurviveSaveAndReload()
{
  const QStringList habits = UserSettings::cameraHabits();
  QCOMPARE(habits, (QStringList{ "pinglymotion", "maya", "blender", "sl" }));
  for (const QString& habit : habits)
  {
    UserSettings::setCameraHabit(habit);
    UserSettings::writeSettings();
    // restart-like: put other values in memory, then read the file
    UserSettings::setExpertMode(false);
    UserSettings::setTipsOn(true);
    UserSettings::setCameraHabit(QStringLiteral("maya"));
    UserSettings::setTextSizePercent(100);
    UserSettings::setDarkTheme(true);
    UserSettings::readSettings();
    QCOMPARE(UserSettings::expertMode(), true);
    QCOMPARE(UserSettings::tipsOn(), false);
    QCOMPARE(UserSettings::cameraHabit(), habit);
    QCOMPARE(UserSettings::textSizePercent(), 150);
    QCOMPARE(UserSettings::darkTheme(), false);
    changeEverything();
  }
}

void TestUserSettings::textSizeIsCutToTheRange()
{
  UserSettings::setTextSizePercent(99);
  QCOMPARE(UserSettings::textSizePercent(), 100);
  UserSettings::setTextSizePercent(-5);
  QCOMPARE(UserSettings::textSizePercent(), 100);
  UserSettings::setTextSizePercent(100);
  QCOMPARE(UserSettings::textSizePercent(), 100);
  UserSettings::setTextSizePercent(200);
  QCOMPARE(UserSettings::textSizePercent(), 200);
  UserSettings::setTextSizePercent(201);
  QCOMPARE(UserSettings::textSizePercent(), 200);
  UserSettings::setTextSizePercent(100000);
  QCOMPARE(UserSettings::textSizePercent(), 200);
}

void TestUserSettings::badStoredValuesFallBack()
{
  // out of range is cut; not a number and unknown names give the default
  writeVersionGroup({ { "text_size_percent", 500 }, { "camera_habit", "photoshop" },
                      { "expert_mode", "maybe" } });
  UserSettings::readSettings();
  QCOMPARE(UserSettings::textSizePercent(), 200);
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("pinglymotion"));
  QCOMPARE(UserSettings::expertMode(), false);

  writeVersionGroup({ { "text_size_percent", "big" }, { "camera_habit", "" },
                      { "expert_mode", "" } });
  changeEverything();
  UserSettings::readSettings();
  QCOMPARE(UserSettings::textSizePercent(), 100);
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("pinglymotion"));
  QCOMPARE(UserSettings::expertMode(), false);

  writeVersionGroup({ { "text_size_percent", 10 }, { "expert_mode", "true" } });
  UserSettings::readSettings();
  QCOMPARE(UserSettings::textSizePercent(), 100);
  QCOMPARE(UserSettings::expertMode(), true);
}

void TestUserSettings::unknownCameraHabitBecomesDefault()
{
  UserSettings::setCameraHabit(QStringLiteral("Maya"));   // names are exact
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("pinglymotion"));
  UserSettings::setCameraHabit(QStringLiteral("maya"));
  QCOMPARE(UserSettings::cameraHabit(), QStringLiteral("maya"));
}

// UX phase 5: the place of the Edit curve window is kept between sessions
void TestUserSettings::windowPlacesSurviveSaveAndReload()
{
  QCOMPARE(UserSettings::windowPlace(QStringLiteral("edit_curve_window")), QRect());
  UserSettings::setWindowPlace(QStringLiteral("edit_curve_window"), QRect(1930, 120, 420, 360));
  UserSettings::writeSettings();
  UserSettings::setWindowPlace(QStringLiteral("edit_curve_window"), QRect(1, 2, 3, 4));
  UserSettings::readSettings();
  QCOMPARE(UserSettings::windowPlace(QStringLiteral("edit_curve_window")), QRect(1930, 120, 420, 360));
  // a negative place (a screen left of the main one) is a place too
  UserSettings::setWindowPlace(QStringLiteral("edit_curve_window"), QRect(-1200, -40, 500, 400));
  UserSettings::writeSettings();
  UserSettings::readSettings();
  QCOMPARE(UserSettings::windowPlace(QStringLiteral("edit_curve_window")), QRect(-1200, -40, 500, 400));
  // an empty rect forgets it
  UserSettings::setWindowPlace(QStringLiteral("edit_curve_window"), QRect());
  UserSettings::writeSettings();
  UserSettings::readSettings();
  QCOMPARE(UserSettings::windowPlace(QStringLiteral("edit_curve_window")), QRect());
}

void TestUserSettings::badWindowPlacesAreLeftOut()
{
  writeVersionGroup({ { "window_places/edit_curve_window", "big" },
                      { "window_places/zero", QRect(10, 10, 0, 0) },
                      { "window_places/huge", QRect(0, 0, 900000, 500) },
                      { "window_places/good", QRect(5, 6, 300, 200) } });
  UserSettings::readSettings();
  QCOMPARE(UserSettings::windowPlace(QStringLiteral("edit_curve_window")), QRect());
  QCOMPARE(UserSettings::windowPlace(QStringLiteral("zero")), QRect());
  QCOMPARE(UserSettings::windowPlace(QStringLiteral("huge")), QRect());
  QCOMPARE(UserSettings::windowPlace(QStringLiteral("good")), QRect(5, 6, 300, 200));
}

void TestUserSettings::expertModeWatchers()
{
  UserSettings::setExpertMode(false);
  QList<bool> seen;
  {
    QObject context;
    UserSettings::watchExpertMode(&context, [&seen](bool on) { seen.append(on); });
    UserSettings::setExpertMode(true);
    UserSettings::setExpertMode(true);    // no change: no call
    UserSettings::setExpertMode(false);
    QCOMPARE(seen, (QList<bool>{ true, false }));
  }
  // the context is gone: no more calls
  UserSettings::setExpertMode(true);
  QCOMPARE(seen, (QList<bool>{ true, false }));
}

// UX phase 6a: the welcome card shows once, the hint card is remembered
void TestUserSettings::welcomeAndHintCardAreRemembered()
{
  // nothing stored: the card was not answered and the first hint card is next
  writeVersionGroup({});
  UserSettings::setWelcomeSeen(true);
  UserSettings::setHintCard(7);
  UserSettings::readSettings();
  QCOMPARE(UserSettings::welcomeSeen(), false);
  QCOMPARE(UserSettings::hintCard(), 0);

  UserSettings::setWelcomeSeen(true);
  UserSettings::setHintCard(4);
  UserSettings::writeSettings();
  UserSettings::setWelcomeSeen(false);
  UserSettings::setHintCard(0);
  UserSettings::readSettings();
  QCOMPARE(UserSettings::welcomeSeen(), true);
  QCOMPARE(UserSettings::hintCard(), 4);

  // the key names in the file
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  QCOMPARE(settings.value(group + QStringLiteral("welcome_seen")).toString(), QStringLiteral("true"));
  QCOMPARE(settings.value(group + QStringLiteral("hint_card")).toInt(), 4);

  // 1 counts as true; anything else does not; a card number is cut to 0 to 99 and a word gives 0
  writeVersionGroup({{QStringLiteral("welcome_seen"), QStringLiteral("1")}, {QStringLiteral("hint_card"), QStringLiteral("500")}});
  UserSettings::readSettings();
  QCOMPARE(UserSettings::welcomeSeen(), true);
  QCOMPARE(UserSettings::hintCard(), 99);
  writeVersionGroup({{QStringLiteral("welcome_seen"), QStringLiteral("yes please")}, {QStringLiteral("hint_card"), QStringLiteral("abc")}});
  UserSettings::readSettings();
  QCOMPARE(UserSettings::welcomeSeen(), false);
  QCOMPARE(UserSettings::hintCard(), 0);
  writeVersionGroup({{QStringLiteral("hint_card"), QStringLiteral("-3")}});
  UserSettings::readSettings();
  QCOMPARE(UserSettings::hintCard(), 0);
}

QTEST_MAIN(TestUserSettings)
#include "tst_usersettings.moc"
