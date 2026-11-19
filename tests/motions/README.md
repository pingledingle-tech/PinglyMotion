# Test animations

Animations with real movement for automatic tests (issue #153).

## From the CMU motion capture database

| File | CMU clip | What | Length |
|---|---|---|---|
| `cmu_02_01_walk.bvh` | 02_01 | walk | 2.8 s |

Converted with `tools/cmu_to_sl.py` (defaults: 30 fps, whole clip) to the classic Second Life skeleton (19 joints, from `bin/data/Relaxed.bvh`). Frame 0 is the SL rest pose, the reference frame Second Life compares against. Body only: no fingers, toes or face.

To make a file again:

```
python3 tools/cmu_to_sl.py <cmu>/data/002/02_01.bvh tests/motions/cmu_02_01_walk.bvh
```

Source: the cgspeed.com BVH conversion (2010 release) of the CMU Graphics Lab Motion Capture Database, <http://mocap.cs.cmu.edu>.

Usage rights, from the conversion's READMEFIRST: CMU places no restrictions on the use of the original dataset, and the BVH conversion adds no further restrictions. CMU asks for this acknowledgment:

> The data used in this project was obtained from mocap.cs.cmu.edu.
> The database was created with funding from NSF EIA-0196217.

## A skeleton that is not the avatar's

| File | What | Length |
|---|---|---|
| `mixamo_style_wave.bvh` | The right arm goes up and waves, the head nods. A person skeleton with Mixamo style joint names (23 joints, one extra twist bone). | 2 s |

Made by hand for the menu "Import BVH from another skeleton" (issue 16): the program has to match the joint names to the avatar, and it leaves the twist bone out. No outside data, no licence terms. The test `tests/retargetdialog` opens it.

## glTF clips

| File | What | Length |
|---|---|---|
| `walk.glb` | A normal walk, on the spot. | 1.7 s |
| `greeting.glb` | A greeting with a wave. | 4.9 s |

Unchanged copies of two of the eight test clips in `tests/data/mesh2motion`, from Mesh2Motion (https://github.com/Mesh2Motion/mesh2motion-app), CC0 1.0. They are here so the Windows zip holds them for the hand test of the menu "Import glTF animation" (issue 12). Thank you to the Mesh2Motion project.
