/*
 * PinglyMotion - test that the timeline panel scrolls to the row of the
 * selected joint (src/timelineview.cpp)
 */
#include <QtTest>
#include <QScrollArea>
#include <QScrollBar>

#include "constants.h"
#include "timelineview.h"

class TimelineViewTest : public QObject
{
  Q_OBJECT
private slots:
  void scrollsToSelectedRow();
  void unknownTrackKeepsScroll();
};

static QScrollBar* outerBar(TimelineView& v)
{
  // the outer area is the first scroll area below the panel
  return v.findChildren<QScrollArea*>().first()->verticalScrollBar();
}

void TimelineViewTest::scrollsToSelectedRow()
{
  Constants::setNumParts(120);
  TimelineView v;
  v.resize(600, 300);
  v.show();
  QVERIFY(QTest::qWaitForWindowExposed(&v));
  QScrollBar* bar = outerBar(v);
  QVERIFY(bar->maximum() > 0);
  QCOMPARE(bar->value(), 0);

  const int last = 100;
  v.selectTrack(last);
  const int y = last * Constants::lineHeight();
  QVERIFY(bar->value() <= y);
  QVERIFY(bar->value() + bar->pageStep() >= y + Constants::lineHeight());

  v.selectTrack(2);
  QVERIFY(bar->value() <= 2 * Constants::lineHeight());
}

void TimelineViewTest::unknownTrackKeepsScroll()
{
  Constants::setNumParts(120);
  TimelineView v;
  v.resize(600, 300);
  v.show();
  QVERIFY(QTest::qWaitForWindowExposed(&v));
  v.selectTrack(100);
  QScrollBar* bar = outerBar(v);
  const int before = bar->value();
  v.selectTrack(-1);
  QCOMPARE(bar->value(), before);
}

QTEST_MAIN(TimelineViewTest)
#include "tst_timelineview.moc"
