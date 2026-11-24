Branch: job/scriptlab-channel-map
Status: active
Pull request: pending
Files:
- src/scriptlab/labchannelmap.*
- src/scriptlab/channelmapview.*
- src/scriptlab/labchains.*
- src/scriptlab/scriptlabpanel.*
- src/scriptlab/effectspreview.*
- src/scriptlab/labwatcher.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/session.*
- src/scriptlab/lsl/control.*
- src/controlapi.*
- src/CMakeLists.txt
- tests/tst_labchannelmap.cpp
- tests/tst_labchains.cpp
- tests/tst_control.cpp
- tests/CMakeLists.txt
- tests/scriptlabbeginner/**
- tests/scriptlabpanel/**
- tools/mcp/commands.json
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md
- docs/TEACHING.md
- CREDITS.md

## What

Job A4 of the approved plan "Script Lab: see and understand": a channel map in
the Script Lab panel (who talks to whom in a run: objects and prims as boxes,
messages as arrows with channel and text, plain-word error for a channel
nobody listens on, servers as hub or web boxes, click selects the line or the
object), then straight chain and ribbon lines in the 3D view between two parts
that a script links (red when locked, green when free, always with words).
