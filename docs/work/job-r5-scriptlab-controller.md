Branch: job/r5-scriptlab-controller
Status: active
Pull request: #478
Files:
- src/scriptlabcontroller.*
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/CMakeLists.txt
- CMakeLists.txt
- tests/scriptlabcontroller/**
- docs/DEV.md

## What

Job R5 (technical review): move the Script Lab wiring out of the main window
constructor into its own class, `ScriptLabController` (src/scriptlabcontroller.*).
It creates the Script Lab panel and its dock, connects it to the control API,
the 3D view and the scene, and owns the Help > Script Lab steps action and the
Script Lab part of View > Reset layout. Pure refactor: nothing changes on screen
and no behaviour changes. A small unit test for the new class.
