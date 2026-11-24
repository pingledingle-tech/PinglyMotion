/*
 * PinglyMotion Script Lab - choose a restriction from the list (job A2)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QObject>
#include <QSet>

#include "labrestrictionchoice.h"

namespace LabRestrictionChoice
{

namespace
{
  bool blocks(const Rlv::Behaviour& b, const QString& actionId)
  {
    return (b.params & Rlv::AllowAddRemove) && b.blocks.contains(actionId) && b.alias.isEmpty();
  }

  ValueKind kindOf(const Rlv::Behaviour& b)
  {
    if (b.option == Rlv::OptionUse::None) return ValueKind::None;
    if (b.value.restriction == Rlv::ValueType::Number) return ValueKind::Distance;
    if (b.value.restriction == Rlv::ValueType::Channel) return ValueKind::ExceptChannel;
    return ValueKind::None;
  }

  // a value typed in the field, without the spaces round it
  QString cleanValue(const QString& value) { return value.trimmed(); }
}

Choice forAction(const QString& actionId, Rlv::Mode mode)
{
  Choice choice;
  choice.action = actionId;
  for (const Rlv::Action& a : Rlv::actions())
    if (a.id == actionId) choice.label = a.label;
  if (choice.label.isEmpty())
  {
    choice.reason = QObject::tr("The sandbox does not know this action.");
    return choice;
  }
  const Rlv::Behaviour* found = nullptr;
  const Rlv::Behaviour* same = Rlv::findBehaviour(actionId, mode);
  if (same && blocks(*same, actionId)) found = same;
  if (!found)
    for (const Rlv::Behaviour& b : Rlv::behaviours())
      if (blocks(b, actionId) && !b.name.endsWith(QLatin1String("_sec")) && Rlv::findBehaviour(b.name, mode))
      {
        found = &b;
        break;
      }
  if (!found)
  {
    // known only in the other mode?
    const Rlv::Mode other = mode == Rlv::Mode::Rlva ? Rlv::Mode::Rlv : Rlv::Mode::Rlva;
    bool inOther = false;
    for (const Rlv::Behaviour& b : Rlv::behaviours())
      if (blocks(b, actionId) && Rlv::findBehaviour(b.name, other)) inOther = true;
    choice.reason = !inOther ? QObject::tr("No RLV line blocks this on its own.")
                  : other == Rlv::Mode::Rlva ? QObject::tr("Only RLVa has this. Switch Viewer rules to RLVa.")
                                             : QObject::tr("Only plain RLV has this. Switch Viewer rules to RLV.");
    return choice;
  }
  choice.behaviour = found->name;
  choice.value = kindOf(*found);
  if (const Rlv::Help* help = Rlv::findHelp(found->name))
  {
    choice.sentence = help->sentence;
    choice.example = help->example;
  }
  return choice;
}

Line blockLine(const Choice& choice, const QString& value)
{
  Line line;
  if (!choice.ok())
  {
    line.error = choice.reason;
    return line;
  }
  const QString v = cleanValue(value);
  if (v.isEmpty() || choice.value == ValueKind::None)
  {
    line.text = QStringLiteral("@%1=n").arg(choice.behaviour);
    return line;
  }
  if (choice.value == ValueKind::Distance)
  {
    if (!Rlv::optionFits(Rlv::ValueType::Number, v) || v.toDouble() <= 0.0)
    {
      line.error = QObject::tr("Type a distance in metres, such as 1.5.");
      return line;
    }
    line.text = QStringLiteral("@%1:%2=n").arg(choice.behaviour, v);
    return line;
  }
  // a channel that stays open
  if (!Rlv::optionFits(Rlv::ValueType::Channel, v))
  {
    line.error = QObject::tr("Type a channel number from 1, such as 5.");
    return line;
  }
  line.text = QStringLiteral("@%1=n,%1:%2=add").arg(choice.behaviour, v);
  return line;
}

QString allowLine(const Choice& choice, const QVector<RlvState::Restriction>& held, const QString& object)
{
  if (!choice.ok()) return QString();
  QStringList commands;
  QSet<QString> seen;
  for (const RlvState::Restriction& r : held)
  {
    if (r.object != object || Rlv::canonicalName(r.behaviour) != choice.behaviour) continue;
    QString command;
    if (r.option.isEmpty()) command = QStringLiteral("%1=y").arg(choice.behaviour);
    else if (choice.value == ValueKind::ExceptChannel) command = QStringLiteral("%1:%2=rem").arg(choice.behaviour, r.option);
    else command = QStringLiteral("%1:%2=y").arg(choice.behaviour, r.option);
    if (seen.contains(command)) continue;
    seen.insert(command);
    commands << command;
  }
  return commands.isEmpty() ? QString() : QStringLiteral("@") + commands.join(QLatin1Char(','));
}

QStringList otherHolders(const Choice& choice, const QVector<RlvState::Restriction>& held, const QString& object)
{
  QStringList out;
  if (!choice.ok()) return out;
  for (const RlvState::Restriction& r : held)
    if (r.object != object && Rlv::canonicalName(r.behaviour) == choice.behaviour && !out.contains(r.object)) out << r.object;
  return out;
}

QString heldDistance(const Choice& choice, const QVector<RlvState::Restriction>& held)
{
  if (choice.value != ValueKind::Distance) return QString();
  for (const RlvState::Restriction& r : held)
    if (Rlv::canonicalName(r.behaviour) == choice.behaviour && !r.option.isEmpty()) return r.option;
  return QString();
}

QStringList explainBlock(const Choice& choice, const QString& value)
{
  QStringList out;
  if (!choice.ok()) return out;
  const QString v = cleanValue(value);
  out << QObject::tr("This line blocks: %1.").arg(choice.label);
  if (!choice.sentence.isEmpty()) out << choice.sentence;
  out << QObject::tr("@ starts an RLV line, and %1 is the command.").arg(choice.behaviour);
  if (choice.value == ValueKind::Distance && !v.isEmpty())
    out << QObject::tr(":%1 is the distance in metres.").arg(v);
  out << QObject::tr("=n means no: the avatar may not do this.");
  if (choice.value == ValueKind::ExceptChannel && !v.isEmpty())
    out << QObject::tr("%1:%2=add keeps channel %2 open.").arg(choice.behaviour, v);
  return out;
}

QStringList explainAllow(const Choice& choice)
{
  QStringList out;
  if (!choice.ok()) return out;
  out << QObject::tr("This line lifts the block again.");
  out << QObject::tr("=y means yes: the avatar may do this again.");
  if (choice.value == ValueKind::ExceptChannel) out << QObject::tr("=rem closes a channel that was kept open.");
  return out;
}

QString scriptLine(const QString& rlvLine)
{
  QString text = rlvLine;
  text.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('"'), QStringLiteral("\\\""));
  return QStringLiteral("llOwnerSay(\"%1\");").arg(text);
}

namespace
{
  // the brace depth before each character (depth[i] counts the braces before position i), with
  // strings and comments skipped; depth has script.size() + 1 entries
  QVector<int> braceDepth(const QString& s, QVector<int>* opens)
  {
    QVector<int> depth(s.size() + 1, 0);
    int d = 0;
    bool inString = false;
    bool lineComment = false;
    bool blockComment = false;
    for (int i = 0; i < s.size(); ++i)
    {
      depth[i] = d;
      const QChar c = s[i];
      const QChar next = i + 1 < s.size() ? s[i + 1] : QChar();
      if (lineComment) { if (c == QLatin1Char('\n')) lineComment = false; continue; }
      if (blockComment) { if (c == QLatin1Char('*') && next == QLatin1Char('/')) { blockComment = false; ++i; depth[i] = d; } continue; }
      if (inString)
      {
        if (c == QLatin1Char('\\')) { ++i; if (i < s.size()) depth[i] = d; }
        else if (c == QLatin1Char('"')) inString = false;
        continue;
      }
      if (c == QLatin1Char('"')) inString = true;
      else if (c == QLatin1Char('/') && next == QLatin1Char('/')) lineComment = true;
      else if (c == QLatin1Char('/') && next == QLatin1Char('*')) blockComment = true;
      else if (c == QLatin1Char('{')) { ++d; if (opens && d == 2) opens->append(i); }
      else if (c == QLatin1Char('}')) d = qMax(0, d - 1);
    }
    depth[s.size()] = d;
    return depth;
  }

  QString indentOf(const QString& s, int position)
  {
    const int start = s.lastIndexOf(QLatin1Char('\n'), position - 1) + 1;
    int end = start;
    while (end < s.size() && (s[end] == QLatin1Char(' ') || s[end] == QLatin1Char('\t'))) ++end;
    return s.mid(start, end - start);
  }
}

Insertion insert(const QString& script, int cursor, const QString& code)
{
  cursor = qBound(0, cursor, int(script.size()));
  QVector<int> opens;   // where each event body starts (the "{" that reaches depth 2)
  const QVector<int> depth = braceDepth(script, &opens);
  int at = -1;
  bool afterBrace = false;
  if (depth[cursor] >= 2)
  {
    // the end of the cursor's line, when the line stays inside the event
    int end = script.indexOf(QLatin1Char('\n'), cursor);
    if (end < 0) end = script.size();
    at = depth[end] >= 2 ? end : cursor;
  }
  else
  {
    int open = -1;
    const int entry = script.indexOf(QLatin1String("state_entry"));
    for (int o : opens)
      if (entry >= 0 && o > entry) { open = o; break; }
    if (open < 0 && !opens.isEmpty()) open = opens.first();
    if (open >= 0)
    {
      at = open + 1;
      afterBrace = true;
    }
    else at = cursor;
  }
  if (!afterBrace && at > 0 && script[at - 1] == QLatin1Char('{')) afterBrace = true;
  const QString indent = indentOf(script, at) + (afterBrace ? QStringLiteral("    ") : QString());
  const QString added = QStringLiteral("\n") + indent + code;
  Insertion result;
  result.text = script.left(at) + added + script.mid(at);
  result.position = at + 1 + int(indent.size());
  result.line = int(result.text.left(result.position).count(QLatin1Char('\n'))) + 1;
  result.at = at;
  result.added = added;
  return result;
}

}
