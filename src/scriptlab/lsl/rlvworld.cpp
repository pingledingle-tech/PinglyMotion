/*
 * PinglyMotion Script Lab - LSL interpreter: the world with RLV (step 4)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <vector>

#include <QJsonArray>

#include "interp.h"
#include "rlvworld.h"
#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"

namespace lsl
{

RlvWorld::RlvWorld(RlvState& rlv) : m_rlv(rlv) {}

void RlvWorld::ownerSay(double time, const QString& text)
{
  if (!text.startsWith('@'))
  {
    World::ownerSay(time, text);
    return;
  }

  const QString line = clip(text);
  if (m_lines >= MAX_RLV_LINES)
  {
    if (m_refused++ == 0)
      log(time, QJsonObject{{"type", "rlv_command"}, {"object", objectKey()}, {"text", line},
                            {"error", QString("Too many RLV lines in one run (at most %1); this and later ones were not run.").arg(MAX_RLV_LINES)}});
    return;
  }
  ++m_lines;

  const RlvState::ApplyResult result = m_rlv.apply(objectKey(), line);
  QJsonObject entry{{"type", "rlv_command"}, {"object", objectKey()}, {"text", line}};
  if (!result.ok())
  {
    entry["error"] = result.error;
    log(time, entry);
    return;
  }

  QJsonArray results;
  for (const RlvState::Outcome& outcome : result.outcomes)
  {
    QJsonObject r{{"command", outcome.command.text}, {"behaviour", outcome.command.behaviour}};
    if (!outcome.command.option.isEmpty()) r["option"] = outcome.command.option;
    r["param"] = outcome.command.param;
    r["type"] = Rlv::paramTypeName(outcome.command.type);
    r["result"] = Rlv::resultName(outcome.result);
    if (!outcome.reason.isEmpty()) r["reason"] = outcome.reason;
    results << r;
  }
  // Anafin HUD test, gap 6: each reply comes as the replies option says (default: at once); a lost or late reply says so
  QJsonArray replies;
  std::vector<AnswerPlan> plans;
  for (const RlvState::Reply& reply : result.replies)
  {
    const AnswerPlan plan = nextAnswer(AnswerKind::Rlv);
    QJsonObject r{{"channel", reply.channel}, {"text", reply.text}};
    if (plan.result == "none") r["lost"] = true;
    else if (plan.delay > 0.0) r["delay"] = plan.delay;
    replies << r;
    plans.push_back(plan);
  }
  entry["results"] = results;
  entry["replies"] = replies;
  entry["restrictions"] = int(m_rlv.restrictions().size());
  log(time, entry);

  // the viewer answers as the owner; the script hears it as a listen event
  if (m_interpreter)
    for (int i = 0; i < int(result.replies.size()); ++i)
    {
      const RlvState::Reply& reply = result.replies[i];
      const AnswerPlan& plan = plans[size_t(i)];
      if (plan.result == "none") continue;
      if (plan.delay > 0.0) m_interpreter->hearLater(time + plan.delay, reply.channel, reply.text);
      else m_interpreter->hear(reply.channel, QString(), QString(), reply.text);
    }
}

QJsonObject RlvWorld::stateJson(const RlvState& rlv)
{
  QJsonObject answer;
  answer["mode"] = Rlv::modeName(rlv.mode());
  answer["rlv_version"] = QString("%1.%2.%3").arg(Rlv::VERSION_MAJOR).arg(Rlv::VERSION_MINOR).arg(Rlv::VERSION_PATCH);

  QJsonObject objects;
  const QMap<QString, QStringList> map = rlv.objects();
  for (auto it = map.constBegin(); it != map.constEnd(); ++it) objects[it.key()] = QJsonArray::fromStringList(it.value());
  answer["objects"] = objects;

  QJsonArray restrictions;
  QJsonObject counts;
  for (const RlvState::Restriction& r : rlv.restrictions())
  {
    QJsonObject entry{{"object", r.object}, {"behaviour", r.behaviour}};
    if (!r.option.isEmpty()) entry["option"] = r.option;
    restrictions << entry;
    const QString key = r.option.isEmpty() ? r.behaviour : r.behaviour + ":" + r.option;
    counts[key] = rlv.count(r.behaviour, r.option);
  }
  answer["restrictions"] = restrictions;
  answer["counts"] = counts;

  QJsonArray actions;
  QJsonArray blocked;
  const QMap<QString, bool> allowed = rlv.actions();
  for (const Rlv::Action& action : Rlv::actions())
  {
    const bool yes = allowed.value(action.id, true);
    actions << QJsonObject{{"id", action.id}, {"label", action.label}, {"allowed", yes}};
    if (!yes) blocked << action.id;
  }
  answer["actions"] = actions;
  answer["blocked"] = blocked;

  answer["behaviours"] = QJsonArray::fromStringList(Rlv::behaviourNames(rlv.mode()));
  QJsonArray rlvaOnly;
  for (const Rlv::Behaviour& behaviour : Rlv::behaviours())
    if (behaviour.dialect == Rlv::Dialect::RlvaOnly) rlvaOnly << behaviour.name;
  answer["rlva_only"] = rlvaOnly;
  QJsonObject recorded;
  const QMap<QString, QString> effects = rlv.recorded();
  for (auto it = effects.constBegin(); it != effects.constEnd(); ++it) recorded[it.key()] = it.value();
  answer["recorded"] = recorded;
  return answer;
}

} // namespace lsl
