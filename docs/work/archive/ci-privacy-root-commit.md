Branch: ci/privacy-root-commit
Status: done
Pull request: #37
Files:
- tools/check_privacy.py
- tests/test_check_privacy.py

## What

`tools/check_privacy.py` could not check a first commit (no parent, so no base to compare with). An empty base, forty zeros or git's empty tree now means "check everything up to head". Three new tests.
