Branch: fix/flaky-windows-control-test
Status: active
Pull request: pending
Files:
- tests/tst_control.cpp
- src/controlserver.*
- docs/DEV.md

## What

The ctest test `control` fails now and then on the Windows CI job and passes on a re-run. Find the cause from the CI logs and make the test (or the control server, if the race is there) deterministic.
