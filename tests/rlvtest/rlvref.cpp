/*
 * PinglyMotion - command-line runner for RLV reference answers (package B, quality plan)
 *
 *   pinglymotion_rlvref [--known <file>] [<folder or file.rlvref> ...]     (default: the folder "reference/rlv")
 *
 * Reads every *.rlvref file (answers a real viewer gave, or answers written from the RLV specification;
 * the format is in src/scriptlab/rlvreference.h), asks the sandbox the same questions and prints one
 * line for each case and, for each difference, one indented line with the line number:
 *
 *   SAME     name
 *   KNOWN    name (intended): why     a difference that known.txt lists as intended or unclear
 *   FIXED    name: why                listed as fixed and now the same
 *   NEW      name                     a difference nobody has listed
 *   BROKEN   name (fixed)             listed as fixed, but it differs again
 *   STALE    name                     listed as intended or unclear, but now the same (update the list)
 *
 * The known differences are in known.txt next to the files of a folder, or the file named with --known
 * ("<state> <file>/<case> <reason>", state fixed, intended or unclear). The exit code is 0 when there is no
 * NEW, BROKEN or STALE case and nothing failed to read, and 1 otherwise. A file that was made from the
 * specification says so in its heading line, every time. A pass is a sandbox result, never a Second Life
 * result. Only QtCore.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QStringList>

#include <cstdio>

#include "scriptlab/rlvreference.h"

namespace
{
  void print(const QString& text)
  {
    std::fputs(text.toUtf8().constData(), stdout);
    std::fputc('\n', stdout);
  }

  bool readFile(const QString& path, qint64 limit, QString& text)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    text = QString::fromUtf8(file.read(limit + 1));
    return true;
  }
}

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  QStringList args = app.arguments().mid(1);
  QString knownPath;
  QStringList paths;
  for (int i = 0; i < args.size(); i++)
  {
    if (args.at(i) == QLatin1String("--known"))
    {
      if (i + 1 >= args.size())
      {
        print(QStringLiteral("FAIL --known needs a file name."));
        return 1;
      }
      knownPath = args.at(++i);
    }
    else paths << args.at(i);
  }
  if (paths.isEmpty()) paths << QStringLiteral("reference/rlv");

  QStringList files;
  for (const QString& a : paths)
  {
    const QFileInfo info(a);
    if (info.isDir())
    {
      for (const QFileInfo& f : QDir(a).entryInfoList({QStringLiteral("*.rlvref")}, QDir::Files, QDir::Name)) files << f.absoluteFilePath();
      if (knownPath.isEmpty() && QFileInfo(QDir(a).filePath(QStringLiteral("known.txt"))).isFile())
        knownPath = QDir(a).filePath(QStringLiteral("known.txt"));
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
    print(QStringLiteral("FAIL no .rlvref file found in: %1").arg(paths.join(QStringLiteral(", "))));
    return 1;
  }

  QMap<QString, RlvReference::Known> known;
  if (!knownPath.isEmpty())
  {
    QString text;
    if (!readFile(knownPath, RlvReference::MAX_FILE_BYTES, text))
    {
      print(QStringLiteral("FAIL %1\n  The file cannot be read.").arg(knownPath));
      return 1;
    }
    const RlvReference::KnownResult parsed = RlvReference::parseKnown(text);
    if (!parsed.ok())
    {
      print(QStringLiteral("FAIL %1\n  %2").arg(QFileInfo(knownPath).fileName(), parsed.error));
      return 1;
    }
    for (const RlvReference::Known& k : parsed.list) known.insert(k.key, k);
  }

  int same = 0, fixed = 0, intended = 0, unclear = 0, bad = 0, unreadable = 0;
  QStringList seenKeys, seenFiles;
  for (const QString& path : files)
  {
    const QString fileName = QFileInfo(path).completeBaseName();
    seenFiles << fileName;
    QString text;
    if (!readFile(path, RlvReference::MAX_FILE_BYTES, text) || text.size() > RlvReference::MAX_FILE_BYTES)
    {
      print(QStringLiteral("FAIL %1\n  The file cannot be read or is too big.").arg(fileName));
      unreadable++;
      continue;
    }
    const RlvReference::Result parsed = RlvReference::parse(text);
    if (!parsed.ok())
    {
      print(QStringLiteral("FAIL %1\n  %2").arg(fileName, parsed.error));
      unreadable++;
      continue;
    }
    const RlvReference::Reference& ref = *parsed.value;
    print(QStringLiteral("Reference %1: %2, %3, %4").arg(fileName, ref.viewer, ref.date,
          ref.fromSpecification() ? QStringLiteral("from the specification, not recorded in Second Life")
                                  : QStringLiteral("recorded in Second Life")));
    for (const RlvReference::CaseResult& c : RlvReference::run(ref))
    {
      const QString key = fileName + QLatin1Char('/') + c.name;
      seenKeys << key;
      const bool listed = known.contains(key);
      const RlvReference::Known k = known.value(key);
      auto details = [&]() { for (const RlvReference::Difference& d : c.differences) print(QStringLiteral("  ") + d.text()); };
      if (c.same())
      {
        if (listed && k.state == RlvReference::KnownState::Fixed)
        {
          print(QStringLiteral("FIXED    %1: %2").arg(key, k.reason));
          fixed++;
        }
        else if (listed)
        {
          print(QStringLiteral("STALE    %1\n  Listed as %2, but the sandbox now gives the same answers. Update known.txt.").arg(key, RlvReference::stateName(k.state)));
          bad++;
        }
        else
        {
          print(QStringLiteral("SAME     %1").arg(key));
          same++;
        }
      }
      else if (!listed)
      {
        print(QStringLiteral("NEW      %1").arg(key));
        details();
        bad++;
      }
      else if (k.state == RlvReference::KnownState::Fixed)
      {
        print(QStringLiteral("BROKEN   %1 (fixed)\n  It was listed as fixed, but it differs again: %2").arg(key, k.reason));
        details();
        bad++;
      }
      else
      {
        print(QStringLiteral("KNOWN    %1 (%2): %3").arg(key, RlvReference::stateName(k.state), k.reason));
        details();
        (k.state == RlvReference::KnownState::Intended ? intended : unclear)++;
      }
    }
  }
  for (auto it = known.constBegin(); it != known.constEnd(); ++it)
  {
    const QString file = it.key().left(it.key().indexOf(QLatin1Char('/')));
    if (seenFiles.contains(file) && !seenKeys.contains(it.key()))
    {
      print(QStringLiteral("STALE    %1\n  known.txt line %2 names a case that is not in the file.").arg(it.key()).arg(it.value().line));
      bad++;
    }
  }
  print(QStringLiteral("%1 cases: %2 the same, %3 fixed, %4 known differences (%5 intended, %6 unclear), %7 things to look at, %8 files not read.")
        .arg(seenKeys.size()).arg(same).arg(fixed).arg(intended + unclear).arg(intended).arg(unclear).arg(bad).arg(unreadable));
  return (bad == 0 && unreadable == 0) ? 0 : 1;
}
