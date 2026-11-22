/*
 * PinglyMotion Script Lab - the RLV behaviours the sandbox knows
 *
 * A small table of common behaviours: which parameter types each accepts,
 * whether it takes an option, and which actions it blocks. Behaviours that
 * are not in the table still parse, but are reported as unknown.
 *
 * Licence note: names, parameter types and meanings follow RLVa's behaviour
 * dictionary (Firestorm viewer, rlvdefines.h and rlvhelper.cpp; LGPL 2.1,
 * copyright Kitty Barnett) and the public RLV API. The marks follow the RLV
 * wiki's list of plain RLV commands; RLVa's own extended and experimental
 * flags differ from it for some names. Modified port: the table is new,
 * smaller and has no viewer types, and no handler is copied. See CREDITS.md.
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
  enum class OptionUse { None, Optional, Required };

  // which viewer family answers to a behaviour: "RLV" is in the plain RLV
  // API as the RLV wiki lists it, "RLVa only" is not listed there (RLVa's own
  // extended and experimental flags differ from the wiki for some names)
  enum class Dialect { Rlv, RlvaOnly };

  // The mode the sandbox answers in. In plain RLV mode the RLVa-only
  // behaviours are unknown.
  enum class Mode { Rlv, Rlva };
  QString modeName(Mode mode);                 // "rlv" or "rlva"
  bool parseMode(const QString& text, Mode& mode);   // accepts "rlv" and "rlva", any case

  // what a behaviour touches
  enum class Group
  {
    Core,         // restrictions the sandbox itself answers (chat, IM, teleport, sit, ...)
    Inventory,    // outfits, attachments, shared folders
    Camera,       // camera and field of view
    Environment,  // sky, water and debug settings
    Interaction,  // touch, edit, view modes, gestures, self view
    Reply         // questions answered on a channel
  };
  QString groupName(Group group);

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
    Dialect dialect = Dialect::Rlv;
    Group group = Group::Core;
    // The behaviour acts on the viewer outside the sandbox (camera, sky,
    // debug settings, ...). The sandbox only records it and shows it in the
    // log; it never changes the real program.
    bool external = false;
    // free-form note on what the sandbox does with it (empty for most)
    QString note;
  };

  // The RLV version the sandbox answers with: 3.4.3.
  const int VERSION_MAJOR = 3;
  const int VERSION_MINOR = 4;
  const int VERSION_PATCH = 3;
  QString versionText(bool legacy);   // @version (legacy) and @versionnew
  QString versionNumber();            // @versionnum: "3040300"

  const QVector<Behaviour>& behaviours();
  // nullptr when the behaviour is not in the table (either dialect)
  const Behaviour* findBehaviour(const QString& name);
  // nullptr when the behaviour is not in the table or not known in this mode
  const Behaviour* findBehaviour(const QString& name, Mode mode);
  // the behaviour names known in this mode, in table order
  QStringList behaviourNames(Mode mode);
  // every action any behaviour can block, in a fixed order
  const QVector<Action>& actions();

  // Checks a parsed command against the table. Empty when it is allowed,
  // otherwise the reason (unknown behaviour, parameter type, option). In plain
  // RLV mode (the default) RLVa-only behaviours are reported as unknown.
  QString checkCommand(const Command& command, Mode mode = Mode::Rlv);
}

#endif
