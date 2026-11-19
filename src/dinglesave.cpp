/*
 * PinglyMotion - file name rules for saving .dingle files (issue #26, jobs 5 and 6)
 * See dinglesave.h.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <QDir>
#include <QFileInfo>

#include <cmath>

#include "dinglesave.h"

namespace DingleSave
{
  QString defaultName()
  {
    return QStringLiteral("pingle.dingle");
  }

  bool needsName(const QString& currentFile,const QString& untitledName)
  {
    if(currentFile.isEmpty() || currentFile==untitledName) return true;
    return QFileInfo(currentFile).suffix().toLower()!=QLatin1String("dingle");
  }

  QString suggestedName(const QString& currentFile,const QString& untitledName)
  {
    if(currentFile.isEmpty() || currentFile==untitledName) return defaultName();
    const QFileInfo info(currentFile);
    const QString name=info.completeBaseName()+QStringLiteral(".dingle");
    return info.path()==QLatin1String(".") ? name : info.path()+QLatin1Char('/')+name;
  }

  QString withDingleEnding(const QString& fileName)
  {
    if(QFileInfo(fileName).suffix().toLower()==QLatin1String("dingle")) return fileName;
    return fileName+QStringLiteral(".dingle");
  }

  bool replacesOldFile(const QString& oldFile,const QString& newFile)
  {
    const QFileInfo oldInfo(oldFile);
    const QFileInfo newInfo(newFile);
    if(oldInfo.suffix().toLower()!=QLatin1String("avm")) return false;
    if(newInfo.suffix().toLower()!=QLatin1String("dingle")) return false;
    if(oldInfo.completeBaseName()!=newInfo.completeBaseName()) return false;
    return QDir::cleanPath(oldInfo.absolutePath())==QDir::cleanPath(newInfo.absolutePath());
  }

  bool sameAnimation(const AnimFile::Animation& a,const AnimFile::Animation& b,float tolerance)
  {
    return firstDifference(a,b,tolerance).isEmpty();
  }

  QString firstDifference(const AnimFile::Animation& a,const AnimFile::Animation& b,float tolerance)
  {
    if(a.frames!=b.frames) return QStringLiteral("frames %1 against %2").arg(a.frames).arg(b.frames);
    if(a.valuesPerFrame!=b.valuesPerFrame) return QStringLiteral("values per frame");
    if(a.joints.size()!=b.joints.size()) return QStringLiteral("joint count");
    if(std::fabs(a.frameTime-b.frameTime)>1.0e-9) return QStringLiteral("frame time");
    if(a.figure!=b.figure) return QStringLiteral("figure");
    if(a.loopIn!=b.loopIn || a.loopOut!=b.loopOut) return QStringLiteral("loop %1,%2 against %3,%4").arg(a.loopIn).arg(a.loopOut).arg(b.loopIn).arg(b.loopOut);
    if(std::fabs(a.avatarScale-b.avatarScale)>1.0e-6f) return QStringLiteral("scale");
    if(a.values.size()!=b.values.size()) return QStringLiteral("value count");
    for(int i=0;i<a.joints.size();i++)
    {
      const AnimFile::Joint& x=a.joints[i];
      const AnimFile::Joint& y=b.joints[i];
      if(x.name!=y.name || x.kind!=y.kind || x.parent!=y.parent || x.channels!=y.channels) return QStringLiteral("joint %1").arg(i);
      if(x.kind!=AnimFile::JointKind::EndSite && x.keys!=y.keys) return QStringLiteral("keys of %1").arg(x.name);
    }
    for(size_t i=0;i<a.values.size();i++)
      if(!(std::fabs(a.values[i]-b.values[i])<=tolerance))
        return QStringLiteral("value %1: %2 against %3").arg(i).arg(a.values[i]).arg(b.values[i]);
    return QString();
  }
}
