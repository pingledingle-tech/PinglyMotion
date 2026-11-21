Branch: fix/control-header-timeout
Status: done
Pull request: #261
Files:
- src/controlserver.cpp
- tests/tst_control.cpp
- docs/CONTROL.md

## What

Issue 249: a control connection must send its complete request header (and with it the key) within 2 seconds, so programs that only hold connections open cannot keep all 8 slots busy for 15 seconds each. After the key is accepted the body keeps the longer time.
