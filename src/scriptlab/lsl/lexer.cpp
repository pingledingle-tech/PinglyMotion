/*
 * PinglyMotion Script Lab - LSL interpreter: lexer
 *
 * Token rules follow the LSL grammar as described by lslint (public domain)
 * and the Second Life wiki; written from that description, no code copied.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "lexer.h"

namespace lsl
{

namespace
{
  bool isIdentStart(QChar c)
  {
    const ushort u = c.unicode();
    return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || u == '_';
  }

  bool isDigit(QChar c)
  {
    const ushort u = c.unicode();
    return u >= '0' && u <= '9';
  }

  bool isHexDigit(QChar c)
  {
    const ushort u = c.unicode();
    return isDigit(c) || (u >= 'a' && u <= 'f') || (u >= 'A' && u <= 'F');
  }

  // longest first
  const char* const OPERATORS[] =
  {
    "<<=", ">>=",
    "+=", "-=", "*=", "/=", "%=", "==", "!=", "<=", ">=", "&&", "||", "<<", ">>", "++", "--",
    "+", "-", "*", "/", "%", "=", "<", ">", "!", "~", "&", "|", "^",
    "(", ")", "{", "}", "[", "]", ",", ";", ".", "@"
  };
}

Lexer::Result Lexer::tokenize(const QString& source)
{
  Result result;
  auto fail = [&](const QString& message, int line, int column)
  {
    result.tokens.clear();
    result.error = message;
    result.line = line;
    result.column = column;
    return result;
  };

  if (source.size() > MAX_SOURCE_CHARS)
    return fail(QString("The script is too long (%1 characters, at most %2).").arg(source.size()).arg(MAX_SOURCE_CHARS), 1, 1);

  const int n = source.size();
  int i = 0;
  int line = 1;
  int column = 1;

  auto advance = [&]()
  {
    if (source[i] == '\n') { ++line; column = 1; }
    else ++column;
    ++i;
  };

  while (true)
  {
    // white space and comments
    while (i < n)
    {
      const QChar c = source[i];
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v') { advance(); continue; }
      if (c == '/' && i + 1 < n && source[i + 1] == '/')
      {
        while (i < n && source[i] != '\n') advance();
        continue;
      }
      if (c == '/' && i + 1 < n && source[i + 1] == '*')
      {
        const int startLine = line, startColumn = column;
        advance(); advance();
        bool closed = false;
        while (i < n)
        {
          if (source[i] == '*' && i + 1 < n && source[i + 1] == '/') { advance(); advance(); closed = true; break; }
          advance();
        }
        if (!closed) return fail("A /* comment is never closed with */.", startLine, startColumn);
        continue;
      }
      break;
    }

    if (int(result.tokens.size()) >= MAX_TOKENS)
      return fail(QString("The script has too many tokens (at most %1).").arg(MAX_TOKENS), line, column);

    Token token;
    token.line = line;
    token.column = column;
    if (i >= n)
    {
      token.kind = TokenKind::End;
      result.tokens.push_back(token);
      return result;
    }

    const QChar c = source[i];
    if (isIdentStart(c))
    {
      const int start = i;
      while (i < n && (isIdentStart(source[i]) || isDigit(source[i]))) advance();
      token.kind = TokenKind::Identifier;
      token.text = source.mid(start, i - start);
      if (token.text.size() > 255) return fail("A name is too long (at most 255 characters).", token.line, token.column);
    }
    else if (isDigit(c) || (c == '.' && i + 1 < n && isDigit(source[i + 1])))
    {
      const int start = i;
      bool isFloat = false;
      if (c == '0' && i + 1 < n && (source[i + 1] == 'x' || source[i + 1] == 'X') && i + 2 < n && isHexDigit(source[i + 2]))
      {
        advance(); advance();
        while (i < n && isHexDigit(source[i])) advance();
      }
      else
      {
        while (i < n && isDigit(source[i])) advance();
        if (i < n && source[i] == '.')
        {
          isFloat = true;
          advance();
          while (i < n && isDigit(source[i])) advance();
        }
        if (i < n && (source[i] == 'e' || source[i] == 'E'))
        {
          int k = i + 1;
          if (k < n && (source[k] == '+' || source[k] == '-')) ++k;
          if (k < n && isDigit(source[k]))
          {
            isFloat = true;
            while (i < k) advance();
            while (i < n && isDigit(source[i])) advance();
          }
        }
        if (isFloat && i < n && (source[i] == 'f' || source[i] == 'F')) advance();
      }
      token.kind = isFloat ? TokenKind::Float : TokenKind::Integer;
      token.text = source.mid(start, i - start);
      if (token.text.size() > 400) return fail("A number is too long.", token.line, token.column);
      if (i < n && isIdentStart(source[i]))
        return fail("A number runs straight into a name; put a space or an operator between them.", line, column);
    }
    else if (c == '"')
    {
      advance();
      QString value;
      bool closed = false;
      while (i < n)
      {
        const QChar d = source[i];
        if (d == '"') { advance(); closed = true; break; }
        if (d == '\\' && i + 1 < n)
        {
          const QChar e = source[i + 1];
          advance(); advance();
          // LSL turns \t into four spaces when it compiles; other unknown escapes keep the character
          if (e == 'n') value += '\n';
          else if (e == 't') value += "    ";
          else value += e;
        }
        else
        {
          value += d;
          advance();
        }
        if (value.size() > MAX_STRING_CHARS)
          return fail(QString("A string is too long (at most %1 characters).").arg(MAX_STRING_CHARS), token.line, token.column);
      }
      if (!closed) return fail("A string is never closed with \".", token.line, token.column);
      token.kind = TokenKind::String;
      token.text = value;
    }
    else
    {
      bool found = false;
      for (const char* op : OPERATORS)
      {
        const QLatin1String text(op);
        if (QStringView(source).mid(i, text.size()) == text)
        {
          for (int k = 0; k < text.size(); ++k) advance();
          token.kind = TokenKind::Operator;
          token.text = text;
          found = true;
          break;
        }
      }
      if (!found)
      {
        const QString shown = c.isPrint() ? QString("'%1'").arg(c) : QString("U+%1").arg(c.unicode(), 4, 16, QChar('0')).toUpper();
        return fail("Unexpected character " + shown + ".", line, column);
      }
    }
    result.tokens.push_back(token);
  }
}

} // namespace lsl
