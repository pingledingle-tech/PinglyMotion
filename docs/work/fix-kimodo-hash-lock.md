Branch: fix/kimodo-hash-lock
Status: active
Pull request: pending
Files:
- tools/kimodo/**
- tests/test_kimodo_helper.py
- .github/workflows/kimodo-lock.yml
- .github/workflows/build.yml

## What

Issue 247: the Kimodo helper installs from a hash-locked requirements file (--require-hashes) and downloads the Kimodo model at a pinned Hugging Face revision. The lock and the model revision are made by a manual workflow on a GitHub runner (the AI sandboxes cannot reach the package and model hosts); a pull request check validates the lock and proves that a changed hash makes the install fail.
