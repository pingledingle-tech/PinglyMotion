// Layout rules for the panels (docs/PANELS_PLAN.md, job P1).
//
// Plain functions and small structs: no widgets, no disk access, nothing that
// changes the screen. Header-only, like easing.h. The window code (jobs P2 to
// P6) calls these; the rules are tested without a display.
#ifndef PANELLAYOUT_H
#define PANELLAYOUT_H

#include <QByteArray>
#include <QList>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <cmath>

namespace PanelLayout
{
  // Setting names (docs/PANELS_PLAN.md, section 7).
  inline const char* const SettingLayout="panel_layout";
  inline const char* const SettingLayoutVersion="panel_layout_version";
  inline const char* const SettingWindowGeometry="main_window_geometry";
  // Retired: read once for the move, never written again.
  inline const char* const LegacyWindowState="window_state";
  inline const char* const LegacyShowTimeline="show_timeline_panel";
  inline const char* const LegacyWidth="main_window_width";
  inline const char* const LegacyHeight="main_window_height";

  // Raised when panels are added or renamed.
  const int CurrentVersion=1;

  const int MinWindowWidth=640;
  const int MinWindowHeight=480;

  // Object names of the panels of the three-panel layout.
  inline const char* const JointsDock="jointsDock";
  inline const char* const JointCardDock="jointCardDock";
  inline const char* const TimelineDock="timelineDock";

  // ---- 1. Is a saved layout usable? ----------------------------------------

  // A layout is usable when it is not empty, the version is the current one
  // and the window is not smaller than the minimum. Otherwise use the default.
  inline bool isLayoutUsable(const QByteArray& state,int savedVersion,QSize windowSize,
                             int currentVersion=CurrentVersion)
  {
    if (state.isEmpty()) return false;
    if (savedVersion!=currentVersion) return false;
    if (windowSize.width()<MinWindowWidth || windowSize.height()<MinWindowHeight) return false;
    return true;
  }

  // ---- 2. Pull a floating panel back onto a screen --------------------------

  namespace Detail
  {
    inline qint64 overlapArea(const QRect& a,const QRect& b)
    {
      const QRect i=a.intersected(b);
      if (i.isEmpty()) return 0;
      return qint64(i.width())*i.height();
    }
  }

  // The panel rectangle for a floating panel whose saved place may no longer be
  // on any screen. Rules:
  //  - no screens known: unchanged;
  //  - the panel keeps its place when at least `minVisible` pixels of the title
  //    end (top strip, `titleHeight` high) lie on one screen;
  //  - otherwise it moves to the screen it overlaps most (the first screen if
  //    it overlaps none), is made no bigger than that screen, and is moved
  //    inside it by the smallest step.
  inline QRect pullOntoScreen(const QRect& panel,const QList<QRect>& screens,int titleHeight=24,
                              int minVisible=48)
  {
    if (screens.isEmpty() || !panel.isValid()) return panel;

    const QRect titleStrip(panel.left(),panel.top(),panel.width(),std::min(titleHeight,panel.height()));
    for (const QRect& s : screens)
    {
      const QRect i=titleStrip.intersected(s);
      if (!i.isEmpty() && i.width()>=std::min(minVisible,panel.width()) && i.height()>=std::min(minVisible/2,titleStrip.height()))
        return panel;
    }

    const QRect* best=&screens.first();
    qint64 bestArea=-1;
    for (const QRect& s : screens)
    {
      const qint64 a=Detail::overlapArea(panel,s);
      if (a>bestArea) { bestArea=a; best=&s; }
    }

    QRect r=panel;
    r.setSize(QSize(std::min(panel.width(),best->width()),std::min(panel.height(),best->height())));
    int x=r.left();
    int y=r.top();
    if (x<best->left()) x=best->left();
    if (y<best->top()) y=best->top();
    if (x+r.width()>best->left()+best->width()) x=best->left()+best->width()-r.width();
    if (y+r.height()>best->top()+best->height()) y=best->top()+best->height()-r.height();
    r.moveTopLeft(QPoint(x,y));
    return r;
  }

  // ---- 3. The Tab state ----------------------------------------------------

  // Remembers which panels were open when Tab hid them, so the second Tab
  // brings back exactly those. The hidden state is never saved to disk.
  class TabState
  {
  public:
    bool isHidden() const { return m_hidden; }

    // Tab pressed. `openPanels`: names of the panels that are open right now.
    // Returns the panels to hide (when hiding) or to show (when showing).
    QStringList toggle(const QStringList& openPanels)
    {
      if (!m_hidden)
      {
        if (openPanels.isEmpty()) return QStringList();  // nothing to hide: stay shown
        m_remembered=openPanels;
        m_hidden=true;
        return m_remembered;
      }
      m_hidden=false;
      const QStringList show=m_remembered;
      m_remembered.clear();
      return show;
    }

    // A panel was closed by the user (or opened) while the others are hidden.
    // A panel closed while hidden stays closed on the next Tab.
    void panelClosed(const QString& name)
    {
      if (m_hidden) m_remembered.removeAll(name);
    }

    // The user opens a panel (View menu) while hidden: Tab is over for it.
    // Returns true when this ended the hidden state.
    bool panelOpened(const QString& name)
    {
      if (!m_hidden) return false;
      m_hidden=false;
      m_remembered.clear();
      Q_UNUSED(name);
      return true;
    }

    void reset() { m_hidden=false; m_remembered.clear(); }

  private:
    bool m_hidden=false;
    QStringList m_remembered;
  };

  // ---- 4. The middle of the free area (used by P5) -------------------------

  // The point of the view where the figure should be: the middle of the biggest
  // rectangle that the panels leave free. Panels are clipped to the view.
  // Approach: panels cover the view from the sides; the free area is what is
  // left after cutting the covered strips from each side, choosing for each
  // panel the side it lies on (the nearest edge). A panel that covers the whole
  // view leaves nothing: the middle of the view is returned.
  inline QRectF freeArea(const QRect& view,const QList<QRect>& panels)
  {
    if (!view.isValid()) return QRectF(view);
    qreal left=view.left();
    qreal top=view.top();
    qreal right=view.left()+view.width();
    qreal bottom=view.top()+view.height();
    const qreal vl=left,vt=top,vr=right,vb=bottom;

    for (const QRect& p0 : panels)
    {
      const QRect p=p0.intersected(view);
      if (p.isEmpty()) continue;
      const qreal pl=p.left(),pt=p.top(),pr=p.left()+p.width(),pb=p.top()+p.height();
      // distance from each view edge to the panel's inner side; the smallest
      // cut that clears the panel from the free area decides.
      const qreal cutLeft=pr-vl;     // panel treated as a left panel
      const qreal cutRight=vr-pl;    // right panel
      const qreal cutTop=pb-vt;      // top panel
      const qreal cutBottom=vb-pt;   // bottom panel
      // A panel is a side panel when it touches (or is nearest to) that side.
      // Use the side where the panel is the thinnest cut.
      const qreal m=std::min(std::min(cutLeft,cutRight),std::min(cutTop,cutBottom));
      if (m==cutLeft) left=std::max(left,pr);
      else if (m==cutRight) right=std::min(right,pl);
      else if (m==cutTop) top=std::max(top,pb);
      else bottom=std::min(bottom,pt);
    }
    if (right<=left || bottom<=top) return QRectF(vl,vt,vr-vl,vb-vt);
    return QRectF(left,top,right-left,bottom-top);
  }

  inline QPointF freeAreaCenter(const QRect& view,const QList<QRect>& panels)
  {
    return freeArea(view,panels).center();
  }

  // ---- 5. Which layout to start with (migration and defaults) ---------------

  struct StartupChoice
  {
    bool useSavedLayout=false;       // restoreState(panel_layout)
    bool timelineHiddenOnce=false;   // old show_timeline_panel=false, first run only
  };

  // Called at start-up with what the settings hold. `legacyShowTimeline` is the
  // old setting (true when missing). The old `window_state` is never used.
  // The Tab "hidden" state is not part of this: the program always starts with
  // the panels shown.
  inline StartupChoice chooseStartup(const QByteArray& panelLayout,int panelLayoutVersion,
                                     QSize windowSize,bool legacyShowTimeline)
  {
    StartupChoice c;
    if (isLayoutUsable(panelLayout,panelLayoutVersion,windowSize))
    {
      c.useSavedLayout=true;
      return c;
    }
    // Only an empty layout (first run after the move) reads the old timeline
    // setting; a refused layout (wrong version) gives the plain default.
    if (panelLayout.isEmpty() && !legacyShowTimeline) c.timelineHiddenOnce=true;
    return c;
  }

  // The window size to start with (P3): the new geometry wins; the old width
  // and height are used only when it is empty. An invalid size gives the default.
  struct StartupWindow
  {
    bool useGeometry=false;
    QSize size;
  };

  inline StartupWindow chooseWindow(const QByteArray& geometry,int legacyWidth,int legacyHeight,
                                    QSize defaultSize)
  {
    StartupWindow w;
    if (!geometry.isEmpty()) { w.useGeometry=true; w.size=defaultSize; return w; }
    if (legacyWidth>=MinWindowWidth && legacyHeight>=MinWindowHeight)
      w.size=QSize(legacyWidth,legacyHeight);
    else
      w.size=defaultSize;
    return w;
  }

  // ---- 6. Panel visibility as part of the layout ---------------------------

  // The open/closed state of each panel. From P2 on the timeline's visibility
  // lives here (and in panel_layout) instead of in show_timeline_panel; the
  // window code reads timelineVisible() where it reads that setting today.
  struct PanelVisibility
  {
    bool joints=true;
    bool jointCard=true;
    bool timeline=true;

    bool timelineVisible() const { return timeline; }

    QStringList openPanels() const
    {
      QStringList l;
      if (joints) l << JointsDock;
      if (jointCard) l << JointCardDock;
      if (timeline) l << TimelineDock;
      return l;
    }

    void set(const QString& name,bool open)
    {
      if (name==JointsDock) joints=open;
      else if (name==JointCardDock) jointCard=open;
      else if (name==TimelineDock) timeline=open;
    }

    // Applies the one-time carry-over of the old timeline setting.
    static PanelVisibility fromStartup(const StartupChoice& c)
    {
      PanelVisibility v;
      if (c.timelineHiddenOnce) v.timeline=false;
      return v;
    }
  };
}

#endif
