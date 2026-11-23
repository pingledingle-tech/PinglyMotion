/*
 * PinglyMotion Script Lab - LSL to SLua converter: the Lua printer
 *
 * Writes the Lua tree of luair.h as Luau text (lane 1, job G): four spaces per
 * level, brackets only where Lua's precedence needs them, strings with Lua's
 * escapes, and a return that is not the last statement of its block written as
 * "do return end" (Lua allows return only at the end of a block). It knows
 * nothing about LSL or SLua; every layer uses the same printer. QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_LUAPRINTER_H
#define LSL_LUAPRINTER_H

#include <QString>

#include "luair.h"

namespace lsl
{
namespace lua
{

class Printer
{
  public:
    // the whole chunk, ending with one newline
    static QString print(const Block& chunk);
    // one expression on its own (for tests)
    static QString expression(const Expr& e);
    // a Lua string literal with quotes and escapes
    static QString quote(const QString& value);
};

} // namespace lua
} // namespace lsl

#endif
