/*
 * PinglyMotion
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

/***************************************************************************
 *   One place for the maths of rotation orders: the order type and the    *
 *   conversion between a quaternion and Euler angles. Used by the IK,     *
 *   the retarget code, the motion clean-up and the importers.             *
 ***************************************************************************/

#ifndef ROTATION_H
#define ROTATION_H

#include <QQuaternion>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace Rotation
{
  // The order of the rotation channels of a joint, as in a BVH file: the
  // rotation is R(first channel) * R(second) * R(third).
  enum class Order : quint8 { XYZ=1, ZYX, XZY, YZX, YXZ, ZXY };

  // The axes (0 = x, 1 = y, 2 = z) in channel order; false for an unknown order.
  inline bool axesOf(Order order,int axes[3])
  {
    switch(order)
    {
      case Order::XYZ: axes[0]=0; axes[1]=1; axes[2]=2; return true;
      case Order::ZYX: axes[0]=2; axes[1]=1; axes[2]=0; return true;
      case Order::XZY: axes[0]=0; axes[1]=2; axes[2]=1; return true;
      case Order::YZX: axes[0]=1; axes[1]=2; axes[2]=0; return true;
      case Order::YXZ: axes[0]=1; axes[1]=0; axes[2]=2; return true;
      case Order::ZXY: axes[0]=2; axes[1]=0; axes[2]=1; return true;
    }
    return false;
  }

  // Euler angles in degrees, in channel order: angles[0] turns around axes[0],
  // angles[1] around axes[1], angles[2] around axes[2] (the inverse of
  // R(axes[0]) * R(axes[1]) * R(axes[2])). q does not need length 1; a zero
  // quaternion gives zero angles.
  inline void eulerFromAxes(const QQuaternion& q,const int axes[3],double angles[3])
  {
    angles[0]=angles[1]=angles[2]=0.0;
    double w=q.scalar(),x=q.x(),y=q.y(),z=q.z();
    const double len=std::sqrt(w*w+x*x+y*y+z*z);
    if(len<1.0e-10) return;
    w/=len; x/=len; y/=len; z/=len;
    const double M[3][3]={
      {1-2*(y*y+z*z), 2*(x*y-z*w),   2*(x*z+y*w)},
      {2*(x*y+z*w),   1-2*(x*x+z*z), 2*(y*z-x*w)},
      {2*(x*z-y*w),   2*(y*z+x*w),   1-2*(x*x+y*y)}};
    const int i=axes[0],j=axes[1],k=axes[2];
    // +1 when the axes are in cyclic order (XYZ, YZX, ZXY), -1 otherwise
    const double s=((j-i+3)%3==1) ? 1.0 : -1.0;
    const double sb=std::clamp(s*M[i][k],-1.0,1.0);
    const double pi=3.14159265358979323846;
    double a,b=std::asin(sb),c;
    // the quaternion has float precision, so very close to a quarter turn of
    // the middle axis the first and last angles cannot be told apart any more
    if(std::sqrt(std::max(0.0,1.0-sb*sb))>1e-3)
    {
      a=std::atan2(-s*M[j][k],M[k][k]);
      c=std::atan2(-s*M[i][j],M[i][i]);
    }
    else
    {
      // gimbal lock: put all of the turn into the first angle
      c=0.0;
      b=sb>0.0 ? pi/2.0 : -pi/2.0;
      a=std::atan2(s*M[k][j],M[j][j]);
    }
    const double toDeg=180.0/pi;
    angles[0]=a*toDeg;
    angles[1]=b*toDeg;
    angles[2]=c*toDeg;
  }
}

#endif
