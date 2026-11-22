Branch: feat/rlv-table-2-camera-aliases
Status: done
Pull request: #316
Files:
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/rlvstate.*
- tests/tst_rlvtable.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- fuzz/rlv.dict
- docs/work/feat-rlv-table-2-camera-aliases.md

## What

Script Lab RLV table, part 2 (part of issue 294). Old names share the state of the new name (camdistmax, camdistmin, camavdist, camtextures, camunlock, fartouch); getstatus shows the name that was used. camzoommax and camzoommin are recorded-only entries; touchfar is added. Exact-answer tests in the new tst_rlvtable.cpp; docs updated.
