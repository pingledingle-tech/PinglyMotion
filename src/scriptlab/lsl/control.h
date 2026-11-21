/*
 * PinglyMotion Script Lab - the control commands lsl_check and lsl_run
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
namespace Control
{
  static constexpr int MAX_INPUTS = 200;
  static constexpr int MAX_SECONDS = 3600;
  static constexpr int DEFAULT_SECONDS = 10;

  // {"command":"lsl_check","source":"..."}: errors and warnings with line and column
  QJsonObject check(const QJsonObject& request);
  // {"command":"lsl_run","source":"...","seconds":10,"inputs":[...],"keep_rlv":false}: the event log.
  // llOwnerSay("@...") goes to rlv (the sandbox's RLV state, the one rlv_state shows); without it
  // the run uses a state of its own. Unless "keep_rlv" is true the state is emptied before the run.
  QJsonObject run(const QJsonObject& request, RlvState* rlv = nullptr);
}
}

#endif
