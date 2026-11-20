/*
 * PinglyMotion - round trip of the three-panel layout with real dock widgets
 *
 * docs/PANELS_PLAN.md, job P2, section 6: a main window with the three
 * panels (Joints, Selected joint, Timeline) as in the program, with the same
 * object names and allowed sides. One panel is moved, the layout is saved,
 * a new window loads it, and the sides are checked. An old layout that names
 * the Properties panel, saved with the old version number, is refused.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>

#include "panellayout.h"

// a main window with the three panels, built as PinglyMotionWindow::buildDocks() does
struct ThreePanelWindow
{
  QMainWindow window;
  QDockWidget* joints;
  QDockWidget* jointCard;
  QDockWidget* timeline;

  ThreePanelWindow()
  {
    window.setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks);
    window.setCentralWidget(new QLabel("3D view"));
    joints=makeDock("Joints",PanelLayout::JointsDock,Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    jointCard=makeDock("Selected joint",PanelLayout::JointCardDock,Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    timeline=makeDock("Timeline",PanelLayout::TimelineDock,Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    window.addDockWidget(Qt::LeftDockWidgetArea,joints);
    window.addDockWidget(Qt::RightDockWidgetArea,jointCard);
    window.addDockWidget(Qt::BottomDockWidgetArea,timeline);
    window.setCorner(Qt::BottomLeftCorner,Qt::LeftDockWidgetArea);
    window.setCorner(Qt::BottomRightCorner,Qt::RightDockWidgetArea);
    window.resize(1440,900);
  }

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

QTEST_MAIN(TestPanelDocks)
#include "tst_paneldocks.moc"
