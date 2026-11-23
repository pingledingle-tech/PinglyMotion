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
#include "scriptlab/lsl/sandbox.h"
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
    void forceCommandsSignal();
    void forcedFieldOfView();
    void forcedFocus();
    void forcedOffsets();
    void forcedModeAndBadForce();

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
  // RLVa refuses a distance that is not a number, so it is never held (docs/RLV_OPTION_TYPES.md)
  const RlvState::Outcome outcome = state.apply("collar", "@setcam_avdistmax:far=n").outcomes.value(0);
  QCOMPARE(outcome.result, Rlv::Result::Failed);
  QCOMPARE(outcome.reason, QString("@setcam_avdistmax:far=n: \"far\" is not a number. Use a number such as 1.5."));
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(!o.active());
  QVERIFY(o.limits().ignored.isEmpty());
  QCOMPARE(o.lines(), QStringList{"The script does not limit the camera now."});
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
  // RLVa refuses an offset that is not three numbers, so it is never held (docs/RLV_OPTION_TYPES.md)
  const RlvState::ApplyResult r = state.apply("collar", "@setcam_eyeoffset:1/2=n,setcam_focusoffset:0/0/500=n,setcam_eyeoffsetscale:0=n");
  QVERIFY(r.ok());
  QCOMPARE(r.outcomes.value(0).result, Rlv::Result::Failed);
  QCOMPARE(r.outcomes.value(0).reason, QString("@setcam_eyeoffset:1/2=n: \"1/2\" is not three numbers x/y/z. Use three numbers such as 0/0/1."));
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), (QStringList{
    "The script does not limit the camera now.",
    "Not used: setcam_focusoffset:0/0/500 is not an offset x/y/z in metres.",
    "Not used: setcam_eyeoffsetscale:0 is not a scale."}));
  QVERIFY(state.apply("collar", "@clear").ok());
  QVERIFY(state.apply("collar", "@setcam_eyeoffsetscale:2=n").ok());
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), (QStringList{
    "The script does not limit the camera now.",
    "Not used: setcam_eyeoffsetscale changes nothing without setcam_eyeoffset."}));
}

void TestCameraOverride::forceCommandsSignal()
{
  RlvState state;
  QSignalSpy spy(&state, &RlvState::changed);
  // a force command that acts outside the sandbox: recorded, owned by its object, and a change
  QCOMPARE(state.apply("collar", "@setcam_fov:0.5=force").outcomes.value(0).result, Rlv::Result::Retained);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(state.recorded(), (QMap<QString, QString>{{"setcam_fov", "0.5"}}));
  QCOMPARE(state.recordedBy(), (QMap<QString, QString>{{"setcam_fov", "collar"}}));
  // the same value again from the same object changes nothing
  QVERIFY(state.apply("collar", "@setcam_fov:0.5=force").ok());
  QCOMPARE(spy.count(), 1);
  // a new value, then the same value from another object: each is a change
  QVERIFY(state.apply("collar", "@setcam_fov:0.75=force").ok());
  QCOMPARE(spy.count(), 2);
  QVERIFY(state.apply("hud", "@setcam_fov:0.75=force").ok());
  QCOMPARE(spy.count(), 3);
  QCOMPARE(state.recordedBy().value("setcam_fov"), QString("hud"));
  // sky and debug values too (one line, one signal)
  QVERIFY(state.apply("collar", "@setenv_daytime:0.25=force,setdebug_renderfarclip:64=force").ok());
  QCOMPARE(spy.count(), 4);
  QCOMPARE(state.recordedBy(), (QMap<QString, QString>{{"setcam_fov", "hud"}, {"setdebug_renderfarclip", "collar"}, {"setenv_daytime", "collar"}}));
  // a force command that is not carried out and not recorded is no change
  QVERIFY(state.apply("collar", "@tpto:128/128/30=force").ok());
  QCOMPARE(spy.count(), 4);
  // @clear of an object that set nothing leaves the values
  QVERIFY(state.apply("other", "@clear").ok());
  QCOMPARE(spy.count(), 4);
  QCOMPARE(state.recorded().size(), 3);
  // @clear with a filter forgets only the collar's matching value
  QVERIFY(state.apply("collar", "@clear=daytime").ok());
  QCOMPARE(spy.count(), 5);
  QCOMPARE(state.recorded(), (QMap<QString, QString>{{"setcam_fov", "0.75"}, {"setdebug_renderfarclip", "64"}}));
  // @clear forgets all of the collar's values
  QVERIFY(state.apply("collar", "@clear").ok());
  QCOMPARE(spy.count(), 6);
  QCOMPARE(state.recorded(), (QMap<QString, QString>{{"setcam_fov", "0.75"}}));
  QCOMPARE(state.apply("collar", "@getdebug_renderfarclip=3").replies.value(0).text, QString());
  // Reset with only a recorded value left is a change
  state.reset();
  QCOMPARE(spy.count(), 7);
  QVERIFY(state.recorded().isEmpty());
  QVERIFY(state.recordedBy().isEmpty());
  state.reset();
  QCOMPARE(spy.count(), 7);
}

void TestCameraOverride::forcedFieldOfView()
{
  RlvState state;   // setcam_fov is plain RLV
  CameraOverride o;
  o.setState(&state);
  o.setEnabled(true);
  Camera camera;
  const CameraPosition before = camera.state();
  connect(&o, &CameraOverride::changed, this, [&]() { apply(o, camera); });

  QVERIFY(state.apply("collar", "@setcam_fov:0.5=force").ok());
  QVERIFY(o.active());
  QVERIFY(o.limits().hasForcedView);
  QCOMPARE(o.limits().forcedView, 28.64788975654116);
  QCOMPARE(o.constraint().fieldOfView, 28.6478898f);
  QVERIFY(!o.constraint().turnAroundPoint);
  QCOMPARE(camera.fieldOfView(), 28.6478898f);
  compareState(camera.state(), before);
  QCOMPARE(o.lines(), (QStringList{
    "Camera limited by the script: field of view forced to 28.6 degrees.",
    "The 3D view shows a field of view of 28.6 degrees instead of 60.",
    "Held by: collar.",
    "Untick the box to get your own camera back."}));

  // a held minimum still holds over the forced value
  QVERIFY(state.apply("hud", "@setcam_fovmin:1.5=n").ok());
  QCOMPARE(camera.fieldOfView(), 85.9436693f);
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: min field of view 85.9 degrees, field of view forced to 28.6 degrees."));
  QCOMPARE(o.lines().at(2), QString("Held by: collar, hud."));
  QVERIFY(state.apply("hud", "@clear").ok());
  QCOMPARE(camera.fieldOfView(), 28.6478898f);

  // a forced value can be wider than 60
  QVERIFY(state.apply("collar", "@setcam_fov:1.5=force").ok());
  QCOMPARE(camera.fieldOfView(), 85.9436693f);

  // unticked and ticked again
  o.setEnabled(false);
  QCOMPARE(camera.fieldOfView(), 60.0f);
  o.setEnabled(true);
  QCOMPARE(camera.fieldOfView(), 85.9436693f);

  // the collar's @clear forgets its forced value: 60 degrees and the camera from before
  QVERIFY(state.apply("collar", "@clear").ok());
  QVERIFY(!o.active());
  QCOMPARE(camera.fieldOfView(), 60.0f);
  compareState(camera.state(), before);
  QCOMPARE(o.lines(), QStringList{"The script does not limit the camera now."});

  // Reset does the same
  QVERIFY(state.apply("collar", "@setcam_fov:0.5=force").ok());
  QCOMPARE(camera.fieldOfView(), 28.6478898f);
  state.reset();
  QCOMPARE(camera.fieldOfView(), 60.0f);
  compareState(camera.state(), before);
}

void TestCameraOverride::forcedFocus()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);   // setcam_focus is RLVa only
  CameraOverride o;
  o.setState(&state);
  Camera camera;
  camera.panBy(20, 5);
  const CameraPosition before = camera.state();
  connect(&o, &CameraOverride::changed, this, [&]() { apply(o, camera); });
  o.setEnabled(true);

  const QString avatar = QString(lsl::Sandbox::OWNER_KEY);
  QCOMPARE(avatar, QString("a2e76fcd-9360-4f6d-a924-000000000001"));
  QVERIFY(state.apply("hud", "@setcam_focus:" + avatar + ";2;0=force").ok());
  QVERIFY(o.limits().forcedFocus);
  QVERIFY(o.active());
  const CameraConstraint c = o.constraint();
  QVERIFY(c.turnAroundPoint);
  QVERIFY(!c.fixedEye);
  QCOMPARE(c.point, CameraOverride::avatarPoint());
  QVERIFY((camera.pivot() - CameraOverride::avatarPoint()).length() < 1e-4f);   // float rounding only
  QCOMPARE(o.lines(), (QStringList{
    "Camera limited by the script: focus forced on the avatar.",
    "The camera turns around the avatar. Held by: hud.",
    "Untick the box to get your own camera back."}));

  // another target: the 3D view has nothing else to look at
  QVERIFY(state.apply("hud", "@setcam_focus:11111111-2222-3333-4444-555555555555=force").ok());
  QVERIFY(!o.active());
  compareState(camera.state(), before);
  QCOMPARE(o.lines(), (QStringList{
    "The script does not limit the camera now.",
    "Not shown: setcam_focus:11111111-2222-3333-4444-555555555555=force. The 3D view has only the avatar to focus on."}));

  // the avatar again (upper case is the same key), then Reset
  QVERIFY(state.apply("hud", "@setcam_focus:" + avatar.toUpper() + "=force").ok());
  QVERIFY(o.active());
  state.reset();
  QVERIFY(!o.active());
  compareState(camera.state(), before);
}

void TestCameraOverride::forcedOffsets()
{
  RlvState state;
  CameraOverride o;
  o.setState(&state);
  Camera camera;
  const CameraPosition before = camera.state();
  connect(&o, &CameraOverride::changed, this, [&]() { apply(o, camera); });
  o.setEnabled(true);

  // a forced focus offset
  QVERIFY(state.apply("hud", "@setcam_focusoffset:0/0/1=force").ok());
  QVERIFY(o.limits().focusOffsetForced);
  QCOMPARE(o.constraint().point, CameraOverride::avatarPoint() + QVector3D(0.0f, 39.3700787f, 0.0f));
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: focus offset 0/0/1 m (forced)."));

  // a held focus offset wins over the forced one
  QVERIFY(state.apply("collar", "@setcam_focusoffset:1/0/0=n").ok());
  QVERIFY(!o.limits().focusOffsetForced);
  QCOMPARE(o.constraint().point, CameraOverride::avatarPoint() + QVector3D(0.0f, 0.0f, 39.3700787f));
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: focus offset 1/0/0 m."));
  QVERIFY(state.apply("collar", "@clear").ok());
  QCOMPARE(o.constraint().point, CameraOverride::avatarPoint() + QVector3D(0.0f, 39.3700787f, 0.0f));

  // forced eye offset and scale: the same camera as the held ones (eyeOffset())
  QVERIFY(state.apply("hud", "@setcam_focusoffset:0/0/0=force,setcam_eyeoffset:-2/0/1=force").ok());
  QCOMPARE(o.constraint().eye, CameraOverride::avatarPoint() + QVector3D(0.0f, 39.3700787f, -78.7401574f));
  QCOMPARE(camera.distance(), 88.0342635f);
  QCOMPARE(camera.xRotation(), 26.5650512f);
  QCOMPARE(camera.yRotation(), 180.0f);
  QVERIFY(state.apply("hud", "@setcam_eyeoffsetscale:0.5=force").ok());
  QCOMPARE(o.constraint().eye, CameraOverride::avatarPoint() + QVector3D(0.0f, 19.6850394f, -39.3700787f));
  QCOMPARE(o.lines().first(), QString("Camera limited by the script: focus offset 0/0/0 m (forced), "
                                      "eye offset -2/0/1 m (forced), eye offset scale 0.5 (forced)."));

  // the hud's @clear: exactly the camera from before
  QVERIFY(state.apply("hud", "@clear").ok());
  QVERIFY(!o.active());
  compareState(camera.state(), before);
}

void TestCameraOverride::forcedModeAndBadForce()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);   // setcam_mode is RLVa only
  CameraOverride o;
  o.setState(&state);
  QSignalSpy spy(&o, &CameraOverride::changed);
  QVERIFY(state.apply("hud", "@setcam_mode:mouselook=force").ok());
  QCOMPARE(spy.count(), 1);
  QVERIFY(o.limits().hasForcedMode);
  QCOMPARE(o.limits().forcedMode, QString("mouselook"));
  QVERIFY(!o.limits().any());
  QCOMPARE(o.lines(), QStringList{"Not shown: setcam_mode:mouselook=force. The 3D view always looks from outside the avatar."});
  o.setEnabled(true);
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), (QStringList{
    "The script does not limit the camera now.",
    "Not shown: setcam_mode:mouselook=force. The 3D view always looks from outside the avatar."}));

  // a forced field of view that is not a number, an offset that is not x/y/z: RLVa refuses
  // them, so nothing is recorded and the 3D view does not change (docs/RLV_OPTION_TYPES.md)
  const QStringList linesBefore = o.lines();
  QCOMPARE(state.apply("hud", "@setcam_fov:wide=force").outcomes.value(0).result, Rlv::Result::Failed);
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), linesBefore);
  QCOMPARE(state.apply("hud", "@setcam_eyeoffset:1/2=force").outcomes.value(0).result, Rlv::Result::Failed);
  QCOMPARE(o.lines(), linesBefore);
  QVERIFY(!state.recorded().contains("setcam_fov"));
  QVERIFY(!state.recorded().contains("setcam_eyeoffset"));
  state.reset();
  QCOMPARE(o.lines(), QStringList{"The script does not limit the camera now."});
}

QTEST_GUILESS_MAIN(TestCameraOverride)
#include "tst_cameraoverride.moc"
