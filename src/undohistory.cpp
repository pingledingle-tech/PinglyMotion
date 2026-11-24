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
  h->record(anim->snapshot());
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
  for(Animation* anim : pending)
    if(UndoHistory* h=history(anim))
    {
      if(!h->started()) startHistory(anim);
      else h->record(anim->snapshot());
    }
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
  UndoHistory* h=history(m_current);
  const std::optional<Animation::Snapshot> state=h->undo();
  if(!state) return false;
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
  UndoHistory* h=history(m_current);
  const std::optional<Animation::Snapshot> state=h->redo();
  if(!state) return false;
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
  if(!m_current) return QCoreApplication::translate("MenuSpec",MenuSpec::openFirst);
  auto it=m_histories.constFind(m_current);
  // an edit not yet taken as a step can be undone too
  if(it!=m_histories.constEnd() && (it->canUndo() || m_pending.contains(m_current))) return QString();
  return QCoreApplication::translate("MenuSpec",MenuSpec::nothingToUndo);
}

QString UndoController::redoBlocked() const
{
  if(!m_blocked.isEmpty()) return m_blocked;
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
