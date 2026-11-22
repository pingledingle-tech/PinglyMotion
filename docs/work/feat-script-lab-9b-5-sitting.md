Branch: feat/script-lab-9b-5-sitting
Status: active
Pull request:
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/control.cpp
- src/controlapi.cpp
- tools/mcp/commands.json
- tests/tst_lslsitting.cpp
- tests/tst_lslevents.cpp
- tests/tst_lslworld.cpp
- tests/tst_lsllink.cpp
- tests/tst_labwatcher.cpp
- tests/tst_lslpure.cpp
- tests/CMakeLists.txt
- fuzz/fuzz_lsl.cpp
- fuzz/corpus/lsl/pose_ball.lsl
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/CONTROL.md
- docs/ROADMAP.md

## What

Script Lab step 9b-5: sitting and pose balls (`llSitTarget`, `llLinkSitTarget`, `llAvatarOnSitTarget`, `llAvatarOnLinkSitTarget`, `llUnSit`, `llGetAnimationList`, `llGetAnimation`, `llGetAgentInfo`), the inputs `sit` and `stand`, and `changed` with `CHANGED_LINK`. An avatar sitting on the object gets the animation permission at once, so a pose ball can start its animation.
