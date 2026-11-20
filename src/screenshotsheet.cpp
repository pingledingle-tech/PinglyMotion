/*
 * PinglyMotion - readable screenshots for AI assistants. See screenshotsheet.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QVector4D>

#include <cmath>

#include "screenshotsheet.h"

namespace ScreenshotSheet
{
  namespace
  {
    Request* g_current = nullptr;

    // pure colours, so a test can find them; blue for the figure's left, orange for its right
    const QColor LEFT_COLOR(0, 120, 255);
    const QColor RIGHT_COLOR(255, 110, 0);
    const QColor MIDDLE_COLOR(255, 255, 255);
    const QColor BOX_COLOR(0, 0, 0, 170);
    const QColor EMPTY_COLOR(40, 40, 40);

    QFont textFont(const QImage& image)
    {
      QFont font;
      font.setPixelSize(qBound(10, image.width() / 40, 18));
      return font;
    }
  }

  Scope::Scope(Request* request) : m_previous(g_current)
  {
    g_current = request;
  }

  Scope::~Scope()
  {
    g_current = m_previous;
  }

  Request* current()
  {
    return g_current;
  }

  const QStringList& labelJoints()
  {
    static const QStringList joints = {
      "hip", "abdomen", "chest", "neck", "head",
      "lCollar", "lShldr", "lForeArm", "lHand",
      "rCollar", "rShldr", "rForeArm", "rHand",
      "lThigh", "lShin", "lFoot",
      "rThigh", "rShin", "rFoot",
    };
    return joints;
  }

  const QStringList& trailJoints()
  {
    static const QStringList joints = {"lHand", "rHand", "lFoot", "rFoot"};
    return joints;
  }

  Side sideOf(const QString& joint)
  {
    if (joint.size() > 1 && joint.at(1).isUpper())
    {
      if (joint.at(0) == QLatin1Char('l')) return Side::Left;
      if (joint.at(0) == QLatin1Char('r')) return Side::Right;
    }
    // Bento names such as mHandThumb1Left
    if (joint.contains("Left")) return Side::Left;
    if (joint.contains("Right")) return Side::Right;
    return Side::Middle;
  }

  QColor colorOf(Side side)
  {
    switch (side)
    {
      case Side::Left: return LEFT_COLOR;
      case Side::Right: return RIGHT_COLOR;
      default: return MIDDLE_COLOR;
    }
  }

  QString legend()
  {
    return "blue = the figure's left side, orange = the figure's right side";
  }

  QVector<Mark> project(const QList<QPair<QString, QVector3D>>& world, const QMatrix4x4& projectionView, const QSize& size)
  {
    QVector<Mark> marks;
    for (const auto& entry : world)
    {
      const QVector4D clip = projectionView * QVector4D(entry.second, 1.0f);
      if (!(clip.w() > 1e-6f)) continue;
      const float x = clip.x() / clip.w();
      const float y = clip.y() / clip.w();
      if (!std::isfinite(x) || !std::isfinite(y)) continue;
      Mark mark;
      mark.joint = entry.first;
      mark.at = QPointF((x + 1.0f) * 0.5f * size.width(), (1.0f - y) * 0.5f * size.height());
      marks << mark;
    }
    return marks;
  }

  void drawLabels(QImage& image, const QVector<Mark>& marks)
  {
    QPainter painter(&image);
    painter.setFont(textFont(image));
    const QFontMetrics metrics(painter.font());
    const int radius = qMax(3, image.width() / 160);
    // names go outwards: left of the dot when the joint is left of the hip on the picture
    double middle = -1.0;
    for (const Mark& mark : marks)
      if (mark.joint == QLatin1String("hip")) middle = mark.at.x();
    for (const Mark& mark : marks)
    {
      if (!labelJoints().contains(mark.joint)) continue;
      const Side side = sideOf(mark.joint);
      const QColor color = colorOf(side);
      // the dot without smoothing, so its pixels have exactly the side's colour
      painter.setRenderHint(QPainter::Antialiasing, false);
      painter.setPen(Qt::NoPen);
      painter.setBrush(color);
      painter.drawEllipse(mark.at, radius, radius);

      // the name (as set_pose uses it) on a dark box beside the dot
      const QString& text = mark.joint;
      const double boxWidth = metrics.horizontalAdvance(text) + 4;
      const bool toLeft = middle >= 0.0 && mark.at.x() < middle - 1.0;
      QRectF box(toLeft ? mark.at.x() - radius - 2 - boxWidth : mark.at.x() + radius + 2,
                 mark.at.y() - metrics.height() / 2.0, boxWidth, metrics.height());
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setBrush(BOX_COLOR);
      painter.drawRect(box);
      painter.setPen(color);
      painter.drawText(box.adjusted(2, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
    }
  }

  void drawTrail(QImage& image, const QVector<QVector<Mark>>& frames, double scale)
  {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    const int radius = qMax(1, image.width() / 320);
    for (const QVector<Mark>& marks : frames)
      for (const Mark& mark : marks)
      {
        if (!trailJoints().contains(mark.joint)) continue;
        painter.setBrush(colorOf(sideOf(mark.joint)));
        painter.drawEllipse(mark.at * scale, radius, radius);
      }
  }

  void drawCaption(QImage& image, const QString& text)
  {
    if (text.isEmpty()) return;
    QPainter painter(&image);
    painter.setFont(textFont(image));
    const QFontMetrics metrics(painter.font());
    const QRectF box(4, 4, metrics.horizontalAdvance(text) + 10, metrics.height() + 6);
    painter.setPen(Qt::NoPen);
    painter.setBrush(BOX_COLOR);
    painter.drawRect(box);
    painter.setPen(Qt::white);
    painter.drawText(box, Qt::AlignCenter, text);
  }

  void drawLegend(QImage& image)
  {
    QPainter painter(&image);
    painter.setFont(textFont(image));
    const QFontMetrics metrics(painter.font());
    const QString left = "blue = figure's left", right = "orange = figure's right";
    const int height = metrics.height() + 6;
    const QRectF box(4, image.height() - height - 4,
                     metrics.horizontalAdvance(left) + metrics.horizontalAdvance(right) + 24, height);
    painter.setPen(Qt::NoPen);
    painter.setBrush(BOX_COLOR);
    painter.drawRect(box);
    painter.setPen(LEFT_COLOR.lighter(130));
    painter.drawText(box.adjusted(5, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, left);
    painter.setPen(RIGHT_COLOR);
    painter.drawText(box.adjusted(15 + metrics.horizontalAdvance(left), 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, right);
  }

  QImage grid(const QVector<QImage>& cells, int columns)
  {
    if (cells.isEmpty() || columns < 1) return QImage();
    const QSize cell = cells.first().size();
    const int rows = (cells.size() + columns - 1) / columns;
    QImage sheet(cell.width() * columns, cell.height() * rows, QImage::Format_RGB32);
    sheet.fill(EMPTY_COLOR);
    QPainter painter(&sheet);
    for (int i = 0; i < cells.size(); i++)
      painter.drawImage(QPoint((i % columns) * cell.width(), (i / columns) * cell.height()), cells.at(i));
    return sheet;
  }
}
