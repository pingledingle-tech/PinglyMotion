/*
 * PinglyMotion - tests for the remembered settings of UX phase 0b:
 * Beginner/Expert mode, hints, camera habit, text size and theme.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
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

QTEST_MAIN(TestUserSettings)
#include "tst_usersettings.moc"
