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
#include <QtGlobal>
#include <QMatrix4x4>
#include <QtMath>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "camera.h"
#include "scenestate.h"

#include <QDebug>

QMatrix4x4 shiftedPerspective(float fovYDegrees,const QRectF& view,const QPointF& center,
                               float nearPlane,float farPlane)
{
  QMatrix4x4 m;
  if(view.width()<=0 || view.height()<=0)
  {
    m.perspective(fovYDegrees,1.0f,nearPlane,farPlane);
    return m;
  }
  const float aspect=float(view.width()/view.height());
  // half the frustum's extent at the near plane, unshifted (the same
  // calculation QMatrix4x4::perspective() uses internally)
  const float halfHeight=nearPlane*std::tan(qDegreesToRadians(fovYDegrees)*0.5f);
  const float halfWidth=halfHeight*aspect;
  // how far the wanted centre is from the view's middle, as a fraction of
  // the half view (screen Y points down, NDC Y points up, hence the minus)
  const float ndcX=float((center.x()-view.center().x())/(view.width()*0.5));
  const float ndcY=-float((center.y()-view.center().y())/(view.height()*0.5));
  const float dx=-halfWidth*ndcX;
  const float dy=-halfHeight*ndcY;
  m.frustum(-halfWidth+dx,halfWidth+dx,-halfHeight+dy,halfHeight+dy,nearPlane,farPlane);
  return m;
}

CameraPosition::CameraPosition(float px,float py,float pz,float rx,float ry)
{
  panX=px;
  panY=py;
  panZ=pz;
  rotX=rx;
  rotY=ry;
  pivotDistance=pz;
}

CameraPosition::~CameraPosition()
{
}

Camera::Camera(QObject* parent)
  : QObject(parent)
{
  positions.append(new CameraPosition(0.0, 40.0, 100.0, 40.0, 37.0));
  positions.append(new CameraPosition(0.0, 40.0, 100.0, 48.0, 320.0));
  positions.append(new CameraPosition(0.0, 40.0, 100.0, 17.0, 180.0));
  positions.append(new CameraPosition(0.0, 15.0, 100.0, 80.0, 360.0));
  reset();
}

Camera::~Camera()
{
  while(!positions.isEmpty())
    delete positions.takeFirst();
}

QVector3D Camera::toCamera(const QVector3D& world) const
{
  // same turns as setModelView(): first around Y, then around X
  QMatrix4x4 m;
  m.rotate(rotX, 1, 0, 0);
  m.rotate(rotY, 0, 1, 0);
  return m.map(world);
}

QVector3D Camera::toWorld(const QVector3D& camera) const
{
  QMatrix4x4 m;
  m.rotate(-rotY, 0, 1, 0);
  m.rotate(-rotX, 1, 0, 0);
  return m.map(camera);
}

QVector3D Camera::pivot() const
{
  // the camera sits at pan in camera space and looks along -Z
  return toWorld(QVector3D(panX, panY, panZ - pivotDistance));
}

CameraDrag Camera::dragFor(Qt::MouseButton button, Qt::KeyboardModifiers keys)
{
  const bool shift = keys & Qt::ShiftModifier;
  const bool ctrl = keys & Qt::ControlModifier;
  const bool alt = keys & Qt::AltModifier;

  switch (button)
  {
    case Qt::LeftButton:
      if (!alt) return CameraDrag::Orbit;
      if (ctrl && shift) return CameraDrag::Pan;
      if (ctrl) return CameraDrag::Orbit;
      return CameraDrag::OrbitAndZoom;
    case Qt::RightButton:
      return CameraDrag::Pan;
    case Qt::MiddleButton:
      if (shift) return CameraDrag::Pan;
      if (ctrl) return CameraDrag::Zoom;
      return CameraDrag::Orbit;
    default:
      return CameraDrag::None;
  }
}

bool Camera::viewForKey(int key, Qt::KeyboardModifiers keys, float* height, float* angle)
{
  // only the number pad; with Num Lock off it sends the arrow block keys
  if (!(keys & Qt::KeypadModifier)) return false;
  const bool opposite = keys & Qt::ControlModifier;
  float h, a;
  switch (key)
  {
    case Qt::Key_1:
    case Qt::Key_End:
      h = 0; a = opposite ? 180 : 0;
      break;
    case Qt::Key_3:
    case Qt::Key_PageDown:
      h = 0; a = opposite ? 270 : 90;
      break;
    case Qt::Key_7:
    case Qt::Key_Home:
      if (opposite) return false;
      h = 90; a = 0;
      break;
    default:
      return false;
  }
  if (height) *height = h;
  if (angle) *angle = a;
  return true;
}

bool Camera::isFocusKey(int key, Qt::KeyboardModifiers keys)
{
  if (key == Qt::Key_F && !(keys & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)))
    return true;
  return (keys & Qt::KeypadModifier) &&
         (key == Qt::Key_Period || key == Qt::Key_Comma || key == Qt::Key_Delete);
}

void Camera::rotate(float x, float y)
{
  const QVector3D centre = pivot();

  // keep between -80 and 80 degrees (below and above the floor, never the
  // exact poles so the view cannot flip); a view outside that (e.g. straight
  // from above) is left smoothly instead of jumping back
  const float oldX = rotX;
  rotX = qBound(qMin(-80.0f, oldX), rotX + x, qMax(80.0f, oldX));
  rotY = std::fmod(rotY + y, 360.0f);
  if (rotY < 0) rotY += 360;

  // move the camera so the pivot stays in the middle, as far away as before
  const QVector3D c = toCamera(centre);
  panX = c.x();
  panY = c.y();
  panZ = c.z() + pivotDistance;
}

void Camera::pan(float x, float y, float z)
{
  panBy(-x / 2, y / 2);
  if (z != 0)
  {
    // forward/back: move towards the pivot, never through it
    const float newDistance = qBound(3.0f, pivotDistance + z, 1000.0f);
    panZ += newDistance - pivotDistance;
    pivotDistance = newDistance;
  }
}

void Camera::panBy(float x, float y)
{
  panX = qBound(-1000.0f, panX + x, 1000.0f);
  panY = qBound(-1000.0f, panY + y, 1000.0f);
}

void Camera::zoom(float factor)
{
  if (factor <= 0) return;
  pan(0, 0, pivotDistance * factor - pivotDistance);
}

void Camera::setView(float height, float angle)
{
  const QVector3D centre = pivot();
  rotX = qBound(0.0f, height, 90.0f);
  rotY = std::fmod(angle, 360.0f);
  if (rotY < 0) rotY += 360;
  const QVector3D c = toCamera(centre);
  panX = c.x();
  panY = c.y();
  panZ = c.z() + pivotDistance;
}

void Camera::setModelView()
{
  SceneState::loadIdentity();
  QMatrix4x4 lookAt;
  lookAt.lookAt(QVector3D(panX, panY, panZ), QVector3D(panX, panY, 0), QVector3D(0, 1, 0));
  SceneState::multMatrix(lookAt);
  SceneState::rotate(rotX, 1, 0, 0);
  SceneState::rotate(rotY, 0, 1, 0);
}

void Camera::reset()
{
  rotX = 10;
  rotY = 0;
  panX = 0;
  panY = 40;
  panZ = 100;
  pivotDistance = panZ;
}

void Camera::lookAt(float x, float y, float z, float distance)
{
  // the scene is rotated by rotY (around Y) and then rotX (around X) after
  // the camera is placed, so turn the point the same way first
  const float ry = rotY * 3.14159265f / 180.0f;
  const float rx = rotX * 3.14159265f / 180.0f;
  const float x1 = x * std::cos(ry) + z * std::sin(ry);
  const float z1 = -x * std::sin(ry) + z * std::cos(ry);
  const float y2 = y * std::cos(rx) - z1 * std::sin(rx);
  const float z2 = y * std::sin(rx) + z1 * std::cos(rx);

  panX = x1;
  panY = y2;
  panZ = z2 + distance;
  pivotDistance = distance;
}

void Camera::orbit(float height, float angle, float distance)
{
  reset();
  rotX = qBound(2.0f, height, 80.0f);
  angle = std::fmod(angle, 360.0f);
  if (angle < 0) angle += 360.0f;
  rotY = angle;
  panZ = qBound(10.0f, distance, 1000.0f);
  pivotDistance = panZ;
}

float Camera::distance() const
{
  return pivotDistance;
}

CameraPosition Camera::state() const
{
  CameraPosition position(panX, panY, panZ, rotX, rotY);
  position.pivotDistance = pivotDistance;
  return position;
}

void Camera::setState(const CameraPosition& position)
{
  panX = position.panX;
  panY = position.panY;
  panZ = position.panZ;
  rotX = position.rotX;
  rotY = position.rotY;
  pivotDistance = position.pivotDistance;
}

float Camera::xRotation() const
{
  return rotX;
}

float Camera::yRotation() const
{
  return rotY;
}

void Camera::storeCameraPosition(int num)
{
  CameraPosition* pos=positions[num];
  pos->panX=panX;
  pos->panY=panY;
  pos->panZ=panZ;
  pos->rotX=rotX;
  pos->rotY=rotY;
  pos->pivotDistance=pivotDistance;
}

void Camera::restoreCameraPosition(int num)
{
  const CameraPosition* pos=positions[num];
  panX=pos->panX;
  panY=pos->panY;
  panZ=pos->panZ;
  rotX=pos->rotX;
  rotY=pos->rotY;
  pivotDistance=pos->pivotDistance;
}
