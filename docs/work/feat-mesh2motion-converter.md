Branch: feat/mesh2motion-converter
Status: active
Pull request: pending
Files:
- tools/glb2sl_bvh.cpp
- tests/gltfimport/CMakeLists.txt
- tests/gltfimport/tst_mesh2motion.cpp
- tests/data/mesh2motion/**
- fuzz/corpus/gltf/**
- docs/work/feat-mesh2motion-converter.md
- docs/DEV.md

## What

Mesh2Motion steps 2 and 3 (issue 27): a converter tool that turns a Mesh2Motion humanoid GLB clip into a Second Life BVH file (30 fps) with the glTF reader and the retargeting core, and tests that read the eight chosen CC0 clips. The converted clips are not added to the test motions yet.
