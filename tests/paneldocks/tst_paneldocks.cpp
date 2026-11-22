/*
 * PinglyMotion - round trip of the three-panel layout with real dock widgets
 *
 * docs/archive/PANELS_PLAN.md, job P2, section 6: a main window with the three
 * panels (Joints, Selected joint, Timeline) as in the program, with the same
 * object names and allowed sides. One panel is moved, the layout is saved,
 * a new window loads it, and the sides are checked. An old layout that names
 * the Properties panel, saved with the old version number, is refused.
 *
 * Jobs P3 and P4 (src/panelwindow.h): the window's place and maximised state
 * come back from saveGeometry; a floating panel off every screen is pulled
 * back; Reset layout gives the default built in code; Tab hides the open
 * panels and Tab again shows the same ones, a panel closed while hidden
 * stays closed.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>

#include "panellayout.h"
#include "panelwindow.h"
#include "paneltitlebar.h"

// a main window with the three panels, built as PinglyMotionWindow::buildDocks() does
struct ThreePanelWindow
{
  QMainWindow window;
  QDockWidget* joints;
  QDockWidget* jointCard;
  QDockWidget* timeline;

  ThreePanelWindow()
  {
    window.setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::GroupedDragging);
    window.setCentralWidget(new QLabel("3D view"));
    joints=makeDock("Joints",PanelLayout::JointsDock,Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    jointCard=makeDock("Selected joint",PanelLayout::JointCardDock,Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    timeline=makeDock("Timeline",PanelLayout::TimelineDock,Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    // the default layout built in code, as the program's buildDocks() does
    PanelWindow::applyDefaultLayout(&window,joints,jointCard,timeline);
    window.resize(1440,900);
  }

  QList<QDockWidget*> docks() const { return {joints,jointCard,timeline}; }

  QDockWidget* makeDock(const char* title,const char* objectName,Qt::DockWidgetAreas areas)
  {
    QDockWidget* dock=new QDockWidget(QString::fromLatin1(title),&window);
    dock->setObjectName(QString::fromLatin1(objectName));
    dock->setWidget(new QLabel(QString::fromLatin1(title)));
    dock->setAllowedAreas(areas);
    return dock;
  }

  QByteArray save() const { return window.saveState(PanelLayout::CurrentVersion); }
  bool load(const QByteArray& state) { return window.restoreState(state,PanelLayout::CurrentVersion); }
};

// the old two-panel window: Properties and Timeline, saved with the old (no) version
struct OldWindow
{
  QMainWindow window;

  OldWindow()
  {
    window.setCentralWidget(new QLabel("3D view"));
    QDockWidget* properties=new QDockWidget("Properties",&window);
    properties->setObjectName("propertiesDock");
    properties->setWidget(new QLabel("properties"));
    QDockWidget* timeline=new QDockWidget("Timeline",&window);
    timeline->setObjectName(PanelLayout::TimelineDock);
    timeline->setWidget(new QLabel("timeline"));
    window.addDockWidget(Qt::RightDockWidgetArea,properties);
    window.addDockWidget(Qt::BottomDockWidgetArea,timeline);
    window.resize(1440,900);
  }
};

class TestPanelDocks : public QObject
{
  Q_OBJECT

  private slots:
    void defaultLayoutHasThreePanelsOnTheirSides();
    void movedPanelComesBackOnItsNewSide();
    void floatingAndClosedPanelsComeBack();
    void tabbedPanelsStayTogether();
    void oldLayoutWithPropertiesPanelIsRefused();
    void layoutOfAnotherVersionIsRefused();
    // P3
    void windowPlaceAndSizeComeBack();
    void maximisedWindowComesBackMaximised();
    void oldWidthAndHeightAreUsedOnlyWithoutGeometry();
    void floatingPanelOffEveryScreenIsPulledBack();
    void resetGivesTheDefaultLayout();
    // P4
    void tabHidesOpenPanelsAndShowsTheSameOnes();
    void panelClosedWhileHiddenStaysClosed();
    void panelOpenedWhileHiddenEndsTheHiddenState();
    void tabWithNoPanelOpenChangesNothing();
    void resetWhileHiddenShowsAllPanels();
    // P6
    void groupedDraggingIsOn();
    void floatingPanelKeepsItsPlaceAndSizeAfterRoundTrip();
    void doubleClickOnTitleBarFloatsAndDocksAgain();
};

void TestPanelDocks::defaultLayoutHasThreePanelsOnTheirSides()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  QCOMPARE(w.window.dockWidgetArea(w.joints),Qt::LeftDockWidgetArea);
  QCOMPARE(w.window.dockWidgetArea(w.jointCard),Qt::RightDockWidgetArea);
  QCOMPARE(w.window.dockWidgetArea(w.timeline),Qt::BottomDockWidgetArea);
  // the saved layout is usable by the start-up rule
  QVERIFY(PanelLayout::isLayoutUsable(w.save(),PanelLayout::CurrentVersion,w.window.size()));
  QVERIFY(PanelLayout::chooseStartup(w.save(),PanelLayout::CurrentVersion,w.window.size(),true).useSavedLayout);
}

void TestPanelDocks::movedPanelComesBackOnItsNewSide()
{
  QByteArray saved;
  {
    ThreePanelWindow before;
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    // the user drags the Joints panel to the right side, and the timeline to the top
    before.window.addDockWidget(Qt::RightDockWidgetArea,before.joints);
    before.window.addDockWidget(Qt::TopDockWidgetArea,before.timeline);
    QCOMPARE(before.window.dockWidgetArea(before.joints),Qt::RightDockWidgetArea);
    saved=before.save();
  }
  QVERIFY(!saved.isEmpty());

  ThreePanelWindow next;
  QVERIFY(next.load(saved));
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  QCOMPARE(next.window.dockWidgetArea(next.joints),Qt::RightDockWidgetArea);
  QCOMPARE(next.window.dockWidgetArea(next.jointCard),Qt::RightDockWidgetArea);
  QCOMPARE(next.window.dockWidgetArea(next.timeline),Qt::TopDockWidgetArea);
  QVERIFY(!next.joints->isFloating());
  // every panel is still on a side it may go
  for(QDockWidget* dock : {next.joints,next.jointCard,next.timeline})
    QVERIFY(next.window.dockWidgetArea(dock) & dock->allowedAreas());
}

void TestPanelDocks::floatingAndClosedPanelsComeBack()
{
  QByteArray saved;
  {
    ThreePanelWindow before;
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    before.timeline->setFloating(true);
    before.jointCard->close();
    saved=before.save();
  }

  ThreePanelWindow next;
  QVERIFY(next.load(saved));
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  QVERIFY(next.timeline->isFloating());
  QVERIFY(next.jointCard->isHidden());
  QVERIFY(next.joints->isVisible());
  // the View menu items follow the panels
  QVERIFY(!next.jointCard->toggleViewAction()->isChecked());
  QVERIFY(next.joints->toggleViewAction()->isChecked());
}

void TestPanelDocks::tabbedPanelsStayTogether()
{
  QByteArray saved;
  {
    ThreePanelWindow before;
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    // Selected joint as a tab on the Joints panel
    before.window.addDockWidget(Qt::LeftDockWidgetArea,before.jointCard);
    before.window.tabifyDockWidget(before.joints,before.jointCard);
    QCOMPARE(before.window.tabifiedDockWidgets(before.joints),QList<QDockWidget*>() << before.jointCard);
    saved=before.save();
  }

  ThreePanelWindow next;
  QVERIFY(next.load(saved));
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  QCOMPARE(next.window.dockWidgetArea(next.joints),Qt::LeftDockWidgetArea);
  QCOMPARE(next.window.dockWidgetArea(next.jointCard),Qt::LeftDockWidgetArea);
  QCOMPARE(next.window.tabifiedDockWidgets(next.joints),QList<QDockWidget*>() << next.jointCard);
}

void TestPanelDocks::oldLayoutWithPropertiesPanelIsRefused()
{
  QByteArray old;
  {
    OldWindow before;
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    old=before.window.saveState();   // version 0, as the old window_state setting
  }
  QVERIFY(!old.isEmpty());

  // the start-up rule says: not usable, use the default layout
  QVERIFY(!PanelLayout::isLayoutUsable(old,0,QSize(1440,900)));
  QVERIFY(!PanelLayout::chooseStartup(old,0,QSize(1440,900),true).useSavedLayout);

  // and even when it were loaded by mistake, Qt refuses the version
  ThreePanelWindow next;
  QVERIFY(!next.load(old));
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  QCOMPARE(next.window.dockWidgetArea(next.joints),Qt::LeftDockWidgetArea);
  QCOMPARE(next.window.dockWidgetArea(next.jointCard),Qt::RightDockWidgetArea);
  QCOMPARE(next.window.dockWidgetArea(next.timeline),Qt::BottomDockWidgetArea);
}

void TestPanelDocks::layoutOfAnotherVersionIsRefused()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  const QByteArray saved=w.save();
  // a later version of the panels raised the number: the saved layout is not used
  QVERIFY(!PanelLayout::isLayoutUsable(saved,PanelLayout::CurrentVersion+1,w.window.size()));
  QVERIFY(!w.window.restoreState(saved,PanelLayout::CurrentVersion+1));
  // the same bytes with the right number are fine
  QVERIFY(w.load(saved));
}

// ---- P3: the window and the floating panels ----------------------------

// The offscreen screen is small (800x600), so the sizes here fit into it;
// Qt itself shrinks a window that would not.
void TestPanelDocks::windowPlaceAndSizeComeBack()
{
  QByteArray geometry;
  {
    ThreePanelWindow before;
    before.window.resize(700,500);
    before.window.move(30,40);
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    geometry=before.window.saveGeometry();
  }
  QVERIFY(!geometry.isEmpty());
  // the start-up rule: a saved geometry wins over the old width and height
  QVERIFY(PanelLayout::chooseWindow(geometry,850,600,QSize(850,600)).useGeometry);

  ThreePanelWindow next;
  QVERIFY(next.window.restoreGeometry(geometry));
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  QCOMPARE(next.window.size(),QSize(700,500));
  QCOMPARE(next.window.pos(),QPoint(30,40));
  QVERIFY(!next.window.isMaximized());
}

void TestPanelDocks::maximisedWindowComesBackMaximised()
{
  QByteArray geometry;
  {
    ThreePanelWindow before;
    before.window.resize(700,500);
    before.window.showMaximized();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    QVERIFY(before.window.isMaximized());
    geometry=before.window.saveGeometry();
  }

  ThreePanelWindow next;
  QVERIFY(next.window.restoreGeometry(geometry));
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  QVERIFY(next.window.isMaximized());
}

void TestPanelDocks::oldWidthAndHeightAreUsedOnlyWithoutGeometry()
{
  const QSize fallback(850,600);
  // first start after the move: no geometry yet, the old size is kept
  PanelLayout::StartupWindow w=PanelLayout::chooseWindow(QByteArray(),1200,800,fallback);
  QVERIFY(!w.useGeometry);
  QCOMPARE(w.size,QSize(1200,800));
  // no old size either (a new user): the default
  w=PanelLayout::chooseWindow(QByteArray(),0,0,fallback);
  QVERIFY(!w.useGeometry);
  QCOMPARE(w.size,fallback);
  // once a geometry exists the old size is not looked at
  ThreePanelWindow window;
  window.window.resize(700,500);
  window.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window.window));
  w=PanelLayout::chooseWindow(window.window.saveGeometry(),1200,800,fallback);
  QVERIFY(w.useGeometry);
}

void TestPanelDocks::floatingPanelOffEveryScreenIsPulledBack()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  w.timeline->setFloating(true);
  // the second screen it was on is gone: its place is beyond every screen
  const QList<QRect> screens{QRect(0,0,800,600)};
  w.timeline->setGeometry(QRect(2400,100,400,200));
  QCOMPARE(PanelWindow::pullFloatingPanelsOntoScreen(w.docks(),screens),1);
  QVERIFY(screens.first().contains(w.timeline->geometry()));
  QCOMPARE(w.timeline->size(),QSize(400,200));
  // a second call changes nothing
  QCOMPARE(PanelWindow::pullFloatingPanelsOntoScreen(w.docks(),screens),0);
  // docked panels are left alone
  w.timeline->setFloating(false);
  QCOMPARE(PanelWindow::pullFloatingPanelsOntoScreen(w.docks(),screens),0);
  // the real screens give at least one rectangle
  QVERIFY(!PanelWindow::screenRects().isEmpty());
}

void TestPanelDocks::resetGivesTheDefaultLayout()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  // the user moved the timeline to the top, floated the Joints panel,
  // closed Selected joint, and tabbed nothing
  w.window.addDockWidget(Qt::TopDockWidgetArea,w.timeline);
  w.joints->setFloating(true);
  w.jointCard->close();
  QCOMPARE(w.window.dockWidgetArea(w.timeline),Qt::TopDockWidgetArea);

  PanelWindow::applyDefaultLayout(&w.window,w.joints,w.jointCard,w.timeline);
  QCOMPARE(w.window.dockWidgetArea(w.joints),Qt::LeftDockWidgetArea);
  QCOMPARE(w.window.dockWidgetArea(w.jointCard),Qt::RightDockWidgetArea);
  QCOMPARE(w.window.dockWidgetArea(w.timeline),Qt::BottomDockWidgetArea);
  for(QDockWidget* dock : w.docks())
  {
    QVERIFY(!dock->isFloating());
    QVERIFY(dock->isVisible());
    QVERIFY(dock->toggleViewAction()->isChecked());
  }

  // two panels tabbed together are split again
  w.window.addDockWidget(Qt::LeftDockWidgetArea,w.jointCard);
  w.window.tabifyDockWidget(w.joints,w.jointCard);
  QCOMPARE(w.window.tabifiedDockWidgets(w.joints).size(),1);
  PanelWindow::applyDefaultLayout(&w.window,w.joints,w.jointCard,w.timeline);
  QVERIFY(w.window.tabifiedDockWidgets(w.joints).isEmpty());
  QCOMPARE(w.window.dockWidgetArea(w.jointCard),Qt::RightDockWidgetArea);
}

// ---- P4: Tab ------------------------------------------------------------

void TestPanelDocks::tabHidesOpenPanelsAndShowsTheSameOnes()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  // the timeline floats, Selected joint was closed by the user
  w.timeline->setFloating(true);
  w.jointCard->close();
  QCOMPARE(PanelWindow::openPanels(w.docks()),QStringList() << PanelLayout::JointsDock << PanelLayout::TimelineDock);

  PanelLayout::TabState tab;
  QVERIFY(PanelWindow::toggleHidden(tab,w.docks()));   // hidden now
  QVERIFY(tab.isHidden());
  for(QDockWidget* dock : w.docks()) QVERIFY(dock->isHidden());
  QVERIFY(PanelWindow::openPanels(w.docks()).isEmpty());

  QVERIFY(!PanelWindow::toggleHidden(tab,w.docks()));  // shown again
  QVERIFY(!tab.isHidden());
  QVERIFY(w.joints->isVisible());
  QVERIFY(w.timeline->isVisible());
  QVERIFY(w.timeline->isFloating());
  QVERIFY(w.jointCard->isHidden());   // was closed before Tab: stays closed
  // the View menu ticks follow
  QVERIFY(w.joints->toggleViewAction()->isChecked());
  QVERIFY(!w.jointCard->toggleViewAction()->isChecked());
}

void TestPanelDocks::panelClosedWhileHiddenStaysClosed()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  PanelLayout::TabState tab;
  QVERIFY(PanelWindow::toggleHidden(tab,w.docks()));
  // while hidden the user unticks Joints in the View menu: that is a close
  tab.panelClosed(PanelLayout::JointsDock);
  QVERIFY(!PanelWindow::toggleHidden(tab,w.docks()));
  QVERIFY(w.joints->isHidden());
  QVERIFY(w.jointCard->isVisible());
  QVERIFY(w.timeline->isVisible());
}

void TestPanelDocks::panelOpenedWhileHiddenEndsTheHiddenState()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  PanelLayout::TabState tab;
  QVERIFY(PanelWindow::toggleHidden(tab,w.docks()));
  // the user ticks Timeline in the View menu
  w.timeline->toggleViewAction()->trigger();
  QVERIFY(w.timeline->isVisible());
  QVERIFY(tab.panelOpened(PanelLayout::TimelineDock));
  QVERIFY(!tab.isHidden());
  // the next Tab hides what is open now: only the timeline
  QVERIFY(PanelWindow::toggleHidden(tab,w.docks()));
  QVERIFY(w.timeline->isHidden());
  QVERIFY(!PanelWindow::toggleHidden(tab,w.docks()));
  QVERIFY(w.timeline->isVisible());
  QVERIFY(w.joints->isHidden());
  QVERIFY(w.jointCard->isHidden());
}

void TestPanelDocks::tabWithNoPanelOpenChangesNothing()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  for(QDockWidget* dock : w.docks()) dock->close();
  PanelLayout::TabState tab;
  QVERIFY(!PanelWindow::toggleHidden(tab,w.docks()));
  QVERIFY(!tab.isHidden());
  for(QDockWidget* dock : w.docks()) QVERIFY(dock->isHidden());
}

void TestPanelDocks::resetWhileHiddenShowsAllPanels()
{
  ThreePanelWindow w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  PanelLayout::TabState tab;
  QVERIFY(PanelWindow::toggleHidden(tab,w.docks()));
  // View > Reset layout while hidden: the state is over, all three are back
  tab.reset();
  PanelWindow::applyDefaultLayout(&w.window,w.joints,w.jointCard,w.timeline);
  QVERIFY(!tab.isHidden());
  for(QDockWidget* dock : w.docks()) QVERIFY(dock->isVisible());
  // the saved layout after that has the panels shown, so the next start shows them
  ThreePanelWindow next;
  QVERIFY(next.load(w.save()));
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  for(QDockWidget* dock : next.docks()) QVERIFY(dock->isVisible());
}

// ---- P6: two screens -----------------------------------------------------

// Panels docked (or tabbed) together on the same side move together while
// one of them is dragged, instead of leaving the others behind.
void TestPanelDocks::groupedDraggingIsOn()
{
  ThreePanelWindow w;
  QVERIFY(w.window.dockOptions() & QMainWindow::GroupedDragging);
}

// A floating panel (as if moved onto a second screen: the offscreen test
// platform only gives one small screen, so this uses a place on it) keeps
// its exact place and size after being saved and loaded again in a new
// window, as long as the screen it was on is still among the known ones
// (docs/archive/PANELS_PLAN.md, job P6, owner test steps 1-2). This is the
// mechanically checkable part of P6: an exact round trip of geometry, not a
// judgement about how a real second monitor looks; that is the owner's
// Windows test.
void TestPanelDocks::floatingPanelKeepsItsPlaceAndSizeAfterRoundTrip()
{
  QByteArray saved;
  const QRect floatingPlace(120,80,400,300);
  {
    ThreePanelWindow before;
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    before.timeline->setFloating(true);
    before.timeline->setGeometry(floatingPlace);
    saved=before.save();
  }
  QVERIFY(!saved.isEmpty());

  ThreePanelWindow next;
  QVERIFY(next.load(saved));
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  QVERIFY(next.timeline->isFloating());
  QCOMPARE(next.timeline->geometry(),floatingPlace);

  // the screen it was on is still among the known screens: pulling onto a
  // screen leaves it exactly where it was (only a screen that is gone moves
  // it, tested by floatingPanelOffEveryScreenIsPulledBack above)
  const QList<QRect> screens{QRect(0,0,800,600)};
  QCOMPARE(PanelWindow::pullFloatingPanelsOntoScreen(next.docks(),screens),0);
  QCOMPARE(next.timeline->geometry(),floatingPlace);
}

// Double-clicking the title bar (not the buttons) floats a docked panel, and
// docks it again, the same as the float button (docs/archive/PANELS_PLAN.md, job P6:
// "docking back by its button or by double-clicking the title bar").
void TestPanelDocks::doubleClickOnTitleBarFloatsAndDocksAgain()
{
  QMainWindow window;
  window.setCentralWidget(new QLabel("3D view"));
  QDockWidget* dock=new QDockWidget("Timeline",&window);
  dock->setObjectName(PanelLayout::TimelineDock);
  dock->setWidget(new QLabel("timeline"));
  dock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
  PanelTitleBar* titleBar=new PanelTitleBar(dock);
  dock->setTitleBarWidget(titleBar);
  window.addDockWidget(Qt::BottomDockWidgetArea,dock);
  window.resize(900,700);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QVERIFY(!dock->isFloating());

  // a point on the bar's left side, inside the title text, away from the
  // float and close buttons on the right
  const QPoint onTitle(10,titleBar->height()/2);
  // the toggle is deferred to the next pass through the event loop (see
  // paneltitlebar.cpp), so events must be processed before checking it
  QTest::mouseDClick(titleBar,Qt::LeftButton,Qt::NoModifier,onTitle);
  QTRY_VERIFY(dock->isFloating());

  QTest::mouseDClick(dock->titleBarWidget(),Qt::LeftButton,Qt::NoModifier,onTitle);
  QTRY_VERIFY(!dock->isFloating());
}

QTEST_MAIN(TestPanelDocks)
#include "tst_paneldocks.moc"
