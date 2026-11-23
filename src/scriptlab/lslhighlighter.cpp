/*
 * PinglyMotion Script Lab - syntax colours for LSL in the editor (lane 1, job F1)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QSet>
#include <QTextCharFormat>

#include "lsl/builtins.h"

#include "lslhighlighter.h"

namespace
{
  // Copied from docs/design_tokens.json ("syntax", dark and light). The test
  // tests/scriptlabpanel (tst_labeditor) reads that file and checks every value.
  struct SyntaxColours
  {
    const char* keyword;
    const char* type;
    const char* event;
    const char* function;
    const char* constant;
    const char* string;
    const char* number;
    const char* comment;
  };
  const SyntaxColours Dark = {"#7FB0FF", "#D0A4F0", "#F4A261", "#5CC8C8", "#E5C07B", "#A8D88A", "#F0A8D8", "#AEB2BA"};
  const SyntaxColours Light = {"#1A56B0", "#7A3AB0", "#9C4A0A", "#0B6E6E", "#7A5200", "#2E6B12", "#A0306E", "#535963"};

  const QSet<QString>& keywords()
  {
    static const QSet<QString> words{"default", "state", "if", "else", "for", "do", "while", "jump", "return"};
    return words;
  }

  const QSet<QString>& types()
  {
    static const QSet<QString> words{"integer", "float", "string", "key", "vector", "rotation", "quaternion", "list"};
    return words;
  }

  bool identStart(QChar c) { return c.isLetter() || c == QLatin1Char('_'); }
  bool identPart(QChar c) { return c.isLetterOrNumber() || c == QLatin1Char('_'); }

  LslHighlighter::Kind wordKind(const QString& word)
  {
    if (keywords().contains(word)) return LslHighlighter::Keyword;
    if (types().contains(word)) return LslHighlighter::Type;
    if (lsl::Builtins::function(word)) return LslHighlighter::Function;
    if (lsl::Builtins::event(word)) return LslHighlighter::Event;
    if (lsl::Builtins::constant(word)) return LslHighlighter::Constant;
    return LslHighlighter::Plain;
  }
}

LslHighlighter::LslHighlighter(QTextDocument* document, bool dark) :
  QSyntaxHighlighter(document),
  m_dark(dark)
{
}

void LslHighlighter::setDark(bool dark)
{
  if (m_dark == dark) return;
  m_dark = dark;
  rehighlight();
}

QList<LslHighlighter::Kind> LslHighlighter::kinds()
{
  return {Keyword, Type, Event, Function, Constant, String, Number, Comment};
}

const char* LslHighlighter::tokenName(Kind kind)
{
  switch (kind)
  {
    case Keyword: return "keyword";
    case Type: return "type";
    case Event: return "event";
    case Function: return "function";
    case Constant: return "constant";
    case String: return "string";
    case Number: return "number";
    case Comment: return "comment";
    case Plain: break;
  }
  return "";
}

QColor LslHighlighter::colour(Kind kind, bool dark)
{
  const SyntaxColours& c = dark ? Dark : Light;
  switch (kind)
  {
    case Keyword: return QColor(QLatin1String(c.keyword));
    case Type: return QColor(QLatin1String(c.type));
    case Event: return QColor(QLatin1String(c.event));
    case Function: return QColor(QLatin1String(c.function));
    case Constant: return QColor(QLatin1String(c.constant));
    case String: return QColor(QLatin1String(c.string));
    case Number: return QColor(QLatin1String(c.number));
    case Comment: return QColor(QLatin1String(c.comment));
    case Plain: break;
  }
  return QColor();
}

QList<LslHighlighter::Span> LslHighlighter::spans(const QString& line, int startState, int* endState)
{
  QList<Span> out;
  const int n = int(line.size());
  int i = 0;
  int state = startState;
  const auto add = [&out](int start, int length, Kind kind) { if (length > 0 && kind != Plain) out << Span{start, length, kind}; };

  if (state == InComment)
  {
    const int close = int(line.indexOf(QLatin1String("*/")));
    if (close < 0)
    {
      add(0, n, Comment);
      if (endState) *endState = InComment;
      return out;
    }
    add(0, close + 2, Comment);
    i = close + 2;
    state = Normal;
  }

  while (i < n)
  {
    const QChar c = line.at(i);
    // comments
    if (c == QLatin1Char('/') && i + 1 < n && line.at(i + 1) == QLatin1Char('/'))
    {
      add(i, n - i, Comment);
      i = n;
      break;
    }
    if (c == QLatin1Char('/') && i + 1 < n && line.at(i + 1) == QLatin1Char('*'))
    {
      const int close = int(line.indexOf(QLatin1String("*/"), i + 2));
      if (close < 0)
      {
        add(i, n - i, Comment);
        state = InComment;
        i = n;
        break;
      }
      add(i, close + 2 - i, Comment);
      i = close + 2;
      continue;
    }
    // strings, with \" and \\ inside; a string not closed on the line ends at the line's end
    if (c == QLatin1Char('"'))
    {
      int j = i + 1;
      while (j < n && line.at(j) != QLatin1Char('"'))
        j += (line.at(j) == QLatin1Char('\\') && j + 1 < n) ? 2 : 1;
      const int end = j < n ? j + 1 : n;
      add(i, end - i, String);
      i = end;
      continue;
    }
    // numbers: 12, 0x1F, 1.5, .5, 1e3, 2.0e-3
    if (c.isDigit() || (c == QLatin1Char('.') && i + 1 < n && line.at(i + 1).isDigit()))
    {
      int j = i;
      if (c == QLatin1Char('0') && i + 1 < n && (line.at(i + 1) == QLatin1Char('x') || line.at(i + 1) == QLatin1Char('X')))
      {
        j = i + 2;
        while (j < n && (line.at(j).isDigit() || QStringLiteral("abcdefABCDEF").contains(line.at(j)))) ++j;
      }
      else
      {
        while (j < n && line.at(j).isDigit()) ++j;
        if (j < n && line.at(j) == QLatin1Char('.')) { ++j; while (j < n && line.at(j).isDigit()) ++j; }
        if (j < n && (line.at(j) == QLatin1Char('e') || line.at(j) == QLatin1Char('E')))
        {
          int k = j + 1;
          if (k < n && (line.at(k) == QLatin1Char('+') || line.at(k) == QLatin1Char('-'))) ++k;
          if (k < n && line.at(k).isDigit()) { j = k; while (j < n && line.at(j).isDigit()) ++j; }
        }
      }
      add(i, j - i, Number);
      i = j;
      continue;
    }
    if (identStart(c))
    {
      int j = i + 1;
      while (j < n && identPart(line.at(j))) ++j;
      add(i, j - i, wordKind(line.mid(i, j - i)));
      i = j;
      continue;
    }
    ++i;
  }
  if (endState) *endState = state;
  return out;
}

void LslHighlighter::highlightBlock(const QString& text)
{
  int end = Normal;
  const QList<Span> list = spans(text, previousBlockState() == InComment ? InComment : Normal, &end);
  for (const Span& s : list)
  {
    QTextCharFormat format;
    format.setForeground(colour(s.kind, m_dark));
    // shape as well as colour: keywords bold, comments italic (readable in grey scale)
    if (s.kind == Keyword) format.setFontWeight(QFont::DemiBold);
    if (s.kind == Comment) format.setFontItalic(true);
    setFormat(s.start, s.length, format);
  }
  setCurrentBlockState(end);
}
