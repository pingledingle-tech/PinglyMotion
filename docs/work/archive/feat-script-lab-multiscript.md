Branch: feat/script-lab-multiscript
Status: done
Pull request: #306
Files:
- src/scriptlab/lsl/*
- tests/tst_lslworld.cpp
- tests/tst_lsllink.cpp
- tests/CMakeLists.txt
- tests/tst_lslinterp.cpp
- tests/tst_control.cpp
- fuzz/fuzz_lsl.cpp
- fuzz/lsl.dict
- fuzz/control.dict
- fuzz/corpus/lsl/**
- fuzz/corpus/control/26.json
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md
- docs/SCRIPT_LAB_PLAN.md
- src/controlapi.cpp
- tools/mcp/commands.json
- docs/DECISIONS.md
- docs/work/feat-script-lab-multiscript.md

## What

Script Lab step 9a (issue 305): several scripts in one object in the LSL sandbox, link messages
(`llMessageLinked`, `link_message`, `llGetLinkNumber`) and Linkset Data (`llLinksetData*` and the
`linkset_data` event). `lsl_run`'s `objects` entries take `scripts`; the old shape keeps working.
Technical, no owner test. After the Script Lab 3D job and the step 8 docs job were merged, this job
also updated the lsl_run tool text and schema (src/controlapi.cpp, tools/mcp/commands.json) and
docs/SCRIPT_LAB.md, docs/ROADMAP.md and docs/SCRIPT_LAB_PLAN.md.
