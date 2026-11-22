Branch: job/scriptlab-region-reads
Status: active
Pull request: #370
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/interp.*
- tests/tst_lslregion.cpp
- tests/CMakeLists.txt
- tests/data/scriptlab/scenarios.json
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- tools/mcp/commands.json

## What

Script Lab, second gap list, step 3 "Region and avatar reads": llGetRegionName, llGetRegionCorner, llGround, llWater,
llGetAgentSize, llGetAgentList, llRequestAgentData, llGetSunDirection, llGetTimeOfDay, llGetTimestamp, llGetWallclock
and llGetDate, with canned values from settings of the sandbox's region (an optional "region" in lsl_run) and the
simulated clock, never the real clock. Exact-answer tests for every function.
