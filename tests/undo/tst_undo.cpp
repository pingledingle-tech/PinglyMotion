/*
 * PinglyMotion - test of Undo and Redo for the animation editing (job B1)
 *
 * Exact answers only (src/undohistory.*, Animation::snapshot() and restore()):
 * after each kind of edit Undo brings back exactly the animation data from
 * before (every key of every joint with its easing and curve, the frame count,
 * the speed and the loop) and Redo brings back exactly the data after it; a
 * new edit clears Redo; at most 100 steps; one drag is one step; the code that
 * opens a file is no step; each animation has its own history; the history
 * survives Save and Undo back to the saved state takes the unsaved mark away;
 * the menu texts, the keys (Ctrl+Z, Ctrl+Shift+Z, Ctrl+Y, also as a Norwegian
 * keyboard sends them), a text field that keeps Ctrl+Z for itself, the grey
 * state and its reasons, the status line texts, the window wiring and the
 * mockup (its Undo texts and its rule that hides the Expert-only buttons).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QAction>
#include <QFile>
#include <QLineEdit>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"
#include "menuspec.h"
#include "timelineclip.h"
#include "undohistory.h"
#include "../windowsource.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }
  QString source(const char* name) { if (QLatin1String(name) == QLatin1String("pinglymotionwindow.cpp")) return WindowSource::read(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp")); return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/") + QLatin1String(name)); }
  QString doc(const char* name) { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/") + QLatin1String(name)); }

  // a new animation (one key per joint at frame 0), 40 frames, Mirror off
  struct Fixture
  {
    BVH bvh;
    Animation anim;
    QAction undoAction{QStringLiteral("Undo")};
    QAction redoAction{QStringLiteral("Redo")};
    UndoController undo{&undoAction, &redoAction};
    bool dragging = false;

    Fixture() : anim(&bvh)
    {
      Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
      anim.setNumberOfFrames(40);
      anim.setLoopOutPoint(39);
      anim.setLoopInPoint(0);
      anim.setMirrored(false);
      anim.setDirty(false);
      undo.setBusyCheck([this]() { return dragging; });
      undo.setAnimation(&anim);
      // the history starts once the opening code is done
      QCoreApplication::processEvents();
    }
    int track(const char* name) { return TimelineClip::trackOf(&anim, QLatin1String(name)); }
    BVHNode* node(const char* name) { return anim.getNode(track(name)); }
    void turn(const char* joint, int frame, double x, double y = 0, double z = 0)
    {
      anim.setFrame(frame);
      anim.setRotation(node(joint), x, y, z);
    }
    // the step of an edit is taken once the edit is over (the next turn of the event loop)
    void settle() { QCoreApplication::processEvents(); }
  };

  QByteArray describe(const Animation::Snapshot& a, const Animation::Snapshot& b)
  {
    if (a.frames != b.frames) return QByteArray("frames ") + QByteArray::number(a.frames) + " != " + QByteArray::number(b.frames);
    if (a.fps != b.fps) return QByteArray("fps differs");
    if (a.loopIn != b.loopIn || a.loopOut != b.loopOut) return QByteArray("loop differs");
    if (a.keys.count() != b.keys.count()) return QByteArray("part count differs");
    for (int i = 0; i < a.keys.count(); ++i)
      if (a.keys[i] != b.keys[i]) return QByteArray("keys of part ") + QByteArray::number(i) + " differ";
    return QByteArray("same");
  }
}

#define COMPARE_STATE(actual, expected) \
  QVERIFY2((actual) == (expected), describe((actual), (expected)).constData())

class UndoTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
  void historyBasics();
  void historyKeepsAtMostHundredSteps();
  void snapshotRestoreKeepsJointsAndIsExact();
  void eachEditUndoesAndRedoesExactly_data();
  void eachEditUndoesAndRedoesExactly();
  void insertAndDeleteFrameMoveTheKeys();
  void newEditClearsRedo();
  void editWithoutChangeIsNoStep();
  void dragIsOneStep();
  void pullIsOneStepAndUndoWaitsForItsEnd();
  void openingCodeIsNoStep();
  void eachAnimationHasItsOwnHistory();
  void historySurvivesSave();
  void greyStateAndReasons();
  void statusTexts();
  void menuTextsAndKeys();
  void keysTriggerUndoAndRedo();
  void textFieldKeepsCtrlZ();
  void windowWiring();
  void mockupTextsAndExpertOnlyRule();
};

void UndoTest::historyBasics()
{
  BVH bvh;
  Animation anim(&bvh);
  Animation::Snapshot a = anim.snapshot();
  Animation::Snapshot b = a;
  b.fps = a.fps + 5;
  Animation::Snapshot c = a;
  c.frames = a.frames + 10;

  UndoHistory h;
  QVERIFY(!h.started());
  // the first record starts the history: nothing to undo
  QVERIFY(!h.record(a));
  QVERIFY(h.started());
  QVERIFY(!h.canUndo());
  QVERIFY(!h.canRedo());
  QVERIFY(!h.record(a));
  QVERIFY(h.record(b));
  QVERIFY(h.record(c));
  QCOMPARE(h.undoCount(), 2);
  QCOMPARE(h.redoCount(), 0);

  std::optional<Animation::Snapshot> back = h.undo();
  QVERIFY(back.has_value());
  COMPARE_STATE(*back, b);
  COMPARE_STATE(h.current(), b);
  QCOMPARE(h.undoCount(), 1);
  QCOMPARE(h.redoCount(), 1);
  back = h.undo();
  COMPARE_STATE(*back, a);
  QVERIFY(!h.undo().has_value());
  QVERIFY(!h.canUndo());
  QCOMPARE(h.redoCount(), 2);

  std::optional<Animation::Snapshot> forward = h.redo();
  COMPARE_STATE(*forward, b);
  forward = h.redo();
  COMPARE_STATE(*forward, c);
  QVERIFY(!h.redo().has_value());

  // the saved state
  QVERIFY(!h.atSaved());
  h.markSaved();
  QVERIFY(h.atSaved());
  h.undo();
  QVERIFY(!h.atSaved());
  h.redo();
  QVERIFY(h.atSaved());

  // start() begins again
  h.start(a);
  QVERIFY(!h.canUndo());
  QVERIFY(!h.canRedo());
  QVERIFY(!h.atSaved());
}

void UndoTest::historyKeepsAtMostHundredSteps()
{
  QCOMPARE(UndoHistory::maxSteps, 100);
  BVH bvh;
  Animation anim(&bvh);
  Animation::Snapshot state = anim.snapshot();
  UndoHistory h;
  h.start(state);
  for (int i = 1; i <= 120; ++i)
  {
    state.fps = i;
    QVERIFY(h.record(state));
  }
  QCOMPARE(h.undoCount(), 100);
  int steps = 0;
  while (h.undo()) steps++;
  QCOMPARE(steps, 100);
  // the oldest kept step is the state after edit 20
  QCOMPARE(h.current().fps, 20);
}

void UndoTest::snapshotRestoreKeepsJointsAndIsExact()
{
  Fixture f;
  BVHNode* thigh = f.node("lThigh");
  BVHNode* position = f.anim.getNode(0);
  QVERIFY(thigh);
  QVERIFY(position);
  const Animation::Snapshot before = f.anim.snapshot();
  // one entry per part, as getNode() counts them
  QCOMPARE(before.keys.count(), BVH::countNodes(f.anim.getMotion()) + 1);
  QCOMPARE(before.keys.at(0), position->keyframeMap());
  QCOMPARE(before.keys.at(f.track("lThigh")), thigh->keyframeMap());
  QCOMPARE(before.frames, 40);
  QCOMPARE(before.loopIn, 0);
  QCOMPARE(before.loopOut, 39);

  f.turn("lThigh", 10, 30.25, -10.5, 5.125);
  f.anim.setCurve(thigh, 10, Easing::Preset::OvershootThenSettle);
  QSignalSpy redraw(&f.anim, &Animation::redrawTrack);
  QSignalSpy frames(&f.anim, &Animation::numberOfFrames);
  f.anim.restore(before);
  // the same joint objects, so the window's pointers stay valid
  QCOMPARE(f.node("lThigh"), thigh);
  QCOMPARE(f.anim.getNode(0), position);
  COMPARE_STATE(f.anim.snapshot(), before);
  QCOMPARE(thigh->keyframeList(), QList<int>({0}));
  // only the changed joint is redrawn, and the frame count did not change
  QCOMPARE(redraw.count(), 1);
  QCOMPARE(redraw.at(0).at(0).toInt(), f.track("lThigh"));
  QCOMPARE(frames.count(), 0);

  // a snapshot of another skeleton is not put back
  Animation::Snapshot wrong = before;
  wrong.keys.removeLast();
  wrong.frames = 7;
  f.anim.restore(wrong);
  QCOMPARE(f.anim.getNumberOfFrames(), 40);
}

// every kind of edit the program has, done the way the program does it
void UndoTest::eachEditUndoesAndRedoesExactly_data()
{
  QTest::addColumn<int>("which");
  const char* names[] = {"set a key (Set key)", "remove a key", "turn a joint (rings, sliders, fields, pull)",
                         "move the Avatar Position", "paste", "paste mirrored", "cut", "mirror one joint",
                         "mirror the whole animation", "flip pose", "T-pose", "reset all to frame 1",
                         "easing tile (stored curve)", "ease in and out switches", "custom curve (Edit curve)",
                         "insert a frame", "delete a frame", "move a key on the timeline", "copy a key on the timeline",
                         "number of frames", "frames per second", "loop points", "remove extra keys (optimize)",
                         "set keys for all joints", "remove keys for all joints"};
  for (int i = 0; i < int(sizeof(names) / sizeof(names[0])); ++i) QTest::newRow(names[i]) << i;
}

void UndoTest::eachEditUndoesAndRedoesExactly()
{
  QFETCH(int, which);
  Fixture f;
  // some content to work on, as a step of its own
  f.turn("lThigh", 10, 30, 10, 5);
  f.turn("lThigh", 20, -10, 2, 1);
  f.turn("rShin", 15, 12, 0, -3);
  // a key that only repeats the pose before it (Remove extra keys takes it away)
  f.turn("lShin", 8, 0, 0, 0);
  f.anim.setFrame(10);
  f.anim.setPosition(1, 2, 3);
  f.settle();
  const int stepsBefore = f.undo.history(&f.anim)->undoCount();
  const Animation::Snapshot before = f.anim.snapshot();
  const int thigh = f.track("lThigh");
  BVHNode* thighNode = f.node("lThigh");

  switch (which)
  {
    case 0: f.anim.setFrame(25); f.anim.addKeyFrame(thighNode); break;
    case 1: f.anim.deleteKeyFrame(thighNode, 20); break;
    case 2: f.turn("lThigh", 10, 45.5, -20.25, 7.75); break;
    case 3: f.anim.setFrame(10); f.anim.setPosition(-4, 5.5, 6); break;
    case 4:
    {
      TimelineClip clip;
      clip.copy(&f.anim, TimelineRange{5, 20, {thigh}});
      clip.paste(&f.anim, 22, false);
      break;
    }
    case 5:
    {
      TimelineClip clip;
      clip.copy(&f.anim, TimelineRange{5, 20, {thigh}});
      clip.paste(&f.anim, 22, true);
      break;
    }
    case 6: TimelineClip::removeKeys(&f.anim, TimelineRange{5, 25, {thigh}}); break;
    case 7: f.anim.mirror(thighNode); break;
    case 8: f.anim.mirror(nullptr); break;
    case 9: f.anim.setFrame(10); f.anim.flipPose(); break;
    case 10: f.anim.setFrame(10); f.anim.tPose(); break;
    case 11: f.anim.setFrame(10); f.anim.resetAllToFrameZero(); break;
    case 12: f.anim.setCurve(thighNode, 10, Easing::Preset::SlowBothEnds); break;
    case 13: f.anim.setEaseIn(thighNode, 10, !f.anim.easeIn(thighNode, 10)); f.anim.setEaseOut(thighNode, 10, !f.anim.easeOut(thighNode, 10)); break;
    case 14: f.anim.setCustomCurve(thighNode, 10, Easing::Bezier{0.1, 0.9, 0.2, 1.1}); break;
    case 15: f.anim.insertFrame(thigh, 12); break;
    case 16: f.anim.deleteFrame(thigh, 12); break;
    case 17: f.anim.moveKeyFrame(thigh, 20, 30); break;
    case 18: f.anim.copyKeyFrame(thigh, 20, 30); break;
    case 19: f.anim.setNumberOfFrames(25); break;
    case 20: f.anim.setFPS(f.anim.fps() + 6); break;
    case 21: f.anim.setLoopInPoint(5); f.anim.setLoopOutPoint(30); break;
    case 22: f.anim.optimize(); break;
    case 23: f.anim.setFrame(33); f.anim.addKeyFrameAllJoints(); break;
    case 24: f.anim.setFrame(10); f.anim.deleteKeyFrameAllJoints(); break;
  }
  const Animation::Snapshot after = f.anim.snapshot();
  QVERIFY2(after != before, "the edit changed nothing");
  QVERIFY(f.anim.dirty());
  // Undo is usable at once, before the step is taken
  QVERIFY(f.undoAction.isEnabled());
  f.settle();
  QCOMPARE(f.undo.history(&f.anim)->undoCount(), stepsBefore + 1);

  QVERIFY(f.undo.undo());
  COMPARE_STATE(f.anim.snapshot(), before);
  QVERIFY(f.redoAction.isEnabled());
  QVERIFY(f.undo.redo());
  COMPARE_STATE(f.anim.snapshot(), after);
  // and back again: the steps can be walked as often as wanted
  QVERIFY(f.undo.undo());
  COMPARE_STATE(f.anim.snapshot(), before);
  QVERIFY(f.undo.redo());
  COMPARE_STATE(f.anim.snapshot(), after);
}

// Inserting or deleting a frame on one joint moves its later keys (fixed in job B1: the old
// code read freed memory when two or more keys followed, and used the current frame instead of
// the frame asked for)
void UndoTest::insertAndDeleteFrameMoveTheKeys()
{
  Fixture f;
  f.turn("lThigh", 10, 30, 10, 5);
  f.turn("lThigh", 20, -10, 2, 1);
  f.turn("lThigh", 25, 4, 5, 6);
  f.anim.setCurve(f.node("lThigh"), 20, Easing::Preset::SlowStart);
  BVHNode* thigh = f.node("lThigh");
  const int track = f.track("lThigh");
  // the current frame is somewhere else on purpose
  f.anim.setFrame(3);
  f.anim.insertFrame(track, 12);
  QCOMPARE(thigh->keyframeList(), QList<int>({0, 10, 21, 26}));
  for (int key : thigh->keyframeList()) QCOMPARE(int(thigh->keyframeMap().value(key).frameNumber()), key);
  QCOMPARE(thigh->frameData(21).rotation(), QVector3D(-10, 2, 1));
  QCOMPARE(thigh->frameData(26).rotation(), QVector3D(4, 5, 6));
  QCOMPARE(thigh->curve(21), std::optional<Easing::Preset>(Easing::Preset::SlowStart));
  // a key on the frame itself moves too
  f.anim.insertFrame(track, 10);
  QCOMPARE(thigh->keyframeList(), QList<int>({0, 11, 22, 27}));
  QCOMPARE(thigh->frameData(11).rotation(), QVector3D(30, 10, 5));
  // deleting a frame with a key takes the key away and moves the later ones back
  f.anim.deleteFrame(track, 11);
  QCOMPARE(thigh->keyframeList(), QList<int>({0, 21, 26}));
  f.anim.deleteFrame(track, 5);
  QCOMPARE(thigh->keyframeList(), QList<int>({0, 20, 25}));
  for (int key : thigh->keyframeList()) QCOMPARE(int(thigh->keyframeMap().value(key).frameNumber()), key);
  QCOMPARE(thigh->frameData(20).rotation(), QVector3D(-10, 2, 1));
  QCOMPARE(thigh->curve(20), std::optional<Easing::Preset>(Easing::Preset::SlowStart));
  // past the last key: nothing moves
  f.anim.insertFrame(track, 30);
  f.anim.deleteFrame(track, 30);
  QCOMPARE(thigh->keyframeList(), QList<int>({0, 20, 25}));
  // the other joints are left alone
  QCOMPARE(f.node("rThigh")->keyframeList(), QList<int>({0}));
}

void UndoTest::newEditClearsRedo()
{
  Fixture f;
  f.turn("lThigh", 10, 30);
  f.settle();
  f.turn("lThigh", 20, -15);
  f.settle();
  QVERIFY(f.undo.undo());
  QVERIFY(f.redoAction.isEnabled());
  QCOMPARE(f.undo.history(&f.anim)->redoCount(), 1);
  // a new edit: Redo is greyed at once, and its step is gone
  f.turn("rShin", 5, 8);
  QVERIFY(!f.redoAction.isEnabled());
  QCOMPARE(f.redoAction.statusTip(), QStringLiteral("Nothing to redo. Redo works after Undo."));
  f.settle();
  QCOMPARE(f.undo.history(&f.anim)->redoCount(), 0);
  QVERIFY(!f.undo.redo());
  QCOMPARE(f.undo.history(&f.anim)->undoCount(), 2);
}

void UndoTest::editWithoutChangeIsNoStep()
{
  Fixture f;
  // the same speed again marks the animation as changed, but nothing changed
  f.anim.setFPS(f.anim.fps());
  f.settle();
  QCOMPARE(f.undo.history(&f.anim)->undoCount(), 0);
  QVERIFY(!f.undoAction.isEnabled());
  QVERIFY(!f.undo.undo());
}

void UndoTest::dragIsOneStep()
{
  Fixture f;
  const Animation::Snapshot before = f.anim.snapshot();
  f.dragging = true;
  for (int i = 1; i <= 10; ++i)
  {
    f.turn("lThigh", 10, i * 3.0);
    f.settle();
  }
  // still dragging: no step yet
  QCOMPARE(f.undo.history(&f.anim)->undoCount(), 0);
  f.dragging = false;
  QTRY_COMPARE(f.undo.history(&f.anim)->undoCount(), 1);
  QVERIFY(f.undo.undo());
  COMPARE_STATE(f.anim.snapshot(), before);
}

void UndoTest::pullIsOneStepAndUndoWaitsForItsEnd()
{
  Fixture f;
  QSignalSpy messages(&f.undo, &UndoController::message);
  f.turn("lThigh", 10, 30);
  f.settle();
  // a pull is running (or a mouse button is held): Undo waits and says why
  f.dragging = true;
  const Animation::Snapshot during = f.anim.snapshot();
  QVERIFY(!f.undo.undo());
  COMPARE_STATE(f.anim.snapshot(), during);
  QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Finish the change you are making first."));
  QVERIFY(!f.undo.redo());
  QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Finish the change you are making first."));
  f.dragging = false;
  // the release sets the keys of all pulled joints at once: one step
  f.turn("lShin", 10, 20);
  f.turn("lFoot", 10, -5);
  f.settle();
  QCOMPARE(f.undo.history(&f.anim)->undoCount(), 2);
  QVERIFY(f.undo.undo());
  COMPARE_STATE(f.anim.snapshot(), during);
}

void UndoTest::openingCodeIsNoStep()
{
  BVH bvh;
  Animation anim(&bvh);
  QAction undo, redo;
  UndoController controller(&undo, &redo);
  controller.watch(&anim);
  controller.setAnimation(&anim);
  // what the window does right after it opens a file: frames, loop, the clean mark
  anim.setNumberOfFrames(60);
  anim.setLoopOutPoint(59);
  anim.setLoopInPoint(1);
  anim.setDirty(false);
  QCoreApplication::processEvents();
  QVERIFY(controller.history(&anim)->started());
  QCOMPARE(controller.history(&anim)->undoCount(), 0);
  QCOMPARE(controller.history(&anim)->current().frames, 60);
  QVERIFY(controller.history(&anim)->atSaved());
  QVERIFY(!undo.isEnabled());
}

void UndoTest::eachAnimationHasItsOwnHistory()
{
  Fixture f;
  BVH bvh2;
  Animation second(&bvh2);
  second.setNumberOfFrames(30);
  second.setDirty(false);
  f.undo.watch(&second);
  QCoreApplication::processEvents();
  const Animation::Snapshot firstBefore = f.anim.snapshot();
  f.turn("lThigh", 10, 30);
  f.settle();
  const Animation::Snapshot firstAfter = f.anim.snapshot();
  const Animation::Snapshot secondBefore = second.snapshot();
  second.setFrame(5);
  second.addKeyFrameAllJoints();
  QCoreApplication::processEvents();
  QCOMPARE(f.undo.history(&second)->undoCount(), 1);

  // the second figure becomes the active one: Undo works on it only
  f.undo.setAnimation(&second);
  QVERIFY(f.undo.undo());
  COMPARE_STATE(second.snapshot(), secondBefore);
  COMPARE_STATE(f.anim.snapshot(), firstAfter);
  QVERIFY(!f.undoAction.isEnabled());
  f.undo.setAnimation(&f.anim);
  QVERIFY(f.undoAction.isEnabled());
  QVERIFY(f.undo.undo());
  COMPARE_STATE(f.anim.snapshot(), firstBefore);

  // a closed animation leaves no history behind
  {
    BVH bvh3;
    auto* third = new Animation(&bvh3);
    f.undo.watch(third);
    QVERIFY(f.undo.history(third));
    f.undo.setAnimation(third);
    delete third;
    QVERIFY(!f.undo.animation());
    QCOMPARE(f.undoAction.statusTip(), QStringLiteral("Open an animation first."));
  }
}

void UndoTest::historySurvivesSave()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("undo.dingle"));
  Fixture f;
  const Animation::Snapshot first = f.anim.snapshot();
  f.turn("lThigh", 10, 30);
  f.settle();
  const Animation::Snapshot second = f.anim.snapshot();
  f.turn("rShin", 15, -20);
  f.settle();
  const Animation::Snapshot third = f.anim.snapshot();
  QVERIFY(f.anim.dirty());

  QString error;
  QVERIFY2(f.anim.saveBVH(path, &error), qPrintable(error));
  QVERIFY(!f.anim.dirty());
  // nothing is lost by saving
  QCOMPARE(f.undo.history(&f.anim)->undoCount(), 2);
  QVERIFY(f.undoAction.isEnabled());

  QVERIFY(f.undo.undo());
  COMPARE_STATE(f.anim.snapshot(), second);
  // not the saved state any more: marked as changed
  QVERIFY(f.anim.dirty());
  QVERIFY(f.undo.undo());
  COMPARE_STATE(f.anim.snapshot(), first);
  QVERIFY(f.anim.dirty());
  QVERIFY(f.undo.redo());
  QVERIFY(f.undo.redo());
  COMPARE_STATE(f.anim.snapshot(), third);
  // back at the saved state: the unsaved mark is gone again
  QVERIFY(!f.anim.dirty());

  // the file holds the saved state
  BVH bvh;
  Animation reopened(&bvh, path);
  QCOMPARE(reopened.getNumberOfFrames(), 40);
  BVHNode* thigh = reopened.getNode(TimelineClip::trackOf(&reopened, QStringLiteral("lThigh")));
  QVERIFY(thigh);
  QVERIFY(thigh->isKeyframe(10));
  QVERIFY(qAbs(thigh->frameData(10).rotation().x() - 30.0f) < 0.01f);

  // an edit after saving, then Undo: back to the saved state, so not changed
  f.turn("lThigh", 20, 5);
  f.settle();
  QVERIFY(f.anim.dirty());
  QVERIFY(f.undo.undo());
  QVERIFY(!f.anim.dirty());
}

void UndoTest::greyStateAndReasons()
{
  // no animation
  {
    QAction undo, redo;
    UndoController controller(&undo, &redo);
    QVERIFY(!undo.isEnabled());
    QVERIFY(!redo.isEnabled());
    QCOMPARE(undo.statusTip(), QStringLiteral("Open an animation first."));
    QCOMPARE(undo.toolTip(), QStringLiteral("Open an animation first."));
    QCOMPARE(redo.statusTip(), QStringLiteral("Open an animation first."));
    QSignalSpy messages(&controller, &UndoController::message);
    QVERIFY(!controller.undo());
    QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Open an animation first."));
  }
  Fixture f;
  QVERIFY(!f.undoAction.isEnabled());
  QVERIFY(!f.redoAction.isEnabled());
  QCOMPARE(f.undoAction.statusTip(), QStringLiteral("Nothing to undo yet."));
  QCOMPARE(f.undoAction.toolTip(), QStringLiteral("Nothing to undo yet."));
  QCOMPARE(f.redoAction.statusTip(), QStringLiteral("Nothing to redo. Redo works after Undo."));
  QCOMPARE(f.redoAction.toolTip(), QStringLiteral("Nothing to redo. Redo works after Undo."));

  f.turn("lThigh", 10, 30);
  f.settle();
  QVERIFY(f.undoAction.isEnabled());
  QCOMPARE(f.undoAction.statusTip(), QString());
  QVERIFY(!f.redoAction.isEnabled());

  // playing or the empty state: the window's reason wins
  f.undo.setBlocked(QStringLiteral("Stop playing first."));
  QVERIFY(!f.undoAction.isEnabled());
  QCOMPARE(f.undoAction.statusTip(), QStringLiteral("Stop playing first."));
  QCOMPARE(f.redoAction.statusTip(), QStringLiteral("Stop playing first."));
  QVERIFY(!f.undo.undo());
  f.undo.setBlocked(QString());
  QVERIFY(f.undoAction.isEnabled());

  QVERIFY(f.undo.undo());
  QVERIFY(!f.undoAction.isEnabled());
  QCOMPARE(f.undoAction.statusTip(), QStringLiteral("Nothing to undo yet."));
  QVERIFY(f.redoAction.isEnabled());
  QCOMPARE(f.redoAction.statusTip(), QString());

  // the texts in the table, letter for letter, and short sentences (at most 12 words)
  QCOMPARE(QString::fromLatin1(MenuSpec::nothingToUndo), QStringLiteral("Nothing to undo yet."));
  QCOMPARE(QString::fromLatin1(MenuSpec::nothingToRedo), QStringLiteral("Nothing to redo. Redo works after Undo."));
  QCOMPARE(QString::fromLatin1(MenuSpec::undoWait), QStringLiteral("Finish the change you are making first."));
  for (const char* text : {MenuSpec::nothingToUndo, MenuSpec::nothingToRedo, MenuSpec::undoWait, MenuSpec::undone, MenuSpec::redone})
    for (const QString& sentence : QString::fromLatin1(text).split(QStringLiteral(". ")))
      QVERIFY2(sentence.split(QLatin1Char(' ')).size() <= 12, text);
}

void UndoTest::statusTexts()
{
  Fixture f;
  QSignalSpy messages(&f.undo, &UndoController::message);
  QSignalSpy restored(&f.undo, &UndoController::restored);
  QVERIFY(!f.undo.undo());
  QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Nothing to undo yet."));
  QVERIFY(!f.undo.redo());
  QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Nothing to redo. Redo works after Undo."));
  QCOMPARE(restored.count(), 0);
  f.turn("lThigh", 10, 30);
  QVERIFY(f.undo.undo());
  QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Undone."));
  QCOMPARE(restored.count(), 1);
  QCOMPARE(restored.last().at(0).value<Animation*>(), &f.anim);
  QVERIFY(f.undo.redo());
  QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Redone."));
  QCOMPARE(restored.count(), 2);
}

void UndoTest::menuTextsAndKeys()
{
  QCOMPARE(QString::fromLatin1(MenuSpec::editUndo.id), QStringLiteral("editUndoAction"));
  QCOMPARE(MenuSpec::text(MenuSpec::editUndo), QStringLiteral("Undo"));
  QCOMPARE(QString::fromLatin1(MenuSpec::editUndo.key), QStringLiteral("Ctrl+Z"));
  QCOMPARE(MenuSpec::reason(MenuSpec::editUndo), QString());
  QCOMPARE(QString::fromLatin1(MenuSpec::editRedo.id), QStringLiteral("editRedoAction"));
  QCOMPARE(MenuSpec::text(MenuSpec::editRedo), QStringLiteral("Redo"));
  QCOMPARE(QString::fromLatin1(MenuSpec::editRedo.key), QStringLiteral("Ctrl+Shift+Z"));
  QCOMPARE(MenuSpec::reason(MenuSpec::editRedo), QString());
  // the top bar buttons follow the menu items, with no reason of their own
  QCOMPARE(QString::fromLatin1(MenuSpec::topUndo.key), QStringLiteral("Ctrl+Z"));
  QCOMPARE(QString::fromLatin1(MenuSpec::topRedo.key), QStringLiteral("Ctrl+Shift+Z"));
  QCOMPARE(QString::fromLatin1(MenuSpec::topUndo.reason), QString());
  QCOMPARE(QString::fromLatin1(MenuSpec::topRedo.reason), QString());

  // the keys: the menu shows the first one
  const QList<QKeySequence> undoKeys = UndoController::undoShortcuts();
  const QList<QKeySequence> redoKeys = UndoController::redoShortcuts();
  QCOMPARE(undoKeys, QList<QKeySequence>({QKeySequence(Qt::CTRL | Qt::Key_Z)}));
  QCOMPARE(redoKeys, QList<QKeySequence>({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z), QKeySequence(Qt::CTRL | Qt::Key_Y)}));
  QCOMPARE(undoKeys.first().toString(QKeySequence::PortableText), QString::fromLatin1(MenuSpec::editUndo.key));
  QCOMPARE(redoKeys.first().toString(QKeySequence::PortableText), QString::fromLatin1(MenuSpec::editRedo.key));
  QCOMPARE(redoKeys.at(1).toString(QKeySequence::PortableText), QStringLiteral("Ctrl+Y"));
}

// A Norwegian keyboard has Z and Y where an English one has them (QWERTY), and Qt
// sends the same keys: Ctrl+Z, Ctrl+Shift+Z (Key_Z with Shift, as Shift makes a
// capital Z) and Ctrl+Y. A German keyboard swaps the two letters on the keys, and
// Qt still sends Key_Z for the letter Z, so the letters count, not the place.
void UndoTest::keysTriggerUndoAndRedo()
{
  Fixture f;
  QWidget window;
  window.resize(200, 100);
  f.undoAction.setShortcuts(UndoController::undoShortcuts());
  f.redoAction.setShortcuts(UndoController::redoShortcuts());
  window.addAction(&f.undoAction);
  window.addAction(&f.redoAction);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  window.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&window));

  const Animation::Snapshot before = f.anim.snapshot();
  f.turn("lThigh", 10, 30);
  f.settle();
  const Animation::Snapshot after = f.anim.snapshot();

  QTest::keyClick(&window, Qt::Key_Z, Qt::ControlModifier);
  COMPARE_STATE(f.anim.snapshot(), before);
  QTest::keyClick(&window, Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier);
  COMPARE_STATE(f.anim.snapshot(), after);
  QTest::keyClick(&window, Qt::Key_Z, Qt::ControlModifier);
  COMPARE_STATE(f.anim.snapshot(), before);
  QTest::keyClick(&window, Qt::Key_Y, Qt::ControlModifier);
  COMPARE_STATE(f.anim.snapshot(), after);
  // Z alone is no Undo (in the 3D view it picks the Zoom tool)
  QTest::keyClick(&window, Qt::Key_Z);
  COMPARE_STATE(f.anim.snapshot(), after);
}

void UndoTest::textFieldKeepsCtrlZ()
{
  Fixture f;
  QWidget window;
  auto* layout = new QVBoxLayout(&window);
  auto* field = new QLineEdit(&window);
  layout->addWidget(field);
  f.undoAction.setShortcuts(UndoController::undoShortcuts());
  window.addAction(&f.undoAction);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  window.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&window));
  field->setFocus();

  f.turn("lThigh", 10, 30);
  f.settle();
  const Animation::Snapshot after = f.anim.snapshot();
  QTest::keyClicks(field, QStringLiteral("12"));
  QCOMPARE(field->text(), QStringLiteral("12"));
  // Ctrl+Z in the field takes back the typing, not the animation edit
  QTest::keyClick(field, Qt::Key_Z, Qt::ControlModifier);
  QCOMPARE(field->text(), QString());
  COMPARE_STATE(f.anim.snapshot(), after);
}

void UndoTest::windowWiring()
{
  const QString window = source("pinglymotionwindow.cpp");
  const QString undo = source("pinglymotionwindowundo.cpp");
  const QString file = source("pinglymotionwindowfile.cpp");
  QVERIFY(!window.isEmpty());
  QVERIFY(!undo.isEmpty());
  // Edit menu: real shortcuts, then the controller
  QVERIFY(window.contains(QStringLiteral("m_undoAction=specAction(this,editUndo,false);")));
  QVERIFY(window.contains(QStringLiteral("m_redoAction=specAction(this,editRedo,false);")));
  QVERIFY(window.contains(QStringLiteral("buildUndo();")));
  QVERIFY(undo.contains(QStringLiteral("m_undoAction->setShortcuts(UndoController::undoShortcuts());")));
  QVERIFY(undo.contains(QStringLiteral("m_redoAction->setShortcuts(UndoController::redoShortcuts());")));
  QVERIFY(undo.contains(QStringLiteral("m_undo=new UndoController(m_undoAction,m_redoAction,this);")));
  // a drag or a pull is one step
  QVERIFY(undo.contains(QStringLiteral("return QGuiApplication::mouseButtons()!=Qt::NoButton || ui->animationView->pulling();")));
  QVERIFY(undo.contains(QStringLiteral("connect(m_undo,&UndoController::restored,this,&PinglyMotionWindow::undoRestored);")));
  QVERIFY(undo.contains(QStringLiteral("statusBar()->showMessage(text,5000);")));
  // every opened animation is watched, the active figure is the one Undo works on
  QVERIFY(file.contains(QStringLiteral("if(m_undo) m_undo->watch(anim);")));
  QVERIFY(window.contains(QStringLiteral("if(m_undo) m_undo->setAnimation(animation);")));
  // greyed while no animation is open or while playing
  const int edit = window.indexOf(QStringLiteral("void PinglyMotionWindow::updateEditActions()"));
  QVERIFY(edit >= 0);
  const int blocked = window.indexOf(QStringLiteral("m_undo->setBlocked(m_empty ? QCoreApplication::translate(\"MenuSpec\",MenuSpec::openFirst)"), edit);
  QVERIFY(blocked > edit);
  QVERIFY(window.indexOf(QStringLiteral("MenuSpec::setKeyStopPlaying"), blocked) > blocked);
  // a restored step shows the fields again without writing them back
  QVERIFY(undo.contains(QStringLiteral("const QSignalBlocker blockIn(ui->loopInSpinBox);")));
  QVERIFY(undo.contains(QStringLiteral("updateInputs();")));
  // a timeline move tells the change mark (and Undo) once it is done
  const QString anim = source("animation.cpp");
  const int move = anim.indexOf(QStringLiteral("void Animation::moveKeyFrame("));
  QVERIFY(move >= 0);
  QVERIFY(anim.indexOf(QStringLiteral("blockSignals(false);\n  // the change mark (and Undo, src/undohistory.h) hear of the move only now\n  setDirty(true);"), move) > move);
}

void UndoTest::mockupTextsAndExpertOnlyRule()
{
  const QString page = doc("mockup/index.html");
  QVERIFY(!page.isEmpty());
  // the reasons and status texts are the mockup's
  QVERIFY(page.contains(QStringLiteral("setDis(e, undoStack.length ? null : \"Nothing to undo yet.\")")));
  QVERIFY(page.contains(QStringLiteral("setDis(e, redoStack.length ? null : \"Nothing to redo. Redo works after Undo.\")")));
  QVERIFY(page.contains(QStringLiteral("restore(undoStack.pop()); feedback(\"Undone.\");")));
  QVERIFY(page.contains(QStringLiteral("restore(redoStack.pop()); feedback(\"Redone.\");")));
  QVERIFY(page.contains(QStringLiteral("if (ctrl && (k === \"y\") && !typing(t)) { e.preventDefault(); clickCmd(\"redo\"); return; }")));
  QVERIFY(page.contains(QStringLiteral("if (undoStack.length > 100) undoStack.shift();")));
  // the Expert-only buttons (Paste mirrored, Edit curve) are hidden in Beginner mode and keep
  // their room, as in the program (charter P12, docs/UX_DECISIONS.md 6k)
  QVERIFY(page.contains(QStringLiteral("body:not(.expert) .expert-only { visibility:hidden; }")));
  QCOMPARE(page.count(QStringLiteral("class=\"btn expert-only\"")), 2);
  QVERIFY(page.contains(QStringLiteral("<button class=\"btn expert-only\" data-cmd=\"pastemirror\"")));
  QVERIFY(page.contains(QStringLiteral("<button class=\"btn expert-only\" data-cmd=\"curve\"")));
}

QTEST_MAIN(UndoTest)
#include "tst_undo.moc"
