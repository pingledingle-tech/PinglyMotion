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

#ifndef MESH_H
#define MESH_H

#include <QVector3D>
#include <QVector4D>

#include <QObject>
#include <QVector>

#include <functional>

class Mesh : public QObject
{
  Q_OBJECT

  public:
    enum class Shape
    {
        NONE = 0,
        CUBE = 1,
        SPHERE = 2,
        CONE = 3,
        TORUS = 4,
        CUSTOM = 5
    };

    explicit Mesh(const QString& fileName, const Shape shape);
    ~Mesh();

    void draw() const;

    // the triangles as plain arrays, 3 floats per vertex, ready to draw
    const QVector<float>& drawVertices() const;
    const QVector<float>& drawNormals() const;
    // goes up whenever the shape changes, so copies on the graphics card can be renewed
    int revision() const { return m_revision; }

    // The 3D view draws meshes with its shaders (also for picking). Without a
    // hook nothing is drawn. The return value is not used.
    static void setDrawHook(std::function<bool(const Mesh&)> hook);

    // false on a missing or broken file; the mesh is then empty
    bool readFromFile(const QString& fileName);
    // why the last readFromFile() failed, empty after a good read
    const QString& errorString() const { return m_errorString; }

  private:
    // the same shapes gluSphere(1, 16, 16) and gluCylinder(1, 0, 2, 16, 16)
    // used to draw, built once as triangles
    void makeSphere(int slices, int stacks);
    void makeCone(int slices, int stacks);
    void addVertex(float x, float y, float z, float nx, float ny, float nz);

    void ensureDrawArrays() const;

    Shape m_shape;
    int m_revision = 0;
    mutable bool m_drawArraysValid = false;
    mutable QVector<float> m_drawVertices;
    mutable QVector<float> m_drawNormals;
    static std::function<bool(const Mesh&)> s_drawHook;

    QList<float> m_vertices;
    QList<float> m_normals;

    QList<int> m_triangleVertexIndices;
    QList<int> m_triangleNormalIndices;

    QString m_errorString;

};

#endif
