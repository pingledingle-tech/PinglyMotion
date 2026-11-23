/*
 * PinglyMotion - the main window's panels at 100, 150 and 200 percent text
 *
 * docs/UX_DECISIONS.md, 6c and charter P9: the text size goes from 100 to
 * 200 percent and the panels rearrange instead of scrolling sideways. This
 * test builds the real Joints, Selected joint and Timeline panels from the
 * designer file with the program's own code (src/panelcontent.h), applies
 * the program's text size and theme (tests/uirender), and for windows of
 * several widths does what the window does when it is resized
 * (PinglyMotionWindow::updateReflow): choose the layout, rearrange the
 * panels, break the timeline rows. Then it checks, exactly:
 *  - the layout that the rule gives for that width and text size;
 *  - the window can really be that narrow (no panel forces it wider), or,
 *    below the one column's least width, no wider than that;
 *  - no panel is wider than the room it is shown in (nothing to scroll
 *    sideways), and no control in a panel is smaller than its size hint
 *    or sticks out of its parent sideways (UiRender::clipping; a panel
 *    taller than its room scrolls down, which is allowed);
 *  - pop-out is greyed out with its reason in one column only.
 * The 3D view is a plain widget (stub/animationview.h). Set
 * PINGLYMOTION_UI_OUT to a folder to get a picture of every case.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QApplication>
#include <QMainWindow>
#include <QToolButton>
#include <QtTest>

#include <QSlider>

#include "panelcontent.h"
#include "selectedjointpanel.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "paneltitlebar.h"
#include "textsize.h"
#include "theme.h"
#include "uirender.h"

#include "ui_pinglymotionwindow.h"

using PanelLayout::Reflow;

namespace
{
  const int WindowHeight = 900;

  // the main window as the program builds it, without the 3D view and the menus' actions
  struct MainWindow
  {
    QMainWindow window;
    Ui::PinglyMotionWindow ui;
    MainPanels panels;
    PanelWindow::Reflower reflower;

    MainWindow()
    {
      ui.setupUi(&window);
      window.setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::GroupedDragging);
      panels.build(&window, &ui);
      PanelWindow::applyDefaultLayout(&window, panels.jointsDock, panels.jointCardDock, panels.timelineDock);
      panels.fit(Reflow::Wide);
    }

    PanelWindow::Docks docks() const
    {
      PanelWindow::Docks d;
      d.joints = panels.jointsDock;
      d.jointCard = panels.jointCardDock;
      d.timeline = panels.timelineDock;
      return d;
    }

    QList<QDockWidget*> dockList() const { return {panels.jointsDock, panels.jointCardDock, panels.timelineDock}; }

    // what PinglyMotionWindow::updateReflow does for a window `width` wide
    Reflow arrange(int width)
    {
      const PanelLayout::ReflowNeeds needs = panels.needs(&window);
      const Reflow next = PanelLayout::chooseReflow(width, needs);
      panels.setTimelineWrapped(PanelLayout::wrapTimeline(next, width, needs));
      reflower.setMode(&window, docks(), next);
      panels.fit(next);
      window.resize(width, WindowHeight);
      window.show();
      settle();
      return next;
    }

    void settle()
    {
      for (int i = 0; i < 5; ++i)
      {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::processEvents();
      }
    }
  };

  // problems in one panel: wider than its room, or a control clipped
  QStringList panelProblems(QDockWidget* dock)
  {
    QStringList problems;
    QScrollArea* scroll = qobject_cast<QScrollArea*>(dock->widget());
    if (!scroll)
      return {QStringLiteral("%1 has no scroll area").arg(dock->objectName())};
    QWidget* panel = scroll->widget();
    if (panel->width() > scroll->viewport()->width())
      problems << QStringLiteral("%1 is %2 wide in a room %3 wide").arg(dock->objectName())
                      .arg(panel->width()).arg(scroll->viewport()->width());
    for (const QString& p : UiRender::clipping(panel))
      problems << dock->objectName() + ": " + p;
    return problems;
  }
}

class TestMainPanels : public QObject
{
  Q_OBJECT

  private slots:
    void panelsRearrangeAndNothingIsClipped_data();
    void panelsRearrangeAndNothingIsClipped();
    void needsGrowWithTheTextSize();
    void timelineRowsBreakOnlyWhenTheyDoNotFit();
    void selectedJointPageFitsItsPanel();
    void expertModeMovesNothing();
    void panelEdgesAreEasyToGrab();
    void jointsPanelCanBeMadeWider();
};

void TestMainPanels::panelsRearrangeAndNothingIsClipped_data()
{
  QTest::addColumn<bool>("dark");
  QTest::addColumn<int>("percent");
  QTest::addColumn<int>("width");
  for (const UiRender::Variant& v : UiRender::variants())
    for (int width : {1920, 1366, 1100, 850, 700})
      QTest::newRow(qPrintable(QStringLiteral("%1, %2 wide").arg(v.name()).arg(width))) << v.dark << v.percent << width;
}

void TestMainPanels::panelsRearrangeAndNothingIsClipped()
{
  QFETCH(bool, dark);
  QFETCH(int, percent);
  QFETCH(int, width);
  const UiRender::Variant variant{dark, percent};
  UiRender::Scope scope(variant);
  MainWindow w;
  const PanelLayout::ReflowNeeds needs = w.panels.needs(&w.window);
  const Reflow expected = PanelLayout::chooseReflow(width, needs);
  const Reflow got = w.arrange(width);
  QCOMPARE(QString::fromLatin1(PanelLayout::reflowName(got)), QString::fromLatin1(PanelLayout::reflowName(expected)));
  QCOMPARE(w.reflower.mode(), expected);
  if (!UiRender::outDir().isEmpty())
    UiRender::save(w.window.grab().toImage(), QStringLiteral("mainpanels_%1").arg(width), variant);

  // the window can be this narrow: no panel forces it wider. Only below the
  // one column's least width (large text in a very small window) is it kept
  // that wide, never wider.
  const int expectedWidth = std::max(width, PanelLayout::narrowNeed(needs));
  QVERIFY2(w.window.width() <= expectedWidth && w.window.width() >= width,
           qPrintable(QStringLiteral("asked %1 wide, the window is %2 (%3 layout; wide needs %4, medium %5, one column %6)")
                          .arg(width).arg(w.window.width()).arg(PanelLayout::reflowName(got))
                          .arg(PanelLayout::wideNeed(needs)).arg(PanelLayout::mediumNeed(needs))
                          .arg(PanelLayout::narrowNeed(needs))));

  // the layout of each kind
  QMainWindow& win = w.window;
  if (got == Reflow::Wide)
  {
    QCOMPARE(win.dockWidgetArea(w.panels.jointsDock), Qt::LeftDockWidgetArea);
    QCOMPARE(win.dockWidgetArea(w.panels.jointCardDock), Qt::RightDockWidgetArea);
  }
  else if (got == Reflow::Medium)
  {
    QCOMPARE(win.dockWidgetArea(w.panels.jointsDock), Qt::RightDockWidgetArea);
    QCOMPARE(win.tabifiedDockWidgets(w.panels.jointCardDock), QList<QDockWidget*>({w.panels.jointsDock}));
  }
  else
  {
    for (QDockWidget* dock : w.dockList())
      QCOMPARE(win.dockWidgetArea(dock), Qt::BottomDockWidgetArea);
  }
  QCOMPARE(win.dockWidgetArea(w.panels.timelineDock), Qt::BottomDockWidgetArea);

  // pop-out greyed out with its reason in one column only
  for (QDockWidget* dock : w.dockList())
  {
    QToolButton* pop = dock->titleBarWidget()->findChildren<QToolButton*>().value(0);
    QCOMPARE(pop->isEnabled(), got != Reflow::Narrow);
    if (got == Reflow::Narrow)
      QCOMPARE(pop->toolTip(), PanelTitleBar::noRoomToFloatText());
  }

  // nothing wider than its room, no control clipped, in every panel that is shown
  QStringList problems;
  for (QDockWidget* dock : w.dockList())
    if (dock->isVisible() && !dock->visibleRegion().isEmpty())
      problems << panelProblems(dock);
  QVERIFY2(problems.isEmpty(), qPrintable(problems.join('\n')));
}

void TestMainPanels::needsGrowWithTheTextSize()
{
  int wide[3] = {0, 0, 0};
  int medium[3] = {0, 0, 0};
  const int sizes[3] = {100, 150, 200};
  for (int i = 0; i < 3; ++i)
  {
    UiRender::Scope scope({true, sizes[i]});
    MainWindow w;
    const PanelLayout::ReflowNeeds needs = w.panels.needs(&w.window);
    wide[i] = PanelLayout::wideNeed(needs);
    medium[i] = PanelLayout::mediumNeed(needs);
    QVERIFY(needs.timelineWrapped < needs.timeline);
    QVERIFY(needs.joints > 0 && needs.jointCard > 0);
  }
  QVERIFY2(wide[0] < wide[1] && wide[1] < wide[2], qPrintable(QStringLiteral("%1 %2 %3").arg(wide[0]).arg(wide[1]).arg(wide[2])));
  QVERIFY2(medium[0] < medium[1] && medium[1] < medium[2], qPrintable(QStringLiteral("%1 %2 %3").arg(medium[0]).arg(medium[1]).arg(medium[2])));
}

void TestMainPanels::timelineRowsBreakOnlyWhenTheyDoNotFit()
{
  UiRender::Scope scope({true, 100});
  MainWindow w;
  const PanelLayout::ReflowNeeds needs = w.panels.needs(&w.window);
  // so wide that the rows fit on one line beside both side panels
  const int roomy = needs.joints + needs.jointCard + 2 * needs.separator + needs.timeline + 10;
  QCOMPARE(w.arrange(roomy), Reflow::Wide);
  QVERIFY(!w.panels.isTimelineWrapped());
  QVERIFY(!w.panels.transport->isWrapped());
  QVERIFY(!w.panels.timelineView->easingTiles()->isWrapped());
  // one column, narrower than the rows on one line
  const int narrow = std::max(needs.joints, std::max(needs.jointCard, needs.timelineWrapped)) + 10;
  QVERIFY(narrow < needs.timeline);
  MainWindow n;
  QCOMPARE(n.arrange(narrow), Reflow::Narrow);
  QVERIFY(n.panels.isTimelineWrapped());
  QVERIFY(n.panels.transport->isWrapped());
  QVERIFY(n.panels.timelineView->easingTiles()->isWrapped());
  // wrapped, Start, Stop and Replay from start stand above Frame back, Frame forward and Loop
  TransportBar* bar = n.panels.transport;
  QCOMPARE(bar->startButton()->y(), bar->replayButton()->y());
  QVERIFY(bar->backButton()->y() > bar->startButton()->y());
  QCOMPARE(bar->backButton()->y(), bar->forwardButton()->y());
}

// The Selected joint panel's joint page (UX phase 2b) with the limit warning
// and More options open, in the program's own panel with the designer file's
// sliders and fields: nothing clipped, at every text size and theme.
void TestMainPanels::selectedJointPageFitsItsPanel()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    MainWindow w;
    SelectedJointPanel* panel = w.panels.selectedJoint;
    QVERIFY(panel);
    QCOMPARE(panel->slider(SelectedJointPanel::Mode::Turn, 0), w.ui.xRotationSlider);
    QCOMPARE(panel->valueField(SelectedJointPanel::Mode::Move, 2), static_cast<QLineEdit*>(w.ui.zPositionEdit));
    QVERIFY(!w.ui.rotationGroupBox->isVisibleTo(&w.window));
    SelectedJointPanel::Joint joint;
    joint.name = QStringLiteral("Right Arm: Shoulder");
    joint.slName = QStringLiteral("mShoulderRight");
    joint.keys = 2;
    joint.changed = true;
    panel->setMoreOpen(true);
    panel->showJoint(joint);
    for (QSlider* slider : {w.ui.xRotationSlider, w.ui.yRotationSlider, w.ui.zRotationSlider})
      slider->setRange(-9000, 9000);
    w.ui.xRotationSlider->setValue(8900);
    QVERIFY(panel->warning()->isVisibleTo(panel));
    w.panels.fit(Reflow::Wide);
    QCOMPARE(w.arrange(1920), Reflow::Wide);
    if (!UiRender::outDir().isEmpty())
      UiRender::save(w.window.grab().toImage(), QStringLiteral("mainpanels_selectedjoint"), v);
    const QStringList problems = panelProblems(w.panels.jointCardDock);
    QVERIFY2(problems.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + problems.join('\n')));
  }
}

// Beginner or Expert mode shows or hides Paste mirrored and moves no other control (charter P12,
// UX phase 6a): the window makes it visible or hidden exactly like this (PinglyMotionWindow)
void TestMainPanels::expertModeMovesNothing()
{
  for (const UiRender::Variant& v : UiRender::variants())
    for (int width : {1920, 1100, 700})
    {
      UiRender::Scope scope(v);
      MainWindow w;
      w.arrange(width);
      QWidget* timeline = w.panels.timelineDock->widget();
      auto geometries = [&]()
      {
        QHash<QString, QRect> found;
        int unnamed = 0;
        for (QWidget* child : timeline->findChildren<QWidget*>())
        {
          if (child == w.panels.pasteMirrored || !child->isVisibleTo(&w.window)) continue;
          if (child->objectName().startsWith(QLatin1String("qt_"))) continue;
          const QString key = child->objectName().isEmpty() ? QStringLiteral("#%1").arg(unnamed++) : child->objectName();
          found.insert(key + QLatin1Char(':') + QString::fromLatin1(child->metaObject()->className()),
                       QRect(child->mapTo(&w.window, QPoint(0, 0)), child->size()));
        }
        return found;
      };
      w.panels.pasteMirrored->setVisible(false);
      w.settle();
      const QHash<QString, QRect> beginner = geometries();
      QVERIFY(!beginner.isEmpty());
      QVERIFY(!w.panels.pasteMirrored->isVisibleTo(&w.window));
      w.panels.pasteMirrored->setVisible(true);
      w.settle();
      QVERIFY(w.panels.pasteMirrored->isVisibleTo(&w.window));
      const QHash<QString, QRect> expert = geometries();
      QCOMPARE(expert.keys().size(), beginner.keys().size());
      for (auto it = beginner.cbegin(); it != beginner.cend(); ++it)
        QVERIFY2(expert.value(it.key()) == it.value(),
                 qPrintable(QStringLiteral("%1 (%2 wide, %3) moved when Expert mode was switched").arg(it.key()).arg(width).arg(v.name())));
    }
}

// UX fixes 1 (the project owner's test notes, 23 November 2026): the line between panels was
// hard to hit. The line between docked panels (and any splitter) is spacing s (8 pixels at
// 100 percent) wide and the edge of a floating panel spacing xs (4), both grow with the text size
void TestMainPanels::panelEdgesAreEasyToGrab()
{
  QCOMPARE(Theme::Size::panelGrab, 8);
  QCOMPARE(Theme::Size::floatingEdge, 4);
  const int expected[][3] = {{100, 8, 4}, {150, 12, 6}, {200, 16, 8}};
  for (const auto& row : expected)
  {
    UiRender::Scope scope({true, row[0]});
    MainWindow w;
    QStyle* style = w.window.style();
    QCOMPARE(style->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent, nullptr, &w.window), row[1]);
    QCOMPARE(style->pixelMetric(QStyle::PM_SplitterWidth, nullptr, &w.window), row[1]);
    QCOMPARE(style->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, w.panels.jointsDock), row[2]);
    // the panel layout counts the same width between the panels
    QCOMPARE(w.panels.needs(&w.window).separator, row[1]);
  }
}

// UX fixes 1: in Expert mode the Joints panel could not be made wider (the wide layout fixed
// its width). It starts as wide as its contents need and the line beside it makes it wider,
// in every layout; it never gets narrower than its contents
void TestMainPanels::jointsPanelCanBeMadeWider()
{
  UiRender::Scope scope({true, 100});
  // windows with room for 120 more pixels beside the 3D view and the timeline
  for (const int width : {1920, 1366})
  {
    MainWindow w;
    const Reflow got = w.arrange(width);
    QScrollArea* scroll = w.panels.jointsScroll;
    const int least = scroll->minimumWidth();
    QVERIFY(least > 0);
    QCOMPARE(scroll->maximumWidth(), QWIDGETSIZE_MAX);
    QCOMPARE(w.panels.jointCardScroll->maximumWidth(), QWIDGETSIZE_MAX);
    if (got != Reflow::Wide) continue;
    // starts at its least width
    QCOMPARE(scroll->width(), least);
    const int before = w.panels.jointsDock->width();
    w.window.resizeDocks({w.panels.jointsDock}, {before + 120}, Qt::Horizontal);
    w.settle();
    QCOMPARE(w.panels.jointsDock->width(), before + 120);
    // and a fit after that (text size or theme change) keeps the width the user gave it
    w.panels.fit(got);
    w.settle();
    QCOMPARE(w.panels.jointsDock->width(), before + 120);
    // never narrower than its contents
    w.window.resizeDocks({w.panels.jointsDock}, {10}, Qt::Horizontal);
    w.settle();
    QVERIFY(scroll->width() >= least);
  }
}

QTEST_MAIN(TestMainPanels)
#include "tst_mainpanels.moc"
