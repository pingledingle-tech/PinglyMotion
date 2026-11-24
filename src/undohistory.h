/*
 * PinglyMotion - Undo and Redo for the animation editing
 *
 * UndoHistory is the list of steps of one animation: the state now, the states
 * Undo goes back to and the states Redo goes forward to (each an
 * Animation::Snapshot). A new edit clears Redo. Saving does not clear anything:
 * the history survives Save (the project owner's wish, also for the Script Lab
 * editor), and the history only remembers which state was saved, so Undo back
 * to it takes the "unsaved" mark away again. At most maxSteps steps are kept.
 *
 * UndoController hangs the histories on the program: one history per open
 * animation, the Edit menu's Undo and Redo (and the top bar buttons, which
 * follow them), their grey state with the reason, and the moment a step is
 * taken. Every edit already marks the animation as changed
 * (Animation::animationDirty), so the controller looks for a change once the
 * edit is over, compares with the last step and keeps a step only when
 * something changed. While a mouse button is held (a drag on a ring, a slider,
 * the timeline) or a pull runs, it waits, so one drag is one step.
 *
 * What a step holds: the keys of every joint and of the Avatar Position, with
 * their easing and curves, the number of frames, frames per second and the
 * loop (Animation::Snapshot). Not in it: the figure choice and size,
 * adding or removing a figure, the picked joint and the current frame.
 *
 * Other edits (a prop moved, turned or sized in the 3D view or its fields,
 * its shape changed) come as their own steps with recordStep(): two
 * functions, one that puts the old state back and one that puts the new
 * state again. Undo and Redo take the steps of the active animation and
 * these in the order they were made.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef UNDOHISTORY_H
#define UNDOHISTORY_H

#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>

#include <functional>
#include <optional>

#include "animation.h"

class QAction;

class UndoHistory
{
  public:
    // how many steps Undo can go back, as the mockup (older steps are dropped)
    static constexpr int maxSteps=100;

    // starts the history at this state; nothing to undo or redo
    void start(const Animation::Snapshot& state);
    bool started() const { return m_started; }

    // a new state after an edit: true and one more Undo step (Redo is cleared)
    // when it differs from the state now; false and nothing changes otherwise
    bool record(const Animation::Snapshot& state);

    // Redo is no longer possible (another kind of edit came after the undone steps)
    void clearRedo() { m_redo.clear(); }

    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    int undoCount() const { return m_undo.count(); }
    int redoCount() const { return m_redo.count(); }

    // one step back or forward: the state to put back, or nothing when there is none
    std::optional<Animation::Snapshot> undo();
    std::optional<Animation::Snapshot> redo();

    const Animation::Snapshot& current() const { return m_current; }

    // the state now is the saved one (after Save); the history stays
    void markSaved();
    // true when the state now is the saved one
    bool atSaved() const;

  private:
    bool m_started=false;
    Animation::Snapshot m_current;
    QList<Animation::Snapshot> m_undo;
    QList<Animation::Snapshot> m_redo;
    std::optional<Animation::Snapshot> m_saved;
};

class UndoController : public QObject
{
  Q_OBJECT

  public:
    UndoController(QAction* undo,QAction* redo,QObject* parent=nullptr);

    // the keys of Undo (Ctrl+Z) and Redo (Ctrl+Shift+Z, Ctrl+Y), from MenuSpec::undoKeys and redoKeys;
    // the first is the one the menu shows
    static QList<QKeySequence> undoShortcuts();
    static QList<QKeySequence> redoShortcuts();

    // follows an animation from now on; its history starts once the code that
    // opened it is done (the next turn of the event loop)
    void watch(Animation* anim);
    // the animation Undo and Redo work on (the active figure's)
    void setAnimation(Animation* anim);
    Animation* animation() const { return m_current; }
    // why Undo and Redo cannot be used now ("Open an animation first.",
    // "Stop playing first."); empty when they can
    void setBlocked(const QString& reason);
    // true while an edit is still going on (a mouse button held, a pull running):
    // the step is taken when it is over. The default looks at the mouse buttons.
    void setBusyCheck(const std::function<bool()>& busy) { m_busy=busy; }

    // takes the steps of edits that are over now (also done by itself after an edit)
    void recordPending();
    // one step of another kind of edit (a prop): `undo` puts the state before
    // back, `redo` the state after; a new step clears Redo
    void recordStep(const std::function<void()>& undo,const std::function<void()>& redo);
    // forgets these steps (the props they belong to are gone)
    void clearSteps();
    int stepCount() const { return m_steps.count(); }
    int redoStepCount() const { return m_redoSteps.count(); }

    // Edit > Undo and Edit > Redo; false when there was nothing to do
    bool undo();
    bool redo();

    UndoHistory* history(Animation* anim);
    // why Undo or Redo is greyed now, empty when it can be used
    QString undoBlocked() const;
    QString redoBlocked() const;

  signals:
    // a step was put back on this animation: the window shows it again
    void restored(Animation* anim);
    // a sentence for the status line ("Undone.", "Redone.", or why not)
    void message(const QString& text);

  private:
    struct Step
    {
      quint64 order=0;
      std::function<void()> undo;
      std::function<void()> redo;
    };

    void startHistory(Animation* anim);
    // takes a step of the animation and gives it its place in the order of all steps
    void recordAnimation(Animation* anim);
    // the place in the order of the active animation's last Undo or next Redo step (0: none)
    quint64 animationUndoOrder() const;
    quint64 animationRedoOrder() const;
    void edited(Animation* anim,bool changed);
    void queueCheck();
    void updateActions();
    bool busy() const;

    QPointer<QAction> m_undoAction;
    QPointer<QAction> m_redoAction;
    QHash<Animation*,UndoHistory> m_histories;
    QSet<Animation*> m_pending;
    Animation* m_current=nullptr;
    QString m_blocked;
    bool m_checkQueued=false;
    bool m_restoring=false;
    std::function<bool()> m_busy;
    QList<Step> m_steps;
    QList<Step> m_redoSteps;
    // the order of each animation's Undo and Redo steps, as long as its history keeps them
    QHash<Animation*,QList<quint64>> m_animationOrder;
    QHash<Animation*,QList<quint64>> m_animationRedoOrder;
    quint64 m_order=0;
};

#endif
