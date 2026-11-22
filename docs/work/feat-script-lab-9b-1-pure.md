Branch: feat/script-lab-9b-1-pure
Status: done
Pull request: #311
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/world.h
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lslbuiltins_extra.inc
- src/controlapi.cpp
- tools/gen_lsl_extra.py
- tools/mcp/commands.json
- tests/tst_lslpure.cpp
- tests/tst_lslworld.cpp
- tests/CMakeLists.txt
- fuzz/corpus/lsl/pure_functions.lsl
- CREDITS.md
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/CONTROL.md
- docs/ROADMAP.md

## What

Script Lab step 9b-1 (issue 310): the signature table gets the names the viewer keyword list lacks (from Second Life's lsl-definitions, MIT), and the sandbox runs 25 pure LSL functions that need no world state (rotation maths, strided lists, `llReplaceSubString`, Base64, MD5, SHA-1, URL escaping, start parameter, object name and description, scale, prim count). `lsl_run` gets `description`, `scale` and `start_parameter`.
