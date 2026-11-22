Branch: job/scriptlab-motion-model
Status: active
Pull request: #381
Files:
- src/scriptlab/lsl/sandbox.h
- src/scriptlab/lsl/sandbox.cpp
- src/scriptlab/lsl/world.h
- src/scriptlab/lsl/world.cpp
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/control.cpp
- tests/tst_lslmovement.cpp
- tests/tst_lslmotion.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/CONTROL.md

## What

Second gap list, step 6, part 3: a small, deliberately scaled-down slice of
the motion model the owner has been waiting for since part 1. Scope, kept
small on purpose (this is flagged as "large" in the private gap notes):

- A new `step` sandbox input (`dt`: seconds) that advances a simple,
  deterministic, non-realtime model for one object: velocity changes by
  simple gravity scaled by buoyancy (`llSetBuoyancy`, now a real setter
  instead of record-only), position changes by velocity. No real continuous
  physics, no physics engine.
- `llApplyImpulse` is now a real handler: it changes the object's velocity
  at once by impulse / mass (using the existing `llGetMass` formula), with
  the local/world rotation handling `movePrim` already has.
- `llGetVel` now answers the model's velocity instead of always zero.
- `moving_start`/`moving_end` now also fire on their own from the `step`
  input when the object's speed crosses a fixed threshold, in addition to
  the existing manual `moving` input from part 2.

Deliberately left out for a follow-up job:
- `llMoveToTarget`/`llStopMoveToTarget` stay record-only; the model does not
  seek a target yet, so `at_target`/`not_at_target` still only fire from the
  manual inputs added in part 2.
- `llPushObject`, `llRotLookAt`, `llVolumeDetect` stay record-only.
- Vehicles (step 7) and real collisions from motion (objects actually
  touching) are untouched; `collision_start`/`collision_end` stay manually
  injected.

Tests: exact numeric results per `step` tick (gravity+buoyancy, impulse),
and a scripted scenario where `moving_start`'s own handler cancels the
velocity so the following tick fires `moving_end`.
