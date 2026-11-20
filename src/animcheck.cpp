/*
 * PinglyMotion - check_animation: exact checks for Second Life (issue #129)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <algorithm>
#include <cmath>
#include <vector>

#include <QJsonArray>
#include <QQuaternion>
#include <QStringList>
#include <QtMath>

#include "animation.h"
#include "animexport.h"
#include "bvhnode.h"
#include "motiondescribe.h"
#include "posedescribe.h"

#include "animcheck.h"

namespace
{
  // ------------------------------------------------------------ thresholds
  // docs/CONTROL.md lists the same numbers.

  // AnimExport: a joint whose every key is within this angle of frame 1 (radians,
  // as the export code compares) never moves and is left out
  const double STATIC_RADIANS = 0.001;
  // a joint is "posed" in frame 1 when it is turned more than this from the start pose (degrees)
  const double POSED_DEGREES = 5.0;
  // a key may pass the joint's limit by this much before it counts (degrees)
  const double LIMIT_TOLERANCE = 0.5;
  // the ankle may sink this far below the floor Second Life stands it on (inches)
  const double UNDER_FLOOR_TOLERANCE = 2.0;
  // both ankles at least this far above that floor ...
  const double FLOATING_HEIGHT = 3.0;
  // ... for at least this long (seconds) is floating; a jump is shorter
  const double FLOATING_SECONDS = 1.0;
  // a foot is down within CONTACT_HEIGHT of the lowest ankle height, up again above
  // LEAVE_HEIGHT (inches), as describe_motion decides contact
  const double CONTACT_HEIGHT = 2.0;
  const double LEAVE_HEIGHT = 3.0;
  // a foot that is down slides when it moves along the floor faster than this
  // (inches per second, 0.5 m/s) for at least SLIDE_STEPS frame steps in a row
  const double SLIDE_SPEED = 20.0;
  const int SLIDE_STEPS = 3;
  // a jerk: one frame step of at least this many degrees (or inches for the hip)
  // that is also at least JERK_RATIO times both neighbouring steps
  const double JERK_DEGREES = 15.0;
  const double JERK_INCHES = 6.0;
  const double JERK_RATIO = 3.0;
  // nothing moves at the start for at least this long (seconds) ...
  const double DEAD_START_SECONDS = 0.5;
  // ... where moving is a joint faster than this (degrees per second) or the hip
  // faster than DEAD_START_HIP_SPEED (inches per second), as describe_motion holds
  const double DEAD_START_SPEED = 10.0;
  const double DEAD_START_HIP_SPEED = 2.0;
  // keys that differ by less than this (degrees, inches) are the same
  const double SAME_KEY = 0.01;
  // lists in a finding are cut after this many entries
  const int MAX_LIST = 8;

  const double HIP_LIMIT_INCHES = AnimExport::MAX_POSITION / AnimExport::INCHES_TO_METERS;

  // ------------------------------------------------------------ helpers

  double rounded(double v, double scale)
  {
    if (!std::isfinite(v)) return 0.0;
    v = qBound(-1e12, v, 1e12);
    return std::round(v * scale) / scale;
  }
  double round1(double v) { return rounded(v, 10.0); }
  double round2(double v) { return rounded(v, 100.0); }
  QString number(double v, int decimals = 1) { return QString::number(rounded(v, std::pow(10.0, decimals)), 'f', decimals); }

  double angleBetween(const QQuaternion& a, const QQuaternion& b)
  {
    const double dot = qBound(0.0, double(qAbs(QQuaternion::dotProduct(a, b))), 1.0);
    return qRadiansToDegrees(2.0 * std::acos(dot));
  }

  QQuaternion orientation(BVHNode* node, const QVector3D& r)
  {
    QQuaternion q;
    for (int i = 0; i < node->numChannels; i++)
    {
      switch (node->channelType[i])
      {
        case BVH_XROT: q = q * QQuaternion::fromAxisAndAngle(1, 0, 0, r.x()); break;
        case BVH_YROT: q = q * QQuaternion::fromAxisAndAngle(0, 1, 0, r.y()); break;
        case BVH_ZROT: q = q * QQuaternion::fromAxisAndAngle(0, 0, 1, r.z()); break;
        default: break;
      }
    }
    return q.normalized();
  }

  void allJoints(BVHNode* node, QList<BVHNode*>& out)
  {
    if (!node) return;
    if (node->type != BVH_END) out << node;
    for (int i = 0; i < node->numChildren(); i++) allJoints(node->child(i), out);
  }

  QJsonObject span(int from, int to) { return QJsonObject{{"from", from}, {"to", to}}; }

  // a finding, collected in order of severity
  struct Findings
  {
    QJsonArray errors, warnings, notes;
    QStringList passed;
    QStringList errorWords, warningWords, noteWords;

    void add(const QString& severity, const QString& check, const QString& message, const QString& word,
             const QJsonObject& extra = QJsonObject())
    {
      QJsonObject finding = extra;
      finding["check"] = check;
      finding["severity"] = severity;
      finding["message"] = message;
      if (severity == "error") { errors << finding; errorWords << word; }
      else if (severity == "warning") { warnings << finding; warningWords << word; }
      else { notes << finding; noteWords << word; }
    }
  };

  // runs of true flags, as frame spans (1-based), at most MAX_LIST, with the total count
  QJsonArray spansOf(const std::vector<bool>& flags, int firstFrame, int minLength, int* total)
  {
    QJsonArray out;
    int count = 0;
    for (int i = 0; i < int(flags.size()); i++)
    {
      if (!flags[i] || (i > 0 && flags[i - 1])) continue;
      int j = i;
      while (j + 1 < int(flags.size()) && flags[j + 1]) j++;
      if (j - i + 1 >= minLength)
      {
        if (count < MAX_LIST) out << span(firstFrame + i, firstFrame + j);
        count++;
      }
      i = j;
    }
    if (total) *total = count;
    return out;
  }
}

QJsonObject AnimCheck::check(Animation* animation)
{
  const int frames = animation->getNumberOfFrames();
  const int last = frames - 1;
  double frameTime = animation->frameTime();
  if (!std::isfinite(frameTime) || frameTime <= 0) frameTime = 1.0 / 30.0;

  for (const QString& name : {QStringLiteral("hip"), QStringLiteral("lThigh"), QStringLiteral("lShin"), QStringLiteral("lFoot"),
                              QStringLiteral("rFoot")})
    if (!PoseDescribe::findJoint(animation, name))
      return QJsonObject{{"ok", false}, {"error", QString("The skeleton has no joint %1, so the animation cannot be checked.").arg(name)}};

  Findings findings;
  QList<BVHNode*> joints;
  allJoints(animation->getMotion(), joints);
  BVHNode* positionNode = animation->getNode(0);

  // ---------------------------------------------------------- length (as AnimExport::write)
  const double duration = std::max(frames - 2, 1) * frameTime;
  if (duration > AnimExport::MAX_DURATION)
    findings.add("error", "length",
                 QString("The animation is %1 s long; Second Life takes at most %2 s. Remove frames (set_frame_count) or raise the frames per second.")
                   .arg(number(duration)).arg(AnimExport::MAX_DURATION, 0, 'f', 0),
                 QString("too long (%1 s)").arg(number(duration)), QJsonObject{{"seconds", round2(duration)}});
  else findings.passed << "length";

  // ---------------------------------------------------------- which joints move away from frame 1
  // keys decide: between two keys that equal frame 1 the value equals it too
  QHash<BVHNode*, bool> moves;
  bool anyMoves = false;
  for (BVHNode* node : joints)
  {
    const QQuaternion q0 = orientation(node, node->frameData(0).rotation());
    bool moved = false;
    for (int key : node->keyframeList())
      if (key > 0 && key <= last && qDegreesToRadians(angleBetween(q0, orientation(node, node->frameData(key).rotation()))) > STATIC_RADIANS)
      { moved = true; break; }
    moves[node] = moved;
    anyMoves = anyMoves || moved;
  }
  bool hipMoves = false;
  {
    const QVector3D p0 = positionNode->frameData(0).position();
    for (int key : positionNode->keyframeList())
      if (key > 0 && key <= last && (positionNode->frameData(key).position() - p0).lengthSquared() > 0.001) { hipMoves = true; break; }
  }
  if (!anyMoves && !hipMoves)
    findings.add("error", "nothing_moves",
                 "No joint moves away from frame 1, so Second Life gets an empty animation. Pose frame 2 or later differently from frame 1.",
                 "nothing moves");
  else findings.passed << "nothing_moves";

  // ---------------------------------------------------------- frame 1 posed, dropped joints
  QJsonArray heldPosed, dropped;
  int droppedOther = 0;
  for (BVHNode* node : joints)
  {
    if (moves.value(node)) continue;
    const bool classic = PoseDescribe::classicJoints().contains(node->name());
    if (!classic) { droppedOther++; continue; }
    dropped << node->name();
    if (node->name() != "hip" && angleBetween(QQuaternion(), orientation(node, node->frameData(0).rotation())) > POSED_DEGREES)
      heldPosed << node->name();
  }
  if (!heldPosed.isEmpty())
  {
    QStringList names;
    for (const QJsonValue& v : heldPosed) names << PoseDescribe::friendlyName(v.toString());
    findings.add("warning", "frame1_posed",
                 QString("Frame 1 is posed, and %1 keep%2 the frame-1 angle all the time, so Second Life drops %3 and %4 pose is lost. "
                         "Put frame 1 back to the start pose (all angles 0) or give %3 a key that differs from frame 1.")
                   .arg(names.join(", ")).arg(names.size() == 1 ? "s" : "").arg(names.size() == 1 ? "it" : "them")
                   .arg(names.size() == 1 ? "its" : "their"),
                 QString("frame 1 posed (%1)").arg(names.join(", ")), QJsonObject{{"joints", heldPosed}});
  }
  else findings.passed << "frame1_posed";
  if (anyMoves || hipMoves)
  {
    if (!dropped.isEmpty())
    {
      QJsonObject extra{{"joints", dropped}};
      if (droppedOther) extra["other_joints"] = droppedOther;
      findings.add("note", "dropped_joints",
                   QString("%1 classic joint%2 never move%3 from frame 1 and %4 left out on export, so other animations or the default pose control %5: %6.")
                     .arg(dropped.size()).arg(dropped.size() == 1 ? "" : "s").arg(dropped.size() == 1 ? "s" : "")
                     .arg(dropped.size() == 1 ? "is" : "are").arg(dropped.size() == 1 ? "it" : "them")
                     .arg([&] { QStringList n; for (const QJsonValue& v : dropped) n << v.toString(); return n.join(", "); }()),
                   QString("%1 joint%2 left out").arg(dropped.size()).arg(dropped.size() == 1 ? "" : "s"), extra);
    }
    else findings.passed << "dropped_joints";
  }

  // ---------------------------------------------------------- joint limits (keys; between keys the
  // value stays between them, and the overshoot is already cut at the limit)
  {
    QJsonArray past;
    QStringList jointsPast;
    int count = 0;
    for (BVHNode* node : joints)
    {
      if (node->type == BVH_POS) continue;
      for (int key : node->keyframeList())
      {
        if (key > last) continue;
        const QVector3D r = node->frameData(key).rotation();
        for (int i = 0; i < node->numChannels; i++)
        {
          int axis = -1;
          if (node->channelType[i] == BVH_XROT) axis = 0;
          else if (node->channelType[i] == BVH_YROT) axis = 1;
          else if (node->channelType[i] == BVH_ZROT) axis = 2;
          if (axis < 0) continue;
          const double low = node->channelMin[i], high = node->channelMax[i];
          if (!(low < high)) continue;
          const double v = r[axis];
          if (v < low - LIMIT_TOLERANCE || v > high + LIMIT_TOLERANCE)
          {
            if (!jointsPast.contains(node->name())) jointsPast << node->name();
            if (count < MAX_LIST)
              past << QJsonObject{{"joint", node->name()}, {"frame", key + 1}, {"axis", QString(QChar('X' + axis))},
                                  {"degrees", round1(v)}, {"limit", round1(v < low ? low : high)}};
            count++;
          }
        }
      }
    }
    if (count)
    {
      QStringList names;
      for (const QString& name : jointsPast.mid(0, MAX_LIST)) names << PoseDescribe::friendlyName(name);
      if (jointsPast.size() > MAX_LIST) names << QString("%1 more joints").arg(jointsPast.size() - MAX_LIST);
      const QJsonObject first = past.first().toObject();
      findings.add("warning", "joint_limits",
                   QString("%1 key value%2 past the joint's natural limit, in %3 (first %4 %5 at frame %6: %7 degrees, limit %8). "
                           "The figure bends in a way a body cannot; motion capture often does this a little.")
                     .arg(count).arg(count == 1 ? " is" : "s are").arg(names.join(", ")).arg(first["joint"].toString(), first["axis"].toString())
                     .arg(first["frame"].toInt()).arg(number(first["degrees"].toDouble())).arg(number(first["limit"].toDouble())),
                   QString("%1 key%2 past joint limits").arg(count).arg(count == 1 ? "" : "s"), QJsonObject{{"keys", past}, {"count", count}});
    }
    else findings.passed << "joint_limits";
  }

  // ---------------------------------------------------------- the motion, frame by frame
  if (frames < 2)
  {
    findings.passed << "hip_offset" << "foot_under_floor" << "floating" << "foot_sliding" << "jerk" << "dead_start" << "loop_seam";
  }
  else
  {
    const MotionDescribe::Samples s = MotionDescribe::sample(animation, 0, last);
    const int played = frames > 2 ? 1 : 0;   // Second Life does not play frame 1

    // hip offset from frame 1, per axis, as the .anim format stores it
    {
      double worst = 0;
      int worstFrame = 0;
      for (int f = 0; f <= last; f++)
      {
        const QVector3D d = s.position["hip"][f] - s.position["hip"][0];
        const double m = qMax(qAbs(d.x()), qMax(qAbs(d.y()), qAbs(d.z())));
        if (m > worst) { worst = m; worstFrame = f; }
      }
      if (worst > HIP_LIMIT_INCHES)
        findings.add("error", "hip_offset",
                     QString("The hip moves %1 inches (%2 m) from frame 1 at frame %3; Second Life takes at most 5 m on each axis and cuts the rest.")
                       .arg(number(worst, 0)).arg(number(worst * AnimExport::INCHES_TO_METERS)).arg(worstFrame + 1),
                     "hip moves over 5 m", QJsonObject{{"frame", worstFrame + 1}, {"inches", round1(worst)}});
      else findings.passed << "hip_offset";
    }

    // the floor Second Life stands the figure on: frame 1's hip at standing
    // height, the legs straight down (the start pose)
    BVHNode* thigh = PoseDescribe::findJoint(animation, "lThigh");
    BVHNode* shin = PoseDescribe::findJoint(animation, "lShin");
    BVHNode* foot = PoseDescribe::findJoint(animation, "lFoot");
    const double legDrop = -(thigh->offset[1] + shin->offset[1] + foot->offset[1]);
    const double floorAnkle = s.position["hip"][0].y() - legDrop;

    {
      std::vector<bool> under;
      double deepest = 0;
      for (int f = played; f <= last; f++)
      {
        const double low = qMin(s.position["lFoot"][f].y(), s.position["rFoot"][f].y());
        under.push_back(low < floorAnkle - UNDER_FLOOR_TOLERANCE);
        deepest = qMax(deepest, floorAnkle - low);
      }
      int total = 0;
      const QJsonArray spans = spansOf(under, played + 1, 1, &total);
      if (total)
        findings.add("warning", "foot_under_floor",
                     QString("A foot goes up to %1 inches under the floor (frames %2 to %3%4). Second Life stands the hip of frame 1 at standing height, "
                             "so lower the hip less or bend the legs.")
                       .arg(number(deepest)).arg(spans.first().toObject()["from"].toInt()).arg(spans.first().toObject()["to"].toInt())
                       .arg(total > 1 ? QString(" and %1 more stretches").arg(total - 1) : QString()),
                     "foot under the floor", QJsonObject{{"frames", spans}, {"inches", round1(deepest)}});
      else findings.passed << "foot_under_floor";
    }

    {
      std::vector<bool> air;
      for (int f = played; f <= last; f++)
        air.push_back(qMin(s.position["lFoot"][f].y(), s.position["rFoot"][f].y()) > floorAnkle + FLOATING_HEIGHT);
      const int minSteps = int(std::ceil(FLOATING_SECONDS / frameTime)) + 1;
      int total = 0;
      const QJsonArray spans = spansOf(air, played + 1, minSteps, &total);
      if (total)
        findings.add("warning", "floating",
                     QString("Both feet are off the floor for %1 s or more (frames %2 to %3). If the figure should stand, lower the hip or straighten the legs; "
                             "for sitting on something this is expected.")
                       .arg(number(FLOATING_SECONDS)).arg(spans.first().toObject()["from"].toInt()).arg(spans.first().toObject()["to"].toInt()),
                     "floating", QJsonObject{{"frames", spans}});
      else findings.passed << "floating";
    }

    // foot sliding: contact relative to the lowest ankle of the played frames
    {
      double floorY = s.position["lFoot"][played].y();
      for (int f = played; f <= last; f++)
        floorY = qMin(floorY, double(qMin(s.position["lFoot"][f].y(), s.position["rFoot"][f].y())));
      QJsonArray slides;
      QStringList where;
      int total = 0;
      for (const QString& side : {QStringLiteral("lFoot"), QStringLiteral("rFoot")})
      {
        const std::vector<QVector3D>& p = s.position[side];
        std::vector<bool> down, sliding;
        bool on = false;
        for (int f = played; f <= last; f++)
        {
          const double h = p[f].y() - floorY;
          on = on ? h <= LEAVE_HEIGHT : h <= CONTACT_HEIGHT;
          down.push_back(on);
        }
        // step i: frame played+i to played+i+1, sliding when the foot is down at both ends
        for (int i = 0; i + 1 < int(down.size()); i++)
        {
          QVector3D step = p[played + i + 1] - p[played + i];
          step.setY(0);
          sliding.push_back(down[i] && down[i + 1] && step.length() / frameTime > SLIDE_SPEED);
        }
        int count = 0;
        const QJsonArray spans = spansOf(sliding, played + 1, SLIDE_STEPS, &count);
        for (const QJsonValue& v : spans)
        {
          QJsonObject o = v.toObject();
          o["to"] = o["to"].toInt() + 1;   // a step ends on the next frame
          o["foot"] = side == "lFoot" ? "left" : "right";
          if (slides.size() < MAX_LIST) slides << o;
        }
        if (count) where << QString("%1 foot %2 time%3").arg(side == "lFoot" ? "left" : "right").arg(count).arg(count == 1 ? "" : "s");
        total += count;
      }
      if (total)
        findings.add("warning", "foot_sliding",
                     QString("A foot that is down slides along the floor faster than %1 inches per second (%2). Keep a planted foot still: move the hip over it instead.")
                       .arg(number(SLIDE_SPEED, 0)).arg(where.join(", ")),
                     "foot sliding", QJsonObject{{"frames", slides}});
      else findings.passed << "foot_sliding";
    }

    // jerks: one step much bigger than both neighbours
    {
      QJsonArray jerks;
      int total = 0;
      auto look = [&](const QString& joint, const std::vector<double>& step, double minimum, const QString& unit) {
        for (int i = 1; i + 1 < int(step.size()); i++)
        {
          if (i < played) continue;
          const double around = qMax(step[i - 1], step[i + 1]);
          if (step[i] >= minimum && step[i] >= JERK_RATIO * around)
          {
            if (total < MAX_LIST)
              jerks << QJsonObject{{"joint", joint}, {"from", i + 1}, {"to", i + 2}, {unit, round1(step[i])}};
            total++;
          }
        }
      };
      for (const QString& name : s.joints)
      {
        const std::vector<QQuaternion>& q = s.orientation[name];
        std::vector<double> step;
        for (int i = 0; i + 1 < s.count; i++) step.push_back(angleBetween(q[i], q[i + 1]));
        look(name, step, JERK_DEGREES, "degrees");
      }
      {
        std::vector<double> step;
        for (int i = 0; i + 1 < s.count; i++) step.push_back((s.position["hip"][i + 1] - s.position["hip"][i]).length());
        look("hip position", step, JERK_INCHES, "inches");
      }
      if (total)
      {
        const QJsonObject first = jerks.first().toObject();
        findings.add("warning", "jerk",
                     QString("%1 sudden jump%2 in a single frame, first %3 from frame %4 to %5. Add a key in between or ease the change.")
                       .arg(total).arg(total == 1 ? "" : "s").arg(first["joint"].toString()).arg(first["from"].toInt()).arg(first["to"].toInt()),
                     QString("%1 jerk%2").arg(total).arg(total == 1 ? "" : "s"), QJsonObject{{"jumps", jerks}, {"count", total}});
      }
      else findings.passed << "jerk";
    }

    // dead start: the first frame anything moves
    {
      int firstMove = -1;
      for (int i = played; i + 1 < s.count && firstMove < 0; i++)
      {
        if ((s.position["hip"][i + 1] - s.position["hip"][i]).length() / frameTime >= DEAD_START_HIP_SPEED) firstMove = i;
        for (const QString& name : s.joints)
          if (angleBetween(s.orientation[name][i], s.orientation[name][i + 1]) / frameTime >= DEAD_START_SPEED) { firstMove = i; break; }
      }
      const double still = firstMove < 0 ? 0.0 : (firstMove - played) * frameTime;
      if (firstMove >= 0 && still >= DEAD_START_SECONDS)
        findings.add("warning", "dead_start",
                     QString("Nothing moves for the first %1 s (frames %2 to %3). Start the motion earlier or remove the frames before it.")
                       .arg(number(still, 2)).arg(played + 1).arg(firstMove + 1),
                     QString("dead start (%1 s)").arg(number(still, 2)), QJsonObject{{"frames", span(played + 1, firstMove + 1)}, {"seconds", round2(still)}});
      else findings.passed << "dead_start";
    }

    // the loop seam, as describe_motion judges it
    {
      const QJsonObject seam = MotionDescribe::loopSeam(animation);
      const QString verdict = seam["seam"].toString();
      if (verdict == "jump" || verdict == "small jump")
      {
        QStringList names;
        for (const QJsonValue& v : seam["joints"].toArray())
          names << QString("%1 %2 degrees").arg(v.toObject()["friendly"].toString()).arg(number(v.toObject()["degrees"].toDouble(), 0));
        if (seam["hip_jump"].toDouble() > 0.5) names << QString("hip %1 inches").arg(number(seam["hip_jump"].toDouble()));
        findings.add(verdict == "jump" ? "warning" : "note", "loop_seam",
                     QString("If the animation loops, it %1 from frame %2 back to frame %3 (%4). Make the last key equal to the first key of the loop.")
                       .arg(verdict == "jump" ? "jumps" : "jumps a little").arg(seam["loop_out"].toInt()).arg(seam["loop_in"].toInt()).arg(names.join(", ")),
                     QString("loop seam %1").arg(verdict), seam);
      }
      else findings.passed << "loop_seam";
    }
  }

  // ---------------------------------------------------------- keys that change nothing: a key equal
  // to the key before it and the one after it (or the last key, equal to the one before)
  {
    QJsonArray which;
    int total = 0;
    auto same = [](const FrameData& a, const FrameData& b) {
      const QVector3D r = a.rotation() - b.rotation(), p = a.position() - b.position();
      return qAbs(r.x()) < SAME_KEY && qAbs(r.y()) < SAME_KEY && qAbs(r.z()) < SAME_KEY &&
             qAbs(p.x()) < SAME_KEY && qAbs(p.y()) < SAME_KEY && qAbs(p.z()) < SAME_KEY;
    };
    QList<BVHNode*> withPosition = joints;
    withPosition.prepend(positionNode);
    for (BVHNode* node : withPosition)
    {
      QList<int> keys;
      for (int k : node->keyframeList()) if (k <= last) keys << k;
      int count = 0;
      for (int i = 1; i < keys.size(); i++)
      {
        const FrameData here = node->frameData(keys[i]);
        const bool sameBefore = same(here, node->frameData(keys[i - 1]));
        const bool sameAfter = i + 1 < keys.size() ? same(here, node->frameData(keys[i + 1])) : true;
        if (sameBefore && sameAfter) count++;
      }
      if (count)
      {
        if (which.size() < MAX_LIST) which << QJsonObject{{"joint", node == positionNode ? QString("hip position") : node->name()}, {"keys", count}};
        total += count;
      }
    }
    if (total)
      findings.add("note", "identical_keys",
                   QString("%1 key%2 change%3 nothing (equal to the keys around them); deleting them changes nothing. "
                           "Keep them only to protect a hold while you edit.")
                     .arg(total).arg(total == 1 ? "" : "s").arg(total == 1 ? "s" : ""),
                   QString("%1 identical key%2").arg(total).arg(total == 1 ? "" : "s"), QJsonObject{{"joints", which}, {"count", total}});
    else findings.passed << "identical_keys";
  }

  // ---------------------------------------------------------- the answer
  QJsonArray all;
  for (const QJsonValue& v : findings.errors) all << v;
  for (const QJsonValue& v : findings.warnings) all << v;
  for (const QJsonValue& v : findings.notes) all << v;
  QJsonArray passed;
  for (const QString& name : findings.passed) passed << name;

  QJsonObject answer;
  answer["ok"] = true;
  answer["frames"] = frames;
  answer["seconds"] = round2(duration);
  answer["errors"] = int(findings.errors.size());
  answer["warnings"] = int(findings.warnings.size());
  answer["notes"] = int(findings.notes.size());
  answer["findings"] = all;
  answer["passed"] = passed;

  QStringList parts;
  auto count = [](int n, const char* word) { return QString("%1 %2%3").arg(n).arg(word).arg(n == 1 ? "" : "s"); };
  if (findings.errors.isEmpty() && findings.warnings.isEmpty())
    parts << "No errors or warnings";
  else
    parts << count(int(findings.errors.size()), "error") + ", " + count(int(findings.warnings.size()), "warning");
  QStringList words = findings.errorWords + findings.warningWords;
  if (!words.isEmpty()) parts.last() += ": " + words.join(", ");
  if (!findings.noteWords.isEmpty()) parts << "notes: " + findings.noteWords.join(", ");
  answer["summary"] = parts.join("; ") + ".";
  return answer;
}
