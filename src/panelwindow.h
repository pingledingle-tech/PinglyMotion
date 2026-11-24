/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The panels on real dock widgets (docs/archive/PANELS_PLAN.md, jobs P3 and P4).
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
#include <QLayout>
#include <QList>
#include <QMainWindow>
#include <QRect>
#include <QScreen>
#include <QStringList>
#include <QTabWidget>

#include <algorithm>

#include "beginnerview.h"
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

  // The default layout of a mode (UX-E): Expert mode is the default layout above. Beginner
  // mode is the same with the Joints panel closed, so the 3D view gets most of the window;
  // the Joints panel stays docked on the left and opens in its place from View > Joints or
  // Show all joints.
  inline void applyModeLayout(QMainWindow* window,QDockWidget* joints,QDockWidget* jointCard,
                              QDockWidget* timeline,bool expert)
  {
    applyDefaultLayout(window,joints,jointCard,timeline);
    if (!BeginnerView::jointsListInDefaultLayout(expert)) joints->hide();
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

  // ---- Rearranging at large text or in a small window (docs/UX_DECISIONS.md, 6c) ----

  // The panels the rearranging moves. scriptLab and library may be null.
  struct Docks
  {
    QDockWidget* joints=nullptr;
    QDockWidget* jointCard=nullptr;
    QDockWidget* timeline=nullptr;
    QDockWidget* scriptLab=nullptr;
    QDockWidget* library=nullptr;

    QList<QDockWidget*> all() const
    {
      QList<QDockWidget*> list;
      for (QDockWidget* dock : {joints,jointCard,scriptLab,library,timeline})
        if (dock) list << dock;
      return list;
    }
  };

  // The sides each panel may go to in the wide and the medium layout.
  inline Qt::DockWidgetAreas usualAreas(const Docks& docks,const QDockWidget* dock)
  {
    if (dock==docks.timeline) return Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea;
    // job UX-B: Script Lab docks on any side (it can be the main focus)
    if (dock==docks.scriptLab) return Qt::AllDockWidgetAreas;
    return Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;
  }

  // Puts the panels into the layout of `reflow`, starting from the wide
  // layout the window shows now:
  //  - Wide: nothing changes;
  //  - Medium: a docked Joints panel becomes a tab next to Selected joint on
  //    the right (or next to Script Lab, or alone on the right when neither is
  //    docked there), and Selected joint stays in front;
  //  - Narrow: every panel is docked (nothing floats), one under the other
  //    below the 3D view: Joints, Selected joint, Script Lab, Library, Timeline; and no
  //    panel may float until there is room again.
  // A closed panel stays closed. Floating is allowed again by restoreUsual().
  inline void applyReflow(QMainWindow* window,const Docks& docks,PanelLayout::Reflow reflow)
  {
    using PanelLayout::Reflow;
    if (reflow==Reflow::Medium)
    {
      if (docks.joints->isFloating()) return;
      QDockWidget* partner=nullptr;
      for (QDockWidget* dock : {docks.jointCard,docks.scriptLab,docks.library})
        if (dock && !dock->isFloating() && !dock->isHidden() && window->dockWidgetArea(dock)==Qt::RightDockWidgetArea)
        { partner=dock; break; }
      if (!partner)
        for (QDockWidget* dock : {docks.jointCard,docks.scriptLab,docks.library})
          if (dock && !dock->isFloating() && window->dockWidgetArea(dock)==Qt::RightDockWidgetArea)
          { partner=dock; break; }
      // the tabs above the panels, as in the clickable mockup
      window->setTabPosition(Qt::RightDockWidgetArea,QTabWidget::North);
      if (!partner) { window->addDockWidget(Qt::RightDockWidgetArea,docks.joints); return; }
      window->tabifyDockWidget(partner,docks.joints);
      // Selected joint in front, as in the first layout of the right side
      if (!docks.jointCard->isFloating() && !docks.jointCard->isHidden()) docks.jointCard->raise();
      else partner->raise();
      return;
    }
    if (reflow==Reflow::Narrow)
    {
      for (QDockWidget* dock : docks.all())
      {
        dock->setFloating(false);
        dock->setAllowedAreas(Qt::BottomDockWidgetArea);
        dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetFloatable);
      }
      // addDockWidget takes the first out of its old place (and out of a tab
      // group); each next one is split off under the one before it
      const QList<QDockWidget*> column=docks.all();
      window->addDockWidget(Qt::BottomDockWidgetArea,column.first());
      for (int i=1;i<column.size();++i)
        window->splitDockWidget(column.at(i-1),column.at(i),Qt::Vertical);
      // the 3D view keeps the upper half; the open panels share the lower half
      QList<QDockWidget*> open;
      for (QDockWidget* dock : column)
        if (!dock->isHidden()) open << dock;
      if (!open.isEmpty())
        window->resizeDocks(open,QList<int>(open.size(),std::max(1,window->height()/2/int(open.size()))),Qt::Vertical);
    }
  }

  // Undoes what applyReflow(Narrow) changed on the panels themselves: their
  // usual sides and pop-out. Their places come back with the saved layout.
  inline void restoreUsual(QMainWindow* window,const Docks& docks)
  {
    window->setTabPosition(Qt::RightDockWidgetArea,QTabWidget::South);
    for (QDockWidget* dock : docks.all())
    {
      dock->setAllowedAreas(usualAreas(docks,dock));
      dock->setFeatures(dock->features() | QDockWidget::DockWidgetFloatable);
    }
  }

  // Remembers the wide layout while another one is shown, so the panels go
  // back to where the user had them when the window has room again. Changes
  // made while the panels are rearranged last until then.
  class Reflower
  {
  public:
    PanelLayout::Reflow mode() const { return m_mode; }

    // Goes to the layout `next`. Answers true when something changed.
    bool setMode(QMainWindow* window,const Docks& docks,PanelLayout::Reflow next)
    {
      using PanelLayout::Reflow;
      if (next==m_mode) return false;
      // the wide layout may be put back for a moment on the way to the next
      // one; it must not make the window grow to its width meanwhile
      QLayout* layout=window->layout();
      const QLayout::SizeConstraint constraint=layout->sizeConstraint();
      layout->setSizeConstraint(QLayout::SetNoConstraint);
      if (m_mode==Reflow::Wide)
        m_wide=window->saveState(PanelLayout::CurrentVersion);
      else
      {
        restoreUsual(window,docks);
        window->restoreState(m_wide,PanelLayout::CurrentVersion);
      }
      m_mode=next;
      applyReflow(window,docks,next);
      layout->setSizeConstraint(constraint);
      return true;
    }

    // The window has just been given a new wide layout (Reset layout, or the
    // saved layout at start): remember it and rearrange it again for the
    // layout shown now.
    void wideLayoutChanged(QMainWindow* window,const Docks& docks)
    {
      if (m_mode==PanelLayout::Reflow::Wide) return;
      QLayout* layout=window->layout();
      const QLayout::SizeConstraint constraint=layout->sizeConstraint();
      layout->setSizeConstraint(QLayout::SetNoConstraint);
      restoreUsual(window,docks);
      m_wide=window->saveState(PanelLayout::CurrentVersion);
      applyReflow(window,docks,m_mode);
      layout->setSizeConstraint(constraint);
    }

    // The layout to keep for the next start: always the wide one, so the
    // next start begins from the user's own layout.
    QByteArray layoutToSave(const QMainWindow* window) const
    {
      if (m_mode==PanelLayout::Reflow::Wide) return window->saveState(PanelLayout::CurrentVersion);
      return m_wide;
    }

  private:
    PanelLayout::Reflow m_mode=PanelLayout::Reflow::Wide;
    QByteArray m_wide;
  };
}

#endif
