/*
 * PinglyMotion - "Connect an AI" (see aiconnectdialog.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "aiconnectdialog.h"

#include <algorithm>

#include <QButtonGroup>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "aiassistant.h"
#include "dialogfit.h"
#include "lucideicons.h"
#include "theme.h"

namespace
{
  // The pages on top of each other, measured by the page that shows only. QStackedWidget leaves
  // a page with an Ignored height out of its size hint, but its height for a given width (used
  // as soon as a page has wrapping text) takes the tallest page of all. After a text size change
  // that made the dialog as tall as the tallest page, also after Ctrl+0 (UX small fixes).
  class ShownPageStack : public QStackedWidget
  {
    public:
      using QStackedWidget::QStackedWidget;

      QSize sizeHint() const override
      {
        QSize s = QStackedWidget::sizeHint();
        if (const QWidget* page = currentWidget()) s.setHeight(pageHeight(page, s.width()));
        return s;
      }

      bool hasHeightForWidth() const override
      {
        return currentWidget() && currentWidget()->hasHeightForWidth();
      }

      int heightForWidth(int width) const override
      {
        const QWidget* page = currentWidget();
        return page ? pageHeight(page, width) : QStackedWidget::heightForWidth(width);
      }

    private:
      int pageHeight(const QWidget* page, int width) const
      {
        const int frame = 2 * frameWidth();
        const int h = page->hasHeightForWidth() ? page->heightForWidth(width - frame) : page->sizeHint().height();
        return std::max(h, page->minimumSizeHint().height()) + frame;
      }
  };

  QIcon icon(const char* name)
  {
    return LucideIcons::icon(QLatin1String(name), Theme::color(Theme::Token::Text), Theme::color(Theme::Token::TextDisabled));
  }

  QLabel* textLabel(QWidget* parent, const QString& text, const QString& name = QString())
  {
    QLabel* label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    if (!name.isEmpty()) label->setObjectName(name);
    return label;
  }

  QLabel* sectionLabel(QWidget* parent, const QString& text)
  {
    QLabel* label = new QLabel(text, parent);
    label->setWordWrap(true);
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    return label;
  }

  QLabel* captionLabel(QWidget* parent, const QString& text, const QString& name = QString())
  {
    QLabel* label = textLabel(parent, text, name);
    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, Theme::color(Theme::Token::TextMuted));
    label->setPalette(palette);
    return label;
  }

  // "Not yet tested" / "Not tested by us": the warning icon and the words, not the colour alone (charter P10)
  QWidget* badge(QWidget* parent, const QString& text, const QString& name)
  {
    QWidget* box = new QWidget(parent);
    box->setObjectName(name);
    QHBoxLayout* row = new QHBoxLayout(box);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(Theme::Size::spacingXs);
    QLabel* mark = new QLabel(box);
    mark->setPixmap(LucideIcons::icon(QStringLiteral("triangle-alert"), Theme::color(Theme::Token::Warning),
                                      Theme::color(Theme::Token::TextDisabled)).pixmap(Theme::Size::iconSmall, Theme::Size::iconSmall));
    row->addWidget(mark);
    QLabel* words = new QLabel(text, box);
    words->setObjectName(name + QStringLiteral("Text"));
    QPalette palette = words->palette();
    palette.setColor(QPalette::WindowText, Theme::color(Theme::Token::Warning));
    words->setPalette(palette);
    QFont font = words->font();
    font.setWeight(QFont::DemiBold);
    words->setFont(font);
    row->addWidget(words);
    row->addStretch(1);
    return box;
  }

  QString link(const QString& url, const QString& text)
  {
    return QStringLiteral("<a href=\"%1\">%2</a>").arg(url.toHtmlEscaped(), text.toHtmlEscaped());
  }

  QString steps(const QStringList& items)
  {
    QString html = QStringLiteral("<ol style=\"margin-left: 0px; -qt-list-indent: 1;\">");
    for (const QString& item : items) html += QStringLiteral("<li>") + item + QStringLiteral("</li>");
    return html + QStringLiteral("</ol>");
  }
}

AiConnectDialog::AiConnectDialog(bool running, QWidget* parent) :
  QDialog(parent),
  m_pages(new ShownPageStack(this)),
  m_claudeRadio(nullptr), m_chatGptRadio(nullptr), m_otherRadio(nullptr),
  m_turnOn(nullptr), m_chatGptResult(nullptr), m_claudeMessage(nullptr), m_otherMessage(nullptr)
{
  setObjectName(QStringLiteral("aiConnectDialog"));
  setWindowTitle(tr("Connect an AI"));
  setWindowIcon(icon("sparkles"));

  QVBoxLayout* outer = DialogFit::contentLayout(this);
  outer->setSpacing(Theme::Size::spacingL);

  QHBoxLayout* header = new QHBoxLayout();
  QLabel* headerIcon = new QLabel(this);
  headerIcon->setPixmap(icon("sparkles").pixmap(Theme::Size::iconGrid, Theme::Size::iconGrid));
  header->addWidget(headerIcon);
  QLabel* title = new QLabel(tr("Connect an AI"), this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
  title->setFont(titleFont);
  header->addWidget(title);
  header->addStretch(1);
  outer->addLayout(header);

  outer->addWidget(captionLabel(this, tr("An AI assistant on this PC can pose, play, look at and save your animation in PinglyMotion.")));

  // Which AI do you use? Claude / ChatGPT / Other
  outer->addWidget(sectionLabel(this, tr("Which AI do you use?")));
  QHBoxLayout* choices = new QHBoxLayout();
  QButtonGroup* group = new QButtonGroup(this);
  m_claudeRadio = new QRadioButton(tr("Claude"), this);
  m_claudeRadio->setObjectName(QStringLiteral("aiClaudeChoice"));
  m_chatGptRadio = new QRadioButton(tr("ChatGPT"), this);
  m_chatGptRadio->setObjectName(QStringLiteral("aiChatGptChoice"));
  m_otherRadio = new QRadioButton(tr("Other"), this);
  m_otherRadio->setObjectName(QStringLiteral("aiOtherChoice"));
  for (QRadioButton* radio : {m_claudeRadio, m_chatGptRadio, m_otherRadio})
  {
    group->addButton(radio);
    choices->addWidget(radio);
  }
  choices->addStretch(1);
  outer->addLayout(choices);

  QLabel* choose = captionLabel(this, tr("Pick your AI to see its steps."), QStringLiteral("aiChoosePage"));
  m_pages->addWidget(choose);
  m_pages->addWidget(makeClaudePage());
  m_pages->addWidget(makeChatGptPage());
  m_pages->addWidget(makeOtherPage());
  outer->addWidget(m_pages);
  connect(m_claudeRadio, &QRadioButton::toggled, this, [this](bool on) { if (on) showPage(Claude); });
  connect(m_chatGptRadio, &QRadioButton::toggled, this, [this](bool on) { if (on) showPage(ChatGpt); });
  connect(m_otherRadio, &QRadioButton::toggled, this, [this](bool on) { if (on) showPage(Other); });

  // How it's kept safe
  outer->addWidget(sectionLabel(this, tr("How it's kept safe")));
  outer->addWidget(textLabel(this, QStringLiteral("<ul style=\"margin-left: 0px; -qt-list-indent: 1;\"><li>%1</li><li>%2</li><li>%3</li><li>%4</li><li>%5</li></ul>")
    .arg(tr("It only works on your own PC. Nothing on the network or the internet can reach it."),
         tr("It is off each time PinglyMotion starts."),
         tr("There is a new secret key each time you turn it on."),
         tr("Requests from web pages are refused."),
         tr("Turn off locks every AI out and deletes the key.")), QStringLiteral("aiSafety")));
  outer->addWidget(captionLabel(this, tr("It has been through a security review and is tested automatically on every change: %1.")
    .arg(link(AiAssistant::securityReportUrl(), tr("security report"))), QStringLiteral("aiSecurityReview")));
  outer->addStretch(1);

  // the buttons stay outside the scroll area
  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->addStretch(1);
  m_turnOn = new QPushButton(this);
  m_turnOn->setObjectName(QStringLiteral("aiTurnOnButton"));
  connect(m_turnOn, &QPushButton::clicked, this, &AiConnectDialog::turnOnRequested);
  buttons->addWidget(m_turnOn);
  QPushButton* close = new QPushButton(tr("Close"), this);
  close->setObjectName(QStringLiteral("aiCloseButton"));
  connect(close, &QPushButton::clicked, this, &QDialog::accept);
  buttons->addWidget(close);
  setRunning(running);

  DialogFit::finish(this, buttons, QSize(620, 640));
  showPage(Choose);
}

QWidget* AiConnectDialog::makeClaudePage()
{
  QWidget* page = new QWidget(this);
  page->setObjectName(QStringLiteral("aiClaudePage"));
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(Theme::Size::spacingS);

  layout->addWidget(textLabel(page, steps({
    tr("Click <b>Show the file</b>. A folder opens with PinglyMotion.mcpb selected."),
    tr("In the Claude app, open <b>Settings &gt; Desktop app</b> and click <b>Manage</b> next to <b>Extensions</b>."),
    tr("Click <b>Advanced settings</b>, then <b>Install Extension...</b>, and pick PinglyMotion.mcpb. Follow the prompts to finish."),
    tr("Click <b>Turn on AI Assistant</b> below. It is off each time PinglyMotion starts."),
    tr("In Claude, start a new chat and ask, for example: \"Raise the avatar's right arm and show me a picture.\"")}),
    QStringLiteral("aiClaudeSteps")));
  layout->addWidget(captionLabel(page, tr("Older layout of the Claude app: Settings &gt; Extensions &gt; Advanced settings &gt; Install Extension...")));
  layout->addWidget(captionLabel(page, tr("In some versions the installed extension is listed under Settings &gt; Plugins.")));

  QPushButton* show = new QPushButton(icon("folder-open"), tr("Show the file"), page);
  show->setObjectName(QStringLiteral("aiShowExtensionButton"));
  show->setToolTip(tr("Opens the mcp folder next to PinglyMotion with PinglyMotion.mcpb selected."));
  layout->addWidget(show, 0, Qt::AlignLeft);
  m_claudeMessage = textLabel(page, QString(), QStringLiteral("aiClaudeMessage"));
  m_claudeMessage->hide();
  layout->addWidget(m_claudeMessage);
  connect(show, &QPushButton::clicked, this, [this]() { showMcpFile(AiAssistant::extensionFile(), m_claudeMessage); });

  layout->addWidget(textLabel(page, link(AiAssistant::claudeHelpUrl(), tr("Anthropic's help page: local MCP servers and extensions in Claude")),
                              QStringLiteral("aiClaudeHelp")));
  return page;
}

QString AiConnectDialog::nodeStatusText(bool installed)
{
  if (installed) return tr("Node is installed on this PC.");
  return tr("Node is not installed on this PC. ChatGPT needs it to start PinglyMotion's helper. "
            "Install the LTS version from %1, then try again.").arg(link(QStringLiteral("https://nodejs.org"), QStringLiteral("nodejs.org")));
}

QWidget* AiConnectDialog::makeChatGptPage()
{
  QWidget* page = new QWidget(this);
  page->setObjectName(QStringLiteral("aiChatGptPage"));
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(Theme::Size::spacingS);

  layout->addWidget(badge(page, tr("Not yet tested"), QStringLiteral("aiChatGptBadge")));
  layout->addWidget(textLabel(page, tr("ChatGPT's \"Add a custom MCP server\" form only takes a web address or a tunnel, "
                                       "so it cannot be used: PinglyMotion only works on your own PC.")));
  layout->addWidget(textLabel(page, tr("<b>Connect ChatGPT</b> adds PinglyMotion to the settings file that the ChatGPT desktop app reads "
                                       "(config.toml in the .codex folder in your user folder). It makes a backup first and changes nothing else in the file.")));

  const bool node = !AiAssistant::findNode().isEmpty();
  layout->addWidget(textLabel(page, nodeStatusText(node), QStringLiteral("aiNodeStatus")));

  QPushButton* connectButton = new QPushButton(icon("sparkles"), tr("Connect ChatGPT"), page);
  connectButton->setObjectName(QStringLiteral("aiConnectChatGptButton"));
  connectButton->setToolTip(tr("Adds PinglyMotion to ChatGPT's settings file. A backup is made first."));
  layout->addWidget(connectButton, 0, Qt::AlignLeft);
  m_chatGptResult = textLabel(page, QString(), QStringLiteral("aiChatGptResult"));
  m_chatGptResult->hide();
  layout->addWidget(m_chatGptResult);
  connect(connectButton, &QPushButton::clicked, this, &AiConnectDialog::connectChatGpt);

  layout->addWidget(textLabel(page, tr("Then restart the ChatGPT app, click <b>Turn on AI Assistant</b> below and ask ChatGPT, "
                                       "for example: \"Raise the avatar's right arm and show me a picture.\"")));
  layout->addWidget(textLabel(page, link(AiAssistant::openAiHelpUrl(), tr("OpenAI's page about MCP servers (see \"Configure with config.toml\")")),
                              QStringLiteral("aiOpenAiHelp")));
  return page;
}

QWidget* AiConnectDialog::makeOtherPage()
{
  QWidget* page = new QWidget(this);
  page->setObjectName(QStringLiteral("aiOtherPage"));
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(Theme::Size::spacingS);

  layout->addWidget(badge(page, tr("Not tested by us"), QStringLiteral("aiOtherBadge")));
  layout->addWidget(textLabel(page, tr("Most AI tools that can start a local MCP server need a command and a file:")));
  layout->addWidget(textLabel(page, QStringLiteral("<ul style=\"margin-left: 0px; -qt-list-indent: 1;\"><li>%1</li><li>%2</li></ul>")
    .arg(tr("Command: <b>node</b>"), tr("Argument: the file <b>pinglymotion_mcp.js</b> in the mcp folder next to PinglyMotion")),
    QStringLiteral("aiOtherCommand")));
  layout->addWidget(captionLabel(page, tr("<b>Copy the command</b> puts the whole line, with the file's full place on your PC, "
                                          "on the clipboard, ready to paste into your AI tool's settings.")));

  QHBoxLayout* row = new QHBoxLayout();
  QPushButton* copy = new QPushButton(icon("terminal"), tr("Copy the command"), page);
  copy->setObjectName(QStringLiteral("aiCopyCommandButton"));
  row->addWidget(copy);
  QPushButton* show = new QPushButton(icon("folder-open"), tr("Show the file"), page);
  show->setObjectName(QStringLiteral("aiShowServerButton"));
  show->setToolTip(tr("Opens the mcp folder next to PinglyMotion with pinglymotion_mcp.js selected."));
  row->addWidget(show);
  row->addStretch(1);
  layout->addLayout(row);
  m_otherMessage = textLabel(page, QString(), QStringLiteral("aiOtherMessage"));
  m_otherMessage->hide();
  layout->addWidget(m_otherMessage);
  connect(show, &QPushButton::clicked, this, [this]() { showMcpFile(AiAssistant::serverFile(), m_otherMessage); });
  connect(copy, &QPushButton::clicked, this, [this]()
  {
    const QString server = AiAssistant::serverFile();
    if (!QFileInfo(server).isFile())
      m_otherMessage->setText(tr("pinglymotion_mcp.js was not found in the mcp folder next to PinglyMotion. Download PinglyMotion again."));
    else
    {
      QGuiApplication::clipboard()->setText(AiAssistant::commandLine(server));
      m_otherMessage->setText(tr("Copied. Paste it into your AI tool's settings."));
    }
    m_otherMessage->show();
  });

  layout->addWidget(textLabel(page, nodeStatusText(!AiAssistant::findNode().isEmpty()), QStringLiteral("aiOtherNodeStatus")));
  layout->addWidget(textLabel(page, tr("Did it work, or not? %1.").arg(link(AiAssistant::feedbackUrl(), tr("Tell us what worked"))),
                              QStringLiteral("aiFeedback")));
  return page;
}

void AiConnectDialog::showMcpFile(const QString& file, QLabel* message)
{
  if (AiAssistant::showFile(file))
  {
    message->hide();
    return;
  }
  message->setText(tr("%1 was not found in the mcp folder next to PinglyMotion. Download PinglyMotion again.")
                   .arg(QFileInfo(file).fileName().toHtmlEscaped()));
  message->show();
}

void AiConnectDialog::connectChatGpt()
{
  const QString server = AiAssistant::serverFile();
  if (!QFileInfo(server).isFile())
  {
    m_chatGptResult->setText(tr("pinglymotion_mcp.js was not found in the mcp folder next to PinglyMotion. Download PinglyMotion again. Nothing was changed."));
    m_chatGptResult->show();
    return;
  }

  const CodexConfig::Result result = CodexConfig::connect(CodexConfig::file(), QDir::toNativeSeparators(QFileInfo(server).absoluteFilePath()));
  QString text;
  switch (result.outcome)
  {
    case CodexConfig::Outcome::Added:
      text = result.backupFile.isEmpty()
        ? tr("Done. PinglyMotion was added to ChatGPT's settings file. Restart the ChatGPT app.")
        : tr("Done. PinglyMotion was added to ChatGPT's settings file. A backup of the old file was saved next to it. Restart the ChatGPT app.");
      break;
    case CodexConfig::Outcome::Updated:
      text = tr("Done. The PinglyMotion entry in ChatGPT's settings file was updated. A backup of the old file was saved next to it. Restart the ChatGPT app.");
      break;
    case CodexConfig::Outcome::Unchanged:
      text = tr("PinglyMotion is already in ChatGPT's settings file. Nothing was changed.");
      break;
    case CodexConfig::Outcome::Refused:
      text = result.error + QLatin1Char(' ') + tr("Nothing was changed. The README in the mcp folder shows the lines to add by hand.");
      break;
    case CodexConfig::Outcome::Failed:
      text = result.error;
      break;
  }
  if (AiAssistant::findNode().isEmpty()) text += QStringLiteral("<br>") + nodeStatusText(false);
  m_chatGptResult->setText(text);
  m_chatGptResult->show();
}

AiConnectDialog::Page AiConnectDialog::page() const
{
  return static_cast<Page>(m_pages->currentIndex());
}

void AiConnectDialog::showPage(Page page)
{
  m_pages->setCurrentIndex(page);
  // only the shown page counts for the size: each AI shows only its own steps
  // (only the height policy changes: a page of wrapping text keeps its height-for-width, so the
  // dialog measures the same before and after a text size change)
  for (int i = 0; i < m_pages->count(); ++i)
  {
    QSizePolicy policy = m_pages->widget(i)->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Preferred);
    policy.setVerticalPolicy(i == page ? QSizePolicy::Preferred : QSizePolicy::Ignored);
    m_pages->widget(i)->setSizePolicy(policy);
  }
  QRadioButton* radio = page == Claude ? m_claudeRadio : page == ChatGpt ? m_chatGptRadio : page == Other ? m_otherRadio : nullptr;
  if (radio && !radio->isChecked()) radio->setChecked(true);
  m_pages->adjustSize();
}

void AiConnectDialog::setRunning(bool running)
{
  m_turnOn->setText(running ? tr("AI Assistant is on") : tr("Turn on AI Assistant"));
  m_turnOn->setEnabled(!running);
  m_turnOn->setToolTip(running ? tr("The AI Assistant is on already. Turn it off in its menu in the status bar.")
                               : tr("Turns on the AI Assistant until you turn it off or close PinglyMotion."));
  m_turnOn->setDefault(!running);
}
