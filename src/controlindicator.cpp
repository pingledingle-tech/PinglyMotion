/*
 * Pingmator - remote-control indicator in the status bar (issue #43)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QToolButton>

#include "controlindicator.h"

ControlIndicator::ControlIndicator(QWidget* parent) :
  QWidget(parent),
  m_led(new QToolButton(this)),
  m_label(new QLabel(this)),
  m_timer(new QTimer(this))
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 4, 0);
  layout->setSpacing(2);
  layout->addWidget(m_led);
  layout->addWidget(m_label);

  m_led->setAutoRaise(true);
  m_led->setText(QStringLiteral("●"));
  m_led->setCursor(Qt::PointingHandCursor);
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
  static const char* colours[] = { "#8a8a8a", "#d4a017", "#2fa84f", "#d0342c" };
  m_led->setStyleSheet(QStringLiteral("QToolButton { color: %1; font-size: 14px; padding: 0 2px; }").arg(colours[current]));

  const QString command = m_commandOk || m_command.isEmpty() ? m_command : tr("%1 failed").arg(m_command);
  m_label->setText(textFor(current, m_client, command, since(m_lastCommandTime)));
}
