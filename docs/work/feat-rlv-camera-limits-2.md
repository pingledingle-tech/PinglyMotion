Branch: feat/rlv-camera-limits-2
Status: active
Pull request: pending
Files:
- src/camera.*
- src/scriptlab/cameraoverride.*
- src/scriptlab/scriptlabpanel.cpp
- src/animationview.cpp
- tests/tst_camera.cpp
- tests/tst_cameraoverride.cpp
- tests/tst_scriptlabcamera.cpp
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md

## What

Script Lab job 7b, part 2: the RLV field of view limits (`setcam_fovmin`, `setcam_fovmax`, and the old zoom limits `camzoommin`, `camzoommax`) also limit the 3D view while the "Show camera limits in 3D view" box is ticked, instead of the fixed 60 degrees. The text under the box names them. Off and exactly restored as in part 1.
