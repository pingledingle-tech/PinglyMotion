Branch: job/lsl-object-reads
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/**
- src/scriptlab/lslbuiltins_extra.inc
- src/controlapi.cpp
- tests/tst_lsl*.cpp
- tests/tst_scriptlabwhole.cpp
- tests/data/scriptlab/**
- tests/smoke_scriptlab.py
- examples/scriptlab/**
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/ROADMAP.md
- docs/PRODUCT.md

## What

Script Lab: cheap reads and state on the script's own object (llDie, llGetVel, llGetMass, llSetScale, llSetLocalRot, llGetLinkName, llGetLinkKey, llDetectedLinkNumber, llGetObjectPermMask, llGetInventoryPermMask, llGetObjectDetails, llSameGroup, llSetScriptState, llResetOtherScript), with exact-answer tests. Lane 1, item 1 of the queue in issue 74.
