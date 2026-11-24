Branch: job/props-from-prims
Status: active
Pull request: #447
Files:
- src/scriptlab/labprimprops.*
- src/scriptlab/labwatcher.*
- src/scriptlab/labfolder.*
- src/scriptlab/scriptlabpanel.*
- src/pinglymotionwindow.cpp
- src/CMakeLists.txt
- tests/tst_labprimprops.cpp
- tests/tst_labwatcher.cpp
- tests/CMakeLists.txt
- tests/scriptlabbeginner/tst_scriptlabbeginner.cpp
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md
- docs/TEACHING.md
- docs/EXAMPLES.md
- examples/objects/guide_now.json

## What

Script Lab, job A1 (plan "Script Lab: see and understand", approved by the project owner on 24 November 2026): with "Show effects in 3D view" on and a folder watched as one object, every prim gets a prop named like the prim (one of the four shapes, attached where the prim sits); a click on a prop sends a touch to the scripts in that prim and says so in the event log; when a script changes a prim's position, size or rotation, its prop jumps to the new place.
