# Reference library index: format (issue 19)

`library/index.json` lists the clips of the reference library (plan: `docs/REFERENCE_LIBRARY_PLAN.md`). In the Windows zip the folder is `library/` next to the program (not inside `examples/`, so the smoke run does not open every clip). `tools/fetch_reference.py` (download of the full sets) writes entries of the same format into a local index. `tools/build_reference_index.py` made the bundled files and the index; `tests/test_reference_library.py` checks them.

## Top level

| Field | Meaning |
|---|---|
| `schema_version` | `1` |
| `tag_vocabulary` | The only words a clip may use as tags (lowercase). A tag outside this list fails the test: tags are text that assistants read, so new words are reviewed by the project owner. |
| `sources` | Object: source id to `{name, url, licence, credit, about_name, origin}`. `about_name` must appear in the Help > About credits text (`src/aboutcredits.cpp`). |
| `clips` | List of clip entries. |

## Clip entry

All fields are required unless marked.

| Field | Meaning |
|---|---|
| `id` | Unique, lowercase letters, digits and `-`. Pattern `<source prefix>-<name>`. |
| `name` | Short title, at most 60 characters. |
| `description` | One plain sentence, at most 140 characters. |
| `tags` | 1 to 8 words from `tag_vocabulary`. |
| `source` | A key of `sources`. |
| `licence` | Licence text for this clip (copied from the source; never empty). |
| `credit` | Credit line to show with the clip (never empty). |
| `origin` | Where the clip comes from (trial, file name, time range, commit). |
| `skeleton` | `sl-classic`: the classic Second Life skeleton, 19 joints. |
| `joints` | `19`. |
| `frames` | Number of frames in the file (frame 0 is the SL rest pose, which Second Life does not play). |
| `fps` | Frames per second. |
| `duration_s` | `(frames - 1) / fps`. At most 60 (the Second Life limit). |
| `file` | Path relative to `index.json`, for a bundled clip. |
| `bytes`, `sha256` | Size and SHA-256 of `file`. |

A clip that is not bundled has `download` instead of `file`: `{"url": ..., "sha256": ..., "conversion": ...}` (written by `tools/fetch_reference.py`; `bytes` and `sha256` of the converted file may be added once it exists). The bundled-file tests apply only to entries with `file`; every entry needs the licence and credit fields.

## Bundled content (November 2026)

55 clips, about 3.8 MB, 11.3 s at most, in `library/cmu/` (28 clips from the CMU Graphics Lab Motion Capture Database, converted with `tools/cmu_to_sl.py`) and `library/mesh2motion/` (27 CC0 clips from Mesh2Motion, converted with `glb2sl_bvh`). Not bundled: the clips already in `tests/motions/` (they stay there), Quaternius (see below).

Quaternius Universal Animation Library: not bundled in this job. Its download is behind the author's page or itch.io with no stable address that can be pinned and hashed from here, and the plan asks for a re-check of the download page and file headers first. The index format already allows it (`sources`, `download`).

## Make the files again

```
cmake --build build --target glb2sl_bvh
python3 tools/build_reference_index.py --cmu <cmu-mocap>/data --m2m <mesh2motion-app>/static/assets/human/animations --glb2sl build/tests/gltfimport/glb2sl_bvh
```

Source commits are in `tools/build_reference_index.py` and in each source's `origin` field.
