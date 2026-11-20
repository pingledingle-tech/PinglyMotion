Branch: fix/loop-end-last-frame-189
Status: active
Pull request: #191
Files:
- src/animation.cpp
- tests/tst_playback.cpp
- tests/assistant/cases.py
- docs/DEV.md

## What

When the frame count changes, a loop end on the last frame stays on the last frame, and a loop past the new end is clamped (issue 189). Removes the eval-set workaround in the `wave_loop` reference.
