/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * Play button logic that needs no window.
 * After a one-shot play the playhead stays on the last frame. Pressing play
 * there must start again, not stop at once.
 */
#ifndef PLAYBACK_H
#define PLAYBACK_H

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
}

#endif
