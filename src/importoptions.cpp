/*
 * PinglyMotion - the clean-up choices of the import dialog (issue #17)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "importoptions.h"

#include <QCoreApplication>
#include <QSettings>

namespace ImportOptions
{
  bool CleanupOptions::operator==(const CleanupOptions& other) const
  {
    return resample==other.resample && startPose==other.startPose && inPlace==other.inPlace &&
           loopSeam==other.loopSeam && smooth==other.smooth;
  }

  static QString tr(const char* text)
  {
    return QCoreApplication::translate("ImportOptions",text);
  }

  const QList<Row>& rows()
  {
    static const QList<Row> list=
    {
      {QStringLiteral("resample"),
       tr("Make it 30 frames a second"),
       tr("Second Life plays 30 frames a second. The movement looks the same."),
       &CleanupOptions::resample},
      {QStringLiteral("start_pose"),
       tr("Add a start pose at frame 1"),
       tr("Second Life does not play frame 1. This makes every moving joint count."),
       &CleanupOptions::startPose},
      {QStringLiteral("in_place"),
       tr("Keep it in one place"),
       tr("Takes away the walk forward. The sway of the hips stays."),
       &CleanupOptions::inPlace},
      {QStringLiteral("loop_seam"),
       tr("Make the end match the start"),
       tr("Helps a loop run without a jump. The last frames change a little."),
       &CleanupOptions::loopSeam},
      {QStringLiteral("smooth"),
       tr("Smooth out the shake"),
       tr("Light smoothing for video capture. Fast small movements get softer."),
       &CleanupOptions::smooth},
    };
    return list;
  }

  MotionCleanup::Result apply(const AnimFile::Animation& anim,const CleanupOptions& options)
  {
    MotionCleanup::Result current;
    current.animation=anim;

    using Step=MotionCleanup::Result(*)(const AnimFile::Animation&);
    const struct { bool on; Step run; } steps[]=
    {
      {options.resample,[](const AnimFile::Animation& a){ return MotionCleanup::resample(a); }},
      {options.smooth,[](const AnimFile::Animation& a){ return MotionCleanup::smooth(a); }},
      {options.inPlace,[](const AnimFile::Animation& a){ return MotionCleanup::inPlace(a); }},
      // foot lock goes here
      {options.loopSeam,[](const AnimFile::Animation& a){ return MotionCleanup::blendLoopSeam(a); }},
      {options.startPose,[](const AnimFile::Animation& a){ return MotionCleanup::insertStartPose(a); }},
    };
    for(const auto& step : steps)
    {
      if(!step.on) continue;
      MotionCleanup::Result next=step.run(*current.animation);
      if(!next.ok()) return next;
      current=std::move(next);
    }
    return current;
  }

  CleanupOptions load(QSettings& settings)
  {
    CleanupOptions options;
    settings.beginGroup(QStringLiteral("import_cleanup"));
    for(const Row& row : rows())
    {
      const QVariant value=settings.value(row.key);
      const QString text=value.toString().toLower();
      if(text==QLatin1String("true")) options.*row.field=true;
      else if(text==QLatin1String("false")) options.*row.field=false;
    }
    settings.endGroup();
    return options;
  }

  void save(QSettings& settings,const CleanupOptions& options)
  {
    settings.beginGroup(QStringLiteral("import_cleanup"));
    for(const Row& row : rows())
      settings.setValue(row.key,options.*row.field);
    settings.endGroup();
    settings.sync();
  }

  static QSettings programSettings()
  {
    return QSettings(QSettings::IniFormat,QSettings::UserScope,QCoreApplication::organizationName(),QCoreApplication::applicationName());
  }

  CleanupOptions load()
  {
    QSettings settings=programSettings();
    return load(settings);
  }

  void save(const CleanupOptions& options)
  {
    QSettings settings=programSettings();
    save(settings,options);
  }
}
