Branch: ci/codeql-skip-unchanged
Status: done
Pull request: #356
Files:
- .github/workflows/codeql.yml
- tools/ci/changes.py
- tests/test_ci_changes.py
- docs/DECISIONS.md

## What

On pull requests, the CodeQL Python analysis runs only when a `.py` file changed and the Actions analysis only when a file under `.github/workflows/` or `.github/actions/` changed, decided by `tools/ci/changes.py` like the C++ analysis. The required `analyze` check still reports; nightly, manual and release runs still analyse everything. Approved by the project owner on 22 Nov 2026 as extra scope of the process review.
