/*
 * PinglyMotion
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

/***************************************************************************
 *   Small helpers for walking the skeleton (a tree of BVHNode), shared by *
 *   the describe code, the control commands and the view.                 *
 ***************************************************************************/

#ifndef SKELETONWALK_H
#define SKELETONWALK_H

#include <QHash>
#include <QList>
#include <QMatrix4x4>
#include <QString>
#include <QVector3D>

#include "bvhnode.h"

namespace SkeletonWalk
{
  // The nodes below "node" (itself included), parents before children, in the
  // order they are drawn. End Sites are left out unless includeEnd is set.
  inline void collect(BVHNode* node, QList<BVHNode*>& list, bool includeEnd = false)
  {
    if (!node) return;
    if (includeEnd || node->type != BVH_END) list << node;
    for (int i = 0; i < node->numChildren(); i++) collect(node->child(i), list, includeEnd);
  }

  // The first node with this name, or nullptr. End Sites count only with includeEnd.
  inline BVHNode* findNode(BVHNode* root, const QString& name, bool includeEnd = true)
  {
    QList<BVHNode*> all;
    collect(root, all, includeEnd);
    for (BVHNode* node : all)
      if (node->name() == name) return node;
    return nullptr;
  }

  // The world matrix of every node at "frame" (0-based): the node's offset,
  // then each rotation channel in file order, as the 3D view draws the figure.
  // rotationOf(node) gives the node's rotation in that frame. The root is
  // placed relative to "parent" (the hip position belongs there). End Sites
  // are left out of "out" when skipEnd is set.
  template <class RotationOf>
  void place(BVHNode* node, const QMatrix4x4& parent, const RotationOf& rotationOf,
             QHash<BVHNode*, QMatrix4x4>& out, bool skipEnd = false)
  {
    if (!node || (skipEnd && node->type == BVH_END)) return;
    QMatrix4x4 m = parent;
    m.translate(node->offset[0], node->offset[1], node->offset[2]);
    const QVector3D rotation = rotationOf(node);
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
    for (int i = 0; i < node->numChildren(); i++) place(node->child(i), m, rotationOf, out, skipEnd);
  }

  inline QString capitalize(QString text)
  {
    if (!text.isEmpty()) text[0] = text[0].toUpper();
    return text;
  }
}

#endif
