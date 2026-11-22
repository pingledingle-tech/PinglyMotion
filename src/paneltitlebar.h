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

  protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

  private:
    enum class Symbol { Float, Dock, Close };
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
