/*
 * PinglyMotion Script Lab - RLV sky and draw distance shown in the 3D view (job 7c)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QLocale>
#include <QSet>

#include "rlvstate.h"

#include "environmentoverride.h"

namespace
{
  // a number from 0 to 1
  bool unit(const QString& option, double* value)
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(option.trimmed(), &ok);
    if (!ok || !std::isfinite(v) || v < 0 || v > 1) return false;
    *value = v;
    return true;
  }

  // a distance in metres: more than 0, at most 10 km
  bool distance(const QString& option, double* value)
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(option.trimmed(), &ok);
    if (!ok || !std::isfinite(v) || v <= 0 || v > 10000) return false;
    *value = v;
    return true;
  }

  // "r/g/b", each 0 to 1
  bool colour(const QString& option, QVector3D* value)
  {
    const QStringList parts = option.split(QLatin1Char('/'));
    if (parts.size() != 3) return false;
    double v[3];
    for (int i = 0; i < 3; ++i)
      if (!unit(parts.at(i), &v[i])) return false;
    *value = QVector3D(float(v[0]), float(v[1]), float(v[2]));
    return true;
  }

  QString number(double value)
  {
    return QLocale::c().toString(value, 'g', 6);
  }

  // the first held option of a behaviour (in the restriction list's order), and its object
  bool held(const QVector<RlvState::Restriction>& list, const QString& behaviour, QString* option, QString* object)
  {
    for (const RlvState::Restriction& r : list)
      if (r.behaviour == behaviour)
      {
        *option = r.option;
        *object = r.object;
        return true;
      }
    return false;
  }

  // the colours of the time of day, from the RLV API's list: midnight 0, sunrise 0.25, midday 0.567, sunset 0.75
  struct DayKey
  {
    double at;
    QVector3D colour;
  };
  const DayKey DayKeys[] = {
    {0.0, QVector3D(0.02f, 0.03f, 0.08f)},
    {0.25, QVector3D(0.80f, 0.55f, 0.45f)},
    {0.567, QVector3D(0.55f, 0.72f, 0.92f)},
    {0.75, QVector3D(0.85f, 0.45f, 0.30f)},
    {1.0, QVector3D(0.02f, 0.03f, 0.08f)},
  };
}

QVector3D EnvironmentOverride::dayColour(double daytime)
{
  daytime = qBound(0.0, daytime, 1.0);
  for (size_t i = 1; i < sizeof(DayKeys) / sizeof(DayKeys[0]); ++i)
  {
    if (daytime > DayKeys[i].at) continue;
    const double t = (daytime - DayKeys[i - 1].at) / (DayKeys[i].at - DayKeys[i - 1].at);
    return DayKeys[i - 1].colour + (DayKeys[i].colour - DayKeys[i - 1].colour) * float(t);
  }
  return DayKeys[0].colour;
}

QString EnvironmentOverride::colourText(const QVector3D& c)
{
  return number(std::round(c.x() * 1000.0) / 1000.0) + QLatin1Char('/') + number(std::round(c.y() * 1000.0) / 1000.0) +
         QLatin1Char('/') + number(std::round(c.z() * 1000.0) / 1000.0);
}

EnvironmentOverride::Look EnvironmentOverride::lookFrom(const RlvState& state)
{
  Look look;
  QSet<QString> objects;
  const QMap<QString, QString> recorded = state.recorded();
  const QMap<QString, QString> by = state.recordedBy();
  const QVector<RlvState::Restriction> list = state.restrictions();
  const auto ignore = [&look](const QString& text) { if (!look.ignored.contains(text)) look.ignored << text; };

  // --- the sky: forced setenv values
  const QString horizon = QStringLiteral("setenv_bluehorizon");
  bool parts = false;
  QVector3D legacy;
  QStringList legacyObjects;
  const char axes[] = {'r', 'g', 'b'};
  for (int i = 0; i < 3; ++i)
  {
    const QString name = horizon + QLatin1Char(axes[i]);
    if (!recorded.contains(name)) continue;
    double v = 0;
    if (!unit(recorded.value(name), &v)) { ignore(name + QLatin1Char(':') + recorded.value(name)); continue; }
    legacy[i] = float(v);
    parts = true;
    legacyObjects << by.value(name);
  }
  if (recorded.contains(horizon))
  {
    QVector3D c;
    if (colour(recorded.value(horizon), &c))
    {
      look.hasSky = true;
      look.sky = c;
      look.skyFrom = QStringLiteral("blue horizon ") + colourText(c);
      objects.insert(by.value(horizon));
    }
    else ignore(horizon + QLatin1Char(':') + recorded.value(horizon));
  }
  if (!look.hasSky && parts)
  {
    // the old names give one part each; a part not given is 0
    look.hasSky = true;
    look.sky = legacy;
    look.skyFrom = QStringLiteral("blue horizon ") + colourText(legacy);
    for (const QString& o : legacyObjects) objects.insert(o);
  }
  const QString daytime = QStringLiteral("setenv_daytime");
  if (recorded.contains(daytime))
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(recorded.value(daytime).trimmed(), &ok);
    if (ok && v == -1)
    {
      // the region's own sky: nothing to show
    }
    else if (ok && std::isfinite(v) && v >= 0 && v <= 1)
    {
      if (!look.hasSky)
      {
        look.hasSky = true;
        look.sky = dayColour(v);
        look.skyFrom = QStringLiteral("daytime ") + number(v);
        objects.insert(by.value(daytime));
      }
    }
    else ignore(daytime + QLatin1Char(':') + recorded.value(daytime));
  }
  for (auto it = recorded.constBegin(); it != recorded.constEnd(); ++it)
  {
    if (!it.key().startsWith(QLatin1String("setenv_")) || it.key() == daytime || it.key() == horizon) continue;
    if (it.key().size() == horizon.size() + 1 && it.key().startsWith(horizon)) continue;
    look.notShown << tr("Not shown: %1 (the 3D view has no sun, moon, clouds or water).").arg(it.key() + QLatin1Char(':') + it.value());
  }

  // --- the draw distance: camdraw* held restrictions (smallest distance, largest alpha, first colour)
  bool camMax = false;
  double camDistance = 0, camAlpha = 1, camMinDistance = 0, camMinAlpha = 0;
  bool camMin = false, camMinAlphaSet = false, camAlphaSet = false, camColourSet = false;
  QVector3D camColour(0, 0, 0);
  QSet<QString> camObjects;
  for (const RlvState::Restriction& r : list)
  {
    const QString text = r.behaviour + QLatin1Char(':') + r.option;
    double v = 0;
    QVector3D c;
    if (r.behaviour == QLatin1String("camdrawmax"))
    {
      if (!distance(r.option, &v)) { ignore(text); continue; }
      camDistance = camMax ? qMin(camDistance, v) : v;
      camMax = true;
    }
    else if (r.behaviour == QLatin1String("camdrawmin"))
    {
      if (!distance(r.option, &v)) { ignore(text); continue; }
      camMinDistance = camMin ? qMin(camMinDistance, v) : v;
      camMin = true;
    }
    else if (r.behaviour == QLatin1String("camdrawalphamax"))
    {
      if (!unit(r.option, &v)) { ignore(text); continue; }
      camAlpha = camAlphaSet ? qMax(camAlpha, v) : v;
      camAlphaSet = true;
    }
    else if (r.behaviour == QLatin1String("camdrawalphamin"))
    {
      if (!unit(r.option, &v)) { ignore(text); continue; }
      camMinAlpha = camMinAlphaSet ? qMax(camMinAlpha, v) : v;
      camMinAlphaSet = true;
    }
    else if (r.behaviour == QLatin1String("camdrawcolor"))
    {
      if (!colour(r.option, &c)) { ignore(text); continue; }
      if (!camColourSet) camColour = c;
      camColourSet = true;
    }
    else continue;
    camObjects.insert(r.object);
  }
  if (camMax)
  {
    look.hasFog = true;
    look.fogFrom = QStringLiteral("camdraw");
    look.fogDistance = camDistance;
    look.fogAlpha = camAlpha;
    look.fogColour = camColour;
    for (const QString& o : camObjects) objects.insert(o);
    if (camMin) look.notShown << tr("Not shown: camdrawmin:%1 (the fog starts at the camera).").arg(number(camMinDistance));
    if (camMinAlphaSet) look.notShown << tr("Not shown: camdrawalphamin:%1 (the fog starts at the camera).").arg(number(camMinAlpha));
  }
  else if (!camObjects.isEmpty())
  {
    // values without camdrawmax: nothing to fade to
    look.notShown << tr("Not shown: the camdraw values, because camdrawmax is not set (nothing to fade to).");
  }

  // --- the vision sphere: setsphere held, its values held or forced (held first)
  QString option, object;
  if (held(list, QStringLiteral("setsphere"), &option, &object))
  {
    QSet<QString> sphereObjects{object};
    const auto value = [&](const QString& name, QString* out) -> bool
    {
      QString o;
      if (held(list, name, out, &o)) { sphereObjects.insert(o); return true; }
      if (recorded.contains(name)) { *out = recorded.value(name); sphereObjects.insert(by.value(name)); return true; }
      return false;
    };
    QString text;
    bool usable = true;
    if (value(QStringLiteral("setsphere_mode"), &text) && text.trimmed() != QLatin1String("0"))
    {
      look.notShown << tr("Not shown: setsphere_mode:%1 (only mode 0, the colour fade, is shown).").arg(text);
      usable = false;
    }
    double sphereDistance = 0, sphereAlpha = 1;
    QVector3D sphereColour(0, 0, 0);
    if (usable && !value(QStringLiteral("setsphere_distmax"), &text))
    {
      look.notShown << tr("Not shown: setsphere, because setsphere_distmax is not set.");
      usable = false;
    }
    else if (usable && !distance(text, &sphereDistance))
    {
      ignore(QStringLiteral("setsphere_distmax:") + text);
      usable = false;
    }
    if (usable && value(QStringLiteral("setsphere_valuemax"), &text) && !unit(text, &sphereAlpha))
    {
      ignore(QStringLiteral("setsphere_valuemax:") + text);
      usable = false;
    }
    if (usable && value(QStringLiteral("setsphere_color"), &text) && !colour(text, &sphereColour))
    {
      ignore(QStringLiteral("setsphere_color:") + text);
      usable = false;
    }
    if (usable)
    {
      if (look.hasFog) look.notShown << tr("Not shown: setsphere (the camdraw fog is shown; one fog at a time).");
      else
      {
        look.hasFog = true;
        look.fogFrom = QStringLiteral("setsphere");
        look.fogDistance = sphereDistance;
        look.fogAlpha = sphereAlpha;
        look.fogColour = sphereColour;
        for (const QString& o : sphereObjects) objects.insert(o);
      }
    }
  }

  objects.remove(QString());
  look.objects = QStringList(objects.begin(), objects.end());
  look.objects.sort();
  return look;
}

EnvironmentOverride::Scene EnvironmentOverride::sceneFor(const Look& look, const Scene& own)
{
  Scene scene = own;
  if (look.hasSky)
  {
    scene.background = look.sky;
    scene.fogColour = look.sky;   // the fog follows the background, as in the view's own look
  }
  if (look.hasFog)
  {
    const double alpha = qMin(look.fogAlpha, MostAlpha);
    scene.fogOn = alpha > 0;
    scene.fogColour = look.fogColour;
    // exponential fog: 1 - exp(-density * depth) reaches the alpha at the distance
    scene.fogDensity = alpha > 0 ? float(-std::log(1.0 - alpha) / (look.fogDistance * InchesPerMetre)) : 0.0f;
    // beyond the distance the colour hides the background as much as the alpha says
    scene.background = scene.background + (look.fogColour - scene.background) * float(look.fogAlpha);
  }
  return scene;
}

EnvironmentOverride::EnvironmentOverride(QObject* parent) :
  QObject(parent)
{
}

void EnvironmentOverride::setState(RlvState* state)
{
  for (const QMetaObject::Connection& c : m_connections) disconnect(c);
  m_connections.clear();
  m_state = state;
  if (state)
  {
    m_connections << connect(state, &RlvState::changed, this, &EnvironmentOverride::update);
    m_connections << connect(state, &RlvState::modeChanged, this, &EnvironmentOverride::update);
  }
  update();
}

void EnvironmentOverride::setEnabled(bool on)
{
  if (on == m_enabled) return;
  m_enabled = on;
  emit changed();
}

void EnvironmentOverride::update()
{
  const Look look = m_state ? lookFrom(*m_state) : Look();
  if (look == m_look) return;
  m_look = look;
  emit changed();
}

QStringList EnvironmentOverride::lines() const
{
  QStringList lines;
  QStringList parts;
  if (m_look.hasSky) parts << tr("sky colour %1 from %2").arg(colourText(m_look.sky), m_look.skyFrom);
  if (m_look.hasFog)
    parts << tr("%1 fog of colour %2 reaching %3 at %4 m")
               .arg(m_look.fogFrom, colourText(m_look.fogColour), number(m_look.fogAlpha), number(m_look.fogDistance));

  if (!m_look.any())
  {
    if (m_enabled && (!m_look.notShown.isEmpty() || !m_look.ignored.isEmpty()))
      lines << tr("The script asks nothing of the sky that the 3D view can show.");
  }
  else if (m_enabled)
  {
    lines << tr("Environment set by the script: %1.").arg(parts.join(QStringLiteral(", ")));
    lines << tr("The 3D view has no sky: it shows this as the background colour and the fog. A rough hint only.");
    lines << tr("Set by: %1.").arg(m_look.objects.join(QStringLiteral(", ")));
    lines << tr("Untick the box to get the 3D view's own look back.");
  }
  else
  {
    lines << tr("The script changes the environment: %1. Tick the box to see a hint in the 3D view.")
               .arg(parts.join(QStringLiteral(", ")));
  }
  if (m_look.hasFog && m_look.fogAlpha > MostAlpha && (m_enabled || !lines.isEmpty()))
    lines << tr("An alpha of 1 never ends in fog: the 3D view shows %1 at that distance.").arg(number(MostAlpha));
  if (m_enabled || m_look.any()) lines << m_look.notShown;
  for (const QString& text : m_look.ignored)
    if (m_enabled || m_look.any()) lines << tr("Not used: %1 is not a value the 3D view can use.").arg(text);
  return lines;
}
