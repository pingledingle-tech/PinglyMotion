/*
 * PinglyMotion - the dialog that shows how a BVH from another skeleton was
 * matched to the avatar (issue #16, menu step). See retargetdialog.h.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "retargetdialog.h"

namespace RetargetUi
{
  QVector<Row> rows(const Retarget::Mapping& mapping,const AnimFile::Animation& source,
                    const AnimFile::Animation& target,
                    const std::function<QString(const QString&)>& friendly)
  {
    QVector<Row> list;
    for(int i=0;i<source.joints.size();i++)
    {
      if(source.joints[i].kind==AnimFile::JointKind::EndSite) continue;
      Row row;
      row.source=source.joints[i].name;

      const Retarget::BoneMatch* bone=nullptr;
      for(const Retarget::BoneMatch& b : mapping.bones)
        if(b.joint==i) { bone=&b; break; }

      int targetJoint=-1;
      for(const QPair<int,int>& pair : mapping.pairs)
        if(pair.first==i) { targetJoint=pair.second; break; }

      if(targetJoint>=0 && targetJoint<target.joints.size())
      {
        const QString name=target.joints[targetJoint].name;
        const QString shown=friendly ? friendly(name) : QString();
        row.target=shown.isEmpty() ? name : shown;
        row.what=QObject::tr("Matched");
        row.matched=true;
      }
      else
      {
        QString reason=bone ? bone->reason : QString();
        row.what=reason.isEmpty() ? QObject::tr("Left out") : QObject::tr("Left out (%1)").arg(reason);
      }
      list.append(row);
    }
    return list;
  }

  QString summary(const QVector<Row>& rows)
  {
    int matched=0;
    for(const Row& row : rows) if(row.matched) matched++;
    return QObject::tr("%1 of %2 joints matched. %3 were left out.")
           .arg(matched).arg(rows.size()).arg(rows.size()-matched);
  }

  QString problem(const Retarget::Result& result)
  {
    if(result.ok()) return QString();
    if(!result.mapping.missingRequired.isEmpty())
      return QObject::tr("Your file has no joints for: %1.\nPick a file for a skeleton with a body, arms and legs.")
             .arg(result.mapping.missingRequired.join(QStringLiteral(", ")));
    return QObject::tr("This file could not be moved onto the avatar.\n%1\nTry a BVH file for a person with arms and legs.")
           .arg(result.error);
  }
}

RetargetDialog::RetargetDialog(const AnimFile::Animation& source,const AnimFile::Animation& target,
                               const QString& fileName,
                               const std::function<QString(const QString&)>& friendly,QWidget* parent)
  : QDialog(parent),
    m_source(source),
    m_target(target),
    m_friendly(friendly)
{
  setWindowTitle(tr("Import a BVH from another skeleton"));
  resize(560,480);

  QVBoxLayout* layout=new QVBoxLayout(this);
  QLabel* intro=new QLabel(tr("%1 has its own skeleton.\nThe program matched its joints to the avatar's joints.").arg(fileName),this);
  intro->setTextFormat(Qt::PlainText);
  layout->addWidget(intro);

  m_summary=new QLabel(this);
  QFont bold=m_summary->font();
  bold.setBold(true);
  m_summary->setFont(bold);
  layout->addWidget(m_summary);

  m_problem=new QLabel(this);
  m_problem->setWordWrap(true);
  m_problem->setStyleSheet(QStringLiteral("color: #b00020;"));
  layout->addWidget(m_problem);

  m_notes=new QLabel(this);
  m_notes->setWordWrap(true);
  m_notes->setTextFormat(Qt::PlainText);
  m_notes->hide();
  layout->addWidget(m_notes);

  m_table=new QTableWidget(this);
  m_table->setColumnCount(3);
  m_table->setHorizontalHeaderLabels({tr("Joint in your file"),tr("Joint on the avatar"),tr("What happens")});
  m_table->horizontalHeader()->setStretchLastSection(true);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setSelectionMode(QAbstractItemView::NoSelection);
  m_table->verticalHeader()->setVisible(false);
  layout->addWidget(m_table,1);

  m_startsInPose=new QCheckBox(tr("My file starts in a T-pose or A-pose"),this);
  m_startsInPose->setToolTip(tr("Tick this if the first frame is a standing pose with the arms out."));
  connect(m_startsInPose,&QCheckBox::toggled,this,&RetargetDialog::run);
  layout->addWidget(m_startsInPose);

  QDialogButtonBox* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel,this);
  m_open=buttons->addButton(tr("Open the animation"),QDialogButtonBox::AcceptRole);
  m_open->setDefault(true);
  connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  layout->addWidget(buttons);

  run();
}

void RetargetDialog::setNotes(const QStringList& notes)
{
  m_notes->setText(notes.isEmpty() ? QString() : tr("Please note:")+QStringLiteral("\n- ")+notes.join(QStringLiteral("\n- ")));
  m_notes->setVisible(!notes.isEmpty());
}

void RetargetDialog::run()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  Retarget::Options options;
  options.firstFrameAsRest=m_startsInPose->isChecked();
  m_result=Retarget::retarget(m_source,m_target,options);
  QApplication::restoreOverrideCursor();

  const QVector<RetargetUi::Row> list=RetargetUi::rows(m_result.mapping,m_source,m_target,m_friendly);
  m_table->setRowCount(list.size());
  for(int i=0;i<list.size();i++)
  {
    m_table->setItem(i,0,new QTableWidgetItem(list[i].source));
    m_table->setItem(i,1,new QTableWidgetItem(list[i].target));
    m_table->setItem(i,2,new QTableWidgetItem(list[i].what));
  }
  m_table->resizeColumnsToContents();
  m_table->horizontalHeader()->setStretchLastSection(true);

  m_summary->setText(RetargetUi::summary(list));
  const QString problem=RetargetUi::problem(m_result);
  m_problem->setText(problem);
  m_problem->setVisible(!problem.isEmpty());
  m_open->setEnabled(problem.isEmpty());
}
