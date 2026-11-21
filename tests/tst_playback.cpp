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
  // transport buttons (docs/UX_DECISIONS.md section 1)
  void transportButtons();
  void startStateFollowsLoop();
  void loopSwitchWhilePlaying();
  void replayPlaysFromTheFirstFrame();
  void replayWithLoopOffPlaysOnce();
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

// Start only while stopped, Stop only while playing, Replay from start always
void PlaybackTest::transportButtons()
{
  const Playback::Buttons stopped=Playback::buttons(PLAYSTATE_STOPPED);
  QVERIFY(stopped.start);
  QVERIFY(!stopped.stop);
  QVERIFY(stopped.replay);
  for(PlayState state : {PLAYSTATE_LOOPING,PLAYSTATE_PLAYING})
  {
    const Playback::Buttons playing=Playback::buttons(state);
    QVERIFY(!playing.start);
    QVERIFY(playing.stop);
    QVERIFY(playing.replay);
  }
}

void PlaybackTest::startStateFollowsLoop()
{
  QCOMPARE(Playback::startState(true,0,9),PLAYSTATE_LOOPING);
  QCOMPARE(Playback::startState(true,8,9),PLAYSTATE_LOOPING);
  QCOMPARE(Playback::startState(true,9,9),PLAYSTATE_PLAYING);   // past the loop end: once
  QCOMPARE(Playback::startState(false,0,9),PLAYSTATE_PLAYING);
}

void PlaybackTest::loopSwitchWhilePlaying()
{
  // stopped stays stopped whatever the switch does
  QCOMPARE(Playback::loopSwitched(PLAYSTATE_STOPPED,true,0,9),PLAYSTATE_STOPPED);
  QCOMPARE(Playback::loopSwitched(PLAYSTATE_STOPPED,false,0,9),PLAYSTATE_STOPPED);
  // off while repeating: play to the end and stop
  QCOMPARE(Playback::loopSwitched(PLAYSTATE_LOOPING,false,4,9),PLAYSTATE_PLAYING);
  // on while playing once: go on repeating
  QCOMPARE(Playback::loopSwitched(PLAYSTATE_PLAYING,true,4,9),PLAYSTATE_LOOPING);
  // on, but already past the loop end: finish once
  QCOMPARE(Playback::loopSwitched(PLAYSTATE_PLAYING,true,9,9),PLAYSTATE_PLAYING);
}

// Replay from start as the window does it: first frame, then play; with Loop
// on it comes back to the loop-in frame after the loop end and never stops
void PlaybackTest::replayPlaysFromTheFirstFrame()
{
  BVH bvh;
  Animation anim(&bvh);
  anim.setNumberOfFrames(10);
  anim.setLoopInPoint(1);
  anim.setLoopOutPoint(9);
  anim.setFrame(6);

  anim.setFrame(Playback::replayFrame());
  QCOMPARE(anim.getFrame(),0);
  const PlayState state=Playback::startState(true,anim.getFrame(),anim.getLoopOutPoint());
  QCOMPARE(state,PLAYSTATE_LOOPING);
  anim.setPlaystate(state);

  for(int step=0;step<9;step++) anim.stepForward();
  QCOMPARE(anim.getFrame(),9);
  anim.stepForward();
  QCOMPARE(anim.getFrame(),1);                // back to the loop-in frame
  anim.setPlaystate(PLAYSTATE_STOPPED);
}

void PlaybackTest::replayWithLoopOffPlaysOnce()
{
  BVH bvh;
  Animation anim(&bvh);
  anim.setNumberOfFrames(10);
  anim.setFrame(9);                           // a one-shot play ended here

  anim.setFrame(Playback::replayFrame());
  QCOMPARE(Playback::startState(false,anim.getFrame(),anim.getLoopOutPoint()),PLAYSTATE_PLAYING);
  QCOMPARE(playOnce(anim),9);
  QCOMPARE(anim.getFrame(),9);
}

QTEST_MAIN(PlaybackTest)
#include "tst_playback.moc"
