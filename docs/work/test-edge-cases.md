Branch: test/edge-cases
Status: done
Pull request: #166
Files:
- tools/gen_edge_motion.py
- tests/test_gen_edge_motion.py
- tests/data/generated/**
- tests/animfile/tst_generated_motion.cpp
- tests/animfile/CMakeLists.txt
- fuzz/corpus/dingle/**
- fuzz/corpus/anim/**
- .github/workflows/build.yml
- docs/DEV.md

## What

Part of #27: generated edge-case files in the .dingle and .anim formats (all six easings, many keys, loop points at both ends, odd frame rates, 60 s, gimbal angles, 216 joints, .anim at the 250,000 byte limit), written by a reproducible script, with exact assertions in a test. The BVH edge cases already exist.
