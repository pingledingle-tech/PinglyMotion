Branch: test/generated-edge-bvh
Status: active
Pull request: pending
Files:
- tools/gen_edge_bvh.py
- tests/data/generated/**
- tests/test_gen_edge_bvh.py
- tests/animfile/CMakeLists.txt
- tests/animfile/tst_generated.cpp
- fuzz/corpus/bvh/**
- .github/workflows/build.yml
- docs/work/test-generated-edge-bvh.md

## What

Mesh2Motion job 2 (issue 27): a small Python generator (standard library only) that writes edge-case BVH files (one frame, the frame limit, extreme rotations, a turn across 180 degrees, big root travel, the smallest and largest frame time, a loop of one frame) into tests/data/generated/, with a test that loads each file and checks that it is accepted or refused for the reason written in the README there.
