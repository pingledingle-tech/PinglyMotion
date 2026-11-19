/*
 * PinglyMotion - joint names, sections, mirror partners and hand presets (#88)
 *
 * Reads data/firestorm/joints.json and the hand presets beside it. The data
 * comes from the Firestorm viewer (LGPL 2.1, see data/firestorm/README.md);
 * tools/make_firestorm_joints.py makes it.
 *
 * Joints can be looked up by the name in the BVH file (hip, lShldr, mHandIndex1Left)
 * or by the Second Life name (mPelvis, mShoulderLeft). Functions that return a
 * joint name answer in the same kind of name they were asked with.
 *
 * The joint list in the properties panel uses the friendly names and sections
 * (JointList, posing UX step 2).
 *
 * The hand presets are kept as Firestorm stores them: Euler angles in radians in
 * Firestorm's own axes. They are NOT in PinglyMotion's BVH rotation model; the
 * conversion is still open (see the README).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef JOINTINFO_H
#define JOINTINFO_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>

class JointInfo
{
  public:
    struct Joint
    {
      QString sl;                 // Second Life name, mShoulderLeft
      QString bvh;                // name in PinglyMotion's BVH files, lShldr
      QString name;               // friendly name, "Whole Arm"
      QString nameFrom;           // "firestorm" or "pinglymotion"
      QString tab;                // Body, Face, Hands or Other (JointGroups::tabOf())
      QString firestormType;      // BODY, FACE, HANDS, MISC, WHOLEAVATAR, empty if not in Firestorm
      QString firestormSection;   // heading in Firestorm's poser list, "Left Arm"
      QString mirror;             // SL name of the left/right partner, empty if none
      bool dontFlipOnMirror=false;
      QString firestormTransform; // Firestorm's axis swap for its sliders, raw
      QString firestormFrame;     // Firestorm's axis swap for its gizmo, raw
      bool firestormDeltaRotate=false;
    };

    struct PresetJoint
    {
      QString sl;
      bool enabled=false;
      bool baseRotationIsZero=false;
      QVector3D rotation;         // as stored by Firestorm (radians, Firestorm axes)
      QVector3D position;
      QVector3D scale;
    };

    struct HandPreset
    {
      QString name;               // "Thumbs Up"
      QString file;               // relative to the data folder
      int version=0;
      bool startFromTPose=false;
      QVector<PresetJoint> joints;
    };

    // dir is the folder that holds joints.json; false and *error when it fails
    bool load(const QString& dir=QStringLiteral("data/firestorm"),QString* error=nullptr);

    const QVector<Joint>& joints() const { return m_joints; }
    const QVector<HandPreset>& handPresets() const { return m_presets; }

    // nullptr when the joint is not known
    const Joint* joint(const QString& name) const;

    // friendly name, empty when the joint is not known
    QString friendlyName(const QString& name) const;
    // friendly name that is unique over all joints: the section is put in front
    // when the friendly name alone is used more than once ("Left Arm: Collar")
    QString uniqueName(const QString& name) const;
    // left/right partner in the same kind of name, empty when there is none
    QString mirrorOf(const QString& name) const;

  private:
    bool loadPreset(const QString& path,HandPreset& preset,QString* error);

    QVector<Joint> m_joints;
    QVector<HandPreset> m_presets;
    QHash<QString,int> m_index;        // BVH and SL names -> m_joints
    QHash<QString,int> m_nameCount;    // friendly name -> how many joints use it
};

#endif
