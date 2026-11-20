/*
 * PinglyMotion - test of the play button logic (src/playback.h)
 *
 * Plays a short animation to its end three times in a row, the way the
 * window does: pressing play at the end must start again every time.
 */
#include <QtTest>

#include "animation.h"
#include "bvh.h"
#include "playback.h"

class PlaybackTest : public QObject
{
  Q_OBJECT
private slots:
  void startFrame();
  void playToEndThreeTimes();
  void playToEndFromLoopIn();
  void loopEndFollowsTheFrameCount();
};

void PlaybackTest::startFrame()
{
  QCOMPARE(Playback::startFrame(4,10,0),4);   // in the middle: stay
  QCOMPARE(Playback::startFrame(9,10,0),0);   // at the end: first frame
  QCOMPARE(Playback::startFrame(9,10,2),2);   // at the end: loop-in frame
  QCOMPARE(Playback::startFrame(9,10,9),0);   // loop-in on the last frame: first frame
  QCOMPARE(Playback::startFrame(0,1,0),0);    // one frame only
}

// one press of play followed by timer ticks until the one-shot play stops;
// returns the number of frames that were stepped
static int playOnce(Animation& anim)
{
  anim.setFrame(Playback::startFrame(anim.getFrame(),anim.getNumberOfFrames(),anim.getLoopInPoint()));
  anim.setPlaystate(PLAYSTATE_PLAYING);
  int steps=0;
  while(!Playback::atEnd(anim.getFrame(),anim.getNumberOfFrames()) && steps<1000)
  {
    anim.stepForward();
    steps++;
  }
  anim.setPlaystate(PLAYSTATE_STOPPED);
  return steps;
}

void PlaybackTest::playToEndThreeTimes()
{
  BVH bvh;
  Animation anim(&bvh);
  anim.setNumberOfFrames(10);
  for(int run=0;run<3;run++)
  {
    QCOMPARE(playOnce(anim),9);
    QCOMPARE(anim.getFrame(),9);
  }
}

void PlaybackTest::playToEndFromLoopIn()
{
  BVH bvh;
  Animation anim(&bvh);
  anim.setNumberOfFrames(10);
  anim.setLoopInPoint(2);
  for(int run=0;run<3;run++)
  {
    QCOMPARE(playOnce(anim),run==0 ? 9 : 7);
    QCOMPARE(anim.getFrame(),9);
  }
}

// issue #189: a loop that ends on the last frame keeps ending on the last
// frame when the frame count changes; a loop end in the middle stays where it
// is, and loop points past a new, shorter end move back onto the last frame
void PlaybackTest::loopEndFollowsTheFrameCount()
{
  BVH bvh;
  Animation anim(&bvh);
  anim.setNumberOfFrames(30);
  anim.setLoopInPoint(1);                   // frame 2 as the program shows it
  anim.setLoopOutPoint(29);                 // frame 30, the last
  anim.setNumberOfFrames(31);
  QCOMPARE(anim.getLoopOutPoint(),30);
  QCOMPARE(anim.getLoopInPoint(),1);
  anim.setNumberOfFrames(60);
  QCOMPARE(anim.getLoopOutPoint(),59);
  anim.setNumberOfFrames(20);
  QCOMPARE(anim.getLoopOutPoint(),19);

  anim.setLoopOutPoint(14);                 // in the middle: stays
  anim.setNumberOfFrames(40);
  QCOMPARE(anim.getLoopOutPoint(),14);
  anim.setNumberOfFrames(10);               // past the new end: the last frame
  QCOMPARE(anim.getLoopOutPoint(),9);
  QCOMPARE(anim.getLoopInPoint(),1);

  anim.setLoopInPoint(8);
  anim.setNumberOfFrames(5);
  QCOMPARE(anim.getLoopOutPoint(),4);
  QCOMPARE(anim.getLoopInPoint(),4);

  // playing the loop reaches the last frame
  anim.setNumberOfFrames(31);
  anim.setLoopInPoint(1);
  QCOMPARE(anim.getLoopOutPoint(),30);
}

QTEST_MAIN(PlaybackTest)
#include "tst_playback.moc"
