/*
 * Pingmator - animation editor for Second Life avatars
 *
 * Wheel guard: a mouse wheel over a slider, spin box or drop-down list inside
 * a scrolling panel must scroll the panel, not change the control (issue #82:
 * scrolling the Properties panel moved the Y position slider and the figure
 * sank below the floor). A control that has the keyboard focus (it was clicked
 * or tabbed to) still takes the wheel, so fine adjustment with the wheel works.
 */
#ifndef WHEELGUARD_H
#define WHEELGUARD_H

#include <QAbstractScrollArea>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <QScrollBar>
#include <QWheelEvent>
#include <QWidget>

#include <memory>

class WheelGuard : public QObject
{
  public:
    explicit WheelGuard(QObject* parent=nullptr) : QObject(parent) { }

    // guard every slider, spin box and combo box below the widget
    void guardChildrenOf(QWidget* root)
    {
      const QList<QWidget*> all=root->findChildren<QWidget*>();
      for(QWidget* w : all)
        if(isGuardable(w)) guard(w);
    }

    void guard(QWidget* w)
    {
      // a wheel must not give the focus to the control either
      w->setFocusPolicy(Qt::StrongFocus);
      w->installEventFilter(this);
    }

    // Scroll bars are not guarded: they belong to a scroll area of their own
    // (a table, a list) and that area forwards its wheel to them. Guarding them
    // sends the wheel to the viewport, the viewport back to the scroll bar, and
    // so on until the stack is full (crash seen in the Windows build of #85).
    static bool isGuardable(QWidget* w)
    {
      if(qobject_cast<QScrollBar*>(w)) return false;
      return qobject_cast<QAbstractSlider*>(w) || qobject_cast<QAbstractSpinBox*>(w) || qobject_cast<QComboBox*>(w);
    }

  protected:
    bool eventFilter(QObject* watched,QEvent* event) override
    {
      if(event->type()!=QEvent::Wheel) return false;
      // a wheel that comes back to us while we pass it on is left alone
      if(m_forwarding) return false;
      QWidget* w=qobject_cast<QWidget*>(watched);
      if(!w || w->hasFocus()) return false;
      // pass the wheel on to the scroll area around the control
      QWidget* target=nullptr;
      for(QWidget* p=w->parentWidget();p && !target;p=p->parentWidget())
        if(QAbstractScrollArea* area=qobject_cast<QAbstractScrollArea*>(p))
          target=area->viewport();
      if(!target) target=w->parentWidget();
      if(target)
      {
        std::unique_ptr<QWheelEvent> copy(static_cast<QWheelEvent*>(event)->clone());
        m_forwarding=true;
        QCoreApplication::sendEvent(target,copy.get());
        m_forwarding=false;
      }
      return true;
    }

  private:
    bool m_forwarding=false;
};

#endif
