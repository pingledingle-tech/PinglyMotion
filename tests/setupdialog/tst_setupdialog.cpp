/*
 * PinglyMotion - tests for the Setup, Camera and keys, and Keys and mouse
 * dialogs (UX phase 1d, docs/UX_DECISIONS.md, section 6c)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

#include "camerahabitspec.h"
#include "camerakeysdialog.h"
#include "constants.h"
#include "keysmousedialog.h"
#include "menuspec.h"
#include "setupdialog.h"
#include "usersettings.h"

class TestSetupDialog : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    // the Setup dialog opens with the settings UserSettings holds
    void opensWithCurrentSettings();
    // each choice emits the matching signal with the new value, and nothing else
    void expertChoiceEmits();
    void cameraChoiceEmits();
    void themeChoiceEmits();
    void textSizeChoiceEmits();
    void hintsSwitchEmits();
    void showAllKeysOpensCameraDialog();
    // a choice applied through UserSettings (the same path the window's own actions take) is
    // still there after a restart-like write and reload, and the dialog shows it again
    void choiceRoundTripsAfterRestart();

    // the Camera and keys dialog: one radio per habit, the table matches the habit exactly,
    // switching the radio switches the table and remembers the choice
    void cameraKeysDialogMatchesSpec();
    void cameraKeysDialogSwitchesTable();

    // the Keys and mouse dialog: every row is there, and the keys it shows are the ones the
    // menus really use (no made-up shortcuts), so it only says what is true
    void keysMouseDialogHasEveryRow();
    void keysMouseDialogKeysMatchTheMenus();

    // the menu items this phase builds are no longer greyed "not built yet"
    void menuItemsAreNoLongerGreyed();

  private:
    QTemporaryDir m_dir;
};

void TestSetupDialog::initTestCase()
{
  QVERIFY(m_dir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void TestSetupDialog::init()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
  UserSettings::setExpertMode(false);
  UserSettings::setTipsOn(true);
  UserSettings::setCameraHabit(QStringLiteral("pinglymotion"));
  UserSettings::setTextSizePercent(100);
  UserSettings::setDarkTheme(true);
}

void TestSetupDialog::opensWithCurrentSettings()
{
  UserSettings::setExpertMode(true);
  UserSettings::setCameraHabit(QStringLiteral("blender"));
  UserSettings::setDarkTheme(false);
  UserSettings::setTextSizePercent(150);
  UserSettings::setTipsOn(false);

  SetupDialog dialog;
  QCOMPARE(dialog.expertChecked(), true);
  QCOMPARE(dialog.cameraHabitChecked(), QStringLiteral("blender"));
  QCOMPARE(dialog.darkChecked(), false);
  QCOMPARE(dialog.textSizeChecked(), 150);
  QCOMPARE(dialog.hintsChecked(), false);
}

void TestSetupDialog::expertChoiceEmits()
{
  SetupDialog dialog;
  QSignalSpy spy(&dialog, &SetupDialog::expertModeRequested);
  QRadioButton* expertRadio = nullptr;
  for (QRadioButton* radio : dialog.findChildren<QRadioButton*>())
    if (radio->text() == QCoreApplication::translate("SetupDialog", SetupText::expertChoice)) expertRadio = radio;
  QVERIFY(expertRadio);
  expertRadio->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), true);
}

void TestSetupDialog::cameraChoiceEmits()
{
  SetupDialog dialog;
  QSignalSpy spy(&dialog, &SetupDialog::cameraHabitRequested);
  QRadioButton* sl = nullptr;
  for (QRadioButton* radio : dialog.findChildren<QRadioButton*>())
    if (radio->text() == CameraHabitSpec::name(CameraHabitSpec::sl)) sl = radio;
  QVERIFY(sl);
  sl->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("sl"));
}

void TestSetupDialog::themeChoiceEmits()
{
  SetupDialog dialog;
  QSignalSpy spy(&dialog, &SetupDialog::darkThemeRequested);
  QRadioButton* light = nullptr;
  for (QRadioButton* radio : dialog.findChildren<QRadioButton*>())
    if (radio->text() == QCoreApplication::translate("SetupDialog", SetupText::lightChoice)) light = radio;
  QVERIFY(light);
  light->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), false);
}

void TestSetupDialog::textSizeChoiceEmits()
{
  SetupDialog dialog;
  QSignalSpy spy(&dialog, &SetupDialog::textSizeRequested);
  QRadioButton* size150 = nullptr;
  for (QRadioButton* radio : dialog.findChildren<QRadioButton*>())
    if (radio->text() == QStringLiteral("150%")) size150 = radio;
  QVERIFY(size150);
  size150->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toInt(), 150);
}

void TestSetupDialog::hintsSwitchEmits()
{
  UserSettings::setTipsOn(true);
  SetupDialog dialog;
  QSignalSpy spy(&dialog, &SetupDialog::hintsRequested);
  QCOMPARE(dialog.hintsChecked(), true);
  // the switch is a TipsSwitch (src/controlindicator.h); clicking its tool button toggles it
  QToolButton* button = dialog.findChild<QToolButton*>();
  QVERIFY(button);
  button->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), false);
}

void TestSetupDialog::showAllKeysOpensCameraDialog()
{
  SetupDialog dialog;
  QSignalSpy spy(&dialog, &SetupDialog::showCameraKeysRequested);
  QPushButton* showKeys = nullptr;
  for (QPushButton* button : dialog.findChildren<QPushButton*>())
    if (button->text() == QCoreApplication::translate("SetupDialog", SetupText::showAllKeys)) showKeys = button;
  QVERIFY(showKeys);
  showKeys->click();
  QCOMPARE(spy.count(), 1);
}

void TestSetupDialog::choiceRoundTripsAfterRestart()
{
  // the window applies a Setup choice through UserSettings, exactly like its own menu actions
  UserSettings::setCameraHabit(QStringLiteral("maya"));
  UserSettings::setExpertMode(true);
  UserSettings::writeSettings();

  // restart-like: other values in memory, then read the file back
  UserSettings::setCameraHabit(QStringLiteral("sl"));
  UserSettings::setExpertMode(false);
  UserSettings::readSettings();

  SetupDialog dialog;
  QCOMPARE(dialog.cameraHabitChecked(), QStringLiteral("maya"));
  QCOMPARE(dialog.expertChecked(), true);
}

void TestSetupDialog::cameraKeysDialogMatchesSpec()
{
  for (const CameraHabitSpec::Habit* habit : CameraHabitSpec::all)
  {
    UserSettings::setCameraHabit(QLatin1String(habit->id));
    CameraKeysDialog dialog;
    QCOMPARE(dialog.habitChecked(), QLatin1String(habit->id));
    QTableWidget* table = dialog.table();
    QCOMPARE(table->rowCount(), 5);
    for (int row = 0; row < 5; ++row)
    {
      QCOMPARE(table->item(row, 0)->text(), CameraHabitSpec::action(habit->rows[row]));
      QCOMPARE(table->item(row, 1)->text(), CameraHabitSpec::gesture(habit->rows[row]));
    }
  }
}

void TestSetupDialog::cameraKeysDialogSwitchesTable()
{
  UserSettings::setCameraHabit(QStringLiteral("pinglymotion"));
  CameraKeysDialog dialog;
  QSignalSpy spy(&dialog, &CameraKeysDialog::cameraHabitRequested);
  QRadioButton* blenderRadio = nullptr;
  for (QRadioButton* radio : dialog.findChildren<QRadioButton*>())
    if (radio->text() == CameraHabitSpec::name(CameraHabitSpec::blender)) blenderRadio = radio;
  QVERIFY(blenderRadio);
  blenderRadio->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("blender"));
  QCOMPARE(dialog.table()->item(0, 1)->text(), CameraHabitSpec::gesture(CameraHabitSpec::blender.rows[0]));
}

void TestSetupDialog::keysMouseDialogHasEveryRow()
{
  KeysMouseDialog dialog;
  const int rowCount = int(sizeof(KeysMouseSpec::rows) / sizeof(KeysMouseSpec::rows[0]));
  QCOMPARE(dialog.table()->rowCount(), rowCount);
  for (int row = 0; row < rowCount; ++row)
  {
    QCOMPARE(dialog.table()->item(row, 0)->text(), KeysMouseSpec::action(KeysMouseSpec::rows[row]));
    QCOMPARE(dialog.table()->item(row, 1)->text(), KeysMouseSpec::gesture(KeysMouseSpec::rows[row]));
  }
}

// the Keys and mouse page must never show a key that is not the menu's own (design charter P6,
// never claim what is not true): check the ones that name a real menu shortcut
void TestSetupDialog::keysMouseDialogKeysMatchTheMenus()
{
  using namespace MenuSpec;
  const QHash<QString, const char*> expectedKey = {
    {QStringLiteral("Front view"), viewFront.key},
    {QStringLiteral("Side view"), viewSide.key},
    {QStringLiteral("Top view"), viewTop.key},
    {QStringLiteral("Home view"), viewHome.key},
    {QStringLiteral("Set key"), editSetKey.key},
    {QStringLiteral("Switch theme"), viewLightTheme.key},
    {QStringLiteral("Expert mode"), viewExpertMode.key},
    {QStringLiteral("Show hints"), helpHints.key},
    {QStringLiteral("Hide panels"), viewHidePanels.key},
    {QStringLiteral("Script Lab"), viewScriptLab.key},
  };
  int checked = 0;
  for (const KeysMouseSpec::Row& row : KeysMouseSpec::rows)
  {
    const QString action = KeysMouseSpec::action(row);
    if (!expectedKey.contains(action)) continue;
    QVERIFY2(KeysMouseSpec::gesture(row) == QLatin1String(expectedKey.value(action)), qPrintable(action));
    ++checked;
  }
  QCOMPARE(checked, expectedKey.size());
  // Text size is a key combination that is not stored in a single menu key string this simply
  QCOMPARE(QString::fromLatin1(viewTextSize.key), QStringLiteral("Ctrl+="));
}

void TestSetupDialog::menuItemsAreNoLongerGreyed()
{
  using namespace MenuSpec;
  for (const Item* item : {&viewSetup, &helpSetup, &helpCamera, &helpKeys})
    QCOMPARE(QString::fromLatin1(item->reason), QString());
}

QTEST_MAIN(TestSetupDialog)
#include "tst_setupdialog.moc"
