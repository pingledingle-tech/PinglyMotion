/*
 * PinglyMotion Script Lab - the Script Lab panel: the Live page (issue 292), the watched folder and the log
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCheckBox>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QTextDocument>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonArray>
#include <QListWidget>
#include <QSet>
#include <QTextBlock>
#include <QUrl>
#include <QEvent>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QSpinBox>

#include "labtree.h"
#include "channelmapview.h"
#include "objectsview.h"
#include "lsl/lint.h"
#include "lucideicons.h"
#include "labplayer.h"
#include "theme.h"
#include "usersettings.h"
#include "scriptlabpanelui.h"

#include "labimproveview.h"
#include "scriptlabpanel.h"

using namespace LabUi;

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
  // queue 1 item 5, parts b and c: the page Improve a setup
  QWidget* improvePage = new QWidget(m_tabs);
  improvePage->setObjectName("scriptLabImprovePage");
  QVBoxLayout* improveLayout = new QVBoxLayout(improvePage);
  improveLayout->setContentsMargins(SpacingM, SpacingM, SpacingM, SpacingM);
  improveLayout->setSpacing(SpacingS);
  m_improve = new LabImproveView(improvePage);
  improveLayout->addWidget(m_improve);
  improveLayout->addStretch(1);
  connect(m_improve, &LabImproveView::importRequested, this, &ScriptLabPanel::chooseFolder);
  connect(m_improve, &LabImproveView::recordRequested, this, [this]()
  {
    showPage(PlayerPage);
    setRecording(true);
  });
  connect(m_improve, &LabImproveView::placeRequested, this, [this](const QString& file, int line)
  {
    if (file != m_shownFile && !showScript(file)) return;
    if (!isPoppedOut(LivePage)) showPage(LivePage);
    showScriptPlace(line, 1);
  });

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
  m_pages[ImprovePage] = makePageScroll(improvePage, "scriptLabImproveScroll");
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
  refreshImprove();

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

