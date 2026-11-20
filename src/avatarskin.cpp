/*
 * PinglyMotion - the avatar body (Ruth2 or Max), bent with the skeleton
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QTextStream>

#include <cmath>
#include <utility>

#include "avatarskin.h"
#include "pmmesh.h"

static const float INCH = 39.3700787f;      // metres to BVH units
static const float PELVIS_HEIGHT = 1.067015f; // mPelvis above the feet, metres

static bool fail(QString* errorMessage, const QString& text)
{
  if (errorMessage) *errorMessage = text;
  return false;
}

// SL (x forward, y left, z up) -> BVH (x left, y up, z forward)
static QVector3D slToBvh(const QVector3D& v)
{
  return QVector3D(v.y(), v.z(), v.x());
}

bool AvatarSkin::load(const QString& dataPath, Body body, QString* errorMessage)
{
  m_skeleton.clear();
  m_skeletonBySlName.clear();
  m_jointNames.clear();
  m_restPositions.clear();
  m_usedJoints.clear();
  m_parts.clear();
  m_male = false;
  m_rothShown = false;
  m_roth = MeshState();
  m_body = body;
  m_maleScales.clear();
  m_maleShifts.clear();
  m_restRevision++;

  if (!loadJoints(dataPath + "/skin_joints.txt", errorMessage)) return false;

  // Male_Skeleton (param 32 in avatar_lad.xml of the Second Life viewer):
  // scale added to the joint, SL axes (x forward, y left, z up)
  struct Change { const char* joint; float sx, sy, sz; float px, py, pz; };
  static const Change maleSkeleton[] = {
    {"mNeck",           0,    0,    0.2f,  0, 0, 0},
    {"mCollarLeft",     0,    0.4f, 0,     0, 0, 0},
    {"mCollarRight",    0,    0.4f, 0,     0, 0, 0},
    {"mShoulderLeft",   0,    0.35f,0,     0, 0, 0},
    {"mShoulderRight",  0,    0.35f,0,     0, 0, 0},
    {"mElbowLeft",      0,    0.1f, 0,     0, 0, 0},
    {"mElbowRight",     0,    0.1f, 0,     0, 0, 0},
    {"mChest",          0.05f,0.05f,0.05f, 0, 0, 0},
    {"mTorso",          0,    0,    0.05f, 0, 0, 0},
    {"mSpine3",         0,    0,    0.05f, 0, 0, 0},
    {"mSpine4",         0,    0,    0.05f, 0, 0, 0},
    {"mHipLeft",        0.05f,0.05f,0,     0, 0, 0},
    {"mHipRight",       0.05f,0.05f,0,     0, 0, 0},
    {"mKneeLeft",       0.05f,0.05f,0.1f,  0, 0, 0},
    {"mKneeRight",      0.05f,0.05f,0.1f,  0, 0, 0},
    {"mHindLimb1Left",  0.05f,0.05f,0,     0, 0, 0},
    {"mHindLimb1Right", 0.05f,0.05f,0,     0, 0, 0},
    {"mHindLimb2Left",  0.05f,0.05f,0.1f,  0, 0, 0},
    {"mHindLimb2Right", 0.05f,0.05f,0.1f,  0, 0, 0},
    {"mWing1Left",      0,    0,    0,     -0.05f, 0, 0},
    {"mWing1Right",     0,    0,    0,     -0.05f, 0, 0},
  };
  for (const Change& change : maleSkeleton)
  {
    const int joint = m_skeletonBySlName.value(change.joint, -1);
    if (joint < 0) continue;
    const QString& name = m_skeleton[joint].fileName;
    const QVector3D scale = slToBvh(QVector3D(change.sx, change.sy, change.sz));
    const QVector3D shift = slToBvh(QVector3D(change.px, change.py, change.pz)) * INCH;
    if (!scale.isNull()) m_maleScales.insert(name, QVector3D(1, 1, 1) + scale);
    if (!shift.isNull()) m_maleShifts.insert(name, shift);
  }

  const QString folder = dataPath + "/character/";
  const QString file = body == MaxBody ? "max/maxine.pmm" : "ruth2/ruth2.pmm";
  if (!loadPmm(folder + file, errorMessage))
  {
    m_parts.clear();
    return false;
  }
  m_jointScales.fill(QVector3D(1, 1, 1), m_jointNames.size());
  // the male figure's body; a missing or damaged file only means the male
  // figure falls back to the body above
  loadMaleMesh(folder + "roth2/roth2.pmm");
  return true;
}

void AvatarSkin::swapMesh(MeshState& other)
{
  std::swap(m_jointNames, other.jointNames);
  std::swap(m_restPositions, other.restPositions);
  std::swap(m_usedJoints, other.usedJoints);
  std::swap(m_parts, other.parts);
  std::swap(m_jointScales, other.jointScales);
}

void AvatarSkin::loadMaleMesh(const QString& fileName)
{
  // load into the members, then move the result out of the way
  MeshState chosen;
  swapMesh(chosen);   // chosen body away, members empty
  QString error;
  const bool ok = loadPmm(fileName, &error);
  if (ok)
    m_jointScales.fill(QVector3D(1, 1, 1), m_jointNames.size());
  else
    qWarning("AvatarSkin: male body not loaded: %s", error.toLatin1().constData());
  swapMesh(m_roth);   // Roth2 (or nothing) into m_roth, members empty again
  swapMesh(chosen);   // chosen body back
  m_roth.loaded = ok;
  if (!ok) m_roth = MeshState();
}

bool AvatarSkin::loadJoints(const QString& fileName, QString* errorMessage)
{
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return fail(errorMessage, QString("Could not open %1: %2").arg(fileName, file.errorString()));

  QTextStream in(&file);
  while (!in.atEnd())
  {
    const QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith('#')) continue;
    const QStringList fields = line.split(' ', Qt::SkipEmptyParts);
    if (fields.size() != 7)
      return fail(errorMessage, QString("%1: bad line '%2'").arg(fileName, line));

    SkinJoint joint;
    joint.fileName = fields[0];
    joint.slName = fields[1];
    joint.parent = m_skeletonBySlName.value(fields[2], -1);
    joint.base = fields[3] == "base";
    joint.rest = QVector3D(fields[4].toFloat(), fields[5].toFloat(), fields[6].toFloat());
    if (joint.parent < 0 && fields[2] != "-")
      return fail(errorMessage, QString("%1: joint %2 comes before its parent").arg(fileName, joint.slName));

    const int index = m_skeleton.size();
    if (joint.parent >= 0) m_skeleton[joint.parent].children << index;
    m_skeletonBySlName.insert(joint.slName, index);
    m_skeleton << joint;
  }
  if (m_skeleton.isEmpty())
    return fail(errorMessage, QString("%1 has no joints").arg(fileName));
  return true;
}

int AvatarSkin::useJoint(int skeletonJoint)
{
  auto found = m_usedJoints.constFind(skeletonJoint);
  if (found != m_usedJoints.constEnd()) return found.value();
  const int index = m_jointNames.size();
  m_jointNames << m_skeleton[skeletonJoint].fileName;
  m_restPositions << m_skeleton[skeletonJoint].rest;
  m_usedJoints.insert(skeletonJoint, index);
  return index;
}

// a Bento mesh body (Max, Ruth2): up to four joints per vertex, straight from the file
bool AvatarSkin::loadPmm(const QString& fileName, QString* errorMessage)
{
  const PMMesh::Result read = PMMesh::read(fileName);
  if (!read.ok())
  {
    if (errorMessage) *errorMessage = read.error;
    return false;
  }
  const PMMesh& mesh = *read.mesh;
  static_assert(PMMesh::INFLUENCES == INFLUENCES, "influence counts must match");

  QVector<int> used;   // file joint -> index in m_jointNames
  for (const QString& name : mesh.joints)
  {
    const int skeletonJoint = m_skeletonBySlName.value(name, -1);
    if (skeletonJoint < 0)
      return fail(errorMessage, QString("%1: joint %2 is not in the skeleton").arg(fileName, name));
    used << useJoint(skeletonJoint);
  }

  const QVector3D pelvis(0, 0, PELVIS_HEIGHT);
  for (const PMMesh::Part& source : mesh.parts)
  {
    Part part;
    part.name = source.name;
    part.eye = source.eye;
    const int count = source.vertices.size();
    for (int i = 0; i < count; i++)
    {
      part.restVertices << slToBvh(source.vertices[i] - pelvis) * INCH;
      part.restNormals << slToBvh(source.normals[i]);
      // PMMesh::load() checked every joint index against the joint list
      int strongest = 0, second = -1;
      for (int k = 0; k < INFLUENCES; k++)
      {
        const int j = i * INFLUENCES + k;
        part.influenceJoints << used[source.joints[j]];
        part.influenceWeights << source.weights[j];
        if (k == 0) continue;
        const float w = source.weights[j];
        if (w > source.weights[i * INFLUENCES + strongest]) { second = strongest; strongest = k; }
        else if (second < 0 || w > source.weights[i * INFLUENCES + second]) second = k;
      }
      // the two strongest joints, for the colour of selected parts
      const float wa = source.weights[i * INFLUENCES + strongest];
      const float wb = second >= 0 ? source.weights[i * INFLUENCES + second] : 0.0f;
      part.jointA << used[source.joints[i * INFLUENCES + strongest]];
      part.jointB << (second >= 0 ? used[source.joints[i * INFLUENCES + second]] : part.jointA.last());
      part.blend << (wa + wb > 0.0f ? wb / (wa + wb) : 0.0f);
    }
    part.maleOffsets.fill(QVector3D(), count);
    part.maleNormalOffsets.fill(QVector3D(), count);
    part.femaleVertices = part.restVertices;
    part.femaleNormals = part.restNormals;
    part.indices = source.indices;
    for (int t = 0; t < part.indices.size() / 3; t++)
    {
      // the joint with most pull on the three corners
      QHash<int,float> pull;
      for (int corner = 0; corner < 3; corner++)
      {
        const int v = part.indices[t * 3 + corner];
        for (int k = 0; k < INFLUENCES; k++)
          pull[part.influenceJoints[v * INFLUENCES + k]] += part.influenceWeights[v * INFLUENCES + k];
      }
      int best = part.influenceJoints[part.indices[t * 3] * INFLUENCES];
      for (auto it = pull.constBegin(); it != pull.constEnd(); ++it)
        if (it.value() > pull.value(best) || (it.value() == pull.value(best) && it.key() < best)) best = it.key();
      part.triangleJoint << best;
    }
    part.vertices = part.restVertices;
    part.normals = part.restNormals;
    m_parts << part;
  }
  return true;
}

QVector3D AvatarSkin::jointScale(const QString& name) const
{
  return m_male && !m_rothShown ? m_maleScales.value(name, QVector3D(1, 1, 1)) : QVector3D(1, 1, 1);
}

QVector3D AvatarSkin::jointShift(const QString& name) const
{
  return m_male && !m_rothShown ? m_maleShifts.value(name) : QVector3D();
}

float AvatarSkin::footDrop() const
{
  if (!m_male || m_rothShown) return 0.0f;
  float drop = 0.0f;
  int joint = m_skeletonBySlName.value("mAnkleLeft", -1);
  while (joint >= 0 && m_skeleton[joint].parent >= 0)
  {
    const SkinJoint& child = m_skeleton[joint];
    const SkinJoint& parent = m_skeleton[child.parent];
    const float offset = child.rest.y() - parent.rest.y();
    drop -= offset * (jointScale(parent.fileName).y() - 1.0f);
    joint = child.parent;
  }
  return drop;
}

void AvatarSkin::setMale(bool male)
{
  if (male == m_male) return;
  m_male = male;
  m_restRevision++;
  if (m_roth.loaded && male != m_rothShown)
  {
    // the male figure uses the Roth2 mesh, the female one the chosen body
    swapMesh(m_roth);
    m_rothShown = male;
  }
  m_jointScales.resize(m_jointNames.size());
  for (int j = 0; j < m_jointNames.size(); j++)
    m_jointScales[j] = jointScale(m_jointNames[j]);
  for (Part& part : m_parts)
  {
    for (int i = 0; i < part.restVertices.size(); i++)
    {
      if (male)
      {
        part.restVertices[i] = part.femaleVertices[i] + part.maleOffsets[i];
        part.restNormals[i] = (part.femaleNormals[i] + part.maleNormalOffsets[i]).normalized();
      }
      else
      {
        part.restVertices[i] = part.femaleVertices[i];
        part.restNormals[i] = part.femaleNormals[i];
      }
    }
  }
}

void AvatarSkin::skinMatrices(const QVector<QMatrix4x4>& jointMatrices,
                              QVector<QMatrix4x4>* positions, QVector<QMatrix3x3>* normals) const
{
  positions->clear();
  normals->clear();
  if (jointMatrices.size() != m_jointNames.size()) return;
  positions->reserve(jointMatrices.size());
  normals->reserve(jointMatrices.size());
  for (int j = 0; j < jointMatrices.size(); j++)
  {
    const QVector3D& scale = m_jointScales[j];
    // M * (scale * (rest - jointRest))
    QMatrix4x4 position = jointMatrices[j];
    position.scale(scale);
    position.translate(-m_restPositions[j]);
    *positions << position;

    // M (without moving) * (normal / scale)
    QMatrix3x3 normal = jointMatrices[j].toGenericMatrix<3, 3>();
    for (int row = 0; row < 3; row++)
    {
      normal(row, 0) /= scale.x();
      normal(row, 1) /= scale.y();
      normal(row, 2) /= scale.z();
    }
    *normals << normal;
  }
}

void AvatarSkin::skin(const QVector<QMatrix4x4>& jointMatrices)
{
  if (jointMatrices.size() != m_jointNames.size()) return;

  for (Part& part : m_parts)
  {
    const int count = part.restVertices.size();
    for (int i = 0; i < count; i++)
    {
      const QVector3D& rest = part.restVertices[i];
      const QVector3D& normal = part.restNormals[i];
      const int* joints = part.influenceJoints.constData() + i * INFLUENCES;
      const float* weights = part.influenceWeights.constData() + i * INFLUENCES;

      // like the viewer: the joint's own scale works on the vertex around
      // the joint, then the joint turns and moves it (normals get 1/scale);
      // the results of the joints are blended by weight
      QVector3D position, direction;
      for (int k = 0; k < INFLUENCES; k++)
      {
        const float w = weights[k];
        if (w <= 0.0f) continue;
        const int j = joints[k];
        position += jointMatrices[j].map((rest - m_restPositions[j]) * m_jointScales[j]) * w;
        direction += jointMatrices[j].mapVector(normal / m_jointScales[j]) * w;
      }
      part.vertices[i] = position;
      part.normals[i] = direction.normalized();
    }
  }
}
