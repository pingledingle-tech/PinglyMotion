Branch: feat/frame-step
Status: active
Pull request: #271
Files:
- src/transportbar.*
- src/playback.h
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/pinglymotionwindowplayback.cpp
- tests/transportbar/**
- docs/UX_DECISIONS.md
- docs/POSING_UX.md
- docs/FIRST_ANIMATION.md

## What

UX build order step 2 (issue 14): Frame back and Frame forward buttons next to
the transport buttons under the timeline, and the left / right arrow keys step
one frame too. Stepping stops at the first and the last frame.
