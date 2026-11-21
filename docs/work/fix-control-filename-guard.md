Branch: fix/control-filename-guard
Status: active
Pull request: pending
Files:
- src/controlapi.*
- tests/tst_control.cpp
- docs/CONTROL.md
- SECURITY.md

## What

Issue 244: open_file, save_file and export_anim refuse unusual file names (network names, drive letters, ':', '..', device names, absolute paths outside the work folder) by their text alone, before any file system call.
