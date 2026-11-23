/*
 * PinglyMotion - the view-direction widget at the top left of the 3D view (UX phase 4b,
 * docs/UX_DECISIONS.md 6b, the mockup's "gizmo", like Blender's navigation gizmo)
 *
 * A round widget with the X (red), Y (green) and Z (blue) axes of the world as the camera
 * sees them: the + ends carry their letter on the axis colour, the - ends a minus and the
 * letter on a pale disc. Clicking an end looks from there (the window does it, with
 * Camera::lookFrom()); dragging the round background turns the view. With the keyboard the
 * widget is one Tab stop: the arrow keys go through the six ends in the mockup's order
 * (+X, -X, +Y, -Y, +Z, -Z), Enter or Space chooses the end that has the focus ring.
 *
 * The texts are in src/viewtoolspec.h; tests/viewtools checks where the ends are for a
 * camera turn, what a click, a drag and the keys do, and the tips.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef VIEWGIZMO_H
#define VIEWGIZMO_H

#include <QPointF>
#include <QVector3D>
#include <QWidget>

#include "viewtoolspec.h"

class ViewGizmo : public QWidget
{
  Q_OBJECT

  public:
    using End = ViewToolSpec::End;

    explicit ViewGizmo(QWidget* parent = nullptr);

    // Where an axis end points for a camera turned like Camera::setView(height, angle):
    // x to the right, y up, z towards the viewer, each -1 to 1
    static QVector3D direction(End end, float height, float angle);

    // the camera turn the widget shows (call after every camera move; repaints only on a change)
    void setCameraAngles(float height, float angle);
    float cameraHeight() const { return m_height; }
    float cameraAngle() const { return m_angle; }

    // the size at 100 percent text size (the mockup's 84 pixels); it grows with the text size
    static constexpr int BaseSize = 84;
    QSize sizeHint() const override;

    // where an end is drawn, in widget pixels, and its radius
    QPointF endPosition(End end) const;
    double endRadius(End end) const;
    // the end under a point, the one nearest the viewer when ends overlap; false for none
    bool endAt(const QPointF& point, End* end) const;

    // the end that the keyboard is on (shown with the focus ring while the widget has the focus)
    End currentEnd() const { return m_current; }
    void setCurrentEnd(End end);

  signals:
    // an end was clicked, or chosen with Enter or Space
    void endChosen(ViewToolSpec::End end);
    // the background was dragged by this many pixels (right and down are positive)
    void dragged(int dx, int dy);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

  private:
    float m_height = 10.0f;
    float m_angle = 0.0f;
    End m_current = End::PlusZ;
    bool m_hoverValid = false;
    End m_hover = End::PlusZ;
    // a press: on an end (chosen on release if the mouse stays on it) or on the background (a drag)
    bool m_pressed = false;
    bool m_pressOnEnd = false;
    End m_pressEnd = End::PlusZ;
    QPoint m_lastPos;
    void updateAccessibleText();
};

#endif
