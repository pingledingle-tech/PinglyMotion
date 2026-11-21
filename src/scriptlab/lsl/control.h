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

namespace lsl
{
namespace Control
{
  static constexpr int MAX_INPUTS = 200;
  static constexpr int MAX_SECONDS = 3600;
  static constexpr int DEFAULT_SECONDS = 10;

  // {"command":"lsl_check","source":"..."}: errors and warnings with line and column
  QJsonObject check(const QJsonObject& request);
  // {"command":"lsl_run","source":"...","seconds":10,"inputs":[...]}: the event log
  QJsonObject run(const QJsonObject& request);
}
}

#endif
