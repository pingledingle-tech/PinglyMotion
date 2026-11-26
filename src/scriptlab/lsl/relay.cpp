/*
 * PinglyMotion Script Lab - LSL interpreter: a built-in, simulated RLV relay (job 23)
 *
 * Written from the relay protocol's public description (1.100) and the ORG core x-tension (0003); see relay.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "relay.h"

#include "scriptlab/rlvcommand.h"
#include "scriptlab/rlvstate.h"
#include "world.h"

namespace lsl
{

const char* const RlvRelay::IMPL_VERSION = "PinglyMotion Script Lab relay 1";
const char* const RlvRelay::ORG_VERSIONS = "ORG=0003";
const char* const RlvRelay::NAME = "RLV Relay";
const char* const RlvRelay::KEY = "4e1a7000-0000-4000-8000-0000000071a7";

RlvRelay::RlvRelay(RlvState& rlv, const QString& wearerKey, const Settings& settings)
  : m_rlv(rlv), m_wearer(wearerKey), m_settings(settings) {}

bool RlvRelay::modeFromName(const QString& name, Mode& mode)
{
  if (name == QLatin1String("auto")) mode = Mode::Auto;
  else if (name == QLatin1String("ask")) mode = Mode::Ask;
  else if (name == QLatin1String("deny")) mode = Mode::Deny;
  else return false;
  return true;
}

QString RlvRelay::modeName(Mode mode)
{
  switch (mode)
  {
    case Mode::Auto: return QStringLiteral("auto");
    case Mode::Ask: return QStringLiteral("ask");
    case Mode::Deny: return QStringLiteral("deny");
  }
  return QString();
}

bool RlvRelay::needsAsking(const QString& command)
{
  const Rlv::Command c = Rlv::parseCommand(command.mid(1).toLower());
  if (c.type == Rlv::ParamType::Remove || c.type == Rlv::ParamType::Clear) return false;
  if (c.type == Rlv::ParamType::Reply)
  {
    static const QStringList versionQueries = {"version", "versionnew", "versionnum", "versionnumbl"};
    return !versionQueries.contains(c.behaviour);
  }
  return true;
}

void RlvRelay::release(const QString& key)
{
  m_rlv.apply(key, QStringLiteral("@clear"));
  m_controllers.remove(key);
}

bool RlvRelay::runRlv(const QString& key, Controller& c, const QString& command, QJsonObject& result, QVector<Message>& viewer)
{
  if (needsAsking(command))
  {
    if (c.trust == Controller::Trust::Unknown)
    {
      QString answer;
      if (m_settings.mode == Mode::Auto) answer = QStringLiteral("accept");
      else if (m_settings.mode == Mode::Deny) answer = QStringLiteral("decline");
      else
      {
        answer = m_nextAnswer < m_settings.answers.size() ? m_settings.answers[m_nextAnswer] : QStringLiteral("decline");
        ++m_nextAnswer;
        result["asked"] = true;
      }
      c.trust = answer == QLatin1String("accept") ? Controller::Trust::Accepted : Controller::Trust::Declined;
      result["answer"] = answer;
    }
    if (c.trust == Controller::Trust::Declined)
    {
      result["reason"] = QStringLiteral("The avatar did not accept this object.");
      return false;
    }
  }
  const RlvState::ApplyResult applied = m_rlv.apply(key, command);
  if (!applied.ok())
  {
    result["reason"] = applied.error;
    return false;
  }
  bool ok = !applied.outcomes.isEmpty();
  for (const RlvState::Outcome& o : applied.outcomes)
  {
    result["result"] = Rlv::resultName(o.result);
    if (!o.reason.isEmpty()) result["reason"] = o.reason;
    if (o.result == Rlv::Result::Failed) ok = false;
  }
  for (const RlvState::Reply& r : applied.replies)
  {
    Message m;
    m.to = key;
    m.channel = r.channel;
    m.text = r.text;
    m.viewer = true;
    viewer.push_back(m);
  }
  return ok;
}

RlvRelay::Heard RlvRelay::hear(const QString& senderKey, const QString& senderName, const QString& text)
{
  Heard heard;
  const QString line = World::clip(text);
  heard.entry = QJsonObject{{"type", "relay"}, {"from", senderKey}, {"object", World::clip(senderName)}, {"text", line}};
  const QStringList fields = text.split(QLatin1Char(','));
  if (fields.size() != 3)
  {
    heard.ignored = QStringLiteral("Not a relay message: it must be <ident>,<avatar key>,<commands>, with exactly two commas.");
    heard.entry["ignored"] = heard.ignored;
    return heard;
  }
  if (fields[1] != m_wearer)
  {
    heard.ignored = QStringLiteral("The message is for another avatar.");
    heard.entry["ignored"] = heard.ignored;
    return heard;
  }
  heard.handled = true;
  const QString& ident = fields[0];
  QStringList commands = fields[2].split(QLatin1Char('|'), Qt::SkipEmptyParts);
  if (commands.size() > MAX_COMMANDS)
  {
    heard.entry["note"] = QString("Only the first %1 commands were run.").arg(MAX_COMMANDS);
    commands = commands.mid(0, MAX_COMMANDS);
  }

  const bool known = m_controllers.contains(senderKey);
  const bool full = !known && m_controllers.size() >= MAX_CONTROLLERS;
  QJsonArray results;
  QVector<Message> viewer;
  for (const QString& command : commands)
  {
    QJsonObject r{{"command", World::clip(command)}};
    QString answer = QStringLiteral("ko");
    bool reply = true;
    if (command == QLatin1String("!version")) answer = QString::number(PROTOCOL_VERSION);
    else if (command == QLatin1String("!implversion")) answer = QString::fromLatin1(IMPL_VERSION);
    else if (command == QLatin1String("!x-orgversions")) answer = QString::fromLatin1(ORG_VERSIONS);
    else if (command == QLatin1String("!release"))
    {
      if (m_controllers.contains(senderKey)) release(senderKey);
      answer = QStringLiteral("ok");
    }
    else if (command == QLatin1String("!pong"))
    {
      reply = false;
      r["note"] = QStringLiteral("The controller answered the relay's ping.");
    }
    else if (command.startsWith(QLatin1Char('!')))
      r["reason"] = QStringLiteral("A meta-command this relay does not know (ORG core: refused with ko).");
    else if (!command.startsWith(QLatin1Char('@')))
      r["reason"] = QStringLiteral("Not an RLV command (it must start with @) or a meta-command (it must start with !).");
    else if (full)
      r["reason"] = QString("The relay holds %1 controllers already.").arg(MAX_CONTROLLERS);
    else
    {
      Controller& c = m_controllers[senderKey];
      c.name = World::clip(senderName);
      if (runRlv(senderKey, c, command, r, viewer)) answer = QStringLiteral("ok");
      // a controller that was never asked and holds nothing is not kept (a decline is kept until its !release)
      if (c.trust == Controller::Trust::Unknown && m_rlv.objects().value(senderKey).isEmpty()) m_controllers.remove(senderKey);
    }
    if (reply)
    {
      Message m;
      m.to = senderKey;
      m.text = ident + QLatin1Char(',') + senderKey + QLatin1Char(',') + command + QLatin1Char(',') + answer;
      heard.messages.push_back(m);
      r["reply"] = answer;
    }
    results.append(r);
  }
  heard.messages += viewer;
  QJsonArray replies;
  for (const Message& m : heard.messages)
  {
    QJsonObject o{{"channel", m.channel}, {"text", World::clip(m.text)}};
    if (m.viewer) o["viewer"] = true;
    replies.append(o);
  }
  heard.entry["commands"] = results;
  heard.entry["replies"] = replies;
  heard.entry["restrictions"] = QJsonArray::fromStringList(m_rlv.objects().value(senderKey));
  return heard;
}

QVector<RlvRelay::Message> RlvRelay::releaseAll(QJsonObject& entry)
{
  QVector<Message> out;
  QJsonArray freed;
  const QStringList keys = m_controllers.keys();
  for (const QString& key : keys)
  {
    Message m;
    m.to = key;
    m.text = QStringLiteral("release,") + key + QStringLiteral(",!release,ok");
    out.push_back(m);
    freed.append(key);
    release(key);
  }
  entry = QJsonObject{{"type", "relay"}, {"action", "release"}, {"freed", freed},
                      {"note", "The avatar freed herself from the relay's menu: every controller was freed and told so."}};
  return out;
}

QVector<RlvRelay::Message> RlvRelay::ping(const QStringList& reachable, QJsonObject& entry)
{
  QVector<Message> out;
  QJsonArray pinged, freed;
  const QStringList keys = m_controllers.keys();
  for (const QString& key : keys)
  {
    if (m_rlv.objects().value(key).isEmpty()) continue;
    if (!reachable.contains(key))
    {
      freed.append(key);
      release(key);
      continue;
    }
    Message m;
    m.to = key;
    m.text = QStringLiteral("ping,") + key + QStringLiteral(",ping,ping");
    out.push_back(m);
    pinged.append(key);
  }
  entry = QJsonObject{{"type", "relay"}, {"action", "ping"}, {"pinged", pinged}, {"freed", freed},
                      {"note", "The relay's reachability check: a controller that is no longer in the world was freed."}};
  return out;
}

QJsonObject RlvRelay::toJson() const
{
  QJsonArray controllers;
  for (auto it = m_controllers.constBegin(); it != m_controllers.constEnd(); ++it)
  {
    const char* trust = it->trust == Controller::Trust::Accepted ? "accepted" : (it->trust == Controller::Trust::Declined ? "declined" : "not asked");
    controllers.append(QJsonObject{{"key", it.key()}, {"name", it->name}, {"trust", trust},
                                   {"restrictions", QJsonArray::fromStringList(m_rlv.objects().value(it.key()))}});
  }
  QJsonObject out{{"mode", modeName(m_settings.mode)}, {"key", QString::fromLatin1(KEY)}, {"controllers", controllers}};
  if (m_settings.mode == Mode::Ask) out["answers_left"] = qMax(0, int(m_settings.answers.size()) - m_nextAnswer);
  return out;
}

} // namespace lsl
