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
 * Slice 2: the OK or Error summary line (its box, icon and bold word), the
 * "Try:" lines, Run again, each part's open or folded state saved and read
 * back, the explanation line that shows with hints, the newest dialog with its
 * buttons inside Objects, and the top bar's Script Lab button, which must show
 * the docked panel (it only pressed itself before).
 *
 * Slice 3: the simulated inventory part (Inventory of <object>, Item and Type,
 * notecard lines, the caption, the object chosen under Send input, the empty
 * line while there is no world, colours and keys), and the empty state's two
 * buttons wrapping at the smallest docked width.
 *
 * Set PINGLYMOTION_SCRIPTLAB_DESIGN_SHOT to a PNG path to save pictures of the
 * panel ("-empty", "-live", "-commands", "-light" are added before ".png").
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFrame>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QMainWindow>
#include <QDockWidget>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include <QTreeWidget>

#include "controlapi.h"
#include "lucideicons.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/objectsview.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/scriptlabsection.h"
#include "constants.h"
#include "theme.h"
#include "topbar.h"
#include "usersettings.h"

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
    void init();
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
    // slice 2
    void summaryLine();
    void tryLines();
    void runAgain();
    void runAgainWithoutScript();
    void partsRemembered();
    void explanationWithHints();
    void dialogInsideObjects();
    void topBarOpensDockedPanel();
    // slice 3
    void inventoryPart();
    void inventoryWithoutWorld();
    void inventoryFollowsObjectAndInput();
    void inventoryTypeWords();
    void inventoryThemesAndKeyboard();
    void emptyStateButtonsWrap();

  private:
    QTemporaryDir m_settingsDir;
};

void TestScriptLabDesign::initTestCase()
{
  // the program applies the theme before the panel is made; the tests do the same
  Theme::apply(true);
  // the settings go to a folder of the test's own, never the user's
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestScriptLabDesign::init()
{
  // every panel starts with the first fold state and hints on
  UserSettings::setScriptLabParts(QString());
  UserSettings::setTipsOn(true);
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
  QVERIFY(panel.liveEmpty()->isAncestorOf(panel.watchButton()));
  QVERIFY(panel.liveEmpty()->isAncestorOf(panel.exampleButton()));
  // the parts of a run are hidden: no folder, no status, no script, no world
  for (const char* key : {"script", "problems", "log", "restrictionsNow", "objects", "input", "inventory"})
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
  QVERIFY(panel.liveStatusText().startsWith("OK: rlv_lock.lsl ran without errors"));
  QVERIFY(panel.liveStatusText().endsWith("RLV restrictions at the end: detach, sendim."));
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
  QCOMPARE(panel.liveStatusText(), QString("Error: the example scripts are not next to the program."));
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
  const QStringList keys = {"restrictions", "actions", "sphere", "replies", "script", "problems", "log", "restrictionsNow", "objects", "input", "inventory"};
  QStringList found;
  for (ScriptLabSection* s : panel.sections()) found << s->key();
  QCOMPARE(found, keys);
  // open at first except Objects and Send input, as in the mockup's fold state, and the vision sphere's sliders
  for (ScriptLabSection* s : panel.sections())
  {
    const bool open = s->key() != "objects" && s->key() != "input" && s->key() != "inventory" && s->key() != "sphere";
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
  // Reset all, then the "Try:" lines, then the Restrictions header, then its list
  const auto nextFocus = [](QWidget* from)
  {
    QWidget* next = from->nextInFocusChain();
    while (next && !(next->focusPolicy() & Qt::TabFocus)) next = next->nextInFocusChain();
    return next;
  };
  QCOMPARE(nextFocus(panel.resetButton()), static_cast<QWidget*>(panel.tryButtons().first()));
  QCOMPARE(nextFocus(panel.tryButtons().last()), static_cast<QWidget*>(restrictions->header()));
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
                           "Type a line above, for example @detach=n, and press Enter.",
                           // slice 2
                           "<span class=\"cap\">Try:</span>", "\"@detach=n,sendim=n\", \"@tploc=n\", \"@clear\", \"@version=2222\", \"@showself=n\"",
                           "Run again", "Runs the last saved script again from the start, with a fresh clock.",
                           "A safe sandbox for RLV and LSL scripts. Nothing here reaches Second Life.",
                           "<b>OK:</b> cuffs.lsl ran without errors (run ", "<b>Error:</b> cuffs.lsl has 1 error and was not run (run ",
                           "icon(ok ? \"circle-check\" : \"circle-x\")", "<div class=\"label\">Dialog from hud</div>",
                           "\"Lock the cuffs?\" on channel -4712",
                           // slice 3
                           "Inventory of cuffs`, \"3 items\"", "<th>Item</th><th>Type</th>", "<td>cuffs.lsl</td><td>Script</td>",
                           "${icon(\"notebook-text\")} Notecard, 4 lines", "<td>wave</td><td>Animation</td>",
                           "Simulated: given with the run, nothing is read from Second Life.", "inv: false"})
    QVERIFY2(page.contains(QString::fromUtf8(text)), text);
}

// --- slice 2 ------------------------------------------------------------------

// the status line as in the mockup: a box in the tone's colour, the circle-check or circle-x icon,
// the word in bold in that colour and the rest in the text colour; idle while nothing ran
void TestScriptLabDesign::summaryLine()
{
  for (const bool dark : {true, false})
  {
    const QJsonObject t = themeTokens(dark);
    RlvState state;
    LabWatcher watcher;
    watcher.setRlvState(&state);
    ScriptLabPanel panel;
    setTheme(panel, dark);
    panel.setState(&state);
    panel.setWatcher(&watcher);
    QTemporaryDir dir;
    QVERIFY(panel.watchFolder(dir.path()));
    const QColor surface(t.value("surface").toString());

    // idle: the border token, the info icon, the whole line muted
    QCOMPARE(panel.liveStatusText(), QString("Watching. No .lsl file in this folder yet: save one here and it runs at once."));
    QCOMPARE(panel.liveStatusTone(), 0);
    QVERIFY(!panel.liveStatusFrame()->isHidden());
    QCOMPARE(panel.liveStatusFrame()->borderColour(), QColor(t.value("border").toString()));
    QCOMPARE(panel.liveStatusFrame()->radius(), Theme::Size::radiusControl);
    QVERIFY(!panel.liveStatusFrame()->dashed());
    QCOMPARE(panel.liveStatus()->palette().color(QPalette::WindowText), QColor(t.value("text_muted").toString()));
    QCOMPARE(panel.liveStatusIcon()->pixmap().toImage(),
             LucideIcons::icon("info", QColor(t.value("text_muted").toString()), QColor(t.value("text_disabled").toString())).pixmap(16, 16).toImage());
    QCOMPARE(panel.liveStatusFrame()->accessibleName(), panel.liveStatusText());

    // OK, in green with the circle-check
    QVERIFY(writeScript(dir.filePath("hello.lsl"), "default\n{\n    state_entry() { llOwnerSay(\"@detach=n\"); }\n}\n"));
    QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 1, 10000);
    QCOMPARE(panel.liveStatusText(), QString("OK: hello.lsl ran without errors (run 1), 3 log line(s). RLV restrictions at the end: detach."));
    QCOMPARE(panel.liveStatusTone(), 1);
    const QColor success(t.value("success").toString());
    QCOMPARE(panel.liveStatusFrame()->borderColour(), success);
    QCOMPARE(panel.liveStatusFrame()->fillColour(), Theme::mix(surface, success, 0.10));
    QCOMPARE(panel.liveStatusIcon()->pixmap().toImage(),
             LucideIcons::icon("circle-check", success, QColor(t.value("text_disabled").toString())).pixmap(16, 16).toImage());
    QCOMPARE(panel.liveStatus()->textFormat(), Qt::RichText);
    QCOMPARE(panel.liveStatus()->text(), "<b>OK:</b><span style=\"color:" + t.value("text").toString()
                                           + "\"> hello.lsl ran without errors (run 1), 3 log line(s). RLV restrictions at the end: detach.</span>");
    QCOMPARE(panel.liveStatus()->palette().color(QPalette::WindowText), success);   // the bold word
    QCOMPARE(panel.liveStatusFrame()->accessibleName(), panel.liveStatusText());

    // Error, in red with the circle-x; a name with < and & is shown as written
    QTest::qWait(1100);
    QVERIFY(writeScript(dir.filePath("hello.lsl"), "default\n{\n    state_entry() { llOwnerSay(1); }\n}\n"));
    QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 2, 10000);
    QCOMPARE(panel.liveStatusText(), QString("Error: hello.lsl has 1 error(s) and was not run (run 2). Fix the line shown below and save."));
    QCOMPARE(panel.liveStatusTone(), 2);
    const QColor danger(t.value("danger").toString());
    QCOMPARE(panel.liveStatusFrame()->borderColour(), danger);
    QCOMPARE(panel.liveStatusFrame()->fillColour(), Theme::mix(surface, danger, 0.10));
    QCOMPARE(panel.liveStatusIcon()->pixmap().toImage(),
             LucideIcons::icon("circle-x", danger, QColor(t.value("text_disabled").toString())).pixmap(16, 16).toImage());
    QVERIFY(panel.liveStatus()->text().startsWith("<b>Error:</b><span style="));
    QCOMPARE(panel.liveStatus()->palette().color(QPalette::WindowText), danger);
    if (dark) saveShot(panel, "-summary", 700);
  }
  Theme::apply(true);
}

// "Try:" and five example lines; a click puts the line in the command field and runs it
void TestScriptLabDesign::tryLines()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  QCOMPARE(panel.tryLabel()->text(), QString("Try:"));
  QVERIFY(panel.tryLabel()->font().pointSizeF() < panel.commandField()->font().pointSizeF());   // the caption size
  const QStringList lines = {"@detach=n,sendim=n", "@tploc=n", "@clear", "@version=2222", "@showself=n"};
  QCOMPARE(ScriptLabPanel::tryLines(), lines);
  QCOMPARE(panel.tryButtons().size(), 5);
  for (int i = 0; i < 5; ++i)
  {
    QPushButton* b = panel.tryButtons().at(i);
    QCOMPARE(b->text(), lines.at(i));
    QCOMPARE(b->toolTip(), QString("Put this line in the command field and run it"));
    QCOMPARE(b->minimumHeight(), Theme::Size::controlCompact);
    QCOMPARE(b->maximumHeight(), Theme::Size::controlCompact);
    QVERIFY(b->focusPolicy() & Qt::TabFocus);
    QVERIFY(b->styleSheet().contains("background: " + themeTokens(true).value("surface_raised").toString() + ";"));
    QCOMPARE(b->parentWidget()->parentWidget(), panel.section("restrictions")->parentWidget());   // on the Type commands page
  }
  QTest::mouseClick(panel.tryButtons().at(0), Qt::LeftButton);
  QCOMPARE(panel.heldChip()->text(), QString("2 held"));
  QCOMPARE(panel.commandField()->text(), QString());   // run, so the field is empty again
  QVERIFY(panel.log()->toPlainText().startsWith("panel: @detach=n,sendim=n\n"));
  QTest::mouseClick(panel.tryButtons().at(1), Qt::LeftButton);
  QCOMPARE(panel.heldChip()->text(), QString("3 held"));
  QTest::mouseClick(panel.tryButtons().at(2), Qt::LeftButton);
  QCOMPARE(panel.heldChip()->text(), QString("0 held"));
  QTest::mouseClick(panel.tryButtons().at(3), Qt::LeftButton);
  QVERIFY2(panel.log()->toPlainText().contains("Reply on channel 2222: "), qPrintable(panel.log()->toPlainText()));
  // the keyboard: Space on the focused line runs it (P8)
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  panel.tryButtons().at(4)->setFocus();
  QTest::keyClick(panel.tryButtons().at(4), Qt::Key_Space);
  QCOMPARE(panel.heldChip()->text(), QString("1 held"));
  QCOMPARE(state.restrictions().first().behaviour, QString("showself"));
  // the lines wrap in a narrow panel instead of widening it
  panel.resize(300, 700);
  QCoreApplication::processEvents();
  QVERIFY(panel.tryButtons().last()->geometry().right() <= panel.tryButtons().last()->parentWidget()->width());
  QVERIFY(panel.tryButtons().last()->y() > panel.tryButtons().first()->y());
}

// Run again runs the shown script once more from its saved file
void TestScriptLabDesign::runAgain()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QVERIFY(panel.runAgainButton()->isHidden());   // no folder: the empty state
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(dir.path()));
  QVERIFY(!panel.runAgainButton()->isHidden());
  QCOMPARE(panel.runAgainButton()->text(), QString("Run again"));
  QVERIFY(!panel.runAgainButton()->icon().isNull());
  QCOMPARE(panel.runAgainButton()->minimumHeight(), Theme::Size::controlCompact);
  QVERIFY(panel.runAgainButton()->styleSheet().contains(":disabled { color: " + themeTokens(true).value("text_disabled").toString() + "; }"));
  QVERIFY(!panel.runAgainButton()->isEnabled());
  QCOMPARE(panel.runAgainButton()->toolTip(), QString("No script yet. Watch a folder and save a .lsl file in it first."));
  QVERIFY(!panel.runAgain());

  QVERIFY(writeScript(dir.filePath("again.lsl"), "default\n{\n    state_entry() { llOwnerSay(\"hi\"); }\n}\n"));
  QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 1, 10000);
  QVERIFY(panel.runAgainButton()->isEnabled());
  QCOMPARE(panel.runAgainButton()->toolTip(), QString("Runs the last saved script again from the start, with a fresh clock."));
  QTest::mouseClick(panel.runAgainButton(), Qt::LeftButton);
  QCOMPARE(watcher.runCount(), 2);
  QCOMPARE(watcher.lastResult().value("file").toString(), QString("again.lsl"));
  QCOMPARE(watcher.lastResult().value("run").toInt(), 2);
  QCOMPARE(panel.liveStatusText(), QString("OK: again.lsl ran without errors (run 2), 3 log line(s)."));
  // a fresh clock: the run starts at 0 s again
  QVERIFY(panel.runLog()->toPlainText().startsWith("0.000 s  state: default"));
  // the keyboard: after Stop watching comes Run again, then Open in editor
  QCOMPARE(panel.unwatchButton()->nextInFocusChain() == panel.runAgainButton()
           || [&]() { QWidget* w = panel.unwatchButton()->nextInFocusChain(); while (w && !(w->focusPolicy() & Qt::TabFocus)) w = w->nextInFocusChain(); return w == panel.runAgainButton(); }(), true);
  QVERIFY(panel.runAgain());
  QCOMPARE(watcher.runCount(), 3);
}

// Run again with the script gone from the folder: greyed out with the reason, nothing is run
void TestScriptLabDesign::runAgainWithoutScript()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(dir.path()));
  QVERIFY(writeScript(dir.filePath("gone.lsl"), "default { state_entry() { } }\n"));
  QTRY_COMPARE_WITH_TIMEOUT(watcher.runCount(), 1, 10000);
  QVERIFY(panel.runAgainButton()->isEnabled());
  QVERIFY(QFile::remove(dir.filePath("gone.lsl")));
  watcher.rescan();
  QVERIFY(!panel.runAgainButton()->isEnabled());
  QCOMPARE(panel.runAgainButton()->toolTip(), QString("The script is not in the folder any more."));
  QVERIFY(!panel.runAgain());
  QCOMPARE(watcher.runCount(), 1);
}

// each part's open or folded state is saved in the settings and read back at the next start
void TestScriptLabDesign::partsRemembered()
{
  {
    ScriptLabPanel panel;
    QCOMPARE(panel.partsState(), QString("restrictions=open,actions=open,sphere=folded,replies=open,script=open,problems=open,log=open,"
                                         "restrictionsNow=open,objects=folded,input=folded,inventory=folded"));
    QCOMPARE(UserSettings::scriptLabParts(), QString());   // nothing saved until a part is folded or opened
    QTest::mouseClick(panel.section("script")->header(), Qt::LeftButton);
    QTest::mouseClick(panel.section("objects")->header(), Qt::LeftButton);
    QTest::mouseClick(panel.section("replies")->header(), Qt::LeftButton);
  }
  const QString saved = "restrictions=open,actions=open,sphere=folded,replies=folded,script=folded,problems=open,log=open,"
                        "restrictionsNow=open,objects=open,input=folded,inventory=folded";
  QCOMPARE(UserSettings::scriptLabParts(), saved);

  // through the settings file, as at the next start
  UserSettings::writeSettings();
  UserSettings::setScriptLabParts(QStringLiteral("script=open"));
  UserSettings::readSettings();
  QCOMPARE(UserSettings::scriptLabParts(), saved);
  {
    QSettings file(QSettings::IniFormat, QSettings::UserScope, Constants::organizationName(), Constants::applicationName());
    QVERIFY2(file.allKeys().filter("scriptlab_parts").size() == 1, qPrintable(file.allKeys().join(", ")));
  }

  ScriptLabPanel next;
  QVERIFY(!next.section("script")->isExpanded());
  QVERIFY(next.section("script")->body()->isHidden());
  QVERIFY(!next.section("replies")->isExpanded());
  QVERIFY(next.section("objects")->isExpanded());
  QVERIFY(!next.section("input")->isExpanded());
  QVERIFY(next.section("log")->isExpanded());
  QCOMPARE(next.partsState(), saved);

  // anything else is left alone: an unknown part, an unknown word, an empty entry
  next.applyPartsState(QStringLiteral("bogus=open,log=maybe,,input=open,=folded"));
  QVERIFY(next.section("input")->isExpanded());
  QVERIFY(next.section("log")->isExpanded());
  QCOMPARE(UserSettings::scriptLabParts(), next.partsState());   // the saved state is always the panel's
  // Space on a header is saved too (P8)
  next.show();
  QVERIFY(QTest::qWaitForWindowExposed(&next));
  next.section("problems")->header()->setFocus();
  QTest::keyClick(next.section("problems")->header(), Qt::Key_Space);
  QVERIFY(UserSettings::scriptLabParts().contains("problems=folded"));
}

// the one-line explanation on top: shown with hints on, gone with hints off (Help > Show hints)
void TestScriptLabDesign::explanationWithHints()
{
  for (const bool dark : {true, false})
  {
    const QJsonObject t = themeTokens(dark);
    ScriptLabPanel panel;
    setTheme(panel, dark);
    QVERIFY(panel.hintsOn());
    QVERIFY(!panel.explanationRow()->isHidden());
    QCOMPARE(panel.explanation()->text(), QString("A safe sandbox for RLV and LSL scripts. Nothing here reaches Second Life."));
    verifyShortSentences(panel.explanation()->text());
    QVERIFY(panel.explanation()->wordWrap());
    QCOMPARE(panel.explanation()->palette().color(QPalette::WindowText), QColor(t.value("text_muted").toString()));
    QVERIFY(!panel.findChild<QLabel*>("scriptLabExplainIcon")->pixmap().isNull());
    // above the Viewer rules
    QVERIFY(panel.explanationRow()->geometry().bottom() <= panel.modeLabel()->mapTo(&panel, QPoint(0, 0)).y() || !panel.isVisible());
    panel.setHintsOn(false);
    QVERIFY(!panel.hintsOn());
    QVERIFY(panel.explanationRow()->isHidden());
    panel.setHintsOn(true);
    QVERIFY(!panel.explanationRow()->isHidden());
  }
  Theme::apply(true);
  UserSettings::setTipsOn(false);
  ScriptLabPanel off;
  QVERIFY(!off.hintsOn());
  QVERIFY(off.explanationRow()->isHidden());
}

// the newest dialog and its buttons sit inside Objects, under the objects, as in the mockup
void TestScriptLabDesign::dialogInsideObjects()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  panel.setSession(&api.lslSession());
  ScriptLabSection* objects = panel.section("objects");
  QCOMPARE(panel.inputDialog()->parentWidget(), objects->body());
  QCOMPARE(panel.dialogMessage()->parentWidget(), objects->body());
  QVERIFY(panel.objectsView()->embedded());
  QVERIFY(panel.objectsView()->dialogText()->isHidden());   // said once, by the panel
  QCOMPARE(panel.inputDialog()->text(), QString("No dialog yet. When a script shows one, its buttons appear here."));
  QVERIFY(panel.dialogMessage()->isHidden());
  QCOMPARE(panel.inputDialog()->font().weight(), QFont::Normal);

  QVERIFY(api.execute(QJsonObject{{"command", "lsl_run"}, {"source",
    "default { state_entry() { llDialog(llGetOwner(), \"Lock the cuffs?\", [\"lock\", \"unlock\"], -4712); } }"}})["ok"].toBool());
  QCOMPARE(panel.inputDialog()->text(), QString("Dialog from Object"));
  QCOMPARE(panel.inputDialog()->font().weight(), QFont::DemiBold);
  QVERIFY(!panel.dialogMessage()->isHidden());
  QCOMPARE(panel.dialogMessage()->text(), QString("\"Lock the cuffs?\" on channel -4712"));
  QCOMPARE(panel.dialogMessage()->palette().color(QPalette::WindowText), QColor(themeTokens(true).value("text_muted").toString()));
  QCOMPARE(panel.dialogButtons().size(), 2);
  for (QPushButton* b : panel.dialogButtons())
  {
    QVERIFY(objects->body()->isAncestorOf(b));
    QVERIFY(!panel.section("input")->body()->isAncestorOf(b));
  }
  QCOMPARE(panel.dialogButtons().at(0)->text(), QString("lock"));
  // the order on screen: objects list, dialog heading, message, buttons
  QVERIFY(objects->bodyLayout()->indexOf(panel.objectsView()) < objects->bodyLayout()->indexOf(panel.inputDialog()));
  QVERIFY(objects->bodyLayout()->indexOf(panel.inputDialog()) < objects->bodyLayout()->indexOf(panel.dialogMessage()));
  // the keyboard: the objects list, the dialog's buttons, then Send input
  QWidget* w = panel.objectsView()->list()->nextInFocusChain();
  while (w && !(w->focusPolicy() & Qt::TabFocus)) w = w->nextInFocusChain();
  QCOMPARE(w, static_cast<QWidget*>(panel.dialogButtons().at(0)));
  QWidget* after = panel.dialogButtons().at(1)->nextInFocusChain();
  while (after && !(after->focusPolicy() & Qt::TabFocus)) after = after->nextInFocusChain();
  QCOMPARE(after, static_cast<QWidget*>(panel.section("input")->header()));
  // pressing one answers the dialog
  panel.dialogButtons().at(1)->click();
  QCOMPARE(panel.inputResult()->text(), QString("Sent: Dialog answer \"unlock\" to Object (1 log line(s))."));
}

// the top bar's Script Lab button shows and hides the docked panel, and follows the panel
// (a bug until slice 2: the button pressed itself but the panel stayed hidden)
void TestScriptLabDesign::topBarOpensDockedPanel()
{
  QMainWindow window;
  window.setCentralWidget(new QWidget(&window));
  QToolBar* bar = window.addToolBar(QStringLiteral("top"));
  ScriptLabPanel* panel = new ScriptLabPanel;
  QDockWidget* dock = ScriptLabPanel::makeDock(&window, panel);
  TopBar::Sources sources;
  sources.scriptLab = dock->toggleViewAction();
  TopBar top(bar, sources, &window);
  window.resize(1200, 800);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QAction* button = top.button("topScriptLabAction");
  QVERIFY(button);
  QVERIFY(button->isCheckable());
  QVERIFY(dock->isHidden());
  QVERIFY(!button->isChecked());

  // a click on the button in the bar
  QTest::mouseClick(top.widgetFor(button), Qt::LeftButton);
  QVERIFY(!dock->isHidden());
  QVERIFY(dock->isVisible());
  QVERIFY(panel->isVisible());
  QVERIFY(dock->width() > 0 && dock->height() > 0);
  QVERIFY(button->isChecked());
  QVERIFY(dock->toggleViewAction()->isChecked());
  // and again: hidden
  QTest::mouseClick(top.widgetFor(button), Qt::LeftButton);
  QVERIFY(dock->isHidden());
  QVERIFY(!button->isChecked());
  // the View menu's action (Ctrl+Shift+L) presses the button
  dock->toggleViewAction()->trigger();
  QVERIFY(!dock->isHidden());
  QVERIFY(button->isChecked());
  // closing the panel with its own x lets the button up
  dock->close();
  QVERIFY(dock->isHidden());
  QVERIFY(!button->isChecked());
  // the keyboard: Space on the focused button opens it (P8)
  top.widgetFor(button)->setFocus();
  QTest::keyClick(top.widgetFor(button), Qt::Key_Space);
  QVERIFY(!dock->isHidden());
  QVERIFY(button->isChecked());
}

// --- slice 3 ------------------------------------------------------------------

namespace
{
  // the mockup's world: cuffs (a script, a notecard of 4 lines, an animation) and a hud
  QJsonObject cuffsRun()
  {
    const QString cuffs = "default { touch_start(integer n) { llRemoveInventory(\"wave\"); } }";
    return QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{
      QJsonObject{{"name", "cuffs"}, {"scripts", QJsonArray{QJsonObject{{"name", "cuffs.lsl"}, {"source", cuffs}}}},
                  {"inventory", QJsonArray{
                     QJsonObject{{"name", "config"}, {"type", "notecard"}, {"lines", QJsonArray{"channel=-4711", "lock=n", "owner=only", "name=cuffs"}}},
                     QJsonObject{{"name", "wave"}, {"type", "animation"}}}}},
      QJsonObject{{"name", "hud"}, {"source", "default { state_entry() { } }"}}}}};
  }

  QStringList rowTexts(const QTreeWidget* list)
  {
    QStringList out;
    for (int i = 0; i < list->topLevelItemCount(); ++i)
      out << list->topLevelItem(i)->text(0) + " | " + list->topLevelItem(i)->text(1);
    return out;
  }
}

// the mockup's Inventory part: "Inventory of cuffs", "3 items", Item and Type, the script first,
// the notecard with its lines and icon, and the caption that it is simulated
void TestScriptLabDesign::inventoryPart()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  panel.setWatcher(&api.lab());
  panel.setSession(&api.lslSession());
  const QJsonObject answer = api.execute(cuffsRun());
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  ScriptLabSection* inv = panel.section("inventory");
  QVERIFY(inv);
  QVERIFY(!inv->isHidden());
  QCOMPARE(inv->title(), QString("Inventory of cuffs"));
  QCOMPARE(inv->count(), QString("3 items"));
  QVERIFY(!inv->isExpanded());   // folded at first, as in the mockup
  QCOMPARE(panel.inventoryList()->parentWidget(), inv->body());
  QCOMPARE(panel.inventoryList()->headerItem()->text(0), QString("Item"));
  QCOMPARE(panel.inventoryList()->headerItem()->text(1), QString("Type"));
  QCOMPARE(rowTexts(panel.inventoryList()), QStringList({"cuffs.lsl | Script", "config | Notecard, 4 lines", "wave | Animation"}));
  QVERIFY(!panel.inventoryList()->topLevelItem(1)->icon(1).isNull());   // the notebook-text icon on the notecard
  QVERIFY(panel.inventoryList()->topLevelItem(0)->icon(1).isNull());
  QVERIFY(panel.inventoryList()->topLevelItem(2)->icon(1).isNull());
  QVERIFY(LucideIcons::names().contains("notebook-text"));
  QVERIFY(panel.inventoryEmpty()->isHidden());
  QVERIFY(!panel.inventoryList()->isHidden());
  QVERIFY(!panel.inventoryNote()->isHidden());
  QCOMPARE(panel.inventoryNote()->text(), QString("Simulated: given with the run, nothing is read from Second Life."));
  QVERIFY(panel.inventoryNote()->font().pointSizeF() < panel.inventoryList()->font().pointSizeF());   // the caption size
  verifyShortSentences(panel.inventoryNote()->text());
  // the last part on the Live page, under Send input
  QCOMPARE(panel.sections().last(), inv);
  QVERIFY(inv->parentWidget()->layout()->indexOf(panel.section("input")) < inv->parentWidget()->layout()->indexOf(inv));
  // the same as lsl_run answered
  const QJsonArray items = answer["objects"].toArray()[0].toObject()["inventory"].toArray();
  QCOMPARE(items.size(), 3);
}

// no world: the part is hidden; a watched folder without a world shows the true reason, no fake items
void TestScriptLabDesign::inventoryWithoutWorld()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  panel.setWatcher(&api.lab());
  panel.setSession(&api.lslSession());
  QVERIFY(panel.section("inventory")->isHidden());
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(panel.watchFolder(dir.path()));
  ScriptLabSection* inv = panel.section("inventory");
  QVERIFY(!inv->isHidden());
  QCOMPARE(inv->title(), QString("Inventory"));
  QCOMPARE(inv->count(), QString("none"));
  QVERIFY(!panel.inventoryEmpty()->isHidden());
  QCOMPARE(panel.inventoryEmpty()->text(), QString("No inventory yet. An AI assistant gives one with lsl_run."));
  verifyShortSentences(panel.inventoryEmpty()->text());
  QVERIFY(panel.inventoryList()->isHidden());
  QVERIFY(panel.inventoryNote()->isHidden());
  QCOMPARE(panel.inventoryList()->topLevelItemCount(), 0);
  // a world comes, then goes again
  QVERIFY(api.execute(cuffsRun())["ok"].toBool());
  QCOMPARE(inv->count(), QString("3 items"));
  api.lslSession().clear();
  QCOMPARE(inv->title(), QString("Inventory"));
  QCOMPARE(inv->count(), QString("none"));
  QVERIFY(!panel.inventoryEmpty()->isHidden());
  panel.stopWatching();
  QVERIFY(inv->isHidden());
}

// the inventory is the one of the object chosen under Send input; a script that removes an item shows at once
void TestScriptLabDesign::inventoryFollowsObjectAndInput()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(cuffsRun())["ok"].toBool());
  QCOMPARE(panel.inputObject()->currentText(), QString("cuffs"));
  panel.inputObject()->setCurrentIndex(1);
  QCOMPARE(panel.section("inventory")->title(), QString("Inventory of hud"));
  QCOMPARE(panel.section("inventory")->count(), QString("1 item"));
  QCOMPARE(rowTexts(panel.inventoryList()), QStringList({"Script | Script"}));
  panel.inputObject()->setCurrentIndex(0);
  QCOMPARE(panel.section("inventory")->title(), QString("Inventory of cuffs"));
  // a touch: cuffs.lsl removes "wave" from its object
  QVERIFY(panel.sendTouch("cuffs"));
  QCOMPARE(panel.section("inventory")->count(), QString("2 items"));
  QCOMPARE(rowTexts(panel.inventoryList()), QStringList({"cuffs.lsl | Script", "config | Notecard, 4 lines"}));
  // a new run keeps the chosen object when it is there
  panel.inputObject()->setCurrentIndex(1);
  QVERIFY(api.execute(cuffsRun())["ok"].toBool());
  QCOMPARE(panel.section("inventory")->title(), QString("Inventory of hud"));
}

// the Type column's words for every INVENTORY_* number the sandbox knows, and the counts
void TestScriptLabDesign::inventoryTypeWords()
{
  const QList<QPair<int, QString>> words = {{0, "Texture"}, {1, "Sound"}, {3, "Landmark"}, {5, "Clothing"}, {6, "Object"},
                                            {10, "Script"}, {13, "Body part"}, {20, "Animation"}, {21, "Gesture"},
                                            {56, "Setting"}, {57, "Material"}};
  for (const auto& w : words) QCOMPARE(ScriptLabPanel::inventoryTypeText(w.first, 0), w.second);
  QCOMPARE(ScriptLabPanel::inventoryTypeText(7, 0), QString("Notecard, 0 lines"));
  QCOMPARE(ScriptLabPanel::inventoryTypeText(7, 1), QString("Notecard, 1 line"));
  QCOMPARE(ScriptLabPanel::inventoryTypeText(7, 4), QString("Notecard, 4 lines"));
  QCOMPARE(ScriptLabPanel::itemCount(0), QString("0 items"));
  QCOMPARE(ScriptLabPanel::itemCount(1), QString("1 item"));
  QCOMPARE(ScriptLabPanel::itemCount(3), QString("3 items"));
}

// the colours are tokens in both themes; the part folds with Space and comes after Send input in the tab order
void TestScriptLabDesign::inventoryThemesAndKeyboard()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(cuffsRun())["ok"].toBool());
  for (const bool dark : {true, false})
  {
    setTheme(panel, dark);
    const QJsonObject t = themeTokens(dark);
    QCOMPARE(panel.inventoryNote()->palette().color(QPalette::WindowText), QColor(t.value("text_muted").toString()));
    QCOMPARE(panel.inventoryEmpty()->palette().color(QPalette::WindowText), QColor(t.value("text_muted").toString()));
    QCOMPARE(rowTexts(panel.inventoryList()).size(), 3);   // the rows survive the switch
    if (!qEnvironmentVariableIsEmpty("PINGLYMOTION_SCRIPTLAB_DESIGN_SHOT"))
    {
      panel.tabs()->setCurrentIndex(ScriptLabPanel::LivePage);
      panel.section("inventory")->setExpanded(true);
      saveShot(panel, dark ? "-inventory" : "-inventory-light", 900);
      panel.section("inventory")->setExpanded(false);
    }
  }
  setTheme(panel, true);

  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::LivePage);
  ScriptLabSection* inv = panel.section("inventory");
  QWidget* w = panel.inputLog()->nextInFocusChain();
  while (w && !(w->focusPolicy() & Qt::TabFocus)) w = w->nextInFocusChain();
  QCOMPARE(w, static_cast<QWidget*>(inv->header()));
  inv->header()->setFocus();
  QTest::keyClick(inv->header(), Qt::Key_Space);
  QVERIFY(inv->isExpanded());
  QVERIFY(!inv->body()->isHidden());
  QVERIFY(panel.partsState().endsWith("inventory=open"));
  QCOMPARE(UserSettings::scriptLabParts(), panel.partsState());   // saved at once
  QWidget* next = inv->header()->nextInFocusChain();
  while (next && !(next->focusPolicy() & Qt::TabFocus)) next = next->nextInFocusChain();
  QCOMPARE(next, static_cast<QWidget*>(panel.inventoryList()));
  QTest::keyClick(inv->header(), Qt::Key_Space);
  QVERIFY(!inv->isExpanded());
}

// at the smallest docked width the empty state's two buttons wrap instead of being cut off
void TestScriptLabDesign::emptyStateButtonsWrap()
{
  ScriptLabPanel panel;
  panel.tabs()->setCurrentIndex(ScriptLabPanel::LivePage);
  panel.resize(282, 600);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  QCoreApplication::processEvents();
  QWidget* card = panel.liveEmpty();
  for (QPushButton* b : {panel.watchButton(), panel.exampleButton()})
  {
    const QRect r(b->mapTo(card, QPoint(0, 0)), b->size());
    QVERIFY2(card->rect().contains(r), qPrintable(QStringLiteral("%1: %2,%3 %4x%5 in %6x%7").arg(b->text()).arg(r.x()).arg(r.y())
                                                    .arg(r.width()).arg(r.height()).arg(card->width()).arg(card->height())));
  }
  // under each other when narrow
  QVERIFY(panel.exampleButton()->mapTo(card, QPoint(0, 0)).y() > panel.watchButton()->mapTo(card, QPoint(0, 0)).y());
  // side by side when there is room, as in the mockup
  panel.resize(700, 600);
  QCoreApplication::processEvents();
  QCOMPARE(panel.exampleButton()->mapTo(card, QPoint(0, 0)).y(), panel.watchButton()->mapTo(card, QPoint(0, 0)).y());
  QVERIFY(panel.exampleButton()->x() > panel.watchButton()->x());
}

QTEST_MAIN(TestScriptLabDesign)
#include "tst_scriptlabdesign.moc"
