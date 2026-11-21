Branch: fix/combined-own-prs-only
Status: active
Pull request: pending
Files:
- tools/update_combined.sh
- tests/test_update_combined.py
- .github/workflows/build.yml

## What

Issue 245: tools/update_combined.sh merges a pull request into test/combined only when it is open, comes from this repository (not a fork) and was opened by the owner account, and it merges exactly the head commit it checked. A dry-run test with a mocked API answer.
