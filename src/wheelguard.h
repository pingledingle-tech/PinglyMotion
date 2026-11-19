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
        if(qobject_cast<QAbstractSlider*>(w) || qobject_cast<QAbstractSpinBox*>(w) || qobject_cast<QComboBox*>(w))
          guard(w);
    }

    void guard(QWidget* w)
    {
      // a wheel must not give the focus to the control either
      w->setFocusPolicy(Qt::StrongFocus);
      w->installEventFilter(this);
    }

  protected:
    bool eventFilter(QObject* watched,QEvent* event) override
    {
      if(event->type()!=QEvent::Wheel) return false;
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
        QCoreApplication::sendEvent(target,copy.get());
      }
      return true;
    }
};

#endif
