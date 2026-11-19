/*
 * PinglyMotion - reader for PinglyMotion mesh files (.pmm)
 *
 * A small skinned mesh format written by tools/make_max_mesh.py, used for
 * the Max Bento avatar (bin/data/character/max/). Each vertex follows up to
 * four joints. Positions are in avatar space in metres,
 * X forward, Y left, Z up, feet at Z=0). The file layout is described in
 * tools/make_max_mesh.py.
 *
 * Every count, index and number is checked; a bad file is refused whole.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef PMMESH_H
#define PMMESH_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>

class PMMesh
{
  public:
    static const int INFLUENCES = 4;

    struct Part
    {
      QString name;
      bool eye = false;
      QVector<QVector3D> vertices;
      QVector<QVector3D> normals;
      QVector<quint8> joints;       // INFLUENCES per vertex, into PMMesh::joints
      QVector<float> weights;       // INFLUENCES per vertex, sum 1
      QVector<quint16> indices;     // three per triangle
    };

    bool load(const QString& fileName, QString* errorMessage = nullptr);

    QStringList joints;             // SL names (mPelvis, mHandIndex1Left ...)
    QVector<Part> parts;
};

#endif
