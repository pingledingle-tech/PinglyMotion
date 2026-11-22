/*
 * PinglyMotion Script Lab - RLV view modes shown in the 3D view (job 7c)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QLocale>
#include <QSet>

#include "rlvstate.h"

#include "viewmodesoverride.h"

namespace
{
  bool unit(const QString& option, double* value)
  {
    bool ok = false;
    const double v = QLocale::c().toDouble(option.trimmed(), &ok);
    if (!ok || !std::isfinite(v) || v < 0 || v > 1) return false;
    *value = v;
    return true;
  }

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
    return QLocale::c().toString(std::round(value * 1000.0) / 1000.0, 'g', 6);
  }

  QString colourText(const QVector3D& c)
  {
    return number(c.x()) + QLatin1Char('/') + number(c.y()) + QLatin1Char('/') + number(c.z());
  }
}

QVector4D ViewModesOverride::plainFloorColour(const QVector4D& tile)
{
  // a red tile (no green) belongs to the protected first frame's floor
  if (tile.y() == 0.0f && tile.x() > 0.0f) return QVector4D(0.55f, 0.0f, 0.0f, tile.w());
  return QVector4D(0.35f, 0.35f, 0.35f, tile.w());
}

ViewModesOverride::Modes ViewModesOverride::modesFrom(const RlvState& state)
{
  Modes modes;
  QSet<QString> objects;
  const QVector<RlvState::Restriction> list = state.restrictions();
  const QMap<QString, QString> recorded = state.recorded();
  const QMap<QString, QString> by = state.recordedBy();

  bool overlayHeld = false;
  QSet<QString> overlayObjects;
  bool wireframe = false, transparent = false, head = false;
  for (const RlvState::Restriction& r : list)
  {
    if (r.behaviour == QLatin1String("showself"))
    {
      modes.hideFigure = true;
      objects.insert(r.object);
    }
    else if (r.behaviour == QLatin1String("setcam_textures") || r.behaviour == QLatin1String("camtextures"))
    {
      if (!modes.plainFloor) modes.texture = r.option;
      modes.plainFloor = true;
      objects.insert(r.object);
    }
    else if (r.behaviour == QLatin1String("setoverlay"))
    {
      overlayHeld = true;
      overlayObjects.insert(r.object);
    }
    else if (r.behaviour == QLatin1String("showselfhead")) head = true;
    else if (r.behaviour == QLatin1String("viewwireframe")) wireframe = true;
    else if (r.behaviour == QLatin1String("viewtransparent")) transparent = true;
  }
  if (head) modes.notShown << tr("Not shown: showselfhead (the figure's head cannot be hidden on its own).");
  if (wireframe)
    modes.notShown << tr("Not shown: viewwireframe blocks the wireframe view, which the 3D view does not have.");
  if (transparent)
    modes.notShown << tr("Not shown: viewtransparent blocks highlighting transparent objects, which the 3D view does not have.");

  if (overlayHeld)
  {
    // a value held as a restriction (the first one), else the one sent as =force
    const auto value = [&](const QString& name, QString* out) -> bool
    {
      for (const RlvState::Restriction& r : list)
        if (r.behaviour == name)
        {
          *out = r.option;
          overlayObjects.insert(r.object);
          return true;
        }
      if (recorded.contains(name))
      {
        *out = recorded.value(name);
        overlayObjects.insert(by.value(name));
        return true;
      }
      return false;
    };
    QString text;
    bool usable = true;
    if (value(QStringLiteral("setoverlay_alpha"), &text) && !unit(text, &modes.alpha))
    {
      modes.ignored << QStringLiteral("setoverlay_alpha:") + text;
      usable = false;
    }
    if (value(QStringLiteral("setoverlay_tint"), &text) && !colour(text, &modes.tint))
    {
      modes.ignored << QStringLiteral("setoverlay_tint:") + text;
      usable = false;
    }
    if (!value(QStringLiteral("setoverlay_texture"), &text) || text.isEmpty())
    {
      modes.notShown << tr("Not shown: setoverlay without setoverlay_texture (an overlay without a picture shows nothing).");
      usable = false;
    }
    else modes.overlayTexture = text;
    if (value(QStringLiteral("setoverlay_tween"), &text))
      modes.notShown << tr("Not shown: setoverlay_tween:%1 (the overlay changes at once).").arg(text);
    if (usable)
    {
      modes.overlay = true;
      for (const QString& o : overlayObjects) objects.insert(o);
    }
    else
    {
      modes.alpha = 1;
      modes.tint = QVector3D(1, 1, 1);
      modes.overlayTexture.clear();
    }
  }

  objects.remove(QString());
  modes.objects = QStringList(objects.begin(), objects.end());
  modes.objects.sort();
  return modes;
}

ViewModesOverride::ViewModesOverride(QObject* parent) :
  QObject(parent)
{
}

void ViewModesOverride::setState(RlvState* state)
{
  for (const QMetaObject::Connection& c : m_connections) disconnect(c);
  m_connections.clear();
  m_state = state;
  if (state)
  {
    m_connections << connect(state, &RlvState::changed, this, &ViewModesOverride::update);
    m_connections << connect(state, &RlvState::modeChanged, this, &ViewModesOverride::update);
  }
  update();
}

void ViewModesOverride::setEnabled(bool on)
{
  if (on == m_enabled) return;
  m_enabled = on;
  emit changed();
}

void ViewModesOverride::update()
{
  const Modes modes = m_state ? modesFrom(*m_state) : Modes();
  if (modes == m_modes) return;
  m_modes = modes;
  emit changed();
}

QStringList ViewModesOverride::lines() const
{
  QStringList lines;
  QStringList parts;
  if (m_modes.hideFigure) parts << tr("your own avatar hidden (showself)");
  if (m_modes.plainFloor)
    parts << (m_modes.texture.isEmpty() ? tr("textures replaced (setcam_textures)")
                                        : tr("textures replaced by %1 (setcam_textures)").arg(m_modes.texture));
  if (m_modes.overlay)
    parts << tr("overlay %1 with tint %2 at alpha %3").arg(m_modes.overlayTexture, colourText(m_modes.tint), number(m_modes.alpha));

  const bool anything = m_modes.any() || !m_modes.notShown.isEmpty() || !m_modes.ignored.isEmpty();
  if (!m_modes.any())
  {
    if (m_enabled && anything) lines << tr("The script changes no view mode that the 3D view can show.");
  }
  else if (m_enabled)
  {
    lines << tr("View changed by the script: %1.").arg(parts.join(QStringLiteral(", ")));
    if (m_modes.hideFigure) lines << tr("The figure and its shadow are not drawn.");
    if (m_modes.plainFloor) lines << tr("The floor is drawn in one plain colour; a texture cannot be loaded here.");
    if (m_modes.overlay) lines << tr("The overlay picture cannot be loaded here: the 3D view is covered by its tint at its alpha.");
    lines << tr("Set by: %1.").arg(m_modes.objects.join(QStringLiteral(", ")));
    lines << tr("Untick the box to get the 3D view's own look back.");
  }
  else
  {
    lines << tr("The script changes the view: %1. Tick the box to see it in the 3D view.").arg(parts.join(QStringLiteral(", ")));
  }
  if (m_enabled || m_modes.any())
  {
    lines << m_modes.notShown;
    for (const QString& text : m_modes.ignored) lines << tr("Not used: %1 is not a value the 3D view can use.").arg(text);
  }
  return lines;
}
