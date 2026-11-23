/*
 * PinglyMotion - the items of the status bar (issues #43, #88 and #7)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QTimer>
#include <QToolButton>

#include "aiassistant.h"
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

  // the coloured dot of the AI Assistant button, drawn as an icon so the dot and the words are one button
  QIcon dotIcon(const char* colour)
  {
    QPixmap pixmap(QSize(24, 24));
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QLatin1String(colour)));
    painter.drawEllipse(QRectF(2.0, 2.0, 20.0, 20.0));
    painter.end();
    return QIcon(pixmap);
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
  m_button(new QToolButton(this)),
  m_timer(new QTimer(this))
{
  QHBoxLayout* layout = pillLayout(this);
  layout->setContentsMargins(4, 1, 8, 1);
  layout->addWidget(m_button);

  // one button: the dot and the words; reachable with Tab, Space or Enter clicks it (charter P8)
  m_button->setObjectName(QStringLiteral("aiAssistantButton"));
  m_button->setAutoRaise(true);
  m_button->setCursor(Qt::PointingHandCursor);
  m_button->setFocusPolicy(Qt::TabFocus);
  m_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_button->setIconSize(QSize(12, 12));
  m_button->setStyleSheet(QStringLiteral("QToolButton { border: none; background: transparent; padding: 0 2px; }"));
  connect(m_button, &QToolButton::clicked, this, &ControlIndicator::clicked);

  // the "3 s ago" text and the timeout need a tick while it is on
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

QString ControlIndicator::textFor(State state, const QStringList& connected)
{
  switch (state)
  {
    case Off: return tr("AI Assistant off");
    case Waiting: return tr("AI Assistant on, waiting for an AI");
    case Problem: return tr("AI Assistant: a wrong key was refused");
    case Connected: break;
  }
  if (connected.size() > 1) return tr("AI Assistant: %1 AI connected").arg(connected.size());
  return tr("AI Assistant: %1").arg(connected.isEmpty() ? AiAssistant::friendlyName(QString()) : connected.first());
}

QString ControlIndicator::tipFor(const QString& command, qint64 msSinceCommand)
{
  QString tip = tr("AI Assistant: lets an AI on this PC pose and play the animation.\n"
                   "Click to set it up, or to turn it on or off.\n"
                   "Grey: off (it is off each time PinglyMotion starts)\n"
                   "Yellow: on, waiting for an AI\n"
                   "Green: an AI is connected\n"
                   "Red: a request with a wrong key was refused");
  if (!command.isEmpty() && msSinceCommand >= 0)
    tip += QLatin1Char('\n') + tr("Last command: %1 (%2)").arg(command, AiAssistant::ago(msSinceCommand));
  return tip;
}

ControlIndicator::State ControlIndicator::state() const
{
  return stateFor(m_running, since(m_lastContact), since(m_lastRefused));
}

QString ControlIndicator::text() const
{
  return m_button->text();
}

bool ControlIndicator::running() const
{
  return m_running;
}

QToolButton* ControlIndicator::button() const
{
  return m_button;
}

QStringList ControlIndicator::connectedNames() const
{
  QStringList names;
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (m_running)
    for (auto it = m_contacts.constBegin(); it != m_contacts.constEnd(); ++it)
      if (now - it.value() < CONTACT_TIMEOUT_MS) names << it.key();
  names.sort();
  return names;
}

QHash<QString, qint64> ControlIndicator::sessionContacts() const
{
  return m_contacts;
}

QMenu* ControlIndicator::buildMenu(QWidget* parent)
{
  QMenu* menu = new QMenu(parent);
  menu->setObjectName(QStringLiteral("aiAssistantMenu"));
  menu->setAttribute(Qt::WA_DeleteOnClose);
  const QList<AiAssistant::MenuLine> lines =
    AiAssistant::menuLines(m_running, AiAssistant::knownAis(), m_contacts, QDateTime::currentMSecsSinceEpoch());
  for (const AiAssistant::MenuLine& line : lines)
  {
    if (line.kind == AiAssistant::MenuLine::Separator)
    {
      menu->addSeparator();
      continue;
    }
    QAction* action = menu->addAction(line.text);
    action->setObjectName(line.id);
    if (line.kind == AiAssistant::MenuLine::Info)
    {
      action->setEnabled(false);
      continue;
    }
    if (line.id == QLatin1String("aiTurnOnAction")) connect(action, &QAction::triggered, this, &ControlIndicator::turnOnRequested);
    else if (line.id == QLatin1String("aiTurnOffAction")) connect(action, &QAction::triggered, this, &ControlIndicator::turnOffRequested);
    else if (line.id == QLatin1String("aiConnectAnotherAction")) connect(action, &QAction::triggered, this, &ControlIndicator::connectAnotherRequested);
  }
  return menu;
}

void ControlIndicator::setRunning(bool running)
{
  if (running == m_running) return;
  m_running = running;
  if (!running)
  {
    m_lastContact = m_lastRefused = m_lastCommandTime = -1;
    m_command.clear();
    m_timer->stop();
  }
  else
    m_timer->start();
  refresh();
}

void ControlIndicator::onContact(const QString& client, const QString& command, bool ok)
{
  if (!m_running) return;   // nothing can talk to a stopped server; a late signal changes nothing
  m_lastContact = QDateTime::currentMSecsSinceEpoch();
  m_lastRefused = -1;   // a good request clears the problem
  const QString name = AiAssistant::friendlyName(client);
  m_contacts.insert(name, m_lastContact);
  // the name is remembered (never the key); written at most once a minute per name
  if (!client.trimmed().isEmpty() && (!m_remembered.contains(name) || m_lastContact - m_remembered.value(name) > 60000))
  {
    AiAssistant::remember(name, m_lastContact);
    m_remembered.insert(name, m_lastContact);
  }
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
  if (!m_running) return;
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
  m_button->setIcon(dotIcon(colours[current]));
  m_button->setText(textFor(current, connectedNames()));
  const QString command = m_commandOk || m_command.isEmpty() ? m_command : tr("%1 failed").arg(m_command);
  m_button->setToolTip(tipFor(command, since(m_lastCommandTime)));
  m_button->setAccessibleName(m_button->text());
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
                       "Yellow: waiting for Aurora. Turn the preview on in Aurora.\n"
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
    case Connecting: return tr("Waiting for Aurora");
    case Live: break;
  }
  return tr("Aurora live");
}

QString AuroraIndicator::titleSuffixFor(State state)
{
  // the window title says the same words as the status bar
  return state == Off ? QString() : QStringLiteral(" - ") + textFor(state);
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
