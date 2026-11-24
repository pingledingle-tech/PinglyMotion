Branch: job/view-and-timeline
Status: done
Pull request: #449
Files:
- src/timelinerows.*
- src/timeline.*
- src/timelineview.*
- src/animationview.*
- src/viewmenuspec.h
- src/viewtoolspec.h
- src/jointrings.h
- src/usersettings.*
- src/pinglymotionwindow.*
- src/pinglymotionwindow.ui
- src/CMakeLists.txt
- CMakeLists.txt
- tests/viewtimeline/**
- tests/menutexts/**
- tests/timelinerows/**
- tests/viewtools/**
- tests/jointrings/**
- tests/twofigures/**
- tests/tst_timelineview.cpp
- translations/pinglymotion_en.ts
- docs/UX_DECISIONS.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/PRODUCT.md
- docs/TEACHING_TEXTS.md
- docs/RENDERING.md

## What

UX job U2 (B2), view and timeline: a timeline filter (body area, and in Expert
mode all joints or only the joints that move), an on/off switch for clicking
and bending joints directly on the figure, and every view setting in the View
menu: the joint display and the joint group switches move there from Options
(Show skeleton becomes View > Joint dots, the same setting as Show > Joint dots
in the 3D view), together with Body and Lighting. Exact automated tests.
