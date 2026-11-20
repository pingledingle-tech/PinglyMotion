Branch: feat/xra-jointmap
Status: active
Pull request: pending
Files:
- src/retarget.*
- src/retargetmapping.*
- src/retargetdialog.cpp
- bin/data/jointmaps/**
- tools/gen_xr_animator_bvh.py
- tests/retarget/**
- tests/test_gen_xr_animator_bvh.py
- tests/data/generated/xr_animator*
- tests/CMakeLists.txt
- docs/RETARGET_MAPPING.md

## What

Ready joint map for the BVH files that XR Animator (webcam/video motion capture) writes:
its skeleton (VRM humanoid bone names) is recognised on BVH import and mapped onto the
classic Second Life joints, Bento spine left alone. Test with a script-generated BVH that
has the same joint names and tree (no third-party motion data). Part of issue 17
(VIDEO_TO_MOTION.md job 1, joint map part).
