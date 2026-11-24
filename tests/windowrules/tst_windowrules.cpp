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
#include <QtTest>

#include "librarypanel.h"
#include "panelcontent.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "scriptlabpanel.h"
#include "textsize.h"
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
  const int restore = source.indexOf(QStringLiteral("restoreState(UserSettings::panelLayout()"));
  const int hide = source.indexOf(QStringLiteral("m_libraryDock->hide();"), restore);
  QVERIFY(restore > 0 && hide > restore);
  QVERIFY(hide - restore < 3000);
}

QTEST_MAIN(TestWindowRules)
#include "tst_windowrules.moc"
