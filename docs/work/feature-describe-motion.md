Branch: feature/describe-motion
Status: active
Pull request: pending
Files:
- src/controlapi.*
- src/motiondescribe.*
- src/CMakeLists.txt
- tests/tst_control.cpp
- tests/CMakeLists.txt
- fuzz/CMakeLists.txt
- fuzz/control.dict
- tools/mcp/commands.json
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md

## What

Control command `describe_motion` (issue 129, the motion half of job 3 in its
"First three jobs" comment): the motion of a frame range as computed text for
AI assistants, in a new `src/motiondescribe.*`: per joint what it does over
time (rises, holds, lowers; how far; speed class), hand and foot paths, foot
contact and weight shift, hip height, rhythm, the loop seam, and a summary.
`check_animation` follows as its own job. Nothing on screen changes.
