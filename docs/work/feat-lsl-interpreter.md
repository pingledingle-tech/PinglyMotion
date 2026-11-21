Branch: feat/lsl-interpreter
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/**
- src/scriptlab/lslbuiltins_data.inc
- tools/gen_lsl_builtins.py
- tests/tst_lsl*.cpp
- examples/scriptlab/*.lsl
- fuzz/fuzz_lsl.cpp
- fuzz/lsl.dict
- fuzz/corpus/lsl/**
- CMakeLists.txt (shared, minimal)
- src/CMakeLists.txt (shared, minimal)
- tests/CMakeLists.txt (shared, minimal)
- fuzz/CMakeLists.txt (shared, minimal)
- .github/workflows/fuzz.yml (shared, one matrix line)
- src/controlapi.* (shared: lsl_check, lsl_run only)
- tests/tst_control.cpp (shared, lsl_* only)
- tests/test_mcp_node.py (shared, minimal)
- tools/mcp/commands.json (generated)
- docs/CONTROL.md (shared, minimal)
- docs/ROADMAP.md (shared, minimal)
- CREDITS.md (shared, minimal)

## What

Script Lab step 3 (issue 289): a small LSL interpreter in C++/QtCore
(recursive descent, no flex/bison) in `src/scriptlab/lsl/`, with hard
limits on steps, call depth, list/string sizes and run time. Control
commands `lsl_check` (errors with line and column) and `lsl_run` (event log
as plain data). Unit tests, a fuzz target for the lexer/parser, examples.
Does not touch `src/scriptlab/rlv*` or the `rlv_*` commands (job feat/rlv-core).
