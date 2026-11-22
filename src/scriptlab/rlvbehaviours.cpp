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
      {"chatshout", "Shout (shouts become normal chat)"},
      {"chatnormal", "Say in the normal voice (it becomes a whisper)"},
      {"chatwhisper", "Whisper (whispers become normal chat)"},
      {"recvemote", "See emotes"},
      {"startim", "Start a new instant message conversation"},
      {"shownames", "See the names of other avatars"},
      {"showinv", "Open the inventory"},
      {"showloc", "See the location"},
      {"showminimap", "Open the minimap"},
      {"showworldmap", "Open the world map"},
      {"share", "Give inventory to others"},
      // newer behaviours (some plain RLV, some RLVa only)
      {"setgroup", "Change the active group"},
      {"temprun", "Run temporarily"},
      {"alwaysrun", "Switch always run on"},
      {"jump", "Jump"},
      {"addattach", "Wear attachments"},
      {"remattach", "Take off attachments"},
      {"addoutfit", "Wear clothing"},
      {"remoutfit", "Take off clothing"},
      {"unsharedwear", "Wear items from outside the shared folder"},
      {"unsharedunwear", "Take off items from outside the shared folder"},
      {"attachthis", "Wear the items of a folder"},
      {"detachthis", "Take off the items of a folder"},
      {"attachallthis", "Wear a folder and its sub-folders"},
      {"detachallthis", "Take off a folder and its sub-folders"},
      {"touchworld", "Touch objects in the world"},
      {"touchattach", "Touch attachments of other avatars"},
      {"touchattachself", "Touch your own attachments"},
      {"touchattachother", "Touch attachments of one other avatar"},
      {"showhovertextall", "See hover text"},
      {"showhovertexthud", "See hover text on HUDs"},
      {"showhovertextworld", "See hover text in the world"},
      {"shownametags", "See name tags"},
      {"viewnote", "Open notecards"},
      {"viewscript", "Open scripts"},
      {"viewtexture", "Open textures"},
      {"editworld", "Edit objects in the world"},
      {"editattach", "Edit attachments"},
      {"buy", "Buy objects"},
      {"pay", "Pay"},
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
      // ---- public chat and emotes: restrictions (n/y) and exceptions (add/rem), secure forms (_sec) ignore exceptions of other objects ----
      {"chatshout", AllowAddRemove, OptionUse::None, {"chatshout"}, "Shouting is turned into normal chat", Dialect::Rlv, Group::Core},
      {"chatnormal", AllowAddRemove, OptionUse::None, {"chatnormal"}, "Normal chat is turned into a whisper", Dialect::Rlv, Group::Core},
      {"chatwhisper", AllowAddRemove, OptionUse::None, {"chatwhisper"}, "Whispering is turned into normal chat", Dialect::Rlv, Group::Core},
      {"redirchat", AllowAddRemove, OptionUse::Required, {}, "Public chat is sent to a private channel instead (option: channel, 1 or higher)", Dialect::Rlv, Group::Core},
      {"rediremote", AllowAddRemove, OptionUse::Required, {}, "Emotes are sent to a private channel instead (option: channel, 1 or higher)", Dialect::Rlv, Group::Core},
      {"emote", AllowAddRemove, OptionUse::None, {}, "Exception: emotes (lines that start with /me) are still allowed while public chat is blocked; blocks nothing itself", Dialect::Rlv, Group::Core},
      {"recvchat_sec", AllowAddRemove, OptionUse::Optional, {"recvchat"}, "Cannot hear public chat, and exceptions of other objects do not count; with a UUID as option: an exception for that avatar", Dialect::Rlv, Group::Core},
      {"recvchatfrom", AllowAddRemove, OptionUse::Required, {}, "Cannot hear public chat from one avatar (option: that avatar's UUID)", Dialect::Rlv, Group::Core},
      {"recvemote", AllowAddRemove, OptionUse::Optional, {"recvemote"}, "Cannot see emotes; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"recvemote_sec", AllowAddRemove, OptionUse::Optional, {"recvemote"}, "Cannot see emotes, and exceptions of other objects do not count; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"recvemotefrom", AllowAddRemove, OptionUse::Required, {}, "Cannot see emotes from one avatar (option: that avatar's UUID)", Dialect::Rlv, Group::Core},
      {"sendchannel_sec", AllowAddRemove, OptionUse::Optional, {"sendchannel"}, "No chat on channels other than 0, and exceptions of other objects do not count; with a channel as option: an exception", Dialect::Rlv, Group::Core},
      // ---- instant messages and other secure forms ----
      {"sendim_sec", AllowAddRemove, OptionUse::Optional, {"sendim"}, "No instant messages out, and exceptions of other objects do not count; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"sendimto", AllowAddRemove, OptionUse::Required, {}, "Cannot send instant messages to one avatar (option: that avatar's UUID)", Dialect::Rlv, Group::Core},
      {"startim", AllowAddRemove, OptionUse::Optional, {"startim"}, "Cannot start a new instant message conversation; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"startimto", AllowAddRemove, OptionUse::Required, {}, "Cannot start a conversation with one avatar (option: that avatar's UUID)", Dialect::Rlv, Group::Core},
      {"recvim_sec", AllowAddRemove, OptionUse::Optional, {"recvim"}, "No instant messages in, and exceptions of other objects do not count; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"recvimfrom", AllowAddRemove, OptionUse::Required, {}, "Cannot receive instant messages from one avatar (option: that avatar's UUID)", Dialect::Rlv, Group::Core},
      {"share_sec", AllowAddRemove, OptionUse::Optional, {"share"}, "Cannot give inventory, and exceptions of other objects do not count; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"tplure_sec", AllowAddRemove, OptionUse::Optional, {"tplure"}, "Cannot accept teleport offers, and exceptions of other objects do not count; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"tprequest_sec", AllowAddRemove, OptionUse::Optional, {"tprequest"}, "No teleport requests, and exceptions of other objects do not count; with a UUID as option: an exception", Dialect::Rlv, Group::Core},
      {"shownames_sec", AllowAddRemove, OptionUse::Optional, {"shownames"}, "Names of other avatars are hidden, and exceptions of other objects do not count; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      // ---- notifications and permissive ----
      {"notify", AllowAddRemove, OptionUse::Required, {}, "Tells the object on a channel whenever it adds or lifts a restriction: \"/<rule>=n\" or \"/<rule>=y\" (option: <channel>[;filter], channel 1 or higher, the filter must be part of the rule name)", Dialect::Rlv, Group::Core, false, "the sandbox sends these messages only for changes made by the object that holds the notification"},
      {"permissive", AllowAddRemove, OptionUse::None, {}, "Marks the object as permissive; the sandbox keeps it as state and does not model how _sec restrictions and exceptions of different objects combine", Dialect::Rlv, Group::Core},
      // ---- outfits and attachments (the sandbox has no avatar or inventory: force commands are retained, not carried out) ----
      {"addattach", AllowAddRemove, OptionUse::Optional, {"addattach"}, "Nothing can be worn (on an attachment point given as option)", Dialect::Rlv, Group::Inventory},
      {"remattach", AllowAddRemove | AllowForce, OptionUse::Optional, {"remattach"}, "Nothing can be taken off (from an attachment point given as option); =force takes off what is worn there", Dialect::Rlv, Group::Inventory},
      {"addoutfit", AllowAddRemove, OptionUse::Optional, {"addoutfit"}, "No clothing can be worn (on a layer given as option)", Dialect::Rlv, Group::Inventory},
      {"remoutfit", AllowAddRemove | AllowForce, OptionUse::Optional, {"remoutfit"}, "No clothing can be taken off (from a layer given as option); =force takes off what is worn there", Dialect::Rlv, Group::Inventory},
      {"detachme", AllowForce, OptionUse::None, {}, "=force takes off the object that sent the command", Dialect::Rlv, Group::Inventory},
      {"defaultwear", AllowAddRemove, OptionUse::None, {}, "Held as state only: the sandbox has no inventory, so it blocks nothing (in the RLV wiki, not in RLVa)", Dialect::RlvOnly, Group::Inventory},
      {"unsharedwear", AllowAddRemove, OptionUse::None, {"unsharedwear"}, "Items from outside the shared folder cannot be worn", Dialect::Rlv, Group::Inventory},
      {"unsharedunwear", AllowAddRemove, OptionUse::None, {"unsharedunwear"}, "Items from outside the shared folder cannot be taken off", Dialect::Rlv, Group::Inventory},
      {"attach", AllowForce, OptionUse::Required, {}, "=force wears a shared folder, replacing what is worn on the same points (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"attachover", AllowForce, OptionUse::Required, {}, "=force wears a shared folder without replacing anything (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"attachoverorreplace", AllowForce, OptionUse::Required, {}, "=force wears a shared folder, adding or replacing as the folder name says (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"attachall", AllowForce, OptionUse::Required, {}, "=force wears a shared folder and its sub-folders, replacing (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"attachallover", AllowForce, OptionUse::Required, {}, "=force wears a shared folder and its sub-folders without replacing (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"attachalloverorreplace", AllowForce, OptionUse::Required, {}, "=force wears a shared folder and its sub-folders, adding or replacing as the names say (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"detachall", AllowForce, OptionUse::Required, {}, "=force takes off a shared folder and its sub-folders (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"attachthis", AllowAddRemove | AllowForce, OptionUse::Optional, {"attachthis"}, "Items of the folder cannot be worn (option: folder path or a point or layer name); =force wears the folder of this object, or the one given as option", Dialect::Rlv, Group::Inventory},
      {"attachthisover", AllowForce, OptionUse::Optional, {}, "=force wears the folder of this object, or the one given as option, without replacing anything", Dialect::Rlv, Group::Inventory},
      {"attachthisoverorreplace", AllowForce, OptionUse::Optional, {}, "=force wears the folder of this object, or the one given as option, adding or replacing as the names say", Dialect::Rlv, Group::Inventory},
      {"attachallthis", AllowAddRemove | AllowForce, OptionUse::Optional, {"attachallthis"}, "The folder and its sub-folders cannot be worn (option as for attachthis); =force wears them", Dialect::Rlv, Group::Inventory},
      {"attachallthisover", AllowForce, OptionUse::Optional, {}, "=force wears the folder and its sub-folders of this object (or the option) without replacing", Dialect::Rlv, Group::Inventory},
      {"attachallthisoverorreplace", AllowForce, OptionUse::Optional, {}, "=force wears the folder and its sub-folders of this object (or the option), adding or replacing as the names say", Dialect::Rlv, Group::Inventory},
      {"detachthis", AllowAddRemove | AllowForce, OptionUse::Optional, {"detachthis"}, "Items of the folder cannot be taken off (option as for attachthis); =force takes the folder off", Dialect::Rlv, Group::Inventory},
      {"detachallthis", AllowAddRemove | AllowForce, OptionUse::Optional, {"detachallthis"}, "The folder and its sub-folders cannot be taken off (option as for attachthis); =force takes them off", Dialect::Rlv, Group::Inventory},
      {"attachthis_except", AllowAddRemove, OptionUse::Required, {}, "Exception to attachthis: this folder stays allowed (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"attachallthis_except", AllowAddRemove, OptionUse::Required, {}, "Exception to attachallthis: this folder stays allowed (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"detachthis_except", AllowAddRemove, OptionUse::Required, {}, "Exception to detachthis: this folder stays allowed (option: folder path)", Dialect::Rlv, Group::Inventory},
      {"detachallthis_except", AllowAddRemove, OptionUse::Required, {}, "Exception to detachallthis: this folder stays allowed (option: folder path)", Dialect::Rlv, Group::Inventory},
      // ---- touching, hover text, names, viewing, editing, money ----
      {"touchworld", AllowAddRemove, OptionUse::Optional, {"touchworld"}, "Cannot touch objects in the world; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"touchthis", AllowAddRemove, OptionUse::Required, {}, "Cannot touch one object (option: that object's UUID)", Dialect::Rlv, Group::Interaction},
      {"touchme", AllowAddRemove, OptionUse::None, {}, "Exception: the object that sent it can be touched even when touching is blocked", Dialect::Rlv, Group::Interaction},
      {"touchattach", AllowAddRemove, OptionUse::None, {"touchattach"}, "Cannot touch attachments of other avatars", Dialect::Rlv, Group::Interaction},
      {"touchattachself", AllowAddRemove, OptionUse::None, {"touchattachself"}, "Cannot touch your own attachments", Dialect::Rlv, Group::Interaction},
      {"touchattachother", AllowAddRemove, OptionUse::Optional, {"touchattachother"}, "Cannot touch attachments of other avatars (option: only that avatar's UUID)", Dialect::Rlv, Group::Interaction},
      {"showhovertextall", AllowAddRemove, OptionUse::None, {"showhovertextall"}, "No hover text is shown", Dialect::Rlv, Group::Interaction},
      {"showhovertext", AllowAddRemove, OptionUse::Required, {}, "The hover text of one object is hidden (option: that object's UUID)", Dialect::Rlv, Group::Interaction},
      {"showhovertexthud", AllowAddRemove, OptionUse::None, {"showhovertexthud"}, "No hover text is shown on HUDs", Dialect::Rlv, Group::Interaction},
      {"showhovertextworld", AllowAddRemove, OptionUse::None, {"showhovertextworld"}, "No hover text is shown in the world", Dialect::Rlv, Group::Interaction},
      {"shownames", AllowAddRemove, OptionUse::Optional, {"shownames"}, "Names of other avatars are hidden; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"shownametags", AllowAddRemove, OptionUse::Optional, {"shownametags"}, "Name tags above avatars are hidden; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"viewnote", AllowAddRemove, OptionUse::Optional, {"viewnote"}, "Notecards cannot be opened; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"viewscript", AllowAddRemove, OptionUse::Optional, {"viewscript"}, "Scripts cannot be opened; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"viewtexture", AllowAddRemove, OptionUse::Optional, {"viewtexture"}, "Textures cannot be opened; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"editobj", AllowAddRemove, OptionUse::Required, {}, "Cannot edit one object (option: that object's UUID)", Dialect::Rlv, Group::Interaction},
      {"editworld", AllowAddRemove, OptionUse::None, {"editworld"}, "Cannot edit objects in the world", Dialect::Rlv, Group::Interaction},
      {"editattach", AllowAddRemove, OptionUse::None, {"editattach"}, "Cannot edit attachments", Dialect::Rlv, Group::Interaction},
      {"buy", AllowAddRemove, OptionUse::None, {"buy"}, "Cannot buy objects", Dialect::Rlv, Group::Interaction},
      {"pay", AllowAddRemove, OptionUse::Optional, {"pay"}, "Cannot pay; with a UUID as option: an exception", Dialect::Rlv, Group::Interaction},
      {"acceptpermission", AllowAddRemove, OptionUse::None, {}, "Permission requests of attached objects are accepted without asking; held as state, the sandbox grants permissions to scripts anyway", Dialect::Rlv, Group::Core},
      {"denypermission", AllowAddRemove, OptionUse::None, {}, "Permission requests of objects are refused without asking (in the RLV wiki, not in RLVa; the wiki lists it as deprecated)", Dialect::RlvOnly, Group::Core},
      // ---- group, movement and teleport (force commands are retained, not carried out) ----
      {"setgroup", AllowAddRemove | AllowForce, OptionUse::Optional, {"setgroup"}, "The active group cannot be changed; =force sets it (option: group name or UUID, or none)", Dialect::Rlv, Group::Core},
      {"temprun", AllowAddRemove, OptionUse::None, {"temprun"}, "Cannot run for a moment (double-tap or run key)", Dialect::Rlv, Group::Interaction},
      {"alwaysrun", AllowAddRemove, OptionUse::None, {"alwaysrun"}, "Cannot switch always run on", Dialect::Rlv, Group::Interaction},
      {"jump", AllowAddRemove, OptionUse::None, {"jump"}, "Cannot jump", Dialect::Rlv, Group::Interaction},
      {"setrot", AllowForce, OptionUse::Required, {}, "=force turns the avatar to an angle (option: radians); held nowhere, the sandbox has no avatar. In the RLV wiki, not in RLVa", Dialect::RlvOnly, Group::Interaction},
      {"adjustheight", AllowForce, OptionUse::Required, {}, "=force changes the avatar height offset (option: offset[;factor[;delta]])", Dialect::Rlv, Group::Interaction},
      {"tpto", AllowForce, OptionUse::Required, {}, "=force teleports to a place (option: x/y/z or region/x/y/z, then an optional look-at)", Dialect::Rlv, Group::Core},
      {"standtp", AllowAddRemove, OptionUse::None, {}, "Changes where the avatar ends up after it stands up; held as state, the sandbox has no avatar position", Dialect::Rlv, Group::Core},
      {"accepttp", AllowAddRemove, OptionUse::Optional, {}, "Teleport offers are accepted automatically (option: only from this avatar's UUID); held as state, the sandbox has no teleports", Dialect::Rlv, Group::Core},
      {"sitground", AllowForce, OptionUse::None, {}, "=force sits the avatar on the ground", Dialect::Rlv, Group::Interaction},
      // ---- questions with fixed sandbox answers (no avatar, group or inventory model yet) ----
      {"getsitid", AllowReply, OptionUse::None, {}, "Answers the UUID of the object sat on; the sandbox answers the null key (nobody sits)", Dialect::Rlv, Group::Reply},
      {"getgroup", AllowReply, OptionUse::None, {}, "Answers the name of the active group; the sandbox answers none", Dialect::Rlv, Group::Reply},
      {"getoutfit", AllowReply, OptionUse::Optional, {}, "Answers 0 or 1 for each clothing layer worn (option: one layer name for a single digit); the sandbox wears nothing and answers zeros", Dialect::Rlv, Group::Reply},
      {"getattach", AllowReply, OptionUse::Optional, {}, "Answers 0 or 1 for each attachment point (option: one point name for a single digit); the sandbox wears nothing and answers zeros", Dialect::Rlv, Group::Reply},
      {"getinv", AllowReply, OptionUse::Optional, {}, "Answers the sub-folders of a shared folder (option: folder path); the sandbox has no inventory and answers empty", Dialect::Rlv, Group::Reply},
      {"getinvworn", AllowReply, OptionUse::Optional, {}, "Answers what is worn from a shared folder (option: folder path); the sandbox has no inventory and answers empty", Dialect::Rlv, Group::Reply},
      {"findfolder", AllowReply, OptionUse::Optional, {}, "Answers the path of the first shared folder whose name contains the option; the sandbox has no inventory and answers empty", Dialect::Rlv, Group::Reply},
      {"getpath", AllowReply, OptionUse::Optional, {}, "Answers the folder path of the object or of an attachment point (option: point or layer); the sandbox has no inventory and answers empty", Dialect::Rlv, Group::Reply},
      {"getpathnew", AllowReply, OptionUse::Optional, {}, "Answers the folder paths of the object, an attachment point or a layer (option); the sandbox has no inventory and answers empty", Dialect::Rlv, Group::Reply},
      // ---- more camera, sphere and overlay effects: recorded as state, nothing real changes ----
      {"camdrawmin", AllowAddRemove, OptionUse::Required, {}, "Distance in front of the camera where the draw distance fade starts (option: metres)", Dialect::RlvOnly, Group::Camera, true, "recorded only: the real view does not change"},
      {"camdrawmax", AllowAddRemove, OptionUse::Required, {}, "Distance where everything is hidden (option: metres)", Dialect::RlvOnly, Group::Camera, true, "recorded only: the real view does not change"},
      {"camdrawalphamin", AllowAddRemove, OptionUse::Required, {}, "Fade alpha at the near distance (option: 0 to 1)", Dialect::RlvOnly, Group::Camera, true, "recorded only: the real view does not change"},
      {"camdrawalphamax", AllowAddRemove, OptionUse::Required, {}, "Fade alpha at the far distance (option: 0 to 1)", Dialect::RlvOnly, Group::Camera, true, "recorded only: the real view does not change"},
      {"camdrawcolor", AllowAddRemove, OptionUse::Required, {}, "Colour of the fade (option: red/green/blue, each 0 to 1)", Dialect::RlvOnly, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere", AllowAddRemove, OptionUse::None, {}, "Turns on the vision sphere effect that hides or blurs what is far from the camera or an origin", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_mode", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "How the sphere acts (option: mode number)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_origin", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Where the sphere is centred (option: origin number)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_color", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Colour of the sphere (option: red/green/blue)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_distmin", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Distance where the effect starts (option: metres)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_distmax", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Distance where the effect is at full strength (option: metres)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_distextend", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "How far the distances are stretched (option: factor)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_param", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Strength of the effect (option: value)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_tween", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Seconds the effect takes to change (option: seconds)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_valuemin", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Strength at the near distance (option: value)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setsphere_valuemax", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Strength at the far distance (option: value)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setoverlay", AllowAddRemove, OptionUse::None, {}, "Turns on a full screen overlay picture", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setoverlay_alpha", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "How see-through the overlay is (option: 0 to 1)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setoverlay_texture", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Picture of the overlay (option: texture UUID)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setoverlay_tint", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Colour of the overlay (option: red/green/blue)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setoverlay_tween", AllowAddRemove | AllowForce, OptionUse::Optional, {}, "Seconds the overlay takes to change (option: seconds)", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      {"setoverlay_touch", AllowAddRemove, OptionUse::None, {}, "Touches go through the overlay to the world", Dialect::Rlv, Group::Camera, true, "recorded only: the real view does not change"},
      // ---- questions: the sandbox answers fixed values ----
      {"versionnumbl", AllowReply, OptionUse::None, {}, "Answers the version number, a comma, then the list of blacklisted behaviours; the sandbox blacklists nothing, so the list is empty (in the RLV wiki, not in RLVa)", Dialect::RlvOnly, Group::Reply},
      {"getblacklist", AllowReply, OptionUse::Optional, {}, "Answers the blacklisted behaviours (option: part of a name); the sandbox blacklists nothing and answers empty (in the RLV wiki, not in RLVa)", Dialect::RlvOnly, Group::Reply},
      {"getcam_zoommin", AllowReply, OptionUse::None, {}, "Answers the smallest zoom multiplier now allowed; the sandbox has no camera and always answers 1 (in the RLV wiki, not in RLVa)", Dialect::RlvOnly, Group::Reply},
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

  const QStringList& clothingLayers()
  {
    static const QStringList list = {"gloves", "jacket", "pants", "shirt", "shoes", "skirt", "socks", "underpants",
                                     "undershirt", "skin", "eyes", "hair", "shape", "alpha", "tattoo", "physics"};
    return list;
  }

  const QStringList& attachmentPoints()
  {
    static const QStringList list = {"none", "chest", "skull", "left shoulder", "right shoulder", "left hand", "right hand",
                                     "left foot", "right foot", "spine", "pelvis", "mouth", "chin", "left ear", "right ear",
                                     "left eyeball", "right eyeball", "nose", "r upper arm", "r forearm", "l upper arm",
                                     "l forearm", "right hip", "r upper leg", "r lower leg", "left hip", "l upper leg",
                                     "l lower leg", "stomach", "left pec", "right pec", "center 2", "top right", "top",
                                     "top left", "center", "bottom left", "bottom", "bottom right", "neck", "root"};
    return list;
  }

  QString wornString(const QStringList& names, const QString& option)
  {
    if (option.isEmpty()) return QString(names.size(), QLatin1Char('0'));
    return names.contains(option) ? QStringLiteral("0") : QString();
  }

  // setdebug_<name>, setenv_<name>, getdebug_<name>, getenv_<name>: any name after the prefix
  static const QVector<Behaviour>& families()
  {
    static const QVector<Behaviour> list =
    {
      {"setdebug_", AllowForce, OptionUse::Required, {}, "Sets one debug setting (setdebug_<name>:<value>=force)", Dialect::Rlv, Group::Environment, true, "recorded only: no debug setting changes"},
      {"setenv_", AllowForce, OptionUse::Required, {}, "Sets one sky or water setting (setenv_<name>:<value>=force)", Dialect::Rlv, Group::Environment, true, "recorded only: the real sky and water do not change"},
      {"getdebug_", AllowReply, OptionUse::None, {}, "Answers one debug setting (getdebug_<name>=<channel>); the sandbox answers the value last set with setdebug_<name>, or empty", Dialect::Rlv, Group::Reply},
      {"getenv_", AllowReply, OptionUse::None, {}, "Answers one sky or water setting (getenv_<name>=<channel>); the sandbox answers the value last set with setenv_<name>, or empty", Dialect::Rlv, Group::Reply},
    };
    return list;
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
    for (const Behaviour& family : families())
      if (name.length() > family.name.length() && name.startsWith(family.name)) return &family;
    return nullptr;
  }

  const Behaviour* findBehaviour(const QString& name, Mode mode)
  {
    const Behaviour* behaviour = findBehaviour(name);
    if (behaviour && behaviour->dialect == Dialect::RlvaOnly && mode == Mode::Rlv) return nullptr;
    if (behaviour && behaviour->dialect == Dialect::RlvOnly && mode == Mode::Rlva) return nullptr;
    return behaviour;
  }

  QStringList behaviourNames(Mode mode)
  {
    QStringList names;
    for (const Behaviour& behaviour : behaviours())
      if (findBehaviour(behaviour.name, mode)) names << behaviour.name;
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
