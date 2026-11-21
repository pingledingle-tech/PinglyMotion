Branch: fix/save-new-names-safely
Status: active
Pull request: pending
Files:
- src/controlapi.cpp
- tests/tst_control.cpp

## What

Issue 248, second part: save_file makes a new file name with NewOnly before the program writes it, so a file that appears between the check and the save is never replaced; a failed save leaves no empty file. The first part (load_reference parses the checked bytes) needs the window's control host, which an open pull request is changing, so it is left for later.
