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
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

#include "retargetdialog.h"
#include "dialogfit.h"

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
      row.joint=i;

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

  QVector<Row> rows(const RetargetMapping::Editor& editor,const AnimFile::Animation& source,
                    const AnimFile::Animation& target,
                    const std::function<QString(const QString&)>& friendly)
  {
    QVector<Row> list=rows(editor.current(),source,target,friendly);
    for(Row& row : list)
    {
      if(!editor.changedByHand(row.joint)) continue;
      row.byHand=true;
      row.what=row.matched ? QObject::tr("Changed by you") : QObject::tr("Left out by you");
    }
    return list;
  }

  QString summary(const QVector<Row>& rows)
  {
    int matched=0;
    int byHand=0;
    for(const Row& row : rows)
    {
      if(row.matched) matched++;
      if(row.byHand) byHand++;
    }
    QString text=QObject::tr("%1 of %2 joints matched. %3 were left out.")
                 .arg(matched).arg(rows.size()).arg(rows.size()-matched);
    if(byHand>0) text+=QLatin1Char(' ')+QObject::tr("You changed %1.").arg(byHand);
    return text;
  }

  QVector<Choice> choices(const RetargetMapping::Editor& editor,const AnimFile::Animation& target,
                          const std::function<QString(const QString&)>& friendly)
  {
    QVector<Choice> list;
    Choice none;
    none.label=QObject::tr("(leave out)");
    list << none;
    for(int pass=0;pass<2;pass++)
    {
      bool first=true;
      for(int t : editor.choices())
      {
        if(editor.isBodyJoint(t)!=(pass==0)) continue;
        Choice c;
        c.name=target.joints[t].name;
        const QString shown=friendly ? friendly(c.name) : QString();
        c.label=shown.isEmpty() ? c.name : shown;
        c.joint=t;
        c.separatorBefore=(pass==1 && first);
        first=false;
        list << c;
      }
    }
    return list;
  }

  QString problem(const Retarget::Result& result,bool changedByHand)
  {
    if(result.ok()) return QString();
    if(changedByHand && !result.mapping.missingRequired.isEmpty())
      return QObject::tr("These avatar joints need a joint from your file: %1.\nPick one in the list, or use the automatic match.")
             .arg(result.mapping.missingRequired.join(QStringLiteral(", ")));
    if(changedByHand)
      return QObject::tr("Your changes do not work for this file.\n%1\nChange them, or use the automatic match.")
             .arg(result.error);
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
    m_friendly(friendly),
    m_editor(source,target)
{
  setWindowTitle(tr("Import a BVH from another skeleton"));

  QVBoxLayout* layout=DialogFit::contentLayout(this);
  QLabel* intro=new QLabel(tr("%1 has its own skeleton.\nThe program matched its joints to the avatar's joints.\n"
                              "Is a match wrong? Pick another avatar joint in the list.").arg(fileName),this);
  intro->setTextFormat(Qt::PlainText);
  layout->addWidget(intro);

  m_summary=new QLabel(this);
  QFont bold=m_summary->font();
  bold.setBold(true);
  m_summary->setFont(bold);
  layout->addWidget(m_summary);

  m_saved=new QLabel(this);
  m_saved->setWordWrap(true);
  m_saved->setTextFormat(Qt::PlainText);
  m_saved->hide();
  layout->addWidget(m_saved);

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
  m_table->setTabKeyNavigation(false);   // Tab would go from cell to cell for ever and never reach the buttons (charter P8)
  layout->addWidget(m_table,1);

  m_startsInPose=new QCheckBox(tr("My file starts in a T-pose or A-pose"),this);
  m_startsInPose->setToolTip(tr("Tick this if the first frame is a standing pose with the arms out."));
  connect(m_startsInPose,&QCheckBox::toggled,this,&RetargetDialog::run);
  layout->addWidget(m_startsInPose);

  // clean-up choices, remembered between imports
  m_cleanup=ImportOptions::load();
  QGroupBox* cleanup=new QGroupBox(tr("Clean up the movement"),this);
  m_cleanupLayout=new QVBoxLayout(cleanup);
  for(const ImportOptions::Row& row : ImportOptions::rows())
  {
    QCheckBox* box=new QCheckBox(row.label,cleanup);
    box->setToolTip(row.tip);
    box->setChecked(m_cleanup.*row.field);
    connect(box,&QCheckBox::toggled,this,[this,row](bool on)
    {
      m_cleanup.*row.field=on;
      run();
    });
    m_cleanupLayout->addWidget(box);
    m_cleanupBoxes << box;
  }
  // foot lock: add its check box to m_cleanupLayout here, after "Keep it in one place"
  layout->addWidget(cleanup);

  QHBoxLayout* mappingRow=new QHBoxLayout;
  m_remember=new QCheckBox(tr("Remember my changes for files with this skeleton"),this);
  m_remember->setToolTip(tr("The next file from the same skeleton gets your changes."));
  m_remember->setChecked(true);
  m_remember->setEnabled(!m_editor.fingerprint().isEmpty());
  mappingRow->addWidget(m_remember,1);
  m_automatic=new QPushButton(tr("Use the automatic match"),this);
  m_automatic->setToolTip(tr("Undo all your changes in this list."));
  connect(m_automatic,&QPushButton::clicked,this,&RetargetDialog::useAutomatic);
  mappingRow->addWidget(m_automatic);
  layout->addLayout(mappingRow);

  QDialogButtonBox* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel,this);
  m_open=buttons->addButton(tr("Open the animation"),QDialogButtonBox::AcceptRole);
  m_open->setDefault(true);
  connect(buttons,&QDialogButtonBox::accepted,this,&RetargetDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  DialogFit::finish(this,buttons,QSize(640,520));

  fillTable();
  loadSaved();
  run();
}

ImportOptions::CleanupOptions RetargetDialog::cleanupOptions() const
{
  return m_cleanup;
}

void RetargetDialog::setCleanupOptions(const ImportOptions::CleanupOptions& options)
{
  m_cleanup=options;
  const QList<ImportOptions::Row>& rows=ImportOptions::rows();
  for(int i=0;i<rows.size() && i<m_cleanupBoxes.size();i++)
  {
    const QSignalBlocker block(m_cleanupBoxes[i]);
    m_cleanupBoxes[i]->setChecked(m_cleanup.*rows[i].field);
  }
  run();
}

void RetargetDialog::setNotes(const QStringList& notes)
{
  m_readerNotes=notes;
  const QStringList all=m_readerNotes+m_mappingNotes;
  m_notes->setText(all.isEmpty() ? QString() : tr("Please note:")+QStringLiteral("\n- ")+all.join(QStringLiteral("\n- ")));
  m_notes->setVisible(!all.isEmpty());
}

void RetargetDialog::setMappingFolder(const QString& folder)
{
  m_store=RetargetMapping::Store(folder);
  m_editor.reset();
  loadSaved();
  run();
}

void RetargetDialog::loadSaved()
{
  const RetargetMapping::Editor::Loaded loaded=m_editor.load(m_store);
  m_mappingNotes=loaded.notes;
  if(!loaded.error.isEmpty())
    m_saved->setText(tr("Your saved changes for this skeleton could not be used:")+QLatin1Char('\n')+loaded.error);
  else if(loaded.found)
    m_saved->setText(tr("Your saved changes for this skeleton were used."));
  else
    m_saved->clear();
  m_saved->setVisible(!m_saved->text().isEmpty());
  setNotes(m_readerNotes);
}

void RetargetDialog::fillTable()
{
  m_choices=RetargetUi::choices(m_editor,m_target,m_friendly);
  const QVector<RetargetUi::Row> list=RetargetUi::rows(m_editor,m_source,m_target,m_friendly);
  m_table->setRowCount(list.size());
  m_combos.clear();
  m_rowJoint.clear();
  for(int i=0;i<list.size();i++)
  {
    m_rowJoint << list[i].joint;
    m_table->setItem(i,0,new QTableWidgetItem(list[i].source));
    m_table->setItem(i,2,new QTableWidgetItem(list[i].what));

    QComboBox* combo=new QComboBox(m_table);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    combo->setMaxVisibleItems(20);
    for(const RetargetUi::Choice& c : m_choices)
    {
      if(c.separatorBefore) combo->insertSeparator(combo->count());
      combo->addItem(c.label,c.joint);
      if(!c.name.isEmpty()) combo->setItemData(combo->count()-1,c.name,Qt::ToolTipRole);
    }
    combo->setToolTip(tr("The avatar joint that %1 moves.").arg(list[i].source));
    connect(combo,QOverload<int>::of(&QComboBox::activated),this,[this,i](int){ choose(i); });
    m_table->setCellWidget(i,1,combo);
    m_combos << combo;
  }
  m_table->resizeColumnsToContents();
  m_table->horizontalHeader()->setStretchLastSection(true);
  // Tab goes down the list of joint choices and then on to the boxes below it (the choices are made
  // after the boxes, so they would come last, and in a loop with the table; charter P8)
  QWidget* before=m_table;
  for(QComboBox* combo : m_combos)
  {
    setTabOrder(before,combo);
    before=combo;
  }
  if(m_startsInPose) setTabOrder(before,m_startsInPose);
}

void RetargetDialog::updateTable()
{
  const QVector<RetargetUi::Row> list=RetargetUi::rows(m_editor,m_source,m_target,m_friendly);
  for(int i=0;i<list.size() && i<m_combos.size();i++)
  {
    QComboBox* combo=m_combos[i];
    const QSignalBlocker block(combo);
    const int index=combo->findData(m_editor.targetOf(list[i].joint));
    combo->setCurrentIndex(index<0 ? 0 : index);

    QTableWidgetItem* what=m_table->item(i,2);
    if(!what) { what=new QTableWidgetItem; m_table->setItem(i,2,what); }
    what->setText(list[i].what);
    QFont font=what->font();
    font.setBold(list[i].byHand);
    what->setFont(font);
  }
  m_summary->setText(RetargetUi::summary(list));
  m_automatic->setEnabled(m_editor.hasChanges());
}

void RetargetDialog::choose(int row)
{
  if(row<0 || row>=m_combos.size() || row>=m_rowJoint.size()) return;
  const QVariant data=m_combos[row]->currentData();
  const int target=data.isValid() ? data.toInt() : -1;
  m_editor.setTarget(m_rowJoint[row],target);   // when it is refused, updateTable() shows the old choice
  run();
}

void RetargetDialog::useAutomatic()
{
  m_editor.reset();
  run();
}

void RetargetDialog::accept()
{
  if(m_remember->isChecked() && m_remember->isEnabled())
  {
    const QString version=QCoreApplication::applicationVersion();
    const QString generator=version.isEmpty() ? QStringLiteral("PinglyMotion") : QStringLiteral("PinglyMotion ")+version;
    const QString error=m_editor.save(m_store,generator);
    if(!error.isEmpty())
      QMessageBox::warning(this,windowTitle(),tr("Your changes could not be remembered.\n%1\nThe animation opens anyway.").arg(error));
  }
  ImportOptions::save(m_cleanup);
  QDialog::accept();
}

void RetargetDialog::run()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  Retarget::Options options;
  options.firstFrameAsRest=m_startsInPose->isChecked();
  m_result=Retarget::retarget(m_source,m_target,m_editor.current(),options);
  QString problem=RetargetUi::problem(m_result,m_editor.hasChanges());
  m_final=m_result;
  if(problem.isEmpty())
  {
    const MotionCleanup::Result cleaned=ImportOptions::apply(*m_result.animation,m_cleanup);
    if(cleaned.ok()) m_final.animation=cleaned.animation;
    else
    {
      m_final.animation.reset();
      problem=cleaned.error;
    }
  }
  QApplication::restoreOverrideCursor();

  updateTable();
  m_problem->setText(problem);
  m_problem->setVisible(!problem.isEmpty());
  m_open->setEnabled(problem.isEmpty());
}
