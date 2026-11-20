/*
 * PinglyMotion - the selected-joint card (#88, posing UX step 4)
 *
 * The content of the Selected joint panel when the Avatar side is chosen
 * (docs/POSING_UX.md, "Main window", and the mockup there):
 *
 *   - friendly name ("Shoulder · Left") and the Second Life name (mShoulderLeft)
 *   - IK switch, lock and reset
 *   - Rotate / Move: Move is only shown for the pelvis and the avatar position
 *   - a trackball and X / Y / Z sliders with value fields; each slider shows
 *     its allowed range, and a warning appears when a value is near a joint limit
 *   - "Turn around: Joint / Avatar / Screen", the axes the trackball turns around
 *
 * The card only shows and reports. The window does the work: it connects the
 * signals to the animation, and it gives the card the old slider and value
 * field widgets from the designer file, so their connections stay as they are.
 * Without those widgets the card makes its own (the tests use that).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef JOINTCARD_H
#define JOINTCARD_H

#include <QQuaternion>
#include <QWidget>

#include <array>

#include "jointturn.h"

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QLabel;
class QLineEdit;
class QSlider;
class QStackedWidget;
class QToolButton;
class JointInfo;

// the ball: shows the joint's axes as seen on the screen; dragging it reports
// how far the mouse moved
class JointTrackball : public QWidget
{
  Q_OBJECT

  public:
    explicit JointTrackball(QWidget* parent=nullptr);

    // how the joint is turned as seen on the screen
    void setOrientation(const QQuaternion& orientation);
    QQuaternion orientation() const { return m_orientation; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  signals:
    // the mouse moved by dx, dy pixels with the button down; fine is true while Shift is held
    void dragged(double dx,double dy,bool fine);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    QQuaternion m_orientation;
    QPointF m_lastPos;
    bool m_dragging=false;
};

// one slider row of the card: the slider and its value field (outside the
// class so it can be a default argument of the constructor)
struct JointCardAxisRow
{
  QSlider* slider=nullptr;
  QLineEdit* edit=nullptr;
};

class JointCard : public QWidget
{
  Q_OBJECT

  public:
    // what is selected
    enum Kind
    {
      NoJoint=0,      // nothing: everything is disabled
      Joint,          // a joint: Rotate only
      Root,           // the pelvis (top joint): Rotate and Move
      PositionTrack   // the avatar position: Move only
    };

    enum Page
    {
      RotatePage=0,
      MovePage
    };

    // one slider row: the slider and its value field
    using AxisRow=JointCardAxisRow;

    struct Header
    {
      QString title;    // "Shoulder · Left"
      QString slName;   // "mShoulderLeft"; for the avatar position a short note
    };

    // precision: slider steps per degree (or per unit of position).
    // Rows without widgets get widgets made by the card.
    explicit JointCard(int precision,const std::array<AxisRow,3>& rotation={},
                       const std::array<AxisRow,3>& position={},QWidget* parent=nullptr);

    // the header for a joint, by the name in the animation file (lShldr) or
    // "position" for the avatar position track; info may be null (no data)
    static Header headerFor(const JointInfo* info,const QString& fileName);

    // the warning for one axis, empty when the value is not near the limit.
    // A range smaller than 0.01 degrees gives no warning (the axis cannot turn).
    // A value of 0 at a limit of 0 (the rest pose, a straight elbow) gives none either.
    static QString limitWarning(const QString& axis,double value,double minimum,double maximum);
    // how close to a limit counts as "close": a tenth of the range, at least
    // 2 and at most 10 degrees
    static double nearLimitMargin(double minimum,double maximum);

    void setJoint(Kind kind,const Header& header);
    Kind kind() const { return m_kind; }

    // available: the joint is part of an arm or leg that has IK; on: IK is on
    void setIk(bool available,bool on);
    bool ikOn() const;

    void setLocked(bool locked);
    bool isLocked() const;

    // false while the animation plays or the frame is protected
    void setEditable(bool editable);
    // the position (Move) controls can be used: false while the animation plays
    void setMoveEditable(bool editable);

    // true when the rotation slider ranges are the joint's Second Life limits
    // (and so warnings make sense); false for the pelvis and with Options >
    // Joint Limits off
    void setLimited(bool limited);
    bool isLimited() const { return m_limited; }

    void setTrackballOrientation(const QQuaternion& orientation);

    // shows the sliders' ranges and the limit warning again; for slider
    // changes made while the sliders' signals were blocked
    void refresh();

    JointTurn::Frame frame() const;
    void setFrame(JointTurn::Frame frame);

    Page page() const;
    void setPage(Page page);

    QSlider* rotationSlider(int axis) const { return m_rotation[axis].slider; }
    QLineEdit* rotationEdit(int axis) const { return m_rotation[axis].edit; }
    QSlider* positionSlider(int axis) const { return m_position[axis].slider; }
    QLineEdit* positionEdit(int axis) const { return m_position[axis].edit; }
    JointTrackball* trackball() const { return m_trackball; }

    // the text of the limit warning now shown, empty when none
    QString warningText() const;
    // the range text under a rotation slider ("-40° … 40°")
    QString rotationRangeText(int axis) const;

  signals:
    void ikToggled(bool on);
    void lockToggled(bool locked);
    void resetClicked();
    // the trackball was dragged: turn by yaw degrees around the frame's up
    // axis and pitch degrees around its sideways axis
    void trackballTurned(double yawDegrees,double pitchDegrees,JointTurn::Frame frame);
    // the trackball shows the joint in another frame
    void frameChanged(JointTurn::Frame frame);

  protected:
    void changeEvent(QEvent* event) override;

  private slots:
    void updateRanges();
    void updateWarning();
    void trackballDragged(double dx,double dy,bool fine);

  private:
    QWidget* makeSliderGrid(std::array<AxisRow,3>& rows,std::array<QLabel*,6>& rangeLabels,
                            const QString& objectPrefix);
    void updateEnabled();
    // the SL name and the range texts in a paler text colour of the palette
    void updateDimmedText();
    static QString degrees(double value);

    const int m_precision;
    Kind m_kind=NoJoint;
    bool m_editable=true;
    bool m_moveEditable=true;
    bool m_ikAvailable=false;
    bool m_limited=false;

    QLabel* m_title=nullptr;
    QLabel* m_slName=nullptr;
    QCheckBox* m_ik=nullptr;
    QToolButton* m_lock=nullptr;
    QToolButton* m_reset=nullptr;
    QToolButton* m_rotateTab=nullptr;
    QToolButton* m_moveTab=nullptr;
    QButtonGroup* m_tabs=nullptr;
    QStackedWidget* m_pages=nullptr;
    JointTrackball* m_trackball=nullptr;
    std::array<AxisRow,3> m_rotation;
    std::array<AxisRow,3> m_position;
    std::array<QLabel*,6> m_rotationRange{};
    std::array<QLabel*,6> m_positionRange{};
    QLabel* m_warning=nullptr;
    QButtonGroup* m_frames=nullptr;
    QWidget* m_frameRow=nullptr;
    QFont m_slFont;
    bool m_dimming=false;
};

#endif
