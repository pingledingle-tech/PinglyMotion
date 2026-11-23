Branch: job/set-camera-focus
Status: done
Pull request: #417
Files:
- src/controlapi.cpp
- src/controlapi.h
- src/camera.cpp
- src/camera.h
- src/animationview.cpp
- src/animationview.h
- src/pinglymotionwindow.cpp
- tests/tst_control.cpp
- tests/tst_camera.cpp
- tests/smoke_control.py
- tests/test_mcp_node.py
- tools/mcp/commands.json
- tools/mcp/pinglymotion_mcp.js
- fuzz/control.dict
- fuzz/corpus/control/
- docs/ASSISTANT_GUIDE.md
- docs/CONTROL.md

## What

Phase 2c step 2c-a: `set_camera` (control command and MCP tool) gets an optional `joint`. The camera then
centres on where that joint is in the posed figure (for example a hand over the head), at the same
distance as the Picked joint framing (F key) unless a distance is given. Without a joint every call
behaves exactly as before. Part of #74.
