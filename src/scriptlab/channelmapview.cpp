/*
 * PinglyMotion Script Lab - the channel map's drawing (job A4)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "channelmapview.h"

#include <QAccessible>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include "lucideicons.h"
#include "theme.h"

using namespace LabChannelMap;

namespace
{
  const int SpacingXs = Theme::Size::spacingXs;
  const int SpacingS = Theme::Size::spacingS;
  const int SpacingM = Theme::Size::spacingM;
  const int Radius = Theme::Size::radiusControl;
  const int IconSize = Theme::Size::iconSmall;
  const int FocusWidth = Theme::Size::focusRingWidth;
}

ChannelMapView::ChannelMapView(QWidget* parent) : QWidget(parent)
{
  setObjectName("scriptLabChannelMap");
  setFocusPolicy(Qt::StrongFocus);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setAccessibleName(tr("Who talks to whom"));
  setMouseTracking(false);
}

void ChannelMapView::setMap(const Map& map)
{
  m_map = map;
  if (m_row >= m_map.arrows.size()) m_row = qMax(0, int(m_map.arrows.size()) - 1);
  updateGeometry();
  updateAccessible();
  update();
}

int ChannelMapView::rowHeight() const
{
  // the box, and above the arrow line its words: two lines of text with room around them
  return qMax(int(Theme::Size::controlDefault), 2 * fontMetrics().height() + SpacingS) + SpacingS;
}

int ChannelMapView::boxWidth() const
{
  int widest = fontMetrics().horizontalAdvance(tr("You"));
  for (const Box& b : m_map.boxes) widest = qMax(widest, fontMetrics().horizontalAdvance(b.label()));
  const int wanted = widest + 2 * SpacingM;
  // never more than a quarter of the width each, so the arrow keeps room for its words
  return qMax(fontMetrics().horizontalAdvance(QStringLiteral("Wwwwww")) + 2 * SpacingM, qMin(wanted, (width() - 2 * SpacingM) / 4));
}

QRect ChannelMapView::partRect(int row, Part part) const
{
  if (row < 0 || row >= m_map.arrows.size()) return QRect();
  const int h = rowHeight();
  const int top = row * h + SpacingS / 2;
  const int boxH = h - SpacingS;
  const int w = boxWidth();
  const int left = SpacingM;
  const int right = width() - SpacingM;
  const bool error = m_map.arrows[row].to < 0;
  // an error takes more room than a box: its words need it
  const int toW = error ? qMin(right - left - w - 2 * SpacingS - 4 * SpacingM, qMax(w, (right - left) / 3)) : w;
  switch (part)
  {
    case FromPart: return QRect(left, top, w, boxH);
    case ToPart: return QRect(right - toW, top, toW, boxH);
    case ArrowPart: return QRect(left + w + SpacingS, top, qMax(0, right - toW - SpacingS - (left + w + SpacingS)), boxH);
    case NoPart: break;
  }
  return QRect();
}

ChannelMapView::Hit ChannelMapView::hitAt(const QPoint& point) const
{
  const int h = rowHeight();
  if (point.y() < 0 || h <= 0) return Hit();
  const int row = point.y() / h;
  if (row >= m_map.arrows.size()) return Hit();
  for (Part part : {FromPart, ArrowPart, ToPart})
    if (partRect(row, part).adjusted(0, -SpacingS / 2, 0, SpacingS / 2).contains(point)) return Hit{row, part};
  return Hit();
}

void ChannelMapView::setCurrent(int row, Part part)
{
  if (m_map.arrows.isEmpty()) return;
  m_row = qBound(0, row, int(m_map.arrows.size()) - 1);
  m_part = part == NoPart ? ArrowPart : part;
  updateAccessible();
  update();
}

void ChannelMapView::activate(int row, Part part)
{
  if (row < 0 || row >= m_map.arrows.size()) return;
  setCurrent(row, part);
  const Arrow& a = m_map.arrows[row];
  if (part == FromPart && a.from >= 0) emit boxActivated(a.from);
  else if (part == ToPart && a.to >= 0) emit boxActivated(a.to);
  else emit arrowActivated(row);
}

QString ChannelMapView::emptyText()
{
  return tr("Nothing talked yet. Save a script or click a prop.");
}

QString ChannelMapView::partText(int row, Part part) const
{
  if (row < 0 || row >= m_map.arrows.size()) return QString();
  const Arrow& a = m_map.arrows[row];
  const auto boxText = [this](int index) -> QString
  {
    if (index < 0 || index >= m_map.boxes.size()) return QString();
    const Box& b = m_map.boxes[index];
    return tr("%1. %2 Click to pick it.").arg(b.label(), b.explain());
  };
  if (part == FromPart) return boxText(a.from);
  if (part == ToPart && a.to >= 0) return boxText(a.to);
  const QString from = m_map.boxes.value(a.from).label();
  const QString to = a.to >= 0 ? m_map.boxes.value(a.to).label() : tr("nobody");
  QString text = tr("%1 to %2: %3.").arg(from, to, a.label());
  if (!a.error.isEmpty()) text += QLatin1Char(' ') + a.error;
  text += QLatin1Char(' ') + (a.file.isEmpty() ? tr("It came from outside the scripts.") : tr("Click to see the script line that sent it."));
  return text;
}

QSize ChannelMapView::sizeHint() const
{
  const int rows = qMax(1, int(m_map.arrows.size()));
  return QSize(4 * (fontMetrics().horizontalAdvance(QStringLiteral("Wwwwww")) + 2 * SpacingM), rows * rowHeight());
}

QSize ChannelMapView::minimumSizeHint() const
{
  return QSize(3 * (fontMetrics().horizontalAdvance(QStringLiteral("Wwwwww")) + 2 * SpacingM), sizeHint().height());
}

void ChannelMapView::paintEvent(QPaintEvent*)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  const QColor text = Theme::color(Theme::Token::Text);
  const QColor muted = Theme::color(Theme::Token::TextMuted);
  const QColor border = Theme::color(Theme::Token::Border);
  const QColor raised = Theme::color(Theme::Token::SurfaceRaised);
  const QColor accent = Theme::color(Theme::Token::Accent);
  const QColor danger = Theme::color(Theme::Token::Danger);
  const QColor focus = Theme::color(Theme::Token::FocusRing);
  const QFontMetrics fm = fontMetrics();

  if (m_map.arrows.isEmpty())
  {
    p.setPen(muted);
    p.drawText(rect().adjusted(SpacingM, 0, -SpacingM, 0), Qt::AlignVCenter | Qt::AlignLeft, emptyText());
    return;
  }

  const auto drawBox = [&](const QRect& r, int index)
  {
    if (index < 0 || index >= m_map.boxes.size()) return;
    const Box& b = m_map.boxes[index];
    QPen pen(b.hub ? accent : border, Theme::Size::borderWidth);
    if (b.kind != BoxKind::Part) pen.setStyle(Qt::DashLine);   // outside the object
    p.setPen(pen);
    p.setBrush(raised);
    p.drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), Radius, Radius);
    p.setPen(text);
    p.drawText(r.adjusted(SpacingS, 0, -SpacingS, 0), Qt::AlignCenter, fm.elidedText(b.label(), Qt::ElideRight, r.width() - 2 * SpacingS));
  };

  for (int row = 0; row < m_map.arrows.size(); ++row)
  {
    const Arrow& a = m_map.arrows[row];
    const QRect from = partRect(row, FromPart);
    const QRect arrow = partRect(row, ArrowPart);
    const QRect to = partRect(row, ToPart);
    if (!rect().intersects(from.united(to))) continue;
    drawBox(from, a.from);
    const bool error = a.to < 0;
    const int lineY = arrow.center().y() + fm.height() / 2;
    // the words over the line
    p.setPen(error ? danger : text);
    p.drawText(QRect(arrow.left() + SpacingXs, arrow.top(), arrow.width() - 2 * SpacingXs, lineY - arrow.top() - SpacingXs),
               Qt::AlignHCenter | Qt::AlignBottom, fm.elidedText(a.label(), Qt::ElideRight, arrow.width() - 2 * SpacingXs));
    // the line: solid with an arrow head, or dashed and ending in a bar when nobody got it
    QPen line(error ? danger : muted, 2);
    if (error) line.setStyle(Qt::DashLine);
    p.setPen(line);
    const int x0 = arrow.left();
    const int x1 = arrow.right();
    p.drawLine(QPointF(x0, lineY), QPointF(x1 - (error ? 0 : SpacingS), lineY));
    if (error)
    {
      p.setPen(QPen(danger, 2));
      p.drawLine(QPointF(x1, lineY - SpacingS / 2 - 2), QPointF(x1, lineY + SpacingS / 2 + 2));
    }
    else
    {
      QPainterPath head;
      head.moveTo(x1, lineY);
      head.lineTo(x1 - SpacingS, lineY - SpacingXs - 1);
      head.lineTo(x1 - SpacingS, lineY + SpacingXs + 1);
      head.closeSubpath();
      p.setPen(Qt::NoPen);
      p.setBrush(muted);
      p.drawPath(head);
    }
    if (error)
    {
      // the icon and the words where the box would be
      const QIcon icon = LucideIcons::icon(QStringLiteral("circle-x"), danger, danger);
      const QRect iconRect(to.left(), to.center().y() - IconSize / 2, IconSize, IconSize);
      icon.paint(&p, iconRect);
      p.setPen(danger);
      p.drawText(to.adjusted(IconSize + SpacingXs, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
                 fm.elidedText(a.error, Qt::ElideRight, to.width() - IconSize - SpacingXs));
    }
    else
      drawBox(to, a.to);
  }

  // the keyboard's place: the focus ring around the focused part (design system "States")
  if (hasFocus())
  {
    const QRect r = partRect(m_row, m_part);
    if (!r.isEmpty())
    {
      p.setPen(QPen(focus, FocusWidth));
      p.setBrush(Qt::NoBrush);
      const int out = Theme::Size::focusRingOffset;
      p.drawRoundedRect(QRectF(r).adjusted(-out + 1, -out + 1, out - 1, out - 1), Radius + out, Radius + out);
    }
  }
}

void ChannelMapView::mousePressEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton) return QWidget::mousePressEvent(event);
  const Hit hit = hitAt(event->position().toPoint());
  if (hit.row < 0) return;
  setFocus(Qt::MouseFocusReason);
  activate(hit.row, hit.part);
}

void ChannelMapView::keyPressEvent(QKeyEvent* event)
{
  if (m_map.arrows.isEmpty() || event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) return QWidget::keyPressEvent(event);
  switch (event->key())
  {
    case Qt::Key_Up:
      if (m_row == 0) return QWidget::keyPressEvent(event);
      setCurrent(m_row - 1, m_part);
      return;
    case Qt::Key_Down:
      if (m_row + 1 >= m_map.arrows.size()) return QWidget::keyPressEvent(event);
      setCurrent(m_row + 1, m_part);
      return;
    case Qt::Key_Left: setCurrent(m_row, Part(qMax(0, int(m_part) - 1))); return;
    case Qt::Key_Right: setCurrent(m_row, Part(qMin(2, int(m_part) + 1))); return;
    case Qt::Key_Home: setCurrent(0, m_part); return;
    case Qt::Key_End: setCurrent(int(m_map.arrows.size()) - 1, m_part); return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space: activate(m_row, m_part); return;
    default: break;
  }
  QWidget::keyPressEvent(event);
}

bool ChannelMapView::event(QEvent* event)
{
  if (event->type() == QEvent::ToolTip)
  {
    QHelpEvent* help = static_cast<QHelpEvent*>(event);
    const Hit hit = hitAt(help->pos());
    if (hit.row >= 0) QToolTip::showText(help->globalPos(), partText(hit.row, hit.part), this, partRect(hit.row, hit.part));
    else QToolTip::hideText();
    return true;
  }
  return QWidget::event(event);
}

void ChannelMapView::focusInEvent(QFocusEvent* event)
{
  QWidget::focusInEvent(event);
  updateAccessible();
  update();
}

void ChannelMapView::focusOutEvent(QFocusEvent* event)
{
  QWidget::focusOutEvent(event);
  update();
}

void ChannelMapView::updateAccessible()
{
  setAccessibleDescription(m_map.arrows.isEmpty() ? emptyText() : partText(m_row, m_part));
  if (hasFocus())
  {
    QAccessibleEvent changed(this, QAccessible::DescriptionChanged);
    QAccessible::updateAccessibility(&changed);
  }
}
