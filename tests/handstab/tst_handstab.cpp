/*
 * PinglyMotion - test of the Hands tab, UX phase 2c
 *
 * Exact answers only (src/handpose.*, src/handstab.*, docs/UX_DECISIONS.md 6i):
 *
 *  - the finger joints' names and rows;
 *  - Firestorm's nine hand presets turned into the program's angles: exact
 *    numbers for both hands, the right hand the mirror of the left (Y and Z
 *    turned round), and every curled preset curling the way the calibration
 *    table in docs/ASSISTANT_GUIDE.md (our own measurement) says;
 *  - Bend each finger: 0 is the open hand, 100 is Tight Grip, in between a
 *    slerp, and the slider reads back what was set, on both hands;
 *  - the joint changes a pick, a slider, a grid cell and Reset hand ask for,
 *    with Mirror, Left and Right;
 *  - the panel: every text, the state chip, greyed controls with their reason,
 *    the keyboard (Tab order, arrows, Enter, Space, Page Up / Page Down, typing
 *    a number, Esc), Beginner and Expert;
 *  - the changes made through Animation::setRotation (the window's path), saved
 *    as .dingle and opened again;
 *  - the panel in the dark and the light theme at 100, 150 and 200 percent text
 *    with nothing clipped.
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"
#include "handpose.h"
#include "handstab.h"
#include "jointinfo.h"
#include "theme.h"
#include "uirender.h"

using HandPose::Hand;
using HandPose::Side;

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  QString mockup() { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/mockup/index.html")); }

  QString vec(const QVector3D& v)
  {
    return QStringLiteral("(%1, %2, %3)").arg(v.x()).arg(v.y()).arg(v.z());
  }

  bool near(const QVector3D& a, const QVector3D& b, float tolerance = 0.01f)
  {
    return std::abs(a.x() - b.x()) <= tolerance && std::abs(a.y() - b.y()) <= tolerance && std::abs(a.z() - b.z()) <= tolerance;
  }

  // the same angles, also when a whole turn apart (a read-back of an Euler angle near +-180)
  bool sameTurn(const QVector3D& a, const QVector3D& b, float tolerance = 0.05f)
  {
    for (int i = 0; i < 3; ++i)
    {
      float d = std::fmod(std::abs(a[i] - b[i]), 360.0f);
      if (d > 180.0f) d = 360.0f - d;
      if (d > tolerance) return false;
    }
    return true;
  }

  // the window's part, without the window: apply the writes to a State as the
  // window applies them to the animation, and give the panel the new state
  struct Harness
  {
    HandsPanel* panel;
    HandsPanel::State state;
    HandPose::Writes lastWrites;
    QString lastMessage;
    QString lastPicked;
    QString lastStatus;
    int changes = 0;

    explicit Harness(HandsPanel* p) : panel(p)
    {
      state.open = true;
      QObject::connect(panel, &HandsPanel::changeRequested, panel, [this](const HandPose::Writes& writes, const QString& message) {
        lastWrites = writes;
        lastMessage = message;
        ++changes;
        for (const HandPose::Write& w : writes)
        {
          Side side;
          const int row = HandPose::indexOf(w.joint, &side);
          (side == Side::Left ? state.left : state.right)[row] = w.angles;
        }
        panel->setState(state);
      });
      QObject::connect(panel, &HandsPanel::jointPicked, panel, [this](const QString& name) {
        lastPicked = name;
        state.pickedJoint = name;
        panel->setState(state);
      });
      QObject::connect(panel, &HandsPanel::statusMessage, panel, [this](const QString& message) { lastStatus = message; });
      panel->setState(state);
    }

    ~Harness()
    {
      // the panel may outlive the harness by a moment: no call back into it then
      QObject::disconnect(panel, nullptr, panel, nullptr);
    }

    void set(const HandsPanel::State& s)
    {
      state = s;
      panel->setState(state);
    }
  };

  void key(QWidget* widget, int k, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
  {
    QTest::keyClick(widget, static_cast<Qt::Key>(k), modifiers);
  }
}

class HandsTabTest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();

    void jointNamesAndRows();
    void firestormAxes();
    void presetsInOrder();
    void presetAnglesExact();
    void rightHandIsTheMirror();
    void curlFollowsTheCalibrationTable();
    void bendBlendsAndReadsBack();
    void writes();

    void textsAreTheMockups();
    void noAnimationGreysEverythingWithAReason();
    void playingGreysWithItsReason();
    void pickPresetWithTheKeyboardInMirror();
    void pickPresetOnOneHand();
    void shownHandFollowsThePickedFingerInMirror();
    void sliderBendsTheFingerOnBothHands();
    void stateChipWords();
    void resetHand();
    void beginnerAndExpert();
    void gridKeyboard();
    void gridTypingAndEsc();
    void tabOrder();
    void keySetText();

    void applySaveAndOpenAgain();
    void bothThemesAndTextSizes();

  private:
    JointInfo m_info;
    QVector<HandPose::Preset> m_presets;
};

void HandsTabTest::initTestCase()
{
  QString error;
  QVERIFY2(m_info.load(Constants::pinglyMotionDataPath() + QStringLiteral("/firestorm"), &error), qPrintable(error));
  m_presets = HandPose::presets(m_info);
  QCOMPARE(m_presets.count(), 9);
}

void HandsTabTest::jointNamesAndRows()
{
  QCOMPARE(HandPose::jointName(Side::Left, 0), QStringLiteral("mHandThumb1Left"));
  QCOMPARE(HandPose::jointName(Side::Left, 4), QStringLiteral("mHandIndex2Left"));
  QCOMPARE(HandPose::jointName(Side::Right, 14), QStringLiteral("mHandPinky3Right"));
  QCOMPARE(HandPose::jointName(Side::Right, 15), QString());
  Side side = Side::Left;
  QCOMPARE(HandPose::indexOf(QStringLiteral("mHandRing2Right"), &side), 10);
  QCOMPARE(side, Side::Right);
  QCOMPARE(HandPose::indexOf(QStringLiteral("mWristLeft")), -1);
  QCOMPARE(HandPose::indexOf(QStringLiteral("rShldr")), -1);
  // every finger joint is a Hands joint of joints.json, with its mirror partner on the other hand
  for (Side s : {Side::Left, Side::Right})
    for (int i = 0; i < HandPose::jointCount; ++i)
    {
      const QString name = HandPose::jointName(s, i);
      QCOMPARE(HandPose::indexOf(name), i);
      const JointInfo::Joint* joint = m_info.joint(name);
      QVERIFY2(joint, qPrintable(name));
      QCOMPARE(joint->tab, QStringLiteral("Hands"));
      QCOMPARE(m_info.mirrorOf(name), HandPose::jointName(HandPose::other(s), i));
    }
  // the grid's row names
  QCOMPARE(HandsSpec::jointLabel(0), QStringLiteral("Thumb Base"));
  QCOMPARE(HandsSpec::jointLabel(4), QStringLiteral("Index Finger Mid"));
  QCOMPARE(HandsSpec::jointLabel(14), QStringLiteral("Pinky Finger Tip"));
}

void HandsTabTest::firestormAxes()
{
  const double r = 30.0 * 3.14159265358979323846 / 180.0;
  // Second Life's X (roll) is our Z, its Y (pitch) our X, its Z (yaw) our Y
  QVERIFY(near(HandPose::fromFirestorm(QVector3D(-r, 0, 0)), QVector3D(0, 0, -30)));
  QVERIFY(near(HandPose::fromFirestorm(QVector3D(0, r, 0)), QVector3D(30, 0, 0)));
  QVERIFY(near(HandPose::fromFirestorm(QVector3D(0, 0, r)), QVector3D(0, 30, 0)));
  // the turn is Rx(roll) * Ry(pitch) * Rz(yaw): two angles together, written in X, Z, Y order
  const QVector3D both = HandPose::fromFirestorm(QVector3D(-r, r, 0));
  QVERIFY2(near(both, QVector3D(30, 0, -30), 0.01f) || std::abs(both.y()) > 0.01f, qPrintable(vec(both)));
  // the mirror rule
  QCOMPARE(HandPose::mirrored(QVector3D(1, 2, 3)), QVector3D(1, -2, -3));
}

void HandsTabTest::presetsInOrder()
{
  const QStringList names = {"Flat", "Grip", "Horns", "Okay", "Pointing", "Relaxed", "Thumbs Up", "Tight Grip", "Two Finger Salute"};
  QStringList got;
  for (const HandPose::Preset& p : m_presets) got << p.name;
  QCOMPARE(got, names);
  QCOMPARE(HandPose::presetIndex(m_presets, QStringLiteral("Tight Grip")), 7);
  QCOMPARE(HandPose::presetIndex(m_presets, QStringLiteral("Fist")), -1);
}

void HandsTabTest::presetAnglesExact()
{
  if (qEnvironmentVariableIsSet("PINGLYMOTION_DUMP_HANDS"))
    for (const HandPose::Preset& p : m_presets)
    {
      QStringList row;
      for (int i = 0; i < HandPose::jointCount; ++i) row << vec(p.left[i]);
      qInfo("%s: %s", qPrintable(p.name), qPrintable(row.join(' ')));
    }

  // Tight Grip, the left hand: every joint, in whole degrees (X, Y, Z)
  const HandPose::Preset& tight = m_presets[HandPose::presetIndex(m_presets, QStringLiteral("Tight Grip"))];
  const QVector3D tightLeft[HandPose::jointCount] = {
    {49, -2, 0}, {25, 17, -18}, {-13, -13, -5},     // Thumb
    {15, 19, -35}, {58, -50, -53}, {74, -63, -43},  // Index
    {-16, 16, -48}, {46, -46, -75}, {0, -4, -64},   // Middle
    {-9, -13, -54}, {-72, 51, -52}, {-24, 19, -51}, // Ring
    {-39, -24, -39}, {-95, 73, -27}, {-46, 34, -45}}; // Pinky
  for (int i = 0; i < HandPose::jointCount; ++i)
    QVERIFY2(tight.left[i] == tightLeft[i], qPrintable(QStringLiteral("%1: %2").arg(HandPose::jointName(Side::Left, i), vec(tight.left[i]))));

  // a few more, both hands
  const HandPose::Preset& okay = m_presets[HandPose::presetIndex(m_presets, QStringLiteral("Okay"))];
  QCOMPARE(okay.left[3], QVector3D(12, 17, -27));
  QCOMPARE(okay.right[3], QVector3D(12, -17, 27));
  QCOMPARE(okay.left[0], QVector3D(45, 11, -6));
  QCOMPARE(okay.right[0], QVector3D(45, -11, 6));
  const HandPose::Preset& flat = m_presets[HandPose::presetIndex(m_presets, QStringLiteral("Flat"))];
  QCOMPARE(flat.left[12], QVector3D(-13, -33, 9));
  QCOMPARE(flat.right[12], QVector3D(-13, 33, -9));
  QCOMPARE(flat.left[3], QVector3D(0, 14, 2));
  // Thumbs Up's middle finger mid joint is at a quarter turn of Z: the angles with the
  // smaller twist and spread are taken (0, -9, -90), not the same turn as (-180, 171, -90)
  const HandPose::Preset& thumbs = m_presets[HandPose::presetIndex(m_presets, QStringLiteral("Thumbs Up"))];
  QVERIFY2(std::abs(thumbs.left[7].x()) <= 1.0f && std::abs(thumbs.left[7].z() + 90.0f) <= 1.0f, qPrintable(vec(thumbs.left[7])));
}

void HandsTabTest::rightHandIsTheMirror()
{
  for (const HandPose::Preset& p : m_presets)
    for (int i = 0; i < HandPose::jointCount; ++i)
      QVERIFY2(p.right[i] == HandPose::mirrored(p.left[i]),
               qPrintable(QStringLiteral("%1 %2: left %3 right %4").arg(p.name, HandPose::jointName(Side::Left, i), vec(p.left[i]), vec(p.right[i]))));
}

void HandsTabTest::curlFollowsTheCalibrationTable()
{
  // our own measurement: the calibration table says where each finger tip goes on +Z
  const QString guide = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/ASSISTANT_GUIDE.md"));
  QVERIFY(!guide.isEmpty());
  int rows = 0;
  for (Side s : {Side::Left, Side::Right})
    for (int i = HandPose::partCount; i < HandPose::jointCount; ++i) // the four fingers; the thumb folds on X
    {
      const QString name = HandPose::jointName(s, i);
      const QRegularExpression row(QStringLiteral("\\| %1 \\| Z \\| \\+ \\| [a-z ]+ tip \\| (up|down)").arg(name));
      const QRegularExpressionMatch m = row.match(guide);
      QVERIFY2(m.hasMatch(), qPrintable(name));
      ++rows;
      // +Z moves the tip up on the left hand, down on the right (palm down)
      const bool plusIsUp = m.captured(1) == QLatin1String("up");
      QCOMPARE(plusIsUp, s == Side::Left);
      // so a curled preset (tip down) has Z of the other sign than "up"
      for (const char* curled : {"Tight Grip", "Grip"})
      {
        const HandPose::Preset& p = m_presets[HandPose::presetIndex(m_presets, QLatin1String(curled))];
        const float z = p.hand(s)[i].z();
        QVERIFY2(plusIsUp ? z < 0.0f : z > 0.0f, qPrintable(QStringLiteral("%1 %2: Z %3").arg(QLatin1String(curled), name).arg(z)));
      }
    }
  QCOMPARE(rows, 24);
}

void HandsTabTest::bendBlendsAndReadsBack()
{
  for (Side s : {Side::Left, Side::Right})
  {
    const Hand& closed = m_presets[7].hand(s);
    const Hand open = HandPose::openHand();
    for (int f = 0; f < HandPose::fingerCount; ++f)
    {
      // 0 is the open hand, 100 is Tight Grip exactly
      const Hand none = HandPose::withBend(open, closed, f, 0);
      const Hand full = HandPose::withBend(open, closed, f, 100);
      for (int k = 0; k < HandPose::partCount; ++k)
      {
        QCOMPARE(none[f * 3 + k], QVector3D(0, 0, 0));
        QCOMPARE(full[f * 3 + k], closed[f * 3 + k]);
      }
      // only that finger changes
      for (int i = 0; i < HandPose::jointCount; ++i)
        if (i / 3 != f) QCOMPARE(full[i], QVector3D(0, 0, 0));
      for (int percent : {0, 1, 25, 40, 50, 73, 99, 100})
      {
        const Hand bent = HandPose::withBend(open, closed, f, percent);
        QCOMPARE(HandPose::bend(bent, closed, f), percent);
      }
    }
    // half way, a finger curls half way: the index base on the curl side, about half of Tight Grip's
    const Hand half = HandPose::withBend(open, closed, 1, 50);
    const float z = half[3].z();
    const float full = closed[3].z();
    QVERIFY2(z * full > 0.0f && std::abs(z) < std::abs(full) && std::abs(z) > 0.3f * std::abs(full), qPrintable(vec(half[3])));
  }
  // the slider of the other hand reads the mirrored hand the same
  const Hand leftBent = HandPose::withBend(HandPose::openHand(), m_presets[7].left, 2, 60);
  QCOMPARE(HandPose::bend(HandPose::mirrored(leftBent), m_presets[7].right, 2), 60);
  // the blend of the left hand is the mirror of the blend of the right hand
  const Hand rightBent = HandPose::withBend(HandPose::openHand(), m_presets[7].right, 2, 60);
  for (int k = 6; k < 9; ++k) QVERIFY2(near(rightBent[k], HandPose::mirrored(leftBent[k]), 0.01f), qPrintable(vec(rightBent[k])));
  // a hand set to Tight Grip reads 100 on every slider
  for (int f = 0; f < HandPose::fingerCount; ++f) QCOMPARE(HandPose::bend(m_presets[7].left, m_presets[7].left, f), 100);
}

void HandsTabTest::writes()
{
  const HandPose::Preset& okay = m_presets[3];
  const HandPose::Writes both = HandPose::presetWrites(okay, true, true);
  QCOMPARE(both.count(), 30);
  QCOMPARE(both[0].joint, QStringLiteral("mHandThumb1Left"));
  QCOMPARE(both[0].angles, okay.left[0]);
  QCOMPARE(both[15].joint, QStringLiteral("mHandThumb1Right"));
  QCOMPARE(both[15].angles, okay.right[0]);
  QCOMPARE(HandPose::presetWrites(okay, false, true).count(), 15);
  QCOMPARE(HandPose::presetWrites(okay, false, true)[0].joint, QStringLiteral("mHandThumb1Right"));

  Hand hand = HandPose::openHand();
  hand[4] = QVector3D(5, 10, -20);
  const HandPose::Writes mirrored = HandPose::handWrites(Side::Left, hand, {4}, true);
  QCOMPARE(mirrored.count(), 2);
  QCOMPARE(mirrored[0].joint, QStringLiteral("mHandIndex2Left"));
  QCOMPARE(mirrored[0].angles, QVector3D(5, 10, -20));
  QCOMPARE(mirrored[1].joint, QStringLiteral("mHandIndex2Right"));
  QCOMPARE(mirrored[1].angles, QVector3D(5, -10, 20));
  QCOMPARE(HandPose::handWrites(Side::Left, hand, {4}, false).count(), 1);

  const HandPose::Writes reset = HandPose::resetWrites(true, false);
  QCOMPARE(reset.count(), 15);
  for (const HandPose::Write& w : reset)
  {
    QVERIFY(w.joint.endsWith(QLatin1String("Left")));
    QCOMPARE(w.angles, QVector3D(0, 0, 0));
  }
}

void HandsTabTest::textsAreTheMockups()
{
  const QString page = mockup();
  QVERIFY(!page.isEmpty());
  // the mockup's own words (docs/mockup/index.html, renderHands and handGrid)
  for (const char* text : {HandsSpec::copies, HandsSpec::onlyThis, HandsSpec::hint, HandsSpec::openFirst, HandsSpec::presetsHeading,
                           HandsSpec::bendHeading, HandsSpec::resetBoth, HandsSpec::resetHand, HandsSpec::resetWhy, HandsSpec::gridHeading,
                           HandsSpec::gridHint, HandsSpec::expertLine, HandsSpec::poseOpen, HandsSpec::poseOwn, HandsSpec::open,
                           HandsSpec::closed, HandsSpec::notANumber, HandsSpec::leftHand, HandsSpec::rightHand, HandsSpec::resetTip})
    QVERIFY2(page.contains(QLatin1String(text)), text);
  for (const char* text : HandsSpec::fingerNames) QVERIFY2(page.contains(QStringLiteral("\"%1\"").arg(QLatin1String(text))), text);
  for (const char* text : HandsSpec::fingerFullNames) QVERIFY2(page.contains(QStringLiteral("\"%1\"").arg(QLatin1String(text))), text);
  for (const char* text : HandsSpec::partNames) QVERIFY2(page.contains(QStringLiteral("\"%1\"").arg(QLatin1String(text))), text);
  QVERIFY(page.contains(QStringLiteral("Pose: changed from ")));
  QVERIFY(page.contains(QStringLiteral(" percent closed")));
  QVERIFY(page.contains(QStringLiteral(": from open to closed")));
  QVERIFY(page.contains(QStringLiteral("Shapes the fingers of ")));
  QVERIFY(page.contains(QStringLiteral(" back to open.")));
}

void HandsTabTest::noAnimationGreysEverythingWithAReason()
{
  HandsPanel panel(m_presets);
  HandsPanel::State state; // no animation open
  panel.setState(state);
  const QString why = QStringLiteral("Open an animation first.");
  QVERIFY(!panel.openLine()->isHidden());
  QCOMPARE(panel.openLine()->text(), QStringLiteral("Open an animation first. Then shape the hands here."));
  QVERIFY(panel.stateChip()->isHidden());
  QVERIFY(panel.hintLine()->parentWidget()->isHidden());
  QVERIFY(!panel.tiles()->isEnabled());
  QCOMPARE(panel.tiles()->statusTip(), why);
  QCOMPARE(panel.tiles()->toolTip(), why);
  for (int f = 0; f < HandPose::fingerCount; ++f)
  {
    QVERIFY(!panel.fingerSlider(f)->isEnabled());
    QCOMPARE(panel.fingerSlider(f)->statusTip(), why);
    QCOMPARE(panel.fingerValue(f)->text(), QStringLiteral("–"));
  }
  QVERIFY(!panel.resetButton()->isEnabled());
  QCOMPARE(panel.resetButton()->statusTip(), why);
  QVERIFY(!panel.grid()->isEnabled());
  QCOMPARE(panel.grid()->cellText(0, 0), QStringLiteral("–"));
  // the right hand in Mirror
  QCOMPARE(panel.handName()->text(), QStringLiteral("Right hand"));
  QCOMPARE(panel.handCaption()->text(), QStringLiteral("The other hand copies it."));
}

void HandsTabTest::playingGreysWithItsReason()
{
  HandsPanel panel(m_presets);
  HandsPanel::State state;
  state.open = true;
  state.blocked = QStringLiteral("Stop playing first.");
  panel.setState(state);
  QVERIFY(panel.openLine()->parentWidget()->isHidden());
  QVERIFY(!panel.tiles()->isEnabled());
  QCOMPARE(panel.tiles()->statusTip(), state.blocked);
  QCOMPARE(panel.fingerSlider(2)->statusTip(), state.blocked);
  QCOMPARE(panel.resetButton()->statusTip(), state.blocked);
  QCOMPARE(panel.grid()->statusTip(), state.blocked);
  // the values still show
  QCOMPARE(panel.fingerValue(0)->text(), QStringLiteral("0%"));
  QCOMPARE(panel.grid()->cellText(0, 0), QStringLiteral("0°"));
}

void HandsTabTest::pickPresetWithTheKeyboardInMirror()
{
  HandsPanel panel(m_presets);
  panel.resize(320, 900);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  Harness h(&panel);
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: open, nothing changed"));
  QCOMPARE(panel.tiles()->checked(), -1);
  QCOMPARE(panel.tiles()->toolTip(), QStringLiteral("Shapes the fingers of both hands. (Enter or Space)\nArrow keys move. Enter or Space picks."));
  panel.tiles()->setFocus();
  QCOMPARE(panel.tiles()->current(), 0);
  // the arrows move the focus, they do not pick
  key(panel.tiles(), Qt::Key_Right);
  key(panel.tiles(), Qt::Key_Right);
  key(panel.tiles(), Qt::Key_Right);
  QCOMPARE(panel.tiles()->current(), 3);
  QCOMPARE(h.changes, 0);
  key(panel.tiles(), Qt::Key_Down);
  QCOMPARE(panel.tiles()->current(), 6);
  key(panel.tiles(), Qt::Key_Up);
  QCOMPARE(panel.tiles()->current(), 3);
  // Enter picks Okay: both hands, 30 joints, each hand its own numbers
  key(panel.tiles(), Qt::Key_Return);
  QCOMPARE(h.changes, 1);
  QCOMPARE(h.lastWrites, HandPose::presetWrites(m_presets[3], true, true));
  QCOMPARE(h.lastMessage, QStringLiteral("Okay on both hands."));
  QCOMPARE(h.state.left, m_presets[3].left);
  QCOMPARE(h.state.right, m_presets[3].right);
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: Okay"));
  QCOMPARE(panel.tiles()->checked(), 3);
  // Space picks too
  key(panel.tiles(), Qt::Key_Left);
  key(panel.tiles(), Qt::Key_Space);
  QCOMPARE(h.lastMessage, QStringLiteral("Horns on both hands."));
  QCOMPARE(panel.tiles()->checked(), 2);
  // the sliders show how far each finger is towards Tight Grip
  for (int f = 0; f < HandPose::fingerCount; ++f)
    QCOMPARE(panel.fingerSlider(f)->value(), HandPose::bend(m_presets[2].right, m_presets[7].right, f));
  // Tight Grip is 100 on every slider
  panel.tiles()->setCurrent(7);
  key(panel.tiles(), Qt::Key_Enter);
  for (int f = 0; f < HandPose::fingerCount; ++f)
  {
    QCOMPARE(panel.fingerSlider(f)->value(), 100);
    QCOMPARE(panel.fingerValue(f)->text(), QStringLiteral("100%"));
  }
  // a click picks
  QTest::mouseClick(panel.tiles(), Qt::LeftButton, Qt::NoModifier, panel.tiles()->tileRect(0).center());
  QCOMPARE(h.lastMessage, QStringLiteral("Flat on both hands."));
  QCOMPARE(panel.tiles()->checked(), 0);
}

void HandsTabTest::pickPresetOnOneHand()
{
  HandsPanel panel(m_presets);
  Harness h(&panel);
  HandsPanel::State s = h.state;
  s.sideMode = QLatin1Char('L');
  h.set(s);
  QCOMPARE(panel.handName()->text(), QStringLiteral("Left hand"));
  QCOMPARE(panel.handCaption()->text(), QStringLiteral("Only this hand changes."));
  QCOMPARE(panel.resetButton()->text(), QStringLiteral("Reset hand"));
  QCOMPARE(panel.resetButton()->accessibleName(), QStringLiteral("Reset left hand"));
  QCOMPARE(panel.tiles()->toolTip().section(QLatin1Char('\n'), 0, 0), QStringLiteral("Shapes the fingers of the left hand. (Enter or Space)"));
  emit panel.tiles()->picked(6);
  QCOMPARE(h.lastWrites, HandPose::presetWrites(m_presets[6], true, false));
  QCOMPARE(h.lastMessage, QStringLiteral("Thumbs Up on the left hand."));
  QCOMPARE(h.state.right, HandPose::openHand());
  s = h.state;
  s.sideMode = QLatin1Char('R');
  h.set(s);
  QCOMPARE(panel.handName()->text(), QStringLiteral("Right hand"));
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: open, nothing changed"));
  QCOMPARE(panel.resetButton()->accessibleName(), QStringLiteral("Reset right hand"));
  emit panel.tiles()->picked(1);
  QCOMPARE(h.lastMessage, QStringLiteral("Grip on the right hand."));
  QCOMPARE(h.lastWrites.count(), 15);
  QCOMPARE(h.state.left, m_presets[6].left);
}

void HandsTabTest::shownHandFollowsThePickedFingerInMirror()
{
  HandsPanel panel(m_presets);
  Harness h(&panel);
  QCOMPARE(panel.shownSide(), Side::Right);
  HandsPanel::State s = h.state;
  s.pickedJoint = QStringLiteral("mHandRing2Left");
  h.set(s);
  QCOMPARE(panel.shownSide(), Side::Left);
  QCOMPARE(panel.handName()->text(), QStringLiteral("Left hand"));
  QCOMPARE(panel.grid()->selectedRow(), 10);
  s.pickedJoint = QStringLiteral("lShldr");
  h.set(s);
  QCOMPARE(panel.shownSide(), Side::Right);
  QCOMPARE(panel.grid()->selectedRow(), -1);
  // Left / Right win over the picked joint
  s.pickedJoint = QStringLiteral("mHandRing2Left");
  s.sideMode = QLatin1Char('R');
  h.set(s);
  QCOMPARE(panel.shownSide(), Side::Right);
  QCOMPARE(panel.grid()->selectedRow(), -1);
}

void HandsTabTest::sliderBendsTheFingerOnBothHands()
{
  HandsPanel panel(m_presets);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  Harness h(&panel);
  QSlider* index = panel.fingerSlider(1);
  QCOMPARE(index->accessibleName(), QStringLiteral("Index: from open to closed"));
  index->setFocus();
  // Page Up four times: 40 percent
  for (int i = 0; i < 4; ++i) key(index, Qt::Key_PageUp);
  QCOMPARE(index->value(), 40);
  QCOMPARE(panel.fingerValue(1)->text(), QStringLiteral("40%"));
  QCOMPARE(h.lastMessage, QStringLiteral("Index: 40 percent closed."));
  // the right hand (shown) and the left hand (Mirror), the index finger's three joints only
  const Hand right = HandPose::withBend(HandPose::openHand(), m_presets[7].right, 1, 40);
  QCOMPARE(h.lastWrites, HandPose::handWrites(Side::Right, right, {3, 4, 5}, true));
  for (int k = 3; k < 6; ++k)
  {
    QCOMPARE(h.state.right[k], right[k]);
    QCOMPARE(h.state.left[k], HandPose::mirrored(right[k]));
  }
  QCOMPARE(h.state.right[0], QVector3D(0, 0, 0));
  // the arrow key moves one percent
  key(index, Qt::Key_Right);
  QCOMPARE(index->value(), 41);
  QCOMPARE(h.lastMessage, QStringLiteral("Index: 41 percent closed."));
  QCOMPARE(index->accessibleDescription(), QStringLiteral("41 percent closed"));
  // in Left only the left hand changes
  HandsPanel::State s = h.state;
  s.sideMode = QLatin1Char('L');
  h.set(s);
  QCOMPARE(panel.fingerSlider(1)->value(), 41);
  QSlider* pinky = panel.fingerSlider(4);
  pinky->setFocus();
  key(pinky, Qt::Key_End);
  QCOMPARE(pinky->value(), 100);
  QCOMPARE(h.lastWrites.count(), 3);
  for (int k = 12; k < 15; ++k)
  {
    QCOMPARE(h.state.left[k], m_presets[7].left[k]);
    QCOMPARE(h.state.right[k], QVector3D(0, 0, 0));
  }
}

void HandsTabTest::stateChipWords()
{
  HandsPanel panel(m_presets);
  Harness h(&panel);
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: open, nothing changed"));
  // a change without a pick: your own
  emit panel.grid()->cellChanged(3, 2, -20.0);
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: your own"));
  // a pick, then a change: changed from it
  emit panel.tiles()->picked(4);
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: Pointing"));
  emit panel.grid()->cellChanged(3, 2, -20.0);
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: changed from Pointing"));
  // a new file forgets the pick
  panel.forgetPicks();
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: your own"));
  // the hand set by hand to a preset's numbers matches it
  HandsPanel::State s = h.state;
  s.right = m_presets[5].right;
  h.set(s);
  QCOMPARE(panel.stateChip()->text(), QStringLiteral("Pose: Relaxed"));
  QCOMPARE(panel.tiles()->checked(), 5);
}

void HandsTabTest::resetHand()
{
  HandsPanel panel(m_presets);
  Harness h(&panel);
  QCOMPARE(panel.resetButton()->text(), QStringLiteral("Reset both hands"));
  QVERIFY(!panel.resetButton()->isEnabled());
  QCOMPARE(panel.resetButton()->statusTip(), QStringLiteral("This hand is not changed."));
  QCOMPARE(panel.resetButton()->toolTip(), QStringLiteral("This hand is not changed."));
  // in Mirror a change on the other hand is enough
  HandsPanel::State s = h.state;
  s.left[0] = QVector3D(10, 0, 0);
  h.set(s);
  QVERIFY(panel.resetButton()->isEnabled());
  QCOMPARE(panel.resetButton()->toolTip(), QStringLiteral("Back to the open hand."));
  QTest::mouseClick(panel.resetButton(), Qt::LeftButton);
  QCOMPARE(h.lastWrites, HandPose::resetWrites(true, true));
  QCOMPARE(h.lastMessage, QStringLiteral("Both hands back to open."));
  QVERIFY(!panel.resetButton()->isEnabled());
  // Right: only the right hand; a change on the left does not count
  s = h.state;
  s.sideMode = QLatin1Char('R');
  s.left[0] = QVector3D(10, 0, 0);
  h.set(s);
  QVERIFY(!panel.resetButton()->isEnabled());
  s.right[4] = QVector3D(0, 0, 30);
  h.set(s);
  QVERIFY(panel.resetButton()->isEnabled());
  QTest::mouseClick(panel.resetButton(), Qt::LeftButton);
  QCOMPARE(h.lastWrites, HandPose::resetWrites(false, true));
  QCOMPARE(h.lastMessage, QStringLiteral("Right hand back to open."));
  QCOMPARE(h.state.left[0], QVector3D(10, 0, 0));
}

void HandsTabTest::beginnerAndExpert()
{
  HandsPanel panel(m_presets);
  Harness h(&panel);
  HandsPanel::State s = h.state;
  s.expert = false;
  s.hints = true;
  h.set(s);
  QVERIFY(panel.gridSection()->isHidden());
  QVERIFY(!panel.expertLine()->isHidden());
  QCOMPARE(panel.expertLine()->text(), QStringLiteral("Expert mode shows every finger joint as numbers."));
  QVERIFY(!panel.hintLine()->parentWidget()->isHidden());
  QCOMPARE(panel.hintLine()->text(), QStringLiteral("Pick a ready-made pose. Then bend each finger below."));
  s.hints = false;
  h.set(s);
  QVERIFY(panel.expertLine()->isHidden());
  QVERIFY(panel.hintLine()->parentWidget()->isHidden());
  s.expert = true;
  h.set(s);
  QVERIFY(!panel.gridSection()->isHidden());
  QVERIFY(panel.gridHint()->isHidden());
  s.hints = true;
  h.set(s);
  QVERIFY(!panel.gridHint()->isHidden());
  QCOMPARE(panel.gridHint()->text(), QStringLiteral("Type a number, or drag a cell sideways. Page Up and Page Down change it by 5."));
  QVERIFY(panel.expertLine()->isHidden());
}

void HandsTabTest::gridKeyboard()
{
  HandsPanel panel(m_presets);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  Harness h(&panel);
  HandsPanel::State s = h.state;
  s.expert = true;
  s.right = m_presets[3].right; // Okay
  h.set(s);
  HandJointGrid* grid = panel.grid();
  grid->setFocus();
  QCOMPARE(grid->currentRow(), 0);
  QCOMPARE(grid->currentAxis(), 0);
  QCOMPARE(grid->cellText(0, 0), QStringLiteral("45°"));
  QCOMPARE(grid->cellText(3, 2), QStringLiteral("27°"));
  // moving to another row picks that joint
  key(grid, Qt::Key_Down);
  QCOMPARE(grid->currentRow(), 1);
  QCOMPARE(h.lastPicked, QStringLiteral("mHandThumb2Right"));
  QCOMPARE(grid->selectedRow(), 1);
  key(grid, Qt::Key_Right);
  key(grid, Qt::Key_Right);
  key(grid, Qt::Key_Right); // stays on Z
  QCOMPARE(grid->currentAxis(), 2);
  h.lastPicked.clear();
  key(grid, Qt::Key_Left);
  QCOMPARE(grid->currentAxis(), 1);
  QVERIFY(h.lastPicked.isEmpty()); // same row: no new pick
  // Page Up: +5, Shift + Page Down: -1, on the right hand and copied to the left (Mirror)
  const float before = h.state.right[1].y();
  key(grid, Qt::Key_PageUp);
  QCOMPARE(h.state.right[1].y(), std::round(before) + 5);
  QCOMPARE(h.state.left[1].y(), -(std::round(before) + 5));
  QCOMPARE(h.lastMessage, QStringLiteral("Thumb Mid, Y: %1 degrees.").arg(int(std::round(before) + 5)));
  key(grid, Qt::Key_PageDown, Qt::ShiftModifier);
  QCOMPARE(h.state.right[1].y(), std::round(before) + 4);
  key(grid, Qt::Key_PageDown);
  QCOMPARE(h.state.right[1].y(), std::round(before) - 1);
  QCOMPARE(h.lastWrites.count(), 2);
  QCOMPARE(h.lastWrites[0].joint, QStringLiteral("mHandThumb2Right"));
  QCOMPARE(h.lastWrites[1].joint, QStringLiteral("mHandThumb2Left"));
  // the number is cut to -180 to 180
  s = h.state;
  s.right[1].setY(178);
  h.set(s);
  key(grid, Qt::Key_PageUp);
  QCOMPARE(h.state.right[1].y(), 180.0f);
}

void HandsTabTest::gridTypingAndEsc()
{
  HandsPanel panel(m_presets);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  Harness h(&panel);
  HandsPanel::State s = h.state;
  s.expert = true;
  s.sideMode = QLatin1Char('L');
  h.set(s);
  HandJointGrid* grid = panel.grid();
  grid->setFocus();
  grid->setCurrent(4, 2);
  // typing a number starts editing; Enter keeps it
  QTest::keyClicks(grid, QStringLiteral("-"));
  QVERIFY(grid->editor());
  QTest::keyClicks(grid->editor(), QStringLiteral("40"));
  QCOMPARE(grid->editor()->text(), QStringLiteral("-40"));
  key(grid->editor(), Qt::Key_Return);
  QVERIFY(!grid->editor());
  QCOMPARE(h.state.left[4], QVector3D(0, 0, -40));
  QCOMPARE(h.state.right[4], QVector3D(0, 0, 0)); // Left: only this hand
  QCOMPARE(h.lastMessage, QStringLiteral("Index Finger Mid, Z: -40 degrees."));
  QCOMPARE(grid->cellText(4, 2), QStringLiteral("-40°"));
  // Enter (or F2) starts editing with the old number; Esc throws the edit away
  const int before = h.changes;
  key(grid, Qt::Key_F2);
  QVERIFY(grid->editor());
  QCOMPARE(grid->editor()->text(), QStringLiteral("-40"));
  QTest::keyClicks(grid->editor(), QStringLiteral("12"));
  key(grid->editor(), Qt::Key_Escape);
  QVERIFY(!grid->editor());
  QCOMPARE(h.changes, before);
  QCOMPARE(h.state.left[4], QVector3D(0, 0, -40));
  QVERIFY(grid->hasFocus());
  // a word is not a number
  key(grid, Qt::Key_Return);
  grid->editor()->setText(QStringLiteral("abc"));
  key(grid->editor(), Qt::Key_Return);
  QCOMPARE(h.changes, before);
  QCOMPARE(h.lastStatus, QStringLiteral("Type a number, for example -40."));
  // a double click edits; dragging sideways turns
  QTest::mouseDClick(grid, Qt::LeftButton, Qt::NoModifier, grid->cellRect(0, 0).center());
  QVERIFY(grid->editor());
  key(grid->editor(), Qt::Key_Escape);
  const QPoint start = grid->cellRect(2, 1).center();
  QTest::mousePress(grid, Qt::LeftButton, Qt::NoModifier, start);
  QMouseEvent move(QEvent::MouseMove, QPointF(start + QPoint(20, 0)), grid->mapToGlobal(QPointF(start + QPoint(20, 0))), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(grid, &move);
  QTest::mouseRelease(grid, Qt::LeftButton, Qt::NoModifier, start + QPoint(20, 0));
  QCOMPARE(h.state.left[2].y(), 10.0f); // 20 pixels: 10 degrees
  // a click on a cell of another row picks that joint
  QTest::mouseClick(grid, Qt::LeftButton, Qt::NoModifier, grid->cellRect(9, 0).center());
  QCOMPARE(h.lastPicked, QStringLiteral("mHandRing1Left"));
}

void HandsTabTest::tabOrder()
{
  HandsPanel panel(m_presets);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  Harness h(&panel);
  HandsPanel::State s = h.state;
  s.expert = true;
  s.right[0] = QVector3D(5, 0, 0); // Reset can be used
  h.set(s);
  QList<QWidget*> expected{panel.tiles()};
  for (int f = 0; f < HandPose::fingerCount; ++f) expected << panel.fingerSlider(f);
  expected << panel.resetButton() << panel.grid();
  panel.tiles()->setFocus();
  QCOMPARE(QApplication::focusWidget(), panel.tiles());
  for (int i = 1; i < expected.count(); ++i)
  {
    key(QApplication::focusWidget(), Qt::Key_Tab);
    QCOMPARE(QApplication::focusWidget(), expected[i]);
  }
  // the poses and the grid are one Tab stop each: their arrows never leave them
  panel.tiles()->setFocus();
  key(panel.tiles(), Qt::Key_Right);
  QCOMPARE(QApplication::focusWidget(), panel.tiles());
  panel.grid()->setFocus();
  key(panel.grid(), Qt::Key_Down);
  QCOMPARE(QApplication::focusWidget(), panel.grid());
}

void HandsTabTest::keySetText()
{
  QCOMPARE(HandsSpec::keySet(5, 1), QStringLiteral("Key set at frame 5."));
  QCOMPARE(HandsSpec::keySet(5, 30), QStringLiteral("Key set at frame 5 for 30 joints."));
}

void HandsTabTest::applySaveAndOpenAgain()
{
  BVH bvh;
  Animation anim(&bvh);
  anim.setNumberOfFrames(10);
  anim.setFrame(3);
  anim.setMirrored(true);
  // every finger joint is in the program's skeleton
  for (Side s : {Side::Left, Side::Right})
    for (int i = 0; i < HandPose::jointCount; ++i) QVERIFY2(HandPose::node(&anim, HandPose::jointName(s, i)), qPrintable(HandPose::jointName(s, i)));

  // a pick on both hands: 30 keys at frame 3, each hand its own numbers
  const HandPose::Preset& okay = m_presets[3];
  QCOMPARE(HandPose::apply(&anim, HandPose::presetWrites(okay, true, true)), 30);
  QVERIFY(anim.getMirrored()); // the animation's own Mirror is back on
  QCOMPARE(HandPose::read(&anim, Side::Left), okay.left);
  QCOMPARE(HandPose::read(&anim, Side::Right), okay.right);
  for (Side s : {Side::Left, Side::Right})
    for (int i = 0; i < HandPose::jointCount; ++i) QVERIFY(HandPose::node(&anim, HandPose::jointName(s, i))->isKeyframe(3));
  // a finger only on the left hand, without the animation's Mirror copying it
  const Hand bent = HandPose::withBend(okay.left, m_presets[7].left, 4, 35);
  QCOMPARE(HandPose::apply(&anim, HandPose::handWrites(Side::Left, bent, {12, 13, 14}, false)), 3);
  QCOMPARE(HandPose::read(&anim, Side::Right), okay.right);
  // another frame keeps its own pose
  anim.setFrame(0);
  QCOMPARE(HandPose::read(&anim, Side::Left), HandPose::openHand());
  anim.setFrame(3);

  // save as .dingle and open it again
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString file = dir.filePath(QStringLiteral("hands.dingle"));
  QString error;
  QVERIFY2(anim.saveBVH(file, &error), qPrintable(error));
  BVH bvh2;
  Animation opened(&bvh2, file);
  opened.setFrame(3);
  const Hand left = HandPose::read(&opened, Side::Left);
  const Hand right = HandPose::read(&opened, Side::Right);
  const Hand wantedLeft = HandPose::read(&anim, Side::Left);
  for (int i = 0; i < HandPose::jointCount; ++i)
  {
    QVERIFY2(sameTurn(left[i], wantedLeft[i]), qPrintable(QStringLiteral("%1: %2 not %3").arg(HandPose::jointName(Side::Left, i), vec(left[i]), vec(wantedLeft[i]))));
    QVERIFY2(sameTurn(right[i], okay.right[i]), qPrintable(QStringLiteral("%1: %2 not %3").arg(HandPose::jointName(Side::Right, i), vec(right[i]), vec(okay.right[i]))));
  }
  QCOMPARE(HandPose::match(right, m_presets, Side::Right), QStringLiteral("Okay"));
  QCOMPARE(HandPose::bend(left, m_presets[7].left, 4), 35);
  for (Side s : {Side::Left, Side::Right})
    for (int i = 0; i < HandPose::jointCount; ++i) QVERIFY(HandPose::node(&opened, HandPose::jointName(s, i))->isKeyframe(3));
}

void HandsTabTest::bothThemesAndTextSizes()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    HandsPanel panel(m_presets);
    HandsPanel::State s;
    s.open = true;
    s.expert = true;
    s.right = m_presets[3].right;
    panel.setState(s);
    const QImage image = UiRender::render(&panel);
    UiRender::save(image, QStringLiteral("handstab"), v);
    const QString why = UiRender::clipping(&panel).join(QLatin1Char('\n'));
    QVERIFY2(why.isEmpty(), qPrintable(v.name() + QStringLiteral(": ") + why));
    QCOMPARE(panel.tiles()->checked(), 3);
    // every tile name fits under its picture, also when it takes several lines
    // (the tiles are drawn by hand, so UiRender::clipping() does not see them)
    HandPresetTiles* tiles = panel.tiles();
    for (int i = 0; i < tiles->count(); ++i)
    {
      const QRect room = tiles->nameRect(i);
      const QRect need = QFontMetrics(tiles->nameFont(true)).boundingRect(QRect(0, 0, room.width(), 10000), Qt::AlignHCenter | Qt::TextWordWrap, m_presets[i].name);
      QVERIFY2(need.height() <= room.height() && need.width() <= room.width(),
               qPrintable(QStringLiteral("%1, %2: needs %3x%4, has %5x%6").arg(v.name(), m_presets[i].name).arg(need.width()).arg(need.height()).arg(room.width()).arg(room.height())));
    }
    // the grid's cells are wide enough for -180 degrees
    const QRect cell = panel.grid()->cellRect(0, 0);
    QVERIFY(QFontMetrics(panel.grid()->font()).horizontalAdvance(QStringLiteral("-180\u00b0")) < cell.width());
    QVERIFY(panel.grid()->cellRect(14, 2).right() < panel.grid()->width());
  }
}

QTEST_MAIN(HandsTabTest)
#include "tst_handstab.moc"
