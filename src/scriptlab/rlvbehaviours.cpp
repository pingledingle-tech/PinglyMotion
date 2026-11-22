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
      {"touchfar", "Touch objects farther away"},
      {"showinv", "Open the inventory"},
      {"showloc", "See the location"},
      {"showminimap", "Open the minimap"},
      {"showworldmap", "Open the world map"},
      {"share", "Give inventory to others"},
      // newer behaviours (some plain RLV, some RLVa only)
      {"accepttprequest", "Decline teleport requests"},
      {"tprequest", "Send teleport requests"},
      {"tplocal", "Teleport within the region"},
      {"interact", "Interact with the world"},
      {"touchhud", "Touch HUD objects"},
      {"sendgesture", "Play gestures that chat"},
      {"showself", "See your own avatar"},
      {"showselfhead", "See your own head attachments"},
      {"shownearby", "See the list of people nearby"},
      {"viewtransparent", "Highlight transparent objects"},
      {"viewwireframe", "View the world as wireframe"},
      {"sharedwear", "Wear items from the shared folder"},
      {"sharedunwear", "Take off items from the shared folder"},
      {"allowidle", "Go idle (away) after the normal time"},
      // plain RLV, effects outside the sandbox
      {"setenv", "Change the sky and water"},
      {"setdebug", "Change debug settings"},
      {"setcam", "Control the camera"},
      {"setcam_mouselook", "Go into mouselook"},
    };
    return list;
  }

  const QVector<Behaviour>& behaviours()
  {
    static const QVector<Behaviour> table =
    {
      {"detach", AllowAddRemove | AllowForce, OptionUse::Optional, {"detach"}, "Locks the object (or an attachment point given as option); =force takes it off", Dialect::Rlv, Group::Inventory},
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
      {"edit", AllowAddRemove, OptionUse::Optional, {"edit"}, "Cannot edit objects; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"rez", AllowAddRemove, OptionUse::None, {"rez"}, "Cannot rez objects"},
      {"touchall", AllowAddRemove, OptionUse::None, {"touch"}, "Cannot touch any object", Dialect::Rlv, Group::Interaction},
      {"showinv", AllowAddRemove, OptionUse::None, {"showinv"}, "Inventory cannot be opened"},
      {"showloc", AllowAddRemove, OptionUse::None, {"showloc"}, "The location is hidden"},
      {"showminimap", AllowAddRemove, OptionUse::None, {"showminimap"}, "The minimap cannot be opened"},
      {"showworldmap", AllowAddRemove, OptionUse::None, {"showworldmap"}, "The world map cannot be opened"},
      {"share", AllowAddRemove, OptionUse::Optional, {"share"}, "Cannot give inventory; with a UUID as option: an exception"},
      {"version", AllowReply, OptionUse::None, {}, "Answers the RLV version on the channel (old name RestrainedLife)", Dialect::Rlv, Group::Reply},
      {"versionnew", AllowReply, OptionUse::None, {}, "Answers the RLV version on the channel (RestrainedLove)", Dialect::Rlv, Group::Reply},
      {"versionnum", AllowReply, OptionUse::None, {}, "Answers the version number on the channel, e.g. 3040300", Dialect::Rlv, Group::Reply},
      {"getstatus", AllowReply, OptionUse::Optional, {}, "Answers this object's restrictions on the channel; option: <filter>[;<separator>]", Dialect::Rlv, Group::Reply},
      {"getstatusall", AllowReply, OptionUse::Optional, {}, "Answers all objects' restrictions on the channel; option: <filter>[;<separator>]", Dialect::Rlv, Group::Reply},
      // ---- plain RLV with effects outside the sandbox: recorded as state, shown in the log ----
      {"setenv", AllowAddRemove, OptionUse::None, {"setenv"}, "Locks the sky and water settings", Dialect::Rlv, Group::Environment, true, "recorded only: the real sky and water do not change"},
      {"setdebug", AllowAddRemove, OptionUse::None, {"setdebug"}, "Locks the debug settings", Dialect::Rlv, Group::Environment, true, "recorded only: no debug setting changes"},
      {"setcam", AllowAddRemove, OptionUse::None, {"setcam"}, "Gives the object exclusive control of the camera", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_avdist", AllowAddRemove, OptionUse::Required, {}, "Distance at which nearby avatars turn into silhouettes (option: metres)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_eyeoffset", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Default camera offset (option: x/y/z)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_eyeoffsetscale", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Default camera offset scale (option: multiplier)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_focusoffset", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Default camera focus offset (option: x/y/z)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_fovmin", AllowAddRemove, OptionUse::Optional, {}, "Minimum field of view (option: angle)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_fovmax", AllowAddRemove, OptionUse::Optional, {}, "Maximum field of view (option: angle)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_mouselook", AllowAddRemove, OptionUse::None, {"setcam_mouselook"}, "Prevents mouselook", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_textures", AllowAddRemove, OptionUse::Optional, {}, "Replaces every texture with one texture (option: UUID)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setcam_unlock", AllowAddRemove, OptionUse::None, {}, "Forces the camera focus back to the avatar", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      // ---- old camera names: the same state as the new name; getstatus shows the name that was used ----
      {"camdistmax", AllowAddRemove, OptionUse::Required, {}, "Old name of setcam_avdistmax: maximum camera distance from the avatar (option: metres)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change", "setcam_avdistmax"},
      {"camdistmin", AllowAddRemove, OptionUse::Required, {}, "Old name of setcam_avdistmin: minimum camera distance from the avatar (option: metres)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change", "setcam_avdistmin"},
      {"camavdist", AllowAddRemove, OptionUse::Required, {}, "Old name of setcam_avdist: distance at which nearby avatars turn into silhouettes (option: metres)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change", "setcam_avdist"},
      {"camtextures", AllowAddRemove, OptionUse::Optional, {}, "Old name of setcam_textures: replaces every texture with one texture (option: UUID)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change", "setcam_textures"},
      {"camunlock", AllowAddRemove, OptionUse::None, {}, "Old name of setcam_unlock: forces the camera focus back to the avatar", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change", "setcam_unlock"},
      // the old zoom limits are a multiplier, not an angle, so they are their own entries and no alias of the field of view limits
      {"camzoommax", AllowAddRemove, OptionUse::Required, {}, "Old limit on how far the camera may zoom in (option: zoom multiplier)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"camzoommin", AllowAddRemove, OptionUse::Required, {}, "Old limit on how far the camera may zoom out (option: zoom multiplier)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      // ---- touching far away ----
      {"touchfar", AllowAddRemove, OptionUse::Optional, {"touchfar"}, "Cannot touch objects farther away than 1.5 m (or the distance given as option, in metres)", Dialect::Rlv, Group::Interaction},
      {"fartouch", AllowAddRemove, OptionUse::Optional, {}, "Old name of touchfar", Dialect::Rlv, Group::Interaction, false, QString(), "touchfar"},
      // ---- newer behaviours: marked RLV where the RLV wiki lists them (RLVa flags some of them extended), RLVa only otherwise ----
      // interaction and teleport
      {"accepttprequest", AllowAddRemove, OptionUse::Optional, {"accepttprequest"}, "Teleport requests are accepted automatically; with a UUID as option: only from that avatar", Dialect::Rlv, Group::Core},
      {"tprequest", AllowAddRemove, OptionUse::Optional, {"tprequest"}, "No teleport requests; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"tplocal", AllowAddRemove, OptionUse::Optional, {"tplocal"}, "No teleport within the region farther than the limit (or the distance given as option)", Dialect::Rlv, Group::Core},
      {"interact", AllowAddRemove, OptionUse::None, {"interact"}, "No interaction with the world (touch, edit and similar)", Dialect::Rlv, Group::Interaction},
      {"touchhud", AllowAddRemove, OptionUse::Optional, {"touchhud"}, "Cannot touch HUD objects; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"sendgesture", AllowAddRemove, OptionUse::None, {"sendgesture"}, "Gestures cannot send chat", Dialect::Rlv, Group::Interaction},
      {"showself", AllowAddRemove, OptionUse::None, {"showself"}, "Your own avatar is hidden from you", Dialect::RlvaOnly, Group::Interaction},
      {"showselfhead", AllowAddRemove, OptionUse::None, {"showselfhead"}, "Your own head attachments are hidden from you", Dialect::RlvaOnly, Group::Interaction},
      {"shownearby", AllowAddRemove, OptionUse::None, {"shownearby"}, "The list of people nearby is hidden", Dialect::Rlv, Group::Interaction},
      {"viewtransparent", AllowAddRemove, OptionUse::None, {"viewtransparent"}, "Transparent objects cannot be highlighted", Dialect::RlvaOnly, Group::Interaction, true, "recorded only: the real view does not change"},
      {"viewwireframe", AllowAddRemove, OptionUse::None, {"viewwireframe"}, "The wireframe view is locked", Dialect::RlvaOnly, Group::Interaction, true, "recorded only: the real view does not change"},
      {"allowidle", AllowAddRemove, OptionUse::None, {"allowidle"}, "The idle (away) timeout is lengthened to 30 minutes", Dialect::Rlv, Group::Interaction},
      {"sendchannel_except", AllowAddRemove, OptionUse::Optional, {}, "Chat only on the listed channel (option: channel); blocks every other channel", Dialect::Rlv, Group::Core},
      // shared folder
      {"sharedwear", AllowAddRemove, OptionUse::None, {"sharedwear"}, "Items from the shared folder cannot be worn", Dialect::Rlv, Group::Inventory},
      {"sharedunwear", AllowAddRemove, OptionUse::None, {"sharedunwear"}, "Items from the shared folder cannot be taken off", Dialect::Rlv, Group::Inventory},
      // camera
      {"setcam_avdistmin", AllowAddRemove, OptionUse::Required, {}, "Minimum camera distance from the avatar (option: metres)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_avdistmax", AllowAddRemove, OptionUse::Required, {}, "Maximum camera distance from the avatar (option: metres)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_origindistmin", AllowAddRemove, OptionUse::Required, {}, "Minimum camera distance from the camera origin (option: metres)", Dialect::RlvaOnly, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_origindistmax", AllowAddRemove, OptionUse::Required, {}, "Maximum camera distance from the camera origin (option: metres)", Dialect::RlvaOnly, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_focus", AllowForce, OptionUse::Optional, {}, "Forces the camera focus or position to an object, avatar or position (option: target)", Dialect::RlvaOnly, Group::Camera, true, "recorded only: the real camera does not move"},
      {"setcam_fov", AllowForce, OptionUse::Optional, {}, "Forces the vertical field of view (option: angle)", Dialect::Rlv, Group::Camera, true, "recorded only: the real camera does not change"},
      {"setcam_mode", AllowForce, OptionUse::Optional, {}, "Switches the camera mode, e.g. mouselook or thirdview (option: mode)", Dialect::RlvaOnly, Group::Camera, true, "recorded only: the real camera does not change"},
      // questions answered on a channel
      {"findfolders", AllowReply, OptionUse::Optional, {}, "Lists every shared folder whose name contains the option", Dialect::Rlv, Group::Reply, false, "the sandbox has no inventory: answers empty"},
      {"getcommand", AllowReply, OptionUse::Optional, {}, "Lists the behaviours this viewer knows whose name contains the option", Dialect::RlvaOnly, Group::Reply},
      {"getheightoffset", AllowReply, OptionUse::None, {}, "Answers the avatar height offset", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no avatar: answers 0"},
      {"getaddattachnames", AllowReply, OptionUse::Optional, {}, "Lists the attachment points where something can be added", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no avatar: answers empty"},
      {"getaddoutfitnames", AllowReply, OptionUse::Optional, {}, "Lists the clothing layers where something can be added", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no avatar: answers empty"},
      {"getattachnames", AllowReply, OptionUse::Optional, {}, "Lists the attachment points that are worn", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no avatar: answers empty"},
      {"getoutfitnames", AllowReply, OptionUse::Optional, {}, "Lists the clothing layers that are worn", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no avatar: answers empty"},
      {"getremattachnames", AllowReply, OptionUse::Optional, {}, "Lists the attachment points where something can be removed", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no avatar: answers empty"},
      {"getremoutfitnames", AllowReply, OptionUse::Optional, {}, "Lists the clothing layers where something can be removed", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no avatar: answers empty"},
      {"getcam_avdist", AllowReply, OptionUse::None, {}, "Answers the current camera distance from the avatar", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no camera: answers empty"},
      {"getcam_avdistmin", AllowReply, OptionUse::None, {}, "Answers the active minimum camera distance from the avatar", Dialect::Rlv, Group::Reply, false, "the sandbox has no camera: answers empty"},
      {"getcam_avdistmax", AllowReply, OptionUse::None, {}, "Answers the active maximum camera distance from the avatar", Dialect::Rlv, Group::Reply, false, "the sandbox has no camera: answers empty"},
      {"getcam_fov", AllowReply, OptionUse::None, {}, "Answers the current field of view (radians)", Dialect::Rlv, Group::Reply, false, "the sandbox has no camera: answers empty"},
      {"getcam_fovmin", AllowReply, OptionUse::None, {}, "Answers the active minimum field of view (radians)", Dialect::Rlv, Group::Reply, false, "the sandbox has no camera: answers empty"},
      {"getcam_fovmax", AllowReply, OptionUse::None, {}, "Answers the active maximum field of view (radians)", Dialect::Rlv, Group::Reply, false, "the sandbox has no camera: answers empty"},
      {"getcam_textures", AllowReply, OptionUse::None, {}, "Answers the UUID of the forced texture, if any", Dialect::RlvaOnly, Group::Reply, false, "the sandbox has no camera: answers empty"},
    };
    return table;
  }

  QString canonicalName(const QString& name)
  {
    const Behaviour* behaviour = findBehaviour(name);
    return (behaviour && !behaviour->alias.isEmpty()) ? behaviour->alias : name;
  }

  const Behaviour* findBehaviour(const QString& name)
  {
    for (const Behaviour& behaviour : behaviours())
      if (behaviour.name == name) return &behaviour;
    return nullptr;
  }

  const Behaviour* findBehaviour(const QString& name, Mode mode)
  {
    const Behaviour* behaviour = findBehaviour(name);
    if (behaviour && behaviour->dialect == Dialect::RlvaOnly && mode == Mode::Rlv) return nullptr;
    return behaviour;
  }

  QStringList behaviourNames(Mode mode)
  {
    QStringList names;
    for (const Behaviour& behaviour : behaviours())
      if (mode == Mode::Rlva || behaviour.dialect == Dialect::Rlv) names << behaviour.name;
    return names;
  }

  QString modeName(Mode mode)
  {
    return mode == Mode::Rlva ? QStringLiteral("rlva") : QStringLiteral("rlv");
  }

  bool parseMode(const QString& text, Mode& mode)
  {
    const QString lower = text.trimmed().toLower();
    if (lower == QLatin1String("rlv")) { mode = Mode::Rlv; return true; }
    if (lower == QLatin1String("rlva")) { mode = Mode::Rlva; return true; }
    return false;
  }

  QString groupName(Group group)
  {
    switch (group)
    {
      case Group::Core: return QStringLiteral("core");
      case Group::Inventory: return QStringLiteral("inventory");
      case Group::Camera: return QStringLiteral("camera");
      case Group::Environment: return QStringLiteral("environment");
      case Group::Interaction: return QStringLiteral("interaction");
      case Group::Reply: return QStringLiteral("reply");
    }
    return QStringLiteral("core");
  }

  QString checkCommand(const Command& command, Mode mode)
  {
    if (!command.valid) return command.error.isEmpty() ? QStringLiteral("bad syntax") : command.error;
    if (command.type == ParamType::Clear) return QString();

    const Behaviour* behaviour = findBehaviour(command.behaviour, mode);
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
    if (behaviour->option == OptionUse::Required && command.option.isEmpty() && command.type != ParamType::Reply)
      return QString("@%1 needs an option").arg(command.behaviour);
    return QString();
  }
}
