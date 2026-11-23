Branch: job/describe-hands
Status: active
Pull request: #419
Files:
- src/posedescribe.*
- src/controlapi.cpp
- tests/tst_control.cpp
- tools/mcp/commands.json
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md

## What

Phase 2c step b: `describe_pose` also reports, for each hand, which way the
palm faces (nearest of forward, back, in, out, up, down in the character's
own frame) and where each fingertip is (Bento finger joints), relative to the
wrist. Existing output stays unchanged. Part of #74.
