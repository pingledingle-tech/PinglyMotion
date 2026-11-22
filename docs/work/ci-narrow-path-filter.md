Branch: ci/narrow-path-filter
Status: active
Pull request: pending
Files:
- .github/workflows/build.yml
- .github/workflows/codeql.yml
- tools/ci/changes.py
- tests/test_ci_changes.py
- CLAUDE.md
- docs/DEV.md

## What

Widen the "no program change" set of the CI path filter: website-only changes (issue templates, `pages.yml`, `tools/site/`, `tests/test_build_status.py`) no longer start the Linux, Windows and sanitizer builds or CodeQL on the C++ program. The decision moves into a small tested script; the required checks still report (as skipped) exactly as before.
