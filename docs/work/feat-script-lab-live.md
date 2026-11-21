Branch: feat/script-lab-live
Status: active
Pull request: pending
Files:
- docs/work/feat-script-lab-live.md
- src/scriptlab/labwatcher.*
- src/scriptlab/scriptlabpanel.*
- src/scriptlab/scriptview.*
- src/controlapi.*
- src/pinglymotionwindow.cpp
- src/CMakeLists.txt
- tests/tst_labwatcher.cpp
- tests/tst_control.cpp
- tests/CMakeLists.txt
- tests/scriptlabpanel/**
- tests/test_mcp_node.py
- tools/mcp/commands.json
- fuzz/CMakeLists.txt
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/ROADMAP.md
- docs/TEACHING_TEXTS.md

## What

Script Lab step 6 (issue 292), "Live": the Script Lab watches a folder the user
picks (read only), and when a `.lsl` file in it changes it checks and runs it
again and shows the script with line numbers and error marks, the errors with
line and column, the event log and the RLV state in the panel. Control commands
`lab_watch`, `lab_unwatch` and `lab_result` let an AI client edit a file on disk
and read the outcome. Bounded file size and count, symlinks not followed out of
the folder, the interpreter's limits plus its real-time cap.
