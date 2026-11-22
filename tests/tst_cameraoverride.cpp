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
    void fieldOfViewLimits();
    void zoomLimits();
    void viewMaxBelowMin();
    void viewWithDistance();
    void badViewOptions();
    void focusOffset();
    void eyeOffset();
    void badOffsets();

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
  QVERIFY(state.apply("collar", "@detach=n,sendim=n,setcam_avdist:2=n,setcam_mouselook=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(!o.limits().any());
  QVERIFY(!o.active());
}

void TestCameraOverride::fieldOfViewLimits()
{
  RlvState state;   // setcam_fovmin and setcam_fovmax are plain RLV
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  Camera camera;
  const CameraPosition before = camera.state();
  connect(&o, &CameraOverride::changed, this, [&]() { apply(o, camera); });

  // at most 0.5 rad (28.6 degrees): narrower than 60
  QVERIFY(state.apply("collar", "@setcam_fovmax:0.5=n").ok());
  QVERIFY(o.active());
  QCOMPARE(o.limits().maxView, 28.64788975654116);
  QCOMPARE(o.constraint().fieldOfView, 28.6478898f);
  QVERIFY(!o.constraint().turnAroundPoint);
  QCOMPARE(camera.fieldOfView(), 28.6478898f);
  compareState(camera.state(), before);   // only the field of view changes
  QCOMPARE(o.lines(), (QStringList{
    "Camera limited by the script: max field of view 28.6 degrees.",
    "The 3D view shows a field of view of 28.6 degrees instead of 60.",
    "Held by: collar.",
    "Untick the box to get your own camera back."}));

  // a second object: at least 1.5 rad (85.9 degrees) from the hud, and the collar lets go
  QVERIFY(state.apply("hud", "@setcam_fovmin:1.5=n").ok());
  QVERIFY(state.apply("collar", "@clear").ok());
  QCOMPARE(camera.fieldOfView(), 85.9436693f);
  QCOMPARE(o.lines().at(1), QString("The 3D view shows a field of view of 85.9 degrees instead of 60."));

  // a minimum below 60 leaves the view as it is
  QVERIFY(state.apply("hud", "@setcam_fovmin:1.5=y,setcam_fovmin:0.25=n").ok());
  QCOMPARE(camera.fieldOfView(), 60.0f);
  QCOMPARE(o.lines().at(1), QString("The 3D view keeps its field of view of 60 degrees."));

  // unticked: 60 degrees and the camera from before
  o.setEnabled(false);
  QCOMPARE(camera.fieldOfView(), 60.0f);
  compareState(camera.state(), before);
}

void TestCameraOverride::zoomLimits()
{
  // the old zoom limits: a multiplier of the default 60 degrees
  RlvState state;
  QVERIFY(state.apply("collar", "@camzoommin:2=n").ok());   // cannot zoom out past 2x: 30 degrees at most
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QCOMPARE(o.limits().maxView, 30.0);
  QCOMPARE(o.constraint().fieldOfView, 30.0f);
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: max field of view 30 degrees."));
  QVERIFY(state.apply("collar", "@camzoommin:2=y,camzoommax:0.5=n").ok());   // cannot zoom in past 0.5x: 120 at least
  QCOMPARE(o.limits().minView, 120.0);
  QCOMPARE(o.constraint().fieldOfView, 120.0f);
}

void TestCameraOverride::viewMaxBelowMin()
{
  RlvState state;
  QVERIFY(state.apply("collar", "@camzoommax:0.5=n,setcam_fovmax:0.5235987755982988=n").ok());   // at least 120, at most 30
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QCOMPARE(o.constraint().fieldOfView, 30.0f);
  QCOMPARE(o.lines(), (QStringList{
    "Camera limited by the script: min field of view 120 degrees, max field of view 30 degrees.",
    "The max field of view is below the min field of view, so the max wins.",
    "The 3D view shows a field of view of 30 degrees instead of 60.",
    "Held by: collar.",
    "Untick the box to get your own camera back."}));
}

void TestCameraOverride::viewWithDistance()
{
  RlvState state;
  QVERIFY(state.apply("collar", "@camdistmax:2=n,setcam_fovmax:0.5=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  const CameraConstraint c = o.constraint();
  QCOMPARE(c.maxDistance, 78.7401574f);
  QCOMPARE(c.fieldOfView, 28.6478898f);
  QVERIFY(c.turnAroundPoint);
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: max distance 2 m, max field of view 28.6 degrees."));
  QCOMPARE(o.lines().at(2), QString("The camera turns around the avatar. Held by: collar."));
}

void TestCameraOverride::badViewOptions()
{
  RlvState state;
  QVERIFY(state.apply("collar", "@setcam_fovmin:4=n,setcam_fovmax=n,camzoommax:0=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), (QStringList{
    "The script does not limit the camera now.",
    "Not used: setcam_fovmin:4 is not an angle in radians.",
    "Not used: setcam_fovmax: is not an angle in radians.",
    "Not used: camzoommax:0 is not a zoom multiplier."}));
}

void TestCameraOverride::focusOffset()
{
  RlvState state;
  QVERIFY(state.apply("collar", "@setcam_focusoffset:0/0/0.5=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(o.active());
  const CameraConstraint c = o.constraint();
  QVERIFY(c.turnAroundPoint);
  QVERIFY(!c.fixedEye);
  QCOMPARE(c.point, QVector3D(0.0f, 43.528519f + 19.6850394f, 0.0f));
  QCOMPARE(o.lines(), (QStringList{
    "Camera limited by the script: focus offset 0/0/0.5 m.",
    "The camera turns around the avatar. Held by: collar.",
    "Untick the box to get your own camera back."}));
  // x forward is the figure's front (+Z), y left its left (+X)
  QCOMPARE(CameraOverride::toWorld(QVector3D(1.0f, 2.0f, 3.0f)), QVector3D(78.7401574f, 118.110236f, 39.3700787f));
}

void TestCameraOverride::eyeOffset()
{
  RlvState state;
  QVERIFY(state.apply("collar", "@setcam_eyeoffset:-2/0/1=n").ok());
  CameraOverride o;
  o.setState(&state);
  Camera camera;
  const CameraPosition before = camera.state();
  connect(&o, &CameraOverride::changed, this, [&]() { apply(o, camera); });
  o.setEnabled(true);
  CameraConstraint c = o.constraint();
  QVERIFY(c.fixedEye);
  QCOMPARE(c.point, CameraOverride::avatarPoint());
  QCOMPARE(c.eye, CameraOverride::avatarPoint() + QVector3D(0.0f, 39.3700787f, -78.7401574f));
  // behind the figure (2 m back, 1 m up): 2.236 m away, looking down at 26.6 degrees from behind
  QCOMPARE(camera.distance(), 88.0342635f);
  QCOMPARE(camera.xRotation(), 26.5650512f);
  QCOMPARE(camera.yRotation(), 180.0f);
  QCOMPARE(o.lines(), (QStringList{
    "Camera limited by the script: eye offset -2/0/1 m.",
    "The camera is held at the eye offset, looking at the avatar: turning and zooming do nothing.",
    "The camera turns around the avatar. Held by: collar.",
    "Untick the box to get your own camera back."}));

  // a scale and a focus offset from another object
  QVERIFY(state.apply("hud", "@setcam_eyeoffsetscale:0.5=n,setcam_focusoffset:0/0/1=n").ok());
  c = o.constraint();
  QCOMPARE(c.point, CameraOverride::avatarPoint() + QVector3D(0.0f, 39.3700787f, 0.0f));
  QCOMPARE(c.eye, CameraOverride::avatarPoint() + QVector3D(0.0f, 19.6850394f, -39.3700787f));
  QCOMPARE(camera.xRotation(), -26.5650512f);   // the focus is above the eye now: looking up
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: focus offset 0/0/1 m, eye offset -2/0/1 m, eye offset scale 0.5."));
  QCOMPARE(o.lines().at(1), QString("The camera is held at the eye offset, looking at the focus offset: turning and zooming do nothing."));

  // every object lets go: exactly the camera from before
  state.reset();
  compareState(camera.state(), before);
}

void TestCameraOverride::badOffsets()
{
  RlvState state;
  QVERIFY(state.apply("collar", "@setcam_eyeoffset:1/2=n,setcam_focusoffset:0/0/500=n,setcam_eyeoffsetscale:0=n").ok());
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), (QStringList{
    "The script does not limit the camera now.",
    "Not used: setcam_eyeoffset:1/2 is not an offset x/y/z in metres.",
    "Not used: setcam_focusoffset:0/0/500 is not an offset x/y/z in metres.",
    "Not used: setcam_eyeoffsetscale:0 is not a scale."}));
  QVERIFY(state.apply("collar", "@clear").ok());
  QVERIFY(state.apply("collar", "@setcam_eyeoffsetscale:2=n").ok());
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), (QStringList{
    "The script does not limit the camera now.",
    "Not used: setcam_eyeoffsetscale changes nothing without setcam_eyeoffset."}));
}

QTEST_GUILESS_MAIN(TestCameraOverride)
#include "tst_cameraoverride.moc"
