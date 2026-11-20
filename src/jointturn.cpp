/*
 * PinglyMotion - turning a joint around the axes of a frame (#88, step 4)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "jointturn.h"

#include <QMatrix3x3>
#include <QtMath>

#include <cmath>

namespace
{
  // axis indices (0=X, 1=Y, 2=Z) in the order, always three
  void axisOrder(const QString& order,int axes[3])
  {
    int count=0;
    bool used[3]={false,false,false};
    for(const QChar c : order)
    {
      const int axis=c.toUpper().unicode()-'X';
      if(axis<0 || axis>2 || used[axis] || count==3) continue;
      used[axis]=true;
      axes[count++]=axis;
    }
    for(int axis=0;axis<3 && count<3;axis++)
      if(!used[axis]) axes[count++]=axis;
  }

  QVector3D unitAxis(int axis)
  {
    QVector3D v;
    v[axis]=1.0f;
    return v;
  }

  // the angle closest to "near", moved by whole turns
  double unwrap(double angle,double near)
  {
    return angle+360.0*std::round((near-angle)/360.0);
  }
}

QQuaternion JointTurn::fromEuler(const QVector3D& degrees,const QString& order)
{
  int axes[3];
  axisOrder(order,axes);
  QQuaternion q;
  for(int n=0;n<3;n++)
    q=q*QQuaternion::fromAxisAndAngle(unitAxis(axes[n]),degrees[axes[n]]);
  return q.normalized();
}

// M = R_i(a) R_j(b) R_k(c) with (i,j,k) the axis order. s is +1 when the
// order is cyclic (XYZ, YZX, ZXY), -1 otherwise. Then
//   M[i][k] = s sin b,  M[j][k] = -s sin a cos b,  M[k][k] = cos a cos b,
//   M[i][j] = -s cos b sin c,  M[i][i] = cos b cos c
QVector3D JointTurn::toEuler(const QQuaternion& rotation,const QString& order,const QVector3D& near)
{
  int axes[3];
  axisOrder(order,axes);
  const int i=axes[0],j=axes[1],k=axes[2];
  const double s=((j-i+3)%3==1) ? 1.0 : -1.0;

  const QMatrix3x3 m=rotation.normalized().toRotationMatrix();
  const double sinB=qBound(-1.0,s*m(i,k),1.0);
  double a,b,c;
  b=std::asin(sinB);
  if(std::abs(sinB)<0.99999)
  {
    a=std::atan2(-s*m(j,k),m(k,k));
    c=std::atan2(-s*m(i,j),m(i,i));
  }
  else
  {
    // gimbal lock: only a+c (or a-c) is known; keep c as it was and find a
    // from M R_k(-c) = R_i(a) R_j(b), whose column j is R_i(a) e_j
    c=qDegreesToRadians(near[k]);
    const QMatrix3x3 back=QQuaternion::fromAxisAndAngle(unitAxis(k),-near[k]).toRotationMatrix();
    const QMatrix3x3 rest=m*back;
    a=std::atan2(s*rest(k,j),rest(j,j));
  }

  // the two answers: (a, b, c) and (a+180, 180-b, c+180)
  const double first[3]={qRadiansToDegrees(a),qRadiansToDegrees(b),qRadiansToDegrees(c)};
  const double second[3]={first[0]+180.0,180.0-first[1],first[2]+180.0};
  QVector3D best;
  double bestDistance=1e30;
  for(const double* answer : {first,second})
  {
    QVector3D result;
    double distance=0;
    for(int n=0;n<3;n++)
    {
      const double angle=unwrap(answer[n],near[axes[n]]);
      result[axes[n]]=angle;
      distance+=std::abs(angle-near[axes[n]]);
    }
    if(distance<bestDistance)
    {
      bestDistance=distance;
      best=result;
    }
  }
  return best;
}

QVector3D JointTurn::turn(const QVector3D& euler,const QString& order,const QQuaternion& parent,
                          const QQuaternion& view,Frame frame,const QVector3D& axis,float degrees)
{
  if(degrees==0.0f || axis.isNull()) return euler;
  const QQuaternion local=fromEuler(euler,order);
  QQuaternion turned;
  if(frame==JointFrame)
  {
    // around the joint's own axes: turn first, then the joint's rotation
    turned=local*QQuaternion::fromAxisAndAngle(axis,degrees);
  }
  else
  {
    // the axis in the figure's axes; the screen's axes are the figure's axes
    // turned by the camera
    const QVector3D figureAxis=(frame==ScreenFrame) ? view.inverted().rotatedVector(axis) : axis;
    // the figure sees the joint as parent*local; turned there by T gives
    // T*parent*local, so the new local is parent^-1*T*parent*local
    const QQuaternion t=QQuaternion::fromAxisAndAngle(figureAxis,degrees);
    turned=parent.inverted()*t*parent*local;
  }
  return toEuler(turned,order,euler);
}
