Branch: feat/lsl-events-objects
Status: active
Pull request: #303
Files:
- src/scriptlab/lsl/**
- src/scriptlab/scriptlabpanel.*
- src/controlapi.*
- src/CMakeLists.txt
- tests/tst_lslworld.cpp
- tests/tst_lslinterp.cpp
- tests/tst_control.cpp
- tests/CMakeLists.txt
- tests/scriptlabpanel/**
- tests/test_mcp_node.py
- tools/mcp/commands.json
- fuzz/fuzz_lsl.cpp
- fuzz/lsl.dict
- fuzz/corpus/lsl/**
- examples/scriptlab/**
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md

## What

Script Lab step 5 (issue 291): several objects in one sandbox world, each with its
own id, owner, name, position, listeners and timers; chat between them on channels
with ranges; injected events (`lsl_event`); a simulated clock so runs are virtual
time and deterministic; per-object RLV restrictions. The panel lists the objects
and shows the last dialog. Folder watching stays with feat/script-lab-live.
