Branch: feat/rlv-camera-limits-3
Status: active
Pull request: #322
Files:
- src/camera.*
- src/scriptlab/cameraoverride.*
- tests/tst_camera.cpp
- tests/tst_cameraoverride.cpp
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md

## What

Script Lab job 7b, part 3: the RLV camera offsets held as restrictions (`setcam_focusoffset`, `setcam_eyeoffset`, `setcam_eyeoffsetscale`) also place the 3D view's camera while the "Show camera limits in 3D view" box is ticked: the focus point moves by the offset from the avatar, and an eye offset holds the camera at that place. Named in words under the box; off and exactly restored as in parts 1 and 2.
