/*
 * PinglyMotion - the dialog and the plain-words helpers for File > Export as
 * Second Life .anim (issue #10, menu step)
 *
 * The helpers (namespace AnimExportUi) have no window and no disk access, so
 * they can be tested. The dialog only asks for the settings that end up in
 * the file. The writer itself is src/animexport.*.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef ANIMEXPORTDIALOG_H
#define ANIMEXPORTDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

#include "animexport.h"
#include "animfile.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

namespace AnimExportUi
{
  // what the user chooses in the dialog
  struct Settings
  {
    int priority=2;            // 0 to 6
    double slowStart=0.3;      // seconds
    double slowFinish=0.3;
    bool loop=false;
    int handPose=1;            // 0 to 13
  };

  // the playing time of the animation: frame 0 is a reference frame and is not played
  double durationSeconds(const AnimFile::Animation& anim);

  // "Length: 4.2 seconds. Second Life accepts up to 60 seconds."
  QString lengthText(double seconds);

  // empty when the animation can be exported; otherwise what is wrong and how to fix it
  QString problemBeforeExport(const AnimFile::Animation& anim);

  // the message of a failed export, with a line that says what to do
  QString explainFailure(const QString& writerError);

  // a suggested file name: the open file's name with .anim, or pingle.anim
  QString suggestedFileName(const QString& currentFile,const QString& untitledName);

  // the file name with .anim added when it has no ending
  QString withAnimEnding(const QString& fileName);

  // the settings as options of the writer. loopInFrame and loopOutFrame are
  // the animation's loop points (frames, as the program stores them).
  AnimExport::Options toOptions(const Settings& settings,double frameTime,int loopInFrame,int loopOutFrame);

  // the names of the 14 hand poses, in the order of the file format
  QStringList handPoseNames();
}

class AnimExportDialog : public QDialog
{
  Q_OBJECT

  public:
    AnimExportDialog(double seconds,QWidget* parent=nullptr);
    AnimExportUi::Settings settings() const;

  private:
    QSpinBox* priority;
    QDoubleSpinBox* slowStart;
    QDoubleSpinBox* slowFinish;
    QCheckBox* loop;
    QComboBox* handPose;
};

#endif
