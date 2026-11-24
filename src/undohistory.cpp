/*
 * PinglyMotion - Undo and Redo for the animation editing (see undohistory.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "undohistory.h"

#include <QAction>
#include <QGuiApplication>
#include <QTimer>

#include "menuspec.h"

// ------------------------------------------------------------------ UndoHistory

void UndoHistory::start(const Animation::Snapshot& state)
{
  m_started=true;
  m_current=state;
  m_undo.clear();
  m_redo.clear();
  m_saved.reset();
}

bool UndoHistory::record(const Animation::Snapshot& state)
{
  if(!m_started)
  {
    start(state);
    return false;
  }
  if(state==m_current) return false;
  m_undo.append(m_current);
  while(m_undo.count()>maxSteps) m_undo.removeFirst();
  m_redo.clear();
  m_current=state;
  return true;
}

std::optional<Animation::Snapshot> UndoHistory::undo()
{
  if(m_undo.isEmpty()) return std::nullopt;
  m_redo.append(m_current);
  m_current=m_undo.takeLast();
  return m_current;
}

std::optional<Animation::Snapshot> UndoHistory::redo()
{
  if(m_redo.isEmpty()) return std::nullopt;
  m_undo.append(m_current);
  m_current=m_redo.takeLast();
  return m_current;
}

void UndoHistory::markSaved()
{
  m_saved=m_current;
}

bool UndoHistory::atSaved() const
{
  return m_saved.has_value() && *m_saved==m_current;
}

// --------------------------------------------------------------- UndoController

namespace
{
  template<std::size_t N>
  QList<QKeySequence> sequences(const char* const (&list)[N])
  {
    QList<QKeySequence> out;
    for(const char* key : list) out << QKeySequence(QLatin1String(key));
    return out;
  }
}

QList<QKeySequence> UndoController::undoShortcuts()
{
  return sequences(MenuSpec::undoKeys);
}

QList<QKeySequence> UndoController::redoShortcuts()
{
  return sequences(MenuSpec::redoKeys);
}

UndoController::UndoController(QAction* undo,QAction* redo,QObject* parent)
  : QObject(parent),
    m_undoAction(undo),
    m_redoAction(redo)
{
  m_busy=[]() { return QGuiApplication::mouseButtons()!=Qt::NoButton; };
  if(m_undoAction) connect(m_undoAction,&QAction::triggered,this,[this]() { this->undo(); });
  if(m_redoAction) connect(m_redoAction,&QAction::triggered,this,[this]() { this->redo(); });
  updateActions();
}

void UndoController::watch(Animation* anim)
{
  if(!anim || m_histories.contains(anim)) return;
  m_histories.insert(anim,UndoHistory());
  connect(anim,&Animation::animationDirty,this,[this,anim](bool changed) { edited(anim,changed); });
  connect(anim,&QObject::destroyed,this,[this,anim]()
  {
    m_histories.remove(anim);
    m_pending.remove(anim);
    m_animationOrder.remove(anim);
    m_animationRedoOrder.remove(anim);
    if(m_current==anim) m_current=nullptr;
    updateActions();
  });
  // the code that opens an animation still sets its loop and frames after this;
  // the history starts from what it left
  QTimer::singleShot(0,this,[this,anim]()
  {
    if(m_histories.contains(anim) && !m_histories[anim].started()) startHistory(anim);
  });
}

void UndoController::startHistory(Animation* anim)
{
  UndoHistory& history=m_histories[anim];
  history.start(anim->snapshot());
  m_animationOrder.remove(anim);
  m_animationRedoOrder.remove(anim);
  if(!anim->dirty()) history.markSaved();
  updateActions();
}

void UndoController::setAnimation(Animation* anim)
{
  if(anim) watch(anim);
  m_current=anim;
  updateActions();
}

void UndoController::setBlocked(const QString& reason)
{
  if(m_blocked==reason) return;
  m_blocked=reason;
  updateActions();
}

UndoHistory* UndoController::history(Animation* anim)
{
  auto it=m_histories.find(anim);
  return it==m_histories.end() ? nullptr : &it.value();
}

bool UndoController::busy() const
{
  return m_busy && m_busy();
}

// every edit marks the animation as changed; Save takes the mark away
void UndoController::edited(Animation* anim,bool changed)
{
  if(m_restoring) return;
  UndoHistory* h=history(anim);
  if(!h || !h->started()) return;
  if(changed)
  {
    m_pending.insert(anim);
    queueCheck();
    updateActions();
    return;
  }
  // saved: what is there now is the saved state, and the history stays
  recordAnimation(anim);
  m_pending.remove(anim);
  h->markSaved();
  updateActions();
}

void UndoController::queueCheck()
{
  if(m_checkQueued) return;
  m_checkQueued=true;
  QTimer::singleShot(0,this,[this]()
  {
    m_checkQueued=false;
    if(busy())
    {
      // the edit goes on (a drag): look again a little later
      if(!m_checkQueued)
      {
        m_checkQueued=true;
        QTimer::singleShot(50,this,[this]() { m_checkQueued=false; if(!m_pending.isEmpty()) queueCheck(); });
      }
      return;
    }
    recordPending();
  });
}

void UndoController::recordPending()
{
  const QSet<Animation*> pending=m_pending;
  m_pending.clear();
  for(Animation* anim : pending) recordAnimation(anim);
  updateActions();
}

void UndoController::recordAnimation(Animation* anim)
{
  UndoHistory* h=history(anim);
  if(!h) return;
  if(!h->started())
  {
    startHistory(anim);
    return;
  }
  if(!h->record(anim->snapshot())) return;
  QList<quint64>& order=m_animationOrder[anim];
  order.append(++m_order);
  while(order.count()>h->undoCount()) order.removeFirst();
  m_animationRedoOrder.remove(anim);
  // a new edit: the undone prop steps cannot come back
  m_redoSteps.clear();
}

quint64 UndoController::animationUndoOrder() const
{
  auto it=m_histories.constFind(m_current);
  if(!m_current || it==m_histories.constEnd() || !it->canUndo()) return 0;
  const QList<quint64> order=m_animationOrder.value(m_current);
  return order.isEmpty() ? 1 : order.last();
}

quint64 UndoController::animationRedoOrder() const
{
  auto it=m_histories.constFind(m_current);
  if(!m_current || it==m_histories.constEnd() || !it->canRedo()) return 0;
  const QList<quint64> order=m_animationRedoOrder.value(m_current);
  return order.isEmpty() ? 1 : order.last();
}

void UndoController::recordStep(const std::function<void()>& undo,const std::function<void()>& redo)
{
  Step step;
  step.order=++m_order;
  step.undo=undo;
  step.redo=redo;
  m_steps.append(step);
  while(m_steps.count()>UndoHistory::maxSteps) m_steps.removeFirst();
  m_redoSteps.clear();
  // a new edit: the undone animation steps cannot come back either
  for(auto it=m_histories.begin();it!=m_histories.end();++it) it->clearRedo();
  m_animationRedoOrder.clear();
  updateActions();
}

void UndoController::clearSteps()
{
  m_steps.clear();
  m_redoSteps.clear();
  updateActions();
}

bool UndoController::undo()
{
  if(busy())
  {
    emit message(QCoreApplication::translate("MenuSpec",MenuSpec::undoWait));
    return false;
  }
  recordPending();
  const QString why=undoBlocked();
  if(!why.isEmpty())
  {
    emit message(why);
    return false;
  }
  // the newest step first: a prop step or the active animation's
  if(!m_steps.isEmpty() && m_steps.last().order>animationUndoOrder())
  {
    const Step step=m_steps.takeLast();
    m_redoSteps.append(step);
    m_restoring=true;
    if(step.undo) step.undo();
    m_restoring=false;
    updateActions();
    emit message(QCoreApplication::translate("MenuSpec",MenuSpec::undone));
    return true;
  }
  UndoHistory* h=history(m_current);
  if(!h) return false;
  const std::optional<Animation::Snapshot> state=h->undo();
  if(!state) return false;
  if(!m_animationOrder.value(m_current).isEmpty())
    m_animationRedoOrder[m_current].append(m_animationOrder[m_current].takeLast());
  m_restoring=true;
  m_current->restore(*state);
  m_current->setDirty(!h->atSaved());
  m_restoring=false;
  updateActions();
  emit restored(m_current);
  emit message(QCoreApplication::translate("MenuSpec",MenuSpec::undone));
  return true;
}

bool UndoController::redo()
{
  if(busy())
  {
    emit message(QCoreApplication::translate("MenuSpec",MenuSpec::undoWait));
    return false;
  }
  recordPending();
  const QString why=redoBlocked();
  if(!why.isEmpty())
  {
    emit message(why);
    return false;
  }
  // the step undone last first: the one with the lowest place among the two
  const quint64 animationOrder=animationRedoOrder();
  if(!m_redoSteps.isEmpty() && (!animationOrder || m_redoSteps.last().order<animationOrder))
  {
    const Step step=m_redoSteps.takeLast();
    m_steps.append(step);
    m_restoring=true;
    if(step.redo) step.redo();
    m_restoring=false;
    updateActions();
    emit message(QCoreApplication::translate("MenuSpec",MenuSpec::redone));
    return true;
  }
  UndoHistory* h=history(m_current);
  if(!h) return false;
  const std::optional<Animation::Snapshot> state=h->redo();
  if(!state) return false;
  if(!m_animationRedoOrder.value(m_current).isEmpty())
    m_animationOrder[m_current].append(m_animationRedoOrder[m_current].takeLast());
  m_restoring=true;
  m_current->restore(*state);
  m_current->setDirty(!h->atSaved());
  m_restoring=false;
  updateActions();
  emit restored(m_current);
  emit message(QCoreApplication::translate("MenuSpec",MenuSpec::redone));
  return true;
}

QString UndoController::undoBlocked() const
{
  if(!m_blocked.isEmpty()) return m_blocked;
  if(!m_steps.isEmpty()) return QString();
  if(!m_current) return QCoreApplication::translate("MenuSpec",MenuSpec::openFirst);
  auto it=m_histories.constFind(m_current);
  // an edit not yet taken as a step can be undone too
  if(it!=m_histories.constEnd() && (it->canUndo() || m_pending.contains(m_current))) return QString();
  return QCoreApplication::translate("MenuSpec",MenuSpec::nothingToUndo);
}

QString UndoController::redoBlocked() const
{
  if(!m_blocked.isEmpty()) return m_blocked;
  // a new edit (not yet a step) clears Redo
  if(!m_redoSteps.isEmpty() && !(m_current && m_pending.contains(m_current))) return QString();
  if(!m_current) return QCoreApplication::translate("MenuSpec",MenuSpec::openFirst);
  auto it=m_histories.constFind(m_current);
  // a new edit (not yet a step) clears Redo
  if(it!=m_histories.constEnd() && it->canRedo() && !m_pending.contains(m_current)) return QString();
  return QCoreApplication::translate("MenuSpec",MenuSpec::nothingToRedo);
}

void UndoController::updateActions()
{
  const QString undoWhy=undoBlocked();
  const QString redoWhy=redoBlocked();
  auto apply=[](QAction* action,const QString& why)
  {
    if(!action) return;
    action->setEnabled(why.isEmpty());
    action->setToolTip(why);
    action->setStatusTip(why);
  };
  apply(m_undoAction,undoWhy);
  apply(m_redoAction,redoWhy);
}
