Branch: feature/describe-motion
Status: done
Pull request: #161
Files:
- src/controlapi.*
- src/motiondescribe.*
- src/posedescribe.*
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
AI assistants, in a new `src/motiondescribe.*`: per classic joint what its main
axis does over time (increases, holds, decreases; how far; speed class; when it
starts), with reach and speed measured as one rotation so gimbal swings do not
count; hand and foot paths relative to the hip (straight, arc, out and back);
foot contact, sliding and weight; hip height and travel; rhythm (period, beats
per minute, legs and arms in opposition, feet alternating); the loop seam; a
key table; a summary. `posedescribe.*` shares its joint helpers. Tests on a
hand-made arm lift, the CMU walk and wave and the generated edge cases.
`check_animation` follows as its own job. Nothing on screen changes.
