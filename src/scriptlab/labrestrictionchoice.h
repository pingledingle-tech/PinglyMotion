/*
 * PinglyMotion Script Lab - choose a restriction from the list (job A2)
 *
 * The Type commands page lists "What the avatar may do", one row per action. Since job A2 every
 * row has an Allowed / Blocked choice. This file holds the logic behind it, without widgets:
 * which RLV command blocks an action, the line that blocks it ("@sendim=n"), the line that lifts
 * what one object holds again ("@sendim=y"), a plain explanation of the line, and the script line
 * that sends it (llOwnerSay) with the place in a script where that line goes.
 *
 * Two kinds of command take a value: a distance (sittp, touchfar, tplocal: "@touchfar:3=n" blocks
 * touching farther away than 3 m) and a channel that stays open ("@sendchannel=n,sendchannel:5=add").
 * The plain sentences and examples come from the editor's RLV help (rlvhelp_plain.inc, job R3).
 *
 * QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABRESTRICTIONCHOICE_H
#define LABRESTRICTIONCHOICE_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "rlvbehaviours.h"
#include "rlvstate.h"

namespace LabRestrictionChoice
{
  // what the value field of a row is for
  enum class ValueKind
  {
    None,           // no field
    Distance,       // a distance in metres, optional: "@touchfar:3=n"
    ExceptChannel   // a channel that stays open, optional: "@sendchannel=n,sendchannel:5=add"
  };

  struct Choice
  {
    QString action;      // the action id, e.g. "sendim"
    QString label;       // the action's words, e.g. "Send instant messages"
    QString behaviour;   // the command that blocks it, e.g. "sendim"; empty when none in this mode
    ValueKind value = ValueKind::None;
    QString sentence;    // the editor's plain sentence for the command
    QString example;     // the editor's example for the command, e.g. "@sendim=n"
    QString reason;      // why there is no command (empty when there is one)
    bool ok() const { return !behaviour.isEmpty(); }
  };

  // The command that blocks this action in this mode: the behaviour of the same name when it
  // blocks it, otherwise the first plain behaviour of the table that does (not a _sec form, not an
  // old name). Without one, reason says why ("Only RLVa has this. Switch Viewer rules to RLVa.").
  Choice forAction(const QString& actionId, Rlv::Mode mode);

  struct Line
  {
    QString text;    // e.g. "@sendim=n"
    QString error;   // a bad value, in plain words; empty when the line is fine
    bool ok() const { return error.isEmpty(); }
  };
  // The line that blocks the action, with the value of the field when there is one (empty: none).
  Line blockLine(const Choice& choice, const QString& value);
  // The line that lifts again what this object holds of the command (every option it holds, with
  // =y, or =rem for an open channel), e.g. "@sendim=y" or "@sendchannel=y,sendchannel:5=rem".
  // Empty when the object holds none of it.
  QString allowLine(const Choice& choice, const QVector<RlvState::Restriction>& held, const QString& object);
  // The objects other than this one that hold the command (they keep the action blocked).
  QStringList otherHolders(const Choice& choice, const QVector<RlvState::Restriction>& held, const QString& object);
  // The distance this object's or any object's held limit gives ("3"), or empty: for a Distance row,
  // a limit with a distance blocks the action farther away than that.
  QString heldDistance(const Choice& choice, const QVector<RlvState::Restriction>& held);

  // The line in plain words, one short sentence each (at most 12 words), for Blocked and for Allowed.
  QStringList explainBlock(const Choice& choice, const QString& value);
  QStringList explainAllow(const Choice& choice);

  // The script line that sends an RLV line: llOwnerSay("@sendim=n");
  QString scriptLine(const QString& rlvLine);

  struct Insertion
  {
    QString text;        // the whole script with the line put in
    int position = 0;    // where the put-in line starts (characters from the start)
    int line = 0;        // its line number, from 1
    int at = 0;          // where the added text goes in the old script
    QString added;       // the added text: a line break, the indent and the code
  };
  // Puts a line of code into a script so that it runs: after the cursor's line when the cursor is
  // inside an event (and that line does not leave it), else at the start of state_entry's body, else
  // at the start of the first event's body, else at the cursor. The line gets the indent of the
  // line before it, four spaces more right after a "{". Strings and comments are skipped when
  // counting braces.
  Insertion insert(const QString& script, int cursor, const QString& code);
}

#endif
