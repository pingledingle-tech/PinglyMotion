Branch: feat/script-lab-lint
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/lint.*
- src/scriptlab/lsl/control.cpp
- src/controlapi.cpp
- src/CMakeLists.txt
- tests/tst_lsllint.cpp
- tests/CMakeLists.txt
- fuzz/fuzz_lsllint.cpp
- fuzz/CMakeLists.txt
- fuzz/README.md
- .github/workflows/fuzz.yml
- tools/mcp/commands.json
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- CREDITS.md

## What

Script Lab: a static checker (lint) for LSL scripts. It reports warnings (line, column, stable id, short message) without running the script, including RLV command checks for `llOwnerSay("@...")`. Exposed as the control command `lsl_lint`. Technical job: no panel UI (showing warnings in the panel is a later job).
