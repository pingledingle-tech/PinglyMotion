Branch: feat/rlv-camera-limits-1
Status: done
Pull request: #313
Files:
- src/camera.*
- src/scriptlab/cameraoverride.*
- src/scriptlab/scriptlabpanel.*
- src/animationview.*
- src/pinglymotionwindow.cpp
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/tst_camera.cpp
- tests/tst_cameraoverride.cpp
- tests/tst_scriptlabcamera.cpp
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md

## What

Script Lab job 7b, part 1: the RLV camera distance limits (`setcam_avdistmin`, `setcam_avdistmax`) and `setcam_unlock` held in the RLV state can be shown in the main 3D view as an override. Off by default, switched on with a box in the Script Lab panel that also says in words what limits the camera; switching it off or the restrictions going away puts back exactly the camera from before. Field of view and forced focus come in later pull requests.
