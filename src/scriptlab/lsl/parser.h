/*
 * PinglyMotion Script Lab - LSL interpreter: parser and checker
 *
 * Recursive descent parser (no flex or bison, so the Windows build needs no
 * extra tools) that turns tokens into the syntax tree in ast.h, and a checker
 * that resolves names and types the way the Second Life compiler does, so
 * mistakes show with line and column before the script runs.
 *
 * The parser stops at the first syntax error; the checker reports up to
 * MAX_ERRORS errors. The input is untrusted: nesting depth is limited.
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
    static const int MAX_DEPTH = 100;
    static const int MAX_ERRORS = 50;

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
