/*
 * PinglyMotion Script Lab - chain and ribbon lines between two parts (job A4, part 2)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "labchains.h"

#include <QMap>
#include <QObject>

#include "rlvbehaviours.h"
#include "rlvstate.h"

namespace LabChains
{

QString Chain::word() const
{
  return locked ? QObject::tr("Locked") : QObject::tr("Free");
}

QVector<Chain> fromLog(const QJsonArray& log, const QList<QPair<int, QString>>& prims, bool locked)
{
  QMap<int, QString> names;
  for (const auto& p : prims) names.insert(p.first, p.second);
  QMap<int, Chain> byEmitter;   // in link order
  for (const QJsonValue& v : log)
  {
    const QJsonObject e = v.toObject();
    if (e.value("type").toString() != QLatin1String("recorded_only")) continue;
    const QString function = e.value("function").toString();
    if (function != QLatin1String("llParticleSystem") && function != QLatin1String("llLinkParticleSystem")) continue;
    const int emitter = e.value("emitter_link").toInt();
    if (emitter <= 0) continue;
    const int target = e.value("target_link").toInt();
    // a new particle system replaces the prim's old one: a chain to a prim, or none
    if (target <= 0 || target == emitter || !names.contains(target) || !names.contains(emitter))
    {
      byEmitter.remove(emitter);
      continue;
    }
    Chain c;
    c.fromLink = emitter;
    c.from = names.value(emitter);
    c.toLink = target;
    c.to = names.value(target);
    c.locked = locked;
    c.file = e.value("file").toString();
    byEmitter.insert(emitter, c);
  }
  return QVector<Chain>(byEmitter.begin(), byEmitter.end());
}

QVector<Chain> fromFolderResult(const QJsonObject& folderResult, bool locked)
{
  const QJsonObject run = folderResult.value("result").toObject();
  if (run.isEmpty()) return {};
  QList<QPair<int, QString>> prims;
  for (const QJsonValue& v : folderResult.value("linkset").toObject().value("prims").toArray())
    prims.append({v.toObject().value("link").toInt(), v.toObject().value("name").toString()});
  return fromLog(run.value("log").toArray(), prims, locked);
}

bool objectLocked(const QJsonObject& folderResult, const RlvState& rlv)
{
  const QJsonArray objects = folderResult.value("result").toObject().value("objects").toArray();
  if (objects.isEmpty()) return false;
  const QString key = objects.first().toObject().value("key").toString();
  for (const RlvState::Restriction& r : rlv.restrictions())
    if (r.object == key && Rlv::canonicalName(r.behaviour) == QLatin1String("detach") && r.option.isEmpty()) return true;
  return false;
}

} // namespace LabChains
