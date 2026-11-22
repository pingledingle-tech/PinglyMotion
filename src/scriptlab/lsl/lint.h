/*
 * PinglyMotion Script Lab - LSL interpreter: static warnings (lint)
 *
 * Looks at a script that compiles and reports things that are valid LSL but
 * most likely a mistake: variables that are never read, functions that are
 * never called, code that can never run, "=" where "==" was meant, list
 * comparisons that only compare lengths, and RLV commands in
 * llOwnerSay("@...") that the RLV parser refuses or does not know. The script
 * is never run. Each warning has a place, a stable id (see ids()) and a short
 * English message.
 *
 * The compile errors themselves come from Parser::parse (the same as
 * lsl_check); a script that does not compile gets those errors and no
 * warnings, because its syntax tree is not complete.
 *
 * Ideas for some checks come from lslint (public domain); no code is copied.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_LINT_H
#define LSL_LINT_H

#include <vector>

#include <QString>
#include <QStringList>

#include "ast.h"
#include "lexer.h"

namespace lsl
{

struct LintWarning
{
  int line = 1;      // starts at 1
  int column = 1;    // starts at 1
  QString id;        // stable, for example "unused-local"
  QString message;   // one short English sentence
};

namespace Lint
{
  static constexpr int MAX_WARNINGS = 200;

  struct Result
  {
    std::vector<Diagnostic> errors;       // compile errors (Parser::parse); no warnings when not empty
    std::vector<LintWarning> warnings;    // sorted by line and column; at most MAX_WARNINGS
    bool valid() const { return errors.empty(); }
  };

  // parses and checks the source, then lints it when it compiles
  Result lint(const QString& source);

  // the warnings for a script that compiled (Parser::parse filled in the expression types)
  std::vector<LintWarning> lintScript(const Script& script);

  // every warning id, in a fixed order
  QStringList ids();
}

} // namespace lsl

#endif
