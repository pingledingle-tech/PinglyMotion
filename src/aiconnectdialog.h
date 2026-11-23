/*
 * PinglyMotion - "Connect an AI" (issue 7)
 *
 * Opens from the AI Assistant button in the status bar: the first time it is
 * clicked (no AI has ever connected), and from "Connect another AI..." in its
 * menu. It asks "Which AI do you use?" (Claude, ChatGPT, Other); each choice
 * shows only its own steps:
 *
 *  - Claude: install PinglyMotion.mcpb in the Claude app. "Show the file"
 *    opens the mcp folder with the file selected; a link to Anthropic's help.
 *  - ChatGPT (not yet tested): "Connect ChatGPT" adds PinglyMotion to the
 *    config.toml that the ChatGPT desktop app reads (src/aiassistant.*,
 *    CodexConfig), says whether Node is installed; a link to OpenAI's page.
 *  - Other (not tested by us): the command and file, "Copy the command",
 *    "Show the file", and where to tell us what worked.
 *
 * Under the steps: "How it's kept safe". At the bottom, outside the scroll
 * area (src/dialogfit.*): "Turn on AI Assistant" and "Close". The dialog
 * never turns anything on by itself and never shows a path of the user's
 * account: files are named, and shown in the file manager or copied.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef AICONNECTDIALOG_H
#define AICONNECTDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;
class QRadioButton;
class QStackedWidget;

class AiConnectDialog : public QDialog
{
  Q_OBJECT

  public:
    enum Page { Choose, Claude, ChatGpt, Other };

    // running: the AI Assistant is on already (then the turn-on button says so and is greyed)
    explicit AiConnectDialog(bool running, QWidget* parent = nullptr);

    Page page() const;
    void showPage(Page page);
    void setRunning(bool running);

    // the texts of the plain-text guides, also in mcp/README.txt
    static QString nodeStatusText(bool installed);

  signals:
    void turnOnRequested();

  private:
    QWidget* makeClaudePage();
    QWidget* makeChatGptPage();
    QWidget* makeOtherPage();
    void connectChatGpt();
    void showMcpFile(const QString& file, QLabel* message);

    QStackedWidget* m_pages;
    QRadioButton* m_claudeRadio;
    QRadioButton* m_chatGptRadio;
    QRadioButton* m_otherRadio;
    QPushButton* m_turnOn;
    QLabel* m_chatGptResult;
    QLabel* m_claudeMessage;
    QLabel* m_otherMessage;
};

#endif
