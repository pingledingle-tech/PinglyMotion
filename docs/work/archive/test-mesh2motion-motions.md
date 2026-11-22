Branch: test/mesh2motion-motions
Status: done
Pull request: #114
Files:
- docs/work/test-mesh2motion-motions.md
- tests/motions/mesh2motion/**
- tests/motions/README.md
- tests/gltfimport/CMakeLists.txt
- tests/gltfimport/tst_m2mmotions.cpp
- tests/data/mesh2motion/README.md

## What

Step 4 of the test-motion plan: the eight Mesh2Motion clips (CC0), converted to Second Life BVH with the merged converter, in `tests/motions/mesh2motion/` so the Windows zip carries them in `test-motions/`. A test loads each file and checks frame count, joint count and finite values.
