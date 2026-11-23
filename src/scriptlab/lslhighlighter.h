/*
 * PinglyMotion Script Lab - syntax colours for LSL in the editor (lane 1, job F1)
 *
 * Colours the words of an LSL script: keywords, types, events, functions,
 * constants, strings, numbers and comments. The colours are the "syntax"
 * tokens of docs/design_tokens.json (dark and light) and are decoration
 * only: nothing in the program means something by a colour here (design
 * charter P10). Errors are shown by the editor with a wavy underline, an
 * icon by the line number and words in the error list.
 *
 * The words come from the same lists the sandbox uses (lsl/builtins.h), so a
 * function name is coloured exactly when the checker knows it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSLHIGHLIGHTER_H
#define LSLHIGHLIGHTER_H

#include <QColor>
#include <QList>
#include <QSyntaxHighlighter>

class LslHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

  public:
    // kinds of words; Plain is not coloured
    enum Kind { Plain = 0, Keyword, Type, Event, Function, Constant, String, Number, Comment };

    // a coloured stretch of one line
    struct Span
    {
      int start = 0;
      int length = 0;
      Kind kind = Plain;
      bool operator==(const Span& o) const { return start == o.start && length == o.length && kind == o.kind; }
    };

    // block states: 0 normal, 1 inside a /* comment */ that goes on to the next line
    enum State { Normal = 0, InComment = 1 };

    explicit LslHighlighter(QTextDocument* document, bool dark = true);

    // the theme the colours are for; colours the whole document again when it changes
    void setDark(bool dark);
    bool dark() const { return m_dark; }

    // the colour of a kind, from docs/design_tokens.json "syntax" (Plain gives an invalid colour)
    static QColor colour(Kind kind, bool dark);
    // the token name of a kind in docs/design_tokens.json ("keyword", "type", ...)
    static const char* tokenName(Kind kind);
    static QList<Kind> kinds();

    // The stretches of one line, given the state the line starts in. Writes the
    // state the line ends in to endState (when not null). No Qt document needed.
    static QList<Span> spans(const QString& line, int startState = Normal, int* endState = nullptr);

  protected:
    void highlightBlock(const QString& text) override;

  private:
    bool m_dark;
};

#endif
