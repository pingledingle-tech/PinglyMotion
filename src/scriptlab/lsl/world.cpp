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

const char* const TransferPlan::FAILURES[4] = {"THROTTLED", "EXPIRED", "SERVICE_ERROR", "LINDENDOLLAR_INSUFFICIENTFUNDS"};

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

// ---- third gap list run, step 8: keyframed motion

namespace
{
// Second Life's rotation product: a * b applies a, then b (so a turn in the object's own axes is turn * rotation)
void quatMul(const double a[4], const double b[4], double out[4])
{
  out[0] = b[3] * a[0] + b[0] * a[3] + b[1] * a[2] - b[2] * a[1];
  out[1] = b[3] * a[1] + b[1] * a[3] + b[2] * a[0] - b[0] * a[2];
  out[2] = b[3] * a[2] + b[2] * a[3] + b[0] * a[1] - b[1] * a[0];
  out[3] = b[3] * a[3] - b[0] * a[0] - b[1] * a[1] - b[2] * a[2];
}

// the part fraction (0 to 1) of a turn, by the shortest way; backwards undoes the turn
void partOfTurn(const float turn[4], double fraction, bool backwards, double out[4])
{
  double q[4] = {turn[0], turn[1], turn[2], turn[3]};
  const double m = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
  out[0] = out[1] = out[2] = 0.0;
  out[3] = 1.0;
  if (!(m > 1.0e-9)) return;
  for (double& c : q) c /= m;
  if (q[3] < 0.0) for (double& c : q) c = -c;
  const double sinHalf = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);
  if (!(sinHalf > 1.0e-9)) return;
  const double half = std::atan2(sinHalf, q[3]) * fraction * (backwards ? -1.0 : 1.0);
  const double k = std::sin(half) / sinHalf;
  out[0] = q[0] * k;
  out[1] = q[1] * k;
  out[2] = q[2] * k;
  out[3] = std::cos(half);
}
}

void KeyframedMotion::reset()
{
  index = mode == MODE_REVERSE && !frames.empty() ? int(frames.size()) - 1 : 0;
  into = 0.0;
  backwards = mode == MODE_REVERSE;
}

void KeyframedMotion::advance(double dt, float pos[3], float rot[4], float velocity[3])
{
  for (int i = 0; i < 3; ++i) velocity[i] = 0.0f;
  if (!playing || frames.empty()) return;
  const int last = int(frames.size()) - 1;
  double remaining = dt;
  for (int guard = 0; remaining > 1.0e-9 && playing && guard < 100000; ++guard)
  {
    if (index < 0 || index > last) index = qBound(0, index, last);
    const Frame& f = frames[size_t(index)];
    const double use = std::min(remaining, f.time - into);
    const double fraction = f.time > 0.0 ? use / f.time : 1.0;
    const double sign = backwards ? -1.0 : 1.0;
    for (int i = 0; i < 3; ++i) pos[i] += float(sign * double(f.move[i]) * fraction);
    double piece[4], cur[4] = {rot[0], rot[1], rot[2], rot[3]}, next[4];
    partOfTurn(f.turn, fraction, backwards, piece);
    quatMul(piece, cur, next);
    const double n = std::sqrt(next[0] * next[0] + next[1] * next[1] + next[2] * next[2] + next[3] * next[3]);
    for (int i = 0; i < 4; ++i) rot[i] = float(n > 1.0e-12 ? next[i] / n : cur[i]);
    into += use;
    remaining -= use;
    if (into < f.time - 1.0e-9) continue;
    // the frame is done: the next one, or what the mode does at the end
    into = 0.0;
    if (!backwards)
    {
      if (index < last) ++index;
      else if (mode == MODE_LOOP) index = 0;
      else if (mode == MODE_PING_PONG) backwards = true;
      else { playing = false; reset(); }
    }
    else
    {
      if (index > 0) --index;
      else if (mode == MODE_PING_PONG) backwards = false;
      else { playing = false; reset(); }   // REVERSE has played every frame
    }
  }
  if (playing && !frames.empty())
  {
    const Frame& f = frames[size_t(qBound(0, index, last))];
    const double sign = backwards ? -1.0 : 1.0;
    for (int i = 0; i < 3; ++i) velocity[i] = f.time > 0.0 ? float(sign * double(f.move[i]) / f.time) : 0.0f;
  }
}

bool World::createLink(double time, const QString& target, bool parent)
{
  if (logFull()) return false;
  log(time, QJsonObject{{"type", "link"}, {"op", "create"}, {"target", target}, {"parent", parent},
                        {"linked", false}, {"note", "There is no other object here to link."}});
  return false;
}

bool World::breakLink(double time, int link)
{
  if (!logFull())
    log(time, QJsonObject{{"type", "link"}, {"op", "break"}, {"link", link}, {"released", false}, {"note", "Nothing was linked with llCreateLink."}});
  return false;
}

int World::breakAllLinks(double time)
{
  if (!logFull())
    log(time, QJsonObject{{"type", "link"}, {"op", "break_all"}, {"released", 0}, {"note", "Nothing was linked with llCreateLink."}});
  return 0;
}

QString World::clip(const QString& text)
{
  return text.size() > MAX_TEXT_CHARS ? text.left(MAX_TEXT_CHARS) : text;
}

void World::log(double time, QJsonObject entry, bool force)
{
  ++m_actions;
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
  ++m_actions;
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

void PrimStore::boundingBox(const float rootSize[3], int count, float outMin[3], float outMax[3])
{
  count = qMax(1, count);
  auto turn = [](const float q[4], const float v[3], float r[3])
  {
    // r = q * v * q^-1 for a unit quaternion q = (x, y, z, s)
    const float tx = 2.0f * (q[1] * v[2] - q[2] * v[1]), ty = 2.0f * (q[2] * v[0] - q[0] * v[2]), tz = 2.0f * (q[0] * v[1] - q[1] * v[0]);
    r[0] = v[0] + q[3] * tx + (q[1] * tz - q[2] * ty);
    r[1] = v[1] + q[3] * ty + (q[2] * tx - q[0] * tz);
    r[2] = v[2] + q[3] * tz + (q[0] * ty - q[1] * tx);
  };
  auto mul = [](const float a[4], const float b[4], float r[4])
  {
    r[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
    r[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
    r[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
    r[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
  };
  const float rootRot[4] = {prim(1, count).rotation[0], prim(1, count).rotation[1], prim(1, count).rotation[2], prim(1, count).rotation[3]};
  bool first = true;
  for (int link = 1; link <= count; ++link)
  {
    const PrimState& p = prim(link, count);
    const float* size = link == 1 ? rootSize : p.size;
    float centre[3] = {0.0f, 0.0f, 0.0f}, rot[4] = {rootRot[0], rootRot[1], rootRot[2], rootRot[3]};
    if (link > 1)
    {
      turn(rootRot, p.position, centre);
      mul(rootRot, p.rotation, rot);
    }
    for (int corner = 0; corner < 8; ++corner)
    {
      const float local[3] = {(corner & 1 ? 0.5f : -0.5f) * size[0], (corner & 2 ? 0.5f : -0.5f) * size[1], (corner & 4 ? 0.5f : -0.5f) * size[2]};
      float r[3];
      turn(rot, local, r);
      for (int k = 0; k < 3; ++k)
      {
        const float v = centre[k] + r[k];
        if (first || v < outMin[k]) outMin[k] = v;
        if (first || v > outMax[k]) outMax[k] = v;
      }
      first = false;
    }
  }
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
    primStore().boundingBox(out.scale, count, out.boundMin, out.boundMax);
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

// ---------------------------------------------------------------- step 19: the experience and its key-value store

qint64 ExperienceState::used() const
{
  qint64 total = 0;
  for (auto it = store.begin(); it != store.end(); ++it) total += it.key().toUtf8().size() + it.value().toUtf8().size();
  return total;
}

// what every operation checks first, in this order: an experience, its state, the store, the sizes
int ExperienceState::ready() const
{
  if (!present) return NoExperience;
  if (state != None) return state;
  if (!storeEnabled) return StoreDisabled;
  return None;
}

int ExperienceState::check(const QString& k, const QString* v) const
{
  const int r = ready();
  if (r != None) return r;
  const int keyBytes = k.toUtf8().size();
  if (keyBytes < 1 || keyBytes > MAX_KEY_BYTES) return InvalidParameters;
  if (v && v->toUtf8().size() > MAX_VALUE_BYTES) return InvalidParameters;
  return None;
}

int ExperienceState::create(const QString& k, const QString& v)
{
  const int e = check(k, &v);
  if (e != None) return e;
  if (store.contains(k)) return StorageException;
  if (used() + k.toUtf8().size() + v.toUtf8().size() > quota) return QuotaExceeded;
  store.insert(k, v);
  return None;
}

int ExperienceState::read(const QString& k, QString& value) const
{
  const int e = check(k, nullptr);
  if (e != None) return e;
  auto it = store.constFind(k);
  if (it == store.constEnd()) return KeyNotFound;
  value = it.value();
  return None;
}

int ExperienceState::update(const QString& k, const QString& v, bool checked, const QString& original)
{
  const int e = check(k, &v);
  if (e != None) return e;
  auto it = store.find(k);
  if (checked && (it == store.end() || it.value() != original)) return RetryUpdate;
  const qint64 before = it == store.end() ? 0 : k.toUtf8().size() + it.value().toUtf8().size();
  if (used() - before + k.toUtf8().size() + v.toUtf8().size() > quota) return QuotaExceeded;
  store.insert(k, v);
  return None;
}

int ExperienceState::remove(const QString& k, QString& value)
{
  const int e = check(k, nullptr);
  if (e != None) return e;
  auto it = store.find(k);
  if (it == store.end()) return StorageException;
  value = it.value();
  store.erase(it);
  return None;
}

QString ExperienceState::answer(int error, const QString& payload)
{
  return error == None ? "1," + payload : QString("0,%1").arg(error);
}

QString ExperienceState::errorMessage(int error)
{
  switch (error)
  {
    case 0: return "no error";
    case 1: return "exceeded throttle";
    case 2: return "experiences are disabled";
    case 3: return "invalid parameters";
    case 4: return "operation not permitted";
    case 5: return "script not associated with an experience";
    case 6: return "not found";
    case 7: return "invalid experience";
    case 8: return "experience is disabled";
    case 9: return "experience is suspended";
    case 11: return "experience data quota exceeded";
    case 12: return "key-value store is disabled";
    case 13: return "key-value store communication failed";
    case 14: return "key doesn't exist";
    case 15: return "retry update";
    case 16: return "experience content rating too high";
    case 17: return "not allowed to run on this land";
    case 18: return "experience permissions request timed out";
    default: return "unknown error";   // 10 and every number that is no code
  }
}

} // namespace lsl
