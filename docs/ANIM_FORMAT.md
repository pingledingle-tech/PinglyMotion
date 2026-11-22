# Second Life `.anim` format

A precise description of Second Life's internal animation format (`.anim`, asset type `AT_ANIMATION`), written as the basis for `.anim` export ([#10](https://github.com/pingledingle-tech/PinglyMotion/issues/10)) and import ([#22](https://github.com/pingledingle-tech/PinglyMotion/issues/22)). File formats were decided in old #118; see the combined plan in [PRODUCT.md](PRODUCT.md).

**Source.** Second Life viewer, https://github.com/secondlife/viewer, branch `develop` at commit `f221e05` (checked 18 November 2026). The code is © Linden Research, Inc., LGPL 2.1. Every rule below names the file and function it comes from; paths are relative to `indra/` unless they start with `scripts/`. Short excerpts are quoted with that source named. PinglyMotion is GPLv2; the plan in [REUSE_REFERENCES.md](archive/REUSE_REFERENCES.md) is to write our own reader and writer in Qt with this code as the reference, not to copy it.

The main files:

| File | What it gives |
|---|---|
| `llcharacter/llkeyframemotion.cpp` | `LLKeyframeMotion::deserialize()` (reader, the rules a file must pass), `serialize()` (writer), `RotationCurve::getValue()` / `PositionCurve::getValue()` (playback) |
| `llcharacter/llkeyframemotion.h` | `KEYFRAME_MOTION_VERSION`, `KEYFRAME_MOTION_SUBVERSION` |
| `llcharacter/llbvhloader.cpp` | `LLBVHLoader`: how a BVH becomes `.anim` keys (axes, time, units, defaults, key reduction) |
| `llcharacter/llbvhconsts.h` | `MAX_ANIM_DURATION`, constraint enums |
| `llcharacter/lljoint.h` | `LL_CHARACTER_MAX_ANIMATED_JOINTS`, `LL_MAX_PELVIS_OFFSET`, `JointPriority` |
| `llcharacter/llhandmotion.h/.cpp` | hand pose enum and names |
| `llcharacter/llanimationstates.cpp` | emote names |
| `llmessage/lldatapacker.cpp` | `LLDataPackerBinaryBuffer`: byte order, strings, vectors |
| `llmath/llquantize.h`, `llmath/v3math.cpp` | `F32_to_U16()`, `U16_to_F32()`, `LLVector3::quantize16()` |
| `llmath/llquaternion.cpp/.h` | `packToVector3()`, `unpackFromVector3()`, `mayaQ()`, quaternion multiplication order |
| `newview/llviewerassetupload.cpp` | `LLNewFileResourceUploadInfo::exportTempFile()`: what the viewer does with an uploaded `.anim` |
| `newview/llfloaterbvhpreview.cpp` | BVH upload preview: checks and the user settings that end up in the file |
| `scripts/content_tools/anim_tool.py` | Linden's own Python reader/writer for `.anim` (LGPL 2.1); useful to cross-check our files |

## 1. Encoding rules

All from `llmessage/lldatapacker.cpp`, class `LLDataPackerBinaryBuffer`:

- **Byte order: little endian.** Every multi-byte value goes through `htolememcpy` (host to little endian) in `packU16()`, `packS32()`, `packU32()`, `packF32()`, `packVector3()`.
- **No padding, no alignment.** Each `pack*` call advances the write pointer by exactly the size of the value.
- **Strings** (`packString()` / `unpackString()`): the bytes of the string followed by one `0x00`. No length prefix. (The wiki page "Anim File Format" says strings are length-prefixed; the source and a real file say otherwise.)
- **Fixed binary** (`packBinaryDataFixed()`): exactly *n* bytes, copied as they are. Used for the 16-byte collision volume names in constraints.
- **`LLVector3`** (`packVector3()`): three `F32` (x, y, z), 12 bytes.
- **`F32`** is IEEE 754 single precision.

There is no magic number, no file length field and no checksum. The file is just the fields below, one after another, and nothing may follow them (the reader does not check for trailing bytes, see §8).

## 2. Byte layout

Types: `U8`/`U16`/`U32` unsigned, `S32` signed, `F32` float, `STR` zero-terminated string, `V3` = 3 × `F32`.

### 2.1 Header

Order and types from `LLKeyframeMotion::serialize()`; checks from `LLKeyframeMotion::deserialize()`.

| # | Field | Type | Bytes | Rule when reading (`deserialize()`) |
|---|---|---|---|---|
| 1 | `version` | U16 | 2 | Must be `1` (`KEYFRAME_MOTION_VERSION`), or `0` together with `sub_version` 1 (old format, §2.5). Anything else fails in release builds. |
| 2 | `sub_version` | U16 | 2 | Must be `0` (`KEYFRAME_MOTION_SUBVERSION`). |
| 3 | `base_priority` | S32 | 4 | `< -1` fails. `>= 7` (`ADDITIVE_PRIORITY`) is changed to `6`. See §3.1. |
| 4 | `duration` | F32 | 4 | Seconds. Fails if `> 60.0` (`MAX_ANIM_DURATION`, `llbvhconsts.h`) or not finite. Negative is not rejected (§8). |
| 5 | `emote_name` | STR | len+1 | Empty = no emote. See §3.4. |
| 6 | `loop_in_point` | F32 | 4 | Seconds. Must be finite. |
| 7 | `loop_out_point` | F32 | 4 | Seconds. Must be finite. |
| 8 | `loop` | S32 | 4 | `0` = no loop; any other value = loop (`static_cast<bool>`). |
| 9 | `ease_in_duration` | F32 | 4 | Seconds. Must be finite. |
| 10 | `ease_out_duration` | F32 | 4 | Seconds. Must be finite. |
| 11 | `hand_pose` | U32 | 4 | Fails if `> 14` (`LLHandMotion::NUM_HAND_POSES`). See §3.3. |
| 12 | `num_joints` | U32 | 4 | Fails if `0` or `> 216` (`LL_CHARACTER_MAX_ANIMATED_JOINTS`, `lljoint.h`). |

With an empty emote the header is **41 bytes**.

### 2.2 Joint block (repeated `num_joints` times)

| # | Field | Type | Bytes | Rule when reading |
|---|---|---|---|---|
| 1 | `joint_name` | STR | len+1 | `mScreen` and `mRoot` fail. An unknown name fails on upload (`allow_invalid_joints = false`), but is only skipped when an existing asset plays. Aliases are accepted and changed to the canonical name (`LLVOAvatar::getJoint()` uses the avatar's joint alias map). |
| 2 | `joint_priority` | S32 | 4 | `< -1` fails. `-1` = use `base_priority`. No upper limit is checked here (§3.1). |
| 3 | `num_rot_keys` | S32 | 4 | `< 0` fails. |
| 4 | rotation keys | 8 bytes each | 8 × n | See below. |
| 5 | `num_pos_keys` | S32 | 4 | `< 0` fails. |
| 6 | position keys | 8 bytes each | 8 × n | See below. |

**Rotation key** (version 1):

| Field | Type | Meaning |
|---|---|---|
| `time` | U16 | `U16_to_F32(time, 0, duration)` seconds. Fails if outside `[0, duration]`. |
| `rot_x`, `rot_y`, `rot_z` | U16 ×3 | `U16_to_F32(v, -1, 1)` each: the x, y, z of a unit quaternion. `w` is not stored (§4.2). |

**Position key** (version 1):

| Field | Type | Meaning |
|---|---|---|
| `time` | U16 | `U16_to_F32(time, 0, duration)` seconds. Not range-checked (it cannot leave the range anyway). |
| `pos_x`, `pos_y`, `pos_z` | U16 ×3 | `U16_to_F32(v, -5, 5)` metres each (`LL_MAX_PELVIS_OFFSET = 5.0`). |

Keys of one joint are stored in a `std::map` keyed on the decoded time (`rCurve->mKeys[time] = rot_key`), so:

- keys may be written in any order; the map sorts them;
- two keys that decode to the same time are merged, **the later one in the file wins**, and the viewer logs "duplicated rotation keys that were removed";
- `serialize()` writes the keys back sorted by time, and writes the **real** key count (`mKeys.size()`), not the count from the file.

A joint may have 0 rotation keys and 0 position keys; it is then in the file but does nothing. Rotation keys set the joint's usage flag `ROT`, position keys `POS` (`deserialize()`, `joint_state->setUsage()`).

### 2.3 Constraints

After the last joint: `num_constraints` (S32). If it is `< 0` or `> 10` (`MAX_CONSTRAINTS`, `llkeyframemotion.cpp`) the viewer logs "Bad number of constraints... ignoring" and **reads no constraints, but the file is still accepted**. Otherwise each constraint is:

| # | Field | Type | Bytes | Rule when reading |
|---|---|---|---|---|
| 1 | `chain_length` | U8 | 1 | Fails if `> num_joints`. |
| 2 | `constraint_type` | U8 | 1 | `0` = point, `1` = plane (`EConstraintType`, `llbvhconsts.h`); `>= 2` fails. |
| 3 | `source_volume` | 16 bytes | 16 | Collision volume name, zero padded (the reader adds its own terminator after byte 16). Must be a known collision volume. |
| 4 | `source_offset` | V3 | 12 | Must be finite. |
| 5 | `target_volume` | 16 bytes | 16 | `GROUND`, or a known collision volume. |
| 6 | `target_offset` | V3 | 12 | Must be finite. |
| 7 | `target_dir` | V3 | 12 | Must be finite. Non-zero = use target offset (`mUseTargetOffset`). |
| 8 | `ease_in_start` | F32 | 4 | Must be finite. |
| 9 | `ease_in_stop` | F32 | 4 | Must be finite. |
| 10 | `ease_out_start` | F32 | 4 | Must be finite. |
| 11 | `ease_out_stop` | F32 | 4 | Must be finite. |

86 bytes each. The reader then walks `chain_length + 1` parents up from the source volume's joint and **fails if any of those joints is not animated in this file** ("No joint index for constraint"). Collision volume names in the default skeleton (`newview/character/avatar_skeleton.xml`): `PELVIS`, `BUTT`, `BELLY`, `LEFT_HANDLE`, `RIGHT_HANDLE`, `LOWER_BACK`, `CHEST`, `LEFT_PEC`, `RIGHT_PEC`, `UPPER_BACK`, `NECK`, `HEAD`, `L_CLAVICLE`, `L_UPPER_ARM`, `L_LOWER_ARM`, `L_HAND`, `R_CLAVICLE`, `R_UPPER_ARM`, `R_LOWER_ARM`, `R_HAND`, `R_UPPER_LEG`, `R_LOWER_LEG`, `R_FOOT`, `L_UPPER_LEG`, `L_LOWER_LEG`, `L_FOOT`.

**Recommendation for #63:** write `num_constraints = 0`. PinglyMotion has no constraint editor, and constraints only come from the BVH importer's `anim.ini` translation file, which in the current viewer holds only the header line `Translations 1.0` (`newview/app_settings/anim.ini`).

### 2.4 File size

```
size = 41 + len(emote_name)
     + Σ over joints ( len(joint_name) + 1 + 4 + 4 + 8·rot_keys + 4 + 8·pos_keys )
     + 4 + 86·num_constraints
```

The viewer computes the size by serializing into a dummy buffer (`LLKeyframeMotion::getFileSize()`, `LLBVHLoader::getOutputSize()`).

### 2.5 Old format (version 0.1)

Read only, never written (`deserialize()`, branch `old_version`): key times are `F32` seconds; rotations are three `F32` Euler angles in degrees turned into a quaternion with `mayaQ(x, y, z, ZYX)`; positions are `V3` in metres, clamped to ±5. PinglyMotion does not need to write this. For import (#134) it is optional; the old assets on the SL grid are not files users have on disk.

## 3. Meaning of the header fields

### 3.1 Priority

`LLJoint::JointPriority` (`llcharacter/lljoint.h`): `USE_MOTION_PRIORITY = -1`, `LOW_PRIORITY = 0`, `MEDIUM = 1`, `HIGH = 2`, `HIGHER = 3`, `HIGHEST = 4`, `ADDITIVE_PRIORITY = LL_CHARACTER_MAX_PRIORITY = 7`.

- `base_priority` is the motion's priority; joints with `joint_priority = -1` use it. While reading, the largest joint priority becomes the motion's `mMaxPriority` (`deserialize()`).
- `base_priority >= 7` is lowered to `6` on read. Values `5` and `6` pass unchanged. **Per-joint priorities have no upper check at all in `deserialize()`.**
- The BVH upload window changes priority with `LLKeyframeMotion::setPriority()`, which shifts every joint priority by the same delta and clamps each to `0…4` (`LOW_PRIORITY…HIGHEST_PRIORITY`). The `.anim` upload path does not call it, so a `.anim` file is the only way to upload priority 5 or 6, and different priorities per joint.
- The BVH importer writes `base_priority = 2` and every joint at `2` unless `anim.ini` says otherwise (`LLBVHLoader::reset()`: `mPriority = 2`; `applyTranslations()`: `joint->mPriority = mPriority + trans.mPriorityModifier`).

### 3.2 Loop and ease

- `loop_in_point`, `loop_out_point`: seconds from the start. The BVH importer writes `0` and `duration` when not looping (`LLBVHLoader::loadBVHFile()`: `if (!mLoop) mLoopOutPoint = mDuration`). The upload window sets them from its percent fields: `percent × 0.01 × duration` (`LLFloaterBvhPreview::onCommitLoop()`).
- The reader only checks that they are finite. It does not check `0 ≤ in ≤ out ≤ duration` (§8). PinglyMotion should write values inside that range (#91 already keeps loop out inside the animation).
- `ease_in_duration`, `ease_out_duration`: seconds. BVH defaults are `0.3` / `0.3` (`LLBVHLoader::reset()`). `setEaseIn()` / `setEaseOut()` clamp to `≥ 0`. For a non-looping animation the BVH importer scales both down so that `ease_in + ease_out ≤ duration` (`LLBVHLoader::optimize()`). The `.anim` reader checks neither.
- Ease is one duration for the whole motion, blending in and out against whatever plays underneath. There is no per-key curve: playback between keys is always linear (§5). A curve editor ([#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11)) therefore has to bake its curves into extra keys.

### 3.3 Hand pose

`LLHandMotion::eHandPose` (`llcharacter/llhandmotion.h`) and the morph names in `gHandPoseNames` (`llhandmotion.cpp`):

| Value | Enum | Morph |
|---|---|---|
| 0 | `HAND_POSE_SPREAD` | (none) |
| 1 | `HAND_POSE_RELAXED` | `Hands_Relaxed` |
| 2 | `HAND_POSE_POINT` | `Hands_Point` |
| 3 | `HAND_POSE_FIST` | `Hands_Fist` |
| 4 | `HAND_POSE_RELAXED_L` | `Hands_Relaxed_L` |
| 5 | `HAND_POSE_POINT_L` | `Hands_Point_L` |
| 6 | `HAND_POSE_FIST_L` | `Hands_Fist_L` |
| 7 | `HAND_POSE_RELAXED_R` | `Hands_Relaxed_R` |
| 8 | `HAND_POSE_POINT_R` | `Hands_Point_R` |
| 9 | `HAND_POSE_FIST_R` | `Hands_Fist_R` |
| 10 | `HAND_POSE_SALUTE_R` | `Hands_Salute_R` |
| 11 | `HAND_POSE_TYPING` | `Hands_Typing` |
| 12 | `HAND_POSE_PEACE_R` | `Hands_Peace_R` |
| 13 | `HAND_POSE_PALM_R` | `Hands_Spread_R` |

The BVH importer default is `1` (`LLBVHLoader::reset()`: `mHand = 1`). These are the old morph hand shapes; Bento finger bones are animated as ordinary joints and do not need this field. The reader accepts `14` (`word > NUM_HAND_POSES` instead of `>=`, an off-by-one in `deserialize()`); PinglyMotion must never write it.

### 3.4 Emote

- `emote_name` is the name of a built-in facial animation from the animation library (`LLAnimationLibrary::stringToAnimState()`, `llcharacter/llanimationstates.cpp`; compared case-insensitively).
- The upload window offers 19 emotes and stores the library name of the `ANIM_AGENT_EXPRESS_*` animation (`LLFloaterBvhPreview` `mIDList`, `LLKeyframeMotion::setEmote()`), for example `express_afraid_emote`, `express_anger_emote`, `express_toothsmile`, `express_bored_emote`, `express_cry_emote`, `express_disdain`, `express_embarrassed_emote`, `express_frown`, `express_kiss`, `express_laugh_emote`, `express_open_mouth`, `express_tongue_out`, `express_repulsed_emote`, `express_sad_emote`, `express_shrug_emote`, `express_smile`, `express_surprise_emote`, `express_wink_emote`, `express_worry_emote`.
- On read, an unknown name is cleared with a warning (not an error); `Closed_Mouth` is cleared on purpose; a name equal to the animation's own asset ID fails (`deserialize()`).
- Recommendation: offer exactly the upload window's list, write the library name, empty for none.

## 4. Numbers: quantisation and quaternions

### 4.1 U16 quantisation

From `llmath/llquantize.h` (LGPL 2.1, Linden Research):

- `F32_to_U16(val, lower, upper)`: clamp to `[lower, upper]`, then `floor((val − lower) / (upper − lower) × 65535)`. Note **floor**, not round.
- `U16_to_F32(ival, lower, upper)`: `ival × (1/65535) × (upper − lower) + lower`, and then *"make sure that zero's come through as zero"*: any result with `|val| < (upper − lower)/65535` becomes exactly `0`.

The three ranges used:

| Value | Range | Step | Code for 0 |
|---|---|---|---|
| key time | `0 … duration` | `duration / 65535` (0.92 ms at 60 s) | 0 |
| quaternion x, y, z | `−1 … 1` | 3.05·10⁻⁵ | 32767 (also 32768; both decode to exactly 0) |
| position | `−5 … 5` m | 0.153 mm | 32767 (also 32768) |

The writer does not call `F32_to_U16()` on the raw value. It first calls `LLVector3::quantize16()` (`llmath/v3math.cpp`), which replaces each component by `U16_to_F32(F32_to_U16(v))`, and then calls `F32_to_U16()` again on that result (`LLKeyframeMotion::serialize()`, `LLBVHLoader::serialize()`). Key times are quantised once, without `quantize16()`.

**This double step is not exact.** A short float32 simulation of these functions, made while writing this document (easy to redo from the formulas above), shows that the second `F32_to_U16()` often lands one code lower than the first: for random inputs about 9 % of rotation components and 14 % of position components end up one step lower than a single `F32_to_U16()` would give. Decoding a stored code and encoding it again (what the viewer does on every upload, §6) changes about 9 % of rotation codes, 14 % of position codes and up to 8 % of time codes by −1. The error is tiny (one step), but it matters for exact round-trip tests (§9).

### 4.2 Quaternion packing

From `llmath/llquaternion.cpp`:

- `LLQuaternion::packToVector3()`: normalises x, y, z by the length of (x, y, z, w); if `w < 0` it negates x, y, z, so the stored quaternion always has `w ≥ 0`. Only x, y, z are written.
- `LLQuaternion::unpackFromVector3()`: x, y, z from the vector, `w = sqrt(1 − x² − y² − z²)`, or `0` if that is not positive.

So a rotation is stored as the unit quaternion with `w ≥ 0` (any rotation, 0…180°, can be stored this way). The decoded quaternion is not renormalised; after rounding, `x² + y² + z²` can be slightly above 1, which gives `w = 0`.

### 4.3 Quaternion and vector conventions

Needed to turn PinglyMotion's Euler angles into `.anim` quaternions, from `llmath/llquaternion.h/.cpp`:

- `LLQuaternion` stores `mQ[4]` as x, y, z, w.
- **Multiplication is reversed from the textbook (Hamilton) order:** `operator*=` computes `a = b ⊗ a` (see the formula in `llquaternion.h`). So in LL code `a * b` means "rotate by `a`, then by `b`".
- Vectors are row vectors: `v * q` rotates `v` by `q`, and `v * M` multiplies a row vector by a matrix.
- `mayaQ(x, y, z, order)` builds a rotation from degrees. For order `ZYX` it computes `zQ * yQ * xQ` (LL order), that is "Z first, then Y, then X" about fixed axes, which equals the matrix `Rx·Ry·Rz` in column-vector notation.

## 5. Playback rules that affect export

From `llcharacter/llkeyframemotion.cpp`:

- **Interpolation is always linear.** `deserialize()` sets `mInterpolationType = IT_LINEAR` for every curve. Rotations use `nlerp` (normalised linear interpolation, not slerp) in `RotationCurve::interp()`; positions use `lerp` in `PositionCurve::interp()`. Spline interpolation is not used.
- **Before the first key** the value of the first key is held; **after the last key** the value of the last key is held (`RotationCurve::getValue()`, `PositionCurve::getValue()`). So a joint does not need keys at exactly 0 and `duration`, but the BVH importer always writes them for joints it keeps.
- `nlerp` between keys far apart (large angles) moves faster in the middle than at the ends. Long holds with big turns should get extra keys if the motion must look like PinglyMotion's own interpolation. PinglyMotion's preview should use the same `nlerp` when it shows an `.anim`.
- Rotation keys are the joint's **local rotation relative to its rest orientation**, in SL axes (§7). The SL skeleton's rest rotations are all identity, so a key with x = y = z = 0 is the rest pose.
- Position keys set the joint's local position (`JointMotion::update()` → `joint_state->setPosition()`). See §7.3 for what the importer puts there.

## 6. What the viewer and server enforce on upload

**Viewer, `.anim` file** (`newview/llviewerassetupload.cpp`, `LLNewFileResourceUploadInfo::exportTempFile()`; the window is `LLFloaterAnimPreview` in `newview/llfloaternamedesc.cpp`, a plain name/description dialog with no preview):

1. Reads the whole file.
2. Runs `LLKeyframeMotion::deserialize(dp, asset_id, false)` against the user's own avatar. All checks in §2 apply, and **unknown joint names fail** (`allow_invalid_joints = false`).
3. If that succeeds, writes the motion out again with `LLKeyframeMotion::dumpToFile()` → `serialize()`, and **uploads the re-serialized file, not the user's bytes.** Consequences: duplicate-time keys are gone, the key counts are the real ones, aliases are replaced by canonical joint names, an unknown emote is cleared, `base_priority ≥ 7` is 6, a bad constraint count drops all constraints, and every quantised value has gone through decode + encode once (§4.1).
4. The same function also serves bulk upload: `.anim` files go through the steps above, `.bvh` files are refused there ("We do not currently support bulk upload of animation files").

**Viewer, `.bvh` file** (`newview/llfloaterbvhpreview.cpp`, `LLFloaterBvhPreview::postBuild()` and `onBtnOK()`): the BVH is converted with `LLBVHLoader`, serialized, deserialized with `allow_invalid_joints = false`, changed by the window's settings (priority via `setPriority()`, loop, ease, hand pose, emote), and serialized again for upload. Duration must be `≤ 60 s` (`MAX_ANIM_DURATION`), checked before conversion.

**Limits:**

| Limit | Value | Where it is enforced | Source |
|---|---|---|---|
| Duration | ≤ 60.0 s | viewer (`deserialize()`, BVH window) | `llbvhconsts.h` `MAX_ANIM_DURATION` |
| Joints per animation | 1 … 216 | viewer (`deserialize()`) | `lljoint.h` `LL_CHARACTER_MAX_ANIMATED_JOINTS` |
| Constraints | 0 … 10 (else ignored) | viewer (`deserialize()`) | `llkeyframemotion.cpp` `MAX_CONSTRAINTS` |
| Position range | ±5 m per axis | by the encoding | `lljoint.h` `LL_MAX_PELVIS_OFFSET` |
| File size | 250,000 bytes | **server** (not in viewer source) | SL wiki "Internal Animation Format" ("The asset uploader enforces a 250,000 byte limit on .anim files") and "Limits" ("Size is after conversion to .anim") |
| Priority | BVH window 0…4; `.anim` up to 6 | viewer | §3.1; the wiki "Limits" page lists 4 for BVH and 6 for `.anim` |
| Animations playing at once | 30 | server / viewer | SL wiki "Limits" (not needed for export) |

There is **no limit on the number of keys** other than the file size: at 8 bytes per key, 250,000 bytes allows roughly 31,000 keys in total, minus the header and joint names. The server is closed source; the size limit is the only server rule documented. Everything else the server might check is an open question (§10).

## 7. How the BVH importer turns BVH into `.anim`

All from `llcharacter/llbvhloader.cpp`, class `LLBVHLoader`. This is the reference for making PinglyMotion's `.anim` export match what users get today when they upload PinglyMotion's BVH.

### 7.1 Joint names and translation

- The constructor loads `anim.ini` (only the header today) and then calls `makeTranslation(alias, joint)` for every entry of the avatar's joint alias map (`LLFloaterBvhPreview::getJointAliases()` → `LLVOAvatar::getJointAliases()`). That map turns BVH names such as `hip` into skeleton names such as `mPelvis`.
- The BVH root must translate to the same name as `hip`, i.e. `mPelvis` (`loadBVHFile()`, "DEV-26188").
- Every translated joint gets the same **frame matrix** with rows `(0,1,0)`, `(0,0,1)`, `(1,0,0)` (`makeTranslation()`).
- `mPelvis` also gets `mRelativePositionKey = true` and `mRelativeRotationKey = true` (`makeTranslation()`).

### 7.2 Axes

With LL's row-vector convention, `v_SL = v_BVH * F` with the frame matrix above gives

```
SL x = BVH z      (forward)
SL y = BVH x      (left)
SL z = BVH y      (up)
```

Rotations are changed the same way: `outRot = frameRotInv * inRot * frameRot` (LL order, `LLBVHLoader::serialize()`), a change of basis of the BVH rotation into SL axes. `offsetRot` is identity because `anim.ini` sets no offset matrix.

BVH Euler angles become a quaternion with `mayaQ(rx, ry, rz, bvhStringToOrder(channel order))`. `bvhStringToOrder()` reverses the BVH channel order string ("XYZ order in BVH files must be passed to mayaQ() as ZYX"), so a BVH joint with `CHANNELS 3 Zrotation Xrotation Yrotation` uses `mayaQ(..., YXZ)` = `Rz·Rx·Ry` (column vectors), the usual BVH meaning.

### 7.3 Pelvis and positions

- **Pelvis rotation:** relative to frame 0. With `mRelativeRotationKey`, `first_frame_rot` is the pelvis rotation in BVH frame 0 and `outRot = frameRotInv * inRot * ~first_frame_rot * frameRot` (LL order). For a BVH whose frame 0 has zero rotation (PinglyMotion's BVH, the usual SL convention) this is just the axis change.
- **Pelvis position:** relative to frame 0: `inPos = (pos − pos_frame0) * ~first_frame_rot`, then `outPos = inPos * frameRot * offsetRot`, then **× 0.02540005** (`INCHES_TO_METERS`: BVH units are taken as inches), then clamped to ±5 m. So the `.anim` pelvis position is an offset in metres from where the pelvis was in frame 0.
- **Other joints:** position keys are written only for joints with 6 channels (`applyTranslations()`: `mIgnorePositions = (mNumChannels == 3)`; "Allow joint position changes as of SL-318"). They are **not** made relative to frame 0, so the BVH channel values, converted to metres in SL axes, become the joint's local position.
- A joint with 3 channels gets `num_pos_keys = 0`.

### 7.4 Time

- Frame 0 is a reference frame and is not played when the file has more than one frame (`NUMBER_OF_IGNORED_FRAMES_AT_START = 1`).
- Key time for frame *k*: `max(k − 1, 0) × frame_time`.
- `duration = max(frames − 2, 1) × frame_time` (`loadBVHFile()`, `NUMBER_OF_UNPLAYED_FRAMES = 2`). The last frame is the target that the second last frame interpolates towards and sits exactly at `duration`.
- Example: 31 frames at 1/30 s → frame 1 at 0 s, frame 30 at 29/30 = 0.967 s = duration.

### 7.5 Key reduction (`optimize()`)

The importer drops keys that a straight line between neighbours already explains:

- Rotation: a key is kept when the deviation of the rotated x and y axes from the interpolated rotation is `≥ 0.01 / max(0.33 × depth_of_child_tree, 1)` (`ROTATION_KEYFRAME_THRESHOLD`; joints with long chains below them keep more keys), and the point of largest deviation since the last kept key is added back.
- Position: a key is kept when it is more than `0.03` BVH units (inches, before conversion) from the interpolated position (`POSITION_KEYFRAME_THRESHOLD_SQUARED`).
- A joint whose rotation never moves more than `0.001` and position never more than `0.001` units from frame 0 is left out completely (`ROTATION_MOTION_THRESHOLD`, `POSITION_MOTION_THRESHOLD_SQUARED`). This is why PinglyMotion's BVH only exports joints that move away from frame 0.

This reduction is not part of the format. A PinglyMotion exporter can write every key it has, or reduce with its own rule, as long as the file stays under 250,000 bytes.

### 7.6 Defaults the importer writes

From `LLBVHLoader::reset()` and `serialize()`: `version 1`, `sub_version 0`, `base_priority 2`, joint priority `2`, emote empty, `loop 0`, `loop_in 0`, `loop_out = duration`, `ease_in 0.3`, `ease_out 0.3`, `hand_pose 1`, `num_constraints 0`. The upload window then lets the user change priority, loop, ease, hand pose and emote before the final serialize.

## 8. Reader weaknesses to avoid in our own parser (#134)

The viewer's reader trusts more than PinglyMotion's should. A PinglyMotion parser must check these itself (and gets a fuzz target from the start, #62, #20):

- `LLDataPackerBinaryBuffer::unpackString()` calls `strlen()` on the buffer **before** checking the length, so a file without a terminating `0x00` reads past the end of the buffer (`llmessage/lldatapacker.cpp`). Our reader must search for the terminator only inside the remaining bytes, and should cap names (joint names in the SL skeleton are short; a cap of 64 bytes is plenty).
- `num_rot_keys` and `num_pos_keys` are only checked for `< 0`; a huge count is caught only when the data runs out. Check `count × 8 ≤ remaining bytes` before reserving memory.
- Negative `duration` passes (`> 60` and non-finite are the only checks). Then every rotation key time fails the `0 … duration` check, unless there are no rotation keys. Require `0 < duration ≤ 60`.
- `loop_in_point`, `loop_out_point`, ease values: only checked for being finite. Require `0 ≤ loop_in ≤ loop_out ≤ duration` and ease `≥ 0` (or clamp, and tell the user).
- `hand_pose = 14` passes (off-by-one). Require `≤ 13`.
- Trailing bytes after the constraints are ignored. PinglyMotion may refuse them or ignore them with a warning.
- Per-joint priority has no upper bound. Clamp or refuse above 6.

## 9. Worked example

A one-second head turn: `mHead` from rest to 30° about the vertical axis (SL z, positive = turning to the avatar's left), priority 3, no loop, ease 0.3 s, hand pose 1, no emote, no position keys, no constraints.

The 30° turn about z is the quaternion `(0, 0, sin 15°, cos 15°) = (0, 0, 0.258819, 0.965926)`; `w ≥ 0`, so it is stored as is.

Quantising (`F32_to_U16(v, −1, 1)` after `quantize16()`):

- `0` → `floor(0.5 × 65535) = 32767` = `0x7FFF`
- `0.258819` → `floor(0.6294095 × 65535) = 41248` = `0xA120`
- time `0.0` → `0`, time `1.0` → `65535` = `0xFFFF`

| Offset | Bytes | Field | Value |
|---|---|---|---|
| 0x00 | `01 00` | version | 1 |
| 0x02 | `00 00` | sub_version | 0 |
| 0x04 | `03 00 00 00` | base_priority | 3 |
| 0x08 | `00 00 80 3F` | duration | 1.0 |
| 0x0C | `00` | emote_name | "" |
| 0x0D | `00 00 00 00` | loop_in_point | 0.0 |
| 0x11 | `00 00 80 3F` | loop_out_point | 1.0 |
| 0x15 | `00 00 00 00` | loop | 0 |
| 0x19 | `9A 99 99 3E` | ease_in_duration | 0.3 |
| 0x1D | `9A 99 99 3E` | ease_out_duration | 0.3 |
| 0x21 | `01 00 00 00` | hand_pose | 1 (relaxed) |
| 0x25 | `01 00 00 00` | num_joints | 1 |
| 0x29 | `6D 48 65 61 64 00` | joint_name | "mHead" |
| 0x2F | `03 00 00 00` | joint_priority | 3 |
| 0x33 | `02 00 00 00` | num_rot_keys | 2 |
| 0x37 | `00 00` `FF 7F` `FF 7F` `FF 7F` | key 1 | t = 0 s, (0, 0, 0) |
| 0x3F | `FF FF` `FF 7F` `FF 7F` `20 A1` | key 2 | t = 1 s, (0, 0, 0.258808) |
| 0x47 | `00 00 00 00` | num_pos_keys | 0 |
| 0x4B | `00 00 00 00` | num_constraints | 0 |

Total 79 bytes (41 + 6 + 4 + 4 + 16 + 4 + 4), matching the size formula in §2.4. Hex dump:

```
0000  01 00 00 00 03 00 00 00 00 00 80 3f 00 00 00 00
0010  00 00 00 80 3f 00 00 00 00 9a 99 99 3e 9a 99 99
0020  3e 01 00 00 00 01 00 00 00 6d 48 65 61 64 00 03
0030  00 00 00 02 00 00 00 00 00 ff 7f ff 7f ff 7f ff
0040  ff ff 7f ff 7f 20 a1 00 00 00 00 00 00 00 00
```

Decoding key 2: `U16_to_F32(41248, −1, 1) = 0.258808`, `w = sqrt(1 − 0.258808²) = 0.965930`, angle `2·atan2(z, w) = 29.9987°`: the loss is about 0.001°. `U16_to_F32(32767, −1, 1) = −1.5·10⁻⁵`, which the zero rule turns into exactly `0`.

PinglyMotion's Euler → SL quaternion path for this key: the head's BVH channels (Y-up, Z-forward) have `Yrotation = 30`, which after the axis change of §7.2 is a rotation about SL z.

## 10. Open questions

The source does not settle these; each needs a test (in the viewer's preview, or an upload on the Aditi beta grid) before #63 is called done.

1. **Server checks.** The upload server is closed source. Besides the 250,000-byte limit, does it re-check duration, joint count, priority (5 and 6), joint names or constraints? The wiki says priority 6 is allowed for `.anim`; not verified.
2. **Non-pelvis positions.** The BVH importer writes them as absolute local positions (§7.3) and the pelvis as an offset from frame 0. How the viewer combines a position key with the joint's default position (and with avatar shape and joint offsets from rigged mesh) is decided by the pose blender (`llcharacter/llmotioncontroller.cpp` / `llpose.cpp`), which this document has not traced. The SL wiki says positions are "relative to the avatar centre, except mPelvis"; unverified. Test: export a pelvis and a hand position from PinglyMotion as BVH, upload in the viewer's preview, compare with the same values in an `.anim`.
3. **Round trip after upload.** Because the viewer re-encodes every `.anim` (§6, §4.1), the uploaded asset can differ by one quantisation step from PinglyMotion's file. To make the stored file identical, the writer can pick codes that are fixed points of decode → encode (`F32_to_U16(quantize16(U16_to_F32(c))) == c`), choosing the nearest such code. This needs a test with the real compiled functions: our numbers come from a float32 simulation, and the compiler may evaluate the C++ with different rounding (x87, FMA).
4. **Floor vs. round.** The viewer encodes with floor (bias of up to −1 step). Should PinglyMotion copy this exactly (bit-identical with a viewer re-serialize) or round (half the error)? Answer depends on question 3.
5. **Ease and loop when looping.** `optimize()` scales ease only for non-looping motions. What the motion controller does with ease when the loop points are inside the animation is not traced here (`LLMotionController`, `LLKeyframeMotion::onUpdate()`).
6. **Emote timing.** The emote is played along with the motion; its timing against ease in/out and loop is not traced.
7. **Firestorm.** #63 notes that some viewers handle `.anim` uploads differently. Firestorm's `exportTempFile()` and `LLKeyframeMotion` have not been compared with the Linden viewer here.
8. **Collision volumes and attachment points as joints.** `LL_CHARACTER_MAX_ANIMATED_JOINTS` counts bones, collision volumes and attachment points ("#bones + #collision_volumes + #attachments + 2", `lljoint.h`). Whether animating collision volumes in an `.anim` is useful or accepted by the server is unknown; PinglyMotion should write bones only.
9. **Constraint behaviour.** Not needed for #63. Reports elsewhere say long chains break playback and `GROUND` targets are refused by the server; not checked.

## 11. Checklist for #63 and #134

Export (#63):

- Write version `1.0`, little endian, zero-terminated strings, no padding.
- Duration `0 < d ≤ 60`; key times `0 … d`; keys sorted, no two keys of one joint with the same U16 time.
- Rotations: PinglyMotion Euler → quaternion in SL axes (§4.3, §7.2), normalise, flip sign if `w < 0`, quantise.
- Positions: metres in SL axes, pelvis as offset from rest (§7.3), clamp to ±5 m, quantise.
- Only joints that move; canonical SL joint names; never `mScreen` or `mRoot`; at most 216.
- Priority: base and per joint, `-1…6`; ease `≥ 0`, `ease_in + ease_out ≤ duration` for non-looping; loop points inside `0 … duration`; hand pose `0…13`; emote from the list in §3.4 or empty; `num_constraints = 0`.
- File size `≤ 250,000` bytes: compute with §2.4 before writing, and warn (or reduce keys) above it.
- Tests: byte-exact comparison with the worked example in §9; decode with `scripts/content_tools/anim_tool.py` as an independent reader; compare with a BVH upload of the same animation in the viewer.

Import (#134):

- Same rules as `deserialize()` (§2), plus the extra checks in §8.
- Old version 0.1 optional.
- Round-trip test: export → import → compare, allowing one quantisation step (§4.1).
- Only for the user's own files; it must not become a way around viewers' export protection.

## 11. Status of the writer (issue #10)

`src/animexport.*` writes `.anim` files from the parsed animation data (`AnimFile::Animation`) as a function; the menu item File > Export for Second Life (.anim) and its dialog are in `src/animexportdialog.*` (the dialog asks for priority, slow start and finish, loop and hand pose; a too long animation is refused before the dialog with the 60 second limit and what to do). It follows sections 2 to 7: axis change, root joint relative to frame 0 in metres (BVH units taken as inches), frame 0 not played, the viewer's quantising (also the double step of section 4.1), priority per joint, loop points, ease, hand pose, emote, no constraints. Joints that never move away from frame 0 are left out (the root is always kept). When the file would be bigger than 250,000 bytes, keys that a straight line explains are removed, a little more each round, and the user gets a warning; if that is not enough, the export fails with a message.

Not done: choosing code values that survive the viewer's decode and encode unchanged (open question 3), and the checks against the real server (open questions 1 and 2). Both need an upload test by the project owner.

### Status of the reader (issue #22, step 1)

`src/animimport.*` reads `.anim` bytes into a plain data model (`AnimImport::Motion`: header, joints, rotation and position keys in seconds, metres and quaternions, constraints) and checks every limit of section 8: names are searched only inside the bytes that are left (64 bytes at most), counts are checked against the bytes that are left, the length must be above 0 and at most 60 seconds, hand pose 0 to 13, priorities above 6 are lowered to 6 with a warning, loop points and negative ease times are moved inside range with a warning, duplicate key times are merged (the later one wins, as in the viewer), a bad constraint count is ignored like the viewer does, trailing bytes are ignored with a warning. Version 0.1 files are refused with a message. It has its own fuzz target (`fuzz/fuzz_anim.cpp`).

Step 2, done: `src/animimportconvert.*` turns the keys into an animation on the Second Life skeleton of the program (`bin/data/SLFemale.bvh`): resampled at 30 frames per second, frame 0 the rest pose, the axes changed back, Euler angles per joint in the joint's own order, the pelvis relative to frame 0 and the hip position in inches. A round trip test (write, read, convert) keeps the joint turns within 0.3 degrees. Joints that are not on the skeleton, moves of joints other than the hip, constraints, priority, ease and hand pose are not imported; the menu item File > Import Second Life .anim lists what was left out in plain words.
