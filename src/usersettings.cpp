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

#include "panellayout.h"
#include "settings.h"

#include "usersettings.h"

const QString UserSettings::m_userSettingsGroup = "user_settings";
const QString UserSettings::m_userSettingsVersionGroup = "1.0";

bool UserSettings::m_loop = true;
const QString UserSettings::m_loopKey = "loop";
bool UserSettings::m_skeleton = false;
const QString UserSettings::m_skeletonKey = "skeleton";
bool UserSettings::m_jointLimits = true;
const QString UserSettings::m_jointLimitsKey = "joint_limits";
bool UserSettings::m_protectFirstFrame = true;
const QString UserSettings::m_protectFirstFrameKey = "protect_first_frame";
bool UserSettings::m_darkTheme = true;
const QString UserSettings::m_darkThemeKey = "dark_theme";
QString UserSettings::m_body = "ruth2";
const QString UserSettings::m_bodyKey = "body";
QString UserSettings::m_language = "system";
const QString UserSettings::m_languageKey = "language";
const QString UserSettings::m_bentoBodyKey = "bento_body";
int UserSettings::m_lightBrightness = 50;
const QString UserSettings::m_lightBrightnessKey = "light_brightness";
int UserSettings::m_lightShine = 50;
const QString UserSettings::m_lightShineKey = "light_shine";
int UserSettings::m_lightWarmth = 50;
const QString UserSettings::m_lightWarmthKey = "light_warmth";
bool UserSettings::m_floorShadow = true;
const QString UserSettings::m_floorShadowKey = "floor_shadow";
int UserSettings::m_hiddenJointGroups = 7;   // JointGroups::DEFAULT_HIDDEN: wings, tail, hind limbs
const QString UserSettings::m_hiddenJointGroupsKey = "hidden_joint_groups";
bool UserSettings::m_hideJointsInLists = false;
const QString UserSettings::m_hideJointsInListsKey = "hide_joints_in_lists";
bool UserSettings::m_tipsOn = true;
const QString UserSettings::m_tipsOnKey = "tips_on";
QString UserSettings::m_scriptLabParts;
const QString UserSettings::m_scriptLabPartsKey = "scriptlab_parts";
bool UserSettings::m_pullLimbOnDrag = true;
const QString UserSettings::m_pullLimbOnDragKey = "pull_limb_on_drag";
bool UserSettings::m_jointMoreOptions = false;
const QString UserSettings::m_jointMoreOptionsKey = "joint_more_options";
bool UserSettings::m_expertMode = false;
const QString UserSettings::m_expertModeKey = "expert_mode";
QString UserSettings::m_cameraHabit = "pinglymotion";
const QString UserSettings::m_cameraHabitKey = "camera_habit";
QString UserSettings::m_viewTool = "pick";
const QString UserSettings::m_viewToolKey = "view_tool";
bool UserSettings::m_showFloor = true;
const QString UserSettings::m_showFloorKey = "show_floor";
bool UserSettings::m_showRings = true;
const QString UserSettings::m_showRingsKey = "show_rings";
bool UserSettings::m_viewToolNames = true;
const QString UserSettings::m_viewToolNamesKey = "view_tool_names";
int UserSettings::m_textSizePercent = 100;
const QString UserSettings::m_textSizePercentKey = "text_size_percent";
QByteArray UserSettings::m_panelLayout;
const QString UserSettings::m_panelLayoutKey = PanelLayout::SettingLayout;
int UserSettings::m_panelLayoutVersion = 0;
const QString UserSettings::m_panelLayoutVersionKey = PanelLayout::SettingLayoutVersion;
QByteArray UserSettings::m_mainWindowGeometry;
const QString UserSettings::m_mainWindowGeometryKey = PanelLayout::SettingWindowGeometry;

int UserSettings::m_mainWindowWidth = 0;
const QString UserSettings::m_mainWindowWidthKey = PanelLayout::LegacyWidth;
int UserSettings::m_mainWindowHeight = 0;
const QString UserSettings::m_mainWindowHeightKey = PanelLayout::LegacyHeight;

QString UserSettings::m_lastPath = QString();
const QString UserSettings::m_lastPathKey = "last_path";

bool UserSettings::m_fog = true;
const QString UserSettings::m_fogKey = "fog";
int  UserSettings::m_floorTranslucency = 33;
const QString UserSettings::m_floorTranslucencyKey = "floor_translucency";

bool UserSettings::m_easeIn = false;
const QString UserSettings::m_easeInKey = "ease_in";
bool UserSettings::m_easeOut = false;
const QString UserSettings::m_easeOutKey = "ease_out";


void UserSettings::setLoop(bool on)               { m_loop = on; }
bool UserSettings::loop()                            { return m_loop; }
void UserSettings::setSkeleton(bool on)           { m_skeleton = on; }
bool UserSettings::skeleton()                        { return m_skeleton; }
void UserSettings::setJointLimits(bool on)        { m_jointLimits = on; }
bool UserSettings::jointLimits()                     { return m_jointLimits; }
void UserSettings::setProtectFirstFrame(bool on)  { m_protectFirstFrame = on; }
bool UserSettings::protectFirstFrame()               { return m_protectFirstFrame; }
void UserSettings::setDarkTheme(bool on)          { m_darkTheme = on; }
bool UserSettings::darkTheme()                       { return m_darkTheme; }
void UserSettings::setBody(const QString& value)
{
  m_body = (value == "max") ? value : QString("ruth2");
}
QString UserSettings::body()                         { return m_body; }
void UserSettings::setLanguage(const QString& value)
{
  const QString code = value.trimmed();
  m_language = code.isEmpty() ? QString("system") : code;
}
QString UserSettings::language()                     { return m_language; }
void UserSettings::setLightBrightness(int value)     { m_lightBrightness = qBound(0, value, 100); }
int UserSettings::lightBrightness()                  { return m_lightBrightness; }
void UserSettings::setLightShine(int value)          { m_lightShine = qBound(0, value, 100); }
int UserSettings::lightShine()                       { return m_lightShine; }
void UserSettings::setLightWarmth(int value)         { m_lightWarmth = qBound(0, value, 100); }
int UserSettings::lightWarmth()                      { return m_lightWarmth; }
void UserSettings::setFloorShadow(bool on)           { m_floorShadow = on; }
bool UserSettings::floorShadow()                     { return m_floorShadow; }
void UserSettings::setHiddenJointGroups(int groups)  { m_hiddenJointGroups = groups & 31; }
int UserSettings::hiddenJointGroups()                { return m_hiddenJointGroups; }
void UserSettings::setHideJointsInLists(bool on)     { m_hideJointsInLists = on; }
bool UserSettings::hideJointsInLists()               { return m_hideJointsInLists; }
void UserSettings::setTipsOn(bool on)                { m_tipsOn = on; }
bool UserSettings::tipsOn()                          { return m_tipsOn; }
void UserSettings::setScriptLabParts(const QString& value) { m_scriptLabParts = value.left(1000); }
QString UserSettings::scriptLabParts()               { return m_scriptLabParts; }
void UserSettings::setPullLimbOnDrag(bool on)        { m_pullLimbOnDrag = on; }
bool UserSettings::pullLimbOnDrag()                  { return m_pullLimbOnDrag; }
void UserSettings::setJointMoreOptions(bool open)    { m_jointMoreOptions = open; }
bool UserSettings::jointMoreOptions()                { return m_jointMoreOptions; }
void UserSettings::setExpertMode(bool on)            { m_expertMode = on; }
bool UserSettings::expertMode()                      { return m_expertMode; }
QStringList UserSettings::cameraHabits()
{
  return { QStringLiteral("pinglymotion"), QStringLiteral("maya"), QStringLiteral("blender"), QStringLiteral("sl") };
}
void UserSettings::setCameraHabit(const QString& value)
{
  m_cameraHabit = cameraHabits().contains(value) ? value : QString("pinglymotion");
}
QString UserSettings::cameraHabit()                  { return m_cameraHabit; }

QStringList UserSettings::viewTools()
{
  return {"pick", "orbit", "pan", "zoom"};
}
void UserSettings::setViewTool(const QString& value)
{
  m_viewTool = viewTools().contains(value) ? value : QString("pick");
}
QString UserSettings::viewTool()                     { return m_viewTool; }
void UserSettings::setShowFloor(bool on)             { m_showFloor = on; }
bool UserSettings::showFloor()                       { return m_showFloor; }
void UserSettings::setShowRings(bool on)             { m_showRings = on; }
bool UserSettings::showRings()                       { return m_showRings; }
void UserSettings::setViewToolNames(bool on)         { m_viewToolNames = on; }
bool UserSettings::viewToolNames()                   { return m_viewToolNames; }
void UserSettings::setTextSizePercent(int value)     { m_textSizePercent = qBound(100, value, 200); }
int UserSettings::textSizePercent()                  { return m_textSizePercent; }
void UserSettings::setPanelLayout(const QByteArray& state) { m_panelLayout = state; }
QByteArray UserSettings::panelLayout()               { return m_panelLayout; }
void UserSettings::setPanelLayoutVersion(int value)  { m_panelLayoutVersion = value; }
int UserSettings::panelLayoutVersion()               { return m_panelLayoutVersion; }
void UserSettings::setMainWindowGeometry(const QByteArray& value) { m_mainWindowGeometry = value; }
QByteArray UserSettings::mainWindowGeometry()        { return m_mainWindowGeometry; }

int UserSettings::mainWindowWidth()                  { return m_mainWindowWidth; }
int UserSettings::mainWindowHeight()                 { return m_mainWindowHeight; }

void UserSettings::setLastPath(QString value)        { m_lastPath = value; }
QString UserSettings::lastPath()                     { return m_lastPath; }

void UserSettings::setFog(bool on)                { m_fog = on; }
bool UserSettings::fog()                             { return m_fog; }
void UserSettings::setFloorTranslucency(int on)   { m_floorTranslucency = on; }
int UserSettings::floorTranslucency()               { return m_floorTranslucency; }

void UserSettings::setEaseIn(bool on)             { m_easeIn = on; }
bool UserSettings::easeIn()                          { return m_easeIn; }
void UserSettings::setEaseOut(bool on)            { m_easeOut = on; }
bool UserSettings::easeOut()                         { return m_easeOut; }

void UserSettings::readSettings()
{
  Settings settings;
  settings.beginGroup(m_userSettingsGroup);

  if (settings.childGroups().contains(m_userSettingsVersionGroup, Qt::CaseInsensitive))
  {
    settings.beginGroup(m_userSettingsVersionGroup);

    // Loop is on by default (docs/UX_DECISIONS.md section 1)
    setLoop(settings.value(m_loopKey, true).toBool());
    setSkeleton(settings.value(m_skeletonKey).toBool());
    setJointLimits(settings.value(m_jointLimitsKey).toBool());
    setProtectFirstFrame(settings.value(m_protectFirstFrameKey).toBool());
    setDarkTheme(settings.value(m_darkThemeKey, true).toBool());
    // older versions saved only "Bento body (Max)" on or off
    if (settings.contains(m_bodyKey))
      setBody(settings.value(m_bodyKey).toString());
    else if (settings.contains(m_bentoBodyKey))
      setBody(settings.value(m_bentoBodyKey).toBool() ? "max" : "ruth2");
    else
      setBody("ruth2");
    setLanguage(settings.value(m_languageKey, "system").toString());
    setLightBrightness(settings.value(m_lightBrightnessKey, 50).toInt());
    setLightShine(settings.value(m_lightShineKey, 50).toInt());
    setLightWarmth(settings.value(m_lightWarmthKey, 50).toInt());
    setFloorShadow(settings.value(m_floorShadowKey, true).toBool());
    setHiddenJointGroups(settings.value(m_hiddenJointGroupsKey, 7).toInt());
    setHideJointsInLists(settings.value(m_hideJointsInListsKey, false).toBool());
    setTipsOn(settings.value(m_tipsOnKey, true).toBool());
    setScriptLabParts(settings.value(m_scriptLabPartsKey).toString());
    setPullLimbOnDrag(settings.value(m_pullLimbOnDragKey, true).toBool());
    setJointMoreOptions(settings.value(m_jointMoreOptionsKey, false).toBool());
    // only true/false (or 1/0) count; anything else gives Beginner mode
    setExpertMode(settings.value(m_expertModeKey, false).toString().toLower() == "true"
                  || settings.value(m_expertModeKey, false).toString() == "1");
    setCameraHabit(settings.value(m_cameraHabitKey, "pinglymotion").toString());
    setViewTool(settings.value(m_viewToolKey, "pick").toString());
    setShowFloor(settings.value(m_showFloorKey, true).toBool());
    setShowRings(settings.value(m_showRingsKey, true).toBool());
    setViewToolNames(settings.value(m_viewToolNamesKey, true).toBool());
    // a stored value that is not a whole number gives 100
    bool sizeOk = false;
    const int size = settings.value(m_textSizePercentKey, 100).toInt(&sizeOk);
    setTextSizePercent(sizeOk ? size : 100);
    // the old window_state (with the Properties panel) is not read any more
    setPanelLayout(settings.value(m_panelLayoutKey).toByteArray());
    setPanelLayoutVersion(settings.value(m_panelLayoutVersionKey, 0).toInt());
    setMainWindowGeometry(settings.value(m_mainWindowGeometryKey).toByteArray());

    // the old width and height: read once, used only while main_window_geometry is empty
    m_mainWindowWidth = settings.value(m_mainWindowWidthKey, 0).toInt();
    m_mainWindowHeight = settings.value(m_mainWindowHeightKey, 0).toInt();

    setLastPath(settings.value(m_lastPathKey).toString());

    // OpenGL settings
    setFog(settings.value(m_fogKey).toBool());
    setFloorTranslucency(settings.value(m_floorTranslucencyKey).toInt());

    // settings for ease in/ease outFrame
    setEaseIn(settings.value(m_easeInKey).toBool());
    setEaseOut(settings.value(m_easeOutKey).toBool());

    settings.endGroup();
  }

  settings.endGroup();
}

void UserSettings::writeSettings()
{
  Settings settings;
  settings.beginGroup(m_userSettingsGroup);
  settings.beginGroup(m_userSettingsVersionGroup);

  settings.setValue(m_loopKey, loop());
  settings.setValue(m_skeletonKey, skeleton());
  settings.setValue(m_jointLimitsKey, jointLimits());
  settings.setValue(m_protectFirstFrameKey, protectFirstFrame());
  settings.setValue(m_darkThemeKey, darkTheme());
  settings.setValue(m_bodyKey, body());
  settings.setValue(m_languageKey, language());
  settings.remove(m_bentoBodyKey);
  settings.setValue(m_lightBrightnessKey, lightBrightness());
  settings.setValue(m_lightShineKey, lightShine());
  settings.setValue(m_lightWarmthKey, lightWarmth());
  settings.setValue(m_floorShadowKey, floorShadow());
  settings.setValue(m_hiddenJointGroupsKey, hiddenJointGroups());
  settings.setValue(m_hideJointsInListsKey, hideJointsInLists());
  settings.setValue(m_tipsOnKey, tipsOn());
  settings.setValue(m_scriptLabPartsKey, scriptLabParts());
  settings.setValue(m_pullLimbOnDragKey, pullLimbOnDrag());
  settings.setValue(m_jointMoreOptionsKey, jointMoreOptions());
  settings.setValue(m_expertModeKey, expertMode());
  settings.setValue(m_cameraHabitKey, cameraHabit());
  settings.setValue(m_viewToolKey, viewTool());
  settings.setValue(m_showFloorKey, showFloor());
  settings.setValue(m_showRingsKey, showRings());
  settings.setValue(m_viewToolNamesKey, viewToolNames());
  settings.setValue(m_textSizePercentKey, textSizePercent());
  settings.setValue(m_panelLayoutKey, panelLayout());
  settings.setValue(m_panelLayoutVersionKey, panelLayoutVersion());
  settings.setValue(m_mainWindowGeometryKey, mainWindowGeometry());
  // show_timeline_panel, main_window_width and main_window_height are retired:
  // read once above, never written again (docs/archive/PANELS_PLAN.md, section 7)

  settings.setValue(m_lastPathKey, lastPath());

  // OpenGL settings
  settings.setValue(m_fogKey, fog());
  settings.setValue(m_floorTranslucencyKey, floorTranslucency());

  // settings for ease in/ease outFrame
  settings.setValue(m_easeInKey, easeIn());
  settings.setValue(m_easeOutKey, easeOut());

  settings.endGroup();
  settings.endGroup();
}
