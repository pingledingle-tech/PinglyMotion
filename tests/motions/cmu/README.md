# CMU motion capture test motions

Seven clips from the CMU Graphics Lab Motion Capture Database, converted to Second Life BVH. In the Windows zip they are in `examples/cmu/` next to the program, together with the walk clip (`cmu_02_01_walk.bvh`, see `tests/motions/README.md`): File > Open, pick a file, press play.

| File | CMU clip | What the figure does | Length |
|---|---|---|---|
| `cmu_09_01_run.bvh` | 09_01 | runs a few steps | 1.2 s |
| `cmu_13_39_jump.bvh` | 13_39 | jumps | 2.9 s |
| `cmu_13_26_wave.bvh` | 13_26, 8.4 to 11.7 s | directs traffic: arms up and waving | 3.3 s |
| `cmu_13_01_sit_down.bvh` | 13_01, 3.0 to 5.2 s | sits down on a high stool (there is no stool in the picture) | 2.2 s |
| `cmu_13_01_stand_up.bvh` | 13_01, 7.3 to 9.0 s | stands up from the stool | 1.7 s |
| `cmu_16_17_walk_turn_left.bvh` | 16_17 | walks and turns 90 degrees to the left | 4.3 s |
| `cmu_05_11_dance.bvh` | 05_11 | modern dance: sideways steps and a pirouette | 4.9 s |

Frame 0 is the Second Life rest pose (the reference frame Second Life does not play); the lengths above do not count it. 30 frames per second, 19 joints (the classic Second Life skeleton), body only. The CMU walk and run clips travel through space; the hip movement is scaled to the Second Life leg length.

## Licence and source

The data is from the CMU Graphics Lab Motion Capture Database, <http://mocap.cs.cmu.edu>. Source of the files: the cgspeed.com BVH conversion (2010 release) of that database, as copied in the public repository `una-dinosauria/cmu-mocap` (commit `09a07f54f3bbb58797325f009282d0b2048a2871`; the clips were taken from the unchanged conversion files in the folders `data/002`, `data/005`, `data/009`, `data/013`, `data/016`).

Usage rights, from the conversion's `READMEFIRST.txt`: CMU places no restrictions on the use of the original dataset (free for research and commercial projects worldwide), and the BVH conversion adds no further restrictions. CMU asks for this acknowledgment:

> The data used in this project was obtained from mocap.cs.cmu.edu.
> The database was created with funding from NSF EIA-0196217.

Thank you to the CMU Graphics Lab and to the maker of the BVH conversion.

## Make the files again

```
python3 tools/cmu_to_sl.py <cmu>/data/009/09_01.bvh tests/motions/cmu/cmu_09_01_run.bvh
python3 tools/cmu_to_sl.py <cmu>/data/013/13_26.bvh tests/motions/cmu/cmu_13_26_wave.bvh --start 8.4 --end 11.7
python3 tools/cmu_to_sl.py <cmu>/data/013/13_01.bvh tests/motions/cmu/cmu_13_01_sit_down.bvh --start 3.0 --end 5.2
python3 tools/cmu_to_sl.py <cmu>/data/013/13_01.bvh tests/motions/cmu/cmu_13_01_stand_up.bvh --start 7.3 --end 9.0
```

The other clips are converted whole (no `--start`/`--end`) in the same way. The test (`tests/gltfimport/tst_cmumotions.cpp`) checks that the files load and export to `.anim`, not that they equal a fresh conversion.
