Branch: feat/scriptlab-script-order
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/control.*
- src/controlapi.cpp
- tests/tst_lslorder.cpp
- tests/CMakeLists.txt
- tests/tst_control.cpp
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- tools/mcp/commands.json
- fuzz/control.dict
- fuzz/corpus/control/34.json

## What

Script Lab: an `order` option on `lsl_run` (`fixed`, the default and today's behaviour, `reverse`, or `shuffle` with a
`seed`) that changes the order in which the scripts of the world get to run when several are due at the same time, so
bugs that depend on that order can be found and a run can be repeated exactly. Second Life runs scripts in parallel;
this does not simulate parallelism or preemption.
