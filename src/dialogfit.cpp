/*
 * PinglyMotion - makes every dialog fit the screen (see dialogfit.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "dialogfit.h"

#include <algorithm>

#include <QApplication>
#include <QDialog>
#include <QDockWidget>
#include <QMainWindow>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPointer>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#include "usersettings.h"

namespace
{
  QRect g_testArea;

  constexpr const char* kScrollName = "dialogFitScroll";
  constexpr const char* kFitterName = "dialogFitFitter";

  // Watches one dialog; lives as long as it does.
  class Fitter : public QObject
  {
    public:
      Fitter(QDialog* dialog, QScrollArea* scroll, const QSize& preferred) :
        QObject(dialog), m_dialog(dialog), m_scroll(scroll), m_preferred(preferred)
      {
        setObjectName(QString::fromLatin1(kFitterName));
        dialog->installEventFilter(this);
      }

      QSize preferred() const { return m_preferred; }
      QScrollArea* scroll() const { return m_scroll; }

      // the name of the kept place (DialogFit::remember), and whether the
      // dialog has its size from the user now rather than from its content
      void setPlaceName(const QString& name) { m_placeName = name; }
      QString placeName() const { return m_placeName; }
      bool keepsSize() const { return m_keepsSize; }

      void later()
      {
        if (m_pending) return;
        m_pending = true;
        QTimer::singleShot(0, this, [this]() { m_pending = false; DialogFit::fit(m_dialog); });
      }

    protected:
      bool eventFilter(QObject* watched, QEvent* event) override
      {
        if (watched == m_dialog)
        {
          switch (event->type())
          {
            case QEvent::Show:
              restorePlace();
              DialogFit::fit(m_dialog);
              connectScreen();
              later();   // the frame is known only after the window is shown
              break;
            case QEvent::Hide:
              storePlace();
              break;
            case QEvent::Resize:
              // a size the user gives a kept window stays, also when the text size changes
              if (!m_placeName.isEmpty() && m_dialog->isVisible() && !m_fitting) m_keepsSize = true;
              break;
            case QEvent::FontChange:
            case QEvent::ApplicationFontChange:
            case QEvent::StyleChange:
              // children get the change after the dialog does: fit when they have
              later();
              break;
            default:
              break;
          }
        }
        return false;
      }

    public:
      // set while DialogFit::fit() resizes, so its own resize is not taken as the user's
      bool m_fitting = false;

    private:
      void restorePlace()
      {
        if (m_placeName.isEmpty()) return;
        const QRect place = UserSettings::windowPlace(m_placeName);
        if (!place.isValid()) return;
        m_fitting = true;
        m_dialog->resize(place.size());
        m_dialog->move(place.topLeft());
        m_fitting = false;
        m_keepsSize = true;
      }

      void storePlace()
      {
        if (m_placeName.isEmpty()) return;
        UserSettings::setWindowPlace(m_placeName, QRect(m_dialog->pos(), m_dialog->size()));
      }

      void connectScreen()
      {
        if (m_screenConnected || !m_dialog->windowHandle()) return;
        m_screenConnected = true;
        connect(m_dialog->windowHandle(), &QWindow::screenChanged, this, [this]() { later(); });
      }

      QDialog* m_dialog;
      QScrollArea* m_scroll;
      QSize m_preferred;
      QString m_placeName;
      bool m_keepsSize = false;
      bool m_pending = false;
      bool m_screenConnected = false;
  };

  Fitter* fitterOf(const QDialog* dialog)
  {
    // not findChild<Fitter*>: Fitter has no Q_OBJECT, so that would match any child
    return static_cast<Fitter*>(dialog->findChild<QObject*>(QString::fromLatin1(kFitterName), Qt::FindDirectChildrenOnly));
  }

  // what the frame (title bar and borders) takes beyond the dialog's own size
  QSize frameExtra(const QDialog* dialog)
  {
    if (dialog->isVisible())
    {
      const QSize extra = dialog->frameGeometry().size() - dialog->geometry().size();
      if (extra.width() >= 0 && extra.height() >= 0 && !extra.isNull()) return extra;
    }
    return QSize(16, 48);   // a generous guess: 8 pixels each side, a title bar of 40
  }
}

QVBoxLayout* DialogFit::contentLayout(QDialog* dialog)
{
  QVBoxLayout* root = new QVBoxLayout(dialog);
  QScrollArea* scroll = new QScrollArea(dialog);
  scroll->setObjectName(QString::fromLatin1(kScrollName));
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  QWidget* body = new QWidget();
  body->setObjectName(QStringLiteral("dialogFitBody"));
  scroll->setWidget(body);
  root->addWidget(scroll, 1);
  return new QVBoxLayout(body);
}

void DialogFit::finish(QDialog* dialog, QWidget* buttons, const QSize& preferred)
{
  QVBoxLayout* root = qobject_cast<QVBoxLayout*>(dialog->layout());
  if (root) root->addWidget(buttons);
  install(dialog, dialog->findChild<QScrollArea*>(QString::fromLatin1(kScrollName)), preferred);
}

void DialogFit::finish(QDialog* dialog, QLayout* buttons, const QSize& preferred)
{
  QVBoxLayout* root = qobject_cast<QVBoxLayout*>(dialog->layout());
  if (root) root->addLayout(buttons);
  install(dialog, dialog->findChild<QScrollArea*>(QString::fromLatin1(kScrollName)), preferred);
}

void DialogFit::install(QDialog* dialog, QScrollArea* scroll, const QSize& preferred)
{
  if (!dialog || !scroll || fitterOf(dialog)) return;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  new Fitter(dialog, scroll, preferred);
  fit(dialog);
}

void DialogFit::setAreaForTests(const QRect& area)
{
  g_testArea = area;
}

void DialogFit::remember(QDialog* dialog, const QString& name)
{
  if (!dialog) return;
  if (Fitter* fitter = fitterOf(dialog)) fitter->setPlaceName(name);
}

QRect DialogFit::availableArea(const QDialog* dialog)
{
  if (!g_testArea.isNull()) return g_testArea;
  const QScreen* screen = nullptr;
  // a kept window is checked against the screen it is on now, which may be
  // a second screen before its window knows it
  const Fitter* fitter = fitterOf(dialog);
  if (fitter && !fitter->placeName().isEmpty() && dialog->isVisible())
    screen = QGuiApplication::screenAt(dialog->frameGeometry().center());
  if (screen) return screen->availableGeometry();
  if (dialog->windowHandle()) screen = dialog->windowHandle()->screen();
  if (!screen && dialog->parentWidget() && dialog->parentWidget()->window()->windowHandle())
    screen = dialog->parentWidget()->window()->windowHandle()->screen();
  if (!screen) screen = QGuiApplication::primaryScreen();
  return screen ? screen->availableGeometry() : QRect(0, 0, 1024, 700);
}

void DialogFit::fit(QDialog* dialog)
{
  Fitter* fitter = fitterOf(dialog);
  if (!fitter || !dialog->layout()) return;

  const QRect area = availableArea(dialog);
  const QSize extra = frameExtra(dialog);
  const QSize maxSize(std::max(200, area.width() - extra.width()), std::max(150, area.height() - extra.height()));

  // what the dialog wants: the layout's hint, with the scroll area given the
  // whole content (a QScrollArea's own hint stops at 36 text heights)
  // every layout inside forgets what it measured (children measured at the old text size)
  for (QLayout* inner : dialog->findChildren<QLayout*>()) inner->invalidate();
  dialog->layout()->invalidate();
  QScrollArea* scroll = fitter->scroll();
  QSize want = dialog->sizeHint();
  if (scroll && scroll->widget())
  {
    const int frame = 2 * scroll->frameWidth();
    const QSize content = scroll->widget()->sizeHint() + QSize(frame, frame);
    want += content - scroll->sizeHint();
    // room for a vertical scroll bar so no sideways scrolling is needed
    want.rwidth() += scroll->verticalScrollBar()->sizeHint().width();
  }
  want.setWidth(std::max(want.width(), fitter->preferred().width()));
  if (scroll && scroll->widget() && scroll->widget()->hasHeightForWidth())
  {
    // wrapping text: measured at the width the content really gets, not at the width of its own
    // size hint. Otherwise the height depends on what was measured before, and a dialog that had
    // been at 200 percent stayed taller after Ctrl+0 (UX small fixes).
    const QMargins m = dialog->layout()->contentsMargins();
    const int frame = 2 * scroll->frameWidth();
    const int bar = scroll->verticalScrollBar()->sizeHint().width();
    const int width = std::min(want.width(), maxSize.width()) - m.left() - m.right() - frame - bar;
    const int contentHeight = scroll->widget()->sizeHint().height();
    const int forWidth = scroll->widget()->heightForWidth(width);
    if (width > 0 && forWidth > 0) want.rheight() += forWidth - contentHeight;
  }
  want = want.expandedTo(fitter->preferred());
  // a kept window keeps the size the user gave it (no smaller than its content needs)
  if (fitter->keepsSize()) want = dialog->size();
  want = want.expandedTo(dialog->minimumSizeHint().boundedTo(maxSize));

  // a minimum that does not fit the screen would make the dialog too big
  if (dialog->minimumWidth() > maxSize.width()) dialog->setMinimumWidth(maxSize.width());
  if (dialog->minimumHeight() > maxSize.height()) dialog->setMinimumHeight(maxSize.height());
  if (dialog->maximumWidth() < maxSize.width() || dialog->maximumHeight() < maxSize.height())
  {
    // a fixed size would block the way back: let it follow the content
    dialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  }

  fitter->m_fitting = true;
  dialog->resize(want.boundedTo(maxSize));
  fitter->m_fitting = false;

  if (!dialog->isVisible()) return;
  const QRect frame = dialog->frameGeometry();
  int x = frame.x();
  int y = frame.y();
  x = std::min(x, area.right() + 1 - frame.width());
  x = std::max(x, area.left());
  y = std::min(y, area.bottom() + 1 - frame.height());
  y = std::max(y, area.top());
  if (x != frame.x() || y != frame.y()) dialog->move(dialog->pos() + QPoint(x - frame.x(), y - frame.y()));
}

QSize DialogFit::leastPanelSize(QDockWidget* dock)
{
  if (!dock) return QSize();
  // every layout inside forgets what it measured at the old text size
  for (QLayout* inner : dock->findChildren<QLayout*>()) inner->invalidate();
  if (dock->layout()) dock->layout()->invalidate();
  QSize least = dock->minimumSizeHint();
  if (QWidget* content = dock->widget()) least = least.expandedTo(content->minimumSizeHint().expandedTo(content->minimumSize()));
  return least.expandedTo(dock->minimumSize());
}

QList<DialogFit::PanelSize> DialogFit::measurePanels(QMainWindow* window, const QList<QDockWidget*>& docks)
{
  QList<PanelSize> out;
  if (!window) return out;
  for (QDockWidget* dock : docks)
  {
    if (!dock || dock->isHidden()) continue;
    PanelSize p;
    p.dock = dock;
    p.floating = dock->isFloating();
    p.area = p.floating ? 0 : int(window->dockWidgetArea(dock));
    p.size = dock->size();
    p.least = leastPanelSize(dock);
    out << p;
  }
  return out;
}

void DialogFit::followTextSize(QMainWindow* window, const QList<PanelSize>& before)
{
  if (!window) return;
  QList<QDockWidget*> wide, tall;
  QList<int> widths, heights;
  for (const PanelSize& p : before)
  {
    QDockWidget* dock = p.dock;
    // gone, hidden, or moved since it was measured: left as it is
    if (!dock || dock->isHidden() || dock->isFloating() != p.floating) continue;
    if (!p.floating && int(window->dockWidgetArea(dock)) != p.area) continue;
    const QSize least = leastPanelSize(dock);
    // the room the user gave it beyond the smallest size stays the same
    const QSize want(least.width() + std::max(0, p.size.width() - p.least.width()),
                     least.height() + std::max(0, p.size.height() - p.least.height()));
    if (p.floating)
    {
      if (dock->size() != want) dock->resize(want);
      continue;
    }
    if (p.area == Qt::LeftDockWidgetArea || p.area == Qt::RightDockWidgetArea)
    {
      wide << dock;
      widths << want.width();
    }
    else if (p.area == Qt::TopDockWidgetArea || p.area == Qt::BottomDockWidgetArea)
    {
      tall << dock;
      heights << want.height();
    }
  }
  if (!wide.isEmpty()) window->resizeDocks(wide, widths, Qt::Horizontal);
  if (!tall.isEmpty()) window->resizeDocks(tall, heights, Qt::Vertical);
}
