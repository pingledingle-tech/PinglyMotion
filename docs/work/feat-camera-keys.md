Branch: feat/camera-keys
Status: active
Pull request: #1
Files:
- src/animationview.cpp
- src/animationview.h
- src/camera.cpp
- src/camera.h
- tests/tst_camera.cpp
- tests/CMakeLists.txt
- README.md
- docs/POSING_UX.md

## What

Step 1 of the posing UX (issue 14): one simple mouse and key scheme for the camera, with Blender and Second Life habits also working; the old joint modifier keys are removed. The Windows test found that the camera cannot pan further down when it looks from floor level; that is fixed in this pull request.
