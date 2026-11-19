/*
 * PinglyMotion - tests for the avatar body setting (Options > Body)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "constants.h"
#include "usersettings.h"

class TestBodySetting : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void defaultIsRuth2();
    void unknownValueBecomesRuth2();
    void choiceIsSaved();
    void bodyKeyWinsOverTheOldKey();
    void systemAvatarSettingBecomesRuth2();
    void oldFileWithBentoBodyOnMeansMax();
    void oldFileWithBentoBodyOffMeansRuth2();
    void oldFileWithNeitherMeansRuth2();
    void savingDropsTheOldKey();

  private:
    void writeVersionGroup(const QVariantMap& values);
    QString read(const QString& key);

    QTemporaryDir m_dir;
};

static const QString group = QStringLiteral("user_settings/1.0/");

void TestBodySetting::initTestCase()
{
  QVERIFY(m_dir.isValid());
  // never touch the real settings of the person running the tests
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void TestBodySetting::init()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
  UserSettings::setBody(QStringLiteral("max"));   // something else than the default
}

void TestBodySetting::writeVersionGroup(const QVariantMap& values)
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  // the group exists as soon as it holds one value (an older PinglyMotion saved "loop" too)
  settings.setValue(group + QStringLiteral("loop"), true);
  for (auto it = values.begin(); it != values.end(); ++it)
    settings.setValue(group + it.key(), it.value());
  settings.sync();
}

QString TestBodySetting::read(const QString& key)
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  return settings.value(group + key).toString();
}

void TestBodySetting::defaultIsRuth2()
{
  // a first start has no settings file: nothing is read, and the default is Ruth2
  UserSettings::setBody(QStringLiteral("ruth2"));
  UserSettings::readSettings();
  QCOMPARE(UserSettings::body(), QStringLiteral("ruth2"));
}

void TestBodySetting::unknownValueBecomesRuth2()
{
  UserSettings::setBody(QStringLiteral("nonsense"));
  QCOMPARE(UserSettings::body(), QStringLiteral("ruth2"));
  UserSettings::setBody(QString());
  QCOMPARE(UserSettings::body(), QStringLiteral("ruth2"));
}

void TestBodySetting::choiceIsSaved()
{
  for (const QString& choice : {QStringLiteral("ruth2"), QStringLiteral("max")})
  {
    UserSettings::setBody(choice);
    UserSettings::writeSettings();
    UserSettings::setBody(QStringLiteral("other"));   // becomes ruth2
    UserSettings::readSettings();
    QCOMPARE(UserSettings::body(), choice);
  }
}

void TestBodySetting::bodyKeyWinsOverTheOldKey()
{
  writeVersionGroup({{QStringLiteral("body"), QStringLiteral("ruth2")},
                     {QStringLiteral("bento_body"), true}});
  UserSettings::readSettings();
  QCOMPARE(UserSettings::body(), QStringLiteral("ruth2"));
}

// the system avatar was removed: a file that still says "system" shows Ruth2
void TestBodySetting::systemAvatarSettingBecomesRuth2()
{
  writeVersionGroup({{QStringLiteral("body"), QStringLiteral("system")}});
  UserSettings::readSettings();
  QCOMPARE(UserSettings::body(), QStringLiteral("ruth2"));
  UserSettings::setBody(QStringLiteral("system"));
  QCOMPARE(UserSettings::body(), QStringLiteral("ruth2"));
}

void TestBodySetting::oldFileWithBentoBodyOnMeansMax()
{
  writeVersionGroup({{QStringLiteral("bento_body"), true}});
  UserSettings::setBody(QStringLiteral("ruth2"));
  UserSettings::readSettings();
  QCOMPARE(UserSettings::body(), QStringLiteral("max"));
}

void TestBodySetting::oldFileWithBentoBodyOffMeansRuth2()
{
  writeVersionGroup({{QStringLiteral("bento_body"), false}});
  UserSettings::readSettings();
  QCOMPARE(UserSettings::body(), QStringLiteral("ruth2"));
}

void TestBodySetting::oldFileWithNeitherMeansRuth2()
{
  writeVersionGroup({});
  UserSettings::readSettings();
  QCOMPARE(UserSettings::body(), QStringLiteral("ruth2"));
}

void TestBodySetting::savingDropsTheOldKey()
{
  writeVersionGroup({{QStringLiteral("bento_body"), true}});
  UserSettings::readSettings();
  UserSettings::writeSettings();
  QCOMPARE(read(QStringLiteral("body")), QStringLiteral("max"));
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  QVERIFY(!settings.contains(group + QStringLiteral("bento_body")));
}

QTEST_MAIN(TestBodySetting)
#include "tst_bodysetting.moc"
