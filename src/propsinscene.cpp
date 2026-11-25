/*
 * PinglyMotion - props of the scene and the animation file
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "propsinscene.h"

#include "propmanager.h"

#include <QFile>
#include <QFileInfo>
#include <QObject>

QList<PropRecord> PropsInScene::recordsOf(const PropManager& props)
{
  QList<PropRecord> records;
  for(int index=0;index<props.count();index++)
  {
    const Prop* prop=props.at(index);
    if(!prop) continue;
    PropRecord record;
    record.name=prop->name();
    record.type=int(prop->propType());
    for(int i=0;i<3;i++)
    {
      record.position[i]=prop->position()[i];
      record.scale[i]=prop->scale()[i];
      record.rotation[i]=prop->rotation()[i];
    }
    record.attachment=int(prop->isAttached());
    records << record;
  }
  return records;
}

Prop* PropsInScene::addRecord(PropManager& props, const PropRecord& record, const QSharedPointer<Mesh>& mesh)
{
  return props.addProp(static_cast<Prop::PropType>(record.type), mesh, record.attachment,
                       record.position[0], record.position[1], record.position[2],
                       record.scale[0], record.scale[1], record.scale[2],
                       record.rotation[0], record.rotation[1], record.rotation[2],
                       record.name);
}

bool PropsInScene::isPropFileName(const QString& fileName)
{
  return QFileInfo(fileName).suffix().compare(QLatin1String("prp"), Qt::CaseInsensitive) == 0;
}

PropsInScene::OpenResult PropsInScene::readPropFile(const QString& fileName)
{
  OpenResult result;
  QFile file(fileName);
  if(!file.exists())
    result.error = QObject::tr("The file was not found.");
  else if(!file.open(QIODevice::ReadOnly))
    result.error = file.errorString();
  else if(file.size() > PropFile::MAX_FILE_BYTES)
    result.error = QObject::tr("The prop file is larger than %1 KB.").arg(PropFile::MAX_FILE_BYTES / 1024);
  else
  {
    const PropFile::Result parsed = PropFile::parse(file.readAll());
    if(parsed.ok()) result.props = *parsed.props;
    else result.error = parsed.error;
  }
  return result;
}
