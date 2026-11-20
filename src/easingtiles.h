// The easing tile row (issue #53 step 5): six small tiles on the timeline, one
// per easing choice, for the pose change that ends at the selected key frame.
// Each tile shows a tiny moving figure that runs from left to right with the
// speed of its choice and leaves a trail of dots at equal time steps, so the
// spacing of the dots shows where the movement is fast and where it is slow.
// No curves, numbers or handles on screen (docs/EASING.md).
//
// The row knows nothing about the window: the window tells it which choice is
// current (setChoice / setNoKey) and listens to choiceClicked. The helpers in
// EasingChoice read and store a choice on the animation; the window and the
// tests use the same functions.
#ifndef EASINGTILES_H
#define EASINGTILES_H

#include <QAbstractButton>
#include <QWidget>

#include <optional>

#include "easing.h"

class Animation;
class BVHNode;
class QLabel;
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

  // Name on the tile and the one-line hint (docs/TEACHING.md EASE-01 to 06),
  // translated.
  QString tileName(Easing::Preset preset);
  QString hint(Easing::Preset preset);
}

// One tile: a checkable button with the moving figure and the name below it.
class EasingTile : public QAbstractButton
{
  Q_OBJECT

  public:
    explicit EasingTile(Easing::Preset preset,QWidget* parent=nullptr);

    Easing::Preset preset() const { return m_preset; }

    // Where the figure is on its way, 0 (start) to 1 (end), set by the row.
    void setPhase(double phase);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

  signals:
    void hovered(Easing::Preset preset);
    void left();

  protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    Easing::Preset m_preset;
    double m_phase=0;
};

// The row: six tiles, one marked as current, and a one-line hint below.
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

    // Whether the hint line under the row shows the help texts (the "Tips on"
    // setting). The "select a key frame" line shows either way.
    void setTipsOn(bool on);

    bool hasChoice() const { return m_hasChoice; }
    Easing::Preset choice() const { return m_choice; }
    EasingTile* tile(Easing::Preset preset) const;
    QString hintText() const;
    bool isAnimating() const;

    // Length of one run of the figure, with the pause at the end, in ms.
    static int cycleMs() { return 2400; }

  signals:
    // The user clicked a tile (not emitted when the window sets the choice).
    void choiceClicked(Easing::Preset preset);

  protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void changeEvent(QEvent* event) override;

  private slots:
    void tick();
    void tileClicked();
    void tileHovered(Easing::Preset preset);
    void tileLeft();

  private:
    void refreshHint();
    void updateAnimation();

    EasingTile* m_tiles[Easing::PresetCount];
    QLabel* m_hint;
    QTimer* m_timer;
    bool m_hasChoice=false;
    Easing::Preset m_choice=Easing::defaultPreset();
    bool m_tipsOn=true;
    std::optional<Easing::Preset> m_hovered;
    int m_elapsedMs=0;
};

#endif
