/*
 * PinglyMotion - test that the timeline panel scrolls to the row of the
 * selected joint (src/timelineview.cpp)
 *
 * Both ways of selecting a joint end in TimelineView::selectTrack: the joint
 * list (PinglyMotionWindow::partChoice) and a click on a joint in the 3D view
 * (AnimationView::partClicked is connected to it). A click in the timeline
 * itself arrives through Timeline::trackClicked. The tests call that one
 * function and the signal, and check the exact result.
 */
#include <QtTest>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>

#include "animation.h"
#include "bvh.h"
#include "constants.h"
#include "jointgroups.h"
#include "timeline.h"
#include "timelineview.h"

class TimelineViewTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
  void listRouteShowsAndHighlightsRow();
  void timelineClickRouteShowsAndHighlightsRow();
  void farJumpsScrollBack();
  void handScrollThenFarJoint();
  void visibleRowDoesNotMove();
  void noSelectionOrUnknownDoesNotScroll();
};

// a panel with a loaded skeleton, so the timeline can paint; small, so it must scroll
struct Fixture
{
  BVH bvh;
  Animation anim;
  TimelineView view;
  Fixture() : anim(&bvh)
  {
    Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
    QStringList names;
    for (int t = 0; t < Constants::numParts(); ++t) names << anim.getPartName(t);
    JointGroups::setTracks(names);
    view.getTimeline()->setAnimation(&anim);
    anim.setNumberOfFrames(20);
    view.resize(600, 150);
    view.show();
    QTest::qWaitForWindowExposed(&view);
  }
  QScrollBar* bar() { return view.rowScroller()->verticalScrollBar(); }
  // the top of the row of a joint inside the scrolled content
  static int rowTop(int track) { return JointGroups::rowOfTrack(track) * Constants::lineHeight() + 2; }
  // the whole row lies inside the visible part
  bool rowVisible(int track)
  {
    const int y = rowTop(track);
    return bar()->value() <= y && y + Constants::lineHeight() <= bar()->value() + bar()->pageStep();
  }
  // the row is the highlighted one in the key frame area and in the joint names
  bool highlighted(int track)
  {
    return view.selectedTrack() == track && view.getTimeline()->selectedTrack() == track
        && view.tracks()->selectedTrackNumber() == track;
  }
  // the tracks with the first and the last row
  int firstTrack() { return JointGroups::trackOfRow(0); }
  int lastTrack() { return JointGroups::trackOfRow(JointGroups::rowCount() - 1); }
  int middleTrack() { return JointGroups::trackOfRow(JointGroups::rowCount() / 2); }
};

void TimelineViewTest::listRouteShowsAndHighlightsRow()
{
  Fixture f;
  QVERIFY(f.bar()->maximum() > 0);
  QCOMPARE(f.bar()->value(), 0);
  // every row, top to bottom and back: shown and highlighted each time
  for (int row = 0; row < JointGroups::rowCount(); ++row)
  {
    const int track = JointGroups::trackOfRow(row);
    f.view.selectTrack(track);
    QVERIFY2(f.rowVisible(track), qPrintable(QString("row %1 not visible").arg(row)));
    QVERIFY2(f.highlighted(track), qPrintable(QString("row %1 not highlighted").arg(row)));
  }
  for (int row = JointGroups::rowCount() - 1; row >= 0; --row)
  {
    const int track = JointGroups::trackOfRow(row);
    f.view.selectTrack(track);
    QVERIFY(f.rowVisible(track));
    QVERIFY(f.highlighted(track));
  }
}

void TimelineViewTest::timelineClickRouteShowsAndHighlightsRow()
{
  Fixture f;
  // a click on a key frame row reports its track the same way
  const int last = f.lastTrack();
  emit f.view.getTimeline()->trackClicked(last);
  QVERIFY(f.rowVisible(last));
  QVERIFY(f.highlighted(last));
}

void TimelineViewTest::farJumpsScrollBack()
{
  Fixture f;
  const int first = f.firstTrack();
  const int last = f.lastTrack();
  QVERIFY(JointGroups::rowOfTrack(last) > 20);
  f.view.selectTrack(last);
  QVERIFY(f.bar()->value() > 0);
  f.view.selectTrack(first);
  QVERIFY(f.rowVisible(first));
  QCOMPARE(f.bar()->value(), 0);
  f.view.selectTrack(last);
  QVERIFY(f.rowVisible(last));
  QVERIFY(f.bar()->value() > 0);
}

void TimelineViewTest::handScrollThenFarJoint()
{
  Fixture f;
  const int mid = f.middleTrack();
  // scroll to the bottom by hand, then select a joint far up
  f.bar()->setValue(f.bar()->maximum());
  f.view.selectTrack(f.firstTrack());
  QVERIFY(f.rowVisible(f.firstTrack()));
  QVERIFY(f.highlighted(f.firstTrack()));
  // scroll to the top by hand, then select a joint far down
  f.bar()->setValue(0);
  f.view.selectTrack(f.lastTrack());
  QVERIFY(f.rowVisible(f.lastTrack()));
  QVERIFY(f.highlighted(f.lastTrack()));
  // and one in the middle, from the bottom
  f.bar()->setValue(f.bar()->maximum());
  f.view.selectTrack(mid);
  QVERIFY(f.rowVisible(mid));
  QVERIFY(f.highlighted(mid));
}

void TimelineViewTest::visibleRowDoesNotMove()
{
  Fixture f;
  const int track = f.middleTrack();
  f.view.selectTrack(track);
  const int before = f.bar()->value();
  // the same joint again, and its neighbour that is in view too
  f.view.selectTrack(track);
  QCOMPARE(f.bar()->value(), before);
  int visibleOther = -1;
  for (int row = 0; row < JointGroups::rowCount(); ++row)
  {
    const int t = JointGroups::trackOfRow(row);
    if (t != track && f.rowVisible(t)) { visibleOther = t; break; }
  }
  QVERIFY(visibleOther >= 0);
  f.view.selectTrack(visibleOther);
  QCOMPARE(f.bar()->value(), before);
  QVERIFY(f.highlighted(visibleOther));
}

void TimelineViewTest::noSelectionOrUnknownDoesNotScroll()
{
  Fixture f;
  f.view.selectTrack(f.middleTrack());
  const int before = f.bar()->value();
  QVERIFY(before > 0);

  f.view.selectTrack(-1);
  QCOMPARE(f.bar()->value(), before);
  QCOMPARE(f.view.selectedTrack(), -1);
  QCOMPARE(f.view.getTimeline()->selectedTrack(), -1);

  f.view.selectTrack(Constants::numParts() + 100);
  QCOMPARE(f.bar()->value(), before);

  f.view.backgroundClicked();
  QCOMPARE(f.bar()->value(), before);
  QCOMPARE(f.view.selectedTrack(), -1);
}

QTEST_MAIN(TimelineViewTest)
#include "tst_timelineview.moc"
