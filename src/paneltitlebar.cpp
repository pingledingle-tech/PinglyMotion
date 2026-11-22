/*
 * PinglyMotion - title bar for the timeline and properties panels
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDockWidget>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QTimer>
#include <QToolButton>

#include "paneltitlebar.h"
#include "textsize.h"
#include "theme.h"

// the icon size at 100 percent text size; it grows with the text size (src/textsize.h)
static const int ICON_SIZE = Theme::Size::iconSmall;

PanelTitleBar::PanelTitleBar(QDockWidget* dock)
  : QWidget(dock),
    m_dock(dock)
{
  m_title = new QLabel(dock->windowTitle(), this);
  // only the weight is set, so the size still follows the program's text size
  QFont bold;
  bold.setBold(true);
  m_title->setFont(bold);

  m_floatButton = new QToolButton(this);
  m_closeButton = new QToolButton(this);
  for (QToolButton* button : {m_floatButton, m_closeButton})
  {
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::ArrowCursor);
  }
  m_closeButton->setToolTip(tr("Close this panel (bring it back from the Options menu)"));

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 3, 4, 3);
  layout->setSpacing(2);
  layout->addWidget(m_title, 1);
  layout->addWidget(m_floatButton);
  layout->addWidget(m_closeButton);

  connect(m_floatButton, &QToolButton::clicked, this, [this]() { m_dock->setFloating(!m_dock->isFloating()); });
  connect(m_closeButton, &QToolButton::clicked, m_dock, &QDockWidget::close);
  connect(m_dock, &QDockWidget::topLevelChanged, this, [this]() { updateButtons(); });
  // pop-out is taken away while the panels stand in one column (src/panelwindow.h)
  connect(m_dock, &QDockWidget::featuresChanged, this, [this]() { updateButtons(); });
  connect(m_dock, &QDockWidget::windowTitleChanged, m_title, &QLabel::setText);

  setCursor(Qt::SizeAllCursor);   // shows that the bar can be dragged
  updateButtons();
}

QString PanelTitleBar::noRoomToFloatText()
{
  return tr("The window is too small to float panels. Make it bigger or the text smaller.");
}

bool PanelTitleBar::canToggleFloating() const
{
  return m_dock->isFloating() || (m_dock->features() & QDockWidget::DockWidgetFloatable);
}

void PanelTitleBar::updateButtons()
{
  // the buttons grow with the text size
  const int icon = TextSize::scaled(ICON_SIZE);
  for (QToolButton* button : {m_floatButton, m_closeButton})
  {
    button->setIconSize(QSize(icon, icon));
    button->setFixedSize(icon + TextSize::scaled(10), icon + TextSize::scaled(8));
  }

  const bool floating = m_dock->isFloating();
  m_floatButton->setIcon(symbolIcon(floating ? Symbol::Dock : Symbol::Float));
  // a button that cannot be clicked says why and what to do (charter P6)
  const bool canFloat = canToggleFloating();
  m_floatButton->setEnabled(canFloat);
  const QString tip = !canFloat ? noRoomToFloatText()
                                : floating ? tr("Put this panel back into the main window")
                                           : tr("Make this panel a window of its own");
  m_floatButton->setToolTip(tip);
  m_floatButton->setStatusTip(tip);
  m_floatButton->setAccessibleDescription(tip);
  m_closeButton->setIcon(symbolIcon(Symbol::Close));
}

// the symbols are drawn in the text colour, so they show in the light and the dark theme
QIcon PanelTitleBar::symbolIcon(Symbol symbol) const
{
  // drawn on the 16 px grid, scaled with the text size so the lines stay sharp;
  // a greyed-out button shows the symbol in the disabled text colour
  const int size = TextSize::scaled(ICON_SIZE);
  const qreal scale = size / qreal(ICON_SIZE);
  const qreal ratio = devicePixelRatioF();
  auto draw = [&](QPalette::ColorGroup group)
  {
    QPixmap pixmap(QSize(size, size) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(scale, scale);
    const QColor ink = palette().color(group, QPalette::WindowText);
    QPen pen(ink);
    pen.setWidthF(1.6);
    painter.setPen(pen);

    switch (symbol)
    {
      case Symbol::Close:
        painter.drawLine(QPointF(4, 4), QPointF(12, 12));
        painter.drawLine(QPointF(12, 4), QPointF(4, 12));
        break;
      case Symbol::Float:
        // two windows, one in front of the other
        painter.drawRect(QRectF(5.5, 2.5, 8, 8));
        painter.fillRect(QRectF(2.5, 5.5, 8, 8), palette().color(QPalette::Window));
        painter.drawRect(QRectF(2.5, 5.5, 8, 8));
        break;
      case Symbol::Dock:
        // a window with a filled strip: back into the frame
        painter.drawRect(QRectF(2.5, 3.5, 11, 9));
        painter.fillRect(QRectF(2.5, 3.5, 4, 9), ink);
        break;
    }
    return pixmap;
  };
  QIcon icon;
  icon.addPixmap(draw(QPalette::Active), QIcon::Normal);
  icon.addPixmap(draw(QPalette::Disabled), QIcon::Disabled);
  return icon;
}

void PanelTitleBar::changeEvent(QEvent* event)
{
  // the theme changed: redraw the symbols in the new text colour;
  // the text size changed: make the buttons fit it
  if (event->type() == QEvent::PaletteChange || event->type() == QEvent::FontChange) updateButtons();
  QWidget::changeEvent(event);
}

void PanelTitleBar::paintEvent(QPaintEvent*)
{
  // a band a little lighter (dark theme) or darker (light theme) than the panel
  QPainter painter(this);
  const QColor window = palette().color(QPalette::Window);
  painter.fillRect(rect(), window.lightness() < 128 ? window.lighter(160) : window.darker(112));
}

// double click anywhere on the bar itself (not the buttons, which take the
// event first): the same as clicking the float button (docs/archive/PANELS_PLAN.md,
// job P6). This is how Qt's own dock title bars work, so it matches what a
// user who has used other docking windows expects.
void PanelTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
  // QDockWidget itself watches mouse presses and releases on its title bar
  // (even a custom one) to drag it loose; a double click without any actual
  // drag makes it decide, on the release right after this event, that the
  // panel was just dropped back where it was and redock it. Doing the float
  // toggle on the spot would then be undone a moment later. Deferring it to
  // the next pass through the event loop lets that settle first, so the
  // toggle sticks: float when it was docked, dock again on the second
  // double click, whichever side Qt's own redock left it on.
  // no pop-out while the panels stand in one column (the button says why)
  if (event->button() == Qt::LeftButton && canToggleFloating())
  {
    QPointer<QDockWidget> dock=m_dock;
    QTimer::singleShot(0,this,[dock]() { if (dock) dock->setFloating(!dock->isFloating()); });
  }
  QWidget::mouseDoubleClickEvent(event);
}
