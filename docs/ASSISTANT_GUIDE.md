# Assistant guide: how to animate in PinglyMotion

For an AI assistant that is connected to PinglyMotion (MCP tool `read_guide`, or this file). Read it once, then work. Plain English on purpose: the words in sections 4 to 6 are the words to use with the user.

Numbers here come from the program's own files and tests, not from memory. Where something is not built yet it says **planned**. If this guide and the program disagree, the program is right: tell the user.

## 1. What Second Life does with an animation

- **Frame 1 is the reference.** Second Life compares every later frame with it and does not play it. PinglyMotion protects frame 1: pose from frame 2. Frame numbers start at 1 (the file's frame 0).
- **A joint that does not move is dropped.** On export, a joint that never moves away from frame 1 is left out of the file. So a joint you want to move must differ from frame 1 at some key.
- **At most 60 seconds.** About 1,800 frames at 30 per second. A longer animation is refused at export.
- **Between keys Second Life moves in a straight line.** PinglyMotion's easing choices are turned into extra keys on export, so the movement is the same.
- **Priority** says which animation wins when two move the same joint; the higher number wins. A `.anim` file can have 0 to 6 (default 2). A BVH uploaded in the viewer is limited to 0 to 4. The export dialog (File > Export as Second Life .anim) asks for it.
- **Loop points** are two moments inside the animation. Between them it repeats. Make the last key equal to the first, or the loop jumps.
- **Ease in and out of the whole animation** (the export dialog calls them slow start and slow finish, in seconds) fade it in and out against whatever else plays. That is not the same as the six easing choices between two keys (section 4).
- **Hip offset.** The hip position (`set_position`) is stored as the offset from frame 1 and cannot be more than 5 metres (about 197 in the program's units, inches). Standing hip height is 43.5 in the start pose and 41 in the relaxed pose. When the legs bend, the hip must come down, see section 5.
- **Hand pose** is a header value of an `.anim` file (14 shapes: relaxed, fist, point and so on). The finger joints of Bento (`mHand...`) are posed as ordinary joints and are separate from it.
- **Upload changes the file a little.** The viewer reads the file and writes it again, so every number can move by one tiny step. Ignore it.
- Commands cannot export `.anim` yet. `save_file` writes `.dingle` (the working file) or `.bvh` (uploadable). The user exports `.anim` from the menu.

## 2. The skeleton

Left and right are **the figure's own sides**, not the screen's. Classic joints start with `l` or `r` (`lShldr`). Bento joints end with `Left` or `Right` (`mHandIndex1Left`). The right side is not a mirror image in numbers: the same sign can mean the opposite on the other side (see the table). With **Mirror** on in the program, posing a left joint also poses the right one.

Classic joints (the body, about 19 joints). Angles go in `set_pose` as `[x, y, z]` degrees; `joints` lists the allowed range.

| Everyday name | Name to use | Second Life name |
|---|---|---|
| whole body, hip | `hip` | mPelvis |
| belly | `abdomen` | mTorso |
| chest | `chest` | mChest |
| neck | `neck` | mNeck |
| head | `head` | mHead |
| collar bone | `lCollar`, `rCollar` | mCollarLeft, mCollarRight |
| shoulder (whole arm) | `lShldr`, `rShldr` | mShoulderLeft, mShoulderRight |
| elbow | `lForeArm`, `rForeArm` | mElbowLeft, mElbowRight |
| wrist | `lHand`, `rHand` | mWristLeft, mWristRight |
| hip joint (whole leg) | `lThigh`, `rThigh` | mHipLeft, mHipRight |
| knee | `lShin`, `rShin` | mKneeLeft, mKneeRight |
| ankle | `lFoot`, `rFoot` | mAnkleLeft, mAnkleRight |

The arms hang from the chest and the head from the neck. Turning a joint turns everything below it.

Bento joints (all start with `m`), as the tabs of the joint list show them:

- **Body:** `mSpine1` to `mSpine4`, `mFootLeft`, `mToeLeft` and the right ones.
- **Face:** `mSkull`, `mEyeLeft`, `mEyeRight`, `mFaceJaw`, eyebrows, eyelids, nose, lips, tongue, ears, cheeks and more (about 49).
- **Hands:** per hand five fingers, three joints each: `mHandThumb1Left`, `mHandIndex1Left`, `mHandMiddle1Left`, `mHandRing1Left`, `mHandPinky1Left`, and `...2...`, `...3...` for the next joints (30 in all).
- **Other:** tail, wings, hind limbs, groin. Hidden by default.

### Calibration table: which way each turn goes

It was measured by the program's own skeleton: from the start pose (every angle 0, arms straight out to the side, legs straight down) each joint was turned by +30 degrees around one axis. The table says where the end point (hand, foot, top of the head, fingertip) goes, from the figure's own point of view. **The same turn with a minus sign goes the opposite way.** "Allowed range" is what `joints` says for that axis. A turn past it is cut back and the answer says so, so a direction that points outside the range cannot be used from the start pose.

How to read the words: "forward", "back", "up", "down" are the figure's own. "Out to the side" is away from the middle of the body and "in across the body" is towards it, for that side. "Turn left" and "tilt left" are towards the figure's left. "Front goes up" means the front of that part (the side that faces forward in the start pose) turns up: the end point itself stays. "(small)" means the end point moves only a little. Once a joint is already turned, the same axes still apply, but check with a picture.

To make the table again after a change to the skeleton or the limits:
`PINGLYMOTION_UPDATE_GUIDE=1 build/tests/calibration/pinglymotion_calibration_tests`
(build target `pinglymotion_calibration_tests`, test name `calibration`). The test fails when this table is out of date.

<!-- calibration-table:begin -->
| Joint | Axis | Sign | What moves | Direction | Allowed range |
|---|---|---|---|---|---|
| hip | X | + | head | forward | none |
| hip | Y | + | body front | turn left | none |
| hip | Z | + | head | tilt right | none |
| abdomen | X | + | head | forward | -45 to 68 |
| abdomen | Y | + | body front | turn left | -45 to 45 |
| abdomen | Z | + | head | tilt right | -30 to 30 |
| chest | X | + | head | forward | -45 to 22 |
| chest | Y | + | chest front | turn left | -45 to 45 |
| chest | Z | + | head | tilt right | -30 to 30 |
| neck | X | + | top of head | forward | -45 to 45 |
| neck | Y | + | face | turn left | -45 to 45 |
| neck | Z | + | top of head | tilt right | -30 to 30 |
| head | X | + | top of head | forward | -45 to 45 |
| head | Y | + | face | turn left | -45 to 45 |
| head | Z | + | top of head | tilt right | -30 to 30 |
| lCollar | X | + | front of left arm | front goes down | -5 to 5 |
| lCollar | Y | + | left hand | back | -30 to 10 |
| lCollar | Z | + | left hand | up | -30 to 30 |
| lShldr | X | + | front of left arm | front goes down | -135 to 90 |
| lShldr | Y | + | left hand | back | -180 to 98 |
| lShldr | Z | + | left hand | up | -91 to 97 |
| lForeArm | X | + | front of left forearm | front goes down | -90 to 79 |
| lForeArm | Y | + | left hand | back | -146 to 0 |
| lForeArm | Z | + | left hand | up | -5 to 5 |
| lHand | X | + | front of left hand | front goes down | -45 to 45 |
| lHand | Y | + | left hand tip | back | -25 to 36 |
| lHand | Z | + | left hand tip | up | -90 to 86 |
| lThigh | X | + | left foot | back | -155 to 45 |
| lThigh | Y | + | front of left leg | turn left | -85 to 105 |
| lThigh | Z | + | left foot | out to the side | -17 to 88 |
| lShin | X | + | left foot | back | 0 to 150 |
| lShin | Y | + | front of left lower leg | turn left | -5 to 5 |
| lShin | Z | + | left foot | out to the side | -5 to 5 |
| lFoot | X | + | left toes | down | -31 to 63 |
| lFoot | Y | + | left toes | out to the side | -26 to 26 |
| lFoot | Z | + | left toes | out to the side (small) | -74 to 15 |
| rCollar | X | + | front of right arm | front goes down | -5 to 5 |
| rCollar | Y | + | right hand | forward | -10 to 30 |
| rCollar | Z | + | right hand | down | -30 to 30 |
| rShldr | X | + | front of right arm | front goes down | -135 to 90 |
| rShldr | Y | + | right hand | forward | -98 to 180 |
| rShldr | Z | + | right hand | down | -97 to 91 |
| rForeArm | X | + | front of right forearm | front goes down | -90 to 79 |
| rForeArm | Y | + | right hand | forward | 0 to 146 |
| rForeArm | Z | + | right hand | down | -5 to 5 |
| rHand | X | + | front of right hand | front goes down | -45 to 45 |
| rHand | Y | + | right hand tip | forward | -36 to 25 |
| rHand | Z | + | right hand tip | down | -86 to 90 |
| rThigh | X | + | right foot | back | -155 to 45 |
| rThigh | Y | + | front of right leg | turn left | -105 to 85 |
| rThigh | Z | + | right foot | in across the body | -88 to 17 |
| rShin | X | + | right foot | back | 0 to 150 |
| rShin | Y | + | front of right lower leg | turn left | -5 to 5 |
| rShin | Z | + | right foot | in across the body | -5 to 5 |
| rFoot | X | + | right toes | down | -31 to 63 |
| rFoot | Y | + | right toes | in across the body | -26 to 26 |
| rFoot | Z | + | right toes | in across the body (small) | -15 to 74 |
| mSkull | X | + | top of head | forward | -45 to 45 |
| mSkull | Y | + | face | turn left | -45 to 45 |
| mSkull | Z | + | top of head | tilt right | -45 to 45 |
| mEyeLeft | X | + | left eye front | down | -45 to 45 |
| mEyeLeft | Y | + | left eye front | turn left | -45 to 45 |
| mEyeLeft | Z | + | left eye front | nothing visible (rolls around its front) | -45 to 45 |
| mEyeRight | X | + | right eye front | down | -45 to 45 |
| mEyeRight | Y | + | right eye front | turn left | -45 to 45 |
| mEyeRight | Z | + | right eye front | nothing visible (rolls around its front) | -45 to 45 |
| mFaceJaw | X | + | chin | back | -30 to 30 |
| mFaceJaw | Y | + | chin | turn left | -30 to 30 |
| mFaceJaw | Z | + | chin | turn left | -30 to 30 |
| mHandThumb1Left | X | + | left thumb tip | down | -90 to 90 |
| mHandThumb1Left | Y | + | left thumb tip | back | -90 to 90 |
| mHandThumb1Left | Z | + | left thumb tip | up | -90 to 90 |
| mHandIndex1Left | X | + | left index tip | down (small) | -15 to 15 |
| mHandIndex1Left | Y | + | left index tip | back | -25 to 25 |
| mHandIndex1Left | Z | + | left index tip | up | -90 to 45 |
| mHandMiddle1Left | X | + | left middle tip | back (small) | -15 to 15 |
| mHandMiddle1Left | Y | + | left middle tip | back | -25 to 25 |
| mHandMiddle1Left | Z | + | left middle tip | up | -90 to 45 |
| mHandRing1Left | X | + | left ring tip | up (small) | -15 to 15 |
| mHandRing1Left | Y | + | left ring tip | back | -25 to 25 |
| mHandRing1Left | Z | + | left ring tip | up | -90 to 45 |
| mHandPinky1Left | X | + | left pinky tip | up | -15 to 15 |
| mHandPinky1Left | Y | + | left pinky tip | in across the body | -25 to 25 |
| mHandPinky1Left | Z | + | left pinky tip | up | -90 to 45 |
| mHandThumb1Right | X | + | right thumb tip | down | -90 to 90 |
| mHandThumb1Right | Y | + | right thumb tip | in across the body | -90 to 90 |
| mHandThumb1Right | Z | + | right thumb tip | down | -90 to 90 |
| mHandIndex1Right | X | + | right index tip | down (small) | -15 to 15 |
| mHandIndex1Right | Y | + | right index tip | forward | -25 to 25 |
| mHandIndex1Right | Z | + | right index tip | down | -45 to 90 |
| mHandMiddle1Right | X | + | right middle tip | back (small) | -15 to 15 |
| mHandMiddle1Right | Y | + | right middle tip | forward | -25 to 25 |
| mHandMiddle1Right | Z | + | right middle tip | down | -45 to 90 |
| mHandRing1Right | X | + | right ring tip | up (small) | -15 to 15 |
| mHandRing1Right | Y | + | right ring tip | forward | -25 to 25 |
| mHandRing1Right | Z | + | right ring tip | down | -45 to 90 |
| mHandPinky1Right | X | + | right pinky tip | up | -15 to 15 |
| mHandPinky1Right | Y | + | right pinky tip | forward | -25 to 25 |
| mHandPinky1Right | Z | + | right pinky tip | down | -45 to 90 |
<!-- calibration-table:end -->

## 3. Typical poses as numbers

All numbers are `[x, y, z]` degrees as `set_pose` takes them, read from the files the program ships (`bin/data/Relaxed.dingle`, `bin/data/TPose.dingle`, `examples/throne.dingle`). Joints not listed are 0. A new animation starts in the start pose (T pose): every angle 0, arms out, hip height 43.5.

**Stand, relaxed** (`bin/data/Relaxed.dingle`). Hip position `[0, 41, 0.4]`.

| Joint | [x, y, z] |
|---|---|
| lCollar | [0.1, 2.0, -4.0] |
| lShldr | [-4.3, 4.0, -50.4] |
| lForeArm | [-4.5, -9.1, 0] |
| lHand | [-3.4, -4.1, -7.5] |
| lThigh | [-11.5, -0.6, 3.6] |
| lShin | [21.8, -1.4, -5.0] |
| lFoot | [-5.1, 3.3, 4.0] |

The right side is the same pose with the arm and leg values for the right joints: rCollar [0.1, -2.0, 4.0], rShldr [-4.3, -4.0, 50.4], rForeArm [-4.5, 9.1, 0], rHand [-3.4, 4.1, 7.5], rThigh [-11.5, 0.6, -3.6], rShin [21.8, 1.4, 5.0], rFoot [-5.1, -3.3, -4.0]. Notice the signs that flip.

**Sit on a chair** (the seated key of `examples/throne.dingle`). Start from the relaxed pose above, then change these. Hip position `[0, 28, -8]`.

| Joint | [x, y, z] |
|---|---|
| lShldr | [3.7, -8.0, -54.4] |
| lForeArm | [20, -77, 0] |
| lHand | [30, -4.1, -7.5] |
| lThigh | [-81.5, -0.6, 3.6] |
| lShin | [78.8, -1.4, -5.0] |
| rShldr | [3.7, 8.0, 54.4] |
| rForeArm | [20, 77, 0] |
| rHand | [30, 4.1, 7.5] |
| rThigh | [-80.5, 0.6, -3.6] |
| rShin | [77.8, 1.4, 5.0] |

Thighs forward about 80, knees bent about 80, hips lower and back, forearms bent forward.

**Kneel** and **lie on the back** are not in any bundled file, so there are no numbers for them here. Build them in steps (section 4) and, when the user likes the result, add the numbers to this guide (see the end).

## 4. The work rhythm

1. **Plan in words first.** Tell the user the movement in a few plain words and the length in seconds. Ask one question only if you cannot choose.
2. **Look at the program.** `state` and `joints`. Is frame 1 the pose you want to start from?
3. **Start from a named pose.** Use section 3 on frame 2, or start from the start pose. There is no command that loads a starter pose yet (**planned**).
4. **One body part at a time.** Arm, then the other arm, then legs, then head. Never change everything in one call.
5. **Keys at seconds.** Frame = seconds times frames per second, plus 1. At 30 per second, 1 second is frame 31. Use `set_frame_count` and `set_fps` first, then `set_pose` at the key frames, with `set_position` for the hip.
6. **Ease by the six names.** For each move say which fits: **Even**, **Slow start**, **Slow finish**, **Slow both ends** (the default, good for most arm and head moves), **Overshoot then settle** (lively, like a wave), **Hold then snap** (like a blink). The commands cannot set the choice yet (**planned**: an `easing` field on `set_pose`), so tell the user which tile to click.
7. **Check.** `get_pose` for the numbers, `keys` for the frames, `screenshot` to look, `play` and `stop` to watch the movement.
8. **Look with the camera.** `set_camera` with `view`: front, back, left, right, three_quarter. It never changes the animation. Never turn the hip to see the side.
9. **At most five tries per part.** If the part is still wrong after five tries, stop and ask the user what they see.
10. **Save.** `save_file` with a `.dingle` name first (it keeps everything), then `.bvh` if the user wants a file for Second Life. Say the file name.

`describe_pose` gives the current frame in words (world positions of the head, hands, feet and hip, plain sentences, the joints that moved from frame 1, left/right symmetry, a one-line summary): use it alongside `get_pose` and the picture, trusting it and the numbers over looks. `describe_motion` gives the movement over a range of frames in words (what each joint does and when, how fast, the paths of the hands and feet, foot contact and weight, the hip, the rhythm, the loop seam, what changes between keys, a one-line summary): use it after each change to check timing and order, which a picture cannot show. Planned, not built yet: `check_animation` (checks against Second Life's rules).

## 5. Known mistakes and the fix

- **The arms follow the chest when it bends.** The arms hang from the chest, so bending `abdomen` or `chest` moves both hands. Bend the body first, then pose the arms, and look again.
- **Left and right are the figure's own.** The figure's left hand is on the right side of a front picture. When the user says "left", they mean the figure's left unless they say "on the screen". Ask once if unsure.
- **Angles read from a picture are approximate.** A picture is wrong by 15 to 20 degrees and mixes up sides. Use the numbers (`get_pose`, section 3, the calibration table) and use the picture to check.
- **Feet on the floor is a number, not a look.** It depends on the hip height and the leg angles together. After bending the legs, lower the hip with `set_position` (the sitting example lowers it from 41 to 28 with thighs at about -81). Look from the side (`set_camera` left) and change the hip height in steps until the feet meet the floor. Write the number down and use it at the other keys.
- **Floating markers are Bento joints.** The small balls around the face and hands in the 3D view are joint markers for face and finger joints. They are not mistakes and not part of the figure.
- **An angle outside the range is cut back.** The answer says what was applied. Do not repeat the same value.
- **Mirror is on.** Posing a left joint then also poses the right one. Say so if the other side moves.
- **Frame 1 does not take poses.** Pose from frame 2.

### Adding a lesson

Lessons are text that every assistant follows, so a lesson could carry hidden instructions (prompt injection) and not only a mistake. Nothing goes in unread.

1. A lesson comes from the "Teach the AI" issue form, or from a session that saw the same mistake twice.
2. The project owner reads the report first. A session never adds a lesson from a report he has not read.
3. When he accepts it, the issue gets the label `lesson-accepted` and a session adds one bullet to this section: what goes wrong, how to see it, and the fix with numbers, one line. If it is a pose, its numbers go in section 3 with the file they came from.
4. A bullet is plain facts about the program or Second Life. It never tells the assistant to ignore other rules, to contact anyone, or to do anything outside animating.
5. Reports that are not accepted get the label `teach-the-ai` only and a short reason in a comment.

The guide is shipped with the Windows zip and the Claude Desktop extension, so a change is visible after the next build.

## 6. Talking to a beginner

The user may never have animated. Use these words, not the left column.

| Not this | Say this |
|---|---|
| key frame, keyframe | key (a saved pose) |
| rotate, rotation | turn |
| interpolation | the movement between two keys |
| ease in, ease out | slow finish, slow start |
| IK, inverse kinematics | drag the hand and the arm follows |
| joint limit | the furthest this joint can turn |
| quaternion, Euler | never say these |
| bake | build extra keys |

- **One thing at a time.** One body part, one change, one picture.
- **Say what you did in one line.** For example: "I turned the left elbow so the hand is by the chin."
- **Ask one question, with a default.** For example: "Should the wave be slow and gentle? I will go with that unless you say otherwise."
- **Say what to do, not what is wrong.** Short sentences. No numbers unless the user asks.
