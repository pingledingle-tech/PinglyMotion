Branch: job/small-leftovers-text-for-ai
Status: active
Pull request: pending
Files:
- src/motionscript.*
- src/posedescribe.cpp
- src/controlapi.cpp
- tests/motionscript/**
- tests/tst_control.cpp
- tests/assistant/**
- tools/mcp/commands.json
- translations/pinglymotion_en.ts
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md
- docs/PRODUCT.md

## What

Small leftovers of "text for AI" (issue 129): (1) load a motion table on top of the open animation (`load_motion_script` with `mode: merge`), one Undo step; (2) shorter pose text (no metre duplicates for fingertips); (3) one example for the merge in the AI test set; (4) the missing English texts.
