/*
 * Pingmator - reader for prop files (.prp)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "propfile.h"

#include <QObject>
#include <QStringList>

#include <cmath>

namespace
{
  bool fail(QString* error, const QString& reason)
  {
    if (error) *error = reason;
    return false;
  }
}

bool PropFile::parse(const QByteArray& data, QList<PropRecord>& props, QString* error)
{
  props.clear();
  // all props or none
  const auto fail = [&props](QString* error, const QString& reason) { props.clear(); return ::fail(error, reason); };
  if (data.size() > MAX_FILE_BYTES)
    return fail(error, QObject::tr("The prop file is larger than %1 KB.").arg(MAX_FILE_BYTES / 1024));

  const QList<QByteArray> lines = data.split('\n');
  for (int lineNumber = 1; lineNumber <= lines.size(); lineNumber++)
  {
    const QString line = QString::fromLatin1(lines[lineNumber - 1]).simplified();
    if (line.isEmpty()) continue;

    const QStringList values = line.split(' ');
    // older files have no attachment value
    if (values.size() != 10 && values.size() != 11)
      return fail(error, QObject::tr("Line %1 has %2 values, expected 11.").arg(lineNumber).arg(values.size()));

    if (props.size() >= MAX_PROPS)
      return fail(error, QObject::tr("The file has more than %1 props.").arg(MAX_PROPS));

    PropRecord prop;
    bool ok = false;
    prop.type = values[0].toInt(&ok);
    if (!ok || prop.type < 0 || prop.type > 3)
      return fail(error, QObject::tr("Line %1: unknown prop type \"%2\".").arg(lineNumber).arg(values[0].left(20)));

    double* targets[9] = {&prop.position[0], &prop.position[1], &prop.position[2],
                          &prop.scale[0], &prop.scale[1], &prop.scale[2],
                          &prop.rotation[0], &prop.rotation[1], &prop.rotation[2]};
    for (int i = 0; i < 9; i++)
    {
      const double value = values[i + 1].toDouble(&ok);
      if (!ok || !std::isfinite(value) || std::fabs(value) > MAX_VALUE)
        return fail(error, QObject::tr("Line %1: \"%2\" is not a usable number.").arg(lineNumber).arg(values[i + 1].left(20)));
      *targets[i] = value;
    }

    if (values.size() == 11)
    {
      prop.attachment = values[10].toInt(&ok);
      if (!ok || prop.attachment < 0 || prop.attachment > MAX_ATTACHMENT)
        return fail(error, QObject::tr("Line %1: bad attachment point \"%2\".").arg(lineNumber).arg(values[10].left(20)));
    }
    props << prop;
  }
  return true;
}
