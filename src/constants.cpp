/***************************************************************************
 *   Copyright (C) 2006 by Zi Ree                                          *
 *   Zi Ree @ SecondLife                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef __APPLE__
#  include <QApplication>
#endif

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QStringList>

#include "versionnumber.h"

#include "constants.h"
#include "textsize.h"

#ifdef BUILD_TIMESTAMP_Z
  const QString Constants::m_buildDateTime = BUILD_TIMESTAMP_Z;
#else
  const QString Constants::m_buildDateTime = QString();
#endif

#ifdef VERSION_NUMBER
  const VersionNumber Constants::m_versionNumber = VersionNumber(VERSION_NUMBER);
#else
  const VersionNumber Constants::m_versionNumber = VersionNumber("0.0.0");
#endif

#ifdef BUILD_NUMBER
  const QString Constants::m_buildNumber = QString(BUILD_NUMBER);
#else
  const QString Constants::m_buildNumber = QString("0");
#endif

#if UPDATE_CHANNEL == 2
  const Constants::UpdateChannel Constants::m_updateChannel =
      UpdateChannel::Release;
  const QString Constants::m_applicationName = QString("PinglyMotion");
#elif UPDATE_CHANNEL == 1
  const Constants::UpdateChannel Constants::m_updateChannel =
      UpdateChannel::Beta;
  const QString Constants::m_applicationName = QString("PinglyMotion-Beta");
#else
  const Constants::UpdateChannel Constants::m_updateChannel =
      UpdateChannel::Development;
  const QString Constants::m_applicationName = QString("PinglyMotion-Development");
#endif

#ifdef PINGLYMOTION_DATAPATH
  const QString Constants::m_pinglyMotionDataPath = QString(PINGLYMOTION_DATAPATH);
#else
  const QString Constants::m_pinglyMotionDataPath = QString();
#endif


// The build source and commit come from a generated header, not from compiler options:
// CMake drops a compiler option that contains "#", so "PR #12" never reached the program.
#include "buildinfo.h"

QString Constants::composeBuildLabel(const QString& version, bool release, const QString& source,
                                     const QString& commit, const QString& build)
{
  if (release) return version;
  const QString dot = QStringLiteral(" \u00b7 ");
  QString label = version + dot + (source.isEmpty() ? QStringLiteral("dev") : source);
  if (!commit.isEmpty()) label += dot + commit;
  if (!build.isEmpty() && build != QStringLiteral("0")) label += dot + QStringLiteral("build ") + build;
  return label;
}

const QString Constants::buildLabel()
{
  return composeBuildLabel(m_versionNumber.toString(), m_updateChannel == UpdateChannel::Release,
                           QString::fromUtf8(PINGLYMOTION_BUILD_SOURCE), QString::fromUtf8(PINGLYMOTION_COMMIT),
                           m_buildNumber);
}

QString Constants::windowTitle(const QString& fileName)
{
  return QStringLiteral("PinglyMotion ") + buildLabel() + QStringLiteral(" [") + fileName + QStringLiteral("]");
}

const QString Constants::buildDateTime()
{
  return m_buildDateTime;
}

const VersionNumber& Constants::versionNumber()
{
  return m_versionNumber;
}

const QString Constants::versionNumberString()
{
  return m_versionNumber.toString();
}

const QString Constants::buildNumber()
{
  return m_buildNumber;
}

const QString Constants::updateChannel()
{
  switch (m_updateChannel)
  {
    case UpdateChannel::Release:
      return "Release";
      break;
    case UpdateChannel::Beta:
      return "Beta";
      break;
    default:
      return "Development";
      break;
  }
}

const QString Constants::applicationName()
{
  return m_applicationName;
}

const QString Constants::organizationName()
{
  return QString("PinglyMotion");
}

const QString Constants::organizationDomain()
{
  return QString("pingledingle-tech.github.io");
}

const QString Constants::pinglyMotionDataPath()
{
  if (!m_pinglyMotionDataPath.isNull())
  {
    return m_pinglyMotionDataPath;
  }
  else
  {
#  ifdef __APPLE__
    return QString(QApplication::applicationDirPath() + "/../Resources");
#  else
    // next to the program: the Windows zip, or a copied build
    const QString besideProgram = QCoreApplication::applicationDirPath() + "/data";
#    ifdef PINGLYMOTION_SOURCE_DATAPATH
    // a program run from its build folder uses the data in the source tree
    if (!QDir(besideProgram).exists())
      return QString(PINGLYMOTION_SOURCE_DATAPATH);
#    endif
    // never the current folder: a data folder planted there would be read
    return besideProgram;
#  endif
  }
}

const QString Constants::defaultPosePath()
{
  return pinglyMotionDataPath() + "/TPose.dingle";
}

const QString Constants::limitsFilePath()
{
  return pinglyMotionDataPath() + "/SL.lim";
}

const QString Constants::maleBvhPath()
{
  return pinglyMotionDataPath() + "/SLMale.bvh";
}

const QString Constants::femaleBvhPath()
{
  return pinglyMotionDataPath() + "/SLFemale.bvh";
}

const QString Constants::cubeMeshFilePath()
{
  return pinglyMotionDataPath() + "/cube.obj";
}

const QString Constants::torusMeshFilePath()
{
  return pinglyMotionDataPath() + "/torus.obj";
}

int Constants::keyWidth()
{
  return 10;
}

int Constants::keyHeight()
{
  // grows with the text size (src/textsize.h), so the joint names fit their rows
  return TextSize::scaled(10);
}

int Constants::lineHeight()
{
  // grows with the text size (src/textsize.h), so the joint names fit their rows
  return TextSize::scaled(11);
}

int Constants::leftStrut()
{
  // grows with the text size (src/textsize.h), so the joint names fit their rows
  return TextSize::scaled(120);
}

int Constants::m_numParts = 25;

int Constants::numParts()
{
  return m_numParts;
}

void Constants::setNumParts(int parts)
{
  m_numParts = parts;
}
