Branch: feature/check-animation
Status: done
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
`src/animcheck.*`: errors (over 60 s, nothing moves, hip over 5 m), warnings
(posed joints dropped because they keep the frame-1 angle, joint limits, foot
under the floor, floating, foot sliding, jerks, dead start, loop seam) and
notes (joints left out, keys that change nothing), each with frames, joints
and a fix. `motiondescribe.*` shares its sampling and loop seam. Tests for
every check on hand-made animations, plus the bundled, CMU and edge-case
files. Docs: `docs/CONTROL.md`, `docs/ASSISTANT_GUIDE.md`. Nothing on screen
changes.
