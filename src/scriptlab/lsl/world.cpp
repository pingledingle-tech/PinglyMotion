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
    if (!m_scriptTag.isEmpty()) entry["script"] = m_scriptTag;
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

bool World::logFull()
{
  if (m_target) return m_target->logFull();
  if (m_log.size() < MAX_LOG_ENTRIES) return false;
  m_truncated = true;
  return true;
}

void World::chat(double time, const QString& kind, int channel, const QString& text)
{
  if (logFull()) return;
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
  const bool textBox = isTextBox(buttons);
  if (!textBox)
    for (const QString& b : buttons) list.append(b);
  QJsonObject entry{{"type", "dialog"}, {"to", avatar}, {"message", clip(message)}, {"buttons", list}, {"channel", channel}};
  if (textBox) entry["text_box"] = true;
  log(time, entry);
}

void World::recordOnly(double time, const QString& function, const QJsonArray& args)
{
  if (logFull()) return;
  log(time, QJsonObject{{"type", "recorded_only"}, {"function", function}, {"args", args},
                        {"note", "Recorded only: the sandbox plays and shows nothing for this."}});
}

void World::messageLinked(double time, int link, int num, const QString& str, const QString& id)
{
  if (logFull()) return;
  log(time, QJsonObject{{"type", "message_linked"}, {"link", link}, {"num", num}, {"text", clip(str)}, {"id", clip(id)}, {"scripts", 0}});
}

void World::linksetDataChanged(double, int, const QString&, const QString&)
{
}

// ---------------------------------------------------------------- Linkset Data

int LinksetData::cost(const QString& name, const QString& value)
{
  return int(name.toUtf8().size() + value.toUtf8().size());
}

int LinksetData::write(const QString& name, const QString& value)
{
  if (name.isEmpty()) return ENoKey;
  auto it = m_pairs.find(name);
  if (value.isEmpty())
  {
    if (it == m_pairs.end()) return NotFound;
    m_used -= cost(name, *it);
    m_pairs.erase(it);
    return Ok;
  }
  if (it != m_pairs.end() && *it == value) return NoUpdate;
  const qint64 old = it == m_pairs.end() ? 0 : cost(name, *it);
  const qint64 now = qint64(name.toUtf8().size()) + value.toUtf8().size();
  if (m_used - old + now > MAX_BYTES) return EMemory;
  m_used = int(m_used - old + now);
  m_pairs.insert(name, value);
  return Ok;
}

int LinksetData::remove(const QString& name)
{
  auto it = m_pairs.find(name);
  if (it == m_pairs.end()) return NotFound;
  m_used -= cost(name, *it);
  m_pairs.erase(it);
  return Ok;
}

QStringList LinksetData::keys(qint64 start, qint64 count) const
{
  QStringList out;
  if (start < 0) start = 0;
  qint64 i = 0;
  for (auto it = m_pairs.constBegin(); it != m_pairs.constEnd(); ++it, ++i)
  {
    if (i < start) continue;
    if (count >= 1 && qint64(out.size()) >= count) break;
    out << it.key();
  }
  return out;
}

QString LinksetData::resultName(int result)
{
  static const char* const names[] = {"LINKSETDATA_OK", "LINKSETDATA_EMEMORY", "LINKSETDATA_ENOKEY",
                                      "LINKSETDATA_EPROTECTED", "LINKSETDATA_NOTFOUND", "LINKSETDATA_NOUPDATE"};
  return result >= 0 && result <= 5 ? QString(names[result]) : QString::number(result);
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
