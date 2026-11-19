/*
 * PinglyMotion - the dialog and the plain-words helpers for File > Import
 * glTF animation (issue #12, menu step)
 *
 * A glTF file can hold several animations and several skeletons. The dialog
 * asks which ones to use (the window skips it when there is nothing to
 * choose). The helpers have no window, so they can be tested. The reader is
 * src/gltfimport.*; the list of matched joints is the dialog of the BVH import
 * (src/retargetdialog.*).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef GLTFIMPORTDIALOG_H
#define GLTFIMPORTDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

#include "gltfimport.h"

class QComboBox;

namespace GltfUi
{
  // "Walk (1.7 seconds)", or "Animation 2 (3.0 seconds)" when it has no name
  QStringList animationLabels(const GltfImport::Document& document);

  // "Body", or "Skeleton 2" when it has no name
  QStringList skeletonLabels(const GltfImport::Document& document);

  // true when the file has more than one animation or more than one skeleton
  bool needsChoice(const GltfImport::Document& document);

  // the message for a file that could not be read, with a line that says what to do
  QString readProblem(const QString& readerError);

  // the message for an animation that could not be turned into a skeleton animation
  QString animationProblem(const QString& error);
}

class GltfImportDialog : public QDialog
{
  Q_OBJECT

  public:
    GltfImportDialog(const GltfImport::Document& document,QWidget* parent=nullptr);

    int animation() const;
    int skeleton() const;

  private:
    QComboBox* m_animations;
    QComboBox* m_skeletons;
};

#endif
