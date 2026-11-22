Branch: feat/rlv-table-1-plain-marks
Status: done
Pull request: #312
Files:
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/rlvstate.cpp
- tests/tst_rlva.cpp
- tests/tst_rlvcommand.cpp
- tests/tst_lslrlv.cpp
- tests/tst_control.cpp
- docs/SCRIPT_LAB.md
- docs/work/feat-rlv-table-1-plain-marks.md

## What

Script Lab RLV table, part of issue 294. Twenty behaviours that the RLV wiki lists as plain RLV (but RLVa flags as extended) were marked "RLVa only", so plain RLV mode refused them. They are now marked RLV and accepted in both modes. Tests assert the exact marks and the exact answers; the docs say the RLVa flag differs from the wiki and give the real table size (76).
