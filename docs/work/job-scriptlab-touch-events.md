Branch: job/scriptlab-touch-events
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/**
- src/controlapi.cpp
- tools/mcp/commands.json
- tests/tst_lsl*.cpp
- tests/data/scriptlab/**
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/ROADMAP.md

## What

Script Lab: the `touch` and `touch_end` events. A touch input can now be held: `touch_start`, then `touch` repeatedly while it is held, then `touch_end`, with the llDetected* values of the toucher, as in Second Life. Inputs through `inputs`, `lsl_run` and `lsl_event`; the parser knows both events are raised; exact-answer tests and docs.
