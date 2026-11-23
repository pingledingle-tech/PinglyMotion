/*
 * PinglyMotion - tests for the AI Assistant (issue 7): the status bar button's states and texts,
 * its menu, the remembered AI names (never the key), the "Connect an AI" dialog with its three
 * guides, the ChatGPT config.toml entry (backup first, every other byte kept) and that no path of
 * the user's account is shown anywhere in the new screens.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QAbstractButton>
#include <QAction>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

#include <memory>

#include "aiassistant.h"
#include "aiconnectdialog.h"
#include "constants.h"
#include "controlindicator.h"

namespace
{
  // a folder name that must never show up on screen: the user's account stands in for it
  const char* const SECRET = "secret-account-folder";

  QByteArray readFile(const QString& path)
  {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
  }

  void writeFile(const QString& path, const QByteArray& data)
  {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data);
  }

  // every text a person can see in a widget tree: labels, buttons, tips, titles, actions
  QStringList visibleTexts(QWidget* root)
  {
    QStringList texts{root->windowTitle(), root->toolTip()};
    for (QLabel* label : root->findChildren<QLabel*>()) texts << label->text() << label->toolTip();
    for (QAbstractButton* button : root->findChildren<QAbstractButton*>()) texts << button->text() << button->toolTip();
    for (QAction* action : root->findChildren<QAction*>()) texts << action->text() << action->toolTip();
    if (QMenu* menu = qobject_cast<QMenu*>(root))
      for (QAction* action : menu->actions()) texts << action->text() << action->toolTip();
    return texts;
  }

  QString lineTexts(const QList<AiAssistant::MenuLine>& lines)
  {
    QStringList out;
    for (const AiAssistant::MenuLine& line : lines)
      out << (line.kind == AiAssistant::MenuLine::Separator ? QStringLiteral("---")
              : line.kind == AiAssistant::MenuLine::Action ? QStringLiteral("[%1] %2").arg(line.id, line.text)
              : line.text);
    return out.join(QLatin1Char('\n'));
  }

  template <typename T> T* child(QWidget* root, const char* name)
  {
    T* found = root->findChild<T*>(QLatin1String(name));
    if (!found) qWarning("missing child %s", name);
    return found;
  }
}

class TestAiAssistant : public QObject
{
  Q_OBJECT

  QTemporaryDir m_dir;
  QString m_secret;     // <temp>/secret-account-folder: stands in for the user's home folder
  QString m_mcp;        // the mcp folder next to the program
  QString m_codex;      // ChatGPT's .codex folder

  private slots:
    void initTestCase()
    {
      QVERIFY(m_dir.isValid());
      QStandardPaths::setTestModeEnabled(true);
      QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.filePath("settings"));
      m_secret = m_dir.filePath(QLatin1String(SECRET));
      m_mcp = m_secret + "/PinglyMotion/mcp";
      m_codex = m_secret + "/.codex";
      writeFile(m_mcp + "/PinglyMotion.mcpb", "zip");
      writeFile(m_mcp + "/pinglymotion_mcp.js", "// server");
      qputenv("PINGLYMOTION_MCP_FOLDER", m_mcp.toUtf8());
      qputenv("CODEX_HOME", m_codex.toUtf8());
      AiAssistant::setNodeForTests(QStringLiteral("/usr/bin/node"));
    }

    void init()
    {
      AiAssistant::forgetAll();
      QDir(m_codex).removeRecursively();
    }

    void cleanupTestCase()
    {
      AiAssistant::setLauncherForTests({});
      AiAssistant::setNodeForTests(QString());
    }

    // ------------------------------------------------------------------ names

    void friendlyNames()
    {
      QCOMPARE(AiAssistant::friendlyName("claude-ai"), QString("Claude"));
      QCOMPARE(AiAssistant::friendlyName("Claude"), QString("Claude"));
      QCOMPARE(AiAssistant::friendlyName("claude-code"), QString("Claude Code"));
      QCOMPARE(AiAssistant::friendlyName("codex-mcp-client"), QString("ChatGPT"));
      QCOMPARE(AiAssistant::friendlyName("ChatGPT"), QString("ChatGPT"));
      QCOMPARE(AiAssistant::friendlyName("  LM Studio "), QString("LM Studio"));
      QCOMPARE(AiAssistant::friendlyName(""), QString("A program"));
    }

    void namesAreRememberedNotTheKey()
    {
      QVERIFY(!AiAssistant::anyKnown());
      AiAssistant::remember("Claude", 1000);
      AiAssistant::remember("ChatGPT", 2000);
      AiAssistant::remember("", 3000);              // a program without a name is not remembered
      AiAssistant::remember("\x01\x02", 3000);      // nor one with nothing printable
      QVERIFY(AiAssistant::anyKnown());
      QList<AiAssistant::Known> known = AiAssistant::knownAis();
      QCOMPARE(known.size(), 2);
      QCOMPARE(known.at(0).name, QString("ChatGPT"));
      QCOMPARE(known.at(0).lastActive, qint64(2000));
      QCOMPARE(known.at(1).name, QString("Claude"));

      // the same name again: one entry, the newer time, first
      AiAssistant::remember("Claude", 5000);
      known = AiAssistant::knownAis();
      QCOMPARE(known.size(), 2);
      QCOMPARE(known.at(0).name, QString("Claude"));
      QCOMPARE(known.at(0).lastActive, qint64(5000));

      // the settings hold the names and times only, in their own group
      QSettings settings(QSettings::IniFormat, QSettings::UserScope, Constants::organizationName(), Constants::applicationName());
      settings.beginGroup("ai_assistant");
      QCOMPARE(settings.childKeys(), QStringList{"known"});
      QCOMPARE(settings.value("known").toStringList(), (QStringList{"5000 Claude", "2000 ChatGPT"}));
    }

    void indicatorRemembersWhoConnected()
    {
      ControlIndicator indicator;
      indicator.setRunning(true);
      indicator.onContact("claude-ai", "set_pose", true);
      indicator.onContact("", "state", true);
      const QList<AiAssistant::Known> known = AiAssistant::knownAis();
      QCOMPARE(known.size(), 1);
      QCOMPARE(known.at(0).name, QString("Claude"));
      QCOMPARE(indicator.connectedNames(), (QStringList{"A program", "Claude"}));
    }

    // ------------------------------------------------------------------ the button

    void buttonStatesAndTexts()
    {
      typedef ControlIndicator CI;
      QCOMPARE(CI::textFor(CI::Off, {}), QString("AI Assistant off"));
      QCOMPARE(CI::textFor(CI::Waiting, {}), QString("AI Assistant on, waiting for an AI"));
      QCOMPARE(CI::textFor(CI::Problem, {}), QString("AI Assistant: a wrong key was refused"));
      QCOMPARE(CI::textFor(CI::Connected, {"Claude"}), QString("AI Assistant: Claude"));
      QCOMPARE(CI::textFor(CI::Connected, {"ChatGPT", "Claude"}), QString("AI Assistant: 2 AI connected"));

      CI indicator;
      QToolButton* button = indicator.button();
      QCOMPARE(button->objectName(), QString("aiAssistantButton"));
      QVERIFY(button->focusPolicy() & Qt::TabFocus);   // reachable with the keyboard
      // always off at start, and it never turns on by itself
      QCOMPARE(indicator.state(), CI::Off);
      QCOMPARE(indicator.text(), QString("AI Assistant off"));
      QCOMPARE(button->accessibleName(), QString("AI Assistant off"));
      indicator.onContact("claude-ai", "state", true);   // a stray contact while off changes nothing
      QCOMPARE(indicator.state(), CI::Off);
      QVERIFY(!indicator.running());

      indicator.setRunning(true);
      QCOMPARE(indicator.state(), CI::Waiting);
      QCOMPARE(indicator.text(), QString("AI Assistant on, waiting for an AI"));
      indicator.onContact("claude-ai", "set_pose", true);
      QCOMPARE(indicator.state(), CI::Connected);
      QCOMPARE(indicator.text(), QString("AI Assistant: Claude"));
      QVERIFY(button->toolTip().contains("Last command: set_pose"));
      indicator.onContact("codex-mcp-client", QString(), true);
      QCOMPARE(indicator.text(), QString("AI Assistant: 2 AI connected"));
      indicator.onRefused();
      QCOMPARE(indicator.state(), CI::Problem);
      QCOMPARE(indicator.text(), QString("AI Assistant: a wrong key was refused"));
      indicator.setRunning(false);
      QCOMPARE(indicator.text(), QString("AI Assistant off"));

      // a click asks the window; the button itself turns nothing on
      QSignalSpy clicked(&indicator, &CI::clicked);
      button->click();
      QCOMPARE(clicked.count(), 1);
      QVERIFY(!indicator.running());
    }

    // ------------------------------------------------------------------ the menu

    void menuWhenOffAndNothingKnown()
    {
      QCOMPARE(lineTexts(AiAssistant::menuLines(false, {}, {}, 100000)),
               QString("[aiTurnOnAction] Turn on\n---\nNo AI has connected yet\n---\n[aiConnectAnotherAction] Connect another AI..."));
    }

    void menuShowsWhoIsConnectedAndWhen()
    {
      const qint64 now = QDateTime(QDate(2026, 11, 23), QTime(12, 0)).toMSecsSinceEpoch();
      const qint64 earlier = QDateTime(QDate(2026, 11, 1), QTime(10, 5)).toMSecsSinceEpoch();
      const QList<AiAssistant::Known> known{{"Claude", now - 5000}, {"ChatGPT", earlier}};
      QHash<QString, qint64> session{{"Claude", now - 5000}, {"A program", now - 45000}};
      QCOMPARE(lineTexts(AiAssistant::menuLines(true, known, session, now)),
               QString("[aiTurnOffAction] Turn off (locks every AI out)\n---\n"
                       "Claude: connected, active 5 s ago\n"
                       "A program: last active 45 s ago\n"
                       "ChatGPT: last active 1 Nov 2026, 10:05\n"
                       "---\n[aiConnectAnotherAction] Connect another AI..."));
      // off: nobody is connected, the times stay
      QCOMPARE(lineTexts(AiAssistant::menuLines(false, known, session, now)),
               QString("[aiTurnOnAction] Turn on\n---\n"
                       "Claude: last active 5 s ago\n"
                       "A program: last active 45 s ago\n"
                       "ChatGPT: last active 1 Nov 2026, 10:05\n"
                       "---\n[aiConnectAnotherAction] Connect another AI..."));
    }

    void menuActionsAskTheWindow()
    {
      AiAssistant::remember("Claude", QDateTime::currentMSecsSinceEpoch() - 60000);
      ControlIndicator indicator;
      std::unique_ptr<QMenu> menu(indicator.buildMenu(nullptr));
      QStringList ids, texts;
      for (QAction* action : menu->actions())
      {
        if (action->isSeparator()) continue;
        ids << action->objectName();
        texts << action->text();
      }
      QCOMPARE(ids, (QStringList{"aiTurnOnAction", "", "aiConnectAnotherAction"}));
      QVERIFY(texts.at(1).startsWith("Claude: last active "));
      QVERIFY(!menu->actions().at(2)->isEnabled());   // the line about Claude is information only

      QSignalSpy on(&indicator, &ControlIndicator::turnOnRequested);
      QSignalSpy another(&indicator, &ControlIndicator::connectAnotherRequested);
      menu->findChild<QAction*>("aiTurnOnAction")->trigger();
      menu->findChild<QAction*>("aiConnectAnotherAction")->trigger();
      QCOMPARE(on.count(), 1);
      QCOMPARE(another.count(), 1);

      indicator.setRunning(true);
      std::unique_ptr<QMenu> onMenu(indicator.buildMenu(nullptr));
      QAction* off = onMenu->findChild<QAction*>("aiTurnOffAction");
      QVERIFY(off);
      QCOMPARE(off->text(), QString("Turn off (locks every AI out)"));
      QVERIFY(!onMenu->findChild<QAction*>("aiTurnOnAction"));
      QSignalSpy offSpy(&indicator, &ControlIndicator::turnOffRequested);
      off->trigger();
      QCOMPARE(offSpy.count(), 1);
    }

    // ------------------------------------------------------------------ config.toml

    void tomlStrings()
    {
      QCOMPARE(CodexConfig::tomlString("C:\\Program Files\\PinglyMotion\\mcp\\pinglymotion_mcp.js"),
               QByteArray("'C:\\Program Files\\PinglyMotion\\mcp\\pinglymotion_mcp.js'"));
      QCOMPARE(CodexConfig::tomlString("C:\\it's\\a \"b\".js"), QByteArray("\"C:\\\\it's\\\\a \\\"b\\\".js\""));
      QCOMPARE(CodexConfig::tomlString(QString::fromUtf8("/home/x/\xc3\xa6/m.js")), QByteArray("'/home/x/\xc3\xa6/m.js'"));
    }

    void configAddedToAnEmptyFile()
    {
      const CodexConfig::Edit edit = CodexConfig::apply(QByteArray(), "C:\\PM\\mcp\\pinglymotion_mcp.js");
      QCOMPARE(edit.outcome, CodexConfig::Outcome::Added);
      QCOMPARE(edit.text, QByteArray("[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = ['C:\\PM\\mcp\\pinglymotion_mcp.js']\n"));
    }

    void configAppendedKeepsEveryOtherByte()
    {
      const QByteArray before = "# my settings\nmodel = \"gpt-5\"\n\n[mcp_servers.context7]\ncommand = \"npx\"\nargs = [\"-y\", \"@upstash/context7-mcp\"]\n";
      const CodexConfig::Edit edit = CodexConfig::apply(before, "/opt/pm/mcp/pinglymotion_mcp.js");
      QCOMPARE(edit.outcome, CodexConfig::Outcome::Added);
      QVERIFY(edit.text.startsWith(before));
      QCOMPARE(edit.text.mid(before.size()), QByteArray("\n[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = ['/opt/pm/mcp/pinglymotion_mcp.js']\n"));

      // no line ending at the end: one is added before the new table
      const QByteArray open = "model = \"gpt-5\"";
      const CodexConfig::Edit edit2 = CodexConfig::apply(open, "/m.js");
      QCOMPARE(edit2.text, QByteArray("model = \"gpt-5\"\n\n[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = ['/m.js']\n"));

      // Windows line endings stay Windows line endings
      const QByteArray crlf = "model = \"gpt-5\"\r\n";
      const CodexConfig::Edit edit3 = CodexConfig::apply(crlf, "/m.js");
      QCOMPARE(edit3.text, QByteArray("model = \"gpt-5\"\r\n\r\n[mcp_servers.pinglymotion]\r\ncommand = \"node\"\r\nargs = ['/m.js']\r\n"));

      // a byte-order mark and text in comments and strings that look like our table are left alone
      const QByteArray tricky = "\xef\xbb\xbf# [mcp_servers.pinglymotion]\nnotes = \"\"\"\n[mcp_servers.pinglymotion]\n\"\"\"\n";
      const CodexConfig::Edit edit4 = CodexConfig::apply(tricky, "/m.js");
      QCOMPARE(edit4.outcome, CodexConfig::Outcome::Added);
      QVERIFY(edit4.text.startsWith(tricky));
    }

    void configUpdatesOnlyOurEntry()
    {
      const QByteArray before =
        "model = \"gpt-5\"\r\n"
        "\r\n"
        "[mcp_servers.pinglymotion]  # added by PinglyMotion\r\n"
        "  command = \"nodejs\" # old\r\n"
        "startup_timeout_sec = 20\r\n"
        "args = [\r\n"
        "  \"D:\\\\old\\\\pinglymotion_mcp.js\",\r\n"
        "  \"--x]\",\r\n"
        "]\r\n"
        "\r\n"
        "[mcp_servers.pinglymotion.env]\r\n"
        "A = \"1\"\r\n"
        "\r\n"
        "[mcp_servers.other]\r\n"
        "command = \"node\"\r\n"
        "args = ['x.js']\r\n";
      const CodexConfig::Edit edit = CodexConfig::apply(before, "C:\\new\\pinglymotion_mcp.js");
      QCOMPARE(edit.outcome, CodexConfig::Outcome::Updated);
      const QByteArray expected =
        "model = \"gpt-5\"\r\n"
        "\r\n"
        "[mcp_servers.pinglymotion]  # added by PinglyMotion\r\n"
        "  command = \"node\"\r\n"
        "startup_timeout_sec = 20\r\n"
        "args = ['C:\\new\\pinglymotion_mcp.js']\r\n"
        "\r\n"
        "[mcp_servers.pinglymotion.env]\r\n"
        "A = \"1\"\r\n"
        "\r\n"
        "[mcp_servers.other]\r\n"
        "command = \"node\"\r\n"
        "args = ['x.js']\r\n";
      QCOMPARE(edit.text, expected);

      // the same again: nothing to change
      const CodexConfig::Edit again = CodexConfig::apply(edit.text, "C:\\new\\pinglymotion_mcp.js");
      QCOMPARE(again.outcome, CodexConfig::Outcome::Unchanged);
      QCOMPARE(again.text, edit.text);

      // a table without command and args gets them right under its header
      const QByteArray bare = "[mcp_servers.pinglymotion]\nenabled = true\n[x]\ny = 1";
      QCOMPARE(CodexConfig::apply(bare, "/m.js").text,
               QByteArray("[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = ['/m.js']\nenabled = true\n[x]\ny = 1"));
      // a quoted table name is the same table
      QCOMPARE(CodexConfig::apply("[ mcp_servers . \"pinglymotion\" ]\ncommand = \"node\"\n", "/m.js").text,
               QByteArray("[ mcp_servers . \"pinglymotion\" ]\ncommand = \"node\"\nargs = ['/m.js']\n"));
    }

    void configRefusesWhatItCannotChangeSafely_data()
    {
      QTest::addColumn<QByteArray>("before");
      QTest::newRow("inline table") << QByteArray("[mcp_servers]\npinglymotion = { command = \"node\" }\n");
      QTest::newRow("dotted key in mcp_servers") << QByteArray("[mcp_servers]\npinglymotion.command = \"node\"\n");
      QTest::newRow("dotted key at the top") << QByteArray("mcp_servers.pinglymotion.command = \"node\"\n");
      QTest::newRow("inline mcp_servers at the top") << QByteArray("mcp_servers = { other = { command = \"x\" } }\n");
      QTest::newRow("two tables") << QByteArray("[mcp_servers.pinglymotion]\ncommand = \"node\"\n[mcp_servers.pinglymotion]\n");
      QTest::newRow("array of tables") << QByteArray("[[mcp_servers]]\nname = \"x\"\n");
      QTest::newRow("only a sub-table") << QByteArray("[mcp_servers.pinglymotion.env]\nA = \"1\"\n");
      QTest::newRow("a web address entry") << QByteArray("[mcp_servers.pinglymotion]\nurl = \"https://example.invalid/mcp\"\n");
      QTest::newRow("command twice") << QByteArray("[mcp_servers.pinglymotion]\ncommand = \"a\"\ncommand = \"b\"\n");
    }

    void configRefusesWhatItCannotChangeSafely()
    {
      QFETCH(QByteArray, before);
      const CodexConfig::Edit edit = CodexConfig::apply(before, "/m.js");
      QCOMPARE(edit.outcome, CodexConfig::Outcome::Refused);
      QCOMPARE(edit.text, before);
      QVERIFY(!edit.error.isEmpty());
    }

    void configFolderFollowsCodexHome()
    {
      QCOMPARE(CodexConfig::folder(), QDir::cleanPath(m_codex));
      QCOMPARE(CodexConfig::file(), QDir::cleanPath(m_codex) + "/config.toml");
      qunsetenv("CODEX_HOME");
      QCOMPARE(CodexConfig::folder(), QDir::homePath() + "/.codex");
      qputenv("CODEX_HOME", m_codex.toUtf8());
    }

    void configFileIsBackedUpAndWrittenInOneStep()
    {
      const QString file = CodexConfig::file();
      const QDateTime now(QDate(2026, 11, 23), QTime(10, 54, 7));

      // no file yet (and no folder): made, no backup needed
      CodexConfig::Result result = CodexConfig::connect(file, "/pm/mcp/pinglymotion_mcp.js", now);
      QCOMPARE(result.outcome, CodexConfig::Outcome::Added);
      QVERIFY(result.backupFile.isEmpty());
      QCOMPARE(readFile(file), QByteArray("[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = ['/pm/mcp/pinglymotion_mcp.js']\n"));
      QCOMPARE(QDir(m_codex).entryList(QDir::Files), QStringList{"config.toml"});

      // the same again: nothing written, no backup
      result = CodexConfig::connect(file, "/pm/mcp/pinglymotion_mcp.js", now);
      QCOMPARE(result.outcome, CodexConfig::Outcome::Unchanged);
      QCOMPARE(QDir(m_codex).entryList(QDir::Files), QStringList{"config.toml"});

      // the user's own file: backed up byte for byte first, then only our lines change
      const QByteArray mine = "model = \"o3\"\n[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = ['/old/pinglymotion_mcp.js']\n[tui]\nx = 1\n";
      writeFile(file, mine);
      result = CodexConfig::connect(file, "/pm/mcp/pinglymotion_mcp.js", now);
      QCOMPARE(result.outcome, CodexConfig::Outcome::Updated);
      QCOMPARE(result.backupFile, file + ".backup-20261123-105407");
      QCOMPARE(readFile(result.backupFile), mine);
      QCOMPARE(readFile(file), QByteArray("model = \"o3\"\n[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = ['/pm/mcp/pinglymotion_mcp.js']\n[tui]\nx = 1\n"));

      // a second backup in the same second never replaces the first
      writeFile(file, "model = \"o4\"\n");
      result = CodexConfig::connect(file, "/pm/mcp/pinglymotion_mcp.js", now);
      QCOMPARE(result.outcome, CodexConfig::Outcome::Added);
      QCOMPARE(result.backupFile, file + ".backup-20261123-105407-2");
      QCOMPARE(readFile(file + ".backup-20261123-105407"), mine);
      QCOMPARE(readFile(result.backupFile), QByteArray("model = \"o4\"\n"));
      QCOMPARE(readFile(file), QByteArray("model = \"o4\"\n\n[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = ['/pm/mcp/pinglymotion_mcp.js']\n"));

      // refused: nothing written, no backup
      const QByteArray odd = "[mcp_servers]\npinglymotion = { command = \"x\" }\n";
      writeFile(file, odd);
      const int files = QDir(m_codex).entryList(QDir::Files).size();
      result = CodexConfig::connect(file, "/pm/mcp/pinglymotion_mcp.js", now);
      QCOMPARE(result.outcome, CodexConfig::Outcome::Refused);
      QVERIFY(!result.ok());
      QCOMPARE(readFile(file), odd);
      QCOMPARE(QDir(m_codex).entryList(QDir::Files).size(), files);
    }

    // ------------------------------------------------------------------ the dialog

    void dialogAsksWhichAiAndShowsOnlyItsSteps()
    {
      AiConnectDialog dialog(false);
      QCOMPARE(dialog.windowTitle(), QString("Connect an AI"));
      QCOMPARE(dialog.page(), AiConnectDialog::Choose);
      QRadioButton* claude = child<QRadioButton>(&dialog, "aiClaudeChoice");
      QRadioButton* chatgpt = child<QRadioButton>(&dialog, "aiChatGptChoice");
      QRadioButton* other = child<QRadioButton>(&dialog, "aiOtherChoice");
      QVERIFY(claude && chatgpt && other);
      QCOMPARE(claude->text(), QString("Claude"));
      QCOMPARE(chatgpt->text(), QString("ChatGPT"));
      QCOMPARE(other->text(), QString("Other"));
      QVERIFY(!claude->isChecked() && !chatgpt->isChecked() && !other->isChecked());
      QVERIFY(dialog.findChild<QLabel*>("aiChoosePage"));

      QStackedWidget* pages = dialog.findChild<QStackedWidget*>();
      QVERIFY(pages);
      dialog.show();
      const QList<QPair<QRadioButton*, QString>> choices{{claude, "aiClaudePage"}, {chatgpt, "aiChatGptPage"}, {other, "aiOtherPage"}};
      for (const auto& choice : choices)
      {
        choice.first->click();
        QCOMPARE(pages->currentWidget()->objectName(), choice.second);
        for (const auto& otherChoice : choices)
          QCOMPARE(dialog.findChild<QWidget*>(otherChoice.second)->isVisible(), otherChoice.second == choice.second);
      }
      dialog.showPage(AiConnectDialog::Claude);
      QVERIFY(claude->isChecked());
    }

    void claudeSteps()
    {
      AiConnectDialog dialog(false);
      dialog.showPage(AiConnectDialog::Claude);
      QLabel* steps = child<QLabel>(&dialog, "aiClaudeSteps");
      QVERIFY(steps);
      QVERIFY(steps->text().contains("<b>Settings &gt; Desktop app</b> and click <b>Manage</b> next to <b>Extensions</b>"));
      QVERIFY(steps->text().contains("<b>Advanced settings</b>, then <b>Install Extension...</b>, and pick PinglyMotion.mcpb"));
      const QStringList texts = visibleTexts(dialog.findChild<QWidget*>("aiClaudePage"));
      QVERIFY(texts.contains("Older layout of the Claude app: Settings &gt; Extensions &gt; Advanced settings &gt; Install Extension..."));
      QVERIFY(texts.contains("In some versions the installed extension is listed under Settings &gt; Plugins."));
      QVERIFY(child<QLabel>(&dialog, "aiClaudeHelp")->text().contains(AiAssistant::claudeHelpUrl()));

      // Show the file: the mcp folder with PinglyMotion.mcpb selected
      QList<AiAssistant::Launch> launched;
      AiAssistant::setLauncherForTests([&](const AiAssistant::Launch& launch) { launched << launch; });
      child<QPushButton>(&dialog, "aiShowExtensionButton")->click();
      QCOMPARE(launched.size(), 1);
#ifdef Q_OS_WIN
      QCOMPARE(launched.at(0).program, QString("explorer.exe"));
      QCOMPARE(launched.at(0).arguments, (QStringList{"/select,", QDir::toNativeSeparators(m_mcp + "/PinglyMotion.mcpb")}));
#else
      QCOMPARE(launched.at(0).url, QUrl::fromLocalFile(m_mcp));
#endif
      QVERIFY(child<QLabel>(&dialog, "aiClaudeMessage")->isHidden());

      // the file is missing: said plainly, without a path
      qputenv("PINGLYMOTION_MCP_FOLDER", m_dir.filePath("nowhere").toUtf8());
      child<QPushButton>(&dialog, "aiShowExtensionButton")->click();
      qputenv("PINGLYMOTION_MCP_FOLDER", m_mcp.toUtf8());
      QCOMPARE(launched.size(), 1);
      QCOMPARE(child<QLabel>(&dialog, "aiClaudeMessage")->text(),
               QString("PinglyMotion.mcpb was not found in the mcp folder next to PinglyMotion. Download PinglyMotion again."));
      AiAssistant::setLauncherForTests({});
    }

    void showInFolderOnEachSystem()
    {
      const AiAssistant::Launch windows = AiAssistant::showInFolder("C:/PM/mcp/PinglyMotion.mcpb", true);
      QCOMPARE(windows.program, QString("explorer.exe"));
      QCOMPARE(windows.arguments, (QStringList{"/select,", QDir::toNativeSeparators("C:/PM/mcp/PinglyMotion.mcpb")}));
      // a real absolute folder, so the test means the same on every system (on Windows "/opt" gets a drive letter)
      const AiAssistant::Launch other = AiAssistant::showInFolder(m_mcp + "/PinglyMotion.mcpb", false);
      QVERIFY(other.program.isEmpty());
      QCOMPARE(other.url, QUrl::fromLocalFile(m_mcp));
      QVERIFY(!AiAssistant::showFile(m_dir.filePath("missing.mcpb")));
    }

    void chatGptSteps()
    {
      AiAssistant::setNodeForTests(QString(""));   // no Node on this PC
      {
        AiConnectDialog dialog(false);
        dialog.showPage(AiConnectDialog::ChatGpt);
        QCOMPARE(child<QLabel>(&dialog, "aiChatGptBadgeText")->text(), QString("Not yet tested"));
        QCOMPARE(child<QLabel>(&dialog, "aiNodeStatus")->text(), AiConnectDialog::nodeStatusText(false));
        QVERIFY(AiConnectDialog::nodeStatusText(false).startsWith("Node is not installed on this PC."));
        QVERIFY(child<QLabel>(&dialog, "aiOpenAiHelp")->text().contains(AiAssistant::openAiHelpUrl()));
        const QString all = visibleTexts(dialog.findChild<QWidget*>("aiChatGptPage")).join('\n');
        QVERIFY(all.contains("ChatGPT's \"Add a custom MCP server\" form only takes a web address or a tunnel"));
      }
      AiAssistant::setNodeForTests(QStringLiteral("/usr/bin/node"));

      AiConnectDialog dialog(false);
      dialog.showPage(AiConnectDialog::ChatGpt);
      QCOMPARE(child<QLabel>(&dialog, "aiNodeStatus")->text(), QString("Node is installed on this PC."));
      QPushButton* connectButton = child<QPushButton>(&dialog, "aiConnectChatGptButton");
      QCOMPARE(connectButton->text(), QString("Connect ChatGPT"));
      connectButton->click();
      QLabel* result = child<QLabel>(&dialog, "aiChatGptResult");
      QCOMPARE(result->text(), QString("Done. PinglyMotion was added to ChatGPT's settings file. Restart the ChatGPT app."));
      const QByteArray expected = "[mcp_servers.pinglymotion]\ncommand = \"node\"\nargs = [" +
        CodexConfig::tomlString(QDir::toNativeSeparators(m_mcp + "/pinglymotion_mcp.js")) + "]\n";
      QCOMPARE(readFile(CodexConfig::file()), expected);
      connectButton->click();
      QCOMPARE(result->text(), QString("PinglyMotion is already in ChatGPT's settings file. Nothing was changed."));
    }

    void otherSteps()
    {
      AiConnectDialog dialog(false);
      dialog.showPage(AiConnectDialog::Other);
      QCOMPARE(child<QLabel>(&dialog, "aiOtherBadgeText")->text(), QString("Not tested by us"));
      const QString command = child<QLabel>(&dialog, "aiOtherCommand")->text();
      QVERIFY(command.contains("Command: <b>node</b>"));
      QVERIFY(command.contains("Argument: the file <b>pinglymotion_mcp.js</b> in the mcp folder next to PinglyMotion"));
      QVERIFY(child<QLabel>(&dialog, "aiFeedback")->text().contains(AiAssistant::feedbackUrl()));
      QCOMPARE(AiAssistant::feedbackUrl(), QString("https://github.com/pingledingle-tech/PinglyMotion/issues/414"));

      child<QPushButton>(&dialog, "aiCopyCommandButton")->click();
      QCOMPARE(QGuiApplication::clipboard()->text(), "node \"" + QDir::toNativeSeparators(m_mcp + "/pinglymotion_mcp.js") + "\"");
      QCOMPARE(child<QLabel>(&dialog, "aiOtherMessage")->text(), QString("Copied. Paste it into your AI tool's settings."));
    }

    void safetySectionAndTurnOn()
    {
      AiConnectDialog dialog(false);
      const QString safety = child<QLabel>(&dialog, "aiSafety")->text();
      for (const char* line : {"It only works on your own PC.", "It is off each time PinglyMotion starts.",
                               "There is a new secret key each time you turn it on.", "Requests from web pages are refused.",
                               "Turn off locks every AI out and deletes the key."})
        QVERIFY2(safety.contains(QLatin1String(line)), line);
      const QString review = child<QLabel>(&dialog, "aiSecurityReview")->text();
      QVERIFY(review.startsWith("It has been through a security review and is tested automatically on every change"));
      QVERIFY(review.contains(AiAssistant::securityReportUrl()));

      QPushButton* turnOn = child<QPushButton>(&dialog, "aiTurnOnButton");
      QCOMPARE(turnOn->text(), QString("Turn on AI Assistant"));
      QVERIFY(turnOn->isEnabled());
      QSignalSpy spy(&dialog, &AiConnectDialog::turnOnRequested);
      turnOn->click();
      QCOMPARE(spy.count(), 1);
      dialog.setRunning(true);
      QCOMPARE(turnOn->text(), QString("AI Assistant is on"));
      QVERIFY(!turnOn->isEnabled());
      QVERIFY(!turnOn->toolTip().isEmpty());   // greyed with a reason (charter P6)
      QCOMPARE(child<QPushButton>(&dialog, "aiCloseButton")->text(), QString("Close"));
    }

    // ------------------------------------------------------------------ no user path anywhere

    void noUserPathIsShown()
    {
      QStringList texts;
      AiAssistant::setLauncherForTests([](const AiAssistant::Launch&) {});
      // every page with every button pressed, and once more with the files missing
      for (bool missing : {false, true})
      {
        if (missing) qputenv("PINGLYMOTION_MCP_FOLDER", (m_secret + "/gone").toUtf8());
        for (AiConnectDialog::Page page : {AiConnectDialog::Choose, AiConnectDialog::Claude, AiConnectDialog::ChatGpt, AiConnectDialog::Other})
        {
          AiConnectDialog dialog(false);
          dialog.showPage(page);
          for (QPushButton* button : dialog.findChildren<QPushButton*>())
            if (button->objectName() != "aiCloseButton" && button->objectName() != "aiTurnOnButton") button->click();
          texts << visibleTexts(&dialog);
        }
      }
      qputenv("PINGLYMOTION_MCP_FOLDER", m_mcp.toUtf8());
      AiAssistant::setLauncherForTests({});

      // the button and its menu
      AiAssistant::remember("Claude", QDateTime::currentMSecsSinceEpoch());
      ControlIndicator indicator;
      indicator.setRunning(true);
      indicator.onContact("claude-ai", "save_file", true);
      texts << visibleTexts(&indicator);
      std::unique_ptr<QMenu> menu(indicator.buildMenu(nullptr));
      texts << visibleTexts(menu.get());

      const QString all = texts.join('\n');
      QVERIFY(all.contains("Connect ChatGPT"));
      QVERIFY2(!all.contains(QLatin1String(SECRET)), qPrintable(all));
      QVERIFY(!all.contains(m_dir.path()));
      const QString home = QDir::homePath();
      if (home.size() > 1)
      {
        QVERIFY(!all.contains(home));
        QVERIFY(!all.contains(QDir::toNativeSeparators(home)));
      }
      // Options is going away: nothing sends the user there
      QVERIFY(!all.contains("Options"));
    }
};

QTEST_MAIN(TestAiAssistant)
#include "tst_aiassistant.moc"
