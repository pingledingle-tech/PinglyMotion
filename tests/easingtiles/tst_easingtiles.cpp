// Tests of the easing tile row (src/easingtiles.*, issue #53 step 5).
//
// - Clicking each tile stores the right switches or curve on the keys of the
//   pose change that ends at the selected key frame.
// - A .dingle file with a stored curve shows the right tile as current.
// - The row is greyed out with the "select a key frame" line when no pose
//   change is selected, and the figures move only while a choice is shown.
#include <QtTest>
#include <QTemporaryDir>
#include <QLabel>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "dingle.h"
#include "easing.h"
#include "easingtiles.h"

static QByteArray readAll(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return QByteArray();
  return file.readAll();
}

static BVHNode* findJoint(BVHNode* node, const QString& name)
{
  if (!node) return nullptr;
  if (node->name() == name && node->type != BVH_END) return node;
  for (int i = 0; i < node->numChildren(); i++)
    if (BVHNode* found = findJoint(node->child(i), name)) return found;
  return nullptr;
}

class EasingTilesTest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void sixTilesInOrder();
    void noKeySelected();
    void clickStoresSwitchesOrCurve_data();
    void clickStoresSwitchesOrCurve();
    void clickEmitsAndMarks();
    void loadedCurveShowsTile_data();
    void loadedCurveShowsTile();
    void segmentEndingAtKey();
    void hintFollowsTips();
    void animatesOnlyWhileShown();

  private:
    // the jump example, with the joint and two neighbouring keys used below
    bool openJump(BVH& bvh, QScopedPointer<Animation>& anim, BVHNode*& joint, int& from, int& to);
};

void EasingTilesTest::initTestCase()
{
  QVERIFY2(QFile::exists(PINGLYMOTION_EXAMPLES "/jump.dingle"), "examples/jump.dingle is needed");
}

bool EasingTilesTest::openJump(BVH& bvh, QScopedPointer<Animation>& anim, BVHNode*& joint, int& from, int& to)
{
  anim.reset(new Animation(&bvh, PINGLYMOTION_EXAMPLES "/jump.dingle"));
  joint = findJoint(anim->getMotion(), "lShldr");
  if (!joint) return false;
  const QList<int> keys = joint->keyframeList();
  if (keys.size() < 2) return false;
  from = keys[0];
  to = keys[1];
  return to > from;
}

void EasingTilesTest::sixTilesInOrder()
{
  EasingTiles row;
  const QStringList names = {"Even", "Slow start", "Slow finish", "Slow both ends",
                             "Overshoot, then settle", "Hold, then snap"};
  for (int i = 0; i < Easing::PresetCount; i++)
  {
    const Easing::Preset preset = static_cast<Easing::Preset>(i);
    EasingTile* tile = row.tile(preset);
    QVERIFY(tile);
    QCOMPARE(tile->preset(), preset);
    QCOMPARE(tile->text(), names[i]);
    QVERIFY(tile->isCheckable());
    // the tiles sit left to right in the order of the choices
    if (i > 0) QVERIFY(tile->x() >= row.tile(static_cast<Easing::Preset>(i - 1))->x());
  }
  // the hint texts are the approved ones (docs/TEACHING.md EASE-01 to 06)
  QVERIFY(EasingChoice::hint(Easing::Preset::Even).startsWith("Same speed all the way"));
  QVERIFY(EasingChoice::hint(Easing::Preset::HoldThenSnap).contains("blink"));
}

void EasingTilesTest::noKeySelected()
{
  EasingTiles row;
  row.setTipsOn(true);
  QVERIFY(!row.hasChoice());
  QCOMPARE(row.hintText(), QString("Select a key frame on the timeline"));
  for (int i = 0; i < Easing::PresetCount; i++)
  {
    QVERIFY(!row.tile(static_cast<Easing::Preset>(i))->isEnabled());
    QVERIFY(!row.tile(static_cast<Easing::Preset>(i))->isChecked());
  }
  // a click on a greyed out tile changes nothing
  QSignalSpy spy(&row, &EasingTiles::choiceClicked);
  row.tile(Easing::Preset::Even)->click();
  QCOMPARE(spy.count(), 0);

  // the same line shows with the tips off: it is the state, not a tip
  row.setTipsOn(false);
  QCOMPARE(row.hintText(), QString("Select a key frame on the timeline"));
}

void EasingTilesTest::clickStoresSwitchesOrCurve_data()
{
  QTest::addColumn<int>("preset");
  QTest::addColumn<bool>("startEaseOut");
  QTest::addColumn<bool>("endEaseIn");
  QTest::addColumn<bool>("hasCurve");
  QTest::newRow("Even") << 0 << false << false << false;
  QTest::newRow("Slow start") << 1 << true << false << false;
  QTest::newRow("Slow finish") << 2 << false << true << false;
  QTest::newRow("Slow both ends") << 3 << true << true << false;
  QTest::newRow("Overshoot, then settle") << 4 << true << true << true;
  QTest::newRow("Hold, then snap") << 5 << true << true << true;
}

// clicking a tile stores the choice on the two keys of the segment, as the
// file format wants it (docs/EASING.md): switches for the four old-style
// choices, the start key's curve for the two new ones
void EasingTilesTest::clickStoresSwitchesOrCurve()
{
  QFETCH(int, preset);
  QFETCH(bool, startEaseOut);
  QFETCH(bool, endEaseIn);
  QFETCH(bool, hasCurve);
  const Easing::Preset chosen = static_cast<Easing::Preset>(preset);

  BVH bvh;
  QScopedPointer<Animation> anim;
  BVHNode* joint = nullptr;
  int from = 0, to = 0;
  QVERIFY(openJump(bvh, anim, joint, from, to));
  anim->setDirty(false);

  // start from a different stored state, so every row really changes something
  anim->setCurve(joint, from, chosen == Easing::Preset::HoldThenSnap ? Easing::Preset::OvershootThenSettle
                                                                   : Easing::Preset::HoldThenSnap);
  anim->setEaseOut(joint, from, !startEaseOut);
  anim->setEaseIn(joint, to, !endEaseIn);
  // a neighbour's switches must not move: the end key's easeOut belongs to the next segment
  anim->setEaseOut(joint, to, true);

  EasingTiles row;
  row.setChoice(EasingChoice::choiceOf(joint, from, to));
  QSignalSpy spy(&row, &EasingTiles::choiceClicked);
  QSignalSpy redraw(anim.data(), &Animation::redrawTrack);
  row.tile(chosen)->click();
  QCOMPARE(spy.count(), 1);
  // the window does this on the signal
  EasingChoice::setChoice(anim.data(), joint, from, to, chosen);

  QCOMPARE(joint->easeOut(from), startEaseOut);
  QCOMPARE(joint->easeIn(to), endEaseIn);
  QCOMPARE(joint->curve(from).has_value(), hasCurve);
  if (hasCurve) QCOMPARE(*joint->curve(from), chosen);
  QVERIFY(!joint->curve(to));
  QVERIFY(joint->easeOut(to));
  QCOMPARE(EasingChoice::choiceOf(joint, from, to), chosen);
  QVERIFY(anim->dirty());
  QVERIFY(redraw.count() >= 1);

  // reading it back marks the same tile
  row.setChoice(EasingChoice::choiceOf(joint, from, to));
  QVERIFY(row.tile(chosen)->isChecked());

  // the choice survives a save: the file has the curve only for the new choices
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.path() + "/tiles.dingle";
  QString error;
  QVERIFY2(anim->saveBVH(path, &error), qPrintable(error));
  const QByteArray text = readAll(path);
  const QByteArray stored = "\"curve\": {\"type\": \"" + QByteArray(Easing::technicalName(chosen)) + "\"}";
  QCOMPARE(text.contains(stored), hasCurve);
  if (!hasCurve) QVERIFY(!text.contains("\"curve\""));
}

void EasingTilesTest::clickEmitsAndMarks()
{
  EasingTiles row;
  row.setChoice(Easing::Preset::SlowBothEnds);
  QVERIFY(row.hasChoice());
  QVERIFY(row.tile(Easing::Preset::SlowBothEnds)->isChecked());
  for (int i = 0; i < Easing::PresetCount; i++)
    QVERIFY(row.tile(static_cast<Easing::Preset>(i))->isEnabled());

  QSignalSpy spy(&row, &EasingTiles::choiceClicked);
  row.tile(Easing::Preset::OvershootThenSettle)->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().first().value<Easing::Preset>(), Easing::Preset::OvershootThenSettle);
  QCOMPARE(row.choice(), Easing::Preset::OvershootThenSettle);
  QVERIFY(row.tile(Easing::Preset::OvershootThenSettle)->isChecked());
  QVERIFY(!row.tile(Easing::Preset::SlowBothEnds)->isChecked());

  // the window setting the choice does not count as a click
  row.setChoice(Easing::Preset::Even);
  QCOMPARE(spy.count(), 1);
  QVERIFY(row.tile(Easing::Preset::Even)->isChecked());
}

void EasingTilesTest::loadedCurveShowsTile_data()
{
  QTest::addColumn<int>("preset");
  for (int i = 0; i < Easing::PresetCount; i++)
    QTest::newRow(Easing::plainName(static_cast<Easing::Preset>(i))) << i;
}

// a .dingle file with a stored choice opens with the right tile marked
void EasingTilesTest::loadedCurveShowsTile()
{
  QFETCH(int, preset);
  const Easing::Preset stored = static_cast<Easing::Preset>(preset);

  // write the file with the model, the way the format test does
  const Dingle::Result read = Dingle::parse(readAll(PINGLYMOTION_EXAMPLES "/jump.dingle"));
  QVERIFY2(read.ok(), qPrintable(read.error));
  AnimFile::Animation model = *read.animation;
  const int joint = model.findJoint("lShldr");
  QVERIFY(joint >= 0);
  QVERIFY(model.joints[joint].keys.size() >= 2);
  const int from = model.joints[joint].keys[0].frame;
  const int to = model.joints[joint].keys[1].frame;
  model.joints[joint].keys[0].curve = stored;
  // switches that would say something else, so the curve must win
  model.joints[joint].keys[0].easeOut = false;
  model.joints[joint].keys[1].easeIn = false;

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.path() + "/stored.dingle";
  {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(Dingle::write(model));
  }

  BVH bvh;
  Animation anim(&bvh, path);
  BVHNode* node = findJoint(anim.getMotion(), "lShldr");
  QVERIFY(node);
  int start = -1;
  QVERIFY(EasingChoice::segmentEndingAt(node, to, start));
  QCOMPARE(start, from);

  EasingTiles row;
  row.setChoice(EasingChoice::choiceOf(node, start, to));
  for (int i = 0; i < Easing::PresetCount; i++)
    QCOMPARE(row.tile(static_cast<Easing::Preset>(i))->isChecked(), i == preset);
}

void EasingTilesTest::segmentEndingAtKey()
{
  BVH bvh;
  QScopedPointer<Animation> anim;
  BVHNode* joint = nullptr;
  int from = 0, to = 0;
  QVERIFY(openJump(bvh, anim, joint, from, to));

  int start = -1;
  // the first key has no pose change before it
  QVERIFY(!EasingChoice::segmentEndingAt(joint, 0, start));
  // a frame that is not a key frame has none either
  int between = -1;
  for (int f = from + 1; f < to; f++)
    if (!joint->isKeyframe(f)) { between = f; break; }
  if (between >= 0) QVERIFY(!EasingChoice::segmentEndingAt(joint, between, start));
  // no joint, no segment
  QVERIFY(!EasingChoice::segmentEndingAt(nullptr, to, start));
  // the second key's change starts at the first key
  QVERIFY(EasingChoice::segmentEndingAt(joint, to, start));
  QCOMPARE(start, from);
  // without a stored curve the two switches decide
  anim->setCurve(joint, from, std::nullopt);
  anim->setEaseOut(joint, from, true);
  anim->setEaseIn(joint, to, false);
  QCOMPARE(EasingChoice::choiceOf(joint, from, to), Easing::Preset::SlowStart);
  anim->setEaseOut(joint, from, false);
  anim->setEaseIn(joint, to, true);
  QCOMPARE(EasingChoice::choiceOf(joint, from, to), Easing::Preset::SlowFinish);
}

void EasingTilesTest::hintFollowsTips()
{
  EasingTiles row;
  row.setTipsOn(true);
  row.setChoice(Easing::Preset::SlowStart);
  QCOMPARE(row.hintText(), EasingChoice::hint(Easing::Preset::SlowStart));
  QCOMPARE(row.tile(Easing::Preset::Even)->toolTip(), EasingChoice::hint(Easing::Preset::Even));

  row.setTipsOn(false);
  QVERIFY(row.hintText().isEmpty());
  QVERIFY(row.tile(Easing::Preset::Even)->toolTip().isEmpty());

  row.setTipsOn(true);
  row.setChoice(Easing::Preset::HoldThenSnap);
  QCOMPARE(row.hintText(), EasingChoice::hint(Easing::Preset::HoldThenSnap));
}

// the figures move only while the row is on screen and a choice is shown
void EasingTilesTest::animatesOnlyWhileShown()
{
  EasingTiles row;
  row.setChoice(Easing::Preset::Even);
  QVERIFY(!row.isAnimating());
  row.show();
  QVERIFY(QTest::qWaitForWindowExposed(&row));
  QVERIFY(row.isAnimating());
  row.setNoKey();
  QVERIFY(!row.isAnimating());
  row.setChoice(Easing::Preset::Even);
  QVERIFY(row.isAnimating());
  row.hide();
  QVERIFY(!row.isAnimating());
}

QTEST_MAIN(EasingTilesTest)
#include "tst_easingtiles.moc"
