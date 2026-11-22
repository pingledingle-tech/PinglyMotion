Branch: job/ux-test-helper
Status: done
Pull request: #359
Files:
- tests/uirender/**
- CMakeLists.txt
- docs/DEV.md

## What

Test helper that draws any panel without a screen (Qt offscreen) in the program's own theme (dark and light, 100 and 200 percent text size), checks token colours and clipping, and saves the images to a folder given by `PINGLYMOTION_UI_OUT`. First user: a new test on the transport bar. Tests only; no change to the program.
