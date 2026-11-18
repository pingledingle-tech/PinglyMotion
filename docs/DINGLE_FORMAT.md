# The `.dingle` file format

`.dingle` is PinglyMotion's own working format. It keeps everything needed to go on editing an animation without loss. This document is the specification for version 1. The decision and its reasons are in [DECISIONS.md](DECISIONS.md) and issue [#26](https://github.com/pingledingle-tech/PinglyMotion/issues/26). The older formats are described in [ANIM_FORMAT.md](ANIM_FORMAT.md).

The program never writes AVM. Old `.avm` files still open, and are upgraded to `.dingle` when saved (see "Opening an old `.avm`").

## Summary

- UTF-8 JSON, one object, file extension `.dingle`.
- Stored frame based, like the program's data model (`AnimFile::Animation`). Seconds are derived from `frameTime`.
- **Key frames only.** Only the values at key frames are stored. The frames in between are computed on load by the same interpolation the program uses.
- **Ease flags only in version 1** (`easeIn`, `easeOut`). Easing curves (`curve`) and camera keys (`camera`) are added later without a version bump.
- Written by the program's own small emitter: fixed key order, floats printed with 9 significant digits, so a saved file is byte-identical every time. `QJsonDocument` is used only for reading.

## Layout (version 1)

```json
{
  "format": "dingle",
  "version": 1,
  "generator": "PinglyMotion 0.5.0",
  "timing": { "frames": 31, "frameTime": 0.0333333 },
  "figure": "female",
  "scale": 1.0,
  "loop": { "in": -1, "out": 31 },
  "skeleton": [
    { "name": "hip", "kind": "root", "parent": -1,
      "offset": [0, 0, 0],
      "channels": ["Xposition", "Yposition", "Zposition", "Zrotation", "Xrotation", "Yrotation"] }
  ],
  "tracks": [
    { "joint": 0,
      "rotation": [ { "frame": 0, "values": [0, 0, 0], "easeIn": false, "easeOut": false } ],
      "position": [ { "frame": 0, "values": [0, 0, 0], "easeIn": false, "easeOut": false } ] }
  ],
  "camera": null,
  "extensions": {}
}
```

| Key | Meaning |
|---|---|
| `format` | The string `"dingle"`. |
| `version` | Whole number, 1 or higher. See "Versions". |
| `generator` | Program name and version, for debugging. No user data. Optional for readers. |
| `timing.frames` | Number of frames, 1 to 10000. |
| `timing.frameTime` | Seconds per frame, 0.001 to 10. |
| `figure` | `"male"` or `"female"` (the program's 0 / 1). |
| `scale` | Avatar scale, as in the program today. |
| `loop.in`, `loop.out` | Loop points as frame numbers. `in: -1` means "no loop points set" (as `AnimFile::Animation::loopIn`). |
| `skeleton` | The joint tree, stored as the model has it, so any BVH or old AVM skeleton survives without loss. Each joint has `name`, `kind`, `parent` (index, `-1` for the root), `offset` (3 numbers) and `channels` (names in file order). Mapping joint names to the SL skeleton stays with the code that builds the scene. |
| `tracks` | One entry per joint that has keys: `joint` (index into `skeleton`), `rotation` keys, and `position` keys on joints with position channels (today the root). |
| `camera` | `null` or absent in version 1. Reserved for camera keys (issue #14). |
| `extensions` | Object reserved for namespaced data from add-ons or experiments. Kept untouched on save when within the limits. |

### Keys

Each key has:

- `frame`: whole number, `0` or higher and below `timing.frames`.
- `values`: the track's channel values in file order (rotation channels for `rotation`, position channels for `position`), so the rotation order is kept exactly. The count equals the channel count of that track.
- `easeIn`, `easeOut`: booleans, as today.

Per track the keys are sorted by `frame`, unique, and frame 0 is always present. Position keys carry their own values; this replaces AVM's separate `Positions:` list.

### Not stored

Joint scale (not supported by BVH or `.anim`), personal data, absolute paths, and the file's own name.

## Versions

- `version` is a whole number. Missing, zero, negative or non-integer: refuse. `format` other than `"dingle"`: refuse.
- **Additive changes never bump `version`.** New optional keys (such as `curve` on a key, or `camera`) are skipped by older readers.
- **`version` is bumped only when the meaning of an existing field changes.** Each bump comes with one small migration step (`v1 → v2`, and so on), chained. The version 1 test file stays in the test data forever.
- **Older version:** migrated in memory and opened normally. The next save writes the current version.
- **Newer version than the program knows:** refuse with "This file was made by a newer version of PinglyMotion. Please update."
- **Same version, but keys this program does not know:** the file opens. The reader counts the ignored keys, and on save the program warns once that these data will be lost. It never drops them silently.

### Planned additions (no version bump)

- `curve` on a key: easing curve, for example `{"type": "bezier", "out": [x, y]}`. The exact shape is decided in issue #11. A reader that sees an unknown `curve.type` uses the plain ease flags and counts the key as ignored.
- `camera`: a top-level object with its own `keys` in the same key shape (issue #14).

## Limits

New `constexpr` values in the `.dingle` header, mostly shared with `AnimFile::`.

| Limit | Value |
|---|---|
| File size | 16 MB (lower than the 64 MB of BVH, because the JSON tree takes many times the file size in memory; real files are far below 1 MB) |
| Joints (`MAX_JOINTS`) | 256 |
| Joint depth (`MAX_DEPTH`) | 64 |
| Channels per joint (`MAX_CHANNELS`) | 6 |
| Frames (`MAX_FRAMES`) | 10000 |
| Frame time | 0.001 to 10 s |
| Joint name | at most 128 bytes |
| Keys per track | at most `MAX_FRAMES` |
| JSON nesting | at most 16 |
| `extensions` | at most 1 MB |

## Validation

The reader works from memory, returns the whole animation or an error (never half a result), never crashes, and checks every count before allocating. It refuses the file when any of these fails:

- Numbers are finite and fit a `float`.
- Integers are whole and in range.
- `parent` points to an earlier joint (so there are no cycles and no recursion is needed).
- `joint` in `tracks` exists and appears once.
- Key frames are sorted, unique, below `frames`, and frame 0 is present.
- `values` has exactly the channel count of the track.
- `loop.in` and `loop.out` are inside `0..frames`.
- String values are valid UTF-8.
- No duplicate keys in an object this version knows.
- All limits above hold.

Unknown keys are skipped but counted. Nothing from the file is used as a path or command.

Writing goes only to the path the user chose, atomically (`QSaveFile`: temporary file, then rename).

## Opening an old `.avm`

1. The AVM reader stays for opening old files. The animation remembers the path of the `.avm` it came from. The title shows the new name `<same name>.dingle` and the file counts as changed, so Save is always offered.
2. Save (or Save As with a name ending in `.dingle`) writes `<same name>.dingle` beside the old file. If it exists, the existing overwrite question comes first.
3. The file is written with `QSaveFile`, so a crash never leaves half a file.
4. **Verify:** the new file is read back with the `.dingle` reader and compared with the animation in memory.
5. Only if it matches, the old `.avm` is moved to the recycle bin with `QFile::moveToTrash()`. If that fails (no recycle bin on that drive or system), the `.avm` stays where it is and the user gets one plain sentence saying so. It is never deleted outright.
6. If anything fails before step 5, nothing is moved and the old `.avm` is untouched.

Other rules:

- A new animation is named `pingle.dingle` by default.
- An opened `.bvh` is never trashed. Save asks for a `.dingle` name (suggesting the BVH's name) and leaves the `.bvh` alone. "Export for Second Life" stays BVH until `.anim` export (#10).
- The control API `open_file` accepts `.dingle`, `.avm` and `.bvh`. Its save writes only `.dingle` and refuses `.avm`.

## Bundled files

The four bundled QAvimator files (`bin/data/TPose.avm`, `bin/data/Relaxed.avm`, `examples/jump.avm`, `examples/throne.avm`) are converted once to `.dingle` with the program's own parser and writer. The originals move to `tests/legacy-avm/` as permanent test input. A test must show that each converted file opens to an animation identical to the `.avm` before the program loads the `.dingle` copies. `fuzz/corpus/avm` and `tools/legacy/TPose.avm` stay as tests of the AVM reader.

## Testing and fuzzing

A fuzz target `fuzz/fuzz_dingle.cpp` takes any bytes: an accepted file must pass every rule above, a refused file must give a reason, and `write(parse(x))` must parse again to an equal animation. Seed corpus: the converted files, the version 1 test file and some `bad_*` files (truncated, duplicate key, huge count, deep nesting, NaN as a string, negative frame), with a dictionary of the key names. The fuzz run on the branch must be green before a parser job is merged.
