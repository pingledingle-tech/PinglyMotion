// Tests of the easing tile row (src/easingtiles.*, issue #53 step 5).
//
// - Clicking each tile stores the right switches or curve on the keys of the
//   pose change that ends at the selected key frame.
// - A .dingle file with a stored curve shows the right tile as current.
// - The row is greyed out with the "select a key frame" line when no pose
//   change is selected.
// - The six names are on the tiles as text, and only the tile under the mouse
//   moves its figure (UX step 3); without a hover the row is still.
#include <QtTest>
#include <QTemporaryDir>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMouseEvent>
#include <QApplication>
#include <cmath>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "dingle.h"
#include "dialogfit.h"
#include "easing.h"
#include "easingtiles.h"
#include "textsize.h"
#include "theme.h"
#include "uirender.h"
#include "usersettings.h"

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
    void namesAreTextOnTheTiles();
    void previewMovesOnlyOnHoveredTile();
    void customButtonFollowsTheKey();
    void customStateMarksButtonNotTiles();
    void customCheckRunsOnSetChoice();
    void customCurveStoredPlayedAndSaved();
    void customCurveStaysInsideLimits();
    void editorDragsHandles();
    void dialogStartAgainAndLimits();
    // UX phase 5 (the approved mockup, docs/UX_DECISIONS.md sections 2 and 3)
    void headingAndExpertOnlyEditButton();
    void greyedRowSaysWhy();
    void customLineShowsWithTipsOff();
    void tabReachesTheCurrentTileOnly();
    void arrowKeysPickTheNextChoice();
    void curveEditorKeyboard();
    void editCurveWindowKeepsItsPlace();
    void editCurveWindowStaysOnTheScreen();
    void editCurveWindowAtDoubleText();
    void pictures();
    void choiceIsPerJoint();

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
  QCOMPARE(row.hintText(), QString("Select a key on the timeline"));
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
  QCOMPARE(row.hintText(), QString("Select a key on the timeline"));
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

// show the row with the mouse far away, so no tile is under it by chance
static bool showAwayFromMouse(EasingTiles& row)
{
  QCursor::setPos(QPoint(-10000, -10000));
  row.move(200, 200);
  row.show();
  if (!QTest::qWaitForWindowExposed(&row)) return false;
  for (int i = 0; i < Easing::PresetCount; i++)
  {
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(row.tile(static_cast<Easing::Preset>(i)), &leave);
  }
  return true;
}

static void hoverTile(EasingTile* tile)
{
  const QPointF centre(tile->width() / 2.0, tile->height() / 2.0);
  QEnterEvent enter(centre, tile->mapTo(tile->window(), centre), tile->mapToGlobal(centre));
  QApplication::sendEvent(tile, &enter);
}

static void leaveTile(EasingTile* tile)
{
  QEvent leave(QEvent::Leave);
  QApplication::sendEvent(tile, &leave);
}

// the figure moves only while the row is on screen, a choice is shown and the
// mouse is over a tile; with no hover the row is still
void EasingTilesTest::animatesOnlyWhileShown()
{
  EasingTiles row;
  row.setChoice(Easing::Preset::Even);
  QVERIFY(!row.isAnimating());
  QVERIFY(showAwayFromMouse(row));
  // shown with a choice, but no hover: nothing moves
  QVERIFY(!row.isAnimating());
  hoverTile(row.tile(Easing::Preset::Even));
  QVERIFY(row.isAnimating());
  row.setNoKey();
  QVERIFY(!row.isAnimating());
  row.setChoice(Easing::Preset::Even);
  QVERIFY(row.isAnimating());
  row.hide();
  QVERIFY(!row.isAnimating());
  // hiding forgets the hover: shown again, the row is still
  QVERIFY(showAwayFromMouse(row));
  QVERIFY(!row.isAnimating());
}

// the six names (docs/EASING.md, unchanged) are the text of the tiles, in order
void EasingTilesTest::namesAreTextOnTheTiles()
{
  EasingTiles row;
  const QStringList expected = {
    QStringLiteral("Even"),
    QStringLiteral("Slow start"),
    QStringLiteral("Slow finish"),
    QStringLiteral("Slow both ends"),
    QStringLiteral("Overshoot, then settle"),
    QStringLiteral("Hold, then snap"),
  };
  QCOMPARE(int(Easing::PresetCount), expected.size());
  for (int i = 0; i < Easing::PresetCount; i++)
  {
    EasingTile* tile = row.findChild<EasingTile*>(QStringLiteral("easingTile%1").arg(i));
    QVERIFY(tile);
    QCOMPARE(tile->text(), expected.at(i));
  }
}

// only the hovered tile moves its figure; the others stay at the start, and
// leaving the tile stops it and puts the figure back at the start
void EasingTilesTest::previewMovesOnlyOnHoveredTile()
{
  EasingTiles row;
  row.setChoice(Easing::Preset::SlowBothEnds);
  QVERIFY(showAwayFromMouse(row));

  // still: no timer, every figure at the start
  QVERIFY(!row.isAnimating());
  QVERIFY(!row.animatingTile().has_value());
  QTest::qWait(200);
  for (int i = 0; i < Easing::PresetCount; i++)
    QCOMPARE(row.tile(static_cast<Easing::Preset>(i))->phase(), 0.0);

  EasingTile* hovered = row.tile(Easing::Preset::OvershootThenSettle);
  hoverTile(hovered);
  QVERIFY(row.isAnimating());
  QCOMPARE(row.animatingTile(), std::optional<Easing::Preset>(Easing::Preset::OvershootThenSettle));
  QTRY_VERIFY(hovered->phase() > 0.0);
  for (int i = 0; i < Easing::PresetCount; i++)
  {
    const Easing::Preset preset = static_cast<Easing::Preset>(i);
    if (preset != Easing::Preset::OvershootThenSettle) QCOMPARE(row.tile(preset)->phase(), 0.0);
  }

  // moving to the next tile: the new one moves, the old one is back at the start,
  // even when the enter of the new tile comes before the leave of the old one
  EasingTile* next = row.tile(Easing::Preset::HoldThenSnap);
  hoverTile(next);
  leaveTile(hovered);
  QCOMPARE(row.animatingTile(), std::optional<Easing::Preset>(Easing::Preset::HoldThenSnap));
  QCOMPARE(hovered->phase(), 0.0);
  QTRY_VERIFY(next->phase() > 0.0);

  leaveTile(next);
  QVERIFY(!row.isAnimating());
  QVERIFY(!row.animatingTile().has_value());
  for (int i = 0; i < Easing::PresetCount; i++)
    QCOMPARE(row.tile(static_cast<Easing::Preset>(i))->phase(), 0.0);

  // a greyed-out row does not move under the mouse
  row.setNoKey();
  hoverTile(row.tile(Easing::Preset::Even));
  QVERIFY(!row.isAnimating());
}

// ---------------------------------------------------------------- custom curve (issue #11)

void EasingTilesTest::customButtonFollowsTheKey()
{
  EasingTiles row;
  QPushButton* button = row.editCurveButton();
  QVERIFY(button);
  QCOMPARE(button->objectName(), QStringLiteral("editCurveButton"));
  QCOMPARE(button->text(), QStringLiteral("Edit curve..."));
  QVERIFY(!button->isCheckable());
  QVERIFY(!button->isEnabled());

  QSignalSpy spy(&row, &EasingTiles::editCurveRequested);
  row.setChoice(Easing::Preset::SlowBothEnds);
  QVERIFY(button->isEnabled());
  button->click();
  QCOMPARE(spy.count(), 1);
  // only the window changes the state, not the click
  QVERIFY(!row.isCustom());

  row.setNoKey();
  QVERIFY(!button->isEnabled());
  QVERIFY(!row.isCustom());
}

void EasingTilesTest::customStateMarksButtonNotTiles()
{
  EasingTiles row;
  row.setChoice(Easing::Preset::SlowStart);
  row.setCustom(true);
  QVERIFY(row.isCustom());
  for (int i = 0; i < Easing::PresetCount; i++)
    QVERIFY(!row.tile(static_cast<Easing::Preset>(i))->isChecked());
  QCOMPARE(row.hintText(), QStringLiteral("This pose change uses your own curve. Click a tile to go back."));

  // a tile click leaves the custom state and is reported like any click
  QSignalSpy spy(&row, &EasingTiles::choiceClicked);
  row.tile(Easing::Preset::Even)->click();
  QCOMPARE(spy.count(), 1);
  QVERIFY(!row.isCustom());
  QVERIFY(row.tile(Easing::Preset::Even)->isChecked());

  // without a key the custom state cannot be set
  row.setNoKey();
  row.setCustom(true);
  QVERIFY(!row.isCustom());
}

void EasingTilesTest::customCheckRunsOnSetChoice()
{
  EasingTiles row;
  bool custom = true;
  row.setCustomCheck([&custom]() { return custom; });
  row.setChoice(Easing::Preset::SlowBothEnds);
  QVERIFY(row.isCustom());
  QVERIFY(!row.tile(Easing::Preset::SlowBothEnds)->isChecked());
  custom = false;
  row.setChoice(Easing::Preset::SlowBothEnds);
  QVERIFY(!row.isCustom());
  QVERIFY(row.tile(Easing::Preset::SlowBothEnds)->isChecked());
}

void EasingTilesTest::customCurveStoredPlayedAndSaved()
{
  BVH bvh;
  QScopedPointer<Animation> anim;
  BVHNode* joint = nullptr;
  int from = 0, to = 0;
  QVERIFY(openJump(bvh, anim, joint, from, to));
  anim->setDirty(false);

  // the editor opens on the segment's named choice first
  const Easing::Preset before = EasingChoice::choiceOf(joint, from, to);
  QCOMPARE(EasingChoice::editorStart(joint, from, to), Easing::bezierFromPreset(before));

  // a curve inside the square, so no joint limit can cut it
  const Easing::Bezier curve{0.7, 0.05, 0.9, 0.5};
  EasingChoice::setCustomCurve(anim.data(), joint, from, to, curve);
  QVERIFY(anim->dirty());
  QCOMPARE(EasingChoice::customCurveOf(joint, from), std::optional<Easing::Bezier>(curve));
  QVERIFY(!joint->curve(from));
  QVERIFY(joint->easeOut(from));
  QVERIFY(joint->easeIn(to));
  QVERIFY(!joint->customCurve(to));
  QCOMPARE(EasingChoice::editorStart(joint, from, to), curve);

  // playback follows the curve on every channel
  const QVector3D a = joint->frameData(from).rotation();
  const QVector3D b = joint->frameData(to).rotation();
  for (int frame = from; frame <= to; frame++)
  {
    const QVector3D r = joint->frameData(frame).rotation();
    for (int axis = 0; axis < 3; axis++)
      QCOMPARE(r[axis], float(Easing::customSegmentValue(curve, a[axis], b[axis], to - from, frame - from)));
  }

  // saved and opened again, the curve is still there
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.path() + "/custom.dingle";
  QString error;
  QVERIFY2(anim->saveBVH(path, &error), qPrintable(error));
  QVERIFY(readAll(path).contains("\"curve\": {\"type\": \"bezier\", \"points\": [0.7, 0.05, 0.9, 0.5]}"));
  BVH bvh2;
  Animation again(&bvh2, path);
  BVHNode* joint2 = findJoint(again.getMotion(), "lShldr");
  QVERIFY(joint2);
  QCOMPARE(EasingChoice::customCurveOf(joint2, from), std::optional<Easing::Bezier>(curve));
  for (int frame = from; frame <= to; frame++)
    QCOMPARE(joint2->frameData(frame).rotation(), joint->frameData(frame).rotation());

  // the tile row shows it as custom when the check says so
  EasingTiles row;
  row.setCustomCheck([&]() { return EasingChoice::customCurveOf(joint2, from).has_value(); });
  row.setChoice(EasingChoice::choiceOf(joint2, from, to));
  QVERIFY(row.isCustom());

  // a tile click removes it again, and the file is as without it
  EasingChoice::setChoice(&again, joint2, from, to, Easing::Preset::Even);
  QVERIFY(!EasingChoice::customCurveOf(joint2, from));
  QCOMPARE(EasingChoice::choiceOf(joint2, from, to), Easing::Preset::Even);
  row.setChoice(EasingChoice::choiceOf(joint2, from, to));
  QVERIFY(!row.isCustom());
  const QString path2 = dir.path() + "/even.dingle";
  QVERIFY2(again.saveBVH(path2, &error), qPrintable(error));
  QVERIFY(!readAll(path2).contains("bezier"));
}

// a custom curve that goes past the new pose stops at the joint's limits,
// like "Overshoot, then settle" (data/SL.lim: lShldr X -135..90)
void EasingTilesTest::customCurveStaysInsideLimits()
{
  BVH bvh;
  QScopedPointer<Animation> anim;
  BVHNode* joint = nullptr;
  int from = 0, to = 0;
  QVERIFY(openJump(bvh, anim, joint, from, to));
  int channel = -1;
  for (int i = 0; i < joint->numChannels; i++)
    if (joint->channelType[i] == BVH_XROT) channel = i;
  QVERIFY(channel >= 0);
  const float high = float(joint->channelMax[channel]);
  QVERIFY2(high < 10000.0f, "the limits file was not loaded (run from bin/)");

  QVector3D start = joint->frameData(from).rotation();
  start.setX(0.0f);
  QVector3D end = joint->frameData(to).rotation();
  end.setX(high);
  joint->setKeyframeRotation(from, start);
  joint->setKeyframeRotation(to, end);
  const Easing::Bezier past{0.2, 1.5, 0.5, 1.5};
  EasingChoice::setCustomCurve(anim.data(), joint, from, to, past);

  bool cut = false;
  for (int frame = from; frame <= to; frame++)
  {
    const float x = joint->frameData(frame).rotation().x();
    QVERIFY2(x <= high, qPrintable(QString("frame %1 x %2").arg(frame).arg(x)));
    if (Easing::customSegmentValue(past, 0.0, high, to - from, frame - from) > high)
    {
      QCOMPARE(x, high);
      cut = true;
    }
  }
  QVERIFY(cut);
}

void EasingTilesTest::editorDragsHandles()
{
  CurveEditor editor;
  editor.resize(360, 300);
  editor.setCurve(Easing::Bezier{0.25, 0.25, 0.75, 0.75});
  QSignalSpy spy(&editor, &CurveEditor::curveChanged);

  // the first handle to a point inside the square
  const QPoint grab = editor.handlePosition(0).toPoint();
  const QPointF wanted = QPointF(0.5, 1.2);
  QPointF target;
  {
    // the widget point of (0.5, 1.2): search the round trip of toCurve
    const QPointF a = editor.handlePosition(0);
    const QPointF ca = editor.toCurve(a);
    QVERIFY(std::fabs(ca.x() - 0.25) < 1e-9 && std::fabs(ca.y() - 0.25) < 1e-9);
    const QPointF b = editor.handlePosition(1);
    const QPointF cb = editor.toCurve(b);
    // the mapping is linear, so two points give it
    const double sx = (b.x() - a.x()) / (cb.x() - ca.x());
    const double sy = (b.y() - a.y()) / (cb.y() - ca.y());
    target = QPointF(a.x() + (wanted.x() - ca.x()) * sx, a.y() + (wanted.y() - ca.y()) * sy);
  }
  QTest::mousePress(&editor, Qt::LeftButton, Qt::NoModifier, grab);
  QMouseEvent move(QEvent::MouseMove, target, editor.mapToGlobal(target), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(&editor, &move);
  QTest::mouseRelease(&editor, Qt::LeftButton, Qt::NoModifier, target.toPoint());
  const Easing::Bezier c = editor.curve();
  const QPointF expected = editor.toCurve(QPointF(target.toPoint()));
  QVERIFY2(std::fabs(c.x1 - expected.x()) < 1e-9 && std::fabs(c.y1 - expected.y()) < 1e-9,
           qPrintable(QString("%1 %2").arg(c.x1).arg(c.y1)));
  QVERIFY(std::fabs(c.x1 - 0.5) < 0.01 && std::fabs(c.y1 - 1.2) < 0.02);
  // the other handle did not move
  QCOMPARE(c.x2, 0.75);
  QCOMPARE(c.y2, 0.75);
  QVERIFY(spy.count() >= 1);

  // dragged far outside, the handle stops at the limits
  const QPoint grab2 = editor.handlePosition(1).toPoint();
  QTest::mousePress(&editor, Qt::LeftButton, Qt::NoModifier, grab2);
  QTest::mouseRelease(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(5000, -5000));
  QCOMPARE(editor.curve().x2, 1.0);
  QCOMPARE(editor.curve().y2, Easing::BezierMaxY);

  // a click away from both handles moves nothing
  const Easing::Bezier still = editor.curve();
  QTest::mouseClick(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(editor.width() / 2, editor.height() - 2));
  QCOMPARE(editor.curve(), still);
}

void EasingTilesTest::dialogStartAgainAndLimits()
{
  const Easing::Bezier start = Easing::bezierFromPreset(Easing::Preset::SlowFinish);
  CurveEditorDialog dialog(start);
  QCOMPARE(dialog.windowTitle(), QStringLiteral("Edit curve"));
  QCOMPARE(dialog.curve(), start);
  QLabel* hint = dialog.findChild<QLabel*>("curveEditorHint");
  QVERIFY(hint);
  QCOMPARE(hint->text(), QStringLiteral("Drag the two round handles. Steep parts move fast, flat parts slowly."));

  dialog.editor()->setCurve(Easing::Bezier{0.1, 0.9, 0.2, 0.3});
  QVERIFY(dialog.curve() != start);
  QPushButton* again = dialog.findChild<QPushButton*>("curveEditorStartAgain");
  QVERIFY(again);
  QCOMPARE(again->text(), QStringLiteral("Start again"));
  again->click();
  QCOMPARE(dialog.curve(), start);

  // values outside the limits are kept inside them
  dialog.editor()->setCurve(Easing::Bezier{-1, 9, 2, -9});
  QCOMPARE(dialog.curve(), (Easing::Bezier{0, Easing::BezierMaxY, 1, Easing::BezierMinY}));
}

// ---------------------------------------------------------------- UX phase 5

void EasingTilesTest::headingAndExpertOnlyEditButton()
{
  UserSettings::setExpertMode(false);
  {
    EasingTiles row;
    QVERIFY(row.heading());
    QCOMPARE(row.heading()->text(), QStringLiteral("How the move feels"));
    QCOMPARE(row.heading()->objectName(), QStringLiteral("easingHeading"));
    QVERIFY(row.heading()->font().weight() >= QFont::DemiBold);
    // Beginner mode: no Edit curve button
    QVERIFY(!row.isExpertMode());
    QVERIFY(!row.editCurveButton()->isVisibleTo(&row));
    // the Expert mode switch shows it at once, and hides it again
    UserSettings::setExpertMode(true);
    QVERIFY(row.isExpertMode());
    QVERIFY(row.editCurveButton()->isVisibleTo(&row));
    UserSettings::setExpertMode(false);
    QVERIFY(!row.editCurveButton()->isVisibleTo(&row));
    UserSettings::setExpertMode(true);
  }
  // a row made in Expert mode shows the button from the start; a deleted row
  // is no longer called when the switch changes (no crash here)
  EasingTiles expertRow;
  QVERIFY(expertRow.editCurveButton()->isVisibleTo(&expertRow));
  UserSettings::setExpertMode(false);
  UserSettings::setExpertMode(true);
  QVERIFY(expertRow.editCurveButton()->isVisibleTo(&expertRow));
  // the button sits right of the tiles, with the mockup's icon
  QVERIFY(!expertRow.editCurveButton()->icon().isNull());
  UserSettings::setExpertMode(false);
}

void EasingTilesTest::greyedRowSaysWhy()
{
  EasingTiles row;
  row.setTipsOn(false);
  EasingTiles::NoKey state = EasingTiles::NoKey::NoAnimation;
  row.setNoKeyCheck([&state]() { return state; });
  QPushButton* edit = row.editCurveButton();
  EasingTile* tile = row.tile(Easing::Preset::Even);

  row.setNoKey();
  QCOMPARE(tile->statusTip(), QStringLiteral("Open an animation first."));
  QCOMPARE(tile->toolTip(), QStringLiteral("Open an animation first."));
  QCOMPARE(tile->property("reasonName").toString(), QStringLiteral("Easing choice"));
  QCOMPARE(edit->statusTip(), QStringLiteral("Open an animation first."));
  QCOMPARE(edit->property("reasonName").toString(), QStringLiteral("Edit curve"));
  // nothing open: the line under the row is empty (the timeline says it)
  QCOMPARE(row.hintText(), QString());

  state = EasingTiles::NoKey::NoJoint;
  row.refreshNoKey();
  QCOMPARE(tile->statusTip(), QStringLiteral("Pick a joint first."));
  QCOMPARE(edit->statusTip(), QStringLiteral("Select a key on the timeline first."));
  QCOMPARE(row.hintText(), QStringLiteral("Select a key on the timeline"));

  state = EasingTiles::NoKey::NoMove;
  row.refreshNoKey();
  QCOMPARE(tile->statusTip(), QStringLiteral("Select a key on the timeline. The first key has no move before it."));
  QCOMPARE(row.tileReason(), tile->statusTip());
  QCOMPARE(edit->statusTip(), QStringLiteral("Select a key on the timeline first."));
  QCOMPARE(row.editCurveReason(), edit->statusTip());
  QCOMPARE(edit->toolTip(), QStringLiteral("Select a key on the timeline first."));

  // a pose change is picked: no reasons, and refreshNoKey leaves it alone
  row.setChoice(Easing::Preset::Even);
  QVERIFY(tile->statusTip().isEmpty());
  QVERIFY(edit->statusTip().isEmpty());
  QVERIFY(row.tileReason().isEmpty());
  state = EasingTiles::NoKey::NoAnimation;
  row.refreshNoKey();
  QVERIFY(row.hasChoice());
  QVERIFY(tile->isEnabled());
}

void EasingTilesTest::customLineShowsWithTipsOff()
{
  EasingTiles row;
  row.setTipsOn(false);
  row.setChoice(Easing::Preset::SlowStart);
  QVERIFY(row.hintText().isEmpty());
  // the custom curve is the state of the pose change, not a tip
  row.setCustom(true);
  QCOMPARE(row.hintText(), QStringLiteral("This pose change uses your own curve. Click a tile to go back."));
}

void EasingTilesTest::tabReachesTheCurrentTileOnly()
{
  EasingTiles row;
  row.setChoice(Easing::Preset::SlowFinish);
  for (int i = 0; i < Easing::PresetCount; i++)
  {
    const Easing::Preset preset = static_cast<Easing::Preset>(i);
    QCOMPARE(row.tile(preset)->focusPolicy(), preset == Easing::Preset::SlowFinish ? Qt::StrongFocus : Qt::ClickFocus);
  }
  // a click moves the Tab stop with the choice
  row.tile(Easing::Preset::HoldThenSnap)->click();
  QCOMPARE(row.tile(Easing::Preset::HoldThenSnap)->focusPolicy(), Qt::StrongFocus);
  QCOMPARE(row.tile(Easing::Preset::SlowFinish)->focusPolicy(), Qt::ClickFocus);
  // a custom curve marks no tile: the first tile is the Tab stop
  row.setCustom(true);
  QCOMPARE(row.tile(Easing::Preset::Even)->focusPolicy(), Qt::StrongFocus);
  QCOMPARE(row.tile(Easing::Preset::HoldThenSnap)->focusPolicy(), Qt::ClickFocus);
}

void EasingTilesTest::arrowKeysPickTheNextChoice()
{
  EasingTiles row;
  row.setChoice(Easing::Preset::SlowFinish);
  QVERIFY(showAwayFromMouse(row));
  row.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&row));
  QSignalSpy spy(&row, &EasingTiles::choiceClicked);

  EasingTile* start = row.tile(Easing::Preset::SlowFinish);
  start->setFocus(Qt::TabFocusReason);
  QVERIFY(start->hasFocus());

  struct Step { int key; Easing::Preset expected; };
  const Step steps[] = {
    {Qt::Key_Right, Easing::Preset::SlowBothEnds},
    {Qt::Key_Down, Easing::Preset::OvershootThenSettle},
    {Qt::Key_Right, Easing::Preset::HoldThenSnap},
    {Qt::Key_Right, Easing::Preset::Even},          // round the end
    {Qt::Key_Left, Easing::Preset::HoldThenSnap},   // and back
    {Qt::Key_Up, Easing::Preset::OvershootThenSettle},
  };
  int count = 0;
  for (const Step& step : steps)
  {
    QWidget* focused = QApplication::focusWidget();
    QVERIFY(focused);
    QTest::keyClick(focused, static_cast<Qt::Key>(step.key));
    count++;
    QCOMPARE(spy.count(), count);
    QCOMPARE(spy.last().at(0).value<Easing::Preset>(), step.expected);
    QVERIFY(row.tile(step.expected)->isChecked());
    QVERIFY(row.tile(step.expected)->hasFocus());
    QCOMPARE(row.choice(), step.expected);
  }

  // Space on the focused tile picks it again (a click)
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_Space);
  QCOMPARE(spy.count(), count + 1);

  // no key selected: the arrow keys change nothing
  row.setNoKey();
  QTest::keyClick(row.tile(Easing::Preset::OvershootThenSettle), Qt::Key_Right);
  QCOMPARE(spy.count(), count + 1);
}

void EasingTilesTest::curveEditorKeyboard()
{
  CurveEditor editor;
  editor.resize(360, 300);
  QCOMPARE(editor.focusPolicy(), Qt::StrongFocus);
  editor.setCurve(Easing::Bezier{0.25, 0.25, 0.75, 0.75});
  QCOMPARE(editor.activeHandle(), 0);

  QTest::keyClick(&editor, Qt::Key_Right);
  QTest::keyClick(&editor, Qt::Key_Up);
  QTest::keyClick(&editor, Qt::Key_Up);
  Easing::Bezier c = editor.curve();
  QVERIFY(std::fabs(c.x1 - 0.26) < 1e-9);
  QVERIFY(std::fabs(c.y1 - 0.27) < 1e-9);
  QCOMPARE(c.x2, 0.75);
  QCOMPARE(c.y2, 0.75);

  // Shift: steps of 0.1; Space: the other handle
  QTest::keyClick(&editor, Qt::Key_Space);
  QCOMPARE(editor.activeHandle(), 1);
  QTest::keyClick(&editor, Qt::Key_Left, Qt::ShiftModifier);
  QTest::keyClick(&editor, Qt::Key_Down, Qt::ShiftModifier);
  c = editor.curve();
  QVERIFY(std::fabs(c.x2 - 0.65) < 1e-9);
  QVERIFY(std::fabs(c.y2 - 0.65) < 1e-9);
  QVERIFY(std::fabs(c.x1 - 0.26) < 1e-9);

  // the keys stop at the limits like a drag
  for (int i = 0; i < 20; i++) QTest::keyClick(&editor, Qt::Key_Right, Qt::ShiftModifier);
  QCOMPARE(editor.curve().x2, 1.0);

  // a drag makes the dragged handle the one the keys move
  QTest::mousePress(&editor, Qt::LeftButton, Qt::NoModifier, editor.handlePosition(0).toPoint());
  QTest::mouseRelease(&editor, Qt::LeftButton, Qt::NoModifier, editor.handlePosition(0).toPoint());
  QCOMPARE(editor.activeHandle(), 0);

  // the window tells how
  CurveEditorDialog dialog(Easing::bezierFromPreset(Easing::Preset::Even));
  QLabel* keys = dialog.findChild<QLabel*>("curveEditorKeys");
  QVERIFY(keys);
  QCOMPARE(keys->text(), QStringLiteral("Keys: the arrow keys move a handle (Shift: bigger steps), Space picks the other handle."));
}

// the Edit curve window opens where it was last time, with the size it had
void EasingTilesTest::editCurveWindowKeepsItsPlace()
{
  DialogFit::setAreaForTests(QRect(0, 0, 1600, 1000));
  UserSettings::setWindowPlace(CurveEditorDialog::placeName(), QRect());
  QCOMPARE(CurveEditorDialog::placeName(), QStringLiteral("edit_curve_window"));
  const Easing::Bezier start = Easing::bezierFromPreset(Easing::Preset::SlowBothEnds);

  QSize firstSize;
  {
    CurveEditorDialog dialog(start);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QApplication::processEvents();
    // the first time: its own small size, inside the screen
    firstSize = dialog.size();
    QVERIFY(firstSize.width() <= 1600 && firstSize.height() <= 1000);
    // the user moves it (to a second screen, say) and makes it bigger
    dialog.move(300, 200);
    dialog.resize(firstSize + QSize(140, 120));
    QApplication::processEvents();
    dialog.reject();
  }
  const QRect kept = UserSettings::windowPlace(CurveEditorDialog::placeName());
  QCOMPARE(kept, QRect(QPoint(300, 200), firstSize + QSize(140, 120)));

  {
    CurveEditorDialog dialog(start);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QApplication::processEvents();
    QCOMPARE(dialog.pos(), QPoint(300, 200));
    QCOMPARE(dialog.size(), firstSize + QSize(140, 120));
    // OK keeps the place too
    dialog.move(320, 240);
    QApplication::processEvents();
    dialog.accept();
  }
  QCOMPARE(UserSettings::windowPlace(CurveEditorDialog::placeName()).topLeft(), QPoint(320, 240));
  DialogFit::setAreaForTests(QRect());
}

// a kept place that no longer fits (a smaller or missing screen) is pulled
// onto the screen and made no larger than it
void EasingTilesTest::editCurveWindowStaysOnTheScreen()
{
  const QRect area(0, 0, 1024, 700);
  DialogFit::setAreaForTests(area);
  const Easing::Bezier start = Easing::bezierFromPreset(Easing::Preset::Even);

  UserSettings::setWindowPlace(CurveEditorDialog::placeName(), QRect(5000, 4000, 400, 360));
  {
    CurveEditorDialog dialog(start);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QApplication::processEvents();
    QVERIFY2(area.contains(dialog.frameGeometry()), qPrintable(QString("%1,%2 %3x%4").arg(dialog.x()).arg(dialog.y()).arg(dialog.width()).arg(dialog.height())));
    QCOMPARE(dialog.size(), QSize(400, 360));
    dialog.reject();
  }

  UserSettings::setWindowPlace(CurveEditorDialog::placeName(), QRect(-300, -200, 4000, 3000));
  {
    CurveEditorDialog dialog(start);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QApplication::processEvents();
    QVERIFY(dialog.width() <= area.width() && dialog.height() <= area.height());
    QVERIFY(area.contains(dialog.frameGeometry()));
    // the title bar is inside the screen
    QVERIFY(dialog.frameGeometry().top() >= area.top());
    dialog.reject();
  }
  DialogFit::setAreaForTests(QRect());
  UserSettings::setWindowPlace(CurveEditorDialog::placeName(), QRect());
}

// at 200 percent text the window grows with its content but keeps the size
// the user gave it when that is bigger, and its buttons stay reachable
void EasingTilesTest::editCurveWindowAtDoubleText()
{
  const QRect area(0, 0, 1280, 800);
  DialogFit::setAreaForTests(area);
  UserSettings::setWindowPlace(CurveEditorDialog::placeName(), QRect(100, 80, 900, 700));
  CurveEditorDialog dialog(Easing::bezierFromPreset(Easing::Preset::Even));
  dialog.show();
  QVERIFY(QTest::qWaitForWindowExposed(&dialog));
  QApplication::processEvents();
  QCOMPARE(dialog.size(), QSize(900, 700));
  TextSize::apply(200);
  QTest::qWait(50);
  QApplication::processEvents();
  QVERIFY(dialog.width() >= 900 && dialog.height() >= 700);
  QVERIFY(dialog.width() <= area.width() && dialog.height() <= area.height());
  QVERIFY(area.contains(dialog.frameGeometry()));
  QDialogButtonBox* buttons = dialog.findChild<QDialogButtonBox*>();
  QVERIFY(buttons);
  QVERIFY(buttons->isVisible());
  const QRect inDialog(buttons->mapTo(&dialog, QPoint(0, 0)), buttons->size());
  QVERIFY(dialog.rect().contains(inDialog));
  TextSize::apply(100);
  dialog.reject();
  DialogFit::setAreaForTests(QRect());
  UserSettings::setWindowPlace(CurveEditorDialog::placeName(), QRect());
}

// the row and the window in both themes at 100, 150 and 200 percent text:
// nothing clipped, the current tile in the selection colour with its check
// mark, the heading and Edit curve there (pictures with PINGLYMOTION_UI_OUT)
void EasingTilesTest::pictures()
{
  UserSettings::setExpertMode(true);
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    EasingTiles row;
    row.setTipsOn(true);
    row.setChoice(Easing::Preset::SlowBothEnds);
    const QImage image = UiRender::render(&row);
    UiRender::save(image, QStringLiteral("easingtiles"), v);
    const QStringList clipped = UiRender::clipping(&row);
    QVERIFY2(clipped.isEmpty(), qPrintable(v.name() + ": " + clipped.join("; ")));
    QVERIFY(row.heading()->isVisible());
    // every name fits in two lines inside its tile (charter P9, 200 percent text)
    for (int i = 0; i < Easing::PresetCount; i++)
    {
      EasingTile* tile = row.tile(static_cast<Easing::Preset>(i));
      QFont small = tile->font();
      if (small.pointSize() > 0) small.setPointSize(small.pointSize() - 1);
      const QFontMetrics metrics(small);
      const QRect needed = metrics.boundingRect(QRect(0, 0, tile->width() - 8, 10000),
                                                Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, tile->text());
      QVERIFY2(needed.height() <= 2 * metrics.height(), qPrintable(v.name() + ": " + tile->text()));
    }
    QVERIFY(row.editCurveButton()->isVisible());
    // the check badge of the current tile (14 px, 3 px from the top right
    // corner) is in the selection colour, at a point beside the check stroke
    EasingTile* current = row.tile(Easing::Preset::SlowBothEnds);
    const QPoint inBadge(current->width() - 10, 6);
    const QString why = UiRender::expectColour(image, &row, current, inBadge,
                                               current->palette().color(QPalette::Highlight), QStringLiteral("check badge"), 24);
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + ": " + why));
    // a tile that is not current has no badge there
    EasingTile* other = row.tile(Easing::Preset::Even);
    QVERIFY(UiRender::colourAt(image, &row, other, inBadge) != other->palette().color(QPalette::Highlight));

    CurveEditorDialog dialog(Easing::bezierFromPreset(Easing::Preset::OvershootThenSettle));
    const QImage window = UiRender::render(&dialog);
    UiRender::save(window, QStringLiteral("editcurve"), v);
    // a word-wrapped label may be narrower than its one-line hint: it is
    // clipped only when it is lower than its text needs at its width
    QStringList clippedWindow;
    for (const QString& problem : UiRender::clipping(&dialog))
    {
      bool wrappedFits = false;
      for (QLabel* label : dialog.findChildren<QLabel*>())
        if (label->wordWrap() && problem.startsWith(QStringLiteral("QLabel '%1' is ").arg(label->objectName()))
            && problem.contains(QStringLiteral("smaller than its size hint"))
            && label->height() >= label->heightForWidth(label->width()))
          wrappedFits = true;
      if (!wrappedFits) clippedWindow << problem;
    }
    QVERIFY2(clippedWindow.isEmpty(), qPrintable(v.name() + ": " + clippedWindow.join("; ")));
  }
  UserSettings::setExpertMode(false);
}

// roadmap item 5, "Easing per joint": a choice changes only the selected
// joint's pose change; another joint with keys on the same frames keeps its own
void EasingTilesTest::choiceIsPerJoint()
{
  BVH bvh;
  QScopedPointer<Animation> anim;
  BVHNode* joint = nullptr;
  int from = 0, to = 0;
  QVERIFY(openJump(bvh, anim, joint, from, to));
  // a second joint with a pose change ending on the same key frame
  BVHNode* other = nullptr;
  int otherFrom = 0;
  for (const char* name : {"rShldr", "lForeArm", "rForeArm", "hip", "abdomen", "chest", "neck", "head"})
  {
    BVHNode* candidate = findJoint(anim->getMotion(), name);
    if (candidate && candidate != joint && EasingChoice::segmentEndingAt(candidate, to, otherFrom))
    {
      other = candidate;
      break;
    }
  }
  QVERIFY2(other, "jump.dingle needs a second joint with a key on the same frame");

  EasingChoice::setChoice(anim.data(), other, otherFrom, to, Easing::Preset::Even);
  EasingChoice::setChoice(anim.data(), joint, from, to, Easing::Preset::HoldThenSnap);
  QCOMPARE(EasingChoice::choiceOf(joint, from, to), Easing::Preset::HoldThenSnap);
  QCOMPARE(EasingChoice::choiceOf(other, otherFrom, to), Easing::Preset::Even);
  EasingChoice::setChoice(anim.data(), other, otherFrom, to, Easing::Preset::SlowStart);
  QCOMPARE(EasingChoice::choiceOf(joint, from, to), Easing::Preset::HoldThenSnap);
  QCOMPARE(EasingChoice::choiceOf(other, otherFrom, to), Easing::Preset::SlowStart);
  // the custom curve is per joint as well
  const Easing::Bezier mine{0.2, 0.1, 0.3, 1.2};
  EasingChoice::setCustomCurve(anim.data(), joint, from, to, mine);
  QVERIFY(EasingChoice::customCurveOf(joint, from).has_value());
  QVERIFY(!EasingChoice::customCurveOf(other, otherFrom).has_value());
  QCOMPARE(EasingChoice::choiceOf(other, otherFrom, to), Easing::Preset::SlowStart);
}

QTEST_MAIN(EasingTilesTest)
#include "tst_easingtiles.moc"
