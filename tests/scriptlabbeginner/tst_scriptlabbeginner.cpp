/*
 * PinglyMotion - Script Lab for beginners (job U3, docs/UX_DECISIONS.md 6o)
 *
 * Exact answers for the behaviour: the folder names and limits (nothing is
 * ever overwritten or deleted), copying the first example, "Make example
 * project" (props in the scene by name, the folder watched, the effects on,
 * the steps shown and ticked), the step buttons, a step that does not work
 * yet, "New folder", selection that follows between the tree and the 3D view
 * both ways and its switch, the tree beside the editor when wide, the word
 * tips (one at a time, never again once closed, none with hints off), the
 * effects button, open and folded parts, "unsaved changes" at Save, the reason
 * line under Run again, the contrast of grey and greyed text, and every new
 * text in the register (docs/TEACHING.md) with at most 12 words a sentence.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QAction>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QMap>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QListWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTemporaryDir>
#include <QToolButton>
#include <QTreeWidget>

#include <cmath>

#include "propmanager.h"
#include "scriptlab/channelmapview.h"
#include "scriptlab/labbeginner.h"
#include "scriptlab/labdialogbox.h"
#include "scriptlab/labeditor.h"
#include "scriptlab/labhelpview.h"
#include "scriptlab/labtree.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/scriptlabsection.h"
#include "theme.h"
#include "usersettings.h"
#include "../windowsource.h"

namespace
{
  QString examplesDir() { return QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/objects"); }

  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  bool writeFile(const QString& path, const QByteArray& data)
  {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    return file.write(data) == data.size();
  }

  // WCAG 2.2 contrast of two colours
  double luminance(const QColor& c)
  {
    const auto channel = [](double v) { v /= 255.0; return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel(c.red()) + 0.7152 * channel(c.green()) + 0.0722 * channel(c.blue());
  }
  double contrast(const QColor& a, const QColor& b)
  {
    const double la = luminance(a), lb = luminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
  }

  void verifyShortSentences(const QString& text)
  {
    for (const QString& line : text.split('\n'))
      for (const QString& sentence : line.split(QRegularExpression("[.!?:](\\s|$)"), Qt::SkipEmptyParts))
        QVERIFY2(sentence.split(' ', Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
  }

  // a scene the tests can see: the props added, the prop lit up, the Props tab shown
  struct FakeScene
  {
    QStringList props;
    QList<LabExample::Prop> added;
    QStringList selected;
    int propsTabShown = 0;
    QMap<QString, LabExample::Prop> places;   // where each prop stands (job A1)
    ScriptLabPanel::SceneHooks hooks()
    {
      ScriptLabPanel::SceneHooks h;
      h.addProp = [this](const LabExample::Prop& p) { if (!props.contains(p.name)) { props << p.name; added << p; places.insert(p.name, p); } return true; };
      h.propPlace = [this](const QString& name, LabExample::Prop& out) { if (!places.contains(name)) return false; out = places.value(name); return true; };
      h.placeProp = [this](const LabExample::Prop& p) { if (!places.contains(p.name)) return false; places[p.name] = p; return true; };
      h.hasProp = [this](const QString& name) { return props.contains(name); };
      h.selectProp = [this](const QString& name) { if (!props.contains(name)) return false; selected << name; return true; };
      h.showPropsTab = [this]() { ++propsTabShown; };
      return h;
    }
  };
}

class TestScriptLabBeginner : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void folderNames();
    void makeFolderOnlyAdds();
    void readFirstExample();
    void copyExampleNeverOverwrites();
    void emptyStateTexts();
    void makeExampleProject();
    void makeExampleRefused();
    void stepButtons();
    void stepThatDoesNotWorkYet();
    void stepsFromHelpMenu();
    void newFolder();
    void selectionFollowsBothWays();
    void treeBesideEditorWhenWide();
    void jargonTips();
    void effectsButton();
    void openAndFoldedParts();
    void unsavedAtSave();
    void reasonUnderRunAgain();
    void contrastOfGreyText();
    void propNamesForExamples();
    void textsInRegister();
    void windowWiring();
    void propsFromPrims();
    void menuBoxAtTheClick();
    void recordAndReplay();
    void menuBoxTextBox();
    void channelMap();
    void chainLines();
    void showLiveScriptsShowsSomething();
    void activeChoiceIsClear();
    void helpViewOutOfThePanel();
    void guideStepsNameRealControls();

  private:
    void setUpPanel(ScriptLabPanel& panel, LabWatcher& watcher, RlvState& state, const QString& root, FakeScene& scene);
    QTemporaryDir m_settingsDir;
};

void TestScriptLabBeginner::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestScriptLabBeginner::init()
{
  UserSettings::setTipsOn(true);
  UserSettings::setScriptLabTipsClosed(QString());
  UserSettings::setScriptLabFollowSelection(true);
  UserSettings::setScriptLabParts(QString());
}

void TestScriptLabBeginner::setUpPanel(ScriptLabPanel& panel, LabWatcher& watcher, RlvState& state, const QString& root, FakeScene& scene)
{
  watcher.setRlvState(&state);
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.setExampleObjectsFolder(examplesDir());
  panel.setScriptsRoot(root);
  panel.setSceneHooks(scene.hooks());
}

// the limits on a name: exact reasons in plain words
void TestScriptLabBeginner::folderNames()
{
  QCOMPARE(LabBeginner::checkFolderName("My object"), QString());
  QCOMPARE(LabBeginner::checkFolderName("Cuff_set-2"), QString());
  QCOMPARE(LabBeginner::checkFolderName(""), QString("Type a name for the folder first."));
  QCOMPARE(LabBeginner::checkFolderName("   "), QString("Type a name for the folder first."));
  QCOMPARE(LabBeginner::checkFolderName(QString(41, 'a')), QString("The name is too long. Use at most 40 characters."));
  QCOMPARE(LabBeginner::checkFolderName(QString(40, 'a')), QString());
  for (const char* bad : {"../up", "a/b", "a\\b", ".hidden", "C:", "name.", "tab\there", " lead", "trail ", "æøå"})
    QCOMPARE(LabBeginner::checkFolderName(QString::fromUtf8(bad)), QString("Use only letters, digits, spaces, - and _ in the name."));
  for (const char* reserved : {"CON", "con", "NUL", "COM1", "LPT9", "aux"})
    QCOMPARE(LabBeginner::checkFolderName(reserved), QString("Windows keeps this name for itself. Pick another name."));
  QCOMPARE(LabBeginner::MaxNameLength, 40);
  QCOMPARE(LabBeginner::MaxPathLength, 200);
  QVERIFY(LabBeginner::defaultScriptsRoot().endsWith("/PinglyMotion/Scripts"));
}

// a new folder is made only where nothing is; an existing one is never touched
void TestScriptLabBeginner::makeFolderOnlyAdds()
{
  QTemporaryDir dir;
  const QString root = dir.path() + "/Scripts";   // made when missing
  QString error;
  const QString made = LabBeginner::makeFolder(root, "My object", &error);
  QCOMPARE(error, QString());
  QCOMPARE(made, QDir::cleanPath(root + "/My object"));
  QVERIFY(QFileInfo(made).isDir());
  QVERIFY(writeFile(made + "/mine.lsl", "default {}\n"));
  QCOMPARE(LabBeginner::makeFolder(root, "My object", &error), QString());
  QCOMPARE(error, QString("A folder with this name is already there. Pick another name."));
  QCOMPARE(readFile(made + "/mine.lsl"), QString("default {}\n"));   // still there, unchanged
  QCOMPARE(LabBeginner::makeFolder(root, "../Outside", &error), QString());
  QCOMPARE(error, QString("Use only letters, digits, spaces, - and _ in the name."));
  QVERIFY(!QFileInfo::exists(dir.path() + "/Outside"));
  // a path over 200 characters is refused
  const QString deep = dir.path() + "/" + QString(qMax(1, 190 - int(dir.path().size())), 'd');
  QCOMPARE(LabBeginner::makeFolder(deep, "Long name here", &error), QString());
  QCOMPARE(error, QString("The folder path would be too long. Use a shorter name."));
}

// the first example is First colour, with its prop and five steps (job E1's data)
void TestScriptLabBeginner::readFirstExample()
{
  QString error;
  const LabExample example = LabBeginner::firstExample(examplesDir(), &error);
  QCOMPARE(error, QString());
  QCOMPARE(example.object, QString("First_colour"));
  QCOMPARE(example.title, QString("First colour"));
  QCOMPARE(example.order, 0);
  QVERIFY(example.worksNow);
  QCOMPARE(example.props.size(), 1);
  QCOMPARE(example.props.first().name, QString("Box 0"));
  QCOMPARE(example.props.first().shape, QString("box"));
  QCOMPARE(example.props.first().joint, QString());
  QCOMPARE(example.props.first().position, QVector3D(10, 40, 10));
  QCOMPARE(example.props.first().scale, QVector3D(10, 10, 10));
  QCOMPARE(example.steps().size(), 5);
  QStringList controls;
  for (const LabExample::Entry& e : example.steps()) controls << e.control;
  QCOMPARE(controls, QStringList({"Props", "Props", "Live scripts", "Watch folder...", "Show effects in 3D view"}));
  QCOMPARE(example.texts("see"), QStringList({"The box turns red. The log says: Box 0 is red now."}));
  QTemporaryDir empty;
  QVERIFY(!LabBeginner::firstExample(empty.path(), &error).isValid());
  QCOMPARE(error, QString("The example objects are not next to the program."));
}

// the copy holds the scripts and example.json; a second copy opens the first as it is
void TestScriptLabBeginner::copyExampleNeverOverwrites()
{
  QTemporaryDir dir;
  const LabExample example = LabBeginner::firstExample(examplesDir());
  QString error;
  const QString copy = LabBeginner::copyExample(example, dir.path(), &error);
  QCOMPARE(error, QString());
  QCOMPARE(copy, QDir::cleanPath(dir.path() + "/First_colour"));
  QCOMPARE(readFile(copy + "/Box 0/colour.lsl"), readFile(examplesDir() + "/First_colour/Box 0/colour.lsl"));
  QVERIFY(QFileInfo::exists(copy + "/example.json"));
  QVERIFY(writeFile(copy + "/Box 0/colour.lsl", "// my change\n"));
  QCOMPARE(LabBeginner::copyExample(example, dir.path(), &error), copy);
  QCOMPARE(readFile(copy + "/Box 0/colour.lsl"), QString("// my change\n"));   // kept, never overwritten
  // a file (not a folder) in the way: refused, nothing changed
  QTemporaryDir other;
  QVERIFY(writeFile(other.path() + "/First_colour", "x"));
  QCOMPARE(LabBeginner::copyExample(example, other.path(), &error), QString());
  QCOMPARE(error, QString("A folder with this name is already there. Pick another name."));
  QCOMPARE(readFile(other.path() + "/First_colour"), QString("x"));
}

// the empty state's words and buttons
void TestScriptLabBeginner::emptyStateTexts()
{
  ScriptLabPanel panel;
  QCOMPARE(panel.liveEmptyText()->text(), QString("Script Lab shows your scripts from a folder on your PC. Choose a folder to see them here."));
  QCOMPARE(panel.makeExampleButton()->text(), QString("Make example project"));
  QCOMPARE(panel.makeExampleButton()->toolTip(), QString("Copies a working example to your folder and shows it in 3D."));
  QCOMPARE(panel.newFolderButton()->text(), QString("New folder"));
  QVERIFY(panel.newFolderButton()->toolTip().startsWith("Makes an empty folder for your scripts and watches it. "));
  QCOMPARE(panel.treeLine()->text(), QString("This tree is your object. Each folder in it is one prim."));
  QCOMPARE(panel.followSelectionBox()->text(), QString("Follow selection in 3D view"));
  // the order on screen and with Tab: Make example project first
  QWidget* card = panel.liveEmpty();
  QVERIFY(panel.makeExampleButton()->mapTo(card, QPoint(0, 0)).y() <= panel.watchButton()->mapTo(card, QPoint(0, 0)).y());
  QWidget* next = panel.makeExampleButton()->nextInFocusChain();
  while (next && !(next->focusPolicy() & Qt::TabFocus)) next = next->nextInFocusChain();
  QCOMPARE(next, static_cast<QWidget*>(panel.watchButton()));
}

// one click: the copy, the props by name, the watch, the effects, the Live page and the steps, all ticked
void TestScriptLabBeginner::makeExampleProject()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  QVERIFY(!panel.stepCard()->isVisibleTo(&panel));
  QTest::mouseClick(panel.makeExampleButton(), Qt::LeftButton);
  const QString project = QDir::cleanPath(dir.path() + "/First_colour");
  QCOMPARE(panel.projectFolder(), project);
  QVERIFY(watcher.watching());
  QVERIFY(watcher.linkset());
  QCOMPARE(QDir::cleanPath(watcher.folder()), project);
  QCOMPARE(scene.propsTabShown, 1);
  QCOMPARE(scene.added.size(), 1);
  QCOMPARE(scene.added.first().name, QString("Box 0"));
  QCOMPARE(scene.added.first().shape, QString("box"));
  QCOMPARE(scene.added.first().position, QVector3D(10, 40, 10));
  QVERIFY(panel.effectsBox()->isChecked());
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));
  // the prop of the same name gets the script's red
  QCOMPARE(panel.propTints().size(), 1);
  QCOMPARE(panel.propTints().first().prim, QString("Box 0"));
  QVERIFY(panel.propTints().first().hasColour);
  QCOMPARE(panel.propTints().first().colour[0], 1.0f);
  QCOMPARE(panel.propTints().first().colour[1], 0.0f);
  QCOMPARE(panel.liveStatusTone(), 1);
  // the steps: shown, what you should see first, every step ticked
  LabStepCard* card = panel.stepCard();
  QVERIFY(card->isVisibleTo(&panel));
  QCOMPARE(card->titleLabel()->text(), QString("Steps: First colour"));
  QCOMPARE(card->seeLabel()->text(), QString("The box turns red. The log says: Box 0 is red now."));
  QCOMPARE(card->nextLabel()->text(), QString("Open colour.lsl. Change <1.0, 0.0, 0.0> to <0.0, 1.0, 0.0> and save. "
                                              "The box turns green. The three numbers are red, green and blue."));
  QCOMPARE(card->countLabel()->text(), QString("5 of 5 done"));
  QCOMPARE(card->doneCount(), 5);
  for (QLabel* tick : card->ticks())
  {
    QVERIFY(tick->property("done").toBool());
    QCOMPARE(tick->accessibleName(), QString("Done"));
  }
  QVERIFY(card->mapTo(&panel, QPoint(0, 0)).y() < panel.liveStatusFrame()->mapTo(&panel, QPoint(0, 0)).y());   // on top
  // a second press opens the same project again: no second prop, no second copy
  panel.stopWatching();
  QVERIFY(panel.makeExampleProject());
  QCOMPARE(scene.added.size(), 1);
  QCOMPARE(QDir(dir.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot), QStringList({"First_colour"}));
}

// no examples next to the program: a reason in the status line, nothing made
void TestScriptLabBeginner::makeExampleRefused()
{
  QTemporaryDir dir;
  QTemporaryDir empty;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  panel.setExampleObjectsFolder(empty.path());
  QVERIFY(!panel.makeExampleProject());
  QCOMPARE(panel.liveStatusText(), QString("Error: The example objects are not next to the program."));
  QVERIFY(!panel.liveStatusFrame()->isHidden());
  QVERIFY(!watcher.watching());
  QVERIFY(scene.added.isEmpty());
  QVERIFY(QDir(dir.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty());
}

// each step's button does that step, and its tick follows
void TestScriptLabBeginner::stepButtons()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  panel.showSteps();
  LabStepCard* card = panel.stepCard();
  QVERIFY(card->isVisibleTo(&panel));
  QCOMPARE(card->countLabel()->text(), QString("0 of 5 done"));
  QStringList buttons;
  for (QPushButton* b : card->stepButtons()) { buttons << b->text(); QVERIFY(b->isEnabled()); }
  QCOMPARE(buttons, QStringList({"Show Props", "Add the props", "Show Live scripts", "Watch the folder", "Show effects"}));
  QCOMPARE(card->stepTexts().first()->text(), QString("1. Start with no props. Pick Props in the Joints panel."));
  QCOMPARE(card->ticks().first()->accessibleName(), QString("Not done"));
  QTest::mouseClick(card->stepButtons().at(0), Qt::LeftButton);
  QCOMPARE(scene.propsTabShown, 1);
  QCOMPARE(card->countLabel()->text(), QString("1 of 5 done"));
  QTest::mouseClick(card->stepButtons().at(1), Qt::LeftButton);
  QCOMPARE(scene.props, QStringList({"Box 0"}));
  QCOMPARE(card->countLabel()->text(), QString("2 of 5 done"));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  QTest::mouseClick(card->stepButtons().at(2), Qt::LeftButton);
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));
  QCOMPARE(card->countLabel()->text(), QString("3 of 5 done"));
  // Watch the folder: no project yet, so it is made (and watched)
  QTest::mouseClick(card->stepButtons().at(3), Qt::LeftButton);
  QVERIFY(watcher.watching());
  QCOMPARE(QDir::cleanPath(watcher.folder()), QDir::cleanPath(dir.path() + "/First_colour"));
  QTest::mouseClick(card->stepButtons().at(4), Qt::LeftButton);
  QVERIFY(panel.effectsBox()->isChecked());
  QCOMPARE(card->countLabel()->text(), QString("5 of 5 done"));
  // the tick follows the state: effects off again takes the last tick away
  panel.effectsBox()->setChecked(false);
  QCOMPARE(card->countLabel()->text(), QString("4 of 5 done"));
  QVERIFY(!card->ticks().at(4)->property("done").toBool());
  // close (also Esc on the card), and nothing else changes
  QTest::mouseClick(card->closeButton(), Qt::LeftButton);
  QVERIFY(!card->isVisibleTo(&panel));
  QVERIFY(!panel.stepsShown());
  QVERIFY(watcher.watching());
}

// a step the program cannot do yet: greyed, with the reason as a line and as the tip
void TestScriptLabBeginner::stepThatDoesNotWorkYet()
{
  QTemporaryDir objects;
  QDir().mkpath(objects.path() + "/Gear_set");
  QVERIFY(QFile::copy(examplesDir() + "/Gear_set/example.json", objects.path() + "/Gear_set/example.json"));
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  panel.setExampleObjectsFolder(objects.path());
  panel.showSteps();
  LabStepCard* card = panel.stepCard();
  QStringList buttons;
  for (QPushButton* b : card->stepButtons()) buttons << b->text();
  QCOMPARE(buttons, QStringList({"Watch the folder", "Show effects", "Touch", "Lock all", "Free all"}));
  for (int i = 2; i < 5; ++i)
  {
    QVERIFY(!card->stepButtons().at(i)->isEnabled());
    QCOMPARE(card->stepButtons().at(i)->toolTip(), QString("This step does not work in the program yet."));
    QCOMPARE(card->reasons().at(i)->text(), QString("This step does not work in the program yet."));
    QVERIFY(!card->reasons().at(i)->isHidden());
    QVERIFY(!panel.doStep(i));
  }
  QVERIFY(card->reasons().at(0)->isHidden());
}

// Help > Script Lab steps opens the steps again
void TestScriptLabBeginner::stepsFromHelpMenu()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QVERIFY(panel.makeExampleProject());
  panel.closeSteps();
  QVERIFY(!panel.stepCard()->isVisibleTo(&panel));
  QObject owner;
  QAction* action = panel.makeStepsAction(&owner);
  QCOMPARE(action->text(), QString("Script Lab steps"));
  QCOMPARE(action->objectName(), QString("helpScriptLabStepsAction"));
  QCOMPARE(action->toolTip(), QString("Show the steps of the example project again."));
  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  action->trigger();
  QVERIFY(panel.stepCard()->isVisibleTo(&panel));
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));
  QCOMPARE(panel.stepCard()->countLabel()->text(), QString("5 of 5 done"));
  // Esc on the card closes it
  QTest::keyClick(panel.stepCard(), Qt::Key_Escape);
  QVERIFY(!panel.stepsShown());
}

// New folder: made under the scripts folder and watched; a used or bad name is refused with its reason
void TestScriptLabBeginner::newFolder()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path() + "/Scripts", scene);
  QVERIFY(panel.makeNewFolder("My object"));
  QVERIFY(watcher.watching());
  QCOMPARE(QDir::cleanPath(watcher.folder()), QDir::cleanPath(dir.path() + "/Scripts/My object"));
  panel.stopWatching();
  QVERIFY(!panel.makeNewFolder("My object"));
  QCOMPARE(panel.liveStatusText(), QString("Error: A folder with this name is already there. Pick another name."));
  QVERIFY(!watcher.watching());
  QVERIFY(!panel.makeNewFolder("a/b"));
  QCOMPARE(panel.liveStatusText(), QString("Error: Use only letters, digits, spaces, - and _ in the name."));
}

// a click on a piece in the tree lights up its prop; a click on the prop picks its row; the switch turns both off
void TestScriptLabBeginner::selectionFollowsBothWays()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QVERIFY(panel.followSelectionBox()->isChecked());   // on at start
  QVERIFY(panel.makeExampleProject());
  LabFolderTree* tree = panel.folderTree();
  // the tree is built again after every run, so each row is looked up when it is used
  const auto prim = [tree]() { return tree->itemForPrim("Box 0"); };
  QVERIFY(prim());
  QCOMPARE(prim()->text(LabFolderTree::NameColumn), QString("Box 0"));
  emit tree->itemClicked(prim(), 0);
  QCOMPARE(scene.selected, QStringList({"Box 0"}));
  // a script in the prim names the same prop
  QTreeWidgetItem* script = tree->itemForFile("Box 0/colour.lsl");
  QVERIFY(script);
  QCOMPARE(LabFolderTree::primOf(script), QString("Box 0"));
  emit tree->itemClicked(script, 0);
  QCOMPARE(scene.selected, QStringList({"Box 0", "Box 0"}));
  // the object row names no prop
  emit tree->itemClicked(tree->objectItem(), 0);
  QCOMPARE(scene.selected.size(), 2);
  // from the 3D view: the row is picked, nothing is sent back
  tree->setCurrentItem(tree->objectItem());
  QVERIFY(panel.selectPrim("Box 0"));
  QCOMPARE(tree->currentItem(), prim());
  QCOMPARE(tree->pickedPrim(), QString("Box 0"));
  QCOMPARE(scene.selected.size(), 2);
  QVERIFY(!panel.selectPrim("No such prop"));
  // the pick holds through the next run
  watcher.runScript("Box 0/colour.lsl");
  QCOMPARE(tree->pickedPrim(), QString("Box 0"));
  // switched off: neither way, and remembered
  panel.followSelectionBox()->setChecked(false);
  QVERIFY(!UserSettings::scriptLabFollowSelection());
  emit tree->itemClicked(prim(), 0);
  QCOMPARE(scene.selected.size(), 2);
  tree->setCurrentItem(tree->objectItem());
  QVERIFY(!panel.selectPrim("Box 0"));
  QCOMPARE(tree->currentItem(), tree->objectItem());
  ScriptLabPanel next;
  QVERIFY(!next.followSelectionBox()->isChecked());
}

// wide: the tree on the left and the editor on the right; narrow: under each other
void TestScriptLabBeginner::treeBesideEditorWhenWide()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QVERIFY(panel.makeExampleProject());
  QCOMPARE(ScriptLabPanel::sideBySideWidth(), 760);
  panel.resize(1000, 800);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  QCoreApplication::processEvents();
  QVERIFY(panel.sideBySide());
  QWidget* tree = panel.treeColumn();
  QWidget* editor = panel.editorColumn();
  QVERIFY(panel.section("tree")->isVisibleTo(tree));
  QVERIFY(panel.section("script")->isVisibleTo(editor));
  QVERIFY(editor->mapTo(&panel, QPoint(0, 0)).x() >= tree->mapTo(&panel, QPoint(0, 0)).x() + tree->width());
  QCOMPARE(editor->mapTo(&panel, QPoint(0, 0)).y(), tree->mapTo(&panel, QPoint(0, 0)).y());
  // a picture for a session to look at (docs/DEV.md, "How a session tests a visible change itself")
  if (qEnvironmentVariableIsSet("PINGLYMOTION_UI_OUT"))
    panel.grab().save(qEnvironmentVariable("PINGLYMOTION_UI_OUT") + "/scriptlab_beginner_wide.png");
  panel.resize(480, 800);
  QCoreApplication::processEvents();
  QVERIFY(!panel.sideBySide());
  QVERIFY(editor->mapTo(&panel, QPoint(0, 0)).y() >= tree->mapTo(&panel, QPoint(0, 0)).y() + tree->height());
  if (qEnvironmentVariableIsSet("PINGLYMOTION_UI_OUT"))
    panel.grab().save(qEnvironmentVariable("PINGLYMOTION_UI_OUT") + "/scriptlab_beginner_narrow.png");
  QCOMPARE(editor->mapTo(&panel, QPoint(0, 0)).x(), tree->mapTo(&panel, QPoint(0, 0)).x());
  panel.resize(759, 800);
  QCoreApplication::processEvents();
  QVERIFY(!panel.sideBySide());
  panel.resize(760, 800);
  QCoreApplication::processEvents();
  QVERIFY(panel.sideBySide());
}

// the word tips: none before something is watched, one at a time, closed for good, none with hints off
void TestScriptLabBeginner::jargonTips()
{
  const QList<LabBeginner::Tip> tips = LabBeginner::jargonTips();
  QStringList keys, words;
  for (const LabBeginner::Tip& t : tips) { keys << QLatin1String(t.key); words << t.word; verifyShortSentences(t.text); }
  QCOMPARE(keys, QStringList({"prim", "rootPrim", "notecard", "sandbox", "viewerRules", "rlv", "rlva", "held", "channel", "listener"}));
  QCOMPARE(words, QStringList({"Prim", "Root prim", "Notecard", "Sandbox", "Viewer rules", "RLV", "RLVa", "N held", "Channel", "Listener"}));
  QCOMPARE(LabBeginner::jargonTip("prim").text, QString("One part of an object. Each folder is one prim."));
  QCOMPARE(LabBeginner::jargonTip("held").text, QString("How many RLV rules hold the avatar right now."));

  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  panel.setHintsOn(true);
  QCOMPARE(panel.tipCards().size(), 10);
  QVERIFY(!panel.shownTip());   // value first: nothing explained before something is watched
  QVERIFY(panel.makeExampleProject());
  QVERIFY(panel.shownTip());
  QCOMPARE(panel.shownTip()->key(), QString("prim"));
  QCOMPARE(panel.shownTip()->wordLabel()->text(), QString("Prim"));
  QCOMPARE(panel.shownTip()->textLabel()->text(), QString("One part of an object. Each folder is one prim."));
  QCOMPARE(panel.shownTip()->closeButton()->toolTip(), QString("Close this tip. It does not come back."));
  int shown = 0;
  for (LabTipCard* card : panel.tipCards()) if (!card->isHidden()) ++shown;
  QCOMPARE(shown, 1);
  QVERIFY(panel.section("tree")->isAncestorOf(panel.shownTip()));   // by its word
  QTest::mouseClick(panel.shownTip()->closeButton(), Qt::LeftButton);
  QCOMPARE(UserSettings::scriptLabTipsClosed(), QString("prim"));
  QCOMPARE(panel.shownTip()->key(), QString("rootPrim"));
  // First colour has no notecard, so the notecard tip waits; the next is the sandbox, under the explanation
  QTest::keyClick(panel.shownTip(), Qt::Key_Escape);
  QCOMPARE(panel.shownTip()->key(), QString("sandbox"));
  QVERIFY(!panel.section("tree")->isAncestorOf(panel.shownTip()));
  // hints off: none; on again: the same one
  panel.setHintsOn(false);
  QVERIFY(!panel.shownTip());
  panel.setHintsOn(true);
  QCOMPARE(panel.shownTip()->key(), QString("sandbox"));
  // a closed tip never comes back, also in a new panel
  ScriptLabPanel again;
  FakeScene scene2;
  LabWatcher watcher2;
  setUpPanel(again, watcher2, state, dir.path(), scene2);
  QVERIFY(again.makeExampleProject());
  QCOMPARE(again.shownTip()->key(), QString("sandbox"));
  // the Viewer rules word keeps its meaning as a tip on hover
  QCOMPARE(panel.modeLabel()->toolTip(), QString("Which kind of RLV the test viewer follows."));
}

// Show effects in 3D view is a clear on/off button: the selection fill and a check mark when on
void TestScriptLabBeginner::effectsButton()
{
  for (bool dark : {true, false})
  {
    Theme::apply(dark);
    ScriptLabPanel panel;
    QPushButton* button = panel.effectsBox();
    QCOMPARE(button->text(), QString("Show effects in 3D view"));
    QVERIFY(button->isCheckable());
    QVERIFY(!button->isChecked());
    const QString selection = Theme::color(Theme::Token::Selection, dark).name(QColor::HexRgb).toUpper();
    QVERIFY(button->styleSheet().contains("QPushButton:checked { background: " + selection + ";"));
    QVERIFY(button->styleSheet().contains("background: " + Theme::color(Theme::Token::SurfaceRaised, dark).name(QColor::HexRgb).toUpper() + ";"));
    const QImage off = button->icon().pixmap(16, 16).toImage();
    button->setChecked(true);
    QVERIFY(button->icon().pixmap(16, 16).toImage() != off);   // eye when off, a check mark when on
    QCOMPARE(button->accessibleDescription(), QString("On: the 3D view shows what the scripts do."));
    button->setChecked(false);
    QCOMPARE(button->accessibleDescription(), QString("Off."));
  }
  Theme::apply(true);
}

// a folded part is filled like a button, an open part is not (besides the chevron)
void TestScriptLabBeginner::openAndFoldedParts()
{
  for (bool dark : {true, false})
  {
    Theme::apply(dark);
    ScriptLabPanel panel;
    ScriptLabSection* guide = panel.section("input");   // Send input is folded at first
    QVERIFY(!guide->isExpanded());
    QCOMPARE(guide->fillColour(), Theme::color(Theme::Token::SurfaceRaised, dark));
    guide->setExpanded(true);
    QVERIFY(!guide->fillColour().isValid());
    guide->setExpanded(false);
    QCOMPARE(guide->fillColour(), Theme::color(Theme::Token::SurfaceRaised, dark));
    // the border stays the same either way
    QCOMPARE(guide->borderColour(), Theme::mix(Theme::color(Theme::Token::Surface, dark), Theme::color(Theme::Token::Border, dark), 0.6));
  }
  Theme::apply(true);
}

// "● unsaved changes" stands right under Save while there are unsaved edits
void TestScriptLabBeginner::unsavedAtSave()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.path() + "/a.lsl", "default {}\n"));
  LabEditor editor;
  editor.setTargetFolder(dir.path());
  editor.openFile(dir.path() + "/a.lsl", "a.lsl");
  QVERIFY(editor.unsavedLabel()->isHidden());
  editor.codeEdit()->setFocus();
  editor.codeEdit()->moveCursor(QTextCursor::End);
  editor.codeEdit()->insertPlainText("// x\n");
  QCoreApplication::processEvents();
  QVERIFY(!editor.unsavedLabel()->isHidden());
  QCOMPARE(editor.unsavedLabel()->text(), QString("● unsaved changes"));
  QCOMPARE(editor.unsavedLabel()->palette().color(QPalette::WindowText),
           Theme::color(Theme::Token::Warning, ScriptLabPanel::isDark(editor.palette())));
  // under Save, starting where Save starts
  QCOMPARE(editor.unsavedLabel()->parentWidget(), editor.saveButton()->parentWidget());
  editor.resize(600, 400);
  editor.show();
  QCoreApplication::processEvents();
  QCOMPARE(editor.unsavedLabel()->x(), editor.saveButton()->x());
  QVERIFY(editor.unsavedLabel()->y() >= editor.saveButton()->y() + editor.saveButton()->height());
  QVERIFY(editor.save());
  QVERIFY(editor.unsavedLabel()->isHidden());
}

// greyed Run again and Open in editor say why in a line under them
void TestScriptLabBeginner::reasonUnderRunAgain()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QVERIFY(panel.scriptButtonsReason()->isHidden());   // nothing watched: the empty state
  QTemporaryDir empty;
  QVERIFY(panel.watchFolder(empty.path()));
  QVERIFY(!panel.runAgainButton()->isEnabled());
  QVERIFY(!panel.openEditorButton()->isEnabled());
  QVERIFY(!panel.scriptButtonsReason()->isHidden());
  QCOMPARE(panel.scriptButtonsReason()->text(),
           QString("Run again, Open in editor: No script yet. Watch a folder and save a .lsl file in it first."));
  QCOMPARE(panel.scriptButtonsReason()->palette().color(QPalette::WindowText),
           Theme::color(Theme::Token::TextMuted, ScriptLabPanel::isDark(panel.palette())));
  QCOMPARE(panel.runAgainButton()->statusTip(), QString("No script yet. Watch a folder and save a .lsl file in it first."));
  panel.stopWatching();
  QVERIFY(panel.makeExampleProject());
  QVERIFY(panel.runAgainButton()->isEnabled());
  QVERIFY(panel.scriptButtonsReason()->isHidden());
}

// grey and greyed text are at least 4.5:1 on every surface they sit on, in both themes
void TestScriptLabBeginner::contrastOfGreyText()
{
  for (bool dark : {true, false})
  {
    const QColor muted = Theme::color(Theme::Token::TextMuted, dark);
    const QColor surface = Theme::color(Theme::Token::Surface, dark);
    const QColor raised = Theme::color(Theme::Token::SurfaceRaised, dark);
    const QColor border = Theme::color(Theme::Token::Border, dark);
    // the surfaces: the panel, buttons, the step card and tips, and the idle status line's fill
    for (const QColor& back : {surface, raised, Theme::mix(surface, border, 0.10)})
      QVERIFY2(contrast(muted, back) >= 4.5, qPrintable(QString("%1 on %2: %3").arg(muted.name(), back.name()).arg(contrast(muted, back))));
    // greyed buttons now use text_muted, higher than text_disabled
    QVERIFY(contrast(muted, raised) > contrast(Theme::color(Theme::Token::TextDisabled, dark), raised));
    Theme::apply(dark);
    ScriptLabPanel panel;
    const QString mutedName = muted.name(QColor::HexRgb).toUpper();
    for (QPushButton* b : {panel.watchButton(), panel.newFolderButton(), panel.exampleButton(), panel.runAgainButton(),
                           panel.openEditorButton(), panel.unwatchButton()})
      QVERIFY2(b->styleSheet().contains(":disabled { color: " + mutedName + ";"), qPrintable(b->objectName()));
    QVERIFY(panel.editor()->saveButton()->styleSheet().contains(":disabled { color: " + muted.name() + ";"));
    // the step card's greyed buttons and reason lines
    LabStepCard::Step step;
    step.text = "x";
    step.button = "Touch";
    step.enabled = false;
    step.reason = "This step does not work in the program yet.";
    panel.stepCard()->setSteps({step});
    QVERIFY(panel.stepCard()->stepButtons().first()->styleSheet().contains(":disabled { color: " + muted.name() + ";"));
    QCOMPARE(panel.stepCard()->reasons().first()->palette().color(QPalette::WindowText), muted);
    QCOMPARE(panel.stepCard()->fillColour(), raised);
  }
  Theme::apply(true);
}

// a prop can get a wanted name (the prim's), when no prop has it yet
void TestScriptLabBeginner::propNamesForExamples()
{
  PropManager props(1);
  QCOMPARE(props.addProp(Prop::Box, {}, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0)->name(), QString("Box 0"));
  QCOMPARE(props.addProp(Prop::Torus, {}, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, "Wrist_left")->name(), QString("Wrist_left"));
  // taken: the program's own name, as before
  QCOMPARE(props.addProp(Prop::Torus, {}, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, "Wrist_left")->name(), QString("Torus 0"));
  QCOMPARE(props.addProp(Prop::Box, {}, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, "Box 0")->name(), QString("Box 1"));
  QCOMPARE(props.addProp(Prop::Sphere, {}, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, "  ")->name(), QString("Sphere 0"));
  QVERIFY(props.getPropByName("Wrist_left"));
}

// every new text is in the register with the same words, at most 12 words a sentence
void TestScriptLabBeginner::textsInRegister()
{
  const QString teaching = readFile(QStringLiteral(PINGLYMOTION_DOCS_DIR "/TEACHING.md"));
  QVERIFY(teaching.contains("### Script Lab for beginners (job U3)"));
  QStringList texts = {
    "Script Lab shows your scripts from a folder on your PC. Choose a folder to see them here.",
    "Make example project", "Copies a working example to your folder and shows it in 3D.",
    "New folder", "Makes an empty folder for your scripts and watches it.",
    "This tree is your object. Each folder in it is one prim.", "Follow selection in 3D view",
    "Click a piece here to light up its prop in 3D view. Click a prop and its row is picked here.",
    "You should see", "Then try", "Close the steps. Help, Script Lab steps shows them again.",
    "Show Props", "Add the props", "Show Live scripts", "Watch the folder", "Show effects",
    "This step does not work in the program yet.", "Script Lab steps", "Show the steps of the example project again.",
    "Close this tip. It does not come back.", "Rules", "Warnings",
    "Type a name for the folder first.", "The name is too long. Use at most 40 characters.",
    "Use only letters, digits, spaces, - and _ in the name.", "Windows keeps this name for itself. Pick another name.",
    "A folder with this name is already there. Pick another name.", "The folder path would be too long. Use a shorter name.",
    "Could not make the folder. Check that Documents can be written.", "The example objects are not next to the program.",
    "The example could not be read.", "On: the 3D view shows what the scripts do.",
    "You clicked %1.", "Error: the touch on %1 was not sent. %2",
    // UX small fixes: the ? button and the help view
    "Help for Script Lab", "Explanations, the words and every example, in a bigger view.", "Script Lab help",
    "Help topics", "Example: %1", "The words Script Lab uses, in plain words.", "Close"};
  QVERIFY(teaching.contains("### UX small fixes (24 Nov 2026)"));
  for (const LabBeginner::Tip& tip : LabBeginner::jargonTips()) texts << tip.text;
  for (const QString& text : texts)
  {
    QVERIFY2(teaching.contains("| " + text + " |"), qPrintable(text));
    verifyShortSentences(text);
  }
}

// the window gives the panel the scene and passes a click on a prop to the tree (source check: the
// window itself is too large for a unit test; the panel's side is tested above)
void TestScriptLabBeginner::windowWiring()
{
  // the wiring is in the Script Lab controller since job R5 (src/scriptlabcontroller.*); the window
  // gives it the 3D view and the scene
  const QString window = WindowSource::read(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp"));
  QVERIFY(window.contains("m_scriptLab->connectView(ui->animationView);"));
  QVERIFY(window.contains("m_scriptLab->connectScene(scene,meshes,hooks);"));
  QVERIFY(window.contains("m_scriptLab->makeStepsAction(this)"));
  const QString source = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/scriptlabcontroller.cpp"));
  const QString view = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/scriptlabcontrollerview.cpp"));
  QVERIFY(source.contains("m_scriptLabPanel->setSceneHooks(hooks);"));
  QVERIFY(view.contains("m_scriptLabPanel->selectPrim(prop->name());"));
  QVERIFY(source.contains("m_scriptLabPanel->makeStepsAction(parent)"));
  QVERIFY(source.contains("scene->addProp(type,mesh,attach,"));
  // job A1: a click on a prop is a touch, and the panel can read and set where a prop stands
  QVERIFY(view.contains("m_scriptLabPanel->touchPrim(prop->name(),view->mapFromGlobal(QCursor::pos()));"));
  // job A2: the menu box stands over the 3D view
  QVERIFY(view.contains("m_scriptLabPanel->setMenuBoxHost(view);"));
  // job A4, part 2: the chains go to the 3D view
  QVERIFY(view.contains("connect(m_scriptLabPanel,&ScriptLabPanel::chainsChanged,this,[this,view]()"));
  QVERIFY(source.contains("lines.append(ChainLines::Line{chain.from,chain.to,chain.locked,chain.word()});"));
  QVERIFY(view.contains("view->setChainLines(chainLines());"));
  QVERIFY(source.contains("hooks.propPlace="));
  QVERIFY(source.contains("hooks.placeProp="));
}

// job A1: with the effects shown, every prim gets a prop; a click on a prop is a touch to that prim's
// scripts, with a line in the event log; a prop jumps where a script moves its prim, and back when
// the effects are turned off
void TestScriptLabBeginner::propsFromPrims()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Set");
  QVERIFY(writeFile(top + "/Root/hud.lsl", "default { state_entry() { llSetScale(<1.0, 1.0, 0.5>); } }\n"));
  QVERIFY(writeFile(top + "/Collar/collar.lsl",
                    "default { touch_start(integer n) { llOwnerSay(\"collar touched\"); llSetPos(<0.0, 0.0, 1.0>); } }\n"));
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QVERIFY(panel.watchFolder(top));
  // effects off: no props, and a click sends nothing
  QVERIFY(!panel.primPropsShown());
  QVERIFY(scene.props.isEmpty());
  QVERIFY(!panel.touchPrim("Collar"));

  panel.effectsBox()->setChecked(true);
  QVERIFY(panel.primPropsShown());
  QCOMPARE(scene.props, QStringList({"Root", "Collar"}));
  QCOMPARE(scene.added[0].shape, QString("box"));
  QCOMPARE(scene.added[0].joint, QString());
  QCOMPARE(scene.added[1].shape, QString("torus"));
  QCOMPARE(scene.added[1].joint, QString("neck"));
  // the HUD script made its prim half as high: the box is half as deep, at once
  QCOMPARE(scene.places["Root"].scale, QVector3D(16, 16, 1));
  QCOMPARE(scene.places["Collar"].position, QVector3D(0, 1.5f, 0));
  QCOMPARE(panel.primMoves().size(), 1);

  // a click on the collar: a touch to collar.lsl, which says so and moves the collar 1 m up
  QVERIFY(panel.touchPrim("Collar"));
  const QString log = panel.runLog()->toPlainText();
  QVERIFY2(log.contains(" s  You clicked Collar.\n"), qPrintable(log));
  QVERIFY2(log.contains("collar (Collar): owner_say: collar touched"), qPrintable(log));
  QVERIFY(log.indexOf("You clicked Collar.") < log.indexOf("collar touched"));
  QCOMPARE(scene.places["Collar"].position, QVector3D(0, 40.87f, 0));
  QCOMPARE(scene.places["Collar"].rotation, QVector3D(90, 0, 0));
  QCOMPARE(panel.primMoves().size(), 2);
  // no prim of that name: nothing is sent
  QVERIFY(!panel.touchPrim("Nope"));
  // a prop that is already there is not made again
  QCOMPARE(scene.added.size(), 2);

  // effects off: the props go back where they stood, and keep their names
  panel.effectsBox()->setChecked(false);
  QCOMPARE(scene.places["Root"].scale, QVector3D(8, 8, 1));
  QCOMPARE(scene.places["Collar"].position, QVector3D(0, 1.5f, 0));
  QVERIFY(panel.primMoves().isEmpty());
  QCOMPARE(scene.props, QStringList({"Root", "Collar"}));
}

// job A2: a menu a script shows after a click on a prop stands next to the click, and a button answers it
void TestScriptLabBeginner::menuBoxAtTheClick()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Set");
  QVERIFY(writeFile(top + "/Root/hud.lsl", "default { state_entry() { } }\n"));
  QVERIFY(writeFile(top + "/Collar/collar.lsl",
                    "default {\n"
                    "  touch_start(integer n) {\n"
                    "    llListen(-77, \"\", llGetOwner(), \"\");\n"
                    "    llDialog(llGetOwner(), \"Lock the collar?\", [\"Lock\", \"Unlock\", \"Cancel\", \"A & B\"], -77);\n"
                    "  }\n"
                    "  listen(integer channel, string name, key id, string message) {\n"
                    "    llOwnerSay(\"picked \" + message);\n"
                    "    if (message == \"Lock\") llOwnerSay(\"@detach=n\");\n"
                    "  }\n"
                    "}\n"));
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QWidget host;   // stands in for the 3D view
  host.resize(800, 600);
  host.show();
  panel.setMenuBoxHost(&host);
  LabDialogBox* box = panel.menuBox();
  QVERIFY(box);
  QCOMPARE(box->parentWidget(), &host);
  QVERIFY(panel.watchFolder(top));
  QVERIFY(!panel.answerMenu("Lock"));   // no menu yet: nothing is sent
  QCOMPARE(panel.liveStatusText(), QString("Error: the menu answer \"Lock\" was not sent. No menu is open. Click a part that shows one first."));
  panel.effectsBox()->setChecked(true);
  QVERIFY(!box->isVisible());

  // a click on the collar at (100, 200): the menu stands to the right of it
  QVERIFY(panel.touchPrim("Collar", QPoint(100, 200)));
  QVERIFY(box->isVisible());
  QCOMPARE(box->title()->text(), QString("Menu from Set"));
  QCOMPARE(box->message()->text(), QString("Lock the collar?"));
  QCOMPARE(box->channelLine()->text(), QString("A button says its text on channel -77. Only the script hears it."));
  QCOMPARE(box->buttons().size(), 4);
  QStringList texts;
  for (QPushButton* b : box->buttons()) texts << b->property("button").toString();
  QCOMPARE(texts, QStringList({"Lock", "Unlock", "Cancel", "A & B"}));
  QCOMPARE(box->buttons()[3]->text(), QString("A && B"));   // "&" shows as itself, not as a shortcut
  QVERIFY(!box->textField()->isVisible());
  // rows of three, the first row on top
  QCOMPARE(box->buttons()[0]->y(), box->buttons()[2]->y());
  QVERIFY(box->buttons()[0]->x() < box->buttons()[1]->x());
  QVERIFY(box->buttons()[3]->y() > box->buttons()[0]->y());
  QCOMPARE(box->buttons()[3]->x(), box->buttons()[0]->x());
  QCOMPARE(box->x(), 100 + LabDialogBox::Gap);
  QCOMPARE(box->y(), qMax(LabDialogBox::Margin, 200 - box->height() / 2));
  QVERIFY(host.rect().contains(box->geometry()));
  QVERIFY(box->buttons()[0]->hasFocus() || !QApplication::focusWidget());   // the keys go to the menu
  QVERIFY(panel.runLog()->toPlainText().contains("You clicked Collar."));
  if (!qEnvironmentVariableIsEmpty("PINGLYMOTION_UI_OUT"))   // a picture for a session to look at
    host.grab().save(qEnvironmentVariable("PINGLYMOTION_UI_OUT") + "/menubox.png");

  // the same menu shows in the panel's world list too? No: the panel's menu belongs to lsl_run's world
  // a click on Lock: the avatar says it on -77, the script hears it and locks
  QTest::mouseClick(box->buttons()[0], Qt::LeftButton);
  QVERIFY(!box->isVisible());
  const QString log = panel.runLog()->toPlainText();
  QVERIFY2(log.contains(" s  You clicked \"Lock\" in the menu (channel -77).\n"), qPrintable(log));
  QVERIFY2(log.contains("collar (Collar): owner_say: picked Lock"), qPrintable(log));
  QVERIFY(log.indexOf("in the menu (channel -77)") < log.indexOf("picked Lock"));
  QCOMPARE(state.actions().value("detach"), false);
  // the menu does not come back by itself
  panel.effectsBox()->setChecked(false);
  panel.effectsBox()->setChecked(true);
  QVERIFY(!box->isVisible());

  // a click near the right edge: the menu stands to the left of the click, inside the view
  QVERIFY(panel.touchPrim("Collar", QPoint(780, 10)));
  QVERIFY(box->isVisible());
  QCOMPARE(box->x(), 780 - LabDialogBox::Gap - box->width());
  QCOMPARE(box->y(), LabDialogBox::Margin);
  // Ignore: the box closes and nothing is said
  const QString before = panel.runLog()->toPlainText();
  QTest::mouseClick(box->ignoreButton(), Qt::LeftButton);
  QVERIFY(!box->isVisible());
  QCOMPARE(panel.runLog()->toPlainText(), before);

  // Esc ignores it too; turning the effects off hides an open menu
  QVERIFY(panel.touchPrim("Collar", QPoint(-1, -1)));
  QVERIFY(box->isVisible());
  QCOMPARE(box->x(), 800 - LabDialogBox::Margin - box->width());   // no click point: the top right corner
  QCOMPARE(box->y(), LabDialogBox::Margin);
  QTest::keyClick(box, Qt::Key_Escape);
  QVERIFY(!box->isVisible());
  QVERIFY(panel.touchPrim("Collar", QPoint(300, 300)));
  QVERIFY(box->isVisible());
  panel.effectsBox()->setChecked(false);
  QVERIFY(!box->isVisible());
  // the panel's own menu (for the world of lsl_run) is untouched: it still has no dialog
  QCOMPARE(panel.dialogButtons().size(), 0);
}

// job A2: llTextBox shows a field; Send says the typed text on the channel
void TestScriptLabBeginner::menuBoxTextBox()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Pet");
  QVERIFY(writeFile(top + "/Collar/collar.lsl",
                    "default {\n"
                    "  touch_start(integer n) { llListen(-88, \"\", NULL_KEY, \"\"); llTextBox(llGetOwner(), \"Name your pet\", -88); }\n"
                    "  listen(integer channel, string name, key id, string message) { llOwnerSay(\"name \" + message); }\n"
                    "}\n"));
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QWidget host;
  host.resize(640, 480);
  host.show();
  panel.setMenuBoxHost(&host);
  QVERIFY(panel.watchFolder(top));
  panel.effectsBox()->setChecked(true);
  QVERIFY(panel.touchPrim(scene.props.last(), QPoint(50, 50)));
  LabDialogBox* box = panel.menuBox();
  QVERIFY(box->isVisible());
  QCOMPARE(box->message()->text(), QString("Name your pet"));
  QCOMPARE(box->buttons().size(), 0);
  QVERIFY(box->textField()->isVisible());
  QCOMPARE(box->channelLine()->text(), QString("What you type is said on channel -88. Only the script hears it."));
  QVERIFY(!box->sendButton()->isEnabled());   // nothing typed yet
  QTest::keyClicks(box->textField(), "Rex");
  QVERIFY(box->sendButton()->isEnabled());
  QTest::keyClick(box->textField(), Qt::Key_Return);
  QVERIFY(!box->isVisible());
  const QString log = panel.runLog()->toPlainText();
  QVERIFY2(log.contains("You clicked \"Rex\" in the menu (channel -88)."), qPrintable(log));
  QVERIFY2(log.contains("owner_say: name Rex"), qPrintable(log));
}

// job A3: Record what I do saves clicks, menu answers and typed lines; every save replays them
void TestScriptLabBeginner::recordAndReplay()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Set");
  QVERIFY(writeFile(top + "/Root/hud.lsl", "default { state_entry() { } }\n"));
  const QByteArray collar =
    "default {\n"
    "  touch_start(integer n) {\n"
    "    llListen(-77, \"\", llGetOwner(), \"\");\n"
    "    llDialog(llGetOwner(), \"Lock the collar?\", [\"Lock\", \"Unlock\"], -77);\n"
    "  }\n"
    "  listen(integer channel, string name, key id, string message) {\n"
    "    llOwnerSay(\"picked \" + message);\n"
    "    if (message == \"Lock\") llOwnerSay(\"@detach=n\");\n"
    "  }\n"
    "}\n";
  QVERIFY(writeFile(top + "/Collar/collar.lsl", collar));
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  panel.setExpertMode(false);
  panel.resize(700, 900);
  panel.show();
  QPushButton* record = panel.recordButton();
  QVERIFY(record);
  // job UX-B: record, recipe and player are on the Player tab
  panel.showPage(ScriptLabPanel::PlayerPage);
  QVERIFY(panel.page(ScriptLabPanel::PlayerPage)->isAncestorOf(record));
  QVERIFY(!record->isVisibleTo(&panel));   // nothing watched: no recipe
  QVERIFY(panel.playerEmpty()->isVisibleTo(&panel));
  QVERIFY(panel.watchFolder(top));
  QCOMPARE(panel.currentPage(), ScriptLabPanel::LivePage);   // a watch that starts shows Live scripts
  panel.showPage(ScriptLabPanel::PlayerPage);
  QVERIFY(record->isVisibleTo(&panel));
  QVERIFY(!panel.playerEmpty()->isVisibleTo(&panel));
  // Beginner mode: one button and the line, no recipe part (charter P5)
  QVERIFY(!panel.recipeSection()->isVisibleTo(&panel));
  QCOMPARE(record->text(), QString("Record what I do"));
  QVERIFY(!record->isChecked());
  QCOMPARE(panel.recipeStatus()->text(), QString("No recipe yet. Press Record what I do, then click your props."));
  QVERIFY(!record->toolTip().isEmpty());
  QVERIFY(record->focusPolicy() != Qt::NoFocus);   // charter P8

  // start with the keyboard: Space presses the button
  record->setFocus();
  QTest::keyClick(record, Qt::Key_Space);
  QVERIFY(watcher.recording());
  QVERIFY(record->isChecked());
  QCOMPARE(record->text(), QString("Stop recording"));
  QCOMPARE(panel.recipeStatus()->text(), QString("Recording. Click props and menu buttons. Steps so far: 0.\n"
                                                 "Turn on Show effects in 3D view to click props."));
  panel.effectsBox()->setChecked(true);
  QVERIFY(panel.touchPrim("Collar", QPoint(100, 200)));
  QVERIFY(panel.answerMenu("Lock"));
  QVERIFY(panel.runLine("@sendim=n"));   // typed in the Command field, from "panel"
  QCOMPARE(panel.recipeStatus()->text(), QString("Recording. Click props and menu buttons. Steps so far: 3."));
  QCOMPARE(readFile(top + "/recipe.rlvtest"),
           QString("# PinglyMotion recipe: what you did, one step per line.\n"
                   "# The program replays it after every save of a script.\n"
                   "# Steps: touch <part>, menu <button>, send <RLV line>, wait <seconds>.\n"
                   "# \"object <name>\" says who sends the next RLV lines. Lines with # are notes.\n"
                   "touch Collar\n"
                   "menu Lock\n"
                   "send @sendim=n\n"));

  // stop: the steps are replayed once, and the line says what is true now
  record->click();
  QVERIFY(!watcher.recording());
  QCOMPARE(record->text(), QString("Record what I do"));
  QCOMPARE(panel.recipeStatus()->text(), QString("Replayed your 3 step(s).\n"
                                                 "This is the first time, so there is nothing to compare.\n"
                                                 "Now:\n"
                                                 "  Set is locked.\n"
                                                 "  Send instant messages is blocked.\n"
                                                 "  Collar is saying \"picked Lock\"."));
  verifyShortSentences(panel.recipeStatus()->text());
  if (!qEnvironmentVariableIsEmpty("PINGLYMOTION_UI_OUT"))   // a picture for a session to look at
  {
    QTest::qWait(50);
    panel.grab().save(qEnvironmentVariable("PINGLYMOTION_UI_OUT") + "/recordreplay.png");
  }

  // a save of a changed script: the recipe is replayed and the line says what changed
  QByteArray noLock = collar;
  noLock.replace("    if (message == \"Lock\") llOwnerSay(\"@detach=n\");\n", "");
  QVERIFY(writeFile(top + "/Collar/collar.lsl", noLock));
  watcher.runScript("Collar/collar.lsl");
  QCOMPARE(panel.recipeStatus()->text(), QString("Replayed your 3 step(s).\n"
                                                 "What changed since last time:\n"
                                                 "  Before: Set was locked. Now: it is not locked."));
  watcher.runScript("Collar/collar.lsl");
  QCOMPARE(panel.recipeStatus()->text(), QString("Replayed your 3 step(s).\nNothing changed since last time."));
  QVERIFY(readFile(top + "/recipe.rlvresult").endsWith("# Nothing changed since the time before.\n"));

  // Expert mode: the recipe part with the file and Replay now; nothing else moves (charter P12)
  const QPoint recordAt = record->mapTo(&panel, QPoint(0, 0));
  panel.setExpertMode(true);
  QVERIFY(panel.recipeSection()->isVisibleTo(&panel));
  QCOMPARE(record->mapTo(&panel, QPoint(0, 0)), recordAt);
  QCOMPARE(panel.recipeFileLine()->text(), QString("File: recipe.rlvtest in the object folder. recipe.rlvresult is kept next to it."));
  QCOMPARE(panel.recipeText()->toPlainText(), readFile(top + "/recipe.rlvtest"));
  QVERIFY(panel.replayButton()->isEnabled());
  panel.replayButton()->click();
  QCOMPARE(panel.recipeStatus()->text(), QString("Replayed your 3 step(s).\nNothing changed since last time."));

  // recording again and stopping without a step keeps the recipe
  QVERIFY(panel.setRecording(true));
  QVERIFY(!panel.replayButton()->isEnabled());
  QCOMPARE(panel.replayButton()->toolTip(), QString("Stop recording first."));
  QVERIFY(panel.setRecording(false));
  QCOMPARE(panel.recipeStatus()->text(), QString("Recording stopped. No steps were recorded, so the recipe stays as it was."));
  QVERIFY(readFile(top + "/recipe.rlvtest").endsWith("send @sendim=n\n"));

  // a recipe with a mistake: Replay now says so in words, in the danger colour
  QVERIFY(writeFile(top + "/recipe.rlvtest", "dance\n"));
  QVERIFY(!panel.replayNow());
  QCOMPARE(panel.recipeStatus()->text(),
           QString("Error: The recipe has a mistake. Line 1: unknown word \"dance\". Use touch, menu, send, say, expect, object or wait."));
  QCOMPARE(panel.recipeStatus()->palette().color(QPalette::WindowText), ScriptLabPanel::dangerColour(true));

  // no recipe file: Replay now is greyed with its reason
  QVERIFY(QFile::remove(top + "/recipe.rlvtest"));
  QVERIFY(panel.setRecording(true));
  QVERIFY(panel.setRecording(false));
  QVERIFY(!panel.replayButton()->isEnabled());
  QCOMPARE(panel.replayButton()->toolTip(), QString("Record what you do first."));
  QCOMPARE(panel.recipeFileLine()->text(), QString("No recipe file yet. Recording makes recipe.rlvtest in the object folder."));

  // Stop watching hides the button and ends a recording
  QVERIFY(panel.setRecording(true));
  panel.stopWatching();
  QVERIFY(!watcher.recording());
  panel.showPage(ScriptLabPanel::PlayerPage);
  QVERIFY(!record->isVisibleTo(&panel));
  QVERIFY(!panel.recipeSection()->isVisibleTo(&panel));
}

// job A4: the channel map of a watched folder: its boxes and arrows, the error for a channel nobody
// listens on, a click on an arrow shows the line that sent it, a click on a box picks its prim, the
// keys, folded for beginners and open for experts, the word tips by the map, and the theme colours
void TestScriptLabBeginner::channelMap()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Gear");
  QVERIFY(writeFile(top + "/Root/hub.lsl",
                    "integer CMD = -77;\n"
                    "default\n"
                    "{\n"
                    "    state_entry()\n"
                    "    {\n"
                    "        llListen(CMD, \"\", NULL_KEY, \"\");\n"
                    "    }\n"
                    "    listen(integer channel, string name, key id, string message)\n"
                    "    {\n"
                    "        llMessageLinked(LINK_ALL_OTHERS, 5, message, NULL_KEY);\n"
                    "    }\n"
                    "}\n"));
  QVERIFY(writeFile(top + "/Collar/collar.lsl",
                    "default\n"
                    "{\n"
                    "    state_entry()\n"
                    "    {\n"
                    "        llSay(7, \"hello\");\n"
                    "        llSay(-77, \"lock\");\n"
                    "    }\n"
                    "    link_message(integer sender, integer num, string str, key id)\n"
                    "    {\n"
                    "        llOwnerSay(\"@detach=n\");\n"
                    "    }\n"
                    "}\n"));
  UserSettings::setExpertMode(false);
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  // job UX-B: the map is the tab Who talks to whom
  panel.showPage(ScriptLabPanel::MapPage);
  QVERIFY(panel.page(ScriptLabPanel::MapPage)->isAncestorOf(panel.mapSection()));
  QVERIFY(!panel.mapSection()->isVisibleTo(&panel));   // nothing watched: no map
  QVERIFY(panel.mapEmpty()->isVisibleTo(&panel));
  QCOMPARE(panel.mapEmpty()->text(), QString("Watch a folder as one object on Live scripts. Then this page shows who talks to whom."));
  QVERIFY(panel.watchFolder(top));
  panel.showPage(ScriptLabPanel::MapPage);
  ScriptLabSection* map = panel.mapSection();
  QCOMPARE(map->key(), QString("map"));
  QCOMPARE(map->title(), QString("Who talks to whom"));
  QVERIFY(map->isVisibleTo(&panel));
  QVERIFY(!panel.mapEmpty()->isVisibleTo(&panel));
  QVERIFY(!map->isExpanded());   // Beginner mode: folded
  QCOMPARE(map->count(), QString("4 messages, 1 problem"));
  QCOMPARE(panel.mapLine()->text(), QString("Each box is a part. Each arrow is a message, with its channel."));
  QCOMPARE(panel.mapProblems()->text(), QString("1 arrow reached nobody. It has a cross and says why."));
  ChannelMapView* view = panel.channelMap();
  QCOMPARE(view->toolTip(), QString("Click a box to pick it in the tree. Click an arrow to see the line that sent it."));
  const LabChannelMap::Map& m = view->map();
  QStringList boxes;
  for (const LabChannelMap::Box& b : m.boxes) boxes << b.label();
  QCOMPARE(boxes, QStringList({"Root", "Collar", "Your viewer (RLV)"}));
  QCOMPARE(m.arrows.size(), 4);
  QCOMPARE(m.arrows[0].error, QString("Nobody listens on channel 7."));
  QCOMPARE(m.arrows[0].label(), QString("say on channel 7: \"hello\""));
  QCOMPARE(view->partText(0, ChannelMapView::ArrowPart),
           QString("Collar to nobody: say on channel 7: \"hello\". Nobody listens on channel 7. Click to see the script line that sent it."));
  QCOMPARE(view->partText(1, ChannelMapView::ToPart), QString("Root. A part of the object (prim 1). Click to pick it."));

  // Expert mode opens it, Beginner mode folds it again
  panel.setExpertMode(true);
  QVERIFY(map->isExpanded());
  panel.setExpertMode(false);
  QVERIFY(!map->isExpanded());
  map->setExpanded(true);

  // a click on the arrow of the link message shows hub.lsl at its line 10
  view->resize(800, view->sizeHint().height());
  QCOMPARE(view->hitAt(view->partRect(2, ChannelMapView::ArrowPart).center()).row, 2);
  QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, view->partRect(2, ChannelMapView::ArrowPart).center());
  QCOMPARE(panel.shownScript(), QString("Root/hub.lsl"));
  QCOMPARE(panel.scriptView()->textCursor().blockNumber(), 9);
  QCOMPARE(panel.scriptView()->textCursor().positionInBlock(), 8);
  // job UX-B: the editor is on Live scripts, so that tab comes to the front; back to the map
  QCOMPARE(panel.currentPage(), ScriptLabPanel::LivePage);
  panel.showPage(ScriptLabPanel::MapPage);
  // the arrow to nobody: collar.lsl at line 5, column 9
  QVERIFY(panel.activateMapArrow(0));
  QCOMPARE(panel.shownScript(), QString("Collar/collar.lsl"));
  QCOMPARE(panel.scriptView()->textCursor().blockNumber(), 4);
  QCOMPARE(panel.scriptView()->textCursor().positionInBlock(), 8);
  QVERIFY(!panel.activateMapArrow(99));

  // a click on a box picks its prim in the tree and lights up its prop
  scene.props << "Collar";
  QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, view->partRect(1, ChannelMapView::FromPart).center());
  QVERIFY(panel.folderTree()->currentItem());
  QCOMPARE(panel.folderTree()->currentItem(), panel.folderTree()->itemForPrim("Collar"));
  QCOMPARE(scene.selected, QStringList({"Collar"}));
  // the viewer is not a prim: nothing to pick
  QVERIFY(!panel.activateMapBox(2));

  // keys: Up and Down move between rows, Left and Right between the parts, Enter does the click
  view->setFocus();
  view->setCurrent(0, ChannelMapView::ArrowPart);
  QTest::keyClick(view, Qt::Key_Down);
  QCOMPARE(view->currentRow(), 1);
  QTest::keyClick(view, Qt::Key_Right);
  QCOMPARE(view->currentPart(), ChannelMapView::ToPart);
  QTest::keyClick(view, Qt::Key_Return);
  QCOMPARE(panel.folderTree()->currentItem(), panel.folderTree()->itemForPrim("Root"));
  QTest::keyClick(view, Qt::Key_End);
  QCOMPARE(view->currentRow(), 3);
  QTest::keyClick(view, Qt::Key_Home);
  QCOMPARE(view->currentRow(), 0);

  // the theme: a box is filled with surface_raised, its words in text
  view->setFocus();
  view->clearFocus();
  const QImage image = view->grab().toImage();
  const QRect box = view->partRect(1, ChannelMapView::FromPart);
  QCOMPARE(image.pixelColor(box.left() + 3, box.center().y()), Theme::color(Theme::Token::SurfaceRaised));

  // the word tips by the map: channel first, then listener (after the others are closed); only
  // while the map's tab is in front (job UX-B: the word must be on screen)
  panel.showPage(ScriptLabPanel::LivePage);
  UserSettings::setScriptLabTipsClosed("prim,rootPrim,notecard,sandbox,viewerRules,rlv,rlva,held");
  panel.setHintsOn(false);
  panel.setHintsOn(true);
  QVERIFY(!panel.shownTip() || panel.shownTip()->key() != QString("channel"));
  panel.showPage(ScriptLabPanel::MapPage);
  UserSettings::setScriptLabTipsClosed("prim,rootPrim,notecard,sandbox,viewerRules,rlv,rlva,held");
  panel.setHintsOn(false);
  panel.setHintsOn(true);
  QVERIFY(panel.shownTip());
  QCOMPARE(panel.shownTip()->key(), QString("channel"));
  QVERIFY(map->isAncestorOf(panel.shownTip()));
  QCOMPARE(panel.shownTip()->textLabel()->text(), QString("A number that scripts talk on. Only listeners on it hear."));
  map->setExpanded(false);
  QVERIFY(!panel.shownTip());   // folded: the word is not on screen
  map->setExpanded(true);
  panel.closeTip("channel");
  QCOMPARE(panel.shownTip()->key(), QString("listener"));
  QCOMPARE(panel.shownTip()->textLabel()->text(), QString("A script that waits for messages on one channel."));

  // every text of the map is in the register (docs/TEACHING.md, LABMAP-01 to LABMAP-20), in short sentences
  const QString teaching = readFile(QStringLiteral(PINGLYMOTION_DOCS_DIR "/TEACHING.md"));
  QVERIFY(teaching.contains("### Script Lab: the channel map (job A4)"));
  const QStringList texts = {
    map->title(), panel.mapLine()->text(), view->toolTip(), ChannelMapView::emptyText(), "Nobody listens on channel %1.",
    "Nobody listens on channel %1, so the menu answer is lost.", "Nobody with that key listens on channel %1.",
    "No script gets link message %1.", "Your viewer refused this line: %1", "1 arrow reached nobody. It has a cross and says why.",
    "%1 arrows reached nobody. Each has a cross and says why.", "The map shows the first %1 arrows only.",
    "You, the avatar. You see menus and open chat, and you click.", "Your viewer. It gets the RLV lines a script sends.",
    "A web server outside Second Life. The sandbox never goes online.", "This part listens and passes messages on, like a server.",
    "Click to see the script line that sent it.", "It came from outside the scripts."};
  for (const QString& text : texts)
  {
    QVERIFY2(teaching.contains("| " + text + " |"), qPrintable(text));
    verifyShortSentences(text);
  }

  // Stop watching: no map
  panel.stopWatching();
  panel.showPage(ScriptLabPanel::MapPage);
  QVERIFY(!map->isVisibleTo(&panel));
  QVERIFY(panel.mapEmpty()->isVisibleTo(&panel));
  QVERIFY(view->map().arrows.isEmpty());
}

// job A4, part 2: a chain a script makes between two prims, shown while the props are, red and the word
// Locked while the object holds @detach, green and Free when it does not
void TestScriptLabBeginner::chainLines()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Set");
  QVERIFY(writeFile(top + "/Root/hud.lsl",
                    "default { touch_start(integer n) { llOwnerSay(\"@detach=n\"); } }\n"));
  QVERIFY(writeFile(top + "/Collar/collar.lsl", "default { state_entry() { } }\n"));
  QVERIFY(writeFile(top + "/Cuff_left/cuff.lsl", "default { state_entry() { llParticleSystem([PSYS_SRC_TARGET_KEY, llGetLinkKey(2)]); } }\n"));
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QSignalSpy changed(&panel, &ScriptLabPanel::chainsChanged);
  QVERIFY(panel.watchFolder(top));
  QVERIFY(panel.chains().isEmpty());   // effects off: no lines
  panel.effectsBox()->setChecked(true);
  QCOMPARE(panel.chains().size(), 1);
  QVERIFY(changed.count() >= 1);
  QCOMPARE(panel.chains()[0].from, QString("Cuff_left"));
  QCOMPARE(panel.chains()[0].to, QString("Collar"));
  QVERIFY(!panel.chains()[0].locked);
  QCOMPARE(panel.chains()[0].word(), QString("Free"));
  // a click on the HUD locks the object: the chain is locked at once
  QVERIFY(panel.touchPrim("Root"));
  QVERIFY(panel.chains()[0].locked);
  QCOMPARE(panel.chains()[0].word(), QString("Locked"));
  // Reset in the panel lifts the lock
  panel.resetAll();
  QVERIFY(!panel.chains()[0].locked);
  // effects off: the line goes
  panel.effectsBox()->setChecked(false);
  QVERIFY(panel.chains().isEmpty());
}

// UX small fixes (the project owner, 24 Nov 2026: pressing Show Live scripts did nothing he could see,
// because the steps card sits on that page): the step brings the tab to the front, scrolls its page to
// the top, puts a short frame round the tab and round the top of the page, and gives the tab the keys
void TestScriptLabBeginner::showLiveScriptsShowsSomething()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  panel.resize(520, 500);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  panel.activateWindow();
  panel.showSteps();
  LabStepCard* card = panel.stepCard();
  QPushButton* show = card->stepButtons().at(2);
  QCOMPARE(show->text(), QString("Show Live scripts"));
  // already on the Live scripts page, scrolled down: the case where nothing used to happen
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));
  QScrollArea* scroll = qobject_cast<QScrollArea*>(panel.tabs()->widget(ScriptLabPanel::LivePage));
  QVERIFY(scroll);
  scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
  QTabBar* bar = panel.tabs()->tabBar();
  QVERIFY(!LabHighlight::on(bar));
  QTest::mouseClick(show, Qt::LeftButton);
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));
  QCOMPARE(scroll->verticalScrollBar()->value(), 0);
  // the frame stands round the Live scripts tab, and round the top of the page (the empty state with
  // Watch folder, as no folder is watched yet)
  LabHighlight* frame = LabHighlight::on(bar);
  QVERIFY(frame);
  QVERIFY(frame->isVisible());
  QCOMPARE(frame->area(), bar->tabRect(ScriptLabPanel::LivePage));
  QCOMPARE(bar->tabText(ScriptLabPanel::LivePage), QString("Live scripts"));
  LabHighlight* top = LabHighlight::on(scroll->widget());
  QVERIFY(top);
  QVERIFY(top->area().contains(QRect(panel.watchButton()->mapTo(scroll->widget(), QPoint(0, 0)), panel.watchButton()->size())));
  // the keys are on the tab bar, and the step is ticked
  QTRY_VERIFY(bar->hasFocus());
  QVERIFY(card->ticks().at(2)->property("done").toBool());
  // from the other page it comes back too
  panel.tabs()->setCurrentIndex(ScriptLabPanel::CommandsPage);
  QVERIFY(panel.doStep(2));
  QCOMPARE(panel.tabs()->currentIndex(), int(ScriptLabPanel::LivePage));
  // the frame goes away by itself
  QTRY_VERIFY_WITH_TIMEOUT(!LabHighlight::on(bar), LabHighlight::ShowMs + 1500);
}

// UX small fixes (the project owner: it was not clear whether RLV or RLVa was on): the chosen button
// has the selection fill, a tick and bold text; the other has none of them. Show effects and Record
// are bold when on as well. Checked in both themes, drawn with the program's theme.
void TestScriptLabBeginner::activeChoiceIsClear()
{
  for (bool dark : {true, false})
  {
    Theme::apply(dark);
    RlvState state;
    ScriptLabPanel panel;
    panel.setState(&state);
    QPushButton* rlv = panel.findChild<QPushButton*>("scriptLabModeRlv");
    QPushButton* rlva = panel.findChild<QPushButton*>("scriptLabModeRlva");
    QVERIFY(rlv && rlva);
    for (int round = 0; round < 2; ++round)
    {
      const bool rlvOn = round == 1;
      panel.setMode(rlvOn ? Rlv::Mode::Rlv : Rlv::Mode::Rlva);
      QPushButton* on = rlvOn ? rlv : rlva;
      QPushButton* off = rlvOn ? rlva : rlv;
      QVERIFY(on->isChecked());
      QVERIFY(!off->isChecked());
      QCOMPARE(on->property("chosen").toBool(), true);
      QCOMPARE(off->property("chosen").toBool(), false);
      QVERIFY(!on->icon().isNull());   // the tick
      QVERIFY(off->icon().isNull());
      QCOMPARE(on->font().weight(), QFont::DemiBold);
      QCOMPARE(off->font().weight(), QFont::Normal);
      // never cut: the chosen look (bold word and tick) fits in both buttons
      for (QPushButton* b : {on, off})
        QVERIFY2(b->minimumWidth() >= QFontMetrics(on->font()).horizontalAdvance(b->text()) + 14 + 2 * Theme::Size::spacingM,
                 qPrintable(b->text()));
      QVERIFY(on->accessibleDescription().startsWith("Chosen."));
      QVERIFY(off->accessibleDescription().startsWith("Not chosen."));
      // the fill: the selection colour behind the chosen one, the raised surface behind the other
      panel.resize(560, 400);
      panel.show();
      QVERIFY(QTest::qWaitForWindowExposed(&panel));
      const QImage onImage = on->grab().toImage();
      const QImage offImage = off->grab().toImage();
      const QPoint corner(4, on->height() / 2);
      QCOMPARE(onImage.pixelColor(corner).name(), Theme::color(Theme::Token::Selection, dark).name());
      QCOMPARE(offImage.pixelColor(QPoint(4, off->height() / 2)).name(), Theme::color(Theme::Token::SurfaceRaised, dark).name());
      panel.hide();
    }
    panel.effectsBox()->setChecked(true);
    QCOMPARE(panel.effectsBox()->font().weight(), QFont::DemiBold);
    panel.effectsBox()->setChecked(false);
    QCOMPARE(panel.effectsBox()->font().weight(), QFont::Normal);
  }
  Theme::apply(true);
}

// UX small fixes (the project owner: help and guides should not sit inside the function): the ?
// button opens one bigger help view with the topics, the words and every example's guide
void TestScriptLabBeginner::helpViewOutOfThePanel()
{
  QTemporaryDir dir;
  RlvState state;
  LabWatcher watcher;
  ScriptLabPanel panel;
  FakeScene scene;
  setUpPanel(panel, watcher, state, dir.path(), scene);
  QToolButton* button = panel.helpButton();
  QVERIFY(button);
  QCOMPARE(button->accessibleName(), QString("Help for Script Lab"));
  QCOMPARE(button->toolTip(), QString("Help for Script Lab\nExplanations, the words and every example, in a bigger view."));
  QVERIFY(!button->icon().isNull());
  QVERIFY(button->focusPolicy() & Qt::TabFocus);
  QVERIFY(!panel.helpView());
  QTest::mouseClick(button, Qt::LeftButton);
  LabHelpView* help = panel.helpView();
  QVERIFY(help);
  QVERIFY(help->isVisible());
  QVERIFY(!help->isModal());   // the window stays usable, so a step can be read and done
  QCOMPARE(help->windowTitle(), QString("Script Lab help"));
  QCOMPARE(help->currentTopic(), QString("start"));
  QCOMPARE(help->topicHeading(), QString("Start here"));
  QCOMPARE(help->topicTitles(), QStringList({"Start here", "How to lay out a folder", "Words", "Hints",
                                             "Example: First colour", "Example: Button HUD", "Example: Gear set",
                                             "Example: Timed lock", "Example: Simple RLV relay", "Example: Work system"}));
  QVERIFY(help->topicText().contains("Script Lab tests Second Life scripts safely on your PC."));
  QVERIFY(help->topicText().contains("Press Make example project on the Live scripts tab."));
  verifyShortSentences(help->topicText());
  // the heading "Examples" stands in the list but cannot be picked
  const QList<QListWidgetItem*> heads = help->topicList()->findItems("Examples", Qt::MatchExactly);
  QCOMPARE(heads.size(), 1);
  QCOMPARE(heads.first()->flags(), Qt::NoItemFlags);
  // the words: every word tip, with its meaning
  QVERIFY(help->showTopic("words"));
  for (const LabBeginner::Tip& tip : LabBeginner::jargonTips())
    QVERIFY2(help->topicText().contains(tip.word + ": " + tip.text), qPrintable(tip.word));
  // hints: what the cross does, as the hint cards now do
  QVERIFY(help->showTopic("hints"));
  QVERIFY(help->topicText().contains("The cross closes a hint. Turn Help > Show hints off and on for the next one."));
  // an example: its guide texts from example.json, word for word, with the steps numbered
  QVERIFY(help->showTopic("example:Gear_set"));
  QCOMPARE(help->topicHeading(), QString("Example: Gear set"));
  const LabExample gear = LabBeginner::readExample(examplesDir() + "/Gear_set");
  QVERIFY(gear.isValid());
  for (const LabExample::Entry& e : gear.guide)
    QVERIFY2(help->topicText().contains(e.text), qPrintable(e.id));
  for (const QString& h : QStringList({"What it is", "Steps", "The pieces", "New words", "You should see", "Then try"}))
    QVERIFY2(help->topicText().contains(h), qPrintable(h));
  // Down in the list goes to the next topic
  QVERIFY(help->showTopic("start"));
  help->topicList()->setFocus();
  QTest::keyClick(help->topicList(), Qt::Key_Down);
  QCOMPARE(help->currentTopic(), QString("folder"));
  QCOMPARE(help->topicHeading(), QString("How to lay out a folder"));
  // a second press brings the same view back, at the start
  help->close();
  QVERIFY(!help->isVisible());
  QTest::mouseClick(button, Qt::LeftButton);
  QCOMPARE(panel.helpView(), help);
  QVERIFY(help->isVisible());
  QCOMPARE(help->currentTopic(), QString("start"));
  QVERIFY(!help->showTopic("nothing"));
  // the picked topic: selection fill and bold, in the program's tokens
  QVERIFY(help->topicList()->styleSheet().contains("font-weight: 600"));
  QVERIFY(help->topicList()->styleSheet().contains(Theme::color(Theme::Token::Selection, true).name()));
  help->close();
}

// every step of every example names a control that is really there, with the name the program
// shows, and in the place the step says (the project owner followed the guide and could not find
// what it described)
void TestScriptLabBeginner::guideStepsNameRealControls()
{
  ScriptLabPanel panel;
  const QString ui = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.ui"));
  QVERIFY(!ui.isEmpty());
  const QStringList objects = QDir(examplesDir()).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  QVERIFY(objects.size() >= 6);
  int checked = 0;
  for (const QString& object : objects)
  {
    const LabExample example = LabBeginner::readExample(examplesDir() + "/" + object);
    QVERIFY2(example.isValid(), qPrintable(object));
    for (const LabExample::Entry& step : example.steps())
    {
      const QString what = example.object + " " + step.id + ": " + step.text;
      const QString c = step.control;
      if (c == "Watch folder...")
      {
        QCOMPARE(panel.watchButton()->text(), QString("Watch folder..."));
        QVERIFY2(step.text.contains("Press Watch folder."), qPrintable(what));
      }
      else if (c == "Show effects in 3D view")
      {
        // a button that is on or off (with a tick when on), not a tick box: "press", not "tick"
        QCOMPARE(panel.effectsBox()->text(), QString("Show effects in 3D view"));
        QVERIFY(panel.effectsBox()->isCheckable());
        QVERIFY2(step.text.startsWith("Press Show effects in 3D view, so it shows a tick."), qPrintable(what));
      }
      else if (c == "Live scripts")
      {
        QCOMPARE(panel.tabs()->tabText(ScriptLabPanel::LivePage), QString("Live scripts"));
        QCOMPARE(int(ScriptLabPanel::LivePage), 0);   // "its first tab"
        QVERIFY2(step.text.contains("Live scripts is its first tab."), qPrintable(what));
      }
      else if (c == "Touch" || c == "Channel" || c == "Say")
      {
        // in the folding part Send input, low on the Live scripts page and folded at first: the step says to open it
        ScriptLabSection* input = panel.section("input");
        QVERIFY(input);
        QCOMPARE(input->title(), QString("Send input"));
        QVERIFY(!input->isExpanded());
        QVERIFY(panel.tabs()->widget(ScriptLabPanel::LivePage)->isAncestorOf(input));
        if (c == "Touch")
        {
          QPushButton* touch = panel.findChild<QPushButton*>("scriptLabTouch");
          QVERIFY(touch);
          QCOMPARE(touch->text(), QString("Touch"));
          QVERIFY(input->isAncestorOf(touch));
          QVERIFY2(step.text.startsWith("Open Send input, low on Live scripts, and press Touch."), qPrintable(what));
        }
        else if (c == "Channel")
        {
          bool channel = false, text = false;
          for (QLabel* l : input->findChildren<QLabel*>()) { channel |= l->text() == "Channel"; text |= l->text() == "Text"; }
          QVERIFY(channel && text);
          QVERIFY2(step.text.startsWith("Open Send input."), qPrintable(what));
          QVERIFY2(step.text.contains("into Channel") && step.text.contains("into Text"), qPrintable(what));
        }
        else
        {
          bool say = false;
          for (QPushButton* b : input->findChildren<QPushButton*>()) say |= b->text() == "Say";
          QVERIFY(say);
        }
      }
      else if (c == "Props")
      {
        // the Props tab of the Joints panel in the window
        QVERIFY(ui.contains("<string>Props</string>"));
        QVERIFY(ui.contains("name=\"newBoxPropButton\""));
        QVERIFY2(step.text.contains("Props"), qPrintable(what));
      }
      else
      {
        // a button of the example's own dialog (llDialog in its scripts)
        bool found = false;
        for (const QString& file : QDir(examplesDir() + "/" + object).entryList({"*.lsl"}, QDir::Files))
          found |= readFile(examplesDir() + "/" + object + "/" + file).contains("\"" + c + "\"");
        for (const QString& sub : QDir(examplesDir() + "/" + object).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
          for (const QString& file : QDir(examplesDir() + "/" + object + "/" + sub).entryList({"*.lsl"}, QDir::Files))
            found |= readFile(examplesDir() + "/" + object + "/" + sub + "/" + file).contains("\"" + c + "\"");
        QVERIFY2(found, qPrintable(what));
      }
      // no step tells the user to "tick" a button, and none points at text that is not on screen
      QVERIFY2(!step.text.contains("Tick "), qPrintable(what));
      QVERIFY2(!step.text.contains("shown below"), qPrintable(what));
      ++checked;
    }
  }
  QVERIFY(checked >= 20);
}

QTEST_MAIN(TestScriptLabBeginner)
#include "tst_scriptlabbeginner.moc"
