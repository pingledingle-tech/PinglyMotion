/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The panels on real dock widgets (docs/PANELS_PLAN.md, jobs P3 and P4).
 *
 * panellayout.h holds the rules without widgets. This header applies them to
 * a QMainWindow and its QDockWidgets: the default layout built in code, the
 * pull of a floating panel back onto a screen, and the Tab hide / show. It
 * is header-only so the test in tests/paneldocks/ can use it with a plain
 * QMainWindow, the same way the program does.
 */
#ifndef PANELWINDOW_H
#define PANELWINDOW_H

#include <QDockWidget>
#include <QGuiApplication>
#include <QList>
#include <QMainWindow>
#include <QRect>
#include <QScreen>
#include <QStringList>

#include "panellayout.h"

namespace PanelWindow
{
  // The height of the timeline in the default layout.
  const int DefaultTimelineHeight=250;

  // The default layout, built in code (not a state taken at start-up): Joints
  // on the left, Selected joint on the right, Timeline at the bottom between
  // them, all three docked and shown, none tabbed. A panel that was floating
  // or a tab on another panel is put back.
  inline void applyDefaultLayout(QMainWindow* window,QDockWidget* joints,QDockWidget* jointCard,
                                 QDockWidget* timeline)
  {
    for (QDockWidget* dock : {joints,jointCard,timeline}) dock->setFloating(false);
    // addDockWidget takes a panel out of its old place (and out of a tab group) first
    window->addDockWidget(Qt::LeftDockWidgetArea,joints);
    window->addDockWidget(Qt::RightDockWidgetArea,jointCard);
    window->addDockWidget(Qt::BottomDockWidgetArea,timeline);
    // the tall panels use the full height, the timeline sits between them under the 3D view
    window->setCorner(Qt::TopLeftCorner,Qt::LeftDockWidgetArea);
    window->setCorner(Qt::BottomLeftCorner,Qt::LeftDockWidgetArea);
    window->setCorner(Qt::TopRightCorner,Qt::RightDockWidgetArea);
    window->setCorner(Qt::BottomRightCorner,Qt::RightDockWidgetArea);
    for (QDockWidget* dock : {joints,jointCard,timeline}) dock->show();
    window->resizeDocks({timeline},{DefaultTimelineHeight},Qt::Vertical);
  }

  // The usable part of every screen, for pullFloatingPanelsOntoScreen.
  inline QList<QRect> screenRects()
  {
    QList<QRect> rects;
    const QList<QScreen*> screens=QGuiApplication::screens();
    for (const QScreen* screen : screens) rects << screen->availableGeometry();
    return rects;
  }

  // A floating panel whose place is no longer on any screen (second screen
  // unplugged, other resolution) is moved back with PanelLayout::pullOntoScreen.
  // Returns how many panels were moved.
  inline int pullFloatingPanelsOntoScreen(const QList<QDockWidget*>& docks,const QList<QRect>& screens)
  {
    int moved=0;
    for (QDockWidget* dock : docks)
    {
      if (!dock->isFloating()) continue;
      const QRect was=dock->geometry();
      const QRect now=PanelLayout::pullOntoScreen(was,screens);
      if (now==was) continue;
      dock->setGeometry(now);
      ++moved;
    }
    return moved;
  }

  // The names of the panels that are open: not closed by the user, docked or
  // floating. Asked before the window is shown it still answers, because a
  // closed panel is hidden on purpose and the others are not.
  inline QStringList openPanels(const QList<QDockWidget*>& docks)
  {
    QStringList names;
    for (const QDockWidget* dock : docks)
      if (!dock->isHidden()) names << dock->objectName();
    return names;
  }

  // Tab: hides every open panel, or shows again the ones Tab hid. The rule is
  // PanelLayout::TabState; this applies its answer to the docks. Returns true
  // when the panels are hidden afterwards.
  inline bool toggleHidden(PanelLayout::TabState& state,const QList<QDockWidget*>& docks)
  {
    const QStringList names=state.toggle(openPanels(docks));
    const bool hide=state.isHidden();
    for (QDockWidget* dock : docks)
      if (names.contains(dock->objectName())) dock->setVisible(!hide);
    return hide;
  }
}

#endif
