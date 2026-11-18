/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Copyright (C) 2006 by Vinay Pulim.
 * All rights reserved.
 *
 */

#include <cmath>
#include <vector>

#include <QtMath>

#include "iktree.h"
#include "usersettings.h"

// Inverse kinematics for hands and feet. The joint tree and the goals come
// from QAvimator; until #97 this used the MoTo maths library (libquat), now
// Qt's QQuaternion and QVector3D. Since #116 it builds each joint's rotation
// in the same order as the 3D view, so it aims at where the hands and feet
// really are.
//
// Since #127 the solver works on the joints' own rotation channels (the
// angles the user sees) with damped least squares:
// - joint limits are simple bounds per channel, so the result never flips
//   to another set of Euler angles;
// - when a goal is out of reach the limb stretches towards it and stops at
//   its limits, instead of swinging around;
// - each solve starts from the previous solution, so a small slider step
//   gives a small change of pose.

static const QVector3D origin(0,0,0);
static const QVector3D xAxis(1,0,0);
static const QVector3D yAxis(0,1,0);
static const QVector3D zAxis(0,0,1);
static const QQuaternion identity(1,0,0,0);  // (scalar, x, y, z)

// below this, a length counts as zero (the value libquat used)
static const double EPSILON = 1.0e-10;

// rotation of "angle" radians around "axis" (the axis does not need length 1)
static QQuaternion axisAngle(const QVector3D &axis, double angle)
{
  const double len = std::sqrt(double(axis.x())*axis.x() + double(axis.y())*axis.y() + double(axis.z())*axis.z());
  if (len < EPSILON) return identity;
  const double s = std::sin(angle * 0.5) / len;
  return QQuaternion(float(std::cos(angle * 0.5)), float(axis.x()*s), float(axis.y()*s), float(axis.z()*s));
}

// rotates v by q; q does not need length 1 (only its direction counts)
static QVector3D rotate(const QQuaternion &q, const QVector3D &v)
{
  const double d = double(q.x())*q.x() + double(q.y())*q.y() + double(q.z())*q.z() + double(q.scalar())*q.scalar();
  if (d < EPSILON * EPSILON) return v;
  // rotation matrix of q scaled by 1/|q|^2, as libquat's MT_Transform did
  const double s = 2.0 / d;
  const double xs = q.x()*s, ys = q.y()*s, zs = q.z()*s;
  const double wx = q.scalar()*xs, wy = q.scalar()*ys, wz = q.scalar()*zs;
  const double xx = q.x()*xs, xy = q.x()*ys, xz = q.x()*zs;
  const double yy = q.y()*ys, yz = q.y()*zs, zz = q.z()*zs;
  return QVector3D(float((1.0-(yy+zz))*v.x() + (xy-wz)*v.y() + (xz+wy)*v.z()),
                   float((xy+wz)*v.x() + (1.0-(xx+zz))*v.y() + (yz-wx)*v.z()),
                   float((xz-wy)*v.x() + (yz+wx)*v.y() + (1.0-(xx+yy))*v.z()));
}

IKTree::IKTree(BVHNode *root)
{
  set(root);
}

void IKTree::set(BVHNode *root)
{
  numBones = 0;
  lastDistance = 0;
  if (root)
    addJoint(root, -1);
}

void IKTree::setGoal(int frame,const QString& name)
{
  // the goal is where the end site is drawn now, with any IK from other
  // hands or feet included, so turning IK on does not move anything
  reset(frame, true);

  for (int i=0; i<numBones; i++) {
    if (bone[i].node->name()==name) {
      // the end site is the joint's first child (see tools/make_bento_skeleton.py)
      int j = bone[i].child[0];
      BVHNode *n = bone[j].node;
      n->ikGoalPos[0] = bone[j].pos[0];
      n->ikGoalPos[1] = bone[j].pos[1];
      n->ikGoalPos[2] = bone[j].pos[2];
      n->ikGoalDir[0] = bone[j].pos[0] - bone[i].pos[0];
      n->ikGoalDir[1] = bone[j].pos[1] - bone[i].pos[1];
      n->ikGoalDir[2] = bone[j].pos[2] - bone[i].pos[2];
    }
  }
}

void IKTree::reset(int frame, bool withIK)
{
  for (int i=0; i<numBones; i++) {
    bone[i].pos = origin;
    bone[i].lRot = identity;
    bone[i].gRot = identity;
    BVHNode *n = bone[i].node;
    QVector3D rotation = n->frameData(frame).rotation();
    if (withIK && n->ikOn) rotation += n->ikRotation;
    QVector3D position = n->frameData(frame).position();

    for (int k=0; k<n->numChannels; k++) {  // rotate each axis in order
      QQuaternion q = identity;
      switch (n->channelType[k]) {
        case BVH_XROT: q = axisAngle(xAxis, qDegreesToRadians(double(rotation.x()))); break;
        case BVH_YROT: q = axisAngle(yAxis, qDegreesToRadians(double(rotation.y()))); break;
        case BVH_ZROT: q = axisAngle(zAxis, qDegreesToRadians(double(rotation.z()))); break;
        case BVH_XPOS: bone[i].pos[0] = position.x(); break;
        case BVH_YPOS: bone[i].pos[1] = position.y(); break;
        case BVH_ZPOS: bone[i].pos[2] = position.z(); break;
      }
      // same order as the 3D view (AnimationView) and the exporter: the
      // channels are applied in file order, each on the right (#116)
      bone[i].lRot = bone[i].lRot * q;
    }
  }
  updateBones(0);
}

void IKTree::addJoint(BVHNode *node, int parent)
{
  bone[numBones].node = node;
  bone[numBones].offset = QVector3D((float)node->offset[0],
                                    (float)node->offset[1],
                                    (float)node->offset[2]);
  bone[numBones].weight = node->ikWeight;
  bone[numBones].parent = parent;
  bone[numBones].numChildren = 0;

  int myNumBones = numBones++;
  for (int i=0; i<node->numChildren(); i++) {
    bone[myNumBones].child[bone[myNumBones].numChildren++] = numBones;
    addJoint(node->child(i), myNumBones);
  }
}

// Euler angles in degrees for the given channel order: the inverse of
// reset(), where the rotation is R(first channel) * R(second) * R(third),
// as in the 3D view. q does not need length 1.
void IKTree::toEuler(const QQuaternion &q, BVHOrderType order,
                     double &x, double &y, double &z)
{
  // the axes in file order (0 = x, 1 = y, 2 = z)
  int a0 = 0, a1 = 1, a2 = 2;
  switch (order) {
    case BVH_XYZ: a0 = 0; a1 = 1; a2 = 2; break;
    case BVH_XZY: a0 = 0; a1 = 2; a2 = 1; break;
    case BVH_YXZ: a0 = 1; a1 = 0; a2 = 2; break;
    case BVH_YZX: a0 = 1; a1 = 2; a2 = 0; break;
    case BVH_ZXY: a0 = 2; a1 = 0; a2 = 1; break;
    case BVH_ZYX: a0 = 2; a1 = 1; a2 = 0; break;
  }

  // rotation matrix of q (normalised)
  double w = q.scalar(), qx = q.x(), qy = q.y(), qz = q.z();
  const double len = std::sqrt(w*w + qx*qx + qy*qy + qz*qz);
  double angles[3] = {0, 0, 0};
  if (len >= EPSILON) {
    w /= len; qx /= len; qy /= len; qz /= len;
    const double m[3][3] = {
      {1 - 2*(qy*qy + qz*qz), 2*(qx*qy - w*qz),     2*(qx*qz + w*qy)},
      {2*(qx*qy + w*qz),     1 - 2*(qx*qx + qz*qz), 2*(qy*qz - w*qx)},
      {2*(qx*qz - w*qy),     2*(qy*qz + w*qx),     1 - 2*(qx*qx + qy*qy)}
    };
    // +1 when the axes are in cyclic order (xyz, yzx, zxy), -1 otherwise
    const double s = ((a1 - a0 + 3) % 3 == 1) ? 1.0 : -1.0;
    angles[a1] = std::asin(qBound(-1.0, s * m[a0][a2], 1.0));
    angles[a0] = std::atan2(-s * m[a1][a2], m[a2][a2]);
    angles[a2] = std::atan2(-s * m[a0][a1], m[a0][a0]);
  }
  x = qRadiansToDegrees(angles[0]);
  y = qRadiansToDegrees(angles[1]);
  z = qRadiansToDegrees(angles[2]);
}

void IKTree::updateBones(int i)
{
  for (int j=0; j<bone[i].numChildren; j++) {
    int k = bone[i].child[j];
    bone[k].gRot = bone[i].gRot * bone[i].lRot;
    bone[k].pos = bone[i].pos + rotate(bone[k].gRot, bone[k].offset);
    updateBones(k);
  }
}

// ---------------------------------------------------------------------------
// The solver (#127). Plain double maths, small and self-contained.

namespace {

struct V3
{
  double x = 0, y = 0, z = 0;
  V3() = default;
  V3(double a, double b, double c) : x(a), y(b), z(c) {}
  V3 operator+(const V3 &o) const { return V3(x+o.x, y+o.y, z+o.z); }
  V3 operator-(const V3 &o) const { return V3(x-o.x, y-o.y, z-o.z); }
  V3 operator*(double s) const { return V3(x*s, y*s, z*s); }
  double length() const { return std::sqrt(x*x + y*y + z*z); }
};

static V3 cross(const V3 &a, const V3 &b)
{
  return V3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

// 3x3 rotation matrix, row major
struct M3
{
  double m[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
  V3 operator*(const V3 &v) const
  {
    return V3(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
              m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
              m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z);
  }
  M3 operator*(const M3 &o) const
  {
    M3 r;
    for (int i=0; i<3; i++)
      for (int j=0; j<3; j++)
        r.m[i][j] = m[i][0]*o.m[0][j] + m[i][1]*o.m[1][j] + m[i][2]*o.m[2][j];
    return r;
  }
  V3 column(int j) const { return V3(m[0][j], m[1][j], m[2][j]); }
};

// rotation of "degrees" around the x (0), y (1) or z (2) axis
static M3 axisRotation(int axis, double degrees)
{
  const double a = qDegreesToRadians(degrees);
  const double c = std::cos(a), s = std::sin(a);
  M3 r;
  switch (axis) {
    case 0: r.m[1][1] = c; r.m[1][2] = -s; r.m[2][1] = s; r.m[2][2] = c; break;
    case 1: r.m[0][0] = c; r.m[0][2] = s; r.m[2][0] = -s; r.m[2][2] = c; break;
    default: r.m[0][0] = c; r.m[0][1] = -s; r.m[1][0] = s; r.m[1][1] = c; break;
  }
  return r;
}

static int rotationAxis(BVHChannelType type)
{
  switch (type) {
    case BVH_XROT: return 0;
    case BVH_YROT: return 1;
    case BVH_ZROT: return 2;
    default: return -1;
  }
}

// solves A x = b for a symmetric positive definite A (n x n, row major) by
// Cholesky; A and b are overwritten. Returns false if A is not positive definite.
static bool solveSPD(double *A, double *b, int n)
{
  for (int j=0; j<n; j++) {
    double d = A[j*n+j];
    for (int k=0; k<j; k++) d -= A[j*n+k]*A[j*n+k];
    if (d <= 0) return false;
    d = std::sqrt(d);
    A[j*n+j] = d;
    for (int i=j+1; i<n; i++) {
      double v = A[i*n+j];
      for (int k=0; k<j; k++) v -= A[i*n+k]*A[j*n+k];
      A[i*n+j] = v / d;
    }
  }
  for (int i=0; i<n; i++) {        // L y = b
    double v = b[i];
    for (int k=0; k<i; k++) v -= A[i*n+k]*b[k];
    b[i] = v / A[i*n+i];
  }
  for (int i=n-1; i>=0; i--) {     // L^T x = y
    double v = b[i];
    for (int k=i+1; k<n; k++) v -= A[k*n+i]*b[k];
    b[i] = v / A[i*n+i];
  }
  return true;
}

// solver settings
const int MAX_ITERATIONS = 60;
const double DAMPING = 1.0;          // starting damping (times the cost of each channel)
const double MIN_DAMPING = 1.0e-3;
const double MAX_STEP = 5.0;         // degrees per channel and iteration
const double DONE_STEP = 1.0e-4;     // degrees: stop when no channel moves more
const double WRIST_WEIGHT = 0.5;     // how much the hand/foot direction counts, end site = 1
const double CONTINUITY = 0.05;      // cost of moving away from the previous solution
// per update, a channel moves at most MIN_CAP degrees, plus CAP_PER_INCH
// for every inch the goals got further away since the last solve
const double MIN_CAP = 8.0;
const double CAP_PER_INCH = 6.0;

} // namespace

void IKTree::solve(int frame)
{
  // which end sites have a goal, and which joints carry them
  QList<int> effectors;
  std::vector<char> inChain(numBones, 0);
  for (int i=0; i<numBones; i++) {
    if (bone[i].numChildren != 0 || !bone[i].node->ikOn || bone[i].parent < 0) continue;
    effectors << i;
    for (int j = bone[i].parent; j >= 0; j = bone[j].parent) inChain[j] = 1;
  }

  // the angles of every joint: key frame, plus the last IK solution for the
  // joints IK moves (warm start), inside the limits
  const bool limits = UserSettings::jointLimits();
  std::vector<V3> keyed(numBones), angle(numBones), lo(numBones), hi(numBones);
  std::vector<V3> rootPos(1);
  for (int i=0; i<numBones; i++) {
    BVHNode *n = bone[i].node;
    const QVector3D r = n->frameData(frame).rotation();
    keyed[i] = V3(r.x(), r.y(), r.z());
    angle[i] = keyed[i];
    lo[i] = V3(-1e9, -1e9, -1e9);
    hi[i] = V3(1e9, 1e9, 1e9);
    if (limits) {
      for (int k=0; k<n->numChannels; k++) {
        const int a = rotationAxis(n->channelType[k]);
        if (a < 0) continue;
        double *l = a == 0 ? &lo[i].x : a == 1 ? &lo[i].y : &lo[i].z;
        double *h = a == 0 ? &hi[i].x : a == 1 ? &hi[i].y : &hi[i].z;
        const double key = a == 0 ? keyed[i].x : a == 1 ? keyed[i].y : keyed[i].z;
        // a joint the pose itself puts outside its limits may stay there,
        // but IK does not push it further out
        *l = qMin(n->channelMin[k], key);
        *h = qMax(n->channelMax[k], key);
      }
    }
    if (inChain[i] && bone[i].weight > 0) {
      angle[i] = keyed[i] + V3(n->ikRotation.x(), n->ikRotation.y(), n->ikRotation.z());
      angle[i] = V3(qBound(lo[i].x, angle[i].x, hi[i].x),
                    qBound(lo[i].y, angle[i].y, hi[i].y),
                    qBound(lo[i].z, angle[i].z, hi[i].z));
    }
  }
  if (numBones) {
    const QVector3D p = bone[0].node->frameData(frame).position();
    rootPos[0] = V3(p.x(), p.y(), p.z());
  }

  // the channels IK may turn: joint, axis
  struct Channel { int joint; int axis; };
  std::vector<Channel> channels;
  auto ref = [](std::vector<V3> &v, const Channel &ch) -> double & {
    return ch.axis == 0 ? v[ch.joint].x : ch.axis == 1 ? v[ch.joint].y : v[ch.joint].z;
  };
  auto value = [&ref](std::vector<V3> &v, const Channel &ch) { return ref(v, ch); };
  for (int i=0; i<numBones; i++) {
    if (!inChain[i] || bone[i].weight <= 0) continue;
    BVHNode *n = bone[i].node;
    for (int k=0; k<n->numChannels; k++) {
      const int a = rotationAxis(n->channelType[k]);
      if (a >= 0) channels.push_back({i, a});
    }
  }

  // targets: each end site to its goal, and the hand/foot joint to where it
  // was when the goal was set (keeps the direction of the hand or foot)
  struct Target { int bone; V3 goal; double weight; bool endSite; };
  std::vector<Target> targets;
  for (int e : effectors) {
    BVHNode *n = bone[e].node;
    const V3 goal(n->ikGoalPos[0], n->ikGoalPos[1], n->ikGoalPos[2]);
    const V3 dir(n->ikGoalDir[0], n->ikGoalDir[1], n->ikGoalDir[2]);
    targets.push_back({e, goal, 1.0, true});
    targets.push_back({bone[e].parent, goal - dir, WRIST_WEIGHT, false});
  }

  // only the joints on the way to a goal and the end sites themselves
  // matter; the bones are stored parents first (addJoint())
  std::vector<int> needed;
  for (int i=0; i<numBones; i++) {
    const bool isEffector = bone[i].numChildren == 0 && effectors.contains(i);
    if (inChain[i] || isEffector) needed.push_back(i);
  }
  std::vector<V3> pos(numBones);
  std::vector<M3> worldRot(numBones);   // orientation after the joint's own rotation
  std::vector<V3> channelAxis(channels.size());   // world axis of each channel
  std::vector<int> channelIndex(numBones * 3, -1);
  for (int c=0; c<int(channels.size()); c++) channelIndex[channels[c].joint*3 + channels[c].axis] = c;

  auto forward = [&]() {
    for (int i : needed) {
      const int p = bone[i].parent;
      M3 r;
      if (p < 0) {
        pos[i] = rootPos[0];
      } else {
        r = worldRot[p];
        pos[i] = pos[p] + r * V3(bone[i].offset.x(), bone[i].offset.y(), bone[i].offset.z());
      }
      // this joint's channels in order; note the world axis of each
      BVHNode *n = bone[i].node;
      for (int k=0; k<n->numChannels; k++) {
        const int a = rotationAxis(n->channelType[k]);
        if (a < 0) continue;
        const int c = channelIndex[i*3 + a];
        if (c >= 0) channelAxis[c] = r.column(a);
        const double v = a == 0 ? angle[i].x : a == 1 ? angle[i].y : angle[i].z;
        r = r * axisRotation(a, v);
      }
      worldRot[i] = r;
    }
  };

  // does turning joint j move bone b (is j an ancestor of b)?
  auto moves = [&](int j, int b) {
    for (int k = bone[b].parent; k >= 0; k = bone[k].parent)
      if (k == j) return true;
    return false;
  };

  // how far the end sites are from their goals now, before solving; the
  // part of that which is new since the last solve is how much the user
  // changed the pose. A small change may only change the IK pose a little,
  // so a goal out of reach cannot make a limb jump to another pose.
  forward();
  double distance = 0;
  for (const Target &tg : targets)
    if (tg.endSite) distance += (tg.goal - pos[tg.bone]).length();
  const double cap = MIN_CAP + CAP_PER_INCH * qMax(0.0, distance - lastDistance);
  for (const Channel &ch : channels) {
    double &l = ref(lo, ch), &h = ref(hi, ch);
    const double v = value(angle, ch);
    l = qMax(l, v - cap);
    h = qMin(h, v + cap);
  }

  const int rows = int(targets.size()) * 3;
  const int cols = int(channels.size());
  if (rows > 0 && cols > 0) {
    // the weights from SL.lim, squared: the limb joints (0.6-0.8) do most of
    // the work, the spine (0.2) and hands/feet (0.1) only a little.
    // cost[c] = 1 / weight^2: how much it costs to turn that channel.
    std::vector<double> cost(cols), start(cols);
    for (int c=0; c<cols; c++) {
      const double w = bone[channels[c].joint].weight;
      cost[c] = 1.0 / (w * w);
      start[c] = value(angle, channels[c]);
    }
    std::vector<double> J(rows * cols), err(rows), H(cols * cols), g(cols);
    std::vector<char> held(cols);

    // what the solver minimises:
    //   |goal error|^2 + CONTINUITY * sum cost * (angle - start)^2
    // "start" is the previous solution, so a goal out of reach does not send
    // the limb to a far-away pose. Angles in radians for the second term.
    auto objective = [&]() {
      forward();
      double f = 0;
      for (const Target &tg : targets) {
        const V3 e = (tg.goal - pos[tg.bone]) * tg.weight;
        f += e.x*e.x + e.y*e.y + e.z*e.z;
      }
      for (int c=0; c<cols; c++) {
        const double d = qDegreesToRadians(value(angle, channels[c]) - start[c]);
        f += CONTINUITY * cost[c] * d * d;
      }
      return f;
    };

    // Levenberg-Marquardt: a step is taken only when it lowers the
    // objective; otherwise the damping grows and the step gets shorter.
    // So the solver always settles, even where two joint axes line up
    // (a straight arm) and many poses are almost equally good.
    double damping = DAMPING;
    double f = objective();
    std::vector<double> before(cols);
    for (int it=0; it<MAX_ITERATIONS; it++) {
      // forward() has run for the current angles (objective())
      // Jacobian: how each target point moves per radian of each channel
      for (int t=0; t<int(targets.size()); t++) {
        const Target &tg = targets[t];
        const V3 p = pos[tg.bone];
        const V3 e = (tg.goal - p) * tg.weight;
        err[t*3] = e.x; err[t*3+1] = e.y; err[t*3+2] = e.z;
        for (int c=0; c<cols; c++) {
          V3 d;
          if (moves(channels[c].joint, tg.bone))
            d = cross(channelAxis[c], p - pos[channels[c].joint]) * tg.weight;
          J[(t*3)*cols + c] = d.x;
          J[(t*3+1)*cols + c] = d.y;
          J[(t*3+2)*cols + c] = d.z;
        }
      }
      for (int c=0; c<cols; c++) before[c] = value(angle, channels[c]);

      bool accepted = false;
      double moved = 0;
      for (int attempt=0; attempt<10 && !accepted; attempt++) {
        // channels at a limit that the step pushes further out are held
        // and the others solved again (at most 3 times)
        std::fill(held.begin(), held.end(), 0);
        bool solved = false;
        for (int pass=0; pass<3; pass++) {
          for (int c=0; c<cols; c++) {
            for (int d=c; d<cols; d++) {
              double v = 0;
              if (!held[c] && !held[d])
                for (int r=0; r<rows; r++) v += J[r*cols+c] * J[r*cols+d];
              H[c*cols+d] = H[d*cols+c] = v;
            }
            if (held[c]) { H[c*cols+c] = 1; g[c] = 0; continue; }
            H[c*cols+c] += (CONTINUITY + damping) * cost[c];
            double v = 0;
            for (int r=0; r<rows; r++) v += J[r*cols+c] * err[r];
            g[c] = v - CONTINUITY * cost[c] * qDegreesToRadians(before[c] - start[c]);
          }
          if (!solveSPD(H.data(), g.data(), cols)) break;
          solved = true;
          bool changed = false;
          for (int c=0; c<cols; c++) {
            if (held[c]) continue;
            if ((before[c] <= value(lo, channels[c]) + 1e-9 && g[c] < 0) ||
                (before[c] >= value(hi, channels[c]) - 1e-9 && g[c] > 0)) {
              held[c] = 1;
              changed = true;
            }
          }
          if (!changed) break;
        }
        if (!solved) break;

        // at most MAX_STEP degrees per channel, the whole step scaled together
        double largest = 0;
        for (int c=0; c<cols; c++) if (!held[c]) largest = qMax(largest, qAbs(qRadiansToDegrees(g[c])));
        const double scale = largest > MAX_STEP ? MAX_STEP / largest : 1.0;
        moved = 0;
        for (int c=0; c<cols; c++) {
          const double next = held[c] ? before[c]
                            : qBound(value(lo, channels[c]),
                                     before[c] + qRadiansToDegrees(g[c]) * scale,
                                     value(hi, channels[c]));
          moved = qMax(moved, qAbs(next - before[c]));
          ref(angle, channels[c]) = next;
        }
        const double fNew = objective();
        if (fNew < f) {
          f = fNew;
          accepted = true;
          damping = qMax(damping * 0.3, MIN_DAMPING);
        } else {
          for (int c=0; c<cols; c++) ref(angle, channels[c]) = before[c];
          damping *= 4;
        }
      }
      if (!accepted) {
        objective();  // forward() for the angles kept
        break;
      }
      if (moved < DONE_STEP) break;
    }
  }

  forward();
  lastDistance = 0;
  for (const Target &tg : targets)
    if (tg.endSite) lastDistance += (tg.goal - pos[tg.bone]).length();

  // the IK rotation is what the solution adds to the key frame; joints on the
  // way to a goal are marked, so the 3D view draws them with IK
  for (int i=0; i<numBones; i++) {
    if (!inChain[i]) continue;
    BVHNode *n = bone[i].node;
    n->ikOn = true;
    n->ikRotation = QVector3D(float(angle[i].x - keyed[i].x),
                              float(angle[i].y - keyed[i].y),
                              float(angle[i].z - keyed[i].z));
  }

  // the solver's own joint positions, for callers that look at them
  reset(frame, true);
}
