# Generated edge-case animations

BVH files written by `tools/gen_edge_bvh.py` (Python standard library only; run `python3 tools/gen_edge_bvh.py`). They are **test data only**: not test motions for the project owner, and not copied into the Windows zip (that is `tests/motions/`). The output is byte-for-byte the same on every run; `tests/test_gen_edge_bvh.py` checks that the files here are what the script writes, so change the script and write the files again together.

`tests/animfile/tst_generated.cpp` loads every file with the BVH parser (`AnimFile::parseBvh`) and checks the result against the table below. Accepted files are also written again and read back, and written as a Second Life `.anim` file (`AnimExport::write`). The files that the old open and save path can also take are copied to `fuzz/corpus/bvh/` as `good_edge_*` and `bad_edge_*` (the old path is tested with them by `tests/animfile` and `tests/tst_roundtrip.cpp`).

Skeleton: the classic 19-joint Second Life skeleton (as in `bin/data/TPose.bvh`), unless the table says otherwise.

| File | What it tests | Result |
|---|---|---|
| `edge_one_frame.bvh` | the smallest useful file: 1 frame | accepted |
| `edge_root_only.bvh` | a skeleton with only the root joint, 1 frame | accepted |
| `edge_max_frames.bvh` | the most frames the parser takes (10000), at 0.006 s per frame = exactly 60 s, the longest Second Life takes; root and abdomen only, to keep the file small | accepted |
| `edge_over_max_frames.bvh` | the header says 10001 frames | refused: "Frames must be a whole number from 1 to 10000" |
| `edge_zero_frames.bvh` | the header says 0 frames | refused: same message |
| `edge_all_identity.bvh` | every joint at identity, 3 equal frames | accepted |
| `edge_extreme_rotations.bvh` | rotations of 720, 1e6, 1e30 and the largest 32 bit float (positive and negative) on every joint | accepted (any finite number is); too large for the retargeting code (`Retarget::validate` takes numbers up to 1e7) |
| `edge_wrap_180.bvh` | turns across the 180 degree wrap: hip 170, 179.9, -179.9, -170 and left thigh 175, 180, -180, -175 | accepted, the numbers are kept as written |
| `edge_large_translation.bvh` | root travel of 1 km, 1e6 and 1e30 inches | accepted; too large for `Retarget::validate`; the `.anim` writer takes it (positions are limited to 5 m there) |
| `edge_min_frame_time.bvh` | frame time 0.001 s, the smallest allowed | accepted |
| `edge_max_frame_time.bvh` | frame time 10 s, the largest allowed | accepted by the parser; see "Found while making these files" |
| `edge_below_min_frame_time.bvh` | frame time 0.0009 s | refused: "Frame Time must be from 0.001 to 10 seconds." |
| `edge_above_max_frame_time.bvh` | frame time 10.5 s | refused: same message |
| `edge_loop_one_frame.bvh` | a loop of exactly one frame: Second Life does not play frame 0 (the reference frame), so two equal frames play one frame over and over | accepted |

## Found while making these files

The old open and save path (`Animation::setFrameTime`) keeps the frame rate as a whole number of frames per second, `(int)(1.0/frameTime+0.5)`. A frame time above 2 s gives 0 frames per second, and saving the file then writes `Frame Time: inf`, which the parser refuses. `edge_max_frame_time.bvh` shows it (open it in the program and save it). That is why this file is not in `fuzz/corpus/bvh/` (the round-trip tests would fail on it). Nothing was changed in the program; this is for a later job.
