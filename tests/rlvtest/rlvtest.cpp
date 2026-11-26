/*
 * PinglyMotion - command-line runner for .rlvtest scenarios (package A, quality plan)
 *
 *   pinglymotion_rlvtest [--objects <folder>] [<folder or file.rlvtest> ...]     (default: the folder "scenarios")
 *
 * Runs every *.rlvtest file of a folder (not its subfolders), in name order, as the Script Lab replays a
 * recipe (LabRecipe::replay), and prints one line per scenario: "PASS name (3 expectations)" or "FAIL name"
 * with one indented line for each broken expectation, with its line number. The exit code is 0 when every
 * scenario passed and 1 when any failed, did not read, did not run, or when nothing was found to run.
 *
 * The object a scenario runs against: with --objects <folder>, the folder of that name inside it when there
 * is one (scenarios/cuffs.rlvtest and scripts/cuffs/, the layout of rlv-tools-template/); else the folder next
 * to the scenario with the same name, when there is one (scenarios/cuffs.rlvtest and scenarios/cuffs/, read
 * as the Script Lab folder model, docs/SCRIPT_LAB.md);
 * otherwise an object with one part, "Root", and one script that does nothing, which is enough for
 * scenarios that only send RLV lines and expect. The file is read whole and the result is text, so the
 * same code runs in a browser later (docs/DEV.md, "Web-ready code"). Only QtCore.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include <cstdio>

#include "scriptlab/labfolder.h"
#include "scriptlab/labrecipe.h"
#include "scriptlab/rlvbehaviours.h"

namespace
{
  void print(const QString& text)
  {
    std::fputs(text.toUtf8().constData(), stdout);
    std::fputc('\n', stdout);
  }

  LabFolder::Linkset idleObject(const QString& name)
  {
    LabFolder::Linkset ls;
    ls.name = name.left(LabFolder::MAX_NAME);
    ls.folder = QStringLiteral("/scenario/") + ls.name;
    ls.prims.push_back({1, QStringLiteral("Root"), QString()});
    ls.scripts.push_back({QStringLiteral("idle"), QStringLiteral("idle.lsl"), 1, QStringLiteral("default { state_entry() { } }")});
    return ls;
  }

  // 0: passed, 1: failed
  int runOne(const QString& path, const QString& objects)
  {
    const QFileInfo info(path);
    const QString name = info.completeBaseName();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
      print(QStringLiteral("FAIL %1\n  The file cannot be read.").arg(name));
      return 1;
    }
    const LabRecipe::Result parsed = LabRecipe::parse(QString::fromUtf8(file.read(LabRecipe::MAX_FILE_BYTES + 1)));
    if (!parsed.ok())
    {
      print(QStringLiteral("FAIL %1\n  %2").arg(name, parsed.error));
      return 1;
    }
    QDir folder(info.dir().filePath(name));
    if (!objects.isEmpty() && QFileInfo(QDir(objects).filePath(name)).isDir()) folder = QDir(QDir(objects).filePath(name));
    const LabFolder::Linkset object = folder.exists() ? LabFolder::open(folder.absolutePath()) : idleObject(name);
    const LabRecipe::Replay replay = LabRecipe::replay(object, QJsonObject(), *parsed.value, Rlv::Mode::Rlva);
    if (!replay.ran)
    {
      print(QStringLiteral("FAIL %1\n  The object did not run: %2").arg(name, replay.error));
      return 1;
    }
    QStringList notDone;
    for (const LabRecipe::Fact& f : replay.facts)
      if (f.key.startsWith(QLatin1String("step:"))) notDone << QStringLiteral("  Step skipped: %1 is %2.").arg(f.subject, f.state);
    if (!replay.passed())
    {
      print(QStringLiteral("FAIL %1").arg(name));
      for (const LabRecipe::Failure& f : replay.failures) print(QStringLiteral("  ") + f.text);
      for (const QString& n : notDone) print(n);
      return 1;
    }
    if (replay.expects == 0)
    {
      print(QStringLiteral("FAIL %1\n  The scenario has no expect line, so it tests nothing.").arg(name));
      return 1;
    }
    print(QStringLiteral("PASS %1 (%2 expectations)").arg(name).arg(replay.expects));
    for (const QString& n : notDone) print(n);
    return 0;
  }
}

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  QStringList args = app.arguments().mid(1);
  QString objects;
  const int flag = args.indexOf(QStringLiteral("--objects"));
  if (flag >= 0)
  {
    if (flag + 1 >= args.size() || !QFileInfo(args.at(flag + 1)).isDir())
    {
      print(QStringLiteral("FAIL --objects needs a folder."));
      return 1;
    }
    objects = args.at(flag + 1);
    args.removeAt(flag + 1);
    args.removeAt(flag);
  }
  if (args.isEmpty()) args << QStringLiteral("scenarios");
  QStringList files;
  for (const QString& a : args)
  {
    const QFileInfo info(a);
    if (info.isDir())
    {
      for (const QFileInfo& f : QDir(a).entryInfoList({QStringLiteral("*.rlvtest")}, QDir::Files, QDir::Name)) files << f.absoluteFilePath();
    }
    else if (info.isFile()) files << info.absoluteFilePath();
    else
    {
      print(QStringLiteral("FAIL %1\n  There is no such file or folder.").arg(a));
      return 1;
    }
  }
  if (files.isEmpty())
  {
    print(QStringLiteral("FAIL no .rlvtest file found in: %1").arg(args.join(QStringLiteral(", "))));
    return 1;
  }
  int failed = 0;
  for (const QString& f : files) failed += runOne(f, objects);
  print(QStringLiteral("%1 of %2 scenarios passed.").arg(files.size() - failed).arg(files.size()));
  return failed == 0 ? 0 : 1;
}
