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

#ifndef USERSETTINGS_H
#define USERSETTINGS_H

/**
	@author Zi Ree <Zi Ree @ Second Life>
*/

#include <QByteArray>
#include <QMap>
#include <QRect>
#include <QStringList>

#include <functional>

class QObject;

class QString;

class UserSettings
{
  public:
    UserSettings() = delete;

    static void setLoop(bool value);
    static bool loop();
    static void setJointLimits(bool value);
    static bool jointLimits();
    static void setProtectFirstFrame(bool value);
    static bool protectFirstFrame();
    // retired: the timeline's visibility is part of panel_layout since the
    // three panels; read for old files only, never written again
    static void setDarkTheme(bool value);
    // the panel layout (QMainWindow::saveState) and the version it was saved
    // with; a layout of another version is not loaded (docs/archive/PANELS_PLAN.md, 7)
    static void setPanelLayout(const QByteArray& value);
    static QByteArray panelLayout();
    static void setPanelLayoutVersion(int value);
    static int panelLayoutVersion();
    // the layout of Beginner mode (UX-E): each mode keeps its own. panelLayout() is the
    // Expert layout (the one every earlier version saved); this one is empty until the user
    // has used Beginner mode with a window of this version, and then the mode's default is used.
    // It is saved with the same version as panelLayout().
    static void setPanelLayoutBeginner(const QByteArray& value);
    static QByteArray panelLayoutBeginner();
    // the layout of the mode asked for, and putting one away
    // the layout of the Script side of the Animate / Script switch (UX-F, src/workmode.h); empty
    // until the Script side has been left once. The Animate side uses panelLayoutFor above.
    static void setPanelLayoutScript(const QByteArray& value);
    static QByteArray panelLayoutScript();
    static QByteArray panelLayoutFor(bool expert) { return expert ? panelLayout() : panelLayoutBeginner(); }
    static void setPanelLayoutFor(bool expert,const QByteArray& value)
    { if(expert) setPanelLayout(value); else setPanelLayoutBeginner(value); }
    // the window's place, size, maximised state and screen
    // (QMainWindow::saveGeometry), docs/archive/PANELS_PLAN.md job P3
    static void setMainWindowGeometry(const QByteArray& value);
    static QByteArray mainWindowGeometry();
    static bool darkTheme();
    // which avatar body the 3D view shows: "ruth2" (default) or "max".
    // Anything else, such as "system" from older versions, becomes "ruth2".
    static void setBody(const QString& value);
    static QString body();
    // the language of the program's texts: "system" (default, follow the computer) or a
    // language code such as "nb" (docs/TRANSLATION.md); not in any menu until a second language exists
    static void setLanguage(const QString& value);
    static QString language();
    // Options > Light...: 0-100, 50 is the standard look
    static void setLightBrightness(int value);
    static int lightBrightness();
    static void setLightShine(int value);
    static int lightShine();
    static void setLightWarmth(int value);
    static int lightWarmth();
    static void setFloorShadow(bool on);
    static bool floorShadow();
    // JointGroups::Group bits of the joint groups that are not shown
    static void setHiddenJointGroups(int groups);
    static int hiddenJointGroups();
    // joints of hidden groups: left out of the timeline and part list (true) or dimmed (false)
    static void setHideJointsInLists(bool on);
    static bool hideJointsInLists();
    // "Tips on" switch in the status bar: short help when the mouse rests on a control (#88)
    static void setTipsOn(bool on);
    static bool tipsOn();
    // Beginner (default, false) or Expert (true) mode, docs/UX_DECISIONS.md section 8.
    // Read by the timeline's Paste mirrored and the Edit curve button.
    // which parts of the Script Lab panel are open or folded: "key=open,key=folded";
    // a part not named keeps its first state (docs/UX_DECISIONS.md, 6d)
    static void setScriptLabParts(const QString& value);
    static QString scriptLabParts();
    // Script Lab for beginners (job U3): the word tips the user closed ("prim,rlv"), and whether
    // the tree and the 3D view pick the same piece (on at start)
    static void setScriptLabTipsClosed(const QString& value);
    static QString scriptLabTipsClosed();
    static void setScriptLabFollowSelection(bool on);
    static bool scriptLabFollowSelection();
    // job UX-B: the Script Lab window's tabs (the tab in front, the popped tabs and their places,
    // LabTabState), and which first sizes were given (1: docked big, 2: floating big)
    static void setScriptLabWindows(const QString& value);
    static QString scriptLabWindows();
    static void setScriptLabSized(int flags);
    static int scriptLabSized();
    // the Selected joint panel (docs/UX_DECISIONS.md, 6): what a drag in the 3D view
    // does, Pull the limb (true, default) or Turn only this joint (read by the drag of
    // UX phase 4b), and whether its More options part is open (default folded)
    static void setPullLimbOnDrag(bool on);
    static bool pullLimbOnDrag();
    static void setJointMoreOptions(bool open);
    static bool jointMoreOptions();

    // the welcome card (UX phase 6a, docs/UX_DECISIONS.md, 6k): true once it was answered or
    // skipped, so it shows only once; Help > Show the welcome card again shows it again
    static void setWelcomeSeen(bool seen);
    static bool welcomeSeen();
    // the hint card now shown (0 = the first one; the number of cards = all seen), kept
    // between sessions; a value below 0 gives 0, above 99 gives 99
    static void setHintCard(int index);
    static int hintCard();

    static void setExpertMode(bool on);
    static bool expertMode();
    // calls `changed` with the new value whenever setExpertMode() changes it, as long as
    // `context` lives (UX phase 5: the Edit curve button shows in Expert mode only)
    static void watchExpertMode(QObject* context, std::function<void(bool)> changed);
    // Script Lab editor (lane 1, jobs F2 and F3), all in Setup: suggestions only with Ctrl+Space
    // (default off: they come up while typing); the Lua switch (default off: no Lua anywhere);
    // the script language, "lsl" or "slua" (anything else gives "lsl"; used only while the Lua
    // switch is on)
    static void setSuggestOnRequestOnly(bool on);
    static bool suggestOnRequestOnly();
    static void setLuaSwitch(bool on);
    static bool luaSwitch();
    static void setScriptLanguage(const QString& language);
    static QString scriptLanguage();
    // calls `changed` whenever one of the three Script Lab editor settings changes, as long as `context` lives
    static void watchScriptEditor(QObject* context, std::function<void()> changed);
    // where a separate window was and how big (frame top left and inner size), by name,
    // kept between sessions (UX phase 5, the Edit curve window); a null rect when unknown
    static void setWindowPlace(const QString& name, const QRect& place);
    static QRect windowPlace(const QString& name);
    // camera habit: one of cameraHabits(), default "pinglymotion"; anything else becomes the default.
    // The 3D view follows it since UX phase 4a.
    static void setCameraHabit(const QString& value);
    static QString cameraHabit();
    static QStringList cameraHabits();   // "pinglymotion", "maya", "blender", "sl"
    // the view toolbar in the 3D view (UX phase 4b): the tool of the left mouse button, one of
    // viewTools() ("pick" by default; anything else becomes "pick"), and its Show menu
    static void setViewTool(const QString& value);
    static QString viewTool();
    static QStringList viewTools();      // "pick", "orbit", "pan", "zoom"
    static void setShowFloor(bool on);
    static bool showFloor();
    static void setShowRings(bool on);
    static bool showRings();
    static void setViewToolNames(bool on);
    static bool viewToolNames();
    // UX job U2 (docs/UX_DECISIONS.md, 6q): View > Joint dots, the same switch as Show > Joint dots
    // ("show_joint_dots", on by default; it replaces Options > Show skeleton, "skeleton", which did
    // nothing any more), View > Click and bend on the figure ("click_and_bend", on by default), and
    // the timeline's row filter: the body area ("timeline_area": "all", "body", "face", "hands" or
    // "other"; anything else becomes "all") and Only changed joints ("timeline_changed_only", on)
    static void setShowJointDots(bool on);
    static bool showJointDots();
    static void setClickAndBend(bool on);
    static bool clickAndBend();
    static void setTimelineArea(const QString& value);
    static QString timelineArea();
    static QStringList timelineAreas();  // "all", "body", "face", "hands", "other"
    static void setTimelineChangedOnly(bool on);
    static bool timelineChangedOnly();
    // text size in percent, 100 (default) to 200; applied to the whole program
    // by src/textsize.* (main() at start, PinglyMotionWindow::setTextSizePercent).
    // A value outside the range is cut to the nearest end.
    static void setTextSizePercent(int value);
    static int textSizePercent();

    // retired: read once for the move to main_window_geometry, never written
    // again (docs/archive/PANELS_PLAN.md, section 7); 0 when the setting is missing
    static int mainWindowWidth();
    static int mainWindowHeight();

    static void setLastPath(QString value);
    static QString lastPath();

    static void setFog(bool on);
    static bool fog();
    static void setFloorTranslucency(int value);
    static int  floorTranslucency();

    static void setEaseIn(bool on);
    static bool easeIn();
    static void setEaseOut(bool on);
    static bool easeOut();

    static void readSettings();
    static void writeSettings();

private:
    static const QString m_userSettingsGroup;
    static const QString m_userSettingsVersionGroup;

    static bool m_loop;
    static const QString m_loopKey;
    static bool m_jointLimits;
    static const QString m_jointLimitsKey;
    static bool m_protectFirstFrame;
    static const QString m_protectFirstFrameKey;
    static bool m_darkTheme;
    static QByteArray m_panelLayout;
    static QByteArray m_panelLayoutBeginner;
    static QByteArray m_panelLayoutScript;
    static const QString m_panelLayoutKey;
    static int m_panelLayoutVersion;
    static const QString m_panelLayoutVersionKey;
    static QByteArray m_mainWindowGeometry;
    static const QString m_mainWindowGeometryKey;
    static const QString m_darkThemeKey;
    static QString m_body;
    static const QString m_bodyKey;
    static QString m_language;
    static const QString m_languageKey;
    static const QString m_bentoBodyKey;   // read from files of older versions
    static int m_lightBrightness;
    static const QString m_lightBrightnessKey;
    static int m_lightShine;
    static const QString m_lightShineKey;
    static int m_lightWarmth;
    static const QString m_lightWarmthKey;
    static bool m_floorShadow;
    static const QString m_floorShadowKey;
    static int m_hiddenJointGroups;
    static const QString m_hiddenJointGroupsKey;
    static bool m_hideJointsInLists;
    static const QString m_hideJointsInListsKey;
    static bool m_tipsOn;
    static const QString m_tipsOnKey;
    static QString m_scriptLabParts;
    static const QString m_scriptLabPartsKey;
    static QString m_scriptLabTipsClosed;
    static const QString m_scriptLabTipsClosedKey;
    static bool m_scriptLabFollowSelection;
    static const QString m_scriptLabFollowSelectionKey;
    static QString m_scriptLabWindows;
    static const QString m_scriptLabWindowsKey;
    static int m_scriptLabSized;
    static const QString m_scriptLabSizedKey;
    static bool m_pullLimbOnDrag;
    static const QString m_pullLimbOnDragKey;
    static bool m_jointMoreOptions;
    static const QString m_jointMoreOptionsKey;
    static bool m_welcomeSeen;
    static const QString m_welcomeSeenKey;
    static int m_hintCard;
    static const QString m_hintCardKey;
    static bool m_expertMode;
    static const QString m_expertModeKey;
    static bool m_suggestOnRequestOnly;
    static const QString m_suggestOnRequestOnlyKey;
    static bool m_luaSwitch;
    static const QString m_luaSwitchKey;
    static QString m_scriptLanguage;
    static const QString m_scriptLanguageKey;
    static QMap<QString, QRect> m_windowPlaces;
    static const QString m_windowPlacesGroup;
    static QString m_cameraHabit;
    static const QString m_cameraHabitKey;
    static QString m_viewTool;
    static const QString m_viewToolKey;
    static bool m_showFloor;
    static const QString m_showFloorKey;
    static bool m_showRings;
    static const QString m_showRingsKey;
    static bool m_viewToolNames;
    static const QString m_viewToolNamesKey;
    static bool m_showJointDots;
    static const QString m_showJointDotsKey;
    static bool m_clickAndBend;
    static const QString m_clickAndBendKey;
    static QString m_timelineArea;
    static const QString m_timelineAreaKey;
    static bool m_timelineChangedOnly;
    static const QString m_timelineChangedOnlyKey;
    static int m_textSizePercent;
    static const QString m_textSizePercentKey;

    static int m_mainWindowWidth;
    static const QString m_mainWindowWidthKey;
    static int m_mainWindowHeight;
    static const QString m_mainWindowHeightKey;

    static QString m_lastPath;
    static const QString m_lastPathKey;

    static bool m_fog;
    static const QString m_fogKey;
    static int  m_floorTranslucency;
    static const QString  m_floorTranslucencyKey;

    static bool m_easeIn;
    static const QString m_easeInKey;
    static bool m_easeOut;
    static const QString m_easeOutKey;
};

#endif
