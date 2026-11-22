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
 * Colours are the success and danger tokens of docs/design_tokens.json; the
 * state is always written as a word as well (design charter P10).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SCRIPTLABPANEL_H
#define SCRIPTLABPANEL_H

#include <QColor>
#include <QJsonObject>
#include <QList>
#include <QPointer>
#include <QWidget>

#include "cameraoverride.h"
#include "environmentoverride.h"
#include "effectspreview.h"
#include "labwatcher.h"
#include "rlvstate.h"


class QCheckBox;
class QComboBox;
class QGridLayout;
class QSpinBox;
class QDockWidget;
class ScriptLabObjectsView;
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
    QLabel* liveScriptName() const { return m_liveScript; }
    QPushButton* openEditorButton() const { return m_openEditor; }
    QPlainTextEdit* scriptView() const { return m_scriptView; }
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
    QLabel* inputDialog() const { return m_inputDialog; }
    QList<QPushButton*> dialogButtons() const;
    QPlainTextEdit* inputLog() const { return m_inputLog; }

  public slots:
    // reads the state again (connected to RlvState::changed)
    void refresh();
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

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void runCommandField();
    void appendLog(const QString& line);
    void applyStyle();
    void buildLivePage();
    void chooseFolder();
    void openInEditor();
    void showProblem(int index);
    void setLiveStatus(const QString& text, int tone);   // tone: 0 muted, 1 success, 2 danger
    void buildInputSection(QWidget* page, QVBoxLayout* layout);
    bool sendInput(QJsonObject request, const QString& what);
    QString inputBlocker() const;      // why nothing can be sent now; empty when it can
    void setInputResult(const QString& text, int tone);

    QPointer<RlvState> m_state;
    QLineEdit* m_object;
    QLineEdit* m_command;
    QPushButton* m_run;
    QPushButton* m_reset;
    QTreeWidget* m_restrictions;
    QLabel* m_empty;
    QTreeWidget* m_actions;
    QPlainTextEdit* m_log;
    QMetaObject::Connection m_changedConnection;
    ScriptLabObjectsView* m_objectsView;

    QPointer<LabWatcher> m_watcher;
    QTabWidget* m_tabs = nullptr;
    QLineEdit* m_liveFolder = nullptr;
    QPushButton* m_watch = nullptr;
    QPushButton* m_unwatch = nullptr;
    QLabel* m_liveStatus = nullptr;
    int m_liveTone = 0;
    bool m_wasWatching = false;
    QString m_refusal;                 // why the last Watch folder was refused, until the next change
    QLabel* m_liveScript = nullptr;
    QPushButton* m_openEditor = nullptr;
    QPlainTextEdit* m_scriptView = nullptr;
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
};

#endif
