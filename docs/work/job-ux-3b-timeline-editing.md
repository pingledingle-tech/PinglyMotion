Branch: job/ux-3b-timeline-editing
Status: done
Pull request: #400
Files:
- src/timelineclip.*
- src/timeline.*
- src/timelineview.*
- src/timelinerows.*
- src/timelinelook.h
- src/panelcontent.h
- src/transportlook.cpp
- src/menuspec.h
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/pinglymotionwindow.ui
- src/pinglymotionwindowplayback.cpp
- src/lucideicons.cpp
- src/CMakeLists.txt
- CMakeLists.txt
- tests/CMakeLists.txt
- tests/timelineclip/**
- tests/timelinerows/**
- tests/menutexts/tst_menutexts.cpp
- tests/reasonline/tst_reasonline.cpp
- tests/tst_timeline.cpp
- translations/*.ts
- docs/UX_DECISIONS.md
- docs/ROADMAP.md
- docs/PRODUCT.md
- docs/DESIGN_SYSTEM.md
- docs/TEACHING_TEXTS.md

## What

UX phase 3b (UX step 8, issue 74 queue item 7, AUTO): select a stretch of the
timeline (frames and rows), Copy, Cut, Paste and Paste mirrored (a left step
becomes a right step), with the keyboard, and exact-answer tests in
`tests/timelineclip`.
