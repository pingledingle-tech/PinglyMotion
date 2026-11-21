/*
 * PinglyMotion Script Lab - LSL interpreter: lexer
 *
 * Turns LSL source text into tokens with line and column (both start at 1).
 * The input is untrusted (it can come from an AI over the control interface),
 * so the source size and the number of tokens are limited.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_LEXER_H
#define LSL_LEXER_H

#include <vector>

#include <QString>

namespace lsl
{

enum class TokenKind
{
  Identifier,   // names and keywords (the parser tells them apart)
  Integer,      // text holds the literal as written
  Float,
  String,       // text holds the value with the escapes already applied
  Operator,     // text holds the operator: "+", "+=", "<<", "(", ...
  End           // end of the source
};

struct Token
{
  TokenKind kind = TokenKind::End;
  QString text;
  int line = 1;
  int column = 1;
};

// one error with its place; line and column start at 1
struct Diagnostic
{
  int line = 1;
  int column = 1;
  QString message;
};

class Lexer
{
  public:
    static const int MAX_SOURCE_CHARS = 262144;
    static const int MAX_TOKENS = 100000;
    static const int MAX_STRING_CHARS = 65536;

    struct Result
    {
      std::vector<Token> tokens;   // ends with an End token when ok
      QString error;               // empty when it worked
      int line = 0;
      int column = 0;
      bool ok() const { return error.isEmpty(); }
    };

    static Result tokenize(const QString& source);
};

} // namespace lsl

#endif
