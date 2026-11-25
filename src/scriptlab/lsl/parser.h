/*
 * PinglyMotion Script Lab - LSL interpreter: parser and checker
 *
 * Recursive descent parser (no flex or bison, so the Windows build needs no
 * extra tools) that turns tokens into the syntax tree in ast.h, and a checker
 * that resolves names and types the way the Second Life compiler does, so
 * mistakes show with line and column before the script runs.
 *
 * The parser stops at the first syntax error; the checker reports up to
 * MAX_ERRORS errors. The input is untrusted: nesting depth is limited, and so is
 * the depth of each expression's tree (MAX_EXPRESSION_DEPTH), because the
 * checker, the static warnings, the SLua converter, the interpreter and
 * freeing the tree all walk it recursively.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_PARSER_H
#define LSL_PARSER_H

#include <memory>
#include <vector>

#include "ast.h"
#include "lexer.h"

namespace lsl
{

class Parser
{
  public:
    static constexpr int MAX_DEPTH = 100;              // nesting while parsing (blocks, statements, brackets)
    static constexpr int MAX_EXPRESSION_DEPTH = 200;   // levels in one expression (a is 1, a + b is 2, a + b + c is 3)
    static constexpr int MAX_ERRORS = 50;

    struct Result
    {
      std::unique_ptr<Script> script;      // null when the source does not parse
      std::vector<Diagnostic> errors;      // the script does not compile when not empty
      std::vector<Diagnostic> warnings;    // valid LSL, but something the sandbox does not run
      bool ok() const { return script && errors.empty(); }
    };

    // lexes, parses and checks
    static Result parse(const QString& source);

    // the events the sandbox raises (others are accepted but never called)
    static bool eventIsRaised(const QString& name);
};

} // namespace lsl

#endif
