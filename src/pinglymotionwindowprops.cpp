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
  ui->propPositionGroup->setEnabled(state);
  ui->propScaleGroup->setEnabled(state);
  ui->propRotationGroup->setEnabled(state);
  ui->attachToLabel->setEnabled(state);
  ui->attachToComboBox->setEnabled(state);
}

void PinglyMotionWindow::enableAvatarRotation(bool state)
{
  ui->rotationGroupBox->setEnabled(state);
}

void PinglyMotionWindow::enableAvatarPosition(bool state)
{
  ui->positionGroupBox->setEnabled(state);
}

void PinglyMotionWindow::updatePropSpins(const QVector3D& position, const QVector3D& rotation, const QVector3D& scale)
{
  ui->propXPosSpin->blockSignals(true);
  ui->propYPosSpin->blockSignals(true);
  ui->propZPosSpin->blockSignals(true);

  ui->propXPosSpin->setValue(static_cast<int>(position.x()));
  ui->propYPosSpin->setValue(static_cast<int>(position.y()));
  ui->propZPosSpin->setValue(static_cast<int>(position.z()));

  ui->propXPosSpin->blockSignals(false);
  ui->propYPosSpin->blockSignals(false);
  ui->propZPosSpin->blockSignals(false);

  ui->propXRotSpin->blockSignals(true);
  ui->propYRotSpin->blockSignals(true);
  ui->propZRotSpin->blockSignals(true);

  ui->propXRotSpin->setValue((360 + static_cast<int>(rotation.x())) % 360);
  ui->propYRotSpin->setValue((360 + static_cast<int>(rotation.y())) % 360);
  ui->propZRotSpin->setValue((360 + static_cast<int>(rotation.z())) % 360);

  ui->propXRotSpin->blockSignals(false);
  ui->propYRotSpin->blockSignals(false);
  ui->propZRotSpin->blockSignals(false);

  ui->propXScaleSpin->blockSignals(true);
  ui->propYScaleSpin->blockSignals(true);
  ui->propZScaleSpin->blockSignals(true);

  ui->propXScaleSpin->setValue(static_cast<int>(scale.x()));
  ui->propYScaleSpin->setValue(static_cast<int>(scale.y()));
  ui->propZScaleSpin->setValue(static_cast<int>(scale.z()));

  ui->propXScaleSpin->blockSignals(false);
  ui->propYScaleSpin->blockSignals(false);
  ui->propZScaleSpin->blockSignals(false);
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
}

void PinglyMotionWindow::onClearProps()
{
  ui->propNameCombo->clear();
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
