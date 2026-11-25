Branch: job/split-script-lab-panel
Status: active
Pull request: pending
Files:
- src/scriptlab/scriptlabpanel.cpp
- src/scriptlab/scriptlabpanel*.cpp
- src/scriptlab/scriptlabpanelui.h
- src/CMakeLists.txt
- docs/DEV.md
- docs/SCRIPT_LAB.md
- docs/TEACHING.md
- docs/PLAYER_CHECK.md
- docs/EXAMPLES.md

## What

Code tidy, part 1: split the Script Lab panel source file (`src/scriptlab/scriptlabpanel.cpp`, 3968 lines) into six files by responsibility. A pure move: same behaviour, same tests, nothing the user sees changes. Approved by the project owner, 24 November 2026, 23:20: «Ja til alt som er lurt».
