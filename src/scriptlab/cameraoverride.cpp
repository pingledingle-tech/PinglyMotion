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
  bool isFovMin(const QString& b) { return b == QLatin1String("setcam_fovmin"); }
  bool isFovMax(const QString& b) { return b == QLatin1String("setcam_fovmax"); }
  bool isZoomMax(const QString& b) { return b == QLatin1String("camzoommax"); }   // limits zooming in: a minimum field of view
  bool isZoomMin(const QString& b) { return b == QLatin1String("camzoommin"); }   // limits zooming out: a maximum field of view
  bool isAngle(const QString& b) { return isFovMin(b) || isFovMax(b); }
  bool isZoom(const QString& b) { return isZoomMax(b) || isZoomMin(b); }

  constexpr double Pi = 3.14159265358979323846;

  // a field of view in radians: more than 0, at most a half turn
  bool angle(const QString& option, double* degrees)
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(option.trimmed(), &ok);
    if (!ok || !std::isfinite(v) || v <= 0 || v > Pi) return false;
    *degrees = v * 180.0 / Pi;
    return true;
  }

  // a zoom multiplier of the default view: more than 0, at most 100; gives the field of view in degrees
  bool zoom(const QString& option, double* degrees)
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(option.trimmed(), &ok);
    if (!ok || !std::isfinite(v) || v <= 0 || v > 100) return false;
    *degrees = double(Camera::DefaultFieldOfView) / v;
    return true;
  }

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
    if (isAngle(r.behaviour) || isZoom(r.behaviour))
    {
      double deg = 0;
      if (!(isAngle(r.behaviour) ? angle(r.option, &deg) : zoom(r.option, &deg)))
      {
        const QString text = r.behaviour + QLatin1Char(':') + r.option;
        if (!limits.ignored.contains(text)) limits.ignored << text;
        continue;
      }
      objects.insert(r.object);
      if (isFovMin(r.behaviour) || isZoomMax(r.behaviour)) limits.minView = qMax(limits.minView, deg);
      else limits.maxView = limits.maxView > 0 ? qMin(limits.maxView, deg) : deg;
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

double CameraOverride::Limits::fieldOfView() const
{
  double view = Camera::DefaultFieldOfView;
  if (minView > 0) view = qMax(view, minView);
  // a maximum below the minimum wins over it
  if (maxView > 0) view = qMin(view, maxView);
  return qBound(NarrowestView, view, WidestView);
}

CameraConstraint CameraOverride::constraintFor(const Limits& limits)
{
  CameraConstraint c;
  if (!limits.any()) return c;
  if (limits.hasViewLimit()) c.fieldOfView = float(limits.fieldOfView());
  if (!limits.aroundAvatar()) return c;
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

QString CameraOverride::degrees(double value)
{
  return QLocale::c().toString(std::round(value * 10.0) / 10.0, 'g', 6);
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
  if (m_limits.minView > 0) parts << tr("min field of view %1 degrees").arg(degrees(m_limits.minView));
  if (m_limits.maxView > 0) parts << tr("max field of view %1 degrees").arg(degrees(m_limits.maxView));
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
    if (m_limits.minView > 0 && m_limits.maxView > 0 && m_limits.minView > m_limits.maxView)
      lines << tr("The max field of view is below the min field of view, so the max wins.");
    if (m_limits.hasViewLimit())
    {
      const double view = m_limits.fieldOfView();
      lines << (view == double(Camera::DefaultFieldOfView)
                  ? tr("The 3D view keeps its field of view of %1 degrees.").arg(degrees(view))
                  : tr("The 3D view shows a field of view of %1 degrees instead of %2.").arg(degrees(view), degrees(Camera::DefaultFieldOfView)));
    }
    const QString holders = m_limits.objects.join(QStringLiteral(", "));
    lines << (m_limits.aroundAvatar() ? tr("The camera turns around the avatar. Held by: %1.").arg(holders)
                                      : tr("Held by: %1.").arg(holders));
    lines << tr("Untick the box to get your own camera back.");
  }
  else
  {
    lines << tr("The script limits the camera: %1. Tick the box to see it in the 3D view.").arg(parts.join(QStringLiteral(", ")));
  }
  for (const QString& text : m_limits.ignored)
  {
    const QString behaviour = text.section(QLatin1Char(':'), 0, 0);
    if (isAngle(behaviour)) lines << tr("Not used: %1 is not an angle in radians.").arg(text);
    else if (isZoom(behaviour)) lines << tr("Not used: %1 is not a zoom multiplier.").arg(text);
    else lines << tr("Not used: %1 is not a distance in metres.").arg(text);
  }
  return lines;
}
