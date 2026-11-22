Branch: feat/script-lab-9b-1-pure
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lslbuiltins_extra.inc
- tools/gen_lsl_extra.py
- tests/tst_lslpure.cpp
- tests/CMakeLists.txt
- fuzz/fuzz_lsl.cpp
- CREDITS.md
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/CONTROL.md
- docs/ROADMAP.md

## What

Script Lab step 9b-1 (issue 310): the signature table gets the names the viewer keyword list lacks, and the sandbox runs pure LSL functions that need no world state (rotation maths, strided lists, Base64, hashes, URL escaping, start parameter, object name and description).
