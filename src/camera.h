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

#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QVector3D>

class CameraPosition
{
  public:
    CameraPosition(float px,float py,float pz,float rx,float ry);
    ~CameraPosition();

    float panX;
    float panY;
    float panZ;
    float rotX;
    float rotY;
    // how far in front of the camera the point is that it turns around
    float pivotDistance;
};

// what dragging the mouse does to the camera (docs/POSING_UX.md, "Mouse and keys")
enum class CameraDrag
{
  None,
  Orbit,          // turn around the pivot
  OrbitAndZoom,   // left/right turns around, up/down zooms (Second Life Alt + drag)
  Pan,            // move sideways/up
  Zoom            // up/down moves closer/further
};

class Camera : public QObject
{
  Q_OBJECT

  public:
    // The camera move for a drag with this mouse button and these keys.
    // Left drag without Alt gives Orbit; the view only uses that on empty
    // space (on a joint or prop the left button selects).
    //   simple:      left = orbit, right = pan
    //   Blender:     middle = orbit, Shift + middle = pan, Ctrl + middle = zoom
    //   Second Life: Alt + left = orbit and zoom, Ctrl + Alt + left = orbit,
    //                Ctrl + Alt + Shift + left = pan
    static CameraDrag dragFor(Qt::MouseButton button, Qt::KeyboardModifiers keys);
    // Blender number pad views: 1 front, 3 side (from the figure's right),
    // 7 top; with Ctrl the opposite side (back, left; no bottom view).
    // Returns false for other keys.
    static bool viewForKey(int key, Qt::KeyboardModifiers keys, float* height, float* angle);
    // Blender number pad "." (focus on the selected joint)
    static bool isFocusKey(int key, Qt::KeyboardModifiers keys);

    Camera(QObject* parent = 0);
    ~Camera();

    // turn the camera around the point in the middle of the view (the
    // pivot): x = degrees up/down, y = degrees around
    void rotate(float x, float y);
    // old pan: x/y move the camera sideways/up (half units), z moves it
    // forward/back (zoom)
    void pan(float x, float y, float z);
    // move the view sideways/up by this much in world units (the pivot moves along)
    void panBy(float x, float y);
    // move towards the pivot (factor < 1) or away from it (factor > 1)
    void zoom(float factor);
    // look at the pivot from a fixed direction: height = degrees above the
    // floor (0..90, 90 = straight down), angle = degrees around (0 = front)
    void setView(float height, float angle);
    void setModelView();
    void reset();

    // move the camera so it looks straight at a point, from a distance
    void lookAt(float x, float y, float z, float distance);
    // look at the middle of the figure: height = degrees above the floor,
    // angle = degrees around the figure (0 = default front view), distance from the middle
    void orbit(float height, float angle, float distance);
    // distance from the camera to the pivot
    float distance() const;
    // the point in the world the camera turns around
    QVector3D pivot() const;
    float xRotation() const;
    float yRotation() const;

  public slots:
    void storeCameraPosition(int num);
    void restoreCameraPosition(int num);

  protected:
    float rotX,rotY;
    float panX,panY,panZ;
    float pivotDistance;

    // turn a world point into camera space and back (without the pan)
    QVector3D toCamera(const QVector3D& world) const;
    QVector3D toWorld(const QVector3D& camera) const;

    QList<CameraPosition*> positions;
};

#endif
