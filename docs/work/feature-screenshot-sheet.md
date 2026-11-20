Branch: feature/screenshot-sheet
Status: active
Pull request: pending
Files:
- src/controlapi.*
- src/screenshotsheet.*
- src/animationview.*
- src/camera.*
- src/CMakeLists.txt
- tests/tst_control.cpp
- tests/CMakeLists.txt
- fuzz/CMakeLists.txt
- fuzz/control.dict
- tools/mcp/commands.json
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md
- docs/AI_HELP_PLAN.md

## What

Readable screenshots for assistants (job C of docs/AI_HELP_PLAN.md, issue
129): `screenshot` gets optional views (a captioned grid), frames (a strip),
labels (classic joint names, the figure's left and right in two colours),
a hand and foot dot trail, and hiding the Bento markers. Only the returned
image changes; the program window, its camera and the current frame stay as
they were. Technical: nothing in the program window changes.
