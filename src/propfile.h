/*
 * PinglyMotion - reader for prop files (.prp)
 *
 * One prop per line, separated by spaces:
 *   type x y z xScale yScale zScale xRotation yRotation zRotation attachment
 * type is 0 box, 1 sphere, 2 cone, 3 torus. Older files have no attachment
 * value (10 values); it is then 0 (not attached).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef PROPFILE_H
#define PROPFILE_H

#include <QByteArray>
#include <QList>
#include <QString>

#include <optional>

struct PropRecord
{
  int type = 0;
  double position[3] = {0, 0, 0};
  double scale[3] = {1, 1, 1};
  double rotation[3] = {0, 0, 0};
  int attachment = 0;
};

namespace PropFile
{
  // limits for files that are read
  const int MAX_PROPS = 1000;
  const int MAX_FILE_BYTES = 1024 * 1024;
  // positions, scales and rotations must be within this, in either direction
  const double MAX_VALUE = 1000000.0;
  // attachment points are joint numbers
  const int MAX_ATTACHMENT = 10000;

  // the result of parse(): all props, or none and why the file was refused
  struct Result
  {
    // set when the file was read (an empty file gives an empty list); then error is empty
    std::optional<QList<PropRecord>> props;
    // why the file cannot be read, for the user
    QString error;

    bool ok() const { return props.has_value(); }
  };

  // reads all props, or none when a line is not a valid prop
  Result parse(const QByteArray& data);

  // the whole file for these props (bytes out, docs/DEV.md "Web-ready code"): one line per prop,
  // "type x y z width height depth turnX turnY turnZ attachment", numbers with 9 significant
  // digits, so a prop's float values come back exactly from parse()
  QByteArray write(const QList<PropRecord>& props);
}

#endif
