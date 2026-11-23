Branch: job/lsl-limits-targets-region
Status: done
Pull request: #396
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/world.h
- src/scriptlab/lsl/world.cpp
- src/scriptlab/lsl/sandbox.cpp
- src/scriptlab/lsl/sandbox.h
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/interp.h
- src/scriptlab/lsl/interp.cpp
- tests/tst_lsllimits.cpp
- tests/tst_lslpure.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/CONTROL.md

## What

Script Lab gap list, third run, steps 1 to 3: object and script limit reads (llGetMemoryLimit,
llSetMemoryLimit, llMinEventDelay, llGetNumberOfSides, llGetLinkNumberOfSides, llGetObjectPrimCount,
llGetObjectMass, llGetBoundingBox, llGetEnergy), llTarget and llTargetRemove, and region reads
(llGetRegionTimeDilation, llGetRegionFPS, llGetRegionAgentCount, llGetSimulatorHostname).
Technical job: nothing changes on screen.
