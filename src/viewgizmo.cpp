/*
 * PinglyMotion - the view-direction widget (see viewgizmo.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "viewgizmo.h"

#include <QHelpEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <QtMath>
#include <algorithm>
#include <cmath>

#include "textsize.h"
#include "theme.h"

namespace
{
  // the mockup's SVG: a viewBox of 96 for 84 pixels, the background radius 46, the ends at 34
  constexpr double ViewBox = 96.0;
  constexpr double BackgroundRadius = 46.0;
  constexpr double EndDistance = 34.0;
  constexpr double PlusRadius = 9.0;
  constexpr double MinusRadius = 7.0;

  Theme::Token axisToken(ViewToolSpec::End end)
  {
    switch (end)
    {
      case ViewToolSpec::End::PlusX:
      case ViewToolSpec::End::MinusX: return Theme::Token::AxisX;
      case ViewToolSpec::End::PlusY:
      case ViewToolSpec::End::MinusY: return Theme::Token::AxisY;
      default: break;
    }
    return Theme::Token::AxisZ;
  }

  bool isPlus(ViewToolSpec::End end)
  {
    return end == ViewToolSpec::End::PlusX || end == ViewToolSpec::End::PlusY || end == ViewToolSpec::End::PlusZ;
  }
}

ViewGizmo::ViewGizmo(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("viewGizmo"));
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setAttribute(Qt::WA_TranslucentBackground);
  setAccessibleName(ViewToolSpec::tr(ViewToolSpec::widgetName));
  updateAccessibleText();
}

QVector3D ViewGizmo::direction(End end, float height, float angle)
{
  static const QVector3D axes[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  const QVector3D v = axes[int(end)];
  // the camera's turn (Camera::setModelView()): first around Y by the angle, then around X by the height
  const double a = qDegreesToRadians(double(angle)), h = qDegreesToRadians(double(height));
  const double x1 = v.x() * std::cos(a) + v.z() * std::sin(a);
  const double z1 = -v.x() * std::sin(a) + v.z() * std::cos(a);
  const double y2 = v.y() * std::cos(h) - z1 * std::sin(h);
  const double z2 = v.y() * std::sin(h) + z1 * std::cos(h);
  return QVector3D(float(x1), float(y2), float(z2));
}

void ViewGizmo::setCameraAngles(float height, float angle)
{
  if (height == m_height && angle == m_angle) return;
  m_height = height;
  m_angle = angle;
  update();
}

QSize ViewGizmo::sizeHint() const
{
  const int side = TextSize::scaled(BaseSize);
  return QSize(side, side);
}

QPointF ViewGizmo::endPosition(End end) const
{
  const double scale = qMin(width(), height()) / ViewBox;
  const QVector3D d = direction(end, m_height, m_angle);
  return QPointF(width() / 2.0 + d.x() * EndDistance * scale, height() / 2.0 - d.y() * EndDistance * scale);
}

double ViewGizmo::endRadius(End end) const
{
  const double scale = qMin(width(), height()) / ViewBox;
  return (isPlus(end) ? PlusRadius : MinusRadius) * scale;
}

bool ViewGizmo::endAt(const QPointF& point, End* end) const
{
  bool found = false;
  float nearest = -2.0f;
  for (const ViewToolSpec::EndSpec& spec : ViewToolSpec::ends)
  {
    const QPointF p = endPosition(spec.end);
    // a little bigger than drawn, so the small ends are easy to hit
    const double r = endRadius(spec.end) + 2.0;
    const double dx = point.x() - p.x(), dy = point.y() - p.y();
    if (dx * dx + dy * dy > r * r) continue;
    const float depth = direction(spec.end, m_height, m_angle).z();
    if (!found || depth > nearest)
    {
      found = true;
      nearest = depth;
      if (end) *end = spec.end;
    }
  }
  return found;
}

void ViewGizmo::setCurrentEnd(End end)
{
  if (end == m_current) return;
  m_current = end;
  updateAccessibleText();
  update();
}

void ViewGizmo::updateAccessibleText()
{
  setAccessibleDescription(ViewToolSpec::tr(ViewToolSpec::endSpec(m_current).name));
}

void ViewGizmo::paintEvent(QPaintEvent*)
{
  using Theme::Token;
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const double scale = qMin(width(), height()) / ViewBox;
  const QPointF centre(width() / 2.0, height() / 2.0);

  // the round background (the mockup: surface at 70 percent, raised at 85 percent on hover)
  QColor fill = underMouse() ? Theme::color(Token::SurfaceRaised) : Theme::color(Token::Surface);
  fill.setAlphaF(underMouse() ? 0.85 : 0.70);
  painter.setPen(QPen(Theme::color(Token::Border), Theme::Size::borderWidth));
  painter.setBrush(fill);
  const double r = BackgroundRadius * scale;
  painter.drawEllipse(centre, r, r);

  // back to front, so the ends nearest the viewer are drawn last
  QList<ViewToolSpec::EndSpec> order(std::begin(ViewToolSpec::ends), std::end(ViewToolSpec::ends));
  std::stable_sort(order.begin(), order.end(), [this](const ViewToolSpec::EndSpec& a, const ViewToolSpec::EndSpec& b)
  {
    return direction(a.end, m_height, m_angle).z() < direction(b.end, m_height, m_angle).z();
  });

  QFont font = this->font();
  font.setBold(true);
  for (const ViewToolSpec::EndSpec& spec : order)
  {
    const QColor axis = Theme::color(axisToken(spec.end));
    const QPointF p = endPosition(spec.end);
    const bool plus = isPlus(spec.end);
    if (plus)
    {
      QPen arm(axis, 2.5 * scale);
      arm.setCapStyle(Qt::RoundCap);
      painter.setPen(arm);
      painter.drawLine(centre, p);
    }
    const bool ring = (hasFocus() && spec.end == m_current) || (m_hoverValid && spec.end == m_hover);
    const double radius = endRadius(spec.end);
    if (plus)
    {
      painter.setBrush(axis);
      painter.setPen(ring ? QPen(Theme::color(Token::FocusRing), 3 * scale) : QPen(Theme::color(Token::Surface), 1.5 * scale));
    }
    else
    {
      painter.setBrush(Theme::mix(Theme::color(Token::Surface), axis, 0.25));
      painter.setPen(ring ? QPen(Theme::color(Token::FocusRing), 3 * scale) : QPen(axis, 2 * scale));
    }
    painter.drawEllipse(p, radius, radius);

    font.setPixelSize(qMax(6, int(std::lround((plus ? 11 : 9) * scale))));
    painter.setFont(font);
    painter.setPen(plus ? Theme::color(Token::OnAxis) : Theme::color(Token::Text));
    painter.drawText(QRectF(p.x() - radius, p.y() - radius, 2 * radius, 2 * radius), Qt::AlignCenter,
                     QString::fromUtf8(spec.letter));
  }
}

void ViewGizmo::mousePressEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
  {
    QWidget::mousePressEvent(event);
    return;
  }
  m_pressed = true;
  m_lastPos = event->position().toPoint();
  m_pressOnEnd = endAt(event->position(), &m_pressEnd);
  if (!m_pressOnEnd) setCursor(Qt::ClosedHandCursor);
  event->accept();
}

void ViewGizmo::mouseMoveEvent(QMouseEvent* event)
{
  const QPoint pos = event->position().toPoint();
  if (m_pressed && !m_pressOnEnd)
  {
    const QPoint moved = pos - m_lastPos;
    m_lastPos = pos;
    if (!moved.isNull()) emit dragged(moved.x(), moved.y());
    return;
  }
  End end;
  const bool valid = endAt(event->position(), &end);
  if (valid != m_hoverValid || (valid && end != m_hover))
  {
    m_hoverValid = valid;
    m_hover = end;
    setCursor(valid ? Qt::PointingHandCursor : Qt::OpenHandCursor);
    update();
  }
}

void ViewGizmo::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton || !m_pressed)
  {
    QWidget::mouseReleaseEvent(event);
    return;
  }
  m_pressed = false;
  unsetCursor();
  End end;
  if (m_pressOnEnd && endAt(event->position(), &end) && end == m_pressEnd)
  {
    setCurrentEnd(end);
    emit endChosen(end);
  }
  event->accept();
}

void ViewGizmo::keyPressEvent(QKeyEvent* event)
{
  const int index = int(m_current);
  switch (event->key())
  {
    case Qt::Key_Right:
    case Qt::Key_Down:
      setCurrentEnd(End((index + 1) % ViewToolSpec::endCount));
      return;
    case Qt::Key_Left:
    case Qt::Key_Up:
      setCurrentEnd(End((index + ViewToolSpec::endCount - 1) % ViewToolSpec::endCount));
      return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
      emit endChosen(m_current);
      return;
    default:
      break;
  }
  QWidget::keyPressEvent(event);
}

void ViewGizmo::leaveEvent(QEvent* event)
{
  m_hoverValid = false;
  update();
  QWidget::leaveEvent(event);
}

bool ViewGizmo::event(QEvent* event)
{
  if (event->type() == QEvent::ToolTip)
  {
    QHelpEvent* help = static_cast<QHelpEvent*>(event);
    End end;
    const QString tip = endAt(help->pos(), &end) ? ViewToolSpec::tr(ViewToolSpec::endSpec(end).name)
                                                 : ViewToolSpec::tr(ViewToolSpec::widgetDrag);
    QToolTip::showText(help->globalPos(), tip, this);
    return true;
  }
  if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) update();
  return QWidget::event(event);
}
