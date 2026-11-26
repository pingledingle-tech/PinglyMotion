Branch: feat/scriptlab-big-setups
Status: active
Pull request: #519
Files:
- src/scriptlab/labfolder.*
- tests/tst_labfolder.cpp
- docs/SCRIPT_LAB.md
- docs/CONTROL.md

## What

Script Lab: a monitored folder (one object) now runs for a time that grows with the number of scripts instead of a flat 10 simulated seconds, so a big collar finishes its setup. A hard upper limit stays, and the answer says how long it ran and when it still ran out.
