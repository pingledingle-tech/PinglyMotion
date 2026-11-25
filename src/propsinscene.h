/*
 * PinglyMotion - props of the scene and the animation file
 *
 * The props the user placed are saved with the animation in the .dingle file ("props",
 * docs/DINGLE_FORMAT.md) and put back when the file is opened. An old .prp file opens through
 * the same code. These functions have no window, so they can be tested.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef PROPSINSCENE_H
#define PROPSINSCENE_H

#include <QByteArray>
#include <QList>
#include <QSharedPointer>
#include <QString>

#include "propfile.h"

class Mesh;
class Prop;
class PropManager;

namespace PropsInScene
{
  // the props of the scene as records, in the order of the scene (name, shape, place, size, turn,
  // attachment point). A colour a script gave a prop (Script Lab) is not kept.
  QList<PropRecord> recordsOf(const PropManager& props);

  // puts one record into the scene under its own name; a name that is empty or taken gives the
  // usual "Box 0", "Box 1", ... The mesh is the one of the record's shape. Returns the new prop.
  Prop* addRecord(PropManager& props, const PropRecord& record, const QSharedPointer<Mesh>& mesh);

  // What File > Open does with a .prp file: the records of the file, or why it cannot be read
  struct OpenResult
  {
    QList<PropRecord> props;
    QString error;
    bool ok() const { return error.isEmpty(); }
  };
  OpenResult readPropFile(const QString& fileName);

  // true when File > Open should treat this file name as a prop file
  bool isPropFileName(const QString& fileName);
}

#endif
