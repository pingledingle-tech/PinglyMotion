Branch: feat/gltf-import
Status: active
Pull request: #69
Files:
- src/gltfimport.*
- tests/gltfimport/**
- CMakeLists.txt
- fuzz/fuzz_gltf.cpp
- fuzz/corpus/gltf/**
- fuzz/CMakeLists.txt
- fuzz/README.md

## What

Step 1 of #12: a safe glTF 2.0 reader (`.glb` and embedded `.gltf`, from
memory, no external files or URLs), reading the skin and the rotation and
translation channels, sampled to a frame rate and handed to the retargeting
core (#66) to give an `AnimFile::Animation` on the SL skeleton. Tests with
hand-built data and a fuzz target `fuzz_gltf`. No menu, no visible change.
