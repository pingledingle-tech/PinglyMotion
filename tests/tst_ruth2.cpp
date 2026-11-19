/*
 * Pingmator automatic tests: the Ruth2 v4 body (#110)
 *
 * Checks bin/data/character/ruth2/ruth2.pmm, made by tools/make_ruth2_mesh.py,
 * and that AvatarSkin loads it as RuthBody (also through PINGLYMOTION_BODY=ruth2).
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QFile>
#include <QSet>
#include <QTextStream>

#include "avatarskin.h"
#include "constants.h"
#include "pmmesh.h"

namespace
{

QString ruthFile()
{
  return Constants::pingmatorDataPath() + "/character/ruth2/ruth2.pmm";
}

// SL names of every joint in Pingmator's skeleton (bin/data/skin_joints.txt)
QSet<QString> skeletonSlNames()
{
  QSet<QString> names;
  QFile file(Constants::pingmatorDataPath() + "/skin_joints.txt");
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return names;
  QTextStream in(&file);
  while (!in.atEnd())
  {
    const QStringList fields = in.readLine().simplified().split(' ', Qt::SkipEmptyParts);
    if (fields.size() < 2 || fields[0].startsWith('#')) continue;
    names.insert(fields[1]);
  }
  return names;
}

QVector<QMatrix4x4> restMatrices(const AvatarSkin& skin)
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

QVector<QMatrix4x4> turnJoint(const AvatarSkin& skin, const QString& name, float degrees, const QVector3D& axis)
{
  QVector<QMatrix4x4> matrices = restMatrices(skin);
  const int j = skin.jointNames().indexOf(name);
  if (j < 0) return matrices;
  QMatrix4x4 m;
  m.translate(skin.restPosition(j));
  m.rotate(degrees, axis);
  matrices[j] = m;
  return matrices;
}

const AvatarSkin::Part* partNamed(const AvatarSkin& skin, const QString& name)
{
  for (const AvatarSkin::Part& part : skin.parts())
    if (part.name == name) return &part;
  return nullptr;
}

// how much of a vertex follows one joint
float weightOn(const AvatarSkin::Part& part, int vertex, int joint)
{
  float w = 0.0f;
  for (int k = 0; k < AvatarSkin::INFLUENCES; k++)
    if (part.influenceJoints[vertex * AvatarSkin::INFLUENCES + k] == joint)
      w += part.influenceWeights[vertex * AvatarSkin::INFLUENCES + k];
  return w;
}

// to run a test with PINGLYMOTION_BODY set, and leave the environment as it was
class BodyVariable
{
  public:
    explicit BodyVariable(const QByteArray& value) : m_had(qEnvironmentVariableIsSet("PINGLYMOTION_BODY")),
                                                     m_old(qgetenv("PINGLYMOTION_BODY"))
    {
      if (value.isNull()) qunsetenv("PINGLYMOTION_BODY");
      else qputenv("PINGLYMOTION_BODY", value);
    }
    ~BodyVariable()
    {
      if (m_had) qputenv("PINGLYMOTION_BODY", m_old);
      else qunsetenv("PINGLYMOTION_BODY");
    }
  private:
    bool m_had;
    QByteArray m_old;
};

}  // namespace

class Ruth2Test : public QObject
{
  Q_OBJECT

  private slots:
    void init() { qunsetenv("PINGLYMOTION_BODY"); }
    void meshFile();
    void jointsAreInSkeleton();
    void bodyLoads();
    void eyesInHead();
    void fingerBends();
    void jawOpens();
    void environmentSelectsRuth();
};

// parts, vertex and triangle counts of the released Ruth2 v4 mesh
// (Mesh/Ruth2_v4/DAE in the Ruth2 repository, see the README next to the file)
void Ruth2Test::meshFile()
{
  PMMesh mesh;
  QString error;
  QVERIFY2(mesh.load(ruthFile(), &error), qPrintable(error));
  QCOMPARE(mesh.parts.size(), 5);

  struct Expected { const char* name; bool eye; int vertices; int triangles; };
  const Expected expected[] = {
    {"Ruth2 body",      false, 10738, 21306},
    {"Ruth2 feet",      false,  2018,  3988},
    {"Ruth2 eyelashes", false,   102,   120},
    {"Ruth2 eye left",  true,    145,   272},
    {"Ruth2 eye right", true,    145,   272},
  };
  for (int p = 0; p < 5; p++)
  {
    const PMMesh::Part& part = mesh.parts[p];
    QCOMPARE(part.name, QString(expected[p].name));
    QCOMPARE(part.eye, expected[p].eye);
    QCOMPARE(part.vertices.size(), expected[p].vertices);
    QCOMPARE(part.indices.size(), expected[p].triangles * 3);
    for (int v = 0; v < part.vertices.size(); v++)
    {
      float sum = 0.0f;
      for (int k = 0; k < PMMesh::INFLUENCES; k++) sum += part.weights[v * PMMesh::INFLUENCES + k];
      QVERIFY2(qAbs(sum - 1.0f) < 0.001f, qPrintable(QString("%1 vertex %2").arg(part.name).arg(v)));
    }
  }
  // 103 joints in the body file plus the feet's ankles and feet; the 23
  // collision volumes were moved to their bones, which leaves 97 bones
  QCOMPARE(mesh.joints.size(), 97);
}

// every joint a vertex is weighted to is a joint of Pingmator's skeleton
void Ruth2Test::jointsAreInSkeleton()
{
  PMMesh mesh;
  QVERIFY(mesh.load(ruthFile()));
  const QSet<QString> skeleton = skeletonSlNames();
  QCOMPARE(skeleton.size(), 133);

  QSet<int> weighted;
  for (const PMMesh::Part& part : mesh.parts)
    for (int i = 0; i < part.weights.size(); i++)
      if (part.weights[i] > 0.0f) weighted.insert(part.joints[i]);
  for (int j : weighted)
    QVERIFY2(skeleton.contains(mesh.joints[j]), qPrintable("not in the skeleton: " + mesh.joints[j]));

  // the Bento parts are there: fingers, face, eyes
  for (const char* name : {"mHandIndex3Left", "mHandThumb2Right", "mFaceJaw", "mFaceLipUpperCenter",
                           "mFaceEyeLidUpperLeft", "mFaceEyebrowInnerRight", "mEyeLeft", "mEyeRight",
                           "mAnkleLeft", "mWristRight", "mPelvis"})
  {
    const int j = mesh.joints.indexOf(name);
    QVERIFY2(j >= 0 && weighted.contains(j), name);
  }
}

void Ruth2Test::bodyLoads()
{
  AvatarSkin skin;
  QString error;
  QVERIFY2(skin.load(Constants::pingmatorDataPath(), AvatarSkin::RuthBody, &error), qPrintable(error));
  QCOMPARE(skin.body(), AvatarSkin::RuthBody);
  QCOMPARE(skin.parts().size(), 5);
  QCOMPARE(skin.jointNames().size(), 97);
  for (const char* name : {"hip", "head", "lHand", "rFoot", "mHandIndex3Left", "mFaceJaw"})
    QVERIFY2(skin.jointNames().contains(name), name);

  // in the rest pose nothing moves, and the body stands where the SL avatar stands
  skin.skin(restMatrices(skin));
  float lowest = 1e9f, highest = -1e9f, left = -1e9f, right = 1e9f;
  for (const AvatarSkin::Part& part : skin.parts())
  {
    QCOMPARE(part.influenceJoints.size(), part.restVertices.size() * AvatarSkin::INFLUENCES);
    QCOMPARE(part.triangleJoint.size(), part.indices.size() / 3);
    for (int i = 0; i < part.vertices.size(); i++)
    {
      QVERIFY2((part.vertices[i] - part.restVertices[i]).length() < 0.001f, qPrintable(part.name));
      lowest = qMin(lowest, part.vertices[i].y());
      highest = qMax(highest, part.vertices[i].y());
      left = qMax(left, part.vertices[i].x());
      right = qMin(right, part.vertices[i].x());
    }
  }
  // inches from the pelvis: soles about 42 below, top of the head about 30 above
  QVERIFY2(lowest > -44.0f && lowest < -39.0f, qPrintable(QString::number(lowest)));
  QVERIFY2(highest > 25.0f && highest < 34.0f, qPrintable(QString::number(highest)));
  // T pose: the fingertips reach a bit past the SL wrist (about 25 inches out)
  QVERIFY2(left > 28.0f && left < 36.0f, qPrintable(QString::number(left)));
  QVERIFY2(right < -28.0f && right > -36.0f, qPrintable(QString::number(right)));

  // switching back gives the system avatar again
  QVERIFY(skin.load(Constants::pingmatorDataPath(), AvatarSkin::ClassicBody, &error));
  QCOMPARE(skin.parts().size(), 6);
}

// the eyeballs were moved to the SL skeleton with the head: each is a cap in
// front of its eye joint (the joint is the centre of the whole eyeball), so
// turning the eye joint turns the eyeball in place
void Ruth2Test::eyesInHead()
{
  AvatarSkin skin;
  QVERIFY(skin.load(Constants::pingmatorDataPath(), AvatarSkin::RuthBody));
  const struct { const char* part; const char* joint; } eyes[] = {
    {"Ruth2 eye left", "mEyeLeft"}, {"Ruth2 eye right", "mEyeRight"}};
  for (const auto& eye : eyes)
  {
    const AvatarSkin::Part* part = partNamed(skin, eye.part);
    QVERIFY(part);
    QVector3D centre;
    for (const QVector3D& v : part->restVertices) centre += v;
    centre /= part->restVertices.size();
    const QVector3D joint = skin.restPosition(skin.jointNames().indexOf(eye.joint));
    // BVH axes: x to the side, y up, z forward; inches
    const QVector3D off = centre - joint;
    QVERIFY2(qAbs(off.x()) < 0.1f && qAbs(off.y()) < 0.1f && off.z() > 0.2f && off.z() < 0.75f,
             qPrintable(QString("%1 centre is %2 %3 %4 inches from its joint")
                        .arg(eye.part).arg(off.x()).arg(off.y()).arg(off.z())));
  }
}

// bending the left index finger moves part of the left hand, not the right hand
void Ruth2Test::fingerBends()
{
  AvatarSkin skin;
  QVERIFY(skin.load(Constants::pingmatorDataPath(), AvatarSkin::RuthBody));
  skin.skin(turnJoint(skin, "mHandIndex1Left", 60, QVector3D(0, 0, 1)));
  const AvatarSkin::Part* body = partNamed(skin, "Ruth2 body");
  QVERIFY(body);
  const float rightHandX = skin.restPosition(skin.jointNames().indexOf("rHand")).x();
  int moved = 0;
  for (int i = 0; i < body->vertices.size(); i++)
  {
    const float distance = (body->vertices[i] - body->restVertices[i]).length();
    if (distance > 0.3f) moved++;
    if (body->restVertices[i].x() < rightHandX + 3.0f) QVERIFY(distance < 0.001f);
  }
  QVERIFY2(moved > 20, qPrintable(QString::number(moved)));
}

// opening the jaw moves the lower face, and only that. Ruth2 weights the
// lower face mostly to the jaw's child bones (chin, lower lip, lower teeth,
// tongue), so they turn with the jaw, as they do in the skeleton
void Ruth2Test::jawOpens()
{
  AvatarSkin skin;
  QVERIFY(skin.load(Constants::pingmatorDataPath(), AvatarSkin::RuthBody));
  const int jaw = skin.jointNames().indexOf("mFaceJaw");
  QVERIFY(jaw >= 0);
  QMatrix4x4 turn;   // 20 degrees around the jaw joint
  turn.translate(skin.restPosition(jaw));
  turn.rotate(20, QVector3D(1, 0, 0));
  turn.translate(-skin.restPosition(jaw));

  const QStringList lowerFace = {"mFaceJaw", "mFaceChin", "mFaceTeethLower", "mFaceLipLowerLeft",
                                 "mFaceLipLowerRight", "mFaceLipLowerCenter", "mFaceTongueBase", "mFaceTongueTip"};
  QVector<int> moving;
  QVector<QMatrix4x4> matrices = restMatrices(skin);
  for (const QString& name : lowerFace)
  {
    const int j = skin.jointNames().indexOf(name);
    QVERIFY2(j >= 0, qPrintable(name));
    matrices[j] = turn * matrices[j];
    moving << j;
  }
  skin.skin(matrices);

  const AvatarSkin::Part* body = partNamed(skin, "Ruth2 body");
  QVERIFY(body);
  int faceMoved = 0, othersStill = 0;
  for (int i = 0; i < body->vertices.size(); i++)
  {
    float onLowerFace = 0.0f;
    for (int j : moving) onLowerFace += weightOn(*body, i, j);
    const float distance = (body->vertices[i] - body->restVertices[i]).length();
    if (onLowerFace > 0.5f && distance > 0.1f) faceMoved++;
    if (onLowerFace == 0.0f)
    {
      QVERIFY(distance < 0.001f);
      othersStill++;
    }
  }
  QVERIFY2(faceMoved > 100, qPrintable(QString::number(faceMoved)));
  QVERIFY(othersStill > 9000);
}

// PINGLYMOTION_BODY=ruth2 picks Ruth2 whatever body is asked for (testing only,
// until part 2 of #110 adds a menu entry); other values change nothing
void Ruth2Test::environmentSelectsRuth()
{
  {
    BodyVariable var("ruth2");
    AvatarSkin skin;
    QVERIFY(skin.load(Constants::pingmatorDataPath()));
    QCOMPARE(skin.body(), AvatarSkin::RuthBody);
    QVERIFY(skin.load(Constants::pingmatorDataPath(), AvatarSkin::MaxBody));
    QCOMPARE(skin.body(), AvatarSkin::RuthBody);
  }
  {
    BodyVariable var(" Ruth2 ");
    QCOMPARE(AvatarSkin::bodyOverride(), int(AvatarSkin::RuthBody));
  }
  {
    BodyVariable var("something else");
    QCOMPARE(AvatarSkin::bodyOverride(), -1);
    AvatarSkin skin;
    QVERIFY(skin.load(Constants::pingmatorDataPath(), AvatarSkin::MaxBody));
    QCOMPARE(skin.body(), AvatarSkin::MaxBody);
  }
  {
    BodyVariable var{QByteArray()};
    QCOMPARE(AvatarSkin::bodyOverride(), -1);
    AvatarSkin skin;
    QVERIFY(skin.load(Constants::pingmatorDataPath()));
    QCOMPARE(skin.body(), AvatarSkin::ClassicBody);
  }
}

QTEST_MAIN(Ruth2Test)
#include "tst_ruth2.moc"
