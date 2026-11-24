Branch: test/lslworld-asan-check
Status: active
Pull request: #452
Files:
- tests/tst_lslworld.cpp

## What

Test stability job T3: find why the `lslworld` test failed in the `asan-ubsan` check on two branches on the night of 22 November 2026, and fix it if it is still a problem. No code change if the cause is already gone.
