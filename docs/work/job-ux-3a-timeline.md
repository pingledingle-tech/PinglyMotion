Branch: job/ux-3a-timeline
Status: active
Pull request: pending
Files:
- src/timeline.*
- src/timelineview.*
- src/timelinelook.h
- src/timelinerows.*
- src/panelcontent.h
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/pinglymotionwindow.ui
- src/pinglymotionwindowplayback.cpp
- src/pinglymotionwindowfile.cpp
- src/lucideicons.cpp
- src/transportlook.*
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/tst_timeline.cpp
- tests/tst_timelineview.cpp
- tests/timelinerows/**
- tests/uirender/**
- tests/uxcolours/**
- translations/*.ts
- docs/UX_DECISIONS.md
- docs/ROADMAP.md
- docs/PRODUCT.md
- docs/DESIGN_SYSTEM.md
- docs/TEACHING_TEXTS.md

## What

UX phase 3a (approved queue, lane 2, item 6): the timeline shows rows only for
changed joints plus the selected joint, each row marked with its body area
name and token colour, Set key as the blue main button in the timeline panel,
scrolling to the selected joint also when it has no keys, and the empty state
"No keys yet" / "Set the first key". Automated tests assert the behaviour.
