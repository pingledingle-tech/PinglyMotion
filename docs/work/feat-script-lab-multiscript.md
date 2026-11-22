Branch: feat/script-lab-multiscript
Status: done
Pull request: #306
Files:
- src/scriptlab/lsl/*
- tests/tst_lslworld.cpp
- tests/tst_lsllink.cpp
- tests/CMakeLists.txt (only the LSL test list line)
- tests/tst_lslinterp.cpp
- tests/tst_control.cpp
- fuzz/fuzz_lsl.cpp
- fuzz/lsl.dict
- fuzz/control.dict
- fuzz/corpus/lsl/**
- fuzz/corpus/control/26.json
- docs/CONTROL.md
- docs/DECISIONS.md
- docs/work/feat-script-lab-multiscript.md

## What

Script Lab step 9a (issue 305): several scripts in one object in the LSL sandbox, link messages
(`llMessageLinked`, `link_message`, `llGetLinkNumber`) and Linkset Data (`llLinksetData*` and the
`linkset_data` event). `lsl_run`'s `objects` entries take `scripts`; the old shape keeps working.
Technical, no owner test. The edits to docs/SCRIPT_LAB.md, docs/ROADMAP.md, docs/SCRIPT_LAB_PLAN.md,
src/controlapi.cpp and tools/mcp/commands.json (the lsl_run tool text and schema) were left for a
follow-up job, because the Script Lab 3D pull request, still open, changes the same lines.
