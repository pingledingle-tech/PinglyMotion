Branch: feat/xra-jointmap
Status: done
Pull request: #176
Files:
- tools/gen_xr_animator_bvh.py
- tests/data/xr_animator/**
- tests/retargetxra/**
- tests/test_gen_xr_animator_bvh.py
- tests/CMakeLists.txt

## What

Ready joint map for the BVH files that XR Animator (webcam/video motion capture) writes:
its skeleton (VRM humanoid bone names) is recognised on BVH import and mapped onto the
classic Second Life joints, Bento spine left alone. The automatic match in src/retarget.*
already gets every joint right, so no program code changes: this job pins that with tests. Test with a script-generated BVH that
has the same joint names and tree (no third-party motion data). Part of issue 17
(VIDEO_TO_MOTION.md job 1, joint map part).
