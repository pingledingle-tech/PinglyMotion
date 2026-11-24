/*
 * PinglyMotion - title bar for the timeline and properties panels
 *
 * Shows the panel name with two buttons that are easy to see and hit:
 * one to float the panel as its own window (or put it back), one to close it.
 * Drag the bar to move the panel; double click it to float or dock.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef PANELTITLEBAR_H
#define PANELTITLEBAR_H

#include <QIcon>
#include <QPalette>
#include <QWidget>

class QDockWidget;
class QLabel;
class QToolButton;

class PanelTitleBar : public QWidget
{
  Q_OBJECT

  public:
    explicit PanelTitleBar(QDockWidget* dock);

    // the reason the pop-out button gives when it is greyed out: the panels
    // stand in one column and there is no room to float one (docs/UX_DECISIONS.md, 6c)
    static QString noRoomToFloatText();

    // the symbols of the title bar, drawn in the text colour of the palette: two windows (pop out,
    // make a window of its own), a window with a strip (put back) and a cross. Script Lab's tabs use
    // the same symbols for the same actions (docs/DESIGN_SYSTEM.md, "Icons": one action, one icon).
    enum class Symbol { Float, Dock, Close };
    static QIcon drawSymbol(Symbol symbol, const QPalette& palette, int size, qreal ratio);

  protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

  private:
    QIcon symbolIcon(Symbol symbol) const;
    // float or dock is allowed: the panel floats, or it may float
    bool canToggleFloating() const;
    void updateButtons();

    QDockWidget* m_dock;
    QLabel* m_title;
    QToolButton* m_floatButton;
    QToolButton* m_closeButton;
};

#endif
