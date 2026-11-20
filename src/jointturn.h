/*
 * PinglyMotion - turning a joint around the axes of a frame (#88, step 4)
 *
 * The trackball on the selected-joint card turns the joint around the axes
 * of one of three frames ("Turn around: Joint / Avatar / Screen"):
 *
 *   Joint   the joint's own axes
 *   Avatar  the axes of the figure (BVH axes: X sideways, Y up, Z forward)
 *   Screen  the axes of the screen (X right, Y up, Z out of the screen)
 *
 * A joint stores its rotation as three angles in the order of its BVH
 * channels (for example Z, X, Y); each axis rotation is multiplied on the
 * right, as in the 3D view, the IK and the .anim export (BVH::bvhPoseNodeQuaternions).
 * The functions here turn that rotation and give the three angles back,
 * close to the old ones (no jump from 179 to -179 degrees).
 *
 * No Qt widgets: the tests check this without a display.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef JOINTTURN_H
#define JOINTTURN_H

#include <QQuaternion>
#include <QString>
#include <QVector3D>

namespace JointTurn
{
  enum Frame
  {
    JointFrame=0,
    AvatarFrame,
    ScreenFrame
  };

  // the rotation of three angles (degrees, x/y/z) in the given axis order,
  // for example "ZXY"; axes missing from the order are added at the end
  QQuaternion fromEuler(const QVector3D& degrees,const QString& order);

  // three angles (degrees) in the given order for the rotation; of the two
  // possible answers the one closest to "near" is given, each angle moved by
  // whole turns to be close to the angle in "near"
  QVector3D toEuler(const QQuaternion& rotation,const QString& order,const QVector3D& near=QVector3D());

  // the joint's angles after turning it "degrees" around "axis", where axis is
  // given in the axes of "frame".
  //   euler   the joint's angles now, in "order"
  //   parent  how the parent joint is turned in the figure (all joints above,
  //           multiplied from the top down); identity for the top joint
  //   view    how the figure is turned on the screen (the camera)
  QVector3D turn(const QVector3D& euler,const QString& order,const QQuaternion& parent,
                 const QQuaternion& view,Frame frame,const QVector3D& axis,float degrees);
}

#endif
