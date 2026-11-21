/*
 * PinglyMotion Script Lab - effects of a script run in the 3D view (issue 293)
 *
 * Off by default. While it is on (the Live page's "Show effects in 3D view"),
 * each Live run is read with LabEffects::fromRun() and:
 *   - the newest animation still playing at the end of the run is loaded from
 *     its file (LabEffects::findAnimation: next to the script, in the watched
 *     folder, then in the examples folder) into a preview animation of its own,
 *     with a BVH reader of its own, and played from the frame it has reached
 *     after the run's simulated seconds, on in real time;
 *   - worn objects and RLV locks become markers at their attachment points.
 * The 3D view draws the preview figure instead of the user's animation while
 * there is one, and the markers on top. Nothing here touches the user's
 * animations, the scene or any file except reading the animation file: turning
 * it off, or a run with no animation playing at the end, simply shows the
 * user's animation again, as it was.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef EFFECTSPREVIEW_H
#define EFFECTSPREVIEW_H

#include <QElapsedTimer>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QTimer>

#include <memory>

#include "labeffects.h"

class Animation;
class BVH;
class LabWatcher;

class EffectsPreview : public QObject
{
  Q_OBJECT

  public:
    explicit EffectsPreview(QObject* parent = nullptr);
    ~EffectsPreview() override;

    // off by default; turning it off removes the preview and the markers at once
    void setEnabled(bool on);
    bool enabled() const { return m_enabled; }

    // the examples folder searched after the script's folders; by default the folder
    // "examples" next to the program's data folder (the Windows zip and the source tree)
    void setExamplesFolder(const QString& folder) { m_examples = folder; }
    QString examplesFolder() const;

    // Shows what one run answer did (does nothing while off). folders: where animations are
    // looked for, in order (the examples folder is added at the end).
    void showRun(const QJsonObject& run, const QStringList& folders);
    // the newest Live result of the watcher: the script's folder and the watched folder are searched
    void showLatest(const LabWatcher* watcher);
    // removes the preview and the markers (the 3D view shows the user's animation again)
    void clear();

    // the preview animation to draw, or nullptr when there is none
    Animation* animation() const { return m_animation.get(); }
    QString animationName() const { return m_name; }
    QString animationPath() const { return m_path; }
    // the frame the preview shows now (0-based), -1 without a preview
    int frame() const;

    struct Marker
    {
      enum Kind { Object, Lock };
      Kind kind = Object;
      int point = 0;
      QStringList joints;   // LabEffects::jointsForPoint()
      QString label;        // object name or point name
    };
    const QList<Marker>& markers() const { return m_markers; }

    // what is shown, in words, for the panel; the first line says Error when something could not be shown
    const QStringList& lines() const { return m_lines; }
    bool hasError() const { return m_error; }

    // plays on by this many seconds (the timer does this in real time; tests call it directly)
    void advance(double seconds);
    // false stops the real-time timer (tests); true by default
    void setRealTime(bool on);

  signals:
    // the preview, its frame, the markers or the lines changed: the view draws again
    void changed();

  private:
    void tick();
    void updateFrame();
    void dropAnimation();

    bool m_enabled = false;
    bool m_realTime = true;
    QString m_examples;
    // the reader is declared first, so it outlives the animation it loaded
    std::unique_ptr<BVH> m_bvh;
    std::unique_ptr<Animation> m_animation;
    QString m_name;
    QString m_path;
    double m_elapsed = 0;           // seconds since the animation started: simulated at the run's end, then real
    QElapsedTimer m_clock;
    QTimer m_timer;
    QList<Marker> m_markers;
    QStringList m_lines;
    bool m_error = false;
};

#endif
