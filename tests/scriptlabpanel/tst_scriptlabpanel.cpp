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
 * Set PINGLYMOTION_SCRIPTLAB_SHOT to a PNG path to save a picture of the
 * panel after "@detach=n,sendim=n" (for the session's own look at it).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDockWidget>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>

#include "controlapi.h"
#include "panellayout.h"
#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"

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
    void colourTokens();
    void floatingPlaceRoundTrip();
    void dockedAndHiddenRoundTrip();
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
  const QString window = readFile(PINGLYMOTION_SOURCE_DIR "/pinglymotionwindow.cpp");
  QVERIFY(!window.isEmpty());
  const int timeline = window.indexOf("PanelVisibility::linkMenuItem(this,m_timelineDock,ui->optionsShowTimelineAction);");
  const int entry = window.indexOf("ui->menuView->addAction(m_scriptLabDock->toggleViewAction());");
  const int hide = window.indexOf("ui->menuView->addAction(m_hidePanelsAction);");
  QVERIFY(timeline > 0);
  QVERIFY(entry > timeline);
  QVERIFY(hide > entry);
  QVERIFY(window.contains("m_scriptLabPanel->setState(&m_controlApi->rlv());"));
  QVERIFY(window.contains("return {m_jointsDock,m_jointCardDock,m_timelineDock,m_scriptLabDock};"));
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

QTEST_MAIN(TestScriptLabPanel)
#include "tst_scriptlabpanel.moc"
