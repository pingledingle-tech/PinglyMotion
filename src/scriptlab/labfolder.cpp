/*
 * PinglyMotion Script Lab - the folder is the object (lane 1, job B)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>

#include "lsl/control.h"
#include "lsl/lexer.h"
#include "lsl/sandbox.h"
#include "lsl/world.h"
#include "labfolder.h"

static_assert(LabFolder::MAX_SCRIPTS == lsl::Sandbox::MAX_SCRIPTS_PER_OBJECT, "one folder is one object of the sandbox");
static_assert(LabFolder::MAX_PRIMS == lsl::Sandbox::MAX_PRIMS, "a folder has at most as many prims as a linkset");
static_assert(LabFolder::MAX_NOTECARDS == lsl::Inventory::MAX_ITEMS - lsl::Sandbox::MAX_SCRIPTS_PER_OBJECT, "notecards and scripts share the inventory");
static_assert(LabFolder::MAX_FILE_BYTES == lsl::Lexer::MAX_SOURCE_CHARS, "a script file is at most one script");
static_assert(LabFolder::MAX_TOTAL_BYTES == lsl::Control::MAX_TOTAL_SOURCE_CHARS, "all scripts of one run");

namespace
{
  // all notecard lines of one object together (as lsl_run's "inventory")
  const qint64 MAX_NOTECARD_TOTAL = 131072;

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

  bool validName(const QString& name)
  {
    if (name.trimmed().isEmpty() || name.size() > LabFolder::MAX_NAME) return false;
    for (const QChar c : name)
      if (c.category() == QChar::Other_Control) return false;
    return true;
  }

  bool isScript(const QString& file) { return file.endsWith(QLatin1String(".lsl"), Qt::CaseInsensitive); }

  bool isNotecard(const QString& file)
  {
    return file.endsWith(QLatin1String(".txt"), Qt::CaseInsensitive) || file.endsWith(QLatin1String(".notecard"), Qt::CaseInsensitive);
  }

  QString withoutSuffix(const QString& file)
  {
    const int dot = file.lastIndexOf('.');
    return dot > 0 ? file.left(dot) : file;
  }

  // a file of a prim found while scanning, read later
  struct Found
  {
    QString absolute;
    QString relative;
    int link = 1;
    bool script = true;
  };

  QString quoted(const QString& text) { return "\"" + text + "\""; }
}

QJsonObject LabFolder::Problem::toJson() const
{
  QJsonObject o{{"message", message}};
  if (!file.isEmpty()) o["file"] = file;
  if (!prim.isEmpty()) o["prim"] = prim;
  if (link > 0) o["link"] = link;
  return o;
}

QString LabFolder::Linkset::primName(int link) const
{
  for (const Prim& p : prims)
    if (p.link == link) return p.name;
  return QString();
}

QJsonObject LabFolder::Linkset::toJson() const
{
  QJsonArray primList;
  for (const Prim& p : prims)
  {
    QJsonArray scriptList;
    for (const Script& s : scripts)
      if (s.link == p.link) scriptList.append(QJsonObject{{"name", s.name}, {"file", s.file}});
    QJsonArray notecardList;
    for (const Notecard& n : notecards)
      if (n.link == p.link) notecardList.append(QJsonObject{{"name", n.name}, {"file", n.file}, {"lines", int(n.lines.size())}});
    primList.append(QJsonObject{{"link", p.link}, {"name", p.name}, {"folder", p.folder}, {"scripts", scriptList}, {"notecards", notecardList}});
  }
  QJsonArray problemList;
  for (const Problem& p : problems) problemList.append(p.toJson());
  QJsonArray noteList;
  for (const Problem& p : notes) noteList.append(p.toJson());
  QJsonObject o{{"object", name}, {"folder", folder}, {"prims", primList}, {"problems", problemList}, {"notes", noteList}};
  if (truncated) o["truncated"] = true;
  return o;
}

QString LabFolder::pathError(const QString& folder, QString* canonical)
{
  if (folder.trimmed().isEmpty()) return "Give the folder to watch.";
  if (folder.size() > MAX_PATH_CHARS) return QString("The folder path is too long (at most %1 characters).").arg(MAX_PATH_CHARS);
  if (folder.startsWith("\\\\") || folder.startsWith("//")) return "Network folders cannot be watched. Use a folder on this computer.";
  if (!QDir::isAbsolutePath(folder)) return "Give the whole path of the folder, not a relative one.";
  const QFileInfo info(folder);
  if (!info.exists()) return "The folder does not exist.";
  if (!info.isDir()) return "That is a file, not a folder. Give the folder that holds the .lsl scripts.";
  const QString path = info.canonicalFilePath();
  if (path.isEmpty()) return "The folder cannot be read.";
  if (canonical) *canonical = path;
  return QString();
}

LabFolder::Linkset LabFolder::open(const QString& folder)
{
  Linkset ls;
  QString canonical;
  ls.error = pathError(folder, &canonical);
  if (!ls.error.isEmpty()) return ls;
  ls.folder = canonical;
  const QDir root(canonical);
  ls.name = QFileInfo(canonical).fileName();
  if (!validName(ls.name))
    ls.problems.push_back({QString("The folder's name is the object's name; it must be 1 to %1 characters. Rename the folder.").arg(MAX_NAME), QString(), QString(), 0});

  int entries = 0;
  auto list = [&](const QString& path) -> QFileInfoList
  {
    QFileInfoList out;
    for (const QFileInfo& info : QDir(path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase))
    {
      if (++entries > MAX_ENTRIES)
      {
        ls.truncated = true;
        break;
      }
      // a name that starts with a dot is hidden on every system (.git)
      if (info.fileName().startsWith(QLatin1Char('.'))) continue;
      out << info;
    }
    return out;
  };
  auto relative = [&](const QFileInfo& info) { return root.relativeFilePath(info.absoluteFilePath()); };

  // the top folder: its files belong to the root prim, its subfolders are prims
  std::vector<Found> files;
  QFileInfoList primFolders;
  for (const QFileInfo& info : list(canonical))
  {
    if (info.isDir())
    {
      if (isLink(info) || !inside(canonical, info.canonicalFilePath()))
      {
        ls.notes.push_back({quoted(relative(info)) + " is a link to a folder; links are not followed, so it is not a prim.", relative(info), QString(), 0});
        continue;
      }
      primFolders << info;
      continue;
    }
    if (!isScript(info.fileName()) && !isNotecard(info.fileName())) continue;
    const QString target = info.canonicalFilePath();
    if (target.isEmpty() || !inside(canonical, target))
    {
      ls.notes.push_back({quoted(relative(info)) + " is a link that leads out of the folder, so it was not read.", relative(info), QString(), 0});
      continue;
    }
    files.push_back({info.absoluteFilePath(), relative(info), 1, isScript(info.fileName())});
  }

  // the root prim: a subfolder named "Root" (any case) joins the top folder's own files
  Prim rootPrim{1, ls.name, QString()};
  int rootIndex = -1;
  for (int i = 0; i < primFolders.size(); ++i)
    if (primFolders[i].fileName().compare(QLatin1String("root"), Qt::CaseInsensitive) == 0)
    {
      rootIndex = i;
      rootPrim.name = primFolders[i].fileName();
      rootPrim.folder = relative(primFolders[i]);
      break;
    }
  ls.prims.push_back(rootPrim);
  QFileInfoList order;
  if (rootIndex >= 0) order << primFolders[rootIndex];
  for (int i = 0; i < primFolders.size(); ++i)
    if (i != rootIndex) order << primFolders[i];

  for (int i = 0; i < order.size(); ++i)
  {
    const QFileInfo& dir = order[i];
    const bool isRoot = i == 0 && rootIndex >= 0;
    int link = 1;
    if (!isRoot)
    {
      link = int(ls.prims.size()) + 1;
      if (link > MAX_PRIMS)
      {
        ls.problems.push_back({QString("The object has more than %1 prims (one per subfolder, plus the root); a Second Life linkset has at most %1.").arg(MAX_PRIMS), QString(), QString(), 0});
        break;
      }
      ls.prims.push_back({link, dir.fileName(), relative(dir)});
      if (!validName(dir.fileName()))
        ls.problems.push_back({QString("A prim's name is its folder's name; it must be 1 to %1 characters. Rename the folder.").arg(MAX_NAME), relative(dir), dir.fileName(), link});
    }
    for (const QFileInfo& info : list(dir.absoluteFilePath()))
    {
      if (info.isDir())
      {
        ls.notes.push_back({quoted(relative(info)) + " is a folder inside a prim's folder. Prims are not nested, so it was not read.", relative(info), ls.prims.back().name, link});
        continue;
      }
      if (!isScript(info.fileName()) && !isNotecard(info.fileName())) continue;
      const QString target = info.canonicalFilePath();
      if (target.isEmpty() || !inside(canonical, target))
      {
        ls.notes.push_back({quoted(relative(info)) + " is a link that leads out of the folder, so it was not read.", relative(info), ls.prims.back().name, link});
        continue;
      }
      files.push_back({info.absoluteFilePath(), relative(info), link, isScript(info.fileName())});
    }
  }
  if (ls.truncated)
    ls.problems.push_back({QString("The folder has more than %1 entries, so it was not read to the end. Open the folder of one object.").arg(MAX_ENTRIES), QString(), QString(), 0});

  // read the files in link order, then by path
  std::stable_sort(files.begin(), files.end(), [](const Found& a, const Found& b) { return a.link < b.link; });
  qint64 total = 0;
  qint64 notecardChars = 0;
  bool totalReported = false;
  int scriptCount = 0;
  int notecardCount = 0;
  QHash<QString, QString> names;   // a name in the object's inventory -> the file that has it
  for (const Found& f : files)
  {
    const QString prim = ls.primName(f.link);
    auto problem = [&](const QString& message) { ls.problems.push_back({message, f.relative, prim, f.link}); };
    if (f.relative.size() > MAX_RELATIVE_PATH)
    {
      problem(QString("The path inside the folder is longer than %1 characters. Use shorter names.").arg(MAX_RELATIVE_PATH));
      continue;
    }
    const QString name = withoutSuffix(QFileInfo(f.relative).fileName());
    if (!validName(name))
    {
      problem(QString("The name (the file name without its extension) must be 1 to %1 characters.").arg(MAX_NAME));
      continue;
    }
    if (f.script && ++scriptCount == MAX_SCRIPTS + 1)
      ls.problems.push_back({QString("The object has more than %1 scripts; the sandbox runs at most %1 in one object.").arg(MAX_SCRIPTS), QString(), QString(), 0});
    if (!f.script && ++notecardCount == MAX_NOTECARDS + 1)
      ls.problems.push_back({QString("The object has more than %1 notecards.").arg(MAX_NOTECARDS), QString(), QString(), 0});
    if (names.contains(name))
    {
      problem("The name " + quoted(name) + " is used twice (" + names.value(name) + " and " + f.relative +
              "). The sandbox keeps one inventory per object, so every script and notecard needs its own name. Rename one.");
      continue;
    }
    names.insert(name, f.relative);

    QFile file(f.absolute);
    if (QFileInfo(f.absolute).size() > MAX_FILE_BYTES)
    {
      problem(QString("The file is larger than %1 bytes, so it was not read.").arg(MAX_FILE_BYTES));
      continue;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
      problem("The file cannot be read: " + file.errorString());
      continue;
    }
    const QByteArray bytes = file.read(MAX_FILE_BYTES + 1);
    if (bytes.size() > MAX_FILE_BYTES)
    {
      problem(QString("The file is larger than %1 bytes, so it was not read.").arg(MAX_FILE_BYTES));
      continue;
    }
    total += bytes.size();
    if (total > MAX_TOTAL_BYTES && !totalReported)
    {
      totalReported = true;
      ls.problems.push_back({QString("The scripts and notecards together are larger than %1 bytes.").arg(MAX_TOTAL_BYTES), QString(), QString(), 0});
    }
    const QString text = QString::fromUtf8(bytes);
    if (f.script)
    {
      ls.scripts.push_back({name, f.relative, f.link, text});
      continue;
    }
    Notecard card{name, f.relative, f.link, QStringList()};
    QString body = text;
    body.remove(QLatin1Char('\r'));
    if (body.endsWith(QLatin1Char('\n'))) body.chop(1);
    if (!body.isEmpty()) card.lines = body.split(QLatin1Char('\n'));
    if (card.lines.size() > lsl::Inventory::MAX_NOTECARD_LINES)
    {
      problem(QString("A notecard has at most %1 lines.").arg(lsl::Inventory::MAX_NOTECARD_LINES));
      continue;
    }
    qint64 chars = 0;
    for (const QString& line : card.lines) chars += line.size();
    if (chars > lsl::Inventory::MAX_NOTECARD_CHARS)
    {
      problem(QString("A notecard has at most %1 characters.").arg(lsl::Inventory::MAX_NOTECARD_CHARS));
      continue;
    }
    notecardChars += chars;
    if (notecardChars > MAX_NOTECARD_TOTAL && notecardChars - chars <= MAX_NOTECARD_TOTAL)
      ls.problems.push_back({QString("The notecards together have more than %1 characters.").arg(MAX_NOTECARD_TOTAL), QString(), QString(), 0});
    ls.notecards.push_back(card);
  }
  return ls;
}

QJsonObject LabFolder::check(const Linkset& linkset)
{
  QJsonArray scripts;
  QJsonArray errors;
  QJsonArray warnings;
  bool valid = true;
  for (const Script& s : linkset.scripts)
  {
    const QString prim = linkset.primName(s.link);
    const QJsonObject answer = lsl::Control::check(QJsonObject{{"command", "lsl_check"}, {"source", s.source}});
    auto tagged = [&](const QJsonArray& in, QJsonArray& all)
    {
      QJsonArray out;
      for (const QJsonValue& v : in)
      {
        QJsonObject d = v.toObject();
        d["file"] = s.file;
        d["script"] = s.name;
        d["prim"] = prim;
        d["link"] = s.link;
        out.append(d);
        all.append(d);
      }
      return out;
    };
    QJsonObject entry{{"file", s.file}, {"script", s.name}, {"prim", prim}, {"link", s.link}};
    if (!answer.value("ok").toBool())
    {
      // the source is refused before it is checked (too long): one error at line 1
      QJsonArray one{QJsonObject{{"line", 1}, {"column", 1}, {"message", answer.value("error").toString()}}};
      entry["valid"] = false;
      entry["errors"] = tagged(one, errors);
      entry["warnings"] = QJsonArray();
      valid = false;
    }
    else
    {
      entry["valid"] = answer.value("valid").toBool();
      entry["errors"] = tagged(answer.value("errors").toArray(), errors);
      entry["warnings"] = tagged(answer.value("warnings").toArray(), warnings);
      if (!answer.value("valid").toBool()) valid = false;
    }
    scripts.append(entry);
  }
  return QJsonObject{{"valid", valid}, {"scripts", scripts}, {"errors", errors}, {"warnings", warnings}};
}

int LabFolder::autoSeconds(int scripts)
{
  return qBound(int(AUTO_BASE_SECONDS), AUTO_BASE_SECONDS + AUTO_PER_SCRIPT_SECONDS * qMax(0, scripts), int(AUTO_MAX_SECONDS));
}

QJsonObject LabFolder::request(const Linkset& linkset, const QJsonObject& options)
{
  QJsonArray scripts;
  for (const Script& s : linkset.scripts) scripts.append(QJsonObject{{"name", s.name}, {"source", s.source}, {"link", s.link}});
  QJsonObject object{{"name", linkset.name}, {"prims", int(linkset.prims.size())}, {"scripts", scripts}};
  if (!linkset.notecards.empty())
  {
    QJsonArray inventory;
    for (const Notecard& n : linkset.notecards)
      inventory.append(QJsonObject{{"name", n.name}, {"type", "notecard"}, {"lines", QJsonArray::fromStringList(n.lines)}});
    object["inventory"] = inventory;
  }
  QJsonObject req;
  for (const char* key : {"seconds", "inputs", "keep_rlv", "script_memory"})
    if (options.contains(key)) req[key] = options.value(key);
  if (!req.contains("seconds")) req["seconds"] = autoSeconds(int(linkset.scripts.size()));
  req["command"] = "lsl_run";
  req["objects"] = QJsonArray{object};
  return req;
}

void LabFolder::tagLog(const Linkset& linkset, QJsonObject& answer)
{
  QHash<QString, const Script*> byName;
  for (const Script& s : linkset.scripts) byName.insert(s.name, &s);
  const QString rootName = linkset.primName(1);
  QJsonArray log = answer.value("log").toArray();
  for (int i = 0; i < log.size(); ++i)
  {
    QJsonObject e = log[i].toObject();
    const Script* s = e.contains("script") ? byName.value(e.value("script").toString(), nullptr) : nullptr;
    if (s)
    {
      e["prim"] = linkset.primName(s->link);
      e["file"] = s->file;
    }
    else
    {
      // an input or the sandbox itself: the prim it went to (an input's "link"), else the root
      const int link = e.value("link").toInt(0);
      const QString prim = linkset.primName(link);
      e["prim"] = prim.isEmpty() ? rootName : prim;
      e["source"] = "sandbox";
    }
    log[i] = e;
  }
  answer["log"] = log;

  // the scripts in "objects" get their file and prim too
  QJsonArray objects = answer.value("objects").toArray();
  for (int i = 0; i < objects.size(); ++i)
  {
    QJsonObject o = objects[i].toObject();
    QJsonArray list = o.value("scripts").toArray();
    for (int k = 0; k < list.size(); ++k)
    {
      QJsonObject s = list[k].toObject();
      if (const Script* script = byName.value(s.value("name").toString(), nullptr))
      {
        s["file"] = script->file;
        s["prim"] = linkset.primName(script->link);
      }
      list[k] = s;
    }
    o["scripts"] = list;
    objects[i] = o;
  }
  answer["objects"] = objects;
}

QJsonObject LabFolder::run(const Linkset& linkset, const QJsonObject& options, RlvState* rlv, lsl::Session* session)
{
  QJsonObject out{{"linkset", linkset.toJson()}};
  if (!linkset.error.isEmpty())
  {
    out["ok"] = false;
    out["error"] = linkset.error;
    return out;
  }
  const QJsonObject checked = check(linkset);
  out["check"] = checked;
  if (!linkset.problems.empty())
  {
    out["ok"] = false;
    out["error"] = QString("The folder was not run: it breaks %1 limit(s) or rule(s), listed in \"problems\" with the file and prim of each.")
                     .arg(linkset.problems.size());
    return out;
  }
  if (linkset.scripts.empty())
  {
    out["ok"] = false;
    out["error"] = "The folder has no .lsl script, so nothing was run.";
    return out;
  }
  if (!checked.value("valid").toBool())
  {
    QStringList broken;
    for (const QJsonValue& v : checked.value("scripts").toArray())
      if (!v.toObject().value("valid").toBool()) broken << v.toObject().value("file").toString();
    out["ok"] = false;
    out["error"] = QString("The folder was not run: %1 script(s) do not compile (%2). Every error names its script and prim.")
                     .arg(broken.size()).arg(broken.join(", "));
    return out;
  }
  QJsonObject answer = lsl::Control::run(request(linkset, options), rlv, session);
  if (!answer.value("ok").toBool())
  {
    out["ok"] = false;
    out["error"] = "The folder was not run: " + answer.value("error").toString();
    return out;
  }
  tagLog(linkset, answer);
  // how long the run was allowed to go on, and why (a visible message, nothing waits in real time)
  const int seconds = request(linkset, options).value("seconds").toInt();
  out["seconds"] = seconds;
  if (options.contains("seconds"))
    out["time_note"] = QString("The setup ran %1 simulated seconds, as asked.").arg(seconds);
  else if (seconds >= AUTO_MAX_SECONDS)
    out["time_note"] = QString("The setup ran the longest automatic time, %1 simulated seconds (%2 scripts). If it needs more, give \"seconds\" (at most 3600).")
                         .arg(seconds).arg(linkset.scripts.size());
  else
    out["time_note"] = QString("The setup ran %1 simulated seconds (%2 + %3 for each of %4 scripts).")
                         .arg(seconds).arg(AUTO_BASE_SECONDS).arg(AUTO_PER_SCRIPT_SECONDS).arg(linkset.scripts.size());
  out["ok"] = true;
  out["result"] = answer;
  return out;
}
