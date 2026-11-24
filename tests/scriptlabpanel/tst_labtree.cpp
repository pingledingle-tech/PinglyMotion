/*
 * PinglyMotion - tests for the Script Lab folder tree and folder mode (lane 1, job D)
 *
 * Watch folder watches the folder as one object (LabFolder, the watcher's
 * "linkset" mode); the "Object" part shows it as a tree with icons, colour
 * dots, sums and memory per script; a click or Enter on a script shows it; a
 * log line of another script shows that script; the folder's limits are in
 * Errors and warnings; the maker guide; the example scripts stay separate.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QFontDatabase>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QToolButton>

#include "lucideicons.h"
#include "scriptlab/labhelpview.h"
#include "scriptlab/labtint.h"
#include "scriptlab/labtree.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/scriptlabsection.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  QJsonObject themeTokens(bool dark)
  {
    const QJsonObject tokens = QJsonDocument::fromJson(readFile(PINGLYMOTION_DOCS_DIR "/design_tokens.json").toUtf8()).object();
    return tokens.value("themes").toObject().value(dark ? "dark" : "light").toObject();
  }

  bool writeFile(const QString& path, const QByteArray& text)
  {
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  QStringList rowTexts(const QTreeWidgetItem* item)
  {
    QStringList out;
    for (int c = 0; c < 4; ++c) out << item->text(c);
    return out;
  }

  // the colour in the middle of a row's dot
  QColor dotColour(const QTreeWidgetItem* item)
  {
    const QImage image = item->icon(LabFolderTree::StateColumn).pixmap(16, 16).toImage();
    return image.pixelColor(8, 8);
  }

  const QByteArray MainScript =
    "string greeting = \"hello\";\ndefault\n{\n    state_entry()\n    {\n        llOwnerSay(\"main ready\");\n        llMessageLinked(LINK_SET, 7, greeting, NULL_KEY);\n    }\n}\n";
  const QByteArray LidScript =
    "default\n{\n    link_message(integer sender, integer num, string text, key id)\n    {\n        llOwnerSay(\"lid got \" + text);\n    }\n}\n";
  const QByteArray BrokenScript = "default\n{\n    state_entry()\n    {\n        llOwnerSay(1, 2);\n    }\n}\n";
}

class TestLabTree : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void words();
    void treeOfTheObject();
    void pickScript();
    void pickWithKeyboard();
    void scriptThatDoesNotCompile();
    void folderLimit();
    void logLineOfAnotherScript();
    void oneScriptFolderKeepsShortLines();
    void exampleScriptsStaySeparate();
    void guide();
    void dotsInBothThemes();
    void tabOrder();
    void propTints();

  private:
    // MyObject/Root/main.lsl, MyObject/Root/settings.txt, MyObject/Lid/lid.lsl
    QString makeObject(const QTemporaryDir& dir, const QByteArray& lid = LidScript)
    {
      const QString top = dir.filePath("MyObject");
      writeFile(top + "/Root/main.lsl", MainScript);
      writeFile(top + "/Root/settings.txt", "colour=red\nsize=2\n");
      writeFile(top + "/Lid/lid.lsl", lid);
      return top;
    }
    QTemporaryDir m_settingsDir;
};

void TestLabTree::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestLabTree::init()
{
  UserSettings::setScriptLabParts(QString());
  UserSettings::setTipsOn(true);
}

// the words of the tree: states, memory and sums
void TestLabTree::words()
{
  QCOMPARE(LabFolderTree::stateWord(LabFolderTree::Ok), QString("OK"));
  QCOMPARE(LabFolderTree::stateWord(LabFolderTree::NotRun), QString("Not run"));
  QCOMPARE(LabFolderTree::stateWord(LabFolderTree::Warning), QString("Warning"));
  QCOMPARE(LabFolderTree::stateWord(LabFolderTree::Stopped), QString("Stopped"));
  QCOMPARE(LabFolderTree::stateWord(LabFolderTree::Error), QString("Error"));
  QCOMPARE(LabFolderTree::stateWord(LabFolderTree::NoScripts), QString("No scripts"));
  QCOMPARE(LabFolderTree::memoryText(0), QString("0.0 KB"));
  QCOMPARE(LabFolderTree::memoryText(4096), QString("4.0 KB"));
  QCOMPARE(LabFolderTree::memoryText(1536), QString("1.5 KB"));
  QCOMPARE(LabFolderTree::memoryText(65536), QString("64.0 KB"));
  QCOMPARE(LabFolderTree::memoryText(-5), QString("0.0 KB"));
  QCOMPARE(LabFolderTree::sumText({}), QString("No scripts"));
  QCOMPARE(LabFolderTree::sumText({LabFolderTree::Ok, LabFolderTree::Ok}), QString("2 OK"));
  QCOMPARE(LabFolderTree::sumText({LabFolderTree::Ok, LabFolderTree::Error}), QString("1 error, 1 OK"));
  QCOMPARE(LabFolderTree::sumText({LabFolderTree::Error, LabFolderTree::Error, LabFolderTree::NotRun, LabFolderTree::Warning,
                                   LabFolderTree::Stopped, LabFolderTree::Stopped}),
           QString("2 errors, 2 stopped, 1 warning, 1 not run"));
  QCOMPARE(LabFolderTree::sumText({LabFolderTree::NotRun, LabFolderTree::NotRun}), QString("2 not run"));
  // the dots are the tokens of docs/design_tokens.json
  for (bool dark : {true, false})
  {
    const QJsonObject t = themeTokens(dark);
    QCOMPARE(LabFolderTree::stateColour(LabFolderTree::Ok, dark).name().toUpper(), t.value("success").toString().toUpper());
    QCOMPARE(LabFolderTree::stateColour(LabFolderTree::Warning, dark).name().toUpper(), t.value("warning").toString().toUpper());
    QCOMPARE(LabFolderTree::stateColour(LabFolderTree::Error, dark).name().toUpper(), t.value("danger").toString().toUpper());
    QCOMPARE(LabFolderTree::stateColour(LabFolderTree::Stopped, dark).name().toUpper(), t.value("danger").toString().toUpper());
    QCOMPARE(LabFolderTree::stateColour(LabFolderTree::NotRun, dark).name().toUpper(), t.value("text_muted").toString().toUpper());
  }
  QVERIFY(LucideIcons::names().contains("box"));
  QVERIFY(LucideIcons::names().contains("square"));
  QVERIFY(LucideIcons::names().contains("file-code"));
  QVERIFY(LucideIcons::names().contains("notebook-text"));
}

// Watch folder: the folder runs as one object and the tree shows it
void TestLabTree::treeOfTheObject()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QVERIFY(panel.section("tree")->isHidden());   // no folder: no tree
  QTemporaryDir dir;
  const QString top = makeObject(dir);
  QVERIFY(panel.watchFolder(top));
  QVERIFY(watcher.linkset());
  QCOMPARE(watcher.runCount(), 1);
  QVERIFY(!panel.section("tree")->isHidden());
  QCOMPARE(panel.section("tree")->title(), QString("Object"));
  QCOMPARE(panel.section("tree")->count(), QString("2 scripts in 2 prims"));

  // the memory each script used, as the run gives it
  QHash<QString, qint64> memory;
  const QJsonArray scripts = watcher.folderResult().value("result").toObject().value("objects").toArray().first().toObject().value("scripts").toArray();
  for (const QJsonValue& v : scripts) memory.insert(v.toObject().value("name").toString(), qint64(v.toObject().value("memory").toDouble()));
  QVERIFY(memory.value("main") > 0);
  QCOMPARE(memory.value("lid"), qint64(0));   // no values kept: the sandbox counts values, not code

  LabFolderTree* tree = panel.folderTree();
  QCOMPARE(tree->headerItem()->text(0), QString("Name"));
  QCOMPARE(tree->headerItem()->text(1), QString("Kind"));
  QCOMPARE(tree->headerItem()->text(2), QString("State"));
  QCOMPARE(tree->headerItem()->text(3), QString("Memory"));
  QCOMPARE(tree->topLevelItemCount(), 1);
  QTreeWidgetItem* object = tree->objectItem();
  QCOMPARE(rowTexts(object), QStringList({"MyObject", "Object, 2 prims", "2 OK", LabFolderTree::memoryText(memory.value("main") + memory.value("lid"))}));
  QCOMPARE(object->childCount(), 2);
  QTreeWidgetItem* root = object->child(0);
  QCOMPARE(rowTexts(root), QStringList({"Root", "Root prim, link 1", "1 OK", LabFolderTree::memoryText(memory.value("main"))}));
  QCOMPARE(root->childCount(), 2);
  QCOMPARE(rowTexts(root->child(0)), QStringList({"main", "LSL script", "OK", LabFolderTree::memoryText(memory.value("main"))}));
  QCOMPARE(rowTexts(root->child(1)), QStringList({"settings", "Notecard, 2 lines", "", ""}));
  QTreeWidgetItem* lid = object->child(1);
  QCOMPARE(rowTexts(lid), QStringList({"Lid", "Prim, link 2", "1 OK", LabFolderTree::memoryText(memory.value("lid"))}));
  QCOMPARE(rowTexts(lid->child(0)), QStringList({"lid", "LSL script", "OK", LabFolderTree::memoryText(memory.value("lid"))}));
  QCOMPARE(lid->child(0)->toolTip(0), QString("Lid/lid.lsl"));
  QCOMPARE(lid->child(0)->toolTip(2), QString("Ran without errors."));
  QCOMPARE(lid->child(0)->toolTip(3), QString("Memory of the script's values at the end of the run, a rough count. Code is not counted. A script may use up to 64 KB."));
  QCOMPARE(tree->scriptState("Lid/lid.lsl"), LabFolderTree::Ok);
  // icons on every row, dots on all but the notecard
  QVERIFY(!object->icon(0).isNull());
  QVERIFY(!root->icon(0).isNull());
  QVERIFY(!root->child(0)->icon(0).isNull());
  QVERIFY(!root->child(1)->icon(0).isNull());
  QVERIFY(!root->child(0)->icon(2).isNull());
  QVERIFY(root->child(1)->icon(2).isNull());

  // the scripts ran together: the link message reached the lid
  const QString log = panel.runLog()->toPlainText();
  QVERIFY2(log.contains("main (Root): owner_say: main ready"), qPrintable(log));
  QVERIFY2(log.contains("lid (Lid): owner_say: lid got hello"), qPrintable(log));
  const int lines = int(watcher.folderResult().value("result").toObject().value("log").toArray().size());
  QCOMPARE(panel.liveStatusText(), QString("OK: the object MyObject ran without errors (run 1), %1 log line(s).").arg(lines));
  QCOMPARE(panel.liveStatusTone(), 1);
}

// a click on a script shows it in the editor; the next run follows the saved script again
void TestLabTree::pickScript()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  const QString top = makeObject(dir);
  QVERIFY(panel.watchFolder(top));
  panel.resize(480, 900);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  const QString first = panel.shownScript();
  QVERIFY(first == "Root/main.lsl" || first == "Lid/lid.lsl");
  const QString other = first == "Lid/lid.lsl" ? "Root/main.lsl" : "Lid/lid.lsl";
  QTreeWidgetItem* row = panel.folderTree()->itemForFile(other);
  QVERIFY(row);
  LabFolderTree* tree = panel.folderTree();
  tree->scrollToItem(row);
  QTest::mouseClick(tree->viewport(), Qt::LeftButton, {}, tree->visualItemRect(row).center());
  QCOMPARE(panel.shownScript(), other);
  QCOMPARE(panel.liveScriptName()->title(), QString("Script: %1").arg(other));
  QCOMPARE(panel.scriptView()->toPlainText(), QString::fromUtf8(other == "Lid/lid.lsl" ? LidScript : MainScript));
  QCOMPARE(tree->pickedScript(), other);
  QCOMPARE(watcher.runCount(), 1);   // picking runs nothing
  // a click on a prim or a notecard picks no script
  QTest::mouseClick(tree->viewport(), Qt::LeftButton, {}, tree->visualItemRect(tree->objectItem()).center());
  QCOMPARE(panel.shownScript(), other);
  QVERIFY(!tree->pickScript("Root/settings.txt"));
  QVERIFY(!panel.showScript("Nowhere/none.lsl"));
  // Run again runs the whole object again, and the picked script stays shown
  QVERIFY(panel.runAgain());
  QCOMPARE(watcher.runCount(), 2);
  QCOMPARE(panel.shownScript(), other);
}

// Enter on a script's row shows it (charter P8)
void TestLabTree::pickWithKeyboard()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(makeObject(dir)));
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  LabFolderTree* tree = panel.folderTree();
  for (const QString& file : {QString("Lid/lid.lsl"), QString("Root/main.lsl")})
  {
    tree->setFocus();
    tree->setCurrentItem(tree->itemForFile(file));
    QTest::keyClick(tree, Qt::Key_Return);
    QCOMPARE(panel.shownScript(), file);
  }
}

// a script that does not compile: its row says Error, the others Not run, and nothing runs
void TestLabTree::scriptThatDoesNotCompile()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  const QString top = makeObject(dir, BrokenScript);
  QVERIFY(panel.watchFolder(top));
  LabFolderTree* tree = panel.folderTree();
  QCOMPARE(tree->scriptState("Lid/lid.lsl"), LabFolderTree::Error);
  QCOMPARE(tree->scriptState("Root/main.lsl"), LabFolderTree::NotRun);
  QCOMPARE(rowTexts(tree->objectItem()), QStringList({"MyObject", "Object, 2 prims", "1 error, 1 not run", ""}));
  QCOMPARE(tree->itemForFile("Lid/lid.lsl")->toolTip(2), QString("1 error. Click to show it in the script."));
  QVERIFY(tree->itemForFile("Root/main.lsl")->toolTip(2).startsWith("Not run: The folder was not run: 1 script(s) do not compile (Lid/lid.lsl)."));
  QCOMPARE(dotColour(tree->objectItem()), Theme::color(Theme::Token::Danger, true));
  QCOMPARE(dotColour(tree->itemForFile("Root/main.lsl")), Theme::color(Theme::Token::TextMuted, true));

  // the broken script shown: its own error
  QVERIFY(panel.showScript("Lid/lid.lsl"));
  QVERIFY(panel.liveStatusText().startsWith("Error: Lid/lid.lsl has 1 error(s) and was not run (run 1)."));
  QCOMPARE(panel.problemList()->count(), 1);
  QVERIFY(panel.problemList()->item(0)->text().startsWith("Error, line 5, column "));
  // the good script shown: the object was not run because of the other one
  QVERIFY(panel.showScript("Root/main.lsl"));
  QCOMPARE(panel.liveStatusText(),
           QString("Error: the object MyObject was not run (run 1): Lid/lid.lsl does not compile. Pick it in the tree to see why."));
  QCOMPARE(panel.liveStatusTone(), 2);
  QCOMPARE(panel.problemList()->count(), 0);
  QCOMPARE(panel.section("log")->count(), QString("not run"));
}

// a limit or rule of the folder: listed under Errors and warnings, nothing runs
void TestLabTree::folderLimit()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  const QString top = makeObject(dir);
  QVERIFY(writeFile(top + "/Lid/main.lsl", MainScript));   // the name "main" twice in one object
  QVERIFY(panel.watchFolder(top));
  QVERIFY(!watcher.folderResult().value("ok").toBool());
  QStringList texts;
  for (int i = 0; i < panel.problemList()->count(); ++i) texts << panel.problemList()->item(i)->text();
  QCOMPARE(texts.size(), 1);
  QVERIFY2(texts.first().startsWith("Folder error, Lid/main.lsl: The name \"main\" is used twice"), qPrintable(texts.first()));
  QCOMPARE(panel.problemList()->item(0)->foreground().color(), Theme::color(Theme::Token::Danger, true));
  QCOMPARE(panel.section("problems")->count(), QString("1 error"));
  QVERIFY(panel.showScript("Root/main.lsl"));
  QCOMPARE(panel.liveStatusText(),
           QString("Error: the object MyObject was not run (run 1): it breaks 1 limit(s) or rule(s), listed under Errors and warnings."));
  QCOMPARE(panel.folderTree()->scriptState("Lid/main.lsl"), LabFolderTree::Error);
  QVERIFY(panel.folderTree()->itemForFile("Lid/main.lsl")->toolTip(2).startsWith("The name \"main\" is used twice"));
  // clicking the folder line moves nothing in the script
  panel.problemList()->setCurrentRow(0);
  emit panel.problemList()->itemActivated(panel.problemList()->item(0));
  QCOMPARE(panel.shownScript(), QString("Root/main.lsl"));
}

// a click on a log line of another script shows that script first
void TestLabTree::logLineOfAnotherScript()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  const QString top = makeObject(dir);
  // the lid sends an RLV line, so its log line points at a code line
  QVERIFY(writeFile(top + "/Lid/lid.lsl",
                    "default\n{\n    link_message(integer sender, integer num, string text, key id)\n    {\n        llOwnerSay(\"@detach=n\");\n    }\n}\n"));
  QVERIFY(panel.watchFolder(top));
  QVERIFY(panel.showScript("Root/main.lsl"));
  const QStringList lines = panel.runLog()->toPlainText().split('\n');
  int rlv = -1;
  for (int i = 0; i < lines.size(); ++i)
    if (lines[i].contains("lid (Lid): rlv_command: @detach=n")) rlv = i;
  QVERIFY2(rlv >= 0, qPrintable(lines.join('\n')));
  QVERIFY(panel.showLogLine(rlv));
  QCOMPARE(panel.shownScript(), QString("Lid/lid.lsl"));
  QCOMPARE(panel.scriptView()->textCursor().blockNumber(), 4);   // line 5 of the lid
}

// a folder of one script: the log lines and the status name the file, as before
void TestLabTree::oneScriptFolderKeepsShortLines()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("hello.lsl"), "default\n{\n    state_entry() { llOwnerSay(\"hi\"); }\n}\n"));
  QVERIFY(panel.watchFolder(dir.path()));
  QCOMPARE(panel.runLog()->toPlainText(), QString("0.000 s  state: default\n0.000 s  event: state_entry()\n0.000 s  owner_say: hi"));
  QCOMPARE(panel.liveStatusText(), QString("OK: hello.lsl ran without errors (run 1), 3 log line(s)."));
  QCOMPARE(panel.section("tree")->count(), QString("1 script in 1 prim"));
  QCOMPARE(panel.folderTree()->objectItem()->child(0)->text(1), QString("Root prim, link 1"));
}

// the example scripts are separate examples: each runs on its own, no tree
void TestLabTree::exampleScriptsStaySeparate()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.setExampleScriptsFolder(QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/scriptlab"));
  QVERIFY(panel.tryExampleScripts());
  QVERIFY(!watcher.linkset());
  QVERIFY(panel.section("tree")->isHidden());
  QVERIFY(!panel.section("guide"));   // the maker guide is in the help view now (UX small fixes)
  QVERIFY(panel.liveStatusText().startsWith("OK: rlv_lock.lsl ran without errors"));
  QCOMPARE(panel.folderTree()->topLevelItemCount(), 0);
  // Watch folder afterwards switches to the folder as one object
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(makeObject(dir)));
  QVERIFY(watcher.linkset());
  QVERIFY(!panel.section("tree")->isHidden());
  panel.stopWatching();
  QVERIFY(panel.section("tree")->isHidden());
  QCOMPARE(panel.folderTree()->topLevelItemCount(), 0);
}

// the maker guide moved out of the Live scripts page to the help view behind the ? button (UX small
// fixes, the project owner, 24 Nov 2026: "How to lay out this folder" looked like a function)
void TestLabTree::guide()
{
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.setExampleObjectsFolder(QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/objects"));
  // no folding part for it on the page any more, before or while a folder is watched
  QVERIFY(!panel.section("guide"));
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(makeObject(dir)));
  QVERIFY(!panel.section("guide"));
  // the ? button opens the help view; its topic "How to lay out a folder" has the guide
  QVERIFY(panel.helpButton());
  QCOMPARE(panel.helpButton()->accessibleName(), QString("Help for Script Lab"));
  QTest::mouseClick(panel.helpButton(), Qt::LeftButton);
  LabHelpView* help = panel.helpView();
  QVERIFY(help);
  QVERIFY(help->isVisible());
  QCOMPARE(help->windowTitle(), QString("Script Lab help"));
  QVERIFY(help->showTopic("folder"));
  QCOMPARE(help->topicHeading(), QString("How to lay out a folder"));
  const QString text = help->topicText();
  for (const QString& line : {QString("Rules"),
                              QString("One folder is one object. Each subfolder is one prim."),
                              QString("Files in the top folder or in Root form the root prim."),
                              QString("Put scripts (.lsl) and notecards (.txt) in their prim's folder."),
                              QString("Give every script and notecard its own name."),
                              QString("Warnings"),
                              QString("Copy scripts from Second Life by hand: select all, copy, paste."),
                              QString("The sandbox is never the same as Second Life. Test there last."),
                              QString("MyObject/        one object"),
                              QString("  Root/          the root prim")})
    QVERIFY2(text.contains(line), qPrintable(line + "\n---\n" + text));
  // the warnings heading has the warning colour; the example is in a fixed-width block
  const QString html = help->textView()->toHtml();
  QVERIFY(html.contains(Theme::color(Theme::Token::Warning, ScriptLabPanel::isDark(panel.palette())).name(), Qt::CaseInsensitive));
  for (const QString& line : text.split('\n'))
    for (const QString& sentence : line.split(". "))
      if (!line.startsWith(' ') && !line.contains('/'))
        QVERIFY2(sentence.split(' ', Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
}

// the dots follow the theme
void TestLabTree::dotsInBothThemes()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(panel.watchFolder(makeObject(dir)));
  for (bool dark : {true, false})
  {
    panel.folderTree()->applyStyle(dark);
    QCOMPARE(dotColour(panel.folderTree()->itemForFile("Root/main.lsl")), Theme::color(Theme::Token::Success, dark));
    QCOMPARE(dotColour(panel.folderTree()->objectItem()), Theme::color(Theme::Token::Success, dark));
    QCOMPARE(panel.folderTree()->itemForFile("Root/main.lsl")->foreground(2).color(), Theme::color(Theme::Token::Text, dark));
  }
}

// keyboard: after Show effects comes the Object part, its tree, then the Script part
void TestLabTree::tabOrder()
{
  ScriptLabPanel panel;
  const auto next = [](QWidget* w) -> QWidget*
  {
    QWidget* n = w->nextInFocusChain();
    while (n && n != w && !(n->focusPolicy() & Qt::TabFocus)) n = n->nextInFocusChain();
    return n;
  };
  QCOMPARE(next(panel.effectsBox()), static_cast<QWidget*>(panel.section("tree")->header()));
  // job U3: the switch that links the tree with the 3D view comes before the tree
  QCOMPARE(next(panel.section("tree")->header()), static_cast<QWidget*>(panel.followSelectionBox()));
  QCOMPARE(next(panel.followSelectionBox()), static_cast<QWidget*>(panel.folderTree()));
  QCOMPARE(next(panel.folderTree()), static_cast<QWidget*>(panel.section("script")->header()));
}

// props as prims (job E): the tints follow the "Show effects in 3D view" box and the folder runs
void TestLabTree::propTints()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QSignalSpy changed(&panel, &ScriptLabPanel::propTintsChanged);
  QTemporaryDir dir;
  const QString top = makeObject(dir);
  QVERIFY(writeFile(top + "/Lid/lid.lsl", "default { state_entry() { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); llSetAlpha(0.5, ALL_SIDES); } }\n"));
  QVERIFY(panel.watchFolder(top));
  QVERIFY(panel.propTints().isEmpty());   // the box is off at start
  QCOMPARE(changed.count(), 0);
  panel.effectsBox()->setChecked(true);
  QCOMPARE(changed.count(), 1);
  QCOMPARE(panel.propTints().size(), 1);
  const LabTint::Tint tint = panel.propTints().first();
  QCOMPARE(tint.prim, QString("Lid"));
  QCOMPARE(tint.link, 2);
  QVERIFY(tint.hasColour);
  QCOMPARE(tint.colour[1], 1.0f);
  QCOMPARE(tint.alpha, 0.5f);
  // the same run again: nothing changed, no signal
  QVERIFY(panel.runAgain());
  QCOMPARE(changed.count(), 1);
  // the lid no longer colours itself: the tint goes
  QVERIFY(writeFile(top + "/Lid/lid.lsl", LidScript));
  watcher.runScript("Lid/lid.lsl");
  QVERIFY(panel.propTints().isEmpty());
  QCOMPARE(changed.count(), 2);
  // unticked or stopped: no tints
  QVERIFY(writeFile(top + "/Lid/lid.lsl", "default { state_entry() { llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES); } }\n"));
  watcher.runScript("Lid/lid.lsl");
  QCOMPARE(panel.propTints().size(), 1);
  panel.effectsBox()->setChecked(false);
  QVERIFY(panel.propTints().isEmpty());
  panel.effectsBox()->setChecked(true);
  QCOMPARE(panel.propTints().size(), 1);
  panel.stopWatching();
  QVERIFY(panel.propTints().isEmpty());
}

QTEST_MAIN(TestLabTree)
#include "tst_labtree.moc"
