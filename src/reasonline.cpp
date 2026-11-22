/*
 * PinglyMotion - the reason line in the status bar (see reasonline.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "reasonline.h"

#include <QAbstractButton>
#include <QAction>
#include <QCoreApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QStatusBar>
#include <QStatusTipEvent>
#include <QToolButton>
#include <QWidget>

#include "lucideicons.h"
#include "textsize.h"
#include "theme.h"

namespace
{
  QString cleaned(QString text)
  {
    const int tab = text.indexOf(QLatin1Char('\t'));
    if (tab >= 0) text.truncate(tab);
    text.remove(QLatin1Char('&'));
    return text.trimmed();
  }

  // "Undo  (Ctrl+Z)" -> "Undo"
  QString withoutKey(const QString& tip)
  {
    QString first = tip.section(QLatin1Char('\n'), 0, 0);
    static const QRegularExpression key(QStringLiteral("\\s+\\([^()]*\\)$"));
    first.remove(key);
    return first.trimmed();
  }

  QString nameOf(const QWidget* widget, const QString& why)
  {
    const QVariant given = widget->property("reasonName");
    if (given.isValid() && !given.toString().isEmpty()) return given.toString();
    if (!widget->accessibleName().isEmpty()) return widget->accessibleName();
    const QAction* action = nullptr;
    if (const auto* tool = qobject_cast<const QToolButton*>(widget)) action = tool->defaultAction();
    if (const auto* button = qobject_cast<const QAbstractButton*>(widget))
    {
      const QString text = cleaned(button->text());
      if (!text.isEmpty()) return text;
    }
    if (action)
    {
      const QString text = cleaned(action->text());
      if (!text.isEmpty()) return text;
    }
    // a tip that only repeats the reason is no name
    const QString tip = withoutKey(widget->toolTip());
    if (tip == why || widget->toolTip().section(QLatin1Char('\n'), 0, 0) == why) return QString();
    return tip;
  }
}

ReasonLine::ReasonLine(QStatusBar* bar, QWidget* window)
  : QObject(window), m_bar(bar), m_window(window)
{
  m_box = new QWidget;
  m_box->setObjectName(QStringLiteral("reasonLine"));
  QHBoxLayout* layout = new QHBoxLayout(m_box);
  layout->setContentsMargins(Theme::Size::spacingS, 0, Theme::Size::spacingS, 0);
  layout->setSpacing(Theme::Size::spacingS);
  m_icon = new QLabel(m_box);
  m_icon->setObjectName(QStringLiteral("reasonLineIcon"));
  m_text = new QLabel(m_box);
  m_text->setObjectName(QStringLiteral("reasonLineText"));
  // a long reason is cut off at the right edge instead of making the window wider
  m_text->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_text->setMinimumWidth(0);
  m_text->setTextInteractionFlags(Qt::NoTextInteraction);
  m_icon->hide();
  layout->addWidget(m_icon);
  layout->addWidget(m_text, 1);
  bar->insertWidget(0, m_box, 1);
  applyLook();
  qApp->installEventFilter(this);
}

ReasonLine::~ReasonLine()
{
  if (qApp) qApp->removeEventFilter(this);
}

QString ReasonLine::lineText(const QString& name, const QString& why)
{
  return name.isEmpty() ? why : name + QStringLiteral(": ") + why;
}

ReasonLine::Reason ReasonLine::reasonFor(const QWidget* widget)
{
  if (!widget || widget->isEnabled()) return {};

  // a panel greyed as a whole: the part with the properties speaks for everything in it
  for (const QWidget* w = widget; w; w = w->parentWidget())
  {
    const QString why = w->property("reasonWhy").toString();
    if (!why.isEmpty()) return {w->property("reasonName").toString(), why};
    if (w->isWindow()) break;
  }

  QString why;
  if (const auto* tool = qobject_cast<const QToolButton*>(widget); tool && tool->defaultAction())
    why = tool->defaultAction()->statusTip();
  if (why.isEmpty()) why = widget->statusTip();
  if (why.isEmpty()) return {};
  return {nameOf(widget, why), why};
}

ReasonLine::Reason ReasonLine::reasonFor(const QAction* action)
{
  if (!action || action->isEnabled() || action->isSeparator() || action->statusTip().isEmpty()) return {};
  return {cleaned(action->text()), action->statusTip()};
}

void ReasonLine::watchMenus()
{
  for (QMenu* menu : m_window->findChildren<QMenu*>())
  {
    if (m_watched.contains(menu)) continue;
    m_watched.insert(menu);
    connect(menu, &QMenu::hovered, this, [this, menu](QAction* action) { menuHovered(menu, action); });
    connect(menu, &QMenu::aboutToHide, this, [this, menu]()
    {
      if (m_source == menu) clear();
    });
    connect(menu, &QObject::destroyed, this, [this, menu]() { m_watched.remove(menu); });
  }
}

void ReasonLine::menuHovered(QMenu* menu, QAction* action)
{
  const Reason reason = reasonFor(action);
  if (reason.valid())
  {
    m_source = menu;
    show(reason);
  }
  else if (m_showing && m_source == menu)
    clear();
}

QString ReasonLine::text() const
{
  return m_showing ? m_text->text() : QString();
}

void ReasonLine::show(const Reason& reason)
{
  if (!reason.valid())
  {
    clear();
    return;
  }
  m_why = reason.why;
  m_showing = true;
  m_bar->clearMessage();
  m_text->setText(lineText(reason.name, reason.why));
  m_icon->show();
}

void ReasonLine::clear()
{
  m_showing = false;
  m_why.clear();
  m_source.clear();
  m_text->clear();
  m_icon->hide();
}

void ReasonLine::applyLook()
{
  using Theme::Token;
  const QColor text = Theme::color(Token::Text);
  const int icon = TextSize::scaled(Theme::Size::iconSmall);
  m_icon->setPixmap(LucideIcons::icon(QStringLiteral("info"), text, text).pixmap(icon, icon));
  m_icon->setFixedSize(icon, icon);
  m_text->setStyleSheet(QStringLiteral("QLabel { color: %1; background: transparent; }").arg(text.name()));
}

bool ReasonLine::belongsToWindow(const QObject* object) const
{
  const auto* widget = qobject_cast<const QWidget*>(object);
  return widget && (widget == m_window || m_window->isAncestorOf(widget));
}

bool ReasonLine::eventFilter(QObject* watched, QEvent* event)
{
  switch (event->type())
  {
    case QEvent::Enter:
    {
      if (!belongsToWindow(watched)) break;
      const auto* widget = static_cast<const QWidget*>(watched);
      const Reason reason = reasonFor(widget);
      if (reason.valid())
      {
        m_source = const_cast<QWidget*>(widget);
        show(reason);
      }
      else if (m_showing && !qobject_cast<QMenu*>(m_source.data()))
        clear();
      break;
    }
    case QEvent::Leave:
      if (m_showing && watched == m_source.data()) clear();
      break;
    case QEvent::MouseMove:
    {
      // Qt does not say "hovered" for a greyed-out menu item, so the pointer is followed here
      auto* menu = qobject_cast<QMenu*>(watched);
      if (!menu || !m_watched.contains(menu)) break;
      const Reason reason = reasonFor(menu->actionAt(static_cast<QMouseEvent*>(event)->position().toPoint()));
      if (reason.valid())
      {
        if (m_source != menu || m_why != reason.why) { m_source = menu; show(reason); }
      }
      else if (m_showing && m_source == menu)
        clear();
      break;
    }
    case QEvent::StatusTip:
      // the same words as a plain message are not wanted next to the line
      if (m_showing && watched == m_window && static_cast<QStatusTipEvent*>(event)->tip() == m_why) return true;
      break;
    default:
      break;
  }
  return QObject::eventFilter(watched, event);
}
