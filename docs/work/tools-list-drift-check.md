Branch: tools/list-drift-check
Status: active
Pull request: pending
Files:
- tools/work.py
- tests/test_work.py
- .github/workflows/work-board.yml
- CLAUDE.md

## What

Adds `work.py jobs-draft` (prints a draft of the jobs list from job files and open pull requests) and `work.py check-lists` (warns when the jobs list in issue 74 disagrees with reality), a warning-only CI step, a unit test and a short CLAUDE.md line. Nothing is ever written to GitHub.
