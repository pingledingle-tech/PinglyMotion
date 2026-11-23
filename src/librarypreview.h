/*
 * PinglyMotion - the small preview figure of the Library panel (docs/UX_DECISIONS.md 6l)
 *
 * Plays the picked library clip itself on a small stick figure: the classic
 * joints of the Second Life skeleton, seen from the front and a little from the
 * side. Nothing moves until the user asks (Play preview); stopped, it shows the
 * clip's first frame. The clip is read with the program's own BVH reader and
 * placed with SkeletonWalk, as the describe commands do; the colours are the
 * theme tokens (background, border, text, accent_text).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef LIBRARYPREVIEW_H
#define LIBRARYPREVIEW_H

#include <QLineF>
#include <QList>
#include <QPointF>
#include <QTimer>
#include <QVector3D>
#include <QWidget>

class LibraryPreview : public QWidget
{
  Q_OBJECT

  public:
    explicit LibraryPreview(QWidget* parent = nullptr);

    // reads the clip (a BVH file); an empty path or a file that cannot be read shows an empty box
    bool setClip(const QString& path);
    QString clip() const { return m_path; }
    int frameCount() const { return int(m_frames.size()); }
    int frame() const { return m_frame; }

    void setPlaying(bool on);
    bool playing() const { return m_timer.isActive(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  signals:
    void playingChanged(bool on);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    // one frame: the bone lines in the figure's own units, and the head
    struct Pose
    {
      QList<QPair<QVector3D, QVector3D>> bones;
      QVector3D head;
    };

    QList<Pose> m_frames;
    QString m_path;
    int m_frame = 0;
    double m_frameTime = 1.0 / 30.0;
    QTimer m_timer;
    // the box all frames fit into, seen from the preview's angle
    double m_minX = 0, m_maxX = 0, m_minY = 0, m_maxY = 0;
};

#endif
