# Hand-made joint mappings (`.jointmap`)

File > Import BVH from another skeleton matches the joints of the file to the Second Life skeleton by name and tree shape (`src/retarget.*`, issue [#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16)). When that guess is wrong for a few joints, the user can correct it by hand. This page describes the small file that holds such a correction: what it is for, its fields, where the program keeps it, and its limits. The code is `src/retargetmapping.*`; the tests are `tests/retargetmapping/`.

State: the file format, reader, writer, fingerprint and store are built (step 4 of #16, core). The dialog that edits a mapping in the program is a later, visible job, so nothing uses the files on screen yet.

## What it is for

- A mapping file lists only the corrections: which joint of the user's file drives which Second Life joint, or that a joint is to be left out. Everything not listed stays as the automatic matching decides.
- The program remembers a mapping **per source skeleton**: the file carries a fingerprint of the skeleton it belongs to, and the store keeps one file per fingerprint. The next BVH from the same tool, with the same joint names and tree, finds its corrections again.
- An AI client that knows the skeleton of a file can write a mapping file and import with it, without the dialog (the control command for that is not built yet; the idea is on issue [#129](https://github.com/pingledingle-tech/PinglyMotion/issues/129)).

## Layout (version 1)

UTF-8 JSON, one object, file ending `.jointmap`. No byte order mark.

```json
{
  "format": "pinglymotion-jointmap",
  "version": 1,
  "generator": "PinglyMotion 0.3.0",
  "skeleton": {
    "fingerprint": "0000000000000000000000000000000000000000000000000000000000000000",
    "joints": 27,
    "root": "mixamorig:Hips"
  },
  "mapping": [
    { "source": "mixamorig:LeftShoulder", "target": "" },
    { "source": "mixamorig:RightHandIndex1", "target": "mHandIndex2Right" }
  ]
}
```

| Key | Meaning |
|---|---|
| `format` | The string `"pinglymotion-jointmap"`. Required. |
| `version` | Whole number, 1 or higher. A file with a higher version than the program knows is refused with "please update". Required. |
| `generator` | Who wrote the file, for debugging. Text, at most 128 bytes. No user data. Optional. |
| `skeleton` | Which source skeleton the entries belong to. Optional as a whole: a file without it fits any skeleton that has the listed joint names (that is how an AI client writes one). |
| `skeleton.fingerprint` | The fingerprint of the source skeleton (see below): 64 lower case hex characters. The store refuses a file whose fingerprint is not the one it is filed under. |
| `skeleton.joints` | Number of joints of the source skeleton without End Sites, 0 to 256. A readable hint only; the reader does not compare it with anything. |
| `skeleton.root` | Name of the root joint, at most 128 bytes. A hint only. |
| `mapping` | The entries, a list of 0 to 256 objects. Required (may be empty). |
| `mapping[].source` | The joint name as written in the user's file, 1 to 256 bytes, no control characters. Each name at most once. When the mapping is used, the joint is looked up by this name, exactly first and then without regard to case. |
| `mapping[].target` | The Second Life joint this source joint drives: a name of the Second Life skeleton (`mShoulderLeft`), or the old name of the 19 joint files (`lShldr`), or with the `avatar_` prefix, in any case. The reader turns it into the canonical name. Each Second Life joint at most once. Empty text or `null` means **leave this joint out**. Anything that is not a joint of the Second Life skeleton is refused. |

Unknown keys are refused (a typo in a hand-written file must not silently do nothing). The full list of Second Life joint names is the second column of `bin/data/bento_joints.txt` (133 joints); the program carries the same list.

### How a mapping is used

The entries are put on top of the automatic mapping (`RetargetMapping::apply`):

- An entry with a target makes that pair. If the automatic matching had given that Second Life joint to another source joint, that joint loses it (the import list then says "its joint was given to … by hand").
- An entry with an empty target takes the source joint out of the retargeting ("left out by hand"). Its turn is then passed on to the next matched joint below, as for any unmatched joint.
- An entry whose source joint is not in the file, or whose target joint is not in the avatar skeleton, is skipped with a note; it is not an error.
- After that the usual rules apply: the pelvis, spine, head, both arms, both hands and both legs must be matched, or the import refuses with the list of what is missing.

### The fingerprint

`RetargetMapping::fingerprint()` is the SHA-256 (as 64 lower case hex characters) of a text with one line per joint of the source skeleton, in file order, End Sites left out: the index of the parent joint (`-1` for the root), a tab, and the joint name. Offsets, channels, frame count and motion do not count, so every BVH from the same tool with the same rig gives the same fingerprint, and a renamed or re-parented joint gives another one.

## Where the files are kept

`RetargetMapping::Store` keeps one file per fingerprint, named `<fingerprint>.jointmap`, in the folder `PinglyMotion/skeleton-maps` under the user's data folder, next to the control connection file (`QStandardPaths::GenericDataLocation`; on Windows `%LOCALAPPDATA%\PinglyMotion\skeleton-maps`). Deleting the folder forgets every hand-made mapping. Tests set the environment variable `PINGLYMOTION_MAPPING_DIR` to use another folder.

A saved file always has `skeleton.fingerprint`. Loading checks that it equals the fingerprint the file is named after; a file without one is used as it is.

## Limits

Files from other people, and files written by an AI client, are untrusted. The reader refuses, with a reason and never a crash:

- an empty file, a file over 1 MB, a byte order mark, JSON nested deeper than 4 levels, anything that is not one JSON object;
- more than 256 entries; a source name over 256 bytes; a target name over 64 bytes; `generator` or `root` over 128 bytes; control characters in any name;
- a source joint listed twice, a Second Life joint used twice, a target that is not a Second Life joint, an unknown key, a wrong `format`, a missing or non-whole `version`.

The writer (`RetargetMapping::write`) has a fixed key order and layout, so the same mapping gives the same bytes every time; `write(parse(x))` reads back equal. The fuzz target `fuzz_retargetmapping` checks all of this with random input (`fuzz/README.md`).
