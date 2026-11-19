Branch: feat/describe-pose-129
Status: active
Pull request: pending
Files:
- src/controlapi.*
- src/posedescribe.*
- tests/tst_control.cpp
- tests/CMakeLists.txt
- fuzz/CMakeLists.txt
- fuzz/control.dict
- tools/mcp/commands.json
- docs/CONTROL.md

## What

Control command `describe_pose` (issue 129): the pose of one frame as computed
text for AI assistants: where head, hands, elbows, knees, feet and hip are in
the world, rule-based sentences (bend of elbows and knees, hands relative to
the body, feet, lean, head, weight), the joints changed since frame 1 with a
direction word, left/right differences and a one-line summary. Nothing on
screen changes.
