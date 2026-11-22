Branch: job/ux-0a-tokens
Status: done
Pull request: #361
Files:
- src/theme.*
- src/viewcolours.*
- src/timeline.*
- src/timelineview.*
- src/animationview.cpp
- src/CMakeLists.txt
- CMakeLists.txt
- tests/uxcolours/**
- tests/CMakeLists.txt
- src/timelinelook.h
- src/viewcolours.h
- docs/DECISIONS.md
- docs/UX_DECISIONS.md
- docs/DESIGN_SYSTEM.md

## What

UX phase 0a: the timeline, the 3D view markers, joint balls and rotation rings and the window palette take every colour from the design tokens (src/theme.*), so the light theme reaches the timeline. Each colour has an exact automatic test.
