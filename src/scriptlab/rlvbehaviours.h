/*
 * PinglyMotion Script Lab - the RLV behaviours the sandbox knows
 *
 * A small table of common behaviours: which parameter types each accepts,
 * whether it takes an option, and which actions it blocks. Behaviours that
 * are not in the table still parse, but are reported as unknown.
 *
 * Licence note: names and meanings follow RLVa's behaviour dictionary
 * (Firestorm viewer, rlvhelper.cpp; LGPL 2.1, copyright Kitty Barnett) and the
 * public RLV API. Modified port: the table is new, smaller and has no viewer
 * types. See CREDITS.md.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef RLVBEHAVIOURS_H
#define RLVBEHAVIOURS_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "rlvcommand.h"

namespace Rlv
{
  // which parameter types a behaviour accepts (bit flags)
  enum ParamFlag
  {
    AllowAddRemove = 1,  // n/add and y/rem: a restriction
    AllowForce = 2,      // =force
    AllowReply = 4       // =<channel>: a question answered on a channel
  };

  // whether a behaviour takes an option after ":"
  enum class OptionUse { None, Optional };

  struct Action
  {
    QString id;     // e.g. "sendim"
    QString label;  // e.g. "Send instant messages"
  };

  struct Behaviour
  {
    QString name;
    int params = 0;              // ParamFlag bits
    OptionUse option = OptionUse::None;
    QStringList blocks;          // action ids it blocks while held without an option
    QString description;
  };

  // The RLV version the sandbox answers with: 3.4.3.
  const int VERSION_MAJOR = 3;
  const int VERSION_MINOR = 4;
  const int VERSION_PATCH = 3;
  QString versionText(bool legacy);   // @version (legacy) and @versionnew
  QString versionNumber();            // @versionnum: "3040300"

  const QVector<Behaviour>& behaviours();
  // nullptr when the behaviour is not in the table
  const Behaviour* findBehaviour(const QString& name);
  // every action any behaviour can block, in a fixed order
  const QVector<Action>& actions();

  // Checks a parsed command against the table. Empty when it is allowed,
  // otherwise the reason (unknown behaviour, parameter type, option).
  QString checkCommand(const Command& command);
}

#endif
