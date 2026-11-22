Branch: fix/flaky-windows-control-test
Status: done
Pull request: #346
Files:
- tests/tst_control.cpp
- tests/CMakeLists.txt
- src/controlserver.*
- src/CMakeLists.txt
- docs/CONTROL.md

## What

The ctest test `control` fails now and then on the Windows CI job and passes on a re-run. Find the cause from the CI logs and make the test (or the control server, if the race is there) deterministic.
