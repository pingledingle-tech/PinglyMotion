/*
 * PinglyMotion - the Hands tab of the Joints panel (see handstab.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "handstab.h"

#include <QEvent>
#include <QFrame>
#include <QFocusEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#include "lucideicons.h"
#include "theme.h"

namespace
{
  QString tr2(const char* text)
  {
    return HandsSpec::tr2(text);
  }

  // semibold, nothing else set, so the size still follows the program's text size
  QFont weightOnly()
  {
    QFont font;
    font.setWeight(QFont::DemiBold);
    return font;
  }

  QFont semibold(const QFont& base)
  {
    QFont font = base;
    font.setWeight(QFont::DemiBold);
    return font;
  }

  // the caption size: one point smaller than the text around it (as the easing tiles)
  QFont caption(const QFont& base)
  {
    QFont font = base;
    if (font.pointSize() > 1) font.setPointSize(font.pointSize() - 1);
    else if (font.pointSizeF() > 1.0) font.setPointSizeF(font.pointSizeF() - 1.0);
    return font;
  }

  // A label that breaks its text into lines and asks only for the width of its
  // longest word (as in the Selected joint panel), so the controls set the width.
  class WrapLabel : public QLabel
  {
    public:
      WrapLabel(const QString& text, QWidget* parent) : QLabel(text, parent)
      {
        setWordWrap(true);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
      }

      QSize sizeHint() const override
      {
        int word = 0;
        const QFontMetrics metrics = fontMetrics();
        for (const QString& part : text().split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts))
          word = std::max(word, metrics.horizontalAdvance(part));
        const QMargins m = contentsMargins();
        const int least = word + m.left() + m.right() + 2 * margin() + 1;
        return QSize(least, heightForWidth(std::max(width(), least)));
      }

      QSize minimumSizeHint() const override
      {
        const QMargins m = contentsMargins();
        return QSize(sizeHint().width(), fontMetrics().height() + m.top() + m.bottom() + 2 * margin());
      }
  };

  QColor axisColour(int axis)
  {
    using Theme::Token;
    return Theme::color(axis == 0 ? Token::AxisX : (axis == 1 ? Token::AxisY : Token::AxisZ));
  }

  const char* axisLetter(int axis)
  {
    return axis == 0 ? "X" : (axis == 1 ? "Y" : "Z");
  }

  // a whole number of degrees for the grid; -0 shows as 0
  int wholeDegrees(double value)
  {
    const int whole = static_cast<int>(std::lround(value));
    return whole == 0 ? 0 : whole;
  }

  // the 3 pixel bar in accent_text on the left of a section heading, as the mockup's .sech
  QFrame* headingBar(QWidget* parent)
  {
    QFrame* bar = new QFrame(parent);
    bar->setObjectName(QStringLiteral("sectionBar"));
    bar->setFixedWidth(3);
    bar->setAutoFillBackground(true);
    bar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    return bar;
  }

  // a section heading's text, semibold
  QLabel* sectionHeading(const QString& text, QWidget* parent, const QString& name)
  {
    QLabel* label = new QLabel(text, parent);
    label->setObjectName(name);
    label->setFont(weightOnly());
    return label;
  }
}

namespace HandsSpec
{
  QString keySet(int frame, int joints)
  {
    if (joints <= 1) return tr2(keySetOne).arg(frame);
    return tr2(keySetMany).arg(frame).arg(joints);
  }

  QString jointLabel(int row)
  {
    if (row < 0 || row >= HandPose::jointCount) return QString();
    return tr2(fingerFullNames[row / HandPose::partCount]) + QLatin1Char(' ') + tr2(partNames[row % HandPose::partCount]);
  }
}

// ---------------------------------------------------------------------------
// HandPresetTiles

HandPresetTiles::HandPresetTiles(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("handPresetTiles"));
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_Hover);
  setMouseTracking(true);
  setCursor(Qt::PointingHandCursor);
  QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  policy.setHeightForWidth(true);
  setSizePolicy(policy);
}

void HandPresetTiles::setPresets(const QVector<HandPose::Preset>& presets)
{
  m_presets = presets;
  m_current = std::clamp(m_current, 0, std::max(0, int(m_presets.count()) - 1));
  updateGeometry();
  update();
}

void HandPresetTiles::setChecked(int index)
{
  if (index < -1 || index >= m_presets.count()) index = -1;
  if (index == m_checked) return;
  m_checked = index;
  update();
}

void HandPresetTiles::setCurrent(int index)
{
  if (m_presets.isEmpty()) return;
  index = std::clamp(index, 0, int(m_presets.count()) - 1);
  if (index == m_current) return;
  m_current = index;
  setAccessibleDescription(m_presets[m_current].name);
  update();
}

int HandPresetTiles::columns() const
{
  // two columns in a very narrow panel, as the mockup (container under 230 pixels)
  return width() > 0 && width() < 200 ? 2 : 3;
}

int HandPresetTiles::tileHeight(int width) const
{
  // the picture (24), a 2 pixel gap and the longest name broken into lines at the
  // tile's width, inside the padding; at least 60 pixels, as the mockup
  const QFontMetrics metrics(semibold(caption(font())));
  const int textWidth = std::max(1, tileWidth(width) - 2 * Theme::Size::spacingXs);
  int lines = 1;
  for (const HandPose::Preset& preset : m_presets)
  {
    const QRect r = metrics.boundingRect(QRect(0, 0, textWidth, 10000), Qt::AlignHCenter | Qt::TextWordWrap, preset.name);
    lines = std::max(lines, (r.height() + metrics.height() - 1) / metrics.height());
  }
  return std::max(60, 2 * Theme::Size::spacingXs + 24 + 2 + lines * metrics.height());
}

int HandPresetTiles::tileWidth(int width) const
{
  const int cols = (width > 0 && width < 200) ? 2 : 3;
  return std::max(1, (width - (cols - 1) * Theme::Size::spacingS) / cols);
}

QRect HandPresetTiles::tileRect(int index) const
{
  const int cols = columns();
  const int w = tileWidth(width());
  const int h = tileHeight(width());
  const int col = index % cols;
  const int row = index / cols;
  return QRect(col * (w + Theme::Size::spacingS), row * (h + Theme::Size::spacingS), w, h);
}

QRect HandPresetTiles::nameRect(int index) const
{
  const QRect rect = tileRect(index);
  const int top = rect.top() + Theme::Size::spacingXs + 24 + 2;
  return QRect(rect.left() + Theme::Size::spacingXs, top, rect.width() - 2 * Theme::Size::spacingXs, rect.bottom() - Theme::Size::spacingXs - top + 1);
}

QFont HandPresetTiles::nameFont(bool picked) const
{
  return picked ? semibold(caption(font())) : caption(font());
}

int HandPresetTiles::heightForWidth(int width) const
{
  const int cols = (width > 0 && width < 200) ? 2 : 3;
  const int rows = (int(m_presets.count()) + cols - 1) / cols;
  return rows <= 0 ? 0 : rows * tileHeight(width) + (rows - 1) * Theme::Size::spacingS;
}

QSize HandPresetTiles::sizeHint() const
{
  // wide enough for the longest word of a name, and the 40 pixel picture
  const QFontMetrics metrics(semibold(caption(font())));
  int widest = 40;
  for (const HandPose::Preset& preset : m_presets)
    for (const QString& word : preset.name.split(QLatin1Char(' '), Qt::SkipEmptyParts))
      widest = std::max(widest, metrics.horizontalAdvance(word));
  const int w = 3 * (widest + 2 * Theme::Size::spacingXs + 4) + 2 * Theme::Size::spacingS;
  return QSize(w, heightForWidth(w));
}

QSize HandPresetTiles::minimumSizeHint() const
{
  const QFontMetrics metrics(semibold(caption(font())));
  int widest = 40;
  for (const HandPose::Preset& preset : m_presets)
    for (const QString& word : preset.name.split(QLatin1Char(' '), Qt::SkipEmptyParts))
      widest = std::max(widest, metrics.horizontalAdvance(word));
  const int w = 2 * (widest + 2 * Theme::Size::spacingXs + 4) + Theme::Size::spacingS;
  return QSize(w, heightForWidth(w));
}

int HandPresetTiles::indexAt(const QPoint& pos) const
{
  for (int i = 0; i < m_presets.count(); ++i)
    if (tileRect(i).contains(pos)) return i;
  return -1;
}

void HandPresetTiles::paintEvent(QPaintEvent*)
{
  using Theme::Token;
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const bool enabled = isEnabled();
  const QColor text = Theme::color(enabled ? Token::Text : Token::TextDisabled);
  const QColor raised = Theme::color(Token::SurfaceRaised);
  const QColor border = Theme::color(Token::Border);
  const QColor selection = Theme::color(Token::Selection);
  const QPoint mouse = mapFromGlobal(QCursor::pos());
  const int radius = Theme::Size::radiusControl;

  for (int i = 0; i < m_presets.count(); ++i)
  {
    const QRect rect = tileRect(i);
    const bool on = i == m_checked;
    const bool hover = enabled && underMouse() && rect.contains(mouse);
    QColor fill = on ? Theme::mix(raised, selection, 0.14) : raised;
    if (hover && !on) fill = Theme::mix(raised, Theme::color(Token::Text), 0.08);
    const int line = on ? 2 : Theme::Size::borderWidth;
    QRectF frame = QRectF(rect).adjusted(line / 2.0, line / 2.0, -line / 2.0, -line / 2.0);
    painter.setPen(QPen(on ? selection : border, line));
    painter.setBrush(fill);
    painter.drawRoundedRect(frame, radius, radius);

    // five bars, shorter when that finger is more bent (from the preset's own numbers)
    const HandPose::Hand& hand = m_presets[i].left;
    const double scale = 40.0 / 44.0;
    const QPointF origin(rect.center().x() - 20.0, rect.top() + Theme::Size::spacingXs);
    painter.setPen(Qt::NoPen);
    painter.setBrush(text);
    for (int f = 0; f < HandPose::fingerCount; ++f)
    {
      double sum = 0.0;
      for (int k = 0; k < HandPose::partCount; ++k)
      {
        const QVector3D& a = hand[f * HandPose::partCount + k];
        // curl is Z; the thumb also folds in across the palm on X
        sum += std::abs(a.z()) + (f == 0 ? std::abs(a.x()) : 0.0);
      }
      const double bentShare = std::min(1.0, sum / 220.0);
      const int length = static_cast<int>(std::lround(20.0 - bentShare * 13.0));
      const double x = 3 + f * 8;
      const double y = (f == 0 ? 24 : 22) - length + (f == 0 ? 2 : 0);
      painter.drawRoundedRect(QRectF(origin.x() + x * scale, origin.y() + y, 5 * scale, length), 2.5 * scale, 2.5 * scale);
    }

    // the name under the picture
    painter.setFont(nameFont(on));
    painter.setPen(text);
    painter.drawText(nameRect(i), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_presets[i].name);

    // the tick of the picked tile: a 16 pixel circle in the selection colour, the check in on_accent
    if (on)
    {
      const QRectF tick(rect.right() - 3 - 16 + 1, rect.top() + 3, 16, 16);
      painter.setPen(Qt::NoPen);
      painter.setBrush(selection);
      painter.drawEllipse(tick);
      const QIcon check = LucideIcons::icon(QStringLiteral("check"), Theme::color(Token::OnAccent), Theme::color(Token::OnAccent));
      check.paint(&painter, tick.adjusted(2, 2, -2, -2).toRect());
    }

    // the keyboard focus ring (charter P8)
    if (hasFocus() && i == m_current)
    {
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(Theme::color(Token::FocusRing), Theme::Size::focusRingWidth));
      painter.drawRoundedRect(QRectF(rect).adjusted(1, 1, -1, -1), radius, radius);
    }
  }
}

void HandPresetTiles::keyPressEvent(QKeyEvent* event)
{
  const int cols = columns();
  const int n = m_presets.count();
  if (n == 0)
  {
    QWidget::keyPressEvent(event);
    return;
  }
  int next = m_current;
  switch (event->key())
  {
    case Qt::Key_Left: next = std::max(0, m_current - 1); break;
    case Qt::Key_Right: next = std::min(n - 1, m_current + 1); break;
    case Qt::Key_Up: next = m_current - cols >= 0 ? m_current - cols : m_current; break;
    case Qt::Key_Down: next = m_current + cols < n ? m_current + cols : m_current; break;
    case Qt::Key_Home: next = 0; break;
    case Qt::Key_End: next = n - 1; break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
      // picking does not follow the focus: one pick changes 15 or 30 joints
      emit picked(m_current);
      return;
    default:
      QWidget::keyPressEvent(event);
      return;
  }
  setCurrent(next);
}

void HandPresetTiles::mousePressEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
  {
    QWidget::mousePressEvent(event);
    return;
  }
  const int index = indexAt(event->position().toPoint());
  if (index < 0) return;
  setCurrent(index);
  emit picked(index);
}

void HandPresetTiles::focusInEvent(QFocusEvent* event)
{
  QWidget::focusInEvent(event);
  update();
}

void HandPresetTiles::focusOutEvent(QFocusEvent* event)
{
  QWidget::focusOutEvent(event);
  update();
}

bool HandPresetTiles::event(QEvent* event)
{
  if (event->type() == QEvent::HoverMove || event->type() == QEvent::HoverLeave || event->type() == QEvent::HoverEnter)
    update();
  return QWidget::event(event);
}

// ---------------------------------------------------------------------------
// HandJointGrid

HandJointGrid::HandJointGrid(QWidget* parent) : QWidget(parent)
{
  setObjectName(QStringLiteral("handJointGrid"));
  setFocusPolicy(Qt::StrongFocus);
  setCursor(Qt::SizeHorCursor);
  m_hand = HandPose::openHand();
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

HandJointGrid::~HandJointGrid()
{
  // an open editor goes without a last change (it would emit editingFinished while it goes)
  if (m_editor) m_editor->disconnect(this);
}

void HandJointGrid::setHand(const HandPose::Hand& hand, bool known)
{
  m_hand = hand;
  m_known = known;
  update();
}

void HandJointGrid::setSelectedRow(int row)
{
  if (row < -1 || row >= HandPose::jointCount) row = -1;
  m_selectedRow = row;
  // the keyboard cell follows the picked joint
  if (row >= 0 && !m_editor) m_row = row;
  update();
}

void HandJointGrid::setCurrent(int row, int axis)
{
  m_row = std::clamp(row, 0, HandPose::jointCount - 1);
  m_axis = std::clamp(axis, 0, 2);
  setAccessibleDescription(QStringLiteral("%1, %2: %3").arg(HandsSpec::jointLabel(m_row), QLatin1String(axisLetter(m_axis)), cellText(m_row, m_axis)));
  update();
}

QString HandJointGrid::cellText(int row, int axis) const
{
  if (!m_known || row < 0 || row >= HandPose::jointCount || axis < 0 || axis > 2) return QStringLiteral("–");
  return QStringLiteral("%1°").arg(wholeDegrees(m_hand[row][axis]));
}

int HandJointGrid::rowHeight() const
{
  return std::max(Theme::Size::controlCompact, fontMetrics().height() + 6);
}

int HandJointGrid::nameWidth() const
{
  int widest = 0;
  for (const char* part : HandsSpec::partNames)
    widest = std::max(widest, QFontMetrics(semibold(font())).horizontalAdvance(tr2(part)));
  return Theme::Size::spacingM + widest + Theme::Size::spacingS;
}

int HandJointGrid::cellWidth() const
{
  const int least = fontMetrics().horizontalAdvance(QStringLiteral("-180°")) + 2 * Theme::Size::spacingXs + 2;
  const int room = (width() - nameWidth() - 3 * Theme::Size::spacingXs) / 3;
  return std::max(least, room);
}

int HandJointGrid::rowTop(int row) const
{
  const int step = rowHeight() + 2;
  if (row < 0) return 0;
  const int finger = row / HandPose::partCount;
  const int part = row % HandPose::partCount;
  // the header row, then per finger its heading row and three joint rows
  return step + finger * 4 * step + step + part * step;
}

QRect HandJointGrid::cellRect(int row, int axis) const
{
  const int x = nameWidth() + axis * (cellWidth() + Theme::Size::spacingXs);
  return QRect(x, rowTop(row), cellWidth(), rowHeight());
}

QSize HandJointGrid::sizeHint() const
{
  const int least = fontMetrics().horizontalAdvance(QStringLiteral("-180°")) + 2 * Theme::Size::spacingXs + 2;
  const int w = nameWidth() + 3 * (least + Theme::Size::spacingXs);
  const int h = rowTop(HandPose::jointCount - 1) + rowHeight() + 2;
  return QSize(w, h);
}

QSize HandJointGrid::minimumSizeHint() const
{
  return sizeHint();
}

bool HandJointGrid::cellAt(const QPoint& pos, int& row, int& axis) const
{
  for (int r = 0; r < HandPose::jointCount; ++r)
    for (int a = 0; a < 3; ++a)
      if (cellRect(r, a).contains(pos))
      {
        row = r;
        axis = a;
        return true;
      }
  return false;
}

void HandJointGrid::paintEvent(QPaintEvent*)
{
  using Theme::Token;
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const bool enabled = isEnabled();
  const QColor text = Theme::color(enabled ? Token::Text : Token::TextDisabled);
  const QColor muted = Theme::color(enabled ? Token::TextMuted : Token::TextDisabled);
  const QColor surface = Theme::color(Token::Surface);
  const QColor selection = Theme::color(Token::Selection);
  QColor cellBorder = Theme::color(Token::Border);
  cellBorder.setAlphaF(0.45);
  const int h = rowHeight();
  const int radius = Theme::Size::radiusSmall;

  // the header: the axis badges in their colour with the letter in on_axis (charter P10)
  painter.setFont(semibold(font()));
  for (int axis = 0; axis < 3; ++axis)
  {
    const QRect cell = cellRect(0, axis);
    const int badgeW = QFontMetrics(painter.font()).horizontalAdvance(QStringLiteral("X")) + 2 * Theme::Size::spacingXs;
    const QRect badge(cell.center().x() - badgeW / 2, Theme::Size::spacingXs / 2, badgeW, h - Theme::Size::spacingXs);
    painter.setPen(Qt::NoPen);
    painter.setBrush(enabled ? axisColour(axis) : Theme::color(Token::TextDisabled));
    painter.drawRoundedRect(badge, radius, radius);
    painter.setPen(Theme::color(Token::OnAxis));
    painter.drawText(badge, Qt::AlignCenter, QLatin1String(axisLetter(axis)));
  }

  for (int finger = 0; finger < HandPose::fingerCount; ++finger)
  {
    // the finger's heading row: a bar in the Hands area colour, then the name
    const int top = rowTop(finger * HandPose::partCount) - (h + 2);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::color(Token::AreaHands));
    painter.drawRoundedRect(QRect(Theme::Size::spacingXs, top + (h - 14) / 2, 3, 14), 2, 2);
    painter.setFont(semibold(font()));
    painter.setPen(text);
    painter.drawText(QRect(Theme::Size::spacingXs + 3 + Theme::Size::spacingS, top, width(), h), Qt::AlignLeft | Qt::AlignVCenter,
                     tr2(HandsSpec::fingerFullNames[finger]));

    for (int part = 0; part < HandPose::partCount; ++part)
    {
      const int row = finger * HandPose::partCount + part;
      const bool selected = row == m_selectedRow;
      painter.setFont(selected ? semibold(font()) : font());
      painter.setPen(selected ? text : muted);
      painter.drawText(QRect(Theme::Size::spacingM, rowTop(row), nameWidth() - Theme::Size::spacingM, h), Qt::AlignLeft | Qt::AlignVCenter,
                       tr2(HandsSpec::partNames[part]));
      painter.setFont(font());
      for (int axis = 0; axis < 3; ++axis)
      {
        const QRect cell = cellRect(row, axis);
        painter.setPen(QPen(cellBorder, Theme::Size::borderWidth));
        painter.setBrush(selected ? Theme::mix(surface, selection, 0.14) : surface);
        painter.drawRoundedRect(QRectF(cell).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
        if (m_editor && row == m_row && axis == m_axis) continue;
        const QString value = cellText(row, axis);
        if (m_known)
        {
          // the number in text, the degree sign in text_muted
          const QString number = value.left(value.size() - 1);
          const QFontMetrics metrics(font());
          const int total = metrics.horizontalAdvance(value);
          const int x = cell.center().x() - total / 2;
          painter.setPen(text);
          painter.drawText(QRect(x, cell.top(), total, cell.height()), Qt::AlignLeft | Qt::AlignVCenter, number);
          painter.setPen(muted);
          painter.drawText(QRect(x + metrics.horizontalAdvance(number), cell.top(), total, cell.height()), Qt::AlignLeft | Qt::AlignVCenter,
                           QStringLiteral("°"));
        }
        else
        {
          painter.setPen(muted);
          painter.drawText(cell, Qt::AlignCenter, value);
        }
      }
    }
  }

  // the keyboard focus ring on the current cell (charter P8)
  if (hasFocus() && !m_editor)
  {
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Theme::color(Token::FocusRing), Theme::Size::focusRingWidth));
    painter.drawRoundedRect(QRectF(cellRect(m_row, m_axis)).adjusted(1, 1, -1, -1), radius, radius);
  }
}

void HandJointGrid::moveTo(int row, int axis)
{
  row = std::clamp(row, 0, HandPose::jointCount - 1);
  axis = std::clamp(axis, 0, 2);
  const bool newRow = row != m_row;
  setCurrent(row, axis);
  // moving to another row picks that joint
  if (newRow && m_known) emit rowPicked(row);
}

void HandJointGrid::change(int row, int axis, double value)
{
  if (!m_known) return;
  value = std::clamp(value, -180.0, 180.0);
  emit cellChanged(row, axis, value);
}

void HandJointGrid::keyPressEvent(QKeyEvent* event)
{
  const int key = event->key();
  switch (key)
  {
    case Qt::Key_Left: moveTo(m_row, m_axis - 1); return;
    case Qt::Key_Right: moveTo(m_row, m_axis + 1); return;
    case Qt::Key_Up: moveTo(m_row - 1, m_axis); return;
    case Qt::Key_Down: moveTo(m_row + 1, m_axis); return;
    case Qt::Key_Home: moveTo(m_row, 0); return;
    case Qt::Key_End: moveTo(m_row, 2); return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_F2:
      startEditing();
      return;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    {
      const int step = (event->modifiers() & Qt::ShiftModifier) ? 1 : 5;
      const int sign = key == Qt::Key_PageUp ? 1 : -1;
      change(m_row, m_axis, wholeDegrees(m_hand[m_row][m_axis]) + sign * step);
      return;
    }
    default:
      break;
  }
  // typing a number in a focused cell edits it
  const QString typed = event->text();
  if (typed.size() == 1 && (typed[0].isDigit() || typed == QLatin1String("-") || typed == QLatin1String("+") || typed == QLatin1String(".")))
  {
    startEditing(typed);
    return;
  }
  QWidget::keyPressEvent(event);
}

void HandJointGrid::startEditing(const QString& first)
{
  if (!m_known || !isEnabled() || m_editor) return;
  m_editor = new QLineEdit(this);
  m_editor->setObjectName(QStringLiteral("handGridEditor"));
  m_editor->setAlignment(Qt::AlignCenter);
  m_editor->setAccessibleName(QStringLiteral("%1, %2").arg(HandsSpec::jointLabel(m_row), QLatin1String(axisLetter(m_axis))));
  m_editor->setGeometry(cellRect(m_row, m_axis));
  m_editor->setStyleSheet(QStringLiteral("QLineEdit { border: %1px solid %2; border-radius: %3px; background: %4; color: %5; padding: 0px %6px; }")
                            .arg(Theme::Size::focusRingWidth)
                            .arg(Theme::color(Theme::Token::FocusRing).name())
                            .arg(Theme::Size::radiusSmall)
                            .arg(Theme::color(Theme::Token::Surface).name(), Theme::color(Theme::Token::Text).name())
                            .arg(Theme::Size::spacingXs));
  if (first.isEmpty())
  {
    m_editor->setText(QString::number(wholeDegrees(m_hand[m_row][m_axis])));
    m_editor->selectAll();
  }
  else
  {
    m_editor->setText(first);
    m_editor->setCursorPosition(first.size());
  }
  m_editor->installEventFilter(this);
  connect(m_editor, &QLineEdit::editingFinished, this, [this]() {
    // focus left the editor (Tab or a click elsewhere): keep the number
    if (m_editor && !m_editor->hasFocus()) finishEditing(true);
  });
  m_editor->show();
  m_editor->setFocus(Qt::OtherFocusReason);
  update();
}

void HandJointGrid::finishEditing(bool keep)
{
  if (!m_editor) return;
  QLineEdit* editor = m_editor;
  m_editor = nullptr;
  const QString text = editor->text().trimmed();
  const bool hadFocus = editor->hasFocus();
  editor->removeEventFilter(this);
  editor->hide();
  editor->deleteLater();
  if (hadFocus) setFocus(Qt::OtherFocusReason);
  update();
  if (!keep) return;
  bool ok = false;
  const double value = text.toDouble(&ok);
  if (ok && std::isfinite(value)) change(m_row, m_axis, value);
  else emit invalidNumber();
}

bool HandJointGrid::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == m_editor && event->type() == QEvent::KeyPress)
  {
    QKeyEvent* key = static_cast<QKeyEvent*>(event);
    if (key->key() == Qt::Key_Escape)
    {
      // Esc throws the edit away
      finishEditing(false);
      return true;
    }
    if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
    {
      // Enter keeps it (handled here: the line editor passes Enter on to the grid,
      // which would start editing again)
      finishEditing(true);
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

void HandJointGrid::mousePressEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton || !m_known)
  {
    QWidget::mousePressEvent(event);
    return;
  }
  int row = 0;
  int axis = 0;
  if (!cellAt(event->position().toPoint(), row, axis)) return;
  m_pressed = true;
  m_dragged = false;
  m_pressPos = event->position().toPoint();
  m_pressRow = row;
  m_pressAxis = axis;
  m_pressValue = wholeDegrees(m_hand[row][axis]);
}

void HandJointGrid::mouseMoveEvent(QMouseEvent* event)
{
  if (!m_pressed) return;
  const int dx = event->position().toPoint().x() - m_pressPos.x();
  if (!m_dragged && std::abs(dx) < 4) return;
  // dragging a cell sideways turns that joint on that axis: one degree for each two pixels
  m_dragged = true;
  const double value = std::clamp(m_pressValue + std::lround(dx / 2.0), -180.0, 180.0);
  if (wholeDegrees(m_hand[m_pressRow][m_pressAxis]) != wholeDegrees(value)) change(m_pressRow, m_pressAxis, value);
}

void HandJointGrid::mouseReleaseEvent(QMouseEvent* event)
{
  if (!m_pressed || event->button() != Qt::LeftButton)
  {
    QWidget::mouseReleaseEvent(event);
    return;
  }
  m_pressed = false;
  const bool newRow = m_pressRow != m_selectedRow;
  setCurrent(m_pressRow, m_pressAxis);
  // a click picks that joint (the Selected joint panel follows)
  if (!m_dragged && newRow) emit rowPicked(m_pressRow);
}

void HandJointGrid::mouseDoubleClickEvent(QMouseEvent* event)
{
  int row = 0;
  int axis = 0;
  if (event->button() == Qt::LeftButton && m_known && cellAt(event->position().toPoint(), row, axis))
  {
    setCurrent(row, axis);
    startEditing();
    return;
  }
  QWidget::mouseDoubleClickEvent(event);
}

void HandJointGrid::focusInEvent(QFocusEvent* event)
{
  QWidget::focusInEvent(event);
  update();
}

void HandJointGrid::focusOutEvent(QFocusEvent* event)
{
  QWidget::focusOutEvent(event);
  update();
}

void HandJointGrid::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  if (m_editor) m_editor->setGeometry(cellRect(m_row, m_axis));
}

void HandJointGrid::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::FontChange) updateGeometry();
  if (event->type() == QEvent::EnabledChange && !isEnabled() && m_editor) finishEditing(false);
}

// ---------------------------------------------------------------------------
// HandsPanel

HandsPanel::HandsPanel(const QVector<HandPose::Preset>& presets, QWidget* parent) : QWidget(parent), m_presets(presets)
{
  setObjectName(QStringLiteral("handsPanel"));
  m_closed = HandPose::presetIndex(m_presets, QLatin1String(HandPose::closedPreset));

  QVBoxLayout* column = new QVBoxLayout(this);
  column->setContentsMargins(0, Theme::Size::spacingS, 0, Theme::Size::spacingS);
  column->setSpacing(Theme::Size::spacingL);

  // which hand, and whether the other copies it
  QWidget* head = new QWidget(this);
  QVBoxLayout* headColumn = new QVBoxLayout(head);
  headColumn->setContentsMargins(0, 0, 0, 0);
  headColumn->setSpacing(Theme::Size::spacingXs);
  QHBoxLayout* nameRow = new QHBoxLayout;
  nameRow->setSpacing(Theme::Size::spacingS);
  m_handIcon = new QLabel(head);
  m_handName = new QLabel(head);
  m_handName->setObjectName(QStringLiteral("handName"));
  m_handName->setFont(weightOnly());
  m_handCaption = new WrapLabel(QString(), head);
  m_handCaption->setObjectName(QStringLiteral("handCaption"));
  nameRow->addWidget(m_handIcon);
  nameRow->addWidget(m_handName);
  nameRow->addWidget(m_handCaption, 1);
  headColumn->addLayout(nameRow);
  // the hand's state as a chip
  m_chip = new QLabel(head);
  m_chip->setObjectName(QStringLiteral("handStateChip"));
  QHBoxLayout* chipRow = new QHBoxLayout;
  chipRow->setContentsMargins(0, 0, 0, 0);
  chipRow->addWidget(m_chip);
  chipRow->addStretch(1);
  headColumn->addLayout(chipRow);
  column->addWidget(head);

  // the hint line and the no-animation line, each with the info icon
  auto explainRow = [this](QLabel*& icon, QLabel*& label, const char* text, const QString& name) {
    QWidget* row = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::Size::spacingS);
    icon = new QLabel(row);
    icon->setAlignment(Qt::AlignTop);
    label = new WrapLabel(tr2(text), row);
    label->setObjectName(name);
    layout->addWidget(icon, 0, Qt::AlignTop);
    layout->addWidget(label, 1);
    return row;
  };
  m_hintRow = explainRow(m_hintIcon, m_hint, HandsSpec::hint, QStringLiteral("handsHint"));
  column->addWidget(m_hintRow);
  m_openRow = explainRow(m_openIcon, m_openLine, HandsSpec::openFirst, QStringLiteral("handsOpenFirst"));
  column->addWidget(m_openRow);

  // the ready-made finger poses
  QWidget* presetSection = new QWidget(this);
  QVBoxLayout* presetColumn = new QVBoxLayout(presetSection);
  presetColumn->setContentsMargins(0, 0, 0, 0);
  presetColumn->setSpacing(Theme::Size::spacingS);
  QHBoxLayout* presetHead = new QHBoxLayout;
  m_presetsHeading = sectionHeading(tr2(HandsSpec::presetsHeading), presetSection, QStringLiteral("presetsHeading"));
  m_headingBars << headingBar(presetSection);
  m_presetsCount = new QLabel(QString::number(m_presets.count()), presetSection);
  m_presetsCount->setObjectName(QStringLiteral("presetsCount"));
  presetHead->setSpacing(Theme::Size::spacingS);
  presetHead->addWidget(m_headingBars.last());
  presetHead->addWidget(m_presetsHeading, 1);
  presetHead->addWidget(m_presetsCount);
  presetColumn->addLayout(presetHead);
  m_tiles = new HandPresetTiles(presetSection);
  m_tiles->setPresets(m_presets);
  m_tiles->setAccessibleName(tr2(HandsSpec::presetsHeading));
  m_tiles->setProperty("reasonName", tr2(HandsSpec::presetsHeading));
  presetColumn->addWidget(m_tiles);
  column->addWidget(presetSection);
  connect(m_tiles, &HandPresetTiles::picked, this, &HandsPanel::pickPreset);

  // Bend each finger
  QWidget* bendSection = new QWidget(this);
  QVBoxLayout* bendColumn = new QVBoxLayout(bendSection);
  bendColumn->setContentsMargins(0, 0, 0, 0);
  bendColumn->setSpacing(Theme::Size::spacingS);
  m_bendHeading = sectionHeading(tr2(HandsSpec::bendHeading), bendSection, QStringLiteral("bendHeading"));
  {
    QHBoxLayout* head = new QHBoxLayout;
    head->setSpacing(Theme::Size::spacingS);
    m_headingBars << headingBar(bendSection);
    head->addWidget(m_headingBars.last());
    head->addWidget(m_bendHeading, 1);
    bendColumn->addLayout(head);
  }
  QGridLayout* fingers = new QGridLayout;
  fingers->setHorizontalSpacing(Theme::Size::spacingS);
  fingers->setVerticalSpacing(Theme::Size::spacingXs);
  for (int f = 0; f < HandPose::fingerCount; ++f)
  {
    m_names[f] = new QLabel(tr2(HandsSpec::fingerNames[f]), bendSection);
    m_names[f]->setObjectName(QStringLiteral("fingerName%1").arg(f));
    m_names[f]->setFont(weightOnly());
    m_sliders[f] = new QSlider(Qt::Horizontal, bendSection);
    m_sliders[f]->setObjectName(QStringLiteral("fingerSlider%1").arg(f));
    m_sliders[f]->setRange(0, 100);
    m_sliders[f]->setSingleStep(1);
    m_sliders[f]->setPageStep(10);
    m_sliders[f]->setFocusPolicy(Qt::StrongFocus);
    m_sliders[f]->setMinimumHeight(Theme::Size::controlCompact);
    m_sliders[f]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    const QString name = tr2(HandsSpec::sliderName).arg(tr2(HandsSpec::fingerNames[f]));
    m_sliders[f]->setAccessibleName(name);
    m_sliders[f]->setProperty("reasonName", name);
    m_names[f]->setBuddy(m_sliders[f]);
    m_values[f] = new QLabel(bendSection);
    m_values[f]->setObjectName(QStringLiteral("fingerValue%1").arg(f));
    m_values[f]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_values[f]->setMinimumWidth(m_values[f]->fontMetrics().horizontalAdvance(QStringLiteral("100%")) + 2);
    fingers->addWidget(m_names[f], f, 0);
    fingers->addWidget(m_sliders[f], f, 1);
    fingers->addWidget(m_values[f], f, 2);
    const int finger = f;
    connect(m_sliders[f], &QSlider::valueChanged, this, [this, finger](int value) {
      if (!m_refreshing) bendFinger(finger, value);
    });
  }
  QHBoxLayout* scale = new QHBoxLayout;
  scale->setContentsMargins(0, 0, 0, 0);
  m_scaleOpen = new QLabel(tr2(HandsSpec::open), bendSection);
  m_scaleOpen->setObjectName(QStringLiteral("scaleOpen"));
  m_scaleClosed = new QLabel(tr2(HandsSpec::closed), bendSection);
  m_scaleClosed->setObjectName(QStringLiteral("scaleClosed"));
  scale->addWidget(m_scaleOpen);
  scale->addStretch(1);
  scale->addWidget(m_scaleClosed);
  fingers->addLayout(scale, HandPose::fingerCount, 1);
  fingers->setColumnStretch(1, 1);
  bendColumn->addLayout(fingers);
  column->addWidget(bendSection);

  // Reset hand
  QHBoxLayout* resetRow = new QHBoxLayout;
  resetRow->setContentsMargins(0, 0, 0, 0);
  m_reset = new QPushButton(this);
  m_reset->setObjectName(QStringLiteral("resetHandButton"));
  m_reset->setAutoDefault(false);
  m_reset->setMinimumHeight(Theme::Size::controlCompact);
  resetRow->addWidget(m_reset);
  resetRow->addStretch(1);
  column->addLayout(resetRow);
  connect(m_reset, &QPushButton::clicked, this, &HandsPanel::resetHand);

  // the joint grid (Expert mode), under the sliders so switching modes moves nothing (charter P12)
  m_gridSection = new QWidget(this);
  m_gridSection->setObjectName(QStringLiteral("handGridSection"));
  QVBoxLayout* gridColumn = new QVBoxLayout(m_gridSection);
  gridColumn->setContentsMargins(0, 0, 0, 0);
  gridColumn->setSpacing(Theme::Size::spacingS);
  m_gridHeading = sectionHeading(tr2(HandsSpec::gridHeading), m_gridSection, QStringLiteral("gridHeading"));
  {
    QHBoxLayout* head = new QHBoxLayout;
    head->setSpacing(Theme::Size::spacingS);
    m_headingBars << headingBar(m_gridSection);
    head->addWidget(m_headingBars.last());
    head->addWidget(m_gridHeading, 1);
    gridColumn->addLayout(head);
  }
  m_grid = new HandJointGrid(m_gridSection);
  m_grid->setAccessibleName(tr2(HandsSpec::gridName));
  m_grid->setProperty("reasonName", tr2(HandsSpec::gridName));
  gridColumn->addWidget(m_grid);
  m_gridHint = new WrapLabel(tr2(HandsSpec::gridHint), m_gridSection);
  m_gridHint->setObjectName(QStringLiteral("gridHint"));
  gridColumn->addWidget(m_gridHint);
  column->addWidget(m_gridSection);
  connect(m_grid, &HandJointGrid::cellChanged, this, &HandsPanel::setCell);
  connect(m_grid, &HandJointGrid::rowPicked, this, [this](int row) {
    emit jointPicked(HandPose::jointName(shownSide(), row));
  });
  connect(m_grid, &HandJointGrid::invalidNumber, this, [this]() { emit statusMessage(tr2(HandsSpec::notANumber)); });

  m_expertLine = new WrapLabel(tr2(HandsSpec::expertLine), this);
  m_expertLine->setObjectName(QStringLiteral("handsExpertLine"));
  column->addWidget(m_expertLine);
  column->addStretch(1);

  // the Tab order of the mockup: the poses, the five sliders, Reset hand, the grid
  setTabOrder(m_tiles, m_sliders[0]);
  for (int f = 1; f < HandPose::fingerCount; ++f) setTabOrder(m_sliders[f - 1], m_sliders[f]);
  setTabOrder(m_sliders[HandPose::fingerCount - 1], m_reset);
  setTabOrder(m_reset, m_grid);

  applyFonts();
  applyLook();
  refresh();
}

// the caption labels are one point smaller than the panel's text; set again when
// the program's text size changes (a font set on a label does not follow by itself)
void HandsPanel::applyFonts()
{
  const QFont small = caption(font());
  for (QLabel* label : {m_chip, m_presetsCount, m_scaleOpen, m_scaleClosed, m_gridHint, m_expertLine})
    label->setFont(small);
}

void HandsPanel::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::FontChange) applyFonts();
}

QSize HandsPanel::sizeHint() const
{
  QSize hint = QWidget::sizeHint();
  if (layout() && layout()->hasHeightForWidth())
    hint.setHeight(std::max(hint.height(), layout()->totalHeightForWidth(hint.width())));
  return hint;
}

HandPose::Side HandsPanel::shownSide() const
{
  if (m_state.sideMode == QLatin1Char('L')) return HandPose::Side::Left;
  if (m_state.sideMode == QLatin1Char('R')) return HandPose::Side::Right;
  HandPose::Side side = HandPose::Side::Right;
  if (HandPose::indexOf(m_state.pickedJoint, &side) >= 0) return side;
  return HandPose::Side::Right;
}

void HandsPanel::setState(const State& state)
{
  m_state = state;
  if (!m_state.open && m_state.blocked.isEmpty()) m_state.blocked = tr2(HandsSpec::openWhy);
  refresh();
}

void HandsPanel::forgetPicks()
{
  m_lastPick[0].clear();
  m_lastPick[1].clear();
  refresh();
}

QString HandsPanel::blockedReason() const
{
  return m_state.blocked;
}

void HandsPanel::setReason(QWidget* widget, bool enabled, const QString& tip, const QString& why)
{
  widget->setEnabled(enabled);
  // a greyed control says why in its tip and in the status line (charter P6, src/reasonline.h)
  widget->setToolTip(enabled ? tip : why);
  widget->setStatusTip(enabled ? QString() : why);
}

void HandsPanel::refresh()
{
  if (m_refreshing) return;
  m_refreshing = true;
  const HandPose::Side side = shownSide();
  const bool left = side == HandPose::Side::Left;
  const bool mirror = m_state.sideMode.isNull();
  const HandPose::Hand& hand = left ? m_state.left : m_state.right;
  const HandPose::Hand& otherHand = left ? m_state.right : m_state.left;
  const QString why = blockedReason();
  const bool open = m_state.open;

  m_handName->setText(tr2(left ? HandsSpec::leftHand : HandsSpec::rightHand));
  m_handCaption->setText(tr2(mirror ? HandsSpec::copies : HandsSpec::onlyThis));

  // the state chip
  const QString match = open ? HandPose::match(hand, m_presets, side) : QString();
  const bool changed = open && HandPose::changed(hand);
  QString chip;
  if (open)
  {
    const QString& last = m_lastPick[left ? 0 : 1];
    if (!match.isEmpty()) chip = tr2(HandsSpec::poseIs).arg(match);
    else if (!changed) chip = tr2(HandsSpec::poseOpen);
    else if (!last.isEmpty()) chip = tr2(HandsSpec::poseChangedFrom).arg(last);
    else chip = tr2(HandsSpec::poseOwn);
  }
  m_chip->setText(chip);
  m_chip->setVisible(open);
  if (m_chip->property("ok").toBool() != !match.isEmpty())
  {
    m_chip->setProperty("ok", !match.isEmpty());
    styleChip();
  }

  m_hintRow->setVisible(m_state.hints && open);
  m_openRow->setVisible(!open);

  // the tiles
  const int matchIndex = HandPose::presetIndex(m_presets, match);
  m_tiles->setChecked(matchIndex);
  if (matchIndex >= 0 && !m_tiles->hasFocus()) m_tiles->setCurrent(matchIndex);
  const char* tip = mirror ? HandsSpec::presetTipBoth : (left ? HandsSpec::presetTipLeft : HandsSpec::presetTipRight);
  setReason(m_tiles, why.isEmpty(), QStringLiteral("%1 (%2)\n%3").arg(tr2(tip), tr2(HandsSpec::presetKey), tr2(HandsSpec::presetsKeys)), why);

  // the sliders
  for (int f = 0; f < HandPose::fingerCount; ++f)
  {
    const int value = (open && m_closed >= 0) ? HandPose::bend(hand, m_presets[m_closed].hand(side), f) : 0;
    {
      QSignalBlocker block(m_sliders[f]);
      m_sliders[f]->setValue(value);
    }
    m_sliders[f]->setAccessibleDescription(tr2(HandsSpec::percentClosed).arg(value));
    m_values[f]->setText(open ? QStringLiteral("%1%").arg(value) : QStringLiteral("–"));
    setReason(m_sliders[f], why.isEmpty() && m_closed >= 0,
              QStringLiteral("%1 (%2)").arg(m_sliders[f]->accessibleName(), tr2(HandsSpec::sliderKeys)), why);
  }

  // Reset hand
  m_reset->setText(tr2(mirror ? HandsSpec::resetBoth : HandsSpec::resetHand));
  const QString resetName = tr2(mirror ? HandsSpec::resetBoth : (left ? HandsSpec::resetLeftName : HandsSpec::resetRightName));
  m_reset->setAccessibleName(resetName);
  m_reset->setProperty("reasonName", resetName);
  const bool anything = changed || (mirror && open && HandPose::changed(otherHand));
  const QString resetWhy = !why.isEmpty() ? why : (anything ? QString() : tr2(HandsSpec::resetWhy));
  setReason(m_reset, resetWhy.isEmpty(), tr2(HandsSpec::resetTip), resetWhy);

  // the grid (Expert mode)
  m_gridSection->setVisible(m_state.expert);
  m_gridHint->setVisible(m_state.expert && m_state.hints);
  m_expertLine->setVisible(!m_state.expert && m_state.hints);
  m_grid->setHand(hand, open);
  HandPose::Side pickedSide;
  const int pickedRow = HandPose::indexOf(m_state.pickedJoint, &pickedSide);
  m_grid->setSelectedRow(pickedRow >= 0 && pickedSide == side ? pickedRow : -1);
  setReason(m_grid, why.isEmpty(), tr2(HandsSpec::gridKeys), why);
  m_refreshing = false;
}

void HandsPanel::pickPreset(int index)
{
  if (index < 0 || index >= m_presets.count() || !blockedReason().isEmpty()) return;
  const bool mirror = m_state.sideMode.isNull();
  const HandPose::Side side = shownSide();
  const bool left = mirror || side == HandPose::Side::Left;
  const bool right = mirror || side == HandPose::Side::Right;
  const HandPose::Preset& preset = m_presets[index];
  if (left) m_lastPick[0] = preset.name;
  if (right) m_lastPick[1] = preset.name;
  const char* text = mirror ? HandsSpec::pickedBoth : (side == HandPose::Side::Left ? HandsSpec::pickedLeft : HandsSpec::pickedRight);
  emit changeRequested(HandPose::presetWrites(preset, left, right), tr2(text).arg(preset.name));
}

void HandsPanel::bendFinger(int finger, int percent)
{
  if (m_closed < 0 || !blockedReason().isEmpty()) return;
  const HandPose::Side side = shownSide();
  const HandPose::Hand& hand = side == HandPose::Side::Left ? m_state.left : m_state.right;
  const HandPose::Hand bentHand = HandPose::withBend(hand, m_presets[m_closed].hand(side), finger, percent);
  QList<int> rows;
  for (int part = 0; part < HandPose::partCount; ++part) rows << finger * HandPose::partCount + part;
  m_values[finger]->setText(QStringLiteral("%1%").arg(percent));
  emit changeRequested(HandPose::handWrites(side, bentHand, rows, m_state.sideMode.isNull()),
                       tr2(HandsSpec::bent).arg(tr2(HandsSpec::fingerNames[finger])).arg(percent));
}

void HandsPanel::setCell(int row, int axis, double value)
{
  if (row < 0 || row >= HandPose::jointCount || axis < 0 || axis > 2 || !blockedReason().isEmpty()) return;
  const HandPose::Side side = shownSide();
  HandPose::Hand hand = side == HandPose::Side::Left ? m_state.left : m_state.right;
  value = std::clamp(value, -180.0, 180.0);
  hand[row][axis] = static_cast<float>(value);
  emit changeRequested(HandPose::handWrites(side, hand, {row}, m_state.sideMode.isNull()),
                       tr2(HandsSpec::cellSet).arg(HandsSpec::jointLabel(row), QLatin1String(axisLetter(axis))).arg(wholeDegrees(value)));
}

void HandsPanel::resetHand()
{
  if (!blockedReason().isEmpty()) return;
  const bool mirror = m_state.sideMode.isNull();
  const HandPose::Side side = shownSide();
  const bool left = mirror || side == HandPose::Side::Left;
  const bool right = mirror || side == HandPose::Side::Right;
  if (left) m_lastPick[0].clear();
  if (right) m_lastPick[1].clear();
  const char* text = mirror ? HandsSpec::resetDoneBoth : (side == HandPose::Side::Left ? HandsSpec::resetDoneLeft : HandsSpec::resetDoneRight);
  emit changeRequested(HandPose::resetWrites(left, right), tr2(text));
}

void HandsPanel::styleChip()
{
  using Theme::Token;
  // the state chip: round, the success colour when the hand matches a preset (the mockup's .chip.ok)
  const QColor chipColour = Theme::color(m_chip->property("ok").toBool() ? Token::Success : Token::Border);
  m_chip->setStyleSheet(QStringLiteral("QLabel { border: %1px solid %2; border-radius: 10px; padding: 0px %3px; min-height: 18px; color: %4; background: %5; }")
                          .arg(Theme::Size::borderWidth)
                          .arg(chipColour.name())
                          .arg(Theme::Size::spacingS)
                          .arg(Theme::color(Token::Text).name(), Theme::mix(Theme::color(Token::Surface), chipColour, 0.14).name()));
}

void HandsPanel::applyLook()
{
  using Theme::Token;
  const QColor muted = Theme::color(Token::TextMuted);
  const QColor disabled = Theme::color(Token::TextDisabled);
  const QColor accentText = Theme::color(Token::AccentText);
  const int icon = Theme::Size::iconSmall;

  m_handIcon->setPixmap(LucideIcons::icon(QStringLiteral("hand"), Theme::color(Token::AreaHands), disabled).pixmap(20, 20));
  for (QLabel* info : {m_hintIcon, m_openIcon})
    info->setPixmap(LucideIcons::icon(QStringLiteral("info"), accentText, disabled).pixmap(icon, icon));
  for (QLabel* label : {m_handCaption, m_hint, m_openLine, m_presetsCount, m_scaleOpen, m_scaleClosed, m_gridHint, m_expertLine})
    label->setStyleSheet(QStringLiteral("color: %1;").arg(muted.name()));
  // the section headings' bar in accent_text (the mockup's .sech)
  for (QFrame* bar : m_headingBars)
  {
    QPalette palette = bar->palette();
    palette.setColor(QPalette::Window, accentText);
    bar->setPalette(palette);
  }
  styleChip();
  m_reset->setIcon(LucideIcons::icon(QStringLiteral("rotate-ccw"), Theme::color(Token::Text), disabled));

  // the finger sliders in the accent colour, the focus ring on the handle (as the Selected joint panel)
  const QColor border = Theme::color(Token::Border);
  const QString groove = QString::number(Theme::Size::spacingXs);
  const QString slider =
    QStringLiteral("QSlider::groove:horizontal { height: %1px; background: %2; border-radius: %3px; }")
      .arg(groove, border.name(), QString::number(Theme::Size::spacingXs / 2)) +
    QStringLiteral("QSlider::sub-page:horizontal { background: %1; border-radius: %2px; }")
      .arg(Theme::color(Token::Accent).name(), QString::number(Theme::Size::spacingXs / 2)) +
    QStringLiteral("QSlider::handle:horizontal { background: %1; border: %2px solid %3; width: %4px; margin: -%5px 0px; border-radius: %6px; }")
      .arg(Theme::color(Token::SurfaceRaised).name(), QString::number(Theme::Size::focusRingWidth), Theme::color(Token::Accent).name(),
           QString::number(icon), QString::number((icon - Theme::Size::spacingXs) / 2), QString::number(icon / 2)) +
    QStringLiteral("QSlider::handle:horizontal:focus { border-color: %1; }").arg(Theme::color(Token::FocusRing).name()) +
    QStringLiteral("QSlider::sub-page:horizontal:disabled { background: %1; } QSlider::handle:horizontal:disabled { border-color: %1; }")
      .arg(disabled.name());
  for (QSlider* s : m_sliders) s->setStyleSheet(slider);
  m_tiles->update();
  m_grid->update();
}
