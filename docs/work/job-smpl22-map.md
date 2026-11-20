Branch: job/smpl22-map
Status: active
Pull request: pending
Files:
- src/retarget.*
- tools/gen_smpl22_bvh.py
- tests/test_gen_smpl22_bvh.py
- tests/retargetsmpl/**
- CMakeLists.txt
- .github/workflows/build.yml
- docs/RETARGET_MAPPING.md
- docs/research/TEXT_TO_MOTION.md
- CREDITS.md

## What

Ready joint mapping for the HumanML3D/SMPL 22-joint skeleton that MoMask and
similar text-to-motion tools write as BVH (issue 18, job 1 of
docs/research/TEXT_TO_MOTION.md; the SOMA 77 half was done in pull request 174):
detected automatically on import, with a script-generated fixture (joint names
and tree only, no motion data) and exact tests.
