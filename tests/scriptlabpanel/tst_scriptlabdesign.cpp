/*
 * PinglyMotion - tests of the Script Lab panel in the new design, slice 1
 *
 * The panel frame from the approved mockup (docs/mockup/index.html, the
 * Script Lab page; docs/UX_DECISIONS.md, 6d), with exact answers only: the
 * "Viewer rules" RLV / RLVa switch and the "N held" chip (both ways: the switch
 * changes the RLV state, and rlv_reset with a mode moves the switch), the two
 * pages "Live scripts" and "Type commands", the empty state "No folder
 * watched" with its two buttons, the folding parts with their counts (script,
 * errors and warnings with the lint warnings, event log, RLV restrictions now,
 * objects, send input; restrictions, what the avatar may do, replies), the
 * keyboard, and the token colours in the dark and the light theme. The texts
 * the mockup has are also looked up in the mockup file itself.
 *
 * Set PINGLYMOTION_SCRIPTLAB_DESIGN_SHOT to a PNG path to save pictures of the
 * panel ("-empty", "-live", "-commands", "-light" are added before ".png").
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QCheckBox>
#include <QFile>
#include <QFrame>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>

#include "controlapi.h"
#include "lucideicons.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/objectsview.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/scriptlabsection.h"
#include "theme.h"

namespace
{
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

  QString mockup() { return readFile(QStringLiteral(PINGLYMOTION_DOCS_DIR "/mockup/index.html")); }

  QJsonObject themeTokens(bool dark)
  {
    const QJsonObject tokens = QJsonDocument::fromJson(readFile(PINGLYMOTION_DOCS_DIR "/design_tokens.json").toUtf8()).object();
    return tokens.value("themes").toObject().value(dark ? "dark" : "light").toObject();
  }

  // the sentences of a text are at most 12 words each (tools/check_texts.py)
  void verifyShortSentences(const QString& text)
  {
    for (const QString& sentence : text.split(QStringLiteral(". ")))
      QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
  }

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

  // the program's theme for the whole application, as View > Light theme does
  void setTheme(QWidget& widget, bool dark)
  {
    Theme::apply(dark);
    QCoreApplication::processEvents();
    QCOMPARE(ScriptLabPanel::isDark(widget.palette()), dark);
  }

  void saveShot(QWidget& panel, const QString& suffix, int height)
  {
    QString shot = qEnvironmentVariable("PINGLYMOTION_SCRIPTLAB_DESIGN_SHOT");
    if (shot.isEmpty()) return;
    shot.replace(".png", suffix + ".png");
    panel.resize(440, height);
    panel.show();
    QVERIFY(panel.grab().save(shot));
  }
}

class TestScriptLabDesign : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void viewerRulesSwitch();
    void switchFollowsControlCommand();
    void pages();
    void emptyState();
    void tryExampleScripts();
    void tryExampleScriptsMissing();
    void sectionsOnTypeCommands();
    void sectionsOnLiveScripts();
    void objectsAndInputWithoutFolder();
    void foldingAndKeyboard();
    void themes();
    void textsInMockup();
};

void TestScriptLabDesign::initTestCase()
{
  // the program applies the theme before the panel is made; the tests do the same
  Theme::apply(true);
}

// "Viewer rules": RLV or RLVa (RLVa first chosen); switching removes every restriction, as after a relog
void TestScriptLabDesign::viewerRulesSwitch()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  QCOMPARE(panel.modeLabel()->text(), QString("Viewer rules"));
  QCOMPARE(panel.modeRlvButton()->text(), QString("RLV"));
  QCOMPARE(panel.modeRlvaButton()->text(), QString("RLVa"));
  QVERIFY(panel.modeRlvaButton()->isChecked());
  QVERIFY(!panel.modeRlvButton()->isChecked());
  QVERIFY(panel.modeRlvaButton()->font().weight() > panel.modeRlvButton()->font().weight());   // a shape, not only a colour (P10)
  QCOMPARE(panel.modeRlvButton()->toolTip(), QString("Plain RLV, as the RLV wiki lists it. Switching removes every restriction, as after a relog."));
  QCOMPARE(panel.modeRlvaButton()->toolTip(), QString("RLVa, as in Firestorm and other viewers. Switching removes every restriction, as after a relog."));
  QCOMPARE(panel.heldChip()->text(), QString("0 held"));
  QCOMPARE(panel.heldChipFrame()->accessibleName(), QString("0 held"));

  QVERIFY(panel.runLine("@detach=n,sendim=n"));
  QCOMPARE(panel.heldChip()->text(), QString("2 held"));

  // to plain RLV: the state's mode, no restrictions left, a line in the log
  QTest::mouseClick(panel.modeRlvButton(), Qt::LeftButton);
  QCOMPARE(state.mode(), Rlv::Mode::Rlv);
  QVERIFY(panel.modeRlvButton()->isChecked());
  QVERIFY(!panel.modeRlvaButton()->isChecked());
  QVERIFY(panel.modeRlvButton()->font().weight() > panel.modeRlvaButton()->font().weight());
  QCOMPARE(state.restrictions().size(), 0);
  QCOMPARE(panel.heldChip()->text(), QString("0 held"));
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 0);
  QVERIFY(panel.log()->toPlainText().endsWith("Viewer rules: RLV. 2 restriction(s) removed."));

  // clicking the chosen one again changes nothing
  QTest::mouseClick(panel.modeRlvButton(), Qt::LeftButton);
  QCOMPARE(state.mode(), Rlv::Mode::Rlv);
  QVERIFY(panel.log()->toPlainText().endsWith("Viewer rules: RLV. 2 restriction(s) removed."));

  // and back to RLVa
  QVERIFY(panel.runLine("@fly=n"));
  QCOMPARE(panel.heldChip()->text(), QString("1 held"));
  QTest::mouseClick(panel.modeRlvaButton(), Qt::LeftButton);
  QCOMPARE(state.mode(), Rlv::Mode::Rlva);
  QVERIFY(panel.modeRlvaButton()->isChecked());
  QCOMPARE(panel.heldChip()->text(), QString("0 held"));
  QVERIFY(panel.log()->toPlainText().endsWith("Viewer rules: RLVa. 1 restriction(s) removed."));
}

// rlv_reset with a mode (an AI client) moves the switch; the panel's switch is seen by rlv_reset's answer
void TestScriptLabDesign::switchFollowsControlCommand()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  QVERIFY(panel.modeRlvaButton()->isChecked());
  QJsonObject answer = api.execute(QJsonObject{{"command", "rlv_reset"}, {"mode", "rlv"}});
  QVERIFY(answer.value("ok").toBool());
  QVERIFY(panel.modeRlvButton()->isChecked());
  QVERIFY(!panel.modeRlvaButton()->isChecked());
  QTest::mouseClick(panel.modeRlvaButton(), Qt::LeftButton);
  answer = api.execute(QJsonObject{{"command", "rlv_reset"}});
  QCOMPARE(answer.value("mode").toString(), QString("rlva"));
  // a new panel on the same state shows the state's mode
  QVERIFY(api.execute(QJsonObject{{"command", "rlv_reset"}, {"mode", "rlv"}}).value("ok").toBool());
  ScriptLabPanel second;
  second.setState(&api.rlv());
  QVERIFY(second.modeRlvButton()->isChecked());
}

// two pages, Live scripts first, with their tips and icons
void TestScriptLabDesign::pages()
{
  ScriptLabPanel panel;
  QCOMPARE(panel.tabs()->count(), 2);
  QCOMPARE(int(ScriptLabPanel::LivePage), 0);
  QCOMPARE(int(ScriptLabPanel::CommandsPage), 1);
  QCOMPARE(panel.tabs()->tabText(ScriptLabPanel::LivePage), QString("Live scripts"));
  QCOMPARE(panel.tabs()->tabText(ScriptLabPanel::CommandsPage), QString("Type commands"));
  QCOMPARE(panel.tabs()->tabToolTip(ScriptLabPanel::LivePage), QString("Live: your .lsl scripts run each time you save them"));
  QCOMPARE(panel.tabs()->tabToolTip(ScriptLabPanel::CommandsPage), QString("Commands: type RLV lines by hand"));
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));
  QVERIFY(!panel.tabs()->tabIcon(ScriptLabPanel::LivePage).isNull());
  QVERIFY(!panel.tabs()->tabIcon(ScriptLabPanel::CommandsPage).isNull());
  QCOMPARE(panel.resetButton()->text(), QString("Reset all"));
  QCOMPARE(panel.resetButton()->toolTip(), QString("Reset: remove every restriction of every object"));
  QCOMPARE(panel.actionList()->headerItem()->text(1), QString("Now"));
}

// no folder watched: the empty state with a heading, one line and two large buttons; nothing else of a run
void TestScriptLabDesign::emptyState()
{
  ScriptLabPanel panel;
  LabWatcher watcher;
  panel.setWatcher(&watcher);
  panel.setExampleScriptsFolder(PINGLYMOTION_EXAMPLES_DIR "/scriptlab");
  QVERIFY(!panel.liveEmpty()->isHidden());
  QCOMPARE(panel.liveEmptyHeading()->text(), QString("No folder watched"));
  QCOMPARE(panel.liveEmptyText()->text(), QString("Pick the folder with your .lsl scripts. Each time you save one, it is checked and run here."));
  verifyShortSentences(panel.liveEmptyText()->text());
  QVERIFY(panel.liveEmptyText()->wordWrap());
  QVERIFY(panel.liveEmptyHeading()->font().pointSizeF() > panel.liveEmptyText()->font().pointSizeF());
  QCOMPARE(panel.watchButton()->text(), QString("Watch folder..."));
  QCOMPARE(panel.exampleButton()->text(), QString("Try the example scripts"));
  QCOMPARE(panel.watchButton()->minimumHeight(), Theme::Size::controlLarge);
  QCOMPARE(panel.exampleButton()->minimumHeight(), Theme::Size::controlLarge);
  QVERIFY(!panel.watchButton()->icon().isNull());
  QVERIFY(!panel.exampleButton()->icon().isNull());
  QVERIFY(panel.exampleButton()->isEnabled());
  QCOMPARE(panel.exampleButton()->toolTip(), QString("Watch the example scripts that come with the program. One runs at once."));
  QCOMPARE(panel.watchButton()->parentWidget(), static_cast<QWidget*>(panel.liveEmpty()));
  QCOMPARE(panel.exampleButton()->parentWidget(), static_cast<QWidget*>(panel.liveEmpty()));
  // the parts of a run are hidden: no folder, no status, no script, no world
  for (const char* key : {"script", "problems", "log", "restrictionsNow", "objects", "input"})
    QVERIFY2(panel.section(key)->isHidden(), key);
  QVERIFY(panel.liveFolderField()->isHidden());
  QVERIFY(panel.unwatchButton()->isHidden());
  QVERIFY(panel.openEditorButton()->isHidden());
  QVERIFY(panel.liveStatus()->isHidden());
  QVERIFY(panel.effectsBox()->isHidden());
  saveShot(panel, "-empty", 520);
}

// "Try the example scripts" watches examples/scriptlab and runs the RLV example at once
void TestScriptLabDesign::tryExampleScripts()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.setExampleScriptsFolder(PINGLYMOTION_EXAMPLES_DIR "/scriptlab");
  QTest::mouseClick(panel.exampleButton(), Qt::LeftButton);
  QVERIFY(watcher.watching());
  QCOMPARE(QFileInfo(watcher.folder()).canonicalFilePath(), QFileInfo(PINGLYMOTION_EXAMPLES_DIR "/scriptlab").canonicalFilePath());
  QCOMPARE(watcher.lastResult().value("file").toString(), QString("rlv_lock.lsl"));
  QVERIFY(panel.liveEmpty()->isHidden());
  QCOMPARE(panel.liveScriptName()->title(), QString("Script: rlv_lock.lsl"));
  QCOMPARE(panel.heldChip()->text(), QString("2 held"));
  QCOMPARE(panel.section("restrictionsNow")->count(), QString("2"));
  QVERIFY(panel.liveStatus()->text().startsWith("OK: rlv_lock.lsl ran without errors"));
  QVERIFY(panel.liveStatus()->text().endsWith("RLV restrictions at the end: detach, sendim."));
  // the example scripts give no warnings (tests/tst_lsllint.cpp)
  QCOMPARE(panel.section("problems")->count(), QString("none"));
}

// without the examples folder the button is greyed out with a reason (charter P6)
void TestScriptLabDesign::tryExampleScriptsMissing()
{
  ScriptLabPanel panel;
  LabWatcher watcher;
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  panel.setExampleScriptsFolder(dir.filePath("no-such-folder"));
  QVERIFY(!panel.exampleButton()->isEnabled());
  QCOMPARE(panel.exampleButton()->toolTip(), QString("The example scripts are not next to the program."));
  QVERIFY(!panel.tryExampleScripts());
  QVERIFY(!watcher.watching());
  QCOMPARE(panel.liveStatus()->text(), QString("Error: the example scripts are not next to the program."));
  QVERIFY(!panel.liveStatus()->isHidden());
}

// Type commands: Restrictions with a count, What the avatar may do, Replies and messages with a count
void TestScriptLabDesign::sectionsOnTypeCommands()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  ScriptLabSection* restrictions = panel.section("restrictions");
  ScriptLabSection* actions = panel.section("actions");
  ScriptLabSection* replies = panel.section("replies");
  QVERIFY(restrictions && actions && replies);
  QCOMPARE(restrictions->title(), QString("Restrictions"));
  QCOMPARE(actions->title(), QString("What the avatar may do"));
  QCOMPARE(replies->title(), QString("Replies and messages"));
  QCOMPARE(restrictions->count(), QString("0"));
  QCOMPARE(actions->count(), QString());
  QVERIFY(actions->countLabel()->isHidden());
  QCOMPARE(replies->count(), QString("none"));
  QCOMPARE(restrictions->header()->accessibleName(), QString("Restrictions, 0"));
  // the empty text instead of an empty table
  QVERIFY(!panel.emptyHint()->isHidden());
  QVERIFY(panel.restrictionList()->isHidden());
  QCOMPARE(panel.restrictionList()->parentWidget(), restrictions->body());
  QCOMPARE(panel.actionList()->parentWidget(), actions->body());
  QCOMPARE(panel.log()->parentWidget(), replies->body());

  QVERIFY(panel.runLine("@detach=n,sendim=n"));
  QCOMPARE(restrictions->count(), QString("2"));
  QCOMPARE(restrictions->countLabel()->text(), QString("2"));
  QCOMPARE(restrictions->header()->accessibleName(), QString("Restrictions, 2"));
  QVERIFY(panel.emptyHint()->isHidden());
  QVERIFY(!panel.restrictionList()->isHidden());
  QCOMPARE(replies->count(), QString("1"));
  QVERIFY(panel.runLine("@versionnum=2222"));
  QCOMPARE(replies->count(), QString("2"));
  panel.resetAll();
  QCOMPARE(restrictions->count(), QString("0"));
  QCOMPARE(replies->count(), QString("3"));
  QCOMPARE(panel.section("restrictionsNow")->count(), QString("0"));
}

// Live scripts while watching: Script (read only), Errors and warnings (with lint), Event log, RLV restrictions now
void TestScriptLabDesign::sectionsOnLiveScripts()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(panel.watchFolder(dir.path()));
  QVERIFY(panel.liveEmpty()->isHidden());
  for (const char* key : {"script", "problems", "log", "restrictionsNow"})
    QVERIFY2(!panel.section(key)->isHidden(), key);
  QVERIFY(!panel.liveFolderField()->isHidden());
  QVERIFY(!panel.unwatchButton()->isHidden());
  QVERIFY(!panel.liveStatus()->isHidden());
  QVERIFY(!panel.effectsBox()->isHidden());
  QCOMPARE(panel.section("script")->title(), QString("Script"));
  QCOMPARE(panel.section("script")->count(), QString("read only"));
  QCOMPARE(panel.section("problems")->title(), QString("Errors and warnings"));
  QCOMPARE(panel.section("problems")->count(), QString("none"));
  QCOMPARE(panel.section("log")->title(), QString("Event log"));
  QCOMPARE(panel.section("log")->count(), QString("none"));
  QCOMPARE(panel.section("restrictionsNow")->title(), QString("RLV restrictions now"));
  QCOMPARE(panel.section("restrictionsNow")->count(), QString("0"));
  QVERIFY(!panel.liveRestrictionsEmpty()->isHidden());
  QCOMPARE(panel.liveRestrictionsEmpty()->text(), QString("No restrictions. A script that says @detach=n holds one."));
  QCOMPARE(panel.scriptView()->parentWidget(), panel.section("script")->body());
  QCOMPARE(panel.problemList()->parentWidget(), panel.section("problems")->body());
  QCOMPARE(panel.runLog()->parentWidget(), panel.section("log")->body());

  // an error: not run
  QVERIFY(writeScript(dir.filePath("collar.lsl"), "default\n{\n    state_entry() { llOwnerSay(1); }\n}\n"));
  QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 1, 10000);
  QCOMPARE(panel.section("script")->title(), QString("Script: collar.lsl"));
  QCOMPARE(panel.section("problems")->count(), QString("1 error"));
  QCOMPARE(panel.section("log")->count(), QString("not run"));

  // fixed, with an unused local: the lint warning shows in the panel, in the warning colour
  QTest::qWait(1100);   // a new file time
  QVERIFY(writeScript(dir.filePath("collar.lsl"),
                      "default\n{\n    state_entry()\n    {\n        integer unused;\n        llOwnerSay(\"@detach=n\");\n    }\n}\n"));
  QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 2, 10000);
  QCOMPARE(listTexts(panel.problemList()), QStringList{"Warning, line 5, column 17: The local variable unused is never used."});
  const bool dark = ScriptLabPanel::isDark(panel.palette());
  QCOMPARE(panel.problemList()->item(0)->foreground().color(), Theme::color(Theme::Token::Warning, dark));
  QCOMPARE(panel.section("problems")->count(), QString("1 warning"));
  QCOMPARE(panel.scriptView()->extraSelections().size(), 1);
  QCOMPARE(panel.section("log")->count(), QString("3 lines"));
  QCOMPARE(panel.runLog()->toPlainText(), QString("0.000 s  state: default\n0.000 s  event: state_entry()\n0.000 s  rlv_command: @detach=n"));
  QCOMPARE(panel.section("restrictionsNow")->count(), QString("1"));
  QCOMPARE(panel.liveRestrictionList()->topLevelItemCount(), 1);
  QCOMPARE(panel.liveRestrictionList()->topLevelItem(0)->text(0), QString("detach"));
  QVERIFY(panel.liveRestrictionsEmpty()->isHidden());
  QCOMPARE(panel.heldChip()->text(), QString("1 held"));
  saveShot(panel, "-live", 900);

  // two errors and a warning from another script
  QTest::qWait(1100);
  QVERIFY(writeScript(dir.filePath("collar.lsl"), "default\n{\n    state_entry() { llOwnerSay(1); llOwnerSay(2); }\n}\n"));
  QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 3, 10000);
  QCOMPARE(panel.section("problems")->count(), QString("2 errors"));

  panel.stopWatching();
  QVERIFY(!panel.liveEmpty()->isHidden());
  QVERIFY(panel.section("script")->isHidden());
}

// an lsl_run world (from an AI assistant) needs no folder: Objects and Send input show with a count
void TestScriptLabDesign::objectsAndInputWithoutFolder()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  panel.setWatcher(&api.lab());
  panel.setSession(&api.lslSession());
  QVERIFY(panel.section("objects")->isHidden());
  QVERIFY(panel.section("input")->isHidden());
  QVERIFY(api.execute(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { llOwnerSay(\"hi\"); } }"}})["ok"].toBool());
  QVERIFY(!panel.section("objects")->isHidden());
  QVERIFY(!panel.section("input")->isHidden());
  QVERIFY(!panel.effectsBox()->isHidden());
  QVERIFY(!panel.liveEmpty()->isHidden());   // still no folder
  QCOMPARE(panel.section("objects")->title(), QString("Objects"));
  QCOMPARE(panel.section("objects")->count(), QString("1"));
  QCOMPARE(panel.section("input")->title(), QString("Send input"));
  QCOMPARE(panel.section("input")->count(), QString());
  QCOMPARE(panel.objectsView()->parentWidget(), panel.section("objects")->body());
  QCOMPARE(panel.touchButton()->parentWidget(), panel.section("input")->body());
  // folded at first, as in the mockup
  QVERIFY(!panel.section("objects")->isExpanded());
  QVERIFY(!panel.section("input")->isExpanded());
}

// each part folds with a click, Space or Enter on its header; the chevron shows the state; tab order follows the screen
void TestScriptLabDesign::foldingAndKeyboard()
{
  ScriptLabPanel panel;
  // made in this order: the Type commands page first, then the Live scripts page
  const QStringList keys = {"restrictions", "actions", "replies", "script", "problems", "log", "restrictionsNow", "objects", "input"};
  QStringList found;
  for (ScriptLabSection* s : panel.sections()) found << s->key();
  QCOMPARE(found, keys);
  // open at first except Objects and Send input, as in the mockup's fold state
  for (ScriptLabSection* s : panel.sections())
  {
    const bool open = s->key() != "objects" && s->key() != "input";
    QCOMPARE(s->isExpanded(), open);
    QCOMPARE(s->body()->isHidden(), !open);
    QCOMPARE(s->objectName(), "scriptLabSection_" + s->key());
    QVERIFY(s->header()->focusPolicy() & Qt::TabFocus);
    QCOMPARE(s->header()->minimumHeight(), Theme::Size::controlCompact);
  }

  ScriptLabSection* restrictions = panel.section("restrictions");
  const qint64 openIcon = restrictions->header()->icon().cacheKey();
  QCOMPARE(restrictions->header()->toolTip(), QString("Hide this part (Space)"));
  QSignalSpy spy(restrictions, &ScriptLabSection::expandedChanged);
  QTest::mouseClick(restrictions->header(), Qt::LeftButton);
  QVERIFY(!restrictions->isExpanded());
  QVERIFY(restrictions->body()->isHidden());
  QCOMPARE(spy.count(), 1);
  QCOMPARE(restrictions->header()->toolTip(), QString("Show this part (Space)"));
  QVERIFY(restrictions->header()->icon().cacheKey() != openIcon);   // chevron right, not down
  // the chevrons are the Lucide ones
  QVERIFY(LucideIcons::names().contains("chevron-right"));
  QVERIFY(LucideIcons::names().contains("chevron-down"));

  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  restrictions->header()->setFocus();
  QTest::keyClick(restrictions->header(), Qt::Key_Space);
  QVERIFY(restrictions->isExpanded());
  QVERIFY(!restrictions->body()->isHidden());
  QCOMPARE(restrictions->header()->icon().cacheKey() == openIcon, false);   // a new icon, the down chevron again
  // Reset all, then the Restrictions header, then its list
  QWidget* next = panel.resetButton()->nextInFocusChain();
  while (next && !(next->focusPolicy() & Qt::TabFocus)) next = next->nextInFocusChain();
  QCOMPARE(next, static_cast<QWidget*>(restrictions->header()));
  // the Viewer rules: Space on the focused button switches (P8)
  RlvState state;
  panel.setState(&state);
  panel.modeRlvButton()->setFocus();
  QTest::keyClick(panel.modeRlvButton(), Qt::Key_Space);
  QCOMPARE(state.mode(), Rlv::Mode::Rlv);
  QVERIFY(panel.modeRlvButton()->focusPolicy() & Qt::TabFocus);
  // an arrow key moves inside the group (docs/DESIGN_SYSTEM.md, "Keyboard")
  QTest::keyClick(panel.modeRlvButton(), Qt::Key_Right);
  QVERIFY(panel.modeRlvaButton()->hasFocus());
  QTest::keyClick(panel.modeRlvaButton(), Qt::Key_Space);
  QCOMPARE(state.mode(), Rlv::Mode::Rlva);
}

// the colours are the tokens of docs/design_tokens.json, in the dark and the light theme
void TestScriptLabDesign::themes()
{
  for (const bool dark : {true, false})
  {
    const QJsonObject t = themeTokens(dark);
    QVERIFY(!t.isEmpty());
    ScriptLabPanel panel;
    setTheme(panel, dark);
    QCOMPARE(ScriptLabPanel::isDark(panel.palette()), dark);

    // the Viewer rules segment: normal buttons, the chosen one in the selection colour
    for (QPushButton* b : {panel.modeRlvButton(), panel.modeRlvaButton()})
    {
      const QString css = b->styleSheet();
      QVERIFY2(css.contains("QPushButton { background: " + t.value("surface_raised").toString() + "; color: " + t.value("text").toString()
                            + "; border: 1px solid " + t.value("border").toString() + ";"), qPrintable(css));
      QVERIFY2(css.contains("QPushButton:checked { background: " + t.value("selection").toString() + "; color: "
                            + t.value("on_accent").toString() + "; border: 1px solid " + t.value("selection").toString() + "; }"), qPrintable(css));
      QVERIFY(css.contains("QPushButton:focus { border: 2px solid " + t.value("focus_ring").toString() + "; }"));
      QCOMPARE(b->minimumHeight(), Theme::Size::controlCompact);
    }
    QVERIFY(panel.modeRlvButton()->styleSheet().contains("border-top-left-radius: 6px; border-bottom-left-radius: 6px;"));
    QVERIFY(panel.modeRlvaButton()->styleSheet().contains("border-top-right-radius: 6px; border-bottom-right-radius: 6px;"));

    // the chip: the border token at 14 percent on the surface, a round border, text in the text colour
    const QColor surface(t.value("surface").toString());
    const QColor border(t.value("border").toString());
    QCOMPARE(panel.heldChipFrame()->borderColour(), border);
    QCOMPARE(panel.heldChipFrame()->fillColour(), Theme::mix(surface, border, 0.14));
    QCOMPARE(panel.heldChipFrame()->radius(), 10);
    QVERIFY(!panel.heldChipFrame()->dashed());
    QCOMPARE(panel.heldChip()->palette().color(QPalette::WindowText), QColor(t.value("text").toString()));

    // the empty state: a dashed border, radius.panel, the line in text_muted; Watch folder is the main button
    QCOMPARE(panel.liveEmpty()->borderColour(), border);
    QVERIFY(!panel.liveEmpty()->fillColour().isValid());
    QCOMPARE(panel.liveEmpty()->radius(), Theme::Size::radiusPanel);
    QVERIFY(panel.liveEmpty()->dashed());
    QCOMPARE(panel.liveEmptyText()->palette().color(QPalette::WindowText), QColor(t.value("text_muted").toString()));
    QVERIFY(panel.watchButton()->styleSheet().contains("background: " + t.value("accent").toString() + ";"));
    QVERIFY(panel.exampleButton()->styleSheet().contains("background: " + t.value("surface_raised").toString() + ";"));
    QVERIFY(panel.exampleButton()->styleSheet().contains(":disabled { color: " + t.value("text_disabled").toString() + "; }"));

    // the parts: the border token at 60 percent on the surface, radius.control; the count in text_muted
    for (ScriptLabSection* s : panel.sections())
    {
      QCOMPARE(s->borderColour(), Theme::mix(surface, border, 0.6));
      QCOMPARE(s->radius(), Theme::Size::radiusControl);
      QVERIFY(!s->dashed());
      QVERIFY(s->styleSheet().isEmpty());   // no style sheet over the labels inside
      QVERIFY(s->header()->styleSheet().contains("color: " + t.value("text").toString() + ";"));
      QVERIFY(s->header()->styleSheet().contains("QPushButton:focus { border: 2px solid " + t.value("focus_ring").toString() + ";"));
      QCOMPARE(s->countLabel()->palette().color(QPalette::WindowText), QColor(t.value("text_muted").toString()));
    }
    if (!dark) saveShot(panel, "-light", 700);
  }

  // switching the theme on a live panel gives the new tokens at once
  ScriptLabPanel panel;
  setTheme(panel, true);
  QVERIFY(panel.section("log")->countLabel()->palette().color(QPalette::WindowText) == QColor(themeTokens(true).value("text_muted").toString()));
  setTheme(panel, false);
  QCOMPARE(panel.section("log")->countLabel()->palette().color(QPalette::WindowText), QColor(themeTokens(false).value("text_muted").toString()));
  QCOMPARE(panel.liveEmpty()->borderColour(), QColor(themeTokens(false).value("border").toString()));
  QCOMPARE(panel.liveEmptyText()->palette().color(QPalette::WindowText), QColor(themeTokens(false).value("text_muted").toString()));
  RlvState state;
  panel.setState(&state);
  panel.runLine("@detach=n");
  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  saveShot(panel, "-commands", 760);
  Theme::apply(true);
}

// the texts that come from the mockup are in the mockup file, letter for letter
void TestScriptLabDesign::textsInMockup()
{
  const QString page = mockup();
  QVERIFY(!page.isEmpty());
  for (const char* text : {"Viewer rules", "Live scripts", "Type commands", "No folder watched", "Watch folder...", "Try the example scripts",
                           "Errors and warnings", "Event log", "RLV restrictions now", "}Objects`", "Send input", "What the avatar may do",
                           "Replies and messages", "Reset all", " held</span>", "read only", "Pick the folder with your .lsl scripts. Each time you save one, it is checked and run here",
                           "Plain RLV, as the RLV wiki lists it", "RLVa, as in Firestorm and other viewers",
                           "Switching removes every restriction, as after a relog.", "Live: your .lsl scripts run each time you save them",
                           "Commands: type RLV lines by hand", "Reset: remove every restriction of every object",
                           "No restrictions. ", "A script that says @detach=n holds one.",
                           "Type a line above, for example @detach=n, and press Enter."})
    QVERIFY2(page.contains(QString::fromUtf8(text)), text);
}

QTEST_MAIN(TestScriptLabDesign)
#include "tst_scriptlabdesign.moc"
