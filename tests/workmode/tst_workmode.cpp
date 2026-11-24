/*
 * PinglyMotion - the Animate / Script switch (UX-F, docs/DECISIONS.md, 24 November 2026)
 *
 * The five panels of the program in a main window built as PinglyMotionWindow builds it, with the
 * real WorkMode::Switcher (src/workmode.h) and a tool bar. Checks, exactly:
 *  - the program starts on Animate: the animation panels open, the Script Lab and Library closed;
 *  - switching shows and hides the right windows, both ways, with the buttons, the Script Lab's
 *    own View item and its close button;
 *  - each side keeps its own layout (places, sizes, what is open) when you switch away and back,
 *    and the layouts come back from the saved settings after a restart (which starts on Animate);
 *  - the Beginner / Expert switch on the Script side touches no layout slot;
 *  - the keys F7 and F8, the buttons by keyboard, and the accessible names; the active side shows
 *    by more than colour (pressed, bold, tick icon);
 *  - Reset layout resets both sides and the side you are on.
 * No window is bigger than the available screen.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <algorithm>

#include <QApplication>
#include <QGuiApplication>
#include <QMainWindow>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>
#include <QtTest>

#include "beginnerview.h"
#include "librarypanel.h"
#include "panelcontent.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "scriptlabpanel.h"
#include "textsize.h"
#include "theme.h"
#include "uirender.h"
#include "usersettings.h"
#include "workmode.h"

#include "ui_pinglymotionwindow.h"

namespace
{
  using WorkMode::Side;

  // the main window as the program builds it, with the switch
  struct Program
  {
    QMainWindow window;
    Ui::PinglyMotionWindow ui;
    MainPanels panels;
    ScriptLabPanel* lab = nullptr;
    LibraryPanel* library = nullptr;
    QDockWidget* labDock = nullptr;
    QDockWidget* libraryDock = nullptr;
    PanelWindow::Reflower reflower;
    WorkMode::Switcher* work = nullptr;
    int afterCalls = 0;

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
      libraryDock->hide();

      WorkMode::Hooks hooks;
      hooks.after = [this]() { ++afterCalls; };
      hooks.layoutToSave = [this]() { return reflower.layoutToSave(&window); };
      hooks.expert = []() { return UserSettings::expertMode(); };
      work = new WorkMode::Switcher(&window, docks5(), hooks, &window);
      work->normaliseAtStart();
      work->addToBar(ui.toolBar);
      window.show();
      settle();
    }

    // what the program does at a start: the saved layout of the Animate side, then Animate
    void startFromSaved(const QByteArray& geometry = QByteArray())
    {
      if (!geometry.isEmpty()) window.restoreGeometry(geometry);
      window.restoreState(UserSettings::panelLayoutFor(UserSettings::expertMode()), PanelLayout::CurrentVersion);
      libraryDock->hide();
      work->normaliseAtStart();
      settle();
    }

    PanelWindow::Docks docks5() const
    {
      PanelWindow::Docks docks;
      docks.joints = panels.jointsDock;
      docks.jointCard = panels.jointCardDock;
      docks.timeline = panels.timelineDock;
      docks.scriptLab = labDock;
      docks.library = libraryDock;
      return docks;
    }

    QList<QDockWidget*> animationPanels() const { return {panels.jointsDock, panels.jointCardDock, panels.timelineDock}; }

    void settle()
    {
      for (int i = 0; i < 5; ++i)
      {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::processEvents();
      }
    }

    QToolButton* button(QAction* action) { return qobject_cast<QToolButton*>(ui.toolBar->widgetForAction(action)); }
  };

  // Expert mode with empty slots, and a Script Lab that has never been opened
  void freshSettings(bool expert = true)
  {
    UserSettings::setExpertMode(expert);
    UserSettings::setPanelLayout(QByteArray());
    UserSettings::setPanelLayoutBeginner(QByteArray());
    UserSettings::setPanelLayoutScript(QByteArray());
    UserSettings::setPanelLayoutVersion(PanelLayout::CurrentVersion);
    UserSettings::setScriptLabSized(0);
  }

  // a place that fits the screen of the test machine
  QRect floatingPlace(const Program&, int index)
  {
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    const int w = std::max(300, std::min(420, screen.width() / 3));
    const int h = std::max(240, std::min(320, screen.height() / 3));
    return QRect(screen.left() + 60 + 30 * index, screen.top() + 40 + 40 * index, w, h);
  }
}

class TestWorkMode : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase() { Theme::apply(true); }
    void init() { freshSettings(); }
    void theProgramStartsOnAnimate();
    void switchingShowsAndHidesTheRightWindows();
    void theScriptSideShowsTheScriptLabBig();
    void theScriptLabItemAndCloseButtonSwitchToo();
    void eachSideKeepsItsOwnLayout();
    void theLayoutsComeBackFromTheSavedSettings();
    void beginnerAndExpertOnTheScriptSideTouchNoSlot();
    void theKeysAndTheButtonsWorkByKeyboard();
    void theActiveSideShowsByMoreThanColour();
    void resetLayoutResetsBothSidesAndTheSideYouAreOn();
    void aHoldKeepsPanelChangesFromSwitching();
};

void TestWorkMode::theProgramStartsOnAnimate()
{
  Program p;
  QCOMPARE(p.work->side(), Side::Animate);
  QVERIFY(p.work->animateAction()->isChecked());
  QVERIFY(!p.work->scriptAction()->isChecked());
  for (QDockWidget* dock : p.animationPanels()) QVERIFY(!dock->isHidden());
  QVERIFY(p.labDock->isHidden());
  QVERIFY(p.libraryDock->isHidden());
  // the two buttons stand first in the bar, in this order
  const QList<QAction*> actions = p.ui.toolBar->actions();
  QVERIFY(actions.size() >= 2);
  QCOMPARE(actions.at(0), p.work->animateAction());
  QCOMPARE(actions.at(1), p.work->scriptAction());
  // a saved Animate layout with the Script Lab open (an older version) starts with it closed
  p.labDock->show();
  p.settle();
  UserSettings::setPanelLayoutFor(true, p.reflower.layoutToSave(&p.window));
  Program again;
  again.startFromSaved();
  QVERIFY(again.labDock->isHidden());
  QCOMPARE(again.work->side(), Side::Animate);
}

void TestWorkMode::switchingShowsAndHidesTheRightWindows()
{
  Program p;
  QVERIFY(p.work->setSide(Side::Script));
  p.settle();
  QCOMPARE(p.work->side(), Side::Script);
  QVERIFY(p.work->scriptAction()->isChecked() && !p.work->animateAction()->isChecked());
  for (QDockWidget* dock : p.animationPanels()) QVERIFY2(dock->isHidden(), qPrintable(dock->objectName()));
  QVERIFY(p.libraryDock->isHidden());
  QVERIFY(!p.labDock->isHidden());
  QVERIFY(p.labDock->toggleViewAction()->isChecked());

  QVERIFY(p.work->setSide(Side::Animate));
  p.settle();
  QCOMPARE(p.work->side(), Side::Animate);
  QVERIFY(p.work->animateAction()->isChecked() && !p.work->scriptAction()->isChecked());
  for (QDockWidget* dock : p.animationPanels()) QVERIFY2(!dock->isHidden(), qPrintable(dock->objectName()));
  QVERIFY(p.labDock->isHidden());
  QVERIFY(p.libraryDock->isHidden());

  // switching to the side that is shown changes nothing
  const int calls = p.afterCalls;
  QVERIFY(!p.work->setSide(Side::Animate));
  QCOMPARE(p.afterCalls, calls);

  // the buttons do the same; switching is possible at any time, many times
  for (int i = 0; i < 3; ++i)
  {
    p.work->scriptAction()->trigger();
    p.settle();
    QCOMPARE(p.work->side(), Side::Script);
    QVERIFY(!p.labDock->isHidden() && p.panels.timelineDock->isHidden());
    p.work->animateAction()->trigger();
    p.settle();
    QCOMPARE(p.work->side(), Side::Animate);
    QVERIFY(p.labDock->isHidden() && !p.panels.timelineDock->isHidden());
  }
  // a click on the button of the side that is shown keeps it
  p.work->animateAction()->trigger();
  QCOMPARE(p.work->side(), Side::Animate);
  QVERIFY(p.work->animateAction()->isChecked());

  // the library is closed on both sides until asked for, and it stays an Animate panel
  p.libraryDock->show();
  p.settle();
  p.work->setSide(Side::Script);
  p.settle();
  QVERIFY(p.libraryDock->isHidden());
}

void TestWorkMode::theScriptSideShowsTheScriptLabBig()
{
  Program p;
  p.work->setSide(Side::Script);
  p.settle();
  QVERIFY(!p.labDock->isFloating());
  // the first time it opens big: most of the window, never more than leaves the 3D view a strip
  QVERIFY2(p.labDock->width() > p.window.width() / 3,
           qPrintable(QStringLiteral("lab %1 of window %2").arg(p.labDock->width()).arg(p.window.width())));
  QVERIFY(p.labDock->height() > p.window.height() / 2);
}

void TestWorkMode::theScriptLabItemAndCloseButtonSwitchToo()
{
  Program p;
  // the top bar button, View > Script Lab and Ctrl+Shift+L are this action
  p.labDock->toggleViewAction()->trigger();
  p.settle();
  QCOMPARE(p.work->side(), Side::Script);
  QVERIFY(!p.labDock->isHidden());
  QVERIFY(p.panels.timelineDock->isHidden());
  QVERIFY(p.work->scriptAction()->isChecked());

  // the panel's close button, and the item again, go back to Animate
  p.labDock->close();
  p.settle();
  QCOMPARE(p.work->side(), Side::Animate);
  QVERIFY(!p.panels.timelineDock->isHidden());
  QVERIFY(p.work->animateAction()->isChecked());

  p.labDock->show();   // shown from code (Help > Script Lab steps does this)
  p.settle();
  QCOMPARE(p.work->side(), Side::Script);
  p.labDock->toggleViewAction()->trigger();
  p.settle();
  QCOMPARE(p.work->side(), Side::Animate);
  QVERIFY(p.labDock->isHidden());
}

void TestWorkMode::eachSideKeepsItsOwnLayout()
{
  Program p;
  // Animate: the Joints list floats and Selected joint is wide
  p.panels.jointsDock->setFloating(true);
  p.panels.jointsDock->setGeometry(floatingPlace(p, 0));
  p.window.resizeDocks({p.panels.jointCardDock}, {std::max(260, p.window.width() / 5)}, Qt::Horizontal);
  p.settle();
  const int cardWidth = p.panels.jointCardDock->width();
  const QRect jointsBefore = p.panels.jointsDock->geometry();

  // Script: the Script Lab floats in a place of its own and the Timeline is opened beside it
  p.work->setSide(Side::Script);
  p.settle();
  QVERIFY(p.panels.jointsDock->isHidden());
  const QRect labPlace = floatingPlace(p, 3);
  p.labDock->setFloating(true);   // Qt hides and shows it for a moment: no switch to Animate
  p.settle();
  QCOMPARE(p.work->side(), Side::Script);
  p.labDock->setGeometry(labPlace);
  p.panels.timelineDock->setFloating(false);
  p.window.addDockWidget(Qt::BottomDockWidgetArea, p.panels.timelineDock);
  p.panels.timelineDock->show();
  p.settle();
  const QRect labBefore = p.labDock->geometry();
  QVERIFY(p.labDock->isFloating());

  // back on Animate: as it was left, and the Script side's changes did not leak into it
  p.work->setSide(Side::Animate);
  p.settle();
  QVERIFY(p.labDock->isHidden());
  QVERIFY(!p.panels.timelineDock->isHidden() && !p.panels.timelineDock->isFloating());
  QVERIFY(!p.panels.jointsDock->isHidden() && p.panels.jointsDock->isFloating());
  QCOMPARE(p.panels.jointsDock->geometry(), jointsBefore);
  QCOMPARE(p.panels.jointCardDock->width(), cardWidth);

  // and on Script again: as it was left
  p.work->setSide(Side::Script);
  p.settle();
  QVERIFY(p.panels.jointsDock->isHidden());
  QVERIFY(!p.labDock->isHidden() && p.labDock->isFloating());
  QCOMPARE(p.labDock->geometry(), labBefore);
  QVERIFY(!p.panels.timelineDock->isHidden() && !p.panels.timelineDock->isFloating());
}

void TestWorkMode::theLayoutsComeBackFromTheSavedSettings()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

  QRect jointsBefore, labBefore;
  QPoint windowPos;
  QByteArray geometry;
  {
    Program p;
    windowPos = p.window.pos();
    geometry = p.window.saveGeometry();
    p.panels.jointsDock->setFloating(true);
    p.panels.jointsDock->setGeometry(floatingPlace(p, 0));
    p.settle();
    jointsBefore = p.panels.jointsDock->geometry();
    p.work->setSide(Side::Script);
    p.settle();
    p.labDock->setFloating(true);
    p.labDock->setGeometry(floatingPlace(p, 3));
    p.settle();
    labBefore = p.labDock->geometry();
    // the program closes while the Script side is shown: its layout goes to the Script slot,
    // the Animate slot stays as it was left
    p.work->saveCurrentLayout();
    UserSettings::writeSettings();
  }
  UserSettings::setPanelLayout(QByteArray());
  UserSettings::setPanelLayoutBeginner(QByteArray());
  UserSettings::setPanelLayoutScript(QByteArray());
  UserSettings::readSettings();
  QVERIFY(!UserSettings::panelLayoutScript().isEmpty());
  QVERIFY(!UserSettings::panelLayoutFor(true).isEmpty());
  QVERIFY(UserSettings::panelLayoutScript() != UserSettings::panelLayoutFor(true));

  // the next start: always on Animate, with the Animate layout
  Program q;
  q.window.move(windowPos);   // the offscreen platform places each window by itself
  q.startFromSaved(geometry);
  QCOMPARE(q.work->side(), Side::Animate);
  QVERIFY(q.labDock->isHidden());
  QVERIFY(q.panels.jointsDock->isFloating());
  QCOMPARE(q.panels.jointsDock->geometry(), jointsBefore);
  // and the Script side is as it was
  q.work->setSide(Side::Script);
  q.settle();
  QVERIFY(q.labDock->isFloating() && !q.labDock->isHidden());
  QCOMPARE(q.labDock->geometry(), labBefore);
}

void TestWorkMode::beginnerAndExpertOnTheScriptSideTouchNoSlot()
{
  Program p;
  p.work->setSide(Side::Script);
  p.settle();
  const QByteArray animateSlot = UserSettings::panelLayoutFor(true);
  const QByteArray scriptSlot = UserSettings::panelLayoutScript();
  QVERIFY(!animateSlot.isEmpty());
  QVERIFY(UserSettings::panelLayoutFor(false).isEmpty());

  // the window leaves the slots alone while the Script side is shown (it asks side()); the
  // mode itself changes
  UserSettings::setExpertMode(false);
  QCOMPARE(UserSettings::panelLayoutFor(true), animateSlot);
  QCOMPARE(UserSettings::panelLayoutScript(), scriptSlot);
  QVERIFY(UserSettings::panelLayoutFor(false).isEmpty());

  // Animate now shows the Beginner side: no saved layout yet, so its default, Joints list closed
  p.work->setSide(Side::Animate);
  p.settle();
  QVERIFY(p.panels.jointsDock->isHidden());
  QVERIFY(!p.panels.jointCardDock->isHidden() && !p.panels.timelineDock->isHidden());
  QVERIFY(p.labDock->isHidden());
  // the Expert slot is still the Expert layout, and the Beginner slot is the one just left
  QCOMPARE(UserSettings::panelLayoutFor(true), animateSlot);
  QVERIFY(UserSettings::panelLayoutFor(false).isEmpty());
  p.work->setSide(Side::Script);
  p.settle();
  QVERIFY(!UserSettings::panelLayoutFor(false).isEmpty());
}

void TestWorkMode::theKeysAndTheButtonsWorkByKeyboard()
{
  Program p;
  // exact keys; Ctrl+1 is the view toolbar's Back key and Ctrl+Alt+digit is AltGr on Norwegian keyboards
  QCOMPARE(p.work->animateAction()->shortcut(), QKeySequence(Qt::Key_F7));
  QCOMPARE(p.work->scriptAction()->shortcut(), QKeySequence(Qt::Key_F8));
  QCOMPARE(p.work->animateAction()->shortcutContext(), Qt::WindowShortcut);
  QVERIFY(p.work->animateAction()->toolTip().contains(QLatin1String("F7")));
  QVERIFY(p.work->scriptAction()->toolTip().contains(QLatin1String("F8")));

  // the keys, pressed in the window
  p.window.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&p.window));
  QTest::keyClick(&p.window, Qt::Key_F8);
  p.settle();
  QCOMPARE(p.work->side(), Side::Script);
  QTest::keyClick(&p.window, Qt::Key_F7);
  p.settle();
  QCOMPARE(p.work->side(), Side::Animate);

  // the buttons are in the Tab order and answer Space and Enter
  QToolButton* script = p.button(p.work->scriptAction());
  QToolButton* animate = p.button(p.work->animateAction());
  QVERIFY(script && animate);
  QVERIFY(script->focusPolicy() & Qt::TabFocus);
  QVERIFY(animate->focusPolicy() & Qt::TabFocus);
  QTest::keyClick(script, Qt::Key_Space);
  p.settle();
  QCOMPARE(p.work->side(), Side::Script);
  QTest::keyClick(animate, Qt::Key_Space);
  p.settle();
  QCOMPARE(p.work->side(), Side::Animate);

  // accessible names: the side's word, and what it does
  QCOMPARE(script->accessibleName(), QStringLiteral("Script"));
  QCOMPARE(animate->accessibleName(), QStringLiteral("Animate"));
  QVERIFY(!script->accessibleDescription().isEmpty());
  QVERIFY(!animate->accessibleDescription().isEmpty());
  QCOMPARE(p.work->animateAction()->text(), QStringLiteral("Animate"));
  QCOMPARE(p.work->scriptAction()->text(), QStringLiteral("Script"));
}

void TestWorkMode::theActiveSideShowsByMoreThanColour()
{
  Program p;
  QToolButton* script = p.button(p.work->scriptAction());
  QToolButton* animate = p.button(p.work->animateAction());
  QVERIFY(script && animate);
  for (Side side : {Side::Animate, Side::Script, Side::Animate})
  {
    p.work->setSide(side);
    p.settle();
    QToolButton* active = side == Side::Animate ? animate : script;
    QToolButton* other = side == Side::Animate ? script : animate;
    // pressed (the fill), bold (the label) and a tick icon the other side has not
    QVERIFY(active->isChecked() && !other->isChecked());
    QVERIFY(active->font().bold() && !other->font().bold());
    QVERIFY(!active->icon().isNull() && !other->icon().isNull());
    QVERIFY(active->icon().pixmap(16, 16).toImage() != other->icon().pixmap(16, 16).toImage());
  }
}

void TestWorkMode::resetLayoutResetsBothSidesAndTheSideYouAreOn()
{
  // what PinglyMotionWindow::resetPanelLayout does, in its order
  auto reset = [](Program& p)
  {
    WorkMode::Switcher::Hold hold(p.work);
    PanelWindow::applyModeLayout(&p.window, p.panels.jointsDock, p.panels.jointCardDock, p.panels.timelineDock,
                                 UserSettings::expertMode());
    p.labDock->setFloating(false);
    p.window.addDockWidget(Qt::RightDockWidgetArea, p.labDock);
    p.labDock->hide();
    p.lab->resetWindows();
    p.libraryDock->setFloating(false);
    p.window.tabifyDockWidget(p.panels.jointCardDock, p.libraryDock);
    p.libraryDock->hide();
    p.work->layoutsReset();
  };
  auto arrange = [](Program& p)
  {
    // Animate: Joints floating; Script: Script Lab floating
    p.panels.jointsDock->setFloating(true);
    p.panels.jointsDock->setGeometry(floatingPlace(p, 0));
    p.work->setSide(Side::Script);
    p.labDock->setFloating(true);
    p.labDock->setGeometry(floatingPlace(p, 3));
    p.settle();
  };

  // on the Animate side
  {
    Program p;
    arrange(p);
    p.work->setSide(Side::Animate);
    p.settle();
    QVERIFY(p.panels.jointsDock->isFloating());
    reset(p);
    p.settle();
    QCOMPARE(p.work->side(), Side::Animate);
    for (QDockWidget* dock : p.animationPanels()) QVERIFY(!dock->isHidden() && !dock->isFloating());
    QCOMPARE(p.window.dockWidgetArea(p.panels.timelineDock), Qt::BottomDockWidgetArea);
    QVERIFY(p.labDock->isHidden());
    QVERIFY(UserSettings::panelLayoutScript().isEmpty());
    // the Script side starts again: default layout, the animation panels closed, the lab docked and big
    p.work->setSide(Side::Script);
    p.settle();
    QVERIFY(!p.labDock->isHidden() && !p.labDock->isFloating());
    for (QDockWidget* dock : p.animationPanels()) QVERIFY(dock->isHidden());
    QVERIFY(p.labDock->width() > p.window.width() / 3);
  }

  // on the Script side
  {
    freshSettings();
    Program p;
    arrange(p);
    QCOMPARE(p.work->side(), Side::Script);
    reset(p);
    p.settle();
    QCOMPARE(p.work->side(), Side::Script);
    QVERIFY(!p.labDock->isHidden() && !p.labDock->isFloating());
    QCOMPARE(p.window.dockWidgetArea(p.labDock), Qt::RightDockWidgetArea);
    for (QDockWidget* dock : p.animationPanels()) QVERIFY(dock->isHidden());
    QVERIFY(p.libraryDock->isHidden());
    QVERIFY(p.labDock->width() > p.window.width() / 3);
    QVERIFY(UserSettings::panelLayoutScript().isEmpty());
    // the Animate side starts again too: the Joints list that floated is docked as in the default
    p.work->setSide(Side::Animate);
    p.settle();
    for (QDockWidget* dock : p.animationPanels()) QVERIFY(!dock->isHidden() && !dock->isFloating());
    QCOMPARE(p.window.dockWidgetArea(p.panels.jointsDock), Qt::LeftDockWidgetArea);
    QVERIFY(p.labDock->isHidden());
  }
}

void TestWorkMode::aHoldKeepsPanelChangesFromSwitching()
{
  Program p;
  {
    WorkMode::Switcher::Hold hold(p.work);
    p.labDock->show();
    p.settle();
    QCOMPARE(p.work->side(), Side::Animate);
    p.labDock->hide();
    p.settle();
    QVERIFY(!p.work->setSide(Side::Script));
  }
  QCOMPARE(p.work->side(), Side::Animate);
  QVERIFY(p.work->setSide(Side::Script));
}

QTEST_MAIN(TestWorkMode)
#include "tst_workmode.moc"
