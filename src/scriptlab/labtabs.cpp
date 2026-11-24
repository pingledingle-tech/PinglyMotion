/*
 * PinglyMotion Script Lab - the tabs inside the Script Lab window (see labtabs.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "labtabs.h"

#include <QApplication>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdlib>

#include "paneltitlebar.h"
#include "textsize.h"
#include "theme.h"

namespace
{
  // design_tokens.json: spacing.s, spacing.m, control_height.default, radius.control
  const int SpacingS = 8;
  const int SpacingM = 12;
  const int ControlHeight = 32;
  const int RadiusControl = 6;
  const int IconSmall = 16;
  // the biggest number taken for a place (a screen is never this far away)
  const int MaxCoordinate = 100000;
}

// --- the remembered state ---------------------------------------------------------------------

LabTabState::State LabTabState::parse(const QString& text, const QStringList& known)
{
  State state;
  const QString cut = text.left(MaxLength);
  for (const QString& rawEntry : cut.split(QLatin1Char(';'), Qt::SkipEmptyParts))
  {
    const QString entry = rawEntry.trimmed();
    const int equals = entry.indexOf(QLatin1Char('='));
    if (equals <= 0) continue;
    const QString name = entry.left(equals);
    const QString value = entry.mid(equals + 1);
    if (name == QLatin1String("tab"))
    {
      if (known.contains(value)) state.current = value;
    }
    else if (name == QLatin1String("pop"))
    {
      if (known.contains(value) && !state.popped.contains(value)) state.popped << value;
    }
    else if (name == QLatin1String("place"))
    {
      const int colon = value.indexOf(QLatin1Char(':'));
      if (colon <= 0) continue;
      const QString key = value.left(colon);
      if (!known.contains(key)) continue;
      const QStringList numbers = value.mid(colon + 1).split(QLatin1Char(','));
      if (numbers.size() != 4) continue;
      int n[4];
      bool ok = true;
      for (int i = 0; i < 4 && ok; ++i)
      {
        n[i] = numbers.at(i).trimmed().toInt(&ok);
        ok = ok && std::abs(n[i]) <= MaxCoordinate;
      }
      if (!ok || n[2] <= 0 || n[3] <= 0) continue;
      state.places.insert(key, QRect(n[0], n[1], n[2], n[3]));
    }
  }
  return state;
}

QString LabTabState::format(const State& state)
{
  QStringList entries;
  if (!state.current.isEmpty()) entries << QStringLiteral("tab=") + state.current;
  for (const QString& key : state.popped) entries << QStringLiteral("pop=") + key;
  for (auto it = state.places.constBegin(); it != state.places.constEnd(); ++it)
  {
    const QRect r = it.value();
    entries << QStringLiteral("place=%1:%2,%3,%4,%5").arg(it.key()).arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
  }
  return entries.join(QLatin1Char(';')).left(MaxLength);
}

// --- the tab bar ------------------------------------------------------------------------------

LabTabBar::LabTabBar(QWidget* parent) :
  QTabBar(parent)
{
  setObjectName("scriptLabTabBar");
  // the tabs keep the approved order (Live scripts first); a tab leaves the bar only by popping out
  setMovable(false);
}

int LabTabBar::pullDistance() const
{
  return std::max(height(), 4 * QApplication::startDragDistance());
}

void LabTabBar::mousePressEvent(QMouseEvent* event)
{
  m_pressed = event->button() == Qt::LeftButton ? tabAt(event->position().toPoint()) : -1;
  QTabBar::mousePressEvent(event);
}

void LabTabBar::mouseReleaseEvent(QMouseEvent* event)
{
  const int pressed = m_pressed;
  m_pressed = -1;
  QTabBar::mouseReleaseEvent(event);
  if (pressed < 0 || event->button() != Qt::LeftButton) return;
  const int d = pullDistance();
  if (rect().adjusted(-d, -d, d, d).contains(event->position().toPoint())) return;
  // after the bar has finished with the click: the tab is taken out of it
  const QPoint at = event->globalPosition().toPoint();
  QTimer::singleShot(0, this, [this, pressed, at]() { if (pressed < count()) emit popOutRequested(pressed, at); });
}

LabTabWidget::LabTabWidget(QWidget* parent) :
  QTabWidget(parent),
  m_bar(new LabTabBar(this))
{
  setTabBar(m_bar);
}

// --- the window of a popped tab ---------------------------------------------------------------

LabPopWindow::LabPopWindow(const QString& key, const QString& tabName, QWidget* parent) :
  QWidget(parent, Qt::Window),
  m_key(key)
{
  setObjectName(QStringLiteral("scriptLabPopWindow_") + key);
  setWindowTitle(tr("Script Lab: %1").arg(tabName));
  setAccessibleName(windowTitle());
  m_layout = new QVBoxLayout(this);
  m_layout->setContentsMargins(0, 0, 0, 0);
  m_layout->setSpacing(0);

  // the top row: the tab's name and Put back, so the way home is always in sight (charter P3)
  QWidget* top = new QWidget(this);
  top->setObjectName("scriptLabPopTop");
  QHBoxLayout* row = new QHBoxLayout(top);
  row->setContentsMargins(SpacingM, SpacingS, SpacingM, SpacingS);
  row->setSpacing(SpacingS);
  m_title = new QLabel(tabName, top);
  m_title->setObjectName("scriptLabPopTitle");
  QFont title = m_title->font();   // the "title" type token: 16 px, semibold, next to the 13 px body
  title.setWeight(QFont::DemiBold);
  if (title.pointSizeF() > 0) title.setPointSizeF(title.pointSizeF() * 16.0 / 13.0);
  m_title->setFont(title);
  row->addWidget(m_title, 1);
  m_putBack = new QPushButton(tr("Put back"), top);
  m_putBack->setObjectName("scriptLabPutBack");
  m_putBack->setMinimumHeight(ControlHeight);
  m_putBack->setAccessibleName(tr("Put this tab back into Script Lab"));
  m_putBack->setToolTip(tr("Put this tab back into Script Lab. Closing this window does the same."));
  m_putBack->setStatusTip(m_putBack->toolTip());
  row->addWidget(m_putBack);
  m_layout->addWidget(top);
  connect(m_putBack, &QPushButton::clicked, this, [this]() { emit putBackRequested(m_key); });
  applyStyle(palette().color(QPalette::Window).lightness() < 128);
}

void LabPopWindow::setPage(QWidget* page)
{
  if (m_page) return;
  m_page = page;
  page->setParent(this);
  m_layout->addWidget(page, 1);
  page->show();
  // the keys go from the page to Put back and round again
  setTabOrder(page, m_putBack);
}

QWidget* LabPopWindow::takePage()
{
  QWidget* page = m_page;
  if (!page) return nullptr;
  m_layout->removeWidget(page);
  page->setParent(nullptr);
  m_page = nullptr;
  return page;
}

void LabPopWindow::applyStyle(bool dark)
{
  // Put back is a normal button (docs/DESIGN_SYSTEM.md, "Shape"): surface_raised fill, border, text,
  // with the title bar's "back into the frame" symbol (the same action, the same icon)
  const QString raised = Theme::color(Theme::Token::SurfaceRaised, dark).name(QColor::HexRgb).toUpper();
  const QString text = Theme::color(Theme::Token::Text, dark).name(QColor::HexRgb).toUpper();
  const QString border = Theme::color(Theme::Token::Border, dark).name(QColor::HexRgb).toUpper();
  const QString focus = Theme::color(Theme::Token::FocusRing, dark).name(QColor::HexRgb).toUpper();
  m_putBack->setStyleSheet(QStringLiteral(
    "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 0 %5px; }"
    "QPushButton:focus { border: 2px solid %6; }").arg(raised, text, border).arg(RadiusControl).arg(SpacingM).arg(focus));
  QPalette ink = m_putBack->palette();
  ink.setColor(QPalette::WindowText, Theme::color(Theme::Token::Text, dark));
  ink.setColor(QPalette::Disabled, QPalette::WindowText, Theme::color(Theme::Token::TextDisabled, dark));
  const int icon = TextSize::scaled(IconSmall);
  m_putBack->setIconSize(QSize(icon, icon));
  m_putBack->setIcon(PanelTitleBar::drawSymbol(PanelTitleBar::Symbol::Dock, ink, icon, devicePixelRatioF()));
}

QSize LabPopWindow::firstSize(const QRect& screen)
{
  return QSize(std::max(1, screen.width() * 3 / 5), std::max(1, screen.height() * 3 / 4));
}

QRect LabPopWindow::fitOnScreen(const QRect& place)
{
  QScreen* screen = QGuiApplication::screenAt(place.center());
  if (!screen) screen = QGuiApplication::primaryScreen();
  if (!screen) return place;
  const QRect area = screen->availableGeometry();
  QRect fitted(place.topLeft(), place.size().boundedTo(area.size()));
  if (fitted.right() > area.right()) fitted.moveRight(area.right());
  if (fitted.bottom() > area.bottom()) fitted.moveBottom(area.bottom());
  if (fitted.left() < area.left()) fitted.moveLeft(area.left());
  if (fitted.top() < area.top()) fitted.moveTop(area.top());
  return fitted;
}

void LabPopWindow::closeEvent(QCloseEvent* event)
{
  // closing the window puts the tab back, as the owner asked: nothing is ever lost by closing
  event->accept();
  emit putBackRequested(m_key);
}

void LabPopWindow::moveEvent(QMoveEvent* event)
{
  QWidget::moveEvent(event);
  emit placeChanged(m_key);
}

void LabPopWindow::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  emit placeChanged(m_key);
}

void LabPopWindow::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::PaletteChange) applyStyle(palette().color(QPalette::Window).lightness() < 128);
}
