Branch: job/scriptlab-chain-lines
Status: done
Pull request: #455
Files:
- src/chainlines.h
- src/scriptlab/labchains.*
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/scriptlabpanel.*
- src/animationview.*
- src/pinglymotionwindow.cpp
- src/controlapi.cpp
- src/CMakeLists.txt
- tools/mcp/commands.json
- tests/tst_labchains.cpp
- tests/CMakeLists.txt
- tests/scriptlabbeginner/tst_scriptlabbeginner.cpp
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md
- docs/DESIGN_SYSTEM.md
- docs/TEACHING.md

## What

Job A4, part 2 (part 1, the channel map, was pull request 453): a straight line in the 3D view
between two parts when a script links them with a chain or ribbon (llParticleSystem or
llLinkParticleSystem with PSYS_SRC_TARGET_KEY to another prim of the object), red and the word
Locked while the object holds @detach, green, dashed and the word Free otherwise. No physics.
