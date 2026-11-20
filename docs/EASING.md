# Easing choices for beginners: names and descriptions

Step 5 is done and merged: the easing choice is picked on a row of six tiles under the timeline (see "Tile row (step 5)"); the project owner tested it on Windows and approved it. In the program before it: step 2, playback computes the four old-style choices through the easing module with exactly the old results (see "Playback (step 2)"), and step 3, a key can carry any of the six choices, stored as `curve` in `.dingle` and used by playback and export (see "Stored choice (step 3)"). This page fixes the **names, the small pictures and the descriptions** of the six easing choices proposed in [#53](https://github.com/pingledingle-tech/PinglyMotion/issues/53), so all the jobs (the maths, the file format, the tile row on the timeline) use the same words. The wording of the hint under each tile lives in [TEACHING.md](TEACHING.md) (IDs `EASE-01` to `EASE-06`); this page does not repeat it, so there is one place to change.

The six choices, their order and "Slow both ends" as the default are proposals in #53. They are not decisions yet; if the project owner changes one, change it here and in `TEACHING.md`.

## Words

- **Easing**: how a movement speeds up or slows down between two keys (saved poses).
- **Choice**: one of the six named options. The user never sees the word "curve" or "ease in/out" (see the plain-word table in [TEACHING.md](TEACHING.md)).
- **Baking**: adding extra in-between keys so that the straight lines Second Life plays between keys follow the curve. Export does this automatically ([ANIM_FORMAT.md](ANIM_FORMAT.md)).

## The six choices

The order is the order of the tiles, left to right. A choice belongs to one pose change (from one key to the next). On screen the user picks it for the pose change that ends at the selected key frame; in the data it lives on the two keys of that change: the start key's `easeOut` and the target key's `easeIn` for the four old-style choices, and the start key's `curve` for the two new ones (see "Stored choice (step 3)").

| # | Name on the tile | Tile picture (our own, simple line drawing) | What happens | Built from | Hint |
|---|---|---|---|---|---|
| 1 | Even | A straight diagonal line | One steady speed from the old pose to the new one | Neither switch | EASE-01 |
| 2 | Slow start | A line that is flat at the left and then climbs | Leaves the old pose gently, then speeds up | The start key's `easeOut` | EASE-02 |
| 3 | Slow finish | A line that climbs fast and flattens at the right | Moves at speed, then settles into the new pose | The target key's `easeIn` | EASE-03 |
| 4 | Slow both ends | A smooth S shape | Starts and stops softly | Both switches | EASE-04 |
| 5 | Overshoot, then settle | A line that rises above the end level and comes back down to it | Goes a little past the new pose, then returns | New stored value (`curve`) | EASE-05 |
| 6 | Hold, then snap | A flat line that jumps up at the right | Stays in the old pose, then jumps to the new one | New stored value (`curve`) | EASE-06 |

"Built from" is the expectation in #53. The first job of #53 must confirm by reading the code what the two switches do today, and may change this column.

## Rules for the names and pictures

- Names are short (at most three words), plain English, no animation jargon, and each starts with what the user sees, not how it is made.
- The default for a new key is "Slow both ends" (proposal 1 in #53). The tile row shows which choice the selected pose change uses.
- Each tile has its name under a small picture. The picture shows the movement from the left (old pose) to the right (new pose); it never shows curves, numbers or handles. Decided by the project owner (19 Nov 2026): the picture is a small moving figure with a trail of still dots, not a graph (the "tile picture" column above was the first idea).
- The pictures are drawn for this program. We do not copy icons, preset pictures or help text from other programs (copyright note in #53, and `DECISIONS.md` for EffectCraft and Aurora: ideas only).
- Overshoot is clamped to the joint limits (proposal 4 in #53), so a tile never lets a joint go past what Second Life allows. A bounce choice is left out for now (proposal 6).
- Per-joint choices and free curve handles are not part of the first version; they would sit under "Advanced" later.

## How the two switches work today (checked 19 Nov 2026, step 1 of #53)

Read from `BVHNode::interpolate` and `BVHNode::frameData` in `src/bvhnode.cpp` (the switches are stored in `FrameData`, one pair per joint and key frame; `AnimFile::Key` in `src/animfile.h` holds the same two flags). Nothing in `src/animation.*`, `src/bvh.cpp` or the timeline changes the maths; `src/animexport.cpp` has no per-key ease (its `easeIn`/`easeOut` are the whole-animation times of the `.anim` header).

**Which key counts.** Between a start key A and the next key B of the same joint, with `steps` = frames from A to B and `pos` = frames since A:

- the first half (`pos <= steps/2`, whole-number division) uses the `easeOut` switch of **A**, the start key;
- the second half (`pos > steps/2`) uses the `easeIn` switch of **B**, the target key.

**The formula.** If the switch that applies to this frame is off, the value is a straight line: `from + (to - from) * pos / steps`. If it is on, the value is `from + (to - from) * (0.5 - cos(3.1415 * pos / steps) / 2)`, a full sine S-curve fitted to the whole span A to B. So the switch only decides, half by half, whether to use the S-curve or the straight line. If `from == to` the value is returned unchanged.

**The same for every channel.** Each of the three rotation angles and the three position values is computed on its own with this formula, with the same pair of switches. Rotations are blended as Euler angles, not as quaternions. There is no clamping to joint limits in this step.

**Worked example.** A joint turns from 0 to 90 degrees over 10 frames (`steps` = 10). The numbers are what the program gives (pi written as 3.1415):

| Frame after A | Both off (Even) | Both on (Slow both ends) |
|---|---|---|
| 2 | 18.00 | 8.59 |
| 5 | 45.00 | 45.00 |
| 8 | 72.00 | 81.40 |

With only A's `easeOut` on, frames 1 to 5 equal the "Both on" column and frames 6 to 9 equal the "Both off" column (frame 8 gives 72.00). With only B's `easeIn` on it is the other way round (frame 2 gives 18.00, frame 8 gives 81.40).

**Does it match #53's idea?** Partly.

- **Even** (neither switch) and **Slow both ends** (both switches) match exactly, apart from the old code writing pi as 3.1415 (a difference of less than 0.01 degrees on 100; `Easing::s` uses the exact pi, playback keeps 3.1415, see step 2).
- **Slow start** (start key's `easeOut`) and **Slow finish** (target key's `easeIn`) are not classic one-sided curves. Each is the S-curve for one half and a straight line for the other half. The two halves meet at the middle, but the speed jumps there: Slow start leaves the old pose gently and then arrives at full speed, with no slowing at the end; Slow finish is the mirror image.
- So the four choices are the four switch combinations, as the issue assumed, but "Slow start" and "Slow finish" keep today's half-and-half shape. The new module defines them as exactly that shape (`src/easing.*`, `Easing::s`), so old files play identically; a smoother one-sided curve would be a new choice and would change old files.

## The maths module (step 1)

`src/easing.h` and `src/easing.cpp`, namespace `Easing`, pure functions with no disk access and no Qt types. Tested in `tests/easing/`. The program calls only `segmentValue` and `presetFromSwitches` (step 2, header-only); the rest is built into the tests only for now.

- `s(preset, t)`: the speed curve for t from 0 to 1. `s(0) = 0` and `s(1) = 1` exactly, always a finite number (t outside 0..1 is clamped, NaN counts as 0).
- Even is `t`. Slow both ends is the sine S-curve. Slow start and Slow finish are the half-and-half shapes above. Overshoot then settle is `1 + 2.5 (t-1)^3 + 1.5 (t-1)^2`: it goes at most 8 % past the new pose (limit: 10 %) and ends at exactly 1. Hold then snap is 0 until the last moment, then 1.
- Every choice is steady (never goes backwards) except Overshoot then settle.
- `blend` mixes two numbers or three angles with `s`; `nlerp` mixes two rotations the way the Second Life viewer does (shortest way, straight blend, normalised, so the result is always a unit quaternion).
- `clampRotation` cuts a blended rotation at the joint's minimum and maximum, so the overshoot choice never leaves the allowed range.
- `bake`, `bakeValues`, `bakeRotations`: the in-between values for a pose change of `frames` frames (`frames + 1` values, first exactly the start, last exactly the end), as the `.anim` export needs.
- `presetFromSwitches` and `switchesFromPreset` map the four switch combinations to the first four choices and back. The last two choices have no switch form; they are stored as `curve` (step 3).

## Playback (step 2)

`BVHNode::interpolate` (`src/bvhnode.cpp`) no longer has its own formula. It turns the start key's `easeOut` and the target key's `easeIn` into one of the four old-style choices (`Easing::presetFromSwitches`) and asks `Easing::segmentValue` for the value of the frame.

- `segmentValue` is the old formula with the same operations in the same order, including pi written as 3.1415 and the half split with whole-number division. Using the exact pi (as `Easing::s` does) would move old animations by up to about 0.003 % of each change, and the saved files would change in the last digits, so playback keeps 3.1415.
- Both functions are in `src/easing.h` (header-only), so the program and every test or fuzz target that builds `src/bvhnode.cpp` needs no extra source file.
- Since step 3 the two new choices follow `s()` in `segmentValue` too (see below).
- Proof: `tests/interpolation/` keeps a copy of the old function and compares it with the new path bit for bit (no tolerance), for every segment length from 1 to 200, every frame, all four switch combinations and many values, and compares every frame `BVHNode::frameData` returns for 200 joints with random keys, gaps and switches. Opening and saving BVH, AVM and `.dingle` use what `frameData` returns, so files play and save identically. Since step 3, `.dingle` uses `segmentValue` as well instead of its own copy of the formula (the copy was the same formula, so the values are unchanged).

## Joint limits on the overshoot (proposal 4)

Playback clamps the Overshoot then settle choice to the joint limits since the fix for proposal 4 (after step 5): `BVHNode::frameData` cuts each of the three rotation angles of an overshoot segment at the minimum and maximum the program loaded for that joint and channel (`data/SL.lim`, `channelMin`/`channelMax` on the joint), so the overshoot never takes a joint past what Second Life allows. An AI that sets Overshoot through the control commands therefore never produces a pose Second Life would reject. The `.anim` export samples `frameData` (`BVH::snapshot`), so it writes the same cut frames; BVH and `.dingle` saving write only the keys, which are not touched.

- Only the overshoot choice is cut. The other five choices never leave the span between the two keys, so they are not touched and stay bit for bit what they were (`tests/interpolation/`).
- Position channels are never cut, and neither is a rotation channel without loaded limits (the `-10000..10000` defaults, as the hip has in `SL.lim`, or a file read without a limits file).
- The cut is never tighter than the two keys themselves: a key that already sits outside its limits (joint limits switched off in the settings, or an old file) still plays as stored, the overshoot is only kept from passing it.
- The in-between frames the `.dingle` reader computes on its own (`src/dingle.cpp`, for tools that read `.dingle` without the program) know no limits file and are not cut; in the program the keys are loaded and `frameData` plays them, so the program's playback and export are cut.
- Tests: `tests/interpolation/` (a segment ending exactly on a limit stays inside on every frame, a segment well inside is unchanged, a channel without limits and the five other choices are untouched, positions are untouched), `tests/tst_roundtrip.cpp` (the program with `SL.lim`: playback and the export input of an overshoot into a limit).

## Stored choice (step 3)

- A key frame can carry an easing choice for the segment that starts at it (`AnimFile::Key::curve`, `FrameData::curve` in the program). With one, it decides the segment alone (`Easing::segmentPreset`); without one, the two switches decide as before. Old files have none, so they play and save as before.
- In `.dingle` it is the key's `curve`, `{"type": "<technical name>"}` with the technical names `linear`, `ease-in`, `ease-out`, `ease-in-out`, `ease-out-overshoot`, `constant` ([DINGLE_FORMAT.md](DINGLE_FORMAT.md)). An unknown value gives Even and counts as an ignored key.
- `segmentValue` now computes the two new choices: Overshoot then settle is `from + (to - from) * s(t)` with `t = pos / steps` (at most 8 % past the target), Hold then snap stays at `from` until the target frame. The four old-style choices are unchanged bit for bit (`tests/interpolation/`).
- Joint limits: playback cuts the overshoot at the joint's limits, see "Joint limits on the overshoot (proposal 4)" above.
- Tests: `tests/dingle/` (round trip of all six, old files byte for byte, unknown values, the frames in between, position keys), `tests/interpolation/` (the stored choice wins over the switches in `BVHNode::frameData`), `tests/animexport/` (each stored choice through `.dingle` and the `.anim` writer within 0.01 degrees), `tests/tst_roundtrip.cpp` (open in the program, playback, the export input, save and open again).

## Tile row (step 5)

`src/easingtiles.*`: the row, one tile per choice, and the helpers that read and store a choice (`EasingChoice`). The row sits under the tracks in the Timeline panel (`src/timelineview.*`); the window (`src/pinglymotionwindow.*`) tells it which choice is current and stores a click (`EasingChoice::setChoice`, through `Animation::setEaseIn`, `setEaseOut` and the new `Animation::setCurve`). Tested in `tests/easingtiles/`.

- **Which pose change.** The tiles belong to the pose change that **ends at the selected key frame** of the selected joint: the user poses a key and then says how the figure gets there. Frame 0 and frames that are not a key frame of the selected joint have no pose change before them: the row is greyed out and the line under it says "Select a key frame on the timeline".
- **What a click stores.** The four old-style choices become the two switches (the start key's `easeOut`, the target key's `easeIn`) and clear any stored `curve`, so the file stays as before. Overshoot then settle and Hold then snap are stored as the start key's `curve`; the two switches are set to the default (both on), so a reader that does not know `curve` plays Slow both ends. Reading goes the other way with `Easing::segmentPreset`: a stored `curve` wins, otherwise the switches say which of the first four it is.
- **The picture.** A tiny stick figure walks from the left edge (old pose) to the right edge (new pose) with the speed of the choice (`Easing::s`), over about two seconds, then rests and starts again; all six move at the same time, so the eye compares them. Behind it is a trail of nine still dots at equal time steps (`s(k/8)`), so close dots mean slow and wide gaps mean fast: Even is evenly spaced, Slow both ends is tight at both ends, Overshoot goes past the right end and the dots crowd there, Hold then snap has all dots on the left and one on the right. The row keeps the right tenth of the way free for the overshoot. Drawn by the program (`QPainter`), no image files, no axes, no numbers, no handles. One timer for the row (25 frames a second), running only while the row is on screen and a key is selected.
- **The hint.** Under the row, one line: the approved hint of the tile under the mouse, else of the current choice (`EASE-01` to `EASE-06` in [TEACHING.md](TEACHING.md), through `tr()`). It follows the "Tips on" switch in the status bar (`tips_on`); the same texts are the tiles' tooltips. With the tips off the line is empty and the tooltips are gone; the "Select a key frame" line shows either way, because it is the state, not a tip.
- **Gone:** the "Ease In / Ease Out" check boxes in the Properties panel. The tiles cover both switches (the selected key's `easeIn` through Slow finish and Slow both ends; its `easeOut` belongs to the next pose change and is set when the next key is selected). The settings dialog still has the defaults for new keys, and files are unchanged.
- **Not yet:** per-joint choices. (The custom speed curve for one pose change is merged, see the curve editor, issue 11. Joint limits on the overshoot are done, see "Joint limits on the overshoot" above. The example wave is step 6, below, and is done.)

## The example wave (step 6)

`examples/wave.dingle`, made by `tools/wave_example.py` from the relaxed start pose (`bin/data/Relaxed.dingle`); running the script again gives the same file. Five keys on the right arm (`rCollar`, `rShldr`, `rForeArm`, `rHand`) at frames 0, 20, 30, 40 and 60, 61 frames at 30 per second, loop from frame 1 to 60. Every other joint keeps the relaxed pose.

| Pose change | What moves | Choice | Stored as |
|---|---|---|---|
| key 1 to 2 | the arm lifts | Slow both ends | key 1 `easeOut`, key 2 `easeIn`, and key 1 `curve` `ease-in-out` |
| key 2 to 3 | the hand swings out | Overshoot, then settle | key 2 `curve` `ease-out-overshoot` (switches on, as the tile row stores it) |
| key 3 to 4 | the hand swings back | Even | no switch |
| key 4 to 5 | the arm comes down | Slow finish | key 5 `easeIn` |

- Key 1 is at frame 0. Opening a file sets the ease switches of every frame 0 key to the user's default (off unless changed; old behaviour, see step 3), but keeps a `curve`, so the first change also names its choice as `curve`. The tile row reads it the same way (`Easing::segmentPreset`).
- The last key equals the first, so the loop has no jump. The overshoot (forearm 70 to 25 degrees, 8 % past is 21.4) stays inside the forearm's limits (0 to 146).
- The notes `WAVE-01` to `WAVE-06` ([TEACHING.md](TEACHING.md)) are stored under `extensions`, `"pinglymotion.exampleNotes"`: one entry per key (`frame`, `choice`, `note`) and the small `task`. The program does not show them yet and does not write `extensions` back when the file is saved; they are also listed in [FIRST_ANIMATION.md](FIRST_ANIMATION.md). Where notes show in the program belongs to the timeline redesign (#14).
- Tests: `tests/test_wave_example.py` (the file is the script's output, keys, choices, notes equal the reviewed texts, run in the Linux build), `tests/tst_roundtrip.cpp` (`exampleWaveUsesFourChoices`: the program opens it without ignored keys, each change plays its choice, every frame inside the limits, the loop closes).

## Where each part is built

| What | Job in #53 |
|---|---|
| Maths for the six choices, no screen change | Step 1 |
| Four old-style choices use today's switches | Step 2 |
| `curve` stored in `.dingle` ([DINGLE_FORMAT.md](DINGLE_FORMAT.md)) (done) | Step 3 |
| Export test for a curved ease | Step 4 |
| Tile row and hint on the timeline (visible, the project owner tests) (done and merged; approved after the project owner's Windows test) | Step 5 |
| The example wave with a note on each change ([TEACHING.md](TEACHING.md), `WAVE-01` to `WAVE-06`) (done; approved after the project owner's Windows test) | Step 6 |
