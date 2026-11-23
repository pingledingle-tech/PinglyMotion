/*
 * PinglyMotion Script Lab - a folding part of the Script Lab panel
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

#include "lucideicons.h"
#include "theme.h"

#include "scriptlabsection.h"

void ScriptLabFrame::setLook(const QColor& border, const QColor& fill, int radius, bool dashed)
{
  m_border = border;
  m_fill = fill;
  m_radius = radius;
  m_dashed = dashed;
  update();
}

void ScriptLabFrame::paintEvent(QPaintEvent*)
{
  if (!m_border.isValid() && !m_fill.isValid()) return;
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(m_border.isValid() ? m_border : Qt::transparent, Theme::Size::borderWidth);
  if (m_dashed) pen.setStyle(Qt::DashLine);
  painter.setPen(pen);
  painter.setBrush(m_fill.isValid() ? QBrush(m_fill) : Qt::NoBrush);
  const qreal half = Theme::Size::borderWidth / 2.0;
  painter.drawRoundedRect(QRectF(rect()).adjusted(half, half, -half, -half), m_radius, m_radius);
}

ScriptLabSection::ScriptLabSection(const QString& key, const QString& title, bool expanded, QWidget* parent) :
  ScriptLabFrame(parent),
  m_key(key),
  m_title(title)
{
  setObjectName(QStringLiteral("scriptLabSection_") + key);

  QVBoxLayout* outer = new QVBoxLayout(this);
  // the mockup's .labsec: header padding s (top, bottom) and m (sides), body padding m
  outer->setContentsMargins(Theme::Size::spacingM, Theme::Size::spacingXs, Theme::Size::spacingM, Theme::Size::spacingXs);
  outer->setSpacing(Theme::Size::spacingXs);

  QHBoxLayout* row = new QHBoxLayout;
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(Theme::Size::spacingS);
  m_header = new QPushButton(this);
  m_header->setObjectName(QStringLiteral("scriptLabSectionHeader_") + key);
  m_header->setCheckable(true);
  m_header->setChecked(expanded);
  m_header->setFlat(true);
  m_header->setFocusPolicy(Qt::StrongFocus);
  m_header->setMinimumHeight(Theme::Size::controlCompact);
  m_header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  QFont label = m_header->font();
  label.setWeight(QFont::DemiBold);   // the "label" type token
  m_header->setFont(label);
  row->addWidget(m_header, 1);

  m_countLabel = new QLabel(this);
  m_countLabel->setObjectName(QStringLiteral("scriptLabSectionCount_") + key);
  QFont caption = m_countLabel->font();   // the "caption" type token: 11 px next to the 13 px body
  if (caption.pointSizeF() > 0) caption.setPointSizeF(caption.pointSizeF() * 11.0 / 13.0);
  else caption.setPixelSize(qMax(1, caption.pixelSize() * 11 / 13));
  m_countLabel->setFont(caption);
  m_countLabel->hide();
  row->addWidget(m_countLabel);
  outer->addLayout(row);

  m_body = new QWidget(this);
  m_body->setObjectName(QStringLiteral("scriptLabSectionBody_") + key);
  m_bodyLayout = new QVBoxLayout(m_body);
  m_bodyLayout->setContentsMargins(0, 0, 0, Theme::Size::spacingS);
  m_bodyLayout->setSpacing(Theme::Size::spacingS);
  m_body->setVisible(expanded);
  outer->addWidget(m_body, 1);

  connect(m_header, &QPushButton::toggled, this, [this](bool on)
  {
    m_body->setVisible(on);
    updateHeader();
    emit expandedChanged(on);
  });
  updateHeader();
}

void ScriptLabSection::setTitle(const QString& title)
{
  m_title = title;
  updateHeader();
}

void ScriptLabSection::setCount(const QString& count)
{
  m_count = count;
  m_countLabel->setText(count);
  m_countLabel->setVisible(!count.isEmpty());
  updateHeader();
}

void ScriptLabSection::setExpanded(bool expanded)
{
  m_header->setChecked(expanded);
}

bool ScriptLabSection::isExpanded() const
{
  return m_header->isChecked();
}

void ScriptLabSection::updateHeader()
{
  m_header->setText(m_title);
  // a screen reader hears the name, the count and whether the part is open
  m_header->setAccessibleName(m_count.isEmpty() ? m_title : m_title + QStringLiteral(", ") + m_count);
  m_header->setToolTip(isExpanded() ? tr("Hide this part (Space)") : tr("Show this part (Space)"));
  const QColor text = Theme::color(Theme::Token::Text, m_dark);
  m_header->setIcon(LucideIcons::icon(isExpanded() ? QStringLiteral("chevron-down") : QStringLiteral("chevron-right"), text,
                                      Theme::color(Theme::Token::TextDisabled, m_dark)));
  // job U3: open and folded parts can be told apart at a glance, not only by the chevron: a folded
  // part is filled like a normal button (surface_raised), an open part is not
  const QColor surface = Theme::color(Theme::Token::Surface, m_dark);
  const QColor frame = Theme::mix(surface, Theme::color(Theme::Token::Border, m_dark), 0.6);
  setLook(frame, isExpanded() ? QColor() : Theme::color(Theme::Token::SurfaceRaised, m_dark), Theme::Size::radiusControl, false);
}

void ScriptLabSection::applyStyle(bool dark)
{
  m_dark = dark;
  // the mockup's .labsec border: the border token at 60 percent on the panel (set in updateHeader,
  // with the fill of a folded part)
  m_header->setStyleSheet(QStringLiteral(
    "QPushButton { background: transparent; color: %1; border: none; text-align: left; padding: 0; }"
    "QPushButton:focus { border: %2px solid %3; border-radius: %4px; }")
      .arg(Theme::color(Theme::Token::Text, dark).name(QColor::HexRgb).toUpper())
      .arg(Theme::Size::focusRingWidth)
      .arg(Theme::color(Theme::Token::FocusRing, dark).name(QColor::HexRgb).toUpper())
      .arg(Theme::Size::radiusControl));
  QPalette p = m_countLabel->palette();
  p.setColor(QPalette::WindowText, Theme::color(Theme::Token::TextMuted, dark));
  m_countLabel->setPalette(p);
  updateHeader();
}
