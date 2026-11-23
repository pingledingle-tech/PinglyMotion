/*
 * PinglyMotion - readable screenshots for AI assistants (issue #129, job C of
 * docs/archive/AI_HELP_PLAN.md)
 *
 * The control command "screenshot" can return more than one picture of the
 * 3D view in one image: several views side by side, a strip of frames, the
 * classic joint names with the figure's left and right side in two colours,
 * and dots where the hands and feet go over the animation. This file holds
 * the parts that need no OpenGL: the request the 3D view reads while it draws
 * such a picture, the projection of joint positions to pixels, and the
 * drawing of labels, dots, captions and the grid with QPainter.
 *
 * How a picture is asked for: the command puts a Request in a Scope and then
 * asks the window for an ordinary screenshot. AnimationView::renderImage()
 * sees the request (current()), turns its own camera for that one picture if
 * the request says so, leaves out the Bento markers if asked, fills in where
 * the joints are on the picture, and puts the camera back. So the window,
 * its camera and its other code do not change.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SCREENSHOTSHEET_H
#define SCREENSHOTSHEET_H

#include <QColor>
#include <QImage>
#include <QList>
#include <QMatrix4x4>
#include <QPair>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector3D>
#include <QVector>

namespace ScreenshotSheet
{
  // where a joint is on a picture, in pixels from the top left
  struct Mark
  {
    QString joint;
    QPointF at;
  };

  // what the command wants from the next picture of the 3D view
  struct Request
  {
    // turn the camera for this picture only: as Camera::orbit(height, angle, distance)
    bool overrideCamera = false;
    float height = 10.0f;
    float angle = 0.0f;
    float distance = 100.0f;
    // leave out the small balls of the Bento joints (face, fingers, wings, tail)
    bool hideMarkers = false;
    // fill in "marks" for the classic joints
    bool wantMarks = false;

    // filled in by the 3D view
    QVector<Mark> marks;
    bool drawn = false;
  };

  // makes "request" the current one while it lives (one at a time)
  class Scope
  {
    public:
      explicit Scope(Request* request);
      ~Scope();
      Scope(const Scope&) = delete;
      Scope& operator=(const Scope&) = delete;

    private:
      Request* m_previous;
  };

  // the request of the picture being drawn now, or nullptr
  Request* current();

  // the classic Second Life joints that get a label, in skeleton order
  const QStringList& labelJoints();
  // the joints that leave a trail of dots
  const QStringList& trailJoints();

  enum class Side { Left, Right, Middle };
  // the figure's own side: lShldr is left, rShldr right, the hip in the middle
  Side sideOf(const QString& joint);
  QColor colorOf(Side side);
  // what the colours mean, for the answer and the picture
  QString legend();

  // world positions to pixels of a picture of this size; projectionView is
  // projection * view, as the 3D view draws. Joints behind the camera are left out.
  QVector<Mark> project(const QList<QPair<QString, QVector3D>>& world, const QMatrix4x4& projectionView, const QSize& size);

  // a dot and the name for every mark (only the joints in labelJoints())
  void drawLabels(QImage& image, const QVector<Mark>& marks);
  // small dots for the trail joints, one set per sampled frame; "scale" turns
  // the marks' pixels into the image's pixels
  void drawTrail(QImage& image, const QVector<QVector<Mark>>& frames, double scale);
  // a line of text at the top left of the picture, on a dark box
  void drawCaption(QImage& image, const QString& text);
  // the legend line at the bottom left
  void drawLegend(QImage& image);

  // the size of grid() for "count" cells of size "cell": made in 64 bits; an empty size when
  // it is not a valid request or too big for a picture
  QSize gridSize(const QSize& cell, int columns, int count);

  // cells of equal size in rows of "columns"; an empty place stays dark grey
  QImage grid(const QVector<QImage>& cells, int columns);
}

#endif
