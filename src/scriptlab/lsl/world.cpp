/*
 * PinglyMotion Script Lab - LSL interpreter: the world around a script
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include "world.h"

namespace lsl
{

QString World::clip(const QString& text)
{
  return text.size() > MAX_TEXT_CHARS ? text.left(MAX_TEXT_CHARS) : text;
}

void World::log(double time, QJsonObject entry, bool force)
{
  if (m_target)
  {
    if (!m_tag.isEmpty()) entry["object"] = m_tag;
    m_target->log(time, entry, force);
    return;
  }
  if (m_log.size() >= MAX_LOG_ENTRIES && !force)
  {
    m_truncated = true;
    return;
  }
  QJsonObject out;
  out["t"] = std::round(time * 1000.0) / 1000.0;
  for (auto it = entry.begin(); it != entry.end(); ++it) out[it.key()] = it.value();
  m_log.append(out);
}

void World::chat(double time, const QString& kind, int channel, const QString& text)
{
  log(time, QJsonObject{{"type", kind}, {"channel", channel}, {"text", clip(text)}});
}

void World::ownerSay(double time, const QString& text)
{
  log(time, QJsonObject{{"type", "owner_say"}, {"text", clip(text)}});
}

void World::instantMessage(double time, const QString& key, const QString& text)
{
  log(time, QJsonObject{{"type", "instant_message"}, {"to", key}, {"text", clip(text)}});
}

void World::animation(double time, bool start, const QString& name)
{
  log(time, QJsonObject{{"type", start ? "animation_start" : "animation_stop"}, {"name", clip(name)}});
}

void World::chatTo(double time, const QString& target, int channel, const QString& text)
{
  log(time, QJsonObject{{"type", "region_say_to"}, {"to", target}, {"channel", channel}, {"text", clip(text)}});
}

void World::dialog(double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel)
{
  QJsonArray list;
  for (const QString& b : buttons) list.append(b);
  log(time, QJsonObject{{"type", "dialog"}, {"to", avatar}, {"message", clip(message)}, {"buttons", list}, {"channel", channel}});
}

QString World::nameOf(const QString& key) const
{
  if (key == ownerKey()) return ownerName();
  if (key == objectKey()) return objectName();
  return QString();
}

QString World::ownerOf(const QString& key) const
{
  if (key == ownerKey() || key == objectKey()) return ownerKey();
  return QString();
}

} // namespace lsl
