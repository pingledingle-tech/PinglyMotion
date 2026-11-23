Branch: job/lsl-objects-give-load
Status: active
Pull request: #393
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/world.h
- src/scriptlab/lsl/world.cpp
- src/scriptlab/lsl/sandbox.cpp
- src/scriptlab/lsl/sandbox.h
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/interp.h
- src/scriptlab/lsl/interp.cpp
- tests/tst_lslobjects.cpp
- tests/tst_lslpure.cpp
- tests/tst_lslstubs.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/CONTROL.md

## What

Script Lab gap list, group "several objects": llGiveInventoryList, llRemoteLoadScriptPin with
llSetRemoteScriptAccessPin, llAllowInventoryDrop (CHANGED_ALLOWED_DROP), llGetInventoryCreator and
llGetInventoryDesc, with real behaviour between objects of the sandbox world where it has them.
Technical job: nothing changes on screen.
