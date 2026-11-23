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

  // What the option after ":" must be, as RLVa checks it (docs/RLV_OPTION_TYPES.md has
  // the RLVa function behind each type). RLVa refuses a command whose option does not
  // fit: nothing changes and the command fails. The sandbox checks this in RLVa mode.
  enum class ValueType
  {
    Unchecked,       // any text; not checked (folder paths, filters, names, or not confirmed in RLVa)
    None,            // RLVa takes no option for this form, although the sandbox table allows one
    Uuid,            // a key: 36 characters, hex digits with a separator at places 9, 14, 19 and 24
    TextureUuid,     // a key as above; spaces around it are allowed (RLVa's texture modifiers)
    UuidOrDistance,  // a key, or a distance in metres: <min>[;<max>], both 0 or more
    UuidOrNumber,    // a key, or a number (a distance in metres)
    Number,          // a decimal number, e.g. 1.5
    Integer,         // a whole number, e.g. 2
    Channel,         // a whole number from 1 (a chat channel)
    ReplyChannel,    // a chat channel from 1 to 2147483646 (2147483647 is the debug channel)
    Vector3,         // three numbers: x/y/z
    Vector4,         // four numbers: a/b/c/d
    AttachPoint,     // an attachment point name, e.g. spine
    Layer,           // a clothing layer name, e.g. shirt
    CameraMode,      // mouselook, thirdperson or reset
    Notify           // <channel>[;filter]; checked by RlvState itself
  };
  QString valueTypeName(ValueType type);    // e.g. "uuid", "number"

  // the option value type of a behaviour, one for the restriction forms (n, y, add,
  // rem) and one for =force; questions (=<channel>) are not checked
  struct OptionValue
  {
    ValueType restriction = ValueType::Unchecked;
    ValueType force = ValueType::Unchecked;
  };

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
    OptionValue value;           // what the option must be (RLVa); Unchecked when any text goes
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

  // The option value type for this form of the behaviour (Unchecked for questions,
  // @clear and unknown behaviours).
  ValueType valueType(const Behaviour& behaviour, ParamType type);
  // true when the option fits the type, with RLVa's own rules (an empty option fits:
  // whether one is needed is checkCommand's question)
  bool optionFits(ValueType type, const QString& option);
  // The attachment point names RLVa knows (lower case): the 55 points of the viewer's
  // avatar and "root", RLVa's other name for "avatar center".
  const QStringList& rlvaAttachPointNames();
  // The clothing layer names RLVa knows: the 17 wearable types, "universal" included.
  const QStringList& rlvaLayerNames();

  // Checks the option value of a syntactically allowed command (checkCommand gave no
  // reason). Empty when it fits, otherwise a plain-words warning that names the
  // command, the bad value and the fix. Only RLVa mode checks; plain RLV mode gives
  // empty, because the RLV viewer's handling of bad values is not confirmed.
  QString optionProblem(const Command& command, Mode mode = Mode::Rlva);

  // ---- help for the script editor (lane 1, job R3; data in rlvhelp_plain.inc) ----

  // One row of help per behaviour of the table and per family (setdebug_, ...): one whole
  // example line in our own words and one plain sentence for a beginner (at most 12 words).
  struct Help
  {
    QString name;
    QString example;    // e.g. "@detach=n"
    QString sentence;   // e.g. "Locks this object on you, so it cannot be taken off."
  };
  const QVector<Help>& helpRows();
  // the row for a behaviour (a family member such as setenv_daytime gets its family's row);
  // nullptr when there is none
  const Help* findHelp(const QString& name);
  // the four families (setdebug_, setenv_, getdebug_, getenv_): any name after the "_"
  const QVector<Behaviour>& familyBehaviours();

  // The forms a behaviour takes after "=", in the order the editor lists them: n, y, add,
  // rem, force, then "<channel>" for a question. Forms only RLVa has are listed too;
  // formIsRlvaOnly() tells.
  QStringList formsOf(const Behaviour& behaviour);
  bool formIsRlvaOnly(const Behaviour& behaviour, const QString& form);
  // the forms in words for a fix, without the RLVa-only ones in plain RLV mode:
  // "n or y", "n, y or force", "a channel number such as 2222"; with withEquals
  // "=n, =y or =force", "=2222 (a channel number)"
  QString formsText(const Behaviour& behaviour, Mode mode, bool withEquals = false);

  // The known name closest to an unknown one (at most two letters off), or empty
  QString nearestName(const QString& name);

  // The warning the editor and the static check show for one command of an owner-say line:
  // empty when the command is fine in this mode, otherwise plain words that start with
  // "@<command>: ", say what is wrong and how to fix it, for example
  // "@detach=x: x is not valid. Use n, y or force." The option value part is
  // optionProblem()'s (RLVa mode only).
  QString plainProblem(const Command& command, Mode mode = Mode::Rlva);
}

#endif
