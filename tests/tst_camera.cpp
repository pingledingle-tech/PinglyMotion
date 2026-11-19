/*
 * PinglyMotion - tests for the camera: which mouse button and keys do what
 * (docs/POSING_UX.md, "Mouse and keys"), and that the camera turns around
 * the point in the middle of the view (#88)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "camera.h"
#include "scenestate.h"

Q_DECLARE_METATYPE(CameraDrag)

class TestCamera : public QObject
{
  Q_OBJECT

  private slots:
    void dragFor_data();
    void dragFor();
    void numberPadViews_data();
    void numberPadViews();
    void focusKeys();
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

  private:
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
  QTest::addColumn<int>("button");
  QTest::addColumn<int>("keys");
  QTest::addColumn<CameraDrag>("drag");

  const int shift = Qt::ShiftModifier, ctrl = Qt::ControlModifier, alt = Qt::AltModifier;

  // simple (default)
  QTest::newRow("left drag") << int(Qt::LeftButton) << 0 << CameraDrag::Orbit;
  QTest::newRow("right drag") << int(Qt::RightButton) << 0 << CameraDrag::Pan;
  // Blender
  QTest::newRow("middle drag") << int(Qt::MiddleButton) << 0 << CameraDrag::Orbit;
  QTest::newRow("shift middle") << int(Qt::MiddleButton) << shift << CameraDrag::Pan;
  QTest::newRow("ctrl middle") << int(Qt::MiddleButton) << ctrl << CameraDrag::Zoom;
  // Second Life
  QTest::newRow("alt drag") << int(Qt::LeftButton) << alt << CameraDrag::OrbitAndZoom;
  QTest::newRow("ctrl alt drag") << int(Qt::LeftButton) << (ctrl | alt) << CameraDrag::Orbit;
  QTest::newRow("ctrl alt shift drag") << int(Qt::LeftButton) << (ctrl | alt | shift) << CameraDrag::Pan;
  // the old joint keys do nothing special to the camera any more
  QTest::newRow("shift drag") << int(Qt::LeftButton) << shift << CameraDrag::Orbit;
  QTest::newRow("ctrl drag") << int(Qt::LeftButton) << ctrl << CameraDrag::Orbit;
  QTest::newRow("other button") << int(Qt::BackButton) << 0 << CameraDrag::None;
}

void TestCamera::dragFor()
{
  QFETCH(int, button);
  QFETCH(int, keys);
  QFETCH(CameraDrag, drag);
  QCOMPARE(Camera::dragFor(Qt::MouseButton(button), Qt::KeyboardModifiers(keys)), drag);
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
  // not the number pad
  QTest::newRow("main 1") << int(Qt::Key_1) << 0 << false << 0.0f << 0.0f;
  QTest::newRow("page down") << int(Qt::Key_PageDown) << 0 << false << 0.0f << 0.0f;
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
  QVERIFY(Camera::isFocusKey(Qt::Key_F, Qt::NoModifier));
  QVERIFY(Camera::isFocusKey(Qt::Key_F, Qt::ShiftModifier));
  QVERIFY(Camera::isFocusKey(Qt::Key_Period, Qt::KeypadModifier));
  // number pads with a decimal comma (e.g. Norwegian) and Num Lock off
  QVERIFY(Camera::isFocusKey(Qt::Key_Comma, Qt::KeypadModifier));
  QVERIFY(Camera::isFocusKey(Qt::Key_Delete, Qt::KeypadModifier));
  QVERIFY(!Camera::isFocusKey(Qt::Key_F, Qt::ControlModifier));
  QVERIFY(!Camera::isFocusKey(Qt::Key_Period, Qt::NoModifier));
  QVERIFY(!Camera::isFocusKey(Qt::Key_Delete, Qt::NoModifier));
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

QTEST_GUILESS_MAIN(TestCamera)
#include "tst_camera.moc"
