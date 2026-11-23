Branch: job/ux-3c-notes-line
Status: active
Pull request: pending
Files:
- src/timelinenotes.*
- src/timelineview.*
- src/transportlook.cpp
- src/lucideicons.cpp
- src/animation.*
- src/bvh.*
- src/pinglymotionwindow.cpp
- src/pinglymotionwindowplayback.cpp
- src/CMakeLists.txt
- CMakeLists.txt
- tests/CMakeLists.txt
- tests/timelinenotes/**
- tests/transportbar/tst_transportbar.cpp
- translations/*.ts
- docs/UX_DECISIONS.md
- docs/ROADMAP.md
- docs/PRODUCT.md
- docs/DESIGN_SYSTEM.md
- docs/TEACHING.md
- docs/TEACHING_TEXTS.md
- docs/DINGLE_FORMAT.md
- docs/FIRST_ANIMATION.md

## What

UX phase 3c: the notes line under the timeline. It shows the teaching note of
the current frame (the wave example's notes, WAVE-01 to WAVE-06, stored in the
file's `extensions`), a Hide notes button, and Help > Show hints shows it
again. The texts and behaviour have exact automated tests; the look (spacing,
tone of colour) goes to the combined Windows review.
