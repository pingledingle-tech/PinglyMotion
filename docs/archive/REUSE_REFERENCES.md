Archived 22 November 2026: parked notes, not planned work; extended by OPEN_SOURCE_CHECK.md.

# Reuse references (parked)

Status: **parked, not planned work.** Notes from a check on 16 November 2026 of other pose tools and viewer code that may be useful later. Nothing here changes the work order in `docs/PRODUCT.md`. Open an issue before building anything from this list.

Licence rule: PinglyMotion is GPLv2. Second Life viewer and Firestorm code is LGPL 2.1 and may be used if the copyright headers stay. Closed tools give ideas only, never code.

## 1. `.anim` export and import (issue #63) – most useful

- Reference: Second Life viewer, `indra/llcharacter/llkeyframemotion.cpp` (LGPL 2.1, Linden Research), checked at commit `8fa97d7` of https://github.com/secondlife/viewer
  - `LLKeyframeMotion::serialize()` (about line 2046) writes the format; `deserialize()` (about line 1229) reads it.
- Format in short: header (`version`, `sub_version`, `base_priority`, `duration`, `emote_name`, `loop_in_point`, `loop_out_point`, `loop`, `ease_in_duration`, `ease_out_duration`, `hand_pose`, `num_joints`), then per joint: name, joint priority, rotation keys (time + quaternion packed to a vector, each part as U16 in −1…1) and position keys (time + position as U16 in ±`LL_MAX_PELVIS_OFFSET`), then constraints.
- The code depends on Linden helper classes (`LLDataPacker`, `LLQuaternion`). Rewriting it in Qt with this code as the reference is likely simpler than copying it.
- Gives what BVH upload cannot: priority per joint, ease and loop points stored in the file.

## 2. Firestorm poser (checked at commit `6a29a98` of https://github.com/FirestormViewer/phoenix-firestorm)

Files in `indra/newview/`: `fsfloaterposer.cpp`, `fsposeranimator.cpp/.h`, `fsjointpose.cpp/.h` (LGPL 2.1).

- **Firestorm already writes BVH.** Check box "Write BVH when saving" (`FSPoserSaveExternalFileAlso`); writer in `FSFloaterPoser::writePoseAsBvh()` (about line 2877): 2 frames, frame 0 all zero, per-joint channel order from `floater_fs_poser.xml`. It only works for poses that start from a T-pose ("BVH requires original work", see the tool tip). So PinglyMotion can most likely open Firestorm poses through BVH today; no XML import is needed.
- **Do not import Firestorm XML diffs.** Poses that adjust someone else's animation are saved only as XML diffs. Importing them would get round Firestorm's protection against copying.
- XML format, for reference only: `savePoseToXml()` / `loadPoseFromXml()` (about lines 634 and 1267). LLSD XML, `version` 8, `startFromTeePose`, per joint `enabled`, `mirrored`, `rotation`/`position`/`scale` (vectors), `jointBaseRotationIsZero`, `userSetBaseRotationToZero`, `worldLocked`.
- Already noted for the posing UX (#88): joint table with mirror joints and groups (`PoserJoints` in `fsposeranimator.h`), mirror/flip code, and 9 hand presets in `indra/newview/app_settings/poses/hand_presets/` (Flat, Grip, Horns, Okay, Pointing, Relaxed, Thumbs_Up, Tight_Grip, Two_Finger_Salute).
- Possible test later: open a BVH written by Firestorm in PinglyMotion and check that it looks the same.

## 3. Black Dragon viewer

- Open source (same licence model as the SL viewer). Has its own poser and a separate pose creator that writes `.anim`. Its XML format is its own, not the same as Firestorm's.
- Source not checked: the Git server (git.alchemyviewer.org) could not be reached from the cloud session.

## 4. PoseForge (https://poseforge.pages.dev) – ideas only

Browser pose editor for SL. No source code or licence found (GitHub projects with the same name are unrelated). Ideas worth keeping:

- Choose which joints are exported (for example arms only, so the user can still walk). PinglyMotion already exports only joints that move away from frame 0, but this is not visible to the user.
- Pose from a photo. For PinglyMotion this can be done through the existing MCP control: the AI looks at the image and sets the joints. No image detection inside PinglyMotion.
- Import of `.anim` together with export.

## 5. Not useful for code

- Avastar, Bento Buddy (paid Blender add-ons, no public source).
