/*
 * PinglyMotion - the window side of Undo and Redo (job B1, src/undohistory.h)
 *
 * Edit > Undo (Ctrl+Z) and Edit > Redo (Ctrl+Shift+Z, Ctrl+Y) and the top bar
 * buttons that follow them. The keys work in the whole window; a text field or
 * the Script Lab editor with the keyboard focus keeps Ctrl+Z for its own text.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QAction>
#include <QGuiApplication>
#include <QSignalBlocker>
#include <QStatusBar>

#include "animation.h"
#include "animationview.h"
#include "menuspec.h"
#include "pinglymotionwindow.h"
#include "scene.h"
#include "timeline.h"
#include "ui_pinglymotionwindow.h"
#include "undohistory.h"

void PinglyMotionWindow::buildUndo()
{
  m_undoAction->setShortcuts(UndoController::undoShortcuts());
  m_redoAction->setShortcuts(UndoController::redoShortcuts());
  delete m_undo;
  m_undo=new UndoController(m_undoAction,m_redoAction,this);
  // one drag (a ring, a slider, the timeline) or one pull is one step
  m_undo->setBusyCheck([this]()
  {
    return QGuiApplication::mouseButtons()!=Qt::NoButton || ui->animationView->pulling();
  });
  connect(m_undo,&UndoController::restored,this,&PinglyMotionWindow::undoRestored);
  connect(m_undo,&UndoController::message,this,[this](const QString& text) { statusBar()->showMessage(text,5000); });
  for(Animation* anim : animationIds) m_undo->watch(anim);
  if(scene && scene->getAnimation()) m_undo->setAnimation(scene->getAnimation());
}

void PinglyMotionWindow::undoRestored(Animation* anim)
{
  if(!scene || anim!=scene->getAnimation()) return;
  // the loop fields show the restored loop without writing it back
  const int frames=qMax(1,anim->getNumberOfFrames());
  if(anim->getLoopInPoint()>=0)
  {
    const QSignalBlocker blockIn(ui->loopInSpinBox);
    const QSignalBlocker blockOut(ui->loopOutSpinBox);
    const int in=anim->getLoopInPoint()+1;
    const int out=anim->getLoopOutPoint()+1;
    ui->loopInSpinBox->setValue(in);
    ui->loopOutSpinBox->setValue(out);
    ui->loopInPercentLabel->setText(QString("(%1%)").arg(in*100/frames));
    ui->loopOutPercentLabel->setText(QString("(%1%)").arg(out*100/frames));
  }
  calculateLongestRunningTime();
  // frame count, speed, the picked joint's turn, the key button
  updateInputs();
  updateEditActions();
  refreshHandsPanel();
  timeline->repaint();
  ui->animationView->update();
}
