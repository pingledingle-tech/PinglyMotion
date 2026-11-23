/*
 * PinglyMotion - the Selected joint panel, UX phase 2b (values and texts)
 *
 * The panel on the right that turns one joint, built from the approved
 * mockup (docs/mockup/index.html, "Selected joint panel"; docs/UX_DECISIONS.md,
 * section 6). It has three pages:
 *
 *  - "Nothing to turn yet" while no animation is open;
 *  - "Pick a joint" while an animation is open and no joint is picked, with
 *    three steps and a button that picks the right shoulder;
 *  - the joint itself: its name, body area, key count and Second Life name,
 *    one hint line (with hints on), the X / Y / Z sliders (X red, Y green,
 *    Z blue, each with its letter, a plain name, the number in degrees and the
 *    allowed range under it), the near-limit warning with "Turn limits off",
 *    the "Warn near the joint's limits" switch, Reset joint, Pull with keys,
 *    the drag choice (Pull the limb or Turn only this joint), and under
 *    More options the Turn / Move choice.
 *
 * The panel does not change the animation itself. The sliders and number
 * fields can be the program's own ones from the designer file (the window
 * passes them in, so its existing code keeps driving them); the panel reads
 * their ranges and values for the range lines and the warning. Without them
 * (the tests) it makes its own.
 *
 * Dragging a limb in the 3D view (UX phase 4b part 2): the drag choice (Pull
 * the limb, the default, or Turn only this joint; UserSettings::pullLimbOnDrag())
 * decides what a drag does, and Pull with keys asks the 3D view to pick the
 * joint up for the arrow keys.
 *
 * One table (SelectedJointSpec) for the widget and for tests/selectedjoint,
 * which checks every text exactly.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef SELECTEDJOINTPANEL_H
#define SELECTEDJOINTPANEL_H

#include <QCoreApplication>
#include <QLineEdit>
#include <QList>
#include <QString>
#include <QWidget>

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QFrame;
class QLabel;
class QPushButton;
class QSlider;
class QStackedWidget;
class QToolButton;

namespace SelectedJointSpec
{
  // the page while no animation is open
  inline constexpr const char* nothingHeading = QT_TRANSLATE_NOOP("SelectedJointPanel", "Nothing to turn yet");
  inline constexpr const char* nothingLine = QT_TRANSLATE_NOOP("SelectedJointPanel", "Open an animation first. Then pick a joint to turn it here.");

  // the page while no joint is picked
  inline constexpr const char* pickHeading = QT_TRANSLATE_NOOP("SelectedJointPanel", "Pick a joint");
  inline constexpr const char* pickLine = QT_TRANSLATE_NOOP("SelectedJointPanel", "This panel turns one joint at a time.");
  inline constexpr const char* pickSteps[] = {
    QT_TRANSLATE_NOOP("SelectedJointPanel", "Click a joint on the figure, or in the Joints list."),
    QT_TRANSLATE_NOOP("SelectedJointPanel", "Turn it with the coloured sliders here."),
    QT_TRANSLATE_NOOP("SelectedJointPanel", "Turning it sets a key at this frame.")};
  inline constexpr const char* pickDemo = QT_TRANSLATE_NOOP("SelectedJointPanel", "Pick the right shoulder for me");
  // the joint the button picks, by its name in the animation file
  inline constexpr const char* pickDemoJoint = "rShldr";

  // the joint page
  inline constexpr const char* slName = QT_TRANSLATE_NOOP("SelectedJointPanel", "Second Life name: %1");
  inline constexpr const char* noKeys = QT_TRANSLATE_NOOP("SelectedJointPanel", "no keys yet");
  inline constexpr const char* oneKey = QT_TRANSLATE_NOOP("SelectedJointPanel", "1 key");
  inline constexpr const char* manyKeys = QT_TRANSLATE_NOOP("SelectedJointPanel", "%1 keys");
  inline constexpr const char* areas[] = {
    QT_TRANSLATE_NOOP("SelectedJointPanel", "Body"), QT_TRANSLATE_NOOP("SelectedJointPanel", "Face"),
    QT_TRANSLATE_NOOP("SelectedJointPanel", "Hands"), QT_TRANSLATE_NOOP("SelectedJointPanel", "Other")};
  inline constexpr const char* hint = QT_TRANSLATE_NOOP("SelectedJointPanel", "Turn this joint with the three sliders. Turning it sets a key.");
  inline constexpr const char* turnHeading = QT_TRANSLATE_NOOP("SelectedJointPanel", "Turn");
  inline constexpr const char* moveHeading = QT_TRANSLATE_NOOP("SelectedJointPanel", "Move");

  struct Axis
  {
    const char* letter;
    const char* turnName;   // what turning on this axis does
    const char* moveName;   // what moving along this axis does
    const char* ring;       // the ring colour's name, for the tooltip
  };
  inline constexpr Axis axes[] = {
    {"X", QT_TRANSLATE_NOOP("SelectedJointPanel", "Tilt forward or back"), QT_TRANSLATE_NOOP("SelectedJointPanel", "Left or right"), QT_TRANSLATE_NOOP("SelectedJointPanel", "red")},
    {"Y", QT_TRANSLATE_NOOP("SelectedJointPanel", "Turn left or right"), QT_TRANSLATE_NOOP("SelectedJointPanel", "Up or down"), QT_TRANSLATE_NOOP("SelectedJointPanel", "green")},
    {"Z", QT_TRANSLATE_NOOP("SelectedJointPanel", "Lean to the side"), QT_TRANSLATE_NOOP("SelectedJointPanel", "Forward or back"), QT_TRANSLATE_NOOP("SelectedJointPanel", "blue")}};
  inline constexpr const char* sliderKeys = QT_TRANSLATE_NOOP("SelectedJointPanel", "Arrow keys: one degree. Page Up and Page Down: ten.");
  inline constexpr const char* allowedRange = QT_TRANSLATE_NOOP("SelectedJointPanel", "allowed range");
  inline constexpr const char* noLimits = QT_TRANSLATE_NOOP("SelectedJointPanel", "no limits");
  inline constexpr const char* nearSuffix = QT_TRANSLATE_NOOP("SelectedJointPanel", " (near its limit)");
  // "Near its limit: X (tilt forward or back)."; several axes are joined with ", "
  inline constexpr const char* nearTitle = QT_TRANSLATE_NOOP("SelectedJointPanel", "Near its limit: %1.");
  // WARN-01 in docs/TEACHING.md, as the mockup has it
  inline constexpr const char* nearText = QT_TRANSLATE_NOOP("SelectedJointPanel", "This joint is almost at the furthest it can turn. Turn it back a little, or pick another joint.");
  inline constexpr const char* turnLimitsOff = QT_TRANSLATE_NOOP("SelectedJointPanel", "Turn limits off");
  inline constexpr const char* limitsSwitch = QT_TRANSLATE_NOOP("SelectedJointPanel", "Warn near the joint's limits");
  inline constexpr const char* limitsTip = QT_TRANSLATE_NOOP("SelectedJointPanel", "Second Life has no limits. Turn them off for odd poses on purpose.");
  inline constexpr const char* resetJoint = QT_TRANSLATE_NOOP("SelectedJointPanel", "Reset joint");
  inline constexpr const char* resetTip = QT_TRANSLATE_NOOP("SelectedJointPanel", "Reset this joint to how it is in the first frame.");
  inline constexpr const char* resetWhy = QT_TRANSLATE_NOOP("SelectedJointPanel", "This joint is not changed.");
  inline constexpr const char* pullKeys = QT_TRANSLATE_NOOP("SelectedJointPanel", "Pull with keys");
  // its tip (the mockup's data-name, data-key and data-tipline; its first sentence has 15 words,
  // over the 12-word limit, so it is said in two)
  inline constexpr const char* pullKeysName = QT_TRANSLATE_NOOP("SelectedJointPanel", "Pull with the keyboard");
  inline constexpr const char* pullKeysKey = QT_TRANSLATE_NOOP("SelectedJointPanel", "Enter in the 3D view");
  inline constexpr const char* pullKeysTip = QT_TRANSLATE_NOOP("SelectedJointPanel", "Pick the joint up and move it with the arrow keys. Enter sets a key. Esc cancels.");
  inline constexpr const char* dragHeading = QT_TRANSLATE_NOOP("SelectedJointPanel", "When you drag this joint in the 3D view");
  inline constexpr const char* pullLimb = QT_TRANSLATE_NOOP("SelectedJointPanel", "Pull the limb");
  inline constexpr const char* turnOnly = QT_TRANSLATE_NOOP("SelectedJointPanel", "Turn only this joint");
  inline constexpr const char* pullLimbName = QT_TRANSLATE_NOOP("SelectedJointPanel", "Pull the limb (IK)");
  inline constexpr const char* pullLimbKey = QT_TRANSLATE_NOOP("SelectedJointPanel", "Shift when you press: the other one");
  // the caption under the choice, then the Shift sentence (the mockup's first sentence has 16
  // words, over the 12-word limit: "like pulling a puppet's hand" is left out)
  inline constexpr const char* dragCaptionPull = QT_TRANSLATE_NOOP("SelectedJointPanel", "The joints above it bend, so the limb follows your pointer. This is called IK.");
  inline constexpr const char* dragCaptionTurn = QT_TRANSLATE_NOOP("SelectedJointPanel", "Only this joint turns; the rest of the body stays still.");
  inline constexpr const char* dragCaptionShift = QT_TRANSLATE_NOOP("SelectedJointPanel", "Hold Shift when you press to get the other one.");
  // the status line after the choice changes
  inline constexpr const char* dragChosenPull = QT_TRANSLATE_NOOP("SelectedJointPanel", "Dragging a joint pulls the limb (IK).");
  inline constexpr const char* dragChosenTurn = QT_TRANSLATE_NOOP("SelectedJointPanel", "Dragging a joint turns only that joint.");
  inline QString tr2(const char* text) { return QCoreApplication::translate("SelectedJointPanel", text); }
  inline constexpr const char* moreOptions = QT_TRANSLATE_NOOP("SelectedJointPanel", "More options");
  inline constexpr const char* moreCaption = QT_TRANSLATE_NOOP("SelectedJointPanel", "turn or move");
  inline constexpr const char* changeLabel = QT_TRANSLATE_NOOP("SelectedJointPanel", "Change");
  inline constexpr const char* turnMode = QT_TRANSLATE_NOOP("SelectedJointPanel", "Turn");
  inline constexpr const char* moveMode = QT_TRANSLATE_NOOP("SelectedJointPanel", "Move");
  inline constexpr const char* moveWhy = QT_TRANSLATE_NOOP("SelectedJointPanel", "Only the pelvis and the Avatar Position can be moved.");
  inline constexpr const char* turnWhy = QT_TRANSLATE_NOOP("SelectedJointPanel", "The Avatar Position can only be moved.");

  // closer to an end than this share of the allowed range (in percent) counts as near the limit, as in the mockup
  inline constexpr int nearPercent = 6;
}

// The number field next to an axis slider: as wide as "-000.00" needs, not as
// wide as Qt's usual field. The designer file's rotation and position fields
// are of this kind.
class NumberField : public QLineEdit
{
  Q_OBJECT

  public:
    explicit NumberField(QWidget* parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
};

class SelectedJointPanel : public QWidget
{
  Q_OBJECT

  public:
    enum class Page { Nothing, PickJoint, Joint };
    enum class Mode { Turn, Move };

    // the program's own sliders and number fields, used instead of new ones
    struct Controls
    {
      QSlider* turn[3] = {nullptr, nullptr, nullptr};
      QLineEdit* turnValue[3] = {nullptr, nullptr, nullptr};
      QSlider* move[3] = {nullptr, nullptr, nullptr};
      QLineEdit* moveValue[3] = {nullptr, nullptr, nullptr};
    };

    // what the joint page shows
    struct Joint
    {
      QString name;          // friendly name, "Right Arm: Shoulder"
      QString slName;        // Second Life name, mShoulderRight (none for the Avatar Position)
      int area = 0;          // JointGroups::Tab: 0 Body, 1 Face, 2 Hands, 3 Other
      int keys = 0;          // how many keys the joint has
      bool canTurn = true;   // false for the Avatar Position
      bool canMove = false;  // the pelvis and the Avatar Position
      bool changed = false;  // different from the first frame: Reset joint works
      bool operator==(const Joint& other) const;
    };

    explicit SelectedJointPanel(QWidget* parent = nullptr, const Controls* controls = nullptr);

    void showNothing();
    void showPickJoint();
    void showJoint(const Joint& joint);
    Page page() const { return m_page; }
    const Joint& joint() const { return m_joint; }

    // greys the sliders, number fields and Reset joint with this reason
    // ("Stop playing first."); an empty reason makes them work again
    void setTurnBlocked(const QString& why);
    void setMoveBlocked(const QString& why);

    // the joint limits switch (UserSettings::jointLimits); the window sets the
    // slider ranges, the panel shows the range lines and the warning
    void setLimitsOn(bool on);
    bool limitsOn() const { return m_limitsOn; }

    // Turn or Move; a joint that cannot move always shows Turn, the Avatar Position always Move
    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    // the drag choice: Pull the limb (true, the default) or Turn only this joint
    bool pullLimb() const;
    void setHintsOn(bool on);
    bool moreOpen() const;
    void setMoreOpen(bool open);

    // the axes that are near their limit now (0 X, 1 Y, 2 Z)
    QList<int> nearAxes() const;
    // the range line of an axis: low end, middle and high end ("-90°", "allowed range", "90°")
    QStringList rangeTexts(int axis) const;

    // the parts, for the window and the tests
    QStackedWidget* pages() const { return m_pages; }
    QSlider* slider(Mode mode, int axis) const { return mode == Mode::Turn ? m_turn[axis] : m_move[axis]; }
    QLineEdit* valueField(Mode mode, int axis) const { return mode == Mode::Turn ? m_turnValue[axis] : m_moveValue[axis]; }
    QLabel* axisBadge(int axis) const { return m_badge[axis]; }
    QLabel* axisName(int axis) const { return m_axisName[axis]; }
    QLabel* nameLabel() const { return m_name; }
    QLabel* areaChip() const { return m_area; }
    QLabel* keysChip() const { return m_keys; }
    // two figures (6m): "Figure 2" beside the key count; empty hides it
    void setFigureChip(const QString& text);
    QLabel* figureChip() const { return m_figure; }
    QLabel* slNameLabel() const { return m_slName; }
    QLabel* hintLabel() const { return m_hint; }
    QLabel* sectionHeading() const { return m_section; }
    QFrame* warning() const { return m_warning; }
    QLabel* warningTitle() const { return m_warningTitle; }
    QLabel* warningText() const { return m_warningText; }
    QPushButton* turnLimitsOffButton() const { return m_limitsOff; }
    QCheckBox* limitsSwitch() const { return m_limitsSwitch; }
    QPushButton* resetButton() const { return m_reset; }
    QPushButton* pullKeysButton() const { return m_pullKeys; }
    QLabel* dragHeading() const { return m_dragHeading; }
    QPushButton* pullLimbButton() const { return m_pullLimb; }
    QPushButton* turnOnlyButton() const { return m_turnOnly; }
    QLabel* dragCaption() const { return m_dragCaption; }
    QToolButton* moreButton() const { return m_more; }
    QWidget* moreBody() const { return m_moreBody; }
    QPushButton* turnModeButton() const { return m_turnMode; }
    QPushButton* moveModeButton() const { return m_moveMode; }
    QLabel* nothingHeading() const { return m_nothingHeading; }
    QLabel* nothingLine() const { return m_nothingLine; }
    QLabel* pickHeading() const { return m_pickHeading; }
    QLabel* pickLine() const { return m_pickLine; }
    QList<QLabel*> pickSteps() const { return m_pickSteps; }
    QPushButton* pickDemoButton() const { return m_pickDemo; }

    // colours and icons again, after the theme changed
    void applyLook();

    // as high as the lines of its wrapped texts need at its width
    QSize sizeHint() const override;

  signals:
    // the switch or "Turn limits off": the window sets UserSettings::jointLimits and the ranges
    void limitsToggled(bool on);
    void modeChanged(SelectedJointPanel::Mode mode);
    void resetRequested();
    void pickDemoRequested();
    // Pull with keys: the 3D view picks the joint up (UX phase 4b part 2)
    void pullKeysRequested();
    // the drag choice changed: true for Pull the limb
    void dragChoiceChanged(bool pullLimb);

  private:
    QWidget* buildNothingPage();
    QWidget* buildPickPage();
    QWidget* buildJointPage(const Controls* controls);
    QWidget* axisRow(int axis, QSlider*& slider, QLineEdit*& field, QSlider* given, QLineEdit* givenField, Mode mode);
    void refresh();
    // the caption under the drag choice, for the choice now
    QString dragCaptionText() const;
    void refreshButtons();
    QStringList rangeTextsFor(int axis, Mode mode) const;
    void applyNearLook();
    static void setReason(QWidget* widget, bool enabled, const QString& tip, const QString& why);
    static QString number(double value);

    Page m_page = Page::Nothing;
    Mode m_mode = Mode::Turn;
    Joint m_joint;
    bool m_limitsOn = true;
    bool m_hintsOn = true;
    QString m_turnBlocked;
    QString m_moveBlocked;
    QList<int> m_lastNear;
    bool m_reasonsSet = false;

    QStackedWidget* m_pages = nullptr;
    QLabel* m_nothingIcon = nullptr;
    QLabel* m_nothingHeading = nullptr;
    QLabel* m_nothingLine = nullptr;
    QLabel* m_pickIcon = nullptr;
    QLabel* m_pickHeading = nullptr;
    QLabel* m_pickLine = nullptr;
    QList<QLabel*> m_pickSteps;
    QPushButton* m_pickDemo = nullptr;
    QList<QFrame*> m_cards;

    QLabel* m_name = nullptr;
    QLabel* m_area = nullptr;
    QLabel* m_keys = nullptr;
    QLabel* m_figure = nullptr;
    QLabel* m_slName = nullptr;
    QLabel* m_hint = nullptr;
    QLabel* m_section = nullptr;
    QWidget* m_turnRows = nullptr;
    QWidget* m_moveRows = nullptr;
    QSlider* m_turn[3] = {nullptr, nullptr, nullptr};
    QLineEdit* m_turnValue[3] = {nullptr, nullptr, nullptr};
    QSlider* m_move[3] = {nullptr, nullptr, nullptr};
    QLineEdit* m_moveValue[3] = {nullptr, nullptr, nullptr};
    QLabel* m_badge[3] = {nullptr, nullptr, nullptr};
    QLabel* m_moveBadge[3] = {nullptr, nullptr, nullptr};
    QLabel* m_axisName[3] = {nullptr, nullptr, nullptr};
    QLabel* m_moveName[3] = {nullptr, nullptr, nullptr};
    QLabel* m_unit[3] = {nullptr, nullptr, nullptr};
    QLabel* m_range[3][3] = {};
    QLabel* m_moveRange[3][3] = {};
    QFrame* m_warning = nullptr;
    QLabel* m_warningIcon = nullptr;
    QLabel* m_warningTitle = nullptr;
    QLabel* m_warningText = nullptr;
    QPushButton* m_limitsOff = nullptr;
    QCheckBox* m_limitsSwitch = nullptr;
    QPushButton* m_reset = nullptr;
    QPushButton* m_pullKeys = nullptr;
    QLabel* m_dragHeading = nullptr;
    QPushButton* m_pullLimb = nullptr;
    QPushButton* m_turnOnly = nullptr;
    QButtonGroup* m_dragGroup = nullptr;
    QLabel* m_dragCaption = nullptr;
    QToolButton* m_more = nullptr;
    QLabel* m_moreCaption = nullptr;
    QWidget* m_moreBody = nullptr;
    QLabel* m_changeLabel = nullptr;
    QPushButton* m_turnMode = nullptr;
    QPushButton* m_moveMode = nullptr;
    QButtonGroup* m_modeGroup = nullptr;
};

#endif
