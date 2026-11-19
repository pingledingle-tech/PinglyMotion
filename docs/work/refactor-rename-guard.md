Branch: refactor/rename-guard
Status: done
Pull request: #99
Files:
- docs/work/refactor-rename-guard.md
- tools/check_old_name.py
- tests/test_check_old_name.py
- .github/workflows/privacy.yml
- docs/DECISIONS.md
- docs/DEV.md
- SECURITY.md
- bin/NEWS
- bin/data/character/**
- bin/data/firestorm/README.md
- tools/firestorm/README.md

## What

Rename job 6 of 6. A quick check that fails when the old name comes back in tracked files or in the commit messages of a pull request, a decision entry for the rename, and the last leftovers of the old name in texts.
