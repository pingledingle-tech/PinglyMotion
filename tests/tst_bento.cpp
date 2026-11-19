/*
 * PinglyMotion automatic tests
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <functional>
#include <cstring>
#include <cmath>
#include <limits>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "avatarskin.h"
#include "constants.h"
#include "releaseinfo.h"
#include "versionnumber.h"
#include "pmmesh.h"
#include "jointgroups.h"
#include "usersettings.h"
#include "propfile.h"

static const int BENTO_JOINTS = 133;

// ---------------------------------------------------------------- helpers

static void collectJoints(BVHNode* node, QList<BVHNode*>& joints)
{
  if (!node) return;
  if (node->type != BVH_END) joints << node;
  for (int i = 0; i < node->numChildren(); i++)
    collectJoints(node->child(i), joints);
}

static QList<BVHNode*> joints(BVHNode* root)
{
  QList<BVHNode*> list;
  collectJoints(root, list);
  return list;
}

static QStringList jointNames(BVHNode* root)
{
  QStringList names;
  for (BVHNode* node : joints(root)) names << node->name();
  return names;
}

static BVHNode* findJoint(BVHNode* root, const QString& name)
{
  for (BVHNode* node : joints(root))
    if (node->name() == name) return node;
  return nullptr;
}

static int uniqueCount(const QStringList& list)
{
  return QSet<QString>(list.begin(), list.end()).count();
}

static bool fuzzyEqual(const QVector3D& a, const QVector3D& b, float tolerance = 0.01f)
{
  return qAbs(a.x() - b.x()) <= tolerance &&
         qAbs(a.y() - b.y()) <= tolerance &&
         qAbs(a.z() - b.z()) <= tolerance;
}

// keep a copy of a saved file when PINGLYMOTION_EXPORT_DIR is set, so
// tools/check_sl_bvh.py can check it like Second Life would
static void keepExport(const QString& file, const QString& name)
{
  const QString dir = qEnvironmentVariable("PINGLYMOTION_EXPORT_DIR");
  if (dir.isEmpty()) return;
  QDir().mkpath(dir);
  QFile::remove(dir + "/" + name);
  QFile::copy(file, dir + "/" + name);
}

static QString vecText(const QVector3D& v)
{
  return QString("(%1, %2, %3)").arg(v.x()).arg(v.y()).arg(v.z());
}

// ---------------------------------------------------------------- tests

class BentoTest : public QObject
{
  Q_OBJECT

  private slots:
    void skeletonFiles_data();
    void skeletonFiles();
    void maleAndFemaleMatch();
    void jointList();
    void newAnimation();
    void oldFilesKeepKeyFrames_data();
    void oldFilesKeepKeyFrames();
    void saveAndLoad_data();
    void saveAndLoad();
    void everyJointHasLimits();
    void limitsAreMirrored();
    void classicLimitsFollowSlWiki();
    void elbowAndKneeBendTheRightWay();
    void headAndNeckBendFarBothWays();
    void fingersBendTowardsPalm();
    void bundledPosesInsideLimits_data();
    void bundledPosesInsideLimits();
    void leftAndRightAreMirrors();
    void posedFingerIsSaved_data();
    void posedFingerIsSaved();
    void updateCheckReadsGitHubRelease_data();
    void updateCheckReadsGitHubRelease();
    void programHasVersionNumber();
    void skinRestPose();
    void skinFollowsSkeleton();
    void skinBendsElbow();
    void skinMaleShape();
    void badFilesAreRefused_data();
    void badFilesAreRefused();
    void dataFolderIsNotTheCurrentFolder();
    void propFiles_data();
    void propFiles();
    void maxBodyLoads();
    void maxFingerBends();
    void maxJawOpens();
    void badMaxMeshIsRefused_data();
    void badMaxMeshIsRefused();
    void jointGroupNames();
    void jointGroupsDefault();
    void jointGroupsInLists();
};

void BentoTest::skeletonFiles_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("female") << Constants::femaleBvhPath();
  QTest::newRow("male") << Constants::maleBvhPath();
  QTest::newRow("default pose") << Constants::defaultPosePath();
}

// the skeleton files have all Bento joints and keep the classic names
void BentoTest::skeletonFiles()
{
  QFETCH(QString, file);
  BVH bvh;
  BVHNode* root = bvh.animRead(file, Constants::limitsFilePath());
  QVERIFY2(root, qPrintable("could not read " + file));

  const QStringList names = jointNames(root);
  QCOMPARE(names.count(), BENTO_JOINTS);
  QCOMPARE(uniqueCount(names), BENTO_JOINTS);  // no duplicates

  const QStringList classic = QString("hip abdomen chest neck head lCollar lShldr lForeArm lHand "
                                      "rCollar rShldr rForeArm rHand lThigh lShin lFoot rThigh rShin rFoot")
                                  .split(' ');
  for (const QString& name : classic)
    QVERIFY2(names.contains(name), qPrintable("missing classic joint " + name));

  const QStringList bento = QStringList() << "mHandIndex1Left" << "mFaceJaw" << "mTail6" << "mWing4Right";
  for (const QString& name : bento)
    QVERIFY2(names.contains(name), qPrintable("missing Bento joint " + name));

  QCOMPARE(root->name(), QString("hip"));
  bvh.bvhDelete(root);
}

// drawing walks the reference skeleton and the animation side by side,
// so both figures must have exactly the same structure
void BentoTest::maleAndFemaleMatch()
{
  BVH bvh;
  BVHNode* female = bvh.animRead(Constants::femaleBvhPath(), Constants::limitsFilePath());
  BVHNode* male = bvh.animRead(Constants::maleBvhPath(), Constants::limitsFilePath());
  BVHNode* pose = bvh.animRead(Constants::defaultPosePath(), Constants::limitsFilePath());

  QCOMPARE(BVH::countNodes(male), BVH::countNodes(female));
  QCOMPARE(BVH::countNodes(pose), BVH::countNodes(female));
  QCOMPARE(jointNames(male), jointNames(female));
  QCOMPARE(jointNames(pose), jointNames(female));

  bvh.bvhDelete(female);
  bvh.bvhDelete(male);
  bvh.bvhDelete(pose);
}

// bento_joints.txt lists every joint once, and every skeleton joint is in it
void BentoTest::jointList()
{
  QFile file(Constants::pinglyMotionDataPath() + "/bento_joints.txt");
  QVERIFY(file.open(QIODevice::ReadOnly));

  QStringList listed;
  while (!file.atEnd())
  {
    const QString line = QString::fromLatin1(file.readLine()).trimmed();
    if (line.isEmpty() || line.startsWith('#')) continue;
    listed << line.split(' ', Qt::SkipEmptyParts).first();
  }
  QCOMPARE(listed.count(), BENTO_JOINTS);
  QCOMPARE(uniqueCount(listed), BENTO_JOINTS);

  BVH bvh;
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), QString());
  QCOMPARE(listed, jointNames(root));
  bvh.bvhDelete(root);
}

// a new animation has all joints, each with a key frame at the start
void BentoTest::newAnimation()
{
  BVH bvh;
  Animation anim(&bvh);
  const QList<BVHNode*> list = joints(anim.getMotion());
  QCOMPARE(list.count(), BENTO_JOINTS);
  for (BVHNode* node : list)
    QVERIFY2(node->isKeyframe(0), qPrintable(node->name() + " has no key frame at frame 0"));
  QVERIFY(anim.getNumberOfFrames() > 0);
}

void BentoTest::oldFilesKeepKeyFrames_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("jump.avm") << QString(PINGLYMOTION_EXAMPLES "/jump.avm");
  QTest::newRow("throne.avm") << QString(PINGLYMOTION_EXAMPLES "/throne.avm");
  QTest::newRow("Relaxed.avm") << QString(PINGLYMOTION_LEGACY_DATA "/Relaxed.avm");
  QTest::newRow("Relaxed.bvh") << QString(PINGLYMOTION_LEGACY_DATA "/Relaxed.bvh");
  QTest::newRow("Relaxed_2.bvh") << QString(PINGLYMOTION_LEGACY_DATA "/Relaxed_2.bvh");
  QTest::newRow("TPose.bvh") << QString(PINGLYMOTION_LEGACY_DATA "/TPose.bvh");
  QTest::newRow("old TPose.avm") << QString(PINGLYMOTION_TOOLS "/legacy/TPose.avm");
  QTest::newRow("old SLFemale.bvh") << QString(PINGLYMOTION_TOOLS "/legacy/SLFemale.bvh");
}

// old 19-joint files open on the Bento skeleton without losing anything
void BentoTest::oldFilesKeepKeyFrames()
{
  QFETCH(QString, file);
  QVERIFY2(QFile::exists(file), qPrintable("missing test file " + file));

  // the file as it is, without the Bento skeleton
  BVH raw;
  BVHNode* original = raw.animRead(file, Constants::limitsFilePath());
  QVERIFY(original);
  const int originalFrames = raw.lastLoadedNumberOfFrames;

  BVH bvh;
  Animation anim(&bvh, file);
  BVHNode* motion = anim.getMotion();
  QCOMPARE(joints(motion).count(), BENTO_JOINTS);
  QCOMPARE(anim.getNumberOfFrames(), originalFrames);

  for (BVHNode* oldJoint : joints(original))
  {
    BVHNode* newJoint = findJoint(motion, oldJoint->name());
    QVERIFY2(newJoint, qPrintable(oldJoint->name() + " missing after loading"));

    for (int key : oldJoint->keyframeList())
    {
      QVERIFY2(newJoint->isKeyframe(key),
               qPrintable(QString("%1: key frame %2 lost").arg(oldJoint->name()).arg(key)));
      const QVector3D before = oldJoint->frameData(key).rotation();
      const QVector3D after = newJoint->frameData(key).rotation();
      QVERIFY2(fuzzyEqual(before, after),
               qPrintable(QString("%1 frame %2: %3 became %4").arg(oldJoint->name()).arg(key)
                              .arg(vecText(before)).arg(vecText(after))));
    }
  }
  raw.bvhDelete(original);
}

void BentoTest::saveAndLoad_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<QString>("format");
  QTest::newRow("jump as avm") << QString(PINGLYMOTION_EXAMPLES "/jump.avm") << QString("avm");
  QTest::newRow("jump as bvh") << QString(PINGLYMOTION_EXAMPLES "/jump.avm") << QString("bvh");
  QTest::newRow("throne as avm") << QString(PINGLYMOTION_EXAMPLES "/throne.avm") << QString("avm");
  QTest::newRow("throne as bvh") << QString(PINGLYMOTION_EXAMPLES "/throne.avm") << QString("bvh");
  QTest::newRow("new as bvh") << QString() << QString("bvh");
}

// saving and opening again gives the same animation, frame by frame
void BentoTest::saveAndLoad()
{
  QFETCH(QString, file);
  QFETCH(QString, format);

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString saved = dir.path() + "/saved." + format;

  BVH bvh1;
  Animation first(&bvh1, file);
  first.saveBVH(saved);
  QVERIFY2(QFileInfo(saved).size() > 0, "nothing was saved");

  BVH bvh2;
  Animation second(&bvh2, saved);

  QCOMPARE(jointNames(second.getMotion()), jointNames(first.getMotion()));
  QCOMPARE(second.getNumberOfFrames(), first.getNumberOfFrames());

  const QList<BVHNode*> a = joints(first.getMotion());
  const QList<BVHNode*> b = joints(second.getMotion());
  for (int frame = 0; frame < first.getNumberOfFrames(); frame++)
  {
    const QVector3D posA = first.getNode(0)->frameData(frame).position();
    const QVector3D posB = second.getNode(0)->frameData(frame).position();
    QVERIFY2(fuzzyEqual(posA, posB, 0.05f),
             qPrintable(QString("position frame %1: %2 became %3").arg(frame).arg(vecText(posA)).arg(vecText(posB))));

    for (int i = 0; i < a.count(); i++)
    {
      const QVector3D rotA = a[i]->frameData(frame).rotation();
      const QVector3D rotB = b[i]->frameData(frame).rotation();
      QVERIFY2(fuzzyEqual(rotA, rotB, 0.05f),
               qPrintable(QString("%1 frame %2: %3 became %4").arg(a[i]->name()).arg(frame)
                              .arg(vecText(rotA)).arg(vecText(rotB))));
    }
  }

  if (format == "bvh")
  {
    keepExport(saved, QString(QTest::currentDataTag()).replace(' ', '_') + ".bvh");

    // the exported BVH has every Bento joint
    QFile out(saved);
    QVERIFY(out.open(QIODevice::ReadOnly));
    const QString text = QString::fromLatin1(out.readAll());
    QCOMPARE(text.count(QRegularExpression("\\b(ROOT|JOINT) ")), BENTO_JOINTS);
  }
}

// every joint except the hip has a usable slider range; an axis may be
// locked (0 to 0, as on the SL wiki), but every joint can move on some axis
void BentoTest::everyJointHasLimits()
{
  BVH bvh;
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), Constants::limitsFilePath());
  for (BVHNode* node : joints(root))
  {
    if (node->type == BVH_ROOT) continue;
    bool moves = false;
    for (int i = 0; i < node->numChannels; i++)
    {
      QVERIFY2(node->channelMin[i] >= -180 && node->channelMax[i] <= 180,
               qPrintable(QString("%1 has no joint limits").arg(node->name())));
      QVERIFY2(node->channelMin[i] <= 0 && node->channelMax[i] >= 0,
               qPrintable(QString("%1 can not reach its rest pose").arg(node->name())));
      if (node->channelMin[i] < node->channelMax[i]) moves = true;
    }
    QVERIFY2(moves, qPrintable(QString("%1 can not move").arg(node->name())));
  }
  bvh.bvhDelete(root);
}

// limits of one joint: X, Y, Z as [min, max]
struct AxisLimits { double min[3]; double max[3]; };

static AxisLimits axisLimits(BVH& bvh, BVHNode* node)
{
  AxisLimits l;
  bvh.bvhGetChannelLimits(node, BVH_XROT, &l.min[0], &l.max[0]);
  bvh.bvhGetChannelLimits(node, BVH_YROT, &l.min[1], &l.max[1]);
  bvh.bvhGetChannelLimits(node, BVH_ZROT, &l.min[2], &l.max[2]);
  return l;
}

static QString otherSide(const QString& name)
{
  if (name.endsWith("Left")) return name.left(name.size() - 4) + "Right";
  if (name.endsWith("Right")) return name.left(name.size() - 5) + "Left";
  if (name.size() > 1 && name[1].isUpper() && name[0] == 'l') return "r" + name.mid(1);
  if (name.size() > 1 && name[1].isUpper() && name[0] == 'r') return "l" + name.mid(1);
  return QString();
}

// left and right joints get mirrored limits: X the same, Y and Z turned around
void BentoTest::limitsAreMirrored()
{
  BVH bvh;
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), Constants::limitsFilePath());
  int pairs = 0;
  for (BVHNode* node : joints(root))
  {
    const QString other = otherSide(node->name());
    if (other.isEmpty()) continue;
    BVHNode* mirror = findJoint(root, other);
    QVERIFY2(mirror, qPrintable(node->name() + " has no other side"));
    const AxisLimits a = axisLimits(bvh, node);
    const AxisLimits b = axisLimits(bvh, mirror);
    const QString what = node->name() + " / " + other;
    QVERIFY2(a.min[0] == b.min[0] && a.max[0] == b.max[0], qPrintable(what + ": X"));
    QVERIFY2(a.min[1] == -b.max[1] && a.max[1] == -b.min[1], qPrintable(what + ": Y"));
    QVERIFY2(a.min[2] == -b.max[2] && a.max[2] == -b.min[2], qPrintable(what + ": Z"));
    pairs++;
  }
  QCOMPARE(pairs, 100);   // every left/right joint of the 133
  bvh.bvhDelete(root);
}

// the classic joints use the table on the SL wiki page (axes the wiki locks get ±5;
// head and neck X use the AAOS cervical range instead, #129)
// "Suggested BVH Joint Rotation Limits" (see tools/make_bento_skeleton.py)
void BentoTest::classicLimitsFollowSlWiki()
{
  BVH bvh;
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), Constants::limitsFilePath());
  struct Row { const char* joint; double x0, x1, y0, y1, z0, z1; };
  const Row rows[] = {
    {"abdomen",  -45, 68,   -45, 45,   -30, 30},
    {"neck",     -45, 45,   -45, 45,   -30, 30},
    {"head",     -45, 45,   -45, 45,   -30, 30},
    {"lCollar",   -5, 5,    -30, 10,   -30, 30},
    {"lShldr",  -135, 90,  -180, 98,   -91, 97},
    {"lForeArm", -90, 79,  -146, 0,     -5, 5},
    {"lHand",    -45, 45,   -25, 36,   -90, 86},
    {"lThigh",  -155, 45,   -85, 105,  -17, 88},
    {"lShin",      0, 150,   -5, 5,     -5, 5},
    {"lFoot",    -31, 63,   -26, 26,   -74, 15},
  };
  for (const Row& row : rows)
  {
    BVHNode* node = findJoint(root, row.joint);
    QVERIFY2(node, row.joint);
    const AxisLimits l = axisLimits(bvh, node);
    const double expected[6] = {row.x0, row.x1, row.y0, row.y1, row.z0, row.z1};
    const double got[6] = {l.min[0], l.max[0], l.min[1], l.max[1], l.min[2], l.max[2]};
    for (int i = 0; i < 6; i++)
      QVERIFY2(got[i] == expected[i], qPrintable(QString("%1: %2 instead of %3")
               .arg(row.joint).arg(got[i]).arg(expected[i])));
  }
  QVERIFY(findJoint(root, "hip")->channelMax[0] > 1000);   // the hip stays free
  bvh.bvhDelete(root);
}

// where a child joint ends up when its parent turns by angle degrees around axis,
// the same way the 3D view turns it (BVH axes: X left, Y up, Z forward)
static QVector3D turned(BVHNode* child, float angle, const QVector3D& axis)
{
  QMatrix4x4 m;
  m.rotate(angle, axis);
  return m.map(QVector3D(child->offset[0], child->offset[1], child->offset[2]));
}

// the wiki's axes are PinglyMotion's axes: at the end of their range the
// elbow bends the hand forward and the knee bends the foot backward
void BentoTest::elbowAndKneeBendTheRightWay()
{
  BVH bvh;
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), Constants::limitsFilePath());
  for (const char* side : {"l", "r"})
  {
    BVHNode* elbow = findJoint(root, QString(side) + "ForeArm");
    const AxisLimits e = axisLimits(bvh, elbow);
    const float bend = std::fabs(e.min[1]) > std::fabs(e.max[1]) ? e.min[1] : e.max[1];
    QVERIFY2(turned(elbow->child(0), bend, QVector3D(0, 1, 0)).z() > 3.0f, side);

    BVHNode* knee = findJoint(root, QString(side) + "Shin");
    const AxisLimits k = axisLimits(bvh, knee);
    QVERIFY2(turned(knee->child(0), k.max[0], QVector3D(1, 0, 0)).z() < -5.0f, side);
  }
  bvh.bvhDelete(root);
}

// head and neck bend at least 45 degrees forward (positive X) and backward (negative X),
// the AAOS cervical range (#129); the top of the joint moves forward / backward
void BentoTest::headAndNeckBendFarBothWays()
{
  BVH bvh;
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), Constants::limitsFilePath());
  for (const char* name : {"neck", "head"})
  {
    BVHNode* joint = findJoint(root, name);
    QVERIFY2(joint, name);
    const AxisLimits l = axisLimits(bvh, joint);
    QVERIFY2(l.max[0] >= 45 && l.min[0] <= -45, name);
    QVERIFY2(turned(joint->child(0), l.max[0], QVector3D(1, 0, 0)).z() > 0, name);   // forward
    QVERIFY2(turned(joint->child(0), l.min[0], QVector3D(1, 0, 0)).z() < 0, name);   // backward
  }
  bvh.bvhDelete(root);
}

// fingers curl towards the palm (down in the SL rest pose) at the long end
// of their Z range, and only a little the other way
void BentoTest::fingersBendTowardsPalm()
{
  BVH bvh;
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), Constants::limitsFilePath());
  int checked = 0;
  for (const char* finger : {"Index", "Middle", "Ring", "Pinky"})
    for (const char* segment : {"1", "2", "3"})
      for (const char* side : {"Left", "Right"})
      {
        const QString name = QString("mHand%1%2%3").arg(finger, segment, side);
        BVHNode* node = findJoint(root, name);
        QVERIFY2(node && node->numChildren() > 0, qPrintable(name));
        const AxisLimits l = axisLimits(bvh, node);
        const bool left = QString(side) == "Left";
        const double bend = left ? l.min[2] : l.max[2];
        const double back = left ? l.max[2] : l.min[2];
        QVERIFY2(std::fabs(bend) >= 90 && std::fabs(back) <= 45,
                 qPrintable(QString("%1: Z %2 to %3").arg(name).arg(l.min[2]).arg(l.max[2])));
        const float restY = node->child(0)->offset[1];
        QVERIFY2(turned(node->child(0), bend, QVector3D(0, 0, 1)).y() < restY - 0.3f,
                 qPrintable(name + " does not bend towards the palm"));
        checked++;
      }
  QCOMPARE(checked, 24);
  bvh.bvhDelete(root);
}

void BentoTest::bundledPosesInsideLimits_data()
{
  QTest::addColumn<QString>("file");
  const QString data = Constants::pinglyMotionDataPath();
  for (const char* name : {"TPose.avm", "TPose.bvh", "Relaxed.avm", "Relaxed.bvh", "Relaxed_2.bvh"})
    QTest::newRow(name) << data + "/" + name;
}

// the poses that come with PinglyMotion stay inside the joint limits
void BentoTest::bundledPosesInsideLimits()
{
  QFETCH(QString, file);
  BVH bvh;
  BVHNode* root = bvh.animRead(file, Constants::limitsFilePath());
  QVERIFY(root);
  QStringList outside;   // one line per joint and axis
  QSet<QString> seen;
  const double margin = 0.5;   // degrees; values in the files are rounded
  for (BVHNode* node : joints(root))
  {
    if (node->type == BVH_ROOT) continue;
    const AxisLimits l = axisLimits(bvh, node);
    for (int k = 0; k < node->numKeyframes(); k++)
    {
      const FrameData key = node->keyframeDataByIndex(k);
      const QVector3D r = key.rotation();
      const double v[3] = {r.x(), r.y(), r.z()};
      for (int a = 0; a < 3; a++)
        if ((v[a] < l.min[a] - margin || v[a] > l.max[a] + margin) &&
            !seen.contains(node->name() + QChar('X' + a)))
        {
          seen.insert(node->name() + QChar('X' + a));
          outside << QString("%1 %2=%3 (limit %4..%5)").arg(node->name())
                       .arg(QChar('X' + a)).arg(v[a]).arg(l.min[a]).arg(l.max[a]);
        }
    }
  }
  bvh.bvhDelete(root);
  QVERIFY2(outside.isEmpty(), qPrintable(outside.join("; ")));
}

// mirror mode finds the matching joint on the other side
void BentoTest::leftAndRightAreMirrors()
{
  BVH bvh;
  Animation anim(&bvh);
  BVHNode* motion = anim.getMotion();

  const QStringList pairs = QStringList() << "lHand" << "rHand"
                                          << "lShldr" << "rShldr"
                                          << "mHandIndex1Left" << "mHandIndex1Right"
                                          << "mHandThumb3Left" << "mHandThumb3Right"
                                          << "mFaceEar2Left" << "mFaceEar2Right"
                                          << "mWing4Left" << "mWing4Right"
                                          << "mEyeLeft" << "mEyeRight";
  for (int i = 0; i < pairs.count(); i += 2)
  {
    BVHNode* a = findJoint(motion, pairs[i]);
    BVHNode* b = findJoint(motion, pairs[i + 1]);
    QVERIFY2(a && b, qPrintable("missing " + pairs[i]));
    QVERIFY2(a->getMirror() == b, qPrintable(pairs[i] + " is not mirrored to " + pairs[i + 1]));
    QVERIFY2(b->getMirror() == a, qPrintable(pairs[i + 1] + " is not mirrored to " + pairs[i]));
  }
  // joints in the middle have no mirror
  QVERIFY(findJoint(motion, "mFaceJaw")->getMirror() == nullptr);
  QVERIFY(findJoint(motion, "mTail3")->getMirror() == nullptr);
}

void BentoTest::posedFingerIsSaved_data()
{
  QTest::addColumn<QString>("format");
  QTest::newRow("bvh") << QString("bvh");
  QTest::newRow("avm") << QString("avm");
}

// a bent finger, jaw and tail keep their pose when saved and opened again
void BentoTest::posedFingerIsSaved()
{
  QFETCH(QString, format);
  QTemporaryDir dir;
  const QString saved = dir.path() + "/pose." + format;

  QHash<QString, QVector3D> pose;
  pose["mHandIndex2Left"] = QVector3D(0, 0, -45);
  pose["mHandThumb1Right"] = QVector3D(10, -20, 30);
  pose["mFaceJaw"] = QVector3D(15, 0, 0);
  pose["mTail2"] = QVector3D(0, 40, 0);

  {
    BVH bvh;
    Animation anim(&bvh);
    for (const QString& name : pose.keys())
    {
      BVHNode* node = findJoint(anim.getMotion(), name);
      QVERIFY2(node, qPrintable("missing " + name));
      node->addKeyframe(5, QVector3D(), pose[name]);
    }
    anim.saveBVH(saved);
    if (format == "bvh") keepExport(saved, "posed_bones.bvh");
  }

  BVH bvh;
  Animation anim(&bvh, saved);
  for (const QString& name : pose.keys())
  {
    const QVector3D rotation = findJoint(anim.getMotion(), name)->frameData(5).rotation();
    QVERIFY2(fuzzyEqual(rotation, pose[name]),
             qPrintable(QString("%1: saved %2, opened %3").arg(name).arg(vecText(pose[name])).arg(vecText(rotation))));
  }
}


// ------------------------------------------------ avatar mesh on the skeleton

// rest pose: every joint sits at its rest position, nothing is turned
static QVector<QMatrix4x4> restMatrices(const AvatarSkin& skin)
{
  QVector<QMatrix4x4> matrices;
  for (int i = 0; i < skin.jointNames().size(); i++)
  {
    QMatrix4x4 m;
    m.translate(skin.restPosition(i));
    matrices << m;
  }
  return matrices;
}

void BentoTest::skinRestPose()
{
  AvatarSkin skin;
  QString error;
  QVERIFY2(skin.load(Constants::pinglyMotionDataPath(), &error), qPrintable(error));
  QCOMPARE(skin.body(), AvatarSkin::RuthBody);   // the default body
  QCOMPARE(skin.parts().size(), 5);   // body, feet, eyelashes, two eyes

  // the mesh follows the 19 classic joints and the two eyes
  const QStringList names = skin.jointNames();
  for (const QString& name : QString("hip abdomen chest neck head lCollar lShldr lForeArm lHand "
                                     "rCollar rShldr rForeArm rHand lThigh lShin lFoot rThigh rShin rFoot "
                                     "mEyeLeft mEyeRight").split(' '))
    QVERIFY2(names.contains(name), qPrintable(name));

  skin.skin(restMatrices(skin));
  for (const AvatarSkin::Part& part : skin.parts())
    for (int i = 0; i < part.vertices.size(); i++)
    {
      QVERIFY2((part.vertices[i] - part.restVertices[i]).length() < 0.001f, qPrintable(part.name));
      QVERIFY2((part.normals[i] - part.restNormals[i].normalized()).length() < 0.001f, qPrintable(part.name));
    }
}

// the rest positions of the mesh are the joint positions of the skeleton file
void BentoTest::skinFollowsSkeleton()
{
  AvatarSkin skin;
  QVERIFY(skin.load(Constants::pinglyMotionDataPath()));

  BVH bvh;
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), Constants::limitsFilePath());
  QVERIFY(root);

  QHash<QString, QVector3D> positions;
  std::function<void(BVHNode*, QVector3D)> walk = [&](BVHNode* node, QVector3D parent) {
    if (node->type == BVH_END) return;
    const QVector3D here = parent + QVector3D(node->offset[0], node->offset[1], node->offset[2]);
    positions.insert(node->name(), here);
    for (int i = 0; i < node->numChildren(); i++) walk(node->child(i), here);
  };
  walk(root, QVector3D());

  for (int i = 0; i < skin.jointNames().size(); i++)
  {
    const QString name = skin.jointNames()[i];
    QVERIFY2(positions.contains(name), qPrintable(name));
    // skeleton offsets are rounded to millimetres in avatar_skeleton.xml
    const QVector3D off = positions[name] - skin.restPosition(i);
    QVERIFY2(off.length() < 0.15f, qPrintable(QString("%1 is off by %2 %3 %4").arg(name).arg(off.x()).arg(off.y()).arg(off.z())));
  }
}

// turning the left elbow moves the left hand, not the chest or the right hand
void BentoTest::skinBendsElbow()
{
  AvatarSkin skin;
  QVERIFY(skin.load(Constants::pinglyMotionDataPath()));
  const QStringList names = skin.jointNames();
  const int elbow = names.indexOf("lForeArm");
  const int hand = names.indexOf("lHand");
  const int otherHand = names.indexOf("rHand");
  const int chest = names.indexOf("chest");
  QVERIFY(elbow >= 0 && hand >= 0 && otherHand >= 0 && chest >= 0);

  QVector<QMatrix4x4> matrices = restMatrices(skin);
  // 90 degrees around the elbow; the hand hangs on the forearm
  QMatrix4x4 turned;
  turned.translate(skin.restPosition(elbow));
  turned.rotate(90, 0, 1, 0);
  matrices[elbow] = turned;
  QMatrix4x4 handMatrix = turned;
  handMatrix.translate(skin.restPosition(hand) - skin.restPosition(elbow));
  matrices[hand] = handMatrix;
  skin.skin(matrices);

  const AvatarSkin::Part* body = nullptr;
  for (const AvatarSkin::Part& part : skin.parts())
    if (part.name == "Ruth2 body") body = &part;
  QVERIFY(body);

  int moved = 0, still = 0;
  for (int i = 0; i < body->vertices.size(); i++)
  {
    // vertices that follow mostly the hand, and vertices that follow only the chest or the right hand
    const float distance = (body->vertices[i] - body->restVertices[i]).length();
    for (int k = 0; k < AvatarSkin::INFLUENCES; k++)
    {
      const float weight = body->influenceWeights[i * AvatarSkin::INFLUENCES + k];
      const int j = body->influenceJoints[i * AvatarSkin::INFLUENCES + k];
      if (j == hand && weight > 0.5f)
      {
        QVERIFY(distance > 3.0f);   // the hand is 8 inches from the elbow
        moved++;
      }
      if ((j == chest || j == otherHand) && weight > 0.999f)
      {
        QVERIFY(distance < 0.001f);
        still++;
      }
    }
  }
  QVERIFY2(moved > 20, qPrintable(QString::number(moved)));
  QVERIFY2(still > 20, qPrintable(QString::number(still)));
}


// the male figure: Ruth2 has no male shape, so only the skeleton changes
// (wider collar and shoulders, longer shins); the mesh itself stays the same
void BentoTest::skinMaleShape()
{
  AvatarSkin skin;
  QVERIFY(skin.load(Constants::pinglyMotionDataPath()));
  QVector<QVector<QVector3D>> female;
  for (const AvatarSkin::Part& part : skin.parts()) female << part.restVertices;

  skin.setMale(true);
  QVERIFY(skin.isMale());
  for (int p = 0; p < skin.parts().size(); p++)
    QCOMPARE(skin.parts()[p].restVertices, female[p]);

  // the male skeleton: wider collar and shoulders, nothing for the female
  QCOMPARE(skin.jointScale("lCollar"), QVector3D(1.4f, 1, 1));
  QCOMPARE(skin.jointScale("rShldr"), QVector3D(1.35f, 1, 1));
  QCOMPARE(skin.jointScale("neck"), QVector3D(1, 1.2f, 1));
  QCOMPARE(skin.jointScale("head"), QVector3D(1, 1, 1));
  // knee scaled 1.1 up and down: the ankle (18.44 inches below) drops 1.84
  QVERIFY(qAbs(skin.footDrop() - 1.844f) < 0.01f);

  skin.setMale(false);
  for (int p = 0; p < skin.parts().size(); p++)
    QCOMPARE(skin.parts()[p].restVertices, female[p]);
  QCOMPARE(skin.jointScale("lCollar"), QVector3D(1, 1, 1));
}

// files that crashed, hung or used gigabytes before (issues #1-#6) must be
// refused with a reason, quickly and without touching memory they do not own
void BentoTest::badFilesAreRefused_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<QByteArray>("content");

  const QByteArray head = "HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 3 Xrotation Yrotation Zrotation } ";
  const QByteArray twoFrames = head + "MOTION Frames: 2 Frame Time: 0.1 0 0 0 0 0 0 ";

  QByteArray channels = "HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 200 ";
  for (int i = 0; i < 200; i++) channels += "Xrotation ";
  channels += "} MOTION Frames: 1 Frame Time: 0.1 ";
  for (int i = 0; i < 200; i++) channels += "0 ";
  QTest::newRow("too many channels (#1)") << "channels.bvh" << channels;
  QTest::newRow("unknown channel") << "channel.bvh" << QByteArray("HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 3 Xrotation Yrotation Qrotation } MOTION Frames: 1 Frame Time: 0.1 0 0 0");

  QTest::newRow("empty file (#2)") << "empty.bvh" << QByteArray();
  QTest::newRow("only HIERARCHY (#2)") << "truncated.bvh" << QByteArray("HIERARCHY");
  QTest::newRow("no MOTION (#2)") << "nomotion.bvh" << head;
  QTest::newRow("empty AVM (#2)") << "empty.avm" << QByteArray();

  QTest::newRow("negative key frame (#3)") << "negkey.avm" << twoFrames + "1 -5";

  QTest::newRow("negative position key (#4)") << "poskey.avm" << twoFrames + "0 Properties 1 0 Positions: 3 -7 999999 5";

  QByteArray deep = "HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 3 Xrotation Yrotation Zrotation ";
  for (int i = 0; i < 20000; i++) deep += "JOINT j { OFFSET 0 0 0 CHANNELS 3 Xrotation Yrotation Zrotation ";
  for (int i = 0; i < 20001; i++) deep += "} ";
  deep += "MOTION Frames: 1 Frame Time: 0.1";
  QTest::newRow("joints nested 20000 deep (#5)") << "deep.bvh" << deep;

  QTest::newRow("20 million frames, no values (#6)") << "frames.bvh" << head + "MOTION Frames: 20000000 Frame Time: 0.1";
  QTest::newRow("fewer values than frames (#6)") << "short.bvh" << head + "MOTION Frames: 100 Frame Time: 0.1 0 0 0";
  QTest::newRow("zero frame time") << "time.bvh" << head + "MOTION Frames: 1 Frame Time: 0 0 0 0";
  QTest::newRow("not a number") << "nan.bvh" << head + "MOTION Frames: 1 Frame Time: 0.1 0 nan 0";
  QTest::newRow("wrong extension") << "pose.txt" << twoFrames;
}

void BentoTest::badFilesAreRefused()
{
  QFETCH(QString, name);
  QFETCH(QByteArray, content);

  QTemporaryDir dir;
  const QString path = dir.filePath(name);
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write(content);
  file.close();

  QElapsedTimer timer;
  timer.start();
  QString error;
  QVERIFY(!BVH::checkFile(path, &error));
  QVERIFY2(!error.isEmpty(), "a reason must be given");
  QVERIFY2(timer.elapsed() < 5000, "refusing a file must be quick");

  // the program keeps working after a refused file
  QString okError;
  QVERIFY2(BVH::checkFile(Constants::defaultPosePath(), &okError), qPrintable(okError));
}

// a data folder in the folder the program is started from must not be read (#7)
void BentoTest::dataFolderIsNotTheCurrentFolder()
{
  const QString path = Constants::pinglyMotionDataPath();
  QVERIFY2(QDir::isAbsolutePath(path), qPrintable(path));
  QVERIFY2(QFile::exists(Constants::femaleBvhPath()), qPrintable(Constants::femaleBvhPath()));

  QTemporaryDir planted;
  QVERIFY(QDir(planted.path()).mkpath("data"));
  const QString before = QDir::currentPath();
  QVERIFY(QDir::setCurrent(planted.path()));
  const QString inPlantedFolder = Constants::pinglyMotionDataPath();
  QDir::setCurrent(before);
  QCOMPARE(inPlantedFolder, path);
}

// prop files: valid lines are read, a bad line refuses the whole file (#21)
void BentoTest::propFiles_data()
{
  QTest::addColumn<QByteArray>("content");
  QTest::addColumn<bool>("valid");
  QTest::addColumn<int>("count");

  QTest::newRow("saved by PinglyMotion") << QByteArray("0 1 2 3 10 10 10 0 45 90 0\n3 -1.5 0 7 5 5 5 0 0 0 12\n") << true << 2;
  QTest::newRow("old file without attachment") << QByteArray("1 0 0 0 1 1 1 0 0 0\n") << true << 1;
  QTest::newRow("empty lines are skipped") << QByteArray("\n2 0 0 0 1 1 1 0 0 0 0\n\n") << true << 1;
  QTest::newRow("empty file") << QByteArray() << true << 0;
  QTest::newRow("unknown type") << QByteArray("9 0 0 0 1 1 1 0 0 0 0\n") << false << 0;
  QTest::newRow("negative type") << QByteArray("-1 0 0 0 1 1 1 0 0 0 0\n") << false << 0;
  QTest::newRow("not a number") << QByteArray("0 x 0 0 1 1 1 0 0 0 0\n") << false << 0;
  QTest::newRow("NaN") << QByteArray("0 nan 0 0 1 1 1 0 0 0 0\n") << false << 0;
  QTest::newRow("infinite scale") << QByteArray("0 0 0 0 inf 1 1 0 0 0 0\n") << false << 0;
  QTest::newRow("huge position") << QByteArray("0 1e300 0 0 1 1 1 0 0 0 0\n") << false << 0;
  QTest::newRow("too few values") << QByteArray("0 0 0\n") << false << 0;
  QTest::newRow("too many values") << QByteArray("0 0 0 0 1 1 1 0 0 0 0 7\n") << false << 0;
  QTest::newRow("negative attachment") << QByteArray("0 0 0 0 1 1 1 0 0 0 -3\n") << false << 0;
  QTest::newRow("bad line after good ones") << QByteArray("0 0 0 0 1 1 1 0 0 0 0\n0 0 0\n") << false << 0;

  QByteArray many;
  for (int i = 0; i <= PropFile::MAX_PROPS; i++) many += "0 0 0 0 1 1 1 0 0 0 0\n";
  QTest::newRow("too many props") << many << false << 0;
}

void BentoTest::propFiles()
{
  QFETCH(QByteArray, content);
  QFETCH(bool, valid);
  QFETCH(int, count);

  QList<PropRecord> props;
  QString error;
  QCOMPARE(PropFile::parse(content, props, &error), valid);
  QCOMPARE(error.isEmpty(), valid);
  QCOMPARE(props.size(), count);
}

// ------------------------------------------------ the Max Bento avatar

static int jointIndex(const AvatarSkin& skin, const QString& name)
{
  return skin.jointNames().indexOf(name);
}

static const AvatarSkin::Part* maxBody(const AvatarSkin& skin)
{
  for (const AvatarSkin::Part& part : skin.parts())
    if (part.name == "Max body") return &part;
  return nullptr;
}

// moves one joint (and nothing else) by a turn around its rest position
static QVector<QMatrix4x4> turnJoint(const AvatarSkin& skin, const QStringList& names, float degrees, const QVector3D& axis)
{
  QVector<QMatrix4x4> matrices = restMatrices(skin);
  for (const QString& name : names)
  {
    const int j = jointIndex(skin, name);
    QMatrix4x4 m;
    m.translate(skin.restPosition(j));
    m.rotate(degrees, axis);
    matrices[j] = m;
  }
  return matrices;
}

void BentoTest::maxBodyLoads()
{
  AvatarSkin skin;
  QString error;
  QVERIFY2(skin.load(Constants::pinglyMotionDataPath(), AvatarSkin::MaxBody, &error), qPrintable(error));
  QCOMPARE(skin.body(), AvatarSkin::MaxBody);
  QCOMPARE(skin.parts().size(), 4);   // body, eyelashes, two eyes

  // fingers and face follow their Bento bones
  for (const char* name : {"mHandIndex3Left", "mHandThumb2Right", "mFaceJaw", "mFaceLipUpperCenter",
                           "mFaceEyeLidUpperLeft", "mFaceEyebrowInnerRight", "lHand", "head", "hip"})
    QVERIFY2(jointIndex(skin, name) >= 0, name);

  // in the rest pose nothing moves, and the body stands where the SL avatar stands
  skin.skin(restMatrices(skin));
  float lowest = 1e9f, highest = -1e9f;
  for (const AvatarSkin::Part& part : skin.parts())
  {
    QCOMPARE(part.influenceJoints.size(), part.restVertices.size() * AvatarSkin::INFLUENCES);
    for (int i = 0; i < part.vertices.size(); i++)
    {
      QVERIFY2((part.vertices[i] - part.restVertices[i]).length() < 0.001f, qPrintable(part.name));
      lowest = qMin(lowest, part.vertices[i].y());
      highest = qMax(highest, part.vertices[i].y());
    }
  }
  // inches from the pelvis: soles about 42 below, top of the head about 30 above
  QVERIFY2(lowest > -44.0f && lowest < -40.0f, qPrintable(QString::number(lowest)));
  QVERIFY2(highest > 25.0f && highest < 34.0f, qPrintable(QString::number(highest)));

}

// bending the left index finger moves part of the left hand, not the right hand
void BentoTest::maxFingerBends()
{
  AvatarSkin skin;
  QVERIFY(skin.load(Constants::pinglyMotionDataPath(), AvatarSkin::MaxBody));
  const QStringList finger = {"mHandIndex1Left", "mHandIndex2Left", "mHandIndex3Left"};
  for (const QString& name : finger) QVERIFY(jointIndex(skin, name) >= 0);
  skin.skin(turnJoint(skin, QStringList{"mHandIndex1Left"}, 60, QVector3D(0, 0, 1)));

  QVERIFY(maxBody(skin));
  const AvatarSkin::Part& body = *maxBody(skin);
  const float rightHandX = skin.restPosition(jointIndex(skin, "rHand")).x();
  int moved = 0;
  for (int i = 0; i < body.vertices.size(); i++)
  {
    const float distance = (body.vertices[i] - body.restVertices[i]).length();
    if (distance > 0.3f) moved++;
    // the right hand is on the other side (BVH x is to the avatar's left)
    if (body.restVertices[i].x() < rightHandX + 3.0f) QVERIFY(distance < 0.001f);
  }
  QVERIFY2(moved > 20, qPrintable(QString::number(moved)));
}

// opening the jaw moves the vertices on the jaw, and only those
void BentoTest::maxJawOpens()
{
  AvatarSkin skin;
  QVERIFY(skin.load(Constants::pinglyMotionDataPath(), AvatarSkin::MaxBody));
  skin.skin(turnJoint(skin, QStringList{"mFaceJaw"}, 20, QVector3D(1, 0, 0)));

  const int jaw = jointIndex(skin, "mFaceJaw");
  QVERIFY(jaw >= 0);
  QVERIFY(maxBody(skin));
  const AvatarSkin::Part& body = *maxBody(skin);
  int jawMoved = 0, othersStill = 0;
  for (int i = 0; i < body.vertices.size(); i++)
  {
    float onJaw = 0.0f;
    for (int k = 0; k < AvatarSkin::INFLUENCES; k++)
      if (body.influenceJoints[i * AvatarSkin::INFLUENCES + k] == jaw)
        onJaw += body.influenceWeights[i * AvatarSkin::INFLUENCES + k];
    const float distance = (body.vertices[i] - body.restVertices[i]).length();
    if (onJaw > 0.9f && distance > 0.2f) jawMoved++;
    if (onJaw == 0.0f)
    {
      QVERIFY(distance < 0.001f);
      othersStill++;
    }
  }
  QVERIFY2(jawMoved > 20, qPrintable(QString::number(jawMoved)));
  QVERIFY(othersStill > 10000);
}

// a broken or hostile mesh file is refused whole, with a reason
void BentoTest::badMaxMeshIsRefused_data()
{
  QFile source(Constants::pinglyMotionDataPath() + "/character/max/maxine.pmm");
  QVERIFY(source.open(QIODevice::ReadOnly));
  const QByteArray good = source.readAll();
  QVERIFY(good.size() > 1000);
  const int jointCount = quint8(good[8]);
  const int partAt = 12 + jointCount * 64 + 4;              // first part header
  const int vertexAt = partAt + 64 + 12;                    // its first vertex
  const int vertexCountAt = partAt + 64 + 4;
  const int indexCountAt = partAt + 64 + 8;

  auto patched = [&good](int at, const QByteArray& bytes) { QByteArray d = good; d.replace(at, bytes.size(), bytes); return d; };
  auto u32 = [](quint32 v) { QByteArray b(4, 0); for (int i = 0; i < 4; i++) b[i] = char((v >> (8 * i)) & 0xff); return b; };
  auto f32 = [&u32](float f) { quint32 bits; std::memcpy(&bits, &f, 4); return u32(bits); };

  QTest::addColumn<QByteArray>("data");
  QTest::newRow("empty") << QByteArray();
  QTest::newRow("wrong header") << patched(0, "XXMESH01");
  QTest::newRow("cut short") << good.left(good.size() / 2);
  QTest::newRow("extra bytes") << good + QByteArray(4, 0);
  QTest::newRow("no joints") << patched(8, u32(0));
  QTest::newRow("too many joints") << patched(8, u32(100000));
  QTest::newRow("joint name without end") << patched(12, QByteArray(64, 'm'));
  QTest::newRow("huge vertex count") << patched(vertexCountAt, u32(0xffffffffu));
  QTest::newRow("index count not three") << patched(indexCountAt, u32(4));
  QTest::newRow("NaN position") << patched(vertexAt, f32(std::numeric_limits<float>::quiet_NaN()));
  QTest::newRow("far away position") << patched(vertexAt, f32(1e30f));
  QTest::newRow("joint outside list") << patched(vertexAt + 24, QByteArray(1, char(0xff)));
  QTest::newRow("negative weight") << patched(vertexAt + 28, f32(-1.0f));
  QTest::newRow("weights not 1") << patched(vertexAt + 28, f32(0.2f) + f32(0.2f) + f32(0.0f) + f32(0.0f));
  QTest::newRow("corner outside mesh") << patched(good.size() - 2, QByteArray("\xff\xff", 2));
  QTest::newRow("unknown joint") << patched(12, QByteArray("mNotABone").leftJustified(64, '\0'));
}

void BentoTest::badMaxMeshIsRefused()
{
  QFETCH(QByteArray, data);
  QTemporaryDir dir;
  QDir(dir.path()).mkpath("character/max");
  // the rest of the data folder is the real one, so only the mesh is bad
  for (const char* name : {"skin_joints.txt"})
    QVERIFY(QFile::copy(Constants::pinglyMotionDataPath() + "/" + name, dir.filePath(name)));
  QFile out(dir.filePath("character/max/maxine.pmm"));
  QVERIFY(out.open(QIODevice::WriteOnly));
  out.write(data);
  out.close();

  AvatarSkin skin;
  QString error;
  QVERIFY(!skin.load(dir.path(), AvatarSkin::MaxBody, &error));
  QVERIFY(!error.isEmpty());
  QVERIFY(!skin.isLoaded());
}

// ------------------------------------------------ joint groups

static void collectTrackNames(BVHNode* node, QStringList& names)
{
  names << node->name();
  for (int i = 0; i < node->numChildren(); i++)
    collectTrackNames(node->child(i), names);
}

// track names like AnimationView::partNames(): "position", then the skeleton
static QStringList skeletonTracks(BVH& bvh)
{
  BVHNode* root = bvh.animRead(Constants::femaleBvhPath(), QString());
  QStringList names;
  names << "position";
  if (root) collectTrackNames(root, names);
  bvh.bvhDelete(root);
  return names;
}

void BentoTest::jointGroupNames()
{
  QCOMPARE(JointGroups::groupOf("mWing2Left"), JointGroups::Wings);
  QCOMPARE(JointGroups::groupOf("mWingsRoot"), JointGroups::Wings);
  QCOMPARE(JointGroups::groupOf("mTail6"), JointGroups::Tail);
  QCOMPARE(JointGroups::groupOf("mHindLimb3Right"), JointGroups::HindLimbs);
  QCOMPARE(JointGroups::groupOf("mFaceJaw"), JointGroups::Face);
  QCOMPARE(JointGroups::groupOf("mEyeLeft"), JointGroups::Face);
  QCOMPARE(JointGroups::groupOf("mSkull"), JointGroups::Face);
  QCOMPARE(JointGroups::groupOf("mHandPinky3Left"), JointGroups::FingersToes);
  QCOMPARE(JointGroups::groupOf("mToeRight"), JointGroups::FingersToes);
  for (const char* name : {"hip", "lHand", "head", "mSpine2", "mGroin", "position", "Site"})
    QCOMPARE(JointGroups::groupOf(name), JointGroups::NoGroup);
}

// out of the box wings, tail and hind limbs are hidden; face and fingers shown
void BentoTest::jointGroupsDefault()
{
  QCOMPARE(int(JointGroups::DEFAULT_HIDDEN), JointGroups::Wings | JointGroups::Tail | JointGroups::HindLimbs);
  QCOMPARE(UserSettings::hiddenJointGroups(), int(JointGroups::DEFAULT_HIDDEN));
  QVERIFY(!UserSettings::hideJointsInLists());
  QVERIFY(!JointGroups::shown("mWing1Left"));
  QVERIFY(!JointGroups::shown("mTail1"));
  QVERIFY(!JointGroups::shown("mHindLimb1Left"));
  QVERIFY(JointGroups::shown("mFaceJaw"));
  QVERIFY(JointGroups::shown("mHandIndex1Left"));
  QVERIFY(JointGroups::shown("lHand"));
}

// dimmed: every track keeps its row; hidden: the group's tracks and their
// End Sites are left out and the other rows close up
void BentoTest::jointGroupsInLists()
{
  BVH bvh;
  const QStringList tracks = skeletonTracks(bvh);
  QVERIFY(tracks.size() > 130);
  const int savedGroups = UserSettings::hiddenJointGroups();
  const bool savedLists = UserSettings::hideJointsInLists();

  UserSettings::setHideJointsInLists(false);
  JointGroups::setTracks(tracks);
  QCOMPARE(JointGroups::rowCount(), tracks.size());
  int dimmed = 0;
  for (int t = 0; t < tracks.size(); t++)
  {
    QCOMPARE(JointGroups::rowOfTrack(t), t);
    QCOMPARE(JointGroups::trackOfRow(t), t);
    if (JointGroups::trackDimmed(t)) dimmed++;
  }
  QVERIFY(dimmed > 20);
  QVERIFY(JointGroups::trackDimmed(tracks.indexOf("mWing1Left")));
  QVERIFY(!JointGroups::trackDimmed(tracks.indexOf("lHand")));

  UserSettings::setHideJointsInLists(true);
  JointGroups::update();
  QCOMPARE(JointGroups::rowCount(), tracks.size() - dimmed);
  for (int t = 0; t < tracks.size(); t++)
  {
    const int row = JointGroups::rowOfTrack(t);
    const QString& name = tracks[t];
    if (name != "Site") QCOMPARE(row < 0, !JointGroups::shown(name));
    if (row >= 0) QCOMPARE(JointGroups::trackOfRow(row), t);
    QVERIFY(!JointGroups::trackDimmed(t));
  }
  QCOMPARE(JointGroups::trackOfRow(JointGroups::rowCount()), -1);
  // the End Site after the last wing bone goes with it
  const int wingTip = tracks.indexOf("mWing4FanLeft");
  QVERIFY(wingTip > 0 && tracks[wingTip + 1] == "Site");
  QCOMPARE(JointGroups::rowOfTrack(wingTip + 1), -1);

  // all groups shown: nothing is left out
  UserSettings::setHiddenJointGroups(0);
  JointGroups::update();
  QCOMPARE(JointGroups::rowCount(), tracks.size());

  UserSettings::setHiddenJointGroups(savedGroups);
  UserSettings::setHideJointsInLists(savedLists);
  JointGroups::setTracks(QStringList());
}

QTEST_MAIN(BentoTest)
// the update check reads the version from GitHub's "latest release" answer
void BentoTest::updateCheckReadsGitHubRelease_data()
{
  QTest::addColumn<QByteArray>("json");
  QTest::addColumn<QString>("version");

  QTest::newRow("normal release") << QByteArray(R"({"tag_name":"v0.3.0","draft":false,"prerelease":false})") << "0.3.0";
  QTest::newRow("tag without v") << QByteArray(R"({"tag_name":"1.2.10"})") << "1.2.10";
  QTest::newRow("pre-release is ignored") << QByteArray(R"({"tag_name":"v0.3.0","prerelease":true})") << "";
  QTest::newRow("draft is ignored") << QByteArray(R"({"tag_name":"v0.3.0","draft":true})") << "";
  QTest::newRow("no releases yet") << QByteArray(R"({"message":"Not Found","status":"404"})") << "";
  QTest::newRow("odd tag name") << QByteArray(R"({"tag_name":"nightly"})") << "";
  QTest::newRow("not JSON") << QByteArray("<html>error</html>") << "";
  QTest::newRow("empty answer") << QByteArray() << "";
}

void BentoTest::updateCheckReadsGitHubRelease()
{
  QFETCH(QByteArray, json);
  QFETCH(QString, version);
  QCOMPARE(ReleaseInfo::latestVersion(json), version);
}

// a build without a real version number would never see an update as "newer"
void BentoTest::programHasVersionNumber()
{
  QVERIFY(Constants::versionNumber().isValid());
  QVERIFY(Constants::versionNumber().compare(VersionNumber("0.0.0")) > 0);
  QVERIFY(VersionNumber("0.10.0").compare(VersionNumber("0.9.3")) > 0);
}

#include "tst_bento.moc"
