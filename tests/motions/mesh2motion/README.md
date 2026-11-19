# Mesh2Motion test motions

Eight humanoid clips, converted to Second Life BVH. In the Windows zip they are in `examples/mesh2motion/` next to the program: File > Open, pick a file, press play.

Test motions from Mesh2Motion (https://github.com/Mesh2Motion/mesh2motion-app), CC0 1.0 – thank you to the Mesh2Motion project.

| File | What the figure does | Frames |
|---|---|---|
| `m2m_idle-a.bvh` | stands and breathes | 94 |
| `m2m_walk.bvh` | walks on the spot (does not travel forward) | 51 |
| `m2m_jog.bvh` | jogs on the spot | 37 |
| `m2m_jump-2.bvh` | jumps | 52 |
| `m2m_sitting-idle.bvh` | sits and moves a little | 63 |
| `m2m_dance-simple.bvh` | a simple dance | 38 |
| `m2m_greeting.bvh` | greets, waves | 149 |
| `m2m_yes.bvh` | nods "yes" | 76 |

Frame 0 is the Second Life rest pose (the reference frame Second Life does not play); the frame counts above include it. 30 frames per second, 19 joints (the classic Second Life skeleton), body only.

## Licence and source

CC0 1.0 Universal (public domain dedication). The Mesh2Motion repository says in `LICENSE-CC0.MD` that all its 3D models, rigs and animations are CC0 1.0. CC0 does not ask for credit; the credit above is given on purpose.

Source: commit `bcb014ea25e44f76b3add78463f72d1faec16418`, folder `static/assets/human/animations/`. The original GLB files are in `tests/data/mesh2motion/` (not in the zip). Nothing from that repository's `static/test-files/` folder is used (it holds a Mixamo file that is not CC0).

## Make the files again

```
cmake -S . -B build -DPINGLYMOTION_TESTS=ON
cmake --build build --target glb2sl_bvh
build/tests/gltfimport/glb2sl_bvh tests/data/mesh2motion/walk.glb tests/motions/mesh2motion/m2m_walk.bvh
```

The same for the other clips (`idle-a`, `jog`, `jump-2`, `sitting-idle`, `dance-simple`, `greeting`, `yes`). The test checks that the files load, not that they equal a fresh conversion.
