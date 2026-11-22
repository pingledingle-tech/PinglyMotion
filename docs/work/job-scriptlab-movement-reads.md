Branch: job/scriptlab-movement-reads
Status: done
Pull request: pending
Files:
- src/scriptlab/lsl/builtins.cpp
- tests/tst_lslmovement.cpp
- tests/tst_lslpure.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/CONTROL.md

## What

Script Lab gap list 2, step 6, part 1: record-only stubs for the own-object
movement and physics functions that need no motion model (llMoveToTarget,
llStopMoveToTarget, llPushObject, llApplyImpulse, llSetBuoyancy, llRotLookAt,
llVolumeDetect). Same generic record-only pattern as the step 9b-8 stubs
(llPointAt, llLookAt, llMakeExplosion, ...): the call and its arguments are
recorded, nothing moves. Collision events and the vehicle functions are a
later job.
