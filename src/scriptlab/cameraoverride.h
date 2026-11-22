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
 *   - setcam_unlock / camunlock: the camera focus stays on the avatar.
 * While any of them holds, the camera turns around the avatar's middle
 * (its hip at rest, AvatarPoint) and panning cannot move it away.
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

    // what the RLV state asks of the camera
    struct Limits
    {
      double minDistance = 0;     // metres; 0: none
      double maxDistance = -1;    // metres; < 0: none (0 is a real maximum)
      bool focusOnAvatar = false; // setcam_unlock held
      QStringList objects;        // the objects that hold a camera limit, sorted
      QStringList ignored;        // held restrictions whose option is not a distance, as "behaviour:option"

      bool hasMin() const { return minDistance > 0; }
      bool hasMax() const { return maxDistance >= 0; }
      bool any() const { return hasMin() || hasMax() || focusOnAvatar; }
      bool operator==(const Limits& o) const
      {
        return minDistance == o.minDistance && maxDistance == o.maxDistance && focusOnAvatar == o.focusOnAvatar &&
               objects == o.objects && ignored == o.ignored;
      }
    };

    // reads the camera restrictions of every object in the state
    static Limits limitsFrom(const RlvState& state);
    // the constraint for the 3D view (distances in inches)
    static CameraConstraint constraintFor(const Limits& limits);
    // "3", "2.5", "0.25": a distance in metres for the text
    static QString metres(double value);

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
