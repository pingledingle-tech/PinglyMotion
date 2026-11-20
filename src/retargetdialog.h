/*
 * PinglyMotion - the dialog that shows how a BVH from another skeleton was
 * matched to the avatar (issue #16, menu step)
 *
 * The helpers (namespace RetargetUi) turn the result of the retargeting core
 * (src/retarget.*) into plain rows and sentences, with no window, so they can
 * be tested. The dialog runs the retargeting, shows which joints were matched
 * and which were left out, lets the user give a joint another avatar joint or
 * leave it out (issue #16, step 4), and lets the user open the result. The
 * hand-made changes are remembered per source skeleton with the store of
 * src/retargetmapping.* (RetargetMapping::Editor holds them).
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
#include <QStringList>
#include <QVector>

#include <functional>

#include "animfile.h"
#include "importoptions.h"
#include "retarget.h"
#include "retargetmapping.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QTableWidget;

namespace RetargetUi
{
  struct Row
  {
    QString source;       // joint name in the user's file
    QString target;       // joint on the avatar, empty when left out
    QString what;         // "Matched", "Left out (twist bone)", "Changed by you"
    bool matched=false;
    int joint=-1;         // index of the joint in the user's file
    bool byHand=false;    // the user changed this joint in the dialog
  };

  // one row for every joint of the file (End Sites are skipped). friendly turns
  // a joint name of the avatar skeleton into the name the program shows.
  QVector<Row> rows(const Retarget::Mapping& mapping,const AnimFile::Animation& source,
                    const AnimFile::Animation& target,
                    const std::function<QString(const QString&)>& friendly=nullptr);

  // the rows for the mapping of an editor: as above, and the joints the user
  // changed say "Changed by you" or "Left out by you"
  QVector<Row> rows(const RetargetMapping::Editor& editor,const AnimFile::Animation& source,
                    const AnimFile::Animation& target,
                    const std::function<QString(const QString&)>& friendly=nullptr);

  // "12 of 31 joints matched. 19 were left out." and, when the user changed
  // joints, " You changed 2."
  QString summary(const QVector<Row>& rows);

  // one entry of the list of avatar joints in the dialog
  struct Choice
  {
    QString label;        // the name the program shows
    QString name;         // the joint name of the avatar skeleton (tool tip)
    int joint=-1;         // index in the avatar skeleton, -1 for "Leave out"
    bool separatorBefore=false;
  };
  // the list a joint can be given: "Leave out" first, then the body joints,
  // then the other joints of the avatar (face, wings, tail) after a separator
  QVector<Choice> choices(const RetargetMapping::Editor& editor,const AnimFile::Animation& target,
                          const std::function<QString(const QString&)>& friendly=nullptr);

  // empty when the result can be opened; otherwise what is wrong and what to
  // do. changedByHand: the mapping has the user's changes, so the advice is
  // to change them, not to pick another file
  QString problem(const Retarget::Result& result,bool changedByHand=false);
}

class RetargetDialog : public QDialog
{
  Q_OBJECT

  public:
    RetargetDialog(const AnimFile::Animation& source,const AnimFile::Animation& target,
                   const QString& fileName,
                   const std::function<QString(const QString&)>& friendly,QWidget* parent=nullptr);

    // the animation to open, valid after the dialog was accepted: the
    // retargeted one with the chosen clean-up steps applied (src/importoptions.*)
    const Retarget::Result& result() const { return m_final; }

    // the retargeted animation before the clean-up
    const Retarget::Result& retargeted() const { return m_result; }

    // the clean-up choices shown (for tests); setting them runs the steps again
    ImportOptions::CleanupOptions cleanupOptions() const;
    void setCleanupOptions(const ImportOptions::CleanupOptions& options);

    // things the reader left out or changed (for example from a glTF file), shown under the summary
    void setNotes(const QStringList& notes);

    // the state of the hand-made changes (for tests)
    const RetargetMapping::Editor& editor() const { return m_editor; }
    // the folder the hand-made changes are remembered in; the default is the
    // user's folder of RetargetMapping::Store. Set it before exec().
    void setMappingFolder(const QString& folder);

  public slots:
    // save the changes when "Remember" is ticked, then close with Accepted
    void accept() override;

  private slots:
    void run();
    void choose(int row);
    void useAutomatic();

  private:
    void loadSaved();
    void fillTable();
    void updateTable();

    AnimFile::Animation m_source;
    AnimFile::Animation m_target;
    std::function<QString(const QString&)> m_friendly;
    Retarget::Result m_result;
    RetargetMapping::Editor m_editor;
    RetargetMapping::Store m_store;
    QVector<RetargetUi::Choice> m_choices;
    QVector<QComboBox*> m_combos;
    QVector<int> m_rowJoint;   // joint of the user's file per table row
    QStringList m_readerNotes;
    QStringList m_mappingNotes;

    ImportOptions::CleanupOptions m_cleanup;
    Retarget::Result m_final;               // m_result with the clean-up applied
    QVector<QCheckBox*> m_cleanupBoxes;     // one per ImportOptions::rows(), same order
    QVBoxLayout* m_cleanupLayout;           // foot lock gets its box here (docs: top of importoptions.h)

    QLabel* m_summary;
    QLabel* m_saved;
    QLabel* m_problem;
    QLabel* m_notes;
    QTableWidget* m_table;
    QCheckBox* m_startsInPose;
    QCheckBox* m_remember;
    QPushButton* m_automatic;
    QPushButton* m_open;
};

#endif
