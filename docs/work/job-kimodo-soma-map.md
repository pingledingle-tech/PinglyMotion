Branch: job/kimodo-soma-map
Status: active
Pull request: #174
Files:
- src/retarget.*
- tools/gen_soma77_bvh.py
- tests/test_gen_soma77_bvh.py
- tests/data/generated/soma77*
- tests/retargetsoma/**
- CMakeLists.txt
- .github/workflows/build.yml
- docs/RETARGET_MAPPING.md
- docs/research/TEXT_TO_MOTION.md

## What

Ready joint mapping for the SOMA 77-joint BVH skeleton that NVIDIA Kimodo writes
(text-to-motion, issue 18, job 1 of docs/research/TEXT_TO_MOTION.md): detected
automatically on import, with a script-generated fixture (joint names and tree
only, no motion data) and exact tests.
