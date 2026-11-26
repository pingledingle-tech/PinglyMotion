/*
 * PinglyMotion Script Lab - the weight of a setup (queue 1, item 5, part b). See labweight.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "labweight.h"

#include <QObject>
#include <QRegularExpression>

namespace
{
  qint64 number(const QJsonValue& v) { return v.isDouble() ? qint64(v.toDouble()) : 0; }
}

bool LabWeight::Script::operator==(const Script& other) const
{
  return name == other.name && limit == other.limit && used == other.used && sent == other.sent
         && received == other.received && unused == other.unused && ignored == other.ignored && stopped == other.stopped;
}

qint64 LabWeight::Weight::reserved() const
{
  qint64 sum = 0;
  for (const Script& s : scripts) sum += s.limit;
  return sum;
}

int LabWeight::Weight::limited() const
{
  int n = 0;
  for (const Script& s : scripts) n += s.limit < SCRIPT_MEMORY ? 1 : 0;
  return n;
}

qint64 LabWeight::Weight::sent() const
{
  qint64 sum = 0;
  for (const Script& s : scripts) sum += s.sent;
  return sum;
}

qint64 LabWeight::Weight::received() const
{
  qint64 sum = 0;
  for (const Script& s : scripts) sum += s.received;
  return sum;
}

qint64 LabWeight::Weight::unused() const
{
  qint64 sum = 0;
  for (const Script& s : scripts) sum += s.unused;
  return sum;
}

LabWeight::Weight LabWeight::measure(const LabFolder::Linkset& linkset, const QJsonArray& objects)
{
  Weight out;
  QJsonArray scripts;
  QStringList held;   // the scripts still in the object's inventory: one that removed itself keeps nothing
  for (const QJsonValue& v : objects)
  {
    const QJsonObject o = v.toObject();
    if (o.value("name").toString() != linkset.name || !o.contains("scripts")) continue;
    scripts = o.value("scripts").toArray();
    for (const QJsonValue& item : o.value("inventory").toArray())
      if (item.toObject().value("type").toInt() == 10) held << item.toObject().value("name").toString();
    break;
  }
  if (scripts.isEmpty()) return out;
  for (const LabFolder::Script& file : linkset.scripts)
  {
    QJsonObject found;
    for (const QJsonValue& v : scripts)
      if (v.toObject().value("name").toString() == file.name) { found = v.toObject(); break; }
    if (found.isEmpty() || !held.contains(file.name)) continue;   // removed itself: it keeps nothing
    Script s;
    s.name = file.name;
    s.prim = linkset.primName(file.link);
    s.file = file.file;
    s.limit = found.contains("memory_limit") ? qBound<qint64>(0, number(found.value("memory_limit")), SCRIPT_MEMORY) : SCRIPT_MEMORY;
    s.used = qMax<qint64>(0, number(found.value("memory")));
    s.code = found.contains("memory_code") ? qMax<qint64>(0, number(found.value("memory_code"))) : -1;
    const QJsonObject m = found.value("messages").toObject();
    s.sent = qMax<qint64>(0, number(m.value("sent")));
    s.received = qMax<qint64>(0, number(m.value("received")));
    s.unused = qBound<qint64>(0, number(m.value("unused")), s.received);
    s.ignored = qMax<qint64>(0, number(m.value("ignored")));
    s.stopped = found.value("stopped").toBool();
    out.scripts.append(s);
  }
  out.valid = true;
  return out;
}

QString LabWeight::kb(qint64 bytes)
{
  const double k = double(bytes) / 1024.0;
  if (k < 10.0)
  {
    QString one = QString::number(k, 'f', 1);
    if (one.endsWith(QLatin1String(".0"))) one.chop(2);
    return QObject::tr("%1 KB").arg(one);
  }
  return QObject::tr("%1 KB").arg(qRound64(k));
}

QStringList LabWeight::summary(const Weight& weight)
{
  QStringList out;
  if (!weight.valid) return out;
  const int n = weight.count();
  const int limited = weight.limited();
  const QString scripts = n == 1 ? QObject::tr("1 script.") : QObject::tr("%1 scripts.").arg(n);
  if (limited == 0)
    out << scripts + QLatin1Char(' ') + QObject::tr("Second Life keeps 64 KB for each: %1 in all.").arg(kb(weight.reserved()));
  else
    out << scripts + QLatin1Char(' ') + QObject::tr("Second Life keeps 64 KB for each, less for a script with a smaller limit: %1 in all.")
                                          .arg(kb(weight.reserved()))
        << (limited == 1 ? QObject::tr("1 script sets a smaller limit with llSetMemoryLimit.")
                         : QObject::tr("%1 scripts set a smaller limit with llSetMemoryLimit.").arg(limited));
  out << QObject::tr("Messages: %1 sent, %2 got, %3 not used (the script that got it did nothing with it).")
           .arg(weight.sent()).arg(weight.received()).arg(weight.unused());
  return out;
}

QStringList LabWeight::compare(const Weight& before, const Weight& now)
{
  QStringList out;
  if (!before.valid || !now.valid || before == now) return out;
  if (before.reserved() != now.reserved() || before.count() != now.count())
    out << QObject::tr("Before: %1 kept for %2 scripts. Now: %3 for %4 scripts.")
             .arg(kb(before.reserved())).arg(before.count()).arg(kb(now.reserved())).arg(now.count());
  if (before.sent() != now.sent())
    out << QObject::tr("Before: %1 messages sent. Now: %2.").arg(before.sent()).arg(now.sent());
  if (before.received() != now.received() || before.unused() != now.unused())
    out << QObject::tr("Before: %1 messages got, %2 not used. Now: %3 got, %4 not used.")
             .arg(before.received()).arg(before.unused()).arg(now.received()).arg(now.unused());
  for (const Script& s : now.scripts)
  {
    const Script* old = nullptr;
    for (const Script& b : before.scripts)
      if (b.name == s.name) { old = &b; break; }
    if (!old) { out << QObject::tr("New: %1 (%2 kept).").arg(s.name, kb(s.limit)); continue; }
    QStringList parts;
    if (old->limit != s.limit) parts << QObject::tr("kept %1, now %2").arg(kb(old->limit), kb(s.limit));
    if (old->sent != s.sent) parts << QObject::tr("sent %1, now %2").arg(old->sent).arg(s.sent);
    if (old->unused != s.unused) parts << QObject::tr("did nothing with %1, now %2").arg(old->unused).arg(s.unused);
    if (!parts.isEmpty()) out << QObject::tr("%1: %2.").arg(s.name, parts.join(QStringLiteral("; ")));
  }
  for (const Script& b : before.scripts)
  {
    bool still = false;
    for (const Script& s : now.scripts) still = still || s.name == b.name;
    if (!still) out << QObject::tr("Gone: %1 (it kept %2).").arg(b.name, kb(b.limit));
  }
  return out;
}

QString LabWeight::writeLines(const Weight& weight)
{
  if (!weight.valid) return QString();
  QString out = QStringLiteral("# The weight at the end (memory kept and messages per script), for Improve a setup:\n=weight %1\n").arg(weight.count());
  for (const Script& s : weight.scripts)
    out += QStringLiteral("=script %1 %2 %3 %4 %5 %6 %7 %8\n").arg(s.limit).arg(s.used).arg(s.sent).arg(s.received).arg(s.unused)
             .arg(s.ignored).arg(s.stopped ? 1 : 0).arg(s.name);
  return out;
}

LabWeight::Weight LabWeight::parseLines(const QString& text)
{
  static const QRegularExpression scriptLine(QStringLiteral("^=script (\\d{1,6}) (\\d{1,9}) (\\d{1,12}) (\\d{1,12}) (\\d{1,12}) (\\d{1,12}) ([01]) (\\S.{0,%1})$")
                                               .arg(LabFolder::MAX_NAME - 1));
  static const QRegularExpression weightLine(QStringLiteral("^=weight (\\d{1,3})$"));
  Weight out;
  int expected = -1;
  for (const QString& raw : text.split(QLatin1Char('\n')))
  {
    const QString line = raw.endsWith(QLatin1Char('\r')) ? raw.chopped(1) : raw;
    if (!line.startsWith(QLatin1Char('='))) continue;
    QRegularExpressionMatch m = weightLine.match(line);
    if (m.hasMatch()) { expected = m.captured(1).toInt(); continue; }
    m = scriptLine.match(line);
    if (!m.hasMatch() || expected < 0 || out.scripts.size() >= qMin(expected, MAX_SCRIPTS)) continue;
    Script s;
    s.limit = qMin<qint64>(m.captured(1).toLongLong(), SCRIPT_MEMORY);
    s.used = m.captured(2).toLongLong();
    s.sent = m.captured(3).toLongLong();
    s.received = m.captured(4).toLongLong();
    s.unused = qMin(m.captured(5).toLongLong(), s.received);
    s.ignored = m.captured(6).toLongLong();
    s.stopped = m.captured(7) == QLatin1String("1");
    s.name = m.captured(8);
    out.scripts.append(s);
  }
  out.valid = expected >= 0 && expected <= MAX_SCRIPTS && out.scripts.size() == expected;
  if (!out.valid) out.scripts.clear();
  return out;
}

QJsonObject LabWeight::toJson(const Weight& weight)
{
  QJsonArray rows;
  for (const Script& s : weight.scripts)
    rows.append(QJsonObject{{"name", s.name}, {"prim", s.prim}, {"limit", double(s.limit)}, {"used", double(s.used)},
                            {"sent", double(s.sent)}, {"received", double(s.received)}, {"unused", double(s.unused)},
                            {"ignored", double(s.ignored)}, {"stopped", s.stopped}});
  return QJsonObject{{"valid", weight.valid}, {"scripts", weight.count()}, {"reserved", double(weight.reserved())},
                     {"limited", weight.limited()}, {"sent", double(weight.sent())}, {"received", double(weight.received())},
                     {"unused", double(weight.unused())}, {"rows", rows}};
}
