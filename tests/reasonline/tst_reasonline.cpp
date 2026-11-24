/*
 * PinglyMotion - test of the reason line in the status bar (UX phase 1c)
 *
 * The exact line for each kind of greyed-out control (top bar button,
 * transport button, menu item, greyed panel), none for a working control, the
 * line gone when the mouse leaves, both themes, and the format against the
 * mockup file itself (docs/mockup/index.html).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QAction>
#include <QFile>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QStatusTipEvent>
#include <QToolBar>
#include <QToolButton>
#include <QtTest>

#include "menuspec.h"
#include "reasonline.h"
#include "timelineclip.h"
#include "theme.h"
#include "topbar.h"
#include "playback.h"
#include "transportbar.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  void enter(QWidget* widget)
  {
    QEnterEvent event(QPointF(2, 2), QPointF(2, 2), QPointF(2, 2));
    QApplication::sendEvent(widget, &event);
  }

  void leave(QWidget* widget)
  {
    QEvent event(QEvent::Leave);
    QApplication::sendEvent(widget, &event);
  }

  struct Window
  {
    QMainWindow window;
    QToolBar* bar = nullptr;
    QAction undo{QStringLiteral("Undo")};
    QAction redo{QStringLiteral("Redo")};
    QAction save{QStringLiteral("Save")};
    QAction exportAnim{QStringLiteral("Export")};
    QAction scriptLab{QStringLiteral("Script Lab")};
    TopBar* top = nullptr;
    ReasonLine* line = nullptr;

    Window()
    {
      bar = window.addToolBar(QStringLiteral("top"));
      undo.setEnabled(false);
      undo.setStatusTip(QCoreApplication::translate("MenuSpec", MenuSpec::nothingToUndo));
      redo.setEnabled(false);
      redo.setStatusTip(QCoreApplication::translate("MenuSpec", MenuSpec::nothingToRedo));
      scriptLab.setCheckable(true);
      TopBar::Sources sources;
      sources.undo = &undo;
      sources.redo = &redo;
      sources.save = &save;
      sources.exportAnim = &exportAnim;
      sources.scriptLab = &scriptLab;
      top = new TopBar(bar, sources, &window);
      window.setCentralWidget(new QWidget);
      window.statusBar();
      line = new ReasonLine(window.statusBar(), &window);
      line->watchMenus();
    }

    QWidget* button(const char* id) { return bar->widgetForAction(top->button(id)); }
  };
}

class TestReasonLine : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase() { Theme::apply(true); }
    void formatIsNameColonReason();
    void formatMatchesMockup();
    void emptyAtStart();
    void greyedTopBarButtonsSayWhy();
    void workingButtonShowsNothing();
    void leavingClearsTheLine();
    void movingToAWorkingButtonClears();
    void saveAndExportWithoutAnimation();
    void transportButtonsSayWhy();
    void greyedPanelNamesItself();
    void menuItemsSayWhy();
    void menuPointerOverGreyedItems();
    void menuStatusTipIsNotShownTwice();
    void noReasonWithoutStatusTip();
    void bothThemes();
};

void TestReasonLine::formatIsNameColonReason()
{
  QCOMPARE(ReasonLine::lineText(QStringLiteral("Save"), QStringLiteral("Open an animation first.")),
           QStringLiteral("Save: Open an animation first."));
  QCOMPARE(ReasonLine::lineText(QString(), QStringLiteral("Open an animation first.")), QStringLiteral("Open an animation first."));
}

// the mockup writes the line as name, colon, reason, with the info icon, in the text colour
void TestReasonLine::formatMatchesMockup()
{
  const QString mockup = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/mockup/index.html"));
  QVERIFY(!mockup.isEmpty());
  QVERIFY(mockup.contains(QStringLiteral("say(d.name + \": \" + d.why, \"reason\")")));
  QVERIFY(mockup.contains(QStringLiteral("kind === \"reason\" ? icon(\"info\")")));
  QVERIFY(mockup.contains(QStringLiteral(".status .msgline.reason { color:var(--text); }")));
}

void TestReasonLine::emptyAtStart()
{
  Window w;
  QCOMPARE(w.line->text(), QString());
  QVERIFY(!w.line->showing());
  QVERIFY(w.line->iconLabel()->isHidden());
}

void TestReasonLine::greyedTopBarButtonsSayWhy()
{
  Window w;
  enter(w.button("topUndoAction"));
  QCOMPARE(w.line->text(), QStringLiteral("Undo: Nothing to undo yet."));
  QCOMPARE(w.line->textLabel()->text(), w.line->text());
  QVERIFY(!w.line->iconLabel()->isHidden());
  leave(w.button("topUndoAction"));
  enter(w.button("topRedoAction"));
  QCOMPARE(w.line->text(), QStringLiteral("Redo: Nothing to redo. Redo works after Undo."));
  leave(w.button("topRedoAction"));
}

void TestReasonLine::workingButtonShowsNothing()
{
  Window w;
  w.top->setAnimationOpen(true);
  for (const char* id : {"topSaveAction", "topExportAction", "topScriptLabAction", "topThemeAction", "topSetupAction"})
  {
    enter(w.button(id));
    QCOMPARE(w.line->text(), QString());
    QVERIFY(!w.line->showing());
    leave(w.button(id));
  }
}

void TestReasonLine::leavingClearsTheLine()
{
  Window w;
  enter(w.button("topUndoAction"));
  QVERIFY(w.line->showing());
  leave(w.button("topUndoAction"));
  QCOMPARE(w.line->text(), QString());
  QVERIFY(w.line->iconLabel()->isHidden());
}

void TestReasonLine::movingToAWorkingButtonClears()
{
  Window w;
  w.top->setAnimationOpen(true);
  enter(w.button("topUndoAction"));
  QVERIFY(w.line->showing());
  enter(w.button("topSaveAction"));
  QCOMPARE(w.line->text(), QString());
}

void TestReasonLine::saveAndExportWithoutAnimation()
{
  Window w;
  w.top->setAnimationOpen(false);
  enter(w.button("topSaveAction"));
  QCOMPARE(w.line->text(), QStringLiteral("Save: Open an animation first."));
  leave(w.button("topSaveAction"));
  enter(w.button("topExportAction"));
  QCOMPARE(w.line->text(), QStringLiteral("Export: Open an animation first."));
  leave(w.button("topExportAction"));
  // with an animation open they work and say nothing
  w.top->setAnimationOpen(true);
  enter(w.button("topSaveAction"));
  QCOMPARE(w.line->text(), QString());
}

void TestReasonLine::transportButtonsSayWhy()
{
  Window w;
  TransportBar* transport = new TransportBar(w.window.centralWidget());
  transport->setPlaystate(PLAYSTATE_STOPPED);
  QPushButton* start = transport->findChild<QPushButton*>(QStringLiteral("transportStartButton"));
  QPushButton* stop = transport->findChild<QPushButton*>(QStringLiteral("transportStopButton"));
  QVERIFY(start && stop);
  QVERIFY(start->isEnabled());
  QVERIFY(!stop->isEnabled());
  enter(stop);
  QCOMPARE(w.line->text(), QStringLiteral("Stop: It is not playing. Click Start to play it."));
  leave(stop);
  enter(start);
  QCOMPARE(w.line->text(), QString());
  leave(start);

  transport->setPlaystate(PLAYSTATE_PLAYING);
  QVERIFY(!start->isEnabled());
  enter(start);
  QCOMPARE(w.line->text(), QStringLiteral("Start: It is already playing. Click Stop to stop it."));
  leave(start);
  enter(stop);
  QCOMPARE(w.line->text(), QString());
}

void TestReasonLine::greyedPanelNamesItself()
{
  Window w;
  QWidget* panel = new QWidget(w.window.centralWidget());
  QWidget* inner = new QPushButton(QStringLiteral("Inner"), panel);
  panel->setEnabled(false);
  panel->setProperty("reasonName", QStringLiteral("Joints"));
  panel->setProperty("reasonWhy", MenuSpec::openFirst);
  enter(inner);
  QCOMPARE(w.line->text(), QStringLiteral("Joints: Open an animation first."));
  leave(inner);
  QCOMPARE(w.line->text(), QString());
  // working again: no line
  panel->setEnabled(true);
  panel->setProperty("reasonName", QString());
  panel->setProperty("reasonWhy", QString());
  enter(inner);
  QCOMPARE(w.line->text(), QString());
}

void TestReasonLine::menuItemsSayWhy()
{
  Window w;
  QMenu* menu = w.window.menuBar()->addMenu(QStringLiteral("&Edit"));
  QAction* paste = menu->addAction(QStringLiteral("Paste mirrored\tCtrl+Shift+V"));
  paste->setEnabled(false);
  // UX phase 3b: greyed with the mockup's reason while nothing is copied
  paste->setStatusTip(ClipSpec::text(ClipSpec::copyFirst));
  QAction* setKey = menu->addAction(QStringLiteral("Set key"));
  QAction* next = menu->addAction(QStringLiteral("&Next area"));
  next->setEnabled(false);
  next->setStatusTip(MenuSpec::reason(MenuSpec::viewNextArea));
  w.line->watchMenus();

  emit menu->hovered(paste);
  QCOMPARE(w.line->text(), QStringLiteral("Paste mirrored: Copy a stretch of the timeline first (select keys, then Ctrl+C)."));
  emit menu->hovered(setKey);
  QCOMPARE(w.line->text(), QString());
  emit menu->hovered(next);
  QCOMPARE(w.line->text(), QStringLiteral("Next area: Next area is not built yet. For now use Tab to move between the fields."));
  emit menu->aboutToHide();
  QCOMPARE(w.line->text(), QString());
}

// the real route: the pointer moves over the items of an open menu (Qt says nothing for a greyed-out item)
void TestReasonLine::menuPointerOverGreyedItems()
{
  Window w;
  QMenu* menu = w.window.menuBar()->addMenu(QStringLiteral("&Edit"));
  QAction* redo = menu->addAction(QStringLiteral("Redo\tCtrl+Shift+Z"));
  redo->setEnabled(false);
  redo->setStatusTip(QCoreApplication::translate("MenuSpec", MenuSpec::nothingToRedo));
  QAction* setKey = menu->addAction(QStringLiteral("Set key"));
  w.line->watchMenus();
  menu->popup(QPoint(20, 20));
  QVERIFY(QTest::qWaitForWindowExposed(menu));

  const auto move = [&](QAction* action)
  {
    const QPointF p = menu->actionGeometry(action).center();
    QMouseEvent event(QEvent::MouseMove, p, menu->mapToGlobal(p), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(menu, &event);
  };
  move(redo);
  QCOMPARE(w.line->text(), QStringLiteral("Redo: Nothing to redo. Redo works after Undo."));
  move(setKey);
  QCOMPARE(w.line->text(), QString());
  move(redo);
  QVERIFY(w.line->showing());
  leave(menu);
  QCOMPARE(w.line->text(), QString());
  menu->hide();
}

// Qt's own status message with the same words must not show beside the line
void TestReasonLine::menuStatusTipIsNotShownTwice()
{
  Window w;
  enter(w.button("topUndoAction"));
  QVERIFY(w.line->showing());
  QStatusTipEvent tip(QCoreApplication::translate("MenuSpec", MenuSpec::nothingToUndo));
  QApplication::sendEvent(&w.window, &tip);
  QCOMPARE(w.window.statusBar()->currentMessage(), QString());
  QCOMPARE(w.line->text(), QStringLiteral("Undo: Nothing to undo yet."));
}

void TestReasonLine::noReasonWithoutStatusTip()
{
  Window w;
  QPushButton* plain = new QPushButton(QStringLiteral("Plain"), w.window.centralWidget());
  plain->setEnabled(false);
  enter(plain);
  QCOMPARE(w.line->text(), QString());
}

// the icon and the text take the colour of the theme's text, dark and light
void TestReasonLine::bothThemes()
{
  Window w;
  for (bool dark : {true, false})
  {
    Theme::apply(dark);
    w.line->applyLook();
    enter(w.button("topUndoAction"));
    const QColor text = Theme::color(Theme::Token::Text);
    QVERIFY(w.line->textLabel()->styleSheet().contains(text.name()));
    const QImage icon = w.line->iconLabel()->pixmap().toImage().convertToFormat(QImage::Format_ARGB32);
    QVERIFY(!icon.isNull());
    bool found = false;
    for (int y = 0; y < icon.height() && !found; y++)
      for (int x = 0; x < icon.width() && !found; x++)
        if (qAlpha(icon.pixel(x, y)) == 255 && (icon.pixel(x, y) & 0xFFFFFF) == (text.rgb() & 0xFFFFFF)) found = true;
    QVERIFY2(found, dark ? "dark" : "light");
    leave(w.button("topUndoAction"));
  }
  Theme::apply(true);
}

QTEST_MAIN(TestReasonLine)
#include "tst_reasonline.moc"
