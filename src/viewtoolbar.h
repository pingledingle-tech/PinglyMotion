/*
 * PinglyMotion - the view toolbar at the top left of the 3D view (UX phase 4b,
 * docs/UX_DECISIONS.md 6b, the mockup's "nav": the view-direction widget, the view's
 * name and the "vtools" toolbar)
 *
 * From the top: the view-direction widget (src/viewgizmo.*), the view's name, then the
 * buttons with their names and keys: what the left mouse button does (Pick V, Look around
 * O, Move the view M, Zoom Z; the active one is marked with an accent bar and frame and
 * is pressed for screen readers), framing (Picked joint F, Whole figure Shift+F, Home
 * view 0), and the menus Look from (Front, Side, Top, Back) and Show (Floor grid,
 * Rotation rings, Joint dots, Tool names in this toolbar).
 *
 * The toolbar only shows and asks: a click sends a signal, and the window does the work
 * through the same code its keys and menus use, then tells the toolbar (setTool(),
 * setPartShown(), setCameraAngles()). When the names are switched off, the buttons show
 * only their icons, and when the view is too low for one column they go into two columns
 * of icons (the mockup's short-view toolbar). The toolbar follows its parent's size itself.
 *
 * The texts are in src/viewtoolspec.h; tests/viewtools checks them, the signals, the
 * keyboard path and the layouts.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef VIEWTOOLBAR_H
#define VIEWTOOLBAR_H

#include <QHash>
#include <QList>
#include <QToolButton>
#include <QWidget>

#include "camera.h"
#include "viewtoolspec.h"

class QAction;
class QFrame;
class QGridLayout;
class QLabel;
class QMenu;
class ViewGizmo;

// a button of the view toolbar, drawn in the design system's shape: icon, name and key
class ViewToolButton : public QToolButton
{
  Q_OBJECT

  public:
    ViewToolButton(const ViewToolSpec::Button& spec, bool menu, QWidget* parent = nullptr);
    // false: only the icon (its name stays the tip and the screen-reader name)
    void setShowLabel(bool show);
    bool showsLabel() const { return m_showLabel; }
    QString label() const { return m_label; }
    QString key() const { return m_key; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QString m_icon;
    QString m_label;
    QString m_key;
    bool m_menu = false;
    bool m_showLabel = true;
};

class ViewToolbar : public QWidget
{
  Q_OBJECT

  public:
    using Part = ViewToolSpec::Part;

    // goes into the top left corner of the parent (the 3D view) and follows its size
    explicit ViewToolbar(QWidget* parent = nullptr);

    void setTool(ViewTool tool);
    ViewTool tool() const { return m_tool; }

    void setPartShown(Part part, bool shown);
    bool partShown(Part part) const;

    // the camera turn: turns the widget and sets the view's name
    void setCameraAngles(float height, float angle);
    QString viewNameText() const;

    // the colours of the current theme (call after the theme changes)
    void applyLook();

    // the widget, the buttons and the menu items, for the window and the tests
    ViewGizmo* gizmo() const { return m_gizmo; }
    QLabel* viewNameLabel() const { return m_viewName; }
    ViewToolButton* button(const char* id) const;
    QList<ViewToolButton*> buttons() const { return m_buttons; }
    ViewToolButton* toolButton(ViewTool tool) const;
    QMenu* lookMenu() const { return m_lookMenu; }
    QMenu* showMenu() const { return m_showMenu; }
    QAction* showAction(Part part) const;

    // true when the buttons are in two columns of icons
    bool twoColumns() const { return m_twoColumns; }
    // lays the buttons out for the height of the parent (called on its resize)
    void relayout();

  signals:
    void toolChosen(ViewTool tool);
    void focusRequested();
    void wholeFigureRequested();
    void homeRequested();
    // a Look from item was chosen: its index in ViewToolSpec::looks
    void lookRequested(int index);
    void partToggled(ViewToolSpec::Part part, bool shown);
    void endChosen(ViewToolSpec::End end);
    void dragged(int dx, int dy);

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    ViewTool m_tool = ViewTool::Pick;
    ViewGizmo* m_gizmo = nullptr;
    QLabel* m_viewName = nullptr;
    QFrame* m_panel = nullptr;
    QGridLayout* m_grid = nullptr;
    QList<ViewToolButton*> m_buttons;
    // the buttons in their order, with a null entry where a divider goes
    QList<QWidget*> m_order;
    QList<QWidget*> m_dividers;
    QMenu* m_lookMenu = nullptr;
    QMenu* m_showMenu = nullptr;
    QHash<int, QAction*> m_showActions;
    bool m_twoColumns = false;
    bool m_relayouting = false;
    ViewToolButton* addButton(const ViewToolSpec::Button& spec, bool menu = false);
    void fillGrid(bool twoColumns, bool labels);
    QString dividerStyle() const;
};

#endif
