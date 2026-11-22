/*
 * PinglyMotion Script Lab - the world of the last lsl_run (step 5)
 *
 * Keeps the Sandbox of the last lsl_run, so lsl_event can send it an event
 * later and run it on, and so the Script Lab panel can show its objects and
 * the last dialog. Owned by the control API next to the RLV state; changed()
 * is emitted after every lsl_run and lsl_event.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_SESSION_H
#define LSL_SESSION_H

#include <memory>

#include <QObject>

#include "sandbox.h"

namespace lsl
{

class Session : public QObject
{
  Q_OBJECT

  public:
    explicit Session(QObject* parent = nullptr);
    ~Session() override;

    // takes the world of a run (replacing the one before) and emits changed()
    void set(std::unique_ptr<Sandbox> sandbox);
    // forgets the world and emits changed()
    void clear();
    Sandbox* sandbox() const { return m_sandbox.get(); }
    // emits changed() (after lsl_event ran the world on)
    void notify() { emit changed(); }

  signals:
    void changed();

  private:
    std::unique_ptr<Sandbox> m_sandbox;
};

} // namespace lsl

#endif
