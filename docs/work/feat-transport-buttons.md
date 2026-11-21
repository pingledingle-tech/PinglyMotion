Branch: feat/transport-buttons
Status: active
Pull request: #251
Files:
- src/playback.h
- src/transportbar.*
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/pinglymotionwindowplayback.cpp
- src/pinglymotionwindow.ui
- src/usersettings.cpp
- src/CMakeLists.txt
- CMakeLists.txt
- tests/transportbar/**
- tests/tst_playback.cpp
- docs/UX_DECISIONS.md
- docs/POSING_UX.md
- docs/CONTROL.md

## What

UX build order step 1 (docs/UX_DECISIONS.md section 9, issue 14): the single
play / stop / loop button on the timeline becomes three buttons, Start, Stop
and Replay from start, plus a Loop check box that is on by default and
remembered. Needs the project owner's Windows test.
