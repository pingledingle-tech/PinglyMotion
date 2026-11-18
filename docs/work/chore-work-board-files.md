Branch: chore/work-board-files
Status: done
Pull request: #5
Files:
- CLAUDE.md
- docs/WORK.md
- docs/work/README.md
- docs/work/HISTORY.md
- tools/work.py
- tests/test_work.py
- .github/workflows/work-board.yml

## What

Replace the shared table in `docs/WORK.md` by one small file per job in
`docs/work/`, plus `tools/work.py` that lists the jobs and warns when two
active jobs claim the same files.
