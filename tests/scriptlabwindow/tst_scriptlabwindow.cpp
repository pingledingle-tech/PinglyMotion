/*
 * PinglyMotion - Script Lab as one window with its own tabs (job UX-B, docs/UX_DECISIONS.md 6x)
 *
 * The project owner, 24 November 2026, 13:26: no tabs on the screen edges; the tabs live inside
 * Script Lab, one window, big by default, it can be the main focus or float anywhere, and each tab
 * can be pulled out into its own window and put back. Checks, exactly:
 *  - the four tabs, their order, names, tips and icons, and every part of the panel in one of them;
 *  - Ctrl+Tab and Ctrl+Shift+Tab go round the tabs from any field, also the code editor;
 *  - Pop out (button, keys and dragging a tab out of the bar) and Put back (button and closing the
 *    window); the last tab stays and the button says why;
 *  - the tab in front, the popped tabs and their places come back after a restart (also through the
 *    settings file), and a broken saved text is read safely;
 *  - Script Lab opens big docked and floating (once each), docks on any side, Reset gives the first
 *    state back; popped windows hide with Script Lab and come back with it;
 *  - nothing has a fixed size, the tree beside the editor follows a popped Live page's width;
 *  - nothing is clipped at 100 and 200 percent text, dark and light.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QStandardPaths>
#include <QTabBar>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

#include "channelmapview.h"
#include "labbeginner.h"
#include "labeditor.h"
#include "labplayer.h"
#include "labtabs.h"
#include "labwatcher.h"
#include "rlvstate.h"
#include "scriptlabpanel.h"
#include "scriptlabsection.h"
#include "textsize.h"
#include "theme.h"
#include "uirender.h"
#include "usersettings.h"

class TestScriptLabWindow : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();
    void tabsInOrder();
    void everyPartIsInATab();
    void ctrlTabGoesRound();
    void popOutAndPutBack();
    void popOutByKeys();
    void dragATabOut();
    void lastTabStays();
    void stateComesBackAfterARestart();
    void stateThroughTheSettingsFile();
    void brokenStateIsReadSafely();
    void opensBigDockedAndFloating();
    void docksOnAnySide();
    void poppedWindowsFollowScriptLab();
    void resetPutsEverythingBack();
    void noFixedSizes();
    void treeFollowsPoppedLiveWidth();
    void noClipping();

  private:
    QTemporaryDir m_settingsDir;
    static void settle()
    {
      for (int i = 0; i < 4; ++i)
      {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
      }
    }
    static QRect screenArea() { return QGuiApplication::primaryScreen()->availableGeometry(); }
};

void TestScriptLabWindow::initTestCase()
{
  Theme::apply(true);
  // the settings go to a folder of the test's own, never the user's
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestScriptLabWindow::init()
{
  UserSettings::setScriptLabWindows(QString());
  UserSettings::setScriptLabSized(0);
}

void TestScriptLabWindow::cleanup()
{
  init();
  settle();
}

void TestScriptLabWindow::tabsInOrder()
{
  ScriptLabPanel panel;
  QTabWidget* tabs = panel.tabs();
  QCOMPARE(tabs->count(), 5);
  const QStringList names = {"Live scripts", "Type commands", "Who talks to whom", "Player", "Improve a setup"};
  const QStringList keys = {"live", "commands", "map", "player", "improve"};
  const QStringList tips = {"Live: your .lsl scripts run each time you save them", "Commands: type RLV lines by hand",
                            "Arrows between the parts: who sends what to whom.", "Step back and forth through your recorded steps.",
                            "Check a setup you made before and see what can be better."};
  for (int p = 0; p < ScriptLabPanel::PageCount; ++p)
  {
    const auto page = ScriptLabPanel::Page(p);
    QCOMPARE(panel.tabIndex(page), p);
    QCOMPARE(tabs->widget(p), panel.page(page));
    QCOMPARE(tabs->tabText(p), names.at(p));
    QCOMPARE(panel.pageName(page), names.at(p));
    QCOMPARE(ScriptLabPanel::pageKey(page), keys.at(p));
    QCOMPARE(tabs->tabToolTip(p), tips.at(p));
    QVERIFY(!tabs->tabIcon(p).isNull());
    QVERIFY(qobject_cast<QScrollArea*>(panel.page(page)));   // every page scrolls, none forces the size up
    QVERIFY(!panel.isPoppedOut(page));
  }
  QCOMPARE(panel.currentPage(), ScriptLabPanel::LivePage);
  QCOMPARE(tabs->accessibleName(), QString("Script Lab pages"));
  // Pop out beside the tabs, with its words and a name for screen readers
  QToolButton* pop = panel.popOutButton();
  QVERIFY(pop);
  QCOMPARE(tabs->cornerWidget(Qt::TopRightCorner), pop);
  QCOMPARE(pop->text(), QString("Pop out"));
  QCOMPARE(pop->accessibleName(), QString("Pop out this tab"));
  QCOMPARE(pop->toolTip(), QString("Pop out \"Live scripts\" into a window of its own. Close that window, or press Put back, to bring it back."));
  QVERIFY(!pop->icon().isNull());
  QCOMPARE(pop->focusPolicy(), Qt::StrongFocus);
  tabs->setCurrentIndex(ScriptLabPanel::MapPage);
  QCOMPARE(pop->toolTip(), QString("Pop out \"Who talks to whom\" into a window of its own. Close that window, or press Put back, to bring it back."));
}

void TestScriptLabWindow::everyPartIsInATab()
{
  ScriptLabPanel panel;
  // every folding part has one page, as the owner listed them; nothing is lost
  const QMap<QString, ScriptLabPanel::Page> where = {
    {"tree", ScriptLabPanel::LivePage}, {"script", ScriptLabPanel::LivePage}, {"problems", ScriptLabPanel::LivePage},
    {"log", ScriptLabPanel::LivePage}, {"restrictionsNow", ScriptLabPanel::LivePage}, {"objects", ScriptLabPanel::LivePage},
    {"input", ScriptLabPanel::LivePage}, {"inventory", ScriptLabPanel::LivePage},
    {"restrictions", ScriptLabPanel::CommandsPage}, {"actions", ScriptLabPanel::CommandsPage},
    {"sphere", ScriptLabPanel::CommandsPage}, {"replies", ScriptLabPanel::CommandsPage},
    {"map", ScriptLabPanel::MapPage}, {"recipe", ScriptLabPanel::PlayerPage}};
  QCOMPARE(panel.sections().size(), where.size());
  for (ScriptLabSection* section : panel.sections())
  {
    QVERIFY2(where.contains(section->key()), qPrintable(section->key()));
    int pages = 0;
    for (int p = 0; p < ScriptLabPanel::PageCount; ++p)
      if (panel.page(ScriptLabPanel::Page(p))->isAncestorOf(section)) ++pages;
    QVERIFY2(pages == 1, qPrintable(section->key()));
    QVERIFY2(panel.page(where.value(section->key()))->isAncestorOf(section), qPrintable(section->key()));
  }
  // and the controls that are not inside a folding part
  const QList<QPair<QWidget*, ScriptLabPanel::Page>> controls = {
    {panel.scriptView(), ScriptLabPanel::LivePage},
    {panel.liveEmpty(), ScriptLabPanel::LivePage}, {panel.liveFolderField(), ScriptLabPanel::LivePage},
    {panel.liveStatus(), ScriptLabPanel::LivePage}, {panel.runAgainButton(), ScriptLabPanel::LivePage},
    {panel.openEditorButton(), ScriptLabPanel::LivePage}, {panel.effectsBox(), ScriptLabPanel::LivePage},
    {panel.editor(), ScriptLabPanel::LivePage}, {panel.stepCard(), ScriptLabPanel::LivePage},
    {panel.commandField(), ScriptLabPanel::CommandsPage}, {panel.objectField(), ScriptLabPanel::CommandsPage},
    {panel.runButton(), ScriptLabPanel::CommandsPage}, {panel.resetButton(), ScriptLabPanel::CommandsPage},
    {panel.cameraBox(), ScriptLabPanel::CommandsPage}, {panel.viewModesBox(), ScriptLabPanel::CommandsPage},
    {panel.channelMap(), ScriptLabPanel::MapPage}, {panel.mapEmpty(), ScriptLabPanel::MapPage},
    {panel.recordButton(), ScriptLabPanel::PlayerPage}, {panel.recipeStatus(), ScriptLabPanel::PlayerPage},
    {panel.player(), ScriptLabPanel::PlayerPage}, {panel.playerEmpty(), ScriptLabPanel::PlayerPage}};
  for (const auto& c : controls)
  {
    QVERIFY(c.first);
    QVERIFY2(panel.page(c.second)->isAncestorOf(c.first), qPrintable(c.first->objectName()));
  }
  // the Viewer rules row and the ? button stay above the tabs, for every page
  for (int p = 0; p < ScriptLabPanel::PageCount; ++p)
  {
    QVERIFY(!panel.page(ScriptLabPanel::Page(p))->isAncestorOf(panel.modeRlvButton()));
    QVERIFY(!panel.page(ScriptLabPanel::Page(p))->isAncestorOf(panel.helpButton()));
  }
  // the empty lines of the new tabs say what fills them
  QCOMPARE(panel.mapEmpty()->text(), QString("Watch a folder as one object on Live scripts. Then this page shows who talks to whom."));
  QCOMPARE(panel.playerEmpty()->text(),
           QString("Watch a folder as one object on Live scripts. Then press Record what I do here, and watch your steps again."));
}

void TestScriptLabWindow::ctrlTabGoesRound()
{
  // a watched folder, so the code editor is on screen
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  {
    QFile file(dir.filePath("hello.lsl"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("default\n{\n    state_entry()\n    {\n        llOwnerSay(\"hi\");\n    }\n}\n");
  }
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QVERIFY(panel.watchFolder(dir.path()));
  panel.resize(700, 600);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.activateWindow();
  QTabWidget* tabs = panel.tabs();
  // from the command field on Type commands
  tabs->setCurrentIndex(ScriptLabPanel::CommandsPage);
  panel.commandField()->setFocus();
  QTRY_VERIFY(panel.commandField()->hasFocus());
  QTest::keyClick(panel.commandField(), Qt::Key_Tab, Qt::ControlModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::MapPage));
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab, Qt::ControlModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::PlayerPage));
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab, Qt::ControlModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::ImprovePage));
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab, Qt::ControlModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::LivePage));   // round to the first
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_Backtab, Qt::ControlModifier | Qt::ShiftModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::ImprovePage));   // and back
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_Backtab, Qt::ControlModifier | Qt::ShiftModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::PlayerPage));
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_PageUp, Qt::ControlModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::MapPage));
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_PageDown, Qt::ControlModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::PlayerPage));
  // from inside the code editor, which takes Tab itself
  tabs->setCurrentIndex(ScriptLabPanel::LivePage);
  panel.editor()->codeEdit()->setFocus();
  QTRY_VERIFY(panel.editor()->codeEdit()->hasFocus());
  const QString before = panel.editor()->codeEdit()->toPlainText();
  QTest::keyClick(panel.editor()->codeEdit(), Qt::Key_Tab, Qt::ControlModifier);
  QCOMPARE(tabs->currentIndex(), int(ScriptLabPanel::CommandsPage));
  QCOMPARE(panel.editor()->codeEdit()->toPlainText(), before);   // no tab typed into the script
  // the Pop out button is in the Tab order
  QVERIFY(panel.popOutButton()->focusPolicy() & Qt::TabFocus);
}

void TestScriptLabWindow::popOutAndPutBack()
{
  ScriptLabPanel panel;
  panel.resize(700, 600);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::MapPage);
  QTest::mouseClick(panel.popOutButton(), Qt::LeftButton);
  settle();
  QVERIFY(panel.isPoppedOut(ScriptLabPanel::MapPage));
  LabPopWindow* window = panel.popWindow(ScriptLabPanel::MapPage);
  QVERIFY(window);
  QVERIFY(window->isWindow());
  QVERIFY(window->isVisible());
  QCOMPARE(window->windowTitle(), QString("Script Lab: Who talks to whom"));
  QCOMPARE(window->accessibleName(), QString("Script Lab: Who talks to whom"));
  QCOMPARE(window->titleLabel()->text(), QString("Who talks to whom"));
  QCOMPARE(window->page(), panel.page(ScriptLabPanel::MapPage));
  QVERIFY(window->isAncestorOf(panel.mapSection()));
  QCOMPARE(window->putBackButton()->text(), QString("Put back"));
  QCOMPARE(window->putBackButton()->accessibleName(), QString("Put this tab back into Script Lab"));
  QCOMPARE(window->putBackButton()->toolTip(), QString("Put this tab back into Script Lab. Closing this window does the same."));
  QVERIFY(!window->putBackButton()->icon().isNull());
  // the tab left the bar; the others keep their order
  QCOMPARE(panel.tabs()->count(), 4);
  QCOMPARE(panel.tabIndex(ScriptLabPanel::MapPage), -1);
  QCOMPARE(panel.tabs()->tabText(0), QString("Live scripts"));
  QCOMPARE(panel.tabs()->tabText(1), QString("Type commands"));
  QCOMPARE(panel.tabs()->tabText(2), QString("Player"));
  QCOMPARE(panel.tabIndex(ScriptLabPanel::PlayerPage), 2);
  // big and on the screen, and as the remembered state says
  const QRect area = screenArea();
  QVERIFY(area.contains(window->geometry()));
  QVERIFY(window->width() >= std::min(area.width(), ScriptLabPanel::minimumPanelSize().width()));
  QVERIFY(UserSettings::scriptLabWindows().contains("pop=map"));
  // showPage brings the popped window to the front, not a tab
  panel.showPage(ScriptLabPanel::MapPage);
  QVERIFY(window->isVisible());
  QCOMPARE(panel.tabs()->count(), 4);

  // Put back: the tab comes back in its place and in front, the window goes
  QPointer<LabPopWindow> gone = window;
  QTest::mouseClick(window->putBackButton(), Qt::LeftButton);
  settle();
  QVERIFY(!gone);
  QVERIFY(!panel.isPoppedOut(ScriptLabPanel::MapPage));
  QCOMPARE(panel.tabs()->count(), 5);
  QCOMPARE(panel.tabIndex(ScriptLabPanel::MapPage), 2);
  QCOMPARE(panel.tabs()->tabText(2), QString("Who talks to whom"));
  QCOMPARE(panel.tabs()->tabToolTip(2), QString("Arrows between the parts: who sends what to whom."));
  QVERIFY(!panel.tabs()->tabIcon(2).isNull());
  QCOMPARE(panel.currentPage(), ScriptLabPanel::MapPage);
  QVERIFY(panel.page(ScriptLabPanel::MapPage)->isAncestorOf(panel.mapSection()));
  QVERIFY(!UserSettings::scriptLabWindows().contains("pop="));
  QVERIFY(!panel.putBack(ScriptLabPanel::MapPage));   // it is in already

  // closing the window puts the tab back too, and the place is kept for next time
  QVERIFY(panel.popOut(ScriptLabPanel::LivePage));
  window = panel.popWindow(ScriptLabPanel::LivePage);
  QVERIFY(window && window->isVisible());
  QVERIFY(!panel.popOut(ScriptLabPanel::LivePage));   // out already
  const QRect place(area.left() + 20, area.top() + 30, std::min(500, area.width() - 40), std::min(400, area.height() - 40));
  window->setGeometry(place);
  settle();
  QCOMPARE(window->geometry(), place);
  gone = window;
  window->close();
  settle();
  QVERIFY(!gone);
  QCOMPARE(panel.tabIndex(ScriptLabPanel::LivePage), 0);
  QVERIFY(UserSettings::scriptLabWindows().contains(QStringLiteral("place=live:%1,%2,%3,%4")
                                                     .arg(place.x()).arg(place.y()).arg(place.width()).arg(place.height())));
  QVERIFY(panel.popOut(ScriptLabPanel::LivePage));
  QCOMPARE(panel.popWindow(ScriptLabPanel::LivePage)->geometry(), place);   // where it was
  QVERIFY(panel.putBack(ScriptLabPanel::LivePage));
}

void TestScriptLabWindow::popOutByKeys()
{
  ScriptLabPanel panel;
  panel.resize(700, 600);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::PlayerPage);
  panel.popOutButton()->setFocus();
  QTest::keyClick(panel.popOutButton(), Qt::Key_Space);
  settle();
  QVERIFY(panel.isPoppedOut(ScriptLabPanel::PlayerPage));
  LabPopWindow* window = panel.popWindow(ScriptLabPanel::PlayerPage);
  // Put back is in the window's Tab order and works with Space too
  QVERIFY(window->putBackButton()->focusPolicy() & Qt::TabFocus);
  window->putBackButton()->setFocus();
  QTest::keyClick(window->putBackButton(), Qt::Key_Space);
  settle();
  QVERIFY(!panel.isPoppedOut(ScriptLabPanel::PlayerPage));
  QCOMPARE(panel.tabIndex(ScriptLabPanel::PlayerPage), 3);
}

void TestScriptLabWindow::dragATabOut()
{
  ScriptLabPanel panel;
  panel.resize(700, 600);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  QTabBar* bar = panel.tabs()->tabBar();
  LabTabBar* labBar = qobject_cast<LabTabBar*>(bar);
  QVERIFY(labBar);
  const QPoint start = bar->tabRect(ScriptLabPanel::CommandsPage).center();
  // let go just under the bar: a click, not a pull
  QTest::mousePress(bar, Qt::LeftButton, Qt::NoModifier, start);
  QTest::mouseRelease(bar, Qt::LeftButton, Qt::NoModifier, start + QPoint(0, bar->height()));
  settle();
  QVERIFY(!panel.isPoppedOut(ScriptLabPanel::CommandsPage));
  QCOMPARE(panel.currentPage(), ScriptLabPanel::CommandsPage);
  // let go well outside the bar: the tab gets a window of its own
  QTest::mousePress(bar, Qt::LeftButton, Qt::NoModifier, start);
  QTest::mouseRelease(bar, Qt::LeftButton, Qt::NoModifier, start + QPoint(0, labBar->pullDistance() + bar->height() + 20));
  settle();
  QVERIFY(panel.isPoppedOut(ScriptLabPanel::CommandsPage));
  QVERIFY(panel.popWindow(ScriptLabPanel::CommandsPage)->isAncestorOf(panel.commandField()));
  QCOMPARE(panel.tabs()->count(), 4);
  QVERIFY(screenArea().contains(panel.popWindow(ScriptLabPanel::CommandsPage)->geometry()));
  // the right button never pulls a tab out
  const QPoint live = bar->tabRect(0).center();
  QTest::mousePress(bar, Qt::RightButton, Qt::NoModifier, live);
  QTest::mouseRelease(bar, Qt::RightButton, Qt::NoModifier, live + QPoint(0, 400));
  settle();
  QVERIFY(!panel.isPoppedOut(ScriptLabPanel::LivePage));
  QVERIFY(panel.putBack(ScriptLabPanel::CommandsPage));
  QCOMPARE(panel.tabIndex(ScriptLabPanel::CommandsPage), 1);
}

void TestScriptLabWindow::lastTabStays()
{
  ScriptLabPanel panel;
  panel.resize(700, 600);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  QVERIFY(panel.popOut(ScriptLabPanel::LivePage));
  QVERIFY(panel.popOut(ScriptLabPanel::MapPage));
  QVERIFY(panel.popOut(ScriptLabPanel::PlayerPage));
  QVERIFY(panel.popOut(ScriptLabPanel::ImprovePage));
  QCOMPARE(panel.tabs()->count(), 1);
  QCOMPARE(panel.currentPage(), ScriptLabPanel::CommandsPage);
  QVERIFY(!panel.popOut(ScriptLabPanel::CommandsPage));
  QVERIFY(!panel.popOutButton()->isEnabled());
  QCOMPARE(panel.popOutButton()->toolTip(), QString("The last tab stays in Script Lab. Put another tab back first."));
  QCOMPARE(panel.popOutButton()->accessibleDescription(), panel.popOutButton()->toolTip());
  // Ctrl+Tab with one tab changes nothing
  panel.commandField()->setFocus();
  QTest::keyClick(panel.commandField(), Qt::Key_Tab, Qt::ControlModifier);
  QCOMPARE(panel.currentPage(), ScriptLabPanel::CommandsPage);
  // put back in any order: each goes to its own place
  QVERIFY(panel.putBack(ScriptLabPanel::PlayerPage));
  QVERIFY(panel.popOutButton()->isEnabled());
  QVERIFY(panel.putBack(ScriptLabPanel::LivePage));
  QVERIFY(panel.putBack(ScriptLabPanel::MapPage));
  QVERIFY(panel.putBack(ScriptLabPanel::ImprovePage));
  for (int p = 0; p < ScriptLabPanel::PageCount; ++p) QCOMPARE(panel.tabIndex(ScriptLabPanel::Page(p)), p);
}

void TestScriptLabWindow::stateComesBackAfterARestart()
{
  const QRect area = screenArea();
  const QRect place(area.left() + 40, area.top() + 20, std::min(520, area.width() - 60), std::min(380, area.height() - 40));
  {
    ScriptLabPanel panel;
    panel.resize(700, 600);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    QVERIFY(panel.popOut(ScriptLabPanel::PlayerPage));
    panel.popWindow(ScriptLabPanel::PlayerPage)->setGeometry(place);
    settle();
    panel.tabs()->setCurrentIndex(panel.tabIndex(ScriptLabPanel::CommandsPage));
    QCOMPARE(UserSettings::scriptLabWindows(),
             QStringLiteral("tab=commands;pop=player;place=player:%1,%2,%3,%4").arg(place.x()).arg(place.y()).arg(place.width()).arg(place.height()));
  }   // the program closes: the popped window goes with it, the state stays
  settle();
  ScriptLabPanel again;
  QVERIFY(again.isPoppedOut(ScriptLabPanel::PlayerPage));
  QCOMPARE(again.currentPage(), ScriptLabPanel::CommandsPage);
  QCOMPARE(again.tabs()->count(), 4);
  LabPopWindow* window = again.popWindow(ScriptLabPanel::PlayerPage);
  QCOMPARE(window->geometry(), place);
  QVERIFY(!window->isVisible());   // shown with Script Lab, not before
  again.resize(700, 600);
  again.show();
  QVERIFY(QTest::qWaitForWindowExposed(&again));
  QVERIFY(window->isVisible());
  QCOMPARE(window->geometry(), place);
  QVERIFY(window->isAncestorOf(again.recordButton()));
}

void TestScriptLabWindow::stateThroughTheSettingsFile()
{
  UserSettings::setScriptLabWindows("tab=map;pop=live;place=live:10,20,600,400");
  UserSettings::setScriptLabSized(3);
  UserSettings::writeSettings();
  UserSettings::setScriptLabWindows(QString());
  UserSettings::setScriptLabSized(0);
  UserSettings::readSettings();
  QCOMPARE(UserSettings::scriptLabWindows(), QString("tab=map;pop=live;place=live:10,20,600,400"));
  QCOMPARE(UserSettings::scriptLabSized(), 3);
}

void TestScriptLabWindow::brokenStateIsReadSafely()
{
  const QStringList known = {"live", "commands", "map", "player", "improve"};
  // the round trip
  LabTabState::State state;
  state.current = "map";
  state.popped = {"player", "live"};
  state.places.insert("live", QRect(-100, 20, 640, 480));
  QCOMPARE(LabTabState::parse(LabTabState::format(state), known), state);
  // unknown keys, broken places, doubles and junk are left out; the rest is taken
  const LabTabState::State read = LabTabState::parse(
    "tab=nothing;pop=map;pop=map;pop=../etc;place=map:1,2,3;place=map:1,2,0,5;place=hud:1,2,3,4;"
    "place=player:9999999,1,2,3;place=live:x,1,2,3;=;;junk;tab;place=commands:5,6,700,500", known);
  QCOMPARE(read.current, QString());
  QCOMPARE(read.popped, QStringList({"map"}));
  QCOMPARE(read.places.size(), 1);
  QCOMPARE(read.places.value("commands"), QRect(5, 6, 700, 500));
  // a long text is cut, and a panel given junk starts as at the first start
  QVERIFY(LabTabState::parse(QString(5000, QLatin1Char('x')), known) == LabTabState::State());
  UserSettings::setScriptLabWindows("tab=;pop=;place=::::;\x01\x02");
  ScriptLabPanel panel;
  QCOMPARE(panel.tabs()->count(), 5);
  QCOMPARE(panel.currentPage(), ScriptLabPanel::LivePage);
  // a place on a screen that is gone comes back on a screen
  const QRect far(50000, 50000, 600, 400);
  QVERIFY(screenArea().contains(LabPopWindow::fitOnScreen(far)));
  const QRect huge(0, 0, 90000, 90000);
  QVERIFY(screenArea().contains(LabPopWindow::fitOnScreen(huge)));
}

void TestScriptLabWindow::opensBigDockedAndFloating()
{
  const QRect area = screenArea();
  QMainWindow window;
  window.setCentralWidget(new QWidget);
  window.resize(std::min(1400, area.width() - 40), std::min(900, area.height() - 40));
  ScriptLabPanel* panel = new ScriptLabPanel;
  QDockWidget* dock = ScriptLabPanel::makeDock(&window, panel);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QVERIFY(dock->isHidden());
  QCOMPARE(UserSettings::scriptLabSized(), 0);
  // shown by a saved layout (not by the user): it keeps the saved size
  dock->show();
  settle();
  QCOMPARE(UserSettings::scriptLabSized(), 0);
  dock->hide();
  // the first open by the user (View > Script Lab, the top bar, Ctrl+Shift+L): most of the main window
  dock->toggleViewAction()->trigger();
  settle();
  QCOMPARE(UserSettings::scriptLabSized() & 1, 1);
  const int big = ScriptLabPanel::bigDockedWidth(window.size());
  QVERIFY(big >= window.width() / 2 || big >= ScriptLabPanel::defaultSize().width());
  QVERIFY2(dock->width() >= std::min(big, window.width() - ScriptLabPanel::minimumPanelSize().width()) - 20,
           qPrintable(QStringLiteral("%1 of %2, wanted %3").arg(dock->width()).arg(window.width()).arg(big)));
  // the user makes it smaller: the next open keeps that
  window.resizeDocks({dock}, {ScriptLabPanel::minimumPanelSize().width() + 40}, Qt::Horizontal);
  settle();
  const int small = dock->width();
  dock->toggleViewAction()->trigger();
  QVERIFY(dock->isHidden());
  dock->toggleViewAction()->trigger();
  settle();
  QCOMPARE(dock->width(), small);
  QVERIFY(!ScriptLabPanel::openBig(&window, dock));
  // a saved layout that floats it keeps its own place and size
  ScriptLabPanel::setLayoutRestoring(true);
  dock->setFloating(true);
  ScriptLabPanel::setLayoutRestoring(false);
  settle();
  QCOMPARE(UserSettings::scriptLabSized(), 1);
  dock->setFloating(false);
  settle();
  // the first float by the user: most of the screen, in its middle, on the screen
  dock->setFloating(true);
  settle();
  QCOMPARE(UserSettings::scriptLabSized(), 3);
  const QSize floating = ScriptLabPanel::bigFloatingSize(area);
  QCOMPARE(floating, QSize(area.width() * 4 / 5, area.height() * 4 / 5).expandedTo(ScriptLabPanel::minimumPanelSize()).boundedTo(area.size()));
  QCOMPARE(dock->size(), floating);
  QVERIFY(area.contains(dock->geometry()));
  // a moved and resized floating Script Lab stays as the user left it
  const QRect place(area.left() + 10, area.top() + 10, std::min(600, area.width() - 20), std::min(450, area.height() - 20));
  dock->setGeometry(place);
  dock->setFloating(false);
  dock->setFloating(true);
  dock->setGeometry(place);
  settle();
  QCOMPARE(dock->geometry(), place);
  // never bigger than the room there is
  QVERIFY(ScriptLabPanel::bigDockedWidth(QSize(500, 400)) <= std::max(500 - TextSize::scaled(240), ScriptLabPanel::minimumPanelSize().width()));
  QVERIFY(ScriptLabPanel::bigFloatingSize(QRect(0, 0, 200, 150)).width() <= 200);
}

void TestScriptLabWindow::docksOnAnySide()
{
  QMainWindow window;
  window.setCentralWidget(new QWidget);
  const QRect area = screenArea();
  window.resize(std::min(1200, area.width() - 40), std::min(800, area.height() - 40));
  ScriptLabPanel* panel = new ScriptLabPanel;
  QDockWidget* dock = ScriptLabPanel::makeDock(&window, panel);
  QCOMPARE(dock->allowedAreas(), Qt::DockWidgetAreas(Qt::AllDockWidgetAreas));
  const QDockWidget::DockWidgetFeatures wanted =
      QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable;
  QCOMPARE(dock->features() & wanted, wanted);
  window.show();
  dock->show();
  for (Qt::DockWidgetArea side : {Qt::LeftDockWidgetArea, Qt::TopDockWidgetArea, Qt::BottomDockWidgetArea, Qt::RightDockWidgetArea})
  {
    window.addDockWidget(side, dock);
    settle();
    QCOMPARE(window.dockWidgetArea(dock), side);
    QVERIFY(dock->isVisible());
  }
}

void TestScriptLabWindow::poppedWindowsFollowScriptLab()
{
  QMainWindow window;
  window.setCentralWidget(new QWidget);
  const QRect area = screenArea();
  window.resize(std::min(1200, area.width() - 40), std::min(800, area.height() - 40));
  ScriptLabPanel* panel = new ScriptLabPanel;
  QDockWidget* dock = ScriptLabPanel::makeDock(&window, panel);
  window.show();
  // popped while Script Lab is closed: the window waits for Script Lab
  QVERIFY(panel->popOut(ScriptLabPanel::MapPage));
  LabPopWindow* popped = panel->popWindow(ScriptLabPanel::MapPage);
  QVERIFY(!popped->isVisible());
  dock->toggleViewAction()->trigger();   // View > Script Lab
  settle();
  QVERIFY(dock->isVisible());
  QVERIFY(popped->isVisible());
  // closed: its windows go too, and the tab is still out (nothing is lost)
  dock->close();
  settle();
  QVERIFY(!popped->isVisible());
  QVERIFY(panel->isPoppedOut(ScriptLabPanel::MapPage));
  dock->toggleViewAction()->trigger();
  settle();
  QVERIFY(popped->isVisible());
  // floating Script Lab: still shown
  dock->setFloating(true);
  settle();
  QVERIFY(popped->isVisible());
}

void TestScriptLabWindow::resetPutsEverythingBack()
{
  ScriptLabPanel panel;
  panel.resize(700, 600);
  panel.show();
  QVERIFY(panel.popOut(ScriptLabPanel::LivePage));
  QVERIFY(panel.popOut(ScriptLabPanel::PlayerPage));
  panel.tabs()->setCurrentIndex(panel.tabIndex(ScriptLabPanel::MapPage));
  UserSettings::setScriptLabSized(3);
  panel.resetWindows();
  settle();
  QCOMPARE(panel.tabs()->count(), 5);
  for (int p = 0; p < ScriptLabPanel::PageCount; ++p)
  {
    QCOMPARE(panel.tabIndex(ScriptLabPanel::Page(p)), p);
    QVERIFY(!panel.isPoppedOut(ScriptLabPanel::Page(p)));
  }
  QCOMPARE(panel.currentPage(), ScriptLabPanel::LivePage);
  QCOMPARE(UserSettings::scriptLabSized(), 0);
  QCOMPARE(UserSettings::scriptLabWindows(), QString("tab=live"));
  // a popped tab opens big again, not at the old place
  QVERIFY(panel.popOut(ScriptLabPanel::MapPage));
  const QSize first = LabPopWindow::firstSize(panel.screen()->availableGeometry()).expandedTo(ScriptLabPanel::minimumPanelSize())
                        .boundedTo(panel.screen()->availableGeometry().size());
  QCOMPARE(panel.popWindow(ScriptLabPanel::MapPage)->size(), first);
}

void TestScriptLabWindow::noFixedSizes()
{
  ScriptLabPanel panel;
  panel.show();
  QVERIFY(panel.minimumWidth() < panel.maximumWidth());
  QVERIFY(panel.minimumHeight() < panel.maximumHeight());
  QVERIFY(panel.tabs()->minimumWidth() < panel.tabs()->maximumWidth());
  for (int p = 0; p < ScriptLabPanel::PageCount; ++p)
  {
    QWidget* page = panel.page(ScriptLabPanel::Page(p));
    QVERIFY2(page->minimumWidth() < page->maximumWidth(), qPrintable(page->objectName()));
    QVERIFY2(page->minimumHeight() < page->maximumHeight(), qPrintable(page->objectName()));
  }
  QVERIFY(panel.popOut(ScriptLabPanel::LivePage));
  LabPopWindow* window = panel.popWindow(ScriptLabPanel::LivePage);
  settle();
  QVERIFY(window->minimumWidth() < window->maximumWidth());
  QVERIFY(window->minimumHeight() < window->maximumHeight());
  QCOMPARE(window->minimumSize(), ScriptLabPanel::minimumPanelSize());
  // it can be made smaller and bigger within the screen
  const QRect area = screenArea();
  const QSize smaller = ScriptLabPanel::minimumPanelSize() + QSize(10, 10);
  window->resize(smaller);
  settle();
  QCOMPARE(window->size(), smaller);
  const QSize bigger = (smaller + QSize(120, 80)).boundedTo(area.size());
  window->resize(bigger);
  settle();
  QCOMPARE(window->size(), bigger);
}

void TestScriptLabWindow::treeFollowsPoppedLiveWidth()
{
  ScriptLabPanel panel;
  const QRect area = screenArea();
  const int narrow = std::max(ScriptLabPanel::minimumPanelSize().width(), ScriptLabPanel::sideBySideWidth() - 100);
  panel.resize(narrow, 500);
  panel.show();
  settle();
  QVERIFY(!panel.sideBySide());
  QVERIFY(panel.popOut(ScriptLabPanel::LivePage));
  LabPopWindow* window = panel.popWindow(ScriptLabPanel::LivePage);
  if (area.width() < ScriptLabPanel::sideBySideWidth() + 40) QSKIP("the screen is narrower than the side by side width");
  window->resize(ScriptLabPanel::sideBySideWidth() + 40, std::min(500, area.height()));
  settle();
  QVERIFY(panel.sideBySide());   // the Live page's own window is wide: tree beside the editor
  window->resize(narrow, std::min(500, area.height()));
  settle();
  QVERIFY(!panel.sideBySide());
}

void TestScriptLabWindow::noClipping()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    if (v.percent == 150) continue;   // 100 and 200 percent, as the job asks
    UiRender::Scope scope(v);
    UserSettings::setScriptLabWindows(QString());   // the last round popped the map: start as new
    ScriptLabPanel panel;
    for (int p = 0; p < ScriptLabPanel::PageCount; ++p)
    {
      panel.tabs()->setCurrentIndex(p);
      const QImage image = UiRender::render(&panel);
      UiRender::save(image, QStringLiteral("scriptlabwindow_%1").arg(ScriptLabPanel::pageKey(ScriptLabPanel::Page(p))), v);
      // the tab bar and Pop out beside it, whole
      const QStringList bar = UiRender::clipping(panel.tabs()->tabBar());
      QVERIFY2(bar.isEmpty(), qPrintable(v.name() + ": " + bar.join('\n')));
      QVERIFY2(panel.popOutButton()->width() >= panel.popOutButton()->minimumSizeHint().width(), qPrintable(v.name()));
      for (int t = 0; t < panel.tabs()->count(); ++t)
      {
        const QRect r = panel.tabs()->tabBar()->tabRect(t);
        QVERIFY2(r.width() >= panel.tabs()->tabBar()->fontMetrics().horizontalAdvance(panel.tabs()->tabText(t)),
                 qPrintable(v.name() + ": " + panel.tabs()->tabText(t)));
      }
    }
    // a popped window: its top row whole
    QVERIFY(panel.popOut(ScriptLabPanel::MapPage));
    LabPopWindow* window = panel.popWindow(ScriptLabPanel::MapPage);
    const QImage image = UiRender::render(window);
    UiRender::save(image, "scriptlabwindow_popped", v);
    QStringList why = UiRender::clipping(window->putBackButton()->parentWidget());
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + ": " + why.join('\n')));
    QVERIFY(window->putBackButton()->width() >= window->putBackButton()->sizeHint().width());
    // the Put back button is a normal button of the design system: surface_raised fill
    const QString fill = UiRender::expectColour(image, window, window->putBackButton(), QPoint(window->putBackButton()->width() - 4,
                                 window->putBackButton()->height() / 2), Theme::color(Theme::Token::SurfaceRaised, v.dark), "Put back fill", 2);
    QVERIFY2(fill.isEmpty(), qPrintable(v.name() + ": " + fill));
  }
}

QTEST_MAIN(TestScriptLabWindow)
#include "tst_scriptlabwindow.moc"
