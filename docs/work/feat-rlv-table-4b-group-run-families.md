Branch: feat/rlv-table-4b-group-run-families
Status: done
Pull request: #324
Files:
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/rlvstate.*
- tests/tst_rlvtable.cpp
- docs/SCRIPT_LAB.md
- fuzz/rlv.dict
- docs/work/feat-rlv-table-4b-group-run-families.md

## What

Script Lab RLV table, part 4b (part of issue 294). setgroup, temprun, alwaysrun, jump, standtp, accepttp and the force commands setrot (RLV only), adjustheight, tpto and sitground; fixed sandbox answers for getsitid, getgroup, getoutfit, getattach and empty answers for the folder questions; setdebug_, setenv_, getdebug_ and getenv_ as prefix families (recorded only, at most 256 entries). Exact-answer tests in tst_rlvtable.cpp; docs updated.
