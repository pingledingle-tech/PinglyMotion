/*
 * PinglyMotion - tests for the camera: which mouse button and keys do what in
 * each camera habit (docs/UX_DECISIONS.md, 6c, UX phase 4a), the view keys,
 * framing the whole figure, and that the camera turns around
 * the point in the middle of the view (#88)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "camera.h"
#include "panellayout.h"
#include "scenestate.h"

Q_DECLARE_METATYPE(CameraDrag)
Q_DECLARE_METATYPE(CameraHabit)

class TestCamera : public QObject
{
  Q_OBJECT

  private slots:
    void dragFor_data();
    void dragFor();
    void numberPadViews_data();
    void numberPadViews();
    void habitIds();
    void focusKeys();
    void viewKeys();
    void keyTurnMovesTenDegrees();
    void frameBoxFitsTheFigure();
    void pivotIsInTheMiddle();
    void rotateKeepsPivot();
    void zoomKeepsPivot();
    void panMovesPivot();
    void topViewLeftSmoothly();
    void orbitBelowTheFloor();
    void heightLimitsHold();
    void panWorksBelowTheFloor();
    void numberPadAfterGoingBelow();
    void storedPositionKeepsDistance();
    void shiftedPerspectiveNoShift();
    void shiftedPerspectiveCentresFreeArea();
    void constraintClampsDistance();
    void constraintTurnsAroundPoint();
    void constraintHoldsAfterMoves();
    void clearConstraintRestoresExactly();
    void secondConstraintKeepsFirstCamera();
    void fieldOfViewFromConstraint();
    void fixedEyeHoldsCamera();

  private:
    static void compareState(const CameraPosition& a, const CameraPosition& b);
    // where a point in front of the camera (world space) ends up on screen
    // (widget pixels), through this projection matrix and an otherwise
    // plain view looking straight down -Z
    static QPointF onScreen(const QMatrix4x4& projection,const QVector3D& point,const QSizeF& view);
    // where a world point ends up in camera space, as drawn
    static QVector3D seen(Camera& camera, const QVector3D& world);
    static void compare(const QVector3D& a, const QVector3D& b, float within = 0.01f);
};

QVector3D TestCamera::seen(Camera& camera, const QVector3D& world)
{
  camera.setModelView();
  return SceneState::modelview().map(world);
}

void TestCamera::compare(const QVector3D& a, const QVector3D& b, float within)
{
  if((a - b).length() > within)
    QFAIL(qPrintable(QString("(%1, %2, %3) is not (%4, %5, %6)")
                     .arg(a.x()).arg(a.y()).arg(a.z()).arg(b.x()).arg(b.y()).arg(b.z())));
}

void TestCamera::dragFor_data()
{
  QTest::addColumn<int>("habit");
  QTest::addColumn<int>("button");
  QTest::addColumn<int>("keys");
  QTest::addColumn<CameraDrag>("drag");
  QTest::addColumn<bool>("picks");

  const int shift = Qt::ShiftModifier, ctrl = Qt::ControlModifier, alt = Qt::AltModifier;
  const int pm = int(CameraHabit::PinglyMotion), maya = int(CameraHabit::Maya),
            blender = int(CameraHabit::Blender), sl = int(CameraHabit::SecondLife);
  const int left = Qt::LeftButton, right = Qt::RightButton, middle = Qt::MiddleButton;

  // PinglyMotion (the default): left picks, or orbits when dragged; right pans
  QTest::newRow("pm left") << pm << left << 0 << CameraDrag::Orbit << true;
  QTest::newRow("pm right") << pm << right << 0 << CameraDrag::Pan << false;
  QTest::newRow("pm middle") << pm << middle << 0 << CameraDrag::None << false;
  QTest::newRow("pm alt left") << pm << left << alt << CameraDrag::Orbit << true;
  QTest::newRow("pm shift left") << pm << left << shift << CameraDrag::Orbit << true;
  QTest::newRow("pm ctrl left") << pm << left << ctrl << CameraDrag::Orbit << true;
  QTest::newRow("pm other button") << pm << int(Qt::BackButton) << 0 << CameraDrag::None << false;
  // Maya: Alt + left orbit, Alt + middle pan, Alt + right zoom; left picks
  QTest::newRow("maya alt left") << maya << left << alt << CameraDrag::Orbit << false;
  QTest::newRow("maya alt middle") << maya << middle << alt << CameraDrag::Pan << false;
  QTest::newRow("maya alt right") << maya << right << alt << CameraDrag::Zoom << false;
  QTest::newRow("maya left") << maya << left << 0 << CameraDrag::None << true;
  QTest::newRow("maya right") << maya << right << 0 << CameraDrag::None << false;
  QTest::newRow("maya middle") << maya << middle << 0 << CameraDrag::None << false;
  // Blender: middle orbit, Shift + middle pan, Ctrl + middle zoom; left picks
  QTest::newRow("blender middle") << blender << middle << 0 << CameraDrag::Orbit << false;
  QTest::newRow("blender shift middle") << blender << middle << shift << CameraDrag::Pan << false;
  QTest::newRow("blender ctrl middle") << blender << middle << ctrl << CameraDrag::Zoom << false;
  QTest::newRow("blender left") << blender << left << 0 << CameraDrag::None << true;
  QTest::newRow("blender alt left") << blender << left << alt << CameraDrag::None << true;
  QTest::newRow("blender right") << blender << right << 0 << CameraDrag::None << false;
  // Second Life: Alt + left orbit and zoom, Ctrl + Alt orbit, Ctrl + Alt + Shift pan; left picks
  QTest::newRow("sl alt left") << sl << left << alt << CameraDrag::OrbitAndZoom << false;
  QTest::newRow("sl ctrl alt left") << sl << left << (ctrl | alt) << CameraDrag::Orbit << false;
  QTest::newRow("sl ctrl alt shift left") << sl << left << (ctrl | alt | shift) << CameraDrag::Pan << false;
  QTest::newRow("sl left") << sl << left << 0 << CameraDrag::None << true;
  QTest::newRow("sl right") << sl << right << 0 << CameraDrag::None << false;
  QTest::newRow("sl middle") << sl << middle << 0 << CameraDrag::None << false;
}

void TestCamera::dragFor()
{
  QFETCH(int, habit);
  QFETCH(int, button);
  QFETCH(int, keys);
  QFETCH(CameraDrag, drag);
  QFETCH(bool, picks);
  const CameraGesture gesture = Camera::gestureFor(CameraHabit(habit), Qt::MouseButton(button), Qt::KeyboardModifiers(keys));
  QCOMPARE(gesture.drag, drag);
  QCOMPARE(gesture.picks, picks);
}

void TestCamera::habitIds()
{
  QCOMPARE(Camera::habitFor(QStringLiteral("pinglymotion")), CameraHabit::PinglyMotion);
  QCOMPARE(Camera::habitFor(QStringLiteral("maya")), CameraHabit::Maya);
  QCOMPARE(Camera::habitFor(QStringLiteral("blender")), CameraHabit::Blender);
  QCOMPARE(Camera::habitFor(QStringLiteral("sl")), CameraHabit::SecondLife);
  QCOMPARE(Camera::habitFor(QString()), CameraHabit::PinglyMotion);
  QCOMPARE(Camera::habitFor(QStringLiteral("Maya")), CameraHabit::PinglyMotion);
  // only these habits focus by Alt + click or double click (the mockup's Focus rows)
  QVERIFY(Camera::focusesOnAltClick(CameraHabit::SecondLife));
  QVERIFY(!Camera::focusesOnAltClick(CameraHabit::PinglyMotion));
  QVERIFY(!Camera::focusesOnAltClick(CameraHabit::Maya));
  QVERIFY(!Camera::focusesOnAltClick(CameraHabit::Blender));
  QVERIFY(Camera::focusesOnDoubleClick(CameraHabit::PinglyMotion));
  QVERIFY(!Camera::focusesOnDoubleClick(CameraHabit::Maya));
  QVERIFY(!Camera::focusesOnDoubleClick(CameraHabit::Blender));
  QVERIFY(!Camera::focusesOnDoubleClick(CameraHabit::SecondLife));
}

void TestCamera::numberPadViews_data()
{
  QTest::addColumn<int>("key");
  QTest::addColumn<int>("keys");
  QTest::addColumn<bool>("isView");
  QTest::addColumn<float>("height");
  QTest::addColumn<float>("angle");

  const int pad = Qt::KeypadModifier, ctrl = Qt::ControlModifier;
  QTest::newRow("1 front") << int(Qt::Key_1) << pad << true << 0.0f << 0.0f;
  QTest::newRow("ctrl 1 back") << int(Qt::Key_1) << (pad | ctrl) << true << 0.0f << 180.0f;
  QTest::newRow("3 side") << int(Qt::Key_3) << pad << true << 0.0f << 90.0f;
  QTest::newRow("ctrl 3 other side") << int(Qt::Key_3) << (pad | ctrl) << true << 0.0f << 270.0f;
  QTest::newRow("7 top") << int(Qt::Key_7) << pad << true << 90.0f << 0.0f;
  // Num Lock off
  QTest::newRow("end front") << int(Qt::Key_End) << pad << true << 0.0f << 0.0f;
  QTest::newRow("page down side") << int(Qt::Key_PageDown) << pad << true << 0.0f << 90.0f;
  QTest::newRow("home top") << int(Qt::Key_Home) << pad << true << 90.0f << 0.0f;
  // the digits above the letters, in the 3D view (the mockup)
  QTest::newRow("main 1 front") << int(Qt::Key_1) << 0 << true << 0.0f << 0.0f;
  QTest::newRow("main ctrl 1 back") << int(Qt::Key_1) << ctrl << true << 0.0f << 180.0f;
  QTest::newRow("main 3 side") << int(Qt::Key_3) << 0 << true << 0.0f << 90.0f;
  QTest::newRow("main 7 top") << int(Qt::Key_7) << 0 << true << 90.0f << 0.0f;
  QTest::newRow("main ctrl 7") << int(Qt::Key_7) << ctrl << false << 0.0f << 0.0f;
  // not a view key
  QTest::newRow("shift 1") << int(Qt::Key_1) << int(Qt::ShiftModifier) << false << 0.0f << 0.0f;
  QTest::newRow("alt 1") << int(Qt::Key_1) << int(Qt::AltModifier) << false << 0.0f << 0.0f;
  QTest::newRow("main 2") << int(Qt::Key_2) << 0 << false << 0.0f << 0.0f;
  QTest::newRow("page down") << int(Qt::Key_PageDown) << 0 << false << 0.0f << 0.0f;
  QTest::newRow("end") << int(Qt::Key_End) << 0 << false << 0.0f << 0.0f;
  QTest::newRow("home") << int(Qt::Key_Home) << 0 << false << 0.0f << 0.0f;
  QTest::newRow("pad 5") << int(Qt::Key_5) << pad << false << 0.0f << 0.0f;
}

void TestCamera::numberPadViews()
{
  QFETCH(int, key);
  QFETCH(int, keys);
  QFETCH(bool, isView);
  QFETCH(float, height);
  QFETCH(float, angle);

  float h = -1, a = -1;
  QCOMPARE(Camera::viewForKey(key, Qt::KeyboardModifiers(keys), &h, &a), isView);
  if(isView)
  {
    QCOMPARE(h, height);
    QCOMPARE(a, angle);
  }
}

void TestCamera::focusKeys()
{
  const Qt::KeyboardModifiers pad = Qt::KeypadModifier;
  for (CameraHabit habit : {CameraHabit::PinglyMotion, CameraHabit::Maya, CameraHabit::Blender, CameraHabit::SecondLife})
  {
    // F in every habit
    QVERIFY(Camera::isFocusKey(habit, Qt::Key_F, Qt::NoModifier));
    // Shift+F is the whole figure now
    QVERIFY(!Camera::isFocusKey(habit, Qt::Key_F, Qt::ShiftModifier));
    QVERIFY(!Camera::isFocusKey(habit, Qt::Key_F, Qt::ControlModifier));
    QVERIFY(!Camera::isFocusKey(habit, Qt::Key_F, Qt::AltModifier));
    QVERIFY(!Camera::isFocusKey(habit, Qt::Key_Period, Qt::NoModifier));
    QVERIFY(!Camera::isFocusKey(habit, Qt::Key_Delete, Qt::NoModifier));
    // number pad "." only in the Blender habit (also with a decimal comma, e.g. Norwegian, and Num Lock off)
    const bool blender = habit == CameraHabit::Blender;
    QCOMPARE(Camera::isFocusKey(habit, Qt::Key_Period, pad), blender);
    QCOMPARE(Camera::isFocusKey(habit, Qt::Key_Comma, pad), blender);
    QCOMPARE(Camera::isFocusKey(habit, Qt::Key_Delete, pad), blender);
    QVERIFY(!Camera::isFocusKey(habit, Qt::Key_Delete, pad | Qt::ControlModifier));
  }
}

void TestCamera::viewKeys()
{
  // Shift+F: the whole figure
  QVERIFY(Camera::isWholeFigureKey(Qt::Key_F, Qt::ShiftModifier));
  QVERIFY(!Camera::isWholeFigureKey(Qt::Key_F, Qt::NoModifier));
  QVERIFY(!Camera::isWholeFigureKey(Qt::Key_F, Qt::ShiftModifier | Qt::ControlModifier));
  QVERIFY(!Camera::isWholeFigureKey(Qt::Key_G, Qt::ShiftModifier));
  // number pad 0 (and Insert with Num Lock off): home view; the main 0 is the menu's own key
  QVERIFY(Camera::isHomeKey(Qt::Key_0, Qt::KeypadModifier));
  QVERIFY(Camera::isHomeKey(Qt::Key_Insert, Qt::KeypadModifier));
  QVERIFY(!Camera::isHomeKey(Qt::Key_0, Qt::NoModifier));
  QVERIFY(!Camera::isHomeKey(Qt::Key_Insert, Qt::NoModifier));
  QVERIFY(!Camera::isHomeKey(Qt::Key_0, Qt::KeypadModifier | Qt::ControlModifier));
  // Alt + arrow keys: 10 degrees around or up and down
  float up = -1, around = -1;
  QVERIFY(Camera::turnForKey(Qt::Key_Left, Qt::AltModifier, &up, &around));
  QCOMPARE(up, 0.0f); QCOMPARE(around, -10.0f);
  QVERIFY(Camera::turnForKey(Qt::Key_Right, Qt::AltModifier, &up, &around));
  QCOMPARE(up, 0.0f); QCOMPARE(around, 10.0f);
  QVERIFY(Camera::turnForKey(Qt::Key_Up, Qt::AltModifier, &up, &around));
  QCOMPARE(up, 10.0f); QCOMPARE(around, 0.0f);
  QVERIFY(Camera::turnForKey(Qt::Key_Down, Qt::AltModifier | Qt::KeypadModifier, &up, &around));
  QCOMPARE(up, -10.0f); QCOMPARE(around, 0.0f);
  // plain arrows step frames, Ctrl or Shift with Alt are not this
  QVERIFY(!Camera::turnForKey(Qt::Key_Left, Qt::NoModifier, &up, &around));
  QVERIFY(!Camera::turnForKey(Qt::Key_Left, Qt::AltModifier | Qt::ShiftModifier, &up, &around));
  QVERIFY(!Camera::turnForKey(Qt::Key_Left, Qt::AltModifier | Qt::ControlModifier, &up, &around));
  QVERIFY(!Camera::turnForKey(Qt::Key_A, Qt::AltModifier, &up, &around));
}

void TestCamera::keyTurnMovesTenDegrees()
{
  Camera camera;
  const float x = camera.xRotation(), y = camera.yRotation();
  const QVector3D pivot = camera.pivot();
  float up, around;
  QVERIFY(Camera::turnForKey(Qt::Key_Right, Qt::AltModifier, &up, &around));
  camera.rotate(up, around);
  QCOMPARE(camera.xRotation(), x);
  QCOMPARE(camera.yRotation(), y + 10.0f);
  QVERIFY(Camera::turnForKey(Qt::Key_Up, Qt::AltModifier, &up, &around));
  camera.rotate(up, around);
  QCOMPARE(camera.xRotation(), x + 10.0f);
  compare(camera.pivot(), pivot);
}

void TestCamera::frameBoxFitsTheFigure()
{
  Camera camera;
  camera.rotate(20, 35);
  const float x = camera.xRotation(), y = camera.yRotation();
  // a figure 70 high, 20 wide and 10 deep, standing off the middle
  const QVector3D low(10, 0, -5), high(30, 70, 5);
  camera.frameBox(low, high, 1.0f);
  // looks at the middle of the box, keeps its turn
  compare(camera.pivot(), QVector3D(20, 35, 0));
  QCOMPARE(camera.xRotation(), x);
  QCOMPARE(camera.yRotation(), y);
  // from radius * margin / sin(half the field of view): 60 degrees, so sin = 0.5
  const float radius = (high - low).length() / 2;
  QVERIFY(qAbs(camera.distance() - radius * Camera::FrameMargin / 0.5f) < 0.01f);
  // a wide view is decided by its height, a narrow one by its width
  QVERIFY(qAbs(camera.frameDistance(10, 2.0f) - 10 * Camera::FrameMargin / 0.5f) < 0.001f);
  const float halfWidth = std::atan(std::tan(qDegreesToRadians(30.0f)) * 0.5f);
  QVERIFY(qAbs(camera.frameDistance(10, 0.5f) - 10 * Camera::FrameMargin / std::sin(halfWidth)) < 0.001f);
  // every corner of the box is inside the view (in front of the camera, within the 60 degree cone)
  for (int i = 0; i < 8; ++i)
  {
    const QVector3D corner(i & 1 ? high.x() : low.x(), i & 2 ? high.y() : low.y(), i & 4 ? high.z() : low.z());
    const QVector3D p = seen(camera, corner);
    QVERIFY(p.z() < 0);
    QVERIFY(qAbs(p.y()) / -p.z() < std::tan(qDegreesToRadians(30.0f)));
    QVERIFY(qAbs(p.x()) / -p.z() < std::tan(qDegreesToRadians(30.0f)));
  }
  // a single point gets a distance too
  camera.frameBox(QVector3D(1, 2, 3), QVector3D(1, 2, 3), 1.0f);
  compare(camera.pivot(), QVector3D(1, 2, 3));
  QVERIFY(qAbs(camera.distance() - 5 * Camera::FrameMargin / 0.5f) < 0.01f);
}

void TestCamera::pivotIsInTheMiddle()
{
  Camera camera;
  compare(seen(camera, camera.pivot()), QVector3D(0, 0, -camera.distance()));

  camera.lookAt(10, 50, 5, 30);
  compare(camera.pivot(), QVector3D(10, 50, 5));
  QCOMPARE(camera.distance(), 30.0f);
  compare(seen(camera, QVector3D(10, 50, 5)), QVector3D(0, 0, -30));
}

void TestCamera::rotateKeepsPivot()
{
  Camera camera;
  camera.lookAt(10, 50, 5, 30);
  camera.rotate(20, 45);
  QCOMPARE(camera.yRotation(), 45.0f);
  compare(camera.pivot(), QVector3D(10, 50, 5));
  // still in the middle of the view, as far away as before
  compare(seen(camera, QVector3D(10, 50, 5)), QVector3D(0, 0, -30));

  camera.rotate(-5, -90);
  QCOMPARE(camera.yRotation(), 315.0f);
  compare(seen(camera, QVector3D(10, 50, 5)), QVector3D(0, 0, -30));
}

void TestCamera::zoomKeepsPivot()
{
  Camera camera;
  camera.lookAt(0, 60, 0, 40);
  camera.zoom(0.5f);
  QCOMPARE(camera.distance(), 20.0f);
  compare(seen(camera, QVector3D(0, 60, 0)), QVector3D(0, 0, -20));
  // never closer than 3 units
  camera.zoom(0.01f);
  QCOMPARE(camera.distance(), 3.0f);
  compare(camera.pivot(), QVector3D(0, 60, 0), 0.1f);
}

void TestCamera::panMovesPivot()
{
  Camera camera;
  camera.lookAt(0, 40, 0, 50);
  camera.setView(0, 0);
  camera.panBy(10, -5);
  // front view: the pivot moves right and down in the world
  compare(camera.pivot(), QVector3D(10, 35, 0));
  QCOMPARE(camera.distance(), 50.0f);
}

void TestCamera::topViewLeftSmoothly()
{
  Camera camera;
  camera.lookAt(0, 40, 0, 80);
  camera.setView(90, 0);
  QCOMPARE(camera.xRotation(), 90.0f);
  compare(seen(camera, QVector3D(0, 40, 0)), QVector3D(0, 0, -80));
  // from above, the head is nearer than the feet
  QVERIFY(seen(camera, QVector3D(0, 60, 0)).z() > seen(camera, QVector3D(0, 0, 0)).z());

  // turning further up stays at the top; turning down goes on from there
  camera.rotate(5, 0);
  QCOMPARE(camera.xRotation(), 90.0f);
  camera.rotate(-5, 0);
  QCOMPARE(camera.xRotation(), 85.0f);
  compare(seen(camera, QVector3D(0, 40, 0)), QVector3D(0, 0, -80));
}

void TestCamera::orbitBelowTheFloor()
{
  Camera camera;
  camera.lookAt(0, 40, 0, 80);
  camera.setView(0, 0);
  camera.rotate(-20, 0);
  QCOMPARE(camera.xRotation(), -20.0f);
  // still looking at the same point from the same distance
  compare(seen(camera, QVector3D(0, 40, 0)), QVector3D(0, 0, -80));
  // seen from below: the feet are nearer than the head
  QVERIFY(seen(camera, QVector3D(0, 0, 0)).z() > seen(camera, QVector3D(0, 60, 0)).z());
}

void TestCamera::heightLimitsHold()
{
  Camera camera;
  camera.lookAt(0, 40, 0, 80);
  camera.setView(0, 0);
  camera.rotate(-500, 0);
  QCOMPARE(camera.xRotation(), -80.0f);
  compare(seen(camera, QVector3D(0, 40, 0)), QVector3D(0, 0, -80));
  camera.rotate(500, 0);
  QCOMPARE(camera.xRotation(), 80.0f);
  compare(seen(camera, QVector3D(0, 40, 0)), QVector3D(0, 0, -80));
}

void TestCamera::panWorksBelowTheFloor()
{
  Camera camera;
  camera.lookAt(0, 40, 0, 50);
  camera.setView(0, 0);
  camera.rotate(-30, 0);
  const float height = camera.xRotation();
  const QVector3D before = camera.pivot();
  camera.panBy(10, -5);
  // the view angle is unchanged and the pivot has moved
  QCOMPARE(camera.xRotation(), height);
  QVERIFY((camera.pivot() - before).length() > 5.0f);
  QCOMPARE(camera.distance(), 50.0f);
  // panning back goes back to the same place
  camera.panBy(-10, 5);
  compare(camera.pivot(), before);
}

void TestCamera::numberPadAfterGoingBelow()
{
  Camera camera;
  camera.lookAt(0, 40, 0, 80);
  camera.rotate(-40, 0);
  QVERIFY(camera.xRotation() < 0);
  float height = -1, angle = -1;
  QVERIFY(Camera::viewForKey(Qt::Key_1, Qt::KeypadModifier, &height, &angle));
  camera.setView(height, angle);
  QCOMPARE(camera.xRotation(), 0.0f);
  QCOMPARE(camera.yRotation(), 0.0f);
  compare(seen(camera, QVector3D(0, 40, 0)), QVector3D(0, 0, -80));
  // setView still never goes below the floor
  camera.setView(-10, 0);
  QCOMPARE(camera.xRotation(), 0.0f);
}

void TestCamera::storedPositionKeepsDistance()
{
  Camera camera;
  camera.lookAt(5, 30, 0, 25);
  camera.storeCameraPosition(1);
  camera.reset();
  camera.restoreCameraPosition(1);
  QCOMPARE(camera.distance(), 25.0f);
  compare(camera.pivot(), QVector3D(5, 30, 0));
}

QPointF TestCamera::onScreen(const QMatrix4x4& projection,const QVector3D& point,const QSizeF& view)
{
  const QVector4D clip=projection*QVector4D(point,1.0f);
  const float w=(clip.w()!=0.0f) ? clip.w() : 1.0f;
  // same mapping as AnimationView::ringPointOnScreen(): NDC to widget pixels
  return QPointF((clip.x()/w*0.5+0.5)*view.width(),(0.5-clip.y()/w*0.5)*view.height());
}

// with the centre at the view's own middle, shiftedPerspective() is the
// plain perspective matrix (P5, docs/archive/PANELS_PLAN.md)
void TestCamera::shiftedPerspectiveNoShift()
{
  const QRectF view(0,0,800,600);
  const QMatrix4x4 shifted=shiftedPerspective(60.0f,view,view.center(),1.0f,2000.0f);
  QMatrix4x4 plain;
  plain.perspective(60.0f,float(view.width()/view.height()),1.0f,2000.0f);
  for(int row=0;row<4;row++)
    for(int col=0;col<4;col++)
      QVERIFY(qAbs(shifted(row,col)-plain(row,col))<1e-5f);
  // a point straight ahead projects to the middle of the view
  compare(QVector3D(onScreen(shifted,QVector3D(0,0,-100),view.size())),QVector3D(400,300,0),0.5f);
}

// a floating panel covers the left half of the view: the free area is the
// right half, and the figure (straight ahead of the camera) should land in
// its middle, not the view's (P5, docs/archive/PANELS_PLAN.md)
void TestCamera::shiftedPerspectiveCentresFreeArea()
{
  const QRect view(0,0,800,600);
  const QList<QRect> panels{QRect(0,0,300,600)};
  const QRectF free=PanelLayout::freeArea(view,panels);
  QCOMPARE(free,QRectF(300,0,500,600));
  const QMatrix4x4 shifted=shiftedPerspective(60.0f,QRectF(view),free.center(),1.0f,2000.0f);
  compare(QVector3D(onScreen(shifted,QVector3D(0,0,-100),QSizeF(view.size()))),
          QVector3D(float(free.center().x()),float(free.center().y()),0),0.5f);
}

void TestCamera::compareState(const CameraPosition& a, const CameraPosition& b)
{
  QCOMPARE(a.panX, b.panX);
  QCOMPARE(a.panY, b.panY);
  QCOMPARE(a.panZ, b.panZ);
  QCOMPARE(a.rotX, b.rotX);
  QCOMPARE(a.rotY, b.rotY);
  QCOMPARE(a.pivotDistance, b.pivotDistance);
}

// Script Lab camera limits (job 7b): distance limits without a fixed point keep the pivot
void TestCamera::constraintClampsDistance()
{
  Camera camera;
  QCOMPARE(camera.distance(), 100.0f);
  const QVector3D pivot = camera.pivot();
  CameraConstraint c;
  c.maxDistance = 50.0f;
  camera.setConstraint(c);
  QVERIFY(camera.constrained());
  QCOMPARE(camera.distance(), 50.0f);
  compare(camera.pivot(), pivot);
  c.maxDistance = 0;
  c.minDistance = 150.0f;
  camera.setConstraint(c);
  QCOMPARE(camera.distance(), 150.0f);
  compare(camera.pivot(), pivot);
}

void TestCamera::constraintTurnsAroundPoint()
{
  Camera camera;
  camera.panBy(30.0f, -10.0f);
  const float rotX = camera.xRotation();
  const float rotY = camera.yRotation();
  CameraConstraint c;
  c.maxDistance = 118.110236f;
  c.turnAroundPoint = true;
  c.point = QVector3D(0.0f, 43.528519f, 0.0f);
  camera.setConstraint(c);
  compare(camera.pivot(), c.point);
  QCOMPARE(camera.distance(), 100.0f);   // already within 3 m
  QCOMPARE(camera.xRotation(), rotX);    // the direction is kept
  QCOMPARE(camera.yRotation(), rotY);
}

void TestCamera::constraintHoldsAfterMoves()
{
  Camera camera;
  CameraConstraint c;
  c.minDistance = 40.0f;
  c.maxDistance = 80.0f;
  c.turnAroundPoint = true;
  c.point = QVector3D(0.0f, 43.528519f, 0.0f);
  camera.setConstraint(c);
  QCOMPARE(camera.distance(), 80.0f);
  camera.zoom(10.0f);
  QCOMPARE(camera.distance(), 80.0f);
  camera.zoom(0.01f);
  QCOMPARE(camera.distance(), 40.0f);
  camera.panBy(200.0f, 50.0f);
  compare(camera.pivot(), c.point);
  camera.lookAt(100.0f, 0.0f, 100.0f, 500.0f);
  compare(camera.pivot(), c.point);
  QCOMPARE(camera.distance(), 80.0f);
  camera.rotate(20.0f, 45.0f);
  compare(camera.pivot(), c.point);
  camera.orbit(30.0f, 90.0f, 300.0f);
  compare(camera.pivot(), c.point);
  QCOMPARE(camera.distance(), 80.0f);
  camera.reset();
  compare(camera.pivot(), c.point);
  QCOMPARE(camera.distance(), 80.0f);
  camera.setView(90.0f, 0.0f);
  compare(camera.pivot(), c.point);
}

void TestCamera::clearConstraintRestoresExactly()
{
  Camera camera;
  camera.rotate(12.5f, 33.0f);
  camera.panBy(7.0f, -3.0f);
  camera.zoom(1.7f);
  const CameraPosition before = camera.state();
  CameraConstraint c;
  c.maxDistance = 20.0f;
  c.turnAroundPoint = true;
  c.point = QVector3D(0.0f, 43.528519f, 0.0f);
  camera.setConstraint(c);
  camera.rotate(-30.0f, 100.0f);
  camera.zoom(0.5f);
  camera.clearConstraint();
  QVERIFY(!camera.constrained());
  compareState(camera.state(), before);
  // without a constraint the camera moves freely again
  camera.zoom(2.0f);
  QVERIFY(camera.distance() > 20.0f);
  // a second clear changes nothing
  const CameraPosition after = camera.state();
  camera.clearConstraint();
  compareState(camera.state(), after);
}

void TestCamera::secondConstraintKeepsFirstCamera()
{
  Camera camera;
  const CameraPosition before = camera.state();
  CameraConstraint c;
  c.maxDistance = 50.0f;
  camera.setConstraint(c);
  c.maxDistance = 30.0f;
  camera.setConstraint(c);
  QCOMPARE(camera.distance(), 30.0f);
  c.maxDistance = 0;
  camera.setConstraint(c);   // the limit was lifted, the moved camera stays until cleared
  QCOMPARE(camera.distance(), 30.0f);
  camera.clearConstraint();
  compareState(camera.state(), before);
}

// Script Lab field of view limits (job 7b, part 2): only while the constraint sets one
void TestCamera::fieldOfViewFromConstraint()
{
  Camera camera;
  QCOMPARE(camera.fieldOfView(), 60.0f);
  const CameraPosition before = camera.state();
  CameraConstraint c;
  c.fieldOfView = 30.0f;
  camera.setConstraint(c);
  QCOMPARE(camera.fieldOfView(), 30.0f);
  // no distance limit and no fixed point: the camera does not move
  compareState(camera.state(), before);
  c.fieldOfView = 0;
  camera.setConstraint(c);
  QCOMPARE(camera.fieldOfView(), 60.0f);
  c.fieldOfView = 90.0f;
  camera.setConstraint(c);
  QCOMPARE(camera.fieldOfView(), 90.0f);
  camera.clearConstraint();
  QCOMPARE(camera.fieldOfView(), 60.0f);
  compareState(camera.state(), before);
}

// Script Lab eye offset (job 7b, part 3): the camera sits at the eye and looks at the point
void TestCamera::fixedEyeHoldsCamera()
{
  Camera camera;
  const CameraPosition before = camera.state();
  CameraConstraint c;
  c.turnAroundPoint = true;
  c.point = QVector3D(0.0f, 43.528519f, 0.0f);
  c.fixedEye = true;
  c.eye = c.point + QVector3D(30.0f, 40.0f, -50.0f);   // behind the figure, to its left, above
  camera.setConstraint(c);
  const float length = (c.eye - c.point).length();
  QCOMPARE(camera.distance(), length);
  compare(seen(camera, c.eye), QVector3D(0, 0, 0));
  compare(seen(camera, c.point), QVector3D(0, 0, -length));
  // turning, zooming and panning do nothing
  camera.rotate(20.0f, 70.0f);
  camera.zoom(3.0f);
  camera.panBy(10.0f, 10.0f);
  camera.setView(90.0f, 0.0f);
  compare(seen(camera, c.eye), QVector3D(0, 0, 0));
  compare(seen(camera, c.point), QVector3D(0, 0, -length));
  // a distance limit still holds along the same line
  c.maxDistance = 20.0f;
  camera.setConstraint(c);
  compare(seen(camera, c.point), QVector3D(0, 0, -20.0f));
  compare(seen(camera, c.point + (c.eye - c.point).normalized() * 20.0f), QVector3D(0, 0, 0));
  // straight in front of the figure
  c.maxDistance = 0;
  c.eye = c.point + QVector3D(0.0f, 0.0f, 80.0f);
  camera.setConstraint(c);
  QCOMPARE(camera.xRotation(), 0.0f);
  QCOMPARE(camera.yRotation(), 0.0f);
  compare(seen(camera, c.eye), QVector3D(0, 0, 0));
  camera.clearConstraint();
  compareState(camera.state(), before);
}

QTEST_GUILESS_MAIN(TestCamera)
#include "tst_camera.moc"
