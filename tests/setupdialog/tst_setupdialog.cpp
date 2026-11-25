/*
 * PinglyMotion - tests for the Setup, Camera and keys, and Keys and mouse
 * dialogs (UX phase 1d, docs/UX_DECISIONS.md, section 6c)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTableWidget>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QTemporaryDir>
#include <QtTest>

#include "camerahabitspec.h"
#include "camerakeysdialog.h"
#include "constants.h"
#include "controlindicator.h"
#include "keysmousedialog.h"
#include "menuspec.h"
#include "setupdialog.h"
#include "usersettings.h"
#include "../windowsource.h"

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
    // "Show the welcome card again" (UX phase 6a, the mockup's Guidance section)
    void welcomeButtonAsksForTheWelcomeCard();
    // a choice applied through UserSettings (the same path the window's own actions take) is
    // still there after a restart-like write and reload, and the dialog shows it again
    void choiceRoundTripsAfterRestart();
    // Script editor (lane 1, jobs F2 and F3): suggestions only with Ctrl+Space and the Lua switch,
    // at the end of the dialog, so nothing above them moves; both emit and are remembered
    void scriptEditorChoices();

    // Connections (docs/UX_DECISIONS.md 6h): AI Assistant and live link to Aurora
    void connectionsStartOffByDefault();
    void connectionsShowTheStateTheWindowGives();
    void connectionsClicksSendRequestsAndNothingElse();
    void connectionsStayAfterEverythingElse();
    void connectionsStringsAreTheStatusBarsOwn();
    void windowConnectsSetupToTheSameCodeAsTheStatusBar();

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
  UserSettings::setSuggestOnRequestOnly(false);
  UserSettings::setLuaSwitch(false);
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
  QCheckBox* box = dialog.findChild<QCheckBox*>();
  QVERIFY(box);
  box->click();
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

void TestSetupDialog::scriptEditorChoices()
{
  SetupDialog dialog;
  dialog.show();
  QVERIFY(QTest::qWaitForWindowExposed(&dialog));
  QCheckBox* suggest = dialog.suggestOnRequestBox();
  QCheckBox* lua = dialog.luaSwitchBox();
  QVERIFY(suggest && lua);
  QCOMPARE(suggest->text(), QStringLiteral("Show suggestions only when I press Ctrl+Space"));
  QCOMPARE(suggest->toolTip(), QStringLiteral("Off: suggestions come up while you type. Enter never picks one by itself."));
  QCOMPARE(lua->text(), QStringLiteral("Lua (trial): show the language choice in the editor"));
  QCOMPARE(lua->toolTip(), QStringLiteral("Lua is not checked or run yet. Scripts stay LSL."));
  QCOMPARE(dialog.suggestOnRequestChecked(), false);
  QCOMPARE(dialog.luaSwitchChecked(), false);
  // both come after every other control of the dialog except Done
  QPushButton* welcome = dialog.welcomeButton();
  QVERIFY(suggest->geometry().top() > welcome->geometry().bottom());
  QVERIFY(lua->geometry().top() > suggest->geometry().bottom());
  // nothing moves when they change
  QList<QRect> before;
  for (QWidget* w : dialog.findChildren<QWidget*>()) before << w->geometry();
  QSignalSpy suggestSpy(&dialog, &SetupDialog::suggestOnRequestRequested);
  QSignalSpy luaSpy(&dialog, &SetupDialog::luaSwitchRequested);
  QSignalSpy expertSpy(&dialog, &SetupDialog::expertModeRequested);
  suggest->click();
  lua->click();
  QCOMPARE(suggestSpy.count(), 1);
  QCOMPARE(suggestSpy.at(0).at(0).toBool(), true);
  QCOMPARE(luaSpy.count(), 1);
  QCOMPARE(luaSpy.at(0).at(0).toBool(), true);
  QCOMPARE(expertSpy.count(), 0);
  QCoreApplication::processEvents();
  QList<QRect> after;
  for (QWidget* w : dialog.findChildren<QWidget*>()) after << w->geometry();
  QCOMPARE(after, before);
  // the window stores them in UserSettings; after a restart-like write and read the dialog shows them
  UserSettings::setSuggestOnRequestOnly(true);
  UserSettings::setLuaSwitch(true);
  UserSettings::writeSettings();
  UserSettings::setSuggestOnRequestOnly(false);
  UserSettings::setLuaSwitch(false);
  UserSettings::readSettings();
  SetupDialog again;
  QCOMPARE(again.suggestOnRequestChecked(), true);
  QCOMPARE(again.luaSwitchChecked(), true);
}

void TestSetupDialog::cameraKeysDialogMatchesSpec()
{
  for (const CameraHabitSpec::Habit* habit : CameraHabitSpec::all)
  {
    UserSettings::setCameraHabit(QLatin1String(habit->id));
    CameraKeysDialog dialog;
    QCOMPARE(dialog.habitChecked(), QLatin1String(habit->id));
    QTableWidget* table = dialog.table();
    QCOMPARE(table->rowCount(), 14);
    QCOMPARE(CameraHabitSpec::sharedRowCount, 9);
    for (int row = 0; row < 5; ++row)
    {
      QCOMPARE(table->item(row, 0)->text(), CameraHabitSpec::action(habit->rows[row]));
      QCOMPARE(table->item(row, 1)->text(), CameraHabitSpec::gesture(habit->rows[row]));
    }
    // UX phase 4a and 4b: the keys every habit shares, under the habit's rows, word for word
    const QStringList shared = {
      QStringLiteral("Pull a limb"), QStringLiteral("Press a joint and drag. Let go to set a key. Esc cancels."),
      QStringLiteral("Turn only that joint"), QStringLiteral("Hold Shift when you press the joint (or choose Turn only this joint)"),
      QStringLiteral("Turn on one axis"), QStringLiteral("Drag the red X, green Y or blue Z ring around the picked joint"),
      QStringLiteral("Pull with the keyboard"), QStringLiteral("Enter on the picked joint, arrow keys, Enter to set a key, Esc cancels"),
      QStringLiteral("View tools: pick, look around, move the view, zoom"), QStringLiteral("V, O, M, Z"),
      QStringLiteral("Frame the picked joint, the whole figure, home view"), QStringLiteral("F, Shift+F, 0"),
      QStringLiteral("Front, side, top view"), QStringLiteral("Numpad 1, 3, 7 (or 1, 3, 7 in the 3D view), or click an end of the axis widget"),
      QStringLiteral("Look around with keys"), QStringLiteral("Alt + arrow keys in the 3D view"),
      QStringLiteral("Zoom with keys"), QStringLiteral("Page Up, Page Down")};
    for (int row = 0; row < 9; ++row)
    {
      QCOMPARE(table->item(5 + row, 0)->text(), shared.at(2 * row));
      QCOMPARE(table->item(5 + row, 1)->text(), shared.at(2 * row + 1));
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
  const int rowCount = int(sizeof(KeysMouseSpec::rows) / sizeof(KeysMouseSpec::rows[0]));
  for (const CameraHabitSpec::Habit* habit : CameraHabitSpec::all)
  {
    UserSettings::setCameraHabit(QLatin1String(habit->id));
    KeysMouseDialog dialog;
    // Pick a joint, the habit's look around, move the view, zoom and focus rows, then the rest
    QCOMPARE(dialog.table()->rowCount(), rowCount + 4);
    QCOMPARE(dialog.table()->item(0, 0)->text(), QStringLiteral("Pick a joint"));
    int habitRow = 0;
    for (int i : {0, 1, 2, 4})
    {
      ++habitRow;
      QCOMPARE(dialog.table()->item(habitRow, 0)->text(), CameraHabitSpec::action(habit->rows[i]));
      QCOMPARE(dialog.table()->item(habitRow, 1)->text(), CameraHabitSpec::gesture(habit->rows[i]));
    }
    for (int row = 1; row < rowCount; ++row)
    {
      QCOMPARE(dialog.table()->item(row + 4, 0)->text(), KeysMouseSpec::action(KeysMouseSpec::rows[row]));
      QCOMPARE(dialog.table()->item(row + 4, 1)->text(), KeysMouseSpec::gesture(KeysMouseSpec::rows[row]));
    }
  }
  // the Maya habit, word for word
  UserSettings::setCameraHabit(QStringLiteral("maya"));
  KeysMouseDialog maya;
  QCOMPARE(maya.table()->item(1, 1)->text(), QStringLiteral("Alt + left-drag"));
  QCOMPARE(maya.table()->item(2, 1)->text(), QStringLiteral("Alt + middle-drag"));
  QCOMPARE(maya.table()->item(3, 1)->text(), QStringLiteral("Alt + right-drag, or wheel"));
  QCOMPARE(maya.table()->item(4, 0)->text(), QStringLiteral("Focus on the picked joint"));
  QCOMPARE(maya.table()->item(4, 1)->text(), QStringLiteral("F"));
  QCOMPARE(maya.table()->item(5, 0)->text(), QStringLiteral("Frame the whole figure"));
  QCOMPARE(maya.table()->item(5, 1)->text(), QStringLiteral("Shift+F"));
  QCOMPARE(maya.table()->item(6, 1)->text(), QStringLiteral("Alt + arrow keys in the 3D view"));
  QCOMPARE(maya.table()->item(7, 1)->text(), QStringLiteral("Page Up, Page Down"));
  // UX phase 4b: dragging a joint (part 2), the rings and the view tools (part 1)
  QCOMPARE(maya.table()->item(8, 0)->text(), QStringLiteral("Pull a limb"));
  QCOMPARE(maya.table()->item(8, 1)->text(), QStringLiteral("Press a joint and drag. Let go to set a key. Esc cancels."));
  QCOMPARE(maya.table()->item(9, 0)->text(), QStringLiteral("Turn only that joint"));
  QCOMPARE(maya.table()->item(9, 1)->text(), QStringLiteral("Hold Shift when you press the joint (or choose Turn only this joint)"));
  QCOMPARE(maya.table()->item(10, 0)->text(), QStringLiteral("Turn on one axis"));
  QCOMPARE(maya.table()->item(10, 1)->text(), QStringLiteral("Drag the red X, green Y or blue Z ring around the picked joint"));
  QCOMPARE(maya.table()->item(11, 0)->text(), QStringLiteral("Pull with the keyboard"));
  QCOMPARE(maya.table()->item(11, 1)->text(), QStringLiteral("Enter on the picked joint, arrow keys, Enter to set a key, Esc cancels"));
  QCOMPARE(maya.table()->item(12, 0)->text(), QStringLiteral("View tools: pick, look around, move the view, zoom"));
  QCOMPARE(maya.table()->item(12, 1)->text(), QStringLiteral("V, O, M, Z"));
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
    {QStringLiteral("Library"), viewLibrary.key},
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
  QCOMPARE(QString::fromLatin1(viewTextSize.key), QStringLiteral("Ctrl++"));
}

void TestSetupDialog::menuItemsAreNoLongerGreyed()
{
  using namespace MenuSpec;
  for (const Item* item : {&viewSetup, &helpSetup, &helpCamera, &helpKeys})
    QCOMPARE(QString::fromLatin1(item->reason), QString());
}

void TestSetupDialog::welcomeButtonAsksForTheWelcomeCard()
{
  SetupDialog dialog;
  QVERIFY(dialog.welcomeButton());
  QCOMPARE(dialog.welcomeButton()->text(), QStringLiteral("Show the welcome card again"));
  QCOMPARE(SetupText::welcomeAgain, "Show the welcome card again");
  QVERIFY(!dialog.welcomeButton()->icon().isNull());
  QSignalSpy spy(&dialog, &SetupDialog::welcomeRequested);
  dialog.welcomeButton()->click();
  QCOMPARE(spy.count(), 1);
  // the same words as Help > Show the welcome card again
  QCOMPARE(MenuSpec::text(MenuSpec::helpWelcome), dialog.welcomeButton()->text());
  QVERIFY(QString::fromLatin1(MenuSpec::helpWelcome.reason).isEmpty());
}

void TestSetupDialog::connectionsStartOffByDefault()
{
  SetupDialog dialog;
  QVERIFY(dialog.aiAssistantBox() && dialog.auroraLinkBox());
  QCOMPARE(dialog.aiAssistantChecked(), false);
  QCOMPARE(dialog.auroraLinkChecked(), false);
  // nothing was requested by opening the dialog
  QSignalSpy aiSpy(&dialog, &SetupDialog::aiAssistantRequested);
  QSignalSpy auroraSpy(&dialog, &SetupDialog::auroraLinkRequested);
  dialog.setAiAssistant(false, ControlIndicator::textFor(ControlIndicator::Off, QStringList()));
  dialog.setAuroraLink(false, AuroraIndicator::textFor(AuroraIndicator::Off));
  QCOMPARE(aiSpy.count(), 0);
  QCOMPARE(auroraSpy.count(), 0);
}

void TestSetupDialog::connectionsShowTheStateTheWindowGives()
{
  SetupDialog dialog;
  // on, then off again: the switch and the line follow the window both ways, with no request
  QSignalSpy aiSpy(&dialog, &SetupDialog::aiAssistantRequested);
  QSignalSpy auroraSpy(&dialog, &SetupDialog::auroraLinkRequested);
  dialog.setAiAssistant(true, ControlIndicator::textFor(ControlIndicator::Waiting, QStringList()));
  QCOMPARE(dialog.aiAssistantChecked(), true);
  QCOMPARE(dialog.aiAssistantStatus(), QStringLiteral("AI Assistant on, waiting for an AI"));
  dialog.setAiAssistant(true, ControlIndicator::textFor(ControlIndicator::Connected, QStringList{QStringLiteral("Claude")}));
  QCOMPARE(dialog.aiAssistantStatus(), QStringLiteral("AI Assistant: Claude"));
  dialog.setAiAssistant(false, ControlIndicator::textFor(ControlIndicator::Off, QStringList()));
  QCOMPARE(dialog.aiAssistantChecked(), false);
  QCOMPARE(dialog.aiAssistantStatus(), QStringLiteral("AI Assistant off"));

  dialog.setAuroraLink(true, AuroraIndicator::textFor(AuroraIndicator::Connecting));
  QCOMPARE(dialog.auroraLinkChecked(), true);
  QCOMPARE(dialog.auroraLinkStatus(), QStringLiteral("Waiting for Aurora"));
  dialog.setAuroraLink(true, AuroraIndicator::textFor(AuroraIndicator::Live));
  QCOMPARE(dialog.auroraLinkChecked(), true);
  QCOMPARE(dialog.auroraLinkStatus(), QStringLiteral("Aurora live"));
  dialog.setAuroraLink(false, AuroraIndicator::textFor(AuroraIndicator::Off));
  QCOMPARE(dialog.auroraLinkChecked(), false);
  QCOMPARE(dialog.auroraLinkStatus(), QStringLiteral("Aurora off"));
  QCOMPARE(aiSpy.count(), 0);
  QCOMPARE(auroraSpy.count(), 0);
}

void TestSetupDialog::connectionsClicksSendRequestsAndNothingElse()
{
  SetupDialog dialog;
  QSignalSpy aiSpy(&dialog, &SetupDialog::aiAssistantRequested);
  QSignalSpy auroraSpy(&dialog, &SetupDialog::auroraLinkRequested);
  QSignalSpy expertSpy(&dialog, &SetupDialog::expertModeRequested);
  QSignalSpy hintsSpy(&dialog, &SetupDialog::hintsRequested);
  dialog.aiAssistantBox()->click();
  QCOMPARE(aiSpy.count(), 1);
  QCOMPARE(aiSpy.at(0).at(0).toBool(), true);
  QCOMPARE(auroraSpy.count(), 0);
  dialog.aiAssistantBox()->click();
  QCOMPARE(aiSpy.count(), 2);
  QCOMPARE(aiSpy.at(1).at(0).toBool(), false);
  dialog.auroraLinkBox()->click();
  QCOMPARE(auroraSpy.count(), 1);
  QCOMPARE(auroraSpy.at(0).at(0).toBool(), true);
  dialog.auroraLinkBox()->click();
  QCOMPARE(auroraSpy.count(), 2);
  QCOMPARE(auroraSpy.at(1).at(0).toBool(), false);
  QCOMPARE(aiSpy.count(), 2);
  QCOMPARE(expertSpy.count(), 0);
  QCOMPARE(hintsSpy.count(), 0);
  // a request the window could not carry out (the AI Assistant could not start): the window
  // shows the real state, which is off, and the switch goes back
  dialog.aiAssistantBox()->click();
  QCOMPARE(dialog.aiAssistantChecked(), true);
  dialog.setAiAssistant(false, ControlIndicator::textFor(ControlIndicator::Off, QStringList()));
  QCOMPARE(dialog.aiAssistantChecked(), false);
  QCOMPARE(aiSpy.count(), 3);
}

void TestSetupDialog::connectionsStayAfterEverythingElse()
{
  SetupDialog dialog;
  dialog.show();
  QVERIFY(QTest::qWaitForWindowExposed(&dialog));
  QVERIFY(dialog.aiAssistantBox()->geometry().top() > dialog.luaSwitchBox()->geometry().bottom());
  QVERIFY(dialog.auroraLinkBox()->geometry().top() > dialog.aiAssistantBox()->geometry().bottom());
  QList<QRect> before;
  for (QWidget* w : dialog.findChildren<QWidget*>()) if (w->objectName().isEmpty() || !w->objectName().startsWith(QLatin1String("setupAi"))) before << w->geometry();
  dialog.setAiAssistant(true, ControlIndicator::textFor(ControlIndicator::Waiting, QStringList()));
  QCoreApplication::processEvents();
  QList<QRect> after;
  for (QWidget* w : dialog.findChildren<QWidget*>()) if (w->objectName().isEmpty() || !w->objectName().startsWith(QLatin1String("setupAi"))) after << w->geometry();
  QCOMPARE(after.size(), before.size());
  // the dialog fits the screen
  const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
  QVERIFY(dialog.height() <= screen.height());
}

void TestSetupDialog::connectionsStringsAreTheStatusBarsOwn()
{
  SetupDialog dialog;
  QCOMPARE(dialog.aiAssistantBox()->text(), QStringLiteral("AI Assistant"));
  QCOMPARE(dialog.auroraLinkBox()->text(), QStringLiteral("Live link to Aurora"));
  QCOMPARE(QString::fromLatin1(SetupText::connectionsLabel), QStringLiteral("Connections"));
  // the status lines are exactly the words of the status bar for every state
  const QList<ControlIndicator::State> aiStates{ControlIndicator::Off, ControlIndicator::Waiting, ControlIndicator::Problem};
  for (ControlIndicator::State state : aiStates)
  {
    dialog.setAiAssistant(state != ControlIndicator::Off, ControlIndicator::textFor(state, QStringList()));
    QCOMPARE(dialog.aiAssistantStatus(), ControlIndicator::textFor(state, QStringList()));
  }
  QCOMPARE(ControlIndicator::textFor(ControlIndicator::Off, QStringList()), QStringLiteral("AI Assistant off"));
  for (AuroraIndicator::State state : {AuroraIndicator::Off, AuroraIndicator::Connecting, AuroraIndicator::Live})
  {
    dialog.setAuroraLink(state != AuroraIndicator::Off, AuroraIndicator::textFor(state));
    QCOMPARE(dialog.auroraLinkStatus(), AuroraIndicator::textFor(state));
  }
}

// the window itself cannot be started in a test; its source must connect the two switches to the
// code the status bar switches call, and show the real state again after every request
void TestSetupDialog::windowConnectsSetupToTheSameCodeAsTheStatusBar()
{
  const QString source = WindowSource::read(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp"));
  QVERIFY(!source.isEmpty());
  const int begin = source.indexOf(QStringLiteral("void PinglyMotionWindow::openSetupDialog()"));
  QVERIFY(begin > 0);
  const QString body = source.mid(begin, source.indexOf(QStringLiteral("\n}\n"), begin) - begin);
  // AI Assistant: the code behind the status bar button (turn on and off, key file, title)
  QVERIFY(body.contains(QStringLiteral("&SetupDialog::aiAssistantRequested")));
  QVERIFY(body.contains(QStringLiteral("setRemoteControl(on);")));
  QVERIFY(source.contains(QStringLiteral("connect(m_controlIndicator,&ControlIndicator::turnOnRequested,this,[this](){ setRemoteControl(true); });")));
  // Aurora: the action the status bar switch toggles, which drives AuroraLink::setEnabled
  QVERIFY(body.contains(QStringLiteral("&SetupDialog::auroraLinkRequested")));
  QVERIFY(body.contains(QStringLiteral("m_auroraAction->setChecked(on);")));
  QVERIFY(source.contains(QStringLiteral("connect(m_auroraIndicator,&AuroraIndicator::toggleRequested,m_auroraAction,&QAction::toggle);")));
  QVERIFY(source.contains(QStringLiteral("m_auroraLink->setEnabled(on); updateWindowTitle();")));
  // the real state, read from the server, the link and the status bar's own text
  QVERIFY(body.contains(QStringLiteral("dialog.setAiAssistant(m_controlServer->running(),m_controlIndicator->text());")));
  QVERIFY(body.contains(QStringLiteral("dialog.setAuroraLink(m_auroraLink->enabled(),m_auroraIndicator->text());")));
  QVERIFY(body.contains(QStringLiteral("AuroraLink::connectedChanged")));
  QVERIFY(body.contains(QStringLiteral("ControlServer::contact")));
  // and the switches never turn on by themselves: setRemoteControl(true) only for --control
  QCOMPARE(body.count(QStringLiteral("setRemoteControl(true)")), 0);
}

QTEST_MAIN(TestSetupDialog)
#include "tst_setupdialog.moc"
