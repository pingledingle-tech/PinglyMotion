/*
 * PinglyMotion - describe_body: the body as text (issue #129)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QHash>
#include <QJsonArray>
#include <QMatrix4x4>

#include "animation.h"
#include "bvhnode.h"
#include "skeletonwalk.h"

#include "bodydescribe.h"

namespace
{
  const double METRES_PER_INCH = 0.0254;

  double round2(double value) { return std::round(value * 100.0) / 100.0; }
  double round3(double value) { return std::round(value * 1000.0) / 1000.0; }

  struct Skeleton
  {
    QHash<QString, QVector3D> joint;   // start-pose world position of each joint
    QHash<QString, QVector3D> tip;     // of the End Site after that joint, when it has one
    double floor = 0;

    bool has(const QString& name) const { return joint.contains(name); }
    QVector3D at(const QString& name) const { return joint.value(name); }
    // the End Site after "name", or the joint itself without one
    QVector3D tipOf(const QString& name) const { return tip.value(name, joint.value(name)); }
  };

  Skeleton startPose(Animation* animation)
  {
    Skeleton skeleton;
    QHash<BVHNode*, QMatrix4x4> world;
    SkeletonWalk::place(animation->getMotion(), QMatrix4x4(), [](BVHNode*) { return QVector3D(); }, world);
    QList<BVHNode*> nodes;
    SkeletonWalk::collect(animation->getMotion(), nodes, true);
    double lowest = 0;
    bool first = true;
    for (BVHNode* node : nodes)
    {
      const QVector3D position = world.value(node).map(QVector3D());
      if (node->type == BVH_END) continue;
      if (!skeleton.joint.contains(node->name())) skeleton.joint.insert(node->name(), position);
      for (int i = 0; i < node->numChildren(); i++)
        if (node->child(i)->type == BVH_END)
        {
          const QVector3D end = world.value(node->child(i)).map(QVector3D());
          skeleton.tip.insert(node->name(), end);
          break;
        }
    }
    // the floor: the lowest point of the feet (ankles, feet, toes and their tips)
    for (const char* name : {"lFoot", "rFoot", "mFootLeft", "mFootRight", "mToeLeft", "mToeRight"})
    {
      if (!skeleton.has(name)) continue;
      for (const QVector3D& point : {skeleton.at(name), skeleton.tipOf(name)})
      {
        if (first || point.y() < lowest) lowest = point.y();
        first = false;
      }
    }
    skeleton.floor = lowest;
    return skeleton;
  }

  QJsonObject size(const QString& what, double inches)
  {
    return QJsonObject{{"what", what}, {"inches", round2(inches)}, {"metres", round3(inches * METRES_PER_INCH)}};
  }

  QString metres(double inches)
  {
    return QString::number(inches * METRES_PER_INCH, 'f', 2) + " m";
  }

  QString percent(double ratio)
  {
    return QString::number(qRound(ratio * 100.0)) + " %";
  }
}

QJsonObject BodyDescribe::describe(Animation* animation, bool male, const QString& body)
{
  const Skeleton s = startPose(animation);
  const double floor = s.floor;
  auto above = [&](const QVector3D& point) { return double(point.y()) - floor; };
  auto distance = [](const QVector3D& a, const QVector3D& b) { return double((a - b).length()); };

  // the top of the head: the skull's tip (Bento), else the head's End Site
  const QVector3D top = s.has("mSkull") ? s.tipOf("mSkull") : s.tipOf("head");
  // a fingertip: the End Site after the middle finger's third joint, else the wrist's End Site
  const QVector3D leftFingers = s.has("mHandMiddle3Left") ? s.tipOf("mHandMiddle3Left") : s.tipOf("lHand");
  const QVector3D rightFingers = s.has("mHandMiddle3Right") ? s.tipOf("mHandMiddle3Right") : s.tipOf("rHand");
  const QVector3D toes = s.has("mToeLeft") ? s.tipOf("mToeLeft") : s.tipOf("lFoot");

  const double height = above(top);
  const double eyes = s.has("mEyeLeft") ? above(s.at("mEyeLeft")) : above(s.at("head"));
  const double shoulderHeight = above(s.at("lShldr"));
  const double hipHeight = above(s.at("lThigh"));
  const double kneeHeight = above(s.at("lShin"));
  const double ankleHeight = above(s.at("lFoot"));
  const double shoulders = distance(s.at("lShldr"), s.at("rShldr"));
  const double hips = distance(s.at("lThigh"), s.at("rThigh"));
  const double upperArm = distance(s.at("lShldr"), s.at("lForeArm"));
  const double forearm = distance(s.at("lForeArm"), s.at("lHand"));
  const double hand = distance(s.at("lHand"), leftFingers);
  const double arm = upperArm + forearm + hand;
  const double span = distance(leftFingers, rightFingers);
  const double thigh = distance(s.at("lThigh"), s.at("lShin"));
  const double shin = distance(s.at("lShin"), s.at("lFoot"));
  const QVector3D ankle = s.at("lFoot");
  const double foot = std::hypot(double(toes.x() - ankle.x()), double(toes.z() - ankle.z()));
  const double torso = distance(s.at("hip"), s.at("neck"));
  const double neckToTop = above(top) - above(s.at("neck"));

  QJsonArray sizes;
  sizes << size("height: floor to the top of the skull", height)
        << size("eye height above the floor", eyes)
        << size("shoulder height above the floor", shoulderHeight)
        << size("hip joint height above the floor (where the legs start)", hipHeight)
        << size("knee height above the floor", kneeHeight)
        << size("ankle height above the floor", ankleHeight)
        << size("shoulder width: shoulder joint to shoulder joint", shoulders)
        << size("hip width: hip joint to hip joint", hips)
        << size("upper arm: shoulder to elbow", upperArm)
        << size("forearm: elbow to wrist", forearm)
        << size("hand: wrist to the tip of the middle finger", hand)
        << size("whole arm: shoulder to the tip of the middle finger", arm)
        << size("arm span: fingertip to fingertip with the arms out", span)
        << size("thigh: hip joint to knee", thigh)
        << size("shin: knee to ankle", shin)
        << size("foot: ankle to the tip of the toes, along the floor", foot)
        << size("body: hip to the base of the neck", torso)
        << size("neck and head: base of the neck to the top of the skull", neckToTop);

  QJsonObject names;
  const QStringList keys = {"height", "eyes", "shoulder_height", "hip_height", "knee_height", "ankle_height",
                            "shoulder_width", "hip_width", "upper_arm", "forearm", "hand", "arm", "arm_span",
                            "thigh", "shin", "foot", "torso", "neck_and_head"};
  for (int i = 0; i < keys.size(); i++)
  {
    QJsonObject entry = sizes.at(i).toObject();
    names[keys.at(i)] = entry;
  }

  const double legRatio = hipHeight / height;
  const double spanRatio = span / height;
  QJsonObject proportions;
  proportions["legs_of_height"] = round2(legRatio);
  proportions["arm_span_of_height"] = round2(spanRatio);
  proportions["shoulders_of_height"] = round2(shoulders / height);
  proportions["thigh_to_shin"] = round2(thigh / shin);
  proportions["upper_arm_to_forearm"] = round2(upperArm / forearm);

  const QString bodyName = body == QLatin1String("max") ? QStringLiteral("Max") : QStringLiteral("Ruth2");
  const float scale = animation->getAvatarScale();

  QJsonArray sentences;
  sentences << QString("The 3D view shows the %1 figure with the %2 body. Both figures and both bodies use the same Second Life skeleton, so every size here is the same for all of them.")
                 .arg(male ? "male" : "female", bodyName);
  sentences << QString("The figure is %1 tall (%2 inches) from the floor to the top of the skull, standing straight in the start pose; the eyes are at %3.")
                 .arg(metres(height)).arg(round2(height)).arg(metres(eyes));
  sentences << QString("The legs start at %1 above the floor, %2 of the height; the knees are at %3.")
                 .arg(metres(hipHeight), percent(legRatio), metres(kneeHeight));
  QString spanWords = spanRatio > 1.03 ? "wider than the figure is tall" : spanRatio < 0.97 ? "less than the height" : "about the same as the height";
  sentences << QString("With the arms out the span is %1 fingertip to fingertip, %2. One arm is %3 from shoulder to fingertip.")
                 .arg(metres(span), spanWords, metres(arm));
  sentences << QString("The shoulders are %1 apart and the hips %2, measured between the joint centres.")
                 .arg(metres(shoulders), metres(hips));
  if (std::fabs(scale - 1.0f) > 0.001f)
    sentences << QString("The 3D view draws the figure at %1 of this size (Scale); the sizes here are those of the skeleton, which the animation file uses.")
                   .arg(percent(scale));
  sentences << QString("Sizes are between joint centres; the skin adds a little. All sizes are in inches, the program's unit, and in metres.");

  QJsonObject answer;
  answer["ok"] = true;
  answer["figure"] = male ? "male" : "female";
  answer["body"] = body == QLatin1String("max") ? "max" : "ruth2";
  answer["body_name"] = bodyName;
  answer["skeleton"] = "Second Life (Bento), the same for both figures and both bodies";
  answer["scale"] = round2(scale);
  answer["units"] = "inches, with metres next to each size";
  answer["floor"] = "the lowest point of the feet in the start pose";
  answer["sizes"] = names;
  answer["proportions"] = proportions;
  answer["sentences"] = sentences;
  answer["summary"] = QString("%1 figure, %2 body: %3 tall, legs %4 of the height, arm span %5.")
                        .arg(male ? "Male" : "Female", bodyName, metres(height), percent(legRatio), metres(span));
  return answer;
}
