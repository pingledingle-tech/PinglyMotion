/*
 * PinglyMotion Script Lab - reference answers for RLV (package B). See rlvreference.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "rlvreference.h"

#include <QSet>

#include "rlvstate.h"

namespace RlvReference
{
  namespace
  {
    QString lineError(int line, const QString& why)
    {
      return QStringLiteral("Line %1: %2").arg(line).arg(why);
    }

    bool channelValue(const QString& word, int& channel)
    {
      bool ok = false;
      const qint64 value = word.toLongLong(&ok);
      if (!ok || value < -2147483648LL || value > 2147483647LL || word.size() > 12) return false;
      channel = static_cast<int>(value);
      return true;
    }

    // "<word>" and the rest of the line after one space
    void splitWord(const QString& line, QString& word, QString& rest)
    {
      const int space = line.indexOf(QLatin1Char(' '));
      word = space < 0 ? line : line.left(space);
      rest = space < 0 ? QString() : line.mid(space + 1);
    }
  }

  QString nameError(const QString& name)
  {
    if (name.isEmpty()) return QStringLiteral("a case needs a name");
    if (name.size() > MAX_NAME_CHARS) return QStringLiteral("a case name has at most %1 characters").arg(MAX_NAME_CHARS);
    for (const QChar c : name)
      if (!(c.isLetterOrNumber() && c.unicode() < 128) && c != QLatin1Char('_') && c != QLatin1Char('-') && c != QLatin1Char('.'))
        return QStringLiteral("a case name uses only letters, digits, \"_\", \"-\" and \".\"");
    return QString();
  }

  Result parse(const QString& text)
  {
    Result out;
    Reference ref;
    QSet<QString> names;
    QString object = QStringLiteral("ref");
    int caseLines = 0;
    bool inCase = false;
    bool sawHeader[3] = {false, false, false};
    int number = 0;
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (QString raw : lines)
    {
      number++;
      if (raw.endsWith(QLatin1Char('\r'))) raw.chop(1);
      if (raw.size() > MAX_LINE_CHARS)
      {
        out.error = lineError(number, QStringLiteral("the line is longer than %1 characters").arg(MAX_LINE_CHARS));
        return out;
      }
      if (raw.trimmed().isEmpty() || raw.startsWith(QLatin1Char('#'))) continue;
      QString word, rest;
      splitWord(raw, word, rest);
      if (word == QLatin1String("viewer") || word == QLatin1String("date") || word == QLatin1String("source"))
      {
        if (inCase)
        {
          out.error = lineError(number, QStringLiteral("\"%1\" belongs in the header, before the first case").arg(word));
          return out;
        }
        const int idx = word == QLatin1String("viewer") ? 0 : word == QLatin1String("date") ? 1 : 2;
        if (sawHeader[idx])
        {
          out.error = lineError(number, QStringLiteral("\"%1\" is given twice").arg(word));
          return out;
        }
        rest = rest.trimmed();
        if (rest.isEmpty())
        {
          out.error = lineError(number, QStringLiteral("\"%1\" needs a value").arg(word));
          return out;
        }
        if (idx == 2 && rest != QLatin1String("recorded") && rest != QLatin1String("specification"))
        {
          out.error = lineError(number, QStringLiteral("\"source\" is \"recorded\" (in Second Life) or \"specification\", not \"%1\"").arg(rest));
          return out;
        }
        sawHeader[idx] = true;
        (idx == 0 ? ref.viewer : idx == 1 ? ref.date : ref.source) = rest;
        continue;
      }
      if (word == QLatin1String("case"))
      {
        if (!(sawHeader[0] && sawHeader[1] && sawHeader[2]))
        {
          out.error = lineError(number, QStringLiteral("the header needs \"viewer\", \"date\" and \"source\" before the first case"));
          return out;
        }
        const QString name = rest.trimmed();
        const QString bad = nameError(name);
        if (!bad.isEmpty())
        {
          out.error = lineError(number, bad);
          return out;
        }
        if (names.contains(name))
        {
          out.error = lineError(number, QStringLiteral("the case \"%1\" is there twice").arg(name));
          return out;
        }
        if (ref.cases.size() >= MAX_CASES)
        {
          out.error = lineError(number, QStringLiteral("more than %1 cases").arg(MAX_CASES));
          return out;
        }
        names.insert(name);
        Case c;
        c.name = name;
        c.line = number;
        ref.cases.push_back(c);
        object = QStringLiteral("ref");
        caseLines = 0;
        inCase = true;
        continue;
      }
      if (!inCase)
      {
        out.error = lineError(number, QStringLiteral("\"%1\" comes before the first case; the header is \"viewer\", \"date\" and \"source\"").arg(word));
        return out;
      }
      Case& current = ref.cases.last();
      if (++caseLines > MAX_LINES_PER_CASE)
      {
        out.error = lineError(number, QStringLiteral("more than %1 lines in one case").arg(MAX_LINES_PER_CASE));
        return out;
      }
      if (word == QLatin1String("mode"))
      {
        rest = rest.trimmed();
        if (rest == QLatin1String("rlva")) current.mode = Rlv::Mode::Rlva;
        else if (rest == QLatin1String("rlv")) current.mode = Rlv::Mode::Rlv;
        else
        {
          out.error = lineError(number, QStringLiteral("\"mode\" is \"rlva\" or \"rlv\""));
          return out;
        }
      }
      else if (word == QLatin1String("object"))
      {
        rest = rest.trimmed();
        if (!RlvState::validObjectId(rest))
        {
          out.error = lineError(number, QStringLiteral("\"%1\" is not an object name (1 to 64 letters, digits, \"-\", \"_\", \".\" or space)").arg(rest));
          return out;
        }
        object = rest;
      }
      else if (word == QLatin1String("send"))
      {
        if (!rest.startsWith(QLatin1Char('@')))
        {
          out.error = lineError(number, QStringLiteral("\"send\" needs an RLV line that starts with \"@\""));
          return out;
        }
        Line l;
        l.kind = Line::Kind::Send;
        l.text = rest;
        l.object = object;
        l.line = number;
        current.lines.push_back(l);
      }
      else if (word == QLatin1String("answer") || word == QLatin1String("prefix") || word == QLatin1String("number") || word == QLatin1String("any"))
      {
        QString channelWord, answerText;
        splitWord(rest, channelWord, answerText);
        Line l;
        l.kind = word == QLatin1String("answer") ? Line::Kind::Answer : word == QLatin1String("prefix") ? Line::Kind::Prefix
                 : word == QLatin1String("number") ? Line::Kind::Number : Line::Kind::Any;
        l.line = number;
        if (!channelValue(channelWord, l.channel))
        {
          out.error = lineError(number, QStringLiteral("\"%1\" needs a channel (a whole number)%2").arg(word, l.kind == Line::Kind::Number || l.kind == Line::Kind::Any ? QString() : QStringLiteral(" and then the text")));
          return out;
        }
        if (l.kind == Line::Kind::Prefix && answerText.isEmpty())
        {
          out.error = lineError(number, QStringLiteral("\"prefix\" needs the start of the answer after the channel"));
          return out;
        }
        if ((l.kind == Line::Kind::Number || l.kind == Line::Kind::Any) && !answerText.isEmpty())
        {
          out.error = lineError(number, QStringLiteral("\"%1\" takes only the channel").arg(word));
          return out;
        }
        if (current.lines.isEmpty() || current.lines.last().kind == Line::Kind::Silent)
        {
          out.error = lineError(number, QStringLiteral("\"%1\" must come below a \"send\" line").arg(word));
          return out;
        }
        l.text = answerText;
        current.lines.push_back(l);
      }
      else if (word == QLatin1String("silent"))
      {
        if (!rest.trimmed().isEmpty() || current.lines.isEmpty() || current.lines.last().kind != Line::Kind::Send)
        {
          out.error = lineError(number, QStringLiteral("\"silent\" stands alone, directly below a \"send\" line"));
          return out;
        }
        Line l;
        l.kind = Line::Kind::Silent;
        l.line = number;
        current.lines.push_back(l);
      }
      else
      {
        out.error = lineError(number, QStringLiteral("\"%1\" is not a known word (case, mode, object, send, answer, prefix, number, any, silent)").arg(word));
        return out;
      }
    }
    if (!(sawHeader[0] && sawHeader[1] && sawHeader[2]))
    {
      out.error = QStringLiteral("The header needs \"viewer\", \"date\" and \"source\".");
      return out;
    }
    if (ref.cases.isEmpty())
    {
      out.error = QStringLiteral("The file has no case.");
      return out;
    }
    for (const Case& c : ref.cases)
    {
      bool checks = false;
      for (const Line& l : c.lines) checks = checks || l.kind != Line::Kind::Send;
      if (!checks)
      {
        out.error = lineError(c.line, QStringLiteral("the case \"%1\" has no \"answer\" or \"silent\" line, so it tests nothing").arg(c.name));
        return out;
      }
    }
    out.value = ref;
    return out;
  }

  QString write(const Reference& ref)
  {
    QString text = QStringLiteral("viewer %1\ndate %2\nsource %3\n").arg(ref.viewer, ref.date, ref.source);
    for (const Case& c : ref.cases)
    {
      text += QStringLiteral("\ncase %1\n").arg(c.name);
      if (c.mode == Rlv::Mode::Rlv) text += QStringLiteral("mode rlv\n");
      QString object = QStringLiteral("ref");
      for (const Line& l : c.lines)
      {
        switch (l.kind)
        {
          case Line::Kind::Send:
            if (l.object != object)
            {
              text += QStringLiteral("object %1\n").arg(l.object);
              object = l.object;
            }
            text += QStringLiteral("send %1\n").arg(l.text);
            break;
          case Line::Kind::Answer: text += QStringLiteral("answer %1 %2\n").arg(l.channel).arg(l.text); break;
          case Line::Kind::Prefix: text += QStringLiteral("prefix %1 %2\n").arg(l.channel).arg(l.text); break;
          case Line::Kind::Number: text += QStringLiteral("number %1\n").arg(l.channel); break;
          case Line::Kind::Any: text += QStringLiteral("any %1\n").arg(l.channel); break;
          case Line::Kind::Silent: text += QStringLiteral("silent\n"); break;
        }
      }
    }
    return text;
  }

  QString Difference::text() const
  {
    return QStringLiteral("Line %1: expected %2, but the sandbox gave %3.").arg(line).arg(expected, got);
  }

  namespace
  {
    QString shown(const RlvState::Reply& r)
    {
      return QStringLiteral("%1: \"%2\"").arg(r.channel).arg(r.text);
    }
  }

  QVector<CaseResult> run(const Reference& reference)
  {
    QVector<CaseResult> out;
    for (const Case& c : reference.cases)
    {
      CaseResult result;
      result.name = c.name;
      RlvState state;
      state.setMode(c.mode);
      QVector<RlvState::Reply> replies;   // those of the last send, not yet matched
      bool checked = false;               // the last send has answer or silent lines
      int sendLine = 0;
      auto leftOver = [&]()
      {
        if (checked && !replies.isEmpty())
          result.differences.push_back({sendLine, QStringLiteral("no more answers"), shown(replies.first()) +
                                        (replies.size() > 1 ? QStringLiteral(" (and %1 more)").arg(replies.size() - 1) : QString())});
        replies.clear();
      };
      for (const Line& l : c.lines)
      {
        switch (l.kind)
        {
          case Line::Kind::Send:
          {
            leftOver();
            const RlvState::ApplyResult applied = state.apply(l.object, l.text);
            replies = applied.replies;
            checked = false;
            sendLine = l.line;
            break;
          }
          case Line::Kind::Silent:
            checked = true;
            if (!replies.isEmpty())
              result.differences.push_back({l.line, QStringLiteral("no answer"), shown(replies.first()) +
                                            (replies.size() > 1 ? QStringLiteral(" (and %1 more)").arg(replies.size() - 1) : QString())});
            replies.clear();
            break;
          case Line::Kind::Answer:
          case Line::Kind::Prefix:
          case Line::Kind::Number:
          case Line::Kind::Any:
          {
            checked = true;
            const QString want = l.kind == Line::Kind::Answer ? QStringLiteral("%1: \"%2\"").arg(l.channel).arg(l.text)
                               : l.kind == Line::Kind::Prefix ? QStringLiteral("%1: an answer starting with \"%2\"").arg(l.channel).arg(l.text)
                               : l.kind == Line::Kind::Any ? QStringLiteral("%1: any answer").arg(l.channel)
                                                           : QStringLiteral("%1: a whole number").arg(l.channel);
            if (replies.isEmpty())
            {
              result.differences.push_back({l.line, want, QStringLiteral("no answer")});
              break;
            }
            const RlvState::Reply got = replies.takeFirst();
            bool same = got.channel == l.channel;
            if (l.kind == Line::Kind::Answer) same = same && got.text == l.text;
            else if (l.kind == Line::Kind::Prefix) same = same && got.text.startsWith(l.text);
            else if (l.kind == Line::Kind::Any) { /* the channel is enough */ }
            else
            {
              same = same && !got.text.isEmpty();
              for (const QChar c : got.text) same = same && c >= QLatin1Char('0') && c <= QLatin1Char('9');
            }
            if (!same) result.differences.push_back({l.line, want, shown(got)});
            break;
          }
        }
      }
      leftOver();
      out.push_back(result);
    }
    return out;
  }

  QString stateName(KnownState state)
  {
    switch (state)
    {
      case KnownState::Fixed: return QStringLiteral("fixed");
      case KnownState::Intended: return QStringLiteral("intended");
      case KnownState::Unclear: break;
    }
    return QStringLiteral("unclear");
  }

  KnownResult parseKnown(const QString& text)
  {
    KnownResult out;
    QSet<QString> keys;
    int number = 0;
    for (QString raw : text.split(QLatin1Char('\n')))
    {
      number++;
      if (raw.endsWith(QLatin1Char('\r'))) raw.chop(1);
      if (raw.size() > MAX_LINE_CHARS)
      {
        out.error = lineError(number, QStringLiteral("the line is longer than %1 characters").arg(MAX_LINE_CHARS));
        return out;
      }
      if (raw.trimmed().isEmpty() || raw.startsWith(QLatin1Char('#'))) continue;
      QString stateWord, rest, key, reason;
      splitWord(raw, stateWord, rest);
      splitWord(rest, key, reason);
      Known k;
      k.line = number;
      if (stateWord == QLatin1String("fixed")) k.state = KnownState::Fixed;
      else if (stateWord == QLatin1String("intended")) k.state = KnownState::Intended;
      else if (stateWord == QLatin1String("unclear")) k.state = KnownState::Unclear;
      else
      {
        out.error = lineError(number, QStringLiteral("the state is \"fixed\", \"intended\" or \"unclear\", not \"%1\"").arg(stateWord));
        return out;
      }
      const int slash = key.indexOf(QLatin1Char('/'));
      if (slash <= 0 || !nameError(key.mid(slash + 1)).isEmpty())
      {
        out.error = lineError(number, QStringLiteral("the second word is \"<file>/<case>\""));
        return out;
      }
      if (reason.trimmed().isEmpty())
      {
        out.error = lineError(number, QStringLiteral("a known difference needs a reason in words"));
        return out;
      }
      if (keys.contains(key))
      {
        out.error = lineError(number, QStringLiteral("\"%1\" is there twice").arg(key));
        return out;
      }
      keys.insert(key);
      k.key = key;
      k.reason = reason.trimmed();
      out.list.push_back(k);
    }
    return out;
  }
}
