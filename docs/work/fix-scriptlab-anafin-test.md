Branch: fix/scriptlab-anafin-test
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/builtins_object.cpp
- src/scriptlab/lsl/builtins_prims.cpp
- src/scriptlab/lsl/builtins_internal.h
- src/scriptlab/lsl/control.*
- src/controlserver.cpp
- src/controlapi.cpp
- tests/tst_lslcollision.cpp
- tests/tst_lslmoney.cpp
- tests/tst_control.cpp
- docs/CONTROL.md
- docs/SCRIPT_LAB.md

## What

Script Lab fixes found while testing a money/worker HUD script: collision events from an avatar report
the avatar's velocity in llDetectedVel, llGetVel in an attachment reports the wearer's velocity,
llTransferLindenDollars gets a control option for its result (delay, failure, no answer), and a too large
control request gets an error message that names the limit and points to lab_watch.
