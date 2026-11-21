/*
 * PinglyMotion Script Lab - effects of a script run in the 3D view (issue 293)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "effectspreview.h"

#include <QDir>
#include <QFileInfo>

#include "animation.h"
#include "bvh.h"
#include "constants.h"
#include "labwatcher.h"

EffectsPreview::EffectsPreview(QObject* parent)
  : QObject(parent)
{
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &EffectsPreview::tick);
}

EffectsPreview::~EffectsPreview()
{
  // the animation first, then the reader it was loaded with
  m_animation.reset();
  m_bvh.reset();
}

QString EffectsPreview::examplesFolder() const
{
  if (!m_examples.isEmpty()) return m_examples;
  return QDir::cleanPath(Constants::pinglyMotionDataPath() + QStringLiteral("/../examples"));
}

void EffectsPreview::setEnabled(bool on)
{
  if (m_enabled == on) return;
  m_enabled = on;
  if (!on) clear();
}

void EffectsPreview::setRealTime(bool on)
{
  m_realTime = on;
  if (!on) m_timer.stop();
  else if (m_animation) m_timer.start();
}

int EffectsPreview::frame() const
{
  return m_animation ? m_animation->getFrame() : -1;
}

void EffectsPreview::dropAnimation()
{
  m_timer.stop();
  m_animation.reset();
  m_bvh.reset();
  m_name.clear();
  m_path.clear();
  m_elapsed = 0;
}

void EffectsPreview::clear()
{
  const bool had = m_animation || !m_markers.isEmpty() || !m_lines.isEmpty();
  dropAnimation();
  m_markers.clear();
  m_lines.clear();
  m_error = false;
  if (had) emit changed();
}

void EffectsPreview::showLatest(const LabWatcher* watcher)
{
  if (!m_enabled) return;
  if (!watcher || !watcher->watching())
  {
    clear();
    return;
  }
  const QJsonObject last = watcher->lastResult();
  if (!last.contains("result"))
  {
    // nothing ran (no script yet, an error, an unreadable file): show nothing old
    clear();
    if (!last.isEmpty())
    {
      m_lines << tr("The script did not run, so the 3D view shows your animation.");
      emit changed();
    }
    return;
  }
  QStringList folders;
  const QString script = watcher->absolutePath(last.value("file").toString());
  if (!script.isEmpty()) folders << QFileInfo(script).absolutePath();
  folders << watcher->folder();
  showRun(last.value("result").toObject(), folders);
}

void EffectsPreview::showRun(const QJsonObject& run, const QStringList& folders)
{
  if (!m_enabled) return;
  dropAnimation();
  m_markers.clear();
  m_lines.clear();
  m_error = false;

  const LabEffects::Effects effects = LabEffects::fromRun(run);

  // the newest animation still playing
  if (!effects.playing.isEmpty())
  {
    const LabEffects::Playing& newest = effects.playing.last();
    QStringList search = folders;
    search << examplesFolder();
    const LabEffects::Found found = LabEffects::findAnimation(newest.name, search);
    QString problem = found.error;
    if (found.ok())
    {
      problem = BVH::checkFile(found.path).error;
      if (problem.isEmpty())
      {
        m_bvh = std::make_unique<BVH>();
        m_animation = std::make_unique<Animation>(m_bvh.get(), found.path);
        if (m_animation->getNumberOfFrames() < 1)
        {
          dropAnimation();
          problem = tr("Animation \"%1\" has no frames.").arg(newest.name);
        }
      }
      else
        problem = tr("Animation \"%1\" could not be read: %2").arg(newest.name, problem);
    }
    if (m_animation)
    {
      m_name = newest.name;
      m_path = found.path;
      m_elapsed = qMax(0.0, effects.endTime - newest.started);
      updateFrame();
      m_lines << tr("Playing in the 3D view: %1 (started at %2 s).").arg(newest.name, QString::number(newest.started, 'f', 3));
      m_clock.start();
      if (m_realTime) m_timer.start(qBound(16, int(m_animation->frameTime() * 1000.0), 1000));
    }
    else
    {
      m_error = true;
      m_lines << tr("Error: %1").arg(problem);
    }
    if (effects.playing.size() > 1)
    {
      QStringList others;
      for (int i = 0; i < effects.playing.size() - 1; ++i) others << effects.playing.at(i).name;
      m_lines << tr("Also playing, not shown (the 3D view shows the newest only): %1.").arg(others.join(QStringLiteral(", ")));
    }
  }

  // worn objects and locks
  QStringList wornText;
  for (const LabEffects::Worn& w : effects.worn)
  {
    const QString point = LabEffects::pointName(w.point);
    if (LabEffects::isHudPoint(w.point))
    {
      wornText << tr("%1 on HUD %2 (not on the avatar)").arg(w.object, point);
      continue;
    }
    Marker object;
    object.kind = Marker::Object;
    object.point = w.point;
    object.joints = LabEffects::jointsForPoint(w.point);
    object.label = w.object;
    m_markers << object;
    if (w.locked)
    {
      Marker lock = object;
      lock.kind = Marker::Lock;
      m_markers << lock;
    }
    wornText << (w.locked ? tr("%1 on %2, locked").arg(w.object, point) : tr("%1 on %2").arg(w.object, point));
  }
  if (!wornText.isEmpty()) m_lines << tr("Worn: %1.").arg(wornText.join(QStringLiteral("; ")));

  QStringList pointText;
  for (int point : effects.lockedPoints)
  {
    pointText << LabEffects::pointName(point);
    if (LabEffects::isHudPoint(point)) continue;
    Marker lock;
    lock.kind = Marker::Lock;
    lock.point = point;
    lock.joints = LabEffects::jointsForPoint(point);
    lock.label = LabEffects::pointName(point);
    m_markers << lock;
  }
  if (!pointText.isEmpty()) m_lines << tr("Locked points: %1.").arg(pointText.join(QStringLiteral(", ")));
  if (!effects.unwornLocks.isEmpty())
    m_lines << tr("Holds @detach=n but is not worn, so nothing is locked on the avatar: %1.")
                 .arg(effects.unwornLocks.join(QStringLiteral(", ")));
  if (!effects.unknownPoints.isEmpty())
    m_lines << tr("Not an attachment point, not shown: %1.").arg(effects.unknownPoints.join(QStringLiteral(", ")));
  if (effects.logTruncated) m_lines << tr("The event log was cut, so some effects may be missing.");
  if (m_lines.isEmpty())
    m_lines << tr("Nothing to show: no animation, worn object or lock at the end of the run.");

  emit changed();
}

void EffectsPreview::updateFrame()
{
  if (!m_animation) return;
  const int wanted = LabEffects::frameAt(m_elapsed, m_animation->frameTime(), m_animation->getNumberOfFrames(),
                                         m_animation->getLoopInPoint(), m_animation->getLoopOutPoint());
  if (wanted != m_animation->getFrame()) m_animation->setFrame(wanted);
}

void EffectsPreview::advance(double seconds)
{
  if (!m_animation || !(seconds > 0)) return;
  const int before = m_animation->getFrame();
  m_elapsed += seconds;
  updateFrame();
  if (m_animation->getFrame() != before) emit changed();
}

void EffectsPreview::tick()
{
  if (!m_animation) return;
  const double seconds = m_clock.restart() / 1000.0;
  advance(seconds);
}
