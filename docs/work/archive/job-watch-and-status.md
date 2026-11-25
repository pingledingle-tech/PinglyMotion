Branch: job/watch-and-status
Status: done
Pull request: #485
Files:
- .github/workflows/watch.yml
- .github/workflows/build.yml
- .github/workflows/release.yml
- .github/workflows/reference-fetch.yml
- .github/workflows/work-board.yml
- tools/watch.py
- tools/work.py
- tests/test_watch.py
- tests/test_work.py
- docs/DEV.md

## What

A watch that GitHub runs by itself (no AI): reruns stuck checks once, comments on stuck, skipped or red pull requests and on a red main. Time limits on every job without one. A short `work.py status` command.
