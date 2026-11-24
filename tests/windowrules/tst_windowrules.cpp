/*
 * PinglyMotion - the window rules (UX-A, docs/DECISIONS.md, 24 November 2026)
 *
 * The five panels of the program (Joints, Selected joint and Timeline from
 * src/panelcontent.h, Script Lab and Library with their own makeDock) in a
 * main window built as PinglyMotionWindow builds it. Checks, exactly:
 *  - no panel has a fixed size (smallest size below biggest size, docked and
 *    floating), and every panel can be moved, popped out and closed;
 *  - a closed panel comes back from its View menu item;
 *  - place and size of floating and docked panels come back in a new window
 *    (a restart), also when the text size is another one at the restart;
 *  - Reset layout (the default layout in code) puts the three panels back;
 *  - the Library is closed until asked for, and the same item opens it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <algorithm>

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QMainWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "beginnerview.h"
#include "librarypanel.h"
#include "panelcontent.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "scriptlabpanel.h"
#include "textsize.h"
#include "usersettings.h"
#include "theme.h"
#include "uirender.h"

#include "ui_pinglymotionwindow.h"

namespace
{
  // the main window as the program builds it, with all five panels
  struct Program
  {
    QMainWindow window;
    Ui::PinglyMotionWindow ui;
    MainPanels panels;
    ScriptLabPanel* lab = nullptr;
    LibraryPanel* library = nullptr;
    QDockWidget* labDock = nullptr;
    QDockWidget* libraryDock = nullptr;

    Program()
    {
      ui.setupUi(&window);
      window.setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::GroupedDragging);
      panels.build(&window, &ui);
      PanelWindow::applyDefaultLayout(&window, panels.jointsDock, panels.jointCardDock, panels.timelineDock);
      lab = new ScriptLabPanel;
      labDock = ScriptLabPanel::makeDock(&window, lab);
      library = new LibraryPanel;
      libraryDock = LibraryPanel::makeDock(&window, library);
      window.tabifyDockWidget(panels.jointCardDock, libraryDock);
      panels.fit(PanelLayout::Reflow::Wide);
      // no bigger than the screen, or restoring the place would cut it down
      const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
      window.resize(std::min(1700, screen.width() - 80), std::min(1000, screen.height() - 80));
      window.show();
      settle();
    }

    QList<QDockWidget*> docks() const
    {
      return {panels.jointsDock, panels.jointCardDock, panels.timelineDock, labDock, libraryDock};
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
}

class TestWindowRules : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase() { Theme::apply(true); }
    void noPanelHasAFixedSize();
    void everyPanelCanMovePopOutAndClose();
    void aClosedPanelComesBackFromItsViewItem();
    void placeAndSizeComeBackAfterARestart();
    void placeAndSizeComeBackAtAnotherTextSize();
    void resetLayoutPutsTheThreePanelsBack();
    void theLibraryIsClosedUntilAskedFor();
    void beginnerViewGivesTheFigureMostOfTheWindow();
    void jointsListCanBeOpenedAgain();
    void resetLayoutGivesTheModesOwnDefault();
    void eachModeKeepsItsLayoutThroughARestart();
    void theWindowKeepsThePicksAndTheWaysBack();
};

void TestWindowRules::noPanelHasAFixedSize()
{
  Program p;
  for (int state = 0; state < 2; ++state)
  {
    for (QDockWidget* dock : p.docks())
    {
      dock->show();
      dock->setFloating(state == 1);
      p.settle();
      const QString what = QStringLiteral("%1, %2").arg(dock->objectName(), state ? "floating" : "docked");
      QVERIFY2(dock->minimumWidth() < dock->maximumWidth(), qPrintable(what + ": width is fixed"));
      QVERIFY2(dock->minimumHeight() < dock->maximumHeight(), qPrintable(what + ": height is fixed"));
      QVERIFY2(dock->minimumSizeHint().width() < QWIDGETSIZE_MAX, qPrintable(what));
      QVERIFY2(dock->widget()->minimumWidth() < dock->widget()->maximumWidth(), qPrintable(what + ": content width is fixed"));
      QVERIFY2(dock->widget()->minimumHeight() < dock->widget()->maximumHeight(), qPrintable(what + ": content height is fixed"));
      // it can be made bigger than it starts, and it is usable at its smallest
      QVERIFY2(dock->minimumSizeHint().width() <= 900, qPrintable(what + ": least width is " + QString::number(dock->minimumSizeHint().width())));
      if (state == 1)
      {
        const QSize bigger = dock->size() + QSize(60, 60);
        dock->resize(bigger);
        p.settle();
        QVERIFY2(dock->width() >= bigger.width() - 1 && dock->height() >= bigger.height() - 1, qPrintable(what + ": cannot be resized"));
      }
    }
    for (QDockWidget* dock : p.docks()) dock->setFloating(false);
    p.settle();
  }
}

void TestWindowRules::everyPanelCanMovePopOutAndClose()
{
  Program p;
  const QDockWidget::DockWidgetFeatures wanted =
      QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable;
  for (QDockWidget* dock : p.docks())
  {
    QCOMPARE(dock->features() & wanted, wanted);
    QVERIFY2(dock->titleBarWidget() != nullptr, qPrintable(dock->objectName()));
    QVERIFY2(!dock->objectName().isEmpty(), "a panel without a name is not saved");
  }
  for (QDockWidget* dock : p.docks())
  {
    dock->show();
    dock->setFloating(true);
    p.settle();
    QVERIFY2(dock->isFloating(), qPrintable(dock->objectName()));
    const QPoint was = dock->pos();
    dock->move(was + QPoint(40, 30));
    p.settle();
    QCOMPARE(dock->pos(), was + QPoint(40, 30));
    dock->close();
    QVERIFY2(dock->isHidden(), qPrintable(dock->objectName() + " did not close"));
  }
}

void TestWindowRules::aClosedPanelComesBackFromItsViewItem()
{
  Program p;
  for (QDockWidget* dock : p.docks())
  {
    QAction* item = dock->toggleViewAction();
    QVERIFY2(item->isCheckable(), qPrintable(dock->objectName()));
    QVERIFY2(!item->text().isEmpty(), qPrintable(dock->objectName()));
    dock->show();
    p.settle();
    QVERIFY(item->isChecked());
    dock->close();
    QVERIFY(dock->isHidden());
    QVERIFY(!item->isChecked());
    item->trigger();
    p.settle();
    QVERIFY2(!dock->isHidden(), qPrintable(dock->objectName() + " did not come back"));
    QVERIFY(item->isChecked());
  }
}

namespace
{
  // what a restart must give back: the program saves the window and the panel layout
  struct Saved
  {
    QByteArray geometry;
    QByteArray state;
  };

  Saved saveOf(Program& p)
  {
    return {p.window.saveGeometry(), p.window.saveState(PanelLayout::CurrentVersion)};
  }

  void arrange(Program& p)
  {
    // (every place and size fits the screen, so restoring pulls nothing in)
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    const int height = std::min(650, screen.height() - 150);
    const int top = screen.top() + 40;
    p.panels.jointsDock->setFloating(true);
    p.panels.jointsDock->setGeometry(QRect(screen.left() + 60, top, 380, height));
    p.labDock->show();
    p.labDock->setFloating(true);
    p.labDock->setGeometry(QRect(screen.left() + 300, top + 20, 400, height));
    p.libraryDock->show();
    p.libraryDock->setFloating(true);
    p.libraryDock->setGeometry(QRect(screen.left() + 220, top + 50, 420, height - 40));
    p.settle();
    p.window.resizeDocks({p.panels.jointCardDock}, {520}, Qt::Horizontal);
    p.window.resizeDocks({p.panels.timelineDock}, {330}, Qt::Vertical);
    p.settle();
  }

  void sameAs(Program& was, Program& now)
  {
    for (QDockWidget* dock : {was.panels.jointsDock, was.labDock, was.libraryDock})
    {
      QDockWidget* other = now.window.findChild<QDockWidget*>(dock->objectName());
      QVERIFY2(other != nullptr, qPrintable(dock->objectName()));
      QVERIFY2(other->isFloating(), qPrintable(dock->objectName() + " is not floating after the restart"));
      QVERIFY2(!other->isHidden(), qPrintable(dock->objectName() + " is closed after the restart"));
      QCOMPARE(other->geometry(), dock->geometry());
    }
    QCOMPARE(now.panels.jointCardDock->width(), was.panels.jointCardDock->width());
    QCOMPARE(now.panels.timelineDock->height(), was.panels.timelineDock->height());
    QCOMPARE(now.window.dockWidgetArea(now.panels.jointCardDock), was.window.dockWidgetArea(was.panels.jointCardDock));
  }
}

void TestWindowRules::placeAndSizeComeBackAfterARestart()
{
  Saved saved;
  QScopedPointer<Program> first(new Program);
  arrange(*first);
  QVERIFY(first->panels.jointsDock->isFloating());
  saved = saveOf(*first);

  // a restart: a new window object, the saved bytes through QSettings' kind of round trip
  QScopedPointer<Program> second(new Program);
  QVERIFY(second->window.restoreGeometry(saved.geometry));
  second->window.move(first->window.pos());   // the offscreen platform places each window by itself
  QVERIFY(second->window.restoreState(saved.state, PanelLayout::CurrentVersion));
  second->settle();
  sameAs(*first, *second);
  QCOMPARE(second->window.size(), first->window.size());

  // a panel closed before the restart stays closed
  first->labDock->close();
  const Saved closed = saveOf(*first);
  QScopedPointer<Program> third(new Program);
  third->labDock->show();
  QVERIFY(third->window.restoreState(closed.state, PanelLayout::CurrentVersion));
  third->settle();
  QVERIFY(third->labDock->isHidden());
}

void TestWindowRules::placeAndSizeComeBackAtAnotherTextSize()
{
  TextSize::apply(100);
  Saved saved;
  QScopedPointer<Program> first(new Program);
  arrange(*first);
  saved = saveOf(*first);

  for (int percent : {150, 200})
  {
    TextSize::apply(percent);
    QScopedPointer<Program> again(new Program);
    QVERIFY(again->window.restoreGeometry(saved.geometry));
    QVERIFY(again->window.restoreState(saved.state, PanelLayout::CurrentVersion));
    again->settle();
    for (QDockWidget* dock : {first->panels.jointsDock, first->labDock, first->libraryDock})
    {
      QDockWidget* other = again->window.findChild<QDockWidget*>(dock->objectName());
      QVERIFY2(other != nullptr && other->isFloating() && !other->isHidden(),
               qPrintable(QStringLiteral("%1 at %2 percent").arg(dock->objectName()).arg(percent)));
      // the place is kept; the size is kept, or grown when the bigger text needs the room
      QCOMPARE(other->pos(), dock->pos());
      QVERIFY2(other->width() >= dock->width() && other->height() >= dock->height(),
               qPrintable(QStringLiteral("%1 shrank at %2 percent").arg(dock->objectName()).arg(percent)));
    }
  }
  TextSize::apply(100);
}

void TestWindowRules::resetLayoutPutsTheThreePanelsBack()
{
  Program p;
  arrange(p);
  p.panels.timelineDock->close();
  p.settle();
  // what PinglyMotionWindow::resetPanelLayout does
  PanelWindow::applyDefaultLayout(&p.window, p.panels.jointsDock, p.panels.jointCardDock, p.panels.timelineDock);
  p.labDock->setFloating(false);
  p.window.addDockWidget(Qt::RightDockWidgetArea, p.labDock);
  p.labDock->hide();
  p.libraryDock->setFloating(false);
  p.window.tabifyDockWidget(p.panels.jointCardDock, p.libraryDock);
  p.libraryDock->hide();
  p.settle();
  for (QDockWidget* dock : {p.panels.jointsDock, p.panels.jointCardDock, p.panels.timelineDock})
  {
    QVERIFY2(!dock->isFloating() && !dock->isHidden(), qPrintable(dock->objectName()));
  }
  QCOMPARE(p.window.dockWidgetArea(p.panels.jointsDock), Qt::LeftDockWidgetArea);
  QCOMPARE(p.window.dockWidgetArea(p.panels.jointCardDock), Qt::RightDockWidgetArea);
  QCOMPARE(p.window.dockWidgetArea(p.panels.timelineDock), Qt::BottomDockWidgetArea);
  QVERIFY(p.labDock->isHidden());
  QVERIFY(p.libraryDock->isHidden());
}

void TestWindowRules::theLibraryIsClosedUntilAskedFor()
{
  Program p;
  // a new window: the Library is closed, its menu item is not ticked
  QVERIFY(p.libraryDock->isHidden());
  QVERIFY(!p.libraryDock->toggleViewAction()->isChecked());
  // View > Library (and the key and the top bar's button, which follow this item) opens it
  p.libraryDock->toggleViewAction()->trigger();
  p.settle();
  QVERIFY(!p.libraryDock->isHidden());
  QVERIFY(p.libraryDock->toggleViewAction()->isChecked());
  QCOMPARE(p.libraryDock->toggleViewAction()->shortcut(), QKeySequence(QStringLiteral("Ctrl+Shift+B")));

  // the window hides it once more after a saved layout is restored (src/pinglymotionwindow.cpp)
  QFile file(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp"));
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QString source = QString::fromUtf8(file.readAll());
  const int restore = source.indexOf(QStringLiteral("restoreState(UserSettings::panelLayoutFor(UserSettings::expertMode())"));
  const int hide = source.indexOf(QStringLiteral("m_libraryDock->hide();"), restore);
  QVERIFY(restore > 0 && hide > restore);
  QVERIFY(hide - restore < 3000);
}

QTEST_MAIN(TestWindowRules)
#include "tst_windowrules.moc"


// ---- UX-E: the beginner animation view ----

namespace
{
  // the width the 3D view (the middle of the window) has in a mode's default layout
  int figureWidth(Program& p, bool expert)
  {
    PanelWindow::applyModeLayout(&p.window, p.panels.jointsDock, p.panels.jointCardDock, p.panels.timelineDock, expert);
    p.settle();
    return p.window.centralWidget()->width();
  }
}

void TestWindowRules::beginnerViewGivesTheFigureMostOfTheWindow()
{
  Program p;
  const int expertWidth = figureWidth(p, true);
  // Expert mode: today's layout, all three panels open
  for (QDockWidget* dock : {p.panels.jointsDock, p.panels.jointCardDock, p.panels.timelineDock})
    QVERIFY2(!dock->isHidden() && !dock->isFloating(), qPrintable(dock->objectName()));
  QCOMPARE(p.window.dockWidgetArea(p.panels.jointsDock), Qt::LeftDockWidgetArea);

  const int beginnerWidth = figureWidth(p, false);
  // Beginner mode: the Joints list is closed, Selected joint and the timeline stay
  QVERIFY(p.panels.jointsDock->isHidden());
  QVERIFY(!p.panels.jointCardDock->isHidden() && !p.panels.timelineDock->isHidden());
  // the 3D view's share of the window is larger, by at least the Joints panel's width
  const int jointsWidth = p.panels.jointsDock->widget()->minimumSizeHint().width();
  QVERIFY2(beginnerWidth > expertWidth && beginnerWidth - expertWidth >= jointsWidth / 2,
           qPrintable(QStringLiteral("3D view %1 px in Beginner mode, %2 px in Expert mode (Joints panel needs %3)")
                          .arg(beginnerWidth).arg(expertWidth).arg(jointsWidth)));
  QVERIFY(double(beginnerWidth) / p.window.width() > double(expertWidth) / p.window.width());
  // and back: Expert mode shows the Joints list again, in the same place
  figureWidth(p, true);
  QVERIFY(!p.panels.jointsDock->isHidden());
  QCOMPARE(p.window.dockWidgetArea(p.panels.jointsDock), Qt::LeftDockWidgetArea);
}

void TestWindowRules::jointsListCanBeOpenedAgain()
{
  Program p;
  figureWidth(p, false);
  QVERIFY(p.panels.jointsDock->isHidden());
  // View > Joints (the item the Show all joints button follows) opens it
  QVERIFY(!p.panels.jointsDock->toggleViewAction()->isChecked());
  p.panels.jointsDock->toggleViewAction()->trigger();
  p.settle();
  QVERIFY(!p.panels.jointsDock->isHidden());
  QVERIFY(p.panels.jointsDock->toggleViewAction()->isChecked());
  // the button's rules: shown in Beginner mode while the Joints list is closed, never in Expert mode
  QVERIFY(BeginnerView::showJointsButtonVisible(false, false));
  QVERIFY(!BeginnerView::showJointsButtonVisible(false, true));
  QVERIFY(!BeginnerView::showJointsButtonVisible(true, false));
  QVERIFY(BeginnerView::showMoreButtonVisible(false));
  QVERIFY(!BeginnerView::showMoreButtonVisible(true));
  // the buttons exist in the Selected joint panel, with plain words and a tip each
  QVERIFY(p.panels.beginnerRow && p.panels.showJointsButton && p.panels.showMoreButton);
  QCOMPARE(p.panels.showJointsButton->text(), BeginnerView::tr(BeginnerView::showJointsButton));
  QCOMPARE(p.panels.showMoreButton->text(), BeginnerView::tr(BeginnerView::showMoreButton));
  QVERIFY(!p.panels.showJointsButton->toolTip().isEmpty() && !p.panels.showMoreButton->toolTip().isEmpty());
  QVERIFY(p.panels.jointCardDock->widget()->isAncestorOf(p.panels.beginnerRow));
}

void TestWindowRules::resetLayoutGivesTheModesOwnDefault()
{
  Program p;
  for (bool expert : {false, true})
  {
    // the user moved things around: the Timeline floats, the Joints panel is on the other state
    p.panels.timelineDock->setFloating(true);
    p.panels.jointsDock->setVisible(!BeginnerView::jointsListInDefaultLayout(expert));
    p.settle();
    // what PinglyMotionWindow::resetPanelLayout does
    PanelWindow::applyModeLayout(&p.window, p.panels.jointsDock, p.panels.jointCardDock, p.panels.timelineDock, expert);
    p.settle();
    QCOMPARE(p.panels.jointsDock->isHidden(), !expert);
    QVERIFY(!p.panels.jointCardDock->isHidden() && !p.panels.timelineDock->isHidden());
    QVERIFY(!p.panels.timelineDock->isFloating());
    QCOMPARE(p.window.dockWidgetArea(p.panels.timelineDock), Qt::BottomDockWidgetArea);
  }
}

// what PinglyMotionWindow::switchPanelLayoutForMode and the start do: the layout of the mode
// that is left goes into its own slot of the settings, the other mode's slot is restored
void TestWindowRules::eachModeKeepsItsLayoutThroughARestart()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
  UserSettings::setPanelLayout(QByteArray());
  UserSettings::setPanelLayoutBeginner(QByteArray());

  QByteArray beginnerMarked;
  {
    Program p;
    // Expert mode: the Timeline floats; Beginner mode: the Joints list was opened and made wide
    PanelWindow::applyModeLayout(&p.window, p.panels.jointsDock, p.panels.jointCardDock, p.panels.timelineDock, true);
    p.panels.timelineDock->setFloating(true);
    p.settle();
    UserSettings::setPanelLayoutFor(true, p.window.saveState(PanelLayout::CurrentVersion));
    PanelWindow::applyModeLayout(&p.window, p.panels.jointsDock, p.panels.jointCardDock, p.panels.timelineDock, false);
    p.panels.jointsDock->show();
    p.window.resizeDocks({p.panels.jointsDock}, {420}, Qt::Horizontal);
    p.settle();
    beginnerMarked = p.window.saveState(PanelLayout::CurrentVersion);
    UserSettings::setPanelLayoutFor(false, beginnerMarked);
    UserSettings::setPanelLayoutVersion(PanelLayout::CurrentVersion);
    UserSettings::writeSettings();
  }
  UserSettings::setPanelLayout(QByteArray());
  UserSettings::setPanelLayoutBeginner(QByteArray());
  UserSettings::readSettings();
  QCOMPARE(UserSettings::panelLayoutFor(false), beginnerMarked);
  QVERIFY(UserSettings::panelLayoutFor(true) != beginnerMarked);

  // a new window, started in Beginner mode: the Joints list is open as it was left
  {
    Program p;
    QVERIFY(PanelLayout::isLayoutUsable(UserSettings::panelLayoutFor(false), UserSettings::panelLayoutVersion(), p.window.size()));
    p.window.restoreState(UserSettings::panelLayoutFor(false), PanelLayout::CurrentVersion);
    p.settle();
    QVERIFY(!p.panels.jointsDock->isHidden());
    QVERIFY(!p.panels.timelineDock->isFloating());
  }
  // and in Expert mode: the Timeline floats as it was left, the Joints list is open
  {
    Program p;
    p.window.restoreState(UserSettings::panelLayoutFor(true), PanelLayout::CurrentVersion);
    p.settle();
    QVERIFY(p.panels.timelineDock->isFloating());
    QVERIFY(!p.panels.jointsDock->isHidden());
  }
}

// the window's code that cannot run without the 3D view: a click on the figure picks the joint
// without the Joints list, the buttons are wired, the layout is saved and restored per mode
void TestWindowRules::theWindowKeepsThePicksAndTheWaysBack()
{
  QFile file(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp"));
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QString source = QString::fromUtf8(file.readAll());
  auto body = [&](const QString& head)
  {
    const int begin = source.indexOf(head);
    if (begin < 0) return QString();
    const int end = source.indexOf(QStringLiteral("\n}\n"), begin);
    return source.mid(begin, end - begin);
  };
  // a click on the figure sets the joint and its sliders; it never opens or needs a panel
  const QString clicked = body(QStringLiteral("void PinglyMotionWindow::partClicked("));
  QVERIFY(!clicked.isEmpty());
  QVERIFY(clicked.contains(QStringLiteral("currentPart=node;")));
  QVERIFY(clicked.contains(QStringLiteral("setX(rotation.x());")));
  QVERIFY(!clicked.contains(QStringLiteral("m_jointsDock")));
  // both buttons are connected, and Show curves and finger grid is the Expert switch
  QVERIFY(source.contains(QStringLiteral("connect(m_panels->showJointsButton,&QPushButton::clicked")));
  QVERIFY(source.contains(QStringLiteral("connect(m_panels->showMoreButton,&QPushButton::clicked")));
  QVERIFY(body(QStringLiteral("connect(m_panels->showMoreButton,&QPushButton::clicked")).contains(QStringLiteral("m_expertModeAction->setChecked(true)")));
  // the mode switch puts the old layout away and restores the new one, and says what changed
  const QString toggled = body(QStringLiteral("connect(m_expertModeAction,&QAction::toggled"));
  QVERIFY(toggled.contains(QStringLiteral("switchPanelLayoutForMode(on)")));
  QVERIFY(toggled.contains(QStringLiteral("BeginnerView::beginnerLayoutMessage")));
  QVERIFY(toggled.contains(QStringLiteral("BeginnerView::expertLayoutMessage")));
  const QString change = body(QStringLiteral("bool PinglyMotionWindow::switchPanelLayoutForMode("));
  QVERIFY(change.contains(QStringLiteral("UserSettings::setPanelLayoutFor(!expert,")));
  QVERIFY(change.contains(QStringLiteral("restoreState(saved,")));
  // the layout is saved to the slot of the mode that is on
  QVERIFY(source.contains(QStringLiteral("UserSettings::setPanelLayoutFor(UserSettings::expertMode(),m_reflower.layoutToSave(this));")));
  // Reset layout gives the mode's default
  QVERIFY(body(QStringLiteral("void PinglyMotionWindow::resetPanelLayout()")).contains(QStringLiteral("applyModeLayout(")));
}
