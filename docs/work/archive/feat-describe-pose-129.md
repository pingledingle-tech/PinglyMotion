Branch: feat/describe-pose-129
Status: done
Pull request: #136
Files:
- src/controlapi.*
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

Control command `describe_pose` (issue 129, job 1 of the "First three jobs"
comment): the pose of one frame as computed text for AI assistants, in a new
`src/posedescribe.*`: world positions of head, hands, feet and hip (inches
and metres, each foot's height above the floor); rule-based sentences with a
dead zone at their thresholds (bent knees or elbows, a hand above the head or
behind the body, feet flat or lifted, weight on one foot, a lean, the head
turned or tipped); the classic joints that changed from frame 1 in everyday
names with "more" or "less"; left/right symmetry of the arms and legs; and a
one-line summary. Classic joints only, not the Bento fingers, face, wings or
tail. Tests on the T-pose, the sitting example and a raised hand. Docs
(`docs/CONTROL.md`, and the one line about `read_guide` it was waiting on)
and `tools/mcp/commands.json` updated; fuzzing run on the branch because the
command table changed. Nothing on screen changes.
