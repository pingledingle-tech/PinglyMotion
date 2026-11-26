/*
 * PinglyMotion Script Lab - LSL interpreter: a built-in, simulated RLV relay (job 23)
 *
 * A relay is the worn device that lets objects the avatar does not own (traps, capture guns, furniture,
 * capture HUDs) send RLV commands to the avatar's viewer. This one answers the relay protocol on
 * channel -1812221819, so such objects can be tested in the sandbox without putting a relay script on a
 * worn object. Written from the protocol's public description (relay protocol 1.100 and the ORG core
 * x-tension 0003); no relay's code was copied.
 *
 * A controller says (or sends with llRegionSayTo to the avatar) "<ident>,<avatar key>,<command>|<command>...".
 * Exactly three comma-separated fields; the wildcard key ffffffff-ffff-ffff-ffff-ffffffffffff counts as the avatar's
 * (ORG core); another avatar's key, or another number of fields, is ignored.
 * For every command the relay answers the sender on the same channel with
 * "<ident>,<controller key>,<command>,<answer>":
 *   !version         1100 (the protocol version)
 *   !implversion     the relay's name and version
 *   !x-orgversions   ORG=0003 (the ORG x-tensions it follows: only the core)
 *   !release         ok; every restriction of that controller is lifted, and it must ask again next time
 *   !pong            no answer (the controller's answer to the relay's ping)
 *   any other "!..." ko (ORG core: an unknown meta-command is refused; !handover and !who are unsettled and refused)
 *   @behaviour=...   ok or ko from the sandbox's RLV core, which holds the restriction under the controller's key
 *                    (restrictions per controller, ORG core); a query (@version=<channel> ...) is answered by the
 *                    viewer on its own channel, as the avatar
 *   anything else    ko
 *
 * Ask or accept: a controller that is not trusted yet must be accepted before a command that sets a restriction,
 * forces something or asks a query other than the version queries. Mode auto accepts every controller, mode deny
 * refuses every one, and mode ask "asks the avatar" through a simulated dialog: the answer is the next entry of
 * the answers list ("accept" or "decline"), or decline when the list is used up. Lifting a restriction (=y, =rem)
 * and @clear never need asking. The choice holds until the controller's !release.
 *
 * Reachability (ORG core): the relay checks that a controller still exists. A ping input sends
 * "ping,<controller key>,ping,ping" to every controller that holds a restriction and frees at once each one that is not
 * reachable: its object is no longer in the world (deleted with llDie) or stands beyond shout range (100 metres) of
 * the avatar. The relay never drops restrictions on its own, so a !pong has nothing to put back. A release input is the avatar freeing
 * herself from the relay's menu: every controller is freed and told "release,<controller key>,!release,ok"
 * (the protocol defines no safeword; this is what the sandbox does).
 *
 * QtCore only. Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_RELAY_H
#define LSL_RELAY_H

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

class RlvState;

namespace lsl
{

class RlvRelay
{
  public:
    static constexpr qint32 CHANNEL = -1812221819;
    static constexpr int PROTOCOL_VERSION = 1100;
    static constexpr int MAX_CONTROLLERS = 64;          // more are refused (ko), so the state stays bounded
    static constexpr int MAX_COMMANDS = 64;             // per message; the rest of a longer message is ignored
    static constexpr int MAX_ANSWERS = 64;              // entries of the answers list
    static const char* const IMPL_VERSION;              // "PinglyMotion Script Lab relay 1"
    static const char* const ORG_VERSIONS;              // "ORG=0003"
    static const char* const NAME;                      // the worn relay's object name
    static const char* const KEY;                       // and key

    enum class Mode { Auto, Ask, Deny };

    struct Settings
    {
      Mode mode = Mode::Auto;
      QStringList answers;   // mode ask: "accept" or "decline", one per controller asked, in order
    };

    // one line the relay or the viewer says to a controller
    struct Message
    {
      QString to;        // the controller's key
      int channel = CHANNEL;
      QString text;
      bool viewer = false;   // said by the viewer as the avatar (a query's answer), not by the relay
    };

    struct Heard
    {
      bool handled = false;       // false: not a relay message for this avatar (ignored says why)
      QString ignored;
      QVector<Message> messages;  // the relay's answers, then the viewer's answers to queries
      QJsonObject entry;          // the log entry ("type": "relay")
    };

    RlvRelay(RlvState& rlv, const QString& wearerKey, const Settings& settings);

    // A message on the relay channel from an object (its key and name).
    Heard hear(const QString& senderKey, const QString& senderName, const QString& text);

    // The avatar frees herself: every controller is freed and told so. The log entry is filled in.
    QVector<Message> releaseAll(QJsonObject& entry);

    // The reachability check: pings every controller that holds a restriction and frees each one that is not in
    // reachable (the keys of the objects still in the world and within shout range). The log entry is filled in.
    QVector<Message> ping(const QStringList& reachable, QJsonObject& entry);

    // {"mode", "controllers": [{key, name, trusted, restrictions}], "answers_left"}
    QJsonObject toJson() const;

    // "auto", "ask", "deny"; false for anything else
    static bool modeFromName(const QString& name, Mode& mode);
    static QString modeName(Mode mode);

  private:
    struct Controller
    {
      QString name;
      enum class Trust { Unknown, Accepted, Declined } trust = Trust::Unknown;
    };

    // ok or ko for one RLV command of a controller; replies collects the viewer's answers
    bool runRlv(const QString& key, Controller& c, const QString& command, QJsonObject& result, QVector<Message>& viewer);
    // whether the command needs the avatar's yes first
    static bool needsAsking(const QString& command);
    void release(const QString& key);

    RlvState& m_rlv;
    QString m_wearer;
    Settings m_settings;
    int m_nextAnswer = 0;
    QMap<QString, Controller> m_controllers;
};

} // namespace lsl

#endif
