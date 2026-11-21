/*
 * PinglyMotion - test that the timeline panel scrolls to the row of the
 * selected joint (src/timelineview.cpp)
 */
#include <QtTest>
#include <QScrollArea>
#include <QScrollBar>

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
  void scrollsToSelectedRow();
  void unknownTrackKeepsScroll();
};

// a panel with a loaded skeleton, so the timeline can paint
struct Fixture
{
  BVH bvh;
  Animation anim;
  TimelineView view;
  Fixture() : anim(&bvh)
  {
    Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
    anim.setNumberOfFrames(20);
    view.getTimeline()->setAnimation(&anim);
    view.resize(600, 150);
  }
};

// the track on the lowest timeline row
static int lastTrack()
{
  int best = 0;
  for (int t = 0; t < Constants::numParts(); ++t)
    if (JointGroups::rowOfTrack(t) > JointGroups::rowOfTrack(best)) best = t;
  return best;
}

static QScrollBar* outerBar(TimelineView& v)
{
  // the outer area is the first scroll area below the panel
  return v.findChildren<QScrollArea*>().first()->verticalScrollBar();
}

void TimelineViewTest::scrollsToSelectedRow()
{
  Fixture f;
  TimelineView& v = f.view;
  v.show();
  QVERIFY(QTest::qWaitForWindowExposed(&v));
  QScrollBar* bar = outerBar(v);
  QVERIFY(bar->maximum() > 0);
  QCOMPARE(bar->value(), 0);

  const int last = lastTrack();
  QVERIFY(JointGroups::rowOfTrack(last) > 5);
  v.selectTrack(last);
  const int y = JointGroups::rowOfTrack(last) * Constants::lineHeight();
  QVERIFY(bar->value() <= y);
  QVERIFY(bar->value() + bar->pageStep() >= y + Constants::lineHeight());

  v.selectTrack(2);
  QVERIFY(bar->value() <= 2 * Constants::lineHeight());
}

void TimelineViewTest::unknownTrackKeepsScroll()
{
  Fixture f;
  TimelineView& v = f.view;
  v.show();
  QVERIFY(QTest::qWaitForWindowExposed(&v));
  v.selectTrack(lastTrack());
  QScrollBar* bar = outerBar(v);
  const int before = bar->value();
  v.selectTrack(-1);
  QCOMPARE(bar->value(), before);
}

QTEST_MAIN(TimelineViewTest)
#include "tst_timelineview.moc"
