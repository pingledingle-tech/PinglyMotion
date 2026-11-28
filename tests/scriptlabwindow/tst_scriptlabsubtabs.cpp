/*
 * PinglyMotion - Script Lab sub-tabs, parts in windows of their own and Reset view (WP-01 Part B)
 *
 * The project owner's beta.2 test, 28 November 2026: Script Lab got very tall with all its parts; the parts
 * of a tab should be sub-tabs on the left, a group (Object and Script) and each part alone should come out
 * into a window of their own, a Reset view should reset Script Lab only, and Expert mode should hide the
 * example's steps. Checks, exactly:
 *  - the remembered text is read and written safely (unknown keys and broken entries left out);
 *  - the sub-tabs of Live scripts and Type commands: their keys, names and order, the list on the left of
 *    the part in front, only that part shown, and the tab only as tall as the part in front;
 *  - the Live scripts list follows the folder (nothing listed without one, "Object and Script" first with one);
 *  - a group pops out whole and comes back with Put back; one part pops out alone, its group keeps the
 *    other part, and closing the window puts it back in the same place;
 *  - Reset view puts every tab, group and part back and the first sub-tabs in front, and leaves the main
 *    window, the other panels and Script Lab's own place and size as they were;
 *  - the sub-tab in front and the popped groups and parts come back after a restart;
 *  - the example's steps hide in Expert mode, and show again with Help > Script Lab steps or in Beginner mode.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCheckBox>
#include <QDockWidget>
#include <QGuiApplication>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtTest>

#include "labbeginner.h"
#include "labsubtabs.h"
#include "labtabs.h"
#include "labwatcher.h"
#include "rlvstate.h"
#include "scriptlabpanel.h"
#include "scriptlabsection.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  const QByteArray MainScript =
    "default\n{\n    state_entry()\n    {\n        llOwnerSay(\"main ready\");\n        llMessageLinked(LINK_SET, 7, \"hi\", NULL_KEY);\n    }\n}\n";
  const QByteArray LidScript =
    "default\n{\n    link_message(integer sender, integer num, string text, key id)\n    {\n        llOwnerSay(\"lid got \" + text);\n    }\n}\n";

  void writeFile(const QString& path, const QByteArray& text)
  {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(text);
  }

  QStringList listedNames(LabSubTabs* sub)
  {
    QStringList names;
    for (int row = 0; row < sub->list()->count(); ++row)
      if (!sub->list()->item(row)->isHidden()) names << sub->list()->item(row)->text();
    return names;
  }
}

class TestScriptLabSubTabs : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();
    void stateIsReadSafely();
    void commandsSubTabs();
    void liveSubTabsFollowTheFolder();
    void groupPopsOutWhole();
    void onePartPopsOutAlone();
    void resetViewOnlyScriptLab();
    void layoutComesBackAfterARestart();
    void stepsHideInExpertMode();

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
    // MyObject/Root/main.lsl and MyObject/Lid/lid.lsl, watched as one object
    static QString makeObject(const QTemporaryDir& dir)
    {
      const QString top = dir.filePath("MyObject");
      writeFile(top + "/Root/main.lsl", MainScript);
      writeFile(top + "/Lid/lid.lsl", LidScript);
      return top;
    }
};

void TestScriptLabSubTabs::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestScriptLabSubTabs::init()
{
  UserSettings::setScriptLabWindows(QString());
  UserSettings::setScriptLabSubTabs(QString());
  UserSettings::setScriptLabSized(0);
  UserSettings::setScriptLabParts(QString());
  UserSettings::setExpertMode(false);
}

void TestScriptLabSubTabs::cleanup()
{
  init();
  settle();
}

void TestScriptLabSubTabs::stateIsReadSafely()
{
  const QMap<QString, QStringList> groups = {{"live", {"work", "log"}}, {"commands", {"actions", "replies"}}};
  const QStringList units = {"work", "log", "actions", "replies", "tree", "script"};
  LabSubTabState::State state;
  state.current = {{"live", "log"}, {"commands", "replies"}};
  state.popped = {"script", "actions"};
  state.places = {{"script", QRect(10, 20, 600, 400)}};
  const QString text = LabSubTabState::format(state);
  QCOMPARE(text, QString("sub=commands:replies;sub=live:log;pop=script;pop=actions;place=script:10,20,600,400"));
  QCOMPARE(LabSubTabState::parse(text, groups, units), state);
  // unknown tabs, groups and keys, broken numbers and a group of another tab are left out
  const LabSubTabState::State odd = LabSubTabState::parse(
    "sub=live:nothing;sub=other:work;sub=live:replies;sub=commands;pop=../x;pop=log;pop=log;place=log:1,2,0,5;"
    "place=tree:a,b,c,d;place=work:5,6,700,500;junk", groups, units);
  QVERIFY(odd.current.isEmpty());
  QCOMPARE(odd.popped, QStringList({"log"}));
  QCOMPARE(odd.places.size(), 1);
  QCOMPARE(odd.places.value("work"), QRect(5, 6, 700, 500));
  QCOMPARE(LabSubTabState::format(LabSubTabState::State()), QString());
  // a very long text is cut before it is read
  QVERIFY(LabSubTabState::format(state).size() <= LabSubTabState::MaxLength);
  QCOMPARE(LabSubTabState::parse(QString(5000, 'x'), groups, units), LabSubTabState::State());
}

void TestScriptLabSubTabs::commandsSubTabs()
{
  ScriptLabPanel panel;
  panel.resize(860, 700);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.showPage(ScriptLabPanel::CommandsPage);
  settle();
  LabSubTabs* sub = panel.subTabs(ScriptLabPanel::CommandsPage);
  QVERIFY(sub);
  QVERIFY(!panel.subTabs(ScriptLabPanel::MapPage));
  QCOMPARE(sub->groupKeys(), QStringList({"restrictions", "actions", "view", "replies"}));
  QCOMPARE(listedNames(sub), QStringList({"Restrictions", "What the avatar may do", "Show in 3D view", "Replies and messages"}));
  QCOMPARE(sub->list()->accessibleName(), QString("Parts of this tab"));
  QCOMPARE(sub->currentGroup(), QString("restrictions"));
  QVERIFY(!sub->groupTitle()->isVisible());   // one part: its own header names it
  QVERIFY(!sub->groupPlaceholder("restrictions"));   // nothing is out
  // the list is on the left of the part in front
  const QPoint listAt = sub->list()->mapTo(&panel, QPoint(0, 0));
  const QPoint stackAt = sub->stack()->mapTo(&panel, QPoint(0, 0));
  QVERIFY(listAt.x() + sub->list()->width() <= stackAt.x());
  QVERIFY(sub->list()->isVisible());
  // only the part in front is shown; the others wait in their sub-tab
  QVERIFY(panel.section("restrictions")->isVisible());
  for (const char* key : {"actions", "replies", "sphere"}) QVERIFY2(!panel.section(key)->isVisible(), key);
  QVERIFY(!panel.cameraBox()->isVisible());
  // a pick in the list brings that part to the front
  sub->list()->setCurrentRow(2);
  settle();
  QCOMPARE(sub->currentGroup(), QString("view"));
  QVERIFY(panel.cameraBox()->isVisible());
  QVERIFY(panel.viewModesBox()->isVisible());
  QVERIFY(panel.section("sphere")->isVisible());
  QVERIFY(!panel.section("restrictions")->isVisible());
  QCOMPARE(sub->groupTitle()->text(), QString("Show in 3D view"));
  // the keys: Down in the list goes to the next part
  sub->list()->setFocus();
  QTest::keyClick(sub->list(), Qt::Key_Down);
  QCOMPARE(sub->currentGroup(), QString("replies"));
  QVERIFY(panel.section("replies")->isVisible());
  // the tab is only as tall as the part in front, not all of them together
  QCOMPARE(sub->stack()->sizeHint(), sub->stack()->currentWidget()->sizeHint().expandedTo(sub->stack()->currentWidget()->minimumSizeHint()));
  int all = 0;
  for (int i = 0; i < sub->stack()->count(); ++i) all += sub->stack()->widget(i)->sizeHint().height();
  QVERIFY(sub->stack()->sizeHint().height() < all);
  QVERIFY(sub->setCurrentGroup("actions"));
  QVERIFY(!sub->setCurrentGroup("nothing"));
  QCOMPARE(sub->currentGroup(), QString("actions"));
  QCOMPARE(sub->stack()->sizeHint().height(), sub->stack()->currentWidget()->sizeHint().height());
  // Pop out of the part in front, with its words, a name and a tip
  QCOMPARE(sub->popOutButton()->text(), QString("Pop out"));
  QCOMPARE(sub->popOutButton()->accessibleName(), QString("Pop out this part"));
  QCOMPARE(sub->popOutButton()->toolTip(),
           QString("Pop out \"What the avatar may do\" into a window of its own. Close that window, or press Put back, to bring it back."));
  QVERIFY(!sub->popOutButton()->icon().isNull());
}

void TestScriptLabSubTabs::liveSubTabsFollowTheFolder()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.resize(1000, 700);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  LabSubTabs* sub = panel.subTabs(ScriptLabPanel::LivePage);
  QCOMPARE(sub->groupKeys(), QStringList({"work", "problems", "log", "restrictionsNow", "objects", "input", "inventory"}));
  // no folder yet: nothing to list, the empty state has the room
  QVERIFY(sub->isHidden());
  QVERIFY(listedNames(sub).isEmpty());
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(makeObject(dir)));
  settle();
  QVERIFY(!sub->isHidden());
  QCOMPARE(listedNames(sub), QStringList({"Object and Script", "Errors and warnings", "Event log", "RLV restrictions now",
                                          "Objects", "Send input", "Inventory"}));
  QCOMPARE(sub->currentGroup(), QString("work"));
  // Object and Script: the tree and the editor together in the first sub-tab, each with its own Pop out
  QWidget* work = sub->groupContent("work");
  QVERIFY(work->isAncestorOf(panel.section("tree")));
  QVERIFY(work->isAncestorOf(panel.section("script")));
  QVERIFY(panel.section("tree")->isVisible());
  QVERIFY(panel.section("script")->isVisible());
  QVERIFY(!panel.section("log")->isVisible());
  QCOMPARE(sub->partKeys(), QStringList({"tree", "script"}));
  QCOMPARE(sub->groupOfPart("script"), QString("work"));
  QCOMPARE(sub->groupTitle()->text(), QString("Object and Script"));
  QVERIFY(sub->groupTitle()->isVisible());
  for (const char* key : {"tree", "script"})
  {
    QToolButton* pop = sub->partPopButton(key);
    QVERIFY2(pop && pop->isVisible(), key);
    QVERIFY(panel.section(key)->isAncestorOf(pop));
    QCOMPARE(pop->accessibleName(), QString(QLatin1String(key) == QLatin1String("tree") ? "Pop out Object" : "Pop out Script"));
    QVERIFY(!pop->icon().isNull());
  }
  // a part alone in its sub-tab: its own header names it, the group's title steps aside
  QVERIFY(sub->setCurrentGroup("log"));
  settle();
  QVERIFY(panel.section("log")->isVisible());
  QVERIFY(!sub->groupTitle()->isVisible());
  QVERIFY(!panel.section("tree")->isVisible());
  // stop watching: the list empties again
  panel.stopWatching();
  settle();
  QVERIFY(sub->isHidden());
}

void TestScriptLabSubTabs::groupPopsOutWhole()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.resize(1000, 700);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(makeObject(dir)));
  settle();
  LabSubTabs* sub = panel.subTabs(ScriptLabPanel::LivePage);
  sub->popOutButton()->click();   // the group in front: Object and Script
  settle();
  QVERIFY(sub->isPoppedOut("work"));
  LabPopWindow* window = sub->popWindow("work");
  QVERIFY(window && window->isVisible());
  QCOMPARE(window->windowTitle(), QString("Script Lab: Object and Script"));
  QVERIFY(window->isAncestorOf(panel.section("tree")));
  QVERIFY(window->isAncestorOf(panel.section("script")));
  QCOMPARE(window->putBackButton()->accessibleName(), QString("Put Object and Script back into Script Lab"));
  // in its place: what it is and the way back; the list says it is in a window; Pop out is greyed with why
  QWidget* placeholder = sub->groupPlaceholder("work");
  QVERIFY(placeholder->isVisible());
  QCOMPARE(placeholder->findChild<QLabel*>("scriptLabOutText")->text(), QString("\"Object and Script\" is in a window of its own."));
  QCOMPARE(sub->list()->item(0)->text(), QString("Object and Script (window)"));
  QVERIFY(!sub->popOutButton()->isEnabled());
  QCOMPARE(sub->popOutButton()->toolTip(), QString("\"Object and Script\" is in a window of its own already. Put back brings it here."));
  QCOMPARE(UserSettings::scriptLabSubTabs(), QStringLiteral("sub=commands:restrictions;sub=live:work;pop=work;place=work:%1,%2,%3,%4")
             .arg(window->geometry().x()).arg(window->geometry().y()).arg(window->geometry().width()).arg(window->geometry().height()));
  // the parts of a group that is out stay with it
  QVERIFY(!sub->popOut("tree"));
  // Put back in its place brings it home
  placeholder->findChild<QPushButton*>("scriptLabOutPutBack")->click();
  settle();
  QVERIFY(!sub->isPoppedOut("work"));
  QVERIFY(sub->groupContent("work")->isAncestorOf(panel.section("tree")));
  QVERIFY(sub->groupContent("work")->isAncestorOf(panel.section("script")));
  QVERIFY(panel.section("tree")->isVisible());
  QVERIFY(!sub->groupPlaceholder("work"));   // made only while it is out
  QCOMPARE(sub->list()->item(0)->text(), QString("Object and Script"));
  QVERIFY(sub->popOutButton()->isEnabled());
  // again, and Put back in the window
  QVERIFY(sub->popOut("work"));
  sub->popWindow("work")->putBackButton()->click();
  settle();
  QVERIFY(!sub->isPoppedOut("work"));
  QVERIFY(sub->groupContent("work")->isAncestorOf(panel.section("script")));
  QCOMPARE(UserSettings::scriptLabSubTabs().left(41), QString("sub=commands:restrictions;sub=live:work;p"));
  QVERIFY(!UserSettings::scriptLabSubTabs().contains("pop="));
}

void TestScriptLabSubTabs::onePartPopsOutAlone()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.resize(1000, 700);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(makeObject(dir)));
  settle();
  LabSubTabs* sub = panel.subTabs(ScriptLabPanel::LivePage);
  ScriptLabSection* tree = panel.section("tree");
  QWidget* column = panel.treeColumn();
  QVBoxLayout* columnLayout = qobject_cast<QVBoxLayout*>(column->layout());
  const int at = columnLayout->indexOf(tree);
  QVERIFY(at >= 0);
  tree->setExpanded(false);
  sub->partPopButton("tree")->click();
  settle();
  QVERIFY(sub->isPoppedOut("tree"));
  LabPopWindow* window = sub->popWindow("tree");
  QVERIFY(window->isVisible());
  QCOMPARE(window->windowTitle(), QString("Script Lab: Object"));
  QVERIFY(window->isAncestorOf(tree));
  QVERIFY(tree->isExpanded());   // a folded part opens in its window: a header alone would show nothing
  QVERIFY(!sub->partPopButton("tree")->isVisible());   // Put back is on top of the window instead
  // the group keeps the other part, and a line with Put back where the tree was
  QVERIFY(sub->groupContent("work")->isAncestorOf(panel.section("script")));
  QVERIFY(panel.section("script")->isVisible());
  QWidget* placeholder = sub->partPlaceholder("tree");
  QVERIFY(placeholder && placeholder->isVisible());
  QCOMPARE(columnLayout->indexOf(placeholder), at);
  QCOMPARE(placeholder->findChild<QLabel*>("scriptLabOutText")->text(), QString("\"Object\" is in a window of its own."));
  QVERIFY(!sub->isPoppedOut("work"));
  QVERIFY(listedNames(sub).contains("Object and Script"));
  // the script alone too: both parts out, the group stays listed with the two lines
  QVERIFY(sub->popOut("script"));
  QVERIFY(sub->popWindow("script")->isAncestorOf(panel.section("script")));
  QCOMPARE(sub->popped(), QStringList({"tree", "script"}));
  // closing the window puts the part back in the same place
  window->close();
  settle();
  QVERIFY(!sub->isPoppedOut("tree"));
  QCOMPARE(columnLayout->indexOf(tree), at);
  QVERIFY(column->isAncestorOf(tree));
  QVERIFY(tree->isVisible());
  QVERIFY(sub->partPopButton("tree")->isVisible());
  QVERIFY(!sub->partPlaceholder("tree"));
  // popping the whole group brings its loose part back into it first: the group goes as one
  QVERIFY(sub->popOut("work"));
  QVERIFY(!sub->isPoppedOut("script"));
  QVERIFY(sub->popWindow("work")->isAncestorOf(panel.section("script")));
  QVERIFY(sub->popWindow("work")->isAncestorOf(tree));
  QVERIFY(sub->putBack("work"));
  QVERIFY(!sub->putBack("work"));
  QVERIFY(!sub->popOut("nothing"));
}

void TestScriptLabSubTabs::resetViewOnlyScriptLab()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  QMainWindow window;
  QDockWidget* other = new QDockWidget("Timeline", &window);
  other->setObjectName("otherDock");
  other->setWidget(new QLabel("timeline"));
  window.addDockWidget(Qt::BottomDockWidgetArea, other);
  window.resize(1400, 900);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  ScriptLabPanel* panel = new ScriptLabPanel;
  panel->setState(&state);
  panel->setWatcher(&watcher);
  QDockWidget* dock = ScriptLabPanel::makeDock(&window, panel);
  dock->toggleViewAction()->trigger();
  settle();
  QTemporaryDir dir;
  QVERIFY(panel->watchFolder(makeObject(dir)));
  settle();
  LabSubTabs* live = panel->subTabs(ScriptLabPanel::LivePage);
  LabSubTabs* commands = panel->subTabs(ScriptLabPanel::CommandsPage);
  // a tab, a group and a part out; other sub-tabs in front
  QVERIFY(panel->popOut(ScriptLabPanel::PlayerPage));
  QVERIFY(live->popOut("script"));
  QVERIFY(live->setCurrentGroup("log"));
  QVERIFY(live->popOut("log"));
  QVERIFY(commands->setCurrentGroup("replies"));
  panel->showPage(ScriptLabPanel::CommandsPage);
  settle();
  const QRect windowPlace = window.geometry();
  const QRect dockPlace = dock->geometry();
  const QRect otherPlace = other->geometry();
  const bool floating = dock->isFloating();
  const int sized = UserSettings::scriptLabSized();
  const QString windows = UserSettings::scriptLabWindows();
  // Reset view, beside Pop out at the right of the tabs
  QToolButton* reset = panel->resetViewButton();
  QCOMPARE(reset->text(), QString("Reset view"));
  QCOMPARE(reset->accessibleName(), QString("Reset the Script Lab view"));
  QCOMPARE(reset->toolTip(), QString("Put every tab and part of Script Lab back in this window, with the first parts in front. "
                                     "Only Script Lab: the rest of the window stays as it is."));
  QVERIFY(!reset->icon().isNull());
  QCOMPARE(reset->focusPolicy(), Qt::StrongFocus);
  QCOMPARE(reset->parentWidget(), panel->tabs()->cornerWidget(Qt::TopRightCorner));
  QCOMPARE(panel->popOutButton()->parentWidget(), reset->parentWidget());
  QVERIFY(reset->isVisible());
  reset->click();
  settle();
  // Script Lab: everything back, the first sub-tabs and Live scripts in front
  QCOMPARE(panel->tabs()->count(), int(ScriptLabPanel::PageCount));
  QVERIFY(!panel->isPoppedOut(ScriptLabPanel::PlayerPage));
  QVERIFY(live->popped().isEmpty());
  QVERIFY(commands->popped().isEmpty());
  QVERIFY(live->places().isEmpty());
  QCOMPARE(live->currentGroup(), QString("work"));
  QCOMPARE(commands->currentGroup(), QString("restrictions"));
  QCOMPARE(panel->currentPage(), ScriptLabPanel::LivePage);
  QVERIFY(live->groupContent("work")->isAncestorOf(panel->section("script")));
  QVERIFY(live->groupContent("log")->isAncestorOf(panel->section("log")));
  QVERIFY(panel->section("script")->isVisible());
  QCOMPARE(UserSettings::scriptLabSubTabs(), QString("sub=commands:restrictions;sub=live:work"));
  QCOMPARE(UserSettings::scriptLabWindows(), QString("tab=live"));
  Q_UNUSED(windows);
  // the rest of the window: as it was, and Script Lab keeps its own place and size
  QCOMPARE(window.geometry(), windowPlace);
  QCOMPARE(other->geometry(), otherPlace);
  QVERIFY(other->isVisible());
  QCOMPARE(dock->geometry(), dockPlace);
  QCOMPARE(dock->isFloating(), floating);
  QVERIFY(dock->isVisible());
  QCOMPARE(UserSettings::scriptLabSized(), sized);   // not opened big again, unlike View > Reset layout
  // a part popped after the reset opens at the first place again, not the old one
  QVERIFY(live->popOut("script"));
  QVERIFY(live->places().contains("script"));
  QVERIFY(live->putBack("script"));
}

void TestScriptLabSubTabs::layoutComesBackAfterARestart()
{
  const QRect area = QGuiApplication::primaryScreen()->availableGeometry();
  const QRect place(area.left() + 30, area.top() + 30, std::min(500, area.width() - 60), std::min(360, area.height() - 60));
  QTemporaryDir dir;
  const QString top = makeObject(dir);
  QString saved;
  {
    RlvState state;
    LabWatcher watcher;
    watcher.setRlvState(&state);
    ScriptLabPanel panel;
    panel.setState(&state);
    panel.setWatcher(&watcher);
    panel.resize(1000, 700);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    QVERIFY(panel.watchFolder(top));
    settle();
    QVERIFY(panel.subTabs(ScriptLabPanel::CommandsPage)->setCurrentGroup("replies"));
    QVERIFY(panel.subTabs(ScriptLabPanel::LivePage)->popOut("script"));
    panel.subTabs(ScriptLabPanel::LivePage)->popWindow("script")->setGeometry(place);
    settle();
    QVERIFY(panel.subTabs(ScriptLabPanel::LivePage)->setCurrentGroup("log"));
    saved = UserSettings::scriptLabSubTabs();
    QCOMPARE(saved, QStringLiteral("sub=commands:replies;sub=live:log;pop=script;place=script:%1,%2,%3,%4")
                      .arg(place.x()).arg(place.y()).arg(place.width()).arg(place.height()));
  }   // the program closes
  settle();
  // through the settings file too
  UserSettings::writeSettings();
  UserSettings::setScriptLabSubTabs(QString());
  UserSettings::readSettings();
  QCOMPARE(UserSettings::scriptLabSubTabs(), saved);
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel again;
  again.setState(&state);
  again.setWatcher(&watcher);
  QVERIFY(again.subTabs(ScriptLabPanel::LivePage)->isPoppedOut("script"));
  QCOMPARE(again.subTabs(ScriptLabPanel::CommandsPage)->currentGroup(), QString("replies"));
  LabPopWindow* window = again.subTabs(ScriptLabPanel::LivePage)->popWindow("script");
  QCOMPARE(window->geometry(), place);
  QVERIFY(window->isAncestorOf(again.section("script")));
  QVERIFY(!window->isVisible());   // shown with Script Lab, not before
  again.resize(1000, 700);
  again.show();
  QVERIFY(QTest::qWaitForWindowExposed(&again));
  QVERIFY(window->isVisible());
  QVERIFY(again.watchFolder(top));
  settle();
  // the Live list is filled once a folder is watched; its sub-tab comes back when it is listed
  again.applySubTabsState(saved);
  QCOMPARE(again.subTabs(ScriptLabPanel::LivePage)->currentGroup(), QString("log"));
  QCOMPARE(UserSettings::scriptLabSubTabs(), saved);
  // a broken saved text: nothing popped, the first sub-tabs
  again.applySubTabsState("pop=nothing;sub=live:none;place=script:x");
  QVERIFY(again.subTabs(ScriptLabPanel::LivePage)->popped().isEmpty());
  QVERIFY(again.subTabs(ScriptLabPanel::LivePage)->groupContent("work")->isAncestorOf(again.section("script")));
}

void TestScriptLabSubTabs::stepsHideInExpertMode()
{
  ScriptLabPanel panel;
  panel.setExampleObjectsFolder(QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/objects"));
  panel.resize(1000, 700);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  // Beginner mode: Help > Script Lab steps shows them
  panel.showSteps();
  settle();
  QVERIFY(panel.stepsShown());
  QVERIFY(panel.stepCard()->isVisible());
  // Expert mode on: the steps go
  UserSettings::setExpertMode(true);
  settle();
  QVERIFY(panel.expertMode());
  QVERIFY(!panel.stepCard()->isVisible());
  // asked for by name in Expert mode: they show
  panel.showSteps();
  settle();
  QVERIFY(panel.stepCard()->isVisible());
  // Beginner mode and Expert mode again: shown, then gone
  UserSettings::setExpertMode(false);
  settle();
  QVERIFY(panel.stepCard()->isVisible());
  UserSettings::setExpertMode(true);
  settle();
  QVERIFY(!panel.stepCard()->isVisible());
  // closed in Beginner mode: they stay closed
  UserSettings::setExpertMode(false);
  panel.closeSteps();
  settle();
  QVERIFY(!panel.stepCard()->isVisible());
  // a panel made while Expert mode is on does not show them by itself
  UserSettings::setExpertMode(true);
  ScriptLabPanel expert;
  expert.setExampleObjectsFolder(QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/objects"));
  expert.show();
  QVERIFY(QTest::qWaitForWindowExposed(&expert));
  QVERIFY(!expert.stepCard()->isVisible());
}

QTEST_MAIN(TestScriptLabSubTabs)
#include "tst_scriptlabsubtabs.moc"
