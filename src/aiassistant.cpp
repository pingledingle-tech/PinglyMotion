/*
 * PinglyMotion - connecting an AI assistant (see aiassistant.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "aiassistant.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>
#include <vector>

#include "settings.h"

namespace
{
  QString tr(const char* text) { return QCoreApplication::translate("AiAssistant", text); }

  const char* const SETTINGS_GROUP = "ai_assistant";
  const char* const KNOWN_KEY = "known";
  const int MAX_KNOWN = 20;
  const int MAX_MENU_NAMES = 8;
  const qint64 CONNECTED_MS = 30000;   // the same as ControlIndicator::CONTACT_TIMEOUT_MS

  std::function<void(const AiAssistant::Launch&)> testLauncher;
  QString testNode;   // null: look for real

  QList<AiAssistant::Known> readKnown()
  {
    Settings settings;
    settings.beginGroup(QLatin1String(SETTINGS_GROUP));
    const QStringList entries = settings.value(QLatin1String(KNOWN_KEY)).toStringList();
    settings.endGroup();
    QList<AiAssistant::Known> known;
    for (const QString& entry : entries)
    {
      const int space = entry.indexOf(QLatin1Char(' '));
      if (space <= 0) continue;
      bool ok = false;
      const qint64 when = entry.left(space).toLongLong(&ok);
      const QString name = entry.mid(space + 1).trimmed();
      if (!ok || name.isEmpty()) continue;
      known.append({name, when});
    }
    std::stable_sort(known.begin(), known.end(), [](const AiAssistant::Known& a, const AiAssistant::Known& b) { return a.lastActive > b.lastActive; });
    return known;
  }

  void writeKnown(const QList<AiAssistant::Known>& known)
  {
    QStringList entries;
    for (const AiAssistant::Known& entry : known) entries << QString::number(entry.lastActive) + QLatin1Char(' ') + entry.name;
    Settings settings;
    settings.beginGroup(QLatin1String(SETTINGS_GROUP));
    if (entries.isEmpty()) settings.remove(QLatin1String(KNOWN_KEY));
    else settings.setValue(QLatin1String(KNOWN_KEY), entries);
    settings.endGroup();
  }
}

QString AiAssistant::friendlyName(const QString& client)
{
  const QString name = client.simplified();
  if (name.isEmpty()) return tr("A program");
  const QString lower = name.toLower();
  if (lower.contains(QLatin1String("claude-code")) || lower.contains(QLatin1String("claude code"))) return QStringLiteral("Claude Code");
  if (lower.contains(QLatin1String("claude"))) return QStringLiteral("Claude");
  if (lower.contains(QLatin1String("chatgpt")) || lower.contains(QLatin1String("codex")) || lower.contains(QLatin1String("openai")))
    return QStringLiteral("ChatGPT");
  return name;
}

QList<AiAssistant::Known> AiAssistant::knownAis()
{
  return readKnown();
}

bool AiAssistant::anyKnown()
{
  return !readKnown().isEmpty();
}

void AiAssistant::remember(const QString& name, qint64 when)
{
  // printable text only, as the control server already makes it; never anything else
  QString clean;
  for (const QChar c : name)
    if (c.unicode() >= 0x20 && c.unicode() < 0x7f) clean += c;
  clean = clean.simplified().left(40);
  if (clean.isEmpty()) return;

  QList<Known> known = readKnown();
  for (int i = known.size() - 1; i >= 0; --i)
    if (known.at(i).name == clean) known.removeAt(i);
  known.prepend({clean, when});
  std::stable_sort(known.begin(), known.end(), [](const Known& a, const Known& b) { return a.lastActive > b.lastActive; });
  while (known.size() > MAX_KNOWN) known.removeLast();
  writeKnown(known);
}

void AiAssistant::forgetAll()
{
  writeKnown({});
}

QString AiAssistant::ago(qint64 ms)
{
  const qint64 seconds = qMax<qint64>(0, ms / 1000);
  if (seconds < 60) return tr("%1 s ago").arg(seconds);
  if (seconds < 3600) return tr("%1 min ago").arg(seconds / 60);
  return tr("%1 h ago").arg(seconds / 3600);
}

QList<AiAssistant::MenuLine> AiAssistant::menuLines(bool running, const QList<Known>& known, const QHash<QString, qint64>& session, qint64 now)
{
  QList<MenuLine> lines;
  if (running) lines.append({MenuLine::Action, QStringLiteral("aiTurnOffAction"), tr("Turn off (locks every AI out)")});
  else lines.append({MenuLine::Action, QStringLiteral("aiTurnOnAction"), tr("Turn on")});
  lines.append({MenuLine::Separator, QString(), QString()});

  // every name once, the most recently active first
  struct Entry { QString name; qint64 when; bool thisSession; };
  std::vector<Entry> entries;
  for (auto it = session.constBegin(); it != session.constEnd(); ++it) entries.push_back({it.key(), it.value(), true});
  for (const Known& entry : known)
  {
    const bool seen = std::any_of(entries.begin(), entries.end(), [&](const Entry& e) { return e.name == entry.name; });
    if (!seen) entries.push_back({entry.name, entry.lastActive, false});
  }
  std::stable_sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b)
  {
    if (a.when != b.when) return a.when > b.when;
    return a.name < b.name;
  });

  if (entries.empty()) lines.append({MenuLine::Info, QString(), tr("No AI has connected yet")});
  int shown = 0;
  for (const Entry& entry : entries)
  {
    if (shown++ == MAX_MENU_NAMES) break;
    QString text;
    if (entry.thisSession && running && now - entry.when < CONNECTED_MS)
      text = tr("%1: connected, active %2").arg(entry.name, ago(now - entry.when));
    else if (entry.thisSession)
      text = tr("%1: last active %2").arg(entry.name, ago(now - entry.when));
    else
      text = tr("%1: last active %2").arg(entry.name,
        QDateTime::fromMSecsSinceEpoch(entry.when).toString(QStringLiteral("d MMM yyyy, HH:mm")));
    lines.append({MenuLine::Info, QString(), text});
  }

  lines.append({MenuLine::Separator, QString(), QString()});
  lines.append({MenuLine::Action, QStringLiteral("aiConnectAnotherAction"), tr("Connect another AI...")});
  return lines;
}

QString AiAssistant::mcpFolder()
{
  const QString override = qEnvironmentVariable("PINGLYMOTION_MCP_FOLDER");
  if (!override.isEmpty()) return override;
  return QCoreApplication::applicationDirPath() + QStringLiteral("/mcp");
}

QString AiAssistant::extensionFile()
{
  return mcpFolder() + QStringLiteral("/PinglyMotion.mcpb");
}

QString AiAssistant::serverFile()
{
  return mcpFolder() + QStringLiteral("/pinglymotion_mcp.js");
}

AiAssistant::Launch AiAssistant::showInFolder(const QString& file, bool windows)
{
  Launch launch;
  if (windows)
  {
    // Explorer opens the folder with the file selected
    launch.program = QStringLiteral("explorer.exe");
    launch.arguments = QStringList{QStringLiteral("/select,"), QDir::toNativeSeparators(file)};
  }
  else
    launch.url = QUrl::fromLocalFile(QFileInfo(file).absolutePath());
  return launch;
}

bool AiAssistant::showFile(const QString& file)
{
  if (!QFileInfo(file).isFile()) return false;
#ifdef Q_OS_WIN
  const Launch launch = showInFolder(file, true);
#else
  const Launch launch = showInFolder(file, false);
#endif
  if (testLauncher)
  {
    testLauncher(launch);
    return true;
  }
  if (!launch.program.isEmpty()) return QProcess::startDetached(launch.program, launch.arguments);
  return QDesktopServices::openUrl(launch.url);
}

void AiAssistant::setLauncherForTests(const std::function<void(const Launch&)>& launcher)
{
  testLauncher = launcher;
}

QString AiAssistant::findNode()
{
  if (!testNode.isNull()) return testNode;
  return QStandardPaths::findExecutable(QStringLiteral("node"));
}

void AiAssistant::setNodeForTests(const QString& node)
{
  testNode = node;
}

QString AiAssistant::commandLine(const QString& serverFile)
{
  return QStringLiteral("node \"") + QDir::toNativeSeparators(serverFile) + QLatin1Char('"');
}

QString AiAssistant::feedbackUrl()
{
  return QStringLiteral("https://github.com/pingledingle-tech/PinglyMotion/issues/414");
}

QString AiAssistant::claudeHelpUrl()
{
  return QStringLiteral("https://support.claude.com/en/articles/10949351-getting-started-with-local-mcp-servers-on-claude-desktop");
}

QString AiAssistant::openAiHelpUrl()
{
  return QStringLiteral("https://developers.openai.com/codex/mcp");
}

QString AiAssistant::securityReportUrl()
{
  return QStringLiteral("https://github.com/pingledingle-tech/PinglyMotion/blob/main/docs/SECURITY_AUDIT_21NOV.md");
}

// ------------------------------------------------------------------ config.toml

namespace
{
  // one line of the file, with what the scanner found in it
  struct Line
  {
    int start = 0;          // offset of the first byte
    int end = 0;            // offset after the line ending
    QByteArray content;     // without the line ending
    QByteArray eol;         // "\n", "\r\n" or "" (last line without one)
    enum Type { Blank, Header, Key, Continuation } type = Blank;
    bool arrayTable = false;
    QStringList keyParts;   // the table name (Header) or the key (Key)
    int lastLine = 0;       // Key: the line where the value ends (multi-line arrays and strings)
  };

  // the parts of a dotted key or table name ("a . \"b\" . 'c'" -> a, b, c); stops at stopAt
  // outside quotes; empty when it cannot be read
  QStringList keyParts(const QByteArray& text, int from, char stopAt, int* stoppedAt)
  {
    QStringList parts;
    QByteArray current;
    bool any = false;
    int i = from;
    for (; i < text.size(); ++i)
    {
      const char c = text.at(i);
      if (c == stopAt) break;
      if (c == '"' || c == '\'')
      {
        const int close = text.indexOf(c, i + 1);
        if (close < 0) return {};
        current += text.mid(i + 1, close - i - 1);
        any = true;
        i = close;
      }
      else if (c == '.')
      {
        if (!any) return {};
        parts << QString::fromUtf8(current);
        current.clear();
        any = false;
      }
      else if (c == ' ' || c == '\t') continue;
      else
      {
        current += c;
        any = true;
      }
    }
    if (i >= text.size() || !any) return {};
    parts << QString::fromUtf8(current);
    if (stoppedAt) *stoppedAt = i;
    return parts;
  }

  // scans a value from offset i, keeping track of open strings and arrays across lines
  void scanValue(const QByteArray& text, int i, bool& multiBasic, bool& multiLiteral, int& depth)
  {
    const int n = text.size();
    while (i < n)
    {
      if (multiBasic)
      {
        if (text.at(i) == '\\') { i += 2; continue; }
        if (text.mid(i, 3) == "\"\"\"") { multiBasic = false; i += 3; continue; }
        ++i;
        continue;
      }
      if (multiLiteral)
      {
        if (text.mid(i, 3) == "'''") { multiLiteral = false; i += 3; continue; }
        ++i;
        continue;
      }
      const char c = text.at(i);
      if (c == '#') return;
      if (text.mid(i, 3) == "\"\"\"") { multiBasic = true; i += 3; continue; }
      if (text.mid(i, 3) == "'''") { multiLiteral = true; i += 3; continue; }
      if (c == '"')
      {
        ++i;
        while (i < n && text.at(i) != '"') i += text.at(i) == '\\' ? 2 : 1;
        ++i;
        continue;
      }
      if (c == '\'')
      {
        const int close = text.indexOf('\'', i + 1);
        i = close < 0 ? n : close + 1;
        continue;
      }
      if (c == '[') ++depth;
      else if (c == ']' && depth > 0) --depth;
      ++i;
    }
  }

  std::vector<Line> scan(const QByteArray& text)
  {
    std::vector<Line> lines;
    int pos = 0;
    while (pos < text.size())
    {
      Line line;
      line.start = pos;
      const int newline = text.indexOf('\n', pos);
      const int stop = newline < 0 ? text.size() : newline;
      line.end = newline < 0 ? text.size() : newline + 1;
      int contentEnd = stop;
      if (newline >= 0)
      {
        if (contentEnd > pos && text.at(contentEnd - 1) == '\r') --contentEnd;
        line.eol = text.mid(contentEnd, line.end - contentEnd);
      }
      line.content = text.mid(pos, contentEnd - pos);
      lines.push_back(line);
      pos = line.end;
    }

    bool multiBasic = false, multiLiteral = false;
    int depth = 0;
    int openKey = -1;   // the key line whose value is still open
    for (int index = 0; index < static_cast<int>(lines.size()); ++index)
    {
      Line& line = lines[index];
      if (multiBasic || multiLiteral || depth > 0)
      {
        line.type = Line::Continuation;
        scanValue(line.content, 0, multiBasic, multiLiteral, depth);
      }
      else
      {
        const QByteArray trimmed = line.content.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) line.type = Line::Blank;
        else if (trimmed.startsWith('['))
        {
          line.type = Line::Header;
          line.arrayTable = trimmed.startsWith("[[");
          line.keyParts = keyParts(trimmed, line.arrayTable ? 2 : 1, ']', nullptr);
          if (line.keyParts.isEmpty()) line.keyParts << QString();   // unreadable: never ours
        }
        else
        {
          line.type = Line::Key;
          int equals = -1;
          line.keyParts = keyParts(line.content, 0, '=', &equals);
          if (equals >= 0) scanValue(line.content, equals + 1, multiBasic, multiLiteral, depth);
          openKey = index;
        }
      }
      if (openKey >= 0 && !(multiBasic || multiLiteral || depth > 0))
      {
        lines[openKey].lastLine = index;
        openKey = -1;
      }
    }
    if (openKey >= 0) lines[openKey].lastLine = static_cast<int>(lines.size()) - 1;
    return lines;
  }

  bool startsWith(const QStringList& parts, const QStringList& prefix)
  {
    if (parts.size() < prefix.size()) return false;
    for (int i = 0; i < prefix.size(); ++i)
      if (parts.at(i) != prefix.at(i)) return false;
    return true;
  }
}

QByteArray CodexConfig::tomlString(const QString& value)
{
  bool literal = true;
  for (const QChar c : value)
    if (c == QLatin1Char('\'') || (c.unicode() < 0x20 && c != QLatin1Char('\t')) || c.unicode() == 0x7f) literal = false;
  if (literal) return '\'' + value.toUtf8() + '\'';

  QByteArray out = "\"";
  for (const QChar c : value)
  {
    const ushort u = c.unicode();
    if (c == QLatin1Char('"')) out += "\\\"";
    else if (c == QLatin1Char('\\')) out += "\\\\";
    else if (u < 0x20 || u == 0x7f) out += "\\u" + QByteArray::number(u, 16).rightJustified(4, '0');
    else out += QString(c).toUtf8();
  }
  return out + '"';
}

CodexConfig::Edit CodexConfig::apply(const QByteArray& before, const QString& serverPath)
{
  const QByteArray eol = before.contains("\r\n") ? QByteArray("\r\n") : QByteArray("\n");
  const QByteArray commandLine = "command = \"node\"";
  const QByteArray argsLine = "args = [" + tomlString(serverPath) + "]";
  const QStringList ours{QStringLiteral("mcp_servers"), QStringLiteral("pinglymotion")};
  const QStringList servers{QStringLiteral("mcp_servers")};

  const auto refuse = [&]()
  {
    return Edit{Outcome::Refused, before,
                tr("ChatGPT's settings file already names PinglyMotion, or its MCP servers, in a form this button cannot change safely.")};
  };

  const std::vector<Line> lines = scan(before);
  int header = -1;
  QStringList table;   // the current table; empty = the top of the file
  bool inArrayTable = false;
  for (int i = 0; i < static_cast<int>(lines.size()); ++i)
  {
    const Line& line = lines[i];
    if (line.type == Line::Header)
    {
      table = line.keyParts;
      inArrayTable = line.arrayTable;
      if (line.arrayTable && startsWith(line.keyParts, servers)) return refuse();
      if (line.keyParts == ours)
      {
        if (header >= 0) return refuse();
        header = i;
      }
      else if (startsWith(line.keyParts, ours) && header < 0) return refuse();
    }
    else if (line.type == Line::Key && !inArrayTable)
    {
      const QStringList full = table + line.keyParts;
      if (table.isEmpty() && startsWith(line.keyParts, servers)) return refuse();     // mcp_servers... = at the top
      if (table == servers && startsWith(full, ours)) return refuse();                // pinglymotion = { ... }
    }
  }

  if (header < 0)
  {
    QByteArray text = before;
    if (!text.isEmpty())
    {
      if (!text.endsWith('\n')) text += eol;
      text += eol;
    }
    text += "[mcp_servers.pinglymotion]" + eol + commandLine + eol + argsLine + eol;
    return Edit{Outcome::Added, text, QString()};
  }

  // the table runs from its header to the next header
  int tableEnd = static_cast<int>(lines.size());
  for (int i = header + 1; i < static_cast<int>(lines.size()); ++i)
    if (lines[i].type == Line::Header) { tableEnd = i; break; }
  int command = -1, args = -1;
  for (int i = header + 1; i < tableEnd; ++i)
  {
    const Line& line = lines[i];
    if (line.type != Line::Key || line.keyParts.isEmpty()) continue;
    const QString key = line.keyParts.first();
    if (key == QLatin1String("url")) return refuse();   // a web server entry: not ours to turn into a command
    if (key == QLatin1String("command") || key == QLatin1String("args"))
    {
      if (line.keyParts.size() != 1) return refuse();
      int& slot = key == QLatin1String("command") ? command : args;
      if (slot >= 0) return refuse();
      slot = i;
    }
  }

  // the indentation of a replaced line stays; each new line ends like the line it replaces
  const auto replacement = [&](int first, const QByteArray& value)
  {
    const Line& line = lines[first];
    int lead = 0;
    while (lead < line.content.size() && (line.content.at(lead) == ' ' || line.content.at(lead) == '\t')) ++lead;
    const QByteArray indent = line.content.left(lead);
    const QByteArray ending = lines[line.lastLine].eol;
    return indent + value + ending;
  };

  QByteArray text;
  for (int i = 0; i < static_cast<int>(lines.size()); ++i)
  {
    const Line& line = lines[i];
    if (i == command)
    {
      text += replacement(i, commandLine);
      i = line.lastLine;
      if (args < 0)
      {
        if (!text.endsWith('\n')) text += eol;
        text += argsLine + eol;
      }
      continue;
    }
    if (i == args)
    {
      text += replacement(i, argsLine);
      i = line.lastLine;
      continue;
    }
    text += before.mid(line.start, line.end - line.start);
    if (i == header && command < 0)
    {
      if (!text.endsWith('\n')) text += eol;
      text += commandLine + eol;
      if (args < 0) text += argsLine + eol;
    }
  }

  if (text == before) return Edit{Outcome::Unchanged, before, QString()};
  return Edit{Outcome::Updated, text, QString()};
}

QString CodexConfig::folder()
{
  const QString home = qEnvironmentVariable("CODEX_HOME");
  if (!home.isEmpty()) return QDir::cleanPath(home);
  return QDir::homePath() + QStringLiteral("/.codex");
}

QString CodexConfig::file()
{
  return folder() + QStringLiteral("/config.toml");
}

CodexConfig::Result CodexConfig::connect(const QString& configFile, const QString& serverPath, const QDateTime& now)
{
  QByteArray before;
  const bool existed = QFileInfo::exists(configFile);
  if (existed)
  {
    QFile in(configFile);
    if (!in.open(QIODevice::ReadOnly)) return {Outcome::Failed, tr("ChatGPT's settings file could not be read."), QString()};
    before = in.readAll();
  }

  const Edit edit = apply(before, serverPath);
  if (edit.outcome != Outcome::Added && edit.outcome != Outcome::Updated) return {edit.outcome, edit.error, QString()};

  if (!QDir().mkpath(QFileInfo(configFile).absolutePath()))
    return {Outcome::Failed, tr("The folder for ChatGPT's settings file could not be made."), QString()};

  QString backup;
  if (existed)
  {
    // a new backup every time, never over an older one
    const QString base = configFile + QStringLiteral(".backup-") + now.toString(QStringLiteral("yyyyMMdd-HHmmss"));
    backup = base;
    for (int n = 2; QFileInfo::exists(backup); ++n) backup = base + QLatin1Char('-') + QString::number(n);
    QFile out(backup);
    if (!out.open(QIODevice::WriteOnly | QIODevice::NewOnly) || out.write(before) != before.size() || !out.flush())
    {
      out.close();
      QFile::remove(backup);
      return {Outcome::Failed, tr("A backup of ChatGPT's settings file could not be made, so nothing was changed."), QString()};
    }
    out.close();
  }

  QSaveFile out(configFile);
  if (!out.open(QIODevice::WriteOnly) || out.write(edit.text) != edit.text.size() || !out.commit())
    return {Outcome::Failed, tr("ChatGPT's settings file could not be written. It was left as it was."), backup};
  return {edit.outcome, QString(), backup};
}
