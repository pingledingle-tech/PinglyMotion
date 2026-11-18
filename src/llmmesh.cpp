/*
 * Pingmator - reader for Second Life avatar meshes (.llm)
 *
 * File layout, all little endian, floats are 32 bit:
 *   char[24]  "Linden Binary Mesh 1.0", padded with zeros
 *   u8        hasWeights
 *   u8        hasDetailTexCoords
 *   float[3]  position
 *   float[3]  rotation angles
 *   u8        rotation order
 *   float[3]  scale
 *   u16       vertex count, then per vertex:
 *               float[3] position, float[3] normal, float[3] binormal,
 *               float[2] tex coord, float[2] detail tex coord (if flag),
 *               float weight (if flag) - each as its own block
 *   u16       triangle count, then s16[3] per triangle
 *   u16       skin joint count, then char[64] per name (if hasWeights)
 *   morphs: char[64] name, s32 vertex count, then per vertex u32 index,
 *           float[3] position, float[3] normal, float[3] binormal,
 *           float[2] tex coord; the list ends with the name "End Morphs"
 *   vertex remaps follow; they are not read here
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDataStream>
#include <QFile>

#include "llmmesh.h"

#include <cmath>

static const char LLM_HEADER[] = "Linden Binary Mesh 1.0";

static bool fail(QString* errorMessage, const QString& text)
{
  if (errorMessage) *errorMessage = text;
  return false;
}

static QVector3D readVector3(QDataStream& in)
{
  float x, y, z;
  in >> x >> y >> z;
  return QVector3D(x, y, z);
}

static QVector2D readVector2(QDataStream& in)
{
  float x, y;
  in >> x >> y;
  return QVector2D(x, y);
}

bool LLMMesh::load(const QString& fileName, QString* errorMessage)
{
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly))
    return fail(errorMessage, QString("Could not open %1: %2").arg(fileName, file.errorString()));

  QDataStream in(&file);
  in.setByteOrder(QDataStream::LittleEndian);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

  char header[24];
  if (in.readRawData(header, sizeof(header)) != sizeof(header) ||
      qstrncmp(header, LLM_HEADER, sizeof(LLM_HEADER)) != 0)
    return fail(errorMessage, QString("%1 is not a Linden Binary Mesh 1.0 file").arg(fileName));

  quint8 flag;
  in >> flag; hasWeights = flag != 0;
  in >> flag; hasDetailTexCoords = flag != 0;
  position = readVector3(in);
  rotationAngles = readVector3(in);
  in >> rotationOrder;
  scale = readVector3(in);

  quint16 vertexCount;
  in >> vertexCount;

  vertices.resize(vertexCount);
  normals.resize(vertexCount);
  binormals.resize(vertexCount);
  texCoords.resize(vertexCount);
  for (auto& v : vertices) v = readVector3(in);
  for (auto& v : normals) v = readVector3(in);
  for (auto& v : binormals) v = readVector3(in);
  for (auto& v : texCoords) v = readVector2(in);

  detailTexCoords.clear();
  if (hasDetailTexCoords)
  {
    detailTexCoords.resize(vertexCount);
    for (auto& v : detailTexCoords) v = readVector2(in);
  }

  weights.clear();
  if (hasWeights)
  {
    weights.resize(vertexCount);
    for (auto& w : weights)
    {
      in >> w;
      // the whole number part picks a joint; NaN or huge values cannot be used
      if (in.status() == QDataStream::Ok && (!std::isfinite(w) || w < 0.0f || w >= 65536.0f))
        return fail(errorMessage, QString("%1: vertex weight %2 is not usable").arg(fileName).arg(w));
    }
  }

  quint16 triangleCount;
  in >> triangleCount;
  indices.resize(triangleCount * 3);
  for (auto& index : indices)
  {
    qint16 value;
    in >> value;
    if (value < 0 || value >= vertexCount)
      return fail(errorMessage, QString("%1: triangle points to vertex %2, but there are only %3")
                                  .arg(fileName).arg(value).arg(vertexCount));
    index = static_cast<quint16>(value);
  }

  skinJoints.clear();
  if (hasWeights)
  {
    quint16 jointCount;
    in >> jointCount;
    for (int i = 0; i < jointCount; i++)
    {
      char name[64];
      if (in.readRawData(name, sizeof(name)) != sizeof(name)) break;
      name[sizeof(name) - 1] = 0;
      skinJoints << QString::fromLatin1(name);
    }
  }

  if (in.status() != QDataStream::Ok)
    return fail(errorMessage, QString("%1 ended too early").arg(fileName));

  // morphs (LLPolyMorphData::loadBinary in the Second Life viewer)
  morphs.clear();
  char morphName[64];
  while (in.readRawData(morphName, sizeof(morphName)) == sizeof(morphName))
  {
    morphName[sizeof(morphName) - 1] = 0;
    if (qstrcmp(morphName, "End Morphs") == 0) break;

    Morph morph;
    morph.name = QString::fromLatin1(morphName);
    qint32 count;
    in >> count;
    if (count < 0 || count > vertexCount)
      return fail(errorMessage, QString("%1: morph %2 has %3 vertices").arg(fileName, morph.name).arg(count));
    for (int i = 0; i < count; i++)
    {
      quint32 index;
      in >> index;
      const QVector3D coord = readVector3(in);
      const QVector3D normal = readVector3(in);
      readVector3(in);   // binormal
      readVector2(in);   // tex coord
      if (index >= vertexCount)
        return fail(errorMessage, QString("%1: morph %2 points to vertex %3").arg(fileName, morph.name).arg(index));
      morph.vertexIndices << index;
      morph.coords << coord;
      morph.normals << normal;
    }
    if (in.status() != QDataStream::Ok)
      return fail(errorMessage, QString("%1: morph %2 ended too early").arg(fileName, morph.name));
    morphs << morph;
  }

  return true;
}

const LLMMesh::Morph* LLMMesh::morph(const QString& name) const
{
  for (const Morph& m : morphs)
    if (m.name == name) return &m;
  return nullptr;
}
