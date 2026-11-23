/*
 * PinglyMotion - test of the Selected joint panel, UX phase 2b (values and texts)
 *
 * Exact answers only (src/selectedjointpanel.*, docs/UX_DECISIONS.md section 6):
 * the three pages and every text on them, the joint's head, a slider step to
 * the number in degrees and back, the allowed range line, the near-limit
 * warning (at the exact edge of six percent of the range), "Turn limits off"
 * and the limits switch, Turn / Move, the drag choice kept between sessions,
 * More options kept between sessions, greyed controls with their reason (also
 * in the status line, src/reasonline.*), the keyboard, and the panel in the
 * dark and the light theme at 100, 150 and 200 percent text with nothing
 * clipped and the axis colours from docs/design_tokens.json. The texts the
 * mockup has are also looked up in the mockup file itself.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QCheckBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QSlider>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QToolButton>

#include "constants.h"
#include "lucideicons.h"
#include "reasonline.h"
#include "selectedjointpanel.h"
#include "sliderscale.h"
#include "theme.h"
#include "uirender.h"
#include "usersettings.h"

using Mode = SelectedJointPanel::Mode;

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  QString mockup() { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/mockup/index.html")); }

  QString token(const char* name, bool dark)
  {
    const QJsonObject tokens = QJsonDocument::fromJson(readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/design_tokens.json")).toUtf8()).object();
    return tokens.value("themes").toObject().value(dark ? "dark" : "light").toObject().value(QLatin1String(name)).toString();
  }

  // the sentences of a text are at most 12 words each (tools/check_texts.py)
  void verifyShortSentences(const QString& text)
  {
    for (const QString& sentence : text.split(QStringLiteral(". ")))
      QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
  }

  SelectedJointPanel::Joint shoulder()
  {
    SelectedJointPanel::Joint joint;
    joint.name = QStringLiteral("Right Arm: Shoulder");
    joint.slName = QStringLiteral("mShoulderRight");
    joint.area = 0;
    joint.keys = 3;
    joint.canTurn = true;
    joint.canMove = false;
    joint.changed = true;
    return joint;
  }

  SelectedJointPanel::Joint pelvis()
  {
    SelectedJointPanel::Joint joint;
    joint.name = QStringLiteral("Pelvis");
    joint.slName = QStringLiteral("mPelvis");
    joint.keys = 1;
    joint.canMove = true;
    return joint;
  }

  SelectedJointPanel::Joint avatarPosition()
  {
    SelectedJointPanel::Joint joint;
    joint.name = QStringLiteral("Avatar Position");
    joint.area = 3;
    joint.canTurn = false;
    joint.canMove = true;
    return joint;
  }

  void setRange(SelectedJointPanel& panel, int axis, int low, int high)
  {
    panel.slider(Mode::Turn, axis)->setRange(low * SliderScale::precision, high * SliderScale::precision);
  }

  void setDegrees(SelectedJointPanel& panel, int axis, double degrees)
  {
    panel.slider(Mode::Turn, axis)->setValue(SliderScale::toSlider(degrees));
  }
}

class TestSelectedJoint : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void nothingToTurnYet();
    void pickAJoint();
    void jointHead();
    void hintLineFollowsHints();
    void sliderStepIsTheNumberInDegrees();
    void axisBadgesAndNames();
    void allowedRangeLine();
    void warningAtSixPercentOfTheRange();
    void warningNamesEveryAxisNearItsLimit();
    void noWarningForAWholeTurnOrWithLimitsOff();
    void turnLimitsOffTurnsTheWarningOff();
    void limitsSwitchIsTheSetting();
    void turnAndMove();
    void avatarPositionOnlyMoves();
    void resetJoint();
    void pullWithKeysIsGreyedWithAReason();
    void dragChoiceIsOnByDefaultAndKept();
    void moreOptionsIsKept();
    void blockedControlsSayWhy();
    void reasonLineNamesTheSlider();
    void keyboard();
    void textsAreShortAndFromTheMockup();
    void bothThemesAndTextSizesWithoutClipping();

  private:
    QTemporaryDir m_dir;
};

void TestSelectedJoint::initTestCase()
{
  QVERIFY(m_dir.isValid());
  // never touch the real settings of the person running the tests
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void TestSelectedJoint::init()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
  // the values a new program starts with
  UserSettings::setJointLimits(true);
  UserSettings::setTipsOn(true);
  UserSettings::setPullLimbOnDrag(true);
  UserSettings::setJointMoreOptions(false);
  Theme::apply(true);
}

void TestSelectedJoint::nothingToTurnYet()
{
  SelectedJointPanel panel;
  QCOMPARE(panel.page(), SelectedJointPanel::Page::Nothing);
  QCOMPARE(panel.pages()->currentIndex(), 0);
  QCOMPARE(panel.nothingHeading()->text(), QStringLiteral("Nothing to turn yet"));
  QCOMPARE(panel.nothingLine()->text(), QStringLiteral("Open an animation first. Then pick a joint to turn it here."));
  panel.showJoint(shoulder());
  panel.showNothing();
  QCOMPARE(panel.page(), SelectedJointPanel::Page::Nothing);
  QCOMPARE(panel.pages()->currentIndex(), 0);
}

void TestSelectedJoint::pickAJoint()
{
  SelectedJointPanel panel;
  panel.showPickJoint();
  QCOMPARE(panel.page(), SelectedJointPanel::Page::PickJoint);
  QCOMPARE(panel.pages()->currentIndex(), 1);
  QCOMPARE(panel.pickHeading()->text(), QStringLiteral("Pick a joint"));
  QCOMPARE(panel.pickLine()->text(), QStringLiteral("This panel turns one joint at a time."));
  QStringList steps;
  for (QLabel* step : panel.pickSteps()) steps << step->text();
  QCOMPARE(steps, QStringList({QStringLiteral("1. Click a joint on the figure, or in the Joints list."),
                               QStringLiteral("2. Turn it with the coloured sliders here."),
                               QStringLiteral("3. Turning it sets a key at this frame.")}));
  QCOMPARE(panel.pickDemoButton()->text(), QStringLiteral("Pick the right shoulder for me"));
  QCOMPARE(QString::fromLatin1(SelectedJointSpec::pickDemoJoint), QStringLiteral("rShldr"));
  QSignalSpy picked(&panel, &SelectedJointPanel::pickDemoRequested);
  panel.pickDemoButton()->click();
  QCOMPARE(picked.count(), 1);
  // the large main button of an empty state
  QCOMPARE(panel.pickDemoButton()->minimumHeight(), Theme::Size::controlLarge);
  QCOMPARE(panel.pickDemoButton()->palette().color(QPalette::Button).name(QColor::HexRgb).toUpper(), token("accent", true));
}

void TestSelectedJoint::jointHead()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  QCOMPARE(panel.page(), SelectedJointPanel::Page::Joint);
  QCOMPARE(panel.pages()->currentIndex(), 2);
  QCOMPARE(panel.nameLabel()->text(), QStringLiteral("Right Arm: Shoulder"));
  QCOMPARE(panel.areaChip()->text(), QStringLiteral("Body"));
  QCOMPARE(panel.keysChip()->text(), QStringLiteral("3 keys"));
  QCOMPARE(panel.slNameLabel()->text(), QStringLiteral("Second Life name: mShoulderRight"));
  QVERIFY(panel.slNameLabel()->isVisibleTo(&panel));
  QVERIFY(panel.areaChip()->styleSheet().contains(token("area_body", true), Qt::CaseInsensitive));

  const QStringList areas = {"Body", "Face", "Hands", "Other"};
  const char* areaTokens[] = {"area_body", "area_face", "area_hands", "area_other"};
  for (int area = 0; area < 4; ++area)
  {
    SelectedJointPanel::Joint joint = shoulder();
    joint.area = area;
    panel.showJoint(joint);
    QCOMPARE(panel.areaChip()->text(), areas[area]);
    // the colour is backed by the area's name (charter P10)
    QVERIFY(panel.areaChip()->styleSheet().contains(token(areaTokens[area], true), Qt::CaseInsensitive));
  }
  SelectedJointPanel::Joint joint = shoulder();
  joint.keys = 0;
  panel.showJoint(joint);
  QCOMPARE(panel.keysChip()->text(), QStringLiteral("no keys yet"));
  joint.keys = 1;
  panel.showJoint(joint);
  QCOMPARE(panel.keysChip()->text(), QStringLiteral("1 key"));
  joint.keys = 12;
  panel.showJoint(joint);
  QCOMPARE(panel.keysChip()->text(), QStringLiteral("12 keys"));

  // the Avatar Position has no Second Life name
  panel.showJoint(avatarPosition());
  QCOMPARE(panel.nameLabel()->text(), QStringLiteral("Avatar Position"));
  QCOMPARE(panel.areaChip()->text(), QStringLiteral("Other"));
  QVERIFY(!panel.slNameLabel()->isVisibleTo(&panel));
}

void TestSelectedJoint::hintLineFollowsHints()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  QCOMPARE(panel.hintLabel()->text(), QStringLiteral("Turn this joint with the three sliders. Turning it sets a key."));
  QVERIFY(panel.hintLabel()->isVisibleTo(&panel));
  panel.setHintsOn(false);
  QVERIFY(!panel.hintLabel()->isVisibleTo(&panel));
  panel.setHintsOn(true);
  QVERIFY(panel.hintLabel()->isVisibleTo(&panel));
}

void TestSelectedJoint::sliderStepIsTheNumberInDegrees()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  setRange(panel, 0, -90, 90);
  QSlider* x = panel.slider(Mode::Turn, 0);
  QLineEdit* field = panel.valueField(Mode::Turn, 0);
  // 100 slider steps are one degree (src/sliderscale.h)
  x->setValue(4550);
  QCOMPARE(field->text(), QStringLiteral("45.5"));
  x->setValue(-3000);
  QCOMPARE(field->text(), QStringLiteral("-30"));
  x->setValue(1);
  QCOMPARE(field->text(), QStringLiteral("0.01"));
  field->setText(QStringLiteral("12.34"));
  emit field->editingFinished();
  QCOMPARE(x->value(), 1234);
  // the slider stays inside its range
  field->setText(QStringLiteral("200"));
  emit field->editingFinished();
  QCOMPARE(x->value(), 9000);
  QCOMPARE(field->text(), QStringLiteral("90"));

  // with the program's own sliders, the panel lays them out and leaves the values to the window
  QSlider own(Qt::Horizontal);
  QLineEdit ownField;
  SelectedJointPanel::Controls controls;
  controls.turn[1] = &own;
  controls.turnValue[1] = &ownField;
  {
    SelectedJointPanel adopted(nullptr, &controls);
    QCOMPARE(adopted.slider(Mode::Turn, 1), &own);
    QCOMPARE(adopted.valueField(Mode::Turn, 1), &ownField);
    QVERIFY(adopted.isAncestorOf(&own));
    own.setValue(500);
    QCOMPARE(ownField.text(), QString());
    // the arrow keys turn one degree, Page Up / Page Down ten
    QCOMPARE(own.singleStep(), 100);
    QCOMPARE(own.pageStep(), 1000);
    own.setParent(nullptr);
    ownField.setParent(nullptr);
  }
}

void TestSelectedJoint::axisBadgesAndNames()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  const QStringList letters = {"X", "Y", "Z"};
  const QStringList names = {"Tilt forward or back", "Turn left or right", "Lean to the side"};
  const char* axisTokens[] = {"axis_x", "axis_y", "axis_z"};
  for (int axis = 0; axis < 3; ++axis)
  {
    QCOMPARE(panel.axisBadge(axis)->text(), letters[axis]);
    QCOMPARE(panel.axisName(axis)->text(), names[axis]);
    // the axis colour as the badge's fill, the letter in on_axis, and the slider's filled part
    QVERIFY(panel.axisBadge(axis)->styleSheet().contains(QStringLiteral("background: ") + token(axisTokens[axis], true), Qt::CaseInsensitive));
    QVERIFY(panel.axisBadge(axis)->styleSheet().contains(QStringLiteral("color: ") + token("on_axis", true), Qt::CaseInsensitive));
    QVERIFY(panel.slider(Mode::Turn, axis)->styleSheet().contains(token(axisTokens[axis], true), Qt::CaseInsensitive));
    QCOMPARE(panel.slider(Mode::Turn, axis)->accessibleName(), letters[axis] + QStringLiteral(": ") + names[axis]);
    QCOMPARE(panel.valueField(Mode::Turn, axis)->accessibleName(), letters[axis] + QStringLiteral(", ") + names[axis] + QStringLiteral(", in degrees"));
  }
  QCOMPARE(panel.slider(Mode::Turn, 0)->toolTip(),
           QStringLiteral("X (red): Tilt forward or back\nArrow keys: one degree. Page Up and Page Down: ten."));
  QCOMPARE(panel.slider(Mode::Turn, 2)->toolTip(),
           QStringLiteral("Z (blue): Lean to the side\nArrow keys: one degree. Page Up and Page Down: ten."));
  QCOMPARE(panel.sectionHeading()->text(), QStringLiteral("Turn"));
}

void TestSelectedJoint::allowedRangeLine()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  setRange(panel, 0, -90, 90);
  QCOMPARE(panel.rangeTexts(0), QStringList({QStringLiteral("-90°"), QStringLiteral("allowed range"), QStringLiteral("90°")}));
  panel.slider(Mode::Turn, 1)->setRange(-4550, 12025);
  QCOMPARE(panel.rangeTexts(1), QStringList({QStringLiteral("-45.5°"), QStringLiteral("allowed range"), QStringLiteral("120.25°")}));
  // the labels under the slider say the same
  QCOMPARE(panel.findChild<QLabel*>(QStringLiteral("rangeY0"))->text(), QStringLiteral("-45.5°"));
  QCOMPARE(panel.findChild<QLabel*>(QStringLiteral("rangeY1"))->text(), QStringLiteral("allowed range"));
  QCOMPARE(panel.findChild<QLabel*>(QStringLiteral("rangeY2"))->text(), QStringLiteral("120.25°"));
  // limits off: the window gives the slider a whole turn, the line says so
  panel.setLimitsOn(false);
  setRange(panel, 0, -180, 180);
  QCOMPARE(panel.rangeTexts(0), QStringList({QStringLiteral("-180°"), QStringLiteral("no limits"), QStringLiteral("180°")}));
}

void TestSelectedJoint::warningAtSixPercentOfTheRange()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  for (int axis = 0; axis < 3; ++axis) setRange(panel, axis, -90, 90);
  // six percent of 180 degrees is 10.8 degrees from either end
  setDegrees(panel, 0, 0);
  QVERIFY(panel.nearAxes().isEmpty());
  QVERIFY(!panel.warning()->isVisibleTo(&panel));
  setDegrees(panel, 0, 79.2);
  QVERIFY(panel.nearAxes().isEmpty());
  setDegrees(panel, 0, 79.21);
  QCOMPARE(panel.nearAxes(), QList<int>({0}));
  QVERIFY(panel.warning()->isVisibleTo(&panel));
  QCOMPARE(panel.warningTitle()->text(), QStringLiteral("Near its limit: X (tilt forward or back)."));
  QCOMPARE(panel.warningText()->text(), QStringLiteral("This joint is almost at the furthest it can turn. Turn it back a little, or pick another joint."));
  QCOMPARE(panel.turnLimitsOffButton()->text(), QStringLiteral("Turn limits off"));
  QCOMPARE(panel.axisName(0)->text(), QStringLiteral("Tilt forward or back (near its limit)"));
  // the name turns the warning colour too, and the words carry the meaning (charter P10)
  QVERIFY(panel.axisName(0)->styleSheet().contains(token("warning", true), Qt::CaseInsensitive));
  QVERIFY(panel.warning()->styleSheet().contains(token("warning", true), Qt::CaseInsensitive));
  // the low end
  setDegrees(panel, 0, -79.2);
  QVERIFY(panel.nearAxes().isEmpty());
  QCOMPARE(panel.axisName(0)->text(), QStringLiteral("Tilt forward or back"));
  QVERIFY(panel.axisName(0)->styleSheet().isEmpty());
  setDegrees(panel, 0, -90);
  QCOMPARE(panel.nearAxes(), QList<int>({0}));
}

void TestSelectedJoint::warningNamesEveryAxisNearItsLimit()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  for (int axis = 0; axis < 3; ++axis) setRange(panel, axis, -90, 90);
  setDegrees(panel, 0, 90);
  setDegrees(panel, 2, -85);
  QCOMPARE(panel.nearAxes(), QList<int>({0, 2}));
  QCOMPARE(panel.warningTitle()->text(), QStringLiteral("Near its limit: X (tilt forward or back), Z (lean to the side)."));
  QCOMPARE(panel.axisName(1)->text(), QStringLiteral("Turn left or right"));
  QCOMPARE(panel.axisName(2)->text(), QStringLiteral("Lean to the side (near its limit)"));
}

void TestSelectedJoint::noWarningForAWholeTurnOrWithLimitsOff()
{
  SelectedJointPanel panel;
  panel.showJoint(pelvis());
  // the pelvis gets almost two whole turns from the window: there is no end to be near
  setRange(panel, 0, -359, 359);
  setDegrees(panel, 0, 359);
  QVERIFY(panel.nearAxes().isEmpty());
  panel.showJoint(shoulder());
  setRange(panel, 0, -90, 90);
  setDegrees(panel, 0, 90);
  QCOMPARE(panel.nearAxes(), QList<int>({0}));
  panel.setLimitsOn(false);
  QVERIFY(panel.nearAxes().isEmpty());
  QVERIFY(!panel.warning()->isVisibleTo(&panel));
}

void TestSelectedJoint::turnLimitsOffTurnsTheWarningOff()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  setRange(panel, 0, -90, 90);
  setDegrees(panel, 0, 88);
  QVERIFY(panel.warning()->isVisibleTo(&panel));
  QSignalSpy toggled(&panel, &SelectedJointPanel::limitsToggled);
  panel.turnLimitsOffButton()->click();
  QCOMPARE(toggled.count(), 1);
  QCOMPARE(toggled.at(0).at(0).toBool(), false);
  QCOMPARE(panel.limitsOn(), false);
  QCOMPARE(panel.limitsSwitch()->isChecked(), false);
  QVERIFY(!panel.warning()->isVisibleTo(&panel));
  QCOMPARE(panel.axisName(0)->text(), QStringLiteral("Tilt forward or back"));
  QCOMPARE(panel.rangeTexts(0).at(1), QStringLiteral("no limits"));
  // the switch brings them back
  panel.limitsSwitch()->setChecked(true);
  QCOMPARE(toggled.count(), 2);
  QCOMPARE(toggled.at(1).at(0).toBool(), true);
  QCOMPARE(panel.limitsOn(), true);
  QVERIFY(panel.warning()->isVisibleTo(&panel));
  // the window telling the panel sends no signal back
  panel.setLimitsOn(false);
  QCOMPARE(toggled.count(), 2);
  QCOMPARE(panel.limitsSwitch()->isChecked(), false);
}

void TestSelectedJoint::limitsSwitchIsTheSetting()
{
  QCOMPARE(QString::fromLatin1(SelectedJointSpec::limitsSwitch), QStringLiteral("Warn near the joint's limits"));
  {
    SelectedJointPanel panel;
    QCOMPARE(panel.limitsSwitch()->text(), QStringLiteral("Warn near the joint's limits"));
    QCOMPARE(panel.limitsSwitch()->toolTip(), QStringLiteral("Second Life has no limits. Turn them off for odd poses on purpose."));
    QCOMPARE(panel.limitsSwitch()->isChecked(), true);
  }
  // the program keeps the choice in joint_limits (UserSettings), read again at the next start
  UserSettings::setJointLimits(false);
  UserSettings::writeSettings();
  UserSettings::setJointLimits(true);
  UserSettings::readSettings();
  QCOMPARE(UserSettings::jointLimits(), false);
  SelectedJointPanel panel;
  QCOMPARE(panel.limitsOn(), false);
  QCOMPARE(panel.limitsSwitch()->isChecked(), false);
}

void TestSelectedJoint::turnAndMove()
{
  SelectedJointPanel panel;
  panel.setMoreOpen(true);
  panel.showJoint(shoulder());
  QCOMPARE(panel.findChild<QLabel*>(QStringLiteral("changeLabel"))->text(), QStringLiteral("Change"));
  QCOMPARE(panel.turnModeButton()->text(), QStringLiteral("Turn"));
  QCOMPARE(panel.moveModeButton()->text(), QStringLiteral("Move"));
  QCOMPARE(panel.mode(), Mode::Turn);
  QVERIFY(panel.turnModeButton()->isChecked());
  // a shoulder cannot move
  QVERIFY(!panel.moveModeButton()->isEnabled());
  QCOMPARE(panel.moveModeButton()->toolTip(), QStringLiteral("Only the pelvis and the Avatar Position can be moved."));
  QCOMPARE(panel.moveModeButton()->statusTip(), QStringLiteral("Only the pelvis and the Avatar Position can be moved."));
  panel.setMode(Mode::Move);
  QCOMPARE(panel.mode(), Mode::Turn);

  // the pelvis can
  panel.showJoint(pelvis());
  QVERIFY(panel.moveModeButton()->isEnabled());
  QVERIFY(panel.turnModeButton()->isEnabled());
  QSignalSpy changed(&panel, &SelectedJointPanel::modeChanged);
  panel.moveModeButton()->click();
  QCOMPARE(panel.mode(), Mode::Move);
  QCOMPARE(changed.count(), 1);
  QVERIFY(panel.moveModeButton()->isChecked());
  QCOMPARE(panel.sectionHeading()->text(), QStringLiteral("Move"));
  QVERIFY(panel.slider(Mode::Move, 0)->isVisibleTo(&panel));
  QVERIFY(!panel.slider(Mode::Turn, 0)->isVisibleTo(&panel));
  // moving has no limits switch and no warning, and no degrees
  QVERIFY(!panel.limitsSwitch()->isVisibleTo(&panel));
  panel.slider(Mode::Move, 1)->setRange(-19600, 19600);
  QCOMPARE(panel.rangeTexts(1), QStringList({QStringLiteral("-196"), QStringLiteral("allowed range"), QStringLiteral("196")}));
  QStringList moveNames;
  for (int axis = 0; axis < 3; ++axis)
    moveNames << panel.slider(Mode::Move, axis)->accessibleName();
  QCOMPARE(moveNames, QStringList({"X: Left or right", "Y: Up or down", "Z: Forward or back"}));
  QCOMPARE(panel.valueField(Mode::Move, 0)->accessibleName(), QStringLiteral("X, Left or right"));
  panel.turnModeButton()->click();
  QCOMPARE(panel.mode(), Mode::Turn);
  QCOMPARE(changed.count(), 2);
  QCOMPARE(panel.sectionHeading()->text(), QStringLiteral("Turn"));
  // a joint that cannot move goes back to Turn
  panel.moveModeButton()->click();
  panel.showJoint(shoulder());
  QCOMPARE(panel.mode(), Mode::Turn);
  QVERIFY(panel.turnModeButton()->isChecked());
}

void TestSelectedJoint::avatarPositionOnlyMoves()
{
  SelectedJointPanel panel;
  panel.setMoreOpen(true);
  panel.showJoint(avatarPosition());
  QCOMPARE(panel.mode(), Mode::Move);
  QVERIFY(panel.moveModeButton()->isChecked());
  QVERIFY(!panel.turnModeButton()->isEnabled());
  QCOMPARE(panel.turnModeButton()->toolTip(), QStringLiteral("The Avatar Position can only be moved."));
  QCOMPARE(panel.sectionHeading()->text(), QStringLiteral("Move"));
  panel.setMode(Mode::Turn);
  QCOMPARE(panel.mode(), Mode::Move);
  QVERIFY(panel.nearAxes().isEmpty());
}

void TestSelectedJoint::resetJoint()
{
  SelectedJointPanel panel;
  SelectedJointPanel::Joint joint = shoulder();
  joint.changed = false;
  panel.showJoint(joint);
  QCOMPARE(panel.resetButton()->text(), QStringLiteral("Reset joint"));
  QVERIFY(!panel.resetButton()->isEnabled());
  QCOMPARE(panel.resetButton()->toolTip(), QStringLiteral("This joint is not changed."));
  QCOMPARE(panel.resetButton()->statusTip(), QStringLiteral("This joint is not changed."));
  joint.changed = true;
  panel.showJoint(joint);
  QVERIFY(panel.resetButton()->isEnabled());
  QCOMPARE(panel.resetButton()->toolTip(), QStringLiteral("Reset this joint to how it is in the first frame."));
  QCOMPARE(panel.resetButton()->statusTip(), QString());
  QSignalSpy reset(&panel, &SelectedJointPanel::resetRequested);
  panel.resetButton()->click();
  QCOMPARE(reset.count(), 1);
}

void TestSelectedJoint::pullWithKeysIsGreyedWithAReason()
{
  // UX phase 4b part 2: Pull with keys works, and is greyed only when the joint cannot turn
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  QCOMPARE(panel.pullKeysButton()->text(), QStringLiteral("Pull with keys"));
  QVERIFY(panel.pullKeysButton()->isEnabled());
  QCOMPARE(panel.pullKeysButton()->toolTip(),
           QStringLiteral("Pull with the keyboard (Enter in the 3D view)\nPick the joint up and move it with the arrow keys. Enter sets a key. Esc cancels."));
  QCOMPARE(panel.pullKeysButton()->statusTip(), QString());
  QSignalSpy pull(&panel, &SelectedJointPanel::pullKeysRequested);
  panel.pullKeysButton()->click();
  QCOMPARE(pull.count(), 1);
  // greyed with the sliders' reason while they are
  panel.setTurnBlocked(QStringLiteral("Stop playing first."));
  QVERIFY(!panel.pullKeysButton()->isEnabled());
  QCOMPARE(panel.pullKeysButton()->toolTip(), QStringLiteral("Stop playing first."));
  QCOMPARE(panel.pullKeysButton()->statusTip(), QStringLiteral("Stop playing first."));
  panel.setTurnBlocked(QString());
  QVERIFY(panel.pullKeysButton()->isEnabled());
  // the Avatar Position only moves
  SelectedJointPanel::Joint position;
  position.name = QStringLiteral("Avatar Position");
  position.canTurn = false;
  position.canMove = true;
  panel.showJoint(position);
  QVERIFY(!panel.pullKeysButton()->isEnabled());
  QCOMPARE(panel.pullKeysButton()->toolTip(), QStringLiteral("The Avatar Position can only be moved."));
}

void TestSelectedJoint::dragChoiceIsOnByDefaultAndKept()
{
  {
    SelectedJointPanel panel;
    panel.showJoint(shoulder());
    QCOMPARE(panel.dragHeading()->text(), QStringLiteral("When you drag this joint in the 3D view"));
    QCOMPARE(panel.pullLimbButton()->text(), QStringLiteral("Pull the limb"));
    QCOMPARE(panel.turnOnlyButton()->text(), QStringLiteral("Turn only this joint"));
    QCOMPARE(panel.dragCaption()->text(), QStringLiteral("The joints above it bend, so the limb follows your pointer. This is called IK. "
                                                         "Hold Shift when you press to get the other one."));
    // on by default
    QCOMPARE(panel.pullLimb(), true);
    QVERIFY(panel.pullLimbButton()->isChecked());
    QVERIFY(!panel.turnOnlyButton()->isChecked());
    // both work since UX phase 4b part 2, with the mockup's tips
    QVERIFY(panel.pullLimbButton()->isEnabled());
    QVERIFY(panel.turnOnlyButton()->isEnabled());
    QCOMPARE(panel.pullLimbButton()->toolTip(), QStringLiteral("Pull the limb (IK) (Shift when you press: the other one)"));
    QCOMPARE(panel.turnOnlyButton()->toolTip(), QStringLiteral("Turn only this joint"));
    // choosing Turn only changes the caption, the setting and says so
    QSignalSpy chosen(&panel, &SelectedJointPanel::dragChoiceChanged);
    panel.turnOnlyButton()->click();
    QCOMPARE(chosen.count(), 1);
    QCOMPARE(chosen.at(0).at(0).toBool(), false);
    QCOMPARE(UserSettings::pullLimbOnDrag(), false);
    QCOMPARE(panel.dragCaption()->text(), QStringLiteral("Only this joint turns; the rest of the body stays still. "
                                                         "Hold Shift when you press to get the other one."));
    panel.pullLimbButton()->click();
    QCOMPARE(chosen.count(), 2);
    QCOMPARE(chosen.at(1).at(0).toBool(), true);
    QCOMPARE(UserSettings::pullLimbOnDrag(), true);
    QCOMPARE(SelectedJointSpec::tr2(SelectedJointSpec::dragChosenPull), QStringLiteral("Dragging a joint pulls the limb (IK)."));
    QCOMPARE(SelectedJointSpec::tr2(SelectedJointSpec::dragChosenTurn), QStringLiteral("Dragging a joint turns only that joint."));
  }
  // the choice is kept in pull_limb_on_drag and read again at the next start
  QCOMPARE(UserSettings::pullLimbOnDrag(), true);
  UserSettings::setPullLimbOnDrag(false);
  UserSettings::writeSettings();
  UserSettings::setPullLimbOnDrag(true);
  UserSettings::readSettings();
  QCOMPARE(UserSettings::pullLimbOnDrag(), false);
  {
    SelectedJointPanel panel;
    QCOMPARE(panel.pullLimb(), false);
    QVERIFY(panel.turnOnlyButton()->isChecked());
  }
  // a settings file without the key gives the default, on
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.setValue(QStringLiteral("user_settings/1.0/loop"), true);
  settings.sync();
  UserSettings::readSettings();
  QCOMPARE(UserSettings::pullLimbOnDrag(), true);
  QCOMPARE(UserSettings::jointMoreOptions(), false);
}

void TestSelectedJoint::moreOptionsIsKept()
{
  {
    SelectedJointPanel panel;
    panel.showJoint(shoulder());
    QCOMPARE(panel.moreButton()->text(), QStringLiteral("More options"));
    QCOMPARE(panel.findChild<QLabel*>(QStringLiteral("moreCaption"))->text(), QStringLiteral("turn or move"));
    // folded at first
    QCOMPARE(panel.moreOpen(), false);
    QVERIFY(!panel.moreBody()->isVisibleTo(&panel));
    panel.moreButton()->click();
    QCOMPARE(panel.moreOpen(), true);
    QVERIFY(panel.moreBody()->isVisibleTo(&panel));
    QCOMPARE(UserSettings::jointMoreOptions(), true);
  }
  UserSettings::writeSettings();
  UserSettings::setJointMoreOptions(false);
  UserSettings::readSettings();
  QCOMPARE(UserSettings::jointMoreOptions(), true);
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  QVERIFY(panel.moreBody()->isVisibleTo(&panel));
}

void TestSelectedJoint::blockedControlsSayWhy()
{
  const QString playing = QStringLiteral("Stop playing first.");
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  panel.setTurnBlocked(playing);
  for (int axis = 0; axis < 3; ++axis)
  {
    QVERIFY(!panel.slider(Mode::Turn, axis)->isEnabled());
    QCOMPARE(panel.slider(Mode::Turn, axis)->toolTip(), playing);
    QCOMPARE(panel.slider(Mode::Turn, axis)->statusTip(), playing);
    QVERIFY(!panel.valueField(Mode::Turn, axis)->isEnabled());
    QCOMPARE(panel.valueField(Mode::Turn, axis)->statusTip(), playing);
  }
  QVERIFY(!panel.resetButton()->isEnabled());
  QCOMPARE(panel.resetButton()->toolTip(), playing);
  // the limits switch is a setting: it always works
  QVERIFY(panel.limitsSwitch()->isEnabled());
  panel.setTurnBlocked(QString());
  for (int axis = 0; axis < 3; ++axis)
  {
    QVERIFY(panel.slider(Mode::Turn, axis)->isEnabled());
    QCOMPARE(panel.slider(Mode::Turn, axis)->statusTip(), QString());
  }
  QVERIFY(panel.resetButton()->isEnabled());

  // moving has its own reason; Reset joint follows the shown one
  const QString locked = QStringLiteral("The first frame is locked. Turn off Lock the first frame to set a key.");
  panel.showJoint(pelvis());
  panel.setMoveBlocked(locked);
  QVERIFY(panel.slider(Mode::Turn, 0)->isEnabled());
  QVERIFY(!panel.slider(Mode::Move, 0)->isEnabled());
  QCOMPARE(panel.slider(Mode::Move, 0)->toolTip(), locked);
  panel.setMode(Mode::Move);
  QVERIFY(!panel.resetButton()->isEnabled());
  QCOMPARE(panel.resetButton()->toolTip(), locked);
}

void TestSelectedJoint::reasonLineNamesTheSlider()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  panel.setTurnBlocked(QStringLiteral("Stop playing first."));
  const ReasonLine::Reason reason = ReasonLine::reasonFor(panel.slider(Mode::Turn, 1));
  QVERIFY(reason.valid());
  QCOMPARE(ReasonLine::lineText(reason.name, reason.why), QStringLiteral("Y: Turn left or right: Stop playing first."));
  const ReasonLine::Reason keys = ReasonLine::reasonFor(panel.pullKeysButton());
  QCOMPARE(ReasonLine::lineText(keys.name, keys.why), QStringLiteral("Pull with keys: Stop playing first."));
  // a working control has no reason
  panel.setTurnBlocked(QString());
  QVERIFY(!ReasonLine::reasonFor(panel.slider(Mode::Turn, 1)).valid());
}

void TestSelectedJoint::keyboard()
{
  SelectedJointPanel panel;
  panel.showJoint(shoulder());
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  setRange(panel, 0, -90, 90);
  setDegrees(panel, 0, 0);
  QSlider* x = panel.slider(Mode::Turn, 0);
  x->setFocus();
  QTest::keyClick(x, Qt::Key_Right);
  QCOMPARE(x->value(), 100);
  QCOMPARE(panel.valueField(Mode::Turn, 0)->text(), QStringLiteral("1"));
  QTest::keyClick(x, Qt::Key_PageUp);
  QCOMPARE(x->value(), 1100);
  QTest::keyClick(x, Qt::Key_Left);
  QCOMPARE(x->value(), 1000);
  QTest::keyClick(x, Qt::Key_End);
  QCOMPARE(x->value(), 9000);
  QVERIFY(panel.warning()->isVisibleTo(&panel));

  // the focused slider's handle shows the focus colour, the others do not
  const auto hasFocusColour = [](QWidget* widget)
  {
    const QImage image = widget->grab().toImage();
    const QColor focusColour = Theme::color(Theme::Token::FocusRing);
    for (int px = 0; px < image.width(); ++px)
      for (int py = 0; py < image.height(); ++py)
      {
        const QColor c = image.pixelColor(px, py);
        if (std::abs(c.red() - focusColour.red()) < 6 && std::abs(c.green() - focusColour.green()) < 6 && std::abs(c.blue() - focusColour.blue()) < 6)
          return true;
      }
    return false;
  };
  QVERIFY2(hasFocusColour(x), "no focus colour on the focused slider");
  QVERIFY2(!hasFocusColour(panel.slider(Mode::Turn, 1)), "focus colour on a slider without focus");

  // every control can take the keyboard focus (charter P8)
  const QList<QWidget*> controls = {panel.slider(Mode::Turn, 0), panel.valueField(Mode::Turn, 0), panel.turnLimitsOffButton(),
                                    panel.limitsSwitch(), panel.resetButton(), panel.moreButton(), panel.turnModeButton(),
                                    panel.moveModeButton()};
  for (QWidget* control : controls)
    QVERIFY2(control->focusPolicy() & Qt::TabFocus, qPrintable(control->objectName()));

  // Space on "Turn limits off" turns them off and leaves the focus on the switch
  panel.turnLimitsOffButton()->setFocus();
  QTest::keyClick(panel.turnLimitsOffButton(), Qt::Key_Space);
  QCOMPARE(panel.limitsOn(), false);
  QVERIFY(!panel.warning()->isVisibleTo(&panel));
  QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(panel.limitsSwitch()));
  QTest::keyClick(panel.limitsSwitch(), Qt::Key_Space);
  QCOMPARE(panel.limitsOn(), true);

  // Space opens More options
  panel.moreButton()->setFocus();
  QTest::keyClick(panel.moreButton(), Qt::Key_Space);
  QVERIFY(panel.moreBody()->isVisibleTo(&panel));

  // Tab follows the screen: the X slider, its number, then the Y slider
  x->setFocus();
  QCOMPARE(x->nextInFocusChain(), static_cast<QWidget*>(panel.valueField(Mode::Turn, 0)));
}

void TestSelectedJoint::textsAreShortAndFromTheMockup()
{
  const QString page = mockup();
  QVERIFY(!page.isEmpty());
  // the texts as the mockup writes them
  const char* fromMockup[] = {SelectedJointSpec::nothingHeading, SelectedJointSpec::nothingLine, SelectedJointSpec::pickHeading,
                              SelectedJointSpec::pickLine, SelectedJointSpec::pickDemo, SelectedJointSpec::nearText,
                              SelectedJointSpec::turnLimitsOff, SelectedJointSpec::limitsSwitch, SelectedJointSpec::resetJoint,
                              SelectedJointSpec::pullKeys, SelectedJointSpec::dragHeading, SelectedJointSpec::pullLimb,
                              SelectedJointSpec::turnOnly, SelectedJointSpec::moreOptions, SelectedJointSpec::changeLabel,
                              SelectedJointSpec::pullKeysName, SelectedJointSpec::pullKeysKey, SelectedJointSpec::pullLimbName,
                              SelectedJointSpec::pullLimbKey, SelectedJointSpec::dragCaptionTurn, SelectedJointSpec::dragCaptionShift,
                              SelectedJointSpec::dragChosenPull, SelectedJointSpec::dragChosenTurn,
                              SelectedJointSpec::moveMode, SelectedJointSpec::moveWhy, SelectedJointSpec::allowedRange,
                              SelectedJointSpec::noLimits, SelectedJointSpec::resetWhy, "Second Life name: ",
                              SelectedJointSpec::axes[0].turnName, SelectedJointSpec::axes[1].turnName, SelectedJointSpec::axes[2].turnName};
  for (const char* text : fromMockup)
  {
    QString t = QString::fromUtf8(text).replace(QLatin1String("'"), QLatin1String("&#39;"));
    QVERIFY2(page.contains(QString::fromUtf8(text)) || page.contains(t), text);
  }
  QVERIFY(page.contains(QStringLiteral("Near its limit: ")));
  QVERIFY(page.contains(QStringLiteral(" (near its limit)")));

  // every text: at most 12 words per sentence
  const char* all[] = {SelectedJointSpec::nothingLine, SelectedJointSpec::pickLine, SelectedJointSpec::pickSteps[0],
                       SelectedJointSpec::pickSteps[1], SelectedJointSpec::pickSteps[2], SelectedJointSpec::hint,
                       SelectedJointSpec::nearText, SelectedJointSpec::limitsTip, SelectedJointSpec::resetTip,
                       SelectedJointSpec::resetWhy, SelectedJointSpec::pullKeysTip, SelectedJointSpec::dragCaptionPull,
                       SelectedJointSpec::dragCaptionTurn, SelectedJointSpec::dragCaptionShift, SelectedJointSpec::dragChosenPull,
                       SelectedJointSpec::dragChosenTurn, SelectedJointSpec::moveWhy, SelectedJointSpec::turnWhy,
                       SelectedJointSpec::sliderKeys, SelectedJointSpec::dragHeading};
  for (const char* text : all)
    verifyShortSentences(QString::fromUtf8(text));

  // the icons exist in the program's Lucide set
  for (const char* name : {"rotate-3d", "mouse-pointer-2", "move-3d", "triangle-alert", "rotate-ccw", "chevron-right", "chevron-down"})
    QVERIFY2(!LucideIcons::path(QLatin1String(name)).isEmpty(), name);
}

void TestSelectedJoint::bothThemesAndTextSizesWithoutClipping()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    for (int pageNumber = 0; pageNumber < 3; ++pageNumber)
    {
      SelectedJointPanel panel;
      panel.setMoreOpen(true);
      if (pageNumber == 1) panel.showPickJoint();
      if (pageNumber == 2)
      {
        panel.showJoint(shoulder());
        for (int axis = 0; axis < 3; ++axis) setRange(panel, axis, -90, 90);
        setDegrees(panel, 0, 89);   // the warning shows
      }
      // the first pass lays out the wrapped texts at their real width; the
      // second draws the panel at the height they then need (in the program
      // the panel's scroll area gives it that height on its own)
      UiRender::render(&panel);
      const QImage image = UiRender::render(&panel);
      UiRender::save(image, QStringLiteral("selectedjoint%1").arg(pageNumber), v);
      const QString why = UiRender::clipping(&panel).join(QLatin1Char('\n'));
      QVERIFY2(why.isEmpty(), qPrintable(v.name() + QStringLiteral(" page ") + QString::number(pageNumber) + QStringLiteral(": ") + why));
      if (pageNumber != 2) continue;
      QVERIFY(panel.warning()->isVisible());
      // the axis colours of this theme, from the tokens
      const char* axisTokens[] = {"axis_x", "axis_y", "axis_z"};
      for (int axis = 0; axis < 3; ++axis)
      {
        QVERIFY(panel.axisBadge(axis)->styleSheet().contains(token(axisTokens[axis], v.dark), Qt::CaseInsensitive));
        // the badge's fill, a little inside its corner
        const QString colour = UiRender::expectColour(image, &panel, panel.axisBadge(axis), QPoint(2, panel.axisBadge(axis)->height() / 2),
                                                      Theme::color(axis == 0 ? Theme::Token::AxisX : (axis == 1 ? Theme::Token::AxisY : Theme::Token::AxisZ), v.dark),
                                                      QStringLiteral("axis badge"), 8);
        QVERIFY2(colour.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + colour));
      }
      QVERIFY(panel.warning()->styleSheet().contains(token("warning", v.dark), Qt::CaseInsensitive));
    }
  }
}

QTEST_MAIN(TestSelectedJoint)
#include "tst_selectedjoint.moc"
