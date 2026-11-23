/*
 * PinglyMotion - the reference library for the Library panel (see libraryindex.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "libraryindex.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace
{
  // the index is small (about 60 kB for 106 clips); a much larger file is not ours
  const qint64 MaxIndexBytes = 8 * 1024 * 1024;

  QStringList words(const QString& text)
  {
    static const QRegularExpression split(QStringLiteral("[^a-z0-9]+"));
    return text.toLower().split(split, Qt::SkipEmptyParts);
  }
}

bool LibraryIndex::load(const QString& folder)
{
  m_clips.clear();
  m_loaded = false;
  m_folder = folder;
  QFile file(QDir(folder).filePath(QStringLiteral("index.json")));
  if (folder.isEmpty() || file.size() > MaxIndexBytes || !file.open(QIODevice::ReadOnly)) return false;
  const QJsonDocument doc = QJsonDocument::fromJson(file.read(MaxIndexBytes));
  if (!doc.isObject()) return false;
  const QJsonObject root = doc.object();
  const QJsonObject sources = root.value(QStringLiteral("sources")).toObject();
  const QJsonArray clips = root.value(QStringLiteral("clips")).toArray();
  for (const QJsonValue& value : clips)
  {
    const QJsonObject o = value.toObject();
    LibraryClip c;
    c.id = o.value(QStringLiteral("id")).toString();
    c.file = o.value(QStringLiteral("file")).toString();
    if (c.id.isEmpty() || c.file.isEmpty()) continue;
    c.name = o.value(QStringLiteral("name")).toString(c.id);
    c.description = o.value(QStringLiteral("description")).toString();
    for (const QJsonValue& tag : o.value(QStringLiteral("tags")).toArray())
      if (tag.isString()) c.tags << tag.toString();
    c.source = o.value(QStringLiteral("source")).toString();
    c.sourceName = sources.value(c.source).toObject().value(QStringLiteral("name")).toString(c.source);
    c.licence = o.value(QStringLiteral("licence")).toString();
    c.frames = o.value(QStringLiteral("frames")).toInt();
    c.joints = o.value(QStringLiteral("joints")).toInt();
    c.seconds = o.value(QStringLiteral("duration_s")).toDouble();
    m_clips << c;
  }
  m_loaded = !m_clips.isEmpty();
  return m_loaded;
}

const LibraryClip* LibraryIndex::find(const QString& id) const
{
  for (const LibraryClip& c : m_clips)
    if (c.id == id) return &c;
  return nullptr;
}

QString LibraryIndex::absolutePath(const LibraryClip& clip) const
{
  return QDir::cleanPath(QDir(m_folder).absoluteFilePath(clip.file));
}

const LibraryClip* LibraryIndex::findByPath(const QString& path) const
{
  if (path.isEmpty() || !m_loaded) return nullptr;
  const QString wanted = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
  for (const LibraryClip& c : m_clips)
    if (absolutePath(c) == wanted) return &c;
  return nullptr;
}

bool LibraryIndex::matches(const LibraryClip& clip, const QString& text)
{
  const QStringList wanted = words(text);
  if (wanted.isEmpty()) return true;
  const QStringList have = words(clip.id + QLatin1Char(' ') + clip.name + QLatin1Char(' ') + clip.description +
                                 QLatin1Char(' ') + clip.tags.join(QLatin1Char(' ')));
  for (const QString& word : wanted)
  {
    bool inside = false;
    for (const QString& h : have)
      if (h.startsWith(word)) { inside = true; break; }
    if (!inside) return false;
  }
  return true;
}

QList<int> LibraryIndex::search(const QString& text, const QString& kind) const
{
  QList<int> hits;
  for (int i = 0; i < m_clips.size(); i++)
  {
    const LibraryClip& c = m_clips.at(i);
    if (!kind.isEmpty() && !c.tags.contains(kind)) continue;
    if (matches(c, text)) hits << i;
  }
  return hits;
}

int LibraryIndex::count(const QString& kind) const
{
  if (kind.isEmpty()) return size();
  int n = 0;
  for (const LibraryClip& c : m_clips)
    if (c.tags.contains(kind)) n++;
  return n;
}

QString LibraryIndex::rowLine(const LibraryClip& clip)
{
  return QString::number(clip.seconds, 'f', 1) + QStringLiteral(" s · ") + clip.tags.join(QStringLiteral(", "));
}

QString LibraryIndex::lengthText(const LibraryClip& clip)
{
  return LibrarySpec::tr(LibrarySpec::lengthValue).arg(QString::number(clip.seconds, 'f', 1)).arg(clip.frames);
}

QString LibraryIndex::skeletonText(const LibraryClip& clip)
{
  return LibrarySpec::tr(LibrarySpec::skeletonValue).arg(clip.joints);
}

QString LibraryIndex::fromText(const LibraryClip& clip)
{
  if (clip.source == QLatin1String("cmu")) return LibrarySpec::tr(LibrarySpec::cmuFrom);
  if (clip.source == QLatin1String("mesh2motion")) return LibrarySpec::tr(LibrarySpec::m2mFrom);
  return clip.sourceName;
}

QString LibraryIndex::licenceText(const LibraryClip& clip)
{
  if (clip.source == QLatin1String("cmu")) return LibrarySpec::tr(LibrarySpec::cmuLicence);
  if (clip.source == QLatin1String("mesh2motion")) return LibrarySpec::tr(LibrarySpec::m2mLicence);
  return clip.licence;
}

QString LibraryIndex::countText(int shown, int all)
{
  if (shown == all) return LibrarySpec::tr(LibrarySpec::countAll).arg(all);
  return LibrarySpec::tr(LibrarySpec::countSome).arg(shown).arg(all);
}
