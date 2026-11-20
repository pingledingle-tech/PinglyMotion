# Reference library index: format (issue 19)

The reference library has two parts, read together by the control commands `find_reference` and `load_reference` (and later by a browser panel):

1. **Bundled:** `library/index.json` and its clips, in the repository. In the Windows zip the folder is `library/` next to the program (not inside `examples/`, so the smoke run does not open every clip).
2. **Downloaded:** a folder on the user's machine, filled by `tools/fetch_reference.py` (the full sets, unchanged) and `tools/convert_reference.py` (the same clips on the Second Life skeleton), with its own `downloaded_index.json`. See "The download folder" below.

Plan: `docs/REFERENCE_LIBRARY_PLAN.md`. `tools/build_reference_index.py` made the bundled files and the index; `tests/test_reference_library.py` checks them.

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

Quaternius Universal Animation Library: not bundled, a download source instead (see below). Bundling clips of it would also need the name Quaternius in the Help > About credits text, which the bundled-index test requires; that is a visible change and a separate job.

## Make the files again

```
cmake --build build --target glb2sl_bvh
python3 tools/build_reference_index.py --cmu <cmu-mocap>/data --m2m <mesh2motion-app>/static/assets/human/animations --glb2sl build/tests/gltfimport/glb2sl_bvh
```

Source commits are in `tools/build_reference_index.py` and in each source's `origin` field.

## The download folder

Default `PinglyMotion-reference` in the user's home folder (`%USERPROFILE%\PinglyMotion-reference` on Windows); the program reads it from there, or from the folder in the environment setting `PINGLYMOTION_REFERENCE_FOLDER`. Both tools take `--dest` for another folder.

```
PinglyMotion-reference/
  downloaded_index.json            the index of everything below (both tools write it)
  cmu/<subject>/<trial>.bvh        originals, unchanged (fetch_reference.py)
  cmu/ATTRIBUTION.txt              licence and credit of the source
  cmu/source-docs/...              the source's own readme and licence files
  mesh2motion/<clip>.glb
  quaternius/AnimationLibrary_Godot_Standard.gltf  (+ .bin next to it)
  sl/cmu/<subject>/<trial>.bvh     converted to the Second Life skeleton (convert_reference.py)
  sl/mesh2motion/<clip>.bvh
  sl/quaternius/AnimationLibrary_Godot_Standard/<animation>.bvh   one file per animation
  sl/<source>/ATTRIBUTION.txt      copied from the source folder
```

### downloaded_index.json

`{"version": 1, "entries": [...]}`, entries sorted by `id`. Every entry has:

| Field | Meaning |
|---|---|
| `id` | `<source>/<path without extension>` for an original, `sl/<source>/<path without extension>` for a converted clip. Unique. |
| `name`, `description` | Short title; description may be empty. |
| `tags`, `tags_reviewed` | Tag words (from the bundled `tag_vocabulary`) and `false` until the project owner has reviewed them. Downloaded clips mostly have no tags; the converter tags the Quaternius animations from a fixed table in the tool. |
| `source`, `licence`, `credit` | Source name, licence text and credit line, copied from `tools/reference_manifest.json`. Never empty. |
| `format` | `bvh`, `glb` or `gltf`. |
| `skeleton` | `sl-classic` for clips PinglyMotion can open; otherwise the original skeleton (`cmu-bvh`, `mesh2motion-humanoid`, `quaternius-rigify`), which `find_reference` lists as not loadable. |
| `path` | The file, relative to the folder; never absolute, never `..`. |
| `sha256`, `size` | Of that file. `load_reference` refuses a file that does not match. |
| `length_s`, `fps` | Length in seconds (`(frames - 1) / fps`) and frame rate; `null` for originals. |
| `downloaded` | `true`. |
| `url` | Originals only: where the file came from. |

A converted entry has also `joints` (19), `frames` and `converted_from`: `{"id", "sha256", "skeleton"}` of the original, and `animation` and `animation_index` when the original file holds more than one animation.

The two tools keep out of each other's entries: `fetch_reference.py` replaces only entries below the folders of the sources it fetches, `convert_reference.py` only entries it converted (`converted_from`). Running either twice changes nothing.

### Converting

```
python3 tools/fetch_reference.py --source quaternius          # or cmu, mesh2motion; no --source = all
cmake --build build --target glb2sl_bvh                        # needed for glTF/GLB sources
python3 tools/convert_reference.py --glb2sl build/tests/gltfimport/glb2sl_bvh
```

- CMU BVH: `tools/cmu_to_sl.py` (in the same process), 30 fps, cut at 60 seconds (`--max-seconds`), the Second Life limit.
- Mesh2Motion GLB and Quaternius glTF: `glb2sl_bvh` (the program's own glTF reader and retargeting). A `.gltf` is first packed into a GLB with its `.bin` (only a `.bin` in the same folder is read; web addresses, other paths and image files are refused). A file with several animations gives one BVH each; a T-pose animation is left out.
- Every result is checked (the 19 classic joints in order, frame 0, frame values, length) before it is listed. A clip that fails is reported and left out; the run goes on and ends with exit code 2.
- `--source` limits the run to one source, `--limit N` to the first N downloaded files (a quick try).

The full CMU set (2548 trials) takes a while in pure Python; the Quaternius file (45 animations) and the Mesh2Motion clips take seconds. The converted clips are reference material: proportions and rest poses differ from Second Life's, so feet may slide; check a clip in PinglyMotion before building on it.
