/***************************************************************************
 *   Copyright (C) 2006 by Zi Ree   *
 *   Zi Ree @ SecondLife   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <QFile>
#include <QtMath>
#include <QStringList>
#include <QVector>
#include <QVector3D>

#include "mesh.h"

std::function<bool(const Mesh&)> Mesh::s_drawHook;

void Mesh::setDrawHook(std::function<bool(const Mesh&)> hook)
{
  s_drawHook = std::move(hook);
}

Mesh::Mesh(const QString& fileName, const Shape shape) :
  QObject(nullptr),
  m_shape(shape),
  m_vertices(),
  m_normals(),
  m_triangleVertexIndices(),
  m_triangleNormalIndices()
{
  if (m_shape == Shape::SPHERE)
    makeSphere(16, 16);
  else if (m_shape == Shape::CONE)
    makeCone(16, 16);
  else
    readFromFile(fileName);
}

Mesh::~Mesh()
{
}

void Mesh::addVertex(float x, float y, float z, float nx, float ny, float nz)
{
  const int index = m_vertices.size() / 3;
  m_vertices << x << y << z;
  m_normals << nx << ny << nz;
  m_triangleVertexIndices << index;
  m_triangleNormalIndices << index;
  m_drawArraysValid = false;
  m_revision++;
}

void Mesh::ensureDrawArrays() const
{
  if (m_drawArraysValid) return;
  const int vertexSize = 3;
  const int count = m_triangleVertexIndices.size();
  m_drawVertices.resize(count * vertexSize);
  m_drawNormals.resize(count * vertexSize);
  for (int i = 0; i < count; ++i)
  {
    for (int j = 0; j < vertexSize; ++j)
    {
      m_drawVertices[(i * vertexSize) + j] = m_vertices.value((m_triangleVertexIndices[i] * vertexSize) + j);
      m_drawNormals[(i * vertexSize) + j] = m_normals.value((m_triangleNormalIndices.value(i) * vertexSize) + j);
    }
  }
  m_drawArraysValid = true;
}

const QVector<float>& Mesh::drawVertices() const
{
  ensureDrawArrays();
  return m_drawVertices;
}

const QVector<float>& Mesh::drawNormals() const
{
  ensureDrawArrays();
  return m_drawNormals;
}

// Unit sphere around the origin, poles on the z axis, laid out like
// gluSphere: x = -sin(theta) sin(rho), y = cos(theta) sin(rho), z = cos(rho)
void Mesh::makeSphere(int slices, int stacks)
{
  auto point = [slices, stacks](int slice, int stack)
  {
    const double rho = M_PI * stack / stacks;
    const double theta = (slice == slices) ? 0.0 : 2.0 * M_PI * slice / slices;
    return QVector3D(float(-qSin(theta) * qSin(rho)), float(qCos(theta) * qSin(rho)), float(qCos(rho)));
  };
  auto add = [this](const QVector3D& p) { addVertex(p.x(), p.y(), p.z(), p.x(), p.y(), p.z()); };

  for (int stack = 0; stack < stacks; ++stack)
  {
    for (int slice = 0; slice < slices; ++slice)
    {
      const QVector3D a = point(slice, stack);
      const QVector3D b = point(slice, stack + 1);
      const QVector3D c = point(slice + 1, stack + 1);
      const QVector3D d = point(slice + 1, stack);
      // the rows at the poles have one triangle per slice
      if (stack != stacks - 1) { add(a); add(b); add(c); }
      if (stack != 0) { add(a); add(c); add(d); }
    }
  }
}

// Cone with base radius 1 at z = 0 and the tip at z = 2, no base,
// laid out like gluCylinder: x = r sin(angle), y = r cos(angle)
void Mesh::makeCone(int slices, int stacks)
{
  const double height = 2.0;
  const double length = qSqrt(1.0 + height * height);
  const float normalXY = float(height / length);
  const float normalZ = float(1.0 / length);

  auto add = [this, slices, stacks, height, normalXY, normalZ](int slice, int stack)
  {
    const double angle = 2.0 * M_PI * (slice % slices) / slices;
    const double z = height * stack / stacks;
    const double radius = 1.0 - double(stack) / stacks;
    addVertex(float(radius * qSin(angle)), float(radius * qCos(angle)), float(z),
              float(qSin(angle)) * normalXY, float(qCos(angle)) * normalXY, normalZ);
  };

  for (int stack = 0; stack < stacks; ++stack)
  {
    for (int slice = 0; slice < slices; ++slice)
    {
      add(slice, stack); add(slice, stack + 1); add(slice + 1, stack + 1);
      if (stack != stacks - 1) { add(slice, stack); add(slice + 1, stack + 1); add(slice + 1, stack); }
    }
  }
}

void Mesh::draw() const
{
  switch (m_shape)
  {
    default:
      if (!m_triangleVertexIndices.isEmpty())
      {
        // the 3D view draws it with its shaders
        if (s_drawHook) s_drawHook(*this);
      }
      break;
  }
}

// Reads the small .obj subset the program's own data files use: "v x y z",
// "vn x y z" and triangles "f v//n v//n v//n" (indices from 1). Other lines
// (comments, groups, smoothing) are ignored. On any error the mesh is left
// empty, the reason is kept for errorString() and logged once.
bool Mesh::readFromFile(const QString& fileName)
{
  m_drawArraysValid = false;
  m_revision++;
  m_vertices.clear();
  m_normals.clear();
  m_triangleVertexIndices.clear();
  m_triangleNormalIndices.clear();
  m_errorString.clear();

  auto fail = [this, &fileName](const QString& reason)
  {
    m_vertices.clear();
    m_normals.clear();
    m_triangleVertexIndices.clear();
    m_triangleNormalIndices.clear();
    m_errorString = reason;
    qWarning("Mesh: cannot read %s: %s", qPrintable(fileName), qPrintable(reason));
    return false;
  };

  QFile file(fileName);
  if (!file.exists())
    return fail(QStringLiteral("the file does not exist"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return fail(QStringLiteral("the file cannot be opened"));

  const QString fileText = QString::fromUtf8(file.readAll());
  const QStringList lines = fileText.split('\n');

  // three numbers after the keyword, all must be valid
  auto readThree = [](const QStringList& tokens, QList<float>& out)
  {
    if (tokens.size() != 4) return false;
    float values[3];
    for (int j = 0; j < 3; ++j)
    {
      bool ok = false;
      values[j] = tokens[j + 1].toFloat(&ok);
      if (!ok || !qIsFinite(values[j])) return false;
    }
    out << values[0] << values[1] << values[2];
    return true;
  };

  for (int i = 0; i < lines.size(); ++i)
  {
    const QStringList tokens = lines[i].simplified().split(' ', Qt::SkipEmptyParts);
    if (tokens.isEmpty()) continue;
    const QString line = QString::number(i + 1);

    if (tokens[0] == QLatin1String("v"))
    {
      if (!readThree(tokens, m_vertices))
        return fail(QStringLiteral("line %1: a vertex needs three numbers").arg(line));
    }
    else if (tokens[0] == QLatin1String("vn"))
    {
      if (!readThree(tokens, m_normals))
        return fail(QStringLiteral("line %1: a normal needs three numbers").arg(line));
    }
    else if (tokens[0] == QLatin1String("f"))
    {
      if (tokens.size() != 4)
        return fail(QStringLiteral("line %1: a face needs three corners").arg(line));
      // each corner is "vertex//normal"
      const QString separator = QStringLiteral("//");
      for (int j = 1; j < 4; ++j)
      {
        bool vertexOk = false;
        bool normalOk = false;
        const QStringList parts = tokens[j].split(separator);
        const int vertex = parts.value(0).toInt(&vertexOk);
        const int normal = parts.value(1).toInt(&normalOk);
        if (parts.size() != 2 || !vertexOk || !normalOk || vertex < 1 || normal < 1)
          return fail(QStringLiteral("line %1: a face corner must be vertex//normal with numbers from 1").arg(line));
        m_triangleVertexIndices.append(vertex - 1);
        m_triangleNormalIndices.append(normal - 1);
      }
    }
    // other lines (comments, groups, smoothing) are not needed
  }

  // every face must point at vertices and normals that exist
  const int vertexCount = m_vertices.size() / 3;
  const int normalCount = m_normals.size() / 3;
  for (int i = 0; i < m_triangleVertexIndices.size(); ++i)
  {
    if (m_triangleVertexIndices[i] >= vertexCount)
      return fail(QStringLiteral("a face uses vertex %1, but there are only %2").arg(m_triangleVertexIndices[i] + 1).arg(vertexCount));
    if (m_triangleNormalIndices[i] >= normalCount)
      return fail(QStringLiteral("a face uses normal %1, but there are only %2").arg(m_triangleNormalIndices[i] + 1).arg(normalCount));
  }

  return true;
}
