Branch: feat/reference-commands-19
Status: active
Pull request: pending
Files:
- src/controlapi.*
- tests/tst_control.cpp
- tools/mcp/commands.json
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md
- docs/REFERENCE_LIBRARY_PLAN.md
- docs/work/feat-reference-commands-19.md

## What

Job 3, first part, of the reference library plan (issue 19): control commands `find_reference` (search the library index by text, tags and duration) and `load_reference` (open a library clip by id). Library files only, no arbitrary paths. Also covers clips listed in the index written by `tools/fetch_reference.py`, marked as downloaded. No browser panel.
