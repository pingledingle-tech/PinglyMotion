/*
 * PinglyMotion - the dialog and the plain-words helpers for File > Import
 * glTF animation (issue #12, menu step). See gltfimportdialog.h.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>

#include "gltfimportdialog.h"
#include "dialogfit.h"

namespace GltfUi
{
  QStringList animationLabels(const GltfImport::Document& document)
  {
    QStringList labels;
    for(int i=0;i<document.animations.size();i++)
    {
      const GltfImport::Clip& clip=document.animations[i];
      const QString name=clip.name.isEmpty() ? QObject::tr("Animation %1").arg(i+1) : clip.name;
      labels << QObject::tr("%1 (%2 seconds)").arg(name).arg(clip.end-clip.start,0,'f',1);
    }
    return labels;
  }

  QStringList skeletonLabels(const GltfImport::Document& document)
  {
    QStringList labels;
    for(int i=0;i<document.skins.size();i++)
      labels << (document.skins[i].name.isEmpty() ? QObject::tr("Skeleton %1").arg(i+1) : document.skins[i].name);
    return labels;
  }

  bool needsChoice(const GltfImport::Document& document)
  {
    return document.animations.size()>1 || document.skins.size()>1;
  }

  QString readProblem(const QString& readerError)
  {
    return QObject::tr("This file could not be read.\n%1\nCheck that it is a glTF or GLB file with an animation.").arg(readerError);
  }

  QString animationProblem(const QString& error)
  {
    return QObject::tr("This animation could not be used.\n%1\nPick another animation or another file.").arg(error);
  }
}

GltfImportDialog::GltfImportDialog(const GltfImport::Document& document,QWidget* parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Import glTF animation"));

  QVBoxLayout* layout=DialogFit::contentLayout(this);
  layout->addWidget(new QLabel(tr("This file holds more than one choice.\nPick the ones you want."),this));

  QFormLayout* form=new QFormLayout();
  layout->addLayout(form);

  m_animations=new QComboBox(this);
  m_animations->addItems(GltfUi::animationLabels(document));
  form->addRow(tr("Animation:"),m_animations);

  m_skeletons=new QComboBox(this);
  m_skeletons->addItems(GltfUi::skeletonLabels(document));
  m_skeletons->setEnabled(document.skins.size()>1);
  form->addRow(tr("Skeleton:"),m_skeletons);

  QDialogButtonBox* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);
  connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
  DialogFit::finish(this,buttons);
}

int GltfImportDialog::animation() const
{
  return std::max(m_animations->currentIndex(),0);
}

int GltfImportDialog::skeleton() const
{
  return std::max(m_skeletons->currentIndex(),0);
}
