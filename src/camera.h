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
#include <QMatrix4x4>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector3D>

// A perspective projection, shifted so a point straight ahead of the camera
// projects to "center" instead of the middle of "view" (the lens shift used
// to keep the figure in the middle of the free area when a floating panel
// covers part of the 3D view, docs/archive/PANELS_PLAN.md, job P5). With
// center == view.center() this is the same matrix as plain
// QMatrix4x4::perspective(fovYDegrees, aspect, nearPlane, farPlane).
QMatrix4x4 shiftedPerspective(float fovYDegrees,const QRectF& view,const QPointF& center,
                               float nearPlane,float farPlane);

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

// Limits a Script Lab script puts on the camera (RLV camera restrictions,
// src/scriptlab/cameraoverride.h). Distances are in world units (inches).
// While a constraint is on, the camera keeps to it after every move.
struct CameraConstraint
{
  float minDistance = 0;          // 0: no minimum
  float maxDistance = 0;          // 0: no maximum
  bool turnAroundPoint = false;   // the camera turns around `point` (the avatar), panning does not move it away
  QVector3D point;
  float fieldOfView = 0;          // vertical field of view in degrees; 0: the camera's own (Camera::DefaultFieldOfView)
  // with turnAroundPoint: the camera is held at `eye`, looking at `point` (it cannot be turned;
  // the distance limits still hold along that line)
  bool fixedEye = false;
  QVector3D eye;

  bool operator==(const CameraConstraint& o) const
  {
    return minDistance == o.minDistance && maxDistance == o.maxDistance &&
           turnAroundPoint == o.turnAroundPoint && point == o.point && fieldOfView == o.fieldOfView &&
           fixedEye == o.fixedEye && eye == o.eye;
  }
  bool operator!=(const CameraConstraint& o) const { return !(*this == o); }
};

// The mouse and key habits of the Camera and keys dialog (src/camerahabitspec.h,
// UserSettings::cameraHabit(), docs/UX_DECISIONS.md section 6c, UX phase 4a)
enum class CameraHabit
{
  PinglyMotion,   // "pinglymotion", the default
  Maya,           // "maya"
  Blender,        // "blender"
  SecondLife      // "sl"
};

// What pressing a mouse button does in the 3D view (the mockup's gestureFor())
struct CameraGesture
{
  // the camera move while the mouse is dragged (None: the camera does not move)
  CameraDrag drag = CameraDrag::None;
  // true: a left press picks the joint, prop or rotation ring under it (a joint or the
  // empty space is picked on release); with a drag as well, moving the mouse first
  // turns it into that camera drag (the PinglyMotion habit's "Drag empty space").
  // false with a drag: only the camera moves, nothing is picked.
  bool picks = false;

  bool operator==(const CameraGesture& o) const { return drag == o.drag && picks == o.picks; }
};

// What a left drag does in the 3D view (the view toolbar's tools, UX phase 4b, docs/UX_DECISIONS.md
// 6b): Pick follows the camera habit; the other three turn, move or zoom the camera with a plain
// left drag in every habit, and nothing is picked
enum class ViewTool
{
  Pick,    // "pick", the default (key V)
  Orbit,   // "orbit": Look around (key O)
  Pan,     // "pan": Move the view (key M)
  Zoom     // "zoom" (key Z)
};

// The name of the view under the view-direction widget (UX phase 4b)
enum class ViewName
{
  Front,
  Back,
  LeftSide,    // the camera on the figure's left (+X)
  RightSide,   // the camera on the figure's right (-X), the Side view (3)
  Top,
  Below,
  Free         // any other direction
};

class Camera : public QObject
{
  Q_OBJECT

  public:
    // the habit of a UserSettings::cameraHabit() id; an unknown id gives PinglyMotion
    static CameraHabit habitFor(const QString& id);
    // What a press of this mouse button with these keys does in this habit
    // (docs/mockup/index.html, gestureFor()):
    //   PinglyMotion: left = pick, or orbit when dragged; right = pan
    //   Maya:         Alt + left = orbit, Alt + middle = pan, Alt + right = zoom; left = pick
    //   Blender:      middle = orbit, Shift + middle = pan, Ctrl + middle = zoom; left = pick
    //   Second Life:  Alt + left = orbit and zoom, Ctrl + Alt + left = orbit,
    //                 Ctrl + Alt + Shift + left = pan; left = pick
    // The mouse wheel zooms in every habit.
    static CameraGesture gestureFor(CameraHabit habit, Qt::MouseButton button, Qt::KeyboardModifiers keys);
    // The same with a view tool chosen (the mockup's gestureFor()): a left press with neither Alt
    // nor Ctrl does the tool's camera drag and picks nothing; ViewTool::Pick and every other
    // press give the habit's gesture
    static CameraGesture gestureFor(CameraHabit habit, ViewTool tool, Qt::MouseButton button, Qt::KeyboardModifiers keys);
    // The view tool keys in the 3D view: V, O, M, Z (alone or with Shift, as the mockup takes
    // either case); false for any other key
    static bool toolForKey(int key, Qt::KeyboardModifiers keys, ViewTool* tool);
    // the id the setting stores ("pick", "orbit", "pan", "zoom") and back; an unknown id gives Pick
    static const char* toolId(ViewTool tool);
    static ViewTool toolFor(const QString& id);
    // The view's name for a camera turn: height and angle as setView() takes them. Within half a
    // degree of 0, 90, 180 or 270 around and no more than 10.5 degrees up or down (the home view
    // looks from 10 degrees up) it is Front, Right side, Back or Left side; from 89.5 degrees up
    // Top, from 89.5 degrees down Below; anything else is Free.
    static ViewName viewNameFor(float height, float angle);
    // Second Life: Alt + click on a joint turns the camera around it
    static bool focusesOnAltClick(CameraHabit habit) { return habit == CameraHabit::SecondLife; }
    // PinglyMotion: a double click on a joint turns the camera around it
    static bool focusesOnDoubleClick(CameraHabit habit) { return habit == CameraHabit::PinglyMotion; }
    // Views by key: number pad 1 front, 3 side (from the figure's right), 7 top;
    // with Ctrl the opposite side (back, left; no bottom view). In the 3D view the
    // keys 1, 3 and 7 above the letters do the same (the mockup). Returns false for
    // other keys.
    static bool viewForKey(int key, Qt::KeyboardModifiers keys, float* height, float* angle);
    // Focus on the picked joint: F in every habit, number pad "." in the Blender habit
    static bool isFocusKey(CameraHabit habit, int key, Qt::KeyboardModifiers keys);
    // Frame the whole figure: Shift+F
    static bool isWholeFigureKey(int key, Qt::KeyboardModifiers keys);
    // Home view: number pad 0 (the 0 above the letters is View > Home view's own key)
    static bool isHomeKey(int key, Qt::KeyboardModifiers keys);
    // Look around with keys: Alt + arrow keys turn the camera by this many degrees
    // (up/down: height, left/right: around), the mockup's 10 degree steps
    static constexpr float KeyTurnDegrees = 10.0f;
    static bool turnForKey(int key, Qt::KeyboardModifiers keys, float* up, float* around);

    // the vertical field of view of the 3D view, in degrees (the user cannot change it)
    static constexpr float DefaultFieldOfView = 60.0f;

    Camera(QObject* parent = 0);
    ~Camera();

    // turn the camera around the point in the middle of the view (the
    // pivot): x = degrees up/down, y = degrees around; the height angle
    // stays between -80 and 80 degrees, so the camera can go below the floor
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
    // the same, but height may go down to -90 (the view-direction widget's "Look from below")
    void lookFrom(float height, float angle);
    void setModelView();
    void reset();

    // move the camera so it looks straight at a point, from a distance
    void lookAt(float x, float y, float z, float distance);
    // Frame the whole figure (Shift+F): look at the middle of the box from low to high,
    // keeping the turn, from just far enough that a ball around the box (FrameMargin
    // times its half diagonal) fits the view; aspect = view width / height
    static constexpr float FrameMargin = 1.15f;
    void frameBox(const QVector3D& low, const QVector3D& high, float aspect);
    // the distance frameBox() uses for a ball of this radius
    float frameDistance(float radius, float aspect) const;
    // Picked joint (F key) and set_camera with a joint: how far from the joint the camera
    // stays, in scene units: 18 for the head and the face bones (as AnimationView::isFaceBone()
    // names them), 20 for a hand, else 60. Inline, so the control commands need no camera.cpp
    static float focusDistance(const QString& joint)
    {
      if (joint == QLatin1String("head") || joint.startsWith(QLatin1String("mFace")) ||
          joint.startsWith(QLatin1String("mEye")) || joint == QLatin1String("mSkull"))
        return 18.0f;
      if (joint.startsWith(QLatin1String("mHand")) || joint == QLatin1String("lHand") || joint == QLatin1String("rHand"))
        return 20.0f;
      return 60.0f;
    }
    // look at the middle of the figure: height = degrees above the floor,
    // angle = degrees around the figure (0 = default front view), distance from the middle
    void orbit(float height, float angle, float distance);
    // distance from the camera to the pivot
    float distance() const;
    // the point in the world the camera turns around
    QVector3D pivot() const;
    float xRotation() const;
    float yRotation() const;
    // the whole camera (turn, pan and pivot), to put it back exactly after a
    // picture taken from another side (screenshot views, issue #129)
    CameraPosition state() const;
    void setState(const CameraPosition& position);

    // Script Lab camera override: the first setConstraint() keeps the camera
    // as it is now, then the limits hold at once and after every move;
    // another setConstraint() while one is on only changes the limits.
    // clearConstraint() puts back exactly the kept camera (moves made while
    // the limits were on are not kept), so the override never changes the
    // user's own camera.
    void setConstraint(const CameraConstraint& constraint);
    void clearConstraint();
    bool constrained() const { return m_constrained; }
    // the vertical field of view to draw with, in degrees: the constraint's while it sets one,
    // otherwise DefaultFieldOfView
    float fieldOfView() const
    {
      return m_constrained && m_constraint.fieldOfView > 0 ? m_constraint.fieldOfView : DefaultFieldOfView;
    }
    CameraConstraint constraint() const { return m_constraint; }

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

  private:
    // moves the camera so it keeps to the constraint (does nothing without one)
    void applyConstraint();
    // places the camera so it looks at a world point from a distance, keeping its turn
    void place(const QVector3D& point, float distance);

    bool m_constrained = false;
    CameraConstraint m_constraint;
    float m_savedPanX = 0, m_savedPanY = 0, m_savedPanZ = 0, m_savedRotX = 0, m_savedRotY = 0, m_savedPivot = 0;
};

#endif
