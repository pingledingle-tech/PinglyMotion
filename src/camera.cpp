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

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "camera.h"
#include "scenestate.h"

#include <QDebug>

CameraPosition::CameraPosition(float px,float py,float pz,float rx,float ry)
{
  panX=px;
  panY=py;
  panZ=pz;
  rotX=rx;
  rotY=ry;
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

void Camera::rotate(float x, float y)
{
  rotX += x;
  if (rotX < 2) rotX = 2;
  if (rotX > 80) rotX = 80;
  rotY += y;
  if (rotY < 0) rotY += 360;
  if (rotY > 360) rotY -= 360;
}

void Camera::pan(float x, float y, float z)
{
  panX -= x/2;
  if (panX < -500) panX = -500;
  if (panX > 500) panX = 500;
  panY += y/2;
  if (panY < 5) panY = 5;
  if (panY > 500) panY = 500;
  panZ += z;
  if (panZ < 10) panZ = 10;
  if (panZ > 1000) panZ = 1000;
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
}

void Camera::orbit(float height, float angle, float distance)
{
  reset();
  rotX = qBound(2.0f, height, 80.0f);
  angle = std::fmod(angle, 360.0f);
  if (angle < 0) angle += 360.0f;
  rotY = angle;
  panZ = qBound(10.0f, distance, 1000.0f);
}

float Camera::distance() const
{
  return panZ;
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
}

void Camera::restoreCameraPosition(int num)
{
  const CameraPosition* pos=positions[num];
  panX=pos->panX;
  panY=pos->panY;
  panZ=pos->panZ;
  rotX=pos->rotX;
  rotY=pos->rotY;
}
