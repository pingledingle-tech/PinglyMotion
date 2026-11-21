Branch: feat/lsl-rlv-link
Status: done
Pull request: #300
Files:
- docs/work/feat-lsl-rlv-link.md
- src/scriptlab/lsl/**
- src/controlapi.cpp
- src/CMakeLists.txt
- tests/tst_lslrlv.cpp
- tests/tst_control.cpp
- tests/test_mcp_node.py
- tools/mcp/commands.json
- examples/scriptlab/rlv_lock.lsl
- examples/scriptlab/rlv_status.lsl
- fuzz/fuzz_lsl.cpp
- fuzz/lsl.dict
- fuzz/README.md
- fuzz/CMakeLists.txt
- fuzz/corpus/lsl/**
- tests/CMakeLists.txt
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md

## What

Script Lab step 4 (issue 290): `llOwnerSay("@...")` in a script run by `lsl_run`
goes to the RLV core for that script's object. Replies come back as listen
events on the reply channel. The log shows the command, the result and the
final restrictions. Does not restructure `src/scriptlab/rlv*`; uses its public API only.

Checked locally: the Script Lab, LSL and control tests pass. The fuzz target `fuzz_lsl` now
runs scripts through `RlvWorld` with an RLV state.
