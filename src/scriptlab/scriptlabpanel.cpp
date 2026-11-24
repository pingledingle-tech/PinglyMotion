/*
 * PinglyMotion Script Lab - the Script Lab panel (issue 288)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QAction>
#include <QCheckBox>
#include <QInputDialog>
#include <QScopeGuard>
#include <QTimer>
#include <QGuiApplication>
#include <QKeySequence>
#include <QScreen>
#include <QShortcut>
#include <QResizeEvent>
#include <QSizeGrip>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QTextDocument>
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
#include <QTabBar>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextEdit>
#include <QUrl>
#include <QEvent>
#include <QHideEvent>
#include <QShowEvent>
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
#include <QTreeWidgetItemIterator>
#include <QToolButton>
#include <QVBoxLayout>

#include "effectspreview.h"
#include "labbeginner.h"
#include "labeditor.h"
#include "labtree.h"
#include "channelmapview.h"
#include "labchains.h"
#include "labwatcher.h"
#include "lsl/control.h"
#include "lsl/session.h"
#include "objectsview.h"
#include "paneltitlebar.h"
#include "rlvbehaviours.h"
#include "textsize.h"
#include "rlvstate.h"
#include "scriptlabsection.h"
#include "spherecontrols.h"
#include "lsl/lint.h"
#include "lucideicons.h"
#include "labdialogbox.h"
#include "labplayer.h"
#include "labrestrictionchoice.h"
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
  const Tokens DarkTokens = {"#56D39A", "#FF7B72", "#C93A32", "#AEB2BA", "#3272DD", "#FFFFFF", "#7C818B", "#9CC2FF", "#F2C14E",
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
  setMinimumSize(minimumPanelSize());
  // the grip for a floating panel (not in a layout: it sits in the bottom right corner)
  m_sizeGrip = new QSizeGrip(this);
  m_sizeGrip->setObjectName("scriptLabSizeGrip");
  m_sizeGrip->setToolTip(tr("Drag to change the size of the Script Lab window"));
  m_sizeGrip->hide();

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
  // in a row that wraps (UX small fixes): the chosen RLV or RLVa has a tick and bold text, which
  // need room, and the ? button stands at the end; in a narrow panel the chip and ? go to a second line
  QWidget* rulesRow = new QWidget(this);
  rulesRow->setObjectName("scriptLabRulesRow");
  FlowLayout* rules = new FlowLayout(rulesRow, SpacingS);
  rules->setContentsMargins(SpacingM, SpacingS, SpacingM, 0);
  m_modeLabel = sectionLabel(tr("Viewer rules"), this);
  m_modeLabel->setMinimumHeight(ControlCompact);
  m_modeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_modeLabel->setObjectName("scriptLabModeLabel");
  rules->addWidget(m_modeLabel);
  QWidget* segmentBox = new QWidget(rulesRow);
  segmentBox->setObjectName("scriptLabModeSegment");
  QHBoxLayout* segment = new QHBoxLayout(segmentBox);
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
  rules->addWidget(segmentBox);
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
  // the chip (20 pixels high) in the middle of a box as high as the buttons
  QWidget* chipBox = new QWidget(rulesRow);
  QHBoxLayout* chipRow = new QHBoxLayout(chipBox);
  chipRow->setContentsMargins(0, 0, 0, 0);
  chipRow->addWidget(m_heldChipFrame, 0, Qt::AlignVCenter);
  chipBox->setMinimumHeight(ControlCompact);
  rules->addWidget(chipBox);
  // UX small fixes: help out of the working parts, behind one ? button (labhelpview.h)
  m_helpButton = new QToolButton(this);
  m_helpButton->setObjectName("scriptLabHelpButton");
  m_helpButton->setAutoRaise(true);
  m_helpButton->setFixedSize(ControlCompact, ControlCompact);
  m_helpButton->setIconSize(QSize(16, 16));
  m_helpButton->setAccessibleName(tr("Help for Script Lab"));
  m_helpButton->setToolTip(tr("Help for Script Lab") + QLatin1Char('\n')
                           + tr("Explanations, the words and every example, in a bigger view."));
  rules->addWidget(m_helpButton);
  connect(m_helpButton, &QToolButton::clicked, this, [this]() { showHelp(); });
  outer->addWidget(rulesRow);
  connect(m_modeRlv, &QPushButton::clicked, this, [this]() { setMode(Rlv::Mode::Rlv); });
  connect(m_modeRlva, &QPushButton::clicked, this, [this]() { setMode(Rlv::Mode::Rlva); });

  // job UX-B: one window with its own tabs; each tab can be popped out into a window of its own
  m_tabs = new LabTabWidget(this);
  m_tabs->setObjectName("scriptLabTabs");
  m_tabs->setDocumentMode(true);
  m_tabs->setAccessibleName(tr("Script Lab pages"));
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
  m_actions->setToolTip(tr("What the avatar may do now. Choose Allowed or Blocked, or press Space on a row."));
  for (const Rlv::Action& action : Rlv::actions())
  {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_actions);
    item->setText(0, action.label);
    item->setData(0, Qt::UserRole, action.id);
  }
  m_actions->setMinimumHeight(6 * m_actions->fontMetrics().height() + 2 * SpacingS);
  m_actions->installEventFilter(this);
  m_actions->setCurrentItem(m_actions->topLevelItem(0));
  m_actionsSection->bodyLayout()->addWidget(m_actions);
  buildChoiceBox(m_actionsSection->body(), m_actionsSection->bodyLayout());

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
    Q_UNUSED(on);
    applyViewEffects();
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
  // the vision sphere's sliders (setsphere part 2): folded at start, greyed until the box is ticked
  m_sphereSection = addSection(QStringLiteral("sphere"), tr("Vision sphere"), false, commandsPage, layout);
  m_sphereControls = new SphereControls(m_sphereSection->body());
  m_sphereSection->bodyLayout()->addWidget(m_sphereControls);
  connect(m_environmentBox, &QCheckBox::toggled, this, [this](bool on)
  {
    Q_UNUSED(on);
    applyViewEffects();
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
    Q_UNUSED(on);
    applyViewEffects();
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
  m_pages[CommandsPage] = commandsScroll;

  // the Live page first, as in the mockup; then Type commands, Who talks to whom and Player (job UX-B)
  buildLivePage();
  buildTips();
  for (int p = 0; p < PageCount; ++p) addPageTab(Page(p));
  m_tabs->setCurrentIndex(LivePage);

  // --- job UX-B: Pop out beside the tabs (the tab in front gets a window of its own), dragging a
  // tab out of the bar does the same; Ctrl+Tab and Ctrl+Shift+Tab go round the tabs from anywhere
  // in the window, also from a text box
  m_popOutButton = new QToolButton(m_tabs);
  m_popOutButton->setObjectName("scriptLabPopOut");
  m_popOutButton->setText(tr("Pop out"));
  m_popOutButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_popOutButton->setAutoRaise(true);
  m_popOutButton->setFocusPolicy(Qt::StrongFocus);
  m_popOutButton->setMinimumHeight(ControlCompact);
  m_popOutButton->setAccessibleName(tr("Pop out this tab"));
  m_tabs->setCornerWidget(m_popOutButton, Qt::TopRightCorner);
  connect(m_popOutButton, &QToolButton::clicked, this, [this]() { popOut(currentPage()); });
  connect(m_tabs->labTabBar(), &LabTabBar::popOutRequested, this, [this](int index, const QPoint& at)
  {
    for (int p = 0; p < PageCount; ++p)
      if (m_tabs->widget(index) == m_pages[p]) { popOut(Page(p), at); return; }
  });
  connect(m_tabs, &QTabWidget::currentChanged, this, [this]()
  {
    saveWindowState();
    refreshPopOutButton();
    refreshTips();   // a word tip shows only on the page in front
  });
  auto stepTabs = [this](int step)
  {
    const int n = m_tabs->count();
    if (n > 1) m_tabs->setCurrentIndex((m_tabs->currentIndex() + step + n) % n);
  };
  for (const QKeySequence& keys : {QKeySequence(Qt::CTRL | Qt::Key_Tab), QKeySequence(Qt::CTRL | Qt::Key_PageDown)})
  {
    QShortcut* next = new QShortcut(keys, this);
    next->setContext(Qt::WidgetWithChildrenShortcut);
    connect(next, &QShortcut::activated, this, [stepTabs]() { stepTabs(1); });
  }
  for (const QKeySequence& keys : {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab),
                                   QKeySequence(Qt::CTRL | Qt::Key_PageUp)})
  {
    QShortcut* previous = new QShortcut(keys, this);
    previous->setContext(Qt::WidgetWithChildrenShortcut);
    connect(previous, &QShortcut::activated, this, [stepTabs]() { stepTabs(-1); });
  }

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
  {
    // job A2: then the choice box, in screen order
    QWidget* before = m_actions;
    for (QWidget* w : {static_cast<QWidget*>(m_choiceValue), static_cast<QWidget*>(m_choiceApply), static_cast<QWidget*>(m_useInScript)})
    {
      setTabOrder(before, w);
      before = w;
    }
    setTabOrder(before, m_cameraBox);
  }
  setTabOrder(m_cameraBox, m_environmentBox);
  setTabOrder(m_environmentBox, m_sphereSection->header());
  setTabOrder(m_sphereSection->header(), m_sphereControls->innerSlider());
  setTabOrder(m_sphereControls->clearButton(), m_viewModesBox);
  setTabOrder(m_viewModesBox, m_repliesSection->header());
  setTabOrder(m_repliesSection->header(), m_log);

  addActionChoices();   // job A2
  applyStyle();
  refresh();
  refreshMode();
  refreshLive();
  refreshInput();

  // each part opens or folds as it was last time; a change is saved at once in the settings
  applyPartsState(UserSettings::scriptLabParts());
  m_mapSection->setExpanded(m_expert);   // job A4: folded for beginners, open for experts
  for (ScriptLabSection* s : m_sections)
    connect(s, &ScriptLabSection::expandedChanged, this, [this]() { UserSettings::setScriptLabParts(partsState()); refreshTips(); });
  updateWorkLayout();
  m_chainsReady = true;

  // job UX-B: the tab in front and the popped tabs as last time (their windows show with the panel)
  applyWindowState(UserSettings::scriptLabWindows());
  m_windowStateReady = true;
  refreshPopOutButton();
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
  if (m_editor) m_editor->setHintsOn(on);   // the editor's card on what an RLV line is
  refreshTips();   // the word tips are guidance too (charter P4)
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
  styleModeButtons();
  m_modeRlv->setAccessibleName(tr("Viewer rules: RLV"));
  m_modeRlva->setAccessibleName(tr("Viewer rules: RLVa"));
  // the editor marks commands the chosen viewer does not know (job R3)
  if (m_editor) m_editor->codeEdit()->setRlvMode(rlv ? Rlv::Mode::Rlv : Rlv::Mode::Rlva);
}

// the chosen viewer rules: the selection fill, a tick and bold text; the other has no tick and
// normal weight (DESIGN_SYSTEM.md, "States"; charter P10: not by colour alone). UX small fixes:
// the project owner could not tell whether RLV or RLVa was on.
void ScriptLabPanel::styleModeButtons()
{
  if (!m_modeRlv || !m_modeRlva) return;
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  for (QPushButton* b : {m_modeRlv, m_modeRlva})
  {
    const bool on = b->isChecked();
    QFont font = b->font();
    font.setWeight(on ? QFont::DemiBold : QFont::Normal);
    b->setFont(font);
    b->setIcon(on ? LucideIcons::icon(QStringLiteral("check"), QColor(QLatin1String(t.onAccent)), QColor(QLatin1String(t.textMuted)))
                  : QIcon());
    b->setIconSize(QSize(14, 14));
    b->setProperty("chosen", on);
  }
  // both as wide as the chosen look needs (bold text and the tick), so switching never cuts the
  // word or moves the row (charter P12)
  QFont bold = m_modeRlv->font();
  bold.setWeight(QFont::DemiBold);
  const QFontMetrics fm(bold);
  const int width = qMax(fm.horizontalAdvance(m_modeRlv->text()), fm.horizontalAdvance(m_modeRlva->text()))
                    + 14 + SpacingXs + 2 * SpacingM + 4 * Theme::Size::borderWidth;
  for (QPushButton* b : {m_modeRlv, m_modeRlva}) b->setMinimumWidth(width);
  const QString relog = tr("Switching removes every restriction, as after a relog.");
  for (QPushButton* b : {m_modeRlv, m_modeRlva})
    b->setAccessibleDescription((b->isChecked() ? tr("Chosen.") : tr("Not chosen.")) + QLatin1Char(' ') + relog);
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
  // Live page: Make example project is the main button there (job U3); Watch folder, New folder,
  // Stop watching and Open in editor are normal buttons (surface_raised fill, border, text). Greyed,
  // they keep text_muted and a half border, so the word stays readable at 4.5:1 (job U3: higher
  // contrast than text_disabled; the reason is a line under them too)
  m_makeExample->setStyleSheet(button.arg(QLatin1String(t.accent), QLatin1String(t.onAccent)).arg(RadiusControl).arg(SpacingM)
                               .arg(QLatin1String(t.focusRing)));
  const QColor raised(QLatin1String(t.surfaceRaised));
  const QString halfBorder = Theme::mix(raised, QColor(QLatin1String(t.border)), 0.5).name(QColor::HexRgb).toUpper();
  const QString normal = QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:disabled { color: %6; border: 1px solid %8; }"
    "QPushButton:focus { border: 2px solid %7; }")
    .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border))
    .arg(RadiusControl).arg(SpacingM)
    .arg(QLatin1String(t.textMuted), QLatin1String(t.focusRing), halfBorder);
  m_watch->setStyleSheet(normal);
  m_newFolder->setStyleSheet(normal);
  m_unwatch->setStyleSheet(normal);
  m_openEditor->setStyleSheet(normal);
  m_runAgain->setStyleSheet(normal);
  m_runAgain->setIcon(LucideIcons::icon(QStringLiteral("refresh-cw"), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textDisabled))));
  for (QPushButton* b : m_tryButtons) b->setStyleSheet(normal);
  m_example->setStyleSheet(normal);
  m_watch->setIcon(LucideIcons::icon(QStringLiteral("folder-open"), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textMuted))));
  m_makeExample->setIcon(LucideIcons::icon(QStringLiteral("box"), QColor(QLatin1String(t.onAccent)), QColor(QLatin1String(t.textMuted))));
  styleEffectsButton();
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
  styleModeButtons();
  m_helpButton->setIcon(LucideIcons::icon(QStringLiteral("circle-help"), Theme::color(Theme::Token::AccentText, dark),
                                          QColor(QLatin1String(t.textDisabled))));
  m_helpButton->setStyleSheet(QStringLiteral("QToolButton { border: none; border-radius: %1px; }"
                                             "QToolButton:hover { background: %2; }"
                                             "QToolButton:focus { border: 2px solid %3; }")
                              .arg(RadiusControl).arg(QLatin1String(t.surfaceRaised), QLatin1String(t.focusRing)));
  if (m_helpView) m_helpView->applyStyle(dark);

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
  m_inventoryEmpty->setPalette(muted);
  m_inventoryNote->setPalette(muted);
  m_treeLine->setPalette(muted);
  m_scriptButtonsReason->setPalette(muted);
  m_folderTree->applyStyle(dark);
  m_stepCard->applyStyle(dark);
  for (LabTipCard* tip : m_tipCards) tip->applyStyle(dark);
  stylePageTabs();
  m_mapEmpty->setPalette(muted);
  m_playerEmpty->setPalette(muted);
  for (ScriptLabSection* section : m_sections) section->applyStyle(dark);
  m_sphereControls->applyStyle(dark);
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

void ScriptLabPanel::applyViewEffects()
{
  const bool shown = !m_hiddenByUser;
  if (m_camera) m_camera->setEnabled(shown && m_cameraBox->isChecked());
  if (m_environment) m_environment->setEnabled(shown && m_environmentBox->isChecked());
  if (m_viewModes) m_viewModes->setEnabled(shown && m_viewModesBox->isChecked());
}

void ScriptLabPanel::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);
  if (event->spontaneous()) return;   // the window came back from being minimised
  m_hiddenByUser = false;
  applyViewEffects();
  // job UX-B: without a dock (the tests) the popped tabs follow the panel; in the program they
  // follow the dock's View item (makeDock)
  if (!qobject_cast<QDockWidget*>(parentWidget())) setPopWindowsShown(true);
}

void ScriptLabPanel::hideEvent(QHideEvent* event)
{
  QWidget::hideEvent(event);
  if (event->spontaneous()) return;   // the window was minimised: the panel is still "shown"
  m_hiddenByUser = true;
  applyViewEffects();
  if (!qobject_cast<QDockWidget*>(parentWidget())) setPopWindowsShown(false);
}

void ScriptLabPanel::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  // the text size changed: the sizes follow it
  if (event->type() == QEvent::FontChange)
  {
    setMinimumSize(minimumPanelSize());
    styleModeButtons();   // RLV and RLVa as wide as their bold word needs at the new size
    updateGeometry();
  }
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
  // job A4: a lock or an unlock turns the chains red or green at once
  if (m_chainsReady) updateChains();

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
  const Rlv::Mode mode = m_state ? m_state->mode() : Rlv::Mode::Rlva;
  for (int i = 0; i < m_actions->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* item = m_actions->topLevelItem(i);
    const QString id = item->data(0, Qt::UserRole).toString();
    // job A2: a distance limit ("@touchfar:3=n") blocks the action farther away, so the row says Blocked too
    const LabRestrictionChoice::Choice choice = LabRestrictionChoice::forAction(id, mode);
    const bool ok = allowed.value(id, true) && LabRestrictionChoice::heldDistance(choice, held).isEmpty();
    item->setText(1, ok ? tr("Allowed") : tr("Blocked"));
    item->setData(1, Qt::UserRole, ok);
    item->setForeground(1, ok ? successColour(dark) : dangerColour(dark));
    QFont font = item->font(1);
    font.setWeight(ok ? QFont::Normal : QFont::DemiBold);   // a second signal besides the word
    item->setFont(1, font);
    // the whole row turns red while blocked, and the word says it too (charter P10)
    item->setForeground(0, ok ? m_actions->palette().color(QPalette::Text) : dangerColour(dark));
    item->setFont(0, font);
    if (QComboBox* combo = qobject_cast<QComboBox*>(m_actions->itemWidget(item, 1)))
    {
      combo->setCurrentIndex(ok ? 0 : 1);
      QPalette p = combo->palette();
      p.setColor(QPalette::ButtonText, ok ? successColour(dark) : dangerColour(dark));
      p.setColor(QPalette::Text, ok ? successColour(dark) : dangerColour(dark));
      combo->setPalette(p);
      combo->setFont(font);
      combo->setEnabled(choice.ok());
      combo->setToolTip(choice.ok() ? (ok ? tr("Allowed now. Choose Blocked to make and run the RLV line.")
                                          : tr("Blocked now. Choose Allowed to lift it again."))
                                    : choice.reason);
      combo->setAccessibleDescription(item->text(1));
    }
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
  // job A3: a typed or chosen line is a step of the recipe while recording
  if (m_watcher) m_watcher->recordLine(object, text);
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
  // job UX-B: it docks on any side or floats anywhere (the owner: it can be the main focus)
  dock->setAllowedAreas(Qt::AllDockWidgetAreas);
  dock->setTitleBarWidget(new PanelTitleBar(dock));

  QAction* action = dock->toggleViewAction();
  action->setObjectName(QString::fromLatin1(ViewActionName));
  action->setText(tr("Script Lab"));
  action->setToolTip(tr("Test RLV commands: type a line such as @detach=n and see what the avatar may still do"));
  action->setStatusTip(action->toolTip());

  // hidden until asked for: someone who only animates never sees it
  window->addDockWidget(Qt::RightDockWidgetArea, dock);
  dock->hide();
  // the size grip shows while the panel floats (the custom title bar takes the system frame away)
  connect(dock, &QDockWidget::topLevelChanged, panel, [panel](bool floating) { panel->setFloatingGrip(floating); });
  // job UX-B: big the first time it opens docked and the first time it floats (remembered); the
  // tabs popped out of it show while it is open and hide while it is closed
  // Docked: when the user opens it (View, the top bar, Ctrl+Shift+L), not when a saved layout
  // shows it, after the main window has laid it out (a size given while it is being shown is lost).
  // Floating: when it becomes a window, at once, so a place the user or the saved layout gives
  // right after wins; never while the saved layout is being put back (setLayoutRestoring).
  connect(action, &QAction::triggered, panel, [window, dock](bool open)
  {
    if (open) QTimer::singleShot(0, dock, [window, dock]() { if (!dock->isFloating()) openBig(window, dock); });
  });
  connect(dock, &QDockWidget::topLevelChanged, panel, [window, dock](bool floating)
  {
    if (floating && !s_layoutRestoring) openBig(window, dock);
  });
  connect(action, &QAction::toggled, panel, [panel](bool open) { panel->setPopWindowsShown(open); });
  panel->setPopWindowsShown(action->isChecked());
  return dock;
}

QSize ScriptLabPanel::defaultSize()
{
  // room for the suggestion list with its help and the warnings side by side with the script;
  // job UX-B: and for the four tabs with Pop out beside them, without scroll arrows
  return QSize(TextSize::scaled(600), TextSize::scaled(720));
}

QSize ScriptLabPanel::minimumPanelSize()
{
  return QSize(TextSize::scaled(300), TextSize::scaled(240));
}

void ScriptLabPanel::setFloatingGrip(bool floating)
{
  m_sizeGrip->setVisible(floating);
  placeSizeGrip();
}

void ScriptLabPanel::placeSizeGrip()
{
  if (!m_sizeGrip) return;
  const QSize hint = m_sizeGrip->sizeHint();
  m_sizeGrip->setGeometry(width() - hint.width(), height() - hint.height(), hint.width(), hint.height());
  m_sizeGrip->raise();
}

void ScriptLabPanel::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  placeSizeGrip();
  updateWorkLayout();
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
  // job UX-B: the channel map and record, recipe and player are tabs of their own now
  QWidget* mapPage = new QWidget(m_tabs);
  mapPage->setObjectName("scriptLabMapPage");
  QVBoxLayout* mapLayout = new QVBoxLayout(mapPage);
  mapLayout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  mapLayout->setSpacing(SpacingS);
  m_mapEmpty = new QLabel(tr("Watch a folder as one object on Live scripts. Then this page shows who talks to whom."), mapPage);
  m_mapEmpty->setObjectName("scriptLabMapEmpty");
  m_mapEmpty->setWordWrap(true);
  mapLayout->addWidget(m_mapEmpty);
  QWidget* playerPage = new QWidget(m_tabs);
  playerPage->setObjectName("scriptLabPlayerPage");
  QVBoxLayout* playerLayout = new QVBoxLayout(playerPage);
  playerLayout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  playerLayout->setSpacing(SpacingS);
  m_playerEmpty = new QLabel(tr("Watch a folder as one object on Live scripts. Then press Record what I do here, and watch your steps again."), playerPage);
  m_playerEmpty->setObjectName("scriptLabPlayerEmpty");
  m_playerEmpty->setWordWrap(true);
  playerLayout->addWidget(m_playerEmpty);

  // --- the example's steps (job U3): shown after Make example project and from Help > Script Lab steps
  m_stepCard = new LabStepCard(page);
  m_stepCard->hide();
  layout->addWidget(m_stepCard);
  connect(m_stepCard, &LabStepCard::stepRequested, this, [this](int index) { doStep(index); });
  connect(m_stepCard, &LabStepCard::closeRequested, this, &ScriptLabPanel::closeSteps);

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
  m_liveEmptyText = new QLabel(tr("Script Lab shows your scripts from a folder on your PC. Choose a folder to see them here."), m_liveEmpty);
  m_liveEmptyText->setObjectName("scriptLabLiveEmptyText");
  m_liveEmptyText->setWordWrap(true);
  card->addWidget(m_liveEmptyText);

  // job U3: a finished example first, from one click (value first); then your own folders
  m_makeExample = new QPushButton(tr("Make example project"), m_liveEmpty);
  m_makeExample->setObjectName("scriptLabMakeExample");
  m_makeExample->setMinimumHeight(ControlLarge);
  m_makeExample->setIconSize(QSize(20, 20));
  m_makeExample->setToolTip(tr("Copies a working example to your folder and shows it in 3D."));
  m_newFolder = new QPushButton(tr("New folder"), m_liveEmpty);
  m_newFolder->setObjectName("scriptLabNewFolder");
  m_newFolder->setMinimumHeight(ControlLarge);
  m_newFolder->setToolTip(tr("Makes an empty folder for your scripts and watches it."));
  m_watch = new QPushButton(tr("Watch folder..."), m_liveEmpty);
  m_watch->setObjectName("scriptLabWatch");
  m_watch->setMinimumHeight(ControlLarge);
  m_watch->setIconSize(QSize(20, 20));
  m_watch->setToolTip(tr("Choose the folder with your .lsl scripts. Each time you save one, it is checked and run again here."));
  m_example = new QPushButton(tr("Try the example scripts"), m_liveEmpty);
  m_example->setObjectName("scriptLabTryExamples");
  m_example->setMinimumHeight(ControlLarge);
  m_example->setIconSize(QSize(20, 20));
  // the two buttons in a row that wraps, so the second goes under the first in a narrow docked panel
  QWidget* emptyButtons = new QWidget(m_liveEmpty);
  emptyButtons->setObjectName("scriptLabLiveEmptyButtons");
  FlowLayout* emptyFlow = new FlowLayout(emptyButtons, SpacingS);
  m_makeExample->setParent(emptyButtons);
  m_watch->setParent(emptyButtons);
  m_newFolder->setParent(emptyButtons);
  m_example->setParent(emptyButtons);
  emptyFlow->addWidget(m_makeExample);
  emptyFlow->addWidget(m_watch);
  emptyFlow->addWidget(m_newFolder);
  emptyFlow->addWidget(m_example);
  card->addWidget(emptyButtons);
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
  // why they are greyed, readable without hovering (job U3, charter P6)
  m_scriptButtonsReason = new QLabel(page);
  m_scriptButtonsReason->setObjectName("scriptLabScriptButtonsReason");
  m_scriptButtonsReason->setWordWrap(true);
  m_scriptButtonsReason->setTextFormat(Qt::PlainText);
  m_scriptButtonsReason->hide();
  layout->addWidget(m_scriptButtonsReason);

  // Script Lab step 7 (issue 293): the run's effects in the 3D view, off until ticked
  // a clear on/off button since job U3: on has the selection fill and a check mark
  m_effectsBox = new QPushButton(tr("Show effects in 3D view"), page);
  m_effectsBox->setObjectName("scriptLabShowEffects");
  m_effectsBox->setCheckable(true);
  m_effectsBox->setChecked(false);
  m_effectsBox->setMinimumHeight(ControlHeight);
  m_effectsBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  m_effectsBox->setToolTip(tr("Play the animation the script started on a preview figure and show worn objects and locks. "
                              "Your animation is not changed; untick to see it again."));
  layout->addWidget(m_effectsBox);
  m_effectsStatus = new QLabel(page);
  m_effectsStatus->setObjectName("scriptLabEffectsStatus");
  m_effectsStatus->setWordWrap(true);
  m_effectsStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_effectsStatus->hide();
  layout->addWidget(m_effectsStatus);
  connect(m_effectsBox, &QPushButton::toggled, this, &ScriptLabPanel::effectsToggled);

  // --- job A3: record what I do, and replay it after every save. One button and one line in
  // Beginner mode; the recipe file and Replay now only in Expert mode (charter P5)
  m_recipeRow = new QWidget(playerPage);
  m_recipeRow->setObjectName("scriptLabRecipeRow");
  QVBoxLayout* recipeRow = new QVBoxLayout(m_recipeRow);
  recipeRow->setContentsMargins(0, 0, 0, 0);
  recipeRow->setSpacing(SpacingXs);
  m_recordButton = new QPushButton(tr("Record what I do"), m_recipeRow);
  m_recordButton->setObjectName("scriptLabRecord");
  m_recordButton->setCheckable(true);
  m_recordButton->setMinimumHeight(ControlHeight);
  m_recordButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  m_recordButton->setToolTip(tr("Records your clicks on props, menu buttons and RLV lines. "
                                "After every save, the program repeats them and says what changed."));
  recipeRow->addWidget(m_recordButton);
  m_recipeStatus = new QLabel(m_recipeRow);
  m_recipeStatus->setObjectName("scriptLabRecipeStatus");
  m_recipeStatus->setWordWrap(true);
  m_recipeStatus->setTextFormat(Qt::PlainText);
  m_recipeStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
  recipeRow->addWidget(m_recipeStatus);
  playerLayout->addWidget(m_recipeRow);
  connect(m_recordButton, &QPushButton::clicked, this, [this](bool on) { setRecording(on); });
  m_recipeSection = addSection(QStringLiteral("recipe"), tr("Recipe"), false, playerPage, playerLayout);
  m_recipeFileLine = new QLabel(m_recipeSection->body());
  m_recipeFileLine->setObjectName("scriptLabRecipeFile");
  m_recipeFileLine->setWordWrap(true);
  m_recipeFileLine->setTextFormat(Qt::PlainText);
  m_recipeFileLine->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_recipeSection->bodyLayout()->addWidget(m_recipeFileLine);
  m_recipeText = new QPlainTextEdit(m_recipeSection->body());
  m_recipeText->setObjectName("scriptLabRecipeText");
  m_recipeText->setReadOnly(true);
  m_recipeText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_recipeText->setToolTip(tr("The recipe file. You may edit it in your own editor."));
  m_recipeText->setMinimumHeight(5 * m_recipeText->fontMetrics().height() + 2 * SpacingS);
  m_recipeSection->bodyLayout()->addWidget(m_recipeText);
  m_replayButton = new QPushButton(tr("Replay now"), m_recipeSection->body());
  m_replayButton->setObjectName("scriptLabReplayNow");
  m_replayButton->setMinimumHeight(ControlCompact);
  m_replayButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  m_replayButton->setToolTip(tr("Repeats the recipe's steps now, without a save."));
  m_recipeSection->bodyLayout()->addWidget(m_replayButton);
  connect(m_replayButton, &QPushButton::clicked, this, [this] { replayNow(); });
  // --- job B3: the player, the newest replay as a film; shown in Beginner mode too
  m_player = new LabPlayer(playerPage);
  playerLayout->addWidget(m_player);
  playerLayout->addStretch(1);
  connect(m_player, &LabPlayer::momentChanged, this, [this] { updatePrimProps(); });
  m_expert = UserSettings::expertMode();
  UserSettings::watchExpertMode(this, [this](bool on) { setExpertMode(on); });

  // --- job U3: the tree on the left and the editor on the right when the panel is wide, stacked when narrow
  m_workArea = new QWidget(page);
  m_workArea->setObjectName("scriptLabWorkArea");
  m_workLayout = new QBoxLayout(QBoxLayout::TopToBottom, m_workArea);
  m_workLayout->setContentsMargins(0, 0, 0, 0);
  m_workLayout->setSpacing(SpacingS);
  m_treeColumn = new QWidget(m_workArea);
  m_treeColumn->setObjectName("scriptLabTreeColumn");
  QVBoxLayout* treeColumn = new QVBoxLayout(m_treeColumn);
  treeColumn->setContentsMargins(0, 0, 0, 0);
  treeColumn->setSpacing(SpacingS);
  m_editorColumn = new QWidget(m_workArea);
  m_editorColumn->setObjectName("scriptLabEditorColumn");
  QVBoxLayout* editorColumn = new QVBoxLayout(m_editorColumn);
  editorColumn->setContentsMargins(0, 0, 0, 0);
  editorColumn->setSpacing(SpacingS);
  m_workLayout->addWidget(m_treeColumn);
  m_workLayout->addWidget(m_editorColumn);
  layout->addWidget(m_workArea);

  // --- the folder as one object (lane 1 job D): a tree with icons, colour dots, sums and memory
  m_treeSection = addSection(QStringLiteral("tree"), tr("Object"), true, m_treeColumn, treeColumn);
  // one line on what the tree shows, and the switch that links it with the 3D view (job U3)
  m_treeLine = new QLabel(tr("This tree is your object. Each folder in it is one prim."), m_treeSection->body());
  m_treeLine->setObjectName("scriptLabTreeLine");
  m_treeLine->setWordWrap(true);
  m_treeSection->bodyLayout()->addWidget(m_treeLine);
  m_followBox = new QCheckBox(tr("Follow selection in 3D view"), m_treeSection->body());
  m_followBox->setObjectName("scriptLabFollowSelection");
  m_followBox->setToolTip(tr("Click a piece here to light up its prop in 3D view. "
                             "Click a prop and its row is picked here."));
  m_followBox->setChecked(UserSettings::scriptLabFollowSelection());
  m_treeSection->bodyLayout()->addWidget(m_followBox);
  connect(m_followBox, &QCheckBox::toggled, this, [](bool on) { UserSettings::setScriptLabFollowSelection(on); });
  m_folderTree = new LabFolderTree(m_treeSection->body());
  m_treeSection->bodyLayout()->addWidget(m_folderTree);
  connect(m_folderTree, &LabFolderTree::scriptPicked, this, [this](const QString& file) { showScript(file); });
  // a piece picked in the tree lights up its prop in the 3D view (the name is the link)
  connect(m_folderTree, &LabFolderTree::primPicked, this, [this](const QString& prim)
  {
    if (m_followBox->isChecked() && m_hooks.selectProp) m_hooks.selectProp(prim);
  });

  // the short maker guide ("How to lay out the folder") moved to the help view behind the ? button
  // (UX small fixes, labhelpview.h): help does not sit among the working parts

  // --- the script, read only, with line numbers
  m_scriptSection = addSection(QStringLiteral("script"), tr("Script"), true, m_editorColumn, editorColumn);
  // the script in our own text box (lane 1, job F1): editable, Save runs it (labeditor.h)
  m_editor = new LabEditor(m_scriptSection->body());
  m_scriptView = m_editor->codeEdit();
  m_scriptView->setObjectName("scriptLabScriptView");
  m_editor->setHintsOn(m_hintsOn);
  m_scriptSection->bodyLayout()->addWidget(m_editor);
  connect(m_editor, &LabEditor::modifiedChanged, this, [this](bool)
  {
    // the part's title follows the editor's file (also a new one from an example), the count its unsaved mark
    if (!m_editor->fileName().isEmpty()) m_scriptSection->setTitle(tr("Script: %1").arg(m_editor->fileName()));
    m_scriptSection->setCount(m_editor->unsavedMark());
    m_scriptSection->setVisible((m_watcher && m_watcher->watching()) || m_editor->isModified());
  });
  connect(m_editor, &LabEditor::saved, this, &ScriptLabPanel::runSavedScript);

  // --- errors and warnings with line and column (lint warnings too)
  m_problemsSection = addSection(QStringLiteral("problems"), tr("Errors and warnings"), true, page, layout);
  m_problems = new QListWidget(m_problemsSection->body());
  m_problems->setObjectName("scriptLabProblems");
  m_problems->setToolTip(tr("Click a line, or press Enter, to show it in the script"));
  m_problems->setMinimumHeight(3 * m_problems->fontMetrics().height() + 2 * SpacingS);
  m_problems->setWordWrap(true);
  m_problemsSection->bodyLayout()->addWidget(m_problems);

  // --- what the run did
  m_logSection = addSection(QStringLiteral("log"), tr("Event log"), true, page, layout);
  m_runLog = new QPlainTextEdit(m_logSection->body());
  m_runLog->setObjectName("scriptLabRunLog");
  m_runLog->setReadOnly(true);
  m_runLog->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  m_runLog->setToolTip(tr("What the script did in the last run, in simulated seconds. Click an error or RLV line to show it in the script."));
  m_runLog->setMinimumHeight(6 * m_runLog->fontMetrics().height() + 2 * SpacingS);
  m_logSection->bodyLayout()->addWidget(m_runLog);

  // --- job A4: the channel map, who talks to whom in the run (LabChannelMap). Folded in Beginner
  // mode and open in Expert mode (charter P5); a click on a box picks its prim in the tree, a click
  // on an arrow shows the script line that sent it
  m_mapSection = addSection(QStringLiteral("map"), tr("Who talks to whom"), false, mapPage, mapLayout);
  m_mapLine = new QLabel(tr("Each box is a part. Each arrow is a message, with its channel."), m_mapSection->body());
  m_mapLine->setObjectName("scriptLabChannelMapLine");
  m_mapLine->setWordWrap(true);
  m_mapLine->setTextFormat(Qt::PlainText);
  m_mapSection->bodyLayout()->addWidget(m_mapLine);
  m_mapView = new ChannelMapView(m_mapSection->body());
  m_mapView->setToolTip(tr("Click a box to pick it in the tree. Click an arrow to see the line that sent it."));
  m_mapSection->bodyLayout()->addWidget(m_mapView);
  m_mapProblems = new QLabel(m_mapSection->body());
  m_mapProblems->setObjectName("scriptLabChannelMapProblems");
  m_mapProblems->setWordWrap(true);
  m_mapProblems->setTextFormat(Qt::PlainText);
  m_mapProblems->hide();
  m_mapSection->bodyLayout()->addWidget(m_mapProblems);
  connect(m_mapView, &ChannelMapView::boxActivated, this, [this](int box) { activateMapBox(box); });
  connect(m_mapView, &ChannelMapView::arrowActivated, this, [this](int arrow) { activateMapArrow(arrow); });
  mapLayout->addStretch(1);

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

  // --- the simulated inventory of the object chosen under Send input (step 9b-4, slice 3); folded at
  // first, as in the mockup: Item and Type, then the caption that nothing comes from Second Life
  m_inventorySection = addSection(QStringLiteral("inventory"), tr("Inventory"), false, page, layout);
  m_inventoryEmpty = new QLabel(tr("No inventory yet. An AI assistant gives one with lsl_run."), m_inventorySection->body());
  m_inventoryEmpty->setObjectName("scriptLabInventoryEmpty");
  m_inventoryEmpty->setWordWrap(true);
  m_inventorySection->bodyLayout()->addWidget(m_inventoryEmpty);
  m_inventory = new QTreeWidget(m_inventorySection->body());
  m_inventory->setObjectName("scriptLabInventory");
  m_inventory->setHeaderLabels({tr("Item"), tr("Type")});
  m_inventory->setRootIsDecorated(false);
  m_inventory->setUniformRowHeights(true);
  m_inventory->setSelectionMode(QAbstractItemView::NoSelection);
  m_inventory->header()->setSectionResizeMode(QHeaderView::Stretch);
  m_inventory->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_inventory->setToolTip(tr("What the object holds in the run: its scripts and the items given with it"));
  m_inventory->setMinimumHeight(4 * m_inventory->fontMetrics().height() + 2 * SpacingS);
  m_inventorySection->bodyLayout()->addWidget(m_inventory);
  m_inventoryNote = new QLabel(tr("Simulated: given with the run, nothing is read from Second Life."), m_inventorySection->body());
  m_inventoryNote->setObjectName("scriptLabInventoryNote");
  m_inventoryNote->setWordWrap(true);
  QFont noteFont = m_inventoryNote->font();   // the "caption" type token, as the mockup's .cap
  if (noteFont.pointSizeF() > 0) noteFont.setPointSizeF(noteFont.pointSizeF() * 11.0 / 13.0);
  m_inventoryNote->setFont(noteFont);
  m_inventorySection->bodyLayout()->addWidget(m_inventoryNote);
  connect(m_inputObject, &QComboBox::currentIndexChanged, this, &ScriptLabPanel::refreshInventory);
  layout->addStretch(1);

  // the page is taller than a small docked panel: it scrolls instead of forcing the panel's height up
  m_pages[LivePage] = makePageScroll(page, "scriptLabLiveScroll");
  m_pages[MapPage] = makePageScroll(mapPage, "scriptLabMapScroll");
  m_pages[PlayerPage] = makePageScroll(playerPage, "scriptLabPlayerScroll");
  // the tree beside the editor follows the Live page's own width (it may be a window of its own)
  m_pages[LivePage]->installEventFilter(this);

  connect(m_watch, &QPushButton::clicked, this, &ScriptLabPanel::chooseFolder);
  connect(m_makeExample, &QPushButton::clicked, this, &ScriptLabPanel::makeExampleProject);
  connect(m_newFolder, &QPushButton::clicked, this, &ScriptLabPanel::chooseNewFolder);
  connect(m_example, &QPushButton::clicked, this, &ScriptLabPanel::tryExampleScripts);
  connect(m_unwatch, &QPushButton::clicked, this, &ScriptLabPanel::stopWatching);
  connect(m_openEditor, &QPushButton::clicked, this, &ScriptLabPanel::openInEditor);
  connect(m_runAgain, &QPushButton::clicked, this, &ScriptLabPanel::runAgain);
  connect(m_problems, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) { showProblem(m_problems->row(item)); });
  // one click is enough to show the line (job R3); Enter and a double-click still work
  connect(m_problems, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) { showProblem(m_problems->row(item)); });
  m_runLog->viewport()->installEventFilter(this);

  setTabOrder(m_makeExample, m_watch);
  setTabOrder(m_watch, m_newFolder);
  setTabOrder(m_newFolder, m_example);
  setTabOrder(m_example, m_liveFolder);
  setTabOrder(m_liveFolder, m_unwatch);
  setTabOrder(m_unwatch, m_runAgain);
  setTabOrder(m_runAgain, m_openEditor);
  setTabOrder(m_openEditor, m_effectsBox);
  setTabOrder(m_effectsBox, m_treeSection->header());
  setTabOrder(m_treeSection->header(), m_followBox);
  setTabOrder(m_followBox, m_folderTree);
  setTabOrder(m_folderTree, m_scriptSection->header());
  setTabOrder(m_scriptSection->header(), m_editor->exampleButton());
  setTabOrder(m_editor->exampleButton(), m_editor->saveButton());
  setTabOrder(m_editor->saveButton(), m_editor->findButton());
  setTabOrder(m_editor->findButton(), m_editor->goToButton());
  setTabOrder(m_editor->goToButton(), m_editor->languageBox());   // shown only with the Lua switch on (job F3)
  setTabOrder(m_editor->languageBox(), m_editor->findField());
  setTabOrder(m_editor->findField(), m_editor->goToField());
  setTabOrder(m_editor->goToField(), m_editor->keepButton());
  setTabOrder(m_editor->keepButton(), m_editor->loadButton());
  setTabOrder(m_editor->loadButton(), m_editor->differenceButton());
  setTabOrder(m_editor->differenceButton(), m_editor->differenceView());
  setTabOrder(m_editor->differenceView(), m_scriptView);
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
  setTabOrder(m_inputLog, m_inventorySection->header());
  setTabOrder(m_inventorySection->header(), m_inventory);
}

void ScriptLabPanel::updateWorldParts()
{
  // the effects box, Objects and Send input belong to a run: a watched folder, or the
  // world of an lsl_run (an AI assistant's), which needs no folder
  const bool watching = m_watcher && m_watcher->watching();
  const bool world = m_session && m_session->sandbox();
  for (QWidget* w : QList<QWidget*>{m_effectsBox, m_objectsSection, m_inputSection, m_inventorySection})
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
  // the examples are separate scripts, not one object: each runs on its own
  if (!watchWith(folder, QJsonObject())) return false;
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
    m_watcherConnections << connect(watcher, &LabWatcher::resultChanged, this, &ScriptLabPanel::refreshMenuBox);
    m_watcherConnections << connect(watcher, &LabWatcher::watchChanged, this, &ScriptLabPanel::refreshMenuBox);
    m_watcherConnections << connect(watcher, &LabWatcher::recipeChanged, this, &ScriptLabPanel::refreshRecipe);
    m_watcherConnections << connect(watcher, &LabWatcher::watchChanged, this, &ScriptLabPanel::refreshRecipe);
  }
  m_refusal.clear();
  connectEffectsToWatcher();
  refreshLive();
  refreshRecipe();
}

bool ScriptLabPanel::watchFolder(const QString& folder)
{
  // the folder is one object (lane 1 job D): every save runs all its scripts together
  return watchWith(folder, QJsonObject{{"linkset", true}});
}

bool ScriptLabPanel::watchWith(const QString& folder, const QJsonObject& options)
{
  if (!m_watcher) return false;
  m_pickedFile.clear();
  const LabWatcher::Result result = m_watcher->watch(QDir::fromNativeSeparators(folder), options);
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
  m_pickedFile.clear();
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
  showScriptPlace(problem.value("line").toInt(), problem.value("column").toInt());
}

bool ScriptLabPanel::showLogLine(int logLine)
{
  const QTextBlock entry = m_runLog->document()->findBlockByNumber(logLine);
  if (!entry.isValid()) return false;
  const QString text = entry.text();
  // a folder run: the line came from another script of the object, so show that script first
  const QString from = m_logFiles.value(logLine);
  if (!from.isEmpty() && from != m_shownFile && !showScript(from)) return false;
  static const QRegularExpression at(QStringLiteral("at line (\\d+), column (\\d+)"));
  const QRegularExpressionMatch m = at.match(text);
  if (m.hasMatch())
  {
    showScriptPlace(m.captured(1).toInt(), m.captured(2).toInt());
    return true;
  }
  // an RLV line: the first script line that holds the same text
  const int start = int(text.indexOf(QStringLiteral("rlv_command: ")));
  if (start < 0) return false;
  QString command = text.mid(start + 13);
  for (const QString& end : {QStringLiteral(" (refused: "), QStringLiteral("; reply on channel ")})
  {
    const int cut = int(command.indexOf(end));
    if (cut >= 0) command = command.left(cut);
  }
  if (command.isEmpty()) return false;
  const QTextDocument* script = m_scriptView->document();
  for (QTextBlock block = script->begin(); block.isValid(); block = block.next())
  {
    const int column = int(block.text().indexOf(command, 0, Qt::CaseInsensitive));
    if (column >= 0)
    {
      showScriptPlace(block.blockNumber() + 1, column + 1);
      return true;
    }
  }
  return false;
}

bool ScriptLabPanel::eventFilter(QObject* watched, QEvent* event)
{
  // job UX-B: the Live page changed its width (in the window or in a window of its own)
  if (watched == m_pages[LivePage] && event->type() == QEvent::Resize) updateWorkLayout();
  // a click on a log line shows its code line (job R3)
  if (m_runLog && watched == m_runLog->viewport() && event->type() == QEvent::MouseButtonRelease)
  {
    const QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() == Qt::LeftButton && !m_runLog->textCursor().hasSelection())
      showLogLine(m_runLog->cursorForPosition(mouse->position().toPoint()).blockNumber());
  }
  // job A2: in "What the avatar may do" the arrow keys pick a row, and Space or Enter switches it
  // between Allowed and Blocked, so the list is one Tab stop (charter P8)
  if (m_actions && watched == m_actions && event->type() == QEvent::KeyPress)
  {
    const QKeyEvent* key = static_cast<QKeyEvent*>(event);
    QTreeWidgetItem* item = m_actions->currentItem();
    if (item && key->modifiers() == Qt::NoModifier &&
        (key->key() == Qt::Key_Space || key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter))
    {
      chooseAction(item->data(0, Qt::UserRole).toString(), item->data(1, Qt::UserRole).toBool());
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

void ScriptLabPanel::showScriptPlace(int line, int column)
{
  const QTextBlock block = m_scriptView->document()->findBlockByNumber(line - 1);
  if (!block.isValid()) return;
  QTextCursor cursor(block);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qBound(0, column - 1, block.length() - 1));
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

bool ScriptLabPanel::showScript(const QString& file)
{
  if (!m_watcher || !m_watcher->watching() || !m_watcher->linkset() || m_watcher->result(file).isEmpty()) return false;
  m_pickedFile = file;
  m_pickedRun = m_watcher->runCount();
  refreshLive();
  return m_shownFile == file;
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
  QString time = QString::number(entry.value("t").toDouble(), 'f', 3) + QStringLiteral(" s  ");
  // a folder run (lane 1 job D) names the source of each line: "main (Root): " or, for an input, "Root: "
  const bool tagged = entry.contains("prim");
  // job A1: a click on a prop in the 3D view, before the touch it sends
  if (type == QLatin1String("clicked") && entry.contains("button"))   // job A2: a button of the menu box
    return time + tr("You clicked \"%1\" in the menu (channel %2).").arg(entry.value("button").toString(), fieldText(entry.value("channel")));
  if (type == QLatin1String("clicked")) return time + tr("You clicked %1.").arg(entry.value("prim").toString());
  if (tagged)
    time += entry.contains("script") ? QStringLiteral("%1 (%2): ").arg(entry.value("script").toString(), entry.value("prim").toString())
                                     : QStringLiteral("%1: ").arg(entry.value("prim").toString());
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
  static const QStringList sourceKeys = {QStringLiteral("script"), QStringLiteral("prim"), QStringLiteral("file"), QStringLiteral("source")};
  for (const QString& key : keys)
    if (key != QLatin1String("t") && key != QLatin1String("type") && !(tagged && sourceKeys.contains(key)))
      parts << key + QStringLiteral("=") + fieldText(entry.value(key));
  return time + type + (parts.isEmpty() ? QString() : QStringLiteral(": ") + parts.join(QStringLiteral(", ")));
}

void ScriptLabPanel::runSavedScript(const QString& path)
{
  // the editor wrote the file: the folder watch checks and runs it now, not after its pause
  if (!m_watcher || !m_watcher->watching())
  {
    m_editor->reportRun(0, 0, tr("no folder is watched."));
    return;
  }
  m_runAtSave = m_watcher->runCount();
  m_watcher->rescan();
  // the same time and size as before (saved twice in one second): run it anyway
  const QString relative = QDir(m_watcher->folder()).relativeFilePath(path);
  if (m_watcher->runCount() == m_runAtSave && !m_watcher->absolutePath(relative).isEmpty()) m_watcher->runScript(relative);
  if (m_watcher->runCount() == m_runAtSave) m_editor->reportRun(0, 0, tr("the folder watch did not find the file."));
}

void ScriptLabPanel::refreshLive()
{
  if (!m_liveStatus || !m_editor) return;
  // job U3: the steps' ticks and the word tips follow every change, also after an early return below
  const auto followUp = qScopeGuard([this]() { refreshSteps(); refreshTips(); refreshUseInScript(); });
  const bool dark = isDark(palette());
  const bool watching = m_watcher && m_watcher->watching();

  m_liveFolder->setText(watching ? QDir::toNativeSeparators(m_watcher->folder()) : QString());
  // a folder watch that just started (here or through lab_watch) brings the Live page to the front
  // (in the window: a popped-out Live page is not pulled to the front of other programs)
  if (watching && !m_wasWatching && tabIndex(LivePage) >= 0) m_tabs->setCurrentIndex(tabIndex(LivePage));
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
  // New folder needs a place it may write to; the root is made when missing
  m_newFolder->setToolTip(tr("Makes an empty folder for your scripts and watches it.") + QLatin1Char(' ')
                          + QDir::toNativeSeparators(scriptsRoot()));
  for (QWidget* w : QList<QWidget*>{m_liveFolderLabel, m_liveFolder, m_unwatch, m_runAgain, m_openEditor,
                                    m_problemsSection, m_logSection, m_liveRestrictionsSection})
    w->setVisible(watching);
  updateWorldParts();

  // the folder as one object (lane 1 job D): the tree, and the script picked in it
  const bool linkset = watching && m_watcher->linkset();
  const QJsonObject folder = linkset ? m_watcher->folderResult() : QJsonObject();
  m_folderTree->setFolderResult(folder);
  m_treeSection->setVisible(linkset);
  m_treeSection->setCount(m_folderTree->countText());
  updatePrimProps();
  int folderScripts = 0;
  for (const QJsonValue& p : folder.value("linkset").toObject().value("prims").toArray())
    folderScripts += int(p.toObject().value("scripts").toArray().size());
  // a pick holds until the next run; then the editor follows the script whose save started it
  const bool picked = linkset && !m_pickedFile.isEmpty() && m_pickedRun == m_watcher->runCount()
                      && !m_watcher->result(m_pickedFile).isEmpty();
  if (!picked) m_pickedFile.clear();
  const QJsonObject last = !watching ? QJsonObject() : picked ? m_watcher->result(m_pickedFile) : m_watcher->lastResult();
  const QString file = last.value("file").toString();

  // the editor: it shows the script of the newest run, but never replaces unsaved edits
  // (another file waits until they are saved or undone; the same file changed on disk asks first)
  m_editor->setTargetFolder(watching ? m_watcher->folder() : QString());
  m_editor->setExamplesFolder(exampleScriptsFolder());
  QString text;
  QString shownPath;
  if (last.isEmpty())
  {
    m_shownFile.clear();
    // closed when the watch stops, or its file is gone; a file just saved and not run yet stays
    if (!watching || (!m_editor->filePath().isEmpty() && !QFileInfo::exists(m_editor->filePath()))) m_editor->closeFile();
  }
  else
  {
    m_shownFile = file;
    shownPath = m_watcher->absolutePath(file);
    QFile in(shownPath);
    if (QFileInfo(in.fileName()).size() <= LabWatcher::MAX_FILE_BYTES && in.open(QIODevice::ReadOnly))
      text = QString::fromUtf8(in.read(LabWatcher::MAX_FILE_BYTES));
    if (!shownPath.isEmpty()) m_editor->openFile(shownPath, file);
  }
  // the marks belong to the text the run used: shown only while the editor holds that file
  // and no changed-outside question is open
  const bool marksFit = !shownPath.isEmpty() && m_editor->filePath() == shownPath && !m_editor->outsideChangePending();
  const QString editorName = m_editor->fileName().isEmpty() ? file : m_editor->fileName();
  m_scriptSection->setVisible(watching || m_editor->isModified());
  m_scriptSection->setTitle(editorName.isEmpty() ? tr("Script") : tr("Script: %1").arg(editorName));
  m_scriptSection->setCount(m_editor->unsavedMark());
  m_openEditor->setEnabled(!m_shownFile.isEmpty());
  if (QTreeWidgetItem* row = m_folderTree->itemForFile(m_shownFile)) m_folderTree->setCurrentItem(row);
  // Run again needs the shown script still in the folder
  const bool canRunAgain = !m_shownFile.isEmpty() && !m_watcher->absolutePath(m_shownFile).isEmpty();
  m_runAgain->setEnabled(canRunAgain);
  m_runAgain->setToolTip(m_shownFile.isEmpty() ? tr("No script yet. Watch a folder and save a .lsl file in it first.")
                         : canRunAgain ? tr("Runs the last saved script again from the start, with a fresh clock.")
                                       : tr("The script is not in the folder any more."));
  m_openEditor->setToolTip(m_shownFile.isEmpty() ? tr("No script yet. Watch a folder and save a .lsl file in it first.")
                                                 : tr("Open the script in the program your computer uses for .lsl files"));
  // the reason as a readable line, not only in the tip (job U3): "Run again, Open in editor: <why>"
  m_runAgain->setStatusTip(canRunAgain ? QString() : m_runAgain->toolTip());
  m_openEditor->setStatusTip(m_openEditor->isEnabled() ? QString() : m_openEditor->toolTip());
  QString reasonLine;
  if (!canRunAgain && !m_openEditor->isEnabled())
    reasonLine = tr("%1, %2: %3").arg(m_runAgain->text(), m_openEditor->text(), m_runAgain->toolTip());
  else if (!canRunAgain)
    reasonLine = tr("%1: %2").arg(m_runAgain->text(), m_runAgain->toolTip());
  m_scriptButtonsReason->setText(reasonLine);
  m_scriptButtonsReason->setVisible(watching && !reasonLine.isEmpty());

  // errors and warnings, with marks in the script
  m_problems->clear();
  m_shownProblems.clear();
  QSet<QString> seen;
  int errorCount = 0;
  int warningCount = 0;
  QList<LabProblem> marks;
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
      if (error) ++errorCount;
      else ++warningCount;
      marks << LabProblem{line, column, item->text(), error};
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
  // a folder run: the limits and rules the folder breaks (nothing ran), and what was left out
  const auto addFolderProblems = [&](const QJsonArray& list, bool error)
  {
    for (const QJsonValue& value : list)
    {
      const QJsonObject p = value.toObject();
      const QString where = p.value("file").toString();
      const QString itemText = where.isEmpty() ? (error ? tr("Folder error: %1") : tr("Folder note: %1")).arg(p.value("message").toString())
                                               : (error ? tr("Folder error, %1: %2") : tr("Folder note, %1: %2")).arg(where, p.value("message").toString());
      QListWidgetItem* item = new QListWidgetItem(itemText, m_problems);
      item->setForeground(error ? dangerColour(dark) : QColor(QString::fromLatin1(tokens(dark).warning)));
      m_shownProblems << QJsonObject();   // no line to show
      if (error) ++errorCount;
      else ++warningCount;
    }
  };
  addFolderProblems(folder.value("linkset").toObject().value("problems").toArray(), true);
  addFolderProblems(folder.value("linkset").toObject().value("notes").toArray(), false);
  // the static warnings (lsl_lint) of a script that compiled: valid LSL that is most likely a mistake
  if (last.value("valid").toBool() && !text.isEmpty()) addProblems(lintWarnings(text), false);
  if (marksFit || shownPath.isEmpty()) m_editor->codeEdit()->setProblems(marksFit ? marks : QList<LabProblem>());
  // the run after the editor's Save: "Saved and run 15:30. 0 errors."
  if (m_editor->waitingForRun() && marksFit && last.value("run").toInt() > m_runAtSave)
  {
    QString notRun = last.value("read_error").toString();
    // a folder that did not run because of this script's own errors: the errors say it
    if (notRun.isEmpty() && last.contains("not_run") && last.value("errors").toArray().isEmpty()) notRun = last.value("not_run").toString();
    m_editor->reportRun(errorCount, warningCount, notRun);
  }
  QStringList problemCount;
  if (errorCount) problemCount << (errorCount == 1 ? tr("1 error") : tr("%1 errors").arg(errorCount));
  if (warningCount) problemCount << (warningCount == 1 ? tr("1 warning") : tr("%1 warnings").arg(warningCount));
  m_problemsSection->setCount(problemCount.isEmpty() ? tr("none") : problemCount.join(QStringLiteral(", ")));

  // the event log of the run
  QStringList lines;
  m_logFiles.clear();
  const QJsonArray runLog = run.value("log").toArray();
  for (const QJsonValue& value : runLog)
  {
    QJsonObject entry = value.toObject();
    // with one script in the folder every line has the same source, so it is not repeated on each line
    if (folderScripts <= 1) entry.remove(QStringLiteral("prim"));
    lines << logLine(entry);
    m_logFiles << entry.value("file").toString();
  }
  if (run.value("log_truncated").toBool()) lines << tr("(the log was cut here: the run wrote more than it keeps)");
  const QString logText = lines.join(QLatin1Char('\n'));
  if (m_runLog->toPlainText() != logText) m_runLog->setPlainText(logText);
  if (last.isEmpty()) m_logSection->setCount(tr("none"));
  else if (!last.contains("result")) m_logSection->setCount(tr("not run"));
  else m_logSection->setCount(runLog.size() == 1 ? tr("1 line") : tr("%1 lines").arg(runLog.size()));
  refreshChannelMap(linkset ? folder : QJsonObject());

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
  // a folder of several scripts is named as the object; a folder of one script by its file, as before
  const QString objectName = folder.value("linkset").toObject().value("object").toString();
  const QString subject = linkset && folderScripts > 1 ? tr("the object %1").arg(objectName) : file;
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
  if (linkset && !folder.value("ok").toBool())
  {
    // the shown script is fine, but the folder did not run: a limit, or another script that does not compile
    const int limits = int(folder.value("linkset").toObject().value("problems").toArray().size());
    QStringList broken;
    for (const QJsonValue& v : folder.value("check").toObject().value("scripts").toArray())
      if (!v.toObject().value("valid").toBool()) broken << v.toObject().value("file").toString();
    if (limits > 0)
      setLiveStatus(tr("Error: the object %1 was not run (run %2): it breaks %n limit(s) or rule(s), listed under Errors and warnings.", nullptr, limits)
                      .arg(objectName).arg(runNumber), 2);
    else if (!broken.isEmpty())
      setLiveStatus(tr("Error: the object %1 was not run (run %2): %3 does not compile. Pick it in the tree to see why.")
                      .arg(objectName).arg(runNumber).arg(broken.join(QStringLiteral(", "))), 2);
    else
      setLiveStatus(tr("Error: the object %1 was not run (run %2). %3").arg(objectName).arg(runNumber).arg(folder.value("error").toString()), 2);
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
    setLiveStatus(tr("Error: %1 ran and was stopped (run %2): %3").arg(subject).arg(runNumber).arg(run.value("stop_reason").toString()) + rlvText, 2);
    return;
  }
  setLiveStatus(tr("OK: %1 ran without errors (run %2), %n log line(s).", nullptr, int(run.value("log").toArray().size()))
                  .arg(subject).arg(runNumber) + rlvText, 1);
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

bool ScriptLabPanel::primPropsShown() const
{
  return m_effectsBox && m_effectsBox->isChecked() && m_watcher && m_watcher->watching() && m_watcher->linkset();
}

void ScriptLabPanel::updatePrimProps()
{
  // props from prims (job A1): every prim of the watched object gets a prop of its name
  const bool show = primPropsShown();
  const QJsonObject folder = show ? shownFolder() : QJsonObject();
  bool added = false;
  if (show && m_hooks.addProp && m_hooks.hasProp)
  {
    // the props of the folder's example.json (an example project), read once per folder
    if (m_primExampleFolder != m_watcher->folder())
    {
      m_primExampleFolder = m_watcher->folder();
      m_primExample = LabBeginner::readExample(m_primExampleFolder).props;
    }
    for (const LabExample::Prop& prop : LabPrimProps::propsFor(folder, m_primExample))
      if (!m_hooks.hasProp(prop.name)) added = m_hooks.addProp(prop) || added;
  }
  // props that move: a prop stands where the script put its prim; it jumps there, no animation
  m_primMoves = show ? LabPrimProps::movesFromFolder(folder) : QList<LabPrimProps::Move>();
  if (m_hooks.placeProp && m_hooks.propPlace)
  {
    QSet<QString> moved;
    for (const LabPrimProps::Move& move : m_primMoves)
    {
      if (!m_primBase.contains(move.prim))
      {
        LabExample::Prop base;
        if (!m_hooks.propPlace(move.prim, base)) continue;
        m_primBase.insert(move.prim, base);
      }
      m_hooks.placeProp(LabPrimProps::moved(m_primBase.value(move.prim), move));
      moved.insert(move.prim);
    }
    // a prop that no script moves now goes back to where it stood
    for (auto it = m_primBase.begin(); it != m_primBase.end();)
    {
      if (moved.contains(it.key())) { ++it; continue; }
      m_hooks.placeProp(it.value());
      it = m_primBase.erase(it);
    }
  }
  // a new prop gets its tint at once
  updatePropTints(added);
  updateChains();
}

void ScriptLabPanel::updateChains()
{
  // job A4, part 2: chains and ribbons between two prims, while the props are shown
  QVector<LabChains::Chain> chains;
  if (primPropsShown())
  {
    const QJsonObject folder = m_watcher->folderResult();
    chains = LabChains::fromFolderResult(folder, m_state && LabChains::objectLocked(folder, *m_state));
  }
  if (chains == m_chains) return;
  m_chains = chains;
  emit chainsChanged();
}

QJsonObject ScriptLabPanel::shownFolder() const
{
  if (!m_watcher) return QJsonObject();
  return m_player && m_player->showing() ? m_player->folderAt(m_watcher->folderResult()) : m_watcher->folderResult();
}

bool ScriptLabPanel::touchPrim(const QString& name, const QPoint& at)
{
  if (!primPropsShown()) return false;
  // job B3: a click is an input to the world of now, so the props show now again first
  if (m_player) m_player->showNow();
  const int link = m_watcher->linkOf(name);
  if (link < 1) return false;
  m_menuAnchor = at;   // a menu the touch makes stands next to the click (job A2)
  const LabWatcher::Result result = m_watcher->touch(link);
  if (!result.ok())
  {
    setLiveStatus(tr("Error: the touch on %1 was not sent. %2").arg(name, result.error), 2);
    return false;
  }
  return true;
}

void ScriptLabPanel::updatePropTints(bool force)
{
  // props as prims (lane 1 job E): only while the effects are shown and the object is watched
  const bool show = primPropsShown();
  const QList<LabTint::Tint> tints = show ? LabTint::fromFolder(shownFolder()) : QList<LabTint::Tint>();
  if (tints == m_propTints && !force) return;
  m_propTints = tints;
  emit propTintsChanged();
}

void ScriptLabPanel::effectsToggled(bool on)
{
  styleEffectsButton();
  refreshSteps();   // the step "Show effects" follows the button
  updatePrimProps();
  refreshMenuBox();   // job A2: the menu box shows only with the props
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
    cameraOverride->setEnabled(m_cameraBox->isChecked() && !m_hiddenByUser);
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
    environmentOverride->setEnabled(m_environmentBox->isChecked() && !m_hiddenByUser);
    m_environmentConnection = connect(environmentOverride, &EnvironmentOverride::changed, this, &ScriptLabPanel::refreshEnvironment);
  }
  m_sphereControls->setOverride(environmentOverride);
  refreshEnvironment();
}

void ScriptLabPanel::refreshEnvironment()
{
  if (!m_environmentStatus) return;
  // the sliders work while the box is ticked; the part's count says when their sphere is in the view
  m_sphereControls->setAvailable(m_environment && m_environmentBox->isChecked());
  m_sphereSection->setCount(m_environment && m_environmentBox->isChecked() && m_environment->hasTrySphere() ? tr("sliders on") : QString());
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
    viewModesOverride->setEnabled(m_viewModesBox->isChecked() && !m_hiddenByUser);
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
    // the same grid as the menu box over the 3D view (job A2): rows of three, the first row on top
    LabDialogBox::fillButtonGrid(m_dialogGrid, m_dialogButtonBox, buttons, QStringLiteral("scriptLabDialogButton"),
                                 [this](const QString& text) { answerDialog(text); });
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
  refreshInventory();
}

QString ScriptLabPanel::itemCount(int items)
{
  return items == 1 ? tr("1 item") : tr("%1 items").arg(items);
}

QString ScriptLabPanel::inventoryTypeText(int type, int lines)
{
  // the INVENTORY_* numbers of the Second Life wiki, as lsl::Inventory knows them
  switch (type)
  {
    case 0: return tr("Texture");
    case 1: return tr("Sound");
    case 3: return tr("Landmark");
    case 5: return tr("Clothing");
    case 6: return tr("Object");
    case 7: return lines == 1 ? tr("Notecard, 1 line") : tr("Notecard, %1 lines").arg(lines);
    case 10: return tr("Script");
    case 13: return tr("Body part");
    case 20: return tr("Animation");
    case 21: return tr("Gesture");
    case 56: return tr("Setting");
    case 57: return tr("Material");
    default: return tr("Type %1").arg(type);
  }
}

void ScriptLabPanel::refreshInventory()
{
  if (!m_inventory) return;
  const lsl::Sandbox* sandbox = m_session ? m_session->sandbox() : nullptr;
  // the combo box lists the objects in the world's order, so its index is the object's
  const int index = sandbox && m_inputObject->currentIndex() < sandbox->objectCount() ? m_inputObject->currentIndex() : -1;
  m_inventory->clear();
  if (!sandbox || index < 0)
  {
    m_inventorySection->setTitle(tr("Inventory"));
    m_inventorySection->setCount(tr("none"));
    m_inventoryEmpty->show();
    m_inventory->hide();
    m_inventoryNote->hide();
    return;
  }
  // the scripts first, then the items given with the run in their order, as the mockup lists them
  const std::vector<lsl::InventoryItem>& items = sandbox->inventory(index).items();
  QList<const lsl::InventoryItem*> shown;
  for (const lsl::InventoryItem& item : items)
    if (item.type == 10) shown << &item;
  for (const lsl::InventoryItem& item : items)
    if (item.type != 10) shown << &item;
  const bool dark = isDark(palette());
  const QIcon notecard = LucideIcons::icon(QStringLiteral("notebook-text"), palette().color(QPalette::Text), mutedColour(dark));
  for (const lsl::InventoryItem* item : shown)
  {
    QTreeWidgetItem* row = new QTreeWidgetItem(m_inventory);
    row->setText(0, item->name);
    row->setText(1, inventoryTypeText(item->type, int(item->lines.size())));
    row->setToolTip(0, item->name);
    if (item->type == 7) row->setIcon(1, notecard);
  }
  m_inventorySection->setTitle(tr("Inventory of %1").arg(m_inputObject->currentText()));
  m_inventorySection->setCount(itemCount(int(shown.size())));
  m_inventoryEmpty->hide();
  m_inventory->show();
  m_inventoryNote->show();
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
  QJsonObject request = LabWatcher::dialogAnswer(dialog, button);   // the same input as the menu box (job A2)
  return sendInput(request, tr("Dialog answer \"%1\" to %2").arg(button, dialog.value("object").toString()));
}

// --- Choose a restriction from the list (job A2) --------------------------------

void ScriptLabPanel::addActionChoices()
{
  // made last in the constructor, so the choices sit at the end of the focus chain; they take no
  // keys anyway (the list does: Space or Enter)
  for (int i = 0; i < m_actions->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* item = m_actions->topLevelItem(i);
    const Rlv::Action action{item->data(0, Qt::UserRole).toString(), item->text(0)};
    // job A2: choose Allowed or Blocked in the row itself; the word stays in the cell under it
    QComboBox* choice = new QComboBox(m_actions);
    choice->setObjectName("scriptLabActionChoice");
    choice->addItem(tr("Allowed"));
    choice->addItem(tr("Blocked"));
    choice->setProperty("action", action.id);
    choice->setAccessibleName(action.label);
    choice->setAutoFillBackground(true);
    choice->setMinimumHeight(ControlCompact);
    choice->setFocusPolicy(Qt::NoFocus);   // the keys go through the list itself (Space or Enter), one Tab stop
    m_actions->setItemWidget(item, 1, choice);
    connect(choice, QOverload<int>::of(&QComboBox::activated), this, [this, id = action.id](int index) { chooseAction(id, index == 1); });
  }
}

void ScriptLabPanel::buildChoiceBox(QWidget* parent, QVBoxLayout* layout)
{
  m_choiceBox = new QFrame(parent);
  m_choiceBox->setObjectName("scriptLabChoiceBox");
  m_choiceBox->setFrameShape(QFrame::StyledPanel);
  QVBoxLayout* box = new QVBoxLayout(m_choiceBox);
  box->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  box->setSpacing(SpacingS);

  m_choiceTitle = new QLabel(m_choiceBox);
  m_choiceTitle->setObjectName("scriptLabChoiceTitle");
  m_choiceTitle->setTextFormat(Qt::PlainText);
  m_choiceTitle->setWordWrap(true);
  QFont titleFont = m_choiceTitle->font();
  titleFont.setWeight(QFont::DemiBold);
  m_choiceTitle->setFont(titleFont);
  box->addWidget(m_choiceTitle);

  m_choiceLine = new QLabel(m_choiceBox);
  m_choiceLine->setObjectName("scriptLabChoiceLine");
  m_choiceLine->setTextFormat(Qt::PlainText);
  m_choiceLine->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_choiceLine->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_choiceLine->setToolTip(tr("The RLV line that was made and run"));
  box->addWidget(m_choiceLine);

  m_choiceExplain = new QLabel(m_choiceBox);
  m_choiceExplain->setObjectName("scriptLabChoiceExplain");
  m_choiceExplain->setTextFormat(Qt::PlainText);
  m_choiceExplain->setWordWrap(true);
  m_choiceExplain->setTextInteractionFlags(Qt::TextSelectableByMouse);
  box->addWidget(m_choiceExplain);

  // a distance or a channel that stays open: a field, used again with Apply or Enter
  m_choiceValueRow = new QWidget(m_choiceBox);
  QHBoxLayout* valueRow = new QHBoxLayout(m_choiceValueRow);
  valueRow->setContentsMargins(0, 0, 0, 0);
  valueRow->setSpacing(SpacingS);
  m_choiceValueLabel = new QLabel(m_choiceValueRow);
  m_choiceValueLabel->setObjectName("scriptLabChoiceValueLabel");
  m_choiceValue = new QLineEdit(m_choiceValueRow);
  m_choiceValue->setObjectName("scriptLabChoiceValue");
  m_choiceValue->setMinimumHeight(ControlHeight);
  m_choiceValue->setMaxLength(16);
  m_choiceValueLabel->setBuddy(m_choiceValue);
  m_choiceApply = new QPushButton(tr("Apply"), m_choiceValueRow);
  m_choiceApply->setObjectName("scriptLabChoiceApply");
  m_choiceApply->setMinimumHeight(ControlHeight);
  m_choiceApply->setToolTip(tr("Make the line again with this value, and run it (Enter)"));
  valueRow->addWidget(m_choiceValueLabel);
  valueRow->addWidget(m_choiceValue, 1);
  valueRow->addWidget(m_choiceApply);
  box->addWidget(m_choiceValueRow);
  connect(m_choiceApply, &QPushButton::clicked, this, [this]() { applyChoiceValue(); });
  connect(m_choiceValue, &QLineEdit::returnPressed, this, [this]() { applyChoiceValue(); });

  m_useInScript = new QPushButton(tr("Use in script"), m_choiceBox);
  m_useInScript->setObjectName("scriptLabUseInScript");
  m_useInScript->setMinimumHeight(ControlHeight);
  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->setSpacing(SpacingS);
  buttons->addWidget(m_useInScript);
  // why the button is greyed, readable without hovering (charter P6)
  m_useInScriptReason = new QLabel(m_choiceBox);
  m_useInScriptReason->setObjectName("scriptLabUseInScriptReason");
  m_useInScriptReason->setTextFormat(Qt::PlainText);
  m_useInScriptReason->setWordWrap(true);
  buttons->addWidget(m_useInScriptReason, 1);
  box->addLayout(buttons);
  connect(m_useInScript, &QPushButton::clicked, this, [this]() { useChoiceInScript(); });

  m_choiceResult = new QLabel(m_choiceBox);
  m_choiceResult->setObjectName("scriptLabChoiceResult");
  m_choiceResult->setTextFormat(Qt::PlainText);
  m_choiceResult->setWordWrap(true);
  m_choiceResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
  box->addWidget(m_choiceResult);

  m_choiceBox->hide();
  layout->addWidget(m_choiceBox);
}

QComboBox* ScriptLabPanel::actionChoice(const QString& actionId) const
{
  for (int i = 0; i < m_actions->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* item = m_actions->topLevelItem(i);
    if (item->data(0, Qt::UserRole).toString() == actionId) return qobject_cast<QComboBox*>(m_actions->itemWidget(item, 1));
  }
  return nullptr;
}

void ScriptLabPanel::setChoiceResult(const QString& text, int tone)
{
  m_choiceResult->setText(text);
  m_choiceResult->setVisible(!text.isEmpty());
  const bool dark = isDark(palette());
  QPalette p = m_choiceResult->palette();
  p.setColor(QPalette::WindowText, tone == 1 ? successColour(dark) : tone == 2 ? dangerColour(dark) : mutedColour(dark));
  m_choiceResult->setPalette(p);
}

void ScriptLabPanel::showChoice(const QString& actionId, bool blocked, const QString& line, const QStringList& words,
                                const QString& result, int tone)
{
  const LabRestrictionChoice::Choice choice = LabRestrictionChoice::forAction(actionId, m_state ? m_state->mode() : Rlv::Mode::Rlva);
  if (m_choiceAction != actionId) m_choiceValue->clear();   // the field belongs to one row
  m_choiceAction = actionId;
  m_choiceRlvLine = line;
  m_choiceTitle->setText(blocked ? tr("%1: Blocked").arg(choice.label) : tr("%1: Allowed").arg(choice.label));
  m_choiceLine->setText(line);
  m_choiceLine->setVisible(!line.isEmpty());
  m_choiceExplain->setText(words.join(QLatin1Char('\n')));
  m_choiceExplain->setVisible(!words.isEmpty());
  const bool field = choice.value != LabRestrictionChoice::ValueKind::None && blocked;
  m_choiceValueRow->setVisible(field);
  if (choice.value == LabRestrictionChoice::ValueKind::Distance)
  {
    m_choiceValueLabel->setText(tr("Farther than (metres)"));
    m_choiceValue->setPlaceholderText(tr("Empty: the viewer's own limit"));
    m_choiceValue->setToolTip(tr("Blocks it only farther away than this. Empty uses the viewer's own limit."));
  }
  else if (choice.value == LabRestrictionChoice::ValueKind::ExceptChannel)
  {
    m_choiceValueLabel->setText(tr("Still allow channel"));
    m_choiceValue->setPlaceholderText(tr("Empty: none"));
    m_choiceValue->setToolTip(tr("This channel stays open while the others are blocked."));
  }
  setChoiceResult(result, tone);
  m_choiceBox->show();
  refreshUseInScript();
}

void ScriptLabPanel::refreshUseInScript()
{
  if (!m_useInScript) return;
  QString reason;
  if (m_choiceRlvLine.isEmpty()) reason = tr("There is no line to use yet. Choose Blocked first.");
  else if (!m_editor || m_editor->filePath().isEmpty()) reason = tr("Open a script on the Live scripts page first.");
  m_useInScript->setEnabled(reason.isEmpty());
  m_useInScriptReason->setText(reason);
  m_useInScriptReason->setVisible(!reason.isEmpty());
  QPalette muted = m_useInScriptReason->palette();
  muted.setColor(QPalette::WindowText, mutedColour(isDark(palette())));
  m_useInScriptReason->setPalette(muted);
  m_useInScript->setToolTip(reason.isEmpty() ? tr("Put %1 into the script, so the script sends this line.")
                                                 .arg(LabRestrictionChoice::scriptLine(m_choiceRlvLine))
                                             : reason);
}

bool ScriptLabPanel::chooseAction(const QString& actionId, bool blocked)
{
  if (!m_state || !actionChoice(actionId)) return false;
  const LabRestrictionChoice::Choice choice = LabRestrictionChoice::forAction(actionId, m_state->mode());
  QString object = m_object->text().trimmed();
  if (object.isEmpty()) object = QString::fromLatin1(DefaultObject);
  const auto followUp = qScopeGuard([this]() { refresh(); });   // the row shows the state again, whatever happened
  if (!choice.ok())
  {
    showChoice(actionId, blocked, QString(), QStringList(), choice.reason, 2);
    return false;
  }
  if (blocked)
  {
    const QString value = m_choiceAction == actionId ? m_choiceValue->text() : QString();
    const LabRestrictionChoice::Line line = LabRestrictionChoice::blockLine(choice, value);
    if (!line.ok())
    {
      showChoice(actionId, true, QString(), QStringList(), line.error, 2);
      return false;
    }
    const bool ran = runLine(line.text);
    showChoice(actionId, true, line.text, LabRestrictionChoice::explainBlock(choice, value),
               ran ? tr("Ran from %1. Blocked now.").arg(object) : tr("Refused. Replies and messages says why."), ran ? 1 : 2);
    return ran;
  }
  const QString lift = LabRestrictionChoice::allowLine(choice, m_state->restrictions(), object);
  const bool ran = !lift.isEmpty() && runLine(lift);
  const QStringList others = LabRestrictionChoice::otherHolders(choice, m_state->restrictions(), object);
  QString result;
  int tone = 1;
  if (!others.isEmpty())
  {
    result = tr("Still blocked, because %1 holds it too.").arg(others.join(QStringLiteral(", ")));
    tone = 2;
  }
  else if (lift.isEmpty()) result = tr("Nothing to lift. %1 held nothing of it.").arg(object);
  else result = ran ? tr("Ran from %1. Allowed now.").arg(object) : tr("Refused. Replies and messages says why.");
  if (!lift.isEmpty() && !ran) tone = 2;
  showChoice(actionId, false, lift, lift.isEmpty() ? QStringList() : LabRestrictionChoice::explainAllow(choice), result, tone);
  return ran;
}

bool ScriptLabPanel::applyChoiceValue()
{
  if (!m_state || m_choiceAction.isEmpty()) return false;
  const LabRestrictionChoice::Choice choice = LabRestrictionChoice::forAction(m_choiceAction, m_state->mode());
  const LabRestrictionChoice::Line line = LabRestrictionChoice::blockLine(choice, m_choiceValue->text());
  if (!line.ok())
  {
    setChoiceResult(line.error, 2);
    return false;
  }
  // the old limit of this object goes first, so only the new one is held
  QString object = m_object->text().trimmed();
  if (object.isEmpty()) object = QString::fromLatin1(DefaultObject);
  const QString lift = LabRestrictionChoice::allowLine(choice, m_state->restrictions(), object);
  if (!lift.isEmpty()) runLine(lift);
  return chooseAction(m_choiceAction, true);
}

bool ScriptLabPanel::useChoiceInScript()
{
  refreshUseInScript();
  if (!m_useInScript->isEnabled())
  {
    setChoiceResult(m_useInScript->toolTip(), 2);
    return false;
  }
  LabCodeEdit* edit = m_editor->codeEdit();
  const QString code = LabRestrictionChoice::scriptLine(m_choiceRlvLine);
  const LabRestrictionChoice::Insertion in = LabRestrictionChoice::insert(edit->toPlainText(), edit->textCursor().position(), code);
  // one undo step, so Ctrl+Z takes it out again
  QTextCursor cursor(edit->document());
  cursor.setPosition(in.at);
  cursor.insertText(in.added);
  cursor.setPosition(in.position + int(code.size()));
  edit->setTextCursor(cursor);
  showPage(LivePage);
  edit->ensureCursorVisible();
  edit->setFocus(Qt::OtherFocusReason);
  setChoiceResult(tr("Put in the script at line %1. Save to run it.").arg(in.line), 1);
  return true;
}

// --- The menu box over the 3D view (job A2) -------------------------------------

void ScriptLabPanel::setMenuBoxHost(QWidget* host)
{
  if (m_menuBox) m_menuBox->deleteLater();
  m_menuBox = nullptr;
  m_menuKey.clear();
  if (!host) return;
  m_menuBox = new LabDialogBox(host);
  connect(m_menuBox, &LabDialogBox::answered, this, [this](const QString& text) { answerMenu(text); });
  connect(m_menuBox, &LabDialogBox::ignored, this, [this]()
  {
    m_menuDone = m_menuKey;
    if (m_menuBox && m_menuBox->parentWidget()) m_menuBox->parentWidget()->setFocus(Qt::OtherFocusReason);
  });
  refreshMenuBox();
}

void ScriptLabPanel::refreshMenuBox()
{
  if (!m_menuBox) return;
  const QJsonObject dialog = primPropsShown() ? m_watcher->dialog() : QJsonObject();
  // the time is part of the key: the same menu shown again after an answer is a new menu
  const QString key = dialog.isEmpty() ? QString() : dialogKey(dialog) + QChar(0x1f) + QString::number(dialog.value("t").toDouble(), 'f', 3);
  if (key.isEmpty() || key == m_menuDone)
  {
    m_menuKey = key;
    m_menuBox->hide();
    return;
  }
  if (key == m_menuKey && m_menuBox->isVisible()) return;
  m_menuKey = key;
  m_menuBox->showDialog(dialog, m_menuAnchor);
}

bool ScriptLabPanel::answerMenu(const QString& button)
{
  if (!m_watcher) return false;
  m_menuDone = m_menuKey;
  if (m_menuBox) m_menuBox->hide();
  const LabWatcher::Result result = m_watcher->answerDialog(button);
  if (!result.ok())
  {
    setLiveStatus(tr("Error: the menu answer \"%1\" was not sent. %2").arg(button, result.error), 2);
    return false;
  }
  return true;
}

// --- Script Lab for beginners (job U3) ----------------------------------------

void ScriptLabPanel::styleEffectsButton()
{
  if (!m_effectsBox) return;
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  const QString selection = Theme::color(Theme::Token::Selection, dark).name(QColor::HexRgb).toUpper();
  // off: a normal button with the eye; on: the selection fill, on_accent text and a check mark
  // (DESIGN_SYSTEM.md, "States": selected has a fill and a shape)
  m_effectsBox->setStyleSheet(QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:checked { background: %6; color: %7; border: 1px solid %6; }"
    "QPushButton:focus { border: 2px solid %8; }")
    .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border))
    .arg(RadiusControl).arg(SpacingM)
    .arg(selection, QLatin1String(t.onAccent), QLatin1String(t.focusRing)));
  const bool on = m_effectsBox->isChecked();
  QFont effectsFont = m_effectsBox->font();   // bold when on: a shape besides the fill (P10)
  effectsFont.setWeight(on ? QFont::DemiBold : QFont::Normal);
  m_effectsBox->setFont(effectsFont);
  m_effectsBox->setIcon(LucideIcons::icon(on ? QStringLiteral("check") : QStringLiteral("eye"),
                                          QColor(QLatin1String(on ? t.onAccent : t.text)), QColor(QLatin1String(t.textMuted))));
  m_effectsBox->setAccessibleDescription(on ? tr("On: the 3D view shows what the scripts do.") : tr("Off."));
  styleRecordButton();
}

// --- record and replay (job A3) -------------------------------------------------------------

void ScriptLabPanel::styleRecordButton()
{
  if (!m_recordButton) return;
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  const QString selection = Theme::color(Theme::Token::Selection, dark).name(QColor::HexRgb).toUpper();
  // the same on/off look as Show effects in 3D view: on has the selection fill, the word and a square
  m_recordButton->setStyleSheet(QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:checked { background: %6; color: %7; border: 1px solid %6; }"
    "QPushButton:focus { border: 2px solid %8; }")
    .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.border))
    .arg(RadiusControl).arg(SpacingM)
    .arg(selection, QLatin1String(t.onAccent), QLatin1String(t.focusRing)));
  const bool on = m_recordButton->isChecked();
  QFont recordFont = m_recordButton->font();   // bold when on, as Show effects in 3D view
  recordFont.setWeight(on ? QFont::DemiBold : QFont::Normal);
  m_recordButton->setFont(recordFont);
  m_recordButton->setIcon(LucideIcons::icon(on ? QStringLiteral("square") : QStringLiteral("video"),
                                            QColor(QLatin1String(on ? t.onAccent : t.text)), QColor(QLatin1String(t.textMuted))));
}

bool ScriptLabPanel::setRecording(bool on)
{
  if (!m_watcher) return false;
  bool changed = false;
  m_recipeNote.clear();
  m_recipeNoteIsError = false;
  if (on && !m_watcher->recording())
  {
    const LabWatcher::Result result = m_watcher->startRecording();
    if (!result.ok())
    {
      m_recipeNote = tr("Error: %1").arg(result.error);
      m_recipeNoteIsError = true;
    }
    changed = result.ok();
  }
  else if (!on && m_watcher->recording())
  {
    const bool none = m_watcher->recordedSteps().isEmpty();
    changed = m_watcher->stopRecording();
    if (none) m_recipeNote = tr("Recording stopped. No steps were recorded, so the recipe stays as it was.");
  }
  refreshRecipe();
  return changed;
}

bool ScriptLabPanel::replayNow()
{
  if (!m_watcher) return false;
  m_recipeNote.clear();
  m_recipeNoteIsError = false;
  const LabWatcher::Result result = m_watcher->replayRecipe();
  if (!result.ok())
  {
    m_recipeNote = tr("Error: %1").arg(result.error);
    m_recipeNoteIsError = true;
  }
  refreshRecipe();
  return result.ok();
}

void ScriptLabPanel::setExpertMode(bool expert)
{
  const bool changed = m_expert != expert;
  m_expert = expert;
  if (changed && m_mapSection) m_mapSection->setExpanded(expert);
  refreshRecipe();
}

void ScriptLabPanel::refreshRecipe()
{
  if (!m_recipeRow) return;
  const bool object = m_watcher && m_watcher->watching() && m_watcher->linkset();
  m_recipeRow->setVisible(object);
  m_playerEmpty->setVisible(!object);   // job UX-B: the tab says what fills it
  m_recipeSection->setVisible(object && m_expert);
  // job B3: the player shows the newest replay, when it has moments
  m_player->setExpertMode(m_expert);
  m_player->setReplay(object ? m_watcher->lastReplay() : QJsonObject());
  m_player->setVisible(object && m_player->momentCount() > 0);
  if (!object)
  {
    m_recordButton->setChecked(false);
    styleRecordButton();
    return;
  }
  const bool recording = m_watcher->recording();
  m_recordButton->setChecked(recording);
  m_recordButton->setText(recording ? tr("Stop recording") : tr("Record what I do"));
  m_recordButton->setAccessibleDescription(recording ? tr("Recording.") : tr("Not recording."));
  styleRecordButton();

  QStringList lines;
  bool bad = m_recipeNoteIsError;
  const QJsonObject replay = m_watcher->lastReplay();
  auto strings = [](const QJsonValue& v)
  {
    QStringList out;
    for (const QJsonValue& s : v.toArray()) out << s.toString();
    return out;
  };
  if (recording)
  {
    lines << tr("Recording. Click props and menu buttons. Steps so far: %1.").arg(m_watcher->recordedSteps().size());
    if (!primPropsShown()) lines << tr("Turn on Show effects in 3D view to click props.");
  }
  else if (!m_recipeNote.isEmpty())
    lines << m_recipeNote;
  else if (!replay.isEmpty())
  {
    const int steps = replay.value("steps").toInt();
    const int done = replay.value("done").toInt();
    if (!replay.value("ran").toBool())
    {
      bad = true;
      lines << tr("The object did not run, so your steps were not replayed.");
      lines << strings(replay.value("changes"));
    }
    else
    {
      lines << (done == steps ? tr("Replayed your %n step(s).", nullptr, steps) : tr("Replayed %1 of %2 steps.").arg(done).arg(steps));
      const QStringList changes = strings(replay.value("changes"));
      if (replay.value("first").toBool())
      {
        lines << tr("This is the first time, so there is nothing to compare.");
        lines << tr("Now:");
        for (const QString& f : strings(replay.value("facts"))) lines << QStringLiteral("  ") + f;
      }
      else if (changes.isEmpty())
        lines << tr("Nothing changed since last time.");
      else
      {
        lines << tr("What changed since last time:");
        for (const QString& c : changes) lines << QStringLiteral("  ") + c;
      }
    }
  }
  else if (m_watcher->hasRecipe())
    lines << tr("The recipe is replayed after the next save.");
  else
    lines << tr("No recipe yet. Press Record what I do, then click your props.");
  m_recipeStatus->setText(lines.join(QLatin1Char('\n')));
  const bool dark = isDark(palette());
  QPalette p = m_recipeStatus->palette();
  p.setColor(QPalette::WindowText, bad ? dangerColour(dark) : palette().color(QPalette::WindowText));
  m_recipeStatus->setPalette(p);

  // Expert: the file and its text
  m_recipeFileLine->setText(m_watcher->hasRecipe()
                              ? tr("File: %1 in the object folder. %2 is kept next to it.").arg(LabRecipe::recipeFile(), LabRecipe::resultFile())
                              : tr("No recipe file yet. Recording makes %1 in the object folder.").arg(LabRecipe::recipeFile()));
  QString text;
  if (m_watcher->hasRecipe())
  {
    QFile file(m_watcher->recipePath());
    if (file.open(QIODevice::ReadOnly)) text = QString::fromUtf8(file.read(LabRecipe::MAX_FILE_BYTES));
  }
  if (m_recipeText->toPlainText() != text) m_recipeText->setPlainText(text);
  m_replayButton->setEnabled(m_watcher->hasRecipe() && !recording);
  m_replayButton->setToolTip(m_replayButton->isEnabled() ? tr("Repeats the recipe's steps now, without a save.")
                                                         : recording ? tr("Stop recording first.") : tr("Record what you do first."));
}

int ScriptLabPanel::sideBySideWidth()
{
  // room for the tree (about 300 px) and the editor with its suggestion list (about 440 px)
  return TextSize::scaled(760);
}

void ScriptLabPanel::updateWorkLayout()
{
  if (!m_workLayout) return;
  // job UX-B: the width of the Live page's own window while it is popped out
  const int room = m_pages[LivePage] && isPoppedOut(LivePage) ? m_pages[LivePage]->width() : width();
  const bool wide = room >= sideBySideWidth();
  if (wide == m_sideBySide && m_workLayout->direction() == (wide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom)) return;
  m_sideBySide = wide;
  m_workLayout->setDirection(wide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
  // side by side the width is shared 2 to 3; stacked, each column takes the height it needs
  m_workLayout->setStretch(0, wide ? 2 : 0);
  m_workLayout->setStretch(1, wide ? 3 : 0);
  // side by side, both columns start at the top
  m_workLayout->setAlignment(m_treeColumn, wide ? Qt::AlignTop : Qt::Alignment());
  m_workLayout->setAlignment(m_editorColumn, wide ? Qt::AlignTop : Qt::Alignment());
  // the page's layout keeps the old size of the work area until it is told: stacked needs far more height
  m_workLayout->invalidate();
  m_workArea->updateGeometry();
}

QString ScriptLabPanel::exampleObjectsFolder() const
{
  if (!m_objectsFolder.isEmpty()) return m_objectsFolder;
  return QDir::cleanPath(EffectsPreview::defaultExamplesFolder() + QStringLiteral("/objects"));
}

QString ScriptLabPanel::scriptsRoot() const
{
  return m_scriptsRoot.isEmpty() ? LabBeginner::defaultScriptsRoot() : m_scriptsRoot;
}

bool ScriptLabPanel::watchingProject() const
{
  return !m_projectFolder.isEmpty() && m_watcher && m_watcher->watching() && m_watcher->linkset()
         && QDir::cleanPath(m_watcher->folder()) == QDir::cleanPath(QDir::fromNativeSeparators(m_projectFolder));
}

bool ScriptLabPanel::makeExampleProject()
{
  QString error;
  const LabExample example = LabBeginner::firstExample(exampleObjectsFolder(), &error);
  if (!example.isValid())
  {
    m_refusal = tr("Error: %1").arg(error);
    refreshLive();
    return false;
  }
  const QString folder = LabBeginner::copyExample(example, scriptsRoot(), &error);
  if (folder.isEmpty())
  {
    m_refusal = tr("Error: %1").arg(error);
    refreshLive();
    return false;
  }
  m_projectFolder = folder;
  m_stepExample = example;
  m_doneSteps.clear();
  // the props first, so the colours of the first run find them by name
  if (m_hooks.showPropsTab) m_hooks.showPropsTab();
  if (m_hooks.addProp)
    for (const LabExample::Prop& prop : example.props) m_hooks.addProp(prop);
  const QList<LabExample::Entry> steps = example.steps();
  for (int i = 0; i < steps.size(); ++i)
    if (steps.at(i).control == QLatin1String("Props") || steps.at(i).control == QLatin1String("Live scripts")) m_doneSteps.insert(i);
  m_stepsShown = true;
  if (!watchFolder(folder)) return false;
  showPage(LivePage);
  m_effectsBox->setChecked(true);
  refreshLive();
  return true;
}

bool ScriptLabPanel::makeNewFolder(const QString& name)
{
  QString error;
  const QString folder = LabBeginner::makeFolder(scriptsRoot(), name, &error);
  if (folder.isEmpty())
  {
    m_refusal = tr("Error: %1").arg(error);
    refreshLive();
    return false;
  }
  return watchFolder(folder);
}

void ScriptLabPanel::chooseNewFolder()
{
  bool ok = false;
  const QString name = QInputDialog::getText(this, tr("New folder"),
                                             tr("Name of the new folder. It goes in %1.").arg(QDir::toNativeSeparators(scriptsRoot())),
                                             QLineEdit::Normal, tr("My object"), &ok);
  if (ok) makeNewFolder(name);
}

QAction* ScriptLabPanel::makeStepsAction(QObject* parent)
{
  QAction* action = new QAction(tr("Script Lab steps"), parent);
  action->setObjectName("helpScriptLabStepsAction");
  action->setToolTip(tr("Show the steps of the example project again."));
  action->setStatusTip(action->toolTip());
  connect(action, &QAction::triggered, this, &ScriptLabPanel::showSteps);
  return action;
}

void ScriptLabPanel::showSteps()
{
  // no example made yet: the steps of the first example, whose Watch step makes the project
  if (!m_stepExample.isValid()) m_stepExample = LabBeginner::firstExample(exampleObjectsFolder());
  m_stepsShown = true;
  showPage(LivePage);
  refreshSteps();
  if (m_stepCard->isVisible())
  {
    for (QPushButton* b : m_stepCard->stepButtons())
      if (b->isEnabled()) { b->setFocus(Qt::OtherFocusReason); break; }
    if (QScrollArea* scroll = qobject_cast<QScrollArea*>(m_pages[LivePage])) scroll->ensureWidgetVisible(m_stepCard);
  }
}

void ScriptLabPanel::showLiveScripts()
{
  // the Live scripts tab to the front; its page from the top, where the folder (or the empty
  // state with Watch folder) is
  showPage(LivePage);
  QScrollArea* scroll = qobject_cast<QScrollArea*>(m_pages[LivePage]);
  if (scroll) scroll->verticalScrollBar()->setValue(0);
  // a short frame round the tab and round the top of the page, so the press always shows
  // something, also when the page was open already (the project owner, 24 Nov 2026)
  const bool dark = isDark(palette());
  QTabBar* bar = m_tabs->tabBar();
  if (tabIndex(LivePage) >= 0) LabHighlight::show(bar, bar->tabRect(tabIndex(LivePage)), dark);
  QWidget* top = m_liveEmpty && !m_liveEmpty->isHidden() ? static_cast<QWidget*>(m_liveEmpty) : m_liveFolder;
  if (scroll && top && scroll->widget())
  {
    const QPoint at = top->mapTo(scroll->widget(), QPoint(0, 0));
    QRect area(at, top->size());
    if (top == m_liveFolder) area = QRect(QPoint(0, at.y()), QSize(scroll->widget()->width(), top->height()));
    LabHighlight::show(scroll->widget(), area.adjusted(-SpacingXs, -SpacingXs, SpacingXs, SpacingXs), dark);
  }
  // the keys go to the tab, so Left and Right move between the pages from here (job UX-B: to the
  // page itself when it is a window of its own)
  if (tabIndex(LivePage) >= 0) bar->setFocus(Qt::OtherFocusReason);
  else m_pages[LivePage]->setFocus(Qt::OtherFocusReason);
}

void ScriptLabPanel::showHelp(const QString& topic)
{
  if (!m_helpView)
  {
    m_helpView = new LabHelpView(exampleObjectsFolder(), this);
    m_helpView->setWindowFlag(Qt::Window, true);
    // the guide page's button: watch the folder it made (job UX-D)
    connect(m_helpView, &LabHelpView::openFolderRequested, this, [this](const QString& folder) {
      if (watchFolder(folder)) showPage(LivePage);
    });
  }
  m_helpView->applyStyle(isDark(palette()));
  if (!m_helpView->showTopic(topic)) m_helpView->showTopic(QStringLiteral("start"));
  m_helpView->show();
  m_helpView->raise();
  m_helpView->activateWindow();
  m_helpView->topicList()->setFocus(Qt::OtherFocusReason);
}

void ScriptLabPanel::closeSteps()
{
  m_stepsShown = false;
  refreshSteps();
}

void ScriptLabPanel::refreshSteps()
{
  if (!m_stepCard) return;
  const bool show = m_stepsShown && m_stepExample.isValid();
  m_stepCard->setVisible(show);
  if (!show) return;
  m_stepCard->setExample(m_stepExample.title, m_stepExample.texts(QStringLiteral("see")), m_stepExample.texts(QStringLiteral("next")));
  const QList<LabExample::Entry> entries = m_stepExample.steps();
  int lastProps = -1;
  for (int i = 0; i < entries.size(); ++i)
    if (entries.at(i).control == QLatin1String("Props")) lastProps = i;
  bool allProps = !m_stepExample.props.isEmpty() && m_hooks.hasProp;
  if (allProps)
    for (const LabExample::Prop& prop : m_stepExample.props) allProps = allProps && m_hooks.hasProp(prop.name);
  QList<LabStepCard::Step> steps;
  for (int i = 0; i < entries.size(); ++i)
  {
    const LabExample::Entry& e = entries.at(i);
    LabStepCard::Step step;
    step.text = e.text;
    const QString notReady = tr("This step does not work in the program yet.");
    if (e.control == QLatin1String("Props") && i == lastProps)
    {
      step.button = tr("Add the props");
      step.enabled = bool(m_hooks.addProp);
      step.reason = notReady;
      step.done = allProps;
    }
    else if (e.control == QLatin1String("Props"))
    {
      step.button = tr("Show Props");
      step.enabled = bool(m_hooks.showPropsTab);
      step.reason = notReady;
      step.done = m_doneSteps.contains(i) || allProps;
    }
    else if (e.control == QLatin1String("Live scripts"))
    {
      step.button = tr("Show Live scripts");
      step.done = m_doneSteps.contains(i) || watchingProject();
    }
    else if (e.control == QLatin1String("Watch folder..."))
    {
      step.button = tr("Watch the folder");
      step.done = watchingProject();
    }
    else if (e.control == QLatin1String("Show effects in 3D view"))
    {
      step.button = tr("Show effects");
      step.done = m_effectsBox->isChecked() && watchingProject();
    }
    else
    {
      // Touch, Say and the dialog buttons do not reach a watched folder yet (docs/EXAMPLES.md)
      step.button = e.control;
      step.enabled = false;
      step.reason = notReady;
    }
    steps << step;
  }
  m_stepCard->setSteps(steps);
}

bool ScriptLabPanel::doStep(int index)
{
  const QList<LabExample::Entry> entries = m_stepExample.steps();
  if (index < 0 || index >= entries.size()) return false;
  const QList<LabStepCard::Step> shown = m_stepCard->steps();
  if (index < shown.size() && !shown.at(index).enabled) return false;
  const QString control = entries.at(index).control;
  int lastProps = -1;
  for (int i = 0; i < entries.size(); ++i)
    if (entries.at(i).control == QLatin1String("Props")) lastProps = i;
  bool ok = true;
  if (control == QLatin1String("Props") && index == lastProps)
  {
    if (!m_hooks.addProp) return false;
    if (m_hooks.showPropsTab) m_hooks.showPropsTab();
    for (const LabExample::Prop& prop : m_stepExample.props) ok = m_hooks.addProp(prop) && ok;
  }
  else if (control == QLatin1String("Props"))
  {
    if (!m_hooks.showPropsTab) return false;
    m_hooks.showPropsTab();
  }
  else if (control == QLatin1String("Live scripts"))
    showLiveScripts();
  else if (control == QLatin1String("Watch folder..."))
  {
    // the example project's folder; made first when there is none yet
    if (m_projectFolder.isEmpty()) ok = makeExampleProject();
    else ok = watchFolder(m_projectFolder);
  }
  else if (control == QLatin1String("Show effects in 3D view"))
    m_effectsBox->setChecked(true);
  else
    return false;
  if (ok) m_doneSteps.insert(index);
  refreshSteps();
  return ok;
}

bool ScriptLabPanel::selectPrim(const QString& name)
{
  if (!m_followBox->isChecked()) return false;
  return m_folderTree->selectPrim(name);
}

void ScriptLabPanel::buildTips()
{
  // where each tip stands: next to its word (prim, root prim and notecard over the tree;
  // sandbox under the explanation; the RLV words under the Viewer rules row)
  QVBoxLayout* outer = qobject_cast<QVBoxLayout*>(layout());
  for (const LabBeginner::Tip& tip : LabBeginner::jargonTips())
  {
    LabTipCard* card = new LabTipCard(tip, this);
    card->hide();
    const QString key = QLatin1String(tip.key);
    if (key == QLatin1String("prim") || key == QLatin1String("rootPrim") || key == QLatin1String("notecard"))
    {
      card->setParent(m_treeSection->body());
      m_treeSection->bodyLayout()->insertWidget(1, card);   // under the line on what the tree shows
    }
    else if (key == QLatin1String("channel") || key == QLatin1String("listener"))
    {
      card->setParent(m_mapSection->body());
      m_mapSection->bodyLayout()->insertWidget(1, card);   // job A4: under the line on what the map shows
    }
    else
    {
      // in a row with the panel's side margins, as the rows around it; a hidden card leaves no gap
      QHBoxLayout* row = new QHBoxLayout;
      row->setContentsMargins(SpacingM, 0, SpacingM, 0);
      row->addWidget(card);
      outer->insertLayout(key == QLatin1String("sandbox") ? outer->indexOf(m_explainFrame) + 1 : outer->indexOf(m_tabs), row);
    }
    // the tip's word is also the tooltip of its word, so it can be read again after closing
    if (key == QLatin1String("viewerRules")) m_modeLabel->setToolTip(tip.text);
    connect(card, &LabTipCard::closed, this, &ScriptLabPanel::closeTip);
    m_tipCards << card;
  }
}

LabTipCard* ScriptLabPanel::shownTip() const
{
  for (LabTipCard* card : m_tipCards)
    if (!card->isHidden()) return card;
  return nullptr;
}

void ScriptLabPanel::closeTip(const QString& key)
{
  QStringList closed = UserSettings::scriptLabTipsClosed().split(QLatin1Char(','), Qt::SkipEmptyParts);
  if (!closed.contains(key)) closed << key;
  UserSettings::setScriptLabTipsClosed(closed.join(QLatin1Char(',')));
  const bool hadFocus = shownTip() && shownTip()->isAncestorOf(focusWidget());
  refreshTips();
  // the keyboard stays in the panel: on the next tip's close button, or the panel
  if (hadFocus)
  {
    if (LabTipCard* next = shownTip()) next->closeButton()->setFocus(Qt::OtherFocusReason);
    else setFocus(Qt::OtherFocusReason);
  }
}

void ScriptLabPanel::refreshTips()
{
  if (m_tipCards.isEmpty()) return;
  // value first: no tips before something is watched, and none while hints are off (charter P4)
  const bool watching = m_watcher && m_watcher->watching();
  const QStringList closed = UserSettings::scriptLabTipsClosed().split(QLatin1Char(','), Qt::SkipEmptyParts);
  bool notecards = false;
  for (QTreeWidgetItemIterator it(m_folderTree); *it; ++it)
    if ((*it)->data(LabFolderTree::NameColumn, LabFolderTree::KindRole).toInt() == LabFolderTree::NotecardRow) notecards = true;
  const bool treeShown = m_treeSection->isVisibleTo(this) && m_treeSection->isExpanded() && m_folderTree->objectItem();
  LabTipCard* due = nullptr;
  if (m_hintsOn && watching)
    for (LabTipCard* card : m_tipCards)
    {
      const QString key = card->key();
      if (closed.contains(key)) continue;
      bool anchored = true;
      if (key == QLatin1String("prim") || key == QLatin1String("rootPrim")) anchored = treeShown;
      else if (key == QLatin1String("notecard")) anchored = treeShown && notecards;
      else if (key == QLatin1String("channel") || key == QLatin1String("listener"))
        anchored = m_mapSection->isVisibleTo(this) && m_mapSection->isExpanded() && !m_mapView->map().arrows.isEmpty();
      if (!anchored) continue;
      due = card;
      break;
    }
  for (LabTipCard* card : m_tipCards) card->setVisible(card == due);
}

// --- job A4: the channel map ------------------------------------------------------------------

void ScriptLabPanel::refreshChannelMap(const QJsonObject& folder)
{
  if (!m_mapSection) return;
  const bool shown = !folder.isEmpty();
  m_mapSection->setVisible(shown);
  m_mapEmpty->setVisible(!shown);   // job UX-B: the tab says what fills it
  // made again only when the run or its log changed (a click on a prop adds lines to the same run)
  const QString key = shown ? QStringLiteral("%1|%2|%3").arg(m_watcher ? m_watcher->folder() : QString())
                                .arg(folder.value("run").toInt())
                                .arg(folder.value("result").toObject().value("log").toArray().size())
                            : QString();
  if (shown && key == m_mapKey) return;
  m_mapKey = key;
  QPointer<LabWatcher> watcher = m_watcher;
  const LabChannelMap::Map map = shown ? LabChannelMap::fromFolderResult(folder, [watcher](const QString& file) -> QString
  {
    // only a script the watch knows: never a path out of the folder
    const QString path = watcher ? watcher->absolutePath(file) : QString();
    QFile in(path);
    if (path.isEmpty() || QFileInfo(path).size() > LabWatcher::MAX_FILE_BYTES || !in.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(in.read(LabWatcher::MAX_FILE_BYTES));
  }) : LabChannelMap::Map();
  m_mapView->setMap(map);
  const int messages = map.messages();
  const int unheard = map.unheard();
  QString count = messages == 0 ? tr("none") : messages == 1 ? tr("1 message") : tr("%1 messages").arg(messages);
  if (unheard > 0) count = tr("%1, %2").arg(count, unheard == 1 ? tr("1 problem") : tr("%1 problems").arg(unheard));
  m_mapSection->setCount(count);
  QStringList problems;
  if (unheard > 0)
    problems << (unheard == 1 ? tr("1 arrow reached nobody. It has a cross and says why.")
                              : tr("%1 arrows reached nobody. Each has a cross and says why.").arg(unheard));
  if (map.truncated) problems << tr("The map shows the first %1 arrows only.").arg(LabChannelMap::MAX_ARROWS);
  m_mapProblems->setText(problems.join(QLatin1Char(' ')));
  QPalette p = m_mapProblems->palette();
  p.setColor(QPalette::WindowText, unheard > 0 ? dangerColour(isDark(palette())) : mutedColour(isDark(palette())));
  m_mapProblems->setPalette(p);
  m_mapProblems->setVisible(!problems.isEmpty());
}

bool ScriptLabPanel::activateMapBox(int box)
{
  const LabChannelMap::Map& map = m_mapView->map();
  if (box < 0 || box >= map.boxes.size() || map.boxes[box].kind != LabChannelMap::BoxKind::Part) return false;
  const QString name = map.boxes[box].name;
  if (!m_folderTree->selectPrim(name)) return false;
  // as a click on the row: the prop lights up when selection follows (job U3)
  if (m_followBox->isChecked() && m_hooks.selectProp) m_hooks.selectProp(name);
  return true;
}

bool ScriptLabPanel::activateMapArrow(int arrow)
{
  const LabChannelMap::Map& map = m_mapView->map();
  if (arrow < 0 || arrow >= map.arrows.size()) return false;
  const LabChannelMap::Arrow& a = map.arrows[arrow];
  if (a.file.isEmpty() || a.line <= 0) return false;
  if (a.file != m_shownFile && !showScript(a.file)) return false;
  // job UX-B: the editor is on Live scripts; that tab comes to the front unless it is in sight
  // already (popped out beside the map)
  if (!isPoppedOut(LivePage)) showPage(LivePage);
  showScriptPlace(a.line, a.column);
  return true;
}

// --- job UX-B: one window with its own tabs ----------------------------------------------------

QWidget* ScriptLabPanel::makePageScroll(QWidget* content, const char* name)
{
  QScrollArea* scroll = new QScrollArea(m_tabs);
  scroll->setObjectName(QString::fromLatin1(name));
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setWidget(content);
  return scroll;
}

QString ScriptLabPanel::pageKey(Page page)
{
  switch (page)
  {
    case LivePage: return QStringLiteral("live");
    case CommandsPage: return QStringLiteral("commands");
    case MapPage: return QStringLiteral("map");
    case PlayerPage: return QStringLiteral("player");
    default: return QString();
  }
}

QString ScriptLabPanel::pageName(Page page) const
{
  switch (page)
  {
    case LivePage: return tr("Live scripts");
    case CommandsPage: return tr("Type commands");
    case MapPage: return tr("Who talks to whom");
    case PlayerPage: return tr("Player");
    default: return QString();
  }
}

void ScriptLabPanel::addPageTab(Page page)
{
  // in the order of the pages: after every page before it that is in the window
  int at = 0;
  for (int p = 0; p < page; ++p)
    if (tabIndex(Page(p)) >= 0) ++at;
  const int index = m_tabs->insertTab(at, m_pages[page], pageName(page));
  // the tips: Live and Type commands as before, the map and player as in the mockup (LABTWO-08, -09)
  static const char* const tips[PageCount] = {
    QT_TR_NOOP("Live: your .lsl scripts run each time you save them"), QT_TR_NOOP("Commands: type RLV lines by hand"),
    QT_TR_NOOP("Arrows between the parts: who sends what to whom."), QT_TR_NOOP("Step back and forth through your recorded steps.")};
  m_tabs->setTabToolTip(index, tr(tips[page]));
  m_tabs->setTabWhatsThis(index, tr(tips[page]));
  stylePageTabs();
}

void ScriptLabPanel::stylePageTabs()
{
  if (!m_tabs) return;
  const bool dark = isDark(palette());
  const Tokens& t = tokens(dark);
  // Lucide icons, in the text colour: as the mockup draws the tabs (file-code, terminal, network, rotate-ccw)
  static const char* const icons[PageCount] = {"file-code", "terminal", "network", "rotate-ccw"};
  for (int p = 0; p < PageCount; ++p)
  {
    const int index = tabIndex(Page(p));
    if (index < 0) continue;
    m_tabs->setTabIcon(index, LucideIcons::icon(QString::fromLatin1(icons[p]), QColor(QLatin1String(t.text)), QColor(QLatin1String(t.textDisabled))));
  }
  if (m_popOutButton)
  {
    const int icon = TextSize::scaled(16);
    QPalette ink = palette();
    ink.setColor(QPalette::WindowText, QColor(QLatin1String(t.text)));
    ink.setColor(QPalette::Disabled, QPalette::WindowText, QColor(QLatin1String(t.textDisabled)));
    m_popOutButton->setIconSize(QSize(icon, icon));
    m_popOutButton->setIcon(PanelTitleBar::drawSymbol(PanelTitleBar::Symbol::Float, ink, icon, devicePixelRatioF()));
    m_popOutButton->setStyleSheet(QStringLiteral("QToolButton { border: none; border-radius: %1px; padding: 0 %2px; color: %4; }"
                                                 "QToolButton:hover { background: %3; }"
                                                 "QToolButton:disabled { color: %5; }"
                                                 "QToolButton:focus { border: 2px solid %6; }")
                                  .arg(RadiusControl).arg(SpacingS)
                                  .arg(QLatin1String(t.surfaceRaised), QLatin1String(t.text), QLatin1String(t.textMuted), QLatin1String(t.focusRing)));
  }
  for (LabPopWindow* window : m_popWindows)
    if (window) window->applyStyle(dark);
}

int ScriptLabPanel::tabIndex(Page page) const
{
  if (!m_tabs || page < 0 || page >= PageCount || !m_pages[page]) return -1;
  return m_tabs->indexOf(m_pages[page]);
}

ScriptLabPanel::Page ScriptLabPanel::currentPage() const
{
  QWidget* current = m_tabs ? m_tabs->currentWidget() : nullptr;
  for (int p = 0; p < PageCount; ++p)
    if (current && m_pages[p] == current) return Page(p);
  return LivePage;
}

LabPopWindow* ScriptLabPanel::popWindow(Page page) const
{
  return page >= 0 && page < PageCount ? m_popWindows[page] : nullptr;
}

void ScriptLabPanel::showPage(Page page)
{
  if (page < 0 || page >= PageCount) return;
  if (LabPopWindow* window = m_popWindows[page])
  {
    if (m_popWindowsShown) window->show();
    window->raise();
    window->activateWindow();
    return;
  }
  m_tabs->setCurrentIndex(tabIndex(page));
}

bool ScriptLabPanel::popOut(Page page, const QPoint& at)
{
  if (page < 0 || page >= PageCount || m_popWindows[page] || m_tabs->count() <= 1) return false;
  const QString key = pageKey(page);
  QWidget* content = m_pages[page];
  m_tabs->removeTab(m_tabs->indexOf(content));
  LabPopWindow* window = new LabPopWindow(key, pageName(page), this);
  m_popWindows[page] = window;
  window->setMinimumSize(minimumPanelSize());
  window->setPage(content);
  // where it was last time, else big beside the main window (or where a dragged tab was let go)
  QRect place = m_popPlaces.value(key);
  if (!place.isValid())
  {
    QScreen* screen = at.isNull() ? this->screen() : QGuiApplication::screenAt(at);
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect area = screen ? screen->availableGeometry() : QRect(0, 0, 1024, 768);
    place = QRect(QPoint(), LabPopWindow::firstSize(area).expandedTo(minimumPanelSize()));
    if (at.isNull()) place.moveCenter(area.center());
    else place.moveTopLeft(at - QPoint(TextSize::scaled(40), TextSize::scaled(16)));
  }
  window->setGeometry(LabPopWindow::fitOnScreen(place));
  m_popPlaces.insert(key, window->geometry());
  connect(window, &LabPopWindow::putBackRequested, this, [this, page]() { putBack(page); });
  connect(window, &LabPopWindow::placeChanged, this, [this, page]()
  {
    if (LabPopWindow* w = m_popWindows[page]; w && w->isVisible()) { m_popPlaces.insert(pageKey(page), w->geometry()); saveWindowState(); }
  });
  if (m_popWindowsShown)
  {
    window->show();
    window->raise();
    if (m_windowStateReady) window->activateWindow();
  }
  stylePageTabs();
  saveWindowState();
  refreshPopOutButton();
  updateWorkLayout();
  refreshTips();
  return true;
}

bool ScriptLabPanel::putBack(Page page)
{
  if (page < 0 || page >= PageCount || !m_popWindows[page]) return false;
  LabPopWindow* window = m_popWindows[page];
  m_popWindows[page] = nullptr;
  if (window->isVisible()) m_popPlaces.insert(pageKey(page), window->geometry());
  window->takePage();
  window->hide();
  window->deleteLater();
  addPageTab(page);
  m_tabs->setCurrentIndex(tabIndex(page));
  saveWindowState();
  refreshPopOutButton();
  updateWorkLayout();
  refreshTips();
  return true;
}

void ScriptLabPanel::refreshPopOutButton()
{
  if (!m_popOutButton) return;
  const bool can = m_tabs->count() > 1;
  m_popOutButton->setEnabled(can);
  // greyed, it says why (charter P6)
  const QString tip = can ? tr("Pop out \"%1\" into a window of its own. Close that window, or press Put back, to bring it back.")
                                .arg(pageName(currentPage()))
                          : tr("The last tab stays in Script Lab. Put another tab back first.");
  m_popOutButton->setToolTip(tip);
  m_popOutButton->setStatusTip(tip);
  m_popOutButton->setAccessibleDescription(tip);
}

void ScriptLabPanel::setPopWindowsShown(bool shown)
{
  m_popWindowsShown = shown;
  for (LabPopWindow* window : m_popWindows)
    if (window) window->setVisible(shown);
}

QString ScriptLabPanel::windowState() const
{
  LabTabState::State state;
  state.current = pageKey(currentPage());
  for (int p = 0; p < PageCount; ++p)
    if (m_popWindows[p]) state.popped << pageKey(Page(p));
  state.places = m_popPlaces;
  return LabTabState::format(state);
}

void ScriptLabPanel::applyWindowState(const QString& text)
{
  QStringList known;
  for (int p = 0; p < PageCount; ++p) known << pageKey(Page(p));
  const LabTabState::State state = LabTabState::parse(text, known);
  const bool ready = m_windowStateReady;
  m_windowStateReady = false;   // nothing is saved half way
  for (int p = 0; p < PageCount; ++p)
    if (m_popWindows[p] && !state.popped.contains(pageKey(Page(p)))) putBack(Page(p));
  m_popPlaces = state.places;
  for (const QString& key : state.popped)
  {
    const int p = int(known.indexOf(key));
    if (p >= 0 && !m_popWindows[p]) popOut(Page(p));
  }
  const int current = int(known.indexOf(state.current));
  if (current >= 0 && tabIndex(Page(current)) >= 0) m_tabs->setCurrentIndex(tabIndex(Page(current)));
  m_windowStateReady = ready;
  saveWindowState();
  refreshPopOutButton();
}

void ScriptLabPanel::saveWindowState()
{
  if (m_windowStateReady) UserSettings::setScriptLabWindows(windowState());
}

void ScriptLabPanel::resetWindows()
{
  for (int p = 0; p < PageCount; ++p) putBack(Page(p));
  m_popPlaces.clear();
  m_tabs->setCurrentIndex(tabIndex(LivePage));
  UserSettings::setScriptLabSized(0);
  saveWindowState();
}

int ScriptLabPanel::bigDockedWidth(const QSize& window)
{
  // most of the main window (the owner: big by default, so everything can be seen), never less
  // than the panel's own first size and never more than leaves the 3D view a strip
  const int wanted = std::max(defaultSize().width(), window.width() * 3 / 5);
  return std::max(minimumPanelSize().width(), std::min(wanted, window.width() - TextSize::scaled(240)));
}

QSize ScriptLabPanel::bigFloatingSize(const QRect& screen)
{
  const QSize wanted(screen.width() * 4 / 5, screen.height() * 4 / 5);
  return wanted.expandedTo(minimumPanelSize()).boundedTo(screen.size());
}

bool ScriptLabPanel::s_layoutRestoring = false;

bool ScriptLabPanel::openBig(QMainWindow* window, QDockWidget* dock)
{
  if (!window || !dock || dock->isHidden()) return false;
  const int sized = UserSettings::scriptLabSized();
  if (dock->isFloating())
  {
    if (sized & 2) return false;
    UserSettings::setScriptLabSized(sized | 2);
    QScreen* screen = window->screen() ? window->screen() : QGuiApplication::primaryScreen();
    if (!screen) return false;
    const QRect area = screen->availableGeometry();
    QRect place(QPoint(), bigFloatingSize(area));
    place.moveCenter(area.center());
    dock->setGeometry(place);
    return true;
  }
  if (sized & 1) return false;
  UserSettings::setScriptLabSized(sized | 1);
  const Qt::DockWidgetArea area = window->dockWidgetArea(dock);
  if (area == Qt::TopDockWidgetArea || area == Qt::BottomDockWidgetArea)
    window->resizeDocks({dock}, {std::max(minimumPanelSize().height(), window->height() * 3 / 5)}, Qt::Vertical);
  else
  {
    window->resizeDocks({dock}, {bigDockedWidth(window->size())}, Qt::Horizontal);
    // sharing its side with another panel (under Selected joint at the first start): most of the height too
    window->resizeDocks({dock}, {window->height()}, Qt::Vertical);
  }
  return true;
}
