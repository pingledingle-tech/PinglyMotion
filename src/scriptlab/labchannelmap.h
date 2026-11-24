/*
 * PinglyMotion Script Lab - the channel map: who talks to whom in a run (job A4)
 *
 * Reads the event log of a Script Lab run (a folder watched as one object, or
 * any lsl_run answer) and makes a map of it: the parts that talk (prims, or
 * objects when the log names no prims), you (the avatar), your viewer (RLV)
 * and a web server outside Second Life, as boxes; every message as an arrow
 * from the box that sent it to each box that got it, with its channel and its
 * text. A message on a channel nobody listens on gets an arrow to nobody with
 * the error in plain words ("Nobody listens on channel 7.").
 *
 * Who hears what follows the sandbox's own rules (lsl::Sandbox::chat,
 * lsl::Interpreter::receive): the listens of each script as the log says
 * (llListen, llListenRemove, a state change or a reset removes them, a stopped
 * script hears nothing), their name, key and message filters, a prim never
 * hears itself, llRegionSayTo reaches only the object with that key, open
 * chat (channel 0) is heard by you, a link message reaches the prims its link
 * number names. Distances are not checked (the map shows one place), and
 * llListenControl is not in the log, so a listen switched off still counts.
 *
 * A box that hears chat or a link message from another box and passes
 * messages on to another box is a hub ("listens and passes on"): the server
 * of a Second Life setup. llHTTPRequest goes to the box "Web server": the
 * sandbox never goes on the network, it answers from canned replies.
 *
 * Each arrow knows the script line that sent it (file, line, column), found
 * in the parsed script: the call of the function that wrote the log entry,
 * the one whose line holds the message text when there are several.
 *
 * QtCore only. Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABCHANNELMAP_H
#define LABCHANNELMAP_H

#include <functional>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

namespace LabChannelMap
{
  // at most this many arrows (same sender, receiver, kind and channel are one arrow with a count)
  static constexpr int MAX_ARROWS = 200;

  enum class BoxKind { Part, You, Viewer, Web };

  struct Box
  {
    BoxKind kind = BoxKind::Part;
    QString name;        // the prim's (or object's) name; "You", "Your viewer (RLV)", "Web server"
    QString object;      // the object a part belongs to ("" for the other kinds)
    int link = 0;        // the prim's link number, 0 when not known
    bool hub = false;    // listens and passes on
    // the box's words: "Collar", "Root (hub)"
    QString label() const;
    // one line on what the box is, for its tooltip
    QString explain() const;
  };

  enum class ArrowKind
  {
    Chat,          // llSay, llWhisper, llShout, llRegionSay, llRegionSayTo
    LinkMessage,   // llMessageLinked
    Menu,          // llDialog, llTextBox: the menu shown to you
    MenuAnswer,    // you pressed a menu button: said on the menu's channel
    YouSay,        // you said something on a channel
    Touch,         // you clicked (touched) a part
    OwnerSay,      // llOwnerSay without @, llInstantMessage
    Rlv,           // llOwnerSay("@..."): an RLV line to your viewer
    RlvReply,      // your viewer's answer to an RLV question, on its channel
    Http,          // llHTTPRequest to a web server outside Second Life
    HttpIn         // a request from outside to a URL a script holds
  };

  struct Arrow
  {
    ArrowKind kind = ArrowKind::Chat;
    int from = -1;       // a box
    int to = -1;         // a box, or -1: nobody got it (error says why)
    QString how;         // the log's word: say, whisper, shout, region_say, region_say_to, owner_say, ...
    int channel = 0;     // chat, menu and RLV reply channel; the number of a link message
    bool hasChannel = false;
    QString text;        // the first message's text
    int count = 1;       // how many messages this arrow stands for
    double time = 0.0;   // when the first was sent (simulated seconds)
    QString error;       // why nobody got it, in plain words ("" when it arrived)
    // the script line that sent the first message ("" and 0 when it came from outside)
    QString file;
    QString script;
    int line = 0;
    int column = 0;
    // the arrow's words: "say on channel 7: \"lock\"", "link message 7002: \"lock\" (8 times)"
    QString label() const;
  };

  struct Map
  {
    QVector<Box> boxes;
    QVector<Arrow> arrows;
    bool truncated = false;   // more than MAX_ARROWS arrows: the rest are left out
    int unheard() const;      // arrows that reached nobody
    int messages() const;     // all messages, counted
    int indexOf(BoxKind kind, const QString& name = QString()) const;
    // {"boxes":[{kind, name, object, link, hub}], "arrows":[{kind, from, to, how, channel, text, count, t, error, file, line, column}]}
    QJsonObject toJson() const;
  };

  // the words of a box and an arrow kind in toJson ("part", "you", ...; "chat", "link_message", ...)
  QString kindName(BoxKind kind);
  QString kindName(ArrowKind kind);

  // A prim of the object the log belongs to (a folder watched as one object).
  struct Prim
  {
    int link = 1;
    QString name;
    bool hasScripts = false;
  };

  // Makes the map of one run.
  //   log:     the run's event log (lsl_run "log"; entries tagged with "object", "script", and in a folder
  //            run "prim" and "file")
  //   objects: the run's "objects" (name and key of each object), for the key filters and llRegionSayTo
  //   prims:   the prims of the object in a folder run, link 1 first (empty for other runs)
  //   source:  gives the source of a script file (folder run: "file"; otherwise the script's name); may be empty
  Map build(const QJsonArray& log, const QJsonArray& objects, const QVector<Prim>& prims,
            const std::function<QString(const QString& file)>& source = nullptr);

  // The map of LabWatcher::folderResult (its "linkset" prims and the run's log); an empty map when the
  // folder did not run.
  Map fromFolderResult(const QJsonObject& folderResult, const std::function<QString(const QString& file)>& source = nullptr);

  // The line (from 1) and column of the call of function in source that sent text: the first call whose
  // line holds text, else the first call. 0 when the source has no such call or does not parse.
  struct Place { int line = 0; int column = 0; };
  Place findCall(const QString& source, const QStringList& functions, const QString& text);
}

#endif
