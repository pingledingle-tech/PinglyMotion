/*
 * PinglyMotion Script Lab - LSL to SLua converter: what changes in SLua
 *
 * For every LSL function and event, what Linden Lab's definition files say
 * changes in SLua (lane 1, job G): index arguments and returns, boolean
 * returns, what is removed, the detected functions and the multi-events. The
 * table is generated into slua_data.inc by tools/gen_slua_data.py from the copy
 * of https://github.com/secondlife/lsl-definitions (Linden Research, Inc., MIT
 * licence) in tests/data/lsl-definitions/. QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_SLUADATA_H
#define LSL_SLUADATA_H

#include <QString>

namespace lsl
{

struct SluaFunction
{
  enum Flag
  {
    ReturnsIndex = 1,      // the returned integer is a 0-based index in LSL, 1-based in SLua
    ReturnsBool = 2,       // the returned integer is TRUE or FALSE in LSL, a boolean in SLua
    Removed = 4,           // removed from ll in SLua
    Detected = 8,          // an llDetected* function (or llAdjustDamage): a DetectedEvent method in SLua
    InLl = 16,             // the ll table has it
    ReturnsNil = 32,       // SLua gives nil where LSL gives -1 ("not found")
    ListIndices = 64,      // returns a list that holds indices
    InLlcompat = 128       // the llcompat table has it (LSL's own rules)
  };
  QString name;
  int flags = 0;
  unsigned indexArgs = 0;  // bit n: argument n (from 0) is a 0-based index in LSL
  bool has(Flag f) const { return (flags & f) != 0; }
  bool indexArg(int n) const { return n >= 0 && n < 32 && (indexArgs >> n) & 1u; }
};

struct SluaEvent
{
  enum Flag
  {
    Multi = 1,             // the handler gets the evts table (DetectedEvent list) instead of num_detected
    Removed = 4,           // removed in SLua (state_entry and state_exit)
    InLlEvents = 16        // LLEvents knows the event name
  };
  QString name;
  int flags = 0;
  bool has(Flag f) const { return (flags & f) != 0; }
};

namespace SluaData
{
  // null when the definitions do not know the name
  const SluaFunction* function(const QString& name);
  const SluaEvent* event(const QString& name);
  // a constant that is a uuid in SLua (NULL_KEY is a string in LSL, a uuid in SLua)
  bool constantIsUuid(const QString& name);
  int functionCount();
  int eventCount();
}

} // namespace lsl

#endif
