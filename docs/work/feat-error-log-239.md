Branch: feat/error-log-239
Status: done
Pull request: #255
Files:
- src/applog.*
- src/main.cpp
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/applog/**
- docs/DEV.md

## What

Issue 239, step 1: a local error log. One logger on Qt's message handler and the
existing logging categories writes every message to a file in the application data
folder (flushed line by line, 5 MB cap with the oldest lines dropped, home folder
replaced by a placeholder), catches uncaught exceptions and fatal signals (best
effort), and notes at start when the last session did not close cleanly. Unit tests
for the cap, the filter and the crash detection. The Help menu entries come in step 2.
