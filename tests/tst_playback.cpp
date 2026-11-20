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

QTEST_MAIN(PlaybackTest)
#include "tst_playback.moc"
