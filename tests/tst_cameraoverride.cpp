/*
 * PinglyMotion - the Script Lab camera override (job 7b)
 *
 * Exact answers: which RLV restrictions limit the camera and how (strictest
 * limit wins over several objects, metres to inches, a maximum below the
 * minimum wins), the words for the panel, off by default, and with a real
 * Camera: the clamped distance and focus while on, and exactly the camera
 * from before after unticking, @clear, Reset and a mode switch.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QSignalSpy>

#include "camera.h"
#include "scriptlab/cameraoverride.h"
#include "scriptlab/rlvstate.h"

class TestCameraOverride : public QObject
{
  Q_OBJECT

  private slots:
    void offByDefault();
    void distanceLimits();
    void strictestWins();
    void maxBelowMin();
    void unlockOnly();
    void oldNamesInPlainRlv();
    void badOption();
    void linesWhileOff();
    void cameraClampedAndRestored();
    void resetAndModeRestore();
    void otherRestrictionsDoNothing();

  private:
    static void compareState(const CameraPosition& a, const CameraPosition& b);
    // what the 3D view does with the override (AnimationView::applyCameraOverride)
    static void apply(const CameraOverride& o, Camera& camera)
    {
      if (o.active()) camera.setConstraint(o.constraint());
      else camera.clearConstraint();
    }
};

void TestCameraOverride::compareState(const CameraPosition& a, const CameraPosition& b)
{
  QCOMPARE(a.panX, b.panX);
  QCOMPARE(a.panY, b.panY);
  QCOMPARE(a.panZ, b.panZ);
  QCOMPARE(a.rotX, b.rotX);
  QCOMPARE(a.rotY, b.rotY);
  QCOMPARE(a.pivotDistance, b.pivotDistance);
}

void TestCameraOverride::offByDefault()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  CameraOverride o;
  o.setState(&state);
  QVERIFY(!o.enabled());
  QVERIFY(state.apply("collar", "@setcam_avdistmax:3=n").ok());
  QVERIFY(o.limits().any());
  QVERIFY(!o.active());
  QCOMPARE(o.limits().maxDistance, 3.0);
}

void TestCameraOverride::distanceLimits()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  CameraOverride o;
  o.setState(&state);
  QSignalSpy spy(&o, &CameraOverride::changed);
  o.setEnabled(true);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(o.lines(), QStringList{"The script does not limit the camera now."});

  QVERIFY(state.apply("collar", "@setcam_avdistmin:1=n,setcam_avdistmax:3=n").ok());
  QVERIFY(o.active());
  const CameraConstraint c = o.constraint();
  QCOMPARE(c.minDistance, 39.3700787f);
  QCOMPARE(c.maxDistance, 118.110236f);
  QVERIFY(c.turnAroundPoint);
  QCOMPARE(c.point, QVector3D(0.0f, 43.528519f, 0.0f));
  QCOMPARE(o.lines(), (QStringList{
    "Camera limited by the script: min distance 1 m, max distance 3 m.",
    "The camera turns around the avatar. Held by: collar.",
    "Untick the box to get your own camera back."}));

  QVERIFY(state.apply("collar", "@clear").ok());
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), QStringList{"The script does not limit the camera now."});
}

void TestCameraOverride::strictestWins()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  QVERIFY(state.apply("collar", "@setcam_avdistmax:5=n,setcam_avdistmin:0.5=n").ok());
  QVERIFY(state.apply("hud", "@setcam_avdistmax:2.5=n,setcam_avdistmin:1.25=n").ok());
  const CameraOverride::Limits l = CameraOverride::limitsFrom(state);
  QCOMPARE(l.maxDistance, 2.5);
  QCOMPARE(l.minDistance, 1.25);
  QCOMPARE(l.objects, (QStringList{"collar", "hud"}));
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: min distance 1.25 m, max distance 2.5 m."));
  QCOMPARE(o.lines().at(1), QString("The camera turns around the avatar. Held by: collar, hud."));
  // the hud lets go: the collar's limits are left
  QVERIFY(state.apply("hud", "@clear").ok());
  QCOMPARE(o.limits().maxDistance, 5.0);
  QCOMPARE(o.limits().minDistance, 0.5);
  QCOMPARE(o.constraint().maxDistance, 196.850394f);
  QCOMPARE(o.constraint().minDistance, 19.6850394f);
}

void TestCameraOverride::maxBelowMin()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  QVERIFY(state.apply("collar", "@setcam_avdistmin:4=n,setcam_avdistmax:0=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QCOMPARE(o.limits().maxDistance, 0.0);
  // 0 m is as close as the view goes (3 inches); the minimum gives way to the maximum
  QCOMPARE(o.constraint().maxDistance, 3.0f);
  QCOMPARE(o.constraint().minDistance, 3.0f);
  QCOMPARE(o.lines(), (QStringList{
    "Camera limited by the script: min distance 4 m, max distance 0 m.",
    "The max distance is below the min distance, so the max distance wins.",
    "The 3D view goes as close as it can.",
    "The camera turns around the avatar. Held by: collar.",
    "Untick the box to get your own camera back."}));
}

void TestCameraOverride::unlockOnly()
{
  // setcam_unlock is plain RLV: no mode switch needed
  RlvState state;
  QVERIFY(state.apply("collar", "@setcam_unlock=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(o.active());
  const CameraConstraint c = o.constraint();
  QCOMPARE(c.minDistance, 0.0f);
  QCOMPARE(c.maxDistance, 0.0f);
  QVERIFY(c.turnAroundPoint);
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: focus kept on the avatar."));
}

void TestCameraOverride::oldNamesInPlainRlv()
{
  // the old names are plain RLV aliases of the new ones
  RlvState state;
  QVERIFY(state.apply("collar", "@camdistmax:2=n,camdistmin:0.5=n,camunlock=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QCOMPARE(o.limits().maxDistance, 2.0);
  QCOMPARE(o.limits().minDistance, 0.5);
  QVERIFY(o.limits().focusOnAvatar);
  QCOMPARE(o.constraint().maxDistance, 78.7401574f);
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: min distance 0.5 m, max distance 2 m, focus kept on the avatar."));
}

void TestCameraOverride::badOption()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  QVERIFY(state.apply("collar", "@setcam_avdistmax:far=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(!o.active());
  QCOMPARE(o.limits().ignored, QStringList{"setcam_avdistmax:far"});
  QCOMPARE(o.lines(), (QStringList{
    "The script does not limit the camera now.",
    "Not used: setcam_avdistmax:far is not a distance in metres."}));
}

void TestCameraOverride::linesWhileOff()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  CameraOverride o;
  o.setState(&state);
  QCOMPARE(o.lines(), QStringList());
  QVERIFY(state.apply("collar", "@setcam_avdistmax:3=n").ok());
  QCOMPARE(o.lines(), QStringList{"The script limits the camera: max distance 3 m. Tick the box to see it in the 3D view."});
}

void TestCameraOverride::cameraClampedAndRestored()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  CameraOverride o;
  o.setState(&state);
  Camera camera;
  camera.rotate(5.0f, 20.0f);
  camera.panBy(12.0f, 4.0f);
  camera.zoom(3.0f);   // 300 inches, about 7.6 m
  const CameraPosition before = camera.state();
  connect(&o, &CameraOverride::changed, this, [&]() { apply(o, camera); });

  // held but off: nothing changes
  QVERIFY(state.apply("collar", "@setcam_avdistmax:3=n").ok());
  compareState(camera.state(), before);

  // ticked: 3 m from the avatar, turning around it, same direction
  o.setEnabled(true);
  QCOMPARE(camera.distance(), 118.110236f);
  QVERIFY((camera.pivot() - CameraOverride::avatarPoint()).length() < 0.01f);
  QCOMPARE(camera.xRotation(), before.rotX);
  QCOMPARE(camera.yRotation(), before.rotY);
  camera.zoom(5.0f);
  QCOMPARE(camera.distance(), 118.110236f);

  // a stricter limit from another object
  QVERIFY(state.apply("hud", "@setcam_avdistmax:1=n").ok());
  QCOMPARE(camera.distance(), 39.3700787f);

  // unticked: exactly the camera from before
  o.setEnabled(false);
  compareState(camera.state(), before);

  // ticked again, then every object lets go with @clear: exactly the camera from before
  o.setEnabled(true);
  QCOMPARE(camera.distance(), 39.3700787f);
  QVERIFY(state.apply("hud", "@clear").ok());
  QCOMPARE(camera.distance(), 39.3700787f);   // 1 m is within the collar's 3 m: the camera stays
  QVERIFY(state.apply("collar", "@clear").ok());
  QVERIFY(!o.active());
  compareState(camera.state(), before);
}

void TestCameraOverride::resetAndModeRestore()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  Camera camera;
  const CameraPosition before = camera.state();
  connect(&o, &CameraOverride::changed, this, [&]() { apply(o, camera); });

  QVERIFY(state.apply("collar", "@setcam_avdistmin:4=n").ok());
  QCOMPARE(camera.distance(), 157.480315f);
  state.reset();
  compareState(camera.state(), before);

  QVERIFY(state.apply("collar", "@setcam_avdistmin:4=n").ok());
  QCOMPARE(camera.distance(), 157.480315f);
  state.setMode(Rlv::Mode::Rlv);   // a mode switch starts clean
  compareState(camera.state(), before);
}

void TestCameraOverride::otherRestrictionsDoNothing()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  QVERIFY(state.apply("collar", "@detach=n,sendim=n,setcam_fovmin:0.5=n,setcam_mouselook=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(!o.limits().any());
  QVERIFY(!o.active());
}

QTEST_GUILESS_MAIN(TestCameraOverride)
#include "tst_cameraoverride.moc"
