Branch: feat/scriptlab-memory-estimate
Status: active
Pull request: #521
Files:
- src/scriptlab/lsl/ast.h
- src/scriptlab/lsl/parser.cpp
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/sandbox.cpp
- src/scriptlab/lsl/control.*
- src/controlapi.cpp
- src/scriptlab/labweight.*
- src/scriptlab/labwatcher.cpp
- src/scriptlab/labfolder.cpp
- src/scriptlab/labimproveview.*
- tests/tst_labfolder.cpp
- tests/tst_lsllink.cpp
- tests/tst_control.cpp
- tests/tst_labwatcher.cpp
- tests/scriptlabpanel/tst_labimproveview.cpp
- tests/data/scriptlab/scenarios.json
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- docs/MANUAL.md

## What

Script Lab estimates the memory of each script (code at 9 bytes per source token plus values), shows it in the Improve a setup table, and a new run option `script_memory` runs scripts with Second Life's per-script limit so a script that would hit a Stack-Heap Collision stops the same way. Second part of the big-setups job; the first part is already merged.
