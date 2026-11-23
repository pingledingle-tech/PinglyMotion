/*
 * PinglyMotion Script Lab - the Script Lab panel (issue 288)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonArray>
#include <QListWidget>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextEdit>
#include <QUrl>
#include <QEvent>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "effectspreview.h"
#include "labwatcher.h"
#include "lsl/control.h"
#include "lsl/session.h"
#include "objectsview.h"
#include "paneltitlebar.h"
#include "rlvbehaviours.h"
#include "rlvstate.h"
#include "scriptlabsection.h"
#include "lsl/lint.h"
#include "lucideicons.h"
#include "theme.h"
#include "usersettings.h"

#include "scriptlabpanel.h"

const char* const ScriptLabPanel::DockName = "scriptLabDock";
const char* const ScriptLabPanel::ViewActionName = "viewScriptLabAction";
const char* const ScriptLabPanel::DefaultObject = "panel";

namespace
{
  // Copied from docs/design_tokens.json (themes.dark / themes.light). The test
  // tests/scriptlabpanel reads that file and checks every value here.
  struct Tokens
  {
    const char* success;
    const char* danger;
    const char* dangerFill;
    const char* textMuted;
    const char* accent;
    const char* onAccent;
    const char* border;
    const char* focusRing;
    const char* warning;
    const char* surfaceRaised;
    const char* text;
    const char* textDisabled;
  };
  const Tokens DarkTokens = {"#56D39A", "#FF7B72", "#C93A32", "#AEB2BA", "#3272DD", "#FFFFFF", "#767B85", "#9CC2FF", "#F2C14E",
                             "#35373C", "#E6E7EA", "#868A93"};
  const Tokens LightTokens = {"#18794E", "#B42318", "#B42318", "#535963", "#1F63C9", "#FFFFFF", "#7A808A", "#1F63C9", "#8A5A00",
                              "#E9EBEF", "#1C1E22", "#7A808A"};

  const Tokens& tokens(bool dark) { return dark ? DarkTokens : LightTokens; }

  // design_tokens.json: spacing.s, spacing.m, spacing.l, control_height.default, radius.control
  const int SpacingXs = 4;
  const int SpacingS = 8;
  const int SpacingM = 12;
  const int SpacingL = 16;
  const int ControlHeight = 32;
  const int ControlCompact = 24;   // control_height.compact
  const int ControlLarge = 40;     // control_height.large: empty-state actions
  const int HeldChipHeight = 20;   // the mockup's .chip min-height
  const int RadiusControl = 6;

  const int LogLines = 500;

  // a section name: the "label" type token (body size, semibold)
  QLabel* sectionLabel(const QString& text, QWidget* parent)
  {
    QLabel* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    return label;
  }

  // A read-only script view with line numbers in a strip on the left. Lines
  // with an error have their number in the danger colour and bold, and the
  // line is underlined (the error list says the same in words, P10).
  class ScriptView : public QPlainTextEdit
  {
    public:
      explicit ScriptView(QWidget* parent) :
        QPlainTextEdit(parent),
        m_numbers(new Numbers(this))
      {
        setReadOnly(true);
        setLineWrapMode(QPlainTextEdit::NoWrap);
        setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        connect(this, &QPlainTextEdit::blockCountChanged, this, [this](int) { updateMargin(); });
        connect(this, &QPlainTextEdit::updateRequest, this, [this](const QRect& rect, int dy)
        {
          if (dy) m_numbers->scroll(0, dy);
          else m_numbers->update(0, rect.y(), m_numbers->width(), rect.height());
        });
        updateMargin();
      }

      void setMarks(const QSet<int>& errorLines, const QColor& danger, const QColor& muted)
      {
        m_errorLines = errorLines;
        m_danger = danger;
        m_muted = muted;
        m_numbers->update();
      }

      int numberWidth() const
      {
        const int digits = QString::number(qMax(1, blockCount())).size();
        return SpacingS + fontMetrics().horizontalAdvance(QLatin1Char('9')) * qMax(2, digits) + SpacingS;
      }

      void paintNumbers(QPaintEvent* event)
      {
        QPainter painter(m_numbers);
        painter.fillRect(event->rect(), palette().color(QPalette::Window));
        QTextBlock block = firstVisibleBlock();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        while (block.isValid() && top <= event->rect().bottom())
        {
          const int bottom = top + qRound(blockBoundingRect(block).height());
          if (block.isVisible() && bottom >= event->rect().top())
          {
            const int line = block.blockNumber() + 1;
            const bool error = m_errorLines.contains(line);
            QFont font = this->font();
            font.setWeight(error ? QFont::Bold : QFont::Normal);
            painter.setFont(font);
            painter.setPen(error ? m_danger : m_muted);
            painter.drawText(0, top, m_numbers->width() - SpacingS, fontMetrics().height(), Qt::AlignRight, QString::number(line));
          }
          block = block.next();
          top = bottom;
        }
      }

    protected:
      void resizeEvent(QResizeEvent* event) override
      {
        QPlainTextEdit::resizeEvent(event);
        const QRect inner = contentsRect();
        m_numbers->setGeometry(QRect(inner.left(), inner.top(), numberWidth(), inner.height()));
      }

    private:
      class Numbers : public QWidget
      {
        public:
          explicit Numbers(ScriptView* view) : QWidget(view), m_view(view) {}
          QSize sizeHint() const override { return QSize(m_view->numberWidth(), 0); }
        protected:
          void paintEvent(QPaintEvent* event) override { m_view->paintNumbers(event); }
        private:
          ScriptView* m_view;
      };

      void updateMargin() { setViewportMargins(numberWidth(), 0, 0, 0); }

      Numbers* m_numbers;
      QSet<int> m_errorLines;
      QColor m_danger;
      QColor m_muted;
  };

  // A layout that puts its items in a row and goes on in the next row when the
  // row is full, so the "Try:" lines wrap in a narrow docked panel
  class FlowLayout : public QLayout
  {
    public:
      FlowLayout(QWidget* parent, int spacing) : QLayout(parent) { setContentsMargins(0, 0, 0, 0); setSpacing(spacing); }
      ~FlowLayout() override { while (QLayoutItem* item = takeAt(0)) delete item; }
      void addItem(QLayoutItem* item) override { m_items << item; }
      int count() const override { return int(m_items.size()); }
      QLayoutItem* itemAt(int index) const override { return m_items.value(index); }
      QLayoutItem* takeAt(int index) override { return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr; }
      Qt::Orientations expandingDirections() const override { return {}; }
      bool hasHeightForWidth() const override { return true; }
      int heightForWidth(int width) const override { return place(QRect(0, 0, width, 0), false); }
      void setGeometry(const QRect& rect) override { QLayout::setGeometry(rect); place(rect, true); }
      QSize sizeHint() const override { return minimumSize(); }
      QSize minimumSize() const override
      {
        QSize size;
        for (const QLayoutItem* item : m_items) size = size.expandedTo(item->minimumSize());
        const QMargins m = contentsMargins();
        return size + QSize(m.left() + m.right(), m.top() + m.bottom());
      }

    private:
      int place(const QRect& rect, bool apply) const
      {
        int x = rect.x();
        int y = rect.y();
        int rowHeight = 0;
        for (QLayoutItem* item : m_items)
        {
          const QSize hint = item->sizeHint().expandedTo(item->minimumSize());
          if (x > rect.x() && x + hint.width() > rect.right() + 1)
          {
            x = rect.x();
            y += rowHeight + spacing();
            rowHeight = 0;
          }
          if (apply) item->setGeometry(QRect(QPoint(x, y), hint));
          x += hint.width() + spacing();
          rowHeight = qMax(rowHeight, hint.height());
        }
        return y + rowHeight - rect.y();
      }

      QList<QLayoutItem*> m_items;
  };

  QString fieldText(const QJsonValue& value)
  {
    if (value.isString()) return value.toString();
    if (value.isDouble()) return QString::number(value.toDouble(), 'g', 10);
    if (value.isBool()) return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isArray())
    {
      QStringList parts;
      for (const QJsonValue& v : value.toArray()) parts << fieldText(v);
      return parts.join(QStringLiteral(", "));
    }
    return QString();
  }
}

ScriptLabPanel::ScriptLabPanel(QWidget* parent) :
  QWidget(parent)
{
  setObjectName("scriptLabPanel");

  QVBoxLayout* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(SpacingS);

  // --- the one-line explanation, shown while hints are on (the mockup's .explain line)
  m_explainFrame = new QWidget(this);
  m_explainFrame->setObjectName("scriptLabExplainRow");
  QHBoxLayout* explainRow = new QHBoxLayout(m_explainFrame);
  explainRow->setContentsMargins(SpacingM, SpacingS, SpacingM, 0);
  explainRow->setSpacing(SpacingS);
  m_explainIcon = new QLabel(m_explainFrame);
  m_explainIcon->setObjectName("scriptLabExplainIcon");
  m_explainIcon->setAlignment(Qt::AlignTop);
  explainRow->addWidget(m_explainIcon, 0, Qt::AlignTop);
  m_explain = new QLabel(tr("A safe sandbox for RLV and LSL scripts. Nothing here reaches Second Life."), m_explainFrame);
  m_explain->setObjectName("scriptLabExplain");
  m_explain->setWordWrap(true);
  explainRow->addWidget(m_explain, 1);
  outer->addWidget(m_explainFrame);
  m_hintsOn = UserSettings::tipsOn();
  m_explainFrame->setVisible(m_hintsOn);

  // --- Viewer rules: RLV or RLVa, and how many restrictions are held (the mockup's top row)
  QHBoxLayout* rules = new QHBoxLayout;
  rules->setContentsMargins(SpacingM, SpacingS, SpacingM, 0);
  rules->setSpacing(SpacingS);
  m_modeLabel = sectionLabel(tr("Viewer rules"), this);
  m_modeLabel->setObjectName("scriptLabModeLabel");
  rules->addWidget(m_modeLabel);
  QHBoxLayout* segment = new QHBoxLayout;
  segment->setContentsMargins(0, 0, 0, 0);
  segment->setSpacing(0);
  const QString relog = tr("Switching removes every restriction, as after a relog.");
  m_modeRlv = new QPushButton(QStringLiteral("RLV"), this);   // tr-ok: a product name
  m_modeRlv->setObjectName("scriptLabModeRlv");
  m_modeRlv->setToolTip(tr("Plain RLV, as the RLV wiki lists it.") + QLatin1Char(' ') + relog);
  m_modeRlva = new QPushButton(QStringLiteral("RLVa"), this);   // tr-ok: a product name
  m_modeRlva->setObjectName("scriptLabModeRlva");
  m_modeRlva->setToolTip(tr("RLVa, as in Firestorm and other viewers.") + QLatin1Char(' ') + relog);
  for (QPushButton* b : {m_modeRlv, m_modeRlva})
  {
    b->setCheckable(true);
    b->setAutoExclusive(true);
    b->setMinimumHeight(ControlCompact);
    b->setAccessibleDescription(relog);
    segment->addWidget(b);
  }
  m_modeRlva->setChecked(true);   // RLVa is the default (RlvState)
  rules->addLayout(segment);
  rules->addStretch(1);
  m_heldChipFrame = new ScriptLabFrame(this);
  m_heldChipFrame->setObjectName("scriptLabHeldChip");
  m_heldChipFrame->setMinimumHeight(HeldChipHeight);
  m_heldChipFrame->setToolTip(tr("How many RLV restrictions are held now, by every object together"));
  QHBoxLayout* chip = new QHBoxLayout(m_heldChipFrame);
  chip->setContentsMargins(SpacingS, 0, SpacingS, 0);
  chip->setSpacing(SpacingXs);
  m_heldIcon = new QLabel(m_heldChipFrame);
  m_heldIcon->setObjectName("scriptLabHeldIcon");
  chip->addWidget(m_heldIcon);
  m_heldChip = new QLabel(m_heldChipFrame);
  m_heldChip->setObjectName("scriptLabHeldText");
  QFont caption = m_heldChip->font();   // the "caption" type token: 11 px next to the 13 px body
  if (caption.pointSizeF() > 0) caption.setPointSizeF(caption.pointSizeF() * 11.0 / 13.0);
  m_heldChip->setFont(caption);
  chip->addWidget(m_heldChip);
  rules->addWidget(m_heldChipFrame);
  outer->addLayout(rules);
  connect(m_modeRlv, &QPushButton::clicked, this, [this]() { setMode(Rlv::Mode::Rlv); });
  connect(m_modeRlva, &QPushButton::clicked, this, [this]() { setMode(Rlv::Mode::Rlva); });

  m_tabs = new QTabWidget(this);
  m_tabs->setObjectName("scriptLabTabs");
  m_tabs->setDocumentMode(true);
  outer->addWidget(m_tabs, 1);

  // --- the page "Type commands": RLV lines typed by hand (issue 288)
  QWidget* commandsPage = new QWidget(m_tabs);
  commandsPage->setObjectName("scriptLabCommandsPage");
  QVBoxLayout* layout = new QVBoxLayout(commandsPage);
  layout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  layout->setSpacing(SpacingS);

  // --- object and command line
  QGridLayout* input = new QGridLayout;
  input->setHorizontalSpacing(SpacingS);
  input->setVerticalSpacing(SpacingS);

  QLabel* objectLabel = new QLabel(tr("Object"), this);
  m_object = new QLineEdit(QString::fromLatin1(DefaultObject), this);
  m_object->setObjectName("scriptLabObject");
  m_object->setMaxLength(RlvState::MAX_OBJECT_ID_LENGTH);
  m_object->setMinimumHeight(ControlHeight);
  m_object->setToolTip(tr("The object that sends the line, as a script in it would with llOwnerSay. "
                          "Each object holds its own restrictions."));
  objectLabel->setBuddy(m_object);

  QLabel* commandLabel = new QLabel(tr("Command"), this);
  m_command = new QLineEdit(this);
  m_command->setObjectName("scriptLabCommand");
  m_command->setPlaceholderText(QStringLiteral("@detach=n,sendim=n"));
  m_command->setMaxLength(1024);
  m_command->setMinimumHeight(ControlHeight);
  m_command->setClearButtonEnabled(true);
  m_command->setToolTip(tr("An RLV line as a script sends it with llOwnerSay, for example @detach=n,sendim=n or @clear. "
                           "Press Enter to run it."));
  commandLabel->setBuddy(m_command);

  m_run = new QPushButton(tr("Run"), this);
  m_run->setObjectName("scriptLabRun");
  m_run->setMinimumHeight(ControlHeight);
  m_run->setToolTip(tr("Run the line (Enter)"));

  m_reset = new QPushButton(tr("Reset all"), this);
  m_reset->setObjectName("scriptLabReset");
  m_reset->setMinimumHeight(ControlHeight);
  m_reset->setToolTip(tr("Reset: remove every restriction of every object"));

  // the buttons get a row of their own, so the command field keeps its
  // width when the panel is docked narrow beside the others
  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->setSpacing(SpacingS);
  buttons->addWidget(m_run);
  buttons->addStretch(1);
  buttons->addWidget(m_reset);

  input->addWidget(objectLabel, 0, 0);
  input->addWidget(m_object, 0, 1);
  input->addWidget(commandLabel, 1, 0);
  input->addWidget(m_command, 1, 1);
  input->addLayout(buttons, 2, 1);
  input->setColumnStretch(1, 1);
  layout->addLayout(input);

  // --- "Try:" example lines: a click puts the line in the command field and runs it
  QWidget* tryRow = new QWidget(commandsPage);
  tryRow->setObjectName("scriptLabTryRow");
  FlowLayout* tryFlow = new FlowLayout(tryRow, SpacingS);
  m_tryLabel = new QLabel(tr("Try:"), tryRow);
  m_tryLabel->setObjectName("scriptLabTryLabel");
  m_tryLabel->setMinimumHeight(ControlCompact);
  QFont tryCaption = m_tryLabel->font();   // the "caption" type token, as the mockup's .cap
  if (tryCaption.pointSizeF() > 0) tryCaption.setPointSizeF(tryCaption.pointSizeF() * 11.0 / 13.0);
  m_tryLabel->setFont(tryCaption);
  tryFlow->addWidget(m_tryLabel);
  for (const QString& line : tryLines())
  {
    QPushButton* b = new QPushButton(line, tryRow);   // tr-ok: an RLV command, typed as is
    b->setObjectName("scriptLabTryLine");
    b->setMinimumHeight(ControlCompact);
    b->setMaximumHeight(ControlCompact);
    b->setToolTip(tr("Put this line in the command field and run it"));
    connect(b, &QPushButton::clicked, this, [this, line]()
    {
      m_command->setText(line);
      runCommandField();
    });
    tryFlow->addWidget(b);
    m_tryButtons << b;
  }
  layout->addWidget(tryRow);

  // --- restrictions per object
  layout->addSpacing(SpacingL - SpacingS);
  m_restrictionsSection = addSection(QStringLiteral("restrictions"), tr("Restrictions"), true, commandsPage, layout);
  m_empty = new QLabel(tr("No restrictions. Type a line above, for example @detach=n, and press Enter."), this);
  m_empty->setObjectName("scriptLabEmpty");
  m_empty->setWordWrap(true);
  m_restrictionsSection->bodyLayout()->addWidget(m_empty);
  m_restrictions = makeRestrictionList(this);
  m_restrictions->setObjectName("scriptLabRestrictions");
  m_restrictionsSection->bodyLayout()->addWidget(m_restrictions);

  // --- actions, allowed or blocked
  m_actionsSection = addSection(QStringLiteral("actions"), tr("What the avatar may do"), true, commandsPage, layout);
  m_actions = new QTreeWidget(this);
  m_actions->setObjectName("scriptLabActions");
  m_actions->setHeaderLabels({tr("Action"), tr("Now")});
  m_actions->setRootIsDecorated(false);
  m_actions->setUniformRowHeights(true);
  m_actions->setSelectionMode(QAbstractItemView::NoSelection);
  m_actions->header()->setStretchLastSection(false);
  m_actions->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_actions->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_actions->setToolTip(tr("What the avatar may do now: Allowed or Blocked"));
  for (const Rlv::Action& action : Rlv::actions())
  {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_actions);
    item->setText(0, action.label);
    item->setData(0, Qt::UserRole, action.id);
  }
  m_actions->setMinimumHeight(6 * m_actions->fontMetrics().height() + 2 * SpacingS);
  m_actionsSection->bodyLayout()->addWidget(m_actions);

  // --- camera limits in the 3D view (job 7b), off at start
  m_cameraBox = new QCheckBox(tr("Show camera limits in 3D view"), commandsPage);
  m_cameraBox->setObjectName("scriptLabShowCameraLimits");
  m_cameraBox->setChecked(false);
  m_cameraBox->setToolTip(tr("Let the camera limits the script holds (distance from the avatar, focus on the avatar, field of view) "
                             "limit the 3D view. Untick to get your own camera back exactly as it was."));
  layout->addWidget(m_cameraBox);
  m_cameraStatus = new QLabel(commandsPage);
  m_cameraStatus->setObjectName("scriptLabCameraStatus");
  m_cameraStatus->setWordWrap(true);
  m_cameraStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_cameraStatus->hide();
  layout->addWidget(m_cameraStatus);
  connect(m_cameraBox, &QCheckBox::toggled, this, [this](bool on)
  {
    if (m_camera) m_camera->setEnabled(on);
    refreshCamera();
  });

  // --- sky and draw distance in the 3D view (job 7c), off at start
  m_environmentBox = new QCheckBox(tr("Show environment in 3D view"), commandsPage);
  m_environmentBox->setObjectName("scriptLabShowEnvironment");
  m_environmentBox->setChecked(false);
  m_environmentBox->setToolTip(tr("Show the sky colour and the draw distance the script asks for as the 3D view's background "
                                  "and fog (a rough hint). Untick to get the 3D view's own look back exactly as it was."));
  layout->addWidget(m_environmentBox);
  m_environmentStatus = new QLabel(commandsPage);
  m_environmentStatus->setObjectName("scriptLabEnvironmentStatus");
  m_environmentStatus->setWordWrap(true);
  m_environmentStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_environmentStatus->hide();
  layout->addWidget(m_environmentStatus);
  connect(m_environmentBox, &QCheckBox::toggled, this, [this](bool on)
  {
    if (m_environment) m_environment->setEnabled(on);
    refreshEnvironment();
  });

  // --- view modes in the 3D view (job 7c), off at start
  m_viewModesBox = new QCheckBox(tr("Show view modes in 3D view"), commandsPage);
  m_viewModesBox->setObjectName("scriptLabShowViewModes");
  m_viewModesBox->setChecked(false);
  m_viewModesBox->setToolTip(tr("Let the view restrictions the script holds (your own avatar hidden, textures replaced, an overlay) "
                                "change the 3D view. Untick to get the 3D view's own look back exactly as it was."));
  layout->addWidget(m_viewModesBox);
  m_viewModesStatus = new QLabel(commandsPage);
  m_viewModesStatus->setObjectName("scriptLabViewModesStatus");
  m_viewModesStatus->setWordWrap(true);
  m_viewModesStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_viewModesStatus->hide();
  layout->addWidget(m_viewModesStatus);
  connect(m_viewModesBox, &QCheckBox::toggled, this, [this](bool on)
  {
    if (m_viewModes) m_viewModes->setEnabled(on);
    refreshViewModes();
  });

  // --- replies and messages
  layout->addSpacing(SpacingL - SpacingS);
  m_repliesSection = addSection(QStringLiteral("replies"), tr("Replies and messages"), true, commandsPage, layout);
  m_log = new QPlainTextEdit(this);
  m_log->setObjectName("scriptLabLog");
  m_log->setReadOnly(true);
  m_log->setMaximumBlockCount(LogLines);
  m_log->setToolTip(tr("The lines you ran, what happened, and the replies a script would hear on its channel"));
  m_log->setMinimumHeight(4 * m_log->fontMetrics().height() + 2 * SpacingS);
  m_repliesSection->bodyLayout()->addWidget(m_log);
  connect(m_log, &QPlainTextEdit::textChanged, this, &ScriptLabPanel::refreshCounts);
  layout->addStretch(1);

  // the page is taller than a small docked panel: it scrolls, as the Live page does
  QScrollArea* commandsScroll = new QScrollArea(m_tabs);
  commandsScroll->setObjectName("scriptLabCommandsScroll");
  commandsScroll->setWidgetResizable(true);
  commandsScroll->setFrameShape(QFrame::NoFrame);
  commandsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  commandsScroll->setWidget(commandsPage);

  // the Live page first, as in the mockup
  buildLivePage();
  m_tabs->insertTab(CommandsPage, commandsScroll, tr("Type commands"));
  m_tabs->setTabToolTip(CommandsPage, tr("Commands: type RLV lines by hand"));
  m_tabs->setCurrentIndex(LivePage);

  connect(m_command, &QLineEdit::returnPressed, this, &ScriptLabPanel::runCommandField);
  connect(m_run, &QPushButton::clicked, this, &ScriptLabPanel::runCommandField);
  connect(m_reset, &QPushButton::clicked, this, &ScriptLabPanel::resetAll);

  // tab order follows the screen (charter P8)
  setTabOrder(m_modeRlv, m_modeRlva);
  setTabOrder(m_object, m_command);
  setTabOrder(m_command, m_run);
  setTabOrder(m_run, m_reset);
  QWidget* beforeHeader = m_reset;
  for (QPushButton* b : m_tryButtons) { setTabOrder(beforeHeader, b); beforeHeader = b; }
  setTabOrder(beforeHeader, m_restrictionsSection->header());
  setTabOrder(m_restrictionsSection->header(), m_restrictions);
  setTabOrder(m_restrictions, m_actionsSection->header());
  setTabOrder(m_actionsSection->header(), m_actions);
  setTabOrder(m_actions, m_cameraBox);
  setTabOrder(m_cameraBox, m_environmentBox);
  setTabOrder(m_environmentBox, m_viewModesBox);
  setTabOrder(m_viewModesBox, m_repliesSection->header());
  setTabOrder(m_repliesSection->header(), m_log);

  applyStyle();
  refresh();
  refreshMode();
  refreshLive();
  refreshInput();

  // each part opens or folds as it was last time; a change is saved at once in the settings
  applyPartsState(UserSettings::scriptLabParts());
  for (ScriptLabSection* s : m_sections)
    connect(s, &ScriptLabSection::expandedChanged, this, [this]() { UserSettings::setScriptLabParts(partsState()); });
}

QStringList ScriptLabPanel::tryLines()
{
  // the mockup's "Try:" row, in its order
  return {QStringLiteral("@detach=n,sendim=n"), QStringLiteral("@tploc=n"), QStringLiteral("@clear"),
          QStringLiteral("@version=2222"), QStringLiteral("@showself=n")};
}

QString ScriptLabPanel::partsState() const
{
  QStringList parts;
  for (const ScriptLabSection* s : m_sections)
    parts << s->key() + (s->isExpanded() ? QStringLiteral("=open") : QStringLiteral("=folded"));
  return parts.join(QLatin1Char(','));
}

void ScriptLabPanel::applyPartsState(const QString& state)
{
  // anything else than "key=open" or "key=folded" of a known part is left alone
  for (const QString& entry : state.split(QLatin1Char(','), Qt::SkipEmptyParts))
  {
    const QString key = entry.section(QLatin1Char('='), 0, 0).trimmed();
    const QString value = entry.section(QLatin1Char('='), 1).trimmed();
    ScriptLabSection* s = section(key);
    if (!s || (value != QLatin1String("open") && value != QLatin1String("folded"))) continue;
    s->setExpanded(value == QLatin1String("open"));
  }
}

void ScriptLabPanel::setHintsOn(bool on)
{
  m_hintsOn = on;
  m_explainFrame->setVisible(on);
}

ScriptLabSection* ScriptLabPanel::addSection(const QString& key, const QString& title, bool expanded, QWidget* page, QVBoxLayout* layout)
{
  ScriptLabSection* section = new ScriptLabSection(key, title, expanded, page);
  m_sections << section;
  layout->addWidget(section);
  return section;
}

QTreeWidget* ScriptLabPanel::makeRestrictionList(QWidget* parent)
{
  QTreeWidget* list = new QTreeWidget(parent);
  list->setHeaderLabels({tr("Behaviour"), tr("Option"), tr("Object")});
  list->setRootIsDecorated(false);
  list->setUniformRowHeights(true);
  list->setSelectionMode(QAbstractItemView::NoSelection);
  list->header()->setSectionResizeMode(QHeaderView::Stretch);
  list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  list->setToolTip(tr("Every restriction that is held, and which object holds it"));
  list->setMinimumHeight(4 * list->fontMetrics().height() + 2 * SpacingS);
  return list;
}

ScriptLabSection* ScriptLabPanel::section(const QString& key) const
{
  for (ScriptLabSection* s : m_sections)
    if (s->key() == key) return s;
  return nullptr;
}

void ScriptLabPanel::setMode(Rlv::Mode mode)
{
  if (!m_state)
  {
    refreshMode();
    return;
  }
  if (m_state->mode() == mode) return;
  const int removed = static_cast<int>(m_state->restrictions().size());
  m_state->setMode(mode);   // removes every restriction, as after a relog
  appendLog(tr("Viewer rules: %1. %n restriction(s) removed.", nullptr, removed)
              .arg(mode == Rlv::Mode::Rlv ? QStringLiteral("RLV") : QStringLiteral("RLVa")));
  refresh();
}

void ScriptLabPanel::refreshMode()
{
  const bool rlv = m_state && m_state->mode() == Rlv::Mode::Rlv;
  m_modeRlv->setChecked(rlv);
  m_modeRlva->setChecked(!rlv);
  for (QPushButton* b : {m_modeRlv, m_modeRlva})
  {
    QFont font = b->font();
    font.setWeight(b->isChecked() ? QFont::DemiBold : QFont::Normal);   // a shape besides the fill (P10)
    b->setFont(font);
  }
  m_modeRlv->setAccessibleName(tr("Viewer rules: RLV"));
  m_modeRlva->setAccessibleName(tr("Viewer rules: RLVa"));
}

void ScriptLabPanel::setState(RlvState* state)
{
  if (m_changedConnection) disconnect(m_changedConnection);
  if (m_modeConnection) disconnect(m_modeConnection);
  m_state = state;
  if (state)
  {
    m_changedConnection = connect(state, &RlvState::changed, this, &ScriptLabPanel::refresh);
    // rlv_reset with a mode switches it too: the switch follows
    m_modeConnection = connect(state, &RlvState::modeChanged, this, [this]() { refreshMode(); refresh(); });
  }
  refresh();
  refreshMode();
}

void ScriptLabPanel::setSession(lsl::Session* session)
{
  m_objectsView->setSession(session);
  m_session = session;
  connectEffectsToWatcher();
  if (m_inputConnection) disconnect(m_inputConnection);
  if (session) m_inputConnection = connect(session, &lsl::Session::changed, this, &ScriptLabPanel::refreshInput);
  refreshInput();
}

bool ScriptLabPanel::isDark(const QPalette& palette)
{
  return palette.color(QPalette::Window).lightness() < 128;
}

QColor ScriptLabPanel::successColour(bool dark) { return QColor(QString::fromLatin1(tokens(dark).success)); }
QColor ScriptLabPanel::dangerColour(bool dark) { return QColor(QString::fromLatin1(tokens(dark).danger)); }
QColor ScriptLabPanel::mutedColour(bool dark) { return QColor(QString::fromLatin1(tokens(dark).textMuted)); }

void ScriptLabPanel::applyStyle()
{
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  // main button: accent fill; Reset removes restrictions, so it is a danger
  // button with its word on it (docs/DESIGN_SYSTEM.md, "Shape")
  const QString button = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %1; border-radius: %3px; padding: 0 %4px; }"
    "QPushButton:focus { border: 2px solid %5; }");
  m_run->setStyleSheet(button.arg(QLatin1String(t.accent), QLatin1String(t.onAccent)).arg(RadiusControl).arg(SpacingM)
                       .arg(QLatin1String(t.focusRing)));
  m_reset->setStyleSheet(button.arg(QLatin1String(t.dangerFill), QLatin1String(t.onAccent)).arg(RadiusControl).arg(SpacingM)
                         .arg(QLatin1String(t.focusRing)));
  if (!m_watch) return;
  // Live page: Watch folder is the main button there; Stop watching and Open
  // in editor are normal buttons (surface_raised fill, border, text), greyed
  // out with text_disabled (DESIGN_SYSTEM.md, "Shape" and "States")
  m_watch->setStyleSheet(button.arg(QLatin1String(t.accent), QLatin1String(t.onAccent)).arg(RadiusControl).arg(SpacingM)
                         .arg(QLatin1String(t.focusRing)));
  const QString normal = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:disabled { color: %6; }"
    "QPushButton:focus { border: 2px solid %7; }")
    .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border))
    .arg(RadiusControl).arg(SpacingM)
    .arg(QLatin1String(t.textDisabled), QLatin1String(t.focusRing));
  m_unwatch->setStyleSheet(normal);
  m_openEditor->setStyleSheet(normal);
  m_runAgain->setStyleSheet(normal);
  m_runAgain->setIcon(LucideIcons::icon(QStringLiteral("refresh-cw"), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textDisabled))));
  for (QPushButton* b : m_tryButtons) b->setStyleSheet(normal);
  m_example->setStyleSheet(normal);
  m_watch->setIcon(LucideIcons::icon(QStringLiteral("folder-open"), QColor(QLatin1String(t.onAccent)), QColor(QLatin1String(t.textDisabled))));
  m_example->setIcon(LucideIcons::icon(QStringLiteral("flask-conical"), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textDisabled))));

  // Viewer rules: two normal buttons joined into one segment; the chosen one has the
  // selection fill, on_accent text and semibold (refreshMode), as the mockup's .seg
  const QString selection = Theme::color(Theme::Token::Selection, dark).name(QColor::HexRgb).toUpper();
  const QString segment = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; padding: 0 %4px; %5 }"
    "QPushButton:checked { background: %6; color: %7; border: 1px solid %6; }"
    "QPushButton:focus { border: 2px solid %8; }");
  m_modeRlv->setStyleSheet(segment.arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border)).arg(SpacingM)
                           .arg(QStringLiteral("border-top-left-radius: %1px; border-bottom-left-radius: %1px;").arg(RadiusControl))
                           .arg(selection, QLatin1String(t.onAccent), QLatin1String(t.focusRing)));
  m_modeRlva->setStyleSheet(segment.arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border)).arg(SpacingM)
                            .arg(QStringLiteral("border-top-right-radius: %1px; border-bottom-right-radius: %1px;").arg(RadiusControl))
                            .arg(selection, QLatin1String(t.onAccent), QLatin1String(t.focusRing)));

  // the "held" chip: caption text with the ban icon, a round border, the border token 14 percent on the surface
  const QColor surface = Theme::color(Theme::Token::Surface, dark);
  const QColor border(QLatin1String(t.border));
  m_heldChipFrame->setLook(border, Theme::mix(surface, border, 0.14), HeldChipHeight / 2, false);
  QPalette chipText = m_heldChip->palette();
  chipText.setColor(QPalette::WindowText, QColor(QLatin1String(t.text)));
  m_heldChip->setPalette(chipText);
  m_heldIcon->setPixmap(LucideIcons::icon(QStringLiteral("ban"), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textDisabled))).pixmap(16, 16));

  // the empty state: a dashed card (radius.panel), a heading, one muted line and two large buttons
  m_liveEmpty->setLook(border, QColor(), Theme::Size::radiusPanel, true);
  QPalette muted = m_liveEmptyText->palette();
  muted.setColor(QPalette::WindowText, QColor(QLatin1String(t.textMuted)));
  m_liveEmptyText->setPalette(muted);
  m_liveEmptyIcon->setPixmap(LucideIcons::icon(QStringLiteral("folder-open"), Theme::color(Theme::Token::AccentText, dark),
                                               QColor(QLatin1String(t.textDisabled))).pixmap(20, 20));
  m_liveRestrictionsEmpty->setPalette(muted);
  m_tabs->setTabIcon(LivePage, LucideIcons::icon(QStringLiteral("file-code"), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textDisabled))));
  if (m_tabs->count() > CommandsPage)
    m_tabs->setTabIcon(CommandsPage, LucideIcons::icon(QStringLiteral("terminal"), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textDisabled))));
  for (ScriptLabSection* section : m_sections) section->applyStyle(dark);
  m_touch->setStyleSheet(normal);
  m_say->setStyleSheet(normal);
  for (QPushButton* b : dialogButtons()) b->setStyleSheet(normal);
  QPalette hint = m_empty->palette();
  hint.setColor(QPalette::WindowText, QColor(QString::fromLatin1(t.textMuted)));
  m_empty->setPalette(hint);
  // the explanation: muted text with the info icon in accent_text (the mockup's .explain);
  // the "Try:" caption and the dialog's message are muted too
  m_explain->setPalette(muted);
  m_tryLabel->setPalette(muted);
  m_inputDialogMessage->setPalette(muted);
  m_explainIcon->setPixmap(LucideIcons::icon(QStringLiteral("info"), Theme::color(Theme::Token::AccentText, dark),
                                             QColor(QLatin1String(t.textDisabled))).pixmap(16, 16));
}

void ScriptLabPanel::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  // the theme was switched: new token colours
  if (event->type() == QEvent::PaletteChange)
  {
    applyStyle();
    refresh();
    refreshLive();
    refreshEffects();
    refreshInput();
    refreshCamera();
    refreshEnvironment();
    refreshViewModes();
  }
}

void ScriptLabPanel::refresh()
{
  const bool dark = isDark(palette());

  QVector<RlvState::Restriction> held;
  if (m_state) held = m_state->restrictions();
  // the same list on both pages: "Restrictions" (Type commands) and "RLV restrictions now" (Live)
  for (QTreeWidget* list : {m_restrictions, m_liveRestrictions})
  {
    list->clear();
    for (const RlvState::Restriction& r : held)
    {
      QTreeWidgetItem* item = new QTreeWidgetItem(list);
      item->setText(0, r.behaviour);
      item->setText(1, r.option);
      item->setText(2, r.object);
    }
    list->setVisible(!held.isEmpty());
  }
  m_empty->setVisible(held.isEmpty());
  m_liveRestrictionsEmpty->setVisible(held.isEmpty());
  m_heldChip->setText(tr("%n held", nullptr, int(held.size())));
  m_heldChipFrame->setAccessibleName(m_heldChip->text());
  refreshCounts();

  QMap<QString, bool> allowed;
  if (m_state) allowed = m_state->actions();
  for (int i = 0; i < m_actions->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* item = m_actions->topLevelItem(i);
    const bool ok = allowed.value(item->data(0, Qt::UserRole).toString(), true);
    item->setText(1, ok ? tr("Allowed") : tr("Blocked"));
    item->setData(1, Qt::UserRole, ok);
    item->setForeground(1, ok ? successColour(dark) : dangerColour(dark));
    QFont font = item->font(1);
    font.setWeight(ok ? QFont::Normal : QFont::DemiBold);   // a second signal besides the word
    item->setFont(1, font);
  }
}

void ScriptLabPanel::appendLog(const QString& line)
{
  m_log->appendPlainText(line);
}

void ScriptLabPanel::runCommandField()
{
  const QString text = m_command->text().trimmed();
  if (text.isEmpty())
  {
    appendLog(tr("Type a line first, for example @detach=n."));
    return;
  }
  if (runLine(text)) m_command->clear();
}

bool ScriptLabPanel::runLine(const QString& text)
{
  if (!m_state)
  {
    appendLog(tr("Script Lab is not ready yet."));
    return false;
  }
  QString object = m_object->text().trimmed();
  if (object.isEmpty()) object = QString::fromLatin1(DefaultObject);

  appendLog(QStringLiteral("%1: %2").arg(object, text));
  const RlvState::ApplyResult result = m_state->apply(object, text);
  if (!result.ok())
  {
    appendLog(tr("  Refused: %1").arg(result.error));
    return false;
  }
  for (const RlvState::Outcome& outcome : result.outcomes)
  {
    QString line = QStringLiteral("  %1: %2").arg(outcome.command.text, Rlv::resultName(outcome.result));
    if (!outcome.reason.isEmpty()) line += QStringLiteral(" (%1)").arg(outcome.reason);
    appendLog(line);
  }
  for (const RlvState::Reply& reply : result.replies)
    appendLog(tr("  Reply on channel %1: %2").arg(reply.channel).arg(reply.text));
  return true;
}

void ScriptLabPanel::resetAll()
{
  if (!m_state) return;
  const int removed = static_cast<int>(m_state->restrictions().size());
  m_state->reset();
  appendLog(tr("Reset: %n restriction(s) removed.", nullptr, removed));
  // reset() only signals when something changed; show the empty state anyway
  refresh();
}

QDockWidget* ScriptLabPanel::makeDock(QMainWindow* window, ScriptLabPanel* panel)
{
  QDockWidget* dock = new QDockWidget(tr("Script Lab"), window);
  dock->setObjectName(QString::fromLatin1(DockName));
  dock->setWidget(panel);
  dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dock->setTitleBarWidget(new PanelTitleBar(dock));

  QAction* action = dock->toggleViewAction();
  action->setObjectName(QString::fromLatin1(ViewActionName));
  action->setText(tr("Script Lab"));
  action->setToolTip(tr("Test RLV commands: type a line such as @detach=n and see what the avatar may still do"));
  action->setStatusTip(action->toolTip());

  // hidden until asked for: someone who only animates never sees it
  window->addDockWidget(Qt::RightDockWidgetArea, dock);
  dock->hide();
  return dock;
}

// --- Live (issue 292) -------------------------------------------------------

void ScriptLabPanel::refreshCounts()
{
  if (!m_restrictionsSection || !m_liveRestrictionsSection || !m_repliesSection) return;
  const int held = m_state ? int(m_state->restrictions().size()) : 0;
  m_restrictionsSection->setCount(QString::number(held));
  m_liveRestrictionsSection->setCount(QString::number(held));
  // one entry for each line run, Reset or switch; the indented lines under it belong to it
  int entries = 0;
  for (QTextBlock block = m_log->document()->begin(); block.isValid(); block = block.next())
    if (!block.text().isEmpty() && !block.text().startsWith(QStringLiteral("  "))) ++entries;
  m_repliesSection->setCount(entries ? QString::number(entries) : tr("none"));
}

void ScriptLabPanel::buildLivePage()
{
  QWidget* page = new QWidget(m_tabs);
  page->setObjectName("scriptLabLivePage");
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  layout->setSpacing(SpacingS);

  // --- the empty state: no folder watched (charter P4)
  m_liveEmpty = new ScriptLabFrame(page);
  m_liveEmpty->setObjectName("scriptLabLiveEmpty");
  QVBoxLayout* card = new QVBoxLayout(m_liveEmpty);
  card->setContentsMargins(SpacingL, SpacingL, SpacingL, SpacingL);
  card->setSpacing(SpacingS);
  QHBoxLayout* headingRow = new QHBoxLayout;
  headingRow->setSpacing(SpacingS);
  m_liveEmptyIcon = new QLabel(m_liveEmpty);
  m_liveEmptyIcon->setObjectName("scriptLabLiveEmptyIcon");
  headingRow->addWidget(m_liveEmptyIcon);
  m_liveEmptyHeading = new QLabel(tr("No folder watched"), m_liveEmpty);
  m_liveEmptyHeading->setObjectName("scriptLabLiveEmptyHeading");
  QFont heading = m_liveEmptyHeading->font();   // the "heading" type token: 20 px next to the 13 px body
  heading.setWeight(QFont::DemiBold);
  if (heading.pointSizeF() > 0) heading.setPointSizeF(heading.pointSizeF() * 20.0 / 13.0);
  m_liveEmptyHeading->setFont(heading);
  headingRow->addWidget(m_liveEmptyHeading);
  headingRow->addStretch(1);
  card->addLayout(headingRow);
  m_liveEmptyText = new QLabel(tr("Pick the folder with your .lsl scripts. Each time you save one, it is checked and run here."), m_liveEmpty);
  m_liveEmptyText->setObjectName("scriptLabLiveEmptyText");
  m_liveEmptyText->setWordWrap(true);
  card->addWidget(m_liveEmptyText);

  m_watch = new QPushButton(tr("Watch folder..."), m_liveEmpty);
  m_watch->setObjectName("scriptLabWatch");
  m_watch->setMinimumHeight(ControlLarge);
  m_watch->setIconSize(QSize(20, 20));
  m_watch->setToolTip(tr("Choose the folder with your .lsl scripts. Each time you save one, it is checked and run again here."));
  m_example = new QPushButton(tr("Try the example scripts"), m_liveEmpty);
  m_example->setObjectName("scriptLabTryExamples");
  m_example->setMinimumHeight(ControlLarge);
  m_example->setIconSize(QSize(20, 20));
  QHBoxLayout* emptyButtons = new QHBoxLayout;
  emptyButtons->setSpacing(SpacingS);
  emptyButtons->addWidget(m_watch);
  emptyButtons->addWidget(m_example);
  emptyButtons->addStretch(1);
  card->addLayout(emptyButtons);
  layout->addWidget(m_liveEmpty);

  // --- the folder and Stop watching, while watching
  m_liveFolderLabel = new QLabel(tr("Folder"), page);
  m_liveFolder = new QLineEdit(page);
  m_liveFolder->setObjectName("scriptLabLiveFolder");
  m_liveFolder->setReadOnly(true);
  m_liveFolder->setMinimumHeight(ControlHeight);
  m_liveFolder->setPlaceholderText(tr("No folder watched"));
  m_liveFolder->setToolTip(tr("The folder with your .lsl scripts. It is only read, never changed."));
  m_liveFolderLabel->setBuddy(m_liveFolder);

  m_unwatch = new QPushButton(tr("Stop watching"), page);
  m_unwatch->setObjectName("scriptLabUnwatch");
  m_unwatch->setMinimumHeight(ControlHeight);

  QHBoxLayout* folderRow = new QHBoxLayout;
  folderRow->setSpacing(SpacingS);
  folderRow->addWidget(m_liveFolderLabel);
  folderRow->addWidget(m_liveFolder, 1);
  folderRow->addWidget(m_unwatch);
  layout->addLayout(folderRow);

  // what happened last, in words (colour and icon only back it up): the mockup's status line,
  // a box in the tone's colour with a circle-check or circle-x icon and the word in bold
  m_liveStatusFrame = new ScriptLabFrame(page);
  m_liveStatusFrame->setObjectName("scriptLabLiveStatusLine");
  QHBoxLayout* statusRow = new QHBoxLayout(m_liveStatusFrame);
  statusRow->setContentsMargins(SpacingS, SpacingS, SpacingS, SpacingS);
  statusRow->setSpacing(SpacingS);
  m_liveStatusIcon = new QLabel(m_liveStatusFrame);
  m_liveStatusIcon->setObjectName("scriptLabLiveStatusIcon");
  statusRow->addWidget(m_liveStatusIcon, 0, Qt::AlignTop);
  m_liveStatus = new QLabel(m_liveStatusFrame);
  m_liveStatus->setObjectName("scriptLabLiveStatus");
  m_liveStatus->setWordWrap(true);
  m_liveStatus->setTextFormat(Qt::RichText);
  m_liveStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  statusRow->addWidget(m_liveStatus, 1);
  layout->addWidget(m_liveStatusFrame);

  m_runAgain = new QPushButton(tr("Run again"), page);
  m_runAgain->setObjectName("scriptLabRunAgain");
  m_runAgain->setMinimumHeight(ControlCompact);
  m_openEditor = new QPushButton(tr("Open in editor"), page);
  m_openEditor->setObjectName("scriptLabOpenEditor");
  m_openEditor->setMinimumHeight(ControlHeight);
  QHBoxLayout* scriptButtons = new QHBoxLayout;
  scriptButtons->setSpacing(SpacingS);
  scriptButtons->addWidget(m_runAgain);
  scriptButtons->addWidget(m_openEditor);
  scriptButtons->addStretch(1);
  layout->addLayout(scriptButtons);

  // Script Lab step 7 (issue 293): the run's effects in the 3D view, off until ticked
  m_effectsBox = new QCheckBox(tr("Show effects in 3D view"), page);
  m_effectsBox->setObjectName("scriptLabShowEffects");
  m_effectsBox->setChecked(false);
  m_effectsBox->setToolTip(tr("Play the animation the script started on a preview figure and show worn objects and locks. "
                              "Your animation is not changed; untick to see it again."));
  layout->addWidget(m_effectsBox);
  m_effectsStatus = new QLabel(page);
  m_effectsStatus->setObjectName("scriptLabEffectsStatus");
  m_effectsStatus->setWordWrap(true);
  m_effectsStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_effectsStatus->hide();
  layout->addWidget(m_effectsStatus);
  connect(m_effectsBox, &QCheckBox::toggled, this, &ScriptLabPanel::effectsToggled);

  // --- the script, read only, with line numbers
  m_scriptSection = addSection(QStringLiteral("script"), tr("Script"), true, page, layout);
  m_scriptView = new ScriptView(m_scriptSection->body());
  m_scriptView->setObjectName("scriptLabScriptView");
  m_scriptView->setToolTip(tr("The script as it is on disk, read only. Change it in your own editor and save."));
  m_scriptView->setMinimumHeight(8 * m_scriptView->fontMetrics().height() + 2 * SpacingS);
  m_scriptSection->bodyLayout()->addWidget(m_scriptView);

  // --- errors and warnings with line and column (lint warnings too)
  m_problemsSection = addSection(QStringLiteral("problems"), tr("Errors and warnings"), true, page, layout);
  m_problems = new QListWidget(m_problemsSection->body());
  m_problems->setObjectName("scriptLabProblems");
  m_problems->setToolTip(tr("Press Enter or double-click a line to show it in the script"));
  m_problems->setMinimumHeight(3 * m_problems->fontMetrics().height() + 2 * SpacingS);
  m_problems->setWordWrap(true);
  m_problemsSection->bodyLayout()->addWidget(m_problems);

  // --- what the run did
  m_logSection = addSection(QStringLiteral("log"), tr("Event log"), true, page, layout);
  m_runLog = new QPlainTextEdit(m_logSection->body());
  m_runLog->setObjectName("scriptLabRunLog");
  m_runLog->setReadOnly(true);
  m_runLog->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  m_runLog->setToolTip(tr("What the script did in the last run: what it said, its listens, timers and RLV commands. Times are simulated seconds."));
  m_runLog->setMinimumHeight(6 * m_runLog->fontMetrics().height() + 2 * SpacingS);
  m_logSection->bodyLayout()->addWidget(m_runLog);

  // --- the RLV restrictions now (the same state as the Type commands page)
  m_liveRestrictionsSection = addSection(QStringLiteral("restrictionsNow"), tr("RLV restrictions now"), true, page, layout);
  m_liveRestrictionsEmpty = new QLabel(tr("No restrictions. A script that says @detach=n holds one."), m_liveRestrictionsSection->body());
  m_liveRestrictionsEmpty->setObjectName("scriptLabLiveRestrictionsEmpty");
  m_liveRestrictionsEmpty->setWordWrap(true);
  m_liveRestrictionsSection->bodyLayout()->addWidget(m_liveRestrictionsEmpty);
  m_liveRestrictions = makeRestrictionList(m_liveRestrictionsSection->body());
  m_liveRestrictions->setObjectName("scriptLabLiveRestrictions");
  m_liveRestrictionsSection->bodyLayout()->addWidget(m_liveRestrictions);

  // --- the objects of the last LSL run and the newest dialog (step 5); folded at first, as in the mockup
  m_objectsSection = addSection(QStringLiteral("objects"), tr("Objects"), false, page, layout);
  m_objectsView = new ScriptLabObjectsView(m_objectsSection->body());
  m_objectsView->setEmbedded(true);
  m_objectsSection->bodyLayout()->addWidget(m_objectsView);
  // the newest dialog with its buttons, under the objects, as in the mockup
  m_inputDialog = sectionLabel(QString(), m_objectsSection->body());
  m_inputDialog->setObjectName("scriptLabInputDialog");
  m_inputDialog->setWordWrap(true);
  m_inputDialog->setTextFormat(Qt::PlainText);
  m_inputDialog->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_objectsSection->bodyLayout()->addWidget(m_inputDialog);
  m_inputDialogMessage = new QLabel(m_objectsSection->body());
  m_inputDialogMessage->setObjectName("scriptLabDialogMessage");
  m_inputDialogMessage->setWordWrap(true);
  m_inputDialogMessage->setTextFormat(Qt::PlainText);
  m_inputDialogMessage->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_objectsSection->bodyLayout()->addWidget(m_inputDialogMessage);
  m_dialogButtonBox = new QWidget(m_objectsSection->body());
  m_dialogButtonBox->setObjectName("scriptLabDialogButtons");
  m_dialogGrid = new QGridLayout(m_dialogButtonBox);
  m_dialogGrid->setContentsMargins(0, 0, 0, 0);
  m_dialogGrid->setSpacing(SpacingS);
  m_objectsSection->bodyLayout()->addWidget(m_dialogButtonBox);

  // --- touch, chat and dialog answers (issue 308); folded at first, as in the mockup
  m_inputSection = addSection(QStringLiteral("input"), tr("Send input"), false, page, layout);
  buildInputSection(m_inputSection->body(), m_inputSection->bodyLayout());
  layout->addStretch(1);

  // the page is taller than a small docked panel: it scrolls instead of forcing the panel's height up
  QScrollArea* scroll = new QScrollArea(m_tabs);
  scroll->setObjectName("scriptLabLiveScroll");
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setWidget(page);
  m_tabs->insertTab(LivePage, scroll, tr("Live scripts"));
  m_tabs->setTabToolTip(LivePage, tr("Live: your .lsl scripts run each time you save them"));

  connect(m_watch, &QPushButton::clicked, this, &ScriptLabPanel::chooseFolder);
  connect(m_example, &QPushButton::clicked, this, &ScriptLabPanel::tryExampleScripts);
  connect(m_unwatch, &QPushButton::clicked, this, &ScriptLabPanel::stopWatching);
  connect(m_openEditor, &QPushButton::clicked, this, &ScriptLabPanel::openInEditor);
  connect(m_runAgain, &QPushButton::clicked, this, &ScriptLabPanel::runAgain);
  connect(m_problems, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) { showProblem(m_problems->row(item)); });

  setTabOrder(m_watch, m_example);
  setTabOrder(m_example, m_liveFolder);
  setTabOrder(m_liveFolder, m_unwatch);
  setTabOrder(m_unwatch, m_runAgain);
  setTabOrder(m_runAgain, m_openEditor);
  setTabOrder(m_openEditor, m_effectsBox);
  setTabOrder(m_effectsBox, m_scriptSection->header());
  setTabOrder(m_scriptSection->header(), m_scriptView);
  setTabOrder(m_scriptView, m_problemsSection->header());
  setTabOrder(m_problemsSection->header(), m_problems);
  setTabOrder(m_problems, m_logSection->header());
  setTabOrder(m_logSection->header(), m_runLog);
  setTabOrder(m_runLog, m_liveRestrictionsSection->header());
  setTabOrder(m_liveRestrictionsSection->header(), m_liveRestrictions);
  setTabOrder(m_liveRestrictions, m_objectsSection->header());
  setTabOrder(m_objectsSection->header(), m_objectsView->list());
  setTabOrder(m_objectsView->list(), m_inputSection->header());
  setTabOrder(m_inputSection->header(), m_inputObject);
  setTabOrder(m_inputObject, m_touch);
  setTabOrder(m_touch, m_chatChannel);
  setTabOrder(m_chatChannel, m_chatName);
  setTabOrder(m_chatName, m_chatText);
  setTabOrder(m_chatText, m_say);
  setTabOrder(m_say, m_inputLog);
}

void ScriptLabPanel::updateWorldParts()
{
  // the effects box, Objects and Send input belong to a run: a watched folder, or the
  // world of an lsl_run (an AI assistant's), which needs no folder
  const bool watching = m_watcher && m_watcher->watching();
  const bool world = m_session && m_session->sandbox();
  for (QWidget* w : QList<QWidget*>{m_effectsBox, m_objectsSection, m_inputSection})
    w->setVisible(watching || world);
}

QJsonArray ScriptLabPanel::lintWarnings(const QString& source)
{
  if (source != m_lintSource)
  {
    m_lintSource = source;
    m_lintWarnings = QJsonArray();
    const lsl::Lint::Result result = lsl::Lint::lint(source);
    for (const lsl::LintWarning& w : result.warnings)
      m_lintWarnings.append(QJsonObject{{"line", w.line}, {"column", w.column}, {"id", w.id}, {"message", w.message}});
  }
  return m_lintWarnings;
}

QString ScriptLabPanel::exampleScriptsFolder() const
{
  if (!m_examplesFolder.isEmpty()) return m_examplesFolder;
  return QDir::cleanPath(EffectsPreview::defaultExamplesFolder() + QStringLiteral("/scriptlab"));
}

bool ScriptLabPanel::tryExampleScripts()
{
  const QString folder = exampleScriptsFolder();
  if (folder.isEmpty() || !QFileInfo(folder).isDir())
  {
    m_refusal = tr("Error: the example scripts are not next to the program.");
    refreshLive();
    return false;
  }
  if (!watchFolder(folder)) return false;
  // the RLV example first, so the restrictions show at once
  if (m_watcher->scripts().contains(QStringLiteral("rlv_lock.lsl"))) m_watcher->runScript(QStringLiteral("rlv_lock.lsl"));
  return true;
}

void ScriptLabPanel::setWatcher(LabWatcher* watcher)
{
  for (const QMetaObject::Connection& c : m_watcherConnections) disconnect(c);
  m_watcherConnections.clear();
  m_watcher = watcher;
  if (watcher)
  {
    m_watcherConnections << connect(watcher, &LabWatcher::resultChanged, this, &ScriptLabPanel::refreshLive);
    m_watcherConnections << connect(watcher, &LabWatcher::watchChanged, this, &ScriptLabPanel::refreshLive);
  }
  m_refusal.clear();
  connectEffectsToWatcher();
  refreshLive();
}

bool ScriptLabPanel::watchFolder(const QString& folder)
{
  if (!m_watcher) return false;
  const LabWatcher::Result result = m_watcher->watch(QDir::fromNativeSeparators(folder));
  if (!result.ok())
  {
    m_refusal = tr("Error: %1").arg(result.error);
    refreshLive();
    return false;
  }
  m_refusal.clear();
  refreshLive();
  return true;
}

void ScriptLabPanel::stopWatching()
{
  if (m_watcher) m_watcher->unwatch();
  m_refusal.clear();
  refreshLive();
}

void ScriptLabPanel::chooseFolder()
{
  const QString start = (m_watcher && m_watcher->watching()) ? m_watcher->folder() : QDir::homePath();
  const QString folder = QFileDialog::getExistingDirectory(this, tr("Choose the folder with your scripts"), start);
  if (!folder.isEmpty()) watchFolder(folder);
}

void ScriptLabPanel::openInEditor()
{
  if (!m_watcher) return;
  const QString path = m_watcher->absolutePath(m_shownFile);
  if (path.isEmpty()) return;
  // the system's own program for .lsl files; nothing is written here
  if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
    setLiveStatus(tr("Error: no program on this computer opens .lsl files. Open %1 in your editor yourself, "
                     "or choose a program for .lsl files in the system settings.").arg(QDir::toNativeSeparators(path)), 2);
}

void ScriptLabPanel::showProblem(int index)
{
  if (index < 0 || index >= m_shownProblems.size()) return;
  const QJsonObject problem = m_shownProblems.at(index);
  const QTextBlock block = m_scriptView->document()->findBlockByNumber(problem.value("line").toInt() - 1);
  if (!block.isValid()) return;
  QTextCursor cursor(block);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qBound(0, problem.value("column").toInt() - 1, block.length() - 1));
  m_scriptView->setTextCursor(cursor);
  m_scriptView->centerCursor();
  m_scriptView->setFocus(Qt::OtherFocusReason);
}

void ScriptLabPanel::setLiveStatus(const QString& text, int tone)
{
  m_liveStatusPlain = text;
  m_liveTone = tone;
  styleLiveStatus();
}

void ScriptLabPanel::styleLiveStatus()
{
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  const int tone = m_liveTone;
  const QString& text = m_liveStatusPlain;
  // the tone's colour: success, danger, or the border token while nothing ran (the mockup's .idle)
  const QColor colour = tone == 1 ? successColour(dark) : tone == 2 ? dangerColour(dark) : QColor(QLatin1String(t.border));
  const QColor surface = Theme::color(Theme::Token::Surface, dark);
  m_liveStatusFrame->setLook(colour, Theme::mix(surface, colour, 0.10), RadiusControl, false);
  m_liveStatusFrame->setAccessibleName(text);
  const QString icon = tone == 1 ? QStringLiteral("circle-check") : tone == 2 ? QStringLiteral("circle-x") : QStringLiteral("info");
  m_liveStatusIcon->setPixmap(LucideIcons::icon(icon, tone == 0 ? mutedColour(dark) : colour, QColor(QLatin1String(t.textDisabled))).pixmap(16, 16));
  // the word before the first colon in bold and in the tone's colour, the rest in the text colour;
  // while nothing ran the whole line is muted
  QPalette p = m_liveStatus->palette();
  p.setColor(QPalette::WindowText, tone == 1 ? successColour(dark) : tone == 2 ? dangerColour(dark) : mutedColour(dark));
  m_liveStatus->setPalette(p);
  const int colon = text.indexOf(QLatin1Char(':'));
  if (tone == 0 || colon <= 0)
  {
    m_liveStatus->setText(text.toHtmlEscaped());
    return;
  }
  m_liveStatus->setText(QStringLiteral("<b>%1</b><span style=\"color:%2\">%3</span>")   // tr-ok: markup, the words are translated above
                          .arg(text.left(colon + 1).toHtmlEscaped(), QLatin1String(t.text), text.mid(colon + 1).toHtmlEscaped()));
}

bool ScriptLabPanel::runAgain()
{
  if (!m_watcher || !m_watcher->watching() || m_shownFile.isEmpty() || m_watcher->absolutePath(m_shownFile).isEmpty()) return false;
  m_watcher->runScript(m_shownFile);   // reads the saved file again; the result comes through resultChanged
  return true;
}

QString ScriptLabPanel::logLine(const QJsonObject& entry)
{
  const QString type = entry.value("type").toString();
  const QString time = QString::number(entry.value("t").toDouble(), 'f', 3) + QStringLiteral(" s  ");
  if (type == QLatin1String("owner_say")) return time + QStringLiteral("owner_say: ") + entry.value("text").toString();
  if (type == QLatin1String("say") || type == QLatin1String("whisper") || type == QLatin1String("shout") || type == QLatin1String("region_say"))
    return time + QStringLiteral("%1 on channel %2: %3").arg(type, fieldText(entry.value("channel")), entry.value("text").toString());
  if (type == QLatin1String("state")) return time + QStringLiteral("state: ") + entry.value("name").toString();
  if (type == QLatin1String("event"))
    return time + QStringLiteral("event: %1(%2)").arg(entry.value("name").toString(), fieldText(entry.value("args")));
  if (type == QLatin1String("error") || type == QLatin1String("limit") || type == QLatin1String("unsupported"))
    return time + QStringLiteral("%1 at line %2, column %3: %4").arg(type, fieldText(entry.value("line")),
                                                                      fieldText(entry.value("column")), entry.value("message").toString());
  if (type == QLatin1String("rlv_command"))
  {
    QString line = time + QStringLiteral("rlv_command: ") + entry.value("text").toString();
    if (entry.contains("error")) line += QStringLiteral(" (refused: %1)").arg(entry.value("error").toString());
    for (const QJsonValue& reply : entry.value("replies").toArray())
      line += QStringLiteral("; reply on channel %1: %2").arg(fieldText(reply.toObject().value("channel")), reply.toObject().value("text").toString());
    return line;
  }
  // every other entry: its fields in a fixed order
  QStringList parts;
  const QStringList keys = entry.keys();
  for (const QString& key : keys)
    if (key != QLatin1String("t") && key != QLatin1String("type")) parts << key + QStringLiteral("=") + fieldText(entry.value(key));
  return time + type + (parts.isEmpty() ? QString() : QStringLiteral(": ") + parts.join(QStringLiteral(", ")));
}

void ScriptLabPanel::refreshLive()
{
  if (!m_liveStatus) return;
  const bool dark = isDark(palette());
  const bool watching = m_watcher && m_watcher->watching();

  m_liveFolder->setText(watching ? QDir::toNativeSeparators(m_watcher->folder()) : QString());
  // a folder watch that just started (here or through lab_watch) brings the Live page to the front
  if (watching && !m_wasWatching) m_tabs->setCurrentIndex(LivePage);
  m_wasWatching = watching;
  m_unwatch->setEnabled(watching);
  m_unwatch->setToolTip(watching ? tr("Stop running the scripts when they change")
                                 : tr("Not watching a folder. Choose one with Watch folder first."));
  const QString examples = exampleScriptsFolder();
  const bool haveExamples = !examples.isEmpty() && QFileInfo(examples).isDir();
  m_example->setEnabled(haveExamples);
  m_example->setToolTip(haveExamples ? tr("Watch the example scripts that come with the program. One runs at once.")
                                     : tr("The example scripts are not next to the program."));

  // the empty state while no folder is watched; the folder, status, script and its parts while watching
  m_liveEmpty->setVisible(!watching);
  for (QWidget* w : QList<QWidget*>{m_liveFolderLabel, m_liveFolder, m_unwatch, m_runAgain, m_openEditor,
                                    m_scriptSection, m_problemsSection, m_logSection, m_liveRestrictionsSection})
    w->setVisible(watching);
  updateWorldParts();

  const QJsonObject last = watching ? m_watcher->lastResult() : QJsonObject();
  const QString file = last.value("file").toString();

  // the script view: read the file again only when another file is shown
  // or the file was run again
  ScriptView* view = static_cast<ScriptView*>(m_scriptView);
  QString text;
  if (last.isEmpty())
  {
    m_shownFile.clear();
    view->clear();
  }
  else
  {
    m_shownFile = file;
    QFile in(m_watcher->absolutePath(file));
    if (QFileInfo(in.fileName()).size() <= LabWatcher::MAX_FILE_BYTES && in.open(QIODevice::ReadOnly))
      text = QString::fromUtf8(in.read(LabWatcher::MAX_FILE_BYTES));
    if (view->toPlainText() != text) view->setPlainText(text);
  }
  m_scriptSection->setTitle(file.isEmpty() ? tr("Script") : tr("Script: %1").arg(file));
  m_scriptSection->setCount(tr("read only"));
  m_openEditor->setEnabled(!m_shownFile.isEmpty());
  // Run again needs the shown script still in the folder
  const bool canRunAgain = !m_shownFile.isEmpty() && !m_watcher->absolutePath(m_shownFile).isEmpty();
  m_runAgain->setEnabled(canRunAgain);
  m_runAgain->setToolTip(m_shownFile.isEmpty() ? tr("No script yet. Watch a folder and save a .lsl file in it first.")
                         : canRunAgain ? tr("Runs the last saved script again from the start, with a fresh clock.")
                                       : tr("The script is not in the folder any more."));
  m_openEditor->setToolTip(m_shownFile.isEmpty() ? tr("No script yet. Watch a folder and save a .lsl file in it first.")
                                                 : tr("Open the script in the program your computer uses for .lsl files"));

  // errors and warnings, with marks in the script
  m_problems->clear();
  m_shownProblems.clear();
  QSet<int> errorLines;
  QSet<QString> seen;
  int errorCount = 0;
  int warningCount = 0;
  QList<QTextEdit::ExtraSelection> marks;
  const auto addProblems = [&](const QJsonArray& list, bool error)
  {
    for (const QJsonValue& value : list)
    {
      const QJsonObject p = value.toObject();
      const int line = p.value("line").toInt();
      const int column = p.value("column").toInt();
      const QString itemText = (error ? tr("Error, line %1, column %2: %3") : tr("Warning, line %1, column %2: %3"))
                                 .arg(line).arg(column).arg(p.value("message").toString());
      if (seen.contains(itemText)) continue;   // a lint warning the checker already gave
      seen.insert(itemText);
      QListWidgetItem* item = new QListWidgetItem(itemText, m_problems);
      item->setForeground(error ? dangerColour(dark) : QColor(QString::fromLatin1(tokens(dark).warning)));
      m_shownProblems << p;
      if (error) { errorLines.insert(line); ++errorCount; }
      else ++warningCount;
      const QTextBlock block = view->document()->findBlockByNumber(line - 1);
      if (block.isValid())
      {
        QTextEdit::ExtraSelection mark;
        mark.cursor = QTextCursor(block);
        mark.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        mark.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        mark.format.setUnderlineColor(error ? dangerColour(dark) : QColor(QString::fromLatin1(tokens(dark).warning)));
        mark.format.setToolTip(item->text());
        marks << mark;
      }
    }
  };
  addProblems(last.value("errors").toArray(), true);
  // a run that a runtime error or a limit stopped also marks its line
  const QJsonObject run = last.value("result").toObject();
  if (run.value("stopped").toBool())
  {
    const QJsonArray log = run.value("log").toArray();
    for (const QJsonValue& value : log)
    {
      const QJsonObject entry = value.toObject();
      const QString type = entry.value("type").toString();
      if (type == QLatin1String("error") || type == QLatin1String("limit") || type == QLatin1String("unsupported"))
        addProblems(QJsonArray{QJsonObject{{"line", entry.value("line")}, {"column", entry.value("column")},
                                           {"message", entry.value("message")}}}, true);
    }
  }
  addProblems(last.value("warnings").toArray(), false);
  // the static warnings (lsl_lint) of a script that compiled: valid LSL that is most likely a mistake
  if (last.value("valid").toBool() && !text.isEmpty()) addProblems(lintWarnings(text), false);
  view->setExtraSelections(marks);
  view->setMarks(errorLines, dangerColour(dark), mutedColour(dark));
  QStringList problemCount;
  if (errorCount) problemCount << (errorCount == 1 ? tr("1 error") : tr("%1 errors").arg(errorCount));
  if (warningCount) problemCount << (warningCount == 1 ? tr("1 warning") : tr("%1 warnings").arg(warningCount));
  m_problemsSection->setCount(problemCount.isEmpty() ? tr("none") : problemCount.join(QStringLiteral(", ")));

  // the event log of the run
  QStringList lines;
  const QJsonArray runLog = run.value("log").toArray();
  for (const QJsonValue& value : runLog) lines << logLine(value.toObject());
  if (run.value("log_truncated").toBool()) lines << tr("(the log was cut here: the run wrote more than it keeps)");
  const QString logText = lines.join(QLatin1Char('\n'));
  if (m_runLog->toPlainText() != logText) m_runLog->setPlainText(logText);
  if (last.isEmpty()) m_logSection->setCount(tr("none"));
  else if (!last.contains("result")) m_logSection->setCount(tr("not run"));
  else m_logSection->setCount(runLog.size() == 1 ? tr("1 line") : tr("%1 lines").arg(runLog.size()));

  // the status line, a word first; with no folder the empty state says what to do
  m_liveStatus->setVisible(watching || !m_refusal.isEmpty());
  m_liveStatusFrame->setVisible(watching || !m_refusal.isEmpty());
  if (!m_refusal.isEmpty()) { setLiveStatus(m_refusal, 2); return; }
  if (!watching)
  {
    setLiveStatus(tr("No folder is watched. Choose the folder with your .lsl scripts: each time you save one, it is checked and run again here."), 0);
    return;
  }
  if (last.isEmpty())
  {
    setLiveStatus(m_watcher->scripts().isEmpty()
                    ? tr("Watching. No .lsl file in this folder yet: save one here and it runs at once.")
                    : tr("Watching %n script(s). Save one to run it.", nullptr, int(m_watcher->scripts().size())), 0);
    return;
  }
  const int runNumber = last.value("run").toInt();
  if (last.contains("read_error"))
  {
    setLiveStatus(tr("Error: %1 was not run (run %2). %3").arg(file).arg(runNumber).arg(last.value("read_error").toString()), 2);
    return;
  }
  const int errors = last.value("errors").toArray().size();
  if (errors > 0)
  {
    setLiveStatus(tr("Error: %1 has %n error(s) and was not run (run %2). Fix the line shown below and save.", nullptr, errors)
                    .arg(file).arg(runNumber), 2);
    return;
  }
  QString rlvText;
  const QJsonArray held = run.value("rlv").toObject().value("state").toObject().value("restrictions").toArray();
  // only for a script that sent RLV lines (or left restrictions held)
  if (run.value("rlv").toObject().value("lines").toInt() > 0 || !held.isEmpty())
  {
    QStringList names;
    for (const QJsonValue& r : held)
    {
      const QJsonObject o = r.toObject();
      names << (o.value("option").toString().isEmpty() ? o.value("behaviour").toString()
                                                       : o.value("behaviour").toString() + QLatin1Char(':') + o.value("option").toString());
    }
    rlvText = names.isEmpty() ? tr(" No RLV restrictions at the end.") : tr(" RLV restrictions at the end: %1.").arg(names.join(QStringLiteral(", ")));
  }
  if (run.value("stopped").toBool())
  {
    setLiveStatus(tr("Error: %1 ran and was stopped (run %2): %3").arg(file).arg(runNumber).arg(run.value("stop_reason").toString()) + rlvText, 2);
    return;
  }
  setLiveStatus(tr("OK: %1 ran without errors (run %2), %n log line(s).", nullptr, int(run.value("log").toArray().size()))
                  .arg(file).arg(runNumber) + rlvText, 1);
}

// --- Script Lab step 7 (issue 293): effects in the 3D view

void ScriptLabPanel::setEffects(EffectsPreview* preview)
{
  for (const QMetaObject::Connection& c : m_effectsConnections) disconnect(c);
  m_effectsConnections.clear();
  m_effects = preview;
  if (preview)
  {
    preview->setEnabled(m_effectsBox && m_effectsBox->isChecked());
    m_effectsConnections << connect(preview, &EffectsPreview::changed, this, &ScriptLabPanel::refreshEffects);
  }
  connectEffectsToWatcher();
  refreshEffects();
}

void ScriptLabPanel::connectEffectsToWatcher()
{
  for (const QMetaObject::Connection& c : m_effectsWatcherConnections) disconnect(c);
  m_effectsWatcherConnections.clear();
  // every new Live run (and the end of a watch), and every lsl_run and lsl_event, goes to the
  // preview, which does nothing while it is off; the newest of the two is shown when it is ticked
  if (!m_effects) return;
  if (m_watcher)
  {
    auto show = [this]() { m_effectsFromSession = false; if (m_effects) m_effects->showLatest(m_watcher); };
    m_effectsWatcherConnections << connect(m_watcher, &LabWatcher::resultChanged, this, show);
    m_effectsWatcherConnections << connect(m_watcher, &LabWatcher::watchChanged, this, show);
  }
  if (m_session)
  {
    auto show = [this]() { m_effectsFromSession = true; if (m_effects) m_effects->showSession(m_session); };
    m_effectsWatcherConnections << connect(m_session, &lsl::Session::changed, this, show);
  }
}

void ScriptLabPanel::effectsToggled(bool on)
{
  if (!m_effects) return;
  m_effects->setEnabled(on);
  // ticked: show the newest run at once, without waiting for the next save
  if (on && m_effectsFromSession) m_effects->showSession(m_session);
  else if (on) m_effects->showLatest(m_watcher);
  refreshEffects();
}

void ScriptLabPanel::refreshEffects()
{
  if (!m_effectsStatus) return;
  const QStringList lines = m_effects && m_effects->enabled() ? m_effects->lines() : QStringList();
  m_effectsStatus->setText(lines.join(QLatin1Char('\n')));
  m_effectsStatus->setVisible(!lines.isEmpty());
  // an error starts with the word Error; the danger colour only backs it up (P10)
  const bool dark = isDark(palette());
  QPalette p = m_effectsStatus->palette();
  p.setColor(QPalette::WindowText, m_effects && m_effects->hasError() ? dangerColour(dark) : mutedColour(dark));
  m_effectsStatus->setPalette(p);
}

// --- Job 7b: camera limits in the 3D view

void ScriptLabPanel::setCameraOverride(CameraOverride* cameraOverride)
{
  if (m_cameraConnection) disconnect(m_cameraConnection);
  m_camera = cameraOverride;
  if (cameraOverride)
  {
    cameraOverride->setEnabled(m_cameraBox->isChecked());
    m_cameraConnection = connect(cameraOverride, &CameraOverride::changed, this, &ScriptLabPanel::refreshCamera);
  }
  refreshCamera();
}

void ScriptLabPanel::refreshCamera()
{
  if (!m_cameraStatus) return;
  const QStringList lines = m_camera ? m_camera->lines() : QStringList();
  m_cameraStatus->setText(lines.join(QLatin1Char('\n')));
  m_cameraStatus->setVisible(!lines.isEmpty());
  // muted text; the words say everything, no colour carries meaning here (P10)
  QPalette p = m_cameraStatus->palette();
  p.setColor(QPalette::WindowText, mutedColour(isDark(palette())));
  m_cameraStatus->setPalette(p);
}

// --- Job 7c: sky and draw distance in the 3D view

void ScriptLabPanel::setEnvironmentOverride(EnvironmentOverride* environmentOverride)
{
  if (m_environmentConnection) disconnect(m_environmentConnection);
  m_environment = environmentOverride;
  if (environmentOverride)
  {
    environmentOverride->setEnabled(m_environmentBox->isChecked());
    m_environmentConnection = connect(environmentOverride, &EnvironmentOverride::changed, this, &ScriptLabPanel::refreshEnvironment);
  }
  refreshEnvironment();
}

void ScriptLabPanel::refreshEnvironment()
{
  if (!m_environmentStatus) return;
  const QStringList lines = m_environment ? m_environment->lines() : QStringList();
  m_environmentStatus->setText(lines.join(QLatin1Char('\n')));
  m_environmentStatus->setVisible(!lines.isEmpty());
  // muted text; the words say everything, no colour carries meaning here (P10)
  QPalette p = m_environmentStatus->palette();
  p.setColor(QPalette::WindowText, mutedColour(isDark(palette())));
  m_environmentStatus->setPalette(p);
}

// --- Job 7c: view modes in the 3D view

void ScriptLabPanel::setViewModesOverride(ViewModesOverride* viewModesOverride)
{
  if (m_viewModesConnection) disconnect(m_viewModesConnection);
  m_viewModes = viewModesOverride;
  if (viewModesOverride)
  {
    viewModesOverride->setEnabled(m_viewModesBox->isChecked());
    m_viewModesConnection = connect(viewModesOverride, &ViewModesOverride::changed, this, &ScriptLabPanel::refreshViewModes);
  }
  refreshViewModes();
}

void ScriptLabPanel::refreshViewModes()
{
  if (!m_viewModesStatus) return;
  const QStringList lines = m_viewModes ? m_viewModes->lines() : QStringList();
  m_viewModesStatus->setText(lines.join(QLatin1Char('\n')));
  m_viewModesStatus->setVisible(!lines.isEmpty());
  // muted text; the words say everything, no colour carries meaning here (P10)
  QPalette p = m_viewModesStatus->palette();
  p.setColor(QPalette::WindowText, mutedColour(isDark(palette())));
  m_viewModesStatus->setPalette(p);
}

// --- Input buttons (issue 308): touch, chat and dialog answers for the world of the last run

namespace
{
  // the section's keys for the dialog the buttons were made for
  QString dialogKey(const QJsonObject& dialog)
  {
    if (dialog.isEmpty()) return QString();
    QStringList parts{dialog.value("object").toString(), dialog.value("message").toString(), QString::number(dialog.value("channel").toInt())};
    for (const QJsonValue& b : dialog.value("buttons").toArray()) parts << b.toString();
    return parts.join(QChar(0x1f));
  }
}

void ScriptLabPanel::buildInputSection(QWidget* page, QVBoxLayout* layout)
{
  // the section's header says "Send input"
  m_inputHint = new QLabel(page);
  m_inputHint->setObjectName("scriptLabInputHint");
  m_inputHint->setWordWrap(true);
  layout->addWidget(m_inputHint);

  QLabel* objectLabel = new QLabel(tr("To object"), page);
  m_inputObject = new QComboBox(page);
  m_inputObject->setObjectName("scriptLabInputObject");
  m_inputObject->setMinimumHeight(ControlHeight);
  m_inputObject->setToolTip(tr("The object that is touched. Chat is heard by every object within range of the avatar."));
  objectLabel->setBuddy(m_inputObject);

  m_touch = new QPushButton(tr("Touch"), page);
  m_touch->setObjectName("scriptLabTouch");
  m_touch->setMinimumHeight(ControlHeight);

  QLabel* channelLabel = new QLabel(tr("Channel"), page);
  m_chatChannel = new QSpinBox(page);
  m_chatChannel->setObjectName("scriptLabChatChannel");
  m_chatChannel->setRange(-2147483647 - 1, 2147483647);
  m_chatChannel->setMinimumHeight(ControlHeight);
  m_chatChannel->setToolTip(tr("The chat channel. 0 is open chat; scripts and HUDs usually listen on a number."));
  channelLabel->setBuddy(m_chatChannel);

  QLabel* nameLabel = new QLabel(tr("Name"), page);
  m_chatName = new QLineEdit(page);
  m_chatName->setObjectName("scriptLabChatName");
  m_chatName->setMinimumHeight(ControlHeight);
  m_chatName->setPlaceholderText(tr("Owner"));
  m_chatName->setToolTip(tr("Who says it. Leave empty for the owner's avatar."));
  nameLabel->setBuddy(m_chatName);

  QLabel* textLabel = new QLabel(tr("Text"), page);
  m_chatText = new QLineEdit(page);
  m_chatText->setObjectName("scriptLabChatText");
  m_chatText->setMinimumHeight(ControlHeight);
  m_chatText->setClearButtonEnabled(true);
  m_chatText->setToolTip(tr("The chat line to say. Press Enter to say it."));
  textLabel->setBuddy(m_chatText);

  m_say = new QPushButton(tr("Say"), page);
  m_say->setObjectName("scriptLabSay");
  m_say->setMinimumHeight(ControlHeight);

  QGridLayout* grid = new QGridLayout;
  grid->setHorizontalSpacing(SpacingS);
  grid->setVerticalSpacing(SpacingS);
  grid->addWidget(objectLabel, 0, 0);
  grid->addWidget(m_inputObject, 0, 1);
  grid->addWidget(m_touch, 1, 1, Qt::AlignLeft);
  grid->addWidget(channelLabel, 2, 0);
  grid->addWidget(m_chatChannel, 2, 1);
  grid->addWidget(nameLabel, 3, 0);
  grid->addWidget(m_chatName, 3, 1);
  grid->addWidget(textLabel, 4, 0);
  grid->addWidget(m_chatText, 4, 1);
  grid->addWidget(m_say, 5, 1, Qt::AlignLeft);
  grid->setColumnStretch(1, 1);
  layout->addLayout(grid);

  m_inputResult = new QLabel(page);
  m_inputResult->setObjectName("scriptLabInputResult");
  m_inputResult->setWordWrap(true);
  m_inputResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_inputResult->hide();
  layout->addWidget(m_inputResult);

  m_inputLog = new QPlainTextEdit(page);
  m_inputLog->setObjectName("scriptLabInputLog");
  m_inputLog->setReadOnly(true);
  m_inputLog->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  m_inputLog->setMaximumBlockCount(LogLines);
  m_inputLog->setToolTip(tr("What each input sent from here made the scripts do. Times are simulated seconds."));
  m_inputLog->setMinimumHeight(3 * m_inputLog->fontMetrics().height() + 2 * SpacingS);
  layout->addWidget(m_inputLog, 2);

  connect(m_touch, &QPushButton::clicked, this, [this]() { sendTouch(m_inputObject->currentText()); });
  auto say = [this]() { sendChat(m_chatChannel->value(), m_chatName->text(), m_chatText->text()); };
  connect(m_say, &QPushButton::clicked, this, say);
  connect(m_chatText, &QLineEdit::returnPressed, this, say);
  connect(m_chatText, &QLineEdit::textChanged, this, [this]() { refreshInput(); });
}

QList<QPushButton*> ScriptLabPanel::dialogButtons() const
{
  QList<QPushButton*> out;
  if (m_dialogButtonBox) out = m_dialogButtonBox->findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
  return out;
}

QString ScriptLabPanel::inputBlocker() const
{
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  if (!sandbox) return tr("Nothing to send to yet: no script has run. An AI assistant starts one with lsl_run; the buttons work then.");
  if (sandbox->allStopped()) return tr("Every script has stopped, so nothing can hear an input. Run the script again.");
  return QString();
}

void ScriptLabPanel::setInputResult(const QString& text, int tone)
{
  m_inputTone = tone;
  m_inputResult->setText(text);
  m_inputResult->setVisible(!text.isEmpty());
  const bool dark = isDark(palette());
  QPalette p = m_inputResult->palette();
  p.setColor(QPalette::WindowText, tone == 1 ? successColour(dark) : tone == 2 ? dangerColour(dark) : mutedColour(dark));
  m_inputResult->setPalette(p);
}

void ScriptLabPanel::refreshInput()
{
  if (!m_inputHint) return;
  const bool dark = isDark(palette());
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  const QString blocker = inputBlocker();
  const bool world = blocker.isEmpty();

  // the objects, keeping the one chosen
  const QString chosen = m_inputObject->currentText();
  QStringList names;
  if (sandbox)
    for (const QJsonValue& o : sandbox->objectsJson()) names << o.toObject().value("name").toString();
  m_objectsSection->setCount(QString::number(names.size()));
  updateWorldParts();
  bool same = m_inputObject->count() == names.size();
  for (int i = 0; same && i < names.size(); ++i) same = m_inputObject->itemText(i) == names[i];
  if (!same)
  {
    m_inputObject->clear();
    m_inputObject->addItems(names);
    const int keep = names.indexOf(chosen);
    if (keep > 0) m_inputObject->setCurrentIndex(keep);
  }

  const bool hasText = !m_chatText->text().isEmpty();
  m_inputObject->setEnabled(world);
  m_touch->setEnabled(world);
  m_say->setEnabled(world && hasText);
  QString reason = blocker;
  if (world && !hasText) reason = tr("Say is off: type the chat text first. Touch and the dialog buttons work.");
  m_touch->setToolTip(world ? tr("Touch the object, as an avatar clicking it") : blocker);
  m_say->setToolTip(!world ? blocker : hasText ? tr("Say the line on the channel (Enter)") : tr("Type the chat text first"));
  m_inputHint->setText(reason.isEmpty() ? tr("Ready: input goes to the world of the last run, which then runs one more simulated second.") : reason);
  QPalette hint = m_inputHint->palette();
  hint.setColor(QPalette::WindowText, mutedColour(dark));
  m_inputHint->setPalette(hint);
  setInputResult(m_inputResult->text(), m_inputTone);

  // the newest dialog, with one button for each of its buttons
  const QJsonObject dialog = sandbox ? sandbox->lastDialog() : QJsonObject();
  QStringList buttons;
  for (const QJsonValue& b : dialog.value("buttons").toArray()) buttons << b.toString();
  const QString key = dialogKey(dialog);
  if (key != m_shownDialogKey)
  {
    m_shownDialogKey = key;
    for (QPushButton* old : dialogButtons())
    {
      m_dialogGrid->removeWidget(old);
      old->deleteLater();
      old->setParent(nullptr);
    }
    for (int i = 0; i < buttons.size(); ++i)
    {
      QPushButton* button = new QPushButton(QString(buttons[i]).replace(QLatin1Char('&'), QStringLiteral("&&")), m_dialogButtonBox);
      button->setObjectName("scriptLabDialogButton");
      button->setProperty("button", buttons[i]);
      button->setMinimumHeight(ControlHeight);
      connect(button, &QPushButton::clicked, this, [this, text = buttons[i]]() { answerDialog(text); });
      m_dialogGrid->addWidget(button, i / 3, i % 3);
    }
    applyStyle();
  }
  if (dialog.isEmpty()) m_inputDialog->setText(tr("No dialog yet. When a script shows one, its buttons appear here."));
  else m_inputDialog->setText(tr("Dialog from %1").arg(dialog.value("object").toString()));
  m_inputDialogMessage->setText(dialog.isEmpty() ? QString()
                                                 : tr("\"%1\" on channel %2").arg(dialog.value("message").toString())
                                                     .arg(dialog.value("channel").toInt()));
  m_inputDialogMessage->setVisible(!dialog.isEmpty());
  QFont dialogFont = m_inputDialog->font();   // the "label" token for a dialog's heading, body text without one
  dialogFont.setWeight(dialog.isEmpty() ? QFont::Normal : QFont::DemiBold);
  m_inputDialog->setFont(dialogFont);
  {
    // the dialog's buttons come after the objects list and before Send input in the tab order
    QWidget* before = m_objectsView->list();
    for (QPushButton* b : dialogButtons()) { setTabOrder(before, b); before = b; }
    setTabOrder(before, m_inputSection->header());
  }
  for (QPushButton* b : dialogButtons())
  {
    b->setEnabled(world);
    b->setToolTip(world ? tr("Press this button in the dialog, as the owner's avatar (channel %1)").arg(dialog.value("channel").toInt()) : blocker);
  }
  m_dialogButtonBox->setVisible(!buttons.isEmpty());
}

bool ScriptLabPanel::sendInput(QJsonObject request, const QString& what)
{
  if (!m_session)
  {
    setInputResult(tr("Error: %1 was not sent. No script world is connected to this panel.").arg(what), 2);
    return false;
  }
  request["command"] = QStringLiteral("lsl_event");
  const QJsonObject answer = lsl::Control::event(request, m_session);   // the same code as the control command
  if (!answer.value("ok").toBool())
  {
    const QString error = answer.value("error").toString();
    setInputResult(tr("Error: %1 was not sent. %2").arg(what, error), 2);
    m_inputLog->appendPlainText(tr("> %1\nError: %2").arg(what, error));
    return false;
  }
  QStringList lines{tr("> %1").arg(what)};
  for (const QJsonValue& value : answer.value("log").toArray()) lines << logLine(value.toObject());
  if (answer.value("log_truncated").toBool()) lines << tr("(the log was cut here: the input made more lines than it keeps)");
  m_inputLog->appendPlainText(lines.join(QLatin1Char('\n')));
  setInputResult(tr("Sent: %1 (%n log line(s)).", nullptr, int(answer.value("log").toArray().size())).arg(what), 1);
  return true;
}

bool ScriptLabPanel::sendTouch(const QString& object)
{
  QJsonObject request{{"type", "touch"}};
  if (!object.isEmpty()) request["object"] = object;
  return sendInput(request, object.isEmpty() ? tr("Touch") : tr("Touch on %1").arg(object));
}

bool ScriptLabPanel::sendChat(int channel, const QString& name, const QString& text)
{
  const QString who = name.isEmpty() ? tr("Owner") : name;
  const QString what = tr("Chat on channel %1 from %2: %3").arg(channel).arg(who, text);
  if (text.isEmpty())
  {
    setInputResult(tr("Error: %1 was not sent. Type the chat text first.").arg(tr("Chat")), 2);
    return false;
  }
  QJsonObject request{{"type", "chat"}, {"channel", channel}, {"text", text}};
  if (!name.isEmpty()) request["name"] = name;
  return sendInput(request, what);
}

bool ScriptLabPanel::answerDialog(const QString& button)
{
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  const QJsonObject dialog = sandbox ? sandbox->lastDialog() : QJsonObject();
  if (dialog.isEmpty())
  {
    setInputResult(tr("Error: the dialog answer was not sent. No script has shown a dialog yet."), 2);
    return false;
  }
  QJsonObject request{{"type", "dialog"}, {"object", dialog.value("object").toString()}, {"button", button}};
  return sendInput(request, tr("Dialog answer \"%1\" to %2").arg(button, dialog.value("object").toString()));
}
