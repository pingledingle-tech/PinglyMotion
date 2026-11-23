/*
 * PinglyMotion Script Lab - LSL to SLua converter
 *
 * Turns an LSL script into SLua text (Second Life's Luau), lane 1, job G. The
 * steps: the Script Lab's own LSL parser and checker (parser.h), then a layer
 * that holds every SLua rule and builds a small Lua tree (luair.h), then the Lua
 * printer (luaprinter.h). Level 1 "LSL style" (slualevel1.h) is the only layer
 * today; a later level 2 ("real Lua style") is another layer, the rest stays.
 * The rules: docs/SLUA_RULES.md. The output is not tested in Second Life.
 *
 * The input is untrusted: the lexer and parser limit its size and nesting, the
 * output and the number of warnings are limited here, and nothing is run.
 * Anything the converter cannot write faithfully gives a warning; it never
 * writes something else in silence. QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_LSL2SLUA_H
#define LSL_LSL2SLUA_H

#include <vector>

#include <QString>

#include "ast.h"
#include "luair.h"

namespace lsl
{

// one note for the user: where (line and column in the LSL text, from 1), what, and what to do
struct SluaWarning
{
  int line = 1;
  int column = 1;
  QString message;   // plain words
  QString fix;       // a suggested fix, plain words
};

// a layer: everything one output style knows about SLua; the rest of the converter is shared
class SluaLayer
{
  public:
    virtual ~SluaLayer() = default;
    // the whole SLua chunk for a script that compiles; warnings are added to the list
    virtual lua::Block lower(const Script& script, std::vector<SluaWarning>& warnings) = 0;
};

class Lsl2Slua
{
  public:
    static constexpr int MAX_WARNINGS = 200;
    static constexpr int MAX_OUTPUT_CHARS = 4 * 1024 * 1024;

    enum class Style
    {
      LslStyle   // level 1: keeps the script's own shape (0-based numbers, integer TRUE and FALSE)
    };

    struct Result
    {
      QString text;                        // the SLua text; empty when the script does not compile
      std::vector<SluaWarning> errors;     // LSL errors: nothing is converted
      std::vector<SluaWarning> warnings;   // what the converter could not write faithfully
      bool ok() const { return errors.empty() && !text.isEmpty(); }
    };

    static Result convert(const QString& source, Style style = Style::LslStyle);
};

} // namespace lsl

#endif
