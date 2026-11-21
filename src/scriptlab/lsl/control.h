/*
 * PinglyMotion Script Lab - the control commands lsl_check, lsl_run and lsl_event
 *
 * Kept here, not in controlapi.cpp, so the Script Lab code stays in one
 * folder; controlapi.cpp only forwards the two commands (docs/CONTROL.md).
 * Both take untrusted input (an AI tool writes the script) and check every
 * field before use.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_CONTROL_H
#define LSL_CONTROL_H

#include <QJsonObject>

class RlvState;

namespace lsl
{
class Session;

namespace Control
{
  static constexpr int MAX_INPUTS = 200;
  static constexpr int MAX_SECONDS = 3600;
  static constexpr int DEFAULT_SECONDS = 10;
  static constexpr int DEFAULT_EVENT_SECONDS = 1;
  // all scripts of one lsl_run together (each one also at most Lexer::MAX_SOURCE_CHARS)
  static constexpr int MAX_TOTAL_SOURCE_CHARS = 524288;

  // {"command":"lsl_check","source":"..."}: errors and warnings with line and column
  QJsonObject check(const QJsonObject& request);
  // {"command":"lsl_run","source":"...","seconds":10,"inputs":[...],"keep_rlv":false}: the event log.
  // Instead of "source", "objects" runs several scripts in one world (step 5):
  // [{"name":"hud","source":"...","position":[128,128,20],"attached":35}, ...].
  // llOwnerSay("@...") goes to rlv (the sandbox's RLV state, the one rlv_state shows); without it
  // the run uses a state of its own. Unless "keep_rlv" is true the state is emptied before the run.
  // With rlv and session, the world is kept in session for lsl_event.
  QJsonObject run(const QJsonObject& request, RlvState* rlv = nullptr, Session* session = nullptr);
  // {"command":"lsl_event","type":"touch","object":"hud","seconds":1}: sends one event to the world
  // of the last lsl_run (kept in session) and runs it on; answers the log of this command only.
  QJsonObject event(const QJsonObject& request, Session* session);
}
}

#endif
