/*
 * PinglyMotion - test of editing a picked prop as in Second Life (Mockup 2)
 *
 * Exact answers only:
 * - the mode: Ctrl gives Turn, Ctrl+Shift gives Size, otherwise the chosen one;
 * - the handles on the screen (src/propedit.*): where the arrows, rings and
 *   squares are, which one is under the mouse;
 * - every drag: the X, Y and Z arrows change only their own place by the
 *   distance the mouse went along them, a ring changes only its turn, a
 *   square changes only its size and never below 0.02 m; Esc gives the exact
 *   state from before;
 * - the keys without a mouse (charter P8);
 * - Undo and Redo of prop steps (UndoController::recordStep), also in turn
 *   with animation steps;
 * - the Selected prop page (src/selectedproppanel.*): the fields show the
 *   prop in metres and degrees, follow it, and a change in a field gives the
 *   exact state before and after;
 * - the Edit prop card (src/propeditcard.*): title, the three choices,
 *   arrow keys, the caption;
 * - colours from the tokens, dark and light, at 100 and 200 percent (the UI
 *   render helper), nothing clipped;
 * - the texts are Mockup 2's words (PROPED-xx in docs/TEACHING.md);
 * - the prop file keeps every value exactly (PropFile::write and parse).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QAction>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>

#include <cmath>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"
#include "propedit.h"
#include "propeditcard.h"
#include "propfile.h"
#include "selectedproppanel.h"
#include "textsize.h"
#include "theme.h"
#include "timelineclip.h"
#include "uirender.h"
#include "undohistory.h"
#include "viewcolours.h"

using PropEdit::Mode;
using PropEdit::State;

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }
  QString source(const char* name) { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/") + QLatin1String(name)); }

  // the text of a register row in docs/TEACHING.md ("Box<br>Ring" gives two)
  QStringList registerText(const QString& id)
  {
    const QString text = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/TEACHING.md"));
    for (const QString& line : text.split(QLatin1Char('\n')))
    {
      const QStringList cells = line.split(QLatin1Char('|'));
      if (cells.size() > 5 && cells.at(1).trimmed() == id)
        return cells.at(4).trimmed().split(QStringLiteral("<br>"));
    }
    return {};
  }

  // a view of 200 x 200 pixels looking at the prop from the front: 10 pixels per inch,
  // X to the right, Y up, Z toward the viewer
  QMatrix4x4 frontView()
  {
    QMatrix4x4 m;
    m.ortho(-10, 10, -10, 10, -100, 100);
    return m;
  }
  // the same from the side: the scene turned 90 degrees around Y, so Z goes to the right
  QMatrix4x4 sideView()
  {
    QMatrix4x4 m = frontView();
    m.rotate(90, 0, 1, 0);
    return m;
  }
  const QSize viewSize(200, 200);

  State unitProp()
  {
    State s;
    s.position = QVector3D(0, 0, 0);
    s.rotation = QVector3D(0, 0, 0);
    s.scale = QVector3D(1, 1, 1);
    return s;
  }

  bool near(double a, double b) { return std::fabs(a - b) < 1e-3; }

  int handleFor(const PropEdit::Gizmo& g, int axis, int sign = 1)
  {
    for (int i = 0; i < g.handles().size(); i++)
      if (g.handles().at(i).axis == axis && g.handles().at(i).sign == sign) return i;
    return -1;
  }

  // a drag from `from` to `to` on the handle there; the state after it
  State drag(const PropEdit::Gizmo& g, const QPointF& from, const QPointF& to, const State& start, int* handle = nullptr)
  {
    const int index = g.hit(from);
    if (handle) *handle = index;
    PropEdit::Drag d;
    if (!d.start(g, index, from, start)) return start;
    return d.moveTo(to);
  }

  // a new animation for the Undo tests (as tests/undo)
  struct Fixture
  {
    BVH bvh;
    Animation anim;
    QAction undoAction{QStringLiteral("Undo")};
    QAction redoAction{QStringLiteral("Redo")};
    UndoController undo{&undoAction, &redoAction};
    Fixture() : anim(&bvh)
    {
      Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
      anim.setNumberOfFrames(40);
      anim.setLoopOutPoint(39);
      anim.setLoopInPoint(0);
      anim.setDirty(false);
      undo.setBusyCheck([]() { return false; });
      undo.setAnimation(&anim);
      QCoreApplication::processEvents();
    }
    BVHNode* node(const char* name) { return anim.getNode(TimelineClip::trackOf(&anim, QLatin1String(name))); }
    void settle() { QCoreApplication::processEvents(); }
  };
}

class TestPropEdit : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();

    void keysChooseTheMode();
    void unitsAndLimits();
    void moveHandlesAreArrowsAlongTheAxes();
    void dragXArrowChangesOnlyX();
    void dragYArrowChangesOnlyY();
    void dragZArrowChangesOnlyZ();
    void ringsTurnAroundTheirAxes();
    void ringDragChangesOnlyItsTurn();
    void sizeSquaresSitOnTheSides();
    void sizeDragChangesOnlyItsSize();
    void sizeNeverBelowTheMinimum();
    void escGivesTheExactOldState();
    void hitFindsNothingAwayFromTheHandles();
    void keysWithoutAMouse();

    void undoAndRedoOfPropSteps();
    void propAndAnimationStepsInTurn();
    void newPropStepClearsRedo();

    void panelShowsThePropInMetres();
    void panelFollowsTheDrags();
    void panelFieldEditGivesExactStates();
    void panelShapeChoice();
    void panelEmptyCard();

    void cardTitleAndChoices();
    void cardArrowKeysChoose();

    void handleColoursComeFromTokens();
    void cardAndPanelTokenColours();
    void cardAndPanelNotClipped();

    void textsAreTheMockupWords();
    void propFileRoundTrip();
    void viewDrawsTrianglesOnly();
    void windowMakesUndoSteps();
};

void TestPropEdit::initTestCase()
{
  Theme::apply(true);
}

void TestPropEdit::keysChooseTheMode()
{
  for (Mode chosen : {Mode::Move, Mode::Turn, Mode::Size})
  {
    QCOMPARE(PropEdit::effectiveMode(chosen, Qt::NoModifier), chosen);
    QCOMPARE(PropEdit::effectiveMode(chosen, Qt::ShiftModifier), chosen);
    QCOMPARE(PropEdit::effectiveMode(chosen, Qt::ControlModifier), Mode::Turn);
    QCOMPARE(PropEdit::effectiveMode(chosen, Qt::ControlModifier | Qt::ShiftModifier), Mode::Size);
  }
  // the handles follow: Ctrl shows the rings, Ctrl+Shift the squares
  PropEdit::Gizmo g;
  g.build(PropEdit::effectiveMode(Mode::Move, Qt::ControlModifier), unitProp(), frontView(), viewSize);
  QCOMPARE(g.mode(), Mode::Turn);
  QCOMPARE(g.handles().size(), 3);
  g.build(PropEdit::effectiveMode(Mode::Move, Qt::ControlModifier | Qt::ShiftModifier), unitProp(), frontView(), viewSize);
  QCOMPARE(g.mode(), Mode::Size);
  QCOMPARE(g.handles().size(), 6);
  g.build(Mode::Move, unitProp(), frontView(), viewSize);
  QCOMPARE(g.handles().size(), 3);
}

void TestPropEdit::unitsAndLimits()
{
  QCOMPARE(PropEdit::Units::metres(10.0), 0.2540005);
  QVERIFY(qFuzzyCompare(PropEdit::Units::inches(PropEdit::Units::metres(7.25)), 7.25));
  QVERIFY(qFuzzyCompare(PropEdit::Units::metres(PropEdit::Units::minSize()), 0.02));
  QCOMPARE(PropEdit::wrapDegrees(370.0), 10.0);
  QCOMPARE(PropEdit::wrapDegrees(-30.0), 330.0);
  QCOMPARE(PropEdit::wrapDegrees(360.0), 0.0);
  QCOMPARE(PropEdit::wrapDegrees(0.0), 0.0);
}

void TestPropEdit::moveHandlesAreArrowsAlongTheAxes()
{
  PropEdit::Gizmo g;
  g.build(Mode::Move, unitProp(), frontView(), viewSize);
  QVERIFY(g.valid());
  QCOMPARE(g.centre(), QPointF(100, 100));
  // the arrow is 0.2 m long (the mockup), its head 10 pixels more
  const double arrow = PropEdit::Units::inches(0.2) * 10.0;
  const PropEdit::Handle& x = g.handles().at(handleFor(g, 0));
  QCOMPARE(x.line.first(), QPointF(100, 100));
  QVERIFY(near(x.at.x(), 100 + arrow + 10));
  QVERIFY(near(x.at.y(), 100.0));
  QVERIFY(near(x.label.x(), 100 + arrow + 22));
  const PropEdit::Handle& y = g.handles().at(handleFor(g, 1));
  QVERIFY(near(y.at.y(), 100 - arrow - 10));
  QVERIFY(near(y.at.x(), 100.0));
  // a bigger prop gets longer arrows, so they start outside it
  State big = unitProp();
  big.scale = QVector3D(20, 2, 2);
  g.build(Mode::Move, big, frontView(), viewSize);
  QVERIFY(g.handles().at(handleFor(g, 0)).at.x() > 100 + 10 * 10);
}

void TestPropEdit::dragXArrowChangesOnlyX()
{
  PropEdit::Gizmo g;
  State start = unitProp();
  start.position = QVector3D(0, 0, 0);
  g.build(Mode::Move, start, frontView(), viewSize);
  int handle = -1;
  // 25 pixels to the right (and 7 down, which is across the arrow): 2.5 inches along X
  const State after = drag(g, QPointF(140, 100), QPointF(165, 107), start, &handle);
  QCOMPARE(g.handles().at(handle).axis, 0);
  QCOMPARE(g.handles().at(handle).mode, Mode::Move);
  QCOMPARE(after.position, QVector3D(2.5f, 0, 0));
  QCOMPARE(after.rotation, start.rotation);
  QCOMPARE(after.scale, start.scale);
  // and back to the left: minus
  QCOMPARE(drag(g, QPointF(140, 100), QPointF(120, 100), start).position, QVector3D(-2.0f, 0, 0));
}

void TestPropEdit::dragYArrowChangesOnlyY()
{
  PropEdit::Gizmo g;
  State start = unitProp();
  start.position = QVector3D(0, 0, 0);
  g.build(Mode::Move, start, frontView(), viewSize);
  int handle = -1;
  const State after = drag(g, QPointF(100, 60), QPointF(103, 20), start, &handle);
  QCOMPARE(g.handles().at(handle).axis, 1);
  QCOMPARE(after.position, QVector3D(0, 4.0f, 0));
  QCOMPARE(after.scale, start.scale);
}

void TestPropEdit::dragZArrowChangesOnlyZ()
{
  PropEdit::Gizmo g;
  const State start = unitProp();
  g.build(Mode::Move, start, sideView(), viewSize);
  int handle = -1;
  const State after = drag(g, QPointF(140, 100), QPointF(170, 100), start, &handle);
  QCOMPARE(g.handles().at(handle).axis, 2);
  QVERIFY(qFuzzyIsNull(after.position.x()));
  QVERIFY(qFuzzyIsNull(after.position.y()));
  QVERIFY(qFuzzyCompare(after.position.z(), 3.0f));
  QCOMPARE(after.rotation, start.rotation);
}

void TestPropEdit::ringsTurnAroundTheirAxes()
{
  // with no turn the rings are around X, Y and Z
  QCOMPARE(PropEdit::turnAxis(0, QVector3D(0, 0, 0)), QVector3D(1, 0, 0));
  QCOMPARE(PropEdit::turnAxis(1, QVector3D(0, 0, 0)), QVector3D(0, 1, 0));
  QCOMPARE(PropEdit::turnAxis(2, QVector3D(0, 0, 0)), QVector3D(0, 0, 1));
  // the Y ring follows the X turn, the Z ring the X and Y turns (the order Prop::draw() turns in)
  const QVector3D y = PropEdit::turnAxis(1, QVector3D(90, 0, 0));
  QVERIFY(qFuzzyIsNull(y.x()) && qFuzzyIsNull(y.y()) && qFuzzyCompare(y.z(), 1.0f));
  const QVector3D z = PropEdit::turnAxis(2, QVector3D(0, 90, 0));
  QVERIFY(qFuzzyCompare(z.x(), 1.0f) && qFuzzyIsNull(z.y()) && qFuzzyIsNull(z.z()));
  // the X turn does not move the X ring
  QCOMPARE(PropEdit::turnAxis(0, QVector3D(30, 40, 50)), QVector3D(1, 0, 0));

  PropEdit::Gizmo g;
  g.build(Mode::Turn, unitProp(), frontView(), viewSize);
  QCOMPARE(g.handles().size(), 3);
  // the Z ring faces the viewer: a circle of 0.17 m around the centre
  const PropEdit::Handle& ring = g.handles().at(handleFor(g, 2));
  const double radius = PropEdit::Units::inches(0.17) * 10.0;
  QCOMPARE(ring.line.size(), PropEdit::Look::ringSegments + 1);
  for (const QPointF& p : ring.line)
    QVERIFY(std::fabs(std::hypot(p.x() - 100, p.y() - 100) - radius) < 1e-3);
}

void TestPropEdit::ringDragChangesOnlyItsTurn()
{
  PropEdit::Gizmo g;
  State start = unitProp();
  start.rotation = QVector3D(10, 20, 30);
  // build at no turn for the arithmetic below, then edit a prop turned 0, 0, 30 around Z only
  start.rotation = QVector3D(0, 0, 30);
  g.build(Mode::Turn, start, frontView(), viewSize);
  const double radius = PropEdit::Units::inches(0.17) * 10.0;
  // the ring point up and to the right of the centre (45 degrees), where only the Z ring is
  const double c = std::cos(M_PI / 4), sn = std::sin(M_PI / 4);
  const QPointF grab(100 + radius * c, 100 - radius * sn);
  int handle = -1;
  // up and to the left: around the ring the positive way (seen from the viewer)
  const QPointF moved(-20, -20);
  const State after = drag(g, grab, grab + moved, start, &handle);
  QCOMPARE(g.handles().at(handle).axis, 2);
  QCOMPARE(g.handles().at(handle).mode, Mode::Turn);
  // one degree moves the point from 45 to 46 degrees around the centre
  const double one = M_PI / 180.0;
  const double tx = radius * (std::cos(M_PI / 4 + one) - c), ty = -radius * (std::sin(M_PI / 4 + one) - sn);
  const double degrees = (moved.x() * tx + moved.y() * ty) / (tx * tx + ty * ty);
  QVERIFY(degrees > 20);
  QVERIFY2(std::fabs(after.rotation.z() - (30 + degrees)) < 1e-3, qPrintable(QString::number(after.rotation.z())));
  QCOMPARE(after.rotation.x(), 0.0f);
  QCOMPARE(after.rotation.y(), 0.0f);
  QCOMPARE(after.position, start.position);
  QCOMPARE(after.scale, start.scale);
  // the other way, past zero: the turn stays between 0 and 360
  const State back = drag(g, grab, grab - moved * 3, start);
  QVERIFY(back.rotation.z() >= 0.0f && back.rotation.z() < 360.0f);
  QVERIFY(std::fabs(back.rotation.z() - PropEdit::wrapDegrees(30 - 3 * degrees)) < 1e-3);
}

void TestPropEdit::sizeSquaresSitOnTheSides()
{
  PropEdit::Gizmo g;
  g.build(Mode::Size, unitProp(), frontView(), viewSize);
  QCOMPARE(g.handles().size(), 6);
  // half the size plus 0.03 m from the centre, on both sides
  const double offset = (0.5 + PropEdit::Units::inches(0.03)) * 10.0;
  QVERIFY(near(g.handles().at(handleFor(g, 0, 1)).at.x(), 100 + offset));
  QVERIFY(near(g.handles().at(handleFor(g, 0, -1)).at.x(), 100 - offset));
  QVERIFY(near(g.handles().at(handleFor(g, 1, 1)).at.y(), 100 - offset));
  // they follow the prop's own turn: turned 90 degrees around Z, its X side is up
  State turned = unitProp();
  turned.rotation = QVector3D(0, 0, 90);
  g.build(Mode::Size, turned, frontView(), viewSize);
  const QPointF xSide = g.handles().at(handleFor(g, 0, 1)).at;
  QVERIFY(std::fabs(xSide.x() - 100) < 1e-3);
  QVERIFY(std::fabs(xSide.y() - (100 - offset)) < 1e-3);
}

void TestPropEdit::sizeDragChangesOnlyItsSize()
{
  PropEdit::Gizmo g;
  const State start = unitProp();
  g.build(Mode::Size, start, frontView(), viewSize);
  const double offset = (0.5 + PropEdit::Units::inches(0.03)) * 10.0;
  int handle = -1;
  // the right square 10 pixels out: one inch on that side, so both sides grow, 2 inches
  State after = drag(g, QPointF(100 + offset, 100), QPointF(110 + offset, 103), start, &handle);
  QCOMPARE(g.handles().at(handle).axis, 0);
  QCOMPARE(g.handles().at(handle).sign, 1);
  QCOMPARE(after.scale, QVector3D(3, 1, 1));
  QCOMPARE(after.position, start.position);
  QCOMPARE(after.rotation, start.rotation);
  // the left square 10 pixels out to the left: the same
  after = drag(g, QPointF(100 - offset, 100), QPointF(90 - offset, 100), start, &handle);
  QCOMPARE(g.handles().at(handle).sign, -1);
  QCOMPARE(after.scale, QVector3D(3, 1, 1));
  // the top square up: height
  after = drag(g, QPointF(100, 100 - offset), QPointF(100, 95 - offset), start, &handle);
  QCOMPARE(g.handles().at(handle).axis, 1);
  QCOMPARE(after.scale, QVector3D(1, 2, 1));
}

void TestPropEdit::sizeNeverBelowTheMinimum()
{
  PropEdit::Gizmo g;
  const State start = unitProp();
  g.build(Mode::Size, start, frontView(), viewSize);
  const double offset = (0.5 + PropEdit::Units::inches(0.03)) * 10.0;
  const State after = drag(g, QPointF(100 + offset, 100), QPointF(100 + offset - 150, 100), start);
  QCOMPARE(after.scale.x(), float(PropEdit::Units::minSize()));
  QCOMPARE(after.scale.y(), 1.0f);
}

void TestPropEdit::escGivesTheExactOldState()
{
  PropEdit::Gizmo g;
  State start = unitProp();
  start.position = QVector3D(1.25f, -3.5f, 0.125f);
  start.rotation = QVector3D(15, 25, 35);
  start.scale = QVector3D(7, 8, 9);
  g.build(Mode::Move, start, frontView(), viewSize);
  PropEdit::Drag d;
  const QPointF from = g.centre() + QPointF(40, 0);
  QVERIFY(d.start(g, g.hit(from), from, start));
  QVERIFY(d.active());
  QVERIFY(d.moveTo(from + QPointF(33, 0)) != start);
  QVERIFY(d.changed());
  const State back = d.cancel();
  QVERIFY(!d.active());
  QCOMPARE(back, start);
  QCOMPARE(d.now(), start);
  // a press without moving changes nothing
  QVERIFY(d.start(g, g.hit(from), from, start));
  QCOMPARE(d.moveTo(from), start);
  QVERIFY(!d.changed());
}

void TestPropEdit::hitFindsNothingAwayFromTheHandles()
{
  PropEdit::Gizmo g;
  g.build(Mode::Move, unitProp(), frontView(), viewSize);
  QCOMPARE(g.hit(QPointF(20, 180)), -1);
  // 7 pixels beside an arrow still grabs it (the mockup's 14 pixel wide line), 8 does not
  QCOMPARE(g.handles().at(g.hit(QPointF(150, 107))).axis, 0);
  QCOMPARE(g.hit(QPointF(150, 108.5)), -1);
  // the letter badge grabs its arrow too
  const QPointF label = g.handles().at(handleFor(g, 1)).label;
  QCOMPARE(g.handles().at(g.hit(label + QPointF(6, 0))).axis, 1);
  // no handles before the view knows the prop
  PropEdit::Gizmo empty;
  QCOMPARE(empty.hit(QPointF(100, 100)), -1);
  // a prop behind the camera has no handles
  QMatrix4x4 perspective;
  perspective.perspective(45, 1, 1, 100);
  State behind = unitProp();
  behind.position = QVector3D(0, 0, 10);
  g.build(Mode::Move, behind, perspective, viewSize);
  QVERIFY(!g.valid());
}

void TestPropEdit::keysWithoutAMouse()
{
  State s = unitProp();
  s.position = QVector3D(1, 2, 3);
  s.rotation = QVector3D(10, 20, 30);
  s.scale = QVector3D(4, 5, 6);
  Mode used = Mode::Move;
  int axis = -1;
  // Move: arrows along X and Y, comma and period along Z, 0.05 m a press (Shift: 0.01 m)
  auto step = [&](Mode chosen, int key, Qt::KeyboardModifiers keys) { return PropEdit::keyStep(chosen, key, keys, s, &used, &axis); };
  std::optional<State> out = step(Mode::Move, Qt::Key_Right, Qt::NoModifier);
  QVERIFY(out);
  QCOMPARE(used, Mode::Move);
  QCOMPARE(axis, 0);
  QVERIFY(qFuzzyCompare(out->position.x(), float(1 + PropEdit::Units::inches(0.05))));
  QCOMPARE(out->position.y(), 2.0f);
  QCOMPARE(out->scale, s.scale);
  out = step(Mode::Move, Qt::Key_Down, Qt::ShiftModifier);
  QCOMPARE(axis, 1);
  QVERIFY(qFuzzyCompare(out->position.y(), float(2 - PropEdit::Units::inches(0.01))));
  out = step(Mode::Move, Qt::Key_Period, Qt::NoModifier);
  QCOMPARE(axis, 2);
  QVERIFY(qFuzzyCompare(out->position.z(), float(3 + PropEdit::Units::inches(0.05))));
  out = step(Mode::Move, Qt::Key_Comma, Qt::NoModifier);
  QVERIFY(qFuzzyCompare(out->position.z(), float(3 - PropEdit::Units::inches(0.05))));
  // Ctrl: turn 5 degrees; Turn chosen with Shift: 1 degree
  out = step(Mode::Move, Qt::Key_Up, Qt::ControlModifier);
  QCOMPARE(used, Mode::Turn);
  QCOMPARE(out->rotation, QVector3D(10, 25, 30));
  QCOMPARE(out->position, s.position);
  out = step(Mode::Turn, Qt::Key_Left, Qt::ShiftModifier);
  QCOMPARE(out->rotation, QVector3D(9, 20, 30));
  // Ctrl+Shift: size, 0.01 m a press, never below the minimum
  out = step(Mode::Move, Qt::Key_Period, Qt::ControlModifier | Qt::ShiftModifier);
  QCOMPARE(used, Mode::Size);
  QVERIFY(qFuzzyCompare(out->scale.z(), float(6 + PropEdit::Units::inches(0.01))));
  QCOMPARE(out->scale.x(), 4.0f);
  State tiny = s;
  tiny.scale = QVector3D(float(PropEdit::Units::minSize()), 1, 1);
  out = PropEdit::keyStep(Mode::Size, Qt::Key_Left, Qt::NoModifier, tiny);
  QCOMPARE(out->scale.x(), float(PropEdit::Units::minSize()));
  // Alt + arrows turn the camera; other keys are not for the prop
  QVERIFY(!step(Mode::Move, Qt::Key_Left, Qt::AltModifier));
  QVERIFY(!step(Mode::Move, Qt::Key_A, Qt::NoModifier));
  QVERIFY(!step(Mode::Move, Qt::Key_PageUp, Qt::NoModifier));
}

void TestPropEdit::undoAndRedoOfPropSteps()
{
  QAction undoAction, redoAction;
  UndoController undo(&undoAction, &redoAction);
  undo.setBusyCheck([]() { return false; });
  State prop = unitProp();
  const State before = prop;
  State after = prop;
  after.position = QVector3D(2.5f, 0, 0);
  prop = after;
  undo.recordStep([&]() { prop = before; }, [&]() { prop = after; });
  QCOMPARE(undo.stepCount(), 1);
  // with no animation open, prop steps can still be undone
  QVERIFY(undo.undoBlocked().isEmpty());
  QVERIFY(undoAction.isEnabled());
  QSignalSpy messages(&undo, &UndoController::message);
  QVERIFY(undo.undo());
  QCOMPARE(prop, before);
  QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Undone."));
  QVERIFY(redoAction.isEnabled());
  QVERIFY(undo.redo());
  QCOMPARE(prop, after);
  QCOMPARE(messages.last().at(0).toString(), QStringLiteral("Redone."));
  QVERIFY(undo.undo());
  QCOMPARE(prop, before);
  QVERIFY(!undo.undo());
  // forgotten when the props are gone
  undo.recordStep([]() {}, []() {});
  undo.clearSteps();
  QCOMPARE(undo.stepCount(), 0);
  QCOMPARE(undo.redoStepCount(), 0);
}

void TestPropEdit::propAndAnimationStepsInTurn()
{
  Fixture f;
  BVHNode* hand = f.node("rHand");
  QVERIFY(hand);
  // 1. a joint turned, 2. a prop moved, 3. the joint turned again
  f.anim.setFrame(0);
  f.anim.setRotation(hand, 10, 0, 0);
  f.settle();
  int prop = 0;
  f.undo.recordStep([&]() { prop = 0; }, [&]() { prop = 1; });
  prop = 1;
  f.anim.setRotation(hand, 20, 0, 0);
  f.settle();
  // Undo goes back in that order: joint, prop, joint
  QVERIFY(f.undo.undo());
  QCOMPARE(f.anim.getRotation(hand).x(), 10.0);
  QCOMPARE(prop, 1);
  QVERIFY(f.undo.undo());
  QCOMPARE(prop, 0);
  QCOMPARE(f.anim.getRotation(hand).x(), 10.0);
  QVERIFY(f.undo.undo());
  QCOMPARE(f.anim.getRotation(hand).x(), 0.0);
  QCOMPARE(prop, 0);
  // Redo forward in the same order
  QVERIFY(f.undo.redo());
  QCOMPARE(f.anim.getRotation(hand).x(), 10.0);
  QCOMPARE(prop, 0);
  QVERIFY(f.undo.redo());
  QCOMPARE(prop, 1);
  QCOMPARE(f.anim.getRotation(hand).x(), 10.0);
  QVERIFY(f.undo.redo());
  QCOMPARE(f.anim.getRotation(hand).x(), 20.0);
  QVERIFY(!f.undo.redo());
}

void TestPropEdit::newPropStepClearsRedo()
{
  Fixture f;
  BVHNode* hand = f.node("rHand");
  f.anim.setFrame(0);
  f.anim.setRotation(hand, 10, 0, 0);
  f.settle();
  QVERIFY(f.undo.undo());
  QVERIFY(f.undo.redoBlocked().isEmpty());
  // a new edit of another kind: the undone joint turn cannot come back
  f.undo.recordStep([]() {}, []() {});
  QVERIFY(!f.undo.redoBlocked().isEmpty());
  QVERIFY(!f.undo.redo());
  // and a new joint edit clears an undone prop step
  QVERIFY(f.undo.undo());
  QCOMPARE(f.undo.redoStepCount(), 1);
  f.anim.setRotation(hand, 30, 0, 0);
  f.settle();
  QVERIFY(f.undo.undoBlocked().isEmpty());
  QVERIFY(!f.undo.redo());
  QCOMPARE(f.undo.redoStepCount(), 0);
}

void TestPropEdit::panelShowsThePropInMetres()
{
  SelectedPropPanel panel;
  State s;
  s.position = QVector3D(10, 40, -20);
  s.rotation = QVector3D(0, 90, 370);
  s.scale = QVector3D(10, 5, 2);
  panel.showProp(QStringLiteral("Box 0"), 0, s);
  QVERIFY(panel.showingProp());
  QCOMPARE(panel.propName(), QStringLiteral("Box 0"));
  QCOMPARE(panel.placeField(0)->value(), 0.25);   // 10 inches, two decimals
  QCOMPARE(panel.placeField(1)->value(), 1.02);
  QCOMPARE(panel.placeField(2)->value(), -0.51);
  QCOMPARE(panel.turnField(1)->value(), 90.0);
  QCOMPARE(panel.turnField(2)->value(), 10.0);
  QCOMPARE(panel.sizeField(0)->value(), 0.25);
  QCOMPARE(panel.sizeField(1)->value(), 0.13);
  QCOMPARE(panel.sizeField(2)->value(), 0.05);
  QCOMPARE(panel.placeField(0)->suffix(), QStringLiteral(" m"));
  QCOMPARE(panel.turnField(0)->suffix(), QStringLiteral("°"));
  QCOMPARE(panel.sizeField(0)->minimum(), 0.02);
  QCOMPARE(panel.placeField(0)->accessibleName(), QStringLiteral("Left or right"));
  QCOMPARE(panel.sizeField(2)->accessibleName(), QStringLiteral("Depth"));
  // the state keeps the exact inches, not the rounded metres
  QCOMPARE(panel.state(), s);
}

void TestPropEdit::panelFollowsTheDrags()
{
  SelectedPropPanel panel;
  const State start = unitProp();
  panel.showProp(QStringLiteral("Box 0"), 0, start);
  QSignalSpy edited(&panel, &SelectedPropPanel::stateEdited);
  // a drag of the X arrow by 25 pixels in the front view: 2.5 inches
  PropEdit::Gizmo g;
  g.build(Mode::Move, start, frontView(), viewSize);
  const State after = drag(g, QPointF(140, 100), QPointF(165, 100), start);
  panel.setState(after);
  QCOMPARE(panel.placeField(0)->value(), 0.06);   // 2.5 inches is 0.0635 m
  QCOMPARE(panel.state(), after);
  // following a drag is not an edit of the field
  QCOMPARE(edited.count(), 0);
}

void TestPropEdit::panelFieldEditGivesExactStates()
{
  SelectedPropPanel panel;
  State s = unitProp();
  s.position = QVector3D(10, 40, 10);
  s.rotation = QVector3D(5, 0, 0);
  s.scale = QVector3D(10, 10, 10);
  panel.showProp(QStringLiteral("Box 0"), 0, s);
  QSignalSpy edited(&panel, &SelectedPropPanel::stateEdited);
  // Up or down set to 2 m: only Y changes, to 2 m in inches
  panel.placeField(1)->setValue(2.0);
  QCOMPARE(edited.count(), 1);
  const State before = edited.last().at(0).value<State>();
  const State after = edited.last().at(1).value<State>();
  QCOMPARE(before, s);
  QCOMPARE(after.position.x(), 10.0f);
  QCOMPARE(after.position.z(), 10.0f);
  QCOMPARE(after.position.y(), float(PropEdit::Units::inches(2.0)));
  QCOMPARE(after.rotation, s.rotation);
  QCOMPARE(after.scale, s.scale);
  // a turn field
  panel.turnField(2)->setValue(45.0);
  QCOMPARE(edited.last().at(1).value<State>().rotation, QVector3D(5, 0, 45));
  // a size field cannot go under 0.02 m
  panel.sizeField(0)->setValue(0.0);
  QCOMPARE(panel.sizeField(0)->value(), 0.02);
  QCOMPARE(edited.last().at(1).value<State>().scale.x(), float(PropEdit::Units::minSize()));
}

void TestPropEdit::panelShapeChoice()
{
  SelectedPropPanel panel;
  panel.showProp(QStringLiteral("Ball 0"), 1, unitProp());
  // Box, Ring, Cone, Ball (the mockup's order) are the prop types 0, 3, 2, 1
  QCOMPARE(panel.shapeButton(0)->text(), QStringLiteral("Box"));
  QCOMPARE(panel.shapeButton(3)->text(), QStringLiteral("Ring"));
  QCOMPARE(panel.shapeButton(2)->text(), QStringLiteral("Cone"));
  QCOMPARE(panel.shapeButton(1)->text(), QStringLiteral("Ball"));
  QVERIFY(panel.shapeButton(1)->isChecked());
  QVERIFY(!panel.shapeButton(0)->isChecked());
  QSignalSpy chosen(&panel, &SelectedPropPanel::shapeChosen);
  panel.shapeButton(3)->click();
  QCOMPARE(chosen.count(), 1);
  QCOMPARE(chosen.last().at(0).toInt(), 3);
  QCOMPARE(panel.propType(), 3);
  QVERIFY(panel.shapeButton(3)->isChecked());
  QVERIFY(panel.shapeButton(3)->font().bold());
  // the same shape again is no change
  panel.shapeButton(3)->click();
  QCOMPARE(chosen.count(), 1);
}

void TestPropEdit::panelEmptyCard()
{
  SelectedPropPanel panel;
  QVERIFY(!panel.showingProp());
  QLabel* heading = panel.findChild<QLabel*>(QStringLiteral("selectedPropNothingHeading"));
  QLabel* line = panel.findChild<QLabel*>(QStringLiteral("selectedPropNothingLine"));
  QVERIFY(heading && line);
  QCOMPARE(heading->text(), QStringLiteral("Selected prop"));
  QCOMPARE(line->text(), QStringLiteral("Pick a prop in the 3D view or in the list."));
  panel.showProp(QStringLiteral("Box 0"), 0, unitProp());
  panel.showNothing();
  QVERIFY(!panel.showingProp());
}

void TestPropEdit::cardTitleAndChoices()
{
  PropEditCard card;
  card.setPropName(QStringLiteral("Box 0"));
  QCOMPARE(card.title(), QStringLiteral("Edit prop: Box 0"));
  QCOMPARE(card.caption(), QStringLiteral("Ctrl turns it, Ctrl+Shift changes its size. This works as in Second Life."));
  QCOMPARE(card.button(Mode::Move)->text(), QStringLiteral("Move"));
  QCOMPARE(card.button(Mode::Turn)->text(), QStringLiteral("Turn"));
  QCOMPARE(card.button(Mode::Size)->text(), QStringLiteral("Size"));
  QCOMPARE(card.button(Mode::Turn)->accessibleDescription(), QStringLiteral("Ctrl"));
  QCOMPARE(card.button(Mode::Size)->accessibleDescription(), QStringLiteral("Ctrl+Shift"));
  QVERIFY(card.button(Mode::Size)->toolTip().contains(QStringLiteral("Size (Ctrl+Shift)")));
  QVERIFY(card.button(Mode::Move)->toolTip().contains(QStringLiteral("Keys: arrows for X and Y, comma and period for Z.")));
  QVERIFY(card.button(Mode::Move)->isChecked());
  // holding Ctrl lights up Turn (the mode in use), without choosing it
  QSignalSpy chosen(&card, &PropEditCard::modeChosen);
  card.showMode(Mode::Turn);
  QVERIFY(card.button(Mode::Turn)->isChecked());
  QVERIFY(card.button(Mode::Turn)->font().bold());
  QVERIFY(!card.button(Mode::Move)->font().bold());
  QCOMPARE(chosen.count(), 0);
  // a click chooses
  card.button(Mode::Size)->click();
  QCOMPARE(chosen.count(), 1);
  QCOMPARE(chosen.last().at(0).value<Mode>(), Mode::Size);
  QCOMPARE(card.shownMode(), Mode::Size);
  // a click does not take the keys from the 3D view; Tab reaches the buttons
  QCOMPARE(card.button(Mode::Move)->focusPolicy(), Qt::TabFocus);
  QVERIFY(card.testAttribute(Qt::WA_NoMousePropagation));
}

void TestPropEdit::cardArrowKeysChoose()
{
  PropEditCard card;
  card.show();
  QVERIFY(QTest::qWaitForWindowExposed(&card));
  QSignalSpy chosen(&card, &PropEditCard::modeChosen);
  card.button(Mode::Move)->setFocus();
  QTest::keyClick(card.button(Mode::Move), Qt::Key_Right);
  QCOMPARE(chosen.last().at(0).value<Mode>(), Mode::Turn);
  QVERIFY(card.button(Mode::Turn)->hasFocus());
  QTest::keyClick(card.button(Mode::Turn), Qt::Key_Down);
  QCOMPARE(chosen.last().at(0).value<Mode>(), Mode::Size);
  QTest::keyClick(card.button(Mode::Size), Qt::Key_Right);
  QCOMPARE(chosen.last().at(0).value<Mode>(), Mode::Move);
  QTest::keyClick(card.button(Mode::Move), Qt::Key_Left);
  QCOMPARE(chosen.last().at(0).value<Mode>(), Mode::Size);
  QTest::keyClick(card.button(Mode::Size), Qt::Key_Space);
  QCOMPARE(chosen.last().at(0).value<Mode>(), Mode::Size);
}

void TestPropEdit::handleColoursComeFromTokens()
{
  for (bool dark : {true, false})
  {
    PropEdit::Gizmo g;
    g.build(Mode::Move, unitProp(), frontView(), viewSize);
    const int hovered = handleFor(g, 1);
    const QVector<PropEdit::Piece> pieces = g.pieces(hovered, -1, dark);
    // three lines, then three letter badges
    QCOMPARE(pieces.size(), 6);
    for (int axis = 0; axis < 3; axis++)
    {
      QCOMPARE(pieces.at(axis).colour, ViewColours::ring(axis, axis == 1, dark));
      QCOMPARE(pieces.at(3 + axis).colour, ViewColours::axis(axis, dark));
      QVERIFY(pieces.at(axis).triangles.size() % 3 == 0 && !pieces.at(axis).triangles.isEmpty());
    }
    // the size squares have an edge in surface under the axis colour
    g.build(Mode::Size, unitProp(), frontView(), viewSize);
    const QVector<PropEdit::Piece> squares = g.pieces(-1, -1, dark);
    QCOMPARE(squares.size(), 12);
    QCOMPARE(squares.at(0).colour, Theme::color(Theme::Token::Surface, dark));
    QCOMPARE(squares.at(1).colour, ViewColours::axis(0, dark));
    // rings: one thick line per segment, two triangles each
    g.build(Mode::Turn, unitProp(), frontView(), viewSize);
    QCOMPARE(g.pieces(-1, -1, dark).at(2).triangles.size(), PropEdit::Look::ringSegments * 6);
  }
  // at 200 percent the lines and badges are twice as wide
  PropEdit::Gizmo small, big;
  small.build(Mode::Size, unitProp(), frontView(), viewSize, 1.0);
  big.build(Mode::Size, unitProp(), frontView(), viewSize, 2.0);
  auto width = [](const PropEdit::Piece& p)
  {
    double lo = 1e9, hi = -1e9;
    for (const QPointF& q : p.triangles) { lo = qMin(lo, q.x()); hi = qMax(hi, q.x()); }
    return hi - lo;
  };
  QVERIFY(qFuzzyCompare(width(big.pieces(-1, -1, true).at(1)), 2 * width(small.pieces(-1, -1, true).at(1))));
  QVERIFY(qFuzzyCompare(width(small.pieces(-1, -1, true).at(1)), double(PropEdit::Look::badge)));
}

void TestPropEdit::cardAndPanelTokenColours()
{
  using Theme::Token;
  for (const UiRender::Variant& v : UiRender::variants())
  {
    if (v.percent == 150) continue;
    UiRender::Scope scope(v);
    PropEditCard card;
    card.setPropName(QStringLiteral("Box 0"));
    QImage image = UiRender::render(&card);
    UiRender::save(image, QStringLiteral("propeditcard"), v);
    QString why = UiRender::expectColour(image, &card, &card, QPoint(card.width() / 2, 3), Theme::color(Token::Surface, v.dark),
                                         v.name() + " card");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // Move is on: selection fill
    QPushButton* move = card.button(Mode::Move);
    why = UiRender::expectColour(image, &card, move, QPoint(4, move->height() / 2), Theme::color(Token::Selection, v.dark),
                                 v.name() + " Move on");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    QPushButton* turn = card.button(Mode::Turn);
    why = UiRender::expectColour(image, &card, turn, QPoint(4, turn->height() / 2), Theme::color(Token::SurfaceRaised, v.dark),
                                 v.name() + " Turn off");
    QVERIFY2(why.isEmpty(), qPrintable(why));

    SelectedPropPanel panel;
    panel.showProp(QStringLiteral("Box 0"), 0, unitProp());
    image = UiRender::render(&panel);
    UiRender::save(image, QStringLiteral("selectedprop"), v);
    const QList<QLabel*> badges = panel.findChildren<QLabel*>(QStringLiteral("propAxisBadge"));
    QCOMPARE(badges.size(), 9);
    for (int i = 0; i < 9; i++)
    {
      QLabel* badge = badges.at(i);
      if (!badge->isVisible()) continue;
      const Token token = i % 3 == 0 ? Token::AxisX : (i % 3 == 1 ? Token::AxisY : Token::AxisZ);
      why = UiRender::expectColour(image, &panel, badge, QPoint(1, badge->height() / 2), Theme::color(token, v.dark),
                                   v.name() + " badge " + QString::number(i));
      QVERIFY2(why.isEmpty(), qPrintable(why));
    }
    why = UiRender::expectColour(image, &panel, panel.shapeButton(0), QPoint(4, panel.shapeButton(0)->height() / 2),
                                 Theme::color(Token::Selection, v.dark), v.name() + " Box on");
    QVERIFY2(why.isEmpty(), qPrintable(why));
  }
}

void TestPropEdit::cardAndPanelNotClipped()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    PropEditCard card;
    card.setPropName(QStringLiteral("Wrist_left"));
    UiRender::render(&card);
    QStringList clipped = UiRender::clipping(&card);
    QVERIFY2(clipped.isEmpty(), qPrintable(v.name() + ": " + clipped.join(QStringLiteral("; "))));
    // the card is 260 pixels wide at 100 percent, as the mockup's .m2edit
    QCOMPARE(card.width(), TextSize::scaled(260));
    SelectedPropPanel panel;
    panel.showProp(QStringLiteral("Wrist_left"), 3, unitProp());
    UiRender::render(&panel);
    clipped = UiRender::clipping(&panel);
    QVERIFY2(clipped.isEmpty(), qPrintable(v.name() + ": " + clipped.join(QStringLiteral("; "))));
  }
}

void TestPropEdit::textsAreTheMockupWords()
{
  QCOMPARE(PropEdit::tr(PropEdit::cardTitle), registerText(QStringLiteral("PROPED-01")).value(0));
  QCOMPARE((QStringList{PropEdit::modeName(Mode::Move), PropEdit::modeName(Mode::Turn), PropEdit::modeName(Mode::Size)}),
           registerText(QStringLiteral("PROPED-02")));
  QCOMPARE(PropEdit::tr(PropEdit::cardCaption), registerText(QStringLiteral("PROPED-03")).value(0));
  for (int i = 0; i < 3; i++)
  {
    QCOMPARE(PropEdit::tr(PropEdit::dragLabel[i]), registerText(QStringLiteral("PROPED-0%1").arg(4 + i)).value(0));
    QCOMPARE(PropEdit::tr(PropEdit::done[i]), registerText(QStringLiteral("PROPED-07")).value(i));
    QCOMPARE(PropEdit::hintText(Mode(i)), registerText(QStringLiteral("PROPED-%1").arg(20 + i)).value(0));
  }
  QCOMPARE(PropEdit::tr(PropEdit::cancelled), registerText(QStringLiteral("PROPED-24")).value(0));
  QCOMPARE(PropEdit::tr(PropEdit::dragKeys), registerText(QStringLiteral("PROPED-25")).value(0));
  QCOMPARE(SelectedPropSpec::tr(SelectedPropSpec::heading), registerText(QStringLiteral("PROPED-13")).value(0));
  QCOMPARE(SelectedPropSpec::tr(SelectedPropSpec::placeHeading), registerText(QStringLiteral("PROPED-14")).value(0));
  QCOMPARE(SelectedPropSpec::tr(SelectedPropSpec::sizeHeading), registerText(QStringLiteral("PROPED-15")).value(0));
  QCOMPARE((QStringList{SelectedPropSpec::tr(SelectedPropSpec::sizeRows[0]), SelectedPropSpec::tr(SelectedPropSpec::sizeRows[1]),
                        SelectedPropSpec::tr(SelectedPropSpec::sizeRows[2])}),
           registerText(QStringLiteral("PROPED-16")));
  QCOMPARE(SelectedPropSpec::tr(SelectedPropSpec::pickLine), registerText(QStringLiteral("PROPED-19")).value(0));
  QCOMPARE(SelectedPropSpec::tr(SelectedPropSpec::shapeHeading), registerText(QStringLiteral("PROPED-23")).value(0));
  QStringList shapes;
  for (const char* s : SelectedPropSpec::shapes) shapes << SelectedPropSpec::tr(s);
  QCOMPARE(shapes, registerText(QStringLiteral("PROPED-11")));
  // the program fills in the names
  QCOMPARE(PropEdit::doneText(Mode::Turn, QStringLiteral("Box 0")), QStringLiteral("Box 0 turned. Ctrl+Z undoes it."));
  QCOMPARE(PropEdit::dragText(Mode::Size, QStringLiteral("Box 0"), 2), QStringLiteral("Changing the size of Box 0 along Z."));
}

void TestPropEdit::propFileRoundTrip()
{
  // values a drag gives (not whole numbers), as the prop keeps them (floats)
  QList<PropRecord> records;
  PropRecord a;
  a.type = 3;
  const float values[9] = {2.5f, -0.0635f, 41.337f, 0.787402f, 12.25f, 3.0f, 29.968f, 359.5f, 0.1f};
  for (int i = 0; i < 3; i++)
  {
    a.position[i] = values[i];
    a.scale[i] = values[3 + i];
    a.rotation[i] = values[6 + i];
  }
  a.attachment = 5;
  PropRecord b;
  b.type = 0;
  records << a << b;
  const QByteArray file = PropFile::write(records);
  const PropFile::Result read = PropFile::parse(file);
  QVERIFY2(read.ok(), qPrintable(read.error));
  QCOMPARE(read.props->size(), 2);
  const PropRecord& back = read.props->at(0);
  QCOMPARE(back.type, 3);
  QCOMPARE(back.attachment, 5);
  for (int i = 0; i < 3; i++)
  {
    QCOMPARE(float(back.position[i]), values[i]);
    QCOMPARE(float(back.scale[i]), values[3 + i]);
    QCOMPARE(float(back.rotation[i]), values[6 + i]);
  }
  QCOMPARE(read.props->at(1).type, 0);
  // written again it is the same file
  QCOMPARE(PropFile::write(*read.props), file);
  QVERIFY(file.startsWith("3 2.5 -0.0635"));
}

void TestPropEdit::viewDrawsTrianglesOnly()
{
  // docs/DEV.md "Web-ready code": the handles are triangles, no wide lines, OpenGL in the view only
  const QString view = source("animationview.cpp");
  const int start = view.indexOf(QStringLiteral("void AnimationView::drawGizmo()"));
  QVERIFY(start > 0);
  const int end = view.indexOf(QStringLiteral("\n}\n"), start);
  const QString body = view.mid(start, end - start);
  QVERIFY(body.contains(QStringLiteral("GL_TRIANGLES")));
  QVERIFY(!body.contains(QStringLiteral("GL_LINES")));
  QVERIFY(!body.contains(QStringLiteral("LineWidth")));
  // the old Avimator handles (cones, spheres, cubes, Shift for scale) are gone
  QVERIFY(!view.contains(QStringLiteral("drawDragHandles")));
  for (const char* name : {"propedit.cpp", "propeditcard.cpp", "selectedproppanel.cpp"})
  {
    const QString text = source(name);
    QVERIFY2(!text.contains(QStringLiteral("glDraw")) && !text.contains(QStringLiteral("GL_")), name);
  }
}

void TestPropEdit::windowMakesUndoSteps()
{
  // the window turns every drag, key and field change into one Undo step and says what happened
  const QString props = source("pinglymotionwindowprops.cpp");
  QVERIFY(props.contains(QStringLiteral("connect(ui->animationView,&AnimationView::propEdited")));
  QVERIFY(props.contains(QStringLiteral("recordPropStep(prop->id(),before,after);")));
  QVERIFY(props.contains(QStringLiteral("statusBar()->showMessage(PropEdit::doneText(mode,prop->name()),5000);")));
  QVERIFY(props.contains(QStringLiteral("connect(m_selectedProp,&SelectedPropPanel::stateEdited")));
  QVERIFY(props.contains(QStringLiteral("m_undo->recordStep(")));
  QVERIFY(props.contains(QStringLiteral("if(m_undo) m_undo->clearSteps();")));
  // Esc in the view cancels a drag and says so
  const QString view = source("animationview.cpp");
  QVERIFY(view.contains(QStringLiteral("if(event->key()==Qt::Key_Escape) cancelPropDrag();")));
  QVERIFY(view.contains(QStringLiteral("emit statusMessage(PropEdit::tr(PropEdit::cancelled));")));
  // the prop file is written in one go
  QVERIFY(source("pinglymotionwindowfile.cpp").contains(QStringLiteral("file.write(PropFile::write(records));")));
}

QTEST_MAIN(TestPropEdit)
#include "tst_propedit.moc"
