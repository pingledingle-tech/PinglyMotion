/*
 * Pingmator - reader for Second Life avatar meshes (.llm)
 *
 * The .llm files ("Linden Binary Mesh 1.0") hold the classic system avatar
 * body: head, upper body, lower body, eyes and so on. Vertex positions are in
 * avatar space (metres, X forward, Y left, Z up, feet at Z=0).
 *
 * Each vertex has one weight. The whole number part picks a joint, the
 * fraction blends towards the next joint. The joint numbers do not point
 * straight into skinJoints: the viewer builds a longer list where the parent
 * of a joint is put in front of it when needed. See LLAvatarJointMesh::setupJoint
 * in the Second Life viewer.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LLMMESH_H
#define LLMMESH_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

class LLMMesh
{
  public:
    // reads a base mesh (not the _1.._4 level-of-detail files)
    bool load(const QString& fileName, QString* errorMessage = nullptr);

    bool hasWeights = false;
    bool hasDetailTexCoords = false;

    QVector3D position;          // mesh offset from the header
    QVector3D rotationAngles;
    quint8 rotationOrder = 0;
    QVector3D scale;

    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<QVector3D> binormals;
    QVector<QVector2D> texCoords;
    QVector<QVector2D> detailTexCoords;
    QVector<float> weights;       // one per vertex, empty if !hasWeights

    QVector<quint16> indices;     // three per triangle
    QStringList skinJoints;       // joint names in file order

    // shape changes stored in the file (Male_Torso, Big_Belly_Torso ...):
    // for some vertices, how far they move when the shape is fully on
    struct Morph
    {
      QString name;
      QVector<quint32> vertexIndices;
      QVector<QVector3D> coords;      // added to the vertex position
      QVector<QVector3D> normals;     // added to the normal
    };
    QVector<Morph> morphs;
    const Morph* morph(const QString& name) const;

    int triangleCount() const { return indices.size() / 3; }
};

#endif
