/*
 * PinglyMotion - the dialog that shows how a BVH from another skeleton was
 * matched to the avatar (issue #16, menu step)
 *
 * The helpers (namespace RetargetUi) turn the result of the retargeting core
 * (src/retarget.*) into plain rows and sentences, with no window, so they can
 * be tested. The dialog runs the retargeting, shows which joints were matched
 * and which were left out, and lets the user open the result.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef RETARGETDIALOG_H
#define RETARGETDIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>

#include <functional>

#include "animfile.h"
#include "retarget.h"

class QCheckBox;
class QLabel;
class QPushButton;
class QTableWidget;

namespace RetargetUi
{
  struct Row
  {
    QString source;       // joint name in the user's file
    QString target;       // joint on the avatar, empty when left out
    QString what;         // "Matched" or "Left out (twist bone)"
    bool matched=false;
  };

  // one row for every joint of the file (End Sites are skipped). friendly turns
  // a joint name of the avatar skeleton into the name the program shows.
  QVector<Row> rows(const Retarget::Mapping& mapping,const AnimFile::Animation& source,
                    const AnimFile::Animation& target,
                    const std::function<QString(const QString&)>& friendly=nullptr);

  // "12 of 31 joints matched. 19 were left out."
  QString summary(const QVector<Row>& rows);

  // empty when the result can be opened; otherwise what is wrong and what to do
  QString problem(const Retarget::Result& result);
}

class RetargetDialog : public QDialog
{
  Q_OBJECT

  public:
    RetargetDialog(const AnimFile::Animation& source,const AnimFile::Animation& target,
                   const QString& fileName,
                   const std::function<QString(const QString&)>& friendly,QWidget* parent=nullptr);

    // the retargeted animation, valid after the dialog was accepted
    const Retarget::Result& result() const { return m_result; }

  private slots:
    void run();

  private:
    AnimFile::Animation m_source;
    AnimFile::Animation m_target;
    std::function<QString(const QString&)> m_friendly;
    Retarget::Result m_result;

    QLabel* m_summary;
    QLabel* m_problem;
    QTableWidget* m_table;
    QCheckBox* m_startsInPose;
    QPushButton* m_open;
};

#endif
