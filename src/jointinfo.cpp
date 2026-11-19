/*
 * PinglyMotion - joint names, sections, mirror partners and hand presets (#88)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "jointinfo.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmlStreamReader>

static bool fail(QString* error,const QString& text)
{
  if(error) *error=text;
  return false;
}

bool JointInfo::load(const QString& dir,QString* error)
{
  m_joints.clear();
  m_presets.clear();
  m_index.clear();
  m_nameCount.clear();

  QFile file(QDir(dir).filePath("joints.json"));
  if(!file.open(QIODevice::ReadOnly))
    return fail(error,QStringLiteral("cannot open %1").arg(file.fileName()));
  QJsonParseError parseError;
  const QJsonDocument doc=QJsonDocument::fromJson(file.readAll(),&parseError);
  if(!doc.isObject())
    return fail(error,QStringLiteral("%1: %2").arg(file.fileName(),parseError.errorString()));

  const QJsonArray joints=doc.object().value("joints").toArray();
  for(const QJsonValue& value : joints)
  {
    const QJsonObject o=value.toObject();
    Joint j;
    j.sl=o.value("sl").toString();
    j.bvh=o.value("bvh").toString();
    j.name=o.value("name").toString();
    j.nameFrom=o.value("nameFrom").toString();
    j.tab=o.value("tab").toString();
    j.firestormType=o.value("firestormType").toString();
    j.firestormSection=o.value("firestormSection").toString();
    j.mirror=o.value("mirror").toString();
    j.dontFlipOnMirror=o.value("dontFlipOnMirror").toBool();
    j.firestormTransform=o.value("firestormTransform").toString();
    j.firestormFrame=o.value("firestormFrame").toString();
    j.firestormDeltaRotate=o.value("firestormDeltaRotate").toBool();
    if(j.sl.isEmpty() || j.bvh.isEmpty())
      return fail(error,QStringLiteral("joint without a name in %1").arg(file.fileName()));
    if(m_index.contains(j.sl) || (j.bvh!=j.sl && m_index.contains(j.bvh)))
      return fail(error,QStringLiteral("joint %1 listed twice").arg(j.sl));
    m_index.insert(j.sl,m_joints.count());
    m_index.insert(j.bvh,m_joints.count());
    m_nameCount[j.name]++;
    m_joints << j;
  }

  const QJsonArray presets=doc.object().value("handPresets").toArray();
  for(const QJsonValue& value : presets)
  {
    HandPreset preset;
    preset.name=value.toObject().value("name").toString();
    preset.file=value.toObject().value("file").toString();
    if(!loadPreset(QDir(dir).filePath(preset.file),preset,error)) return false;
    m_presets << preset;
  }
  return true;
}

// reads the next <real> values of an LLSD <array>
static QVector3D readVector(QXmlStreamReader& xml)
{
  float v[3]={0,0,0};
  int n=0;
  while(xml.readNextStartElement())
  {
    if(xml.name()==QLatin1String("real") && n<3)
      v[n++]=xml.readElementText().toFloat();
    else
      xml.skipCurrentElement();
  }
  return QVector3D(v[0],v[1],v[2]);
}

// reads <map><key>value</key><boolean|integer>…</…></map>
static QString readValueMap(QXmlStreamReader& xml)
{
  QString text;
  while(xml.readNextStartElement())
  {
    if(xml.name()==QLatin1String("key"))
      xml.skipCurrentElement();
    else
      text=xml.readElementText();
  }
  return text;
}

// Firestorm pose file: LLSD XML, a map of joint name -> map of
// enabled, jointBaseRotationIsZero, position, rotation, scale
bool JointInfo::loadPreset(const QString& path,HandPreset& preset,QString* error)
{
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
    return fail(error,QStringLiteral("cannot open %1").arg(path));
  QXmlStreamReader xml(&file);
  if(!xml.readNextStartElement() || xml.name()!=QLatin1String("llsd") ||
     !xml.readNextStartElement() || xml.name()!=QLatin1String("map"))
    return fail(error,QStringLiteral("%1 is not an LLSD map").arg(path));

  QString key;
  while(xml.readNextStartElement())
  {
    if(xml.name()==QLatin1String("key"))
    {
      key=xml.readElementText();
      continue;
    }
    if(xml.name()!=QLatin1String("map"))
    {
      xml.skipCurrentElement();
      continue;
    }
    if(key==QLatin1String("version"))
      preset.version=readValueMap(xml).toInt();
    else if(key==QLatin1String("startFromTeePose"))
      preset.startFromTPose=readValueMap(xml).toInt()!=0;
    else
    {
      PresetJoint pj;
      pj.sl=key;
      QString field;
      while(xml.readNextStartElement())
      {
        if(xml.name()==QLatin1String("key")) { field=xml.readElementText(); continue; }
        if(field==QLatin1String("enabled")) pj.enabled=xml.readElementText().toInt()!=0;
        else if(field==QLatin1String("jointBaseRotationIsZero")) pj.baseRotationIsZero=xml.readElementText().toInt()!=0;
        else if(field==QLatin1String("rotation")) pj.rotation=readVector(xml);
        else if(field==QLatin1String("position")) pj.position=readVector(xml);
        else if(field==QLatin1String("scale")) pj.scale=readVector(xml);
        else xml.skipCurrentElement();
      }
      preset.joints << pj;
    }
  }
  if(xml.hasError())
    return fail(error,QStringLiteral("%1: %2").arg(path,xml.errorString()));
  return true;
}

const JointInfo::Joint* JointInfo::joint(const QString& name) const
{
  const auto it=m_index.constFind(name);
  return it==m_index.constEnd() ? nullptr : &m_joints[it.value()];
}

QString JointInfo::friendlyName(const QString& name) const
{
  const Joint* j=joint(name);
  return j ? j->name : QString();
}

QString JointInfo::uniqueName(const QString& name) const
{
  const Joint* j=joint(name);
  if(!j) return QString();
  if(m_nameCount.value(j->name)<=1 || j->firestormSection.isEmpty()) return j->name;
  return j->firestormSection+QStringLiteral(": ")+j->name;
}

QString JointInfo::mirrorOf(const QString& name) const
{
  const Joint* j=joint(name);
  if(!j || j->mirror.isEmpty()) return QString();
  const Joint* partner=joint(j->mirror);
  if(!partner) return QString();
  return (name==j->bvh) ? partner->bvh : partner->sl;
}
