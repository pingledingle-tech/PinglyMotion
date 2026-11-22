/*
 * PinglyMotion Script Lab - RLV command parser
 *
 * Licence note: syntax, parameter types and return codes follow RLVa
 * (Firestorm viewer, LGPL 2.1, copyright Kitty Barnett); modified port, see
 * rlvcommand.h and CREDITS.md.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "rlvcommand.h"

namespace
{
  bool isBehaviourChar(QChar c)
  {
    return (c >= QLatin1Char('a') && c <= QLatin1Char('z')) || (c >= QLatin1Char('0') && c <= QLatin1Char('9')) ||
           c == QLatin1Char('_');
  }

  // no control characters (new lines, NUL, ...) in options and parameters
  bool isPrintable(const QString& text)
  {
    for (const QChar c : text)
      if (c.unicode() < 0x20 || c.unicode() == 0x7f) return false;
    return true;
  }

  // a whole decimal number from 1 to 2147483647, nothing else
  bool readChannel(const QString& text, int& channel)
  {
    if (text.isEmpty() || text.length() > 10) return false;
    qint64 value = 0;
    for (const QChar c : text)
    {
      if (c < QLatin1Char('0') || c > QLatin1Char('9')) return false;
      value = value * 10 + (c.unicode() - '0');
    }
    if (value < 1 || value > 2147483647LL) return false;
    channel = static_cast<int>(value);
    return true;
  }
}

namespace Rlv
{
  QString resultName(Result result)
  {
    switch (result)
    {
      case Result::Success: return QStringLiteral("success");
      case Result::Unset: return QStringLiteral("unset");
      case Result::Duplicate: return QStringLiteral("duplicate");
      case Result::Failed: return QStringLiteral("failed");
      case Result::Retained: return QStringLiteral("retained");
    }
    return QStringLiteral("failed");
  }

  QString paramTypeName(ParamType type)
  {
    switch (type)
    {
      case ParamType::Add: return QStringLiteral("add");
      case ParamType::Remove: return QStringLiteral("remove");
      case ParamType::Force: return QStringLiteral("force");
      case ParamType::Reply: return QStringLiteral("reply");
      case ParamType::Clear: return QStringLiteral("clear");
      case ParamType::Invalid: break;
    }
    return QStringLiteral("invalid");
  }

  Command parseCommand(const QString& input)
  {
    Command command;
    command.text = input.toLower();
    const QString& text = command.text;

    const int equals = text.indexOf(QLatin1Char('='));
    const QString head = equals < 0 ? text : text.left(equals);
    if (equals >= 0) command.param = text.mid(equals + 1);

    const int colon = head.indexOf(QLatin1Char(':'));
    command.behaviour = colon < 0 ? head : head.left(colon);
    if (colon >= 0) command.option = head.mid(colon + 1);

    if (command.behaviour.isEmpty()) { command.error = QStringLiteral("no behaviour name"); return command; }
    if (command.behaviour.length() > MAX_BEHAVIOUR_LENGTH) { command.error = QStringLiteral("behaviour name too long"); return command; }
    for (const QChar c : command.behaviour)
      if (!isBehaviourChar(c)) { command.error = QStringLiteral("behaviour name may only hold a-z, 0-9 and _"); return command; }
    if (command.option.length() > MAX_OPTION_LENGTH) { command.error = QStringLiteral("option too long"); return command; }
    if (command.param.length() > MAX_PARAM_LENGTH) { command.error = QStringLiteral("parameter too long"); return command; }
    if (!isPrintable(command.option) || !isPrintable(command.param)) { command.error = QStringLiteral("control characters are not allowed"); return command; }

    // @clear and @clear=<filter>: the only command without the usual parameter
    if (command.behaviour == QLatin1String("clear"))
    {
      if (colon >= 0) { command.error = QStringLiteral("@clear takes no option; use @clear=<filter>"); return command; }
      command.type = ParamType::Clear;
      command.valid = true;
      return command;
    }

    if (equals < 0) { command.error = QStringLiteral("missing \"=\" and parameter"); return command; }

    const QString& param = command.param;
    if (param == QLatin1String("n") || param == QLatin1String("add")) command.type = ParamType::Add;
    else if (param == QLatin1String("y") || param == QLatin1String("rem")) command.type = ParamType::Remove;
    else if (param == QLatin1String("force")) command.type = ParamType::Force;
    else if (readChannel(param, command.channel)) command.type = ParamType::Reply;
    else
    {
      command.error = QStringLiteral("parameter must be n, add, y, rem, force or a channel number from 1 to 2147483647");
      return command;
    }
    command.valid = true;
    return command;
  }

  ParseResult parseLine(const QString& text)
  {
    ParseResult result;
    if (text.length() > MAX_TEXT_LENGTH)
    {
      result.error = QString("RLV text is too long (at most %1 characters)").arg(MAX_TEXT_LENGTH);
      return result;
    }
    if (!text.startsWith(QLatin1Char('@')))
    {
      result.error = QStringLiteral("RLV text must start with \"@\"");
      return result;
    }

    const QStringList pieces = text.mid(1).split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (pieces.isEmpty())
    {
      result.error = QStringLiteral("no command after \"@\"");
      return result;
    }
    if (pieces.size() > MAX_COMMANDS_PER_LINE)
    {
      result.error = QString("too many commands in one line (at most %1)").arg(MAX_COMMANDS_PER_LINE);
      return result;
    }
    for (const QString& piece : pieces) result.commands << parseCommand(piece);
    return result;
  }
}
