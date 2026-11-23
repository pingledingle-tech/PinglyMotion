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
#include <QPointer>
#include <QWidget>

#include "cameraoverride.h"
#include "environmentoverride.h"
#include "viewmodesoverride.h"
#include "effectspreview.h"
#include "labeditor.h"
#include "labwatcher.h"
#include "rlvstate.h"


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
    // chosen; false when refused (the status line says why)
    bool watchFolder(const QString& folder);
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

    // --- Viewer rules (the panel in the new design): plain RLV or RLVa, as the RLV state answers.
    // Switching removes every restriction, as after a relog (RlvState::setMode).
    void setMode(Rlv::Mode mode);
    // --- Effects in the 3D view (issue 293): the preview the "Show effects in 3D view" box turns
    // on and off (off at start). Not owned (the window owns it and gives it to the 3D view).
    void setEffects(EffectsPreview* preview);
    EffectsPreview* effects() const { return m_effects; }
    QCheckBox* effectsBox() const { return m_effectsBox; }
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
    // the folding parts, by key: "script", "problems", "log", "restrictionsNow", "objects", "input",
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
    int m_runAtSave = 0;               // the watch's run count when the editor saved
    void runSavedScript(const QString& path);
    QListWidget* m_problems = nullptr;
    QPlainTextEdit* m_runLog = nullptr;
    QString m_shownFile;               // the script shown in the view (relative to the folder)
    QList<QJsonObject> m_shownProblems;
    QList<QMetaObject::Connection> m_watcherConnections;

    void effectsToggled(bool on);
    void connectEffectsToWatcher();
    QPointer<EffectsPreview> m_effects;
    QCheckBox* m_effectsBox = nullptr;
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
};

#endif
