/*
 * PinglyMotion - the finger joints of the Hands tab (see handpose.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "handpose.h"

#include <QQuaternion>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <limits>

#include "animation.h"
#include "bvhnode.h"
#include "jointinfo.h"

namespace
{
  constexpr double toDegrees = 180.0 / 3.14159265358979323846;

  QVector3D axisVector(int axis)
  {
    return axis == 0 ? QVector3D(1, 0, 0) : (axis == 1 ? QVector3D(0, 1, 0) : QVector3D(0, 0, 1));
  }

  // the turn of a joint with these angles (X, Y, Z in degrees) and channel order
  QQuaternion turnOf(const QVector3D& angles, Rotation::Order order)
  {
    int axes[3];
    if (!Rotation::axesOf(order, axes)) return QQuaternion();
    QQuaternion q;
    for (int axis : axes) q = q * QQuaternion::fromAxisAndAngle(axisVector(axis), angles[axis]);
    return q;
  }

  double wrap(double degrees)
  {
    while (degrees > 180.0) degrees -= 360.0;
    while (degrees <= -180.0) degrees += 360.0;
    return degrees;
  }

  // the angles (X, Y, Z) of a turn in channel order; of the two sets of
  // angles that make the same turn the one with the smaller angles in all is
  // taken when it is clearly smaller
  QVector3D anglesOf(const QQuaternion& q, Rotation::Order order, bool smallest)
  {
    int axes[3];
    if (!Rotation::axesOf(order, axes)) return QVector3D();
    double a[3];
    Rotation::eulerFromAxes(q, axes, a);
    if (smallest)
    {
      // the other angles for the same turn: first and last half a turn on, the middle one mirrored
      const double b[3] = {wrap(a[0] + 180.0), wrap(180.0 - a[1]), wrap(a[2] + 180.0)};
      // the set with the smaller angles in all is taken (a curl past a quarter turn reads
      // better as twist 0 and curl -90 than as twist -180, spread 171 and curl -90)
      double now = 0.0, then = 0.0;
      for (int i = 0; i < 3; ++i)
      {
        now += std::abs(a[i]);
        then += std::abs(b[i]);
      }
      if (then + 1.0e-6 < now) std::copy(b, b + 3, a);
    }
    QVector3D out;
    for (int i = 0; i < 3; ++i) out[axes[i]] = static_cast<float>(a[i]);
    return out;
  }

  double angleBetween(const QQuaternion& a, const QQuaternion& b)
  {
    const double dot = std::min(1.0, std::abs(static_cast<double>(QQuaternion::dotProduct(a.normalized(), b.normalized()))));
    return 2.0 * std::acos(dot);
  }
}

namespace HandPose
{
  QString jointName(Side side, int index)
  {
    if (index < 0 || index >= jointCount) return QString();
    return QStringLiteral("mHand%1%2%3")
      .arg(QLatin1String(fingerKeys[index / partCount]))
      .arg(index % partCount + 1)
      .arg(side == Side::Left ? QStringLiteral("Left") : QStringLiteral("Right"));
  }

  QStringList jointNames(Side side)
  {
    QStringList names;
    for (int i = 0; i < jointCount; ++i) names << jointName(side, i);
    return names;
  }

  int indexOf(const QString& name, Side* side)
  {
    static const QRegularExpression pattern(QStringLiteral("^mHand(Thumb|Index|Middle|Ring|Pinky)([123])(Left|Right)$"));
    const QRegularExpressionMatch m = pattern.match(name);
    if (!m.hasMatch()) return -1;
    int finger = 0;
    while (finger < fingerCount && m.captured(1) != QLatin1String(fingerKeys[finger])) ++finger;
    if (side) *side = m.captured(3) == QLatin1String("Left") ? Side::Left : Side::Right;
    return finger * partCount + m.captured(2).toInt() - 1;
  }

  Side other(Side side)
  {
    return side == Side::Left ? Side::Right : Side::Left;
  }

  Hand openHand()
  {
    Hand hand;
    hand.fill(QVector3D(0, 0, 0));
    return hand;
  }

  QVector3D fromFirestorm(const QVector3D& radians, Rotation::Order order)
  {
    // Rx(roll) * Ry(pitch) * Rz(yaw) around Second Life's axes, written with the
    // BVH axes: Second Life's X is our Z, its Y our X, its Z our Y
    const QQuaternion q = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), radians.x() * toDegrees) *
                          QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), radians.y() * toDegrees) *
                          QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), radians.z() * toDegrees);
    return anglesOf(q, order, true);
  }

  QVector3D mirrored(const QVector3D& angles)
  {
    // + 0.0f keeps a zero from turning into -0
    return QVector3D(angles.x(), -angles.y() + 0.0f, -angles.z() + 0.0f);
  }

  Hand mirrored(const Hand& hand)
  {
    Hand out;
    for (int i = 0; i < jointCount; ++i) out[i] = mirrored(hand[i]);
    return out;
  }

  QVector<Preset> presets(const JointInfo& info)
  {
    QVector<Preset> out;
    for (const JointInfo::HandPreset& source : info.handPresets())
    {
      Preset preset;
      preset.name = source.name;
      preset.left = openHand();
      preset.right = openHand();
      for (const JointInfo::PresetJoint& joint : source.joints)
      {
        Side side;
        const int row = indexOf(joint.sl, &side);
        if (row < 0) continue;
        QVector3D angles = fromFirestorm(joint.rotation);
        for (int axis = 0; axis < 3; ++axis) angles[axis] = static_cast<float>(std::round(angles[axis])) + 0.0f;
        (side == Side::Left ? preset.left : preset.right)[row] = angles;
      }
      out << preset;
    }
    return out;
  }

  int presetIndex(const QVector<Preset>& presets, const QString& name)
  {
    for (int i = 0; i < presets.count(); ++i)
      if (presets[i].name == name) return i;
    return -1;
  }

  bool changed(const Hand& hand)
  {
    for (const QVector3D& angles : hand)
      for (int axis = 0; axis < 3; ++axis)
        if (std::abs(angles[axis]) > 0.5f) return true;
    return false;
  }

  QString match(const Hand& hand, const QVector<Preset>& presets, Side side)
  {
    for (const Preset& preset : presets)
    {
      const Hand& wanted = preset.hand(side);
      bool same = true;
      for (int i = 0; i < jointCount && same; ++i)
        for (int axis = 0; axis < 3 && same; ++axis)
          same = std::abs(hand[i][axis] - wanted[i][axis]) < 0.5f;
      if (same) return preset.name;
    }
    return QString();
  }

  QVector3D blend(const QVector3D& closed, double t, Rotation::Order order)
  {
    t = std::clamp(t, 0.0, 1.0);
    if (t <= 0.0) return QVector3D(0, 0, 0);
    if (t >= 1.0) return closed;
    const QQuaternion q = QQuaternion::slerp(QQuaternion(), turnOf(closed, order), static_cast<float>(t));
    return anglesOf(q, order, true);
  }

  Hand withBend(const Hand& hand, const Hand& closed, int finger, int percent)
  {
    Hand out = hand;
    if (finger < 0 || finger >= fingerCount) return out;
    percent = std::clamp(percent, 0, 100);
    for (int part = 0; part < partCount; ++part)
    {
      const int row = finger * partCount + part;
      out[row] = blend(closed[row], percent / 100.0);
    }
    return out;
  }

  int bend(const Hand& hand, const Hand& closed, int finger)
  {
    if (finger < 0 || finger >= fingerCount) return 0;
    QQuaternion now[partCount];
    QQuaternion full[partCount];
    for (int part = 0; part < partCount; ++part)
    {
      now[part] = turnOf(hand[finger * partCount + part], fingerOrder);
      full[part] = turnOf(closed[finger * partCount + part], fingerOrder);
    }
    int best = 0;
    double bestError = std::numeric_limits<double>::max();
    for (int percent = 0; percent <= 100; ++percent)
    {
      double error = 0.0;
      for (int part = 0; part < partCount; ++part)
        error += angleBetween(now[part], QQuaternion::slerp(QQuaternion(), full[part], percent / 100.0f));
      if (error < bestError - 1.0e-9)
      {
        bestError = error;
        best = percent;
      }
    }
    return best;
  }

  Writes presetWrites(const Preset& preset, bool left, bool right)
  {
    Writes writes;
    for (Side side : {Side::Left, Side::Right})
    {
      if ((side == Side::Left && !left) || (side == Side::Right && !right)) continue;
      for (int i = 0; i < jointCount; ++i) writes << Write{jointName(side, i), preset.hand(side)[i]};
    }
    return writes;
  }

  Writes handWrites(Side side, const Hand& hand, const QList<int>& rows, bool mirror)
  {
    Writes writes;
    for (int row : rows)
      if (row >= 0 && row < jointCount) writes << Write{jointName(side, row), hand[row]};
    if (mirror)
      for (int row : rows)
        if (row >= 0 && row < jointCount) writes << Write{jointName(other(side), row), mirrored(hand[row])};
    return writes;
  }

  Writes resetWrites(bool left, bool right)
  {
    Preset open;
    open.left = openHand();
    open.right = openHand();
    return presetWrites(open, left, right);
  }

  BVHNode* node(Animation* anim, const QString& name)
  {
    if (!anim) return nullptr;
    QList<BVHNode*> todo{anim->getMotion()};
    while (!todo.isEmpty())
    {
      BVHNode* n = todo.takeFirst();
      if (!n) continue;
      if (n->name() == name) return n;
      for (int i = 0; i < n->numChildren(); ++i) todo << n->child(i);
    }
    return nullptr;
  }

  int apply(Animation* anim, const Writes& writes)
  {
    if (!anim) return 0;
    const bool mirrored = anim->getMirrored();
    anim->setMirrored(false);
    int done = 0;
    for (const Write& write : writes)
    {
      BVHNode* joint = node(anim, write.joint);
      if (!joint) continue;
      anim->setRotation(joint, write.angles.x(), write.angles.y(), write.angles.z());
      ++done;
    }
    anim->setMirrored(mirrored);
    return done;
  }

  Hand read(Animation* anim, Side side)
  {
    Hand hand = openHand();
    for (int i = 0; i < jointCount; ++i)
      if (BVHNode* joint = node(anim, jointName(side, i))) hand[i] = anim->getRotation(joint);
    return hand;
  }
}
