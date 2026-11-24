/*
 * PinglyMotion - the view settings in the View menu, and the switch for
 * clicking and bending joints on the figure (UX job U2,
 * docs/UX_DECISIONS.md 6q).
 *
 * Every setting of what the 3D view shows is in the View menu now: the four
 * Show items of the view toolbar (the same settings), Click and bend on the
 * figure, the joint groups (from Options > Joints), Body and Lighting (from
 * Options). Options > Show skeleton became View > Joint dots, the same switch
 * as Show > Joint dots in the view toolbar. One table for the window and for
 * tests/viewtimeline, which checks every text, key and tip exactly.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef VIEWMENUSPEC_H
#define VIEWMENUSPEC_H

#include <QCoreApplication>
#include <QString>

#include "jointgroups.h"
#include "menuspec.h"
#include "viewtoolspec.h"

namespace ViewMenuSpec
{
  using MenuSpec::Item;

  // the group of view settings, between "Look from" and "Panels"
  inline constexpr const char* showHeading = QT_TRANSLATE_NOOP("MenuSpec", "Show");

  // the view toolbar's Show items, in its order (ViewToolSpec::parts); the same settings
  inline constexpr Item viewFloorGrid = {"viewFloorGridAction", QT_TRANSLATE_NOOP("MenuSpec", "Floor grid"), "", ""};
  inline constexpr Item viewRotationRings = {"viewRotationRingsAction", QT_TRANSLATE_NOOP("MenuSpec", "Rotation rings"), "", ""};
  // Options > Show skeleton before U2; it hides the dots on the joints, not the bone lines
  inline constexpr Item viewJointDots = {"viewJointDotsAction", QT_TRANSLATE_NOOP("MenuSpec", "Joint dots"), "", ""};
  inline constexpr Item viewToolNames = {"viewToolNamesAction", QT_TRANSLATE_NOOP("MenuSpec", "Tool names in the view toolbar"), "", ""};
  // the items above, in the order of ViewToolSpec::Part
  inline constexpr const Item* partItems[] = {&viewFloorGrid, &viewRotationRings, &viewJointDots, &viewToolNames};

  // B works while the 3D view has the keys, like the other letter keys of the view
  inline constexpr Item viewClickAndBend = {"viewClickAndBendAction", QT_TRANSLATE_NOOP("MenuSpec", "Click and bend on the figure"), "B", ""};

  // the joint groups, from Options > Joints (display only; JointGroups)
  inline constexpr const char* jointGroupsMenu = QT_TRANSLATE_NOOP("MenuSpec", "Joint groups");
  struct Group
  {
    const char* id;
    int group;            // JointGroups::Group
    const char* text;
  };
  inline constexpr Group groups[] = {
    {"viewGroupWingsAction", JointGroups::Wings, QT_TRANSLATE_NOOP("MenuSpec", "Wings")},
    {"viewGroupTailAction", JointGroups::Tail, QT_TRANSLATE_NOOP("MenuSpec", "Tail")},
    {"viewGroupHindLimbsAction", JointGroups::HindLimbs, QT_TRANSLATE_NOOP("MenuSpec", "Hind limbs")},
    {"viewGroupFaceAction", JointGroups::Face, QT_TRANSLATE_NOOP("MenuSpec", "Face")},
    {"viewGroupFingersToesAction", JointGroups::FingersToes, QT_TRANSLATE_NOOP("MenuSpec", "Fingers and toes")}};
  inline constexpr Item viewHideInLists = {"viewHideInListsAction", QT_TRANSLATE_NOOP("MenuSpec", "Hide in lists"), "", ""};

  // Body and Lighting, from Options
  inline constexpr const char* bodyMenu = QT_TRANSLATE_NOOP("MenuSpec", "Body");
  inline constexpr Item viewLighting = {"viewLightingAction", QT_TRANSLATE_NOOP("MenuSpec", "Lighting..."), "", ""};

  // Fog and the floor's transparency, from the old Settings window (job "remove old menus",
  // docs/UX_DECISIONS.md 6h). Fog is off at the first start. The floor's see-through share is
  // picked from the list below; the default is 33 percent, as the old window had it.
  inline constexpr Item viewFog = {"viewFogAction", QT_TRANSLATE_NOOP("MenuSpec", "Fog"), "", ""};
  inline constexpr const char* floorTransparencyMenu = QT_TRANSLATE_NOOP("MenuSpec", "Floor transparency");
  inline constexpr int floorTransparencyChoices[] = {0, 25, 33, 50, 75};
  inline constexpr int floorTransparencyDefault = 33;

  // the tips (charter P3); at most 12 words a sentence
  inline constexpr const char* jointDotsTip = QT_TRANSLATE_NOOP("MenuSpec", "A dot on each joint you can pick. Off: you can still click them.");
  inline constexpr const char* clickAndBendTip = QT_TRANSLATE_NOOP("MenuSpec", "Pick and bend joints by clicking them on the figure. Off: use the Joints list and the sliders.");
  inline constexpr const char* jointGroupsTip = QT_TRANSLATE_NOOP("MenuSpec", "Show or hide groups of extra joints. Hiding changes only what you see.");
  inline constexpr const char* hideInListsTip = QT_TRANSLATE_NOOP("MenuSpec", "Leaves hidden joints out of the timeline and the joint list. Off: they show dimmed.");
  inline constexpr const char* fogTip = QT_TRANSLATE_NOOP("MenuSpec", "Fades the far end of the floor into the background. Off at the start.");
  inline constexpr const char* floorTransparencyTip = QT_TRANSLATE_NOOP("MenuSpec", "How much you see through the floor. 0% is solid.");
  inline constexpr const char* lightingTip = QT_TRANSLATE_NOOP("MenuSpec", "Change the light on the body: brightness, shine, warmth, floor shadow.");

  // the status line after Click and bend was switched, and when the figure is clicked while it is off
  inline constexpr const char* clickAndBendOn = QT_TRANSLATE_NOOP("MenuSpec", "Click and bend is on. Click a joint to pick it, drag to bend.");
  inline constexpr const char* clickAndBendOff = QT_TRANSLATE_NOOP("MenuSpec", "Click and bend is off. Clicks on the figure do nothing now.");
  inline constexpr const char* clickAndBendBlocked = QT_TRANSLATE_NOOP("MenuSpec", "Click and bend is off. Press B or use the View menu to turn it on.");

  inline QString tr(const char* source)
  {
    return QCoreApplication::translate("MenuSpec", source);
  }

  inline QString clickAndBendMessage(bool on)
  {
    return tr(on ? clickAndBendOn : clickAndBendOff);
  }

  // A press of the left button in the 3D view while picking: with Click and bend off, a
  // joint or a rotation ring under the mouse does nothing (no pick, no pull, no ring turn);
  // a prop, a prop handle and empty space work as before.
  inline bool pressBlocked(bool clickAndBend, bool onJoint, bool onRing)
  {
    return !clickAndBend && (onJoint || onRing);
  }

  // The rotation rings of the picked joint are drawn only while they can be used:
  // Show > Rotation rings on and Click and bend on.
  inline bool ringsShown(bool showRings, bool clickAndBend)
  {
    return showRings && clickAndBend;
  }

  // The joint dots are drawn while Joint dots is on. While picking they are always
  // drawn (in the pick colours only), so a click on a joint picks it with the dots off.
  inline bool dotsDrawn(bool showJointDots, bool picking)
  {
    return showJointDots || picking;
  }
}

#endif
