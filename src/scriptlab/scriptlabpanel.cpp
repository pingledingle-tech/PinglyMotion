/*
 * PinglyMotion Script Lab - the Script Lab panel (issue 288): the constructor, the parts, the mode, the style, the events, the dock and its size
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCheckBox>
#include <QTimer>
#include <QKeySequence>
#include <QShortcut>
#include <QResizeEvent>
#include <QSizeGrip>
#include <QComboBox>
#include <QDockWidget>
#include <QScrollArea>
#include <QTabWidget>
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
#include <QToolButton>
#include <QVBoxLayout>

#include "labtree.h"
#include "objectsview.h"
#include "paneltitlebar.h"
#include "textsize.h"
#include "spherecontrols.h"
#include "lucideicons.h"
#include "labrestrictionchoice.h"
#include "theme.h"
#include "usersettings.h"
#include "scriptlabpanelui.h"

#include "scriptlabpanel.h"

using namespace LabUi;

const char* const ScriptLabPanel::DockName = "scriptLabDock";
const char* const ScriptLabPanel::ViewActionName = "viewScriptLabAction";
const char* const ScriptLabPanel::DefaultObject = "panel";


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
  // job UX-B: and for the tabs with Pop out beside them, without scroll arrows (five since queue 1 item 5, Improve a setup)
  return QSize(TextSize::scaled(740), TextSize::scaled(720));
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

