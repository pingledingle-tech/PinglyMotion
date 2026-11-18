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

bool Mesh::readFromFile(const QString& fileName)
{
  m_drawArraysValid = false;
  m_revision++;
  QFile file(fileName);
  if (!file.exists())
  {
    // TODO Error: File does not exist
    return false;
  }
  if (!file.open(QIODevice::ReadOnly
                 | QIODevice::Text))
  {
    // TODO Error: Unable to open file for reading
    return false;
  }

  QString fileText = (QString)(file.readAll());

  QStringList lines = fileText.split("\n", Qt::SkipEmptyParts);

  for (int i = 0; i < lines.size(); ++i)
  {
    auto tokens = lines[i].split(" ", Qt::SkipEmptyParts);

    if (tokens[0] == "v")
    {
      if (tokens.size() == 4)
      {
        // TODO Handle errors converting to float
        m_vertices << tokens[1].toFloat() << tokens[2].toFloat() << tokens[3].toFloat();
      }
      else
      {
        // TODO Handle unexpected number of tokens
        return false;
      }
    }
    else if (tokens[0] == "vn")
    {
      if (tokens.size() == 4)
      {
        // TODO Handle errors converting to float
        m_normals << tokens[1].toFloat() << tokens[2].toFloat() << tokens[3].toFloat();
      }
      else
      {
        // TODO Handle unexpected number of tokens
        return false;
      }
    }
    else if (tokens[0] == "f")
    {
      // Separates the vertex index on the left from the normal index on the right
      QString separator = "//";
      if (tokens.size() == 4)
      {
        // TODO Handle errors converting to float
        for (int j = 1; j < 4; ++j)
        {
          m_triangleVertexIndices.append(tokens[j].section(separator, 0, 0).toFloat() - 1.0f);
          m_triangleNormalIndices.append(tokens[j].section(separator, 1, 1).toFloat() - 1.0f);
        }
      }
      else
      {
        // TODO Handle unexpected number of tokens
        return false;
      }
    }
    else
    {
      // TODO Handle unexpected tokens; ignore for now
    }
  }

  return true;
}
