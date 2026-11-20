Branch: fix/play-replay-after-end
Status: done
Pull request: #143
Files:
- src/playback.h
- src/pinglymotionwindow.cpp
- tests/CMakeLists.txt
- tests/tst_playback.cpp
- docs/work/fix-play-replay-after-end.md

## What

Play works once, then pressing play again does nothing: after a one-shot play
the playhead stays on the last frame, so the next play stops at once. Pressing
play at the end now starts again from the loop-in frame (or the first frame).
