/*
 * PinglyMotion Script Lab - props made from the prims, and props that move (job A1)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QJsonArray>
#include <QMap>
#include <QMatrix3x3>
#include <QtMath>

#include "labprimprops.h"

namespace
{
  // three decimals, and never -0, so the values compare exactly in tests and in the Props panel
  float tidy(double v)
  {
    const double r = std::round(v * 1000.0) / 1000.0;
    return r == 0.0 ? 0.0f : float(r);
  }
  QVector3D tidy(const QVector3D& v) { return QVector3D(tidy(v.x()), tidy(v.y()), tidy(v.z())); }

  bool readVector(const QJsonValue& value, QVector3D& out)
  {
    const QJsonArray a = value.toArray();
    if (a.size() != 3) return false;
    for (const QJsonValue& n : a)
      if (!n.isDouble() || !std::isfinite(n.toDouble())) return false;
    out = QVector3D(float(a[0].toDouble()), float(a[1].toDouble()), float(a[2].toDouble()));
    return true;
  }

  bool readRotation(const QJsonValue& value, QQuaternion& out)
  {
    const QJsonArray a = value.toArray();
    if (a.size() != 4) return false;
    for (const QJsonValue& n : a)
      if (!n.isDouble() || !std::isfinite(n.toDouble())) return false;
    // the log writes x, y, z, s, as LSL does
    out = QQuaternion(float(a[3].toDouble()), float(a[0].toDouble()), float(a[1].toDouble()), float(a[2].toDouble())).normalized();
    return !out.isNull();
  }

  // the right side: "Wrist_right", "cuff R", "r_ankle"; everything else is the left side
  bool rightSide(const QString& lower)
  {
    if (lower.contains(QLatin1String("right"))) return true;
    for (const char* end : {"_r", "-r", " r", ".r"})
      if (lower.endsWith(QLatin1String(end))) return true;
    for (const char* start : {"r_", "r-", "r "})
      if (lower.startsWith(QLatin1String(start))) return true;
    return false;
  }

  LabExample::Prop make(const QString& name, const char* shape, const QString& joint,
                        const QVector3D& position, const QVector3D& scale, const QVector3D& rotation)
  {
    LabExample::Prop p;
    p.name = name;
    p.shape = QString::fromLatin1(shape);
    p.joint = joint;
    p.position = position;
    p.scale = scale;
    p.rotation = rotation;
    return p;
  }

  // the table in docs/SCRIPT_LAB.md, "Props from prims"; false when no word matches
  bool fromName(const LabPrimProps::Prim& prim, LabExample::Prop& out)
  {
    const QString lower = prim.name.toLower();
    const bool right = rightSide(lower);
    const auto has = [&lower](std::initializer_list<const char*> words)
    {
      for (const char* w : words)
        if (lower.contains(QLatin1String(w))) return true;
      return false;
    };
    if (has({"ankle"}))
      out = make(prim.name, "torus", right ? "rFoot" : "lFoot", {0, 2, 0}, {3.4f, 3.4f, 1.2f}, {90, 0, 0});
    else if (has({"wrist", "cuff"}))
      out = make(prim.name, "torus", right ? "rHand" : "lHand", {right ? 1.0f : -1.0f, 0, 0}, {2.8f, 2.8f, 1.2f}, {0, 90, 0});
    else if (has({"collar", "neck"}))
      out = make(prim.name, "torus", "neck", {0, 1.5f, 0}, {5, 5, 1.6f}, {90, 0, 0});
    else if (has({"blindfold", "eye"}))
      out = make(prim.name, "torus", "head", {0, 3.1f, 0.6f}, {8.2f, 8.2f, 1.4f}, {90, 0, 0});
    else if (has({"gag", "ball"}))
      out = make(prim.name, "sphere", "head", {0, 0.9f, 3.6f}, {1.6f, 1.6f, 1.6f}, {0, 0, 0});
    else if (has({"hood", "hat", "head"}))
      out = make(prim.name, "cone", "head", {0, 4, 0}, {9, 9, 10}, {-90, 0, 0});
    else if (has({"hud"}) || lower == QLatin1String("root"))
      out = make(prim.name, "box", QString(), {-20, 45, 0}, {8, 8, 1}, {0, 0, 0});
    else
      return false;
    return true;
  }
}

QVector3D LabPrimProps::startRootPosition()
{
  return QVector3D(128.0f, 128.0f, 20.0f);
}

QList<LabPrimProps::Prim> LabPrimProps::primsOf(const QJsonObject& folder)
{
  QList<Prim> out;
  for (const QJsonValue& v : folder.value("linkset").toObject().value("prims").toArray())
  {
    const QJsonObject p = v.toObject();
    const QString name = p.value("name").toString();
    if (name.isEmpty()) continue;
    // a root prim that is only the top folder, with nothing in it, is not shown (First colour's root)
    if (p.value("folder").toString().isEmpty() && p.value("scripts").toArray().isEmpty() && p.value("notecards").toArray().isEmpty())
      continue;
    out << Prim{name, qMax(1, p.value("link").toInt(1))};
  }
  return out;
}

bool LabPrimProps::nameMatches(const QString& primName)
{
  LabExample::Prop unused;
  return fromName(Prim{primName, 2}, unused);
}

LabExample::Prop LabPrimProps::propFor(const Prim& prim, const QList<LabExample::Prop>& examples, int index)
{
  for (const LabExample::Prop& p : examples)
    if (p.name == prim.name) return p;
  LabExample::Prop out;
  if (fromName(prim, out)) return out;
  // a small box beside the figure, on the other side from a HUD; the next one 6 units further out
  return make(prim.name, "box", QString(), {20.0f + 6.0f * float(qMax(0, index)), 45, 0}, {4, 4, 4}, {0, 0, 0});
}

QList<LabExample::Prop> LabPrimProps::propsFor(const QJsonObject& folder, const QList<LabExample::Prop>& examples)
{
  QList<LabExample::Prop> out;
  int index = 0;
  for (const Prim& prim : primsOf(folder))
  {
    bool example = false;
    for (const LabExample::Prop& p : examples) example = example || p.name == prim.name;
    const bool plain = !example && !nameMatches(prim.name);
    out << propFor(prim, examples, plain ? index : 0);
    if (plain) ++index;
  }
  return out;
}

bool LabPrimProps::Move::operator==(const Move& other) const
{
  return prim == other.prim && link == other.link && hasPosition == other.hasPosition && hasSize == other.hasSize
         && hasRotation == other.hasRotation && (!hasPosition || position == other.position)
         && (!hasSize || size == other.size) && (!hasRotation || rotation == other.rotation);
}

QList<LabPrimProps::Move> LabPrimProps::movesFromFolder(const QJsonObject& folder)
{
  QList<Move> out;
  if (!folder.value("ok").toBool()) return out;
  QMap<int, QString> names;
  for (const Prim& p : primsOf(folder)) names.insert(p.link, p.name);
  QMap<int, Move> moves;
  for (const QJsonValue& v : folder.value("result").toObject().value("log").toArray())
  {
    const QJsonObject e = v.toObject();
    if (e.value("type").toString() != QLatin1String("prim")) continue;
    const QString op = e.value("op").toString();
    if (op != QLatin1String("position") && op != QLatin1String("size") && op != QLatin1String("rotation")) continue;
    // the log writes link 0 for an object of one prim
    const int link = qMax(1, e.value("link").toInt());
    if (!names.contains(link)) continue;
    if (op == QLatin1String("position"))
    {
      // the sandbox refused the move (outside the region, worn, physical): the prim stayed
      const QString note = e.value("note").toString();
      if (note.contains(QLatin1String("did not move")) || note.contains(QLatin1String("cannot be moved"))
          || note.contains(QLatin1String("does nothing")))
        continue;
    }
    Move& m = moves[link];
    m.link = link;
    m.prim = names.value(link);
    QVector3D vector;
    QQuaternion turn;
    if (op == QLatin1String("position") && readVector(e.value("position"), vector)) { m.hasPosition = true; m.position = vector; }
    if (op == QLatin1String("size"))
    {
      if (readVector(e.value("size"), vector)) { m.hasSize = true; m.size = vector; }
      // llScaleByFactor moves a child prim along with its size
      if (e.contains("position") && readVector(e.value("position"), vector)) { m.hasPosition = true; m.position = vector; }
    }
    if (op == QLatin1String("rotation") && readRotation(e.value("rotation"), turn)) { m.hasRotation = true; m.rotation = turn; }
  }
  for (const Move& m : moves)
    if (m.hasPosition || m.hasSize || m.hasRotation) out << m;
  return out;
}

QVector3D LabPrimProps::toScene(const QVector3D& v)
{
  return QVector3D(v.y(), v.z(), v.x());
}

QQuaternion LabPrimProps::toScene(const QQuaternion& q)
{
  // the axes are only renamed (a turn of the axes, not a mirror), so the vector part is renamed the same way
  return QQuaternion(q.scalar(), q.y(), q.z(), q.x());
}

QQuaternion LabPrimProps::fromPropAngles(const QVector3D& degrees)
{
  return QQuaternion::fromAxisAndAngle(1, 0, 0, degrees.x()) * QQuaternion::fromAxisAndAngle(0, 1, 0, degrees.y())
         * QQuaternion::fromAxisAndAngle(0, 0, 1, degrees.z());
}

QVector3D LabPrimProps::toPropAngles(const QQuaternion& rotation)
{
  // R = Rx(a) Ry(b) Rz(c): R02 = sin b, R12 = -sin a cos b, R22 = cos a cos b, R01 = -cos b sin c, R00 = cos b cos c
  const QMatrix3x3 r = rotation.normalized().toRotationMatrix();
  const double r02 = qBound(-1.0, double(r(0, 2)), 1.0);
  double a, b, c;
  b = std::asin(r02);
  if (std::fabs(r02) > 0.999999)
  {
    c = 0.0;
    a = std::atan2(double(r(2, 1)), double(r(1, 1)));
  }
  else
  {
    a = std::atan2(-double(r(1, 2)), double(r(2, 2)));
    c = std::atan2(-double(r(0, 1)), double(r(0, 0)));
  }
  // to a hundredth of a degree: the log keeps a rotation to five decimals, which is about that close
  const auto hundredth = [](double radians)
  {
    const double r = std::round(qRadiansToDegrees(radians) * 100.0) / 100.0;
    return r == 0.0 ? 0.0f : float(r);
  };
  return QVector3D(hundredth(a), hundredth(b), hundredth(c));
}

LabExample::Prop LabPrimProps::moved(const LabExample::Prop& base, const Move& move)
{
  LabExample::Prop out = base;
  if (move.hasPosition)
  {
    const QVector3D start = move.link <= 1 ? startRootPosition() : QVector3D();
    out.position = tidy(base.position + toScene(move.position - start) * UnitsPerMetre);
  }
  if (move.hasSize)
  {
    // each side of the prop grows as the prim's own side did (local axes: x to x, y to y, z to z)
    out.scale = tidy(QVector3D(base.scale.x() * move.size.x() / StartSize, base.scale.y() * move.size.y() / StartSize,
                               base.scale.z() * move.size.z() / StartSize));
  }
  if (move.hasRotation) out.rotation = toPropAngles(toScene(move.rotation) * fromPropAngles(base.rotation));
  return out;
}
