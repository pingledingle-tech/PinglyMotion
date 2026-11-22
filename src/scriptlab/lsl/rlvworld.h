/*
 * PinglyMotion Script Lab - LSL interpreter: the world with RLV (step 4)
 *
 * A World whose llOwnerSay("@...") goes to the RLV core (RlvState) for the
 * script's object. The core's reply lines are heard by the script as listen
 * events on the reply channel, the way a viewer answers an @version=<channel>.
 * llOwnerSay text that does not start with "@" stays a normal owner_say entry.
 *
 * Only the public API of RlvState is used. The log entry "rlv_command" shows
 * the command, one result per command and the replies.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_RLVWORLD_H
#define LSL_RLVWORLD_H

#include <QJsonObject>

#include "world.h"

class RlvState;

namespace lsl
{

class Interpreter;

class RlvWorld : public World
{
  public:
    // at most this many "@" lines per run; later ones are refused (the interpreter's own limits also apply)
    static constexpr int MAX_RLV_LINES = 100;

    // rlv: the restriction state the script works on; the object id used is objectKey()
    explicit RlvWorld(RlvState& rlv);

    // where the replies go (set after the Interpreter is made)
    void attach(Interpreter* interpreter) { m_interpreter = interpreter; }

    void ownerSay(double time, const QString& text) override;

    int rlvLines() const { return m_lines; }
    int refusedLines() const { return m_refused; }

    // The restrictions as the core reports them, same shape as the answer of rlv_state.
    static QJsonObject stateJson(const RlvState& rlv);

  private:
    RlvState& m_rlv;
    Interpreter* m_interpreter = nullptr;
    int m_lines = 0;
    int m_refused = 0;
};

} // namespace lsl

#endif
