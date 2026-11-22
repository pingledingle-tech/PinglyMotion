/*
 * PinglyMotion - tests of the Script Lab effects in the 3D view (issue 293)
 *
 * Exact answers only: attachment point names and joints; what a run answer
 * says is playing, worn and locked at the end; the frame an animation shows
 * after a given time; the safe lookup of an animation by name (no path can
 * leave the folders); and the preview: off by default, a script's
 * llStartAnimation of a bundled example plays it at the expected frame after
 * the run's simulated seconds, llStopAnimation and turning it off remove it,
 * an unknown name gives a clear error, and an animation the user has open is
 * never changed or marked changed.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <memory>

#include "animation.h"
#include "bvh.h"
#include "constants.h"
#include "scriptlab/effectspreview.h"
#include "scriptlab/labeffects.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

namespace
{
  QString examples() { return QString::fromUtf8(PINGLYMOTION_EXAMPLES); }

  bool writeFile(const QString& path, const QByteArray& text)
  {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  // a script that starts one animation at 1 s (and stops it at stopAt when that is above 1)
  QString startScript(const QString& name, double stopAt = 0)
  {
    return QStringLiteral(
      "integer started = FALSE;\n"
      "default {\n"
      "  state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION); }\n"
      "  run_time_permissions(integer p) { llSetTimerEvent(1.0); }\n"
      "  timer() {\n"
      "    llSetTimerEvent(0.0);\n"
      "    if (!started) { started = TRUE; llStartAnimation(\"%1\"); %2 }\n"
      "    else llStopAnimation(\"%1\");\n"
      "  }\n"
      "}\n").arg(name, stopAt > 1.0 ? QStringLiteral("llSetTimerEvent(%1);").arg(stopAt - 1.0) : QString());
  }

  QJsonObject run(const QString& source, double seconds)
  {
    return lsl::Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}});
  }
}

class TestLabEffects : public QObject
{
  Q_OBJECT

  private slots:
    void pointsAndJoints();
    void playingFromALog();
    void startTwiceAndStop();
    void wornObjectsAndLocks();
    void singleObjectLockIsNotWorn();
    void frames();
    void names();
    void lookup();
    void lookupIgnoresLinksAndDepth();
    void previewOffByDefault();
    void previewPlaysTheExample();
    void previewStopRestores();
    void previewUnknownName();
    void previewMarkers();
    void userAnimationUntouched();
    void sessionAttachLockAndClear();
};

void TestLabEffects::pointsAndJoints()
{
  QCOMPARE(LabEffects::pointName(1), QString("Chest"));
  QCOMPARE(LabEffects::pointName(39), QString("Neck"));
  QCOMPARE(LabEffects::pointName(55), QString("Right Hind Foot"));
  QCOMPARE(LabEffects::pointName(0), QString());
  QCOMPARE(LabEffects::pointName(56), QString());
  QCOMPARE(LabEffects::pointNumber("left hand"), 5);
  QCOMPARE(LabEffects::pointNumber(" Right Pec "), 30);
  QCOMPARE(LabEffects::pointNumber("elbow"), 0);
  QVERIFY(LabEffects::isHudPoint(35));
  QVERIFY(!LabEffects::isHudPoint(39));
  QCOMPARE(LabEffects::jointsForPoint(5), QStringList({"lHand", "lForeArm"}));
  QCOMPARE(LabEffects::jointsForPoint(39), QStringList({"neck"}));
  QCOMPARE(LabEffects::jointsForPoint(35), QStringList());
  QCOMPARE(LabEffects::jointsForPoint(99), QStringList());
}

void TestLabEffects::playingFromALog()
{
  const QJsonObject answer = run(startScript("wave"), 2.5);
  QVERIFY2(answer.value("ok").toBool(), qPrintable(QJsonDocument(answer).toJson()));
  const LabEffects::Effects effects = LabEffects::fromRun(answer);
  QCOMPARE(effects.playing.size(), 1);
  QCOMPARE(effects.playing.first().name, QString("wave"));
  QCOMPARE(effects.playing.first().started, 1.0);
  QCOMPARE(effects.endTime, 2.5);
  QVERIFY(effects.worn.isEmpty());
  QVERIFY(effects.lockedPoints.isEmpty());
  QVERIFY(!effects.logTruncated);
}

void TestLabEffects::startTwiceAndStop()
{
  const QJsonArray log{
    QJsonObject{{"t", 0.5}, {"type", "animation_start"}, {"name", "a"}},
    QJsonObject{{"t", 1.0}, {"type", "animation_start"}, {"name", "b"}},
    QJsonObject{{"t", 1.5}, {"type", "animation_start"}, {"name", "a"}},   // already playing: no change
    QJsonObject{{"t", 2.0}, {"type", "animation_stop"}, {"name", "c"}},    // not playing: no change
    QJsonObject{{"t", 2.5}, {"type", "animation_start"}, {"name", "c"}},
    QJsonObject{{"t", 3.0}, {"type", "animation_stop"}, {"name", "b"}}};
  const LabEffects::Effects effects = LabEffects::fromRun(QJsonObject{{"ok", true}, {"log", log}, {"time", 4.0}});
  QCOMPARE(effects.playing.size(), 2);
  QCOMPARE(effects.playing.at(0).name, QString("a"));
  QCOMPARE(effects.playing.at(0).started, 0.5);
  QCOMPARE(effects.playing.at(1).name, QString("c"));
  QCOMPARE(effects.playing.at(1).started, 2.5);

  const LabEffects::Effects stopped = LabEffects::fromRun(run(startScript("wave", 2.0), 3.0));
  QVERIFY(stopped.playing.isEmpty());
}

void TestLabEffects::wornObjectsAndLocks()
{
  // the answer of lsl_run with several objects (docs/CONTROL.md): objects with their attachment point,
  // and the RLV restrictions under rlv.state
  const QJsonObject answer{
    {"ok", true}, {"time", 5.0}, {"log", QJsonArray()},
    {"objects", QJsonArray{QJsonObject{{"name", "hud"}, {"key", "k-hud"}, {"attached", 35}},
                           QJsonObject{{"name", "cuffs"}, {"key", "k-cuffs"}, {"attached", 30}},
                           QJsonObject{{"name", "collar"}, {"key", "k-collar"}, {"attached", 39}},
                           QJsonObject{{"name", "chair"}, {"key", "k-chair"}, {"attached", 0}}}},
    {"rlv", QJsonObject{{"state", QJsonObject{{"restrictions", QJsonArray{
      QJsonObject{{"object", "k-cuffs"}, {"behaviour", "detach"}},
      QJsonObject{{"object", "k-cuffs"}, {"behaviour", "sendim"}},
      QJsonObject{{"object", "k-chair"}, {"behaviour", "detach"}},
      QJsonObject{{"object", "k-collar"}, {"behaviour", "detach"}, {"option", "left hand"}},
      QJsonObject{{"object", "k-hud"}, {"behaviour", "detach"}, {"option", "Chest"}},
      QJsonObject{{"object", "k-hud"}, {"behaviour", "detach"}, {"option", "chest"}},
      QJsonObject{{"object", "k-hud"}, {"behaviour", "detach"}, {"option", "elbow"}}}}}}}}};
  const LabEffects::Effects effects = LabEffects::fromRun(answer);
  QCOMPARE(effects.worn.size(), 3);
  QCOMPARE(effects.worn.at(0).object, QString("hud"));
  QCOMPARE(effects.worn.at(0).point, 35);
  QVERIFY(!effects.worn.at(0).locked);
  QCOMPARE(effects.worn.at(1).object, QString("cuffs"));
  QCOMPARE(effects.worn.at(1).point, 30);
  QVERIFY(effects.worn.at(1).locked);
  QCOMPARE(effects.worn.at(2).object, QString("collar"));
  QVERIFY(!effects.worn.at(2).locked);
  QCOMPARE(effects.lockedPoints, QList<int>({1, 5}));
  QCOMPARE(effects.unwornLocks, QStringList({"chair"}));
  QCOMPARE(effects.unknownPoints, QStringList({"elbow"}));
}

void TestLabEffects::singleObjectLockIsNotWorn()
{
  const QString source = "default { state_entry() { llOwnerSay(\"@detach=n\"); } }";
  const LabEffects::Effects effects = LabEffects::fromRun(run(source, 1.0));
  QVERIFY(effects.worn.isEmpty());
  QCOMPARE(effects.unwornLocks, QStringList({"Object"}));
}

void TestLabEffects::frames()
{
  const double ft = 1.0 / 30.0;
  // wave.dingle: 61 frames, loop 1 to 60 (0-based)
  QCOMPARE(LabEffects::frameAt(0.0, ft, 61, 1, 60), 1);
  QCOMPARE(LabEffects::frameAt(1.5, ft, 61, 1, 60), 46);
  QCOMPARE(LabEffects::frameAt(1.9, ft, 61, 1, 60), 58);
  QCOMPARE(LabEffects::frameAt(59.0 * ft, ft, 61, 1, 60), 60);
  QCOMPARE(LabEffects::frameAt(2.0, ft, 61, 1, 60), 1);     // 60 frames on: past the loop end, back to its start
  QCOMPARE(LabEffects::frameAt(2.5, ft, 61, 1, 60), 16);
  QCOMPARE(LabEffects::frameAt(1.0, ft, 61, 20, 30), 20);   // 1 + 30 = 31: past 30, so 20 + (31 - 20) % 11
  QCOMPARE(LabEffects::frameAt(1.1, ft, 61, 20, 30), 23);   // 1 + 33 = 34: 20 + (34 - 20) % 11
  QCOMPARE(LabEffects::frameAt(1.0, ft, 61, -1, -1), 31);   // no loop points: 1 to the last frame
  QCOMPARE(LabEffects::frameAt(5.0, ft, 1, 0, 0), 0);       // one frame
  QCOMPARE(LabEffects::frameAt(-3.0, ft, 61, 1, 60), 1);
  QCOMPARE(LabEffects::frameAt(1e30, ft, 61, 1, 60), 1);
}

void TestLabEffects::names()
{
  QVERIFY(LabEffects::validAnimationName("wave"));
  QVERIFY(LabEffects::validAnimationName("jump_slow"));
  QVERIFY(LabEffects::validAnimationName("my dance 2.0"));
  QVERIFY(!LabEffects::validAnimationName(""));
  QVERIFY(!LabEffects::validAnimationName("../wave"));
  QVERIFY(!LabEffects::validAnimationName("a/b"));
  QVERIFY(!LabEffects::validAnimationName("a\\b"));
  QVERIFY(!LabEffects::validAnimationName("C:wave"));
  QVERIFY(!LabEffects::validAnimationName(".hidden"));
  QVERIFY(!LabEffects::validAnimationName(" wave"));
  QVERIFY(!LabEffects::validAnimationName(QString("wa\nve")));
  QVERIFY(!LabEffects::validAnimationName(QString(64, QChar('a'))));
  QVERIFY(LabEffects::validAnimationName(QString(63, QChar('a'))));
}

void TestLabEffects::lookup()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString root = dir.path();
  QVERIFY(QDir(root).mkpath("one/two/three"));
  QVERIFY(writeFile(root + "/pose.bvh", "x"));
  QVERIFY(writeFile(root + "/pose.dingle", "x"));
  QVERIFY(writeFile(root + "/one/deep.avm", "x"));
  QVERIFY(writeFile(root + "/one/two/deeper.bvh", "x"));
  QVERIFY(writeFile(root + "/one/two/three/deepest.bvh", "x"));
  QVERIFY(writeFile(root + "/notes.txt", "x"));

  // .dingle before .bvh in the same folder
  QCOMPARE(LabEffects::findAnimation("pose", {root}).path, QDir(root).absoluteFilePath("pose.dingle"));
  QCOMPARE(LabEffects::findAnimation("deep", {root}).path, QDir(root).absoluteFilePath("one/deep.avm"));
  QCOMPARE(LabEffects::findAnimation("deeper", {root}).path, QDir(root).absoluteFilePath("one/two/deeper.bvh"));
  // three levels down is too deep
  QVERIFY(!LabEffects::findAnimation("deepest", {root}).ok());
  // case matters, as in Second Life; other file types are not animations
  QVERIFY(!LabEffects::findAnimation("Pose", {root}).ok());
  QVERIFY(!LabEffects::findAnimation("notes", {root}).ok());
  // folders in order: the first one with the name wins
  QCOMPARE(LabEffects::findAnimation("deeper", {root + "/one/two", root}).path,
           QDir(root).absoluteFilePath("one/two/deeper.bvh"));
  // names that try to leave the folder are refused before any lookup
  const LabEffects::Found escape = LabEffects::findAnimation("../pose", {root + "/one"});
  QVERIFY(!escape.ok());
  QCOMPARE(escape.error, QString("\"../pose\" is not a name an animation file can have."));
  const LabEffects::Found missing = LabEffects::findAnimation("dance1", {root});
  QCOMPARE(missing.error, QString("Animation \"dance1\" was not found: put dance1.dingle, dance1.bvh or dance1.avm "
                                  "next to the script or in the examples folder."));
  // the bundled example
  QCOMPARE(LabEffects::findAnimation("wave", {examples()}).path, QDir(examples()).absoluteFilePath("wave.dingle"));
}

void TestLabEffects::lookupIgnoresLinksAndDepth()
{
  QTemporaryDir outside;
  QTemporaryDir dir;
  QVERIFY(outside.isValid() && dir.isValid());
  QVERIFY(writeFile(outside.path() + "/secret.bvh", "x"));
  QVERIFY(QDir(dir.path()).mkpath(".hidden"));
  QVERIFY(writeFile(dir.path() + "/.hidden/hid.bvh", "x"));
  QVERIFY(writeFile(dir.path() + "/.dot.bvh", "x"));
  const bool linked = QFile::link(outside.path(), dir.path() + "/linked") &&
                      QFile::link(outside.path() + "/secret.bvh", dir.path() + "/secret.bvh");
  QVERIFY(!LabEffects::findAnimation("hid", {dir.path()}).ok());
  QVERIFY(!LabEffects::findAnimation(".dot", {dir.path()}).ok());
  if (!linked) QSKIP("links cannot be made here");
  QVERIFY(!LabEffects::findAnimation("secret", {dir.path()}).ok());
}

void TestLabEffects::previewOffByDefault()
{
  EffectsPreview preview;
  QVERIFY(!preview.enabled());
  // by default the folder "examples" next to the data folder, or the source tree's when the data folder is its bin/data
  const QString data = QDir::cleanPath(Constants::pinglyMotionDataPath());
  if (data.endsWith(QLatin1String("/bin/data")) && !QDir(data + "/../examples").exists())
    QCOMPARE(preview.examplesFolder(), QDir::cleanPath(data + "/../../examples"));
  else
    QCOMPARE(preview.examplesFolder(), QDir::cleanPath(data + "/../examples"));
  preview.setExamplesFolder(examples());
  preview.showRun(run(startScript("wave"), 2.5), {});
  QVERIFY(preview.animation() == nullptr);
  QVERIFY(preview.lines().isEmpty());
  QVERIFY(preview.markers().isEmpty());
  QCOMPARE(preview.frame(), -1);
}

void TestLabEffects::previewPlaysTheExample()
{
  EffectsPreview preview;
  preview.setRealTime(false);
  preview.setExamplesFolder(examples());
  preview.setEnabled(true);
  QSignalSpy changed(&preview, &EffectsPreview::changed);
  // the bundled example script: starts "wave" at 1 s; the run ends at 2.5 s
  QFile file(examples() + "/scriptlab/anim_wave.lsl");
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QJsonObject answer = run(QString::fromUtf8(file.readAll()), 2.5);
  QVERIFY(answer.value("ok").toBool());
  preview.showRun(answer, {});
  QCOMPARE(changed.count(), 1);
  QVERIFY(preview.animation() != nullptr);
  QVERIFY(!preview.hasError());
  QCOMPARE(preview.animationName(), QString("wave"));
  QCOMPARE(preview.animationPath(), QDir(examples()).absoluteFilePath("wave.dingle"));
  QCOMPARE(preview.animation()->getNumberOfFrames(), 61);
  // 1.5 s after the start at 30 frames per second: 45 frames on from frame 1
  QCOMPARE(preview.frame(), 46);
  QCOMPARE(preview.lines(), QStringList({"Playing in the 3D view: wave (started at 1.000 s)."}));
  // and on in time
  preview.advance(0.4);
  QCOMPARE(preview.frame(), 58);
  QCOMPARE(changed.count(), 2);
  preview.advance(0.1);
  QCOMPARE(preview.frame(), 1);   // past the loop end at frame 60: back to the loop start
}

void TestLabEffects::previewStopRestores()
{
  EffectsPreview preview;
  preview.setRealTime(false);
  preview.setExamplesFolder(examples());
  preview.setEnabled(true);
  preview.showRun(run(startScript("wave"), 2.5), {});
  QVERIFY(preview.animation() != nullptr);

  // a later run in which the script stops it: no preview, the 3D view shows the user's animation again
  preview.showRun(run(startScript("wave", 2.0), 3.0), {});
  QVERIFY(preview.animation() == nullptr);
  QCOMPARE(preview.frame(), -1);
  QCOMPARE(preview.lines(), QStringList({"Nothing to show: no animation, worn object or lock at the end of the run."}));

  // turning it off removes everything at once
  preview.showRun(run(startScript("wave"), 2.5), {});
  QVERIFY(preview.animation() != nullptr);
  QSignalSpy changed(&preview, &EffectsPreview::changed);
  preview.setEnabled(false);
  QCOMPARE(changed.count(), 1);
  QVERIFY(preview.animation() == nullptr);
  QVERIFY(preview.lines().isEmpty());
}

void TestLabEffects::previewUnknownName()
{
  EffectsPreview preview;
  preview.setRealTime(false);
  preview.setExamplesFolder(examples());
  preview.setEnabled(true);
  preview.showRun(run(startScript("dance1"), 2.5), {});
  QVERIFY(preview.animation() == nullptr);
  QVERIFY(preview.hasError());
  QCOMPARE(preview.lines(), QStringList({"Error: Animation \"dance1\" was not found: put dance1.dingle, dance1.bvh or "
                                         "dance1.avm next to the script or in the examples folder."}));
  // a name that tries to leave the folders
  preview.showRun(run(startScript("../wave"), 2.5), {examples() + "/scriptlab"});
  QVERIFY(preview.animation() == nullptr);
  QCOMPARE(preview.lines(), QStringList({"Error: \"../wave\" is not a name an animation file can have."}));
}

void TestLabEffects::previewMarkers()
{
  EffectsPreview preview;
  preview.setRealTime(false);
  preview.setExamplesFolder(examples());
  preview.setEnabled(true);
  const QJsonObject answer{
    {"ok", true}, {"time", 5.0}, {"log", QJsonArray()},
    {"objects", QJsonArray{QJsonObject{{"name", "hud"}, {"key", "k-hud"}, {"attached", 35}},
                           QJsonObject{{"name", "cuffs"}, {"key", "k-cuffs"}, {"attached", 5}}}},
    {"rlv", QJsonObject{{"state", QJsonObject{{"restrictions", QJsonArray{
      QJsonObject{{"object", "k-cuffs"}, {"behaviour", "detach"}},
      QJsonObject{{"object", "k-hud"}, {"behaviour", "detach"}, {"option", "neck"}}}}}}}}};
  preview.showRun(answer, {});
  QVERIFY(preview.animation() == nullptr);
  QCOMPARE(preview.markers().size(), 3);
  QCOMPARE(preview.markers().at(0).kind, EffectsPreview::Marker::Object);
  QCOMPARE(preview.markers().at(0).label, QString("cuffs"));
  QCOMPARE(preview.markers().at(0).joints, QStringList({"lHand", "lForeArm"}));
  QCOMPARE(preview.markers().at(1).kind, EffectsPreview::Marker::Lock);
  QCOMPARE(preview.markers().at(1).point, 5);
  QCOMPARE(preview.markers().at(2).kind, EffectsPreview::Marker::Lock);
  QCOMPARE(preview.markers().at(2).joints, QStringList({"neck"}));
  QCOMPARE(preview.lines(), QStringList({"Worn: hud on HUD Center (not on the avatar); cuffs on Left Hand, locked.",
                                         "Locked points: Neck."}));

  // @clear: the next run has no restrictions, and a detached object is gone
  const QJsonObject cleared{{"ok", true}, {"time", 5.0}, {"log", QJsonArray()},
                            {"objects", QJsonArray{QJsonObject{{"name", "cuffs"}, {"key", "k-cuffs"}, {"attached", 0}}}}};
  preview.showRun(cleared, {});
  QVERIFY(preview.markers().isEmpty());
}

void TestLabEffects::userAnimationUntouched()
{
  // the user's open animation, with a reader of its own as the window's scene has
  BVH userBvh;
  Animation user(&userBvh, examples() + "/jump.dingle");
  user.setFrame(3);
  user.setDirty(false);
  const int frames = user.getNumberOfFrames();

  EffectsPreview preview;
  preview.setRealTime(false);
  preview.setExamplesFolder(examples());
  preview.setEnabled(true);
  preview.showRun(run(startScript("jump"), 2.5), {});
  QVERIFY(preview.animation() != nullptr);
  QVERIFY(preview.animation() != &user);
  QCOMPARE(preview.animationPath(), QDir(examples()).absoluteFilePath("jump.dingle"));
  preview.advance(1.0);
  preview.setEnabled(false);

  QCOMPARE(user.getFrame(), 3);
  QVERIFY(!user.dirty());
  QCOMPARE(user.getNumberOfFrames(), frames);
}

void TestLabEffects::sessionAttachLockAndClear()
{
  // lsl_run with a worn object (step 5): it locks itself, starts "wave" and says @clear when told to
  RlvState rlv;
  lsl::Session session;
  const QString cuffs =
    "default {\n"
    "  state_entry() { llOwnerSay(\"@detach=n\"); llListen(1, \"\", NULL_KEY, \"\");\n"
    "                  llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION); }\n"
    "  run_time_permissions(integer p) { llStartAnimation(\"wave\"); }\n"
    "  listen(integer c, string n, key id, string m) { llOwnerSay(\"@clear\"); }\n"
    "}\n";
  EffectsPreview preview;
  preview.setRealTime(false);
  preview.setExamplesFolder(examples());
  preview.setEnabled(true);
  QObject::connect(&session, &lsl::Session::changed, &preview, [&]() { preview.showSession(&session); });

  QJsonObject a = lsl::Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1.5},
                                                {"objects", QJsonArray{QJsonObject{{"name", "cuffs"}, {"source", cuffs}, {"attached", 5}}}}},
                                    &rlv, &session);
  QVERIFY2(a.value("ok").toBool(), qPrintable(a.value("error").toString()));
  QCOMPARE(preview.animationName(), QString("wave"));
  QCOMPARE(preview.frame(), 46);   // started at 0, 1.5 s on
  QCOMPARE(preview.markers().size(), 2);
  QCOMPARE(preview.markers().at(0).kind, EffectsPreview::Marker::Object);
  QCOMPARE(preview.markers().at(0).label, QString("cuffs"));
  QCOMPARE(preview.markers().at(1).kind, EffectsPreview::Marker::Lock);
  QCOMPARE(preview.lines(), QStringList({"Playing in the 3D view: wave (started at 0.000 s).",
                                         "Worn: cuffs on Left Hand, locked."}));

  // lsl_event: chat makes it send @clear; the lock goes, the object and the animation stay
  a = lsl::Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "chat"}, {"channel", 1}, {"text", "x"}, {"seconds", 0.4}},
                          &session);
  QVERIFY2(a.value("ok").toBool(), qPrintable(a.value("error").toString()));
  QCOMPARE(preview.markers().size(), 1);
  QCOMPARE(preview.markers().at(0).kind, EffectsPreview::Marker::Object);
  QCOMPARE(preview.animationName(), QString("wave"));
  QCOMPARE(preview.frame(), 58);   // 1.9 s after the start, from the earlier command's log
  QCOMPARE(preview.lines(), QStringList({"Playing in the 3D view: wave (started at 0.000 s).", "Worn: cuffs on Left Hand."}));

  // detach: the object goes too
  a = lsl::Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "detach"}, {"seconds", 0.0}}, &session);
  QVERIFY2(a.value("ok").toBool(), qPrintable(a.value("error").toString()));
  QVERIFY(preview.markers().isEmpty());
  QCOMPARE(preview.animationName(), QString("wave"));

  // a new lsl_run is a new world: nothing carried over
  a = lsl::Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { } }"}}, &rlv, &session);
  QVERIFY(a.value("ok").toBool());
  QVERIFY(preview.animation() == nullptr);
  QVERIFY(preview.markers().isEmpty());
}

QTEST_GUILESS_MAIN(TestLabEffects)
#include "tst_labeffects.moc"
