/*
 * PinglyMotion automatic tests: the Roth2 v2 male body
 *
 * Checks bin/data/character/roth2/roth2.pmm, made by tools/make_roth2_mesh.py,
 * and that AvatarSkin shows it for a male figure (and Ruth2 or Max for a
 * female one). Run from the bin/ folder, so the program finds ./data.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>

#include "avatarskin.h"
#include "constants.h"
#include "pmmesh.h"

namespace
{

QString dataPath()
{
  return Constants::pinglyMotionDataPath();
}

QString rothFile()
{
  return dataPath() + "/character/roth2/roth2.pmm";
}

QSet<QString> skeletonSlNames()
{
  QSet<QString> names;
  QFile file(dataPath() + "/skin_joints.txt");
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

bool hasPart(const AvatarSkin& skin, const QString& name)
{
  for (const AvatarSkin::Part& part : skin.parts())
    if (part.name == name) return true;
  return false;
}

}  // namespace

class Roth2Test : public QObject
{
  Q_OBJECT

  private slots:
    void meshFile();
  void meshFileRefused();
    void noNails();
    void jointsAreInSkeleton();
    void maleFigureShowsRoth2();
    void femaleFigureKeepsChosenBody();
    void maleBodyStandsRight();
    void missingFileFallsBack();
};

// parts, vertex and triangle counts of the released Roth2 v2 mesh
// (Mesh/DAE in the Roth2 repository, see the README next to the file)
// a refused file gives no mesh and the reason (PMMesh::Result, issue 207)
void Roth2Test::meshFileRefused()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString missing = dir.filePath("missing.pmm");
  const PMMesh::Result none = PMMesh::read(missing);
  QVERIFY(!none.ok());
  QVERIFY2(none.error.startsWith("Could not open " + missing + ": "), qPrintable(none.error));

  const QString bad = dir.filePath("bad.pmm");
  QFile file(bad);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("not a mesh");
  file.close();
  const PMMesh::Result wrong = PMMesh::read(bad);
  QVERIFY(!wrong.ok());
  QCOMPARE(wrong.error, bad + " is not a valid PinglyMotion mesh file: wrong header");
}

void Roth2Test::meshFile()
{
  const PMMesh::Result read = PMMesh::read(rothFile());
  QVERIFY2(read.ok(), qPrintable(read.error));
  const PMMesh& mesh = *read.mesh;
  QCOMPARE(mesh.parts.size(), 3);

  struct Expected { const char* name; bool eye; int vertices; int triangles; };
  const Expected expected[] = {
    {"Roth2 body",      false, 16877, 33538},
    {"Roth2 eye left",  true,    145,   272},
    {"Roth2 eye right", true,    145,   272},
  };
  for (int p = 0; p < 3; p++)
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
  // the 26 collision volumes were moved to their bones, which leaves 96 joints
  QCOMPARE(mesh.joints.size(), 96);
}

// the nails of Roth2 are non-commercial and must never be in the file:
// the only parts are the body (no nail part) and the two eyes
void Roth2Test::noNails()
{
  const PMMesh::Result read = PMMesh::read(rothFile());
  QVERIFY(read.ok());
  const PMMesh& mesh = *read.mesh;
  for (const PMMesh::Part& part : mesh.parts)
    QVERIFY2(!part.name.contains("nail", Qt::CaseInsensitive), qPrintable(part.name));
}

void Roth2Test::jointsAreInSkeleton()
{
  const PMMesh::Result read = PMMesh::read(rothFile());
  QVERIFY(read.ok());
  const PMMesh& mesh = *read.mesh;
  const QSet<QString> skeleton = skeletonSlNames();
  QCOMPARE(skeleton.size(), 133);
  for (const QString& joint : mesh.joints)
    QVERIFY2(skeleton.contains(joint), qPrintable("not in the skeleton: " + joint));
  for (const char* name : {"mHandIndex3Left", "mHandThumb2Right", "mFaceJaw", "mEyeLeft", "mEyeRight",
                           "mAnkleLeft", "mWristRight", "mPelvis"})
    QVERIFY2(mesh.joints.contains(name), name);
}

// a male figure shows Roth2, a female one Ruth2; no menu choice involved
void Roth2Test::maleFigureShowsRoth2()
{
  AvatarSkin skin;
  { const AvatarSkin::Result loaded = skin.load(dataPath(), AvatarSkin::RuthBody); QVERIFY2(loaded.ok(), qPrintable(loaded.error)); }
  QVERIFY(skin.hasMaleMesh());
  QVERIFY(hasPart(skin, "Ruth2 body"));
  QVERIFY(!skin.maleMeshShown());
  const int ruthRevision = skin.restRevision();

  skin.setMale(true);
  QVERIFY(skin.maleMeshShown());
  QVERIFY(skin.restRevision() != ruthRevision);
  QVERIFY(hasPart(skin, "Roth2 body"));
  QVERIFY(!hasPart(skin, "Ruth2 body"));
  QCOMPARE(skin.parts().size(), 3);
  QCOMPARE(skin.jointNames().size(), 96);
  QCOMPARE(skin.body(), AvatarSkin::RuthBody);   // the menu choice is unchanged

  // Roth2 is male in shape already: the male skeleton scaling is not added
  for (const char* name : {"neck", "lCollar", "rCollar", "lShldr", "chest", "lThigh", "lShin"})
    QCOMPARE(skin.jointScale(name), QVector3D(1, 1, 1));
  QCOMPARE(skin.footDrop(), 0.0f);

  // and back
  skin.setMale(false);
  QVERIFY(!skin.maleMeshShown());
  QVERIFY(hasPart(skin, "Ruth2 body"));
  QCOMPARE(skin.parts().size(), 5);
  QCOMPARE(skin.jointNames().size(), 97);
  QCOMPARE(skin.jointScale("neck"), QVector3D(1, 1, 1));
}

// Max stays an option for the female figure
void Roth2Test::femaleFigureKeepsChosenBody()
{
  AvatarSkin skin;
  { const AvatarSkin::Result loaded = skin.load(dataPath(), AvatarSkin::MaxBody); QVERIFY2(loaded.ok(), qPrintable(loaded.error)); }
  QCOMPARE(skin.body(), AvatarSkin::MaxBody);
  const QStringList maxJoints = skin.jointNames();
  const int maxParts = skin.parts().size();
  QVERIFY(!hasPart(skin, "Roth2 body"));

  skin.setMale(true);
  QVERIFY(hasPart(skin, "Roth2 body"));
  skin.setMale(false);
  QVERIFY(!hasPart(skin, "Roth2 body"));
  QCOMPARE(skin.jointNames(), maxJoints);
  QCOMPARE(skin.parts().size(), maxParts);
  QCOMPARE(skin.body(), AvatarSkin::MaxBody);
}

// in the rest pose nothing moves, and the male body stands where the avatar stands
void Roth2Test::maleBodyStandsRight()
{
  AvatarSkin skin;
  QVERIFY(skin.load(dataPath(), AvatarSkin::RuthBody).ok());
  skin.setMale(true);
  skin.skin(restMatrices(skin));
  float lowest = 1e9f, highest = -1e9f, left = -1e9f, right = 1e9f;
  for (const AvatarSkin::Part& part : skin.parts())
  {
    QCOMPARE(part.influenceJoints.size(), part.restVertices.size() * AvatarSkin::INFLUENCES);
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
  QVERIFY2(left > 28.0f && left < 36.0f, qPrintable(QString::number(left)));
  QVERIFY2(right < -28.0f && right > -36.0f, qPrintable(QString::number(right)));
}

// without roth2.pmm a male figure shows the chosen body with the male skeleton
// scaling, as before
void Roth2Test::missingFileFallsBack()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(QDir().mkpath(dir.path() + "/character/ruth2"));
  QVERIFY(QFile::copy(dataPath() + "/skin_joints.txt", dir.path() + "/skin_joints.txt"));
  QVERIFY(QFile::copy(dataPath() + "/character/ruth2/ruth2.pmm", dir.path() + "/character/ruth2/ruth2.pmm"));

  AvatarSkin skin;
  { const AvatarSkin::Result loaded = skin.load(dir.path(), AvatarSkin::RuthBody); QVERIFY2(loaded.ok(), qPrintable(loaded.error)); }
  QVERIFY(!skin.hasMaleMesh());
  skin.setMale(true);
  QVERIFY(skin.isMale());
  QVERIFY(!skin.maleMeshShown());
  QVERIFY(hasPart(skin, "Ruth2 body"));
  QVERIFY(skin.jointScale("neck") != QVector3D(1, 1, 1));
}

QTEST_MAIN(Roth2Test)
#include "tst_roth2.moc"
