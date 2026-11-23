/*
 * PinglyMotion - tests of the Script Lab input buttons (issue 308)
 *
 * Exact answers only: the disabled reasons before a run and after every script
 * stopped, the buttons after an lsl_run with two objects, the exact event log
 * lines a touch, a chat line and a dialog answer make, the same lines as
 * lsl_event gives, refused input, and keyboard use.
 *
 * Set PINGLYMOTION_SCRIPTLAB_INPUT_SHOT to a PNG path to save a picture of the
 * Live page after a dialog answer (for the session's own look at it).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTreeWidget>
#include <QTabWidget>

#include "controlapi.h"
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


  const char* const Hud =
    "default { state_entry() { llListen(-7001, \"\", llGetOwner(), \"\"); }"
    " touch_start(integer n) { llDialog(llDetectedKey(0), \"Lock the cuffs?\", [\"Lock\", \"Unlock\", \"A&B\"], -7001); }"
    " listen(integer c, string n, key id, string m) { llRegionSay(-7002, llToLower(m)); } }";
  const char* const Cuffs =
    "default { state_entry() { llListen(-7002, \"\", NULL_KEY, \"\"); }"
    " listen(integer c, string n, key id, string m) { if (m == \"lock\") llOwnerSay(\"@detach=n\"); if (m == \"crash\") { integer z = 0; z = 1 / z; } } }";

  QJsonObject twoObjects()
  {
    return QJsonObject{{"command", "lsl_run"}, {"seconds", 1},
                       {"objects", QJsonArray{QJsonObject{{"name", "hud"}, {"source", Hud}, {"attached", 35}},
                                              QJsonObject{{"name", "cuffs"}, {"source", Cuffs}}}}};
  }

  QString ownerKey(ControlApi& api)
  {
    return api.lslSession().sandbox()->objectsJson().at(0).toObject().value("owner").toString();
  }

  QStringList lines(const QPlainTextEdit* edit)
  {
    return edit->toPlainText().split(QLatin1Char('\n'));
  }
}

class TestScriptLabInput : public QObject
{
  Q_OBJECT

  private slots:
    void disabledBeforeARun();
    void touchAndDialog();
    void chatLine();
    void sameLinesAsLslEvent();
    void refusedInput();
    void stoppedWorld();
    void keyboard();
    void darkTheme();
    void picture();
};

void TestScriptLabInput::disabledBeforeARun()
{
  ScriptLabPanel panel;
  NoAnimationHost host;
  ControlApi api(&host);
  panel.setSession(&api.lslSession());
  QVERIFY(!panel.touchButton()->isEnabled());
  QVERIFY(!panel.sayButton()->isEnabled());
  QVERIFY(!panel.inputObject()->isEnabled());
  const QString reason = "Nothing to send to yet: no script has run. An AI assistant starts one with lsl_run; the buttons work then.";
  QCOMPARE(panel.inputHint()->text(), reason);
  QCOMPARE(panel.touchButton()->toolTip(), reason);
  QCOMPARE(panel.sayButton()->toolTip(), reason);
  QCOMPARE(panel.inputDialog()->text(), QString("No dialog yet. When a script shows one, its buttons appear here."));
  QVERIFY(panel.dialogButtons().isEmpty());
  QVERIFY(panel.inputResult()->isHidden());
  // sending anyway is refused in words
  QVERIFY(!panel.sendTouch(QString()));
  QCOMPARE(panel.inputResult()->text(), QString("Error: Touch was not sent. There is no running world to send the event to. Start one with lsl_run first (with \"source\" or \"objects\")."));
  QCOMPARE(panel.inputLog()->toPlainText(), QString("> Touch\nError: There is no running world to send the event to. Start one with lsl_run first (with \"source\" or \"objects\")."));
}

void TestScriptLabInput::touchAndDialog()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(twoObjects())["ok"].toBool());
  const QString owner = ownerKey(api);

  QCOMPARE(panel.inputObject()->count(), 2);
  QCOMPARE(panel.inputObject()->itemText(0), QString("hud"));
  QCOMPARE(panel.inputObject()->itemText(1), QString("cuffs"));
  QVERIFY(panel.touchButton()->isEnabled());
  QCOMPARE(panel.inputHint()->text(), QString("Say is off: type the chat text first. Touch and the dialog buttons work."));
  QVERIFY(!panel.sayButton()->isEnabled());
  QCOMPARE(panel.sayButton()->toolTip(), QString("Type the chat text first"));
  QVERIFY(panel.dialogButtons().isEmpty());

  panel.touchButton()->click();
  QCOMPARE(panel.inputResult()->text(), QString("Sent: Touch on hud (3 log line(s))."));
  QCOMPARE(lines(panel.inputLog()), QStringList({
    "> Touch on hud",
    "1.000 s  input: input=touch, name=Owner, object=hud",
    "1.000 s  event: touch_start(1)",
    "1.000 s  dialog: buttons=Lock, Unlock, A&B, channel=-7001, message=Lock the cuffs?, object=hud, to=" + owner}));

  // the dialog the touch made: its text and one button for each button, & kept (inside Objects, as in the mockup)
  QCOMPARE(panel.inputDialog()->text(), QString("Dialog from hud"));
  QCOMPARE(panel.dialogMessage()->text(), QString("\"Lock the cuffs?\" on channel -7001"));
  QStringList buttons;
  for (QPushButton* b : panel.dialogButtons()) { buttons << b->property("button").toString(); QVERIFY(b->isEnabled()); }
  QCOMPARE(buttons, QStringList({"Lock", "Unlock", "A&B"}));
  QCOMPARE(panel.dialogButtons().at(2)->text(), QString("A&&B"));
  QCOMPARE(panel.dialogButtons().at(0)->toolTip(), QString("Press this button in the dialog, as the owner's avatar (channel -7001)"));

  panel.dialogButtons().at(0)->click();
  QCOMPARE(panel.inputResult()->text(), QString("Sent: Dialog answer \"Lock\" to hud (5 log line(s))."));
  const QStringList all = lines(panel.inputLog());
  QCOMPARE(all.mid(4), QStringList({
    "> Dialog answer \"Lock\" to hud",
    "2.000 s  input: channel=-7001, input=dialog, name=Owner, object=hud, text=Lock",
    "2.000 s  event: listen(-7001, Owner, " + owner + ", Lock)",
    "2.000 s  region_say on channel -7002: lock",
    "2.000 s  event: listen(-7002, hud, c0ffee00-0000-4000-8000-000000000002, lock)",
    "2.000 s  rlv_command: @detach=n"}));
  // the restriction the cuffs set is in the panel's restriction list: the world really ran
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 1);
  QCOMPARE(panel.restrictionList()->topLevelItem(0)->text(0), QString("detach"));
}

void TestScriptLabInput::chatLine()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(twoObjects())["ok"].toBool());
  const QString owner = ownerKey(api);

  panel.chatChannel()->setValue(-7001);
  panel.chatText()->setText("Hello");
  QVERIFY(panel.sayButton()->isEnabled());
  QCOMPARE(panel.inputHint()->text(), QString("Ready: input goes to the world of the last run, which then runs one more simulated second."));
  panel.sayButton()->click();
  QCOMPARE(panel.inputResult()->text(), QString("Sent: Chat on channel -7001 from Owner: Hello (4 log line(s))."));
  QCOMPARE(lines(panel.inputLog()), QStringList({
    "> Chat on channel -7001 from Owner: Hello",
    "1.000 s  input: channel=-7001, input=chat, name=Owner, text=Hello",
    "1.000 s  event: listen(-7001, Owner, " + owner + ", Hello)",
    "1.000 s  region_say on channel -7002: hello",
    "1.000 s  event: listen(-7002, hud, c0ffee00-0000-4000-8000-000000000002, hello)"}));

  // a chat name is given as the speaker; chat does not name an object (every object in range hears it)
  panel.chatName()->setText("Guest");
  panel.inputObject()->setCurrentIndex(1);
  panel.chatChannel()->setValue(-7002);
  panel.chatText()->setText("lock");
  panel.sayButton()->click();
  QCOMPARE(panel.inputResult()->text(), QString("Sent: Chat on channel -7002 from Guest: lock (3 log line(s))."));
  QCOMPARE(lines(panel.inputLog()).mid(5), QStringList({
    "> Chat on channel -7002 from Guest: lock",
    "2.000 s  input: channel=-7002, input=chat, name=Guest, text=lock",
    "2.000 s  event: listen(-7002, Guest, " + owner + ", lock)",
    "2.000 s  rlv_command: @detach=n"}));
  // the choice of object survives the refresh after the input
  QCOMPARE(panel.inputObject()->currentText(), QString("cuffs"));
}

void TestScriptLabInput::sameLinesAsLslEvent()
{
  // the panel and the control command use one code: the lines are the answer's log, read as the Live page reads it
  NoAnimationHost host;
  ControlApi api(&host);
  QVERIFY(api.execute(twoObjects())["ok"].toBool());
  const QJsonObject answer = api.execute(QJsonObject{{"command", "lsl_event"}, {"type", "touch"}, {"object", "hud"}});
  QStringList expected;
  for (const QJsonValue& v : answer["log"].toArray()) expected << ScriptLabPanel::logLine(v.toObject());
  QCOMPARE(expected.size(), 3);

  NoAnimationHost host2;
  ControlApi api2(&host2);
  ScriptLabPanel panel;
  panel.setSession(&api2.lslSession());
  QVERIFY(api2.execute(twoObjects())["ok"].toBool());
  QVERIFY(panel.sendTouch("hud"));
  QStringList got = lines(panel.inputLog());
  QCOMPARE(got.takeFirst(), QString("> Touch on hud"));
  // the owner's key differs between two worlds only if the sandbox makes keys at random
  QCOMPARE(got.size(), expected.size());
  QCOMPARE(got.mid(0, 2), expected.mid(0, 2));
}

void TestScriptLabInput::refusedInput()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(twoObjects())["ok"].toBool());
  QVERIFY(!panel.sendTouch("nobody"));
  QCOMPARE(panel.inputResult()->text(), QString("Error: Touch on nobody was not sent. There is no object named \"nobody\" in the world of the last lsl_run."));
  QVERIFY(!panel.sendChat(0, QString(), QString()));
  QCOMPARE(panel.inputResult()->text(), QString("Error: Chat was not sent. Type the chat text first."));
  QVERIFY(!panel.answerDialog("Lock"));
  QCOMPARE(panel.inputResult()->text(), QString("Error: the dialog answer was not sent. No script has shown a dialog yet."));
  QCOMPARE(panel.inputResult()->palette().color(QPalette::WindowText), ScriptLabPanel::dangerColour(ScriptLabPanel::isDark(panel.palette())));
  // a good one is shown with the success colour, and the word Sent
  QVERIFY(panel.sendTouch("hud"));
  QVERIFY(panel.inputResult()->text().startsWith("Sent: "));
  QCOMPARE(panel.inputResult()->palette().color(QPalette::WindowText), ScriptLabPanel::successColour(ScriptLabPanel::isDark(panel.palette())));
}

void TestScriptLabInput::stoppedWorld()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setSession(&api.lslSession());
  QJsonObject run{{"command", "lsl_run"}, {"seconds", 1}, {"source", "default { state_entry() { integer z = 0; z = 1 / z; } }"}};
  QVERIFY(api.execute(run)["ok"].toBool());
  const QString reason = "Every script has stopped, so nothing can hear an input. Run the script again.";
  QVERIFY(!panel.touchButton()->isEnabled());
  QCOMPARE(panel.inputHint()->text(), reason);
  QCOMPARE(panel.touchButton()->toolTip(), reason);
  // and the buttons come back with a new run
  QVERIFY(api.execute(twoObjects())["ok"].toBool());
  QVERIFY(panel.touchButton()->isEnabled());
}

void TestScriptLabInput::keyboard()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(twoObjects())["ok"].toBool());
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::LivePage);
  // Enter in the text field says the line
  panel.chatText()->setFocus();
  QTest::keyClicks(panel.chatText(), "hi");
  QTest::keyClick(panel.chatText(), Qt::Key_Return);
  QCOMPARE(panel.inputResult()->text(), QString("Sent: Chat on channel 0 from Owner: hi (1 log line(s))."));
  // every control can take the keyboard focus, and Space presses the focused button
  for (QWidget* w : QList<QWidget*>{panel.inputObject(), panel.touchButton(), panel.chatChannel(), panel.chatName(), panel.chatText(), panel.sayButton()})
    QVERIFY2(w->focusPolicy() & Qt::TabFocus, qPrintable(w->objectName()));
  panel.touchButton()->setFocus();
  QTest::keyClick(panel.touchButton(), Qt::Key_Space);
  QVERIFY(panel.inputResult()->text().startsWith("Sent: Touch on hud"));
  for (QPushButton* b : panel.dialogButtons()) QVERIFY(b->focusPolicy() & Qt::TabFocus);
}

void TestScriptLabInput::darkTheme()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(twoObjects())["ok"].toBool());
  QPalette dark = panel.palette();
  dark.setColor(QPalette::Window, QColor("#1E1F22"));
  panel.setPalette(dark);
  QVERIFY(ScriptLabPanel::isDark(panel.palette()));
  QVERIFY(panel.sendTouch("hud"));
  // the same words, the dark tokens: success green #56D39A, danger #FF7B72, muted #AEB2BA
  QCOMPARE(panel.inputResult()->palette().color(QPalette::WindowText), QColor("#56D39A"));
  QVERIFY(!panel.sendTouch("nobody"));
  QCOMPARE(panel.inputResult()->palette().color(QPalette::WindowText), QColor("#FF7B72"));
  QCOMPARE(panel.inputHint()->palette().color(QPalette::WindowText), QColor("#AEB2BA"));
  QVERIFY(panel.touchButton()->styleSheet().contains("#35373C"));
  for (QPushButton* b : panel.dialogButtons()) QVERIFY(b->styleSheet().contains("#35373C"));
}

void TestScriptLabInput::picture()
{
  const QString shot = qEnvironmentVariable("PINGLYMOTION_SCRIPTLAB_INPUT_SHOT");
  if (shot.isEmpty()) QSKIP("no PINGLYMOTION_SCRIPTLAB_INPUT_SHOT");
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(twoObjects())["ok"].toBool());
  panel.tabs()->setCurrentIndex(ScriptLabPanel::LivePage);
  panel.sendTouch("hud");
  panel.chatText()->setText("Hello");
  panel.resize(520, 1100);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.grab().save(shot);
}

QTEST_MAIN(TestScriptLabInput)
#include "tst_scriptlabinput.moc"
