/*
 * PinglyMotion - describe_motion: the motion as computed text (issue #129)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <algorithm>
#include <climits>
#include <cmath>
#include <vector>

#include <QJsonArray>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QStringList>
#include <QtMath>

#include "animation.h"
#include "animexport.h"
#include "bvhnode.h"
#include "posedescribe.h"

#include "motiondescribe.h"
#include "skeletonwalk.h"

namespace
{
  // ------------------------------------------------------------ thresholds
  // Every word in the answer comes from one of these; docs/CONTROL.md lists them.

  // a joint "moves" when its main axis spans more than this (degrees),
  // the same 5 degrees describe_pose uses for a changed joint
  const double JOINT_MOVES_FROM = 5.0;
  // a joint "holds" while its main axis turns slower than this (degrees per second)
  const double JOINT_HOLD_BELOW = 10.0;
  // a turn of less than this (degrees) is folded into the hold around it
  const double JOINT_MIN_TURN = 2.0;
  // speed classes by the top speed of a turn (degrees per second)
  const double SPEED_MEDIUM_FROM = 60.0;
  const double SPEED_FAST_FROM = 180.0;
  const double SPEED_VERY_FAST_FROM = 360.0;

  // a hand or foot "moves" while faster than this (inches per second, 0.05 m/s)
  const double PATH_MOVES_FROM = 2.0;
  // a move shorter than this along its path (inches) is folded into the stillness around it
  const double PATH_MIN_LENGTH = 1.0;
  // shape: "out and back" when start and end are closer than this share of the path;
  // otherwise "straight" when no point is further from the start-end line than this share of it
  const double PATH_OUT_AND_BACK_BELOW = 0.3;
  const double PATH_STRAIGHT_UP_TO = 0.1;
  // a move "rises" or "lowers" when its height changes by at least this (inches)
  const double PATH_HEIGHT_CHANGE = 1.0;

  // a hold or stillness shorter than this many frame steps between two moves
  // is folded into the move before it
  const int SHORT_PAUSE_STEPS = 3;

  // foot contact, by height with a dead zone: a foot is down when its ankle
  // comes to at most CONTACT_HEIGHT above the floor and up again when higher
  // than LEAVE_HEIGHT (inches; 2 in = 5 cm). Speed does not decide contact, so
  // a foot that slides is still down; how fast it slides is reported instead.
  const double CONTACT_HEIGHT = 2.0;
  const double LEAVE_HEIGHT = 3.0;
  // weight with both feet down: on one foot when the hip is this far towards
  // it (inches), back in the middle below the second number (describe_pose's band)
  const double WEIGHT_SIDE_FROM = 3.0;
  const double WEIGHT_MIDDLE_UP_TO = 1.5;

  // rhythm: the motion repeats when the autocorrelation of all classic joint
  // angles reaches this at some period between REPEAT_MIN_SECONDS and half the range
  const double REPEAT_MIN_CORRELATION = 0.7;
  const double REPEAT_MIN_SECONDS = 0.25;
  // at most this many frames go into the autocorrelation (longer ranges are sampled)
  const int REPEAT_MAX_SAMPLES = 600;
  // left and right "move in opposition" below minus this, "together" above it
  const double PAIR_CORRELATION = 0.5;

  // loop seam: "smooth" up to the first pair, "small jump" up to the second
  const double SEAM_SMOOTH_DEGREES = 2.0;
  const double SEAM_SMOOTH_INCHES = 0.5;
  const double SEAM_SMALL_DEGREES = 10.0;
  const double SEAM_SMALL_INCHES = 2.0;

  // key table: left out when the range has more keys than this (motion capture)
  const int KEY_TABLE_MAX_KEYS = 40;
  // a joint is named in the key table when its main change is at least this (degrees)
  const double KEY_CHANGE_FROM = 2.0;

  // lists are cut after this many entries, with a "more" count
  const int MAX_LIST = 8;

  // ------------------------------------------------------------ helpers

  // rounding that stays defined for any number a file can hold (qRound
  // overflows an int on the 1e30 of the edge-case files): huge values are
  // capped, and anything not finite becomes 0
  double rounded(double v, double scale)
  {
    if (!std::isfinite(v)) return 0.0;
    v = qBound(-1e12, v, 1e12);
    return std::round(v * scale) / scale;
  }
  double whole(double v) { return rounded(v, 1.0); }
  double round1(double v) { return rounded(v, 10.0); }
  double round2(double v) { return rounded(v, 100.0); }
  QString wholeText(double v) { return QString::number(whole(v), 'f', 0); }

  // the shortest signed difference between two angles in degrees
  double angleDelta(double from, double to)
  {
    double d = std::fmod(to - from, 360.0);
    if (d > 180.0) d -= 360.0;
    if (d < -180.0) d += 360.0;
    return d;
  }

  // an angle series without the 360-degree jumps of Euler angles (179 to -179)
  std::vector<double> unwrap(const std::vector<double>& raw)
  {
    std::vector<double> out(raw.size());
    if (raw.empty()) return out;
    out[0] = raw[0];
    for (size_t i = 1; i < raw.size(); i++) out[i] = out[i - 1] + angleDelta(raw[i - 1], raw[i]);
    return out;
  }

  // the angle in degrees of the one rotation that takes a to b
  double angleBetween(const QQuaternion& a, const QQuaternion& b)
  {
    const double dot = qBound(0.0, double(qAbs(QQuaternion::dotProduct(a, b))), 1.0);
    return qRadiansToDegrees(2.0 * std::acos(dot));
  }

  // an angle in -180 to 180, as get_pose shows it
  double normalized(double angle)
  {
    double a = std::fmod(angle, 360.0);
    if (a > 180.0) a -= 360.0;
    if (a <= -180.0) a += 360.0;
    return a;
  }

  QString speedClass(double degreesPerSecond)
  {
    if (degreesPerSecond >= SPEED_VERY_FAST_FROM) return "very fast";
    if (degreesPerSecond >= SPEED_FAST_FROM) return "fast";
    if (degreesPerSecond >= SPEED_MEDIUM_FROM) return "medium";
    return "slow";
  }

  // a run of frame steps with one class: steps first..last (step i goes from
  // frame i to frame i+1, counted inside the range)
  struct Run { int kind; int first; int last; };

  std::vector<Run> runsOf(const std::vector<int>& kinds)
  {
    std::vector<Run> runs;
    for (int i = 0; i < int(kinds.size()); i++)
    {
      if (!runs.empty() && runs.back().kind == kinds[i]) runs.back().last = i;
      else runs.push_back({kinds[i], i, i});
    }
    return runs;
  }

  std::vector<Run> merge(const std::vector<Run>& runs)
  {
    std::vector<Run> out;
    for (const Run& run : runs)
    {
      if (!out.empty() && out.back().kind == run.kind) out.back().last = run.last;
      else out.push_back(run);
    }
    return out;
  }

  // fold short pauses (kind 0) between two moves into the move before them
  // (a turnaround or a hiccup, not a hold worth naming)
  std::vector<Run> foldShortPauses(std::vector<Run> runs)
  {
    for (size_t i = 1; i + 1 < runs.size(); i++)
    {
      if (runs[i].kind == 0 && runs[i - 1].kind != 0 && runs[i + 1].kind != 0 &&
          runs[i].last - runs[i].first + 1 < SHORT_PAUSE_STEPS)
        runs[i].kind = runs[i - 1].kind;
    }
    return merge(runs);
  }

  // Pearson correlation of two equally long sets of 3D samples
  double correlation(const std::vector<QVector3D>& a, const std::vector<QVector3D>& b)
  {
    if (a.size() < 2 || a.size() != b.size()) return 0.0;
    QVector3D meanA, meanB;
    for (size_t i = 0; i < a.size(); i++) { meanA += a[i]; meanB += b[i]; }
    meanA /= float(a.size());
    meanB /= float(b.size());
    double ab = 0, aa = 0, bb = 0;
    for (size_t i = 0; i < a.size(); i++)
    {
      const QVector3D da = a[i] - meanA, db = b[i] - meanB;
      ab += QVector3D::dotProduct(da, db);
      aa += QVector3D::dotProduct(da, da);
      bb += QVector3D::dotProduct(db, db);
    }
    if (aa <= 1e-9 || bb <= 1e-9) return 0.0;
    return ab / std::sqrt(aa * bb);
  }

  // ------------------------------------------------------------ what is sampled

  using MotionDescribe::Samples;

  const QStringList POINTS = {"hip", "head", "lHand", "rHand", "lFoot", "rFoot"};

  QVector3D flat(const QVector3D& v)
  {
    QVector3D out(v.x(), 0, v.z());
    return out.lengthSquared() > 1e-9 ? out.normalized() : QVector3D(0, 0, 1);
  }

  // FrameData of a joint at a frame, without BVHNode::frameData()'s
  // frame-by-frame walk back to the previous key where it is not needed: past
  // the last key, and between two equal keys, the value is the key's own.
  // Only frames between two different keys go through frameData(), so the
  // cost stays that of playing the animation once.
  class KeyCache
  {
    public:
      FrameData at(BVHNode* node, int frame)
      {
        auto it = m_keys.find(node);
        if (it == m_keys.end()) it = m_keys.insert(node, node->keyframeList());
        const QList<int>& keys = it.value();
        if (keys.isEmpty()) return node->frameData(frame);
        if (frame >= keys.last()) return node->frameData(keys.last());
        const auto after = std::upper_bound(keys.cbegin(), keys.cend(), frame);
        if (after == keys.cbegin()) return node->frameData(frame);
        const int before = *(after - 1);
        if (before == frame) return node->frameData(frame);
        const FrameData a = node->frameData(before), b = node->frameData(*after);
        if (a.rotation() == b.rotation() && a.position() == b.position()) return a;
        return node->frameData(frame);
      }

    private:
      QHash<BVHNode*, QList<int>> m_keys;
  };

  // the world matrix of every joint, as PoseDescribe::worldTransforms() and
  // drawPart() in animationview.cpp place them, through the KeyCache
  Samples sampleFrames(Animation* animation, int from, int to)
  {
    Samples s;
    s.from = from;
    s.to = to;
    s.count = to - from + 1;
    // the old open path can leave 0 frames per second (frame time above 2 s, see
    // tests/data/generated/README.md): then count 30 frames per second
    s.frameTime = animation->frameTime();
    if (!std::isfinite(s.frameTime) || s.frameTime <= 0) s.frameTime = 1.0 / 30.0;

    QHash<QString, BVHNode*> node;
    for (const QString& name : PoseDescribe::classicJoints())
    {
      BVHNode* found = PoseDescribe::findJoint(animation, name);
      if (!found) continue;
      node[name] = found;
      s.joints << name;
    }

    KeyCache cache;
    QHash<QString, std::vector<double>> raw[3];
    for (int f = from; f <= to; f++)
    {
      QHash<BVHNode*, QMatrix4x4> world;
      QMatrix4x4 base;
      base.translate(cache.at(animation->getNode(0), f).position());
      SkeletonWalk::place(animation->getMotion(), base,
                          [&cache, f](BVHNode* node) { return cache.at(node, f).rotation(); }, world, true);
      for (const QString& name : s.joints)
      {
        const QVector3D r = cache.at(node[name], f).rotation();
        QQuaternion q;
        for (int i = 0; i < node[name]->numChannels; i++)
        {
          switch (node[name]->channelType[i])
          {
            case BVH_XROT: q = q * QQuaternion::fromAxisAndAngle(1, 0, 0, r.x()); break;
            case BVH_YROT: q = q * QQuaternion::fromAxisAndAngle(0, 1, 0, r.y()); break;
            case BVH_ZROT: q = q * QQuaternion::fromAxisAndAngle(0, 0, 1, r.z()); break;
            default: break;
          }
        }
        s.orientation[name].push_back(q.normalized());
        raw[0][name].push_back(r.x());
        raw[1][name].push_back(r.y());
        raw[2][name].push_back(r.z());
      }
      for (const QString& name : POINTS)
        if (node.contains(name)) s.position[name].push_back(world.value(node[name]).map(QVector3D()));
      const QMatrix4x4 hip = node.contains("hip") ? world.value(node["hip"]) : QMatrix4x4();
      s.hipForward.push_back(flat(hip.mapVector(QVector3D(0, 0, 1))));
      s.hipLeft.push_back(flat(hip.mapVector(QVector3D(1, 0, 0))));
    }
    for (const QString& name : s.joints)
    {
      const std::vector<double> x = unwrap(raw[0][name]), y = unwrap(raw[1][name]), z = unwrap(raw[2][name]);
      std::vector<QVector3D>& out = s.rotation[name];
      for (int i = 0; i < s.count; i++) out.push_back(QVector3D(float(x[i]), float(y[i]), float(z[i])));
    }
    return s;
  }

  int frameNumber(const Samples& s, int index) { return s.from + index + 1; }

  // ------------------------------------------------------------ joints

  struct JointMotion
  {
    QString name;
    int axis = 0;
    double travel = 0;
    double reach = 0;
    int firstMove = -1;   // sample index, -1 when it never turns
    QJsonObject json;
  };

  QString axisName(int axis) { return QString(QChar('X' + axis)); }

  std::vector<JointMotion> jointMotions(const Samples& s, QStringList& still)
  {
    std::vector<JointMotion> out;
    for (const QString& name : s.joints)
    {
      // how far the joint gets from where it starts, as one rotation, so
      // Euler angles that swing near gimbal lock do not count as movement
      const std::vector<QQuaternion>& q = s.orientation[name];
      double reach = 0;
      for (const QQuaternion& v : q) reach = qMax(reach, angleBetween(q.front(), v));
      if (reach <= JOINT_MOVES_FROM) { still << name; continue; }

      // the main axis: the Euler axis (as set_pose takes it) that spans most
      const std::vector<QVector3D>& r = s.rotation[name];
      double low[3], high[3];
      for (int a = 0; a < 3; a++) { low[a] = high[a] = r[0][a]; }
      for (const QVector3D& v : r)
        for (int a = 0; a < 3; a++) { low[a] = qMin(low[a], double(v[a])); high[a] = qMax(high[a], double(v[a])); }
      int axis = 0;
      for (int a = 1; a < 3; a++) if (high[a] - low[a] > high[axis] - low[axis]) axis = a;

      std::vector<double> angle;
      for (const QVector3D& v : r) angle.push_back(v[axis]);

      // speeds come from the one-rotation steps, the direction from the main axis
      std::vector<double> stepSpeed;
      std::vector<int> kinds;
      double travel = 0, topSpeed = 0;
      for (int i = 0; i + 1 < s.count; i++)
      {
        const double step = angleBetween(q[i], q[i + 1]);
        const double speed = step / s.frameTime;
        stepSpeed.push_back(speed);
        travel += step;
        topSpeed = qMax(topSpeed, speed);
        const double delta = angle[i + 1] - angle[i];
        kinds.push_back(speed < JOINT_HOLD_BELOW ? 0 : (delta > 0 ? 1 : -1));
      }

      std::vector<Run> runs = foldShortPauses(runsOf(kinds));
      for (Run& run : runs)
        if (run.kind != 0 && qAbs(angle[run.last + 1] - angle[run.first]) < JOINT_MIN_TURN) run.kind = 0;
      runs = foldShortPauses(merge(runs));

      JointMotion motion;
      motion.name = name;
      motion.axis = axis;
      motion.travel = travel;
      motion.reach = reach;

      QJsonArray segments;
      int shown = 0;
      for (const Run& run : runs)
      {
        if (run.kind != 0 && motion.firstMove < 0) motion.firstMove = run.first;
        if (shown >= MAX_LIST) { shown++; continue; }
        shown++;
        QJsonObject segment;
        segment["from"] = frameNumber(s, run.first);
        segment["to"] = frameNumber(s, run.last + 1);
        if (run.kind == 0)
        {
          segment["change"] = "holds";
        }
        else
        {
          double top = 0;
          for (int i = run.first; i <= run.last; i++) top = qMax(top, stepSpeed[i]);
          segment["change"] = run.kind > 0 ? "increases" : "decreases";
          segment["degrees"] = round1(qAbs(angle[run.last + 1] - angle[run.first]));
          segment["top_speed"] = whole(top);
          segment["speed"] = speedClass(top);
        }
        segments << segment;
      }

      QJsonObject j;
      j["joint"] = name;
      j["friendly"] = PoseDescribe::friendlyName(name);
      j["axis"] = axisName(axis);
      j["start"] = round1(normalized(angle.front()));
      j["end"] = round1(normalized(angle.back()));
      j["reach"] = round1(reach);
      j["travel"] = round1(travel);
      j["top_speed"] = whole(topSpeed);
      j["speed"] = speedClass(topSpeed);
      if (motion.firstMove >= 0) j["first_moves"] = frameNumber(s, motion.firstMove);
      j["segments"] = segments;
      if (shown > MAX_LIST) j["segments_more"] = shown - MAX_LIST;
      motion.json = j;
      out.push_back(motion);
    }

    // the order the joints start in, then the one that travels furthest
    std::stable_sort(out.begin(), out.end(), [](const JointMotion& a, const JointMotion& b) {
      const int fa = a.firstMove < 0 ? INT_MAX : a.firstMove, fb = b.firstMove < 0 ? INT_MAX : b.firstMove;
      if (fa != fb) return fa < fb;
      return a.travel > b.travel;
    });
    return out;
  }

  // ------------------------------------------------------------ paths

  double distanceToLine(const QVector3D& p, const QVector3D& a, const QVector3D& b)
  {
    const QVector3D ab = b - a;
    if (ab.lengthSquared() < 1e-9) return (p - a).length();
    const double t = qBound(0.0, double(QVector3D::dotProduct(p - a, ab) / ab.lengthSquared()), 1.0);
    return (p - (a + ab * float(t))).length();
  }

  // the moves are measured relative to the hip, so walking or running
  // forward does not hide the swing of a hand or foot; the heights are the
  // world heights
  QJsonObject pathOf(const Samples& s, const QString& name)
  {
    const std::vector<QVector3D>& world = s.position[name];
    std::vector<QVector3D> p;
    for (int i = 0; i < s.count; i++) p.push_back(world[i] - s.position["hip"][i]);
    std::vector<int> kinds;
    double length = 0, topSpeed = 0, lowest = world[0].y(), highest = world[0].y();
    for (int i = 0; i + 1 < s.count; i++)
    {
      const double step = (p[i + 1] - p[i]).length();
      length += step;
      topSpeed = qMax(topSpeed, step / s.frameTime);
      kinds.push_back(step / s.frameTime >= PATH_MOVES_FROM ? 1 : 0);
    }
    for (const QVector3D& v : world) { lowest = qMin(lowest, double(v.y())); highest = qMax(highest, double(v.y())); }

    // a new move starts where the hand or foot turns back (a swing forward,
    // then back): inside a moving stretch, once it has come back towards
    // where the move started by a quarter of its furthest distance (at least
    // PATH_MIN_LENGTH), the move ends at that furthest point
    int current = 1;
    for (int i = 0; i + 1 < s.count;)
    {
      if (!kinds[i]) { i++; continue; }
      const int start = i;
      int furthest = i + 1;
      double furthestDistance = 0;
      int j = i;
      for (; j + 1 < s.count && kinds[j]; j++)
      {
        const double d = (p[j + 1] - p[start]).length();
        if (d > furthestDistance) { furthestDistance = d; furthest = j + 1; }
        else if (d < furthestDistance - qMax(PATH_MIN_LENGTH, 0.25 * furthestDistance)) break;
      }
      const int end = (j + 1 < s.count && kinds[j]) ? furthest : j;   // first step not in this move
      for (int k = start; k < end; k++) kinds[k] = current;
      current = 3 - current;
      i = qMax(end, start + 1);
    }

    auto runLength = [&](const Run& run) {
      double l = 0;
      for (int i = run.first; i <= run.last; i++) l += (p[i + 1] - p[i]).length();
      return l;
    };
    std::vector<Run> runs = foldShortPauses(runsOf(kinds));
    for (Run& run : runs) if (run.kind != 0 && runLength(run) < PATH_MIN_LENGTH) run.kind = 0;
    runs = merge(runs);

    QJsonArray moves;
    int count = 0;
    for (const Run& run : runs)
    {
      if (run.kind == 0) continue;
      if (count++ >= MAX_LIST) continue;
      const QVector3D a = p[run.first], b = p[run.last + 1];
      const double l = runLength(run), chord = (b - a).length();
      double deviation = 0, top = 0;
      for (int i = run.first; i <= run.last + 1; i++) deviation = qMax(deviation, distanceToLine(p[i], a, b));
      for (int i = run.first; i <= run.last; i++) top = qMax(top, double((p[i + 1] - p[i]).length()) / s.frameTime);
      QString shape;
      if (chord < PATH_OUT_AND_BACK_BELOW * l) shape = "out and back";
      else if (deviation <= PATH_STRAIGHT_UP_TO * chord) shape = "straight";
      else shape = "arc";
      const double rise = b.y() - a.y();

      QJsonObject move;
      move["from"] = frameNumber(s, run.first);
      move["to"] = frameNumber(s, run.last + 1);
      move["shape"] = shape;
      move["path"] = round1(l);
      move["distance"] = round1(chord);
      if (rise >= PATH_HEIGHT_CHANGE) move["height"] = QString("rises %1").arg(round1(rise));
      else if (rise <= -PATH_HEIGHT_CHANGE) move["height"] = QString("lowers %1").arg(round1(-rise));
      move["top_speed"] = round1(top);
      moves << move;
    }

    QJsonObject out;
    out["path"] = round1(length);
    out["top_speed"] = round1(topSpeed);
    out["top_speed_meters_per_second"] = round2(topSpeed * AnimExport::INCHES_TO_METERS);
    out["lowest"] = round2(lowest);
    out["highest"] = round2(highest);
    out["moves"] = moves;
    if (count > MAX_LIST) out["moves_more"] = count - MAX_LIST;
    return out;
  }

  // ------------------------------------------------------------ contact and weight

  struct Contact
  {
    double floor = 0;
    QHash<QString, std::vector<bool>> down;   // "l"/"r" -> per sample
  };

  Contact contactOf(const Samples& s)
  {
    Contact c;
    c.floor = s.position["lFoot"][0].y();
    for (const QString& foot : {QStringLiteral("lFoot"), QStringLiteral("rFoot")})
      for (const QVector3D& v : s.position[foot]) c.floor = qMin(c.floor, double(v.y()));

    for (const QString& side : {QStringLiteral("l"), QStringLiteral("r")})
    {
      const std::vector<QVector3D>& p = s.position[side + "Foot"];
      std::vector<bool>& down = c.down[side];
      bool on = false;
      for (int i = 0; i < s.count; i++)
      {
        const double height = p[i].y() - c.floor;
        if (i == 0 || !on) on = height <= CONTACT_HEIGHT;
        else on = height <= LEAVE_HEIGHT;
        down.push_back(on);
      }
    }
    return c;
  }

  QJsonArray periodsOf(const Samples& s, const std::vector<bool>& flags, int* more)
  {
    QJsonArray out;
    int count = 0;
    for (int i = 0; i < s.count; i++)
    {
      if (!flags[i] || (i > 0 && flags[i - 1])) continue;
      int j = i;
      while (j + 1 < s.count && flags[j + 1]) j++;
      if (count++ < MAX_LIST) out << QJsonObject{{"from", frameNumber(s, i)}, {"to", frameNumber(s, j)}};
    }
    if (more) *more = qMax(0, count - MAX_LIST);
    return out;
  }

  // touch-downs after the first sample, in order: 'l' or 'r'
  QString touchDowns(const Samples& s, const Contact& c)
  {
    QString order;
    for (int i = 1; i < s.count; i++)
    {
      if (c.down["l"][i] && !c.down["l"][i - 1]) order += 'l';
      if (c.down["r"][i] && !c.down["r"][i - 1]) order += 'r';
    }
    return order;
  }

  QJsonObject contactJson(const Samples& s, const Contact& c, QString& weightLine)
  {
    QJsonObject out;
    out["floor"] = round2(c.floor);
    for (const QString& side : {QStringLiteral("l"), QStringLiteral("r")})
    {
      const std::vector<bool>& down = c.down[side];
      int frames = 0;
      for (bool d : down) frames += d ? 1 : 0;
      int more = 0;
      QJsonObject foot;
      foot["on_floor_percent"] = whole(100.0 * frames / s.count);
      foot["on_floor"] = periodsOf(s, down, &more);
      if (more) foot["on_floor_more"] = more;
      int landings = 0;
      for (int i = 1; i < s.count; i++) if (down[i] && !down[i - 1]) landings++;
      foot["landings"] = landings;
      // the fastest the ankle moves along the floor while down (foot sliding)
      double slide = 0;
      const std::vector<QVector3D>& p = s.position[side + "Foot"];
      for (int i = 1; i < s.count; i++)
        if (down[i] && down[i - 1])
        {
          QVector3D step = p[i] - p[i - 1];
          step.setY(0);
          slide = qMax(slide, double(step.length()) / s.frameTime);
        }
      foot["slide_top_speed"] = round1(slide);
      out[side + "Foot"] = foot;
    }
    std::vector<bool> air;
    for (int i = 0; i < s.count; i++) air.push_back(!c.down["l"][i] && !c.down["r"][i]);
    int more = 0;
    const QJsonArray airborne = periodsOf(s, air, &more);
    out["both_feet_off_floor"] = airborne;
    if (more) out["both_feet_off_floor_more"] = more;

    // weight, per sample: the foot that is down, or with both down the side the hip leans to
    std::vector<int> weight;   // 0 both, 1 left, 2 right, 3 none
    int side = 0;
    for (int i = 0; i < s.count; i++)
    {
      const bool l = c.down["l"][i], r = c.down["r"][i];
      if (l && !r) { weight.push_back(1); side = 0; continue; }
      if (r && !l) { weight.push_back(2); side = 0; continue; }
      if (!l && !r) { weight.push_back(3); side = 0; continue; }
      const QVector3D middle = (s.position["lFoot"][i] + s.position["rFoot"][i]) / 2.0f;
      const double sideways = QVector3D::dotProduct(s.position["hip"][i] - middle, s.hipLeft[i]);
      if (qAbs(sideways) >= WEIGHT_SIDE_FROM) side = sideways > 0 ? 1 : 2;
      else if (qAbs(sideways) <= WEIGHT_MIDDLE_UP_TO) side = 0;
      weight.push_back(side);
    }
    static const char* words[] = {"both feet", "left foot", "right foot", "in the air"};
    QJsonArray weightJson;
    QStringList shifts;
    const std::vector<Run> runs = runsOf(weight);
    int shown = 0;
    for (const Run& run : runs)
    {
      if (shown++ >= MAX_LIST) continue;
      weightJson << QJsonObject{{"from", frameNumber(s, run.first)}, {"to", frameNumber(s, run.last)}, {"on", words[run.kind]}};
      shifts << words[run.kind];
    }
    out["weight"] = weightJson;
    if (shown > MAX_LIST) out["weight_more"] = shown - MAX_LIST;
    static const char* lineWords[] = {"weight on both feet", "weight on the left foot", "weight on the right foot", "in the air"};
    if (runs.size() == 1) weightLine = QString("%1 throughout").arg(lineWords[runs.front().kind]);
    else if (runs.size() <= 4) weightLine = "weight: " + shifts.join(", then ");
    else weightLine = QString("weight shifts %1 times").arg(runs.size() - 1);
    return out;
  }

  // ------------------------------------------------------------ hip

  QJsonObject hipJson(const Samples& s, QString& travelLine)
  {
    const std::vector<QVector3D>& p = s.position["hip"];
    double lowest = p[0].y(), highest = p[0].y(), path = 0;
    for (int i = 0; i < s.count; i++)
    {
      lowest = qMin(lowest, double(p[i].y()));
      highest = qMax(highest, double(p[i].y()));
      if (i > 0) { QVector3D step = p[i] - p[i - 1]; step.setY(0); path += step.length(); }
    }
    const QVector3D move = p.back() - p.front();
    const double forward = QVector3D::dotProduct(move, s.hipForward.front());
    const double left = QVector3D::dotProduct(move, s.hipLeft.front());

    QJsonObject out;
    out["start_height"] = round2(p.front().y());
    out["end_height"] = round2(p.back().y());
    out["lowest"] = round2(lowest);
    out["highest"] = round2(highest);
    out["bob"] = round2(highest - lowest);
    out["forward"] = round1(forward);
    out["left"] = round1(left);
    out["up"] = round1(move.y());
    out["floor_path"] = round1(path);

    QStringList parts;
    if (qAbs(forward) >= 3.0) parts << QString("%1 in %2").arg(wholeText(qAbs(forward))).arg(forward > 0 ? "forward" : "back");
    if (qAbs(left) >= 3.0) parts << QString("%1 in to the %2").arg(wholeText(qAbs(left))).arg(left > 0 ? "left" : "right");
    if (qAbs(move.y()) >= 3.0) parts << QString("%1 in %2").arg(wholeText(qAbs(move.y()))).arg(move.y() > 0 ? "up" : "down");
    travelLine = parts.isEmpty() ? QString() : "the hip ends " + parts.join(", ");
    return out;
  }

  // ------------------------------------------------------------ rhythm

  QJsonObject rhythmJson(const Samples& s, const Contact& c, bool anyMotion, QString& rhythmLine)
  {
    QJsonObject out;
    if (!anyMotion) { out["repeats"] = false; return out; }
    const int step = qMax(1, (s.count + REPEAT_MAX_SAMPLES - 1) / REPEAT_MAX_SAMPLES);
    std::vector<std::vector<double>> x;   // per sample, every joint angle minus its mean
    for (int i = 0; i < s.count; i += step)
    {
      std::vector<double> v;
      for (const QString& name : s.joints)
        for (int a = 0; a < 3; a++) v.push_back(s.rotation[name][i][a]);
      x.push_back(v);
    }
    const int n = int(x.size());
    const int dims = x.empty() ? 0 : int(x[0].size());
    for (int d = 0; d < dims; d++)
    {
      double mean = 0;
      for (int i = 0; i < n; i++) mean += x[i][d];
      mean /= n;
      for (int i = 0; i < n; i++) x[i][d] -= mean;
    }
    const double sampleTime = s.frameTime * step;
    const int minLag = qMax(2, int(std::ceil(REPEAT_MIN_SECONDS / sampleTime)));
    const int maxLag = n / 2;
    std::vector<double> r(maxLag + 2, 0.0);
    for (int lag = minLag; lag <= maxLag + 1 && lag < n; lag++)
    {
      double ab = 0, aa = 0, bb = 0;
      for (int i = 0; i + lag < n; i++)
        for (int d = 0; d < dims; d++)
        {
          ab += x[i][d] * x[i + lag][d];
          aa += x[i][d] * x[i][d];
          bb += x[i + lag][d] * x[i + lag][d];
        }
      r[lag] = (aa > 1e-9 && bb > 1e-9) ? ab / std::sqrt(aa * bb) : 0.0;
    }
    // the first peak that is nearly as high as the best one: the shortest repeat, not a multiple of it
    int best = -1;
    double bestValue = 0;
    for (int lag = minLag + 1; lag <= maxLag; lag++)
      if (r[lag] >= r[lag - 1] && r[lag] >= r[lag + 1] && r[lag] > bestValue) { bestValue = r[lag]; best = lag; }
    int period = -1;
    if (best > 0 && bestValue >= REPEAT_MIN_CORRELATION)
      for (int lag = minLag + 1; lag <= maxLag; lag++)
        if (r[lag] >= r[lag - 1] && r[lag] >= r[lag + 1] && r[lag] >= 0.9 * bestValue) { period = lag; break; }

    if (period > 0)
    {
      const double seconds = period * sampleTime;
      out["repeats"] = true;
      out["period_frames"] = period * step;
      out["period_seconds"] = round2(seconds);
      out["per_minute"] = whole(60.0 / seconds);
      out["cycles"] = round1(s.count * s.frameTime / seconds);
      out["match"] = round2(r[period]);
      rhythmLine = QString("repeats every %1 s (%2 per minute)").arg(round2(seconds)).arg(wholeText(60.0 / seconds));
    }
    else
    {
      out["repeats"] = false;
    }

    // left against right: shoulders and thighs, the right side mirrored (negate Y and Z, as BVHNode::mirror())
    QStringList pairLines;
    auto pair = [&](const QString& key, const QString& left, const QString& right, const QString& plural) {
      if (!s.rotation.contains(left) || !s.rotation.contains(right)) return;
      std::vector<QVector3D> a = s.rotation[left], b;
      for (const QVector3D& v : s.rotation[right]) b.push_back(QVector3D(v.x(), -v.y(), -v.z()));
      auto spread = [](const std::vector<QVector3D>& v) {
        QVector3D lo = v[0], hi = v[0];
        for (const QVector3D& p : v) { lo = QVector3D(qMin(lo.x(), p.x()), qMin(lo.y(), p.y()), qMin(lo.z(), p.z()));
                                       hi = QVector3D(qMax(hi.x(), p.x()), qMax(hi.y(), p.y()), qMax(hi.z(), p.z())); }
        const QVector3D d = hi - lo;
        return qMax(d.x(), qMax(d.y(), d.z()));
      };
      if (spread(a) <= JOINT_MOVES_FROM || spread(b) <= JOINT_MOVES_FROM) { out[key] = "not both moving"; return; }
      const double k = correlation(a, b);
      QString word = "move independently";
      if (k <= -PAIR_CORRELATION) word = "move in opposition";
      else if (k >= PAIR_CORRELATION) word = "move together";
      out[key] = word;
      out[key + "_correlation"] = round2(k);
      if (word != "move independently") pairLines << plural + " " + word;
    };
    pair("legs", "lThigh", "rThigh", "legs");
    pair("arms", "lShldr", "rShldr", "arms");

    const QString order = touchDowns(s, c);
    bool alternate = order.size() >= 3;
    for (int i = 1; i < order.size() && alternate; i++) if (order[i] == order[i - 1]) alternate = false;
    out["feet_alternate"] = alternate;
    if (alternate) pairLines << "feet alternate";

    if (!pairLines.isEmpty()) rhythmLine += (rhythmLine.isEmpty() ? "" : "; ") + pairLines.join("; ");
    return out;
  }

  // ------------------------------------------------------------ loop seam

  QJsonObject seamJson(Animation* animation, QString& seamLine)
  {
    const int frames = animation->getNumberOfFrames();
    // frame 1 is the reference that Second Life does not play (docs/ASSISTANT_GUIDE.md section 1)
    int in = qMax(animation->getLoopInPoint(), frames > 1 ? 1 : 0);
    int out = qMin(animation->getLoopOutPoint(), frames - 1);
    if (out <= in) { in = frames > 1 ? 1 : 0; out = frames - 1; }

    QJsonObject seam;
    seam["loop_in"] = in + 1;
    seam["loop_out"] = out + 1;
    if (out <= in) { seam["seam"] = "no loop: one frame"; seamLine = "no loop (one frame)"; return seam; }

    struct Diff { QString joint; double degrees; };
    QList<Diff> diffs;
    double worst = 0;
    for (const QString& name : PoseDescribe::classicJoints())
    {
      BVHNode* node = PoseDescribe::findJoint(animation, name);
      if (!node) continue;
      const QVector3D a = node->frameData(out).rotation(), b = node->frameData(in).rotation();
      double d = 0;
      for (int axis = 0; axis < 3; axis++) d = qMax(d, qAbs(angleDelta(a[axis], b[axis])));
      worst = qMax(worst, d);
      if (d >= SEAM_SMOOTH_DEGREES) diffs << Diff{name, d};
    }
    std::sort(diffs.begin(), diffs.end(), [](const Diff& a, const Diff& b) { return a.degrees > b.degrees; });

    const QHash<BVHNode*, QMatrix4x4> wa = PoseDescribe::worldTransforms(animation, out);
    const QHash<BVHNode*, QMatrix4x4> wb = PoseDescribe::worldTransforms(animation, in);
    BVHNode* hip = PoseDescribe::findJoint(animation, "hip");
    const double hipJump = hip ? (wa.value(hip).map(QVector3D()) - wb.value(hip).map(QVector3D())).length() : 0.0;

    QString verdict = "jump";
    if (worst <= SEAM_SMOOTH_DEGREES && hipJump <= SEAM_SMOOTH_INCHES) verdict = "smooth";
    else if (worst <= SEAM_SMALL_DEGREES && hipJump <= SEAM_SMALL_INCHES) verdict = "small jump";

    QJsonArray largest;
    QStringList names;
    for (int i = 0; i < diffs.size() && i < 3; i++)
    {
      largest << QJsonObject{{"joint", diffs[i].joint}, {"friendly", PoseDescribe::friendlyName(diffs[i].joint)},
                             {"degrees", round1(diffs[i].degrees)}};
      names << QString("%1 %2 degrees").arg(PoseDescribe::friendlyName(diffs[i].joint)).arg(wholeText(diffs[i].degrees));
    }
    seam["seam"] = verdict;
    seam["largest_degrees"] = round1(worst);
    seam["hip_jump"] = round2(hipJump);
    seam["joints"] = largest;
    seamLine = "loop seam " + verdict;
    if (verdict != "smooth" && !names.isEmpty()) seamLine += " (" + names.join(", ") + ")";
    return seam;
  }

  // ------------------------------------------------------------ keys

  QJsonObject keysJson(Animation* animation, const Samples& s)
  {
    QList<int> keys;
    auto add = [&](BVHNode* node) {
      if (!node) return;
      for (int k : node->keyframeList()) if (k >= s.from && k <= s.to) keys << k;
    };
    add(animation->getNode(0));
    for (const QString& name : s.joints) add(PoseDescribe::findJoint(animation, name));
    keys << s.from << s.to;
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    QJsonObject out;
    out["count"] = int(keys.size());
    if (keys.size() > KEY_TABLE_MAX_KEYS)
    {
      out["note"] = "keys on most frames (motion capture or baked easing): no key table";
      return out;
    }
    QJsonArray table;
    for (int i = 0; i + 1 < keys.size(); i++)
    {
      const int a = keys[i] - s.from, b = keys[i + 1] - s.from;
      QJsonArray changes;
      for (const QString& name : s.joints)
      {
        const QVector3D d = s.rotation[name][b] - s.rotation[name][a];
        int axis = 0;
        for (int k = 1; k < 3; k++) if (qAbs(d[k]) > qAbs(d[axis])) axis = k;
        if (qAbs(d[axis]) < KEY_CHANGE_FROM) continue;
        changes << QJsonObject{{"joint", name}, {"friendly", PoseDescribe::friendlyName(name)},
                               {"axis", axisName(axis)}, {"degrees", round1(d[axis])}};
      }
      QJsonObject row;
      row["from"] = keys[i] + 1;
      row["to"] = keys[i + 1] + 1;
      row["seconds"] = round2((keys[i + 1] - keys[i]) * s.frameTime);
      const double hipMove = (s.position["hip"][b] - s.position["hip"][a]).length();
      if (hipMove >= 0.5) row["hip_moves"] = round1(hipMove);
      row["changes"] = changes;
      table << row;
    }
    out["table"] = table;
    return out;
  }
}

QJsonObject MotionDescribe::describe(Animation* animation, int from, int to)
{
  for (const QString& name : POINTS)
    if (!PoseDescribe::findJoint(animation, name))
      return QJsonObject{{"ok", false}, {"error", QString("The skeleton has no joint %1, so the motion cannot be described.").arg(name)}};

  const Samples s = sampleFrames(animation, from, to);

  QStringList still;
  const std::vector<JointMotion> motions = jointMotions(s, still);
  QJsonArray joints, order;
  for (const JointMotion& m : motions)
  {
    joints << m.json;
    if (m.firstMove >= 0) order << m.name;
  }

  QJsonObject paths;
  for (const QString& name : {QStringLiteral("lHand"), QStringLiteral("rHand"), QStringLiteral("lFoot"), QStringLiteral("rFoot")})
    if (s.position.contains(name)) paths[name] = pathOf(s, name);

  const Contact contact = contactOf(s);
  QString weightLine, travelLine, rhythmLine, seamLine;
  const QJsonObject contactAnswer = contactJson(s, contact, weightLine);
  const QJsonObject hipAnswer = hipJson(s, travelLine);
  const QJsonObject rhythmAnswer = rhythmJson(s, contact, !motions.empty(), rhythmLine);
  const QJsonObject seamAnswer = seamJson(animation, seamLine);

  QJsonObject answer;
  answer["ok"] = true;
  answer["from"] = from + 1;
  answer["to"] = to + 1;
  answer["frames"] = s.count;
  answer["fps"] = animation->fps();
  answer["seconds"] = round2((to - from) * s.frameTime);
  answer["units"] = "inches (1 inch = 0.0254 m), degrees, seconds";
  answer["joints"] = joints;
  answer["order"] = order;
  QJsonArray stillJson;
  for (const QString& name : still) stillJson << name;
  answer["still"] = stillJson;
  answer["paths"] = paths;
  answer["contact"] = contactAnswer;
  answer["hip"] = hipAnswer;
  answer["rhythm"] = rhythmAnswer;
  answer["loop"] = seamAnswer;
  answer["keys"] = keysJson(animation, s);

  // the one-line summary
  QStringList parts;
  parts << QString("%1 s at %2 fps (frames %3 to %4)").arg(round2((to - from) * s.frameTime)).arg(animation->fps()).arg(from + 1).arg(to + 1);
  if (motions.empty())
  {
    parts << "nothing moves";
  }
  else
  {
    const JointMotion* most = &motions.front();
    for (const JointMotion& m : motions) if (m.travel > most->travel) most = &m;
    parts << QString("%1 %2 move, the %3 most (turns up to %4 degrees, %5)")
               .arg(motions.size()).arg(motions.size() == 1 ? "joint" : "joints")
               .arg(PoseDescribe::friendlyName(most->name)).arg(wholeText(most->reach)).arg(most->json["speed"].toString());
  }
  if (!rhythmLine.isEmpty()) parts << rhythmLine;
  parts << weightLine;
  if (!travelLine.isEmpty()) parts << travelLine;
  parts << seamLine;
  answer["summary"] = SkeletonWalk::capitalize(parts.join("; ")) + ".";
  return answer;
}

MotionDescribe::Samples MotionDescribe::sample(Animation* animation, int from, int to)
{
  return sampleFrames(animation, from, to);
}

QJsonObject MotionDescribe::loopSeam(Animation* animation)
{
  QString line;
  return seamJson(animation, line);
}
