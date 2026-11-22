Branch: feat/script-lab-3d
Status: done
Pull request: #304
Files:
- src/scriptlab/labeffects.*
- src/scriptlab/effectspreview.*
- src/scriptlab/scriptlabpanel.*
- src/animationview.*
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/tst_labeffects.cpp
- tests/tst_scriptlabeffects.cpp
- examples/scriptlab/anim_*.lsl
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md
- docs/SCRIPT_LAB_PLAN.md
- docs/CONTROL.md
- src/controlapi.cpp
- tools/mcp/commands.json

## What

Script Lab step 7 (issue 293): what a script run (Live, `lsl_run`, `lsl_event`) does shows in the 3D view, only while the panel's
"Show effects in 3D view" box is ticked (off by default): `llStartAnimation` / `llStopAnimation` play or stop a
bundled animation on a preview figure (the user's animation is never changed), and worn objects and RLV locks
appear as small markers at their attachment points.
