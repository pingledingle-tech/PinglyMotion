/*
 * PinglyMotion - is it still the same animation after a dialog?
 *
 * Code that opens a dialog and then goes on with the animation must not keep a plain pointer to
 * it over the dialog: while the dialog is open, other code can run (a control command, a timer)
 * and replace or delete the animation. DocumentWatch remembers the animation that was current
 * before the dialog with a QPointer, which becomes null when the animation is deleted, and says
 * afterwards whether that animation is still the current one:
 *
 *   const DocumentWatch watch(scene->getAnimation());
 *   dialog.exec();
 *   if(!watch.unchanged(scene->getAnimation())) { ...stop with a status message...; return; }
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef DOCUMENTWATCH_H
#define DOCUMENTWATCH_H

#include <QCoreApplication>
#include <QPointer>
#include <QString>

#include "animation.h"

class DocumentWatch
{
  public:
    explicit DocumentWatch(Animation* current) : m_animation(current) {}

    // true when the animation is still there and is still the current one ("now")
    bool unchanged(const Animation* now) const { return !m_animation.isNull() && m_animation.data() == now; }
    // the animation, or null when it was deleted
    Animation* animation() const { return m_animation.data(); }

    // what the status line says when the code stops because the animation changed
    static QString changedMessage()
    {
      return QCoreApplication::translate("DocumentWatch", "The animation changed while a window was open. Nothing was done. Try again.");
    }

  private:
    QPointer<Animation> m_animation;
};

#endif
