/*
 * PinglyMotion - test of the timeline's stretch: select, copy, cut, paste and
 * Paste mirrored, UX phase 3b (behaviour and texts)
 *
 * Exact answers only (src/timelineclip.*, src/timeline.*, docs/UX_DECISIONS.md
 * section 5): which keys a copy keeps (the keys inside and the pose at both
 * ends), where a paste puts them and with which pose, the joint on the other
 * side for Paste mirrored and the left-right swap of a turn and of the Avatar
 * Position, the easing kept, keys past the end left out and counted, Cut that
 * keeps frame 0, a clip pasted into another animation, the selection with
 * Shift and the mouse and with Shift and the arrow keys, Escape, what Copy
 * takes without a selection, the status texts and the reasons, a pasted
 * stretch that is saved and opened again, and the Paste mirrored button and
 * menu item as in the mockup.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QFile>
#include <QImage>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"
#include "jointgroups.h"
#include "lucideicons.h"
#include "menuspec.h"
#include "theme.h"
#include "timeline.h"
#include "timelineclip.h"
#include "timelinelook.h"
#include "timelineview.h"
#include "transportlook.h"
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
  QString mockup() { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/mockup/index.html")); }

  bool same(const QVector3D& a, const QVector3D& b, float tolerance = 1e-4f)
  {
    return qAbs(a.x() - b.x()) <= tolerance && qAbs(a.y() - b.y()) <= tolerance && qAbs(a.z() - b.z()) <= tolerance;
  }
  QByteArray show(const QVector3D& v) { return QStringLiteral("(%1, %2, %3)").arg(v.x()).arg(v.y()).arg(v.z()).toUtf8(); }
}

#define COMPARE_VEC(actual, expected) \
  QVERIFY2(same((actual), (expected)), show(actual) + " is not " + show(expected))

// a new animation (every joint unturned, one key at frame 0), with the Mirror
// switch off so a turn changes only the joint it is set on
struct Fixture
{
  BVH bvh;
  Animation anim;
  TimelineView view;
  explicit Fixture(int frames = 40) : anim(&bvh)
  {
    Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
    QStringList names;
    for (int t = 0; t < Constants::numParts(); ++t) names << anim.getPartName(t);
    JointGroups::setTracks(names);
    view.getTimeline()->setAnimation(&anim);
    anim.setNumberOfFrames(frames);
    anim.setMirrored(false);
  }
  int track(const char* name) const { return TimelineClip::trackOf(const_cast<Animation*>(&anim), QLatin1String(name)); }
  BVHNode* node(int track) { return anim.getNode(track); }
  BVHNode* node(const char* name) { return node(track(name)); }
  void turn(int track, int frame, double x, double y = 0, double z = 0)
  {
    anim.setFrame(frame);
    anim.setRotation(node(track), x, y, z);
  }
  Timeline* timeline() { return view.getTimeline(); }
  void showView()
  {
    view.resize(900, 400);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
  }
  static QPoint at(int frame, int row) { return QPoint(frame * Constants::keyWidth() + 1, row * Constants::lineHeight() + 2); }
};

class TimelineClipTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
  void rangeBasics();
  void copyKeepsKeysInsideAndThePoseAtBothEnds();
  void copyOfOneFrameIsOneKeyPerJoint();
  void pasteFromTheCurrentFrameOnTheSameJoints();
  void pastedStretchPlaysLikeTheCopiedOne();
  void pasteMirroredLeftStepBecomesRightStep();
  void pasteMirroredSwapsBothSides();
  void jointWithoutOtherSideIsSwappedInPlace();
  void avatarPositionTurnsXRound();
  void easingIsKept();
  void keysPastTheEndAreLeftOutAndCounted();
  void removeKeysKeepsFrameZero();
  void clipPastesIntoAnotherAnimationByName();
  void mirrorPartners();
  void pasteRedrawsRowsAndMarksChanged();
  void shiftDragSelectsAStretch();
  void shiftClickMovesTheFarCornerAndAPlainClickLeaves();
  void shiftOnAKeyStillCopiesTheKey();
  void keyboardGrowsAndShrinksTheStretch();
  void escapeLeavesTheStretch();
  void copyRangeWithoutSelectionIsTheCurrentFrame();
  void newAnimationLeavesTheStretch();
  void selectionBoxIsDrawnInTheSelectionColour();
  void messages();
  void reasons();
  void savedAndOpenedAgain();
  void buttonAndMenuAsInTheMockup();
};

void TimelineClipTest::rangeBasics()
{
  TimelineRange range;
  QVERIFY(range.isEmpty());
  QCOMPARE(range.frameCount(), 0);
  range.firstFrame = 3;
  range.lastFrame = 7;
  QVERIFY(range.isEmpty());   // no joints
  range.tracks = {5};
  QVERIFY(!range.isEmpty());
  QCOMPARE(range.frameCount(), 5);
  range.lastFrame = 2;
  QVERIFY(range.isEmpty());
  TimelineRange other{3, 2, {5}};
  QVERIFY(range == other);
}

void TimelineClipTest::copyKeepsKeysInsideAndThePoseAtBothEnds()
{
  Fixture f;
  const int thigh = f.track("lThigh");
  QVERIFY(thigh > 0);
  f.turn(thigh, 10, 30, 10, 5);
  f.turn(thigh, 20, -10);
  const QVector3D at5 = f.node(thigh)->frameData(5).rotation();
  const QVector3D at15 = f.node(thigh)->frameData(15).rotation();
  QVERIFY(!at5.isNull() && !same(at5, QVector3D(30, 10, 5)));

  TimelineClip clip;
  QCOMPARE(clip.copy(&f.anim, TimelineRange{5, 15, {thigh}}), 3);
  QCOMPARE(clip.length(), 11);
  QCOMPARE(clip.keyCount(), 3);
  QCOMPARE(clip.joints().count(), 1);
  const TimelineClip::Joint& joint = clip.joints().first();
  QCOMPARE(joint.name, QStringLiteral("lThigh"));
  QVERIFY(!joint.position);
  QCOMPARE(joint.keys.count(), 3);
  QCOMPARE(joint.keys[0].offset, 0);
  QCOMPARE(joint.keys[1].offset, 5);
  QCOMPARE(joint.keys[2].offset, 10);
  COMPARE_VEC(joint.keys[0].data.rotation(), at5);
  COMPARE_VEC(joint.keys[1].data.rotation(), QVector3D(30, 10, 5));
  COMPARE_VEC(joint.keys[2].data.rotation(), at15);
  // copying changes nothing in the animation
  QCOMPARE(f.node(thigh)->keyframeList(), QList<int>({0, 10, 20}));
}

void TimelineClipTest::copyOfOneFrameIsOneKeyPerJoint()
{
  Fixture f;
  const int thigh = f.track("lThigh");
  const int head = f.track("head");
  f.turn(thigh, 10, 30);
  f.turn(head, 4, 12);
  TimelineClip clip;
  QCOMPARE(clip.copy(&f.anim, TimelineRange{10, 10, {thigh, head}}), 2);
  QCOMPARE(clip.length(), 1);
  COMPARE_VEC(clip.joints()[0].keys[0].data.rotation(), QVector3D(30, 0, 0));
  COMPARE_VEC(clip.joints()[1].keys[0].data.rotation(), QVector3D(12, 0, 0));
}

void TimelineClipTest::pasteFromTheCurrentFrameOnTheSameJoints()
{
  Fixture f;
  const int thigh = f.track("lThigh");
  const int shin = f.track("lShin");
  f.turn(thigh, 4, 30);
  f.turn(shin, 6, -20);
  TimelineClip clip;
  clip.copy(&f.anim, TimelineRange{0, 8, {thigh, shin}});
  const TimelineClip::PasteResult result = clip.paste(&f.anim, 20, false);
  // thigh: 0, 4, 8; shin: 0, 6, 8
  QCOMPARE(result.keys, 6);
  QCOMPARE(result.leftOut, 0);
  QCOMPARE(result.tracks, QList<int>({thigh, shin}));
  QCOMPARE(f.node(thigh)->keyframeList(), QList<int>({0, 4, 20, 24, 28}));
  QCOMPARE(f.node(shin)->keyframeList(), QList<int>({0, 6, 20, 26, 28}));
  COMPARE_VEC(f.node(thigh)->frameData(24).rotation(), QVector3D(30, 0, 0));
  COMPARE_VEC(f.node(shin)->frameData(26).rotation(), QVector3D(-20, 0, 0));
  // the other side is not touched by a plain paste
  QCOMPARE(f.node("rThigh")->keyframeList(), QList<int>({0}));
}

// With keys that move evenly the pasted stretch is the copied one at every frame. With the keys a new
// animation now starts with (smooth, slow in and out; 24 Nov 2026) a copy that starts or ends
// between two keys cuts a curve into two, so the pasted stretch is the same from the first inner key
// to the last inner key (frames 3 to 9 here), and the two ends follow the cut curves.
void TimelineClipTest::pastedStretchPlaysLikeTheCopiedOne()
{
  for (const bool smooth : {false, true})
  {
    Fixture f;
    const int thigh = f.track("lThigh");
    f.turn(thigh, 3, 25, 5);
    f.turn(thigh, 9, -15);
    f.turn(thigh, 14, 40);
    if (!smooth)
      for (const int key : f.node(thigh)->keyframeList())
      {
        f.node(thigh)->setEaseIn(key, false);
        f.node(thigh)->setEaseOut(key, false);
      }
    else
      QVERIFY(f.node(thigh)->easeIn(3) && f.node(thigh)->easeOut(3));
    TimelineClip clip;
    clip.copy(&f.anim, TimelineRange{2, 12, {thigh}});
    clip.paste(&f.anim, 25, false);
    const int from = smooth ? 1 : 0;
    const int to = smooth ? 7 : 10;
    for (int k = from; k <= to; ++k)
      QVERIFY2(same(f.node(thigh)->frameData(25 + k).rotation(), f.node(thigh)->frameData(2 + k).rotation()),
               qPrintable(QStringLiteral("%1 frame %2").arg(smooth ? "smooth" : "even").arg(25 + k)));
  }
}

void TimelineClipTest::pasteMirroredLeftStepBecomesRightStep()
{
  // the owner test of UX step 8: a left-leg step pasted mirrored is a right-leg step
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int lShin = f.track("lShin");
  const int rThigh = f.track("rThigh");
  const int rShin = f.track("rShin");
  QVERIFY(lThigh > 0 && lShin > 0 && rThigh > 0 && rShin > 0);
  f.turn(lThigh, 5, -30, 10, 5);
  f.turn(lThigh, 10, 15, -4, 2);
  f.turn(lShin, 5, 40, 3, -6);
  f.turn(lShin, 10, 5);

  TimelineClip clip;
  clip.copy(&f.anim, TimelineRange{0, 10, {lThigh, lShin}});
  const TimelineClip::PasteResult result = clip.paste(&f.anim, 15, true);
  QCOMPARE(result.tracks, QList<int>({rThigh, rShin}));
  QCOMPARE(result.keys, 6);
  QCOMPARE(f.node(rThigh)->keyframeList(), QList<int>({0, 15, 20, 25}));
  QCOMPARE(f.node(rShin)->keyframeList(), QList<int>({0, 15, 20, 25}));
  COMPARE_VEC(f.node(rThigh)->frameData(20).rotation(), QVector3D(-30, -10, -5));
  COMPARE_VEC(f.node(rThigh)->frameData(25).rotation(), QVector3D(15, 4, -2));
  COMPARE_VEC(f.node(rShin)->frameData(20).rotation(), QVector3D(40, -3, 6));
  // every in-between frame is the left leg's, swapped
  for (int k = 0; k <= 10; ++k)
  {
    const QVector3D left = f.node(lThigh)->frameData(k).rotation();
    QVERIFY2(same(f.node(rThigh)->frameData(15 + k).rotation(), QVector3D(left.x(), -left.y(), -left.z())),
             qPrintable(QStringLiteral("frame %1").arg(15 + k)));
  }
  // the left leg keeps its own keys only
  QCOMPARE(f.node(lThigh)->keyframeList(), QList<int>({0, 5, 10}));
  QCOMPARE(f.node(lShin)->keyframeList(), QList<int>({0, 5, 10}));
}

void TimelineClipTest::pasteMirroredSwapsBothSides()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int rThigh = f.track("rThigh");
  f.turn(lThigh, 5, 20, 6, 0);
  f.turn(rThigh, 5, -12, 0, 8);
  TimelineClip clip;
  clip.copy(&f.anim, TimelineRange{5, 5, {lThigh, rThigh}});
  clip.paste(&f.anim, 30, true);
  COMPARE_VEC(f.node(rThigh)->frameData(30).rotation(), QVector3D(20, -6, 0));
  COMPARE_VEC(f.node(lThigh)->frameData(30).rotation(), QVector3D(-12, 0, -8));
}

void TimelineClipTest::jointWithoutOtherSideIsSwappedInPlace()
{
  Fixture f;
  const int chest = f.track("chest");
  f.turn(chest, 4, 10, 20, 30);
  QCOMPARE(TimelineClip::mirrorTrack(&f.anim, chest), chest);
  TimelineClip clip;
  clip.copy(&f.anim, TimelineRange{4, 4, {chest}});
  const TimelineClip::PasteResult result = clip.paste(&f.anim, 12, true);
  QCOMPARE(result.tracks, QList<int>({chest}));
  COMPARE_VEC(f.node(chest)->frameData(12).rotation(), QVector3D(10, -20, -30));
}

void TimelineClipTest::avatarPositionTurnsXRound()
{
  Fixture f;
  BVHNode* position = f.node(0);
  QCOMPARE(position->type, BVH_POS);
  position->addKeyframe(6, QVector3D(10, 40, 5), QVector3D());
  TimelineClip clip;
  clip.copy(&f.anim, TimelineRange{6, 6, {0}});
  QVERIFY(clip.joints().first().position);
  clip.paste(&f.anim, 20, true);
  COMPARE_VEC(position->frameData(20).position(), QVector3D(-10, 40, 5));
  clip.paste(&f.anim, 25, false);
  COMPARE_VEC(position->frameData(25).position(), QVector3D(10, 40, 5));
  QCOMPARE(TimelineClip::mirrored(FrameData(0, QVector3D(1, 2, 3), QVector3D(4, 5, 6)), true).position(), QVector3D(-1, 2, 3));
  QCOMPARE(TimelineClip::mirrored(FrameData(0, QVector3D(1, 2, 3), QVector3D(4, 5, 6)), true).rotation(), QVector3D(4, 5, 6));
  QCOMPARE(TimelineClip::mirrored(FrameData(0, QVector3D(1, 2, 3), QVector3D(4, 5, 6)), false).rotation(), QVector3D(4, -5, -6));
  QCOMPARE(TimelineClip::mirrored(FrameData(0, QVector3D(1, 2, 3), QVector3D(4, 5, 6)), false).position(), QVector3D(1, 2, 3));
}

void TimelineClipTest::easingIsKept()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int rThigh = f.track("rThigh");
  f.turn(lThigh, 8, 30);
  f.node(lThigh)->setEaseIn(8, true);
  f.node(lThigh)->setEaseOut(8, true);
  f.node(lThigh)->setCurve(8, Easing::Preset::OvershootThenSettle);
  TimelineClip clip;
  clip.copy(&f.anim, TimelineRange{8, 8, {lThigh}});
  clip.paste(&f.anim, 20, true);
  QVERIFY(f.node(rThigh)->easeIn(20));
  QVERIFY(f.node(rThigh)->easeOut(20));
  QCOMPARE(f.node(rThigh)->curve(20), std::optional<Easing::Preset>(Easing::Preset::OvershootThenSettle));
}

void TimelineClipTest::keysPastTheEndAreLeftOutAndCounted()
{
  Fixture f(30);
  const int thigh = f.track("lThigh");
  f.turn(thigh, 4, 30);
  f.turn(thigh, 8, 10);
  TimelineClip clip;
  clip.copy(&f.anim, TimelineRange{0, 8, {thigh}});   // keys at 0, 4, 8
  const TimelineClip::PasteResult result = clip.paste(&f.anim, 24, false);
  QCOMPARE(result.keys, 2);      // 24 and 28
  QCOMPARE(result.leftOut, 1);   // 32 is past frame 29
  QCOMPARE(f.node(thigh)->keyframeList(), QList<int>({0, 4, 8, 24, 28}));
}

void TimelineClipTest::removeKeysKeepsFrameZero()
{
  Fixture f;
  const int thigh = f.track("lThigh");
  f.turn(thigh, 0, 5);
  f.turn(thigh, 10, 30);
  f.turn(thigh, 20, 10);
  QCOMPARE(TimelineClip::removeKeys(&f.anim, TimelineRange{0, 15, {thigh}}), 1);
  QCOMPARE(f.node(thigh)->keyframeList(), QList<int>({0, 20}));
  COMPARE_VEC(f.node(thigh)->frameData(0).rotation(), QVector3D(5, 0, 0));
  QCOMPARE(TimelineClip::removeKeys(&f.anim, TimelineRange{21, 30, {thigh}}), 0);
}

void TimelineClipTest::clipPastesIntoAnotherAnimationByName()
{
  TimelineClip clip;
  {
    Fixture f;
    f.turn(f.track("lThigh"), 5, 22, 3, 0);
    clip.copy(&f.anim, TimelineRange{5, 5, {f.track("lThigh")}});
  }
  Fixture g;
  clip.paste(&g.anim, 7, true);
  COMPARE_VEC(g.node("rThigh")->frameData(7).rotation(), QVector3D(22, -3, 0));
}

void TimelineClipTest::mirrorPartners()
{
  Fixture f;
  QCOMPARE(TimelineClip::mirrorTrack(&f.anim, f.track("lThigh")), f.track("rThigh"));
  QCOMPARE(TimelineClip::mirrorTrack(&f.anim, f.track("rShin")), f.track("lShin"));
  QCOMPARE(TimelineClip::mirrorTrack(&f.anim, f.track("lCollar")), f.track("rCollar"));
  QCOMPARE(TimelineClip::mirrorTrack(&f.anim, f.track("mHandIndex1Left")), f.track("mHandIndex1Right"));
  QCOMPARE(TimelineClip::mirrorTrack(&f.anim, f.track("head")), f.track("head"));
  QCOMPARE(TimelineClip::mirrorTrack(&f.anim, 0), 0);
  QCOMPARE(TimelineClip::trackOf(&f.anim, QStringLiteral("noSuchJoint")), -1);
}

void TimelineClipTest::pasteRedrawsRowsAndMarksChanged()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int rThigh = f.track("rThigh");
  f.turn(lThigh, 5, 30);
  f.anim.setDirty(false);
  QCOMPARE(f.timeline()->rows().tracks(), QVector<int>({lThigh}));
  TimelineClip clip;
  clip.copy(&f.anim, TimelineRange{0, 5, {lThigh}});
  QSignalSpy changed(&f.anim, &Animation::frameChanged);
  clip.paste(&f.anim, 10, true);
  // the right leg changed, so it has a row now, after the left leg in skeleton order
  QVERIFY(f.timeline()->rows().rowOfTrack(rThigh) >= 0);
  QVERIFY(f.anim.dirty());
  QCOMPARE(changed.count(), 1);
}

void TimelineClipTest::shiftDragSelectsAStretch()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int lShin = f.track("lShin");
  const int rThigh = f.track("rThigh");
  f.turn(lThigh, 5, 30);
  f.turn(lShin, 5, 20);
  f.turn(rThigh, 5, 10);
  f.anim.setFrame(0);
  f.showView();
  Timeline* t = f.timeline();
  const int row0 = t->rows().rowOfTrack(lThigh);
  const int row1 = t->rows().rowOfTrack(lShin);
  QCOMPARE(row1, row0 + 1);
  QSignalSpy changed(t, &Timeline::selectionChanged);
  QTest::mousePress(t, Qt::LeftButton, Qt::ShiftModifier, Fixture::at(3, row0));
  QTest::mouseMove(t, Fixture::at(8, row1));
  QTest::mouseRelease(t, Qt::LeftButton, Qt::ShiftModifier, Fixture::at(8, row1));
  QVERIFY(t->hasSelection());
  QCOMPARE(t->selection(), (TimelineRange{3, 8, {lThigh, lShin}}));
  QVERIFY(changed.count() >= 2);
  // the current frame follows the far corner
  QCOMPARE(f.anim.getFrame(), 8);
  // a drag back past the start turns the stretch round
  QTest::mousePress(t, Qt::LeftButton, Qt::ShiftModifier, Fixture::at(1, row0));
  QCOMPARE(t->selection(), (TimelineRange{1, 3, {lThigh}}));
}

void TimelineClipTest::shiftClickMovesTheFarCornerAndAPlainClickLeaves()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int lShin = f.track("lShin");
  const int rThigh = f.track("rThigh");
  f.turn(lThigh, 5, 30);
  f.turn(lShin, 5, 20);
  f.turn(rThigh, 5, 10);
  f.showView();
  Timeline* t = f.timeline();
  const int row = t->rows().rowOfTrack(lThigh);
  QTest::mouseClick(t, Qt::LeftButton, Qt::ShiftModifier, Fixture::at(2, row));
  QCOMPARE(t->selection(), (TimelineRange{2, 2, {lThigh}}));
  QTest::mouseClick(t, Qt::LeftButton, Qt::ShiftModifier, Fixture::at(12, t->rows().rowOfTrack(rThigh)));
  QCOMPARE(t->selection(), (TimelineRange{2, 12, {lThigh, lShin, rThigh}}));
  // a plain click picks the joint and leaves the stretch
  QSignalSpy changed(t, &Timeline::selectionChanged);
  QTest::mouseClick(t, Qt::LeftButton, {}, Fixture::at(9, row));
  QVERIFY(!t->hasSelection());
  QVERIFY(t->selection().isEmpty());
  QCOMPARE(changed.count(), 1);
  QCOMPARE(t->selectedTrack(), lThigh);
}

void TimelineClipTest::shiftOnAKeyStillCopiesTheKey()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  f.turn(lThigh, 5, 30);
  f.showView();
  Timeline* t = f.timeline();
  const int row = t->rows().rowOfTrack(lThigh);
  QTest::mousePress(t, Qt::LeftButton, Qt::ShiftModifier, Fixture::at(5, row));
  QVERIFY(!t->hasSelection());
  QTest::mouseRelease(t, Qt::LeftButton, Qt::ShiftModifier, Fixture::at(5, row));
}

void TimelineClipTest::keyboardGrowsAndShrinksTheStretch()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int lShin = f.track("lShin");
  f.turn(lThigh, 5, 30);
  f.turn(lShin, 5, 20);
  f.showView();
  Timeline* t = f.timeline();
  f.view.selectTrack(lThigh);
  f.anim.setFrame(4);
  t->setFocus();
  for (int i = 0; i < 3; ++i) QTest::keyClick(t, Qt::Key_Right, Qt::ShiftModifier);
  QCOMPARE(t->selection(), (TimelineRange{4, 7, {lThigh}}));
  QCOMPARE(f.anim.getFrame(), 7);
  QTest::keyClick(t, Qt::Key_Down, Qt::ShiftModifier);
  QCOMPARE(t->selection(), (TimelineRange{4, 7, {lThigh, lShin}}));
  // past the last row nothing changes
  QTest::keyClick(t, Qt::Key_Down, Qt::ShiftModifier);
  QCOMPARE(t->selection(), (TimelineRange{4, 7, {lThigh, lShin}}));
  QTest::keyClick(t, Qt::Key_Up, Qt::ShiftModifier);
  for (int i = 0; i < 5; ++i) QTest::keyClick(t, Qt::Key_Left, Qt::ShiftModifier);
  QCOMPARE(t->selection(), (TimelineRange{2, 4, {lThigh}}));
  // not before frame 0
  for (int i = 0; i < 5; ++i) QTest::keyClick(t, Qt::Key_Left, Qt::ShiftModifier);
  QCOMPARE(t->selection(), (TimelineRange{0, 4, {lThigh}}));
  // the picked joint stays the picked joint
  QCOMPARE(t->selectedTrack(), lThigh);
}

void TimelineClipTest::escapeLeavesTheStretch()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  f.turn(lThigh, 5, 30);
  f.showView();
  Timeline* t = f.timeline();
  f.view.selectTrack(lThigh);
  t->setFocus();
  QTest::keyClick(t, Qt::Key_Right, Qt::ShiftModifier);
  QVERIFY(t->hasSelection());
  QTest::keyClick(t, Qt::Key_Escape);
  QVERIFY(!t->hasSelection());
}

void TimelineClipTest::copyRangeWithoutSelectionIsTheCurrentFrame()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int head = f.track("head");
  f.turn(head, 5, 30);
  f.turn(lThigh, 5, 30);
  f.anim.setFrame(9);
  const TimelineRange range = f.timeline()->copyRange();
  QCOMPARE(range.firstFrame, 9);
  QCOMPARE(range.lastFrame, 9);
  QCOMPARE(range.tracks, QList<int>(f.timeline()->rows().tracks().begin(), f.timeline()->rows().tracks().end()));
  QVERIFY(range.tracks.contains(lThigh) && range.tracks.contains(head));
  f.timeline()->setSelection(lThigh, 2, lThigh, 6);
  QCOMPARE(f.timeline()->copyRange(), (TimelineRange{2, 6, {lThigh}}));
}

void TimelineClipTest::newAnimationLeavesTheStretch()
{
  Fixture f;
  const int lThigh = f.track("lThigh");
  f.turn(lThigh, 5, 30);
  f.timeline()->setSelection(lThigh, 2, lThigh, 6);
  QVERIFY(f.timeline()->hasSelection());
  BVH bvh2;
  Animation other(&bvh2);
  f.timeline()->setAnimation(&other);
  QVERIFY(!f.timeline()->hasSelection());
  f.timeline()->setAnimation(&f.anim);
}

void TimelineClipTest::selectionBoxIsDrawnInTheSelectionColour()
{
  for (bool dark : {true, false})
  {
    QCOMPARE(TimelineLook::selectionBoxBorder(dark), Theme::color(Theme::Token::Selection, dark));
    QColor box = Theme::color(Theme::Token::Selection, dark);
    box.setAlphaF(0.22);
    QCOMPARE(TimelineLook::selectionBox(dark), box);
  }
  Fixture f;
  const int lThigh = f.track("lThigh");
  const int head = f.track("head");
  f.turn(head, 30, 30);
  f.turn(lThigh, 30, 30);
  f.anim.setFrame(39);
  f.showView();
  Timeline* t = f.timeline();
  const int row = t->rows().rowOfTrack(lThigh);
  // a point on the row's background, away from the middle line, keys and the current frame
  const QPoint inside(4 * Constants::keyWidth() + 2, row * Constants::lineHeight() + 2);
  const QImage beforeImage = t->grab().toImage();
  const QColor before = beforeImage.pixelColor(inside);
  t->setSelection(lThigh, 2, lThigh, 8);
  const QImage after = t->grab().toImage();
  const QColor sel = Theme::color(Theme::Token::Selection, Theme::isDark());
  const QColor got = after.pixelColor(inside);
  auto mixed = [&](int a, int b) { return qRound(a * 0.78 + b * 0.22); };
  QVERIFY2(qAbs(got.red() - mixed(before.red(), sel.red())) <= 2 && qAbs(got.green() - mixed(before.green(), sel.green())) <= 2
             && qAbs(got.blue() - mixed(before.blue(), sel.blue())) <= 2,
           qPrintable(got.name() + QLatin1String(" from ") + before.name()));
  // outside the stretch nothing changed
  const QPoint outside(12 * Constants::keyWidth() + 2, row * Constants::lineHeight() + 2);
  QCOMPARE(after.pixelColor(outside), beforeImage.pixelColor(outside));
  QCOMPARE(after.pixelColor(QPoint(2 * Constants::keyWidth(), row * Constants::lineHeight() + 3)), sel);
}

void TimelineClipTest::messages()
{
  QCOMPARE(ClipSpec::jointCount(1), QStringLiteral("1 joint"));
  QCOMPARE(ClipSpec::jointCount(3), QStringLiteral("3 joints"));
  // frames counted from 1, as in the frame field
  QCOMPARE(ClipSpec::selectedMessage(TimelineRange{3, 8, {5, 6}}), QStringLiteral("Selected frames 4 to 9 on 2 joints. Ctrl+C copies them."));
  QCOMPARE(ClipSpec::selectedMessage(TimelineRange{}), QString());
  QCOMPARE(ClipSpec::copyMessage(TimelineRange{3, 8, {5, 6}}, false), QStringLiteral("Copied frames 4 to 9 of 2 joints."));
  QCOMPARE(ClipSpec::copyMessage(TimelineRange{3, 3, {5}}, false), QStringLiteral("Copied frame 4 of 1 joint."));
  QCOMPARE(ClipSpec::copyMessage(TimelineRange{3, 8, {5}}, true), QStringLiteral("Cut frames 4 to 9 of 1 joint."));
  QCOMPARE(ClipSpec::copyMessage(TimelineRange{0, 0, {5, 6, 7}}, true), QStringLiteral("Cut frame 1 of 3 joints."));
  TimelineClip::PasteResult result;
  result.keys = 6;
  result.tracks = {5, 6};
  QCOMPARE(ClipSpec::pasteMessage(result, 14, false), QStringLiteral("Pasted at frame 15 on 2 joints."));
  QCOMPARE(ClipSpec::pasteMessage(result, 14, true), QStringLiteral("Pasted mirrored at frame 15 on 2 joints. Left and right are swapped."));
  result.leftOut = 1;
  QCOMPARE(ClipSpec::pasteMessage(result, 14, false), QStringLiteral("Pasted at frame 15 on 2 joints. 1 key went past the last frame and was left out."));
  result.leftOut = 3;
  QCOMPARE(ClipSpec::pasteMessage(result, 14, true),
           QStringLiteral("Pasted mirrored at frame 15 on 2 joints. Left and right are swapped. 3 keys went past the last frame and were left out."));
}

void TimelineClipTest::reasons()
{
  const QString open = QStringLiteral("Open an animation first.");
  const QString copyFirst = QStringLiteral("Copy a stretch of the timeline first (select keys, then Ctrl+C).");
  const QString stop = QStringLiteral("Stop playing first.");
  const QString locked = QStringLiteral("The first frame is locked. Turn off Lock the first frame to set a key.");
  // in this order: no animation, nothing copied, playing, first frame locked
  QCOMPARE(ClipSpec::pasteBlocked(false, true, true, true, 0), open);
  QCOMPARE(ClipSpec::pasteBlocked(true, true, true, true, 0), copyFirst);
  QCOMPARE(ClipSpec::pasteBlocked(true, false, true, true, 0), stop);
  QCOMPARE(ClipSpec::pasteBlocked(true, false, false, true, 0), locked);
  QCOMPARE(ClipSpec::pasteBlocked(true, false, false, true, 5), QString());
  QCOMPARE(ClipSpec::pasteBlocked(true, false, false, false, 0), QString());
  QCOMPARE(ClipSpec::copyBlocked(false, true, true), open);
  QCOMPARE(ClipSpec::copyBlocked(true, true, true), stop);
  QCOMPARE(ClipSpec::copyBlocked(true, true, false), QStringLiteral("Nothing to copy yet. Set a key first."));
  QCOMPARE(ClipSpec::copyBlocked(true, false, false), QString());
  // the Paste mirrored reason is the mockup's, word for word
  QVERIFY(mockup().contains(QStringLiteral("\"Copy a stretch of the timeline first (select keys, then Ctrl+C).\"")));
}

void TimelineClipTest::savedAndOpenedAgain()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("step.bvh"));
  QList<QVector3D> expected;
  {
    Fixture f;
    const int lThigh = f.track("lThigh");
    f.turn(lThigh, 5, -30, 10, 5);
    f.turn(lThigh, 10, 15, -4, 2);
    TimelineClip clip;
    clip.copy(&f.anim, TimelineRange{0, 10, {lThigh}});
    clip.paste(&f.anim, 15, true);
    for (int frame = 0; frame < 40; ++frame) expected << f.node("rThigh")->frameData(frame).rotation();
    QString error;
    QVERIFY2(f.anim.saveBVH(path, &error), qPrintable(error));
  }
  BVH bvh;
  Animation reopened(&bvh, path);
  QCOMPARE(reopened.getNumberOfFrames(), 40);
  BVHNode* rThigh = reopened.getNode(TimelineClip::trackOf(&reopened, QStringLiteral("rThigh")));
  QVERIFY(rThigh);
  for (int frame = 0; frame < 40; ++frame)
    QVERIFY2(same(rThigh->frameData(frame).rotation(), expected[frame], 0.01f),
             qPrintable(QStringLiteral("frame %1: ").arg(frame)) + show(rThigh->frameData(frame).rotation()) + " is not " + show(expected[frame]));
}

void TimelineClipTest::buttonAndMenuAsInTheMockup()
{
  const QString page = mockup();
  // the mockup's timeline button and menu item: flip-horizontal-2, "Paste mirrored", Ctrl+Shift+V, Expert mode only
  QVERIFY(page.contains(QStringLiteral("<button class=\"btn expert-only\" data-cmd=\"pastemirror\" data-name=\"Paste mirrored\" data-key=\"Ctrl+Shift+V\"><svg class=\"i\" data-i=\"flip-horizontal-2\"></svg>Paste mirrored</button>")));
  QCOMPARE(ClipSpec::text(ClipSpec::pasteMirroredButton), QStringLiteral("Paste mirrored"));
  QCOMPARE(MenuSpec::text(MenuSpec::editPasteMirrored), QStringLiteral("Paste mirrored"));
  QCOMPARE(QLatin1String(MenuSpec::editPasteMirrored.key), QLatin1String("Ctrl+Shift+V"));
  QCOMPARE(QLatin1String(MenuSpec::editPasteMirrored.reason), QLatin1String(""));
  QPushButton button;
  button.setObjectName(QStringLiteral("pasteMirroredButton"));
  QCOMPARE(TransportLook::iconName(&button), QStringLiteral("flip-horizontal-2"));
  QCOMPARE(TransportLook::kind(&button), TransportLook::Kind::Normal);
  QVERIFY(LucideIcons::names().contains(QStringLiteral("flip-horizontal-2")));
  QVERIFY(!LucideIcons::path(QStringLiteral("flip-horizontal-2")).isEmpty());

  // the panel puts it right of Set key; the window shows it in Expert mode only and
  // greys it, the menu item, Copy, Cut and Paste with their reasons
  const QString panels = source("panelcontent.h");
  const int setKey = panels.indexOf(QStringLiteral("keyBarLayout->addWidget(setKey);"));
  const int paste = panels.indexOf(QStringLiteral("keyBarLayout->addWidget(pasteMirrored);"));
  QVERIFY(setKey >= 0 && paste > setKey);
  QVERIFY(panels.contains(QStringLiteral("pasteMirrored->setObjectName(QStringLiteral(\"pasteMirroredButton\"));")));
  const QString window = source("pinglymotionwindow.cpp");
  QVERIFY(window.contains(QStringLiteral("m_panels->pasteMirrored->setVisible(UserSettings::expertMode());")));
  QVERIFY(window.contains(QStringLiteral("if(m_panels && m_panels->pasteMirrored) m_panels->pasteMirrored->setVisible(on);")));
  QVERIFY(window.contains(QStringLiteral("connect(m_panels->pasteMirrored,&QPushButton::clicked,this,&PinglyMotionWindow::pasteMirroredFromMenu);")));
  for (const char* line : {"grey(ui->editCopyAction,copyWhy);", "grey(ui->editCutAction,copyWhy);", "grey(ui->editPasteAction,pasteWhy);",
                           "grey(m_pasteMirroredAction,pasteWhy);", "grey(m_panels->pasteMirrored,pasteWhy);"})
    QVERIFY2(window.contains(QLatin1String(line)), line);
}

QTEST_MAIN(TimelineClipTest)
#include "tst_timelineclip.moc"
