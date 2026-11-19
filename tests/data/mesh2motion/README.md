# Mesh2Motion test clips

Eight humanoid animation clips used by the automatic tests (`tests/gltfimport/tst_mesh2motion.cpp`) and by the converter `tools/glb2sl_bvh.cpp` (issue 27). They are **test data only**. Two of them (`walk.glb`, `greeting.glb`) are also copied into `tests/motions/`, so the Windows zip holds them for the hand test of the glTF menu; the rest are not in the zip.

Test motions from Mesh2Motion (https://github.com/Mesh2Motion/mesh2motion-app), CC0 1.0 – thank you to the Mesh2Motion project.

| File | Shows |
|---|---|
| `idle-a.glb` | standing idle |
| `walk.glb` | normal walk (on the spot) |
| `jog.glb` | jog |
| `jump-2.glb` | jump |
| `sitting-idle.glb` | seated idle |
| `dance-simple.glb` | simple dance |
| `greeting.glb` | greeting / wave |
| `yes.glb` | nod, "yes" gesture |

## Source and licence

- Repository: https://github.com/Mesh2Motion/mesh2motion-app, commit `bcb014ea25e44f76b3add78463f72d1faec16418`, folder `static/assets/human/animations/`. The files are unchanged copies.
- Licence: CC0 1.0 Universal. `LICENSE-CC0.MD` in the repository root says that all 3D models, blend files, rigs and animations are CC0 1.0 (public domain dedication).
- **Never use `static/test-files/` of that repository.** It holds sample rigs of other origin (including a Mixamo one) that are not CC0.

## What is in the files

Each file is a GLB with the Mesh2Motion humanoid skeleton and one animation. The animation data is compressed with `EXT_meshopt_compression` (mode ATTRIBUTES, version 0, filters QUATERNION and EXPONENTIAL); `src/gltfimport.cpp` decodes it. The clips are in place (no travel), so a walk bobs and sways but does not move forward.

## Convert one clip to Second Life BVH

```
cmake -S . -B build -DPINGLYMOTION_TESTS=ON
cmake --build build --target glb2sl_bvh
build/tests/gltfimport/glb2sl_bvh tests/data/mesh2motion/walk.glb walk.bvh
```

The tool uses `bin/data/TPose.bvh` as the Second Life skeleton, writes 30 frames per second, and puts the rest pose in front as frame 0 (Second Life does not play frame 0). The converted clips are in `tests/motions/mesh2motion/`.

Three of the files (`walk`, `sitting-idle`, `yes`) are also start files of the glTF fuzz target, in `fuzz/corpus/gltf/` as `m2m_*.glb`.
