Branch: feature/export-anim
Status: done
Pull request: #185
Files:
- src/controlapi.*
- tests/tst_control.cpp
- tests/CMakeLists.txt
- fuzz/CMakeLists.txt
- fuzz/control.dict
- tools/mcp/commands.json
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md
- docs/AI_HELP_PLAN.md

## What

Control command `export_anim` (job B of docs/AI_HELP_PLAN.md, issue 129): an
assistant writes the Second Life `.anim` file itself, with priority, loop,
slow start and finish and hand pose, through the same writer and defaults as
File > Export as Second Life .anim. It runs `check_animation` first and
refuses on errors. Files only in the control folder, never replaced.
Technical: nothing in the program window changes.
