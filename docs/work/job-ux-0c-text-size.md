Branch: job/ux-0c-text-size
Status: active
Pull request: #363
Files:
- src/panellayout.h
- src/panelwindow.h
- src/paneltitlebar.*
- src/pinglymotionwindow.*
- src/pinglymotion.cpp
- src/main.cpp
- src/theme.*
- src/textsize.*
- src/usersettings.*
- src/CMakeLists.txt
- src/scriptlab/scriptlabpanel.*
- tests/panellayout/**
- tests/paneldocks/**
- tests/uirender/**
- tests/textsize/**
- tests/CMakeLists.txt
- tests/animfile/CMakeLists.txt
- fuzz/CMakeLists.txt
- src/constants.cpp
- src/timeline.cpp
- src/timelineview.cpp
- src/panelcontent.h
- CMakeLists.txt
- translations/pinglymotion_en.ts
- docs/UX_DECISIONS.md
- docs/DESIGN_SYSTEM.md
- docs/DEV.md
- docs/PRODUCT.md
- docs/TEACHING_TEXTS.md
- tools/check_texts.py

## What

UX phase 0c (lane 2 of the queue in issue 74): the stored text size (100 to 200
percent, `src/usersettings.*`) is applied to the whole window, at start and when
it changes, and at large text or in a small window the panels rearrange instead
of scrolling sideways (docs/UX_DECISIONS.md, 6c): Joints becomes a tab on the
right side, at the extreme everything stacks in one column, and pop-out is greyed
out with its reason. Exact automated tests for the rules, the button state and
clipping at 100, 150 and 200 percent.
