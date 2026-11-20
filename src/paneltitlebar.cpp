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

static const int ICON_SIZE = 16;

PanelTitleBar::PanelTitleBar(QDockWidget* dock)
  : QWidget(dock),
    m_dock(dock)
{
  m_title = new QLabel(dock->windowTitle(), this);
  QFont bold = m_title->font();
  bold.setBold(true);
  m_title->setFont(bold);

  m_floatButton = new QToolButton(this);
  m_closeButton = new QToolButton(this);
  for (QToolButton* button : {m_floatButton, m_closeButton})
  {
    button->setAutoRaise(true);
    button->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    button->setFixedSize(ICON_SIZE + 10, ICON_SIZE + 8);
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
  connect(m_dock, &QDockWidget::windowTitleChanged, m_title, &QLabel::setText);

  setCursor(Qt::SizeAllCursor);   // shows that the bar can be dragged
  updateButtons();
}

void PanelTitleBar::updateButtons()
{
  const bool floating = m_dock->isFloating();
  m_floatButton->setIcon(symbolIcon(floating ? Symbol::Dock : Symbol::Float));
  m_floatButton->setToolTip(floating ? tr("Put this panel back into the main window")
                                     : tr("Make this panel a window of its own"));
  m_closeButton->setIcon(symbolIcon(Symbol::Close));
}

// the symbols are drawn in the text colour, so they show in the light and the dark theme
QIcon PanelTitleBar::symbolIcon(Symbol symbol) const
{
  const qreal ratio = devicePixelRatioF();
  QPixmap pixmap(QSize(ICON_SIZE, ICON_SIZE) * ratio);
  pixmap.setDevicePixelRatio(ratio);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(palette().color(QPalette::WindowText));
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
      painter.fillRect(QRectF(2.5, 3.5, 4, 9), palette().color(QPalette::WindowText));
      break;
  }
  return QIcon(pixmap);
}

void PanelTitleBar::changeEvent(QEvent* event)
{
  // the theme changed: redraw the symbols in the new text colour
  if (event->type() == QEvent::PaletteChange) updateButtons();
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
// event first): the same as clicking the float button (docs/PANELS_PLAN.md,
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
  if (event->button() == Qt::LeftButton)
  {
    QPointer<QDockWidget> dock=m_dock;
    QTimer::singleShot(0,this,[dock]() { if (dock) dock->setFloating(!dock->isFloating()); });
  }
  QWidget::mouseDoubleClickEvent(event);
}
