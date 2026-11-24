/*
 * PinglyMotion Script Lab - the Script Lab panel (issue 288)
 *
 * A hidden panel, opened with View > Script Lab, where the user types an RLV
 * owner-say line such as "@detach=n,sendim=n" and sees what the RLV core
 * (rlvstate.h) makes of it: the restrictions each object holds, every action
 * shown Allowed or Blocked (word and colour), and the replies a script would
 * hear. It follows the core's changed() signal, so lines sent through the
 * control commands (rlv_command, rlv_reset) show up here too.
 *
 * In the new design (docs/mockup/index.html, docs/UX_DECISIONS.md 6d, slice 1):
 * the "Viewer rules" RLV / RLVa switch with an "N held" chip on top, the pages
 * "Live scripts" (with an empty state while no folder is watched) and "Type
 * commands", and folding parts with counts (scriptlabsection.h).
 * Slice 2: the OK or Error summary line with its icon, Run again, the "Try:"
 * example lines, each part's open or folded state remembered, the one-line
 * explanation shown with hints, and the newest dialog's buttons inside Objects.
 * Slice 3: the simulated inventory of an object of the world (items, type,
 * notecard lines), the last part of the Live page.
 * Lane 1 job F1: the Script part is an editor in our own text box (labeditor.h);
 * Save writes the file and the folder watch runs it at once.
 * Lane 1 job D: Watch folder watches the folder as one object (LabFolder, the
 * watcher's "linkset" mode); the "Object" part shows it as a tree (labtree.h)
 * with a colour dot, the sums and the memory per script, and a click on a
 * script shows it. "How to lay out the folder" is the short maker guide. The
 * example scripts stay separate examples (each runs on its own).
 *
 * Job U3, Script Lab for beginners (docs/UX_DECISIONS.md, 6o): the tree on the
 * left and the editor on the right when the panel is wide, an empty state in
 * plain words with "Make example project" first (it copies the first example
 * to the user's own folder, watches it, puts its props in the scene, shows the
 * effects and the steps), "New folder", the example's steps with ticks and a
 * button each, selection that follows between the tree and the 3D view, short
 * explanations of the jargon words, and clearer states.
 *
 * Colours are the success and danger tokens of docs/design_tokens.json; the
 * state is always written as a word as well (design charter P10).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SCRIPTLABPANEL_H
#define SCRIPTLABPANEL_H

#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QWidget>

#include <functional>

#include "cameraoverride.h"
#include "environmentoverride.h"
#include "viewmodesoverride.h"
#include "effectspreview.h"
#include "labbeginner.h"
#include "labeditor.h"
#include "labprimprops.h"
#include "labtint.h"
#include "labdialogbox.h"
#include "labwatcher.h"
#include "rlvstate.h"


class QAbstractButton;
class QAction;
class QBoxLayout;
class QCheckBox;
class QComboBox;
class QGridLayout;
class QSpinBox;
class QDockWidget;
class QFrame;
class ScriptLabObjectsView;
class ScriptLabFrame;
class ScriptLabSection;
class SphereControls;
class LabFolderTree;
class LabPlayer;
class ChannelMapView;
namespace lsl { class Session; }
class QLabel;
class QLineEdit;
class QMainWindow;
class QPlainTextEdit;
class QListWidget;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QVBoxLayout;

class ScriptLabPanel : public QWidget
{
  Q_OBJECT

  public:
    // object names, used by the window, the saved layout and the tests
    static const char* const DockName;        // "scriptLabDock"
    static const char* const ViewActionName;  // "viewScriptLabAction"
    // the object id lines typed in the panel are sent from
    static const char* const DefaultObject;   // "panel"
    // the two pages, in the mockup's order
    enum Page { LivePage = 0, CommandsPage = 1 };

    // Size (the project owner, 23 Nov 2026, 17:48: the window could not be resized and needs
    // more room). The panel starts at defaultSize() when docked or floated for the first time,
    // never gets smaller than minimumPanelSize(), and a floating panel has a size grip in its
    // bottom right corner (the custom title bar leaves the floating window without a system
    // frame, so its edges cannot be dragged). Both sizes follow the text size.
    static QSize defaultSize();
    static QSize minimumPanelSize();
    QSize sizeHint() const override { return defaultSize(); }
    // the grip: shown only while the panel floats
    QWidget* sizeGrip() const { return m_sizeGrip; }
    void setFloatingGrip(bool floating);

    explicit ScriptLabPanel(QWidget* parent = nullptr);

    // The RLV state to show and change. The panel does not own it (the
    // control API does, so the panel and rlv_command share one state).
    void setState(RlvState* state);
    RlvState* state() const { return m_state; }

    // The world of the last LSL run, for the Objects and Dialog part (step 5). Not owned.
    void setSession(lsl::Session* session);
    ScriptLabObjectsView* objectsView() const { return m_objectsView; }

    // Runs one line from the command field's object, as Enter does. Returns
    // false when the whole line was refused (the log says why).
    bool runLine(const QString& text);
    // Removes every restriction of every object (the control command rlv_reset).
    void resetAll();

    // --- Live (issue 292): the folder watch to show and start. Not owned
    // (the control API owns it, so the panel and lab_watch share one watch).
    void setWatcher(LabWatcher* watcher);
    LabWatcher* watcher() const { return m_watcher; }
    // starts watching a folder, as Watch folder does after the folder is
    // chosen; false when refused (the status line says why). The folder is
    // one object (lane 1 job D): every save runs all its scripts together.
    bool watchFolder(const QString& folder);
    // --- the folder as one object (lane 1 job D)
    // the tree of the "Object" part (object, prims, scripts, notecards)
    LabFolderTree* folderTree() const { return m_folderTree; }
    // the maker guide's text and its example folder, in the "How to lay out the folder" part
    QLabel* guideText() const { return m_guideText; }
    QLabel* guideExample() const { return m_guideExample; }
    // shows one script of the watched object in the editor, as a click on it in the tree does;
    // false when the folder is not watched as one object or the script is not in it
    bool showScript(const QString& file);
    // the script the Script part shows ("" when none), relative to the folder
    QString shownScript() const { return m_shownFile; }
    // --- props as prims (lane 1 job E): the colour and transparency the scripts of the watched
    // object gave its prims, while "Show effects in 3D view" is ticked; empty otherwise. The
    // window gives each tint to the prop of the same name (Prop::setTint) and clears the rest.
    QList<LabTint::Tint> propTints() const { return m_propTints; }
    // --- props from prims (job A1): while "Show effects in 3D view" is on and a folder is watched as
    // one object, every prim gets a prop of its name (LabPrimProps::propFor, through the scene hooks),
    // and a prop whose prim a script moved, scaled or turned stands where the script put it
    // (LabPrimProps::moved). Turning the effects off, a run without moves or Stop watching puts the
    // props back where they stood.
    bool primPropsShown() const;
    // the moves of the last run that the props show (empty while primPropsShown() is false)
    QList<LabPrimProps::Move> primMoves() const { return m_primMoves; }
    // a click on the prop of that name, as the 3D view sends it: a touch to the scripts in that prim
    // (LabWatcher::touch), with "You clicked <name>." in the event log. False when the props are not
    // shown, no prim has that name, or the touch was refused (the status line says why).
    // at: where the click was in the menu box's host (the 3D view), for the menu box (job A2)
    bool touchPrim(const QString& name, const QPoint& at = QPoint(-1, -1));
    // --- the menu box over the 3D view (job A2): when a script of the watched object shows a menu
    // (llDialog, llTextBox) while the props are shown, a LabDialogBox shows it over the host (the
    // window gives the 3D view) next to the last click. A button answers through LabWatcher::answerDialog
    // (the panel's own menu sends the same input to the world of the last lsl_run). The box closes
    // after an answer, after Ignore, when the effects go off and when the folder runs again.
    void setMenuBoxHost(QWidget* host);
    LabDialogBox* menuBox() const { return m_menuBox; }
    // answers the menu the box shows, as a click on its button does; false when nothing was sent
    bool answerMenu(const QString& button);
    // --- record and replay (job A3): "Record what I do" records clicks on props, menu answers and
    // typed RLV lines as a recipe in the object folder (LabWatcher::startRecording); every save replays
    // it and the line under the button says what changed since last time. Beginner mode shows only the
    // button and that line; Expert mode adds the folded "Recipe" part with the file and Replay now.
    QPushButton* recordButton() const { return m_recordButton; }
    QLabel* recipeStatus() const { return m_recipeStatus; }
    ScriptLabSection* recipeSection() const { return m_recipeSection; }
    QLabel* recipeFileLine() const { return m_recipeFileLine; }
    QPlainTextEdit* recipeText() const { return m_recipeText; }
    QPushButton* replayButton() const { return m_replayButton; }
    // starts or stops recording, as the button does; false when nothing changed (the line says why)
    bool setRecording(bool on);
    // replays the recipe now, as Replay now does; false when refused (the line says why)
    bool replayNow();
    // --- the player (job B3, labplayer.h): under the recipe line, the newest replay as a film. While
    // the user looks at a moment (LabPlayer::showing), the props show that moment's colours and places
    // instead of now; a click on a prop goes back to now first (it is an input to the world of now).
    LabPlayer* player() const { return m_player; }
    // --- the channel map (job A4): who talks to whom in the last folder run (LabChannelMap), in the
    // folding part "Who talks to whom" after the event log. Shown while a folder is watched as one
    // object; folded in Beginner mode, open in Expert mode (switching the mode folds or opens it).
    ScriptLabSection* mapSection() const { return m_mapSection; }
    ChannelMapView* channelMap() const { return m_mapView; }
    QLabel* mapLine() const { return m_mapLine; }
    QLabel* mapProblems() const { return m_mapProblems; }
    // a box: picks its prim in the tree (and lights its prop when selection follows); false for a
    // box that is not a prim of the object
    bool activateMapBox(int box);
    // an arrow: shows the script line that sent it in the editor; false when it came from outside
    bool activateMapArrow(int arrow);
    // Expert mode (View > Expert mode): the panel starts with UserSettings and follows it
    void setExpertMode(bool expert);
    bool expertMode() const { return m_expert; }

    // stops watching, as Stop watching does
    void stopWatching();
    // watches the example scripts that come with the program (examples/scriptlab), as
    // "Try the example scripts" does, and runs rlv_lock.lsl; false when refused
    bool tryExampleScripts();
    // runs the script shown again from the start, as Run again does; false when there is none
    bool runAgain();
    // the folder "Try the example scripts" watches; the tests may give another one
    QString exampleScriptsFolder() const;
    void setExampleScriptsFolder(const QString& folder) { m_examplesFolder = folder; refreshLive(); }

    // --- Script Lab for beginners (job U3) -----------------------------------------
    // What the panel asks of the scene; the window fills these in (the tests give their own).
    struct SceneHooks
    {
      std::function<bool(const LabExample::Prop&)> addProp;   // puts the prop in the scene; true when it is there
      std::function<bool(const QString&)> hasProp;            // a prop of that name is in the scene
      std::function<bool(const QString&)> selectProp;         // lights up the prop of that name; false when none
      std::function<void()> showPropsTab;                     // shows Props in the Joints panel
      // job A1, props that move: reads the place of the prop of that name (false when none) and
      // puts the prop of that name at a place; false when there is no such prop
      std::function<bool(const QString&, LabExample::Prop&)> propPlace;
      std::function<bool(const LabExample::Prop&)> placeProp;
    };
    void setSceneHooks(const SceneHooks& hooks) { m_hooks = hooks; refreshSteps(); }
    // "Make example project": copies the first example of exampleObjectsFolder() to scriptsRoot()
    // (an earlier copy is used as it is), puts its props in the scene, watches it, shows the effects,
    // the Live page and the steps. False when refused (the status line says why).
    bool makeExampleProject();
    // "New folder": makes scriptsRoot()/<name> and watches it; false when refused (the status line says why)
    bool makeNewFolder(const QString& name);
    // the examples "Make example project" copies from (examples/objects next to the program)
    QString exampleObjectsFolder() const;
    void setExampleObjectsFolder(const QString& folder) { m_objectsFolder = folder; }
    // where the program makes folders: Documents/PinglyMotion/Scripts (the tests give another one)
    QString scriptsRoot() const;
    void setScriptsRoot(const QString& folder) { m_scriptsRoot = folder; }
    // the example project made last ("" when none)
    QString projectFolder() const { return m_projectFolder; }
    // the steps of the example (Help > Script Lab steps opens them again)
    void showSteps();
    void closeSteps();
    bool stepsShown() const { return m_stepsShown; }
    LabStepCard* stepCard() const { return m_stepCard; }
    // does step <index> as its button does; false when it cannot be done
    bool doStep(int index);
    // the Help menu's "Script Lab steps" item (the window adds it and shows the panel)
    QAction* makeStepsAction(QObject* parent);
    // the empty state's buttons
    QPushButton* makeExampleButton() const { return m_makeExample; }
    QPushButton* newFolderButton() const { return m_newFolder; }
    // the line over the tree that says what it shows
    QLabel* treeLine() const { return m_treeLine; }
    // selection that follows between the tree and the 3D view, on at start (remembered)
    QCheckBox* followSelectionBox() const { return m_followBox; }
    // picks the prim of that name in the tree, as a click on its prop does; false when it is not
    // in the tree or following is off. Sends nothing back.
    bool selectPrim(const QString& name);
    // the tree beside the editor when the panel is at least sideBySideWidth() wide, under it otherwise
    bool sideBySide() const { return m_sideBySide; }
    static int sideBySideWidth();
    QWidget* treeColumn() const { return m_treeColumn; }
    QWidget* editorColumn() const { return m_editorColumn; }
    // the short explanations of jargon words: one shows at a time, a closed one never again
    QList<LabTipCard*> tipCards() const { return m_tipCards; }
    LabTipCard* shownTip() const;
    void closeTip(const QString& key);
    // the maker guide in two parts: rules (guideText) and warnings
    QLabel* guideWarnings() const { return m_guideWarnings; }
    QLabel* guideRulesHeading() const { return m_guideRulesHeading; }
    QLabel* guideWarningsHeading() const { return m_guideWarningsHeading; }
    // why Run again and Open in editor are greyed, as a readable line under them
    QLabel* scriptButtonsReason() const { return m_scriptButtonsReason; }

    // --- Viewer rules (the panel in the new design): plain RLV or RLVa, as the RLV state answers.
    // Switching removes every restriction, as after a relog (RlvState::setMode).
    void setMode(Rlv::Mode mode);
    // --- Effects in the 3D view (issue 293): the preview the "Show effects in 3D view" box turns
    // on and off (off at start). Not owned (the window owns it and gives it to the 3D view).
    void setEffects(EffectsPreview* preview);
    EffectsPreview* effects() const { return m_effects; }
    // a clear on/off button since job U3 (it was a tick box)
    QPushButton* effectsBox() const { return m_effectsBox; }
    QLabel* effectsStatus() const { return m_effectsStatus; }
    // --- Camera limits in the 3D view (job 7b): the override the Commands page's "Show camera limits
    // in 3D view" box turns on and off (off at start). Not owned (the window owns it and gives it to the 3D view).
    void setCameraOverride(CameraOverride* cameraOverride);
    CameraOverride* cameraOverride() const { return m_camera; }
    QCheckBox* cameraBox() const { return m_cameraBox; }
    QLabel* cameraStatus() const { return m_cameraStatus; }
    // the sky and draw distance shown in the 3D view (job 7c), off at start
    void setEnvironmentOverride(EnvironmentOverride* environmentOverride);
    QCheckBox* environmentBox() const { return m_environmentBox; }
    QLabel* environmentStatus() const { return m_environmentStatus; }
    // the vision sphere's sliders and RLV line, in the "Vision sphere" part under the environment box
    SphereControls* sphereControls() const { return m_sphereControls; }
    // the view modes shown in the 3D view (job 7c), off at start
    void setViewModesOverride(ViewModesOverride* viewModesOverride);
    QCheckBox* viewModesBox() const { return m_viewModesBox; }
    QLabel* viewModesStatus() const { return m_viewModesStatus; }
    // --- Input buttons (issue 308): send a simulated touch, a chat line or a dialog answer to the world of
    // the last lsl_run (the one the Objects part shows), as the control command lsl_event does (same code).
    // Each returns false when nothing was sent (no world, every script stopped, empty chat text, unknown
    // object, no dialog); the reason is in inputResult() and, for the buttons, in inputHint().
    // object: the object's name; empty means the first. The world runs on one simulated second.
    bool sendTouch(const QString& object);
    // name empty: the owner's avatar says it; every object within range hears it (open chat range)
    bool sendChat(int channel, const QString& name, const QString& text);
    // answers the newest dialog with the button of that text
    bool answerDialog(const QString& button);
    // the text of a run's event log entry, as the Live page shows it
    static QString logLine(const QJsonObject& entry);

    // The colour tokens the panel uses, from docs/design_tokens.json; dark
    // or light is chosen from the window colour of the palette.
    static QColor successColour(bool dark);
    static QColor dangerColour(bool dark);
    static QColor mutedColour(bool dark);
    static bool isDark(const QPalette& palette);

    // Builds the panel's dock the way the other panels are built (movable,
    // floatable, closable, PanelTitleBar), with its View menu action named
    // ViewActionName and the text "Script Lab". Hidden until asked for.
    static QDockWidget* makeDock(QMainWindow* window, ScriptLabPanel* panel);

    // --- Hints (Help > Show hints, the "Tips on" switch): the one-line explanation on top shows with them
    void setHintsOn(bool on);
    bool hintsOn() const { return m_hintsOn; }
    QLabel* explanation() const { return m_explain; }
    QWidget* explanationRow() const { return m_explainFrame; }

    // The example lines of the "Try:" row on the Type commands page, in order
    static QStringList tryLines();

    // Each part's open or folded state as saved in the settings ("key=open,key=folded"),
    // and back. A part not named keeps the state it had.
    QString partsState() const;
    void applyPartsState(const QString& state);

    // read by the tests
    QLineEdit* commandField() const { return m_command; }
    QLineEdit* objectField() const { return m_object; }
    QTreeWidget* restrictionList() const { return m_restrictions; }
    QTreeWidget* actionList() const { return m_actions; }
    // --- choose a restriction from the list (job A2): every row of "What the avatar may do" has an
    // Allowed / Blocked choice (column 1). Blocked makes the line (LabRestrictionChoice::blockLine,
    // with the value field for a distance or an open channel) and runs it from the Object field's
    // object, as the Command field does; Allowed lifts what that object holds. The box under the list
    // shows the line, says it in plain words and has "Use in script". False when nothing was run.
    bool chooseAction(const QString& actionId, bool blocked);
    // the choice in a row (nullptr for an unknown action)
    QComboBox* actionChoice(const QString& actionId) const;
    // the value field's text is used again: the old limit is lifted and the new line runs
    bool applyChoiceValue();
    // puts the line shown into the script of the Live page with llOwnerSay (one undo step) and shows
    // the Live page; false, with the reason under the button, when there is no line or no script
    bool useChoiceInScript();
    QFrame* choiceBox() const { return m_choiceBox; }
    QLabel* choiceTitle() const { return m_choiceTitle; }
    QLabel* choiceLine() const { return m_choiceLine; }
    QLabel* choiceExplain() const { return m_choiceExplain; }
    QLabel* choiceValueLabel() const { return m_choiceValueLabel; }
    QLineEdit* choiceValue() const { return m_choiceValue; }
    QPushButton* choiceApply() const { return m_choiceApply; }
    QPushButton* useInScriptButton() const { return m_useInScript; }
    QLabel* useInScriptReason() const { return m_useInScriptReason; }
    QLabel* choiceResult() const { return m_choiceResult; }
    QPlainTextEdit* log() const { return m_log; }
    QPushButton* runButton() const { return m_run; }
    QPushButton* resetButton() const { return m_reset; }
    QLabel* emptyHint() const { return m_empty; }
    QTabWidget* tabs() const { return m_tabs; }
    QLineEdit* liveFolderField() const { return m_liveFolder; }
    QPushButton* watchButton() const { return m_watch; }
    QPushButton* unwatchButton() const { return m_unwatch; }
    QLabel* liveStatus() const { return m_liveStatus; }
    // the status line's text without its bold word's markup ("OK: ...", "Error: ...")
    QString liveStatusText() const { return m_liveStatusPlain; }
    // the status line's box (border and fill in the tone's colour) and icon
    ScriptLabFrame* liveStatusFrame() const { return m_liveStatusFrame; }
    QLabel* liveStatusIcon() const { return m_liveStatusIcon; }
    // 0 idle, 1 OK, 2 error
    int liveStatusTone() const { return m_liveTone; }
    QPushButton* runAgainButton() const { return m_runAgain; }
    QLabel* tryLabel() const { return m_tryLabel; }
    QList<QPushButton*> tryButtons() const { return m_tryButtons; }
    // the Script part: its title is "Script" or "Script: <file>"
    ScriptLabSection* liveScriptName() const { return m_scriptSection; }
    QPushButton* modeRlvButton() const { return m_modeRlv; }
    QPushButton* modeRlvaButton() const { return m_modeRlva; }
    QLabel* modeLabel() const { return m_modeLabel; }
    // the text of the "N held" chip
    QLabel* heldChip() const { return m_heldChip; }
    ScriptLabFrame* heldChipFrame() const { return m_heldChipFrame; }
    // the empty state of the Live page and its parts
    ScriptLabFrame* liveEmpty() const { return m_liveEmpty; }
    QLabel* liveEmptyHeading() const { return m_liveEmptyHeading; }
    QLabel* liveEmptyText() const { return m_liveEmptyText; }
    QPushButton* exampleButton() const { return m_example; }
    QTreeWidget* liveRestrictionList() const { return m_liveRestrictions; }
    QLabel* liveRestrictionsEmpty() const { return m_liveRestrictionsEmpty; }
    // the folding parts, by key: "tree", "guide", "script", "problems", "log", "restrictionsNow", "objects", "input",
    // "inventory" (Live scripts) and "restrictions", "actions", "replies" (Type commands)
    ScriptLabSection* section(const QString& key) const;
    QList<ScriptLabSection*> sections() const { return m_sections; }
    QPushButton* openEditorButton() const { return m_openEditor; }
    // the script's text box (an editor since lane 1 job F1) and the whole editor around it
    LabCodeEdit* scriptView() const { return m_scriptView; }
    LabEditor* editor() const { return m_editor; }
    QListWidget* problemList() const { return m_problems; }
    QPlainTextEdit* runLog() const { return m_runLog; }
    QComboBox* inputObject() const { return m_inputObject; }
    QPushButton* touchButton() const { return m_touch; }
    QSpinBox* chatChannel() const { return m_chatChannel; }
    QLineEdit* chatName() const { return m_chatName; }
    QLineEdit* chatText() const { return m_chatText; }
    QPushButton* sayButton() const { return m_say; }
    QLabel* inputHint() const { return m_inputHint; }
    QLabel* inputResult() const { return m_inputResult; }
    // the newest dialog, inside the Objects part: "Dialog from <object>" (or "No dialog yet. ...")
    // and the muted line "\"<message>\" on channel <n>"
    QLabel* inputDialog() const { return m_inputDialog; }
    QLabel* dialogMessage() const { return m_inputDialogMessage; }
    QList<QPushButton*> dialogButtons() const;
    QPlainTextEdit* inputLog() const { return m_inputLog; }
    // --- the simulated inventory (slice 3): the items of the object chosen under Send input (the first
    // one at start), scripts first, as "Inventory of <object>" with "N items"; a muted line while
    // there is no world, and the caption that it is simulated
    QTreeWidget* inventoryList() const { return m_inventory; }
    QLabel* inventoryEmpty() const { return m_inventoryEmpty; }
    QLabel* inventoryNote() const { return m_inventoryNote; }
    // the Type column's words for an INVENTORY_* number ("Script", "Notecard, 4 lines")
    static QString inventoryTypeText(int type, int lines);
    // the "N items" count
    static QString itemCount(int items);

  signals:
    // propTints() changed (lane 1 job E)
    void propTintsChanged();

  public slots:
    // reads the state again (connected to RlvState::changed)
    void refresh();
    // the Viewer rules switch follows the state's mode (connected to RlvState::modeChanged)
    void refreshMode();
    // reads the watch and its newest result again (connected to LabWatcher)
    void refreshLive();
    // shows the effects preview's lines under the box (connected to EffectsPreview::changed)
    void refreshEffects();
    // reads the world again: which buttons work and why not, the objects, the newest dialog
    // (connected to lsl::Session::changed)
    void refreshInput();
    // shows the camera override's lines under its box (connected to CameraOverride::changed)
    void refreshCamera();
    // shows the environment override's lines under its box (connected to EnvironmentOverride::changed)
    void refreshEnvironment();
    // shows the view modes override's lines under its box (connected to ViewModesOverride::changed)
    void refreshViewModes();
    // reads the chosen object's inventory again (from refreshInput and when the object is changed)
    void refreshInventory();

    // The three view overrides (camera limits, environment, view modes) change the 3D view
    // only while this panel is visible; hidden, they are all off, and shown again they come
    // back as the boxes were ticked. A minimised window does not count as hidden.
    bool viewEffectsActive() const { return !m_hiddenByUser; }

    // A click on a line of the event log (job R3): an error line ("at line 4, column 9") or an
    // RLV line ("rlv_command: @detach=x") shows its code line in the script. False when the log
    // line points at no line of the script.
    bool showLogLine(int logLine);

  protected:
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

  private:
    void runCommandField();
    // gives the three overrides their on/off: ticked and the panel not hidden
    void applyViewEffects();
    void appendLog(const QString& line);
    void applyStyle();
    void buildLivePage();
    void chooseFolder();
    bool watchWith(const QString& folder, const QJsonObject& options);
    void openInEditor();
    void showProblem(int index);
    void showScriptPlace(int line, int column);   // line and column from 1
    void setLiveStatus(const QString& text, int tone);   // tone: 0 muted, 1 success, 2 danger
    void styleLiveStatus();
    void buildInputSection(QWidget* page, QVBoxLayout* layout);
    bool sendInput(QJsonObject request, const QString& what);
    QString inputBlocker() const;      // why nothing can be sent now; empty when it can
    void setInputResult(const QString& text, int tone);
    ScriptLabSection* addSection(const QString& key, const QString& title, bool expanded, QWidget* page, QVBoxLayout* layout);
    QTreeWidget* makeRestrictionList(QWidget* parent);
    void refreshCounts();
    void updateWorldParts();
    QJsonArray lintWarnings(const QString& source);

    QPointer<RlvState> m_state;
    bool m_hintsOn = true;
    QWidget* m_explainFrame = nullptr;
    QLabel* m_explainIcon = nullptr;
    QLabel* m_explain = nullptr;
    QLabel* m_tryLabel = nullptr;
    QList<QPushButton*> m_tryButtons;
    QPushButton* m_runAgain = nullptr;
    ScriptLabFrame* m_liveStatusFrame = nullptr;
    QLabel* m_liveStatusIcon = nullptr;
    QString m_liveStatusPlain;
    QLabel* m_inputDialogMessage = nullptr;
    QLineEdit* m_object = nullptr;
    QLineEdit* m_command = nullptr;
    QPushButton* m_run = nullptr;
    QPushButton* m_reset = nullptr;
    QTreeWidget* m_restrictions = nullptr;
    QLabel* m_empty = nullptr;
    QTreeWidget* m_actions = nullptr;
    // job A2: the choice box under the list
    QFrame* m_choiceBox = nullptr;
    QLabel* m_choiceTitle = nullptr;
    QLabel* m_choiceLine = nullptr;
    QLabel* m_choiceExplain = nullptr;
    QWidget* m_choiceValueRow = nullptr;
    QLabel* m_choiceValueLabel = nullptr;
    QLineEdit* m_choiceValue = nullptr;
    QPushButton* m_choiceApply = nullptr;
    QPushButton* m_useInScript = nullptr;
    QLabel* m_useInScriptReason = nullptr;
    QLabel* m_choiceResult = nullptr;
    QString m_choiceAction;            // the row the box speaks of
    QString m_choiceRlvLine;           // the line the box shows ("" when none)
    void buildChoiceBox(QWidget* parent, QVBoxLayout* layout);
    void addActionChoices();
    void showChoice(const QString& actionId, bool blocked, const QString& line, const QStringList& words, const QString& result, int tone);
    void setChoiceResult(const QString& text, int tone);
    void refreshUseInScript();
    QPlainTextEdit* m_log = nullptr;
    QMetaObject::Connection m_changedConnection;
    QMetaObject::Connection m_modeConnection;
    QLabel* m_modeLabel = nullptr;
    QPushButton* m_modeRlv = nullptr;
    QPushButton* m_modeRlva = nullptr;
    ScriptLabFrame* m_heldChipFrame = nullptr;
    QLabel* m_heldIcon = nullptr;
    QLabel* m_heldChip = nullptr;
    QList<ScriptLabSection*> m_sections;
    ScriptLabSection* m_restrictionsSection = nullptr;
    ScriptLabSection* m_actionsSection = nullptr;
    ScriptLabSection* m_repliesSection = nullptr;
    ScriptLabSection* m_scriptSection = nullptr;
    ScriptLabSection* m_problemsSection = nullptr;
    ScriptLabSection* m_logSection = nullptr;
    ScriptLabSection* m_liveRestrictionsSection = nullptr;
    ScriptLabSection* m_objectsSection = nullptr;
    ScriptLabSection* m_inputSection = nullptr;
    ScriptLabSection* m_inventorySection = nullptr;
    QTreeWidget* m_inventory = nullptr;
    QLabel* m_inventoryEmpty = nullptr;
    QLabel* m_inventoryNote = nullptr;
    ScriptLabFrame* m_liveEmpty = nullptr;
    QLabel* m_liveEmptyIcon = nullptr;
    QLabel* m_liveEmptyHeading = nullptr;
    QLabel* m_liveEmptyText = nullptr;
    QPushButton* m_example = nullptr;
    QLabel* m_liveFolderLabel = nullptr;
    QTreeWidget* m_liveRestrictions = nullptr;
    QLabel* m_liveRestrictionsEmpty = nullptr;
    QString m_examplesFolder;
    QString m_lintSource;
    QJsonArray m_lintWarnings;
    ScriptLabObjectsView* m_objectsView = nullptr;

    QPointer<LabWatcher> m_watcher;
    // job A4
    void refreshChannelMap(const QJsonObject& folder);
    ScriptLabSection* m_mapSection = nullptr;
    ChannelMapView* m_mapView = nullptr;
    QLabel* m_mapLine = nullptr;
    QLabel* m_mapProblems = nullptr;
    QString m_mapKey;
    QTabWidget* m_tabs = nullptr;
    QLineEdit* m_liveFolder = nullptr;
    QPushButton* m_watch = nullptr;
    QPushButton* m_unwatch = nullptr;
    QLabel* m_liveStatus = nullptr;
    int m_liveTone = 0;
    bool m_wasWatching = false;
    QString m_refusal;                 // why the last Watch folder was refused, until the next change
    QPushButton* m_openEditor = nullptr;
    LabCodeEdit* m_scriptView = nullptr;
    LabEditor* m_editor = nullptr;
    QWidget* m_sizeGrip = nullptr;
    void placeSizeGrip();
    int m_runAtSave = 0;               // the watch's run count when the editor saved
    void runSavedScript(const QString& path);
    QListWidget* m_problems = nullptr;
    QPlainTextEdit* m_runLog = nullptr;
    QStringList m_logFiles;            // folder mode: the script file of each event log line ("" for none)
    ScriptLabSection* m_treeSection = nullptr;
    LabFolderTree* m_folderTree = nullptr;
    ScriptLabSection* m_guideSection = nullptr;
    QLabel* m_guideText = nullptr;
    QLabel* m_guideExample = nullptr;
    QString m_pickedFile;              // the script picked in the tree, for the run m_pickedRun
    int m_pickedRun = -1;
    QList<LabTint::Tint> m_propTints;
    void updatePropTints(bool force = false);
    // job A1: makes the missing props, places the moved ones, then updates the tints
    void updatePrimProps();
    // shows or hides the menu box for the newest menu of the watched object (job A2)
    void refreshMenuBox();
    QList<LabPrimProps::Move> m_primMoves;
    QMap<QString, LabExample::Prop> m_primBase;   // where each moved prop stood before a script moved it
    QString m_primExampleFolder;                  // the folder m_primExample was read from
    QList<LabExample::Prop> m_primExample;        // the props of that folder's example.json, if any
    QString m_shownFile;               // the script shown in the view (relative to the folder)
    QList<QJsonObject> m_shownProblems;
    QList<QMetaObject::Connection> m_watcherConnections;

    void effectsToggled(bool on);
    void connectEffectsToWatcher();
    QPointer<EffectsPreview> m_effects;
    QPushButton* m_effectsBox = nullptr;
    void styleEffectsButton();
    // job A3: record and replay
    void refreshRecipe();
    void styleRecordButton();
    QWidget* m_recipeRow = nullptr;
    QPushButton* m_recordButton = nullptr;
    QLabel* m_recipeStatus = nullptr;
    ScriptLabSection* m_recipeSection = nullptr;
    QLabel* m_recipeFileLine = nullptr;
    QPlainTextEdit* m_recipeText = nullptr;
    QPushButton* m_replayButton = nullptr;
    LabPlayer* m_player = nullptr;   // job B3
    // the folder run the props show: the player's moment while it shows one, else the newest run
    QJsonObject shownFolder() const;
    QString m_recipeNote;    // a one-off line (recording stopped without steps, a refused replay)
    bool m_recipeNoteIsError = false;
    bool m_expert = false;
    QLabel* m_effectsStatus = nullptr;
    QList<QMetaObject::Connection> m_effectsConnections;
    QList<QMetaObject::Connection> m_effectsWatcherConnections;
    QPointer<lsl::Session> m_session;
    QMetaObject::Connection m_inputConnection;
    QComboBox* m_inputObject = nullptr;
    QPushButton* m_touch = nullptr;
    QSpinBox* m_chatChannel = nullptr;
    QLineEdit* m_chatName = nullptr;
    QLineEdit* m_chatText = nullptr;
    QPushButton* m_say = nullptr;
    QLabel* m_inputHint = nullptr;
    QLabel* m_inputResult = nullptr;
    QLabel* m_inputDialog = nullptr;
    QWidget* m_dialogButtonBox = nullptr;
    QGridLayout* m_dialogGrid = nullptr;
    QPlainTextEdit* m_inputLog = nullptr;
    QString m_shownDialogKey;          // the dialog the buttons were made for
    QPointer<LabDialogBox> m_menuBox;  // job A2: the menu over the 3D view (owned by its host)
    QPoint m_menuAnchor{-1, -1};       // the last click on a prop, in the host's coordinates
    QString m_menuKey;                 // the menu the box was made for
    QString m_menuDone;                // the menu answered or ignored last: not shown again
    int m_inputTone = 0;
    bool m_effectsFromSession = false;   // the newest run came from lsl_run or lsl_event, not Live
    QPointer<CameraOverride> m_camera;
    QMetaObject::Connection m_cameraConnection;
    QCheckBox* m_cameraBox = nullptr;
    QLabel* m_cameraStatus = nullptr;
    QPointer<EnvironmentOverride> m_environment;
    QMetaObject::Connection m_environmentConnection;
    QCheckBox* m_environmentBox = nullptr;
    QLabel* m_environmentStatus = nullptr;
    ScriptLabSection* m_sphereSection = nullptr;
    SphereControls* m_sphereControls = nullptr;
    QPointer<ViewModesOverride> m_viewModes;
    QMetaObject::Connection m_viewModesConnection;
    bool m_hiddenByUser = false;   // hidden for real (not just minimised)
    QCheckBox* m_viewModesBox = nullptr;
    QLabel* m_viewModesStatus = nullptr;

    // --- job U3
    void buildTips();
    void refreshTips();
    void refreshSteps();
    void updateWorkLayout();
    void chooseNewFolder();
    SceneHooks m_hooks;
    QString m_objectsFolder;
    QString m_scriptsRoot;
    QString m_projectFolder;
    LabExample m_stepExample;
    QSet<int> m_doneSteps;
    bool m_stepsShown = false;
    LabStepCard* m_stepCard = nullptr;
    QPushButton* m_makeExample = nullptr;
    QPushButton* m_newFolder = nullptr;
    QLabel* m_treeLine = nullptr;
    QCheckBox* m_followBox = nullptr;
    bool m_sideBySide = false;
    QWidget* m_workArea = nullptr;
    QBoxLayout* m_workLayout = nullptr;
    QWidget* m_treeColumn = nullptr;
    QWidget* m_editorColumn = nullptr;
    QList<LabTipCard*> m_tipCards;
    QLabel* m_guideRulesHeading = nullptr;
    QLabel* m_guideWarningsHeading = nullptr;
    QLabel* m_guideWarnings = nullptr;
    QLabel* m_scriptButtonsReason = nullptr;
    bool watchingProject() const;
};

#endif
