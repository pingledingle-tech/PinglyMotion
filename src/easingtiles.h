/*
 * PinglyMotion
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

// The easing tile row (issue #53 step 5): six small tiles on the timeline, one
// per easing choice, for the pose change that ends at the selected key frame.
// Each tile shows its name as text under a small picture: a tiny figure and a
// trail of dots at equal time steps, so the spacing of the dots shows where the
// movement is fast and where it is slow. The figure runs from left to right
// with the speed of its choice only while the mouse is over that tile (UX step
// 3, docs/UX_DECISIONS.md); otherwise the row is still. No curves, numbers or
// handles on screen (docs/EASING.md).
//
// The row knows nothing about the window: the window tells it which choice is
// current (setChoice / setNoKey) and listens to choiceClicked. The helpers in
// EasingChoice read and store a choice on the animation; the window and the
// tests use the same functions.
#ifndef EASINGTILES_H
#define EASINGTILES_H

#include <QAbstractButton>
#include <QDialog>
#include <QWidget>

#include <functional>
#include <optional>

#include "easing.h"

class Animation;
class BVHNode;
class QBoxLayout;
class QButtonGroup;
class QLabel;
class QPushButton;
class QTimer;

// Reading and storing the choice of one pose change (segment) on the animation.
namespace EasingChoice
{
  // The segment that ends at key frame `endFrame` of `joint`: its start key is
  // the previous key frame of the same joint. False when there is none (frame 0,
  // not a key frame, no joint).
  bool segmentEndingAt(const BVHNode* joint,int endFrame,int& startFrame);

  // The choice the segment from startFrame to endFrame plays with: the start
  // key's stored curve when it has one, otherwise the two ease switches
  // (Easing::segmentPreset).
  Easing::Preset choiceOf(const BVHNode* joint,int startFrame,int endFrame);

  // Store a choice on the segment. The first four choices become the two ease
  // switches (start key's easeOut, end key's easeIn) and clear the stored curve,
  // so the file stays as before. The last two store the curve on the start key
  // and set both switches, so a reader without `curve` plays the default.
  void setChoice(Animation* anim,BVHNode* joint,int startFrame,int endFrame,Easing::Preset preset);

  // The custom curve of the segment (issue #11), if it has one.
  std::optional<Easing::Bezier> customCurveOf(const BVHNode* joint,int startFrame);

  // Store a custom curve on the segment: on the start key, with the named
  // choice cleared and both switches on (the default for a reader that does
  // not know custom curves). setChoice() removes it again.
  void setCustomCurve(Animation* anim,BVHNode* joint,int startFrame,int endFrame,const Easing::Bezier& curve);

  // The handles the curve editor opens with: the segment's custom curve, or a
  // close match of its named choice (Easing::bezierFromPreset).
  Easing::Bezier editorStart(const BVHNode* joint,int startFrame,int endFrame);

  // Name on the tile and the one-line hint (docs/TEACHING.md EASE-01 to 06),
  // translated.
  QString tileName(Easing::Preset preset);
  QString hint(Easing::Preset preset);
}

// One tile: a checkable button with the moving figure and the name below it.
// The current tile has a check mark in its corner (the mockup's tick). With
// the keyboard focus it shows the focus ring; the arrow keys go to the next or
// the previous tile (UX phase 5, the mockup's radio group).
class EasingTile : public QAbstractButton
{
  Q_OBJECT

  public:
    explicit EasingTile(Easing::Preset preset,QWidget* parent=nullptr);

    Easing::Preset preset() const { return m_preset; }

    // Where the figure is on its way, 0 (start) to 1 (end), set by the row.
    void setPhase(double phase);
    double phase() const { return m_phase; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

  signals:
    void hovered(Easing::Preset preset);
    void left();
    // an arrow key: +1 for the next tile (Right, Down), -1 for the previous one (Left, Up)
    void stepRequested(int step);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    QFont nameFont() const;
    int nameHeight() const;

    Easing::Preset m_preset;
    double m_phase=0;
};

// The row: the heading "How the move feels", six tiles, one marked as
// current, the "Edit curve..." button (Expert mode only) and a one-line hint
// below. Only the tile under the mouse moves its figure; the others stand at
// the start. Keyboard (UX phase 5): Tab reaches the row at the current tile,
// the arrow keys pick the next or previous choice (round the end), Space
// picks the tile with the focus.
class EasingTiles : public QWidget
{
  Q_OBJECT

  public:
    explicit EasingTiles(QWidget* parent=nullptr);

    // A key frame with a pose change before it is selected: mark its choice.
    void setChoice(Easing::Preset preset);
    // No key frame (or none with a pose change before it) is selected: the row
    // is greyed out and the hint says what to do.
    void setNoKey();

    // Why there is no pose change to pick for (asked at every setNoKey()): no
    // animation open, no joint picked, or no key with a move before it. The
    // timeline sets it (it knows the animation and the joint).
    enum class NoKey { NoAnimation, NoJoint, NoMove };
    void setNoKeyCheck(std::function<NoKey()> check) { m_noKeyCheck=std::move(check); }
    // asks the check again while no key is selected (the animation or the
    // joint changed)
    void refreshNoKey();
    // the reason the greyed-out tiles and Edit curve give (their tip and the
    // status line, charter P6); empty while a pose change is selected
    QString tileReason() const;
    QString editCurveReason() const;

    // Expert mode shows the "Edit curve..." button (the mockup's expert-only
    // button); it starts with UserSettings::expertMode() and follows it.
    void setExpertMode(bool expert);
    bool isExpertMode() const { return m_expert; }

    // Whether the hint line under the row shows the help texts (the "Tips on"
    // setting). The "select a key frame" line shows either way.
    void setTipsOn(bool on);

    // The pose change has a custom curve (issue #11): no tile is marked and
    // the hint line says so (with the tips on or off). Call after setChoice().
    void setCustom(bool custom);
    bool isCustom() const { return m_custom; }
    // Asked at the end of every setChoice(): does the selected pose change have
    // a custom curve? The timeline sets it (it knows the joint and the frame),
    // so the window needs no change.
    void setCustomCheck(std::function<bool()> check) { m_customCheck=std::move(check); }
    // "Edit curve..." (issue #11, UX phase 5): opens the Edit curve window
    QPushButton* editCurveButton() const { return m_editButton; }
    QLabel* heading() const { return m_heading; }

    bool hasChoice() const { return m_hasChoice; }
    Easing::Preset choice() const { return m_choice; }
    EasingTile* tile(Easing::Preset preset) const;
    QString hintText() const;
    // The preview runs only on the tile under the mouse, and only while the row
    // is on screen and a key is selected.
    bool isAnimating() const;
    // The tile whose figure moves now, if any.
    std::optional<Easing::Preset> animatingTile() const;

    // Length of one run of the figure, with the pause at the end, in ms.
    static int cycleMs() { return 2400; }

    // The tiles on one line (false, the first layout) or in two lines when
    // they do not fit (docs/UX_DECISIONS.md, 6c), and the smallest width of
    // the row either way, whichever it shows now.
    void setWrapped(bool wrapped);
    bool isWrapped() const;
    int oneLineWidth() const;
    int wrappedWidth() const;

  signals:
    // The user clicked a tile (not emitted when the window sets the choice).
    void choiceClicked(Easing::Preset preset);
    // The user clicked "Edit curve..." (issue #11).
    void editCurveRequested();

  protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void changeEvent(QEvent* event) override;

  private slots:
    void tick();
    void tileClicked();
    void tileHovered(Easing::Preset preset);
    void tileLeft();
    void tileStep(int step);

  private:
    void refreshHint();
    void refreshFocusOrder();
    void applyNoKeyState();
    void updateAnimation();
    int widthWith(int rowWidth) const;

    EasingTile* m_tiles[Easing::PresetCount];
    QButtonGroup* m_group;
    QLabel* m_heading;
    QPushButton* m_editButton;
    QBoxLayout* m_rows=nullptr;
    QLabel* m_hint;
    QTimer* m_timer;
    bool m_hasChoice=false;
    bool m_custom=false;
    std::function<bool()> m_customCheck;
    std::function<NoKey()> m_noKeyCheck;
    NoKey m_noKey=NoKey::NoMove;
    bool m_expert=false;
    Easing::Preset m_choice=Easing::defaultPreset();
    bool m_tipsOn=true;
    std::optional<Easing::Preset> m_hovered;
    int m_elapsedMs=0;
};

// The curve editor (issue #11): the speed of one pose change as a curve from
// the bottom left (start, old pose) to the top right (end, new pose), with two
// round handles to drag. Time runs left to right, the way to the new pose
// bottom to top. Under it, the same dot trail as on the tiles.
class CurveEditor : public QWidget
{
  Q_OBJECT

  public:
    explicit CurveEditor(QWidget* parent=nullptr);

    Easing::Bezier curve() const { return m_curve; }
    void setCurve(const Easing::Bezier& curve);

    // Where a handle (0 or 1) is drawn, and the curve point for a widget
    // position; used by the mouse and by the tests.
    QPointF handlePosition(int handle) const;
    QPointF toCurve(const QPointF& position) const;

    // Keyboard (charter: everything works without a mouse): the arrow keys
    // move the active handle by keyStep() (Shift: bigStep()), Space makes the
    // other handle active. A drag makes the dragged handle active.
    int activeHandle() const { return m_active; }
    static double keyStep() { return 0.01; }
    static double bigStep() { return 0.1; }

    QSize sizeHint() const override { return QSize(360,300); }
    QSize minimumSizeHint() const override { return QSize(240,200); }

  signals:
    void curveChanged(const Easing::Bezier& curve);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

  private:
    QRectF graphRect() const;
    QPointF toWidget(double x,double y) const;
    void moveHandle(int handle,const QPointF& position);

    Easing::Bezier m_curve;
    int m_dragging=-1;
    int m_active=0;
};

// The Edit curve window around the curve editor (UX_DECISIONS.md section 3,
// UX phase 5): hint, "Start again", OK and Cancel. A separate window that can
// be moved, resized and put on a second screen; it opens small, and after that
// where it was last time with the size it had (kept between sessions under
// placeName(), clamped to the screen by src/dialogfit.*).
class CurveEditorDialog : public QDialog
{
  Q_OBJECT

  public:
    CurveEditorDialog(const Easing::Bezier& start,QWidget* parent=nullptr);

    static QString placeName() { return QStringLiteral("edit_curve_window"); }

    Easing::Bezier curve() const;
    CurveEditor* editor() const { return m_editor; }

  private:
    CurveEditor* m_editor;
    Easing::Bezier m_start;
};

#endif
