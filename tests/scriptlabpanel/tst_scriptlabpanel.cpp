/*
 * PinglyMotion - tests of the Script Lab panel (issue 288)
 *
 * Exact answers only: the View menu entry and its text, the restrictions and
 * the Allowed / Blocked actions after a line typed in the panel and Enter,
 * @clear, the Reset button, replies on a channel, a line sent through the
 * control command rlv_command showing up in the panel, the colour tokens from
 * docs/design_tokens.json, and the panel's place, size and visibility after
 * saving and loading the layout.
 *
 * Live page (issue 292): the empty state, the folder shown after watching,
 * a saved script with an error (status line, error list with line and
 * column, the line mark), the fixed script (status, event log, script text),
 * a control command lab_watch showing in the panel, and Stop watching.
 *
 * Set PINGLYMOTION_SCRIPTLAB_SHOT to a PNG path to save a picture of the
 * panel after "@detach=n,sendim=n" (for the session's own look at it), and
 * PINGLYMOTION_SCRIPTLAB_LIVE_SHOT for pictures of the Live page (an error,
 * then a run; "-error" and "-ok" are added before ".png").
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>

#include "../laterfile.h"
#include "controlapi.h"
#include "panellayout.h"
#include "scriptlab/labeditor.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/scriptlabsection.h"
#include "../windowsource.h"

namespace
{
  // the control commands need a host; the RLV commands never touch it
  class NoAnimationHost : public ControlHost
  {
    public:
      Animation* animation() override { return nullptr; }
      void goToFrame(int) override {}
      void refresh() override {}
      void selectJoint(const QString&) override {}
      QImage screenshot(int, int) override { return QImage(); }
      bool newAnimation(QString&) override { return false; }
      bool openFile(const QString&, QString&) override { return false; }
      bool openFileData(const QString&, const QByteArray&, QString&) override { return false; }
      bool saveFile(const QString&, QString&) override { return false; }
      void setPlaying(bool) override {}
      bool playing() override { return false; }
      QString fileName() override { return QString(); }
      bool unsavedChanges() override { return false; }
      bool male() override { return false; }
      void setMale(bool) override {}
      void setCamera(float, float, float) override {}
      void resetCamera() override {}
      QVector3D camera() override { return QVector3D(); }
  };

  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  // action id -> the State text shown in the panel
  QMap<QString, QString> shownStates(const ScriptLabPanel& panel)
  {
    QMap<QString, QString> states;
    QTreeWidget* list = panel.actionList();
    for (int i = 0; i < list->topLevelItemCount(); ++i)
      states[list->topLevelItem(i)->data(0, Qt::UserRole).toString()] = list->topLevelItem(i)->text(1);
    return states;
  }

  QStringList restrictionRows(const ScriptLabPanel& panel)
  {
    QStringList rows;
    QTreeWidget* list = panel.restrictionList();
    for (int i = 0; i < list->topLevelItemCount(); ++i)
    {
      const QTreeWidgetItem* item = list->topLevelItem(i);
      rows << item->text(0) + "|" + item->text(1) + "|" + item->text(2);
    }
    return rows;
  }

  void typeAndEnter(ScriptLabPanel& panel, const QString& text)
  {
    panel.commandField()->clear();
    QTest::keyClicks(panel.commandField(), text);
    QTest::keyClick(panel.commandField(), Qt::Key_Return);
  }

  // a main window with a central widget and the Script Lab dock, as the program builds it
  struct LabWindow
  {
    QMainWindow window;
    ScriptLabPanel* panel;
    QDockWidget* dock;

    LabWindow()
    {
      window.setCentralWidget(new QLabel("3D view"));
      panel = new ScriptLabPanel;
      dock = ScriptLabPanel::makeDock(&window, panel);
      window.resize(1200, 800);
    }
    QByteArray save() const { return window.saveState(PanelLayout::CurrentVersion); }
    bool load(const QByteArray& state) { return window.restoreState(state, PanelLayout::CurrentVersion); }
  };
}

class TestScriptLabPanel : public QObject
{
  Q_OBJECT

  private slots:
    void menuEntry();
    void detachAndSendIm();
    void clearEmptiesTheList();
    void resetButton();
    void replyAndRefusedLine();
    void controlCommandUpdatesPanel();
    void chooseRestriction();
    void useInScript();
    void colourTokens();
    void floatingPlaceRoundTrip();
    void dockedAndHiddenRoundTrip();
    void liveTabAndEmptyState();
    void liveShowsErrorsAndRuns();
    void liveFollowsControlCommands();
};

// View > Script Lab: the dock's own action, with exactly this text, hidden at first
void TestScriptLabPanel::menuEntry()
{
  LabWindow w;
  QAction* action = w.dock->toggleViewAction();
  QCOMPARE(action->text(), QString("Script Lab"));
  QCOMPARE(action->objectName(), QString("viewScriptLabAction"));
  QCOMPARE(w.dock->objectName(), QString("scriptLabDock"));
  QCOMPARE(w.dock->windowTitle(), QString("Script Lab"));
  QVERIFY(action->isCheckable());
  QVERIFY(!action->toolTip().isEmpty());
  QVERIFY(w.dock->isHidden());
  QCOMPARE(w.window.dockWidgetArea(w.dock), Qt::RightDockWidgetArea);
  QVERIFY(w.dock->features() & QDockWidget::DockWidgetFloatable);
  QVERIFY(w.dock->features() & QDockWidget::DockWidgetClosable);
  QVERIFY(w.dock->features() & QDockWidget::DockWidgetMovable);

  // the window puts that action in the View menu, after Timeline and before
  // Hide panels, and gives the panel the control API's RLV state
  const QString window = WindowSource::read(PINGLYMOTION_SOURCE_DIR "/pinglymotionwindow.cpp");
  QVERIFY(!window.isEmpty());
  const int timeline = window.indexOf("PanelVisibility::linkMenuItem(this,m_timelineDock,ui->optionsShowTimelineAction);");
  const int entry = window.indexOf("ui->menuView->addAction(m_scriptLabDock->toggleViewAction());");
  const int hide = window.indexOf("ui->menuView->addAction(m_hidePanelsAction);");
  QVERIFY(timeline > 0);
  QVERIFY(entry > timeline);
  QVERIFY(hide > entry);
  // the Script Lab controller (job R5) makes the panel and connects it
  QVERIFY(window.contains("m_scriptLab=new ScriptLabController(this);"));
  QVERIFY(window.contains("m_scriptLab->connectControl(m_controlApi.data());"));
  const QString controller = readFile(PINGLYMOTION_SOURCE_DIR "/scriptlabcontroller.cpp");
  QVERIFY(controller.contains("m_scriptLabPanel->setState(&m_controlApi->rlv());"));
  QVERIFY(window.contains("return {m_jointsDock,m_jointCardDock,m_timelineDock,m_scriptLabDock,m_libraryDock};"));
  const QString ui = readFile(PINGLYMOTION_SOURCE_DIR "/pinglymotionwindow.ui");
  QVERIFY(ui.contains("<widget class=\"QMenu\" name=\"menuView\">\n    <property name=\"title\">\n     <string>View</string>"));
}

void TestScriptLabPanel::detachAndSendIm()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 0);
  QVERIFY(!panel.emptyHint()->isHidden());

  typeAndEnter(panel, "@detach=n,sendim=n");

  QCOMPARE(restrictionRows(panel), QStringList() << "detach||panel" << "sendim||panel");
  QVERIFY(panel.emptyHint()->isHidden());
  QCOMPARE(panel.commandField()->text(), QString());   // a line that ran is cleared

  const QMap<QString, QString> states = shownStates(panel);
  QCOMPARE(states.size(), static_cast<int>(Rlv::actions().size()));
  for (auto it = states.constBegin(); it != states.constEnd(); ++it)
  {
    const bool blocked = it.key() == "detach" || it.key() == "sendim";
    QCOMPARE(it.value(), blocked ? QString("Blocked") : QString("Allowed"));
  }
  // the labels the user reads for the two blocked actions
  QTreeWidget* list = panel.actionList();
  QCOMPARE(list->topLevelItem(0)->text(0), QString("Take off locked attachments"));
  QCOMPARE(list->topLevelItem(1)->text(0), QString("Send instant messages"));
  // colour backs the word, never replaces it (charter P10)
  const bool dark = ScriptLabPanel::isDark(panel.palette());
  QCOMPARE(list->topLevelItem(0)->foreground(1).color(), ScriptLabPanel::dangerColour(dark));
  QCOMPARE(list->topLevelItem(2)->foreground(1).color(), ScriptLabPanel::successColour(dark));

  const QString shot = qEnvironmentVariable("PINGLYMOTION_SCRIPTLAB_SHOT");
  if (!shot.isEmpty())
  {
    typeAndEnter(panel, "@versionnew=2222");
    panel.resize(420, 760);
    panel.show();
    QVERIFY(panel.grab().save(shot));
  }
}

void TestScriptLabPanel::clearEmptiesTheList()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  typeAndEnter(panel, "@detach=n,sendim=n");
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 2);

  typeAndEnter(panel, "@clear");
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 0);
  QVERIFY(!panel.emptyHint()->isHidden());
  for (const QString& shown : shownStates(panel)) QCOMPARE(shown, QString("Allowed"));
}

void TestScriptLabPanel::resetButton()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.objectField()->setText("collar");
  typeAndEnter(panel, "@fly=n");
  QCOMPARE(restrictionRows(panel), QStringList() << "fly||collar");
  QCOMPARE(shownStates(panel).value("fly"), QString("Blocked"));

  QTest::mouseClick(panel.resetButton(), Qt::LeftButton);
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 0);
  QCOMPARE(state.restrictions().size(), 0);
  QCOMPARE(shownStates(panel).value("fly"), QString("Allowed"));
  QVERIFY(panel.log()->toPlainText().endsWith("Reset: 1 restriction(s) removed."));
}

void TestScriptLabPanel::replyAndRefusedLine()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  typeAndEnter(panel, "@versionnum=2222");
  QCOMPARE(panel.log()->toPlainText(), QString("panel: @versionnum=2222\n"
                                               "  versionnum=2222: success\n"
                                               "  Reply on channel 2222: 3040300"));
  panel.log()->clear();

  // no "@": refused, the line stays in the field so it can be fixed
  typeAndEnter(panel, "detach=n");
  QVERIFY(panel.log()->toPlainText().contains("  Refused: "));
  QCOMPARE(panel.commandField()->text(), QString("detach=n"));
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 0);
}

// rlv_command from an AI client changes the same state, so the panel follows
void TestScriptLabPanel::controlCommandUpdatesPanel()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());

  QJsonObject command;
  command["command"] = "rlv_command";
  command["object"] = "collar";
  command["text"] = "@detach=n,sendim=n";
  QVERIFY(api.execute(command).value("ok").toBool());
  QCOMPARE(restrictionRows(panel), QStringList() << "detach||collar" << "sendim||collar");
  QCOMPARE(shownStates(panel).value("detach"), QString("Blocked"));
  QCOMPARE(shownStates(panel).value("sendim"), QString("Blocked"));
  QCOMPARE(shownStates(panel).value("fly"), QString("Allowed"));

  // a line typed in the panel is seen by rlv_state
  typeAndEnter(panel, "@fly=n");
  QJsonObject ask;
  ask["command"] = "rlv_state";
  const QJsonObject answer = api.execute(ask);
  QVERIFY(answer.value("ok").toBool());
  const QJsonObject objects = answer.value("objects").toObject();
  QCOMPARE(objects.value("panel").toArray(), QJsonArray({"fly"}));
  QCOMPARE(objects.value("collar").toArray(), QJsonArray({"detach", "sendim"}));

  QJsonObject reset;
  reset["command"] = "rlv_reset";
  QVERIFY(api.execute(reset).value("ok").toBool());
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 0);
  QCOMPARE(shownStates(panel).value("detach"), QString("Allowed"));
}

// the colours are the tokens of docs/design_tokens.json, in both themes
void TestScriptLabPanel::colourTokens()
{
  const QJsonObject tokens = QJsonDocument::fromJson(readFile(PINGLYMOTION_DOCS_DIR "/design_tokens.json").toUtf8()).object();
  const QJsonObject themes = tokens.value("themes").toObject();
  QVERIFY(!themes.isEmpty());
  for (const bool dark : {true, false})
  {
    const QJsonObject theme = themes.value(dark ? "dark" : "light").toObject();
    QCOMPARE(ScriptLabPanel::successColour(dark), QColor(theme.value("success").toString()));
    QCOMPARE(ScriptLabPanel::dangerColour(dark), QColor(theme.value("danger").toString()));
    QCOMPARE(ScriptLabPanel::mutedColour(dark), QColor(theme.value("text_muted").toString()));
  }

  // the buttons: Run is the main button (accent), Reset a danger button
  ScriptLabPanel panel;
  const bool dark = ScriptLabPanel::isDark(panel.palette());
  const QJsonObject theme = themes.value(dark ? "dark" : "light").toObject();
  QVERIFY(panel.runButton()->styleSheet().contains("background: " + theme.value("accent").toString() + ";"));
  QVERIFY(panel.resetButton()->styleSheet().contains("background: " + theme.value("danger_fill").toString() + ";"));
  QVERIFY(panel.runButton()->styleSheet().contains("color: " + theme.value("on_accent").toString() + ";"));
  QVERIFY(panel.runButton()->styleSheet().contains("border: 2px solid " + theme.value("focus_ring").toString() + ";"));
  QVERIFY(panel.runButton()->styleSheet().contains(QString("border-radius: %1px;").arg(tokens.value("radius").toObject().value("control").toInt())));
  QCOMPARE(panel.runButton()->minimumHeight(), tokens.value("control_height").toObject().value("default").toInt());

  // the Live page (issue 292): since job U3 Make example project is the main button, Watch folder and the
  // others are normal buttons, greyed with text_muted (higher contrast)
  QVERIFY(panel.makeExampleButton()->styleSheet().contains("background: " + theme.value("accent").toString() + ";"));
  for (QPushButton* normal : {panel.watchButton(), panel.unwatchButton(), panel.openEditorButton()})
  {
    QVERIFY(normal->styleSheet().contains("background: " + theme.value("surface_raised").toString() + ";"));
    QVERIFY(normal->styleSheet().contains("color: " + theme.value("text").toString() + ";"));
    QVERIFY(normal->styleSheet().contains("border: 1px solid " + theme.value("border").toString() + ";"));
    QVERIFY(normal->styleSheet().contains(":disabled { color: " + theme.value("text_muted").toString() + ";"));
    QVERIFY(normal->styleSheet().contains("border: 2px solid " + theme.value("focus_ring").toString() + ";"));
    if (normal != panel.watchButton())   // the empty state's buttons are large (control_height.large)
      QCOMPARE(normal->minimumHeight(), tokens.value("control_height").toObject().value("default").toInt());
  }
}

// a floating Script Lab keeps its place, size and visibility after save and load
void TestScriptLabPanel::floatingPlaceRoundTrip()
{
  const QRect place(200, 150, 420, 600);
  QByteArray state;
  {
    LabWindow before;
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    before.dock->show();
    before.dock->setFloating(true);
    before.dock->setGeometry(place);
    QTRY_COMPARE(before.dock->geometry(), place);
    state = before.save();
  }
  LabWindow next;
  next.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next.window));
  QVERIFY(next.dock->isHidden());
  QVERIFY(next.load(state));
  QVERIFY(next.dock->isFloating());
  QVERIFY(!next.dock->isHidden());
  QTRY_COMPARE(next.dock->geometry(), place);
}

void TestScriptLabPanel::dockedAndHiddenRoundTrip()
{
  QByteArray state;
  int width = 0;
  {
    LabWindow before;
    before.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&before.window));
    before.window.addDockWidget(Qt::LeftDockWidgetArea, before.dock);
    before.dock->show();
    before.window.resizeDocks({before.dock}, {360}, Qt::Horizontal);
    QTRY_COMPARE(before.dock->width(), 360);
    width = before.dock->width();
    state = before.save();
  }
  {
    LabWindow next;
    next.window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&next.window));
    QVERIFY(next.load(state));
    QCOMPARE(next.window.dockWidgetArea(next.dock), Qt::LeftDockWidgetArea);
    QVERIFY(!next.dock->isFloating());
    QVERIFY(!next.dock->isHidden());
    QTRY_COMPARE(next.dock->width(), width);
    // closed again: stays closed after the next load
    next.dock->toggleViewAction()->trigger();
    QVERIFY(next.dock->isHidden());
    state = next.save();
  }
  LabWindow last;
  last.window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&last.window));
  QVERIFY(last.load(state));
  QVERIFY(last.dock->isHidden());
  QVERIFY(!last.dock->toggleViewAction()->isChecked());
  QCOMPARE(last.window.dockWidgetArea(last.dock), Qt::LeftDockWidgetArea);
}

namespace
{
  bool writeScript(const QString& path, const QByteArray& text)
  {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  QStringList listTexts(const QListWidget* list)
  {
    QStringList out;
    for (int i = 0; i < list->count(); ++i) out << list->item(i)->text();
    return out;
  }

  void saveShot(ScriptLabPanel& panel, const QString& suffix)
  {
    QString shot = qEnvironmentVariable("PINGLYMOTION_SCRIPTLAB_LIVE_SHOT");
    if (shot.isEmpty()) return;
    shot.replace(".png", suffix + ".png");
    panel.resize(460, 860);
    panel.show();
    QVERIFY(panel.grab().save(shot));
  }
}

// the pages, Live scripts and Type commands first (job UX-B: then Who talks to whom and Player);
// the Live page says what to do when no folder is watched
void TestScriptLabPanel::liveTabAndEmptyState()
{
  ScriptLabPanel panel;
  LabWatcher watcher;
  panel.setWatcher(&watcher);
  QCOMPARE(panel.tabs()->count(), 5);
  QCOMPARE(panel.tabs()->tabText(0), QString("Live scripts"));
  QCOMPARE(panel.tabs()->tabText(1), QString("Type commands"));
  QCOMPARE(panel.tabs()->tabText(2), QString("Who talks to whom"));
  QCOMPARE(panel.tabs()->tabText(3), QString("Player"));
  QCOMPARE(panel.tabs()->tabText(4), QString("Improve a setup"));
  QCOMPARE(panel.tabs()->currentIndex(), 0);
  QCOMPARE(panel.watchButton()->text(), QString("Watch folder..."));
  QCOMPARE(panel.unwatchButton()->text(), QString("Stop watching"));
  QCOMPARE(panel.openEditorButton()->text(), QString("Open in editor"));
  // the empty state says what to do; the status line is hidden until there is something to say
  QVERIFY(!panel.liveEmpty()->isHidden());
  QVERIFY(panel.liveStatus()->isHidden());
  QCOMPARE(panel.liveFolderField()->text(), QString());
  QCOMPARE(panel.liveFolderField()->placeholderText(), QString("No folder watched"));
  QVERIFY(panel.liveFolderField()->isReadOnly());
  QVERIFY(!panel.scriptView()->isReadOnly());   // an editor since lane 1 job F1
  // greyed out with a reason (charter P6)
  QVERIFY(!panel.unwatchButton()->isEnabled());
  QCOMPARE(panel.unwatchButton()->toolTip(), QString("Not watching a folder. Choose one with Watch folder first."));
  QVERIFY(!panel.openEditorButton()->isEnabled());
  QCOMPARE(panel.openEditorButton()->toolTip(), QString("No script yet. Watch a folder and save a .lsl file in it first."));
  // a refused folder says why
  QVERIFY(!panel.watchFolder("relative"));
  QVERIFY(!panel.liveStatus()->isHidden());
  QCOMPARE(panel.liveStatusText(), QString("Error: Give the whole path of the folder, not a relative one."));
  QCOMPARE(panel.liveStatus()->palette().color(QPalette::WindowText), ScriptLabPanel::dangerColour(ScriptLabPanel::isDark(panel.palette())));
}

// a saved script with an error shows the error with line and column; the fixed script runs
void TestScriptLabPanel::liveShowsErrorsAndRuns()
{
  ScriptLabPanel panel;
  LabWatcher watcher;
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  QVERIFY(panel.watchFolder(dir.path()));
  QCOMPARE(panel.liveFolderField()->text(), QDir::toNativeSeparators(QFileInfo(dir.path()).canonicalFilePath()));
  QCOMPARE(panel.liveStatusText(), QString("Watching. No .lsl file in this folder yet: save one here and it runs at once."));
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));   // the Live page comes to the front
  QVERIFY(panel.unwatchButton()->isEnabled());

  QVERIFY(writeScript(dir.filePath("collar.lsl"), "default\n{\n    state_entry() { llOwnerSay(1); }\n}\n"));
  watcher.rescan();   // the change is looked at now, not when the system's notice comes
  QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 1, 10000);
  QCOMPARE(panel.liveStatusText(), QString("Error: collar.lsl has 1 error(s) and was not run (run 1). Fix the line shown below and save."));
  QCOMPARE(listTexts(panel.problemList()), QStringList{"Error, line 3, column 32: Argument 1 of llOwnerSay must be a string, not an integer."});
  const bool dark = ScriptLabPanel::isDark(panel.palette());
  QCOMPARE(panel.problemList()->item(0)->foreground().color(), ScriptLabPanel::dangerColour(dark));
  QCOMPARE(panel.liveScriptName()->title(), QString("Script: collar.lsl"));
  QCOMPARE(panel.scriptView()->toPlainText(), QString("default\n{\n    state_entry() { llOwnerSay(1); }\n}\n"));
  // the line is marked, and Enter on the error moves the cursor there
  QCOMPARE(panel.scriptView()->extraSelections().size(), 1);
  QCOMPARE(panel.scriptView()->extraSelections().first().cursor.blockNumber(), 2);
  QVERIFY(panel.openEditorButton()->isEnabled());
  QCOMPARE(panel.runLog()->toPlainText(), QString());
  panel.problemList()->setCurrentRow(0);
  emit panel.problemList()->itemActivated(panel.problemList()->item(0));
  QCOMPARE(panel.scriptView()->textCursor().blockNumber(), 2);
  QCOMPARE(panel.scriptView()->textCursor().positionInBlock(), 31);
  saveShot(panel, "-error");

  QVERIFY(LaterFile::write(dir.filePath("collar.lsl"), "default\n{\n    state_entry() { llOwnerSay(\"locked\"); }\n}\n"));
  watcher.rescan();   // the change is looked at now, not when the system's notice comes
  QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 2, 10000);
  QCOMPARE(panel.problemList()->count(), 0);
  QCOMPARE(panel.scriptView()->extraSelections().size(), 0);
  QCOMPARE(panel.runLog()->toPlainText(), QString("0.000 s  state: default\n0.000 s  event: state_entry()\n0.000 s  owner_say: locked"));
  QCOMPARE(panel.liveStatusText(), QString("OK: collar.lsl ran without errors (run 2), 3 log line(s)."));
  QCOMPARE(panel.liveStatus()->palette().color(QPalette::WindowText), ScriptLabPanel::successColour(dark));
  saveShot(panel, "-ok");

  panel.stopWatching();
  QCOMPARE(panel.liveFolderField()->text(), QString());
  QCOMPARE(panel.scriptView()->toPlainText(), QString());
  QCOMPARE(panel.runLog()->toPlainText(), QString());
  QVERIFY(!panel.unwatchButton()->isEnabled());
  QVERIFY(panel.liveStatus()->isHidden());
  QVERIFY(!panel.liveEmpty()->isHidden());
}

// lab_watch and lab_unwatch from an AI client show up in the panel at once
void TestScriptLabPanel::liveFollowsControlCommands()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setWatcher(&api.lab());
  QTemporaryDir dir;
  QVERIFY(writeScript(dir.filePath("hello.lsl"), "default { state_entry() { llOwnerSay(\"hi\"); } }"));
  const QJsonObject answer = api.execute(QJsonObject{{"command", "lab_watch"}, {"path", dir.path()}});
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(panel.liveFolderField()->text(), QDir::toNativeSeparators(QFileInfo(dir.path()).canonicalFilePath()));
  QCOMPARE(panel.liveScriptName()->title(), QString("Script: hello.lsl"));
  QCOMPARE(panel.runLog()->toPlainText(), QString("0.000 s  state: default\n0.000 s  event: state_entry()\n0.000 s  owner_say: hi"));
  // a script with RLV lines: the restrictions show in the status line and on both pages
  panel.setState(&api.rlv());
  QVERIFY(LaterFile::write(dir.filePath("hello.lsl"), "default { state_entry() { llOwnerSay(\"@detach=n,sendim=n\"); } }"));
  api.lab().rescan();   // the change is looked at now, not when the system's notice comes
  QTRY_COMPARE_WITH_TIMEOUT(api.lab().runCount(), 2, 10000);
  QCOMPARE(panel.liveStatusText(),
           QString("OK: hello.lsl ran without errors (run 2), 3 log line(s). RLV restrictions at the end: detach, sendim."));
  QCOMPARE(restrictionRows(panel).size(), 2);
  QCOMPARE(shownStates(panel).value("detach"), QString("Blocked"));
  QVERIFY(panel.runLog()->toPlainText().endsWith("rlv_command: @detach=n,sendim=n"));
  QVERIFY(api.execute(QJsonObject{{"command", "lab_unwatch"}})["ok"].toBool());
  QCOMPARE(panel.liveFolderField()->text(), QString());
}

// job A2: every row of "What the avatar may do" has an Allowed / Blocked choice
void TestScriptLabPanel::chooseRestriction()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  QTreeWidget* list = panel.actionList();
  // every row has the choice, on Allowed at start
  for (int i = 0; i < list->topLevelItemCount(); ++i)
  {
    QComboBox* combo = qobject_cast<QComboBox*>(list->itemWidget(list->topLevelItem(i), 1));
    QVERIFY(combo);
    QCOMPARE(combo->count(), 2);
    QCOMPARE(combo->itemText(0), QString("Allowed"));
    QCOMPARE(combo->itemText(1), QString("Blocked"));
    QCOMPARE(combo->currentIndex(), 0);
  }
  QVERIFY(panel.choiceBox()->isHidden());
  QVERIFY(!panel.actionChoice("nothing"));

  // Blocked on "Send instant messages", chosen as a user does: the line runs, the row turns red
  QComboBox* sendim = panel.actionChoice("sendim");
  sendim->setFocus();
  QTest::keyClick(sendim, Qt::Key_Down);   // the next item is chosen and "activated"
  QCOMPARE(sendim->currentIndex(), 1);
  QCOMPARE(state.restrictions().size(), 1);
  QCOMPARE(state.restrictions()[0].object, QString("panel"));
  QCOMPARE(state.restrictions()[0].behaviour, QString("sendim"));
  QCOMPARE(state.actions().value("sendim"), false);
  QCOMPARE(shownStates(panel).value("sendim"), QString("Blocked"));
  QTreeWidgetItem* row = nullptr;
  for (int i = 0; i < list->topLevelItemCount(); ++i)
    if (list->topLevelItem(i)->data(0, Qt::UserRole).toString() == "sendim") row = list->topLevelItem(i);
  QVERIFY(row);
  const bool dark = ScriptLabPanel::isDark(panel.palette());
  QCOMPARE(row->foreground(0).color(), ScriptLabPanel::dangerColour(dark));
  QCOMPARE(row->foreground(1).color(), ScriptLabPanel::dangerColour(dark));
  QCOMPARE(sendim->palette().color(QPalette::ButtonText), ScriptLabPanel::dangerColour(dark));
  QCOMPARE(sendim->font().weight(), QFont::DemiBold);
  // the box shows the line and says it in plain words
  QVERIFY(!panel.choiceBox()->isHidden());
  QCOMPARE(panel.choiceTitle()->text(), QString("Send instant messages: Blocked"));
  QCOMPARE(panel.choiceLine()->text(), QString("@sendim=n"));
  QCOMPARE(panel.choiceExplain()->text(),
           QString("This line blocks: Send instant messages.\n") + Rlv::findHelp("sendim")->sentence +
           "\n@ starts an RLV line, and sendim is the command.\n=n means no: the avatar may not do this.");
  QCOMPARE(panel.choiceResult()->text(), QString("Ran from panel. Blocked now."));
  QVERIFY(panel.choiceValueLabel()->isHidden() || !panel.choiceValue()->isVisibleTo(panel.choiceBox()));
  QVERIFY(panel.log()->toPlainText().contains("panel: @sendim=n"));
  // no script open: Use in script is greyed with the reason
  QVERIFY(!panel.useInScriptButton()->isEnabled());
  QCOMPARE(panel.useInScriptButton()->toolTip(), QString("Open a script on the Live scripts page first."));
  QCOMPARE(panel.useInScriptReason()->text(), QString("Open a script on the Live scripts page first."));
  QVERIFY(!panel.useChoiceInScript());
  QCOMPARE(panel.choiceResult()->text(), QString("Open a script on the Live scripts page first."));

  // Allowed lifts it again
  QVERIFY(panel.chooseAction("sendim", false));
  QVERIFY(state.restrictions().isEmpty());
  QCOMPARE(shownStates(panel).value("sendim"), QString("Allowed"));
  QCOMPARE(sendim->currentIndex(), 0);
  QCOMPARE(panel.choiceTitle()->text(), QString("Send instant messages: Allowed"));
  QCOMPARE(panel.choiceLine()->text(), QString("@sendim=y"));
  QCOMPARE(panel.choiceExplain()->text(), QString("This line lifts the block again.\n=y means yes: the avatar may do this again."));
  QCOMPARE(panel.choiceResult()->text(), QString("Ran from panel. Allowed now."));

  // a restriction another object holds: Allowed cannot lift it, and says who holds it
  QVERIFY(state.apply("collar", "@detach=n").ok());
  QCOMPARE(panel.actionChoice("detach")->currentIndex(), 1);
  QVERIFY(!panel.chooseAction("detach", false));
  QCOMPARE(panel.actionChoice("detach")->currentIndex(), 1);
  QCOMPARE(panel.choiceResult()->text(), QString("Still blocked, because collar holds it too."));
  QCOMPARE(panel.choiceResult()->palette().color(QPalette::WindowText), ScriptLabPanel::dangerColour(dark));

  // a value: Blocked on "Touch objects farther away" shows the distance field
  QVERIFY(panel.chooseAction("touchfar", true));
  QCOMPARE(panel.choiceLine()->text(), QString("@touchfar=n"));
  QVERIFY(panel.choiceValue()->isVisibleTo(panel.choiceBox()));
  QCOMPARE(panel.choiceValueLabel()->text(), QString("Farther than (metres)"));
  panel.choiceValue()->setText("3");
  QTest::keyClick(panel.choiceValue(), Qt::Key_Return);
  QCOMPARE(panel.choiceLine()->text(), QString("@touchfar:3=n"));
  QStringList held;
  for (const RlvState::Restriction& r : state.restrictions()) held << r.object + ":" + r.behaviour + ":" + r.option;
  QCOMPARE(held, QStringList({"collar:detach:", "panel:touchfar:3"}));   // the old limit was lifted first
  QCOMPARE(shownStates(panel).value("touchfar"), QString("Blocked"));      // blocked farther than 3 m
  QVERIFY(panel.choiceExplain()->text().contains(":3 is the distance in metres."));
  panel.choiceValue()->setText("far");
  QVERIFY(!panel.applyChoiceValue());
  QCOMPARE(panel.choiceResult()->text(), QString("Type a distance in metres, such as 1.5."));
  QCOMPARE(state.restrictions().size(), 2);   // nothing changed
  QVERIFY(panel.chooseAction("touchfar", false));
  QCOMPARE(panel.choiceLine()->text(), QString("@touchfar:3=y"));
  QCOMPARE(shownStates(panel).value("touchfar"), QString("Allowed"));

  // a channel that stays open
  QVERIFY(panel.chooseAction("sendchannel", true));
  QCOMPARE(panel.choiceValueLabel()->text(), QString("Still allow channel"));
  panel.choiceValue()->setText("5");
  QVERIFY(panel.applyChoiceValue());
  QCOMPARE(panel.choiceLine()->text(), QString("@sendchannel=n,sendchannel:5=add"));
  if (!qEnvironmentVariableIsEmpty("PINGLYMOTION_UI_OUT"))   // a picture for a session to look at
  {
    panel.resize(520, 1400);
    panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
    panel.grab().save(qEnvironmentVariable("PINGLYMOTION_UI_OUT") + "/choice.png");
  }
  QCOMPARE(state.actions().value("sendchannel"), false);

  // plain RLV mode: an RLVa-only row cannot be chosen, and says why
  panel.setMode(Rlv::Mode::Rlv);
  QVERIFY(!panel.actionChoice("jump")->isEnabled());
  QCOMPARE(panel.actionChoice("jump")->toolTip(), QString("Only RLVa has this. Switch Viewer rules to RLVa."));
  QVERIFY(!panel.chooseAction("jump", true));
  QVERIFY(panel.actionChoice("sendim")->isEnabled());

  // keys: the list is one Tab stop; Space or Enter on a row switches it
  QCOMPARE(int(panel.actionChoice("sendim")->focusPolicy()), int(Qt::NoFocus));
  list->setCurrentItem(row);
  QCOMPARE(shownStates(panel).value("sendim"), QString("Allowed"));
  QTest::keyClick(list, Qt::Key_Space);
  QCOMPARE(shownStates(panel).value("sendim"), QString("Blocked"));
  QCOMPARE(panel.choiceLine()->text(), QString("@sendim=n"));
  QTest::keyClick(list, Qt::Key_Return);
  QCOMPARE(shownStates(panel).value("sendim"), QString("Allowed"));
  QCOMPARE(panel.choiceLine()->text(), QString("@sendim=y"));

  // the Command field and Try: still work as before
  typeAndEnter(panel, "@fly=n");
  QCOMPARE(shownStates(panel).value("fly"), QString("Blocked"));
  QCOMPARE(panel.actionChoice("fly")->currentIndex(), 1);
}

// job A2: "Use in script" puts the line into the script of the Live page with llOwnerSay
void TestScriptLabPanel::useInScript()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QFile file(dir.filePath("lock.lsl"));
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("default\n{\n    state_entry()\n    {\n        llOwnerSay(\"hi\");\n    }\n}\n");
  file.close();
  QVERIFY(panel.watchFolder(dir.path()));
  QCOMPARE(panel.shownScript(), QString("lock.lsl"));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  QVERIFY(panel.chooseAction("sendim", true));
  QVERIFY(panel.useInScriptButton()->isEnabled());
  QVERIFY(panel.useInScriptReason()->isHidden());
  QCOMPARE(panel.useInScriptButton()->toolTip(), QString("Put llOwnerSay(\"@sendim=n\"); into the script, so the script sends this line."));
  QTest::mouseClick(panel.useInScriptButton(), Qt::LeftButton);
  QCOMPARE(panel.editor()->codeEdit()->toPlainText(),
           QString("default\n{\n    state_entry()\n    {\n        llOwnerSay(\"@sendim=n\");\n        llOwnerSay(\"hi\");\n    }\n}\n"));
  QCOMPARE(panel.choiceResult()->text(), QString("Put in the script at line 5. Save to run it."));
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));
  QCOMPARE(panel.editor()->codeEdit()->textCursor().blockNumber(), 4);
  // one undo step takes it out again; nothing is saved by itself
  panel.editor()->codeEdit()->undo();
  QCOMPARE(panel.editor()->codeEdit()->toPlainText(), QString("default\n{\n    state_entry()\n    {\n        llOwnerSay(\"hi\");\n    }\n}\n"));
  QFile again(dir.filePath("lock.lsl"));
  QVERIFY(again.open(QIODevice::ReadOnly));
  QVERIFY(!QString::fromUtf8(again.readAll()).contains("sendim"));
}

QTEST_MAIN(TestScriptLabPanel)
#include "tst_scriptlabpanel.moc"
