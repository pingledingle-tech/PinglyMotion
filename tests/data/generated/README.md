# Generated edge-case animations

BVH files written by `tools/gen_edge_bvh.py` (Python standard library only; run `python3 tools/gen_edge_bvh.py`). They are **test data only**: not test motions for the project owner, and, apart from the Windows zip's folder `examples/edge-cases/`, not meant for hand testing. The output is byte-for-byte the same on every run; `tests/test_gen_edge_bvh.py` checks that the files here are what the script writes, so change the script and write the files again together.

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


# Generated .dingle and .anim edge cases

Written by `tools/gen_edge_motion.py` (Python standard library only; run `python3 tools/gen_edge_motion.py`), same rules as above: test data, byte-for-byte the same on every run, `tests/test_gen_edge_motion.py` checks that the files here are what the script writes. `tests/animfile/tst_generated_motion.cpp` has a table with the exact result of every file: the `.dingle` reader (`Dingle::parse`) or the `.anim` reader (`AnimImport::parse`) opens it, or refuses it with the named reason. An opened `.dingle` is also written and read again (equal animation, same bytes the second time) and exported as a Second Life `.anim` (or refused with the named reason). Small files are copied to `fuzz/corpus/dingle/` and `fuzz/corpus/anim/` as `good_edge_*` and `bad_edge_*`. The `.dingle` files also go through the smoke run (they open in the program); `.anim` files cannot be opened in the program yet (the control command `open_file` takes `.dingle`, `.bvh` and `.avm`), so only the reader tests use them.

Skeleton of the `.dingle` files: the classic 19-joint Second Life skeleton with its 5 End Sites (24 entries), unless the table says otherwise.

| File | What it tests | Result |
|---|---|---|
| `edge_easings_all_six.dingle` | all six easing choices, one per segment, 7 keys on the left shoulder | accepted; every frame in between equals `Easing::segmentValue` for its segment |
| `edge_many_keys.dingle` | a key on every one of 1000 frames (0.06 s, 60 s) on three tracks | accepted; the export is thinned to fit 250,000 bytes |
| `edge_loop_full.dingle` | loop points at both ends (0 and the frame count) | accepted |
| `edge_loop_last_frame.dingle` | loop starts at the last frame | accepted |
| `edge_loop_empty.dingle` | loop start equals loop end | accepted |
| `edge_loop_start_after_end.dingle` | loop start 20, loop end 5 | accepted: the reader only checks each number against `0..frames` |
| `edge_loop_out_past_end.dingle` | loop end one past the frame count | refused: "The loop end" |
| `edge_loop_in_below_minus_one.dingle` | loop start -2 | refused: "The loop start" |
| `edge_fps_24.dingle`, `edge_fps_60.dingle`, `edge_fps_7.dingle` | frame times 1/24, 1/60 and 1/7 s | accepted |
| `edge_duration_60s.dingle` | 122 frames at 0.5 s: the export plays frames 1 to 121 = exactly 60 s | accepted, export works |
| `edge_duration_over_60s.dingle` | 123 frames at 0.5 s = 60.5 s | accepted, the export is refused (60 second limit) |
| `edge_one_frame.dingle` | one frame | accepted |
| `edge_root_only.dingle` | a skeleton with only the root joint | accepted |
| `edge_gimbal.dingle` | the middle rotation channel of every joint at +-90 and +-89.9999 degrees | accepted, numbers kept as written |
| `edge_joint_limits.dingle` | every joint at the lowest and highest value of `bin/data/SL.lim`, axis by axis | accepted, values exact |
| `edge_anim_216_joints.anim` | the most joints (216), two keys each | accepted |
| `edge_anim_217_joints.anim` | 217 joints | refused (header) |
| `edge_anim_250000_bytes.anim` | 216 joints, 60 s, exactly 250,000 bytes (the upload limit) | accepted |
| `edge_anim_250001_bytes.anim` | the same, one byte more | accepted by the reader (it only refuses above 4 MB); the exporter never writes more than 250,000 bytes |
| `edge_anim_60s_loop_ends.anim` | 60 s, loop points exactly at 0 and 60 s | accepted, no warning |
| `edge_anim_loop_swapped.anim` | loop start 8 s after loop end 2 s | accepted with one warning (points moved inside) |
| `edge_anim_one_key.anim` | one key only | accepted |
| `edge_anim_quaternion_corners.anim` | x, y, z each 0 or 65535 (quantiser corners, not unit length) | accepted, every number finite |
| `edge_anim_position_limits.anim` | position keys at the 5 m limits | accepted |
