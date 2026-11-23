/*
 * PinglyMotion - test of the timeline, UX phase 3a (behaviour and texts)
 *
 * Exact answers only (src/timelinerows.*, src/timeline.*, src/timelineview.*,
 * docs/UX_DECISIONS.md section 5): which joints have a row for which keys and
 * in which order, the selected joint's row (also without keys) and the scroll
 * to it, the body area and its name and token colour per row in the dark and
 * the light theme, Set key as the main button with its text and icon, a key
 * set at the right frame for the right joint with the pose it has, the
 * reasons Set key is greyed with, the empty states and when they show, the
 * keyboard, and the panel at 100 and 200 percent text with nothing clipped.
 * The texts the mockup has are also looked up in the mockup file itself.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStandardPaths>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"
#include "jointgroups.h"
#include "lucideicons.h"
#include "menuspec.h"
#include "theme.h"
#include "timeline.h"
#include "timelinelook.h"
#include "timelinerows.h"
#include "timelineview.h"
#include "transportlook.h"
#include "uirender.h"

using Theme::Token;

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

  QString tr2(const char* source) { return TimelineSpec::text(source); }
}

// a new animation (the T-pose: every joint unturned, one key at frame 0), 30 frames
struct Fixture
{
  BVH bvh;
  Animation anim;
  TimelineView view;
  explicit Fixture(int frames = 30) : anim(&bvh)
  {
    Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
    QStringList names;
    for (int t = 0; t < Constants::numParts(); ++t) names << anim.getPartName(t);
    JointGroups::setTracks(names);
    view.getTimeline()->setAnimation(&anim);
    anim.setNumberOfFrames(frames);
  }
  int track(const char* name) const
  {
    for (int t = 0; t < Constants::numParts(); ++t)
      if (anim.getPartName(t) == QLatin1String(name)) return t;
    return -1;
  }
  BVHNode* node(int track) { return anim.getNode(track); }
  // turns a joint at a frame, as a slider does (the turn is a key at once)
  void turn(int track, int frame, double x = 20)
  {
    anim.setFrame(frame);
    anim.setRotation(node(track), x, 0, 0);
  }
  const TimelineRows& rows() const { return view.rows(); }
  QVector<int> shown() const { return rows().tracks(); }
  QScrollBar* bar() { return view.rowScroller()->verticalScrollBar(); }
  static int rowTop(int row) { return row * Constants::lineHeight() + 2; }
  bool rowVisible(int track)
  {
    const int row = rows().rowOfTrack(track);
    if (row < 0) return false;
    const int y = rowTop(row);
    return bar()->value() <= y && y + Constants::lineHeight() <= bar()->value() + bar()->pageStep();
  }
};

class TimelineRowsTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
  void newAnimationHasNoRows();
  void rowsOnlyForChangedJointsInSkeletonOrder();
  void selectedJointHasRowAtTheEnd();
  void keyAfterFirstFrameCountsAsChanged();
  void importedJointKeyedOnEveryFrameAtRestHasNoRow();
  void avatarPositionChangesByPlace();
  void deletingTheKeyTakesTheRowAway();
  void areasAndTheirNames();
  void rowColoursAreAreaTokens();
  void setKeyAddsKeyAtFrameWithPose();
  void setKeyNeverTakesAKeyAway();
  void setKeyMessages();
  void setKeyBlockedReasons();
  void setKeyButtonIsTheMainButton();
  void emptyStateNoKeys();
  void emptyStateNoAnimation();
  void firstKeyButtonAsksForAKeyAndIsGreyedWithReason();
  void scrollsToSelectedJointWithoutKeys();
  void scrollsToSelectedChangedJoint();
  void keyboardUpDownMovesBetweenRows();
  void keyboardPressesSetTheFirstKey();
  void namesAndAreaNamesFitAtLargeText();
  void textsAreShortAndInTheMockup();
};

void TimelineRowsTest::newAnimationHasNoRows()
{
  Fixture f;
  QCOMPARE(f.rows().rowCount(), 0);
  QVERIFY(!f.rows().anyChanged());
  // no rows: the key frame area has no height
  QCOMPARE(f.view.getTimeline()->height(), 0);
  QCOMPARE(f.view.getTimeline()->width(), 30 * Constants::keyWidth());
  // every joint is in the start pose
  for (int t = 0; t < Constants::numParts(); ++t)
    QVERIFY2(!TimelineRows::jointChanged(&f.anim, t), qPrintable(f.anim.getPartName(t)));
}

void TimelineRowsTest::rowsOnlyForChangedJointsInSkeletonOrder()
{
  Fixture f;
  const int head = f.track("head");
  const int chest = f.track("chest");
  const int hand = f.track("mHandIndex1Left");
  QVERIFY(chest >= 0 && head > chest && hand > head);
  f.turn(head, 5);
  QCOMPARE(f.shown(), QVector<int>({head}));
  // a joint higher up in the skeleton comes first, whatever order they were turned in
  f.turn(hand, 9);
  f.turn(chest, 3);
  QCOMPARE(f.shown(), QVector<int>({chest, head, hand}));
  QCOMPARE(f.rows().rowOfTrack(chest), 0);
  QCOMPARE(f.rows().rowOfTrack(head), 1);
  QCOMPARE(f.rows().rowOfTrack(hand), 2);
  QCOMPARE(f.rows().trackOfRow(2), hand);
  QCOMPARE(f.rows().trackOfRow(3), -1);
  QCOMPARE(f.rows().rowOfTrack(f.track("neck")), -1);
  QVERIFY(f.rows().anyChanged());
  // the key frame area is as high as its three rows
  f.view.resize(600, 300);
  f.view.show();
  QTest::qWaitForWindowExposed(&f.view);
  QCOMPARE(f.view.getTimeline()->height(), 3 * Constants::lineHeight());
  // a click in the second row picks the head
  QSignalSpy clicked(f.view.getTimeline(), &Timeline::trackClicked);
  QTest::mouseClick(f.view.getTimeline(), Qt::LeftButton, {}, QPoint(3 * Constants::keyWidth(), Constants::lineHeight() + 2));
  QCOMPARE(clicked.count(), 1);
  QCOMPARE(clicked.first().first().toInt(), head);
  QCOMPARE(f.view.selectedTrack(), head);
}

void TimelineRowsTest::selectedJointHasRowAtTheEnd()
{
  Fixture f;
  const int chest = f.track("chest");
  const int neck = f.track("neck");
  const int head = f.track("head");
  f.turn(head, 5);
  // the neck has not changed: picked, it gets a row after the changed joints
  f.view.selectTrack(neck);
  QCOMPARE(f.shown(), QVector<int>({head, neck}));
  QCOMPARE(f.view.getTimeline()->selectedTrack(), neck);
  QCOMPARE(f.view.tracks()->selectedTrackNumber(), neck);
  // picking a changed joint takes the neck's row away again
  f.view.selectTrack(head);
  QCOMPARE(f.shown(), QVector<int>({head}));
  // the chest, picked with nothing changed in it, then turned: it moves to its place
  f.view.selectTrack(chest);
  QCOMPARE(f.shown(), QVector<int>({head, chest}));
  f.turn(chest, 2);
  QCOMPARE(f.shown(), QVector<int>({chest, head}));
  // nothing picked: only the changed joints
  f.view.backgroundClicked();
  QCOMPARE(f.shown(), QVector<int>({chest, head}));
  f.view.selectTrack(neck);
  f.view.selectTrack(-1);
  QCOMPARE(f.shown(), QVector<int>({chest, head}));
  // End Sites never have a row
  int site = -1;
  for (int t = 0; t < Constants::numParts(); ++t)
    if (f.anim.getPartName(t) == QLatin1String("Site")) { site = t; break; }
  QVERIFY(site > 0);
  f.view.selectTrack(site);
  QCOMPARE(f.rows().rowOfTrack(site), -1);
}

void TimelineRowsTest::keyAfterFirstFrameCountsAsChanged()
{
  Fixture f;
  const int head = f.track("head");
  // a key at frame 10 that keeps the start pose (Set key without turning)
  f.anim.setFrame(10);
  QVERIFY(TimelineSpec::addKey(&f.anim, f.node(head)));
  QVERIFY(TimelineRows::jointChanged(&f.anim, head));
  QCOMPARE(f.shown(), QVector<int>({head}));
  // a key only at frame 0 in the start pose has not changed
  QVERIFY(!TimelineRows::jointChanged(&f.anim, f.track("neck")));
  // a turn at frame 0 has
  f.turn(f.track("neck"), 0);
  QVERIFY(TimelineRows::jointChanged(&f.anim, f.track("neck")));
}

void TimelineRowsTest::importedJointKeyedOnEveryFrameAtRestHasNoRow()
{
  Fixture f(5);
  const int head = f.track("head");
  const int neck = f.track("neck");
  // keys on every frame, as an imported file has: the head never turns, the neck does at frame 3
  for (int frame = 0; frame < 5; ++frame)
  {
    f.anim.setFrame(frame);
    f.anim.addKeyFrame(f.node(head));
    f.anim.setRotation(f.node(neck), frame == 3 ? 15 : 0, 0, 0);
  }
  QCOMPARE(f.node(head)->numKeyframes(), 5);
  QVERIFY(!TimelineRows::jointChanged(&f.anim, head));
  QVERIFY(TimelineRows::jointChanged(&f.anim, neck));
  QCOMPARE(f.shown(), QVector<int>({neck}));
  // the animation made longer: the head's keys are now set by hand, so it has changed
  f.anim.setNumberOfFrames(8);
  QVERIFY(TimelineRows::jointChanged(&f.anim, head));
  QVERIFY(neck < head);
  QCOMPARE(f.shown(), QVector<int>({neck, head}));
}

void TimelineRowsTest::avatarPositionChangesByPlace()
{
  {
    Fixture f;
    QCOMPARE(f.anim.getPartName(0), QStringLiteral("position"));
    QVERIFY(!TimelineRows::jointChanged(&f.anim, 0));
    // the Avatar Position starts in the middle of the floor at the figure's height
    f.anim.setFrame(0);
    const QVector3D start = f.anim.getPosition();
    QVERIFY(start.y() > 1);
    QVERIFY(qAbs(start.x()) < 0.001 && qAbs(start.z()) < 0.001);
    // moved sideways at frame 0: changed
    f.anim.setPosition(start.x() + 4, start.y(), start.z());
    QVERIFY(TimelineRows::jointChanged(&f.anim, 0));
    QCOMPARE(f.shown(), QVector<int>({0}));
    QCOMPARE(f.rows().areaOfTrack(0), int(JointGroups::BodyTab));
  }
  {
    Fixture f;
    // up at frame 9: another place than the first key
    f.anim.setFrame(9);
    const QVector3D start = f.anim.getPosition();
    f.anim.setPosition(start.x(), start.y() + 3, start.z());
    QVERIFY(TimelineRows::jointChanged(&f.anim, 0));
    QCOMPARE(f.shown(), QVector<int>({0}));
  }
}

void TimelineRowsTest::deletingTheKeyTakesTheRowAway()
{
  Fixture f;
  const int head = f.track("head");
  f.turn(head, 6);
  QCOMPARE(f.shown(), QVector<int>({head}));
  f.anim.deleteKeyFrame(f.node(head), 6);
  QCOMPARE(f.shown(), QVector<int>());
  QVERIFY(!f.rows().anyChanged());
}

void TimelineRowsTest::areasAndTheirNames()
{
  // the Joints panel's tabs (JointGroups::tabOf); the Avatar Position is on Body
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("position")), int(JointGroups::BodyTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("hip")), int(JointGroups::BodyTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("head")), int(JointGroups::BodyTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mFaceJaw")), int(JointGroups::FaceTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mSkull")), int(JointGroups::FaceTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mHandIndex1Left")), int(JointGroups::HandsTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mWing1Left")), int(JointGroups::OtherTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mGroin")), int(JointGroups::OtherTab));
  QCOMPARE(TimelineSpec::areaName(JointGroups::BodyTab), QStringLiteral("Body"));
  QCOMPARE(TimelineSpec::areaName(JointGroups::FaceTab), QStringLiteral("Face"));
  QCOMPARE(TimelineSpec::areaName(JointGroups::HandsTab), QStringLiteral("Hands"));
  QCOMPARE(TimelineSpec::areaName(JointGroups::OtherTab), QStringLiteral("Other"));
  QCOMPARE(TimelineSpec::areaName(99), QStringLiteral("Other"));
  // the colours are the four area tokens of docs/design_tokens.json
  for (bool dark : {true, false})
  {
    QCOMPARE(TimelineLook::area(JointGroups::BodyTab, dark).name(), token("area_body", dark).toLower());
    QCOMPARE(TimelineLook::area(JointGroups::FaceTab, dark).name(), token("area_face", dark).toLower());
    QCOMPARE(TimelineLook::area(JointGroups::HandsTab, dark).name(), token("area_hands", dark).toLower());
    QCOMPARE(TimelineLook::area(JointGroups::OtherTab, dark).name(), token("area_other", dark).toLower());
  }
}

void TimelineRowsTest::rowColoursAreAreaTokens()
{
  for (bool dark : {true, false})
  {
    UiRender::Scope scope({dark, 100});
    Fixture f;
    const int head = f.track("head");             // Body
    const int jaw = f.track("mFaceJaw");          // Face
    const int hand = f.track("mHandIndex1Left");  // Hands
    QVERIFY(jaw >= 0);
    f.turn(head, 5);
    f.turn(jaw, 5);
    f.turn(hand, 5);
    f.anim.setFrame(0);
    f.view.selectTrack(hand);
    f.view.resize(700, 300);
    const QImage image = UiRender::render(&f.view);
    UiRender::save(image, QStringLiteral("timeline_areas"), {dark, 100});
    TimelineTracks* names = f.view.tracks();
    Timeline* keys = f.view.getTimeline();
    struct Row { int track; int area; bool selected; QString name; };
    const QList<Row> rows = {{head, JointGroups::BodyTab, false, "head"}, {jaw, JointGroups::FaceTab, false, "jaw"},
                             {hand, JointGroups::HandsTab, true, "hand"}};
    for (const Row& r : rows)
    {
      const int row = f.rows().rowOfTrack(r.track);
      QVERIFY(row >= 0);
      const QString what = QStringLiteral("%1 %2").arg(dark ? "dark" : "light", r.name);
      const QColor area = TimelineLook::area(r.area, dark);
      // the stripe at the left of the name: the area token itself
      QString why = UiRender::expectColour(image, &f.view, names, QPoint(1, row * Constants::lineHeight() + 2 + 1), area, what + " stripe");
      QVERIFY2(why.isEmpty(), qPrintable(why));
      // the name's background: surface with 9 percent of the area colour, selection when selected
      const QColor nameBack = r.selected ? Theme::color(Token::Selection, dark)
                                         : Theme::mix(Theme::color(Token::Surface, dark), area, 0.09);
      QCOMPARE(TimelineLook::nameRow(r.selected, r.area, dark), nameBack);
      why = UiRender::expectColour(image, &f.view, names, QPoint(names->width() - 2, row * Constants::lineHeight() + 2), nameBack, what + " name row", 1);
      QVERIFY2(why.isEmpty(), qPrintable(why));
      // the key frame row: surface with 5 percent of the area colour (first block of ten frames)
      const QColor rowBack = r.selected ? Theme::color(Token::Selection, dark)
                                        : Theme::mix(Theme::color(Token::Surface, dark), area, 0.05);
      QCOMPARE(TimelineLook::row(r.selected, r.area, dark), rowBack);
      why = UiRender::expectColour(image, &f.view, keys, QPoint(7 * Constants::keyWidth(), row * Constants::lineHeight() + 1), rowBack, what + " key row", 1);
      QVERIFY2(why.isEmpty(), qPrintable(why));
      // the second block of ten frames is mixed toward the background
      why = UiRender::expectColour(image, &f.view, keys, QPoint(12 * Constants::keyWidth(), row * Constants::lineHeight() + 1),
                                   Theme::mix(rowBack, Theme::color(Token::Background, dark), 0.35), what + " key row, second block", 1);
      QVERIFY2(why.isEmpty(), qPrintable(why));
      // the key: the area colour (on_accent on the selected row)
      QCOMPARE(TimelineLook::key(r.selected, r.area, dark), r.selected ? Theme::color(Token::OnAccent, dark) : area);
      // the area's name in text_muted (on_accent when selected)
      QCOMPARE(TimelineLook::areaName(r.selected, dark), Theme::color(r.selected ? Token::OnAccent : Token::TextMuted, dark));
    }
    // the key at frame 5 of the head is drawn in the Body colour
    const int headRow = f.rows().rowOfTrack(head);
    const QPoint keyCentre(5 * Constants::keyWidth() + Constants::keyWidth() / 2, headRow * Constants::lineHeight() + Constants::lineHeight() / 2);
    const QString why = UiRender::expectColour(image, &f.view, keys, keyCentre, TimelineLook::area(JointGroups::BodyTab, dark), "head key", 2);
    QVERIFY2(why.isEmpty(), qPrintable(why));
  }
}

void TimelineRowsTest::setKeyAddsKeyAtFrameWithPose()
{
  Fixture f;
  const int head = f.track("head");
  f.turn(head, 4, 30);
  // frame 12: between keys the head holds 30 degrees; Set key keeps that pose
  f.anim.setFrame(12);
  QVERIFY(!f.node(head)->isKeyframe(12));
  QVERIFY(TimelineSpec::addKey(&f.anim, f.node(head)));
  QVERIFY(f.node(head)->isKeyframe(12));
  QCOMPARE(f.node(head)->numKeyframes(), 3);
  QVERIFY(qAbs(f.node(head)->frameData(12).rotation().x() - 30) < 0.001);
  // only that joint got a key
  QVERIFY(!f.node(f.track("neck"))->isKeyframe(12));
  QVERIFY(!f.node(f.track("chest"))->isKeyframe(12));
  // the timeline knows the key
  QVERIFY(f.anim.isKeyFrame(head, 12));
}

void TimelineRowsTest::setKeyNeverTakesAKeyAway()
{
  Fixture f;
  const int head = f.track("head");
  f.turn(head, 8, 12);
  f.anim.setFrame(8);
  QVERIFY(!TimelineSpec::addKey(&f.anim, f.node(head)));
  QVERIFY(f.node(head)->isKeyframe(8));
  QCOMPARE(f.node(head)->numKeyframes(), 2);
  QVERIFY(qAbs(f.node(head)->frameData(8).rotation().x() - 12) < 0.001);
  // frame 0 always has a key
  f.anim.setFrame(0);
  QVERIFY(!TimelineSpec::addKey(&f.anim, f.node(f.track("neck"))));
  QCOMPARE(f.node(f.track("neck"))->numKeyframes(), 1);
  // nothing to key
  QVERIFY(!TimelineSpec::addKey(&f.anim, nullptr));
  QVERIFY(!TimelineSpec::addKey(nullptr, f.node(head)));
}

void TimelineRowsTest::setKeyMessages()
{
  // frames are counted from 1 on screen
  QCOMPARE(TimelineSpec::setKeyMessage(true, 11, QStringLiteral("Head")), QStringLiteral("Key set at frame 12 for Head."));
  QCOMPARE(TimelineSpec::setKeyMessage(true, 0, QStringLiteral("Left Shoulder")), QStringLiteral("Key set at frame 1 for Left Shoulder."));
  QCOMPARE(TimelineSpec::setKeyMessage(false, 5, QStringLiteral("Head")), QStringLiteral("This frame has a key already. Turning the joint changes it."));
}

void TimelineRowsTest::setKeyBlockedReasons()
{
  QCOMPARE(TimelineSpec::setKeyBlocked(true, true, false, false), QString());
  QCOMPARE(TimelineSpec::setKeyBlocked(false, true, false, false), QStringLiteral("Open an animation first."));
  QCOMPARE(TimelineSpec::setKeyBlocked(false, false, true, true), QStringLiteral("Open an animation first."));
  QCOMPARE(TimelineSpec::setKeyBlocked(true, false, true, true), QStringLiteral("Pick a joint first. Click it on the figure or in the list."));
  QCOMPARE(TimelineSpec::setKeyBlocked(true, true, true, true), QStringLiteral("Stop playing first."));
  QCOMPARE(TimelineSpec::setKeyBlocked(true, true, false, true), QStringLiteral("The first frame is locked. Turn off Lock the first frame to set a key."));
}

void TimelineRowsTest::setKeyButtonIsTheMainButton()
{
  // Set key (the designer file's keyframeButton) and Set the first key: main buttons with diamond-plus
  for (const char* name : {"keyframeButton", "timelineFirstKeyButton"})
  {
    QPushButton button;
    button.setObjectName(QLatin1String(name));
    QCOMPARE(TransportLook::kind(&button), TransportLook::Kind::Main);
    QCOMPARE(TransportLook::iconName(&button), QStringLiteral("diamond-plus"));
  }
  QCOMPARE(tr2(TimelineSpec::setKey), QStringLiteral("Set key"));
  QVERIFY(!LucideIcons::path(QStringLiteral("diamond-plus")).isEmpty());
  // drawn in the accent colour with on_accent text, in both themes
  for (bool dark : {true, false})
  {
    const TransportLook::Colours c = TransportLook::colours(TransportLook::Kind::Main, TransportLook::State::Normal, dark);
    QCOMPARE(c.fill, Theme::color(Token::Accent, dark));
    QCOMPARE(c.text, Theme::color(Token::OnAccent, dark));
  }
  // the timeline's own Set the first key button really is one
  Fixture f;
  QPushButton* first = f.view.firstKeyButton();
  QCOMPARE(first->objectName(), QStringLiteral("timelineFirstKeyButton"));
  QCOMPARE(first->text(), QStringLiteral("Set the first key"));
  QCoreApplication::processEvents();
  QVERIFY(!first->icon().isNull());
}

void TimelineRowsTest::emptyStateNoKeys()
{
  Fixture f;
  f.view.resize(600, 300);
  f.view.show();
  QTest::qWaitForWindowExposed(&f.view);
  QVERIFY(f.view.noKeys()->isVisible());
  QVERIFY(!f.view.noAnimationLine()->isVisible());
  QCOMPARE(f.view.noKeysTitle()->text(), QStringLiteral("No keys yet"));
  QCOMPARE(f.view.noKeysLine()->text(), QStringLiteral("Set a key to save your first pose."));
  QCOMPARE(f.view.firstKeyButton()->text(), QStringLiteral("Set the first key"));
  // picking a joint gives it a row; still no keys
  const int head = f.track("head");
  f.view.selectTrack(head);
  QCOMPARE(f.shown(), QVector<int>({head}));
  QVERIFY(f.view.noKeys()->isVisible());
  // a turn: the empty state goes
  f.turn(head, 3);
  QVERIFY(!f.view.noKeys()->isVisible());
  // the key taken away again: it comes back
  f.anim.deleteKeyFrame(f.node(head), 3);
  QVERIFY(f.view.noKeys()->isVisible());
}

void TimelineRowsTest::emptyStateNoAnimation()
{
  Fixture f;
  f.view.resize(600, 300);
  f.view.show();
  QTest::qWaitForWindowExposed(&f.view);
  f.turn(f.track("head"), 3);
  f.view.setNoAnimation(true);
  QVERIFY(f.view.noAnimationLine()->isVisible());
  QCOMPARE(f.view.noAnimationLine()->text(), QStringLiteral("The timeline shows your keys over time. Open an animation first."));
  QVERIFY(!f.view.noKeys()->isVisible());
  QVERIFY(!f.view.getTimeline()->isVisible());
  QVERIFY(!f.view.tracks()->isVisible());
  f.view.setNoAnimation(false);
  QVERIFY(!f.view.noAnimationLine()->isVisible());
  QVERIFY(f.view.getTimeline()->isVisible());
  QVERIFY(!f.view.noKeys()->isVisible());
}

void TimelineRowsTest::firstKeyButtonAsksForAKeyAndIsGreyedWithReason()
{
  Fixture f;
  f.view.resize(600, 300);
  f.view.show();
  QTest::qWaitForWindowExposed(&f.view);
  QSignalSpy asked(&f.view, &TimelineView::setKeyRequested);
  QTest::mouseClick(f.view.firstKeyButton(), Qt::LeftButton);
  QCOMPARE(asked.count(), 1);
  const QString why = TimelineSpec::setKeyBlocked(true, false, false, false);
  f.view.setFirstKeyBlocked(why);
  QVERIFY(!f.view.firstKeyButton()->isEnabled());
  QCOMPARE(f.view.firstKeyButton()->toolTip(), why);
  QCOMPARE(f.view.firstKeyButton()->statusTip(), why);
  QTest::mouseClick(f.view.firstKeyButton(), Qt::LeftButton);
  QCOMPARE(asked.count(), 1);
  // greyed: drawn pale (text_disabled on surface_raised)
  const TransportLook::Colours pale = TransportLook::colours(TransportLook::Kind::Main, TransportLook::state(f.view.firstKeyButton()), true);
  QCOMPARE(pale.text, Theme::color(Token::TextDisabled, true));
  f.view.setFirstKeyBlocked(QString());
  QVERIFY(f.view.firstKeyButton()->isEnabled());
  QCOMPARE(f.view.firstKeyButton()->statusTip(), QString());
}

void TimelineRowsTest::scrollsToSelectedJointWithoutKeys()
{
  Fixture f;
  // many changed joints, a low panel: the rows must scroll
  QVector<int> changed;
  for (int t = 1; t < Constants::numParts() && changed.size() < 40; ++t)
  {
    const QString name = f.anim.getPartName(t);
    if (name == QLatin1String("Site") || JointGroups::rowOfTrack(t) < 0) continue;
    f.turn(t, 5);
    changed << t;
  }
  QCOMPARE(f.rows().rowCount(), 40);
  f.view.resize(600, 150);
  f.view.show();
  QTest::qWaitForWindowExposed(&f.view);
  QVERIFY(f.bar()->maximum() > 0);
  f.view.selectTrack(changed.first());
  QCOMPARE(f.bar()->value(), 0);
  // a joint without keys: its row comes at the end, and the timeline scrolls to it
  int unchanged = -1;
  for (int t = Constants::numParts() - 1; t > 0; --t)
    if (f.anim.getPartName(t) != QLatin1String("Site") && JointGroups::rowOfTrack(t) >= 0 && !f.rows().changed(t)) { unchanged = t; break; }
  QVERIFY(unchanged > 0);
  f.view.selectTrack(unchanged);
  QCOMPARE(f.rows().rowOfTrack(unchanged), 40);
  QVERIFY2(f.rowVisible(unchanged), qPrintable(QString("value %1 page %2 max %3").arg(f.bar()->value()).arg(f.bar()->pageStep()).arg(f.bar()->maximum())));
  QVERIFY(f.bar()->value() > 0);
  QCOMPARE(f.view.tracks()->selectedTrackNumber(), unchanged);
  // back to the first joint: scrolled to the top, the joint without keys lost its row
  f.view.selectTrack(changed.first());
  QCOMPARE(f.bar()->value(), 0);
  QCOMPARE(f.rows().rowOfTrack(unchanged), -1);
  // and once more, the same way as the first time (the owner's test note: the same for joints without animation)
  f.view.selectTrack(unchanged);
  QVERIFY(f.rowVisible(unchanged));
}

void TimelineRowsTest::scrollsToSelectedChangedJoint()
{
  Fixture f;
  QVector<int> changed;
  for (int t = 1; t < Constants::numParts() && changed.size() < 40; ++t)
  {
    if (f.anim.getPartName(t) == QLatin1String("Site") || JointGroups::rowOfTrack(t) < 0) continue;
    f.turn(t, 5);
    changed << t;
  }
  f.view.resize(600, 150);
  f.view.show();
  QTest::qWaitForWindowExposed(&f.view);
  for (int i : {39, 0, 20, 39, 1})
  {
    f.view.selectTrack(changed[i]);
    QVERIFY2(f.rowVisible(changed[i]), qPrintable(QString::number(i)));
  }
  // a row already in view does not move the timeline
  const int before = f.bar()->value();
  f.view.selectTrack(changed[2]);
  QVERIFY(f.rowVisible(changed[2]));
  QCOMPARE(f.bar()->value(), before);
}

void TimelineRowsTest::keyboardUpDownMovesBetweenRows()
{
  Fixture f;
  const int chest = f.track("chest");
  const int neck = f.track("neck");
  const int head = f.track("head");
  f.turn(chest, 2);
  f.turn(neck, 2);
  f.turn(head, 2);
  f.view.resize(600, 300);
  f.view.show();
  QTest::qWaitForWindowExposed(&f.view);
  Timeline* timeline = f.view.getTimeline();
  timeline->setFocus();
  QSignalSpy clicked(timeline, &Timeline::trackClicked);
  // nothing picked: Down picks the first row, Up the last
  QTest::keyClick(timeline, Qt::Key_Down);
  QCOMPARE(f.view.selectedTrack(), chest);
  QTest::keyClick(timeline, Qt::Key_Down);
  QCOMPARE(f.view.selectedTrack(), neck);
  QTest::keyClick(timeline, Qt::Key_Down);
  QCOMPARE(f.view.selectedTrack(), head);
  // at the end it stays
  QTest::keyClick(timeline, Qt::Key_Down);
  QCOMPARE(f.view.selectedTrack(), head);
  QTest::keyClick(timeline, Qt::Key_Up);
  QCOMPARE(f.view.selectedTrack(), neck);
  // each move is reported as a click on the row (the stop at the end is not)
  QCOMPARE(clicked.count(), 4);
  QCOMPARE(clicked.last().first().toInt(), neck);
  f.view.backgroundClicked();
  QTest::keyClick(timeline, Qt::Key_Up);
  QCOMPARE(f.view.selectedTrack(), head);
}

void TimelineRowsTest::keyboardPressesSetTheFirstKey()
{
  Fixture f;
  f.view.resize(600, 300);
  f.view.show();
  QTest::qWaitForWindowExposed(&f.view);
  QSignalSpy asked(&f.view, &TimelineView::setKeyRequested);
  QPushButton* button = f.view.firstKeyButton();
  QCOMPARE(button->focusPolicy() & Qt::TabFocus, Qt::TabFocus);
  button->setFocus(Qt::TabFocusReason);
  QTest::keyClick(button, Qt::Key_Space);
  QCOMPARE(asked.count(), 1);
}

void TimelineRowsTest::namesAndAreaNamesFitAtLargeText()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    if (v.percent == 150) continue;
    UiRender::Scope scope(v);
    Fixture f;
    // long friendly names, as the window gives them
    f.view.setNameOf([](const QString& name) { return name == QLatin1String("mHandIndex1Left") ? QStringLiteral("Left Hand: Index Finger Base") : name; });
    const int hand = f.track("mHandIndex1Left");
    f.turn(hand, 3);
    f.turn(f.track("mFaceJaw"), 3);
    f.view.resize(800, 300);
    const QImage image = UiRender::render(&f.view);
    UiRender::save(image, QStringLiteral("timeline_rows"), v);
    TimelineTracks* names = f.view.tracks();
    QCOMPARE(names->shownName(hand), QStringLiteral("Left Hand: Index Finger Base"));
    // the name column is as wide as the widest name with its area name
    const QFontMetrics metrics(names->font());
    const int need = TimelineLook::areaStripeWidth() + 3 * Theme::Size::spacingS
                     + metrics.horizontalAdvance(QStringLiteral("Left Hand: Index Finger Base")) + metrics.horizontalAdvance(QStringLiteral("Hands"));
    QVERIFY2(names->width() >= need, qPrintable(QString("%1: %2 < %3").arg(v.name()).arg(names->width()).arg(need)));
    QVERIFY(names->width() >= Constants::leftStrut());
    // the empty state is not shown here; with no keys it must not be clipped either
    const QStringList clipped = UiRender::clipping(&f.view);
    QVERIFY2(clipped.isEmpty(), qPrintable(v.name() + ": " + clipped.join("; ")));
  }
  for (const UiRender::Variant& v : UiRender::variants())
  {
    if (v.percent == 150) continue;
    UiRender::Scope scope(v);
    Fixture f;
    f.view.resize(300, 300);
    const QImage image = UiRender::render(&f.view);
    UiRender::save(image, QStringLiteral("timeline_nokeys"), v);
    QVERIFY(f.view.noKeys()->isVisible());
    const QStringList clipped = UiRender::clipping(f.view.noKeys());
    QVERIFY2(clipped.isEmpty(), qPrintable(v.name() + ": " + clipped.join("; ")));
    QVERIFY(f.view.firstKeyButton()->width() >= f.view.firstKeyButton()->minimumSizeHint().width());
  }
}

void TimelineRowsTest::textsAreShortAndInTheMockup()
{
  const QString page = mockup();
  QVERIFY(!page.isEmpty());
  // the mockup's own words
  for (const char* text : {TimelineSpec::noKeysTitle, TimelineSpec::noKeysLine, TimelineSpec::firstKey, TimelineSpec::setKey, TimelineSpec::noAnimation})
    QVERIFY2(page.contains(QLatin1String(text)), text);
  QVERIFY(page.contains(QStringLiteral("\"Key set at frame \"")));
  for (const char* area : TimelineSpec::areas)
    QVERIFY2(page.contains(QStringLiteral("name: \"%1\"").arg(QLatin1String(area))), area);
  // every sentence at most 12 words
  for (const char* text : {TimelineSpec::noKeysTitle, TimelineSpec::noKeysLine, TimelineSpec::firstKey, TimelineSpec::setKey,
                           TimelineSpec::noAnimation, TimelineSpec::keySet, TimelineSpec::keyThere})
    for (const QString& sentence : QString::fromLatin1(text).split(QStringLiteral(". ")))
      QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, text);
}

QTEST_MAIN(TimelineRowsTest)
#include "tst_timelinerows.moc"
