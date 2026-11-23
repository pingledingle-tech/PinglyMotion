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

CameraHabit Camera::habitFor(const QString& id)
{
  if (id == QLatin1String("maya")) return CameraHabit::Maya;
  if (id == QLatin1String("blender")) return CameraHabit::Blender;
  if (id == QLatin1String("sl")) return CameraHabit::SecondLife;
  return CameraHabit::PinglyMotion;
}

CameraGesture Camera::gestureFor(CameraHabit habit, Qt::MouseButton button, Qt::KeyboardModifiers keys)
{
  const bool shift = keys & Qt::ShiftModifier;
  const bool ctrl = keys & Qt::ControlModifier;
  const bool alt = keys & Qt::AltModifier;
  const bool left = button == Qt::LeftButton;
  CameraGesture gesture;

  switch (habit)
  {
    case CameraHabit::Maya:
      if (alt && left) gesture.drag = CameraDrag::Orbit;
      else if (alt && button == Qt::MiddleButton) gesture.drag = CameraDrag::Pan;
      else if (alt && button == Qt::RightButton) gesture.drag = CameraDrag::Zoom;
      else gesture.picks = left;
      break;
    case CameraHabit::Blender:
      if (button == Qt::MiddleButton)
        gesture.drag = shift ? CameraDrag::Pan : ctrl ? CameraDrag::Zoom : CameraDrag::Orbit;
      else gesture.picks = left;
      break;
    case CameraHabit::SecondLife:
      if (left && alt)
        gesture.drag = ctrl && shift ? CameraDrag::Pan : ctrl ? CameraDrag::Orbit : CameraDrag::OrbitAndZoom;
      else gesture.picks = left;
      break;
    case CameraHabit::PinglyMotion:
      if (button == Qt::RightButton) gesture.drag = CameraDrag::Pan;
      else if (left)
      {
        gesture.drag = CameraDrag::Orbit;
        gesture.picks = true;
      }
      break;
  }
  return gesture;
}

bool Camera::viewForKey(int key, Qt::KeyboardModifiers keys, float* height, float* angle)
{
  // the number pad (with Num Lock off it sends the arrow block keys), or the
  // digits above the letters with nothing but Ctrl held
  const bool keypad = keys & Qt::KeypadModifier;
  if (keys & (Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier)) return false;
  const bool opposite = keys & Qt::ControlModifier;
  float h, a;
  switch (key)
  {
    case Qt::Key_1:
      h = 0; a = opposite ? 180 : 0;
      break;
    case Qt::Key_End:
      if (!keypad) return false;
      h = 0; a = opposite ? 180 : 0;
      break;
    case Qt::Key_3:
      h = 0; a = opposite ? 270 : 90;
      break;
    case Qt::Key_PageDown:
      if (!keypad) return false;
      h = 0; a = opposite ? 270 : 90;
      break;
    case Qt::Key_7:
      if (opposite) return false;
      h = 90; a = 0;
      break;
    case Qt::Key_Home:
      if (!keypad || opposite) return false;
      h = 90; a = 0;
      break;
    default:
      return false;
  }
  if (height) *height = h;
  if (angle) *angle = a;
  return true;
}

bool Camera::isFocusKey(CameraHabit habit, int key, Qt::KeyboardModifiers keys)
{
  if (key == Qt::Key_F && !(keys & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)))
    return true;
  return habit == CameraHabit::Blender && (keys & Qt::KeypadModifier) &&
         !(keys & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) &&
         (key == Qt::Key_Period || key == Qt::Key_Comma || key == Qt::Key_Delete);
}

bool Camera::isWholeFigureKey(int key, Qt::KeyboardModifiers keys)
{
  return key == Qt::Key_F &&
         (keys & ~Qt::KeypadModifier) == Qt::ShiftModifier;
}

bool Camera::isHomeKey(int key, Qt::KeyboardModifiers keys)
{
  return (keys & Qt::KeypadModifier) &&
         !(keys & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) &&
         (key == Qt::Key_0 || key == Qt::Key_Insert);
}

bool Camera::turnForKey(int key, Qt::KeyboardModifiers keys, float* up, float* around)
{
  // only Alt (arrow keys may carry the keypad flag on some systems)
  if ((keys & ~Qt::KeypadModifier) != Qt::AltModifier) return false;
  float u = 0, a = 0;
  switch (key)
  {
    case Qt::Key_Left:  a = -KeyTurnDegrees; break;
    case Qt::Key_Right: a = KeyTurnDegrees; break;
    case Qt::Key_Up:    u = KeyTurnDegrees; break;
    case Qt::Key_Down:  u = -KeyTurnDegrees; break;
    default: return false;
  }
  if (up) *up = u;
  if (around) *around = a;
  return true;
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
  applyConstraint();
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
  applyConstraint();
}

void Camera::panBy(float x, float y)
{
  panX = qBound(-1000.0f, panX + x, 1000.0f);
  panY = qBound(-1000.0f, panY + y, 1000.0f);
  applyConstraint();
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
  applyConstraint();
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
  applyConstraint();
}

void Camera::lookAt(float x, float y, float z, float distance)
{
  place(QVector3D(x, y, z), distance);
  applyConstraint();
}

float Camera::frameDistance(float radius, float aspect) const
{
  // the narrower of the two half angles of the view decides
  const float halfV = qDegreesToRadians(fieldOfView()) * 0.5f;
  float half = halfV;
  if (aspect > 0)
    half = qMin(halfV, std::atan(std::tan(halfV) * aspect));
  return radius * FrameMargin / std::sin(half);
}

void Camera::frameBox(const QVector3D& low, const QVector3D& high, float aspect)
{
  const QVector3D middle = (low + high) * 0.5f;
  // a single point still gets a sensible distance (a tenth of the home view's)
  const float radius = qMax((high - low).length() * 0.5f, 5.0f);
  place(middle, frameDistance(radius, aspect));
  applyConstraint();
}

void Camera::place(const QVector3D& point, float distance)
{
  // the scene is rotated by rotY (around Y) and then rotX (around X) after
  // the camera is placed, so turn the point the same way first
  const float x = point.x(), y = point.y(), z = point.z();
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

void Camera::setConstraint(const CameraConstraint& constraint)
{
  if (!m_constrained)
  {
    // keep the user's camera to put it back exactly in clearConstraint()
    m_savedPanX = panX; m_savedPanY = panY; m_savedPanZ = panZ;
    m_savedRotX = rotX; m_savedRotY = rotY; m_savedPivot = pivotDistance;
  }
  m_constrained = true;
  m_constraint = constraint;
  applyConstraint();
}

void Camera::clearConstraint()
{
  if (!m_constrained) return;
  m_constrained = false;
  m_constraint = CameraConstraint();
  panX = m_savedPanX; panY = m_savedPanY; panZ = m_savedPanZ;
  rotX = m_savedRotX; rotY = m_savedRotY; pivotDistance = m_savedPivot;
}

void Camera::applyConstraint()
{
  if (!m_constrained) return;
  float d = pivotDistance;
  if (m_constraint.maxDistance > 0) d = qMin(d, m_constraint.maxDistance);
  if (m_constraint.minDistance > 0) d = qMax(d, m_constraint.minDistance);
  if (m_constraint.turnAroundPoint && m_constraint.fixedEye)
  {
    // look from the eye at the point: the turn that puts the camera there
    // (the camera sits at pivot + (-d cos x sin y, d sin x, d cos x cos y))
    const QVector3D v = m_constraint.eye - m_constraint.point;
    const float length = v.length();
    if (length > 0.001f)
    {
      rotX = qBound(-89.0f, float(qRadiansToDegrees(std::asin(qBound(-1.0f, v.y() / length, 1.0f)))), 89.0f);
      rotY = float(qRadiansToDegrees(std::atan2(-v.x(), v.z())));
      if (rotY < 0) rotY += 360;
      d = length;
      if (m_constraint.maxDistance > 0) d = qMin(d, m_constraint.maxDistance);
      if (m_constraint.minDistance > 0) d = qMax(d, m_constraint.minDistance);
    }
    place(m_constraint.point, d);
  }
  else if (m_constraint.turnAroundPoint)
    place(m_constraint.point, d);
  else if (d != pivotDistance)
  {
    // closer or further along the line of sight; the pivot stays where it is
    panZ += d - pivotDistance;
    pivotDistance = d;
  }
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
  applyConstraint();
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
  applyConstraint();
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
  applyConstraint();
}
