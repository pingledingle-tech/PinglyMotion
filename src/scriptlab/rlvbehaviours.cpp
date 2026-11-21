/*
 * PinglyMotion Script Lab - the RLV behaviours the sandbox knows
 *
 * Licence note: names and meanings follow RLVa's behaviour dictionary
 * (Firestorm viewer, LGPL 2.1, copyright Kitty Barnett) and the public RLV
 * API; modified port, see rlvbehaviours.h and CREDITS.md.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "rlvbehaviours.h"

namespace Rlv
{
  QString versionText(bool legacy)
  {
    return QString("%1 viewer v%2.%3.%4 (PinglyMotion Script Lab)")
        .arg(legacy ? QStringLiteral("RestrainedLife") : QStringLiteral("RestrainedLove"))
        .arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH);
  }

  QString versionNumber()
  {
    return QString::number(VERSION_MAJOR * 1000000 + VERSION_MINOR * 10000 + VERSION_PATCH * 100);
  }

  const QVector<Action>& actions()
  {
    static const QVector<Action> list =
    {
      {"detach", "Take off locked attachments"},
      {"sendim", "Send instant messages"},
      {"recvim", "Receive instant messages"},
      {"sendchat", "Chat in public (channel 0)"},
      {"recvchat", "Hear public chat"},
      {"sendchannel", "Chat on other channels"},
      {"tplm", "Teleport to a landmark"},
      {"tploc", "Teleport to a location on the map"},
      {"tplure", "Accept teleport offers"},
      {"fly", "Fly"},
      {"sit", "Sit on objects"},
      {"unsit", "Stand up"},
      {"sittp", "Sit on objects far away"},
      {"edit", "Edit objects"},
      {"rez", "Rez objects"},
      {"touch", "Touch objects"},
      {"showinv", "Open the inventory"},
      {"showloc", "See the location"},
      {"showminimap", "Open the minimap"},
      {"showworldmap", "Open the world map"},
      {"share", "Give inventory to others"},
    };
    return list;
  }

  const QVector<Behaviour>& behaviours()
  {
    static const QVector<Behaviour> table =
    {
      {"detach", AllowAddRemove | AllowForce, OptionUse::Optional, {"detach"}, "Locks the object (or an attachment point given as option); =force takes it off"},
      {"sendim", AllowAddRemove, OptionUse::Optional, {"sendim"}, "No instant messages out; with a UUID as option: an exception for that avatar"},
      {"recvim", AllowAddRemove, OptionUse::Optional, {"recvim"}, "No instant messages in; with a UUID as option: an exception"},
      {"sendchat", AllowAddRemove, OptionUse::None, {"sendchat"}, "No public chat"},
      {"recvchat", AllowAddRemove, OptionUse::Optional, {"recvchat"}, "Cannot hear public chat; with a UUID as option: an exception"},
      {"sendchannel", AllowAddRemove, OptionUse::Optional, {"sendchannel"}, "No chat on channels other than 0; with a channel as option: an exception"},
      {"tplm", AllowAddRemove, OptionUse::None, {"tplm"}, "No teleport by landmark"},
      {"tploc", AllowAddRemove, OptionUse::None, {"tploc"}, "No teleport by map location"},
      {"tplure", AllowAddRemove, OptionUse::Optional, {"tplure"}, "Cannot accept teleport offers; with a UUID as option: an exception"},
      {"fly", AllowAddRemove, OptionUse::None, {"fly"}, "Cannot fly"},
      {"sit", AllowAddRemove | AllowForce, OptionUse::Optional, {"sit"}, "Cannot sit; sit:<uuid>=force sits on that object"},
      {"unsit", AllowAddRemove | AllowForce, OptionUse::None, {"unsit"}, "Cannot stand up; =force stands up"},
      {"sittp", AllowAddRemove, OptionUse::Optional, {"sittp"}, "Cannot sit on objects farther away than 1.5 m (or the distance given as option)"},
      {"edit", AllowAddRemove, OptionUse::Optional, {"edit"}, "Cannot edit objects; with a UUID as option: an exception"},
      {"rez", AllowAddRemove, OptionUse::None, {"rez"}, "Cannot rez objects"},
      {"touchall", AllowAddRemove, OptionUse::None, {"touch"}, "Cannot touch any object"},
      {"showinv", AllowAddRemove, OptionUse::None, {"showinv"}, "Inventory cannot be opened"},
      {"showloc", AllowAddRemove, OptionUse::None, {"showloc"}, "The location is hidden"},
      {"showminimap", AllowAddRemove, OptionUse::None, {"showminimap"}, "The minimap cannot be opened"},
      {"showworldmap", AllowAddRemove, OptionUse::None, {"showworldmap"}, "The world map cannot be opened"},
      {"share", AllowAddRemove, OptionUse::Optional, {"share"}, "Cannot give inventory; with a UUID as option: an exception"},
      {"version", AllowReply, OptionUse::None, {}, "Answers the RLV version on the channel (old name RestrainedLife)"},
      {"versionnew", AllowReply, OptionUse::None, {}, "Answers the RLV version on the channel (RestrainedLove)"},
      {"versionnum", AllowReply, OptionUse::None, {}, "Answers the version number on the channel, e.g. 3040300"},
      {"getstatus", AllowReply, OptionUse::Optional, {}, "Answers this object's restrictions on the channel; option: <filter>[;<separator>]"},
      {"getstatusall", AllowReply, OptionUse::Optional, {}, "Answers all objects' restrictions on the channel; option: <filter>[;<separator>]"},
    };
    return table;
  }

  const Behaviour* findBehaviour(const QString& name)
  {
    for (const Behaviour& behaviour : behaviours())
      if (behaviour.name == name) return &behaviour;
    return nullptr;
  }

  QString checkCommand(const Command& command)
  {
    if (!command.valid) return command.error.isEmpty() ? QStringLiteral("bad syntax") : command.error;
    if (command.type == ParamType::Clear) return QString();

    const Behaviour* behaviour = findBehaviour(command.behaviour);
    if (!behaviour) return QString("unknown behaviour \"%1\"").arg(command.behaviour);

    int needed = 0;
    switch (command.type)
    {
      case ParamType::Add:
      case ParamType::Remove: needed = AllowAddRemove; break;
      case ParamType::Force: needed = AllowForce; break;
      case ParamType::Reply: needed = AllowReply; break;
      default: return QStringLiteral("bad syntax");
    }
    if (!(behaviour->params & needed))
      return QString("@%1 does not take =%2").arg(command.behaviour, command.param);
    if (behaviour->option == OptionUse::None && !command.option.isEmpty())
      return QString("@%1 takes no option").arg(command.behaviour);
    return QString();
  }
}
