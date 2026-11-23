/*
 * PinglyMotion - from the 3D view's clip space to widget pixels
 *
 * One place for the sum the view does for ring letters, ring drags and joint
 * pulls. The widget size is turned into double before it is multiplied, so a
 * huge size can never overflow an int (CodeQL rule
 * cpp/integer-multiplication-cast-to-long). tests/jointrings checks it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef VIEWPROJECTION_H
#define VIEWPROJECTION_H

#include <QPointF>
#include <QVector4D>

namespace ViewProjection
{
  // a clip-space point (projection * view * point) in widget pixels, top left
  // 0,0; depth gets the normalised depth (-1 near .. 1 far) when asked for
  inline QPointF toWidget(const QVector4D& clip,int width,int height,float* depth=nullptr)
  {
    const double w=clip.w()!=0.0f ? double(clip.w()) : 1.0;
    if(depth) *depth=float(double(clip.z())/w);
    const double x=double(clip.x())/w*0.5+0.5;
    const double y=0.5-double(clip.y())/w*0.5;
    return QPointF(x*double(width),y*double(height));
  }
}

#endif
