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
#include <QPointer>
#include <QWidget>

#include "rlvstate.h"

class QDockWidget;
class ScriptLabObjectsView;
namespace lsl { class Session; }
class QLabel;
class QLineEdit;
class QMainWindow;
class QPlainTextEdit;
class QPushButton;
class QTreeWidget;

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

  public slots:
    // reads the state again (connected to RlvState::changed)
    void refresh();

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void runCommandField();
    void appendLog(const QString& line);
    void applyStyle();

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
};

#endif
