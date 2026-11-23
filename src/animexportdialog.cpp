/*
 * PinglyMotion - the dialog and the plain-words helpers for File > Export as
 * Second Life .anim (issue #10, menu step). See animexportdialog.h.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>

#include "animexportdialog.h"
#include "dialogfit.h"

namespace AnimExportUi
{
  double durationSeconds(const AnimFile::Animation& anim)
  {
    if(anim.frames<1) return 0.0;
    return std::max(anim.frames-2,1)*anim.frameTime;
  }

  QString lengthText(double seconds)
  {
    return QObject::tr("Length: %1 seconds. Second Life accepts up to %2 seconds.")
           .arg(seconds,0,'f',1).arg(AnimExport::MAX_DURATION,0,'f',0);
  }

  QString problemBeforeExport(const AnimFile::Animation& anim)
  {
    if(anim.joints.isEmpty() || anim.frames<1 || anim.valuesPerFrame<1)
      return QObject::tr("There is nothing to export yet.\nOpen a file or make a pose, then try again.");

    const double seconds=durationSeconds(anim);
    if(!(seconds>0.0))
      return QObject::tr("The animation has no length.\nAdd more frames, then try again.");
    if(seconds>AnimExport::MAX_DURATION)
      return QObject::tr("This animation is too long to export.\n"
                         "It is %1 seconds. Second Life accepts up to %2.\n"
                         "Shorten it, or delete keys at the end.")
             .arg(seconds,0,'f',1).arg(AnimExport::MAX_DURATION,0,'f',0);
    return QString();
  }

  QString explainFailure(const QString& writerError)
  {
    return QObject::tr("The file was not saved.\n%1\nCheck the animation, then try again.").arg(writerError);
  }

  QString suggestedFileName(const QString& currentFile,const QString& untitledName)
  {
    if(currentFile.isEmpty() || currentFile==untitledName)
      return QStringLiteral("pingle.anim");
    const QFileInfo info(currentFile);
    return info.path()+QLatin1Char('/')+info.completeBaseName()+QStringLiteral(".anim");
  }

  QString withAnimEnding(const QString& fileName)
  {
    if(QFileInfo(fileName).suffix().toLower()==QLatin1String("anim")) return fileName;
    return fileName+QStringLiteral(".anim");
  }

  AnimExport::Options toOptions(const Settings& settings,double frameTime,int loopInFrame,int loopOutFrame)
  {
    AnimExport::Options options;
    options.basePriority=std::min(std::max(settings.priority,0),AnimExport::MAX_PRIORITY);
    options.easeIn=std::max(settings.slowStart,0.0);
    options.easeOut=std::max(settings.slowFinish,0.0);
    options.handPose=std::min(std::max(settings.handPose,0),AnimExport::MAX_HAND_POSE);
    options.loop=settings.loop;
    if(settings.loop)
    {
      // frame 0 is the reference frame, so the first played frame is frame 1 at time 0
      options.loopIn=std::max(loopInFrame-1,0)*frameTime;
      options.loopOut=std::max(loopOutFrame-1,0)*frameTime;
    }
    return options;
  }

  QStringList handPoseNames()
  {
    return {QObject::tr("Fingers spread"),
            QObject::tr("Relaxed"),
            QObject::tr("Pointing"),
            QObject::tr("Fist"),
            QObject::tr("Relaxed, left hand only"),
            QObject::tr("Pointing, left hand only"),
            QObject::tr("Fist, left hand only"),
            QObject::tr("Relaxed, right hand only"),
            QObject::tr("Pointing, right hand only"),
            QObject::tr("Fist, right hand only"),
            QObject::tr("Salute, right hand"),
            QObject::tr("Typing"),
            QObject::tr("Peace sign, right hand"),
            QObject::tr("Flat hand, right hand")};
  }
}

AnimExportDialog::AnimExportDialog(double seconds,QWidget* parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Export as Second Life .anim"));

  QVBoxLayout* layout=DialogFit::contentLayout(this);

  layout->addWidget(new QLabel(AnimExportUi::lengthText(seconds),this));

  QFormLayout* form=new QFormLayout();
  layout->addLayout(form);

  priority=new QSpinBox(this);
  priority->setRange(0,AnimExport::MAX_PRIORITY);
  priority->setValue(2);
  priority->setToolTip(tr("When two animations move the same joint,\nthe higher number wins."));
  form->addRow(tr("Priority (0 to 6):"),priority);

  slowStart=new QDoubleSpinBox(this);
  slowStart->setRange(0.0,AnimExport::MAX_DURATION);
  slowStart->setDecimals(1);
  slowStart->setSingleStep(0.1);
  slowStart->setSuffix(tr(" seconds"));
  slowStart->setValue(0.3);
  slowStart->setToolTip(tr("How long the animation takes to fade in."));
  form->addRow(tr("Slow start:"),slowStart);

  slowFinish=new QDoubleSpinBox(this);
  slowFinish->setRange(0.0,AnimExport::MAX_DURATION);
  slowFinish->setDecimals(1);
  slowFinish->setSingleStep(0.1);
  slowFinish->setSuffix(tr(" seconds"));
  slowFinish->setValue(0.3);
  slowFinish->setToolTip(tr("How long the animation takes to fade out."));
  form->addRow(tr("Slow finish:"),slowFinish);

  loop=new QCheckBox(tr("Play again and again"),this);
  loop->setToolTip(tr("The loop points of the animation are used."));
  form->addRow(QString(),loop);

  handPose=new QComboBox(this);
  handPose->addItems(AnimExportUi::handPoseNames());
  handPose->setCurrentIndex(1);
  handPose->setToolTip(tr("How the fingers look while it plays."));
  form->addRow(tr("Hands:"),handPose);

  QDialogButtonBox* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel,this);
  QPushButton* ok=buttons->addButton(tr("Choose file..."),QDialogButtonBox::AcceptRole);
  ok->setDefault(true);
  connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  DialogFit::finish(this,buttons);
}

AnimExportUi::Settings AnimExportDialog::settings() const
{
  AnimExportUi::Settings result;
  result.priority=priority->value();
  result.slowStart=slowStart->value();
  result.slowFinish=slowFinish->value();
  result.loop=loop->isChecked();
  result.handPose=handPose->currentIndex();
  return result;
}
