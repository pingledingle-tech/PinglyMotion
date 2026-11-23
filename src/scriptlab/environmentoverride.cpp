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

  // a whole number from low to high
  bool whole(const QString& option, int low, int high, int* value)
  {
    bool ok = false;
    const int v = QLocale::c().toInt(option.trimmed(), &ok);
    if (!ok || v < low || v > high) return false;
    *value = v;
    return true;
  }

  // a sphere distance in metres: 0 to 10 km
  bool reach(const QString& option, double* value)
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(option.trimmed(), &ok);
    if (!ok || !std::isfinite(v) || v < 0 || v > 10000) return false;
    *value = v;
    return true;
  }

  // a sphere strength: 0 to 100 (1 is full for the colour fade; a blur can take more)
  bool strength(const QString& option, double* value)
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(option.trimmed(), &ok);
    if (!ok || !std::isfinite(v) || v < 0 || v > 100) return false;
    *value = v;
    return true;
  }

  // "x[/y[/z[/w]]]": one to four numbers, a part not given is 0
  bool params(const QString& option, QVector4D* value)
  {
    const QStringList parts = option.split(QLatin1Char('/'));
    if (parts.isEmpty() || parts.size() > 4) return false;
    QVector4D out(0, 0, 0, 0);
    for (int i = 0; i < parts.size(); ++i)
    {
      bool ok = false;
      const double v = QLocale::c().toDouble(parts.at(i).trimmed(), &ok);
      if (!ok || !std::isfinite(v) || std::fabs(v) > 10000) return false;
      out[i] = float(v);
    }
    *value = out;
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

  // --- the vision sphere: setsphere held (RLVa only), its values forced (RLVa takes them with =force only)
  QString option, object;
  if (held(list, QStringLiteral("setsphere"), &option, &object))
  {
    QSet<QString> sphereObjects{object};
    Sphere sphere;
    bool usable = true;
    const auto value = [&](const QString& name, QString* out) -> bool
    {
      if (!recorded.contains(name)) return false;
      *out = recorded.value(name);
      sphereObjects.insert(by.value(name));
      return true;
    };
    QString text;
    double v = 0;
    if (value(QStringLiteral("setsphere_mode"), &text))
    {
      if (!whole(text, 0, 4, &sphere.mode)) ignore(QStringLiteral("setsphere_mode:") + text);
      else if (sphere.mode == Sphere::ChromaticAberration)
      {
        look.notShown << tr("Not shown: setsphere_mode:3, the colour split (only the colour fade 0, blur 1 and 2, "
                            "and pixel 4 are shown).");
        usable = false;
      }
    }
    if (value(QStringLiteral("setsphere_origin"), &text) && !whole(text, 0, 1, &sphere.origin))
      ignore(QStringLiteral("setsphere_origin:") + text);
    if (value(QStringLiteral("setsphere_distmin"), &text))
    {
      if (reach(text, &v)) sphere.distMin = v;
      else ignore(QStringLiteral("setsphere_distmin:") + text);
    }
    if (value(QStringLiteral("setsphere_distmax"), &text))
    {
      if (reach(text, &v)) sphere.distMax = v;
      else ignore(QStringLiteral("setsphere_distmax:") + text);
    }
    if (value(QStringLiteral("setsphere_distextend"), &text) && !whole(text, 0, 3, &sphere.distExtend))
      ignore(QStringLiteral("setsphere_distextend:") + text);
    if (value(QStringLiteral("setsphere_valuemin"), &text))
    {
      if (strength(text, &v)) sphere.valueMin = v;
      else ignore(QStringLiteral("setsphere_valuemin:") + text);
    }
    if (value(QStringLiteral("setsphere_valuemax"), &text))
    {
      if (strength(text, &v)) sphere.valueMax = v;
      else ignore(QStringLiteral("setsphere_valuemax:") + text);
    }
    // the colour (r/g/b) and the params (up to four numbers) set the same value in RLVa; the params win here
    QVector3D c;
    if (value(QStringLiteral("setsphere_color"), &text))
    {
      if (colour(text, &c)) sphere.params = QVector4D(c, 1.0f);
      else ignore(QStringLiteral("setsphere_color:") + text);
    }
    QVector4D p;
    if (value(QStringLiteral("setsphere_param"), &text))
    {
      if (params(text, &p)) sphere.params = p;
      else ignore(QStringLiteral("setsphere_param:") + text);
    }
    if (value(QStringLiteral("setsphere_tween"), &text))
      look.notShown << tr("Not shown: setsphere_tween:%1 (the 3D view changes at once).").arg(text);
    if (usable)
    {
      look.hasSphere = true;
      look.sphere = sphere;
      for (const QString& o : sphereObjects) objects.insert(o);
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

double EnvironmentOverride::Sphere::strengthAt(double metres) const
{
  // as RLVa's sphere shader: a straight line from valueMin at distMin to valueMax at distMax; below and
  // beyond, the end value if that side is extended, otherwise 0
  if (metres < distMin) return (distExtend & ExtendMin) ? valueMin : 0.0;
  if (metres > distMax) return (distExtend & ExtendMax) ? valueMax : 0.0;
  if (distMax <= distMin) return valueMax;
  return valueMin + (valueMax - valueMin) * (metres - distMin) / (distMax - distMin);
}

namespace
{
  // a box blur of all four parts (the alpha too, so a picture read as premultiplied stays right), across and then down, with running sums; done twice it is
  // close to a soft (Gaussian) blur. The edges repeat the last pixel.
  void boxBlur(QImage& image, int radius)
  {
    if (radius < 1) return;
    const int w = image.width(), h = image.height();
    std::vector<int> line;
    std::vector<uchar> out;
    const int span = 2 * radius + 1;
    // pass 0 across a row, pass 1 down a column
    for (int pass = 0; pass < 2; ++pass)
    {
      const int lines = pass == 0 ? h : w;
      const int length = pass == 0 ? w : h;
      line.assign(size_t(length) * 4, 0);
      out.assign(size_t(length) * 4, 0);
      for (int l = 0; l < lines; ++l)
      {
        const auto at = [&](int i) -> uchar*
        {
          i = qBound(0, i, length - 1);
          return pass == 0 ? image.scanLine(l) + i * 4 : image.scanLine(i) + l * 4;
        };
        int sum[4] = {0, 0, 0, 0};
        for (int i = -radius; i <= radius; ++i)
          for (int k = 0; k < 4; ++k) sum[k] += at(i)[k];
        for (int i = 0; i < length; ++i)
        {
          for (int k = 0; k < 4; ++k) out[size_t(i) * 4 + k] = uchar((sum[k] + span / 2) / span);
          for (int k = 0; k < 4; ++k) sum[k] += at(i + radius + 1)[k] - at(i - radius)[k];
        }
        for (int i = 0; i < length; ++i)
        {
          uchar* px = pass == 0 ? image.scanLine(l) + i * 4 : image.scanLine(i) + l * 4;
          for (int k = 0; k < 4; ++k) px[k] = out[size_t(i) * 4 + k];
        }
      }
    }
  }

  uchar mixed(uchar a, double b, double t)
  {
    return uchar(qBound(0.0, std::round(a + (b - a) * t), 255.0));
  }
}

void EnvironmentOverride::applySphere(QImage& image, const std::vector<float>& metres, const Sphere& sphere)
{
  const int w = image.width(), h = image.height();
  if (w <= 0 || h <= 0 || metres.size() != size_t(w) * size_t(h)) return;
  if (image.format() != QImage::Format_RGBA8888 && image.format() != QImage::Format_RGBA8888_Premultiplied &&
      image.format() != QImage::Format_RGBX8888)
    image.convertTo(QImage::Format_RGBA8888);

  std::vector<float> strengths(metres.size());
  bool any = false;
  for (size_t i = 0; i < metres.size(); ++i)
  {
    strengths[i] = float(sphere.strengthAt(double(metres[i])));
    if (strengths[i] > 0) any = true;
  }
  if (!any) return;

  if (sphere.mode == Sphere::ColourFade)
  {
    const double c[3] = {qBound(0.0, double(sphere.params.x()), 1.0) * 255.0, qBound(0.0, double(sphere.params.y()), 1.0) * 255.0,
                         qBound(0.0, double(sphere.params.z()), 1.0) * 255.0};
    for (int y = 0; y < h; ++y)
    {
      uchar* row = image.scanLine(y);
      for (int x = 0; x < w; ++x)
      {
        const double t = qMin(1.0, double(strengths[size_t(y) * w + x]));
        if (t <= 0) continue;
        for (int k = 0; k < 3; ++k) row[x * 4 + k] = mixed(row[x * 4 + k], c[k], t);
      }
    }
  }
  else if (sphere.mode == Sphere::Blur || sphere.mode == Sphere::BlurVariable)
  {
    // the whole picture blurred once, then mixed in by the strength (variable: fully wherever there is any)
    const int radius = sphere.mode == Sphere::Blur ? BlurPixels
                                                   : qBound(1, int(std::lround(sphere.params.x() / 2.0)), MostPixels);
    QImage blurred = image.copy();
    boxBlur(blurred, radius);
    boxBlur(blurred, radius);
    for (int y = 0; y < h; ++y)
    {
      uchar* row = image.scanLine(y);
      const uchar* soft = blurred.constScanLine(y);
      for (int x = 0; x < w; ++x)
      {
        const float s = strengths[size_t(y) * w + x];
        if (s <= 0) continue;
        const double t = sphere.mode == Sphere::Blur ? qMin(1.0, double(s)) : 1.0;
        for (int k = 0; k < 4; ++k) row[x * 4 + k] = mixed(row[x * 4 + k], soft[x * 4 + k], t);
      }
    }
  }
  else if (sphere.mode == Sphere::Pixel)
  {
    // blocks of params x by y pixels, each the colour of its first pixel, wherever there is any strength
    const int bw = qBound(1, int(std::lround(sphere.params.x())), MostPixels);
    const int bh = qBound(1, int(std::lround(sphere.params.y())), MostPixels);
    if (bw == 1 && bh == 1) return;
    const QImage source = image.copy();
    for (int y = 0; y < h; ++y)
    {
      uchar* row = image.scanLine(y);
      const uchar* from = source.constScanLine((y / bh) * bh);
      for (int x = 0; x < w; ++x)
      {
        if (strengths[size_t(y) * w + x] <= 0) continue;
        const int sx = (x / bw) * bw;
        for (int k = 0; k < 4; ++k) row[x * 4 + k] = from[sx * 4 + k];
      }
    }
  }
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

QString EnvironmentOverride::sphereText(const Sphere& sphere)
{
  QString kind;
  switch (sphere.mode)
  {
    case Sphere::ColourFade:
      kind = tr("colour fade to %1").arg(colourText(sphere.params.toVector3D()));
      break;
    case Sphere::Blur: kind = tr("blur"); break;
    case Sphere::BlurVariable:
      kind = tr("blur of %1 pixels").arg(qBound(1, int(std::lround(sphere.params.x() / 2.0)), MostPixels));
      break;
    default:
      kind = tr("pixel blocks of %1 x %2")
               .arg(qBound(1, int(std::lround(sphere.params.x())), MostPixels))
               .arg(qBound(1, int(std::lround(sphere.params.y())), MostPixels));
      break;
  }
  const QString origin = sphere.origin == Sphere::Camera ? tr("camera") : tr("avatar");
  return tr("%1, strength %2 at %3 m to %4 at %5 m from the %6")
    .arg(kind, number(sphere.valueMin), number(sphere.distMin), number(sphere.valueMax), number(sphere.distMax), origin);
}

void EnvironmentOverride::setTrySphere(const Sphere& sphere)
{
  if (m_try && *m_try == sphere) return;
  m_try = sphere;
  emit changed();
}

void EnvironmentOverride::clearTrySphere()
{
  if (!m_try) return;
  m_try.reset();
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
  QStringList all = parts;
  if (m_look.hasSphere) all << tr("vision sphere (%1)").arg(sphereText(m_look.sphere));

  // the sliders' sphere counts only while the box is ticked; with the box off the text stays the script's
  const bool trying = m_enabled && m_try.has_value();
  if (!m_look.any() && !trying)
  {
    if (m_enabled && (!m_look.notShown.isEmpty() || !m_look.ignored.isEmpty()))
      lines << tr("The script asks nothing of the sky that the 3D view can show.");
  }
  else if (m_enabled)
  {
    if (!parts.isEmpty())
    {
      lines << tr("Environment set by the script: %1.").arg(parts.join(QStringLiteral(", ")));
      lines << tr("The 3D view has no sky: it shows this as the background colour and the fog. A rough hint only.");
    }
    if (trying)
    {
      lines << tr("Vision sphere from the sliders: %1.").arg(sphereText(*m_try));
      lines << tr("The whole picture changes by the distance from the %1. A rough look only: check it in Second Life.")
                 .arg(m_try->origin == Sphere::Camera ? tr("camera") : tr("avatar"));
      if (m_look.hasSphere) lines << tr("The sliders' sphere is shown instead of the script's. Press Clear to show the script's again.");
    }
    else if (m_look.hasSphere)
    {
      lines << tr("Vision sphere set by the script: %1.").arg(sphereText(m_look.sphere));
      lines << tr("The whole picture changes by the distance from the %1. A rough look only: check it in Second Life.")
                 .arg(m_look.sphere.origin == Sphere::Camera ? tr("camera") : tr("avatar"));
      if (m_look.sphere.mode == Sphere::Pixel && qBound(1, int(std::lround(m_look.sphere.params.x())), MostPixels) == 1 &&
          qBound(1, int(std::lround(m_look.sphere.params.y())), MostPixels) == 1)
        lines << tr("Pixel blocks of 1 x 1 change nothing: the block size comes from setsphere_param:<width>/<height>.");
    }
    if (m_look.any()) lines << tr("Set by: %1.").arg(m_look.objects.join(QStringLiteral(", ")));
    lines << tr("Untick the box to get the 3D view's own look back.");
  }
  else
  {
    lines << tr("The script changes the environment: %1. Tick the box to see a hint in the 3D view.")
               .arg(all.join(QStringLiteral(", ")));
  }
  if (m_look.hasFog && m_look.fogAlpha > MostAlpha && (m_enabled || !lines.isEmpty()))
    lines << tr("An alpha of 1 never ends in fog: the 3D view shows %1 at that distance.").arg(number(MostAlpha));
  if (m_enabled || m_look.any()) lines << m_look.notShown;
  for (const QString& text : m_look.ignored)
    if (m_enabled || m_look.any()) lines << tr("Not used: %1 is not a value the 3D view can use.").arg(text);
  return lines;
}
