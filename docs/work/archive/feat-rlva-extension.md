Branch: feat/rlva-extension
Status: done
Pull request: #299
Files:
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/rlvstate.*
- src/controlapi.cpp
- src/controlapi.h
- tests/tst_rlva.cpp
- tests/tst_rlvstate.cpp
- tests/tst_rlvcommand.cpp
- tests/tst_control.cpp
- tests/CMakeLists.txt
- fuzz/fuzz_rlv.cpp
- fuzz/rlv.dict
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md
- CREDITS.md
- tools/mcp/commands.json
- fuzz/corpus/rlv/08.txt
- docs/work/feat-rlva-extension.md

## What

Script Lab step 9 (issue 297): the behaviours that only the RLVa implementation has, so scripts written for RLVa
viewers can be tested. The table marks each behaviour "RLV" or "RLVa only"; the state model gets a mode switch
(plain RLV by default; in plain mode the RLVa-only behaviours answer as unknown); behaviours with effects outside the
sandbox (camera, environment, debug settings) are recorded as state and shown in the log. Technical: nothing visible
changes.
