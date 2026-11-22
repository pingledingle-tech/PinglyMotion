/*
 * PinglyMotion Script Lab - RLV camera limits shown in the 3D view (job 7b)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QLocale>
#include <QSet>

#include "rlvstate.h"

#include "cameraoverride.h"

namespace
{
  bool isMin(const QString& b) { return b == QLatin1String("setcam_avdistmin") || b == QLatin1String("camdistmin"); }
  bool isMax(const QString& b) { return b == QLatin1String("setcam_avdistmax") || b == QLatin1String("camdistmax"); }
  bool isUnlock(const QString& b) { return b == QLatin1String("setcam_unlock") || b == QLatin1String("camunlock"); }

  // a distance in metres: a plain number, 0 or more, at most 10 km
  bool distance(const QString& option, double* value)
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(option.trimmed(), &ok);
    if (!ok || !std::isfinite(v) || v < 0 || v > 10000) return false;
    *value = v;
    return true;
  }
}

CameraOverride::Limits CameraOverride::limitsFrom(const RlvState& state)
{
  Limits limits;
  QSet<QString> objects;
  for (const RlvState::Restriction& r : state.restrictions())
  {
    if (isUnlock(r.behaviour))
    {
      limits.focusOnAvatar = true;
      objects.insert(r.object);
      continue;
    }
    if (!isMin(r.behaviour) && !isMax(r.behaviour)) continue;
    double v = 0;
    if (!distance(r.option, &v))
    {
      const QString text = r.behaviour + QLatin1Char(':') + r.option;
      if (!limits.ignored.contains(text)) limits.ignored << text;
      continue;
    }
    objects.insert(r.object);
    // several objects: the strictest limit wins (largest minimum, smallest maximum)
    if (isMin(r.behaviour)) limits.minDistance = qMax(limits.minDistance, v);
    else limits.maxDistance = limits.hasMax() ? qMin(limits.maxDistance, v) : v;
  }
  limits.objects = QStringList(objects.begin(), objects.end());
  limits.objects.sort();
  return limits;
}

CameraConstraint CameraOverride::constraintFor(const Limits& limits)
{
  CameraConstraint c;
  if (!limits.any()) return c;
  if (limits.hasMax()) c.maxDistance = qMax(ClosestDistance, float(limits.maxDistance * InchesPerMetre));
  if (limits.hasMin())
  {
    c.minDistance = float(limits.minDistance * InchesPerMetre);
    // a maximum below the minimum wins over it
    if (limits.hasMax() && c.minDistance > c.maxDistance) c.minDistance = c.maxDistance;
  }
  c.turnAroundPoint = true;
  c.point = avatarPoint();
  return c;
}

QString CameraOverride::metres(double value)
{
  return QLocale::c().toString(value, 'g', 6);
}

CameraOverride::CameraOverride(QObject* parent) :
  QObject(parent)
{
}

void CameraOverride::setState(RlvState* state)
{
  for (const QMetaObject::Connection& c : m_connections) disconnect(c);
  m_connections.clear();
  m_state = state;
  if (state)
  {
    m_connections << connect(state, &RlvState::changed, this, &CameraOverride::update);
    m_connections << connect(state, &RlvState::modeChanged, this, &CameraOverride::update);
  }
  update();
}

void CameraOverride::setEnabled(bool on)
{
  if (on == m_enabled) return;
  m_enabled = on;
  emit changed();
}

void CameraOverride::update()
{
  const Limits limits = m_state ? limitsFrom(*m_state) : Limits();
  if (limits == m_limits) return;
  m_limits = limits;
  emit changed();
}

QStringList CameraOverride::lines() const
{
  QStringList lines;
  QStringList parts;
  if (m_limits.hasMin()) parts << tr("min distance %1 m").arg(metres(m_limits.minDistance));
  if (m_limits.hasMax()) parts << tr("max distance %1 m").arg(metres(m_limits.maxDistance));
  if (m_limits.focusOnAvatar) parts << tr("focus kept on the avatar");

  if (!m_limits.any())
  {
    if (m_enabled) lines << tr("The script does not limit the camera now.");
  }
  else if (m_enabled)
  {
    lines << tr("Camera limited by the script: %1.").arg(parts.join(QStringLiteral(", ")));
    if (m_limits.hasMin() && m_limits.hasMax() && m_limits.minDistance > m_limits.maxDistance)
      lines << tr("The max distance is below the min distance, so the max distance wins.");
    if (m_limits.hasMax() && m_limits.maxDistance * InchesPerMetre < ClosestDistance)
      lines << tr("The 3D view goes as close as it can.");
    lines << tr("The camera turns around the avatar. Held by: %1.").arg(m_limits.objects.join(QStringLiteral(", ")));
    lines << tr("Untick the box to get your own camera back.");
  }
  else
  {
    lines << tr("The script limits the camera: %1. Tick the box to see it in the 3D view.").arg(parts.join(QStringLiteral(", ")));
  }
  for (const QString& text : m_limits.ignored)
    lines << tr("Not used: %1 is not a distance in metres.").arg(text);
  return lines;
}
