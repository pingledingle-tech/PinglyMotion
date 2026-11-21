Branch: feat/script-lab-panel
Status: active
Pull request: pending
Files:
- src/scriptlab/rlvpanel.*
- src/scriptlab/rlvstate.* (small accessor only, if needed)
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/pinglymotionwindow.ui
- src/panellayout.h
- src/panelvisibility.h
- src/controlapi.* (shared: rlv_* only, if needed)
- src/CMakeLists.txt (shared, minimal)
- tests/CMakeLists.txt (shared, minimal)
- tests/tst_rlvpanel.cpp
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md (shared, minimal)

## What

Script Lab step 2 (issue 288): the hidden "Script Lab" panel under the View menu. Input line for RLV `@` commands, the
active restrictions per object, the actions shown allowed or blocked (text and colour), the reply lines, and a Reset
button. Follows the RLV core's changed signal, so commands sent through the control API show up too.
