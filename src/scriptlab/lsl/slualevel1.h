/*
 * PinglyMotion Script Lab - LSL to SLua converter: level 1, "LSL style"
 *
 * The layer that knows SLua (lane 1, job G): it turns the checked LSL tree into
 * the Lua tree of luair.h and keeps the script's own shape, so the converted
 * script still counts from 0 and keeps integer TRUE and FALSE inside. What
 * changes at the edges: ll.* names, 1-based positions into ll functions,
 * "not found" (nil) back to -1, booleans from ll back to 1 and 0 where the
 * script uses them as numbers, the evts table of multi-events, states as
 * tables with a state() function, LSL's integer division and casts, list and
 * string joins, and vectors that cannot be changed in place. jump cannot be
 * converted and gives a warning. Every rule and its source: docs/SLUA_RULES.md.
 * Not tested in Second Life. QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_SLUALEVEL1_H
#define LSL_SLUALEVEL1_H

#include "lsl2slua.h"

namespace lsl
{

class SluaLevel1 : public SluaLayer
{
  public:
    lua::Block lower(const Script& script, std::vector<SluaWarning>& warnings) override;
};

} // namespace lsl

#endif
