Branch: feat/script-lab-panel
Status: done
Pull request: #298
Files:
- src/scriptlab/scriptlabpanel.*
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/CMakeLists.txt (shared, minimal)
- CMakeLists.txt (shared, minimal)
- tests/scriptlabpanel/**
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md (shared, minimal)

## What

Script Lab step 2 (issue 288): the hidden "Script Lab" panel under the View menu. Input line for RLV `@` commands, the
active restrictions per object, the actions shown allowed or blocked (text and colour), the reply lines, and a Reset
button. Follows the RLV core's changed signal, so commands sent through the control API show up too.
