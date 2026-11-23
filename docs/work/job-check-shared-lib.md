Branch: job/check-shared-lib
Status: done
Pull request: #440
Files:
- tools/check_shared_lib.py
- tests/test_check_shared_lib.py
- .github/workflows/build.yml
- tests/CMakeLists.txt
- docs/DEV.md

## What

Adds a check that fails when a test lists a source file of the shared test library (`PINGLYMOTION_SHARED_SRC`) in `ADD_EXECUTABLE` instead of linking `pinglymotion_shared`. Runs in the Linux job. Also fixes the top comment of `tests/CMakeLists.txt`. Technical job, nothing visible changes.
