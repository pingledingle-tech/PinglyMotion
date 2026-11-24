/***************************************************************************
 *   Copyright (C) 2006 by Zi Ree   *
 *   Zi Ree @ SecondLife   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * features added by Darkside Eldrich
 */

#ifndef PINGLYMOTIONWINDOW_H
#define PINGLYMOTIONWINDOW_H

#include <QMainWindow>
#include <QHash>
#include <QMap>
#include <QScopedPointer>
#include <QTimer>

#include "easing.h"
#include "animfile.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "playstate.h"
#include "prop.h"
#include "propedit.h"
#include "timelineclip.h"
#include "updatechecker.h"
#include "workmode.h"
#include "viewtoolspec.h"

class QCloseEvent;
class QMenu;
class QResizeEvent;
class MainPanels;
class SelectedJointPanel;
class SelectedPropPanel;
class QFileInfo;
class QButtonGroup;
class QLineEdit;
class QPushButton;
class QSlider;
class QTabBar;
class QTableWidget;

class Animation;
class BVHNode;
class JointInfo;
class HandsPanel;
namespace HandPose { struct Write; }
class PinglyMotion;
struct RotationLimits;
class ControlApi;
class ControlServer;
class ControlIndicator;
class AuroraIndicator;
class TipsSwitch;
class TransportBar;
class TopBar;
class ReasonLine;
class EmptyState;
class HintCards;
class HelperMode;
class ViewToolbar;
class ScriptLabPanel;
class LibraryPanel;
class FiguresBox;
class UndoController;
class FigureTag;
class AuroraLink;
class WindowControlHost;
class Scene;
class Timeline;

class QScrollArea;

namespace Ui {
  class QDockWidget;

class PinglyMotionWindow;
}

class PinglyMotionWindow : public QMainWindow
{
  Q_OBJECT

  public:
    PinglyMotionWindow();
    ~PinglyMotionWindow();

    void queueAfterShow();
    // Help > Show the welcome card again (UX phase 6a)
    void showWelcomeCard();

  signals:
    void resetCamera();
    void protectFrame(bool state);
    void newProp(Prop::PropType, QSharedPointer<Mesh> mesh);
    void selectProp(int index);
    void attachProp(int attachmentPoint);
    void propPositionChanged(double xPosition, double yPosition, double zPosition);
    void propScaleChanged(double xScale, double yScale, double zScale);
    void propRotationChanged(double xRotation, double yRotation, double zRotation);
    void deleteProp();
    void clearProps();

  protected slots:
    void afterShow();

    void configChanged();

    void partClicked(BVHNode* node, QVector3D rotation, RotationLimits rotLimits, QVector3D position);
    void partDragged(BVHNode* node,double changeX,double changeY,double changeZ);
    void propClicked(Prop* prop);
    void backgroundClicked();

    void frameTimeout();

    void setCurrentFrame(int frame);

    void selectAnimation(Animation* animation);

    // autoconnection from designer UI

    // ------- Menu Action Slots --------
    void on_fileNewAction_triggered();
    void on_fileCloseAction_triggered();
    void on_fileOpenAction_triggered();
    void on_fileAddAction_triggered();
    void on_fileSaveAction_triggered();
    void on_fileSaveAsAction_triggered();
    void on_fileExportForSecondLifeAction_triggered();
    void on_fileExportAnimAction_triggered();
    void on_fileImportAnimAction_triggered();
    void on_fileImportBvhAction_triggered();
    void on_fileImportGltfAction_triggered();
    void on_fileLoadPropsAction_triggered();
    void on_fileSavePropsAction_triggered();
    void on_fileExitAction_triggered();

    void on_editCutAction_triggered();
    void on_editCopyAction_triggered();
    void on_editPasteAction_triggered();

    void on_toolsOptimizeBVHAction_triggered();
    void on_toolsMirrorAction_triggered();

    void on_optionsJointLimitsAction_toggled(bool on);
    void on_optionsLoopAction_toggled(bool on);
    void on_optionsProtectFirstFrameAction_toggled(bool on);
    void on_optionsShowTimelineAction_toggled(bool on);
    void on_optionsConfigurePinglyMotionAction_triggered();

    void on_helpAboutAction_triggered();
    void on_helpWhatsNewAction_triggered();
    void on_helpOpenLogAction_triggered();
    void on_helpClearLogAction_triggered();

    // ------- Additional Toolbar Element Slots -------

    void on_resetCameraAction_triggered();

    // ------- UI Element Slots --------
    void on_selectAnimationCombo_activated(int);
    void on_figureCombo_activated(int);
    void on_scaleSpin_valueChanged(int newValue);
    void on_xRotationEdit_editingFinished();
    void on_xRotationSlider_valueChanged(int);
    void on_yRotationEdit_editingFinished();
    void on_yRotationSlider_valueChanged(int);
    void on_zRotationEdit_editingFinished();
    void on_zRotationSlider_valueChanged(int);
    void on_xPositionEdit_editingFinished();
    void on_xPositionSlider_valueChanged(int);
    void on_yPositionEdit_editingFinished();
    void on_yPositionSlider_valueChanged(int);
    void on_zPositionEdit_editingFinished();
    void on_zPositionSlider_valueChanged(int);

    void on_newBoxPropButton_clicked();
    void on_newSpherePropButton_clicked();
    void on_newConePropButton_clicked();
    void on_newTorusPropButton_clicked();
    void on_propNameCombo_activated(int);
    void on_deletePropButton_clicked();
    void on_attachToComboBox_activated(int attachmentPoint);
    void on_propXPosSpin_valueChanged(int);
    void on_propYPosSpin_valueChanged(int);
    void on_propZPosSpin_valueChanged(int);
    void on_propXScaleSpin_valueChanged(int);
    void on_propYScaleSpin_valueChanged(int);
    void on_propZScaleSpin_valueChanged(int);
    void on_propXRotSpin_valueChanged(int);
    void on_propYRotSpin_valueChanged(int);
    void on_propZRotSpin_valueChanged(int);

    void on_currentFrameSlider_valueChanged(int newValue);
    void on_keyframeButton_clicked();
    void on_loopInSpinBox_editingFinished();
    void on_loopOutSpinBox_editingFinished();
    void on_framesSpin_valueChanged(int num);
    void on_fpsSpin_valueChanged(int num);
    // end autoconnection from designer UI

  public:
    // the text size, 100 to 200 percent (src/textsize.h): saved, applied to the
    // whole window at once, and the panels rearranged if they no longer fit.
    // View > Text size calls this; the Setup dialog will too.
    void setTextSizePercent(int percent);

  protected:
    // prevent closing of main window if there are unsaved changes
    virtual void closeEvent(QCloseEvent* event);
    // left / right arrow keys: one frame back / forward, when the key is not
    // used by a field, list or slider that has the keyboard focus
    void keyPressEvent(QKeyEvent* event) override;
    // the window's width changed: the panels may need to rearrange (docs/UX_DECISIONS.md, 6c)
    void resizeEvent(QResizeEvent* event) override;
    // the text size changed: panel widths and the rearranging follow it
    void changeEvent(QEvent* event) override;

    // true when the animation was made (false: the user cancelled the question about unsaved changes)
    bool fileNew();
    // File > Close animation: closes everything and shows the empty state (UX phase 1b)
    void fileClose();
    void fileOpen();
    void fileOpen(const QString& fileName);
    // opens fileName from these bytes instead of reading the file (load_reference
    // checks the bytes against the library index first)
    void fileOpenData(const QString& fileName,const QByteArray& data);
    // "add" a new file without clearing the old one(s)
    void fileAdd();
    void fileAdd(const QString& fileName);
    bool checkAnimationFile(const QString& fileName,const QByteArray* data=nullptr);
    void addAnimationFile(const QString& fileName,const QByteArray* data=nullptr);

    // Two figures in one scene, Expert mode only (docs/UX_DECISIONS.md 6m)
    // File > Add a second figure...: asks From the library or From a file...
    void addSecondFigure();
    // adds a checked file as the next figure at its own place and makes it active; false when
    // the scene is full or the file cannot be read
    bool addFigureFile(const QString& fileName,const QByteArray* data=nullptr);
    // the Library panel's Add as a second figure
    void addLibraryClipAsFigure(const QString& clipId);
    // the figure the panels work on (0-based, figure order), -1 with none
    int activeFigure() const;
    // makes a figure active: the Joints list, Selected joint and the timeline follow it
    void makeFigureActive(int index);
    // takes a figure out of the scene (asks first when it has unsaved changes); its file is not
    // changed and the other figure stays where it is; false when nothing was removed
    bool removeFigure(int index);
    // the Figures box, the name tags, the timeline's header rows, the chip, the title and the
    // reasons of the two add commands follow the figures and the mode
    void refreshFigures();
    // the name tags go over the heads as the 3D view drew them
    void placeFigureTags();
    bool fileSave();
    bool fileSaveAs();
    // after a save to newFile: when the animation came from an old .avm file with the same name,
    // checks the new file and moves the old one to the recycle bin
    void replaceOldAvm(const QString& avmFile,const QString& newFile);
    void fileExportForSecondLife();
    void fileExportAnim();
    void fileImportAnim();
    void fileImportBvh();
    void fileImportGltf();
    // the Second Life skeleton of the program (frame 0 is the rest pose), for imports
    bool loadSkeleton(AnimFile::Animation& skeleton,const QString& title);
    // opens an animation that was made from another file (an import) as a new, unsaved animation.
    // loopIn and loopOut are in the window's frame numbers; -1 keeps the defaults.
    bool openConvertedAnimation(const AnimFile::Animation& anim,const QString& name,int loopIn,int loopOut,const QString& title);
    void fileLoadProps();
    void fileSaveProps();
    void fileExit();

    void editCut();
    void editCopy();
    void editPaste();
    // UX phase 3b: the copied stretch, mirrored (Edit > Paste mirrored, Ctrl+Shift+V)
    void pasteMirroredFromMenu();
    void pasteClip(bool mirrored);

    void toolsOptimizeBVH();
    void toolsMirror();

    void setJointLimits(bool on);
    void setLoop(bool on);
    void setProtectFirstFrame(bool on);
    void showTimeline(bool state);
    void configure();

    void helpAbout();
    void helpWhatsNew();

    void animationChanged(int which);
    void setAvatarShape(int shape);
    void setAvatarScale(int percent);
    void partChoice();
    void rotationValue();
    void rotationSlider(const QObject* slider);
    void positionValue();
    void positionSlider(const QObject* slider);

    // a tile of the easing row was clicked (issue #53 step 5): store the choice
    // on the pose change that ends at the selected key frame
    void easingChoiceClicked(Easing::Preset preset);

    void frameSlider(int position);
    // the transport buttons under the timeline (docs/UX_DECISIONS.md section 1);
    // remote control play and stop do the same
    void startPlaying();
    void stopPlaying();
    void replayFromStart();
    // Frame back (-1) / Frame forward (+1): stops playing and goes one frame,
    // never past the first or the last frame (Playback::stepFrame)
    void stepFrame(int delta);
    void beginPlaying(Animation* anim);
    void setDefaultLoopIn(Animation* anim);
    void connectAnimation(Animation* anim);
    // the window shows what the model found wrong with the program's own data files
    void showLoadProblems(Animation* anim);
    void setLoopInPoint(int inFrame);
    void setLoopOutPoint(int outFrame);
    void numFramesChanged(int num);
    void setFPS(int fps);

    void setSliderValue(QSlider* slider,QLineEdit* edit,float value);

    QString selectFileToOpen(const QString& caption);
    void addToOpenFiles(const QString& fileName);
    bool clearOpenFiles();
    bool askToSaveChanges();

    void setPlaystate(PlayState state);

    bool checkFileOverwrite(const QFileInfo& fileInfo);
    void setCurrentFile(const QString& fileName);
    void enableInputs(bool state);

    void updateFps();
    void updateKeyBtn();
    // a joint's name that tells left and right apart, or its file name
    QString jointDisplayName(const QString& fileName) const;
    // Edit > Set key: enabled, tip and reason follow the state of the window
    void updateEditActions();
    void updateInputs();

    // calculates the longest running time of all loaded animations, returns it
    // and stores it in longestRunningTime member variable
    double calculateLongestRunningTime();

    void setX(float x);
    void setY(float y);
    void setZ(float z);

    float getX();
    float getY();
    float getZ();

    void setXPos(float x);
    void setYPos(float y);
    void setZPos(float z);

    float getXPos();
    float getYPos();
    float getZPos();

    QString currentFile;
    QStringList openFiles;
    QTimer timer;
    // list of animation ids mapped to combo box indexes
    QList<Animation*> animationIds;

    // joint picker: tabs Body / Face / Hands / Other and a search box above a
    // table of joints with friendly names under section headings (#14, step 2)
    QTabBar* m_jointTabs=nullptr;
    QLineEdit* m_jointSearch=nullptr;
    QTableWidget* m_jointTable=nullptr;
    // the Hands tab (UX phase 2c, src/handstab.h): shown instead of the joint
    // table while the Hands tab is chosen and the search box is empty
    HandsPanel* m_handsPanel=nullptr;
    void refreshHandsPanel();
    void handsChangeRequested(const QVector<HandPose::Write>& writes,const QString& message);
    void handsJointPicked(const QString& name);
    // friendly names and sections from data/firestorm (empty when not found)
    QSharedPointer<JointInfo> m_jointInfo;
    // track number of each row in the joint table, -1 for heading rows
    QList<int> nodeMapping;
    // track selected in the joint table (0 = position, 1 = hip)
    int m_selectedTrack=1;
    void buildJointPicker();
    void fillJointTable();
    void showSelectedTrack();
    void selectTrackInTable(int track);

    // the Joints panel's Left / Right / Mirror control and the Whole pose
    // actions Flip, T-pose and Reset all (posing UX phase 2a, docs/mockup).
    // m_jointSide is a null QChar for Mirror (every joint listed; clicking
    // Mirror sets Animation::setMirrored(true) on the animation open at that
    // moment, so a Selected-joint edit also turns the other side); 'L' or 'R'
    // for a single side (setMirrored(false), only that side and the centre
    // line listed, JointList::sideOf()). The side control only changes the
    // currently open animation when clicked: it is not forced onto every
    // file as it is opened, so control-API and scripted edits are never
    // silently mirrored behind the user's back.
    QButtonGroup* m_sideGroup=nullptr;
    QPushButton* m_leftSideButton=nullptr;
    QPushButton* m_rightSideButton=nullptr;
    QPushButton* m_mirrorSideButton=nullptr;
    QPushButton* m_flipPoseButton=nullptr;
    QPushButton* m_tPoseButton=nullptr;
    QPushButton* m_resetAllButton=nullptr;
    QChar m_jointSide=QChar();
    void setJointSide(QChar side);
    void flipPoseClicked();
    void tPoseClicked();
    void resetAllClicked();
    // the Selected joint panel (UX phase 2b, src/selectedjointpanel.h): its page
    // and joint follow the window (no animation, no joint picked, the joint);
    // Reset joint puts the picked joint back as it is in the first frame
    SelectedJointPanel* m_selectedJoint=nullptr;
    // two figures (6m): the box in the 3D view, the name tags, Alt+1 and Alt+2, and each
    // figure's place sideways (kept, so removing one figure never moves the other)
    FiguresBox* m_figuresBox=nullptr;
    QList<FigureTag*> m_figureTags;
    QList<QAction*> m_figureActions;
    QHash<const Animation*,int> m_figurePlace;
    int m_announcedFigure=-1;
    void setupFigures();
    void refreshJointPanel();
    // why the sliders cannot be used now ("Stop playing first."), or empty
    QString jointBlockedReason() const;
    void resetJointClicked();
    void pickDemoJoint();
    void buildPartLists();
    void jointGroupsChanged();
    void buildDocks();
    void buildMenus();
    void resetPanelLayout();
    void updateBeginnerRow();
    bool switchPanelLayoutForMode(bool expert);
    // Tab, or View > Hide panels: hides every open panel, or shows again the
    // ones Tab hid (docs/archive/PANELS_PLAN.md, job P4)
    void togglePanelsHidden();
    QList<QDockWidget*> panelDocks() const;
    void fitPanelWidths();
    // rearranging at large text or in a small window (docs/UX_DECISIONS.md, 6c):
    // the layout for the window's width now
    PanelWindow::Docks reflowDocks() const;
    void updateReflow();
    // updateReflow() once, after the events that are waiting (resizing sends many)
    void scheduleReflow();
    void showLightDialog();
    void saveUserSettings();
    // the three panels (docs/archive/PANELS_PLAN.md, job P2): Joints, Selected joint, Timeline
    QDockWidget* m_timelineDock=nullptr;
    QDockWidget* m_jointsDock=nullptr;
    QDockWidget* m_jointCardDock=nullptr;
    // Script Lab (issue 288): hidden until View > Script Lab
    QDockWidget* m_scriptLabDock=nullptr;
    ScriptLabPanel* m_scriptLabPanel=nullptr;
    // the Library panel (roadmap item 6, issue 19): hidden until View > Library
    QDockWidget* m_libraryDock=nullptr;
    LibraryPanel* m_libraryPanel=nullptr;
    QScrollArea* m_jointsScroll=nullptr;
    QScrollArea* m_jointCardScroll=nullptr;
    // Tab (job P4): which panels Tab hid, so the next Tab shows the same ones;
    // never saved. The flag keeps the panels' own toggle signals from changing
    // the state while Tab is applied.
    PanelLayout::TabState m_tabState;
    bool m_applyingTab=false;
    // the layout the panels stand in now, and the wide layout to go back to
    PanelWindow::Reflower m_reflower;
    // the Animate / Script switch at the top (UX-F, src/workmode.h)
    WorkMode::Switcher* m_workMode=nullptr;
    // the contents of the three panels and how wide they must be (src/panelcontent.h)
    QScopedPointer<MainPanels> m_panels;
    bool m_reflowPending=false;
    QAction* m_hidePanelsAction=nullptr;
    QAction* m_textSizeAction=nullptr;
    QAction* m_hintsAction=nullptr;

    BVHNode* currentPart;

    Timeline* timeline;
    // icons for play button
    // Start, Stop, Replay from start and Loop under the timeline
    TransportBar* m_transport=nullptr;

    // holds the current playing status
    PlayState playstate=PLAYSTATE_STOPPED;

    // the stretch of the timeline Copy or Cut took last (UX phase 3b)
    TimelineClip m_clip;
    // will be true if a frame is protected
    bool protect=false;

    // holds the longest running time of all currently opened animations
    double longestRunningTime;

  private slots:
    void enableProps(bool state);
    void enableAvatarRotation(bool state);
    void enableAvatarPosition(bool state);
    void updatePropSpins(const QVector3D& position, const QVector3D& rotation, const QVector3D& scale);
    void onNewProp(const QString& propName);
    void onSelectProp(bool isSelected, int attachmentPoint);
    void onDeleteProp(const QString propName);
    void onClearProps();

  private:
    // editing a picked prop (Mockup 2): the Selected prop page, the handles in the 3D view, Undo
    void setupPropEditing();
    // the Selected prop page shows the picked prop, or the empty card
    void showSelectedProp();
    // one Undo step that puts the prop with this id back to `before` (Redo: `after`)
    void recordPropStep(unsigned int propId, const PropEdit::State& before, const PropEdit::State& after);
    void applyPropStep(unsigned int propId, const PropEdit::State& state);
    SelectedPropPanel* m_selectedProp = nullptr;

  private slots:


  private:
    // UX phase 1b: the top bar and the empty state
    void buildEditMenu();
    void buildTopBar();
    void setEmptyMode(bool empty);
    // the start choice with this library clip id was clicked in the empty state
    void startFromLibrary(const QString& clipId);
    // Use this in the Library panel: asks about unsaved changes, then opens the clip
    void useLibraryClip(const QString& clipId);
    bool emptyMode() const { return m_empty; }
    void setKeyFromMenu();

    // the AI Assistant (remote control, issue 7): the button in the status bar, see controlapi.h
    friend class WindowControlHost;
    void setRemoteControl(bool on);
    // the button was clicked: the setup dialog the first time, its menu after that
    void aiAssistantClicked();
    void showAiConnectDialog();
    void updateWindowTitle();

    void selectProp();

    QScopedPointer<Ui::PinglyMotionWindow> ui;

    // TODO Temporary pointer while logic is moved around.
    //  Eventually, PinglyMotion should own PinglyMotionWindow
    QScopedPointer<PinglyMotion> m_pinglyMotion;

    Scene* scene;

    UpdateChecker updateChecker;

    QSharedPointer<Mesh> cubeMesh;
    QSharedPointer<Mesh> sphereMesh;
    QSharedPointer<Mesh> coneMesh;
    QSharedPointer<Mesh> torusMesh;

    const QString m_animFilter;
    const QString m_dingleFilter;
    // opened .avm files: the .dingle name the animation will be saved under -> the old .avm file
    QMap<QString,QString> m_avmOrigin;
    const QString m_propFilter;
    const QString m_untitledName;
    // why the last file could not be opened
    QString m_lastOpenError;
    // true while remote control opens a file: problems are reported back, not shown in a message box
    bool m_quietDialogs = false;
    const int m_precision;

    QScopedPointer<WindowControlHost> m_controlHost;
    QScopedPointer<ControlApi> m_controlApi;
    QScopedPointer<ControlServer> m_controlServer;
    AuroraLink* m_auroraLink = nullptr;
    QAction* m_auroraAction = nullptr;
    ControlIndicator* m_controlIndicator;
    // status bar (#88): Aurora link status and the "Tips on" switch
    AuroraIndicator* m_auroraIndicator = nullptr;
    TipsSwitch* m_tipsSwitch = nullptr;

    // UX phase 1b
    TopBar* m_topBar = nullptr;
    ReasonLine* m_reasonLine = nullptr;
    EmptyState* m_emptyState = nullptr;
    HintCards* m_hintCards = nullptr;
    bool m_startedWithFileOrControl = false;
    void buildHintCards();
    // helper mode (job UX-C, src/helpermode.h): the Helper switch at the top, the helper panel
    // with the hint cards and the lessons, and the ring around the control they talk about
    HelperMode* m_helper = nullptr;
    void buildHelper();
    QWidget* helperTarget(const QString& name) const;
    bool helperAlreadyDone(const QString& event) const;
    // true while no animation is open: a clean untitled animation stays behind the empty state
    // so the old code never meets "no animation"; the commands see none (see WindowControlHost)
    bool m_empty = false;
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    // Undo and Redo (job B1, src/undohistory.h; window side in pinglymotionwindowundo.cpp)
    UndoController* m_undo = nullptr;
    void buildUndo();
    // a step was put back: the fields, the timeline and the view show the animation again
    void undoRestored(Animation* anim);
    QAction* m_setKeyAction = nullptr;
    QAction* m_pasteMirroredAction = nullptr;
    QAction* m_lightThemeAction = nullptr;
    QAction* m_expertModeAction = nullptr;
    // UX job U2: the view settings in the View menu (src/viewmenuspec.h). Body, Joint groups
    // and Lighting come from Options; the four Show items are the view toolbar's settings.
    QMenu* m_bodyMenu = nullptr;
    QMenu* m_jointGroupsMenu = nullptr;
    QAction* m_lightingAction = nullptr;
    QAction* m_viewPartActions[4] = {nullptr, nullptr, nullptr, nullptr};
    QAction* m_clickAndBendAction = nullptr;
    // what the actions that need an animation were before the empty state greyed them
    struct ActionState { bool enabled; QString tip; QString status; };
    QMap<QAction*,ActionState> m_beforeEmpty;

    // UX phase 1d: Setup, Camera and keys, Keys and mouse (src/setupdialog.*, src/camerakeysdialog.*,
    // src/keysmousedialog.*)
    void openSetupDialog();
    void openCameraKeysDialog();
    void openKeysMouseDialog();
    // UX phase 4a: store the camera habit, let the 3D view follow it at once and say so
    // in the status line ("Camera and keys like Maya.")
    void chooseCameraHabit(const QString& habit);

    // UX phase 4b: the view toolbar in the 3D view (src/viewtoolbar.*). Every choice goes through
    // these, from the toolbar and from the keys alike: they store it, pass it to the 3D view and
    // the toolbar, and say so in the status line (the mockup's words, src/viewtoolspec.h).
    ViewToolbar* m_viewToolbar = nullptr;
    void setupViewToolbar();
    void chooseViewTool(ViewTool tool);
    void chooseViewPart(ViewToolSpec::Part part, bool shown);
    static bool viewPartShown(ViewToolSpec::Part part);
    // View > Click and bend on the figure (UX job U2)
    void setClickAndBend(bool on);
    void focusPickedJoint();
    // UX phase 4b part 2: a pull of a joint in the 3D view was released; sets the keys
    void pullFinished(BVHNode* pulled,const QList<QPair<BVHNode*,QVector3D>>& turns,bool turnOnly);
    // the hint about what is under the mouse in the 3D view, in the status line while hints are on
    void showHoverHint(const QString& hint);
    QString m_shownHoverHint;
};

#endif
