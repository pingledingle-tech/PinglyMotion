/*
 * PinglyMotion Script Lab - the player (job B3): exact tests
 *
 * The moments of a replay and what is true at each (red locked, green free, with words), the lines,
 * the log up to a moment, the play buttons and when each can be clicked (with the reason when not),
 * the list of moments and the keyboard, playing to the end, first difference, and in the real panel:
 * the player in Beginner mode, the log in Expert mode, and the props' colours and places that follow
 * the chosen moment, back to now with Show now or a click on a prop.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "labplayer.h"
#include "labtint.h"
#include "rlvstate.h"
#include "scriptlabpanel.h"
#include "labwatcher.h"
#include "theme.h"
#include "transportbar.h"
#include "usersettings.h"

namespace
{
  bool writeFile(const QString& path, const QString& text)
  {
    QDir().mkpath(QFileInfo(path).path());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return f.write(text.toUtf8()) >= 0;
  }

  QJsonObject fact(const QString& key, const QString& text) { return QJsonObject{{"key", key}, {"text", text}}; }

  // a replay as LabWatcher::lastReplay gives it: the start and three steps
  QJsonObject fakeReplay(const QJsonValue& firstDifference = QJsonValue(), bool first = false)
  {
    QJsonArray log;
    log.append(QJsonObject{{"type", "prim"}, {"op", "color"}, {"link", 2}, {"face", -1}, {"color", QJsonArray{0, 1, 0}}, {"prim", "Cuff"}, {"t", 0}});
    log.append(QJsonObject{{"type", "step"}, {"step", 1}, {"text", "touch Root"}, {"t", 10}, {"source", "recipe"}});
    log.append(QJsonObject{{"type", "owner_say"}, {"text", "Ready."}, {"prim", "Root"}, {"script", "hud"}, {"t", 10}});
    log.append(QJsonObject{{"type", "step"}, {"step", 2}, {"text", "menu Lock"}, {"t", 11}, {"source", "recipe"}});
    log.append(QJsonObject{{"type", "prim"}, {"op", "color"}, {"link", 2}, {"face", -1}, {"color", QJsonArray{1, 0, 0}}, {"prim", "Cuff"}, {"t", 11}});
    log.append(QJsonObject{{"type", "step"}, {"step", 3}, {"text", "menu Free"}, {"t", 12}, {"source", "recipe"}});
    QJsonArray frames;
    frames.append(QJsonObject{{"step", 0}, {"t", 10}, {"log_end", 1}, {"done", true}, {"text", "Start."},
                              {"facts", QJsonArray{fact("colour:Cuff", "Cuff is green.")}}});
    frames.append(QJsonObject{{"step", 1}, {"t", 11}, {"log_end", 3}, {"done", true}, {"text", "Click Root."},
                              {"facts", QJsonArray{fact("colour:Cuff", "Cuff is green."), fact("menu", "The menu is open: \"Cuffs\" with Lock, Free.")}}});
    frames.append(QJsonObject{{"step", 2}, {"t", 12}, {"log_end", 5}, {"done", true}, {"text", "Press \"Lock\" in the menu."},
                              {"facts", QJsonArray{fact("lock:Cuffs", "Cuffs is locked."), fact("blocked:sendim", "Send instant messages is blocked."),
                                                   fact("colour:Cuff", "Cuff is red.")}}});
    frames.append(QJsonObject{{"step", 3}, {"t", 13}, {"log_end", 6}, {"done", false}, {"text", "Press \"Free\" in the menu."},
                              {"facts", QJsonArray{fact("step:3", "Step 3 (menu Free) is not done: no menu was open."), fact("lock:Cuffs", "Cuffs is locked."),
                                                   fact("colour:Cuff", "Cuff is red.")}}});
    return QJsonObject{{"ran", true}, {"steps", 3}, {"done", 2}, {"first", first}, {"log", log}, {"frames", frames},
                       {"first_difference", firstDifference}};
  }

  QStringList rowTexts(const QListWidget* list)
  {
    QStringList out;
    for (int i = 0; i < list->count(); ++i) out << list->item(i)->text();
    return out;
  }

  // the object "Cuffs": a HUD in the root that shows a menu, a cuff that locks, turns red and moves
  const char* Hud =
    "default {\n"
    "  state_entry() { llListen(-77, \"\", llGetOwner(), \"\"); llOwnerSay(\"Ready.\"); }\n"
    "  touch_start(integer n) { llDialog(llDetectedKey(0), \"Cuffs\", [\"Lock\", \"Free\"], -77); }\n"
    "  listen(integer c, string name, key id, string m) { llMessageLinked(LINK_ALL_OTHERS, 1, m, NULL_KEY); }\n"
    "}\n";

  QString cuff(bool lock)
  {
    return QString("default {\n"
                   "  state_entry() { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); }\n"
                   "  link_message(integer s, integer n, string m, key id) {\n"
                   "    if (m == \"Lock\") { llSetColor(<1.0, 0.0, 0.0>, ALL_SIDES); llSetPos(<0.5, 0.0, 0.0>); %1llOwnerSay(\"Locked.\"); }\n"
                   "    else { llSetColor(<0.0, 1.0, 0.0>, ALL_SIDES); llOwnerSay(\"@detach=y\"); llOwnerSay(\"Free.\"); }\n"
                   "  }\n"
                   "}\n").arg(lock ? "llOwnerSay(\"@detach=n\"); " : "");
  }

  const char* Recipe = "touch Root\nmenu Lock\nsend @sendim=n\n";

  // a scene the panel can make props in (as tests/scriptlabbeginner)
  struct FakeScene
  {
    QStringList props;
    QMap<QString, LabExample::Prop> places;
    ScriptLabPanel::SceneHooks hooks()
    {
      ScriptLabPanel::SceneHooks h;
      h.addProp = [this](const LabExample::Prop& p) { if (!props.contains(p.name)) { props << p.name; places.insert(p.name, p); } return true; };
      h.propPlace = [this](const QString& name, LabExample::Prop& out) { if (!places.contains(name)) return false; out = places.value(name); return true; };
      h.placeProp = [this](const LabExample::Prop& p) { if (!places.contains(p.name)) return false; places[p.name] = p; return true; };
      h.hasProp = [this](const QString& name) { return props.contains(name); };
      h.selectProp = [](const QString&) { return true; };
      h.showPropsTab = []() {};
      return h;
    }
  };

  // the colour the panel gives the prop of that name: "red", "green", or "" for none
  QString tintOf(const ScriptLabPanel& panel, const QString& prim)
  {
    for (const LabTint::Tint& t : panel.propTints())
      if (t.prim == prim && t.hasColour)
        return t.colour[0] > 0.5f ? QStringLiteral("red") : t.colour[1] > 0.5f ? QStringLiteral("green") : QStringLiteral("other");
    return QString();
  }
}

class TestScriptLabPlayer : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void momentsFromReplay();
    void rowsAtEachMoment();
    void folderAtMoment();
    void logUpToMoment();
    void buttonsAndReasons();
    void stepAndJump();
    void playToTheEnd();
    void firstDifferenceButton();
    void keyboard();
    void emptyPlayer();
    void panelFollowsTheMoment();
    void textsInRegister();

  private:
    QTemporaryDir m_settingsDir;
};

void TestScriptLabPlayer::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestScriptLabPlayer::momentsFromReplay()
{
  const QVector<LabPlayerModel::Moment> m = LabPlayerModel::moments(fakeReplay());
  QCOMPARE(m.size(), 4);
  QCOMPARE(m.at(0).step, 0);
  QCOMPARE(m.at(2).step, 2);
  QCOMPARE(m.at(2).t, 12.0);
  QCOMPARE(m.at(2).logEnd, 5);
  QVERIFY(m.at(2).done);
  QVERIFY(!m.at(3).done);
  QCOMPARE(m.at(2).facts.size(), 3);
  QCOMPARE(m.at(2).facts.at(0), (LabPlayerModel::Fact{"lock:Cuffs", "Cuffs is locked."}));
  QCOMPARE(LabPlayerModel::momentTitle(m.at(0)), QString("Start"));
  QCOMPARE(LabPlayerModel::momentTitle(m.at(2)), QString("Step 2: Press \"Lock\" in the menu."));
  QCOMPARE(LabPlayerModel::momentTitle(m.at(3)), QString("Step 3: Press \"Free\" in the menu. Not done."));
  QCOMPARE(LabPlayerModel::positionLine(m, 0), QString("Start, at 10 s."));
  QCOMPARE(LabPlayerModel::positionLine(m, 2), QString("Step 2 of 3, at 12 s: Press \"Lock\" in the menu."));
  QCOMPARE(LabPlayerModel::positionLine(m, 3), QString("Step 3 of 3, at 13 s: Press \"Free\" in the menu. It could not be done."));
  QCOMPARE(LabPlayerModel::positionLine({}, 0), QString("No replay yet. Press Record what I do, click your props, then stop recording."));
  // first difference: a moment of the replay, else -1
  QCOMPARE(LabPlayerModel::firstDifference(fakeReplay()), -1);
  QCOMPARE(LabPlayerModel::firstDifference(fakeReplay(2)), 2);
  QCOMPARE(LabPlayerModel::firstDifference(fakeReplay(4)), -1);
  QCOMPARE(LabPlayerModel::firstDifference(fakeReplay(-1)), -1);
  QCOMPARE(LabPlayerModel::moments(QJsonObject()).size(), 0);
}

void TestScriptLabPlayer::rowsAtEachMoment()
{
  using LabPlayerModel::Look;
  using LabPlayerModel::Row;
  const QVector<LabPlayerModel::Moment> m = LabPlayerModel::moments(fakeReplay());
  // the start: the cuff is free (it is locked later), instant messages allowed (blocked later)
  QCOMPARE(LabPlayerModel::rows(m, 0), (QVector<Row>{{"Free: Cuffs", Look::Free, false},
                                                     {"Allowed: Send instant messages", Look::Allowed, false},
                                                     {"Cuff is green.", Look::Plain, false}}));
  // step 1: the menu opened (changed here)
  QCOMPARE(LabPlayerModel::rows(m, 1), (QVector<Row>{{"Free: Cuffs", Look::Free, false},
                                                     {"Allowed: Send instant messages", Look::Allowed, false},
                                                     {"Cuff is green.", Look::Plain, false},
                                                     {"The menu is open: \"Cuffs\" with Lock, Free.", Look::Plain, true}}));
  // step 2: locked and blocked in red, the new colour
  QCOMPARE(LabPlayerModel::rows(m, 2), (QVector<Row>{{"Locked: Cuffs", Look::Locked, true},
                                                     {"Blocked: Send instant messages", Look::Blocked, true},
                                                     {"Cuff is red.", Look::Plain, true}}));
  // step 3: not done; the block is gone, so the action is allowed again
  QCOMPARE(LabPlayerModel::rows(m, 3), (QVector<Row>{{"Locked: Cuffs", Look::Locked, false},
                                                     {"Allowed: Send instant messages", Look::Allowed, true},
                                                     {"Step 3 (menu Free) is not done: no menu was open.", Look::Problem, true},
                                                     {"Cuff is red.", Look::Plain, false}}));
  // nothing true: one plain line
  LabPlayerModel::Moment empty;
  QCOMPARE(LabPlayerModel::rows({empty}, 0), (QVector<Row>{{"Nothing locked, coloured or said yet.", Look::Plain, false}}));
  QCOMPARE(LabPlayerModel::rows(m, 9).size(), 0);
}

void TestScriptLabPlayer::folderAtMoment()
{
  const QJsonObject replay = fakeReplay();
  const QJsonObject folder{{"ok", true}, {"linkset", QJsonObject{{"prims", QJsonArray{QJsonObject{{"name", "Root"}, {"link", 1}, {"folder", "Root"}},
                                                                                     QJsonObject{{"name", "Cuff"}, {"link", 2}, {"folder", "Cuff"}}}}}},
                           {"result", QJsonObject{{"log", QJsonArray()}}}};
  const QJsonArray log = replay.value("log").toArray();
  const QJsonObject atStart = LabPlayerModel::folderAt(folder, log, 1);
  QCOMPARE(atStart.value("linkset"), folder.value("linkset"));
  QCOMPARE(atStart.value("result").toObject().value("log").toArray().size(), 1);
  const QList<LabTint::Tint> green = LabTint::fromFolder(atStart);
  QCOMPARE(green.size(), 1);
  QCOMPARE(green.first().prim, QString("Cuff"));
  QCOMPARE(green.first().colour[1], 1.0f);
  const QList<LabTint::Tint> red = LabTint::fromFolder(LabPlayerModel::folderAt(folder, log, 5));
  QCOMPARE(red.first().colour[0], 1.0f);
  QCOMPARE(red.first().colour[1], 0.0f);
  // kept inside the log; a folder that did not run gives nothing
  QCOMPARE(LabPlayerModel::folderAt(folder, log, 99).value("result").toObject().value("log").toArray().size(), log.size());
  QCOMPARE(LabPlayerModel::folderAt(folder, log, -3).value("result").toObject().value("log").toArray().size(), 0);
  QVERIFY(LabPlayerModel::folderAt(QJsonObject{{"ok", false}}, log, 3).isEmpty());
}

void TestScriptLabPlayer::logUpToMoment()
{
  const QJsonArray log = fakeReplay().value("log").toArray();
  QCOMPARE(LabPlayerModel::logLines(log, 3).size(), 3);
  QCOMPARE(LabPlayerModel::logLines(log, 3).at(1), QString("--- Step 1: touch Root ---"));
  QCOMPARE(LabPlayerModel::logLines(log, 3).at(2), QString("10.000 s  hud (Root): owner_say: Ready."));
  QCOMPARE(LabPlayerModel::logLines(log, 0), QStringList());
}

void TestScriptLabPlayer::buttonsAndReasons()
{
  LabPlayer player;
  player.setReplay(fakeReplay());
  TransportBar* bar = player.transportBar();
  // the timeline's buttons, with words for steps; no Loop switch for a replay
  QCOMPARE(bar->startButton()->text(), QString("Start"));
  QCOMPARE(bar->stopButton()->text(), QString("Stop"));
  QCOMPARE(bar->replayButton()->text(), QString("Replay from start"));
  QCOMPARE(bar->backButton()->text(), QString("Step back"));
  QCOMPARE(bar->forwardButton()->text(), QString("Step forward"));
  QVERIFY(bar->loopCheckBox()->isHidden());
  // a new replay: on the last moment, the 3D view shows now
  QCOMPARE(player.momentCount(), 4);
  QCOMPARE(player.current(), 3);
  QVERIFY(!player.showing());
  QVERIFY(!player.playing());
  QVERIFY(bar->startButton()->isEnabled());
  QVERIFY(!bar->stopButton()->isEnabled());
  QCOMPARE(bar->stopButton()->toolTip(), QString("It is not playing. Click Start to play it."));
  QVERIFY(bar->replayButton()->isEnabled());
  QVERIFY(bar->backButton()->isEnabled());
  QVERIFY(!bar->forwardButton()->isEnabled());
  QCOMPARE(bar->forwardButton()->toolTip(), QString("This is the last step. Click Replay from start to see it again."));
  QCOMPARE(bar->backButton()->toolTip(), QString("Go to the step before"));
  QVERIFY(!player.nowButton()->isEnabled());
  QCOMPARE(player.nowButton()->toolTip(), QString("The 3D view already shows now."));
  QVERIFY(!player.firstDifferenceButton()->isEnabled());
  QCOMPARE(player.firstDifferenceButton()->toolTip(), QString("Every step gave the same as the replay before."));
  QCOMPARE(player.viewLabel()->text(), QString("The props in the 3D view show now. Pick a moment to see it there."));
  QCOMPARE(player.positionLabel()->text(), QString("Step 3 of 3, at 13 s: Press \"Free\" in the menu. It could not be done."));
  QCOMPARE(rowTexts(player.momentList()), QStringList({"Start", "Step 1: Click Root.", "Step 2: Press \"Lock\" in the menu.",
                                                       "Step 3: Press \"Free\" in the menu. Not done."}));
  QCOMPARE(player.momentList()->currentRow(), 3);
  // what is true now, with words, in red and green (charter P10: never by colour alone)
  QCOMPARE(rowTexts(player.stateList()), QStringList({"Locked: Cuffs", "Allowed: Send instant messages (changed at this step)",
                                                      "Step 3 (menu Free) is not done: no menu was open. (changed at this step)",
                                                      "Cuff is red."}));
  QCOMPARE(player.stateList()->item(0)->foreground().color(), Theme::color(Theme::Token::Danger, true));
  QCOMPARE(player.stateList()->item(1)->foreground().color(), Theme::color(Theme::Token::Success, true));
  QCOMPARE(player.stateList()->item(2)->foreground().color(), Theme::color(Theme::Token::Warning, true));
  QCOMPARE(player.stateList()->item(3)->foreground().color(), Theme::color(Theme::Token::Text, true));
  QVERIFY(player.stateList()->item(1)->font().bold());
  QVERIFY(!player.stateList()->item(0)->font().bold());
  QVERIFY(!player.stateList()->item(0)->icon().isNull());
  // the two buttons are normal buttons in the design system's colours, with their icons
  const QString css = player.nowButton()->styleSheet();
  QVERIFY2(css.contains("QPushButton { background: " + Theme::color(Theme::Token::SurfaceRaised, true).name().toUpper() + "; color: "
                        + Theme::color(Theme::Token::Text, true).name().toUpper() + "; border: 1px solid "
                        + Theme::color(Theme::Token::Border, true).name().toUpper() + "; border-radius: 6px; padding: 0 12px; }"), qPrintable(css));
  QVERIFY(css.contains("QPushButton:focus { border: 2px solid " + Theme::color(Theme::Token::FocusRing, true).name().toUpper() + "; }"));
  QCOMPARE(player.firstDifferenceButton()->styleSheet(), css);
  QVERIFY(!player.nowButton()->icon().isNull());
  QVERIFY(!player.firstDifferenceButton()->icon().isNull());
  QCOMPARE(player.nowButton()->minimumHeight(), Theme::Size::controlCompact);
  // a first replay: nothing to compare with
  player.setReplay(fakeReplay(QJsonValue(), true));
  QCOMPARE(player.firstDifferenceButton()->toolTip(), QString("This is the first replay, so there is nothing to compare with."));
}

void TestScriptLabPlayer::stepAndJump()
{
  LabPlayer player;
  player.setReplay(fakeReplay());
  QSignalSpy changed(&player, &LabPlayer::momentChanged);
  TransportBar* bar = player.transportBar();
  bar->backButton()->click();
  QCOMPARE(player.current(), 2);
  QVERIFY(player.showing());
  QCOMPARE(changed.count(), 1);
  QVERIFY(bar->forwardButton()->isEnabled());
  QVERIFY(player.nowButton()->isEnabled());
  QCOMPARE(player.nowButton()->toolTip(), QString("Show the props as they are now, not at this moment"));
  QCOMPARE(player.viewLabel()->text(), QString("The props in the 3D view show this moment. Press Show now to see now again."));
  QCOMPARE(player.positionLabel()->text(), QString("Step 2 of 3, at 12 s: Press \"Lock\" in the menu."));
  QCOMPARE(rowTexts(player.stateList()).first(), QString("Locked: Cuffs (changed at this step)"));
  bar->backButton()->click();
  bar->backButton()->click();
  QCOMPARE(player.current(), 0);
  QVERIFY(!bar->backButton()->isEnabled());
  QCOMPARE(bar->backButton()->toolTip(), QString("This is the start. There is no step before it."));
  // step back at the start stays at the start
  player.stepBy(-1);
  QCOMPARE(player.current(), 0);
  QCOMPARE(player.positionLabel()->text(), QString("Start, at 10 s."));
  bar->forwardButton()->click();
  QCOMPARE(player.current(), 1);
  // a pick in the list jumps there
  player.momentList()->setCurrentRow(3);
  QCOMPARE(player.current(), 3);
  QVERIFY(player.showing());
  // Show now: the 3D view shows now, the moment stays chosen
  player.nowButton()->click();
  QVERIFY(!player.showing());
  QCOMPARE(player.current(), 3);
  QVERIFY(!player.nowButton()->isEnabled());
  // the same replay again changes nothing; a new one starts on its last moment, showing now
  player.showMoment(1);
  player.setReplay(fakeReplay());
  QCOMPARE(player.current(), 1);
  QVERIFY(player.showing());
  player.setReplay(fakeReplay(1));
  QCOMPARE(player.current(), 3);
  QVERIFY(!player.showing());
}

void TestScriptLabPlayer::playToTheEnd()
{
  LabPlayer player;
  player.setPlayInterval(5);
  player.setReplay(fakeReplay());
  TransportBar* bar = player.transportBar();
  QCOMPARE(int(LabPlayer::PLAY_INTERVAL_MS), 1000);
  // Start on the last moment begins at the start, as the timeline does
  bar->startButton()->click();
  QVERIFY(player.playing());
  QCOMPARE(player.current(), 0);
  QVERIFY(player.showing());
  QVERIFY(!bar->startButton()->isEnabled());
  QCOMPARE(bar->startButton()->toolTip(), QString("It is already playing. Click Stop to stop it."));
  QVERIFY(bar->stopButton()->isEnabled());
  QTRY_VERIFY(!player.playing());
  QCOMPARE(player.current(), 3);
  QVERIFY(bar->startButton()->isEnabled());
  // Stop stops on the moment it is on
  player.showMoment(0);
  player.setPlayInterval(60000);
  player.play();
  QVERIFY(player.playing());
  bar->stopButton()->click();
  QVERIFY(!player.playing());
  QCOMPARE(player.current(), 0);
  // a step while playing stops playing
  player.play();
  bar->forwardButton()->click();
  QVERIFY(!player.playing());
  QCOMPARE(player.current(), 1);
  // Replay from start: the start, playing
  bar->replayButton()->click();
  QCOMPARE(player.current(), 0);
  QVERIFY(player.playing());
  player.stop();
}

void TestScriptLabPlayer::firstDifferenceButton()
{
  LabPlayer player;
  player.setReplay(fakeReplay(2));
  QVERIFY(player.firstDifferenceButton()->isEnabled());
  QCOMPARE(player.firstDifferenceButton()->toolTip(), QString("Go to step 2, where this replay first differed"));
  QCOMPARE(player.momentList()->item(2)->text(), QString("Step 2: Press \"Lock\" in the menu. First difference."));
  player.firstDifferenceButton()->click();
  QCOMPARE(player.current(), 2);
  QVERIFY(player.showing());
  player.setReplay(fakeReplay(0));
  QCOMPARE(player.firstDifferenceButton()->toolTip(), QString("Go to the start: it already differed from the replay before"));
  QVERIFY(player.goToFirstDifference());
  QCOMPARE(player.current(), 0);
  player.setReplay(fakeReplay());
  QVERIFY(!player.goToFirstDifference());
  QCOMPARE(player.current(), 3);
}

void TestScriptLabPlayer::keyboard()
{
  LabPlayer player;
  player.setReplay(fakeReplay());
  player.show();
  QVERIFY(QTest::qWaitForWindowExposed(&player));
  // the list takes the focus with Tab; the arrow keys move one step (charter P8)
  QVERIFY(player.momentList()->focusPolicy() & Qt::TabFocus);
  QVERIFY(player.firstDifferenceButton()->focusPolicy() & Qt::TabFocus);
  QVERIFY(player.nowButton()->focusPolicy() & Qt::TabFocus);
  QVERIFY(player.transportBar()->startButton()->focusPolicy() & Qt::TabFocus);
  player.momentList()->setFocus();
  QTest::keyClick(player.momentList(), Qt::Key_Up);
  QCOMPARE(player.current(), 2);
  QVERIFY(player.showing());
  QTest::keyClick(player.momentList(), Qt::Key_Home);
  QCOMPARE(player.current(), 0);
  QTest::keyClick(player.momentList(), Qt::Key_Down);
  QCOMPARE(player.current(), 1);
  // every control has a name or a tip (charter P3)
  for (QPushButton* b : player.findChildren<QPushButton*>())
    QVERIFY2(!b->text().isEmpty() && !b->toolTip().isEmpty(), qPrintable(b->objectName()));
  QVERIFY(!player.momentList()->accessibleName().isEmpty());
  QVERIFY(!player.stateList()->accessibleName().isEmpty());
}

void TestScriptLabPlayer::emptyPlayer()
{
  LabPlayer player;
  QCOMPARE(player.momentCount(), 0);
  QCOMPARE(player.positionLabel()->text(), QString("No replay yet. Press Record what I do, click your props, then stop recording."));
  for (QPushButton* b : {player.transportBar()->startButton(), player.transportBar()->stopButton(), player.transportBar()->replayButton(),
                         player.transportBar()->backButton(), player.transportBar()->forwardButton()})
  {
    QVERIFY(!b->isEnabled());
    QCOMPARE(b->toolTip(), QString("No replay yet. Record what you do first."));
  }
  QVERIFY(!player.firstDifferenceButton()->isEnabled());
  QCOMPARE(player.firstDifferenceButton()->toolTip(), QString("No replay yet. Record what you do first."));
  player.play();
  player.stepBy(1);
  QVERIFY(!player.playing());
  QVERIFY(!player.showing());
  // with nothing to step in, the folder is the one given
  const QJsonObject folder{{"ok", true}, {"x", 1}};
  QCOMPARE(player.folderAt(folder), folder);
}

void TestScriptLabPlayer::panelFollowsTheMoment()
{
  QTemporaryDir dir;
  const QString top = dir.filePath("Cuffs");
  QVERIFY(writeFile(top + "/Root/hud.lsl", Hud));
  QVERIFY(writeFile(top + "/Cuff/cuff.lsl", cuff(true)));
  QVERIFY(writeFile(top + "/recipe.rlvtest", Recipe));
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  FakeScene scene;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.setScriptsRoot(dir.path());
  panel.setSceneHooks(scene.hooks());
  panel.setExpertMode(false);
  panel.resize(700, 1200);
  panel.show();
  LabPlayer* player = panel.player();
  QVERIFY(player);
  QVERIFY(!player->isVisibleTo(&panel));   // nothing watched: no player
  // watching runs the folder, and the run replays the recipe: the player has the start and 3 steps
  QVERIFY(panel.watchFolder(top));
  QVERIFY(player->isVisibleTo(&panel));    // Beginner mode shows the player (first meeting)
  QCOMPARE(player->momentCount(), 4);
  QVERIFY(!player->logView()->isVisibleTo(&panel));   // the log only in Expert mode (charter P5)
  QCOMPARE(rowTexts(player->momentList()), QStringList({"Start", "Step 1: Click Root.", "Step 2: Press \"Lock\" in the menu.",
                                                        "Step 3: panel sends @sendim=n."}));
  QCOMPARE(rowTexts(player->stateList()), QStringList({"Locked: Cuffs", "Blocked: Send instant messages (changed at this step)",
                                                       "Cuff is red.", "Root is saying \"Ready.\".", "Cuff is saying \"Locked.\"."}));

  // the props: shown with the effects; now the cuff is green (the live run did not lock it)
  panel.effectsBox()->setChecked(true);
  QVERIFY(scene.props.contains("Cuff"));
  QCOMPARE(tintOf(panel, "Cuff"), QString("green"));
  QVERIFY(panel.primMoves().isEmpty());
  // the moment after "menu Lock": red, and moved to where the script put it
  player->showMoment(2);
  QCOMPARE(tintOf(panel, "Cuff"), QString("red"));
  QCOMPARE(panel.primMoves().size(), 1);
  QCOMPARE(panel.primMoves().first().prim, QString("Cuff"));
  QVERIFY(panel.primMoves().first().hasPosition);
  QCOMPARE(rowTexts(player->stateList()), QStringList({"Locked: Cuffs (changed at this step)", "Allowed: Send instant messages",
                                                       "Cuff is red. (changed at this step)", "Root is saying \"Ready.\".",
                                                       "Cuff is saying \"Locked.\". (changed at this step)"}));
  // step back: green again, back where it stood
  player->transportBar()->backButton()->click();
  QCOMPARE(player->current(), 1);
  QCOMPARE(tintOf(panel, "Cuff"), QString("green"));
  QVERIFY(panel.primMoves().isEmpty());
  QCOMPARE(rowTexts(player->stateList()), QStringList({"Free: Cuffs", "Allowed: Send instant messages", "Cuff is green.",
                                                       "Root is saying \"Ready.\".",
                                                       "The menu is open: \"Cuffs\" with Lock, Free. (changed at this step)"}));
  // Show now: the props show the live run again
  player->showMoment(3);
  QCOMPARE(tintOf(panel, "Cuff"), QString("red"));
  player->nowButton()->click();
  QCOMPARE(tintOf(panel, "Cuff"), QString("green"));
  // a click on a prop is an input to now: the props show now first
  player->showMoment(2);
  QVERIFY(player->showing());
  QVERIFY(panel.touchPrim("Root"));
  QVERIFY(!player->showing());
  QCOMPARE(tintOf(panel, "Cuff"), QString("green"));

  // Expert mode adds the log up to the moment; the player does not move (charter P12)
  const QPoint at = player->mapTo(&panel, QPoint(0, 0));
  panel.setExpertMode(true);
  QCOMPARE(player->mapTo(&panel, QPoint(0, 0)).y() >= at.y(), true);
  QVERIFY(player->logView()->isVisibleTo(&panel));
  player->showMoment(1);
  const QStringList log = player->logView()->toPlainText().split('\n');
  QCOMPARE(log.last(), QString("10.000 s  hud (Root): dialog: buttons=Lock, Free, channel=-77, message=Cuffs, object=Cuffs, "
                               "to=a2e76fcd-9360-4f6d-a924-000000000001"));
  QVERIFY(log.contains("--- Step 1: touch Root ---"));
  QVERIFY(!log.contains("--- Step 2: menu Lock ---"));
  panel.setExpertMode(false);

  // a save of a changed script: the cuff no longer locks; the player can jump to step 2
  QVERIFY(writeFile(top + "/Cuff/cuff.lsl", cuff(false)));
  watcher.runScript("Cuff/cuff.lsl");
  QVERIFY(!player->showing());
  QVERIFY(player->firstDifferenceButton()->isEnabled());
  QCOMPARE(player->momentList()->item(2)->text(), QString("Step 2: Press \"Lock\" in the menu. First difference."));
  player->firstDifferenceButton()->click();
  QCOMPARE(player->current(), 2);
  // nothing locks any more, so no lock row at all
  QCOMPARE(rowTexts(player->stateList()), QStringList({"Allowed: Send instant messages", "Cuff is red. (changed at this step)",
                                                       "Root is saying \"Ready.\".", "Cuff is saying \"Locked.\". (changed at this step)"}));
  if (!qEnvironmentVariableIsEmpty("PINGLYMOTION_UI_OUT"))   // a picture for a session to look at
  {
    QTest::qWait(50);
    panel.grab().save(qEnvironmentVariable("PINGLYMOTION_UI_OUT") + "/player.png");
  }

  // stop watching: no player
  panel.stopWatching();
  QVERIFY(!player->isVisibleTo(&panel));
}

// every text of the player is a row of docs/TEACHING.md (charter P13), with at most 12 words a sentence
void TestScriptLabPlayer::textsInRegister()
{
  QFile file(QStringLiteral(PINGLYMOTION_DOCS_DIR "/TEACHING.md"));
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QString teaching = QString::fromUtf8(file.readAll());
  QVERIFY(teaching.contains("### Script Lab: the player (job B3)"));
  const QStringList texts = {
    "Player: watch your steps again", "Step back", "Step forward", "Go to first difference", "Show now",
    "True at this moment:", "Log up to this moment:", "| Start |", "Step %1: %2", " Not done.", " First difference.",
    "Start, at %1.", "Step %1 of %2, at %3: %4", " It could not be done.",
    "No replay yet. Press Record what I do, click your props, then stop recording.", "%1 s",
    "Locked: %1", "Free: %1", "Blocked: %1", "Allowed: %1", "%1 (changed at this step)", "Nothing locked, coloured or said yet.",
    "The props in the 3D view show this moment. Press Show now to see now again.",
    "The props in the 3D view show now. Pick a moment to see it there.",
    "Pick a moment to see what was true then. The arrow keys move one step.",
    "Play the steps one by one, one each second", "Stop playing; the player stays on this step",
    "Go back to the start and play the steps", "Go to the step before", "Go to the next step",
    "This is the start. There is no step before it.", "This is the last step. Click Replay from start to see it again.",
    "No replay yet. Record what you do first.", "Go to step %1, where this replay first differed",
    "Go to the start: it already differed from the replay before", "This is the first replay, so there is nothing to compare with.",
    "Every step gave the same as the replay before.", "Show the props as they are now, not at this moment",
    "The 3D view already shows now.", "--- Step %1: %2 ---"};
  for (const QString& t : texts)
  {
    QVERIFY2(teaching.contains(t), qPrintable(t));
    for (const QString& sentence : t.split(QRegularExpression("[.!?:;](\\s|$)"), Qt::SkipEmptyParts))
      QVERIFY2(sentence.split(' ', Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
  }
}

QTEST_MAIN(TestScriptLabPlayer)
#include "tst_scriptlabplayer.moc"
