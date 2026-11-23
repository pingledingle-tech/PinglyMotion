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

#ifndef IKTREE_H
#define IKTREE_H

#include "bvhnode.h"

#include <cstddef>

#include <QQuaternion>
#include <QVector3D>

// rows * cols as a size: the product is made in a wide type before it is used
// as the size of an array (an int product could overflow first)
inline std::size_t ikMatrixCells(int rows, int cols)
{
  if (rows <= 0 || cols <= 0) return 0;
  return std::size_t(rows) * std::size_t(cols);
}

struct IKBone
{
  BVHNode *node;
  double weight;
  QVector3D offset;
  QVector3D pos;
  QQuaternion lRot;  // local rotation (not always of length 1, see solveJoint())
  QQuaternion gRot;  // global rotation
  int parent;        // -1 for the root
  int numChildren;
  // Size of the array is the maximum number of children
  int child[32];
};

class IKTree
{
  public:
    explicit IKTree(BVHNode *root = nullptr);

    void set(BVHNode *root);
    void setGoal(int frame, const QString& name);
    void solve(int frame);

  protected:
    enum {AXIS_X, AXIS_Y, AXIS_Z};

    int numBones;
    // how far the end sites were from their goals after the last solve (inches)
    double lastDistance = 0;
    // Size of the array is the maximum number of bones
    IKBone bone[512];

    // withIK: also apply the IK rotation the 3D view shows now
    void reset(int frame, bool withIK = false);
    void addJoint(BVHNode *node, int parent);
    void toEuler(const QQuaternion &q, BVHOrderType order, double &x, double &y, double &z);
    void updateBones(int startIndex);
};

#endif
