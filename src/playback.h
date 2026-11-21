/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * Play button logic that needs no window (the transport buttons Start, Stop,
 * Replay from start and the Loop switch use it; tests/tst_playback.cpp).
 * After a one-shot play the playhead stays on the last frame. Pressing play
 * there must start again, not stop at once.
 */
#ifndef PLAYBACK_H
#define PLAYBACK_H

#include "playstate.h"

namespace Playback
{
// a one-shot play is finished when the last frame is shown
inline bool atEnd(int frame,int totalFrames)
{
  return frame>=totalFrames-1;
}

// frame to start at when play is pressed: at the end, go back to the loop-in
// frame (or the first frame when that is the end too); anywhere else, stay
inline int startFrame(int frame,int totalFrames,int loopIn)
{
  if(!atEnd(frame,totalFrames)) return frame;
  return (loopIn>=0 && loopIn<totalFrames-1) ? loopIn : 0;
}

// --- transport buttons (docs/UX_DECISIONS.md section 1): Start, Stop,
// Replay from start, and the Loop switch

// Replay from start always begins on the first frame
inline int replayFrame()
{
  return 0;
}

// how playing goes when it starts on this frame: with Loop on it repeats
// between the loop points, unless it starts past the loop end; otherwise it
// plays once to the last frame and stops
inline PlayState startState(bool loop,int frame,int loopOut)
{
  return (loop && frame<loopOut) ? PLAYSTATE_LOOPING : PLAYSTATE_PLAYING;
}

// the Loop switch changed while the animation plays (or not): switched off,
// it plays to the last frame and stops; switched on, it goes on repeating
// unless it is already past the loop end. Stopped stays stopped.
inline PlayState loopSwitched(PlayState state,bool loop,int frame,int loopOut)
{
  if(state==PLAYSTATE_STOPPED) return PLAYSTATE_STOPPED;
  return startState(loop,frame,loopOut);
}

// which transport buttons can be clicked: Start only while stopped, Stop only
// while playing, Replay from start always
struct Buttons
{
  bool start;
  bool stop;
  bool replay;
};

inline Buttons buttons(PlayState state)
{
  const bool playing=state!=PLAYSTATE_STOPPED;
  return Buttons{!playing,playing,true};
}
}

#endif
