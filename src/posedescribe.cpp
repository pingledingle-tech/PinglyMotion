/*
 * PinglyMotion - describe_pose: the pose as computed text (issue #129)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QMatrix4x4>
#include <QStringList>
#include <QtMath>

#include "animation.h"
#include "animexport.h"
#include "bvhnode.h"

#include "posedescribe.h"

namespace
{
  // The floor is y = 0 in the program's own units (inches), the same space as
  // set_position/get_pose: the start pose stands with the hip at 43.5 and the
  // legs straight down to the floor (docs/ASSISTANT_GUIDE.md section 1).
  const double FLOOR_Y = 0.0;

  // -------------------------------------------------------------- the tree

  void findAll(BVHNode* node, QList<BVHNode*>& list)
  {
    if (!node) return;
    if (node->type != BVH_END) list << node;
    for (int i = 0; i < node->numChildren(); i++) findAll(node->child(i), list);
  }

  BVHNode* findNode(BVHNode* root, const QString& name)
  {
    QList<BVHNode*> all;
    findAll(root, all);
    for (BVHNode* node : all)
      if (node->name() == name) return node;
    return nullptr;
  }

  // world matrix of every joint at "frame" (0-based): the joint's offset,
  // then each rotation channel in file order, exactly as drawPart() in
  // animationview.cpp draws the figure (the mesh-only scale and shift of
  // AvatarSkin are left out: they fit the visible body to the skeleton and
  // are not part of the skeleton itself). The root (hip) has offset (0,0,0)
  // in the shipped skeletons; the hip's own world position is the animation's
  // position track (Animation::getNode(0)), added once, here.
  void place(BVHNode* node, const QMatrix4x4& parent, int frame, QHash<BVHNode*, QMatrix4x4>& out)
  {
    QMatrix4x4 m = parent;
    m.translate(node->offset[0], node->offset[1], node->offset[2]);
    const QVector3D rotation = node->frameData(frame).rotation();
    for (int i = 0; i < node->numChannels; i++)
    {
      switch (node->channelType[i])
      {
        case BVH_XROT: m.rotate(rotation.x(), 1, 0, 0); break;
        case BVH_YROT: m.rotate(rotation.y(), 0, 1, 0); break;
        case BVH_ZROT: m.rotate(rotation.z(), 0, 0, 1); break;
        default: break;
      }
    }
    out[node] = m;
    for (int i = 0; i < node->numChildren(); i++) place(node->child(i), m, frame, out);
  }

  QHash<BVHNode*, QMatrix4x4> worldTransforms(Animation* animation, int frame)
  {
    QHash<BVHNode*, QMatrix4x4> out;
    QMatrix4x4 base;
    base.translate(animation->getNode(0)->frameData(frame).position());
    place(animation->getMotion(), base, frame, out);
    return out;
  }

  QVector3D worldPosition(const QHash<BVHNode*, QMatrix4x4>& world, BVHNode* node)
  {
    return world.value(node).map(QVector3D());
  }

  // -------------------------------------------------------------- friendly names

  QString friendlyName(const QString& joint)
  {
    static const QHash<QString, QString> names = {
      {"hip", "hip"}, {"abdomen", "waist"}, {"chest", "chest"}, {"neck", "neck"}, {"head", "head"},
      {"lCollar", "left collar bone"}, {"rCollar", "right collar bone"},
      {"lShldr", "left shoulder"}, {"rShldr", "right shoulder"},
      {"lForeArm", "left elbow"}, {"rForeArm", "right elbow"},
      {"lHand", "left wrist"}, {"rHand", "right wrist"},
      {"lThigh", "left hip"}, {"rThigh", "right hip"},
      {"lShin", "left knee"}, {"rShin", "right knee"},
      {"lFoot", "left ankle"}, {"rFoot", "right ankle"},
    };
    return names.value(joint, joint);
  }

  // the classic joints covered by describe_pose (not the Bento fingers, face,
  // wings, tail or groin: see the file comment in posedescribe.h)
  const QStringList CLASSIC_JOINTS = {
    "hip", "abdomen", "chest", "neck", "head",
    "lCollar", "rCollar", "lShldr", "rShldr", "lForeArm", "rForeArm", "lHand", "rHand",
    "lThigh", "rThigh", "lShin", "rShin", "lFoot", "rFoot",
  };

  // joints whose everyday action word is "bends", not "turns"
  bool bendsRatherThanTurns(const QString& joint)
  {
    return joint == "lForeArm" || joint == "rForeArm" || joint == "lShin" || joint == "rShin";
  }

  // -------------------------------------------------------------- JSON helpers

  QJsonObject positionJson(const QVector3D& inches)
  {
    auto round2 = [](double v) { return qRound(v * 100.0) / 100.0; };
    auto round3 = [](double v) { return qRound(v * 1000.0) / 1000.0; };
    const QVector3D metres = inches * AnimExport::INCHES_TO_METERS;
    QJsonObject out;
    out["x"] = round2(inches.x());
    out["y"] = round2(inches.y());
    out["z"] = round2(inches.z());
    out["meters"] = QJsonObject{{"x", round3(metres.x())}, {"y", round3(metres.y())}, {"z", round3(metres.z())}};
    return out;
  }

  QString capitalize(QString text)
  {
    if (!text.isEmpty()) text[0] = text[0].toUpper();
    return text;
  }

  // -------------------------------------------------------------- rule sentences

  // Three-way classification with a dead zone between "low" and "high", so a
  // tiny change near the old single threshold never flips the word: nothing
  // is said while the value sits between the two.
  enum class Band { Low, Dead, High };
  Band classify(double value, double low, double high)
  {
    if (value <= low) return Band::Low;
    if (value >= high) return Band::High;
    return Band::Dead;
  }

  double elbowBend(const QString& side, const QVector3D& rotation)
  {
    // left: -146 to 0 (more negative is more bent); right: 0 to 146
    return side == "l" ? -rotation.y() : rotation.y();
  }

  double kneeBend(const QVector3D& rotation)
  {
    // both knees: 0 to 150, positive is bent
    return rotation.x();
  }

  struct Rules
  {
    QStringList sentences;
    QHash<QString, double> footAboveFloor;  // "l"/"r" -> inches
  };

  Rules buildRules(const QHash<BVHNode*, QMatrix4x4>& world, const QHash<QString, BVHNode*>& joint,
                    const QHash<QString, QVector3D>& rotation)
  {
    Rules rules;

    // knees: straight at 10 degrees or less, bent at 25 or more
    QStringList bentKnees;
    for (const QString& side : {QStringLiteral("l"), QStringLiteral("r")})
    {
      const double bend = kneeBend(rotation.value(side + "Shin"));
      if (classify(bend, 10.0, 25.0) == Band::High) bentKnees << (side == "l" ? "left" : "right");
    }
    if (bentKnees.size() == 2) rules.sentences << "knees bent";
    else if (bentKnees.size() == 1) rules.sentences << bentKnees.first() + " knee bent";

    // elbows: same bands
    for (const QString& side : {QStringLiteral("l"), QStringLiteral("r")})
    {
      const double bend = elbowBend(side, rotation.value(side + "ForeArm"));
      if (classify(bend, 10.0, 25.0) == Band::High)
        rules.sentences << QString(side == "l" ? "left" : "right") + " elbow bent";
    }

    // hands versus the head and the body
    const QVector3D head = worldPosition(world, joint.value("head"));
    const QVector3D hip = worldPosition(world, joint.value("hip"));
    const QMatrix4x4 hipMatrix = world.value(joint.value("hip"));
    const QVector3D forward = hipMatrix.mapVector(QVector3D(0, 0, 1)).normalized();
    const QVector3D left = hipMatrix.mapVector(QVector3D(1, 0, 0)).normalized();

    QStringList handsAbove, handsBehind;
    for (const QString& side : {QStringLiteral("l"), QStringLiteral("r")})
    {
      const QVector3D hand = worldPosition(world, joint.value(side + "Hand"));
      const QString word = side == "l" ? "left" : "right";
      if (classify(hand.y() - head.y(), 1.5, 3.0) == Band::High) handsAbove << word;
      if (classify(-QVector3D::dotProduct(hand - hip, forward), 1.5, 3.0) == Band::High) handsBehind << word;
    }
    if (handsAbove.size() == 2) rules.sentences << "hands above head";
    else if (handsAbove.size() == 1) rules.sentences << handsAbove.first() + " hand above head";
    if (handsBehind.size() == 2) rules.sentences << "hands behind the body";
    else if (handsBehind.size() == 1) rules.sentences << handsBehind.first() + " hand behind the body";

    // feet: flat on the floor, or lifted
    QStringList liftedFeet;
    bool anyFlatKnown = false, allFlat = true;
    for (const QString& side : {QStringLiteral("l"), QStringLiteral("r")})
    {
      const QVector3D foot = worldPosition(world, joint.value(side + "Foot"));
      const double above = foot.y() - FLOOR_Y;
      rules.footAboveFloor[side] = above;
      const Band band = classify(above, 1.5, 3.0);
      if (band == Band::High) { liftedFeet << (side == "l" ? "left" : "right"); allFlat = false; }
      else if (band == Band::Dead) allFlat = false;
      else anyFlatKnown = true;
    }
    if (liftedFeet.size() == 2) rules.sentences << "both feet lifted off the floor";
    else if (liftedFeet.size() == 1) rules.sentences << liftedFeet.first() + " foot lifted off the floor";
    else if (allFlat && anyFlatKnown) rules.sentences << "feet flat on the floor";

    // weight: the hip shifted sideways towards one foot
    const QVector3D lFoot = worldPosition(world, joint.value("lFoot"));
    const QVector3D rFoot = worldPosition(world, joint.value("rFoot"));
    const QVector3D footMiddle = (lFoot + rFoot) / 2.0;
    const double sideways = QVector3D::dotProduct(hip - footMiddle, left);
    const Band weightBand = classify(qAbs(sideways), 1.5, 3.0);
    if (weightBand == Band::High) rules.sentences << QString("weight on the %1 foot").arg(sideways > 0 ? "left" : "right");

    // lean: the head forward or back of the hip, along the body's own forward axis
    const double leanAmount = QVector3D::dotProduct(head - hip, forward);
    const Band leanBand = classify(qAbs(leanAmount), 3.0, 6.0);
    if (leanBand == Band::High) rules.sentences << QString("leaning %1").arg(leanAmount > 0 ? "forward" : "back");

    // head: turned left/right, or tipped up/down, from its own rotation
    const QVector3D headRotation = rotation.value("head");
    const Band turnBand = classify(qAbs(headRotation.y()), 10.0, 20.0);
    if (turnBand == Band::High) rules.sentences << QString("head turned %1").arg(headRotation.y() > 0 ? "left" : "right");
    const Band nodBand = classify(qAbs(headRotation.x()), 10.0, 20.0);
    if (nodBand == Band::High) rules.sentences << QString("head looking %1").arg(headRotation.x() > 0 ? "down" : "up");

    return rules;
  }

  // -------------------------------------------------------------- changed joints

  QJsonArray changedJoints(const QHash<QString, QVector3D>& rotation, const QHash<QString, QVector3D>& reference)
  {
    // a joint counts as changed when its total turn (the length of its
    // rotation vector) differs from frame 1 by more than this; smaller
    // differences are the kind of noise easing and re-saving can leave behind
    const double CHANGE_THRESHOLD = 5.0;

    QJsonArray out;
    for (const QString& jointName : CLASSIC_JOINTS)
    {
      const double now = rotation.value(jointName).length();
      const double was = reference.value(jointName).length();
      if (qAbs(now - was) <= CHANGE_THRESHOLD) continue;

      QJsonObject entry;
      entry["joint"] = jointName;
      entry["friendly"] = friendlyName(jointName);
      entry["verb"] = bendsRatherThanTurns(jointName) ? "bends" : "turns";
      entry["direction"] = now > was ? "more" : "less";
      out << entry;
    }
    return out;
  }

  // -------------------------------------------------------------- symmetry

  QJsonArray symmetryNotes(const QHash<QString, QVector3D>& rotation)
  {
    // BVHNode::mirror()'s own rule: a mirrored rotation negates Y and Z and
    // keeps X. A symmetric pose has the right side equal to the mirrored left.
    struct Region { QString name; QStringList joints; };
    static const QList<Region> regions = {
      {"arms", {"lCollar", "lShldr", "lForeArm", "lHand"}},
      {"legs", {"lThigh", "lShin", "lFoot"}},
    };
    const double SYMMETRIC_UP_TO = 10.0;
    const double ASYMMETRIC_FROM = 20.0;

    QJsonArray out;
    for (const Region& region : regions)
    {
      QString worstLeft, worstRight;
      double worstDiff = -1, worstLeftLen = 0, worstRightLen = 0;
      for (const QString& leftJoint : region.joints)
      {
        const QString rightJoint = "r" + leftJoint.mid(1);
        const QVector3D left = rotation.value(leftJoint);
        const QVector3D right = rotation.value(rightJoint);
        const QVector3D mirroredLeft(left.x(), -left.y(), -left.z());
        const double diff = (right - mirroredLeft).length();
        if (diff > worstDiff)
        {
          worstDiff = diff;
          worstLeft = leftJoint;
          worstRight = rightJoint;
          worstLeftLen = left.length();
          worstRightLen = right.length();
        }
      }
      if (worstDiff >= ASYMMETRIC_FROM)
      {
        const QString moreBentSide = worstLeftLen >= worstRightLen ? friendlyName(worstLeft) : friendlyName(worstRight);
        out << QString("%1 are not symmetric: the %2 moved more").arg(region.name, moreBentSide);
      }
      else if (worstDiff <= SYMMETRIC_UP_TO)
      {
        out << QString("%1 are symmetric").arg(region.name);
      }
      // in the dead zone (10 to 20): no note, so a tiny change near the old
      // single threshold cannot flip "symmetric" and "not symmetric"
    }
    return out;
  }
}

QJsonObject PoseDescribe::describe(Animation* animation, int frame)
{
  BVHNode* root = animation->getMotion();

  QHash<QString, BVHNode*> joint;
  for (const QString& name : CLASSIC_JOINTS) joint[name] = findNode(root, name);

  const QHash<BVHNode*, QMatrix4x4> world = worldTransforms(animation, frame);

  QHash<QString, QVector3D> rotation, referenceRotation;
  for (const QString& name : CLASSIC_JOINTS)
  {
    rotation[name] = joint.value(name)->frameData(frame).rotation();
    referenceRotation[name] = joint.value(name)->frameData(0).rotation();
  }

  QJsonObject positions;
  positions["head"] = positionJson(worldPosition(world, joint.value("head")));
  positions["lHand"] = positionJson(worldPosition(world, joint.value("lHand")));
  positions["rHand"] = positionJson(worldPosition(world, joint.value("rHand")));
  positions["lFoot"] = positionJson(worldPosition(world, joint.value("lFoot")));
  positions["rFoot"] = positionJson(worldPosition(world, joint.value("rFoot")));
  positions["hip"] = positionJson(worldPosition(world, joint.value("hip")));

  const Rules rules = buildRules(world, joint, rotation);
  for (auto it = rules.footAboveFloor.constBegin(); it != rules.footAboveFloor.constEnd(); ++it)
  {
    QJsonObject foot = positions.value(it.key() == "l" ? "lFoot" : "rFoot").toObject();
    const double metres = it.value() * AnimExport::INCHES_TO_METERS;
    foot["above_floor"] = qRound(it.value() * 100.0) / 100.0;
    foot["above_floor_meters"] = qRound(metres * 1000.0) / 1000.0;
    positions[it.key() == "l" ? "lFoot" : "rFoot"] = foot;
  }

  QJsonArray sentences;
  for (const QString& sentence : rules.sentences) sentences << sentence;

  QJsonObject answer;
  answer["ok"] = true;
  answer["frame"] = frame + 1;
  answer["positions"] = positions;
  answer["sentences"] = sentences;
  answer["changed_joints"] = changedJoints(rotation, referenceRotation);
  answer["symmetry"] = symmetryNotes(rotation);
  answer["summary"] = rules.sentences.isEmpty()
                         ? QString("A neutral pose: nothing bent, raised or shifted enough to describe.")
                         : capitalize(rules.sentences.join("; ")) + ".";
  return answer;
}
