# PinglyMotion and Blender

Status: **guide, written 19 November 2026 and checked in a real Blender (5.3) on 20 November 2026 (the sections on what is planned in section 3 are older: glTF import ([#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12)) and the retargeting menu ([#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16)) now exist, see 1.7).** Sections 1 and 2 describe BVH files. Issue: [#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13). Decisions behind it: [DECISIONS.md](DECISIONS.md) (18 Nov 2026, "Supported file formats and their order").

The facts about PinglyMotion below come from its source code (`src/bvh.cpp`, `src/animfile.h`, `bin/data/bento_joints.txt`, `bin/data/TPose.bvh`). The facts about Blender and Second Life come from the sources at the end. The BVH route (section 1) was run for real in Blender 5.3 with the scripts in 1.6; the glTF route is only described. Things not tested are marked "not tested"; see [Open questions](#4-open-questions).

## Who this is for

You make animations for Second Life, and you want to use Blender for part of the work: for example, block out a motion in PinglyMotion, refine curves in Blender, and bring it back to PinglyMotion to check it on the SL body and export it. Or the other way round. You do not need to be a programmer.

## A few words first

- **BVH** is an old, simple text format for motion. It stores a skeleton (joint names, how they hang together, bone offsets) and then one line of numbers per frame. Both PinglyMotion and Blender read and write it. Second Life accepts it for upload.
- **Joint** and **bone** mean the same thing here.
- **Axes:** PinglyMotion's BVH files are "Y up" (Y points to the sky, Z points forward). Blender is "Z up". Every trip between the two has to turn the skeleton by 90 degrees, and that is where most problems come from.
- **Units:** Second Life reads BVH numbers as **inches**. PinglyMotion's bundled skeleton has the hip about 43.5 units above the floor, which is about 1.1 metres.
- **Frame rate:** frames per second (fps). PinglyMotion's bundled files use 30 fps. A new Blender scene usually uses 24 fps.

## 1. What works today: BVH out and back in

### 1.1 What PinglyMotion writes

When you use **File > Export BVH**, PinglyMotion writes:

| Thing | What PinglyMotion writes |
|---|---|
| Root joint | `hip`, with 6 channels: 3 positions, then 3 rotations |
| Other joints | 3 rotation channels each, no positions |
| Joint names | The classic SL names for the old body joints (`hip`, `abdomen`, `chest`, `neck`, `head`, `lCollar`, `lShldr`, `lForeArm`, `lHand`, `lThigh`, `lShin`, `lFoot` and the right side with `r`), and the Bento names for the rest (`mSpine1`, `mFaceRoot`, `mHandIndex1Left`, `mWing1Left`, `mTail1` …) |
| Rotation order | Not the same for every joint. Most joints use `Xrotation Zrotation Yrotation`; the collars use `Y Z X`, the shoulders `Z Y X`. Blender must keep each joint's own order (see 1.3) |
| Axes | Y up, Z forward (the usual BVH and SL convention) |
| Units | Inches |
| Frame rate | The FPS value in the program (a whole number), written as `Frame Time` |
| First frame | The reference frame. Second Life does not play it; it uses it as the starting point |
| Every frame | Every frame is written in full, so Blender sees a key on every frame |

### 1.2 What PinglyMotion reads back

When you open a `.bvh` file, PinglyMotion puts the motion on its **own** Bento skeleton:

- **Joints are matched by name.** It accepts three spellings for each joint: its own name (`lShldr`), the SL name (`mShoulderLeft`), and the SL name with `avatar_` in front (`avatar_mShoulderLeft`). The full list is `bin/data/bento_joints.txt`.
- **Joints with any other name are dropped without a warning.** Their children move up to the next known joint. So a bone you renamed in Blender, or a Rigify or Mixamo bone, simply does nothing.
- **Bone lengths in the file are ignored.** PinglyMotion keeps its own skeleton. Making a bone longer in Blender has no effect.
- **Rotations are taken as three angles** and applied in PinglyMotion's own order for that joint (see 1.1). If the file uses a different order for a joint, the angles are read in the wrong order (see 2.4).
- **The hip position is taken as it is**, in the file's units. If the file is in metres instead of inches, the hip hardly moves.
- **Every frame becomes a key frame.**
- Limits: at most 64 MB, 256 joints, 10,000 frames and a frame time between 0.001 and 10 seconds (`src/animfile.h`). Second Life itself allows at most 60 seconds per animation.

### 1.3 Step by step: PinglyMotion → Blender

The add-on "BioVision Motion Capture (BVH) format" comes with Blender and is normally switched on. If **File > Import > Motion Capture (.bvh)** is missing, open **Edit > Preferences > Add-ons**, search for "BVH" and tick it.

1. In PinglyMotion, note the FPS value. Save your work as `.dingle` first (BVH does not keep key frames, easing or loop points, see 2.6). Then export the animation as `.bvh`.
2. In Blender, start a new scene. In **Output Properties > Format**, set **Frame Rate** to the same value as in PinglyMotion (for example 30).
3. **File > Import > Motion Capture (.bvh)**. In the import window:
   - **Target:** Armature.
   - **Scale:** leave at **1.0**. The skeleton will look very big (about 44 metres high), because Blender takes the inches as metres. That is fine: the numbers stay the same, and the way back stays simple. If you scale here, you must scale back by exactly the opposite amount when you export (see 2.2).
   - **Rotation:** leave at the default, which keeps each joint's own Euler rotation order from the file. Do **not** choose Quaternion.
   - **Forward / Up:** these turn BVH's "Y up" into Blender's "Z up". The usual setting for SL files is Forward **-Z**, Up **Y** (the add-on's default in Blender 5.3, checked).
   - **Start Frame:** 1 is fine. Remember that this first frame is the reference frame.
   - **Update Scene FPS** and **Update Scene Duration:** tick both (both are off by default in Blender 5.3), so the scene matches the file. Update Scene Duration only ever makes the scene longer, never shorter: a new scene is 250 frames long, so set the End frame to the file's last frame yourself before you export. Leave **Scale FPS** off when the frame rates already match.
4. The skeleton should now stand upright on the floor grid. Play the animation and check that it looks like it did in PinglyMotion.

### 1.4 Step by step: Blender → PinglyMotion

1. Do not rename bones and do not add or remove bones that PinglyMotion should see. Extra helper bones (controls, IK targets) are dropped by PinglyMotion, which is fine, but the motion must be baked onto the real SL bones (see 2.5).
2. Keep frame 1 (the first frame of the range) as the reference pose. Animate from the second frame on.
3. Select the armature. **File > Export > Motion Capture (.bvh)**. In the export window:
   - **Scale:** the opposite of what you used on import. With 1.0 on import, use **1.0** here.
   - **Rotation:** leave at the default, which writes each bone's own rotation order. Do not force one order for all bones (see 2.4).
   - **Root Translation Only:** **tick it**. Then only the hip gets position channels, as in PinglyMotion's own files. Without it every bone gets 6 channels; PinglyMotion and Second Life keep positions only for the hip unless you really want bone positions.
   - **Start / End:** the frame range of your animation, starting with the reference frame. The default Start is 0, not 1; set both to the real range, or the file gets the scene's 250 frames.
4. Turn the axes (see 2.1): the exporter has no Forward/Up setting and writes Blender's own Z-up axes. **This is needed for every skeleton, also one that came in through the BVH importer** (tested in Blender 5.3: without it Y and Z are swapped in every bone offset). In Object Mode select the armature, rotate it by -90 degrees around X, then **Object > Apply > Rotation**, then export. Do this on a copy of your work, or undo it afterwards.
5. In PinglyMotion, open the `.bvh`. Check: the avatar stands upright, faces the same way as before, and the hip is at the right height. Then save as `.dingle`.

### 1.5 A quick check you can do once

Before real work, do one round trip with something simple: in PinglyMotion, make a 2-second animation where the head turns to the left and the hip moves 10 units forward. Export, import into Blender, export again without changing anything, open in PinglyMotion. The head must still turn left (not right, not nod), and the hip must move forward by the same distance. If one of these is wrong, see section 2 before you put more work in.

### 1.6 Scripted round trip

Status: run for real on 20 November 2026 in Blender 5.3 (the `bpy` module, headless). The bundled skeleton and four motion files (CMU walk and run, a Mixamo-style wave, the SL female skeleton) all come back identical. The CI job `Blender round trip` (`.github/workflows/blender.yml`, not required) repeats this when the scripts change, or by hand. The comparer is also tested in CI without Blender.

Two small scripts in `tools/blender/` do the check from 1.5 without clicking:

- `pinglymotion_roundtrip.py` runs inside Blender. It sets the scene frame rate to the file's, imports the BVH with the settings from 1.3 (Forward -Z, Up Y, Scale 1.0, own rotation order per joint), turns the armature -90 degrees around X and applies it, sets the frame range to the file's, exports it again with the settings from 1.4 (Scale 1.0, own rotation order, Root Translation Only), and can also write a glTF binary (`.glb`) with the animation.
- `bvh_compare.py` needs only Python, no Blender. It compares two BVH files: joint names and order, parents, channels, frame count, frame time, bone offsets and every frame value within a tolerance (default 0.001; angles are compared as angles, so 180 and -180 are equal).

Run it:

```
blender -b --python tools/blender/pinglymotion_roundtrip.py -- in.bvh out.bvh --check
blender -b --python tools/blender/pinglymotion_roundtrip.py -- in.bvh out.bvh --gltf out.glb
python3 tools/blender/bvh_compare.py in.bvh out.bvh --tol 0.01
```

`--check` makes the Blender run fail (exit code 1) when the result differs. Joint names are never changed, so a difference in names means the rig, not the script. A small difference in rotation values after Blender is normal if Blender wrote fewer decimals; raise `--tol` a little before you decide something is wrong.

To run it without installing Blender: `pip install bpy` (Python 3.13 for bpy 5.3), then `python3 tools/blender/pinglymotion_roundtrip.py -- in.bvh out.bvh --check`.

The glTF file it writes (`--gltf`) keeps the SL joint names and the animation (one clip, rotation channels for every joint) and uses the numbers unchanged, so inches count as metres. See 1.7 for using glTF.

If the check finds differences in axes, scale or rotation order, section 2 lists the usual causes.

### 1.7 glTF from Blender into PinglyMotion

PinglyMotion has **File > Import glTF animation** (it uses the same joint-matching window as BVH from another skeleton, see [RETARGET_MAPPING.md](RETARGET_MAPPING.md)). In Blender use **File > Export > glTF 2.0** with Format glTF Binary (.glb), tick **Animation**, tick **Include > Selected Objects** with your armature selected, and keep **+Y Up** on (Blender converts its Z-up to glTF's Y-up for you, so no -90 degree turn is needed). Keep the SL joint names, so the window can match them without hand work. glTF has no inches: Blender writes the numbers as metres, so a skeleton that came in from a PinglyMotion BVH with Scale 1.0 stays 44 "metres" high, which PinglyMotion's retargeting does not care about because it keeps its own skeleton. Not tested: opening a `.glb` written by Blender in PinglyMotion's window; the script only checks that Blender writes the file with the names and animation.

Mixamo warning (also in 2.9): Mixamo's terms allow its animations in your own projects but not passing the raw files on. The scripts do not check where a motion comes from.

## 2. What goes wrong and how to avoid it

### 2.1 The avatar lies on its back or faces sideways

The axes were not turned, or turned twice. Check the import Forward/Up settings, and for a skeleton built in Blender, rotate the armature -90 degrees around X and apply the rotation before export (1.4, step 4; tested, also needed for a skeleton that came in through the importer). Second Life itself turns BVH axes like this: SL forward = BVH Z, SL left = BVH X, SL up = BVH Y ([ANIM_FORMAT.md](ANIM_FORMAT.md), §7.2).

### 2.2 The hip flies away or hardly moves

The scale does not match. Second Life multiplies BVH hip positions by 0.0254 (inches to metres) and limits the hip to 5 metres from its start. If you imported with Scale 0.0254 to get a "real size" rig, export with Scale 39.37 (1 ÷ 0.0254). The simplest rule: **use 1.0 both ways.**

### 2.3 The animation is too fast or too slow

The frame rates differ. Set Blender's scene frame rate to PinglyMotion's FPS before import, or tick **Update Scene FPS**. PinglyMotion only shows whole numbers, so avoid frame rates like 23.976 or 29.97.

### 2.4 Arms or shoulders twist strangely in big poses

The rotation order changed. PinglyMotion uses a different order for different joints and, when reading a file, applies its own order to the three angles. If Blender writes all joints in one order (for example because "Rotation" was set to XYZ on export, or the bones were switched to Quaternion while you worked), small rotations still look almost right, but large rotations on two axes come out wrong. Keep the rotation setting at its default on import and export, and do not change the bones' rotation mode in Blender. Whether PinglyMotion should convert between orders on import is an open question (section 4).

### 2.5 Some joints do not move at all

- The bone name does not match (1.2). Check for typos, `.L`/`.R` endings that Blender adds when you mirror, or names from another rig.
- The motion is on a control bone, not on the SL bone. Bake it first: **Pose > Animation > Bake Action**, with "Visual Keying" on, onto the deform bones.
- In Second Life: a joint that never moves away from the first frame is left out of the upload ([ANIM_FORMAT.md](ANIM_FORMAT.md), §7). That is normal.
- In Second Life: Bento bones (fingers, face) keep their last pose after an animation stops unless another animation sets them. That is SL behaviour, not a file problem.

### 2.6 Key frames, easing and loop points are gone

BVH has no key frames, no easing and no loop points: every frame is a full pose. After a trip through BVH, PinglyMotion shows a key on every frame. Keep your PinglyMotion work as `.dingle` and use BVH only for the part you edit in Blender. Set loop, ease and priority again in PinglyMotion or in the SL upload window.

### 2.7 The first frame plays as a jump

Second Life does not play the first frame of a BVH; it uses it as the reference. If you delete it in Blender or start animating on it, the real first pose is lost or the animation starts with a jump. Keep the reference pose on the first frame.

### 2.8 Bone positions you did not want

If **Root Translation Only** is off, every bone gets position channels. Second Life then moves bones away from where the wearer's shape and mesh body put them. Tick Root Translation Only unless you really want to move bones.

### 2.9 Licence of the motion (Mixamo and others)

The file format does not care where the motion comes from, but you should. Mixamo's terms allow using its animations in your own projects but not passing the raw files on, so selling an almost unchanged Mixamo clip in Second Life may break its terms. PinglyMotion does not block this; it is your responsibility. Motion from sources with a clear free licence (for example the CC0 animations from Mesh2Motion, see [OPEN_SOURCE_CHECK.md](OPEN_SOURCE_CHECK.md)) has no such problem.

## 3. What a real round trip needs later

The BVH route above works only when the Blender skeleton already has SL joint names and the SL rest pose. Most Blender rigs do not.

### 3.1 Planned

- **Retargeting from any skeleton ([#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16)).** Reads a BVH from another skeleton (Mixamo, Rigify, motion capture), finds which bone is which by name and position in the tree, and moves the motion onto the SL Bento skeleton. The core is in the code since pull request #66, tested but without a menu yet. With a menu, "bones with other names do nothing" (2.5) goes away.
- **glTF 2.0 import ([#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12)).** Blender exports glTF (`.glb`/`.gltf`) with its built-in exporter. glTF stores the rest pose, the bone names, metres and its own axis rule, so scale, axes and frame rate mistakes (2.1 to 2.3) mostly disappear. It uses the retargeting core from #16. When #12 is done, this guide gets a glTF section, and glTF becomes the recommended way from Blender into PinglyMotion.
- **`.anim` export** is already done (#63): after editing in Blender and coming back, PinglyMotion can write SL's own `.anim` format with priority, ease and loop points, which BVH upload cannot.

### 3.2 Out of scope (decided)

- **glTF export** from PinglyMotion: Second Life has no glTF animation upload; BVH and `.anim` are enough for the way back into Blender.
- **FBX:** only on real demand; Blender converts FBX to BVH or glTF for free.
- **Collada (`.dae`):** legacy in Blender and not used for SL animation.
- **Reading `.blend` files:** not a stable exchange format.
- **A PinglyMotion add-on inside Blender** or a live link to Blender: not planned. (The live link that exists goes to the Aurora viewer, see [AURORA_LINK.md](AURORA_LINK.md).)
- **Rigging and modelling:** Blender stays the tool for meshes and rigs; PinglyMotion is for animation and SL export ([PRODUCT.md](PRODUCT.md)).

### 3.3 Other tools people use today

These are not part of PinglyMotion; they are listed so you know the landscape.

| Tool | What it does | Licence / price |
|---|---|---|
| Blender's BVH add-on | BVH import and export, comes with Blender | Part of Blender, GNU GPL |
| Avastar (Machinimatrix) | Blender add-on with the full SL skeleton; exports BVH and `.anim` | Paid, closed licence |
| Bento Buddy | Blender add-on for SL rigging and animation; exports `.anim` | Paid, sold through the SL Marketplace |
| blender-anim-exporter (GitHub project) | Free Blender add-on that writes SL `.anim` from an armature with SL bone names | No licence found in the README; treat as unknown |
| Linden Lab's Bento skeleton files | Official reference skeletons (Collada, Maya, FBX); there is no official Blender file | Linden Lab's creator resources; terms not stated on the wiki page |
| Machinimatrix "Avatar Workbench" `.blend` | Free SL skeleton file for Blender | Terms not checked |
| `Second-Life-Skeleton` (GitHub project, listed on the SL wiki Bento page) | Community glTF/GLB of the SL skeleton (aligned differently from the Linden `.dae`) | Not checked |

## 4. Open questions

1. ~~Blender's BVH defaults~~ Answered 20 November 2026: checked in Blender 5.3 (import Forward -Z, Up Y, Scale 1.0, Rotation Native; export Scale 1.0, Rotation Native; Root Translation Only off by default, export Start frame 0).
2. ~~Does an imported SL skeleton export the right way up?~~ Answered 20 November 2026: no. It needs the -90 degree turn around X before export (1.4, step 4). Still not tested: a skeleton built from scratch in Blender with motion on control bones.
3. **Rotation order on import.** PinglyMotion applies its own per-joint rotation order to the three angles it reads and does not convert from the file's order (`BVH::copySkeletonNode` in `src/bvh.cpp`). Should it convert? That would make files from Blender with "Rotation: XYZ" safe. A code change, so a separate issue if wanted.
4. **A warning for dropped joints.** PinglyMotion drops unknown joints silently. A message like "12 joints were not recognised: …" would save users a lot of searching. Belongs with the retargeting menu (#16) or as its own small issue.
5. **Free SL rig for Blender.** Is there a free rig with a clear open licence that we can point users to (or ship as a test file)? The Machinimatrix file and the `Second-Life-Skeleton` project need their terms checked first.
6. ~~Blender in CI~~ Done: the `bpy` module installs from PyPI, so the round trip runs in CI as a non-required check (`.github/workflows/blender.yml`).

## Sources

PinglyMotion source (checked 19 November 2026, `main`):

- `src/bvh.cpp` (`loadBentoJoints`, `findLoadedNode`, `copySkeletonNode`, `removeNoSLNodes`), `src/animfile.h` (limits), `src/animation.cpp` (FPS), `bin/data/bento_joints.txt`, `bin/data/TPose.bvh`
- [ANIM_FORMAT.md](ANIM_FORMAT.md), §7: how the SL viewer turns BVH into `.anim` (axes, inches, first frame, key reduction)
- [DECISIONS.md](DECISIONS.md), [PRODUCT.md](PRODUCT.md), [OPEN_SOURCE_CHECK.md](OPEN_SOURCE_CHECK.md), [ROADMAP.md](ROADMAP.md)

Web (read 19 November 2026):

- Blender Manual 5.0, "BioVision Motion Capture (BVH)": https://docs.blender.org/manual/nb/5.0/addons/import_export/anim_bvh.html
- Second Life Wiki, "Project Bento Resources and Information": https://wiki.secondlife.com/wiki/Project_Bento_Resources_and_Information
- A personal notes page on Second Life development (Blender export settings for SL, first frame, Root Translation Only, -90° X rotation), on neocities.org; URL left out because it carries a private person's name
- blender-anim-exporter, project README on GitHub (read through a mirror; licence not stated)
- Second Life forum, "Is there a convenient way to create animations in actual 3D animation software" (2017): https://community.secondlife.com/forums/topic/413389-is-there-a-convenient-way-to-create-animations-in-actual-3d-animation-software/
- Second Life forum, "What is better, Bento Buddy or Avastar": https://community.secondlife.com/forums/topic/510529-what-is-better-bento-buddy-or-avastar/
- Avastar: https://blog.machinimatrix.org/avastar/
