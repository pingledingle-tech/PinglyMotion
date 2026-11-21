Branch: docs/self-test-visible
Status: done
Pull request: #281
Files:
- docs/work/docs-self-test-visible.md
- CLAUDE.md
- docs/DEV.md

## What

A session tests a visible change itself (sandbox build, offscreen GUI, control commands, screenshots, exact tests) before asking the project owner, and writes in the pull request what it did and saw. New section in `docs/DEV.md`. CLAUDE.md (Test page rules, With every merge): tests live only in the test page store (`active`) and in the owner test list in the same step; "Approve and archive" on the page; a note without a tick is a problem.
