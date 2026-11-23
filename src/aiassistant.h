/*
 * PinglyMotion - connecting an AI assistant (issue 7)
 *
 * The parts of the "AI Assistant" button and its setup dialog that need no
 * window, so the tests can check them exactly:
 *
 *  - friendlyName(): the name an AI tool sends (for example "claude-ai")
 *    as the user knows it ("Claude");
 *  - the names of the AIs that have connected, with the time each was last
 *    active, kept in the program's settings (never the secret key);
 *  - the menu of the button: Turn on / Turn off, who is connected and when
 *    each was last active, and "Connect another AI...";
 *  - where the mcp folder is (next to the program), and showing a file in
 *    the file manager;
 *  - CodexConfig: the PinglyMotion entry in the config.toml that the
 *    ChatGPT desktop app reads (~/.codex/config.toml, or $CODEX_HOME). The
 *    file is backed up first and every other byte stays as it was.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <functional>

namespace AiAssistant
{
  // the name an AI tool sends in X-PinglyMotion-Client, as the user knows it:
  // "claude-ai" -> "Claude", "codex-mcp-client" -> "ChatGPT"; empty -> "A program"
  QString friendlyName(const QString& client);

  // an AI that has connected at some time: its friendly name and when it was
  // last active (milliseconds since 1970)
  struct Known
  {
    QString name;
    qint64 lastActive;
  };

  // the remembered AIs, the most recently active first
  QList<Known> knownAis();
  bool anyKnown();
  // remembers the name (not the key); a program without a name is not remembered
  void remember(const QString& name, qint64 when);
  // tests only: forget every remembered name
  void forgetAll();

  // "3 s ago", "2 min ago", "1 h ago"
  QString ago(qint64 ms);

  // one line of the button's menu
  struct MenuLine
  {
    enum Kind { Action, Info, Separator };
    Kind kind;
    QString id;     // object name of an action ("aiTurnOnAction", "aiTurnOffAction", "aiConnectAnotherAction")
    QString text;
  };

  // the menu: Turn on (when off) or Turn off (when on); then each AI with
  // when it was last active (an AI that talked in the last 30 s of this
  // session is "connected"); then "Connect another AI..."
  // session: friendly name -> time of the last contact in this session
  QList<MenuLine> menuLines(bool running, const QList<Known>& known, const QHash<QString, qint64>& session, qint64 now);

  // the mcp folder next to the program (or $PINGLYMOTION_MCP_FOLDER) and its files
  QString mcpFolder();
  QString extensionFile();   // PinglyMotion.mcpb
  QString serverFile();      // pinglymotion_mcp.js

  // what shows a file selected in the file manager: explorer /select on
  // Windows, the folder opened elsewhere
  struct Launch
  {
    QString program;     // empty: open url instead
    QStringList arguments;
    QUrl url;
  };
  Launch showInFolder(const QString& file, bool windows);
  // false when the file does not exist (nothing is started)
  bool showFile(const QString& file);
  // tests only: catch what showFile() would start (an empty function: start it for real)
  void setLauncherForTests(const std::function<void(const Launch&)>& launcher);

  // Node on the PATH (empty when not found)
  QString findNode();
  // tests only: answer this instead of looking (a null string: look for real)
  void setNodeForTests(const QString& node);

  // the command another AI tool needs: node "<path to pinglymotion_mcp.js>"
  QString commandLine(const QString& serverFile);

  // the GitHub issue where people say what worked with another AI
  QString feedbackUrl();
  QString claudeHelpUrl();
  QString openAiHelpUrl();
  QString securityReportUrl();
}

namespace CodexConfig
{
  // $CODEX_HOME, or .codex in the user's home folder
  QString folder();
  QString file();

  enum class Outcome { Added, Updated, Unchanged, Refused, Failed };

  struct Edit
  {
    Outcome outcome;
    QByteArray text;    // the whole new file (Added, Updated); the old one otherwise
    QString error;      // why (Refused, Failed); never holds a path
  };

  // The new file: adds the table [mcp_servers.pinglymotion] with
  // command = "node" and args = ["<serverPath>"] at the end, or changes only
  // the command and args lines of that table when it is there. Every other
  // byte stays as it was. Refused when the file names PinglyMotion in another
  // form (a dotted key, an inline table, a second table) that cannot be
  // changed safely.
  Edit apply(const QByteArray& before, const QString& serverPath);

  // a TOML string for the path: '...' when it can be, "..." with escapes otherwise
  QByteArray tomlString(const QString& value);

  struct Result
  {
    Outcome outcome;
    QString error;
    QString backupFile;   // empty when no backup was needed (new file, or nothing changed)
    bool ok() const { return outcome == Outcome::Added || outcome == Outcome::Updated || outcome == Outcome::Unchanged; }
  };

  // Backs the file up (config.toml.backup-<date>-<time>), then writes the new
  // file in one step. Nothing is written when the outcome is Unchanged, Refused or Failed.
  Result connect(const QString& configFile, const QString& serverPath, const QDateTime& now = QDateTime::currentDateTime());
}

#endif
