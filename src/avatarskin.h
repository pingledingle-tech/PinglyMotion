/*
 * PinglyMotion - the avatar body, bent with the skeleton
 *
 * Loads a Bento mesh body from a PinglyMotion mesh file (.pmm): Ruth2 v4
 * (bin/data/character/ruth2/ruth2.pmm, the default) or Max
 * (bin/data/character/max/maxine.pmm). Their fingers and faces are weighted
 * to the Bento bones; each vertex follows up to four joints.
 *
 * A male figure shows Roth2 v2 (bin/data/character/roth2/roth2.pmm) instead,
 * chosen by setMale(); there is no menu entry for it. If that file cannot be
 * loaded, the male figure is the chosen body with the male skeleton scaling.
 *
 * No OpenGL in here; the view draws the result.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef AVATARSKIN_H
#define AVATARSKIN_H

#include <QHash>
#include <QMatrix4x4>
#include <QGenericMatrix>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>

class AvatarSkin
{
  public:
    struct Part
    {
      QString name;
      bool eye = false;
      // rest pose, in BVH axes and inches, relative to the pelvis
      QVector<QVector3D> restVertices;
      QVector<QVector3D> restNormals;
      // each vertex: joint A, joint B and how far it leans towards B (0..1);
      // the numbers point into jointNames()
      QVector<int> jointA;
      QVector<int> jointB;
      QVector<float> blend;
      // what skin() uses: INFLUENCES joints and weights per vertex, from the
      // mesh file (joint A/B above hold the two strongest, for colouring)
      QVector<int> influenceJoints;
      QVector<float> influenceWeights;
      // the male shape: added to the female rest pose (Male_Head, Male_Torso, Male_Legs)
      QVector<QVector3D> maleOffsets;
      QVector<QVector3D> maleNormalOffsets;
      QVector<QVector3D> femaleVertices;
      QVector<QVector3D> femaleNormals;
      QVector<quint16> indices;       // three per triangle
      QVector<int> triangleJoint;     // the joint a triangle mostly belongs to
      // result of skin()
      QVector<QVector3D> vertices;
      QVector<QVector3D> normals;
    };

    static const int INFLUENCES = 4;

    enum Body
    {
      RuthBody,       // the Ruth2 v4 Bento avatar, character/ruth2/ruth2.pmm (default)
      MaxBody         // the Max Bento avatar, character/max/maxine.pmm
    };

    // reads skin_joints.txt and the body's mesh files from the data folder
    bool load(const QString& dataPath, QString* errorMessage = nullptr) { return load(dataPath, RuthBody, errorMessage); }
    bool load(const QString& dataPath, Body body, QString* errorMessage = nullptr);
    Body body() const { return m_body; }
    bool isLoaded() const { return !m_parts.isEmpty(); }

    // names (as in the BVH files: hip, abdomen, lShldr ...) of the joints the mesh follows
    const QStringList& jointNames() const { return m_jointNames; }
    // rest position of a joint from jointNames(), relative to the pelvis
    QVector3D restPosition(int joint) const { return m_restPositions[joint]; }

    // Moves all vertices. jointMatrices holds one matrix per entry of
    // jointNames(): it takes a point from the joint's own space (origin at
    // the joint) to wherever the caller wants the result.
    void skin(const QVector<QMatrix4x4>& jointMatrices);

    // The same sums as skin(), as one position matrix and one normal matrix
    // per joint, for a shader: vertex = sum of weight * positions[j] * rest,
    // normal = normalize(sum of weight * normals[j] * restNormal).
    void skinMatrices(const QVector<QMatrix4x4>& jointMatrices,
                      QVector<QMatrix4x4>* positions, QVector<QMatrix3x3>* normals) const;

    // goes up whenever the rest pose changes (a new body is loaded, or the
    // male/female switch), so copies of it on the graphics card can be renewed
    int restRevision() const { return m_restRevision; }

    const QVector<Part>& parts() const { return m_parts; }

    // switches the rest pose between the female and the male body
    // (the "male" shape parameter in the Second Life viewer, fully on or off)
    void setMale(bool male);
    bool isMale() const { return m_male; }
    // true when the male figure uses the Roth2 mesh (it is already male in
    // shape, so the male skeleton scaling is not applied on top of it)
    bool hasMaleMesh() const { return m_roth.loaded; }
    bool maleMeshShown() const { return m_male && m_roth.loaded; }

    // The male skeleton (Male_Skeleton in the Second Life viewer): some joints
    // are scaled, which makes the body around them bigger and moves their
    // children further out. Names as in the BVH files; BVH axes.
    // 1,1,1 / 0,0,0 for joints that don't change or when the body is female.
    QVector3D jointScale(const QString& name) const;
    QVector3D jointShift(const QString& name) const;   // inches, added to the offset
    // how much the male skeleton's longer shins push the feet down, in inches;
    // lift the figure by this so it still stands on the floor
    float footDrop() const;

  private:
    struct SkinJoint
    {
      QString fileName;
      QString slName;
      int parent;
      bool base;
      QVector3D rest;
      QVector<int> children;
    };

    // everything that belongs to one loaded mesh file
    struct MeshState
    {
      QStringList jointNames;
      QVector<QVector3D> restPositions;
      QHash<int,int> usedJoints;
      QVector<Part> parts;
      QVector<QVector3D> jointScales;
      bool loaded = false;
    };
    void swapMesh(MeshState& other);       // exchanges the mesh in use with other
    void loadMaleMesh(const QString& fileName);

    bool loadJoints(const QString& fileName, QString* errorMessage);
    bool loadPmm(const QString& fileName, QString* errorMessage);
    int useJoint(int skeletonJoint);

    QVector<SkinJoint> m_skeleton;          // whole skeleton, SL order
    QHash<QString,int> m_skeletonBySlName;
    QStringList m_jointNames;               // only the joints the mesh follows
    QVector<QVector3D> m_restPositions;
    QHash<int,int> m_usedJoints;            // skeleton joint -> index in m_jointNames
    QVector<Part> m_parts;
    bool m_male = false;
    Body m_body = RuthBody;
    QHash<QString,QVector3D> m_maleScales;   // by BVH name, scale (1 = no change)
    QHash<QString,QVector3D> m_maleShifts;   // by BVH name, inches
    QVector<QVector3D> m_jointScales;        // per entry of m_jointNames, for skin()
    int m_restRevision = 0;
    MeshState m_roth;                        // Roth2 while the female body is in use, else the female body
    bool m_rothShown = false;                // the mesh in use is the Roth2 one
};

#endif
