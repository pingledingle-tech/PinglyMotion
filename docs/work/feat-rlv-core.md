Branch: feat/rlv-core
Status: active
Pull request: pending
Files:
- src/scriptlab/**
- src/CMakeLists.txt
- src/controlapi.*
- tests/tst_rlvcommand.cpp
- tests/tst_rlvstate.cpp
- tests/tst_control.cpp
- tests/CMakeLists.txt
- tests/test_mcp_node.py
- tools/mcp/commands.json
- fuzz/fuzz_rlv.cpp
- fuzz/rlv.dict
- fuzz/corpus/rlv/**
- fuzz/CMakeLists.txt
- fuzz/README.md
- .github/workflows/fuzz.yml
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/ROADMAP.md
- CREDITS.md

## What

Script Lab step 1 (issue 287): the RLV core in `src/scriptlab/` (command parser, behaviour table,
per-object restriction state) and the control commands `rlv_command`, `rlv_state` and `rlv_reset`.
Technical: nothing on screen changes.
