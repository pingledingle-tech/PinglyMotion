# Joint data from the Firestorm viewer

Data for the posing interface (#88, see `docs/POSING_UX.md`): a friendly name,
a section, a left/right mirror partner and the "do not flip on mirror" flag for
every joint of PinglyMotion's skeleton, plus Firestorm's 9 hand presets.
`src/jointinfo.h` reads it; `tests/tst_jointinfo.cpp` checks it. The joint
list uses the friendly names and sections (`src/jointlist.h`, posing UX step 2);
the hand presets are the ready-made finger poses of the Hands tab
(`src/handpose.h`, `src/handstab.h`, UX phase 2c).

## Source

Taken from the Firestorm viewer:
https://github.com/FirestormViewer/phoenix-firestorm
(commit 69355fade65cdf39f854da2d187a2a1624ce56aa)

| PinglyMotion file | Firestorm file |
|---|---|
| `joints.json` | `indra/newview/fsposeranimator.h` (`PoserJoints`: joint order, type, mirror partner, `dontFlipOnMirror`) and `indra/newview/skins/default/xui/en/floater_fs_poser.xml` (`title_*` names, `header_*` section headings, `joint_transform_*`, `joint_frame_*`, `joint_delta_rotate_*`) |
| `hand_presets/*.xml` | `indra/newview/app_settings/poses/hand_presets/*.xml`, copied unchanged |
| `LICENSE.txt` | `doc/LGPL-license.txt`, unchanged |

`joints.json` is made by `tools/make_firestorm_joints.py` (run from the
repository root with a Firestorm checkout and its commit). To update the data,
run it again on a newer Firestorm commit and change the commit above.

## Licence

Phoenix Firestorm Viewer Source Code
Copyright (c) 2024 Angeldark Raymaker @ Second Life
Copyright (C) Linden Research, Inc.

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; version 2.1 of the License only.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details
(`LICENSE.txt` in this folder).

The files in this folder stay under the LGPL 2.1. PinglyMotion itself is GPL
version 2 or later; LGPL 2.1 material may be used in it.

## What is in `joints.json`

One entry per joint, in Firestorm's list order (body, face, hands, tail, hind
limbs, wings, spine), then the joints Firestorm does not list:

| Field | Meaning |
|---|---|
| `sl` | Second Life joint name (`mShoulderLeft`) |
| `bvh` | Name in PinglyMotion's BVH files (`lShldr`), from `bin/data/bento_joints.txt` |
| `name` | Friendly name (`Whole Arm`) |
| `nameFrom` | `firestorm` or `pinglymotion` (see below) |
| `tab` | PinglyMotion's joint table tab: Body, Face, Hands or Other, the same as `JointGroups::tabOf()` (a test checks this) |
| `firestormType` | Firestorm's tab: `BODY`, `FACE`, `HANDS`, `MISC` or `WHOLEAVATAR` (the pelvis); empty when Firestorm does not list the joint |
| `firestormSection` | Heading the joint stands under in Firestorm's list (`Left Arm`, `Eyes/Eye Lids`) |
| `mirror` | SL name of the left/right partner, empty if none |
| `dontFlipOnMirror` | Firestorm's flag: when the whole body is mirrored, this joint does not flip its partner (set on the right-side joints, so each pair is flipped once) |
| `firestormTransform`, `firestormFrame`, `firestormDeltaRotate` | Firestorm's axis swaps for its sliders and gizmo, and its "rotate as delta" flag, as raw text; only where Firestorm sets them. Kept for the rotation conversion below |

`handPresets` lists the 9 presets with their file names: Flat, Grip, Horns,
Okay, Pointing, Relaxed, Thumbs Up, Tight Grip, Two Finger Salute. Each sets
the 15 finger joints of both hands.

Firestorm's collision volumes (`HEAD`, `L_UPPER_ARM` …) are left out: they are
not joints in BVH files.

## Changes and additions by PinglyMotion

- Names that Firestorm does not have, marked `"nameFrom": "pinglymotion"`:
  `mFaceRoot` (Face Root) and `mFaceChin` (Chin) are in Firestorm's list but
  have no title there; `mFaceEyeAltLeft` / `mFaceEyeAltRight` (Left/Right Eye
  (alternative)) are not in Firestorm's list at all. These two got a mirror
  pair, a section and `dontFlipOnMirror` on the right side, like Firestorm's
  other pairs. `mFaceJawShaper` is not in Firestorm's list either, but its
  title (Jaw Shaper) is in Firestorm's floater and is used.
- `bvh` and `tab` are PinglyMotion's.
- Everything else is as Firestorm has it, including these details:
  - Many names only make sense with their section: "Collar", "Thumb Base",
    "Left 2" (hind limb or wing). `JointInfo::uniqueName()` puts the section in
    front when a name is used more than once ("Left Arm: Collar").
  - `mFaceTongueTip` has `dontFlipOnMirror` set but no partner.
  - `mGroin` stands under the "Tail" heading, because it follows the tail in
    Firestorm's list.

## Rotation format of the hand presets (used since UX phase 2c)

The preset files are stored **as found**. Firestorm's pose files (LLSD XML,
`version` 5) give, per joint, `rotation` as three Euler angles in **radians**
in Second Life's joint axes (X forward, Y left, Z up), with
`jointBaseRotationIsZero` set: the rotation replaces the joint's base rotation
instead of adding to it. When loading a file Firestorm builds the rotation with
`LLMatrix3(x, y, z)` (roll, pitch, yaw) and no axis swap
(`FSPoserAnimator::loadJointRotation()` in `fsposeranimator.cpp`); written as a
rotation of column vectors that is Rx(roll) * Ry(pitch) * Rz(yaw)
(`LLMatrix3::setRot()` and `LLQuaternion::getEulerAngles()` in `indra/llmath`).

PinglyMotion keeps rotations as BVH Euler angles in **degrees**, in BVH axes
(X the figure's left, Y up, Z forward), applied channel by channel in the
file's order (#116; the finger joints are Xrotation Zrotation Yrotation).
`HandPose::fromFirestorm()` (`src/handpose.cpp`) converts a preset: radians to
degrees, Second Life's X to our Z, its Y to our X and its Z to our Y, and the
turn written in the joint's channel order; of the two sets of angles for the
same turn the smaller one is kept, and each angle is rounded to a whole degree.
The program's own measurement agrees (the calibration table in
`docs/ASSISTANT_GUIDE.md`: finger curl is Z, negative on the left hand,
positive on the right), and `tests/handstab` checks that every curled preset
curls that way and that the right hand is the left hand mirrored (Y and Z
turned round), as the files have it. A picture test against Firestorm itself
was not made; how the poses look on a Second Life avatar is for the project
owner's combined review.
