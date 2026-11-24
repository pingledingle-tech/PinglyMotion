/*
 * PinglyMotion Script Lab - Live: watch a folder and run changed scripts (issue 292)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include "lsl/control.h"
#include "lsl/session.h"
#include "lsl/world.h"
#include "labfolder.h"
#include "labwatcher.h"
#include "rlvstate.h"

#include <QCoreApplication>

namespace
{
  const int MAX_WATCHED_FOLDERS = 256;

  bool inside(const QString& root, const QString& canonical)
  {
    return canonical == root || canonical.startsWith(root.endsWith('/') ? root : root + '/');
  }

  bool isNotecard(const QString& name)
  {
    return name.endsWith(QLatin1String(".txt"), Qt::CaseInsensitive) || name.endsWith(QLatin1String(".notecard"), Qt::CaseInsensitive);
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
  m_world(new lsl::Session(this)),
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
    const bool known = m_snapshot.contains(relative) || (m_linkset && m_notecards.contains(relative));
    if (known && !m_pending.contains(relative)) m_pending.append(relative);
    scheduleRescan();
  });
}

LabWatcher::~LabWatcher() = default;

LabWatcher::Result LabWatcher::watch(const QString& folder, const QJsonObject& runOptions)
{
  if (folder.trimmed().isEmpty()) return {tr("Give the folder to watch.")};
  if (folder.size() > MAX_PATH_CHARS) return {tr("The folder path is too long (at most %1 characters).").arg(MAX_PATH_CHARS)};
  if (folder.startsWith("\\\\") || folder.startsWith("//"))
    return {tr("Network folders cannot be watched. Use a folder on this computer.")};
  if (!QDir::isAbsolutePath(folder)) return {tr("Give the whole path of the folder, not a relative one.")};
  const QFileInfo info(folder);
  if (!info.exists()) return {tr("The folder does not exist.")};
  if (!info.isDir()) return {tr("That is a file, not a folder. Give the folder that holds the .lsl scripts.")};
  const QString canonical = info.canonicalFilePath();
  if (canonical.isEmpty()) return {tr("The folder cannot be read.")};

  // seconds and inputs are checked the way lsl_run checks them: an empty
  // script never compiles, so an answer with "errors" means the options are fine
  for (auto it = runOptions.begin(); it != runOptions.end(); ++it)
    if (it.key() != QLatin1String("seconds") && it.key() != QLatin1String("inputs") && it.key() != QLatin1String("keep_rlv")
        && it.key() != QLatin1String("linkset"))
      return {"Unknown run option \"" + it.key().left(40) + "\". Options: seconds, inputs, keep_rlv, linkset."};
  if (runOptions.contains("linkset") && !runOptions.value("linkset").isBool())
    return {"\"linkset\" must be true (the folder is one object) or false (every script on its own)."};
  const bool linkset = runOptions.value("linkset").toBool(false);
  QJsonObject probe = runOptions;
  probe.remove("linkset");
  probe["command"] = "lsl_run";
  probe["source"] = QString();
  const QJsonObject answer = lsl::Control::run(probe);
  if (!answer.contains("errors") && !answer.value("ok").toBool()) return {answer.value("error").toString()};

  unwatch();
  m_folder = canonical;
  m_options = runOptions;
  m_options.remove("linkset");
  m_linkset = linkset;
  m_pending.clear();
  QStringList folders;
  m_snapshot = scan(&folders, m_linkset ? &m_notecards : nullptr);
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
  m_linkset = false;
  m_notecards.clear();
  m_folderResult = QJsonObject();
  m_lastLinkset = LabFolder::Linkset();
  m_world->clear();
  m_recording = false;
  m_recorded.clear();
  m_replay = QJsonObject();
  emit watchChanged();
  emit recipeChanged();
  emit resultChanged();
  return true;
}

QString LabWatcher::absolutePath(const QString& relative) const
{
  if (m_folder.isEmpty() || !m_snapshot.contains(relative)) return QString();
  return QDir(m_folder).filePath(relative);
}

QMap<QString, LabWatcher::Entry> LabWatcher::scan(QStringList* folders, QMap<QString, Entry>* notecards)
{
  if (notecards) notecards->clear();
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
      const bool script = info.suffix().compare(QLatin1String("lsl"), Qt::CaseInsensitive) == 0;
      if (!script && !(notecards && isNotecard(info.fileName()))) continue;
      // a linked file counts only when its target is inside the folder
      const QString target = info.canonicalFilePath();
      if (target.isEmpty() || !inside(m_folder, target)) continue;
      if (!script)
      {
        // linkset mode: a notecard of the object, watched so that saving it runs the folder again
        if (notecards->size() < MAX_SCRIPTS) notecards->insert(root.relativeFilePath(info.absoluteFilePath()), Entry{info.lastModified(), info.size()});
        continue;
      }
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
  for (auto it = m_notecards.constBegin(); it != m_notecards.constEnd(); ++it) paths << QDir(m_folder).filePath(it.key());
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
  QMap<QString, Entry> notecards;
  const QMap<QString, Entry> now = scan(&folders, m_linkset ? &notecards : nullptr);
  // linkset mode: a notecard that was saved, added or removed runs the folder again too
  QString notecardChanged;
  if (m_linkset)
  {
    for (auto it = notecards.constBegin(); it != notecards.constEnd(); ++it)
    {
      const auto before = m_notecards.constFind(it.key());
      if (before == m_notecards.constEnd() || before->modified != it.value().modified || before->size != it.value().size) notecardChanged = it.key();
    }
    if (notecardChanged.isEmpty() && notecards.keys() != m_notecards.keys()) notecardChanged = QStringLiteral(".");
    m_notecards = notecards;
  }
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
    if (!m_snapshot.contains(m_pending[i]) && !(m_linkset && m_notecards.contains(m_pending[i]))) m_pending.removeAt(i);
  // an editor that saves by writing a new file and renaming it drops the old
  // file from the watch list, so the list is made again each time
  updateWatchedPaths(folders);
  if (listChanged) emit watchChanged();
  if (m_linkset)
  {
    // one run of the whole folder for every change, however many files changed
    if (m_pending.isEmpty() && notecardChanged.isEmpty() && !listChanged) return;
    const QString trigger = !m_pending.isEmpty() ? m_pending.first() : (notecardChanged == QLatin1String(".") ? QString() : notecardChanged);
    m_pending.clear();
    runFolder(trigger);
    return;
  }
  runPending();
}

void LabWatcher::runPending()
{
  if (m_linkset)
  {
    if (m_pending.isEmpty()) return;
    const QString trigger = m_pending.first();
    m_pending.clear();
    runFolder(trigger);
    return;
  }
  int runs = 0;
  while (!m_pending.isEmpty() && runs < MAX_RUNS_PER_BATCH)
  {
    runScript(m_pending.takeFirst());
    ++runs;
  }
  // more waiting: let the program draw and answer first, then go on
  if (!m_pending.isEmpty()) QTimer::singleShot(0, this, &LabWatcher::runPending);
}

void LabWatcher::runFolder(const QString& trigger)
{
  if (m_folder.isEmpty()) return;
  const LabFolder::Linkset linkset = LabFolder::open(m_folder);
  // the world is kept for touch() (job A1); it needs an RLV state that outlives it
  RlvState* rlv = m_rlv;
  if (!rlv)
  {
    if (!m_ownRlv) m_ownRlv.reset(new RlvState);
    rlv = m_ownRlv.get();
  }
  QJsonObject folder = LabFolder::run(linkset, m_options, rlv, m_world);
  if (!folder.value("ok").toBool()) m_world->clear();
  m_lastLinkset = linkset;
  const int run = ++m_runs;
  folder["run"] = run;
  folder["file"] = trigger;
  m_folderResult = folder;

  // every script's own check, and the shared run (or why the folder did not run)
  QMap<QString, QJsonObject> checks;
  for (const QJsonValue& v : folder.value("check").toObject().value("scripts").toArray()) checks.insert(v.toObject().value("file").toString(), v.toObject());
  QMap<QString, QString> left;   // a script that is not part of the object, and why
  for (const LabFolder::Problem& p : linkset.problems)
    if (!p.file.isEmpty() && !left.contains(p.file)) left.insert(p.file, p.message);
  for (const LabFolder::Problem& p : linkset.notes)
    if (!p.file.isEmpty() && !left.contains(p.file)) left.insert(p.file, p.message);
  const QJsonArray problems = folder.value("linkset").toObject().value("problems").toArray();
  m_results.clear();
  for (auto it = m_snapshot.constBegin(); it != m_snapshot.constEnd(); ++it)
  {
    const QString& file = it.key();
    QJsonObject result{{"file", file}, {"linkset", true}, {"run", run}};
    const auto check = checks.constFind(file);
    if (check != checks.constEnd())
    {
      result["script"] = check->value("script");
      result["prim"] = check->value("prim");
      result["link"] = check->value("link");
      result["valid"] = check->value("valid");
      result["errors"] = check->value("errors");
      result["warnings"] = check->value("warnings");
    }
    else
    {
      // inside a folder that is not a prim (nested, linked), or a file that broke a limit
      QString why = left.value(file);
      if (why.isEmpty())
        for (auto l = left.constBegin(); l != left.constEnd() && why.isEmpty(); ++l)
          if (file.startsWith(l.key() + '/')) why = l.value();
      if (why.isEmpty()) why = QStringLiteral("The script is not part of the object.");
      result["valid"] = false;
      result["read_error"] = why;
    }
    if (folder.value("ok").toBool())
      result["result"] = folder.value("result");
    else
    {
      result["not_run"] = folder.value("error");
      if (!problems.isEmpty()) result["problems"] = problems;
    }
    m_results.insert(file, result);
  }
  m_last = m_results.value(trigger);
  if (m_last.isEmpty() && !m_results.isEmpty()) m_last = m_results.first();
  emit resultChanged();
  // job A3: every run of the folder (a save) replays the recipe, unless the user is recording one
  if (!m_recording && hasRecipe()) replayRecipe();
}

void LabWatcher::runScript(const QString& relative)
{
  const QString path = absolutePath(relative);
  if (path.isEmpty()) return;
  if (m_linkset)
  {
    runFolder(relative);
    return;
  }

  QJsonObject result{{"file", relative}};
  QString problem;
  QString source;
  QFile file(path);
  if (QFileInfo(path).size() > MAX_FILE_BYTES)
    problem = QCoreApplication::translate("LabWatcher","The file is larger than %1 bytes, so it was not read.").arg(MAX_FILE_BYTES);
  else if (!file.open(QIODevice::ReadOnly))
    problem = QCoreApplication::translate("LabWatcher","The file cannot be read: %1").arg(file.errorString());
  else
  {
    const QByteArray bytes = file.read(MAX_FILE_BYTES + 1);
    if (bytes.size() > MAX_FILE_BYTES)
      problem = QCoreApplication::translate("LabWatcher","The file is larger than %1 bytes, so it was not read.").arg(MAX_FILE_BYTES);
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

int LabWatcher::linkOf(const QString& prim) const
{
  if (prim.isEmpty()) return 0;
  for (const LabFolder::Prim& p : m_lastLinkset.prims)
    if (p.name == prim) return p.link;
  return 0;
}

LabWatcher::Result LabWatcher::touch(int link)
{
  if (!watching() || !m_linkset) return {tr("No folder is watched as one object.")};
  if (!m_folderResult.value("ok").toBool() || !m_world->sandbox())
    return {tr("The object did not run, so it cannot be touched. Fix it and save.")};
  const QString prim = m_lastLinkset.primName(link);
  if (link < 1 || prim.isEmpty()) return {tr("The object has no prim with link %1.").arg(link)};
  QJsonObject request{{"command", "lsl_event"}, {"type", "touch"}};
  // a single prim is link 0 in the sandbox; the touch goes to it without a link
  if (m_lastLinkset.prims.size() > 1) request["link"] = link;
  const Result sent = sendEvent(request, QJsonObject{{"type", "clicked"}, {"link", link}, {"prim", prim}, {"source", "sandbox"}});
  if (sent.ok())
  {
    LabRecipe::Step step;
    step.kind = LabRecipe::Step::Kind::Touch;
    step.text = prim;
    recordStep(step);
  }
  return sent;
}

QJsonObject LabWatcher::dialog() const
{
  if (!watching() || !m_linkset || !m_folderResult.value("ok").toBool() || !m_world->sandbox()) return QJsonObject();
  return m_world->sandbox()->lastDialog();
}

QJsonObject LabWatcher::dialogAnswer(const QJsonObject& dialog, const QString& button)
{
  return QJsonObject{{"type", "dialog"}, {"object", dialog.value("object").toString()}, {"button", button}};
}

LabWatcher::Result LabWatcher::answerDialog(const QString& button)
{
  if (!watching() || !m_linkset) return {tr("No folder is watched as one object.")};
  const QJsonObject shown = dialog();
  if (shown.isEmpty()) return {tr("No menu is open. Click a part that shows one first.")};
  QJsonObject request = dialogAnswer(shown, button);
  request["command"] = QStringLiteral("lsl_event");
  const Result sent = sendEvent(request, QJsonObject{{"type", "clicked"}, {"button", button}, {"channel", shown.value("channel").toInt()},
                                                      {"source", "sandbox"}});
  if (sent.ok())
  {
    LabRecipe::Step step;
    step.kind = LabRecipe::Step::Kind::Menu;
    step.text = button;
    recordStep(step);
  }
  return sent;
}

LabWatcher::Result LabWatcher::sendEvent(const QJsonObject& request, QJsonObject clicked)
{
  const double at = std::round(m_world->sandbox()->now() * 1000.0) / 1000.0;
  QJsonObject answer = lsl::Control::event(request, m_world);
  if (!answer.value("ok").toBool()) return {answer.value("error").toString()};
  LabFolder::tagLog(m_lastLinkset, answer);

  QJsonObject run = m_folderResult.value("result").toObject();
  QJsonArray log = run.value("log").toArray();
  bool truncated = run.value("log_truncated").toBool() || answer.value("log_truncated").toBool();
  clicked["t"] = at;
  QJsonArray added{clicked};
  for (const QJsonValue& v : answer.value("log").toArray()) added.append(v);
  for (const QJsonValue& v : added)
  {
    // the log of one run keeps as many lines as the sandbox does
    if (log.size() >= lsl::World::MAX_LOG_ENTRIES) { truncated = true; break; }
    log.append(v);
  }
  run["log"] = log;
  run["log_truncated"] = truncated;
  for (const char* key : {"stopped", "time", "objects", "rlv"})
    if (answer.contains(key)) run[key] = answer.value(key);
  if (answer.contains("dialog")) run["dialog"] = answer.value("dialog");
  m_folderResult["result"] = run;
  for (auto it = m_results.begin(); it != m_results.end(); ++it)
    if (it->contains("result")) (*it)["result"] = run;
  if (m_last.contains("result")) m_last["result"] = run;
  emit resultChanged();
  return {};
}

// --- record and replay (job A3)

namespace
{
  // a file at the top of the folder that may be read and written: not a link, not a folder
  bool usableFile(const QString& path, bool mustExist)
  {
    const QFileInfo info(path);
    if (isLink(info)) return false;
    if (!info.exists()) return !mustExist;
    return info.isFile();
  }

  bool writeText(const QString& path, const QString& text)
  {
    if (!usableFile(path, false)) return false;
    QSaveFile file(path);
    // plain "\n" line ends on every system, so the files are the same everywhere (the reader takes "\r\n" too)
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(text.toUtf8());
    return file.commit();
  }

  // the text of a file of at most LabRecipe::MAX_FILE_BYTES; null when it cannot be read
  QString readText(const QString& path, QString* why)
  {
    if (!usableFile(path, true))
    {
      *why = QCoreApplication::translate("LabWatcher", "There is no recipe yet. Record what you do first.");
      return QString();
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
      *why = QCoreApplication::translate("LabWatcher", "The recipe cannot be read: %1").arg(file.errorString());
      return QString();
    }
    const QByteArray bytes = file.read(LabRecipe::MAX_FILE_BYTES + 1);
    if (bytes.size() > LabRecipe::MAX_FILE_BYTES)
    {
      *why = QCoreApplication::translate("LabWatcher", "The recipe is larger than %1 bytes.").arg(LabRecipe::MAX_FILE_BYTES);
      return QString();
    }
    return QString::fromUtf8(bytes);
  }
}

QString LabWatcher::recipePath() const
{
  if (!watching() || !m_linkset) return QString();
  return QDir(m_folder).filePath(LabRecipe::recipeFile());
}

bool LabWatcher::hasRecipe() const
{
  const QString path = recipePath();
  return !path.isEmpty() && usableFile(path, true);
}

LabWatcher::Result LabWatcher::startRecording()
{
  if (!watching() || !m_linkset) return {tr("No folder is watched as one object.")};
  m_recording = true;
  m_recorded.clear();
  emit recipeChanged();
  return {};
}

bool LabWatcher::stopRecording()
{
  if (!m_recording) return false;
  m_recording = false;
  if (!m_recorded.isEmpty()) replayRecipe(true);
  emit recipeChanged();
  return true;
}

void LabWatcher::recordLine(const QString& object, const QString& line)
{
  if (!m_recording || !line.trimmed().startsWith('@') || !RlvState::validObjectId(object)) return;
  if (line.size() > LabRecipe::MAX_LINE_CHARS - 5 || line.contains('\n') || line.contains('\r')) return;
  LabRecipe::Step step;
  step.kind = LabRecipe::Step::Kind::Send;
  step.text = line.trimmed();
  step.object = object;
  recordStep(step);
}

void LabWatcher::recordStep(const LabRecipe::Step& step)
{
  if (!m_recording || m_recorded.size() >= LabRecipe::MAX_STEPS) return;
  // a name or text the file cannot hold on one line is left out
  if (step.text.contains('\n') || step.text.contains('\r') || step.text.trimmed() != step.text || step.text.isEmpty()) return;
  m_recorded.append(step);
  LabRecipe::Recipe recipe;
  recipe.steps = m_recorded;
  writeText(recipePath(), LabRecipe::write(recipe));
  emit recipeChanged();
}

LabWatcher::Result LabWatcher::replayRecipe(bool first)
{
  if (!watching() || !m_linkset) return {tr("No folder is watched as one object.")};
  QString why;
  const QString text = readText(recipePath(), &why);
  if (!why.isEmpty()) return {why};
  const LabRecipe::Result parsed = LabRecipe::parse(text);
  if (!parsed.ok()) return {tr("The recipe has a mistake. %1").arg(parsed.error)};
  if (parsed.value->steps.isEmpty()) return {tr("The recipe has no steps. Record what you do first.")};

  const LabFolder::Linkset linkset = m_lastLinkset.name.isEmpty() ? LabFolder::open(m_folder) : m_lastLinkset;
  const Rlv::Mode mode = m_rlv ? m_rlv->mode() : Rlv::Mode::Rlva;
  const LabRecipe::Replay replay = LabRecipe::replay(linkset, m_options, *parsed.value, mode);

  // compare with the facts kept from last time, then keep the new ones
  const QString resultPath = QDir(m_folder).filePath(LabRecipe::resultFile());
  QString ignored;
  const QString before = first ? QString() : readText(resultPath, &ignored);
  const bool nothingBefore = first || !ignored.isEmpty();
  const QStringList changes = nothingBefore ? QStringList() : LabRecipe::compare(LabRecipe::parseResult(before), replay.facts);
  QJsonObject out = LabRecipe::toJson(replay, changes, nothingBefore);
  out["run"] = m_runs;
  out["recipe"] = LabRecipe::recipeFile();
  QStringList lines;
  for (const LabRecipe::Step& s : parsed.value->steps) lines << LabRecipe::stepLine(s);
  out["steps_text"] = QJsonArray::fromStringList(lines);
  if (!writeText(resultPath, LabRecipe::writeResult(replay, changes, nothingBefore)))
    out["note"] = tr("The result could not be kept in %1.").arg(LabRecipe::resultFile());
  m_replay = out;
  emit recipeChanged();
  return {};
}
