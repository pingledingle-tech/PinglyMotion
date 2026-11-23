/*
 * PinglyMotion Script Lab - LSL interpreter: the world around a script
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <algorithm>
#include <cmath>

#include <QCryptographicHash>
#include <QDate>

#include "world.h"

namespace lsl
{

bool HttpReply::matches(const QString& requestMethod, const QString& requestUrl) const
{
  if (!method.isEmpty() && method != requestMethod) return false;
  if (url.endsWith('*')) return requestUrl.startsWith(url.left(url.size() - 1));
  return requestUrl == url;
}

const HttpReply* HttpReply::find(const std::vector<HttpReply>& replies, const QString& method, const QString& url)
{
  for (const HttpReply& r : replies)
    if (r.matches(method, url)) return &r;
  return nullptr;
}

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

// ---------------------------------------------------------------- inventory (step 9b-4)

namespace
{
  struct TypeName { const char* name; int number; };
  const TypeName TYPE_NAMES[] = {{"texture", 0}, {"sound", 1}, {"landmark", 3}, {"clothing", 5}, {"object", 6}, {"notecard", 7},
                                 {"script", 10}, {"bodypart", 13}, {"animation", 20}, {"gesture", 21}, {"setting", 56}, {"material", 57}};

  // sorted by name: upper and lower case alike, then the plain text order
  bool nameLess(const InventoryItem* a, const InventoryItem* b)
  {
    const int c = QString::compare(a->name, b->name, Qt::CaseInsensitive);
    return c != 0 ? c < 0 : a->name < b->name;
  }
}

int Inventory::typeFromName(const QString& name)
{
  QString n = name.trimmed().toLower();
  if (n.startsWith("inventory_")) n = n.mid(10);
  for (const TypeName& t : TYPE_NAMES)
    if (n == t.name) return t.number;
  return -2;
}

bool Inventory::knownType(int type)
{
  for (const TypeName& t : TYPE_NAMES)
    if (t.number == type) return true;
  return false;
}

QString Inventory::derivedKey(const QString& seed, const QString& name)
{
  // a fixed key from the seed and the name, so runs are repeatable
  const QByteArray h = QCryptographicHash::hash((seed + "/" + name).toUtf8(), QCryptographicHash::Md5).toHex();
  return QString("%1-%2-%3-%4-%5").arg(QString::fromLatin1(h.mid(0, 8)), QString::fromLatin1(h.mid(8, 4)),
                                         QString::fromLatin1(h.mid(12, 4)), QString::fromLatin1(h.mid(16, 4)), QString::fromLatin1(h.mid(20, 12)));
}

bool Inventory::add(InventoryItem item, const QString& seed)
{
  if (item.name.isEmpty() || int(m_items.size()) >= MAX_ITEMS || find(item.name)) return false;
  if (item.key.isEmpty())
  {
    item.key = derivedKey(seed, item.name);
  }
  m_items.push_back(std::move(item));
  return true;
}

bool Inventory::remove(const QString& name)
{
  for (size_t i = 0; i < m_items.size(); ++i)
    if (m_items[i].name == name)
    {
      m_items.erase(m_items.begin() + std::ptrdiff_t(i));
      return true;
    }
  return false;
}

const InventoryItem* Inventory::find(const QString& name) const
{
  for (const InventoryItem& i : m_items)
    if (i.name == name) return &i;
  return nullptr;
}

int Inventory::count(int type) const
{
  if (type == -1) return int(m_items.size());
  int n = 0;
  for (const InventoryItem& i : m_items)
    if (i.type == type) ++n;
  return n;
}

const InventoryItem* Inventory::at(int type, int index) const
{
  if (index < 0) return nullptr;
  std::vector<const InventoryItem*> list;
  for (const InventoryItem& i : m_items)
    if (type == -1 || i.type == type) list.push_back(&i);
  if (index >= int(list.size())) return nullptr;
  std::stable_sort(list.begin(), list.end(), nameLess);
  return list[size_t(index)];
}

QString World::giveInventory(double, const QString& to, const InventoryItem&)
{
  return to == ownerKey() ? "given" : "unknown_target";
}

QString World::giveInventoryList(double time, const QString& to, const std::vector<InventoryItem>& items, QStringList& refused)
{
  Q_UNUSED(time); Q_UNUSED(items);
  refused.clear();
  return to == ownerKey() ? "given" : "unknown_target";
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

const char* const World::GROUP_KEY = "9a0c0000-0000-4000-8000-000000000001";

float PrimStore::mass(const float rootSize[3], int count)
{
  float total = 0.0f;
  for (int link = 1; link <= qMax(1, count); ++link)
  {
    const float* s = link == 1 ? rootSize : prim(link, count).size;
    total += s[0] * s[1] * s[2] * 10.0f;
  }
  return total;
}

bool World::details(const QString& key, ObjectDetails& out)
{
  out = ObjectDetails();
  out.key = key;
  if (key == objectKey())
  {
    out.name = objectName();
    out.description = objectDesc();
    out.owner = ownerKey();
    position(out.position);
    scale(out.scale);
    const int count = qMax(1, numberOfPrims());
    const PrimState& root = primStore().prim(1, count);
    for (int i = 0; i < 4; ++i) out.rotation[i] = root.rotation[i];
    out.prims = count;
    out.sitters = sitterCount();
    out.attached = attachPoint();
    out.runningScripts = out.totalScripts = 1;   // a plain World runs one script
    out.inventoryItems = int(inventory().items().size());
    out.physics = (primStore().status & 1) ? 1 : 0;    // STATUS_PHYSICS
    out.phantom = (primStore().status & 16) ? 1 : 0;   // STATUS_PHANTOM
    out.tempOnRez = root.tempOnRez;
    out.mass = primStore().mass(out.scale, count);
    out.text = root.text;
    for (int i = 0; i < 3; ++i) out.textColor[i] = root.textColor[i];
    out.textAlpha = root.textAlpha;
    return true;
  }
  if (key == ownerKey())
  {
    out.avatar = true;
    out.name = ownerName();
    out.owner = key;
    const float avatar[3] = {128.0f, 128.0f, 20.0f};   // where the owner stands
    for (int i = 0; i < 3; ++i) out.position[i] = avatar[i];
    out.height = RegionSettings::DEFAULT_AVATAR_HEIGHT;
    return true;
  }
  return false;
}

// ---------------------------------------------------------------- the region (gap list 2, step 3)

namespace
{
const double DAY = 86400.0;

// UTC seconds since 1970 at simulated time now, in whole microseconds (so the answers do not depend on rounding)
qint64 microsecondsAt(qint64 unixStart, double now)
{
  return unixStart * 1000000 + qint64(std::llround(now * 1.0e6));
}

QDate dateOf(qint64 seconds)
{
  qint64 days = seconds / 86400;
  if (seconds % 86400 < 0) --days;
  return QDate(1970, 1, 1).addDays(days);
}

// the n-th Sunday (n >= 1) of a month, as days since 1970
qint64 nthSunday(int year, int month, int n)
{
  const QDate first(year, month, 1);
  const int firstSunday = 1 + (7 - first.dayOfWeek()) % 7;   // dayOfWeek: 1 Monday to 7 Sunday
  return QDate(1970, 1, 1).daysTo(QDate(year, month, firstSunday + 7 * (n - 1)));
}
}

QString RegionSettings::date(double now) const
{
  return dateOf(microsecondsAt(unixStart, now) / 1000000).toString("yyyy-MM-dd");
}

QString RegionSettings::timestamp(double now) const
{
  const qint64 us = microsecondsAt(unixStart, now);
  qint64 seconds = us / 1000000, micro = us % 1000000;
  if (micro < 0) { micro += 1000000; --seconds; }
  qint64 inDay = seconds % 86400;
  if (inDay < 0) inDay += 86400;
  return QString("%1T%2:%3:%4.%5Z")
      .arg(dateOf(seconds).toString("yyyy-MM-dd"))
      .arg(inDay / 3600, 2, 10, QChar('0'))
      .arg(inDay / 60 % 60, 2, 10, QChar('0'))
      .arg(inDay % 60, 2, 10, QChar('0'))
      .arg(micro, 6, 10, QChar('0'));
}

double RegionSettings::wallclock(double now) const
{
  // Second Life time is US Pacific time: UTC-8, and UTC-7 in summer time, from the second Sunday of March at 2:00
  // standard time (10:00 UTC) to the first Sunday of November at 2:00 summer time (9:00 UTC), the US rule since 2007
  const double utc = double(microsecondsAt(unixStart, now)) / 1.0e6;
  const int year = dateOf(qint64(std::floor(utc))).year();
  const double summerStart = double(nthSunday(year, 3, 2)) * DAY + 10.0 * 3600.0;
  const double summerEnd = double(nthSunday(year, 11, 1)) * DAY + 9.0 * 3600.0;
  const double local = utc + (utc >= summerStart && utc < summerEnd ? -7.0 : -8.0) * 3600.0;
  return local - std::floor(local / DAY) * DAY;
}

double RegionSettings::timeOfDayAt(double now) const
{
  const double t = timeOfDay + now;
  return t - std::floor(t / dayLength) * dayLength;
}

void RegionSettings::sunDirection(double now, float out[3]) const
{
  // a sun that rises in the east (+x) at a quarter of the day, stands straight up at half and sets in the west (-x) at
  // three quarters (the sandbox's simple model: Second Life's sun also leans to one side); values within 1e-6 of a
  // whole number are made whole, so noon is exactly <0, 0, 1>
  const double angle = 2.0 * 3.14159265358979323846 * timeOfDayAt(now) / dayLength;
  const double v[3] = {std::sin(angle), 0.0, -std::cos(angle)};
  for (int i = 0; i < 3; ++i)
  {
    const double r = std::round(v[i]);
    out[i] = float(std::fabs(v[i] - r) < 1.0e-6 ? r : v[i]);
    if (out[i] == 0.0f) out[i] = 0.0f;   // no negative zero
  }
}

// ---------------------------------------------------------------- gap list 2, step 4: fake web server and mailbox

const char* const World::URL_HOST = "sandbox.invalid";   // ".invalid" never resolves (RFC 2606)

QString World::makeUrl(bool secure)
{
  ++m_urlCounter;
  const QString cap = Inventory::derivedKey(objectKey() + "/" + scriptName(), QString("url %1").arg(m_urlCounter));
  return QString(secure ? "https://%1:12043/cap/%2" : "http://%1:12046/cap/%2").arg(URL_HOST, cap);
}

QString World::grantUrl(bool secure)
{
  if (int(m_urls.size()) >= MAX_URLS) return QString();
  const QString url = makeUrl(secure);
  m_urls.append(url);
  return url;
}

bool emailMatches(const Email& email, const QString& address, const QString& subject)
{
  return (address.isEmpty() || email.address.compare(address, Qt::CaseInsensitive) == 0) &&
         (subject.isEmpty() || email.subject.compare(subject, Qt::CaseInsensitive) == 0);
}

bool World::nextEmail(const QString& address, const QString& subject, Email& out, int& left)
{
  for (auto it = m_mail.begin(); it != m_mail.end(); ++it)
    if (emailMatches(*it, address, subject))
    {
      out = *it;
      m_mail.erase(it);
      left = int(m_mail.size());
      return true;
    }
  return false;
}

} // namespace lsl
