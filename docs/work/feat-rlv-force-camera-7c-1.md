Branch: feat/rlv-force-camera-7c-1
Status: active
Pull request: #329
Files:
- src/scriptlab/rlvstate.*
- src/scriptlab/cameraoverride.*
- src/scriptlab/scriptlabpanel.cpp
- tests/tst_cameraoverride.cpp
- tests/tst_rlvtable.cpp
- tests/tst_scriptlabcamera.cpp
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md
- docs/DECISIONS.md

## What

Job 7c, part 1 (issue 294): the RLV state tells the rest of the program when a force command changes a recorded
value (setcam_fov, setcam_focus, setcam_mode, the offsets sent as =force, setenv_*, setdebug_*), and the camera
override from job 7b ("Show camera limits in 3D view") follows a forced field of view, a forced focus on the avatar
and the forced offsets. Mouselook stays recorded only. Exact-answer tests.
