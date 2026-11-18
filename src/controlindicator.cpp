/*
 * Pingmator - the items of the status bar (issues #43 and #88)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QToolButton>

#include "controlindicator.h"

namespace
{
  // dot colours: grey (off), yellow (waiting), green (connected / live), red (problem)
  const char* const GREY = "#8a8a8a";
  const char* const YELLOW = "#d4a017";
  const char* const GREEN = "#2fa84f";
  const char* const RED = "#d0342c";

  // the clickable coloured dot at the start of an item
  QToolButton* makeDot(QWidget* parent)
  {
    QToolButton* dot = new QToolButton(parent);
    dot->setAutoRaise(true);
    dot->setText(QStringLiteral("●"));
    dot->setCursor(Qt::PointingHandCursor);
    return dot;
  }

  void setDotColour(QToolButton* dot, const char* colour)
  {
    dot->setStyleSheet(QStringLiteral("QToolButton { color: %1; font-size: 14px; padding: 0 2px; border: none; background: transparent; }").arg(colour));
  }

  QHBoxLayout* pillLayout(QWidget* pill)
  {
    QHBoxLayout* layout = new QHBoxLayout(pill);
    layout->setContentsMargins(4, 1, 10, 1);
    layout->setSpacing(2);
    return layout;
  }
}

StatusPill::StatusPill(QWidget* parent) :
  QWidget(parent)
{
  setAttribute(Qt::WA_Hover);
}

void StatusPill::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const QRectF box = QRectF(rect()).adjusted(0.5, 1.5, -0.5, -1.5);
  const qreal radius = box.height() / 2.0;
  painter.setPen(QPen(palette().color(QPalette::Midlight), 1.0));
  painter.setBrush(palette().color(underMouse() ? QPalette::Light : QPalette::Button));
  painter.drawRoundedRect(box, radius, radius);
}

void StatusPill::enterEvent(QEnterEvent* event)
{
  QWidget::enterEvent(event);
  update();
}

void StatusPill::leaveEvent(QEvent* event)
{
  QWidget::leaveEvent(event);
  update();
}

ControlIndicator::ControlIndicator(QWidget* parent) :
  StatusPill(parent),
  m_led(makeDot(this)),
  m_label(new QLabel(this)),
  m_timer(new QTimer(this))
{
  QHBoxLayout* layout = pillLayout(this);
  layout->addWidget(m_led);
  layout->addWidget(m_label);

  m_led->setToolTip(tr("Remote control. Click to turn it on or off.\n"
                       "Grey: off\nYellow: on, no assistant connected\n"
                       "Green: assistant connected\nRed: a request with a wrong key was refused"));
  connect(m_led, &QToolButton::clicked, this, &ControlIndicator::toggleRequested);

  // the "3 s ago" text and the timeout need a tick while remote control is on
  m_timer->setInterval(1000);
  connect(m_timer, &QTimer::timeout, this, &ControlIndicator::refresh);

  refresh();
}

ControlIndicator::State ControlIndicator::stateFor(bool running, qint64 msSinceContact, qint64 msSinceRefused)
{
  if (!running) return Off;
  // a refused request counts until a good one comes after it
  if (msSinceRefused >= 0 && msSinceRefused < CONTACT_TIMEOUT_MS && (msSinceContact < 0 || msSinceRefused <= msSinceContact))
    return Problem;
  if (msSinceContact >= 0 && msSinceContact < CONTACT_TIMEOUT_MS) return Connected;
  return Waiting;
}

QString ControlIndicator::ago(qint64 ms)
{
  const qint64 seconds = qMax<qint64>(0, ms / 1000);
  if (seconds < 60) return tr("%1 s ago").arg(seconds);
  if (seconds < 3600) return tr("%1 min ago").arg(seconds / 60);
  return tr("%1 h ago").arg(seconds / 3600);
}

QString ControlIndicator::textFor(State state, const QString& client, const QString& command, qint64 msSinceCommand)
{
  switch (state)
  {
    case Off: return tr("Remote control off");
    case Waiting: return tr("Remote control on, no assistant connected");
    case Problem: return tr("Remote control: request with a wrong key refused");
    case Connected: break;
  }
  QString text = tr("%1 connected").arg(client.isEmpty() ? tr("A program") : client);
  if (!command.isEmpty() && msSinceCommand >= 0)
    text += QStringLiteral(" · ") + command + " (" + ago(msSinceCommand) + ")";
  return text;
}

ControlIndicator::State ControlIndicator::state() const
{
  return stateFor(m_running, since(m_lastContact), since(m_lastRefused));
}

QString ControlIndicator::text() const
{
  return m_label->text();
}

void ControlIndicator::setRunning(bool running)
{
  if (running == m_running) return;
  m_running = running;
  if (!running)
  {
    m_lastContact = m_lastRefused = m_lastCommandTime = -1;
    m_client.clear();
    m_command.clear();
    m_timer->stop();
  }
  else
    m_timer->start();
  refresh();
}

void ControlIndicator::onContact(const QString& client, const QString& command, bool ok)
{
  m_lastContact = QDateTime::currentMSecsSinceEpoch();
  m_lastRefused = -1;   // a good request clears the problem
  if (!client.isEmpty()) m_client = client;
  if (!command.isEmpty())
  {
    m_command = command;
    m_commandOk = ok;
    m_lastCommandTime = m_lastContact;
  }
  refresh();
}

void ControlIndicator::onRefused()
{
  m_lastRefused = QDateTime::currentMSecsSinceEpoch();
  refresh();
}

qint64 ControlIndicator::since(qint64 time) const
{
  return time < 0 ? -1 : QDateTime::currentMSecsSinceEpoch() - time;
}

void ControlIndicator::refresh()
{
  const State current = state();
  static const char* const colours[] = { GREY, YELLOW, GREEN, RED };
  setDotColour(m_led, colours[current]);

  const QString command = m_commandOk || m_command.isEmpty() ? m_command : tr("%1 failed").arg(m_command);
  m_label->setText(textFor(current, m_client, command, since(m_lastCommandTime)));
}

AuroraIndicator::AuroraIndicator(QWidget* parent) :
  StatusPill(parent),
  m_led(makeDot(this)),
  m_label(new QLabel(this))
{
  QHBoxLayout* layout = pillLayout(this);
  layout->addWidget(m_led);
  layout->addWidget(m_label);

  m_led->setToolTip(tr("Live link to Aurora. Click to turn it on or off.\n"
                       "Grey: off\n"
                       "Yellow: waiting for Aurora (in Aurora, open LiveMesh and click Enable preview)\n"
                       "Green: live, the pose shows on your avatar"));
  connect(m_led, &QToolButton::clicked, this, &AuroraIndicator::toggleRequested);

  refresh();
}

AuroraIndicator::State AuroraIndicator::stateFor(bool enabled, bool connected)
{
  if (!enabled) return Off;
  return connected ? Live : Connecting;
}

QString AuroraIndicator::textFor(State state)
{
  switch (state)
  {
    case Off: return tr("Aurora off");
    case Connecting: return tr("Aurora · connecting…");
    case Live: break;
  }
  return tr("Aurora · live");
}

AuroraIndicator::State AuroraIndicator::state() const
{
  return m_state;
}

QString AuroraIndicator::text() const
{
  return m_label->text();
}

void AuroraIndicator::setStatus(bool enabled, bool connected)
{
  m_state = stateFor(enabled, connected);
  refresh();
}

void AuroraIndicator::refresh()
{
  static const char* const colours[] = { GREY, YELLOW, GREEN };
  setDotColour(m_led, colours[m_state]);
  m_label->setText(textFor(m_state));
}

TipsSwitch::TipsSwitch(QWidget* parent) :
  StatusPill(parent),
  m_button(new QToolButton(this))
{
  QHBoxLayout* layout = pillLayout(this);
  layout->setContentsMargins(6, 1, 8, 1);
  layout->addWidget(m_button);

  m_button->setCheckable(true);
  m_button->setChecked(true);
  m_button->setAutoRaise(true);
  m_button->setCursor(Qt::PointingHandCursor);
  m_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_button->setIconSize(QSize(14, 14));
  // the pill shows the state; no pressed-button frame on top of it
  m_button->setStyleSheet(QStringLiteral("QToolButton { border: none; background: transparent; padding: 0; }"));
  m_button->setToolTip(tr("Help tips: short help when the mouse rests on a control.\nClick to turn them on or off."));
  connect(m_button, &QToolButton::clicked, this, [this](bool on)
  {
    refresh();
    emit toggled(on);
  });

  refresh();
}

QString TipsSwitch::textFor(bool on)
{
  return on ? tr("Tips on") : tr("Tips off");
}

bool TipsSwitch::isOn() const
{
  return m_button->isChecked();
}

QString TipsSwitch::text() const
{
  return m_button->text();
}

void TipsSwitch::setOn(bool on)
{
  m_button->setChecked(on);
  refresh();
}

void TipsSwitch::changeEvent(QEvent* event)
{
  StatusPill::changeEvent(event);
  // the icon is drawn in the text colour of the theme
  if (event->type() == QEvent::PaletteChange) refresh();
}

void TipsSwitch::refresh()
{
  const bool on = isOn();
  m_button->setText(textFor(on));

  // a question mark in a circle, dimmed when the tips are off
  const qreal scale = devicePixelRatioF();
  QPixmap icon(QSize(14, 14) * scale);
  icon.setDevicePixelRatio(scale);
  icon.fill(Qt::transparent);
  QPainter painter(&icon);
  painter.setRenderHint(QPainter::Antialiasing);
  const QColor colour = palette().color(on ? QPalette::Active : QPalette::Disabled, QPalette::WindowText);
  painter.setPen(QPen(colour, 1.2));
  painter.drawEllipse(QRectF(1.0, 1.0, 12.0, 12.0));
  QFont font = painter.font();
  font.setPixelSize(9);
  font.setBold(true);
  painter.setFont(font);
  painter.drawText(QRectF(0.0, 0.0, 14.0, 14.0), Qt::AlignCenter, QStringLiteral("?"));
  painter.end();
  m_button->setIcon(QIcon(icon));
}
