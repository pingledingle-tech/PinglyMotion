Branch: job/tooling-work-status
Status: active
Pull request: pending
Files:
- tools/work.py
- tests/test_work.py
- tests/CMakeLists.txt
- docs/DEV.md

## What

Approval: the project owner, 22 Nov 2026, 22:27: «ta det trygge». Queue item 1, lane 1 in the jobs list (issue 74).

Adds a read-only `python3 tools/work.py status` that prints, for every open pull request, draft or ready, merge conflict state, the latest run of each check and minutes since the last push. Also a comment at the top of `tests/CMakeLists.txt` about linking `pinglymotion_shared` instead of listing files again. No new CI check, nothing under `.github/workflows/`.
