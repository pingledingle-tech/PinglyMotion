Branch: feat/script-lab-9b-6-controls
Status: done
Pull request: #340
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/parser.cpp
- src/controlapi.cpp
- tools/mcp/commands.json
- tests/tst_lslcontrols.cpp
- tests/tst_lslevents.cpp
- tests/tst_lslworld.cpp
- tests/tst_labwatcher.cpp
- tests/tst_lslpure.cpp
- tests/CMakeLists.txt
- fuzz/fuzz_lsl.cpp
- fuzz/corpus/lsl/controls.lsl
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/CONTROL.md
- docs/ROADMAP.md

## What

Script Lab step 9b-6: controls. `llTakeControls` and `llReleaseControls` (with PERMISSION_TAKE_CONTROLS), the `control` input (levels and edges) and the `control` event.
