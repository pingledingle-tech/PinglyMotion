/*
 * PinglyMotion - makes every dialog fit the screen (see dialogfit.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "dialogfit.h"

#include <algorithm>

#include <QApplication>
#include <QDialog>
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
              DialogFit::fit(m_dialog);
              connectScreen();
              later();   // the frame is known only after the window is shown
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

    private:
      void connectScreen()
      {
        if (m_screenConnected || !m_dialog->windowHandle()) return;
        m_screenConnected = true;
        connect(m_dialog->windowHandle(), &QWindow::screenChanged, this, [this]() { later(); });
      }

      QDialog* m_dialog;
      QScrollArea* m_scroll;
      QSize m_preferred;
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

QRect DialogFit::availableArea(const QDialog* dialog)
{
  if (!g_testArea.isNull()) return g_testArea;
  const QScreen* screen = nullptr;
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
  want = want.expandedTo(fitter->preferred());
  want = want.expandedTo(dialog->minimumSizeHint().boundedTo(maxSize));

  // a minimum that does not fit the screen would make the dialog too big
  if (dialog->minimumWidth() > maxSize.width()) dialog->setMinimumWidth(maxSize.width());
  if (dialog->minimumHeight() > maxSize.height()) dialog->setMinimumHeight(maxSize.height());
  if (dialog->maximumWidth() < maxSize.width() || dialog->maximumHeight() < maxSize.height())
  {
    // a fixed size would block the way back: let it follow the content
    dialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  }

  dialog->resize(want.boundedTo(maxSize));

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
