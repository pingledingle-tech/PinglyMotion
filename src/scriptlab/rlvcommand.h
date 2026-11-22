/*
 * PinglyMotion Script Lab - RLV command parser
 *
 * Reads the text an LSL script sends with llOwnerSay("@..."): one or more
 * commands separated by commas, each <behaviour>[:<option>]=<param>.
 *
 * Licence note: the command syntax, the parameter types and the return codes
 * follow RLVa, the RLV implementation of the Firestorm viewer (rlvcommon.h,
 * rlvhelper.cpp; LGPL 2.1, copyright Kitty Barnett). This is a modified port
 * written for PinglyMotion with Qt types; RLVa's viewer-side handler is not
 * used. See CREDITS.md.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef RLVCOMMAND_H
#define RLVCOMMAND_H

#include <QString>
#include <QVector>

namespace Rlv
{
  // Limits for text that comes from a script or a control client. Second Life
  // cuts llOwnerSay at 1024 bytes; the rest keeps one line of commands bounded.
  const int MAX_TEXT_LENGTH = 1024;
  const int MAX_COMMANDS_PER_LINE = 64;
  const int MAX_BEHAVIOUR_LENGTH = 64;
  const int MAX_OPTION_LENGTH = 255;
  const int MAX_PARAM_LENGTH = 255;

  // What the part after "=" means.
  enum class ParamType
  {
    Invalid,   // missing, empty or not understood
    Add,       // "n" or "add": set a restriction
    Remove,    // "y" or "rem": lift a restriction
    Force,     // "force": do something once (detach, unsit, ...)
    Reply,     // a channel number: answer on that channel
    Clear      // @clear or @clear=<filter>
  };

  // What happened to one command (RLVa's RLV_RET_* in short).
  enum class Result
  {
    Success,     // done
    Unset,       // a remove of something that was not set (still a success)
    Duplicate,   // an add of something already set by this object (still a success)
    Failed,      // refused; the reason says why
    Retained     // accepted but not carried out in the sandbox (force commands)
  };

  QString resultName(Result result);
  QString paramTypeName(ParamType type);

  struct Command
  {
    QString text;        // the command as written, lower case
    QString behaviour;
    QString option;      // empty when there is none
    QString param;       // the text after "=", empty when there is none
    ParamType type = ParamType::Invalid;
    int channel = 0;     // for Reply: the channel, 1 to 2147483647
    bool valid = false;  // syntax ok (the behaviour may still be unknown)
    QString error;       // why the syntax was refused
  };

  struct ParseResult
  {
    QVector<Command> commands;
    QString error;       // empty when the line could be split into commands
    bool ok() const { return error.isEmpty(); }
  };

  // One command without "@" and without commas, e.g. "sendim:uuid=n".
  Command parseCommand(const QString& text);

  // A whole owner-say line, e.g. "@detach=n,sendim=n". The text is made lower
  // case; empty pieces between commas are skipped. A line that does not start
  // with "@", is too long or has too many commands gives an error and no commands.
  ParseResult parseLine(const QString& text);
}

#endif
