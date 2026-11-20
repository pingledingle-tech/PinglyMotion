/*
 * PinglyMotion - test of the Timeline panel at start and when the window closes
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QAction>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>

#include "panelvisibility.h"

// a main window with a timeline panel and a "Show Timeline" menu item, linked as in the program
struct Window
{
  QMainWindow window;
  QDockWidget* dock;
  QAction* menuItem;

  Window()
  {
    window.setCentralWidget(new QLabel("3D view"));
    dock = new QDockWidget("Timeline", &window);
    dock->setObjectName("timelineDock");
    dock->setWidget(new QLabel("timeline"));
    window.addDockWidget(Qt::BottomDockWidgetArea, dock);
    menuItem = new QAction("Show Timeline", &window);
    menuItem->setCheckable(true);
    menuItem->setChecked(true);
    QObject::connect(menuItem, &QAction::toggled, dock, &QDockWidget::setVisible);
    PanelVisibility::linkMenuItem(&window, dock, menuItem);
  }
};

class TestPanelVisibility : public QObject
{
  Q_OBJECT

  private slots:
    void closingTheWindowKeepsTheTimelineOn();
    void savedLayoutWithHiddenTimelineShowsItAtStart();
    void panelCloseButtonUnticksTheMenuItem();
};

void TestPanelVisibility::closingTheWindowKeepsTheTimelineOn()
{
  Window w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));
  QVERIFY(w.dock->isVisible());

  w.window.close();   // as the window's X button does

  QVERIFY(w.menuItem->isChecked());
  QVERIFY(!w.dock->isHidden());
}

void TestPanelVisibility::savedLayoutWithHiddenTimelineShowsItAtStart()
{
  QByteArray savedLayout;
  {
    Window before;
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    before.menuItem->setChecked(false);
    QVERIFY(before.dock->isHidden());
    savedLayout = before.window.saveState();
  }

  Window next;
  QVERIFY(next.window.restoreState(savedLayout));
  QVERIFY(next.dock->isHidden());        // the old layout alone would hide it
  PanelVisibility::showAtStart(next.dock, next.menuItem);
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));

  QVERIFY(next.dock->isVisible());
  QVERIFY(next.menuItem->isChecked());
}

void TestPanelVisibility::panelCloseButtonUnticksTheMenuItem()
{
  Window w;
  w.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&w.window));

  w.dock->close();    // the panel's own close button

  QVERIFY(!w.menuItem->isChecked());
  QVERIFY(w.window.isVisible());
}

QTEST_MAIN(TestPanelVisibility)
#include "tst_panelvisibility.moc"
