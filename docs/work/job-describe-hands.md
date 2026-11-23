Branch: job/describe-hands
Status: active
Pull request: pending
Files:
- src/posedescribe.*
- src/controlapi.cpp
- tests/tst_control.cpp
- tests/test_mcp_node.py
- tools/mcp/commands.json
- tools/mcp/pinglymotion_mcp.js
- fuzz/corpus/control/
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md
- docs/ROADMAP.md

## What

Phase 2c step b: `describe_pose` also reports, for each hand, which way the
palm faces (nearest of forward, back, in, out, up, down in the character's
own frame) and where each fingertip is (Bento finger joints), relative to the
wrist. Existing output stays unchanged. Part of #74.
