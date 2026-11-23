/*
 * PinglyMotion - the finger joints of the Hands tab, without a screen (UX phase 2c)
 *
 * The Hands tab of the Joints panel (src/handstab.h, docs/UX_DECISIONS.md 6i)
 * shapes the 15 finger joints of a hand: the thumb and four fingers, each with
 * a base, mid and tip joint. This file holds the sums behind it, so they can be
 * tested exactly:
 *
 *  - the nine ready-made finger poses: Firestorm's hand presets
 *    (bin/data/firestorm/hand_presets, LGPL 2.1, read by JointInfo) turned into
 *    PinglyMotion's angles (fromFirestorm(), below);
 *  - Bend each finger: one number per finger from 0 (the open hand, every angle
 *    0) to 100 (the Tight Grip preset); in between each of the finger's three
 *    joints turns the same share of the way along the shortest turn (a slerp);
 *  - the copy rule of Mirror: the other hand gets the same angles with Y and Z
 *    turned round, the rule Animation::setRotation() and Flip already use;
 *  - the joint changes ("writes") a pick, a finger slider, a grid cell or Reset
 *    hand asks the window to make.
 *
 * Firestorm stores a joint's turn as three angles in radians (roll, pitch,
 * yaw) around Second Life's X (forward), Y (left) and Z (up) axes, read and
 * written by LLQuaternion::getEulerAngles() and LLMatrix3::setRot(): the turn
 * is Rx(roll) * Ry(pitch) * Rz(yaw). PinglyMotion's joints use the BVH axes:
 * X is the figure's left, Y up and Z forward. So Second Life's X becomes our Z,
 * its Y our X and its Z our Y, and the turn is then written as the joint's own
 * three channels (Xrotation Zrotation Yrotation for the fingers). Our own
 * measurement agrees (the calibration table in docs/ASSISTANT_GUIDE.md): finger
 * curl is Z, negative on the left hand and positive on the right, and every
 * curled preset bends its fingers that way (tests/handstab checks it).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef HANDPOSE_H
#define HANDPOSE_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>

#include <array>

#include "rotation.h"

class Animation;
class BVHNode;
class JointInfo;

namespace HandPose
{
  enum class Side { Left, Right };

  inline constexpr int fingerCount = 5;
  inline constexpr int partCount = 3;
  inline constexpr int jointCount = fingerCount * partCount;

  // in the order of the mockup and of joints.json: Thumb, Index, Middle, Ring, Pinky
  inline constexpr const char* fingerKeys[fingerCount] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};

  // the preset that is 100 on the Bend each finger sliders
  inline constexpr const char* closedPreset = "Tight Grip";

  // the channel order of the finger joints in the program's skeletons
  inline constexpr Rotation::Order fingerOrder = Rotation::Order::XZY;

  // the 15 joints of a hand, row by row: Thumb Base, Thumb Mid, Thumb Tip, Index Base, ...
  using Hand = std::array<QVector3D, jointCount>;

  // "mHandIndex2Left" for side Left, index 4 (finger 1, part 1)
  QString jointName(Side side, int index);
  QStringList jointNames(Side side);
  // the row (0 to 14) of a finger joint and its side; -1 when name is not a finger joint
  int indexOf(const QString& name, Side* side = nullptr);
  Side other(Side side);

  // every angle 0
  Hand openHand();

  // Firestorm's stored turn (radians, Second Life axes, see above) as the
  // program's angles in degrees, in the order X, Y, Z, for a joint whose
  // channels come in the given order. Of the two sets of angles that make the
  // same turn, the one with the smaller angles in all is taken.
  QVector3D fromFirestorm(const QVector3D& radians, Rotation::Order order = fingerOrder);

  // the same turn on the other hand: Y and Z turned round
  QVector3D mirrored(const QVector3D& angles);
  Hand mirrored(const Hand& hand);

  struct Preset
  {
    QString name;     // "Thumbs Up"
    Hand left;
    Hand right;
    const Hand& hand(Side side) const { return side == Side::Left ? left : right; }
  };

  // the presets in JointInfo's order (joints.json), each angle rounded to a whole degree
  QVector<Preset> presets(const JointInfo& info);
  int presetIndex(const QVector<Preset>& presets, const QString& name);

  // true when an angle of the hand differs from 0 by more than half a degree
  bool changed(const Hand& hand);
  // the name of the preset the hand matches (every angle within half a degree), empty when none
  QString match(const Hand& hand, const QVector<Preset>& presets, Side side);

  // the turn that is t (0 to 1) of the way from no turn to "closed", as angles
  QVector3D blend(const QVector3D& closed, double t, Rotation::Order order = fingerOrder);
  // the hand with finger (0 to 4) bent percent (0 to 100) of the way to closed
  Hand withBend(const Hand& hand, const Hand& closed, int finger, int percent);
  // how far the finger is towards closed, 0 to 100: the share whose blend is nearest
  int bend(const Hand& hand, const Hand& closed, int finger);

  // one joint change for the window: the joint's name and its new angles
  struct Write
  {
    QString joint;
    QVector3D angles;
    bool operator==(const Write& o) const { return joint == o.joint && qFuzzyCompare(angles + QVector3D(1, 1, 1), o.angles + QVector3D(1, 1, 1)); }
  };
  using Writes = QVector<Write>;

  // the preset on the left hand, the right hand or both
  Writes presetWrites(const Preset& preset, bool left, bool right);
  // the given rows of hand on side; with mirror the other hand's same rows get the mirrored angles
  Writes handWrites(Side side, const Hand& hand, const QList<int>& rows, bool mirror);
  // every finger joint of the left hand, the right hand or both back to 0
  Writes resetWrites(bool left, bool right);

  // The window and the tests make the changes the same way: each write is
  // Animation::setRotation() at the current frame (a key at once, as the
  // Selected joint panel's sliders), with the animation's own Mirror copy off
  // for the moment, because the writes already hold both hands where wanted.
  // Answers how many joints were changed (a joint the skeleton lacks is skipped).
  int apply(Animation* anim, const Writes& writes);
  // a hand's angles at the current frame; a joint the skeleton lacks reads 0
  Hand read(Animation* anim, Side side);
  // the joint of that name in the animation, nullptr when there is none
  BVHNode* node(Animation* anim, const QString& name);
}

#endif
