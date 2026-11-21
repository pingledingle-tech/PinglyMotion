Branch: feat/lsl-interpreter
Status: active
Pull request: #296
Files:
- src/scriptlab/lsl/**
- src/scriptlab/lslbuiltins_data.inc
- tools/gen_lsl_builtins.py
- tests/tst_lsl*.cpp
- src/CMakeLists.txt
- tests/CMakeLists.txt
- examples/scriptlab/*.lsl
- fuzz/fuzz_lsl.cpp
- fuzz/lsl.dict
- fuzz/corpus/lsl/**
- fuzz/CMakeLists.txt
- fuzz/README.md
- .github/workflows/fuzz.yml
- src/controlapi.*
- tests/tst_control.cpp
- tests/test_mcp_node.py
- tools/mcp/commands.json
- docs/CONTROL.md
- docs/ROADMAP.md
- docs/SCRIPT_LAB_PLAN.md
- docs/SCRIPT_LAB.md
- CREDITS.md

## What

Shared files above get small separate hunks only.

Script Lab step 3 (issue 289): a small LSL interpreter in C++/QtCore
(recursive descent, no flex/bison) in `src/scriptlab/lsl/`, with hard
limits on steps, call depth, list/string sizes and run time. Control
commands `lsl_check` (errors with line and column) and `lsl_run` (event log
as plain data). Unit tests, a fuzz target for the lexer/parser, examples.
Does not touch `src/scriptlab/rlv*` or the `rlv_*` commands (job feat/rlv-core).
