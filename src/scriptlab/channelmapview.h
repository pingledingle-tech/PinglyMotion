/*
 * PinglyMotion Script Lab - the channel map's drawing (job A4)
 *
 * Draws a LabChannelMap::Map as rows, one per arrow: the box that sent the
 * message on the left, the arrow with its words (kind, channel and text) in
 * the middle, and the box that got it on the right. An arrow that reached
 * nobody is a dashed line in the danger colour that ends in a bar, with the
 * circle-x icon and the error in words where the box would be (design charter
 * P10: never colour alone). A box outside the object (you, your viewer, the
 * web server) has a dashed border; a hub has "(hub)" after its name.
 *
 * A click on a box or an arrow, or Enter or Space on the focused part, sends
 * boxActivated or arrowActivated (the panel picks the prim in the tree or
 * shows the script line). Keyboard: Up and Down move between rows, Left and
 * Right between the sender, the arrow and the receiver, Home and End go to
 * the first and last row (charter P8). Colours and sizes come from the theme
 * tokens (docs/DESIGN_SYSTEM.md).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef CHANNELMAPVIEW_H
#define CHANNELMAPVIEW_H

#include <QWidget>

#include "labchannelmap.h"

class ChannelMapView : public QWidget
{
  Q_OBJECT

  public:
    enum Part { NoPart = -1, FromPart = 0, ArrowPart = 1, ToPart = 2 };

    explicit ChannelMapView(QWidget* parent = nullptr);

    void setMap(const LabChannelMap::Map& map);
    const LabChannelMap::Map& map() const { return m_map; }

    // the row (arrow) and part under a point; row -1 when none
    struct Hit { int row = -1; Part part = NoPart; };
    Hit hitAt(const QPoint& point) const;
    // where a part of a row is drawn (empty when there is no such row)
    QRect partRect(int row, Part part) const;
    int rowHeight() const;

    // the focused row and part (the keyboard's place)
    int currentRow() const { return m_row; }
    Part currentPart() const { return m_part; }
    void setCurrent(int row, Part part);
    // does what a click on that part does
    void activate(int row, Part part);

    // the words of a part, as its tooltip and the screen reader say them
    QString partText(int row, Part part) const;
    // the line shown while there is no arrow
    static QString emptyText();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  signals:
    void boxActivated(int box);
    void arrowActivated(int arrow);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

  private:
    int boxWidth() const;
    void updateAccessible();

    LabChannelMap::Map m_map;
    int m_row = 0;
    Part m_part = ArrowPart;
};

#endif
