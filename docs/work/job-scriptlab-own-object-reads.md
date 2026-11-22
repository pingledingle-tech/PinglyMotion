Branch: job/scriptlab-own-object-reads
Status: active
Pull request: #365
Files:
- src/scriptlab/lsl/builtins.*
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/session.*
- src/scriptlab/lslbuiltins_extra.inc
- tests/tst_scriptlab*.cpp
- tests/data/scriptlab/**
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md

## What

Script Lab: cheap own-object reads and state (second gap list, step 2). Adds llDie, llGetVel, llGetMass, llSetScale, llSetLocalRot, llGetLinkName, llGetLinkKey, llDetectedLinkNumber, llGetObjectPermMask, llGetInventoryPermMask, llGetObjectDetails, llSameGroup, llSetScriptState, llResetOtherScript and a few equally cheap neighbours to the interpreter, with exact-answer tests and docs.
