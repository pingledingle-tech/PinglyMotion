/*
 * Pingmator - reader for Pingmator mesh files (.pmm)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>

#include <cmath>
#include <cstring>

#include "pmmesh.h"

static const char PMM_MAGIC[8] = {'P', 'M', 'M', 'E', 'S', 'H', '0', '1'};
static const qint64 MAX_FILE_SIZE = 16 * 1024 * 1024;
static const quint32 MAX_JOINTS = 255;
static const quint32 MAX_PARTS = 16;
static const quint32 MAX_VERTICES = 65535;
static const quint32 MAX_INDICES = 3 * 200000;
static const float MAX_COORDINATE = 10.0f;      // metres; the avatar is about 2 m tall
static const int NAME_SIZE = 64;
static const int VERTEX_SIZE = 6 * 4 + PMMesh::INFLUENCES * (1 + 4);

static bool fail(QString* errorMessage, const QString& text)
{
  if (errorMessage) *errorMessage = text;
  return false;
}

namespace
{
  // reads little-endian values from a byte array; every read checks the length
  class Reader
  {
    public:
      explicit Reader(const QByteArray& data) : m_data(data) {}

      bool u32(quint32& value)
      {
        if (!has(4)) return false;
        const uchar* p = reinterpret_cast<const uchar*>(m_data.constData() + m_pos);
        value = quint32(p[0]) | quint32(p[1]) << 8 | quint32(p[2]) << 16 | quint32(p[3]) << 24;
        m_pos += 4;
        return true;
      }
      bool u16(quint16& value)
      {
        if (!has(2)) return false;
        const uchar* p = reinterpret_cast<const uchar*>(m_data.constData() + m_pos);
        value = quint16(p[0] | p[1] << 8);
        m_pos += 2;
        return true;
      }
      bool u8(quint8& value)
      {
        if (!has(1)) return false;
        value = quint8(m_data.constData()[m_pos]);
        m_pos += 1;
        return true;
      }
      bool f32(float& value)
      {
        quint32 bits;
        if (!u32(bits)) return false;
        std::memcpy(&value, &bits, sizeof(value));
        return true;
      }
      // fixed-size, zero-padded ASCII name; must end with a zero
      bool name(QString& value)
      {
        if (!has(NAME_SIZE)) return false;
        const char* p = m_data.constData() + m_pos;
        const int length = int(qstrnlen(p, NAME_SIZE));
        if (length == 0 || length >= NAME_SIZE) return false;
        for (int i = 0; i < length; i++)
          if (p[i] < 0x20 || p[i] > 0x7e) return false;
        value = QString::fromLatin1(p, length);
        m_pos += NAME_SIZE;
        return true;
      }
      bool raw(const char* expected, int size)
      {
        if (!has(size) || std::memcmp(m_data.constData() + m_pos, expected, size) != 0) return false;
        m_pos += size;
        return true;
      }
      qint64 left() const { return m_data.size() - m_pos; }

    private:
      bool has(qint64 count) const { return count >= 0 && m_data.size() - m_pos >= count; }
      const QByteArray& m_data;
      qint64 m_pos = 0;
  };
}

static bool finiteVector(const QVector3D& v, float limit)
{
  return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z()) &&
         std::fabs(v.x()) <= limit && std::fabs(v.y()) <= limit && std::fabs(v.z()) <= limit;
}

bool PMMesh::load(const QString& fileName, QString* errorMessage)
{
  joints.clear();
  parts.clear();

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly))
    return fail(errorMessage, QString("Could not open %1: %2").arg(fileName, file.errorString()));
  if (file.size() > MAX_FILE_SIZE)
    return fail(errorMessage, QString("%1 is too big").arg(fileName));
  const QByteArray data = file.readAll();
  const QString bad = QString("%1 is not a valid Pingmator mesh file: %2").arg(fileName);

  Reader in(data);
  if (!in.raw(PMM_MAGIC, sizeof(PMM_MAGIC)))
    return fail(errorMessage, bad.arg("wrong header"));

  QStringList names;
  quint32 jointCount;
  if (!in.u32(jointCount) || jointCount == 0 || jointCount > MAX_JOINTS)
    return fail(errorMessage, bad.arg("joint count"));
  for (quint32 j = 0; j < jointCount; j++)
  {
    QString name;
    if (!in.name(name)) return fail(errorMessage, bad.arg("joint name"));
    names << name;
  }

  quint32 partCount;
  if (!in.u32(partCount) || partCount == 0 || partCount > MAX_PARTS)
    return fail(errorMessage, bad.arg("part count"));
  QVector<Part> loaded;
  for (quint32 p = 0; p < partCount; p++)
  {
    Part part;
    quint32 flags, vertexCount, indexCount;
    if (!in.name(part.name) || !in.u32(flags) || !in.u32(vertexCount) || !in.u32(indexCount))
      return fail(errorMessage, bad.arg("part header"));
    if (vertexCount == 0 || vertexCount > MAX_VERTICES)
      return fail(errorMessage, bad.arg("vertex count"));
    if (indexCount == 0 || indexCount % 3 != 0 || indexCount > MAX_INDICES)
      return fail(errorMessage, bad.arg("index count"));
    // check the length before reserving memory
    if (in.left() < qint64(vertexCount) * VERTEX_SIZE + qint64(indexCount) * 2)
      return fail(errorMessage, bad.arg("file is too short"));
    part.eye = (flags & 1) != 0;

    part.vertices.reserve(vertexCount);
    part.normals.reserve(vertexCount);
    part.joints.reserve(vertexCount * INFLUENCES);
    part.weights.reserve(vertexCount * INFLUENCES);
    for (quint32 v = 0; v < vertexCount; v++)
    {
      float f[6];
      for (float& value : f)
        if (!in.f32(value)) return fail(errorMessage, bad.arg("vertex"));
      const QVector3D position(f[0], f[1], f[2]);
      const QVector3D normal(f[3], f[4], f[5]);
      if (!finiteVector(position, MAX_COORDINATE) || !finiteVector(normal, 2.0f))
        return fail(errorMessage, bad.arg("vertex position or normal out of range"));
      part.vertices << position;
      part.normals << normal;

      quint8 joint[INFLUENCES];
      float weight[INFLUENCES];
      for (quint8& value : joint)
        if (!in.u8(value) || value >= jointCount) return fail(errorMessage, bad.arg("vertex joint"));
      float total = 0.0f;
      for (float& value : weight)
      {
        if (!in.f32(value) || !std::isfinite(value) || value < 0.0f || value > 1.001f)
          return fail(errorMessage, bad.arg("vertex weight"));
        total += value;
      }
      if (std::fabs(total - 1.0f) > 0.01f)
        return fail(errorMessage, bad.arg("vertex weights do not add up to 1"));
      for (int i = 0; i < INFLUENCES; i++)
      {
        part.joints << joint[i];
        part.weights << weight[i] / total;
      }
    }
    part.indices.reserve(indexCount);
    for (quint32 i = 0; i < indexCount; i++)
    {
      quint16 index;
      if (!in.u16(index) || index >= vertexCount) return fail(errorMessage, bad.arg("triangle corner"));
      part.indices << index;
    }
    loaded << part;
  }
  if (in.left() != 0)
    return fail(errorMessage, bad.arg("extra data at the end"));
  joints = names;
  parts = loaded;
  return true;
}
