Branch: fix/frame-after-open-187
Status: active
Pull request: #209
Files:
- src/controlapi.cpp
- src/controlapi.h
- tests/tst_control.cpp
- tests/assistant/cases.py
- tests/assistant/lib.py
- tests/assistant/run_golden.py
- docs/DEV.md

## What

Control commands that take a frame always land on that frame, also right after `open_file` when the window's frame slider still shows the old animation's frame (issue 187). Removes the eval-set workarounds in `cases.opening` and `lib.refresh`.
