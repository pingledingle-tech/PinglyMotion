/*
 * PinglyMotion Script Lab - RLV camera limits shown in the 3D view (job 7b)
 *
 * The RLV state records the camera restrictions a script holds, for example
 * @setcam_avdistmax:3=n. This class reads them and, only while it is switched
 * on (the Commands page's "Show camera limits in 3D view", off at start),
 * gives the 3D view a CameraConstraint (camera.h) to keep to. It also says in
 * plain words what limits the camera, for the panel.
 *
 * What is shown:
 *   - setcam_avdistmin / camdistmin: the camera stays at least this far
 *     from the avatar (metres; with several objects the largest wins);
 *   - setcam_avdistmax / camdistmax: at most this far (the smallest wins;
 *     a maximum below the minimum wins over it);
 *   - setcam_unlock / camunlock: the camera focus stays on the avatar;
 *   - setcam_fovmin / setcam_fovmax: the vertical field of view in radians
 *     at least / at most this (the view's own is 60 degrees);
 *   - camzoommax / camzoommin: the old zoom limits, a multiplier of the
 *     default 60 degrees (zoom 2 = 30 degrees): camzoommax:2 keeps the field
 *     of view at 30 degrees or more, camzoommin:2 at 30 degrees or less.
 *   - setcam_focusoffset:x/y/z: the camera looks at the avatar's middle
 *     moved by this many metres (Second Life axes: x forward, y left, z up);
 *   - setcam_eyeoffset:x/y/z (times setcam_eyeoffsetscale): the camera is
 *     held at the avatar's middle moved by this offset, looking at the focus;
 *     it cannot be turned or zoomed, only the distance limits still hold.
 *   With several objects holding an offset, the first one in the restriction
 *   list is used.
 * Force commands (job 7c), kept by the RLV state as recorded values:
 *   - setcam_fov:<radians>=force: the field of view the view starts from instead
 *     of its own 60 degrees (the min and max limits above still hold);
 *   - setcam_focus:<avatar key>=force: the camera focus stays on the avatar (the
 *     3D view has only the avatar, so another target is named as not shown);
 *   - setcam_focusoffset, setcam_eyeoffset, setcam_eyeoffsetscale with =force:
 *     used as the held offsets above when no object holds that offset;
 *   - setcam_mode=force: not shown (the 3D view always looks from outside the
 *     avatar), only named in the text.
 *   A forced value lasts until another force command changes it, its object's
 *   @clear, Reset or a mode switch.
 * While a distance limit, an offset or setcam_unlock holds, the camera turns around the
 * avatar's middle (its hip at rest, avatarPoint()) and panning cannot move it
 * away. With several objects the strictest limit wins; a maximum below the
 * minimum wins over it.
 *
 * The user's camera is never changed for good: Camera::setConstraint() keeps
 * the camera as it was, and switching the box off, @clear, Reset or the last
 * limit going away puts it back exactly.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef CAMERAOVERRIDE_H
#define CAMERAOVERRIDE_H

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVector3D>

#include "camera.h"

class RlvState;

class CameraOverride : public QObject
{
  Q_OBJECT

  public:
    // world units are inches
    static constexpr float InchesPerMetre = 39.3700787f;
    // the point the camera turns around while limited: the avatar's hip at rest (bin/data/SLFemale.bvh)
    static QVector3D avatarPoint() { return QVector3D(0.0f, 43.528519f, 0.0f); }
    // the 3D view cannot come closer than this (Camera::pan), so a smaller maximum is shown as this
    static constexpr float ClosestDistance = 3.0f;
    // the field of view the 3D view can show, in degrees
    static constexpr double NarrowestView = 1.0;
    static constexpr double WidestView = 170.0;

    // what the RLV state asks of the camera
    struct Limits
    {
      double minDistance = 0;     // metres; 0: none
      double maxDistance = -1;    // metres; < 0: none (0 is a real maximum)
      bool focusOnAvatar = false; // setcam_unlock held
      double minView = 0;         // field of view in degrees; 0: none
      double maxView = 0;         // degrees; 0: none
      bool hasFocusOffset = false;
      QVector3D focusOffset;      // metres, Second Life axes (x forward, y left, z up)
      bool hasEyeOffset = false;
      QVector3D eyeOffset;        // metres, Second Life axes
      bool hasEyeScale = false;
      double eyeScale = 1;        // setcam_eyeoffsetscale
      bool hasForcedView = false;
      double forcedView = 0;      // setcam_fov=force, degrees
      bool forcedFocus = false;   // setcam_focus=force on the avatar
      bool focusOffsetForced = false;   // the focus offset comes from =force
      bool eyeOffsetForced = false;
      bool eyeScaleForced = false;
      bool hasForcedMode = false;
      QString forcedMode;         // setcam_mode=force option (not shown)
      QStringList objects;        // the objects that hold a camera limit, sorted
      QStringList ignored;        // held restrictions whose option cannot be used, as "behaviour:option"

      bool hasMin() const { return minDistance > 0; }
      bool hasMax() const { return maxDistance >= 0; }
      bool hasViewLimit() const { return minView > 0 || maxView > 0; }
      // the camera turns around the avatar (a distance limit or setcam_unlock)
      // the view's field of view is set by a limit or a force command
      bool setsView() const { return hasViewLimit() || hasForcedView; }
      bool aroundAvatar() const
      {
        return hasMin() || hasMax() || focusOnAvatar || forcedFocus || hasFocusOffset || hasEyeOffset;
      }
      bool any() const { return aroundAvatar() || setsView(); }
      // the field of view the limits leave of the view's own 60 degrees (or of a forced one)
      double fieldOfView() const;
      bool operator==(const Limits& o) const
      {
        return minDistance == o.minDistance && maxDistance == o.maxDistance && focusOnAvatar == o.focusOnAvatar &&
               minView == o.minView && maxView == o.maxView && hasFocusOffset == o.hasFocusOffset &&
               focusOffset == o.focusOffset && hasEyeOffset == o.hasEyeOffset && eyeOffset == o.eyeOffset &&
               hasEyeScale == o.hasEyeScale && eyeScale == o.eyeScale && hasForcedView == o.hasForcedView &&
               forcedView == o.forcedView && forcedFocus == o.forcedFocus &&
               focusOffsetForced == o.focusOffsetForced && eyeOffsetForced == o.eyeOffsetForced &&
               eyeScaleForced == o.eyeScaleForced && hasForcedMode == o.hasForcedMode && forcedMode == o.forcedMode && objects == o.objects &&
               ignored == o.ignored;
      }
    };

    // reads the camera restrictions of every object in the state
    static Limits limitsFrom(const RlvState& state);
    // the constraint for the 3D view (distances in inches)
    static CameraConstraint constraintFor(const Limits& limits);
    // "3", "2.5", "0.25": a distance in metres for the text
    static QString metres(double value);
    // "30", "28.6": degrees for the text, at most one decimal
    static QString degrees(double value);
    // an offset in Second Life axes (metres) as a point in the 3D view's world (inches): x forward is
    // the figure's front (+Z), y left is the figure's left (+X), z up is +Y
    static QVector3D toWorld(const QVector3D& secondLife);

    explicit CameraOverride(QObject* parent = nullptr);

    // the RLV state to follow (not owned; the control API owns it)
    void setState(RlvState* state);

    // off by default
    void setEnabled(bool on);
    bool enabled() const { return m_enabled; }

    Limits limits() const { return m_limits; }
    // switched on and the state holds a camera limit: the 3D view keeps to constraint()
    bool active() const { return m_enabled && m_limits.any(); }
    CameraConstraint constraint() const { return constraintFor(m_limits); }

    // what limits the camera, in words, for the panel (empty while off and nothing is held)
    QStringList lines() const;

  signals:
    // limits() or enabled() changed: the 3D view applies or removes the constraint
    void changed();

  private:
    void update();

    QPointer<RlvState> m_state;
    QList<QMetaObject::Connection> m_connections;
    bool m_enabled = false;
    Limits m_limits;
};

#endif
