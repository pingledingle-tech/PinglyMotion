Branch: feat/mesh2motion-converter
Status: done
Pull request: #100
Files:
- tools/glb2sl_bvh.cpp
- tests/gltfimport/CMakeLists.txt
- tests/gltfimport/tst_mesh2motion.cpp
- tests/data/mesh2motion/**
- fuzz/corpus/gltf/**
- docs/work/feat-mesh2motion-converter.md
- docs/PRODUCT.md
- src/gltfimport.cpp
- src/gltfimport.h
- tests/gltfimport/tst_gltfimport.cpp
- fuzz/gltf.dict

## What

Mesh2Motion steps 2 and 3 (issue 27): a converter tool that turns a Mesh2Motion humanoid GLB clip into a Second Life BVH file (30 fps) with the glTF reader and the retargeting core, and tests that read the eight chosen CC0 clips. The clips are compressed with meshopt, so the glTF reader now decodes that (animation data only, with limits checked first). The converted clips are not added to the test motions yet.
