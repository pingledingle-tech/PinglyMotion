/*
 * PinglyMotion Script Lab - Live: watch a folder and run changed scripts (issue 292)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "lsl/control.h"
#include "labwatcher.h"

namespace
{
  const int MAX_WATCHED_FOLDERS = 256;

  bool inside(const QString& root, const QString& canonical)
  {
    return canonical == root || canonical.startsWith(root.endsWith('/') ? root : root + '/');
  }

  bool isLink(const QFileInfo& info)
  {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    if (info.isJunction()) return true;
#endif
    return info.isSymLink();
  }
}

LabWatcher::LabWatcher(QObject* parent) :
  QObject(parent),
  m_watcher(new QFileSystemWatcher(this))
{
  m_debounce.setSingleShot(true);
  m_debounce.setInterval(DEBOUNCE_MS);
  connect(&m_debounce, &QTimer::timeout, this, &LabWatcher::rescan);
  connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString&) { scheduleRescan(); });
  connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path)
  {
    // the exact file is known: run it even if its time and size look the same
    const QString relative = QDir(m_folder).relativeFilePath(path);
    if (m_snapshot.contains(relative) && !m_pending.contains(relative)) m_pending.append(relative);
    scheduleRescan();
  });
}

LabWatcher::Result LabWatcher::watch(const QString& folder, const QJsonObject& runOptions)
{
  if (folder.trimmed().isEmpty()) return {"Give the folder to watch."};
  if (folder.size() > MAX_PATH_CHARS) return {QString("The folder path is too long (at most %1 characters).").arg(MAX_PATH_CHARS)};
  if (folder.startsWith("\\\\") || folder.startsWith("//"))
    return {"Network folders cannot be watched. Use a folder on this computer."};
  if (!QDir::isAbsolutePath(folder)) return {"Give the whole path of the folder, not a relative one."};
  const QFileInfo info(folder);
  if (!info.exists()) return {"The folder does not exist."};
  if (!info.isDir()) return {"That is a file, not a folder. Give the folder that holds the .lsl scripts."};
  const QString canonical = info.canonicalFilePath();
  if (canonical.isEmpty()) return {"The folder cannot be read."};

  // seconds and inputs are checked the way lsl_run checks them: an empty
  // script never compiles, so an answer with "errors" means the options are fine
  for (auto it = runOptions.begin(); it != runOptions.end(); ++it)
    if (it.key() != QLatin1String("seconds") && it.key() != QLatin1String("inputs") && it.key() != QLatin1String("keep_rlv"))
      return {"Unknown run option \"" + it.key().left(40) + "\". Options: seconds, inputs, keep_rlv."};
  QJsonObject probe = runOptions;
  probe["command"] = "lsl_run";
  probe["source"] = QString();
  const QJsonObject answer = lsl::Control::run(probe);
  if (!answer.contains("errors") && !answer.value("ok").toBool()) return {answer.value("error").toString()};

  unwatch();
  m_folder = canonical;
  m_options = runOptions;
  m_pending.clear();
  QStringList folders;
  m_snapshot = scan(&folders);
  updateWatchedPaths(folders);
  emit watchChanged();

  // run the newest script once, so there is a result at once
  QString newest;
  for (auto it = m_snapshot.constBegin(); it != m_snapshot.constEnd(); ++it)
    if (newest.isEmpty() || it.value().modified > m_snapshot.value(newest).modified) newest = it.key();
  if (!newest.isEmpty()) runScript(newest);
  return {};
}

bool LabWatcher::unwatch()
{
  if (m_folder.isEmpty()) return false;
  m_debounce.stop();
  const QStringList paths = m_watcher->files() + m_watcher->directories();
  if (!paths.isEmpty()) m_watcher->removePaths(paths);
  m_folder.clear();
  m_options = QJsonObject();
  m_snapshot.clear();
  m_pending.clear();
  m_results.clear();
  m_last = QJsonObject();
  m_truncated = false;
  emit watchChanged();
  emit resultChanged();
  return true;
}

QString LabWatcher::absolutePath(const QString& relative) const
{
  if (m_folder.isEmpty() || !m_snapshot.contains(relative)) return QString();
  return QDir(m_folder).filePath(relative);
}

QMap<QString, LabWatcher::Entry> LabWatcher::scan(QStringList* folders)
{
  QMap<QString, Entry> found;
  m_truncated = false;
  if (m_folder.isEmpty()) return found;
  const QDir root(m_folder);
  if (!root.exists()) return found;

  struct Pending { QString path; int depth; };
  QList<Pending> queue{{m_folder, 0}};
  int entries = 0;
  while (!queue.isEmpty())
  {
    const Pending current = queue.takeFirst();
    if (folders && folders->size() < MAX_WATCHED_FOLDERS) folders->append(current.path);
    // no hidden entries: .git and the like are never read
    const QFileInfoList list = QDir(current.path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                                                QDir::Name);
    for (const QFileInfo& info : list)
    {
      if (++entries > MAX_ENTRIES)
      {
        m_truncated = true;
        return found;
      }
      // a name that starts with a dot is hidden on every system (on Windows
      // hidden is a separate flag, so .git would show up otherwise)
      if (info.fileName().startsWith(QLatin1Char('.'))) continue;
      if (info.isDir())
      {
        // a linked folder is never entered, so a link cannot lead out of the folder
        if (isLink(info) || current.depth + 1 > MAX_DEPTH) continue;
        if (!inside(m_folder, info.canonicalFilePath())) continue;
        queue.append({info.absoluteFilePath(), current.depth + 1});
        continue;
      }
      if (info.suffix().compare(QLatin1String("lsl"), Qt::CaseInsensitive) != 0) continue;
      // a linked file counts only when its target is inside the folder
      const QString target = info.canonicalFilePath();
      if (target.isEmpty() || !inside(m_folder, target)) continue;
      if (found.size() >= MAX_SCRIPTS)
      {
        m_truncated = true;
        continue;
      }
      Entry entry;
      entry.modified = info.lastModified();
      entry.size = info.size();
      found.insert(root.relativeFilePath(info.absoluteFilePath()), entry);
    }
  }
  return found;
}

void LabWatcher::updateWatchedPaths(const QStringList& folders)
{
  const QStringList old = m_watcher->files() + m_watcher->directories();
  if (!old.isEmpty()) m_watcher->removePaths(old);
  QStringList paths = folders;
  for (auto it = m_snapshot.constBegin(); it != m_snapshot.constEnd(); ++it) paths << QDir(m_folder).filePath(it.key());
  if (!paths.isEmpty()) m_watcher->addPaths(paths);
}

void LabWatcher::scheduleRescan()
{
  if (!m_folder.isEmpty()) m_debounce.start();   // restarts: several writes count as one change
}

void LabWatcher::rescan()
{
  if (m_folder.isEmpty()) return;
  QStringList folders;
  const QMap<QString, Entry> now = scan(&folders);
  for (auto it = now.constBegin(); it != now.constEnd(); ++it)
  {
    const auto before = m_snapshot.constFind(it.key());
    const bool changed = before == m_snapshot.constEnd() || before->modified != it.value().modified || before->size != it.value().size;
    if (changed && !m_pending.contains(it.key())) m_pending.append(it.key());
  }
  const bool listChanged = now.keys() != m_snapshot.keys();
  m_snapshot = now;
  // a script that was removed has no result any more
  for (const QString& gone : m_results.keys())
    if (!m_snapshot.contains(gone)) m_results.remove(gone);
  for (int i = m_pending.size() - 1; i >= 0; --i)
    if (!m_snapshot.contains(m_pending[i])) m_pending.removeAt(i);
  // an editor that saves by writing a new file and renaming it drops the old
  // file from the watch list, so the list is made again each time
  updateWatchedPaths(folders);
  if (listChanged) emit watchChanged();
  runPending();
}

void LabWatcher::runPending()
{
  int runs = 0;
  while (!m_pending.isEmpty() && runs < MAX_RUNS_PER_BATCH)
  {
    runScript(m_pending.takeFirst());
    ++runs;
  }
  // more waiting: let the program draw and answer first, then go on
  if (!m_pending.isEmpty()) QTimer::singleShot(0, this, &LabWatcher::runPending);
}

void LabWatcher::runScript(const QString& relative)
{
  const QString path = absolutePath(relative);
  if (path.isEmpty()) return;

  QJsonObject result{{"file", relative}};
  QString problem;
  QString source;
  QFile file(path);
  if (QFileInfo(path).size() > MAX_FILE_BYTES)
    problem = QString("The file is larger than %1 bytes, so it was not read.").arg(MAX_FILE_BYTES);
  else if (!file.open(QIODevice::ReadOnly))
    problem = "The file cannot be read: " + file.errorString();
  else
  {
    const QByteArray bytes = file.read(MAX_FILE_BYTES + 1);
    if (bytes.size() > MAX_FILE_BYTES)
      problem = QString("The file is larger than %1 bytes, so it was not read.").arg(MAX_FILE_BYTES);
    else
      source = QString::fromUtf8(bytes);
  }

  if (problem.isEmpty())
  {
    const QJsonObject check = lsl::Control::check(QJsonObject{{"command", "lsl_check"}, {"source", source}});
    if (!check.value("ok").toBool())
      problem = check.value("error").toString();
    else
    {
      result["valid"] = check.value("valid");
      result["errors"] = check.value("errors");
      result["warnings"] = check.value("warnings");
      if (check.value("valid").toBool())
      {
        QJsonObject request = m_options;
        request["command"] = "lsl_run";
        request["source"] = source;
        // llOwnerSay("@...") goes to the sandbox's RLV state, the one the panel and rlv_state show
        result["result"] = lsl::Control::run(request, m_rlv);
      }
    }
  }
  if (!problem.isEmpty())
  {
    result["valid"] = false;
    result["read_error"] = problem;
  }

  result["run"] = ++m_runs;
  m_results.insert(relative, result);
  m_last = result;
  emit resultChanged();
}
