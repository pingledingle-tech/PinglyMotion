/*
 * PinglyMotion Script Lab - the RLV behaviours the sandbox knows
 *
 * A small table of common behaviours: which parameter types each accepts,
 * whether it takes an option, and which actions it blocks. Behaviours that
 * are not in the table still parse, but are reported as unknown.
 *
 * Licence note: names, parameter types and meanings follow RLVa's behaviour
 * dictionary (Firestorm viewer, rlvdefines.h and rlvhelper.cpp; LGPL 2.1,
 * copyright Kitty Barnett) and the public RLV API. RLVa is the default
 * standard; the marks say where the RLV wiki's specification (the plain RLV
 * of Kokua) differs. tests/tst_rlvsources.cpp checks every name, parameter
 * type and mark against data files made from both sources
 * (tools/rlv_sources.py). Modified port: the table is new, smaller and has no
 * viewer types, and no handler is copied. See CREDITS.md.
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

  // which viewer family answers to a behaviour: "RLV" is in RLVa and in the
  // RLV wiki's specification, "RLVa only" is in RLVa but not in the wiki's
  // specification (it does not work in Kokua), "RLV only" is in the wiki but
  // RLVa does not have it
  enum class Dialect { Rlv, RlvaOnly, RlvOnly };

  // The mode the sandbox answers in: RLVa (the default) or plain RLV. In plain
  // RLV mode the RLVa-only behaviours and parameter types are unknown; in RLVa
  // mode the RLV-only ones are.
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
    // An old name for another behaviour: the name of that behaviour. Both names
    // share one state (holding one counts as holding the other); getstatus shows
    // the name that was used. Empty for ordinary behaviours.
    QString alias;
    // ParamFlag bits that only RLVa accepts for a behaviour both families know
    // (the "does not work in Kokua" mark on a parameter type, e.g. fly=force);
    // plain RLV mode treats them as missing. 0 for most.
    int rlvaOnlyParams = 0;
  };

  // The RLV version the sandbox answers with: 3.4.3.
  const int VERSION_MAJOR = 3;
  const int VERSION_MINOR = 4;
  const int VERSION_PATCH = 3;
  QString versionText(bool legacy);   // @version (legacy) and @versionnew
  QString versionNumber();            // @versionnum: "3040300"

  const QVector<Behaviour>& behaviours();
  // The clothing layers (as @getoutfit lists them, 16 of them) and the attachment points
  // (@getattach: index 0 is "none", then 40 points).
  const QStringList& clothingLayers();
  const QStringList& attachmentPoints();
  // The sandbox wears nothing: one "0" per entry of the list, or a single "0" when the
  // option names an entry of the list; empty when the option names none.
  QString wornString(const QStringList& names, const QString& option);
  // the name the state is kept under: the target of an alias, else the name itself
  QString canonicalName(const QString& name);
  // nullptr when the behaviour is not in the table (either dialect)
  const Behaviour* findBehaviour(const QString& name);
  // nullptr when the behaviour is not in the table or not known in this mode (RLVa only: unknown in plain RLV
  // mode; RLV only: unknown in RLVa mode)
  const Behaviour* findBehaviour(const QString& name, Mode mode);
  // the behaviour names known in this mode, in table order
  QStringList behaviourNames(Mode mode);
  // every action any behaviour can block, in a fixed order
  const QVector<Action>& actions();

  // Checks a parsed command against the table. Empty when it is allowed,
  // otherwise the reason (unknown behaviour, parameter type, option). In RLVa
  // mode (the default) RLV-only behaviours are reported as unknown, in plain
  // RLV mode the RLVa-only ones and RLVa-only parameter types.
  QString checkCommand(const Command& command, Mode mode = Mode::Rlva);
}

#endif
