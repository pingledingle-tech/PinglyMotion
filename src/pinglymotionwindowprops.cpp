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

#include <QScopedValueRollback>
#include <QActionGroup>
#include <QMap>
#include <QMenu>
#include <QRegularExpression>
#include <QDockWidget>
#include <QScrollArea>
#include <QStackedWidget>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include "startfolder.h"
#include "logging.h"
#include <QMessageBox>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QScopedPointer>
#include <QSignalBlocker>
#include <QSharedPointer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QTabBar>
#include <QTableWidget>

#include "aboutdialog.h"
#include "animexportdialog.h"
#include "animimportconvert.h"
#include "dinglesave.h"
#include "gltfimport.h"
#include "gltfimportdialog.h"
#include "retargetdialog.h"
#include "animationview.h"
#include "bvh.h"
#include "dingle.h"
#include "easingtiles.h"
#include "propfile.h"
#include "constants.h"
#include "controlapi.h"
#include "controlserver.h"
#include "controlindicator.h"
#include "auroralink.h"
#include "sliderscale.h"
#include <QStatusBar>
#include "prop.h"
#include "propedit.h"
#include "propmanager.h"
#include "panelcontent.h"
#include "selectedproppanel.h"
#include "undohistory.h"
#include "jointgroups.h"
#include "jointinfo.h"
#include "jointlist.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "paneltitlebar.h"
#include "pinglymotion.h"
#include "scene.h"
#include "timeline.h"
#include "timelineview.h"
#include "updatersettings.h"
#include "usersettings.h"
#include "usersettingsdialog.h"

#include "panelvisibility.h"
#include "pinglymotionwindow.h"
#include "selectedjointpanel.h"
#include "playback.h"
#include "theme.h"
#include "ui_pinglymotionwindow.h"
#include "whatsnewdialog.h"
#include "wheelguard.h"

// What the remote control commands do in the window: the same steps as clicking.

// Part of PinglyMotionWindow: the prop panel and the avatar rotation and position switches.
// (Split out of pinglymotionwindow.cpp; nothing here changes what the window does.)

void PinglyMotionWindow::enableProps(bool state)
{
  if(m_selectedProp) m_selectedProp->setEnabled(state);
  ui->propPositionGroup->setEnabled(state);
  ui->propScaleGroup->setEnabled(state);
  ui->propRotationGroup->setEnabled(state);
  ui->attachToLabel->setEnabled(state);
  ui->attachToComboBox->setEnabled(state);
}

// the Selected joint panel greys its sliders with the reason (UX phase 2b)
void PinglyMotionWindow::enableAvatarRotation(bool state)
{
  if(m_selectedJoint) m_selectedJoint->setTurnBlocked(state ? QString() : jointBlockedReason());
}

void PinglyMotionWindow::enableAvatarPosition(bool state)
{
  if(m_selectedJoint) m_selectedJoint->setMoveBlocked(state ? QString() : jointBlockedReason());
}

// the prop's place, turn and size changed somewhere (a click, a script, Undo): the Selected prop
// page follows (Mockup 2; the designer file's old prop boxes are no longer shown)
void PinglyMotionWindow::updatePropSpins(const QVector3D&, const QVector3D&, const QVector3D&)
{
  showSelectedProp();
}

void PinglyMotionWindow::onNewProp(const QString& propName)
{
  ui->propNameCombo->addItem(propName);
  ui->propNameCombo->setCurrentIndex(ui->propNameCombo->count() - 1);
  selectProp();
  ui->attachToComboBox->setCurrentIndex(0);
}

void PinglyMotionWindow::onSelectProp(bool isSelected, int attachmentPoint)
{
  ui->propNameCombo->setEnabled(isSelected);
  ui->deletePropButton->setEnabled(isSelected);
  if (isSelected)
  {
    ui->attachToComboBox->setCurrentIndex(attachmentPoint);
  }
  ui->animationView->selectProp();
  showSelectedProp();
}

void PinglyMotionWindow::onDeleteProp(const QString propName)
{
  for (int index = 0; index < ui->propNameCombo->count(); index++)
  {
    if (ui->propNameCombo->itemText(index) == propName)
    {
      ui->propNameCombo->removeItem(index);
      selectProp();
    }
  }
  showSelectedProp();
}

void PinglyMotionWindow::onClearProps()
{
  ui->propNameCombo->clear();
  // the props are gone, and so are their Undo steps
  if(m_undo) m_undo->clearSteps();
  showSelectedProp();
}

void PinglyMotionWindow::selectProp()
{
  emit selectProp(ui->propNameCombo->currentIndex());
}

void PinglyMotionWindow::on_newBoxPropButton_clicked()
{
  emit newProp(Prop::Box, cubeMesh);
}

void PinglyMotionWindow::on_newSpherePropButton_clicked()
{
  emit newProp(Prop::Sphere, sphereMesh);
}

void PinglyMotionWindow::on_newConePropButton_clicked()
{
  emit newProp(Prop::Cone, coneMesh);
}

void PinglyMotionWindow::on_newTorusPropButton_clicked()
{
  emit newProp(Prop::Torus, torusMesh);
}

void PinglyMotionWindow::on_propNameCombo_activated(int)
{
  selectProp();
}

void PinglyMotionWindow::on_deletePropButton_clicked()
{
  emit deleteProp();
}

void PinglyMotionWindow::on_attachToComboBox_activated(int attachmentPoint)
{
  // FIXME: find better solution for filtering endpoint for joints
  if (ui->attachToComboBox->currentText() == "-")
  {
    attachmentPoint = 0;
  }
  emit attachProp(attachmentPoint);
}

void PinglyMotionWindow::on_propXPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PinglyMotionWindow::on_propYPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PinglyMotionWindow::on_propZPosSpin_valueChanged(int)
{
  emit propPositionChanged(ui->propXPosSpin->value(), ui->propYPosSpin->value(), ui->propZPosSpin->value());
}

void PinglyMotionWindow::on_propXScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PinglyMotionWindow::on_propYScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PinglyMotionWindow::on_propZScaleSpin_valueChanged(int)
{
  emit propScaleChanged(ui->propXScaleSpin->value(), ui->propYScaleSpin->value(), ui->propZScaleSpin->value());
}

void PinglyMotionWindow::on_propXRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

void PinglyMotionWindow::on_propYRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

void PinglyMotionWindow::on_propZRotSpin_valueChanged(int)
{
  emit propRotationChanged(ui->propXRotSpin->value(), ui->propYRotSpin->value(), ui->propZRotSpin->value());
}

// ------------------------------------------------- editing a picked prop (Mockup 2, src/propedit.h)

void PinglyMotionWindow::setupPropEditing()
{
  m_selectedProp=m_panels->selectedProp;
  // while a handle is dragged the number fields follow
  connect(ui->animationView,&AnimationView::propChanged,this,[this](Prop* prop)
  {
    if(m_selectedProp && prop && scene && scene->propManager()->getSelectedPropId()==prop->id())
      m_selectedProp->setState(AnimationView::propState(prop));
  });
  // a drag or a key ended: one Undo step, and the status line says what happened (PROPED-07)
  connect(ui->animationView,&AnimationView::propEdited,this,
          [this](Prop* prop,const PropEdit::State& before,const PropEdit::State& after,PropEdit::Mode mode)
  {
    if(!prop) return;
    recordPropStep(prop->id(),before,after);
    showSelectedProp();
    statusBar()->showMessage(PropEdit::doneText(mode,prop->name()),5000);
  });
  // a number field: the prop and the 3D view follow, one Undo step
  connect(m_selectedProp,&SelectedPropPanel::stateEdited,this,[this](const PropEdit::State& before,const PropEdit::State& after)
  {
    Prop* prop=scene ? scene->getPropById(scene->propManager()->getSelectedPropId()) : nullptr;
    if(!prop) return;
    AnimationView::applyPropState(prop,after);
    recordPropStep(prop->id(),before,after);
    ui->animationView->update();
  });
  // the Shape choice: the prop gets the other shape, one Undo step
  connect(m_selectedProp,&SelectedPropPanel::shapeChosen,this,[this](int type)
  {
    Prop* prop=scene ? scene->getPropById(scene->propManager()->getSelectedPropId()) : nullptr;
    if(!prop) return;
    const unsigned int id=prop->id();
    const int old=int(prop->propType());
    auto apply=[this,id](int shape)
    {
      Prop* p=scene ? scene->getPropById(id) : nullptr;
      if(!p) return;
      QSharedPointer<Mesh> mesh=cubeMesh;
      if(shape==Prop::Sphere) mesh=sphereMesh;
      else if(shape==Prop::Cone) mesh=coneMesh;
      else if(shape==Prop::Torus) mesh=torusMesh;
      p->setShape(static_cast<Prop::PropType>(shape),mesh);
      showSelectedProp();
      ui->animationView->update();
    };
    apply(type);
    if(m_undo) m_undo->recordStep([apply,old]() { apply(old); },[apply,type]() { apply(type); });
  });
  showSelectedProp();
}

void PinglyMotionWindow::showSelectedProp()
{
  if(!m_selectedProp) return;
  const Prop* prop=scene ? scene->getPropById(scene->propManager()->getSelectedPropId()) : nullptr;
  if(!prop)
  {
    m_selectedProp->showNothing();
    return;
  }
  m_selectedProp->showProp(prop->name(),int(prop->propType()),AnimationView::propState(prop));
}

void PinglyMotionWindow::applyPropStep(unsigned int propId,const PropEdit::State& state)
{
  Prop* prop=scene ? scene->getPropById(propId) : nullptr;
  // the prop was deleted since: nothing to put back
  if(!prop) return;
  AnimationView::applyPropState(prop,state);
  showSelectedProp();
  ui->animationView->update();
}

void PinglyMotionWindow::recordPropStep(unsigned int propId,const PropEdit::State& before,const PropEdit::State& after)
{
  if(!m_undo || before==after) return;
  m_undo->recordStep([this,propId,before]() { applyPropStep(propId,before); },
                     [this,propId,after]() { applyPropStep(propId,after); });
}
