/*
 * PinglyMotion - the Script Lab page «Improve a setup» (queue 1, item 5, parts b and c)
 *
 * The page in the real panel, with exact texts: the empty state and the greyed Record button with its
 * reason; a watched setup's weight lines, table, before and after, suggestions and «Can this run here?»;
 * Record today's behaviour starts the recording on Player; a suggestion about a line shows that line in
 * the editor; a replay after a changed script says what changed in the weight; the keyboard order; and
 * the page in both themes and at 100 and 200 percent text with nothing clipped.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>

#include "labimproveview.h"
#include "labwatcher.h"
#include "rlvstate.h"
#include "scriptlabpanel.h"
#include "uirender.h"
#include "theme.h"
#include "usersettings.h"

class TestLabImproveView : public QObject
{
    Q_OBJECT
  private slots:
    void init();
    void emptyState();
    void watchedSetup();
    void recordAndPlace();
    void beforeAndAfter();
    void keyboardOrder();
    void noClipping();

  private:
    static bool writeFile(const QString& path, const QByteArray& text);
    static QString makeSetup(const QTemporaryDir& dir);
};

bool TestLabImproveView::writeFile(const QString& path, const QByteArray& text)
{
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  return f.open(QIODevice::WriteOnly) && f.write(text) == text.size();
}

// the same small setup as tests/tst_labweight.cpp
QString TestLabImproveView::makeSetup(const QTemporaryDir& dir)
{
  const QString top = dir.filePath("Setup");
  writeFile(top + "/Root/hub.lsl", "default { state_entry() {\n  integer i;\n  for (i = 1; i <= 3; ++i) llMessageLinked(LINK_SET, i, \"go\", \"\");\n} }\n");
  writeFile(top + "/Root/idle.lsl", "list names = [\"a\", \"b\", \"c\", \"d\", \"e\", \"f\", \"g\", \"h\", \"i\", \"j\",\n"
                                    "              \"k\", \"l\", \"m\", \"n\", \"o\", \"p\", \"q\", \"r\", \"s\", \"t\"];\n"
                                    "integer spare;\n"
                                    "noop() { }\n"
                                    "default { link_message(integer s, integer n, string m, key id) { if (n == 99) llOwnerSay(llList2String(names, 0)); } }\n");
  writeFile(top + "/Root/idle2.lsl", "default { link_message(integer s, integer n, string m, key id) { if (n == 99) llOwnerSay(m); } }\n");
  writeFile(top + "/User/user.lsl", "integer got;\n"
                                    "default { state_entry() { llSetMemoryLimit(16384); }\n"
                                    "  link_message(integer s, integer n, string m, key id) { got = got + n; if (got == 6) llOwnerSay(\"all\"); } }\n");
  return top;
}

void TestLabImproveView::init()
{
  UserSettings::setScriptLabWindows(QString());
}

void TestLabImproveView::emptyState()
{
  ScriptLabPanel panel;
  LabWatcher watcher;
  panel.setWatcher(&watcher);
  LabImproveView* view = panel.improveView();
  QVERIFY(view);
  QVERIFY(panel.page(ScriptLabPanel::ImprovePage)->isAncestorOf(view));
  QCOMPARE(view->findChild<QLabel*>("scriptLabImproveTitle")->text(), QString("Improve a finished setup"));
  QCOMPARE(view->findChild<QLabel*>("scriptLabImproveSteps")->text(),
           QString("1. Watch the folder as one object on Live scripts.\n2. Record what it does today on Player.\n3. Read below what can be better."));
  QCOMPARE(view->statusLine(), QString("Watch a folder as one object on Live scripts, or press Import a setup. "
                                       "Then this page shows how heavy it is and what can be better."));
  QPushButton* import = view->findChild<QPushButton*>("scriptLabImproveImport");
  QPushButton* record = view->findChild<QPushButton*>("scriptLabImproveRecord");
  QCOMPARE(import->text(), QString("Import a setup"));
  QVERIFY(import->isEnabled());
  QCOMPARE(record->text(), QString("Record today's behaviour"));
  // greyed, with the reason in its tip (charter P6)
  QVERIFY(!record->isEnabled());
  QCOMPARE(record->toolTip(), QString("Import a setup first."));
  QVERIFY(!import->icon().isNull());
  QVERIFY(!record->icon().isNull());
  QCOMPARE(view->weightLines(), QStringList());
  QCOMPARE(view->tableRows(), QStringList());
  QCOMPARE(view->suggestionLines(), QStringList());
  QCOMPARE(view->runRows(), QStringList());
  // Import a setup asks for the folder (the panel connects it to its own Watch folder dialog, so the click is tried
  // on a page of its own here)
  LabImproveView alone;
  QSignalSpy asked(&alone, &LabImproveView::importRequested);
  alone.show();
  QTest::mouseClick(alone.findChild<QPushButton*>("scriptLabImproveImport"), Qt::LeftButton);
  QCOMPARE(asked.count(), 1);
}

void TestLabImproveView::watchedSetup()
{
  QTemporaryDir dir;
  const QString top = makeSetup(dir);
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QVERIFY(panel.watchFolder(top));
  LabImproveView* view = panel.improveView();
  QCOMPARE(view->statusLine(), QString());
  QCOMPARE(view->weightLines(), QStringList({
    "4 scripts. Second Life keeps 64 KB for each, less for a script with a smaller limit: 208 KB in all.",
    "1 script sets a smaller limit with llSetMemoryLimit.",
    "Messages: 3 sent, 9 got, 6 not used (the script that got it did nothing with it)."}));
  QTreeWidget* table = view->findChild<QTreeWidget*>("scriptLabImproveTable");
  QCOMPARE(table->headerItem()->text(0), QString("Script"));
  QCOMPARE(table->headerItem()->text(5), QString("Not used"));
  QCOMPARE(view->tableRows(), QStringList({"hub | 64 KB | 0 KB | 3 | 0 | 0",
                                           "idle | 64 KB | 0.4 KB | 0 | 3 | 3",
                                           "idle2 | 64 KB | 0 KB | 0 | 3 | 3",
                                           "user | 16 KB | 0 KB | 0 | 3 | 0"}));
  QCOMPARE(table->topLevelItem(0)->toolTip(0), QString("hub, in Root. 6 of its messages were not used by the script that got them."));
  QCOMPARE(view->beforeAfterLines(), QStringList({"Record what the setup does on Player. Every later save replays it and shows the weight before and after."}));
  const QStringList better = view->suggestionLines();
  QCOMPARE(better.size(), 5);
  QVERIFY(better.at(0).startsWith("idle: the list names is written with 20 entries."));
  QVERIFY(better.at(1).startsWith("Messages that the script getting them did nothing with: hub (3 sent, 6 not used)."));
  QVERIFY(better.at(4).startsWith("idle never uses the global variable spare and the function noop."));
  QListWidget* list = view->findChild<QListWidget*>("scriptLabImproveBetter");
  QCOMPARE(list->item(0)->toolTip(), QString("Root/idle.lsl, line 1. Press Enter to see it in the editor."));
  QCOMPARE(list->item(2)->toolTip(), QString());   // about several scripts: no line
  QCOMPARE(view->runRows(), QStringList({"hub: Runs here.", "idle: Runs here.", "idle2: Runs here.", "user: Runs here."}));
  QTreeWidget* run = view->findChild<QTreeWidget*>("scriptLabImproveRun");
  QCOMPARE(run->topLevelItem(0)->childCount(), 2);
  QCOMPARE(run->topLevelItem(0)->child(0)->text(0), QString("Works: 1 of the 1 built-in functions it calls run here."));
  QCOMPARE(run->topLevelItem(0)->child(1)->text(0), QString("Works: It handled these events: state_entry."));
  QVERIFY(!run->topLevelItem(0)->icon(0).isNull());   // a word and an icon, not colour alone (charter P10)
  QPushButton* record = view->findChild<QPushButton*>("scriptLabImproveRecord");
  QVERIFY(record->isEnabled());
  QCOMPARE(record->toolTip(), QString("Go to Player and record what the setup does today. Every later save replays it and compares."));
}

void TestLabImproveView::recordAndPlace()
{
  QTemporaryDir dir;
  const QString top = makeSetup(dir);
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QVERIFY(panel.watchFolder(top));
  panel.resize(900, 700);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.showPage(ScriptLabPanel::ImprovePage);
  LabImproveView* view = panel.improveView();
  // a suggestion about a line: Enter shows it in the editor on Live scripts
  QListWidget* list = view->findChild<QListWidget*>("scriptLabImproveBetter");
  list->setCurrentRow(4);   // idle never uses spare: line 3
  list->setFocus();
  QTest::keyClick(list, Qt::Key_Return);
  QCOMPARE(panel.currentPage(), ScriptLabPanel::LivePage);
  QCOMPARE(panel.shownScript(), QString("Root/idle.lsl"));
  QCOMPARE(panel.scriptView()->textCursor().blockNumber(), 2);
  // a suggestion about several scripts has no line: nothing moves
  panel.showPage(ScriptLabPanel::ImprovePage);
  list->setCurrentRow(2);
  QTest::keyClick(list, Qt::Key_Return);
  QCOMPARE(panel.currentPage(), ScriptLabPanel::ImprovePage);
  // Record today's behaviour: Player comes to the front and records; the button is greyed and says why
  QPushButton* record = view->findChild<QPushButton*>("scriptLabImproveRecord");
  QTest::mouseClick(record, Qt::LeftButton);
  QCOMPARE(panel.currentPage(), ScriptLabPanel::PlayerPage);
  QVERIFY(watcher.recording());
  QVERIFY(!record->isEnabled());
  QCOMPARE(record->toolTip(), QString("Recording now: stop it on Player."));
  QVERIFY(panel.setRecording(false));
  QVERIFY(record->isEnabled());
}

void TestLabImproveView::beforeAndAfter()
{
  QTemporaryDir dir;
  const QString top = makeSetup(dir);
  QVERIFY(writeFile(top + "/recipe.rlvtest", "wait 1\n"));
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QVERIFY(panel.watchFolder(top));
  LabImproveView* view = panel.improveView();
  QCOMPARE(view->beforeAfterLines(), QStringList({"This is the first replay with the weight, so there is nothing to compare yet."}));
  // hub sends to the prim that needs it: the save replays the recording, and the page says what changed
  QVERIFY(writeFile(top + "/Root/hub.lsl", "default { state_entry() {\n  integer i;\n  for (i = 1; i <= 3; ++i) llMessageLinked(2, i, \"go\", \"\");\n} }\n"));
  watcher.runScript("Root/hub.lsl");
  QCOMPARE(view->beforeAfterLines(), QStringList({"Before: 9 messages got, 6 not used. Now: 3 got, 0 not used.",
                                                  "idle: did nothing with 3, now 0.",
                                                  "idle2: did nothing with 3, now 0."}));
  QCOMPARE(view->weightLines().last(), QString("Messages: 3 sent, 3 got, 0 not used (the script that got it did nothing with it)."));
  // the suggestions follow the new run: hub's messages are used now
  for (const QString& s : view->suggestionLines()) QVERIFY2(!s.startsWith("Messages that"), qPrintable(s));
  watcher.runScript("Root/hub.lsl");
  QCOMPARE(view->beforeAfterLines(), QStringList({"Nothing changed in the weight since the last replay."}));
}

void TestLabImproveView::keyboardOrder()
{
  QTemporaryDir dir;
  const QString top = makeSetup(dir);
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QVERIFY(panel.watchFolder(top));
  panel.resize(900, 700);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.activateWindow();
  panel.showPage(ScriptLabPanel::ImprovePage);
  LabImproveView* view = panel.improveView();
  // Tab goes down the page in screen order (charter P8); every control has a name
  const QList<QWidget*> order = {view->findChild<QPushButton*>("scriptLabImproveImport"), view->findChild<QPushButton*>("scriptLabImproveRecord"),
                                 view->findChild<QTreeWidget*>("scriptLabImproveTable"), view->findChild<QListWidget*>("scriptLabImproveBetter"),
                                 view->findChild<QTreeWidget*>("scriptLabImproveRun")};
  order.first()->setFocus();
  QTRY_COMPARE(QApplication::focusWidget(), order.first());
  for (int i = 1; i < order.size(); ++i)
  {
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
    QCOMPARE(QApplication::focusWidget(), order.at(i));
  }
  for (QWidget* w : order) QVERIFY2(!w->accessibleName().isEmpty(), qPrintable(w->objectName()));
}

void TestLabImproveView::noClipping()
{
  QTemporaryDir dir;
  const QString top = makeSetup(dir);
  for (const UiRender::Variant& v : UiRender::variants())
  {
    if (v.percent == 150) continue;
    UiRender::Scope scope(v);
    RlvState state;
    LabWatcher watcher;
    ScriptLabPanel panel;
    panel.setState(&state);
    panel.setWatcher(&watcher);
    QVERIFY(panel.watchFolder(top));
    panel.showPage(ScriptLabPanel::ImprovePage);
    const QImage image = UiRender::render(&panel);
    UiRender::save(image, QStringLiteral("scriptlab_improve"), v);
    const QStringList clipped = UiRender::clipping(panel.improveView());
    QVERIFY2(clipped.isEmpty(), qPrintable(v.name() + ": " + clipped.join('\n')));
    // the colours are the theme's tokens: the title in text, the steps muted
    QLabel* steps = panel.improveView()->findChild<QLabel*>("scriptLabImproveSteps");
    QCOMPARE(steps->palette().color(QPalette::WindowText), Theme::color(Theme::Token::TextMuted, v.dark));
  }
}

QTEST_MAIN(TestLabImproveView)
#include "tst_labimproveview.moc"
