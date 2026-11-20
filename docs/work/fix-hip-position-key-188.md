Branch: fix/hip-position-key-188
Status: active
Pull request: pending
Files:
- src/bvh.cpp
- tests/tst_roundtrip.cpp
- tests/assistant/cases.py
- docs/DEV.md

## What

A hip position key without a hip rotation key at the same frame keeps its value when a `.dingle` is opened again (issue 188). Removes the eval-set workaround in `cases.hip`.
