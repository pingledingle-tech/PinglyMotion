/*
 * PinglyMotion Script Lab - props as prims (lane 1, job E)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QMap>

#include "labtint.h"

namespace
{
  float clamp01(double v) { return float(qBound(0.0, v, 1.0)); }
}

bool LabTint::Tint::operator==(const Tint& other) const
{
  return prim == other.prim && link == other.link && hasColour == other.hasColour && hasAlpha == other.hasAlpha
         && colour[0] == other.colour[0] && colour[1] == other.colour[1] && colour[2] == other.colour[2] && alpha == other.alpha;
}

QList<LabTint::Tint> LabTint::fromFolder(const QJsonObject& folder)
{
  QList<Tint> out;
  if (!folder.value("ok").toBool()) return out;
  QMap<int, QString> names;
  for (const QJsonValue& p : folder.value("linkset").toObject().value("prims").toArray())
    names.insert(p.toObject().value("link").toInt(), p.toObject().value("name").toString());
  QMap<int, Tint> tints;
  for (const QJsonValue& v : folder.value("result").toObject().value("log").toArray())
  {
    const QJsonObject e = v.toObject();
    if (e.value("type").toString() != QLatin1String("prim")) continue;
    const QString op = e.value("op").toString();
    if (op != QLatin1String("color") && op != QLatin1String("alpha")) continue;
    const int face = e.value("face").toInt(-1);
    if (face != -1 && face != 0) continue;    // ALL_SIDES or face 0 only
    // the log writes link 0 for an object of one prim
    const int link = qMax(1, e.value("link").toInt());
    if (!names.contains(link)) continue;
    Tint& t = tints[link];
    t.link = link;
    t.prim = names.value(link);
    const QJsonArray colour = e.value("color").toArray();
    if (colour.size() == 3)
    {
      t.hasColour = true;
      for (int i = 0; i < 3; ++i) t.colour[i] = clamp01(colour.at(i).toDouble());
    }
    if (e.contains("alpha"))
    {
      t.hasAlpha = true;
      t.alpha = clamp01(e.value("alpha").toDouble());
    }
  }
  for (const Tint& t : tints) out << t;
  return out;
}
