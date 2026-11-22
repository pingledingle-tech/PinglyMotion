Branch: ci/label-auto-merge
Status: done
Pull request: #348
Files:
- .github/workflows/auto-merge.yml
- .github/workflows/build.yml
- tools/ci/auto_merge.py
- tests/test_ci_auto_merge.py
- CLAUDE.md
- docs/DEV.md
- docs/DECISIONS.md

## What

A small workflow that squash-merges an open technical pull request carrying the
`auto-merge` label once every required check of the main ruleset is green
(the project owner's choice, 22 Nov 2026). The main ruleset is unchanged. The
decision logic lives in `tools/ci/auto_merge.py` with a test.
