Branch: feat/panels-p2
Status: active
Pull request: pending
Files:
- src/pinglymotionwindow.*
- src/usersettings.*
- src/panellayout.h
- CMakeLists.txt
- tests/paneldocks/**
- docs/POSING_UX.md
- docs/PANELS_PLAN.md
- docs/PRODUCT.md

## What

Posing UX step 3, job P2 of docs/PANELS_PLAN.md: the Properties panel is split
into Joints (left) and Selected joint (right); loop in / out, frame count and
FPS move to the Timeline panel; a new View menu (Joints, Selected joint,
Timeline, Reset layout) takes over the old Options items; the layout is saved
under the versioned setting panel_layout, so an old window_state with the
Properties panel is not loaded. Visible: the project owner tests on Windows.
