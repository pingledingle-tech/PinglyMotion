Branch: job/scriptlab-channel-map
Status: done
Pull request: #453
Files:
- src/scriptlab/labchannelmap.*
- src/scriptlab/channelmapview.*
- src/scriptlab/scriptlabpanel.*
- src/scriptlab/labbeginner.cpp
- src/CMakeLists.txt
- tests/tst_labchannelmap.cpp
- tests/CMakeLists.txt
- tests/scriptlabbeginner/**
- tests/scriptlabpanel/tst_scriptlabdesign.cpp
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md
- docs/TEACHING.md

## What

Part 1 of job A4 (this pull request): the channel map. Part 2 (chain and ribbon lines in the 3D view) comes in its own pull request after this one.

Job A4 of the approved plan "Script Lab: see and understand": a channel map in
the Script Lab panel (who talks to whom in a run: objects and prims as boxes,
messages as arrows with channel and text, plain-word error for a channel
nobody listens on, servers as hub or web boxes, click selects the line or the
object), then straight chain and ribbon lines in the 3D view between two parts
that a script links (red when locked, green when free, always with words).
