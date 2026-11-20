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
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>
#include <cmath>

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
    void customButtonFollowsTheKey();
    void customStateMarksButtonNotTiles();
    void customCheckRunsOnSetChoice();
    void customCurveStoredPlayedAndSaved();
    void customCurveStaysInsideLimits();
    void editorDragsHandles();
    void dialogStartAgainAndLimits();

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

// ---------------------------------------------------------------- custom curve (issue #11)

void EasingTilesTest::customButtonFollowsTheKey()
{
  EasingTiles row;
  QPushButton* button = row.customButton();
  QVERIFY(button);
  QCOMPARE(button->objectName(), QStringLiteral("customCurveButton"));
  QCOMPARE(button->text(), QStringLiteral("Custom curve..."));
  QVERIFY(!button->isEnabled());

  QSignalSpy spy(&row, &EasingTiles::customCurveRequested);
  row.setChoice(Easing::Preset::SlowBothEnds);
  QVERIFY(button->isEnabled());
  button->click();
  QCOMPARE(spy.count(), 1);
  // only the dialog changes the state, not the click
  QVERIFY(!button->isChecked());
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
  QVERIFY(row.customButton()->isChecked());
  for (int i = 0; i < Easing::PresetCount; i++)
    QVERIFY(!row.tile(static_cast<Easing::Preset>(i))->isChecked());
  QCOMPARE(row.hintText(), QStringLiteral("This pose change uses your own curve. Click a tile to go back."));

  // a tile click leaves the custom state and is reported like any click
  QSignalSpy spy(&row, &EasingTiles::choiceClicked);
  row.tile(Easing::Preset::Even)->click();
  QCOMPARE(spy.count(), 1);
  QVERIFY(!row.isCustom());
  QVERIFY(!row.customButton()->isChecked());
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
  QCOMPARE(dialog.windowTitle(), QStringLiteral("Custom curve"));
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

QTEST_MAIN(EasingTilesTest)
#include "tst_easingtiles.moc"
