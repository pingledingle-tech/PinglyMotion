/*
 * PinglyMotion - the Hands tab of the Joints panel, UX phase 2c
 *
 * Built from the approved mockup (docs/mockup/index.html, Joints panel, Hands
 * tab; docs/UX_DECISIONS.md 6i). Instead of the joint list the Hands tab shows:
 *
 *  - which hand is shown ("Left hand" / "Right hand") and whether the other
 *    hand copies it (Mirror) or only this hand changes (Left or Right);
 *  - the hand's state: "Pose: Okay" when it matches a preset, "Pose: changed
 *    from Okay", "Pose: your own" or "Pose: open, nothing changed";
 *  - the nine ready-made finger poses as tiles with a small picture of five
 *    bars (one Tab stop; the arrow keys move, Enter or Space picks);
 *  - Bend each finger: one slider per finger from Open (0 percent) to Closed
 *    (100 percent, the Tight Grip preset);
 *  - Reset hand ("Reset both hands" in Mirror);
 *  - in Expert mode the joint grid "Each joint, in degrees": a row per finger
 *    joint and the columns X, Y and Z (one Tab stop; arrows move, a number
 *    typed edits, Enter or F2 starts editing, Page Up / Page Down change by 5,
 *    with Shift by 1, dragging a cell sideways turns it, a double click edits).
 *
 * The panel does not change the animation itself. The window tells it what is
 * true (setState: an animation open, why nothing can change now, the side
 * switch, the picked joint, both hands' angles, Expert mode, hints) and the
 * panel asks for changes (changeRequested) with the joint changes worked out
 * by HandPose (src/handpose.h) and the status line text; the window makes
 * them through Animation::setRotation(), so each change is a key at once, as
 * with the Selected joint panel's sliders, and calls setState again.
 *
 * One table (HandsSpec) for the widget and for tests/handstab, which checks
 * every text exactly.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef HANDSTAB_H
#define HANDSTAB_H

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QVector>
#include <QWidget>

#include "handpose.h"

class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;

namespace HandsSpec
{
  inline QString tr2(const char* text) { return QCoreApplication::translate("HandsPanel", text); }

  inline constexpr const char* leftHand = QT_TRANSLATE_NOOP("HandsPanel", "Left hand");
  inline constexpr const char* rightHand = QT_TRANSLATE_NOOP("HandsPanel", "Right hand");
  inline constexpr const char* copies = QT_TRANSLATE_NOOP("HandsPanel", "The other hand copies it.");
  inline constexpr const char* onlyThis = QT_TRANSLATE_NOOP("HandsPanel", "Only this hand changes.");

  inline constexpr const char* poseIs = QT_TRANSLATE_NOOP("HandsPanel", "Pose: %1");
  inline constexpr const char* poseOpen = QT_TRANSLATE_NOOP("HandsPanel", "Pose: open, nothing changed");
  inline constexpr const char* poseChangedFrom = QT_TRANSLATE_NOOP("HandsPanel", "Pose: changed from %1");
  inline constexpr const char* poseOwn = QT_TRANSLATE_NOOP("HandsPanel", "Pose: your own");

  inline constexpr const char* hint = QT_TRANSLATE_NOOP("HandsPanel", "Pick a ready-made pose. Then bend each finger below.");
  inline constexpr const char* openFirst = QT_TRANSLATE_NOOP("HandsPanel", "Open an animation first. Then shape the hands here.");
  // the reason of a greyed control while no animation is open (charter P6)
  inline constexpr const char* openWhy = QT_TRANSLATE_NOOP("HandsPanel", "Open an animation first.");

  inline constexpr const char* presetsHeading = QT_TRANSLATE_NOOP("HandsPanel", "Ready-made finger poses");
  inline constexpr const char* presetKey = QT_TRANSLATE_NOOP("HandsPanel", "Enter or Space");
  inline constexpr const char* presetTipBoth = QT_TRANSLATE_NOOP("HandsPanel", "Shapes the fingers of both hands.");
  inline constexpr const char* presetTipLeft = QT_TRANSLATE_NOOP("HandsPanel", "Shapes the fingers of the left hand.");
  inline constexpr const char* presetTipRight = QT_TRANSLATE_NOOP("HandsPanel", "Shapes the fingers of the right hand.");
  inline constexpr const char* presetsKeys = QT_TRANSLATE_NOOP("HandsPanel", "Arrow keys move. Enter or Space picks.");

  inline constexpr const char* bendHeading = QT_TRANSLATE_NOOP("HandsPanel", "Bend each finger");
  inline constexpr const char* fingerNames[HandPose::fingerCount] = {
    QT_TRANSLATE_NOOP("HandsPanel", "Thumb"), QT_TRANSLATE_NOOP("HandsPanel", "Index"), QT_TRANSLATE_NOOP("HandsPanel", "Middle"),
    QT_TRANSLATE_NOOP("HandsPanel", "Ring"), QT_TRANSLATE_NOOP("HandsPanel", "Pinky")};
  inline constexpr const char* sliderName = QT_TRANSLATE_NOOP("HandsPanel", "%1: from open to closed");
  inline constexpr const char* sliderKeys = QT_TRANSLATE_NOOP("HandsPanel", "Arrow keys");
  inline constexpr const char* percentClosed = QT_TRANSLATE_NOOP("HandsPanel", "%1 percent closed");
  inline constexpr const char* open = QT_TRANSLATE_NOOP("HandsPanel", "Open");
  inline constexpr const char* closed = QT_TRANSLATE_NOOP("HandsPanel", "Closed");

  inline constexpr const char* resetHand = QT_TRANSLATE_NOOP("HandsPanel", "Reset hand");
  inline constexpr const char* resetBoth = QT_TRANSLATE_NOOP("HandsPanel", "Reset both hands");
  inline constexpr const char* resetLeftName = QT_TRANSLATE_NOOP("HandsPanel", "Reset left hand");
  inline constexpr const char* resetRightName = QT_TRANSLATE_NOOP("HandsPanel", "Reset right hand");
  inline constexpr const char* resetTip = QT_TRANSLATE_NOOP("HandsPanel", "Back to the open hand.");
  inline constexpr const char* resetWhy = QT_TRANSLATE_NOOP("HandsPanel", "This hand is not changed.");

  inline constexpr const char* gridHeading = QT_TRANSLATE_NOOP("HandsPanel", "Each joint, in degrees");
  inline constexpr const char* fingerFullNames[HandPose::fingerCount] = {
    QT_TRANSLATE_NOOP("HandsPanel", "Thumb"), QT_TRANSLATE_NOOP("HandsPanel", "Index Finger"), QT_TRANSLATE_NOOP("HandsPanel", "Middle Finger"),
    QT_TRANSLATE_NOOP("HandsPanel", "Ring Finger"), QT_TRANSLATE_NOOP("HandsPanel", "Pinky Finger")};
  inline constexpr const char* partNames[HandPose::partCount] = {
    QT_TRANSLATE_NOOP("HandsPanel", "Base"), QT_TRANSLATE_NOOP("HandsPanel", "Mid"), QT_TRANSLATE_NOOP("HandsPanel", "Tip")};
  inline constexpr const char* gridHint = QT_TRANSLATE_NOOP("HandsPanel", "Type a number, or drag a cell sideways. Page Up and Page Down change it by 5.");
  inline constexpr const char* gridName = QT_TRANSLATE_NOOP("HandsPanel", "Each finger joint, X, Y and Z in degrees");
  inline constexpr const char* gridKeys = QT_TRANSLATE_NOOP("HandsPanel", "Type a number, or Page Up / Page Down");
  inline constexpr const char* expertLine = QT_TRANSLATE_NOOP("HandsPanel", "Expert mode shows every finger joint as numbers.");

  // the status line after a change; the window adds where the key went (keySet)
  inline constexpr const char* pickedBoth = QT_TRANSLATE_NOOP("HandsPanel", "%1 on both hands.");
  inline constexpr const char* pickedLeft = QT_TRANSLATE_NOOP("HandsPanel", "%1 on the left hand.");
  inline constexpr const char* pickedRight = QT_TRANSLATE_NOOP("HandsPanel", "%1 on the right hand.");
  inline constexpr const char* bent = QT_TRANSLATE_NOOP("HandsPanel", "%1: %2 percent closed.");
  inline constexpr const char* cellSet = QT_TRANSLATE_NOOP("HandsPanel", "%1, %2: %3 degrees.");
  inline constexpr const char* notANumber = QT_TRANSLATE_NOOP("HandsPanel", "Type a number, for example -40.");
  inline constexpr const char* resetDoneBoth = QT_TRANSLATE_NOOP("HandsPanel", "Both hands back to open.");
  inline constexpr const char* resetDoneLeft = QT_TRANSLATE_NOOP("HandsPanel", "Left hand back to open.");
  inline constexpr const char* resetDoneRight = QT_TRANSLATE_NOOP("HandsPanel", "Right hand back to open.");
  inline constexpr const char* keySetOne = QT_TRANSLATE_NOOP("HandsPanel", "Key set at frame %1.");
  inline constexpr const char* keySetMany = QT_TRANSLATE_NOOP("HandsPanel", "Key set at frame %1 for %2 joints.");

  // "Key set at frame 5." for one joint, "Key set at frame 5 for 30 joints." for more;
  // frame counts from 1, as the frame counter shows it
  QString keySet(int frame, int joints);
  // "Index Finger Mid" for row 4
  QString jointLabel(int row);
}

// The nine preset tiles: one Tab stop, arrows move between them, Enter or Space picks
class HandPresetTiles : public QWidget
{
  Q_OBJECT

  public:
    explicit HandPresetTiles(QWidget* parent = nullptr);

    void setPresets(const QVector<HandPose::Preset>& presets);
    // the tile with the tick and the selection border, -1 for none
    void setChecked(int index);
    int checked() const { return m_checked; }
    // the tile with the keyboard focus ring
    void setCurrent(int index);
    int current() const { return m_current; }
    int count() const { return m_presets.count(); }
    // three columns, two when the panel is narrow
    int columns() const;
    QRect tileRect(int index) const;
    // where a tile's name is drawn, under its picture
    QRect nameRect(int index) const;
    // the font of the names (the caption size; semibold for the picked tile)
    QFont nameFont(bool picked) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;

  signals:
    void picked(int index);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    bool event(QEvent* event) override;

  private:
    int tileWidth(int width) const;
    int tileHeight(int width) const;
    int indexAt(const QPoint& pos) const;

    QVector<HandPose::Preset> m_presets;
    int m_checked = -1;
    int m_current = 0;
};

// The joint grid of Expert mode: a row per finger joint, the columns X, Y and Z
class HandJointGrid : public QWidget
{
  Q_OBJECT

  public:
    explicit HandJointGrid(QWidget* parent = nullptr);
    ~HandJointGrid() override;

    void setHand(const HandPose::Hand& hand, bool known);
    void setSelectedRow(int row);
    int selectedRow() const { return m_selectedRow; }
    void setCurrent(int row, int axis);
    int currentRow() const { return m_row; }
    int currentAxis() const { return m_axis; }
    // the text a cell shows: "-40°", or "–" when no animation is open
    QString cellText(int row, int axis) const;
    QRect cellRect(int row, int axis) const;
    // the line editor while a cell is edited, else nullptr
    QLineEdit* editor() const { return m_editor; }
    void startEditing(const QString& first = QString());

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  signals:
    // a cell's new value (already cut to -180 to 180)
    void cellChanged(int row, int axis, double value);
    void rowPicked(int row);
    void invalidNumber();

  protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    int rowHeight() const;
    int nameWidth() const;
    int cellWidth() const;
    // the y of a joint row (0 to 14), or of the header row for -1
    int rowTop(int row) const;
    bool cellAt(const QPoint& pos, int& row, int& axis) const;
    void moveTo(int row, int axis);
    void finishEditing(bool keep);
    void change(int row, int axis, double value);

    HandPose::Hand m_hand;
    bool m_known = false;
    int m_selectedRow = -1;
    int m_row = 0;
    int m_axis = 0;
    QLineEdit* m_editor = nullptr;
    // a drag of a cell sideways
    bool m_pressed = false;
    bool m_dragged = false;
    QPoint m_pressPos;
    int m_pressRow = -1;
    int m_pressAxis = -1;
    double m_pressValue = 0.0;
};

class HandsPanel : public QWidget
{
  Q_OBJECT

  public:
    struct State
    {
      bool open = false;          // an animation is open
      QString blocked;            // why nothing can change now (empty: it can); "Open an animation first." when !open
      QChar sideMode;             // 'L', 'R', or a null QChar for Mirror
      QString pickedJoint;        // the picked joint's name (a finger joint decides the shown hand in Mirror)
      HandPose::Hand left = HandPose::openHand();
      HandPose::Hand right = HandPose::openHand();
      bool expert = false;
      bool hints = true;
    };

    explicit HandsPanel(const QVector<HandPose::Preset>& presets, QWidget* parent = nullptr);

    void setState(const State& state);
    const State& state() const { return m_state; }
    // the hand the panel shows: Left or Right, or in Mirror the hand of the picked finger joint, else the right
    HandPose::Side shownSide() const;
    const QVector<HandPose::Preset>& presets() const { return m_presets; }
    // the presets picked last stay named in "Pose: changed from ..." until this (a new file)
    void forgetPicks();

    // applies the theme's colours (both themes, charter P7)
    void applyLook();

    QSize sizeHint() const override;

    // for the tests
    HandPresetTiles* tiles() const { return m_tiles; }
    HandJointGrid* grid() const { return m_grid; }
    QSlider* fingerSlider(int finger) const { return m_sliders[finger]; }
    QLabel* fingerValue(int finger) const { return m_values[finger]; }
    QPushButton* resetButton() const { return m_reset; }
    QLabel* handName() const { return m_handName; }
    QLabel* handCaption() const { return m_handCaption; }
    QLabel* stateChip() const { return m_chip; }
    QLabel* hintLine() const { return m_hint; }
    QLabel* openLine() const { return m_openLine; }
    QLabel* gridHint() const { return m_gridHint; }
    QLabel* expertLine() const { return m_expertLine; }
    QWidget* gridSection() const { return m_gridSection; }

  signals:
    // the window makes the changes (each a key at once) and shows message plus where the key went
    void changeRequested(const HandPose::Writes& writes, const QString& message);
    // a grid row was picked: the window picks that joint (the Selected joint panel follows)
    void jointPicked(const QString& name);
    // a short message for the status line that changes nothing
    void statusMessage(const QString& message);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void applyFonts();
    void refresh();
    void pickPreset(int index);
    void bendFinger(int finger, int percent);
    void setCell(int row, int axis, double value);
    void resetHand();
    QString blockedReason() const;
    static void setReason(QWidget* widget, bool enabled, const QString& tip, const QString& why);
    void styleChip();

    QVector<HandPose::Preset> m_presets;
    int m_closed = -1;
    State m_state;
    QString m_lastPick[2];
    bool m_refreshing = false;

    QLabel* m_handIcon = nullptr;
    QLabel* m_handName = nullptr;
    QLabel* m_handCaption = nullptr;
    QLabel* m_chip = nullptr;
    QLabel* m_hintIcon = nullptr;
    QLabel* m_hint = nullptr;
    QWidget* m_hintRow = nullptr;
    QLabel* m_openIcon = nullptr;
    QLabel* m_openLine = nullptr;
    QWidget* m_openRow = nullptr;
    QList<QFrame*> m_headingBars;
    QLabel* m_presetsHeading = nullptr;
    QLabel* m_presetsCount = nullptr;
    HandPresetTiles* m_tiles = nullptr;
    QLabel* m_bendHeading = nullptr;
    QLabel* m_names[HandPose::fingerCount] = {};
    QSlider* m_sliders[HandPose::fingerCount] = {};
    QLabel* m_values[HandPose::fingerCount] = {};
    QLabel* m_scaleOpen = nullptr;
    QLabel* m_scaleClosed = nullptr;
    QPushButton* m_reset = nullptr;
    QWidget* m_gridSection = nullptr;
    QLabel* m_gridHeading = nullptr;
    HandJointGrid* m_grid = nullptr;
    QLabel* m_gridHint = nullptr;
    QLabel* m_expertLine = nullptr;
};

#endif
