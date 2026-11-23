/*
 * PinglyMotion - pulling a limb by one of its joints (UX phase 4b part 2,
 * docs/UX_DECISIONS.md 6a; docs/mockup/index.html: limbAbove(), tipOf(),
 * solveIK())
 *
 * Press a joint in the 3D view and drag it: the joints above it bend so the
 * joint follows the pointer ("Pull the limb", IK). With Shift, or with "Turn
 * only this joint", only the joint itself turns, so that its own bone points
 * at the pointer. This file holds the part without a screen, so the tests can
 * check the exact angles a pull gives:
 *
 *  - which joints bend (drivenAbove(): up to three joints above, nearest
 *    first, never the hip; a classic joint never bends a Bento-only bone);
 *  - the chain of those joints as matrices (Chain, tipPosition());
 *  - the solver (solve()), the mockup's own: 14 rounds, in each round every
 *    driven joint, nearest first, turns on X, then Y, then Z by the step a
 *    one-degree test turn says brings the tip closest to the target, times
 *    the joint's weight (1, 0.6, 0.35), at most 20 degrees per step, and
 *    inside the joint's limits while they are on.
 *
 * The numbers (rounds, weights, the largest step) decide how the pull feels;
 * they are kept as the mockup has them and go to the combined review.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef LIMBPULL_H
#define LIMBPULL_H

#include <QList>
#include <QMatrix4x4>
#include <QPair>

#include <functional>
#include <QString>
#include <QVector3D>
#include <QVector>

class BVHNode;

namespace LimbPull
{
  // how the pull feels (the mockup's values; SMAK, see the combined review)
  inline constexpr int rounds = 14;
  inline constexpr double weights[3] = {1.0, 0.6, 0.35};
  inline constexpr double largestStep = 20.0;   // degrees per step
  inline constexpr double testTurn = 1.0;       // degrees, to see which way a turn moves the tip
  // an axis that moves the tip less than this many inches per degree turns only a little
  // (damped least squares; the mockup has almost none, 1e-7): without it, an axis that only
  // twists the bone (the shoulder around a straight arm) spins by the largest step each time
  inline constexpr double damping = 0.05;
  // how many joints above the pulled one bend at most
  inline constexpr int limbLength = 3;

  // one joint of a chain
  struct Joint
  {
    QString name;
    // the channel order of its rotation: axes[0] turns first (as BVHNode's channels)
    int axes[3] = {0, 1, 2};
    // its turn in degrees around X, Y and Z (not in channel order)
    QVector3D rotation;
    QVector3D minimum = QVector3D(-180, -180, -180);
    QVector3D maximum = QVector3D(180, 180, 180);
    // from the frame before (the world for the first joint, else the turned frame of the
    // joint before) to this joint, before its own turn
    QMatrix4x4 link;
  };

  struct Chain
  {
    // from the top joint down to the last one
    QVector<Joint> joints;
    // the point that follows the pointer, in the turned frame of the last joint (the origin
    // when a limb is pulled by that joint: its own place)
    QVector3D tip;
    // the joints that turn, as indices into joints, in the order they are solved (nearest first)
    QVector<int> driven;
    // true when only the last joint turns ("Turn only this joint")
    bool turnOnly = false;
  };

  // a joint's turn as a matrix: R(axes[0]) * R(axes[1]) * R(axes[2]), as the 3D view draws it
  QMatrix4x4 turnMatrix(const Joint& joint);
  // the frame of joint i after its turn, in world coordinates
  QMatrix4x4 frameAfter(const Chain& chain, int index);
  // where the tip is now, in world coordinates
  QVector3D tipPosition(const Chain& chain);

  // turns the driven joints so the tip comes as close to the target as it can (the mockup's
  // solveIK()); limits: keep every turn inside its joint's minimum and maximum
  void solve(Chain& chain, const QVector3D& target, bool limits);

  // the joints that bend when this joint is pulled: up to limbLength joints above it, nearest
  // first. Stops below the hip (the root) and the Avatar Position. Joints that are not part of
  // Second Life's skeleton are passed over, and for a classic joint so are the Bento-only bones
  // (their names start with "m" and a capital, like mSpine1). Empty: the joint only turns
  // (the hip, the abdomen). root is the hip, the top of the joint tree.
  QList<BVHNode*> drivenAbove(BVHNode* root, BVHNode* joint);
  // the joints from the root down to joint, both included; empty when joint is not in the tree
  QList<BVHNode*> pathTo(BVHNode* root, BVHNode* joint);
  // A pull of one joint: the chain, the joints behind it and the turns before the pull.
  struct Pull
  {
    Chain chain;
    // the nodes of chain.joints, in the same order (top first, the pulled joint last)
    QList<BVHNode*> nodes;
    BVHNode* pulled = nullptr;
    // true when the joint was asked to pull but has nothing above it (the hip, the abdomen):
    // it turns only, and the label says "(it holds the body)"
    bool held = false;
    // the turns of the driven joints before the pull, to put back on Esc
    QVector<QVector3D> before;
  };

  // where a joint's frame is in the world before its own turn; false when the joint is not known
  using FrameOf = std::function<bool(BVHNode* node, QMatrix4x4& frame)>;
  // the turn a joint has now and its limits (the window reads them from the animation)
  using TurnOf = std::function<QVector3D(BVHNode* node)>;
  using LimitsOf = std::function<void(BVHNode* node, QVector3D& minimum, QVector3D& maximum)>;

  // Sets up the pull of joint. A joint already turned past one of its limits keeps that turn as
  // the end of its range for this pull, so the pull never snaps it back. turnOnly: only the joint turns (Shift, or Turn only this
  // joint); a joint with nothing above it turns only too (held). frameOf gives each joint's
  // frame before its turn (the 3D view: where it was drawn; the tests: frameFromSkeleton()).
  // False when a frame is missing or the joint has no rotation.
  bool setUp(Pull& pull, BVHNode* root, BVHNode* joint, bool turnOnly,
             const FrameOf& frameOf, const TurnOf& turnOf, const LimitsOf& limitsOf);

  // a FrameOf from the joint tree itself: the offsets and the turns turnOf gives, from the
  // root at the origin (no avatar shape, no position); for the tests and the keyboard pull
  // without a drawn picture
  FrameOf frameFromSkeleton(BVHNode* root, const TurnOf& turnOf);

  // the driven joints' nodes and their turns now, nearest first (the solve order)
  QList<QPair<BVHNode*, QVector3D>> drivenTurns(const Pull& pull);

  // true for a Bento-only bone name (like AnimationView::isBentoBone())
  bool isBentoName(const QString& name);
  // a joint's channel order as axes (0 X, 1 Y, 2 Z); XYZ for a joint without rotation channels
  void axesOf(const BVHNode* node, int axes[3]);
}

#endif
