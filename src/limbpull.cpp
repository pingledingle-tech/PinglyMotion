/*
 * PinglyMotion - pulling a limb by one of its joints (see limbpull.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "limbpull.h"

#include "bvhnode.h"

#include <algorithm>

namespace LimbPull
{

static double component(const QVector3D& v, int axis)
{
  return axis == 0 ? v.x() : axis == 1 ? v.y() : v.z();
}

static void setComponent(QVector3D& v, int axis, double value)
{
  if (axis == 0) v.setX(float(value));
  else if (axis == 1) v.setY(float(value));
  else v.setZ(float(value));
}

QMatrix4x4 turnMatrix(const Joint& joint)
{
  QMatrix4x4 m;
  for (int i = 0; i < 3; ++i)
  {
    const int axis = joint.axes[i];
    m.rotate(float(component(joint.rotation, axis)), axis == 0, axis == 1, axis == 2);
  }
  return m;
}

QMatrix4x4 frameAfter(const Chain& chain, int index)
{
  QMatrix4x4 m;
  for (int i = 0; i <= index && i < chain.joints.size(); ++i)
    m = m * chain.joints[i].link * turnMatrix(chain.joints[i]);
  return m;
}

QVector3D tipPosition(const Chain& chain)
{
  if (chain.joints.isEmpty()) return QVector3D();
  return frameAfter(chain, int(chain.joints.size()) - 1).map(chain.tip);
}

void solve(Chain& chain, const QVector3D& target, bool limits)
{
  for (int round = 0; round < rounds; ++round)
  {
    for (int k = 0; k < chain.driven.size(); ++k)
    {
      Joint& joint = chain.joints[chain.driven[k]];
      const double weight = weights[std::min(k, 2)];
      for (int axis = 0; axis < 3; ++axis)
      {
        const double now = component(joint.rotation, axis);
        const QVector3D before = tipPosition(chain);
        setComponent(joint.rotation, axis, now + testTurn);
        const QVector3D after = tipPosition(chain);
        // how far the tip moves for one degree, and where it should go
        const QVector3D moved = after - before;
        const QVector3D wanted = target - before;
        const double step = double(QVector3D::dotProduct(moved, wanted)) /
                            (double(QVector3D::dotProduct(moved, moved)) + damping * damping) * weight;
        double value = now + std::clamp(step, -largestStep, largestStep);
        if (limits)
          value = std::clamp(value, component(joint.minimum, axis), component(joint.maximum, axis));
        setComponent(joint.rotation, axis, value);
      }
    }
  }
}

static Joint jointFor(BVHNode* node, const TurnOf& turnOf, const LimitsOf& limitsOf)
{
  Joint joint;
  joint.name = node->name();
  axesOf(node, joint.axes);
  joint.rotation = turnOf(node);
  limitsOf(node, joint.minimum, joint.maximum);
  return joint;
}

bool setUp(Pull& pull, BVHNode* root, BVHNode* joint, bool turnOnly,
           const FrameOf& frameOf, const TurnOf& turnOf, const LimitsOf& limitsOf)
{
  pull = Pull();
  if (!joint || (joint->type != BVH_JOINT && joint->type != BVH_ROOT)) return false;
  QList<BVHNode*> above;
  if (!turnOnly)
  {
    above = drivenAbove(root, joint);
    if (above.isEmpty())
    {
      turnOnly = true;
      pull.held = true;
    }
  }
  for (int i = int(above.size()) - 1; i >= 0; --i) pull.nodes.append(above[i]);
  pull.nodes.append(joint);
  pull.pulled = joint;
  pull.chain.turnOnly = turnOnly;

  QVector<QMatrix4x4> frames;
  for (BVHNode* node : pull.nodes)
  {
    QMatrix4x4 frame;
    if (!frameOf(node, frame)) { pull = Pull(); return false; }
    frames.append(frame);
    pull.chain.joints.append(jointFor(node, turnOf, limitsOf));
  }
  for (int i = 0; i < frames.size(); ++i)
  {
    if (i == 0) pull.chain.joints[i].link = frames[0];
    else pull.chain.joints[i].link = (frames[i - 1] * turnMatrix(pull.chain.joints[i - 1])).inverted() * frames[i];
  }

  const int last = int(pull.nodes.size()) - 1;
  if (turnOnly)
  {
    // the joint's own bone follows the pointer: the tip is where its first child is
    pull.chain.tip = QVector3D(0, -3, 0);
    if (joint->numChildren() > 0)
    {
      QMatrix4x4 childFrame;
      if (frameOf(joint->child(0), childFrame))
        pull.chain.tip = (frames[last] * turnMatrix(pull.chain.joints[last])).inverted().map(childFrame.map(QVector3D()));
    }
    pull.chain.driven = {last};
  }
  else
  {
    pull.chain.tip = QVector3D();
    for (int i = last - 1; i >= 0; --i) pull.chain.driven.append(i);
  }
  for (int index : pull.chain.driven) pull.before.append(pull.chain.joints[index].rotation);
  return true;
}

FrameOf frameFromSkeleton(BVHNode* root, const TurnOf& turnOf)
{
  return [root, turnOf](BVHNode* node, QMatrix4x4& frame) -> bool
  {
    const QList<BVHNode*> path = pathTo(root, node);
    if (path.isEmpty()) return false;
    QMatrix4x4 m;
    for (BVHNode* step : path)
    {
      if (step != root) m.translate(float(step->offset[0]), float(step->offset[1]), float(step->offset[2]));
      if (step == node) break;
      Joint joint;
      axesOf(step, joint.axes);
      joint.rotation = turnOf(step);
      m = m * turnMatrix(joint);
    }
    frame = m;
    return true;
  };
}

QList<QPair<BVHNode*, QVector3D>> drivenTurns(const Pull& pull)
{
  QList<QPair<BVHNode*, QVector3D>> turns;
  for (int index : pull.chain.driven)
    turns.append(qMakePair(pull.nodes[index], pull.chain.joints[index].rotation));
  return turns;
}

bool isBentoName(const QString& name)
{
  return name.length() > 1 && name[0] == QLatin1Char('m') && name[1].isUpper();
}

static bool findPath(BVHNode* node, BVHNode* joint, QList<BVHNode*>& path)
{
  if (!node) return false;
  path.append(node);
  if (node == joint) return true;
  for (int i = 0; i < node->numChildren(); ++i)
    if (findPath(node->child(i), joint, path)) return true;
  path.removeLast();
  return false;
}

QList<BVHNode*> pathTo(BVHNode* root, BVHNode* joint)
{
  QList<BVHNode*> path;
  if (!findPath(root, joint, path)) path.clear();
  return path;
}

QList<BVHNode*> drivenAbove(BVHNode* root, BVHNode* joint)
{
  QList<BVHNode*> driven;
  if (!joint || joint->type == BVH_POS || joint->type == BVH_ROOT) return driven;
  const QList<BVHNode*> path = pathTo(root, joint);
  const bool classic = !isBentoName(joint->name());
  // from the parent upwards
  for (int i = int(path.size()) - 2; i >= 0 && driven.size() < limbLength; --i)
  {
    BVHNode* node = path[i];
    if (node->type == BVH_ROOT || node->type == BVH_POS) break;
    if (node->type != BVH_JOINT) continue;
    if (classic && isBentoName(node->name())) continue;
    driven.append(node);
  }
  return driven;
}

void axesOf(const BVHNode* node, int axes[3])
{
  axes[0] = 0; axes[1] = 1; axes[2] = 2;
  if (!node) return;
  int found = 0;
  int order[3] = {0, 1, 2};
  for (int i = 0; i < node->numChannels && found < 3; ++i)
  {
    switch (node->channelType[i])
    {
      case BVH_XROT: order[found++] = 0; break;
      case BVH_YROT: order[found++] = 1; break;
      case BVH_ZROT: order[found++] = 2; break;
      default: break;
    }
  }
  if (found == 3)
    for (int i = 0; i < 3; ++i) axes[i] = order[i];
}

}
