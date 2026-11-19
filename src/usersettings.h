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

class QString;

class UserSettings
{
  public:
    UserSettings() = delete;

    static void setLoop(bool value);
    static bool loop();
    static void setSkeleton(bool value);
    static bool skeleton();
    static void setJointLimits(bool value);
    static bool jointLimits();
    static void setProtectFirstFrame(bool value);
    static bool protectFirstFrame();
    static void setShowTimelinePanel(bool value);
    static bool showTimelinePanel();
    static void setDarkTheme(bool value);
    static void setWindowState(const QByteArray& value);
    static QByteArray windowState();
    static bool darkTheme();
    // which avatar body the 3D view shows: "ruth2" (default) or "max".
    // Anything else, such as "system" from older versions, becomes "ruth2".
    static void setBody(const QString& value);
    static QString body();
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

    static void setMainWindowWidth(int value);
    static int mainWindowWidth();
    static void setMainWindowHeight(int value);
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
    static bool m_skeleton;
    static const QString m_skeletonKey;
    static bool m_jointLimits;
    static const QString m_jointLimitsKey;
    static bool m_protectFirstFrame;
    static const QString m_protectFirstFrameKey;
    static bool m_showTimelinePanel;
    static const QString m_showTimelinePanelKey;
    static bool m_darkTheme;
    static QByteArray m_windowState;
    static const QString m_windowStateKey;
    static const QString m_darkThemeKey;
    static QString m_body;
    static const QString m_bodyKey;
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
