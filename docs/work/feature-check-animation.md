Branch: feature/check-animation
Status: active
Pull request: #181
Files:
- src/controlapi.*
- src/animcheck.*
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

Control command `check_animation` (issue 129, the rest of job 3 in its
"First three jobs" comment): exact checks of the whole animation against
Second Life's rules and common mistakes, with fixed thresholds, in a new
`src/animcheck.*`. Nothing on screen changes.
