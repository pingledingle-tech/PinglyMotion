/*
 * PinglyMotion Script Lab - props made from the prims, and props that move (job A1)
 *
 * When a folder is watched as one object (labfolder.h) and "Show effects in
 * 3D view" is on, every prim of the object gets a prop in the scene with the
 * prim's name, so the 3D view shows the object (docs/SCRIPT_LAB.md, "Props
 * from prims"). The sandbox knows nothing about a prim's shape or where on
 * the body it sits, so:
 *
 * - propFor() takes the prop of the same name from the folder's example.json
 *   when there is one (the example objects, docs/EXAMPLES.md), and otherwise
 *   picks a shape and a joint from words in the prim's name (the table in
 *   docs/SCRIPT_LAB.md, "Props from prims"): a collar is a ring at the neck,
 *   a hood a cone on the head, and so on. Anything else is a small box beside
 *   the figure.
 * - movesFromFolder() reads the run's log for the last position, size and
 *   rotation a script gave each prim (llSetPos, llSetScale, llSetRot,
 *   llSetLocalRot, llSetPrimitiveParams and the link functions).
 * - moved() gives a prop its new place: the change from the prim's starting
 *   value is added to the prop's own place, so the prop jumps by what the
 *   script moved it, scaled by what the script scaled it, and turned by what
 *   the script turned it. No animation.
 *
 * Second Life axes and metres become the scene's axes and units: Second Life
 * x, y, z are the scene's z, x, y (as in animexport.cpp), and one metre is
 * 39.37 scene units (inches).
 *
 * No widgets; QtCore and the QtGui math types only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABPRIMPROPS_H
#define LABPRIMPROPS_H

#include <QJsonObject>
#include <QList>
#include <QQuaternion>
#include <QString>
#include <QVector3D>

#include "labbeginner.h"

namespace LabPrimProps
{
  // scene units (inches) in one Second Life metre
  constexpr float UnitsPerMetre = 39.37f;
  // where a prim starts in the sandbox (lsl::PrimState and lsl::ObjectSpec): a child prim at the
  // root, not turned, 0.5 m on each side; a rezzed root at the region's middle, 20 m up
  QVector3D startRootPosition();     // 128, 128, 20 (Second Life metres)
  constexpr float StartSize = 0.5f;

  // one prim of the watched object
  struct Prim
  {
    QString name;
    int link = 1;
  };
  // the prims of a folder run (LabWatcher::folderResult(), LabFolder::run()), link 1 first; also when
  // the folder did not run, as long as it could be read. A root prim that is only the top folder and
  // holds no script or notecard is left out.
  QList<Prim> primsOf(const QJsonObject& folder);

  // The prop for a prim: the example's prop of the same name, else one from the words in its name.
  // index counts the prims that got no example prop and no word match, so their boxes stand in a row.
  LabExample::Prop propFor(const Prim& prim, const QList<LabExample::Prop>& examples, int index = 0);
  // propFor() for every prim of the folder run, in link order
  QList<LabExample::Prop> propsFor(const QJsonObject& folder, const QList<LabExample::Prop>& examples);
  // true when propFor() found the prim's shape from its name (not the box beside the figure)
  bool nameMatches(const QString& primName);

  // The last position, size and rotation scripts gave one prim, in Second Life values.
  struct Move
  {
    QString prim;
    int link = 1;
    bool hasPosition = false;
    QVector3D position;      // metres: local to the root for a child prim, in the region for the root
    bool hasSize = false;
    QVector3D size;          // metres
    bool hasRotation = false;
    QQuaternion rotation;    // local to the root for a child prim, in the region for the root
    bool operator==(const Move& other) const;
    bool operator!=(const Move& other) const { return !(*this == other); }
  };
  // the moves of a folder run, in link order; empty when the folder did not run or nothing moved.
  // A position the sandbox refused (outside the region, a worn or physical object) is left out.
  QList<Move> movesFromFolder(const QJsonObject& folder);

  // the prop at its new place: base is the prop where it stood before the script moved it
  LabExample::Prop moved(const LabExample::Prop& base, const Move& move);

  // Second Life vector (x, y, z) to the scene's axes (y, z, x), without changing the units
  QVector3D toScene(const QVector3D& secondLife);
  // a Second Life rotation as the same turn in the scene's axes
  QQuaternion toScene(const QQuaternion& secondLife);
  // the prop's angles (degrees, turned about x, then y, then z, as Prop::draw does) and back
  QQuaternion fromPropAngles(const QVector3D& degrees);
  QVector3D toPropAngles(const QQuaternion& rotation);
}

#endif
