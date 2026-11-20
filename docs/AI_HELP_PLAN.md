# AI help plan: the next three jobs (issue 129)

Status 20 Nov 2026. Done: the assistant guide, the calibration test, `read_guide` and `describe_pose`.
In progress (another job, not covered here): `describe_motion` and `check_animation`.
This page plans the three jobs after those. Nothing here is started; each job waits for the project owner's go.

## Gaps an assistant still hits with a beginner

Assume `describe_motion` and `check_animation` are merged. An assistant helping a beginner make a
Second Life animation can then read the pose and the motion and find rule breaks, but:

1. **It cannot set the timing it plans.** The guide tells it to choose one of six easing names per
   move, but `set_pose` has no easing field, so it has to ask the user to click a tile.
   `keys` returns frame numbers only (no easing), and `state` does not report the loop points,
   so it cannot set or read where a looping stand or dance repeats.
2. **It cannot finish the job.** `save_file` writes `.dingle` and `.bvh` only. The program can
   export `.anim` (priority, loop, slow start and finish, hand pose), but no command reaches it,
   so the last step to Second Life is a manual dialog the assistant can only describe.
3. **It sees one still frame from one side.** `screenshot` gives one view of one frame with no
   labels. Research in the issue: a model misreads angles from pictures and confuses left and
   right; several views and labelled sides in one image help it check its own work, and a frame
   strip shows a movement without many calls.

Built: the evaluation set (`tests/assistant/`, `docs/DEV.md`, "Evaluation set for the AI assistant").

Later (not in these three): copy/move/mirror keys and undo through commands,
starter poses and hand presets, `.anim` import through commands, worked examples, the
Teach-the-AI form (issue 20).

## Job A: easing and loop points through the control commands

- **Goal:** the assistant sets and reads the easing of a key and the loop points, so it can do
  steps 5 and 6 of the guide's work rhythm itself.
- **Commands:** `set_pose` gets an optional `easing` (`linear`, `ease-in`, `ease-out`,
  `ease-in-out`, `ease-out-overshoot`, `constant`) applied to every key it makes; `keys` returns
  each key's easing; new `set_loop` (`in`, `out` frame numbers, or `off`); `state` returns
  `loop_in` and `loop_out`.
- **Acceptance tests (in `tests/tst_control.cpp`):**
  - `set_pose` with `easing: "ease-out-overshoot"` at frame 31, then `keys` returns exactly that
    name for that joint and frame; a key without `easing` returns `ease-in-out` (the default).
  - An unknown easing name fails with an error that lists the six names, and no key is made.
  - `set_loop` `{in: 2, out: 61}`, then `state` returns `loop_in: 2`, `loop_out: 61`; save as
    `.dingle`, `new_animation`, `open_file`: the same easing names and loop points come back.
  - `set_loop` with `in` after `out`, or outside the frame count, fails and changes nothing.
  - The six names match `src/easing.*` (one test loops over the enum, so a new easing kind fails
    the test until the command knows it).
  - Fuzz run on the branch is green (the command table changes).
- **Files:** `src/controlapi.*`, `tests/tst_control.cpp`, `tools/mcp/commands.json`,
  `fuzz/control.dict`, `docs/CONTROL.md`, `docs/ASSISTANT_GUIDE.md` (drop "planned" in step 6).
- **Kind:** technical. **Model:** light (Sonnet): the model already has the data.
- **Depends on:** `describe_motion` and `check_animation` merged (they hold `src/controlapi.*`);
  the curve editor job (it changes `src/easing.*` and `src/animation.*`) merged first.

## Job B: `export_anim` command

- **Goal:** the assistant writes the Second Life `.anim` file itself, with the right priority,
  loop and hand pose, and tells the user the file name to upload.
- **Commands:** new `export_anim`: `path` (must end in `.anim`, same folder rules as
  `save_file`, never replaces a file), `priority` (0 to 6, default 2 as in the export code), `loop` (uses the loop
  points), `ease_in` and `ease_out` seconds, `hand_pose` by name (the 14 names the export dialog
  uses). It runs `check_animation` first and refuses on an error that Second Life would refuse
  (for example over 60 s); the answer lists warnings, joints left out and the file size.
- **Acceptance tests:**
  - Export `examples/jump.dingle` through the command, read the file back with the `.anim`
    reader: same frame count and duration (to one frame), `priority` 4 when asked for 4, loop
    flag and loop points as set, hand pose index matching the name.
  - Same options through the command and through `AnimExport` directly give byte-identical files.
  - A path without `.anim`, outside the folder, or an existing file fails; nothing is written.
  - An unknown hand pose name fails and lists the 14 names; priority 7 fails.
  - An animation longer than 60 s fails with the `check_animation` reason.
  - Fuzz run on the branch is green.
- **Files:** `src/controlapi.*`, `tests/tst_control.cpp`, `tools/mcp/commands.json`,
  `fuzz/control.dict`, `docs/CONTROL.md`, `docs/ASSISTANT_GUIDE.md` (step 10: save, then export).
- **Kind:** technical. **Model:** light (Sonnet): it wraps existing export code.
- **Depends on:** `check_animation` merged; after job A (both edit `src/controlapi.*`, and the
  loop option uses `set_loop`).

## Job C: readable screenshots

- **Goal:** one picture the assistant can check reliably: several views at once, the figure's
  left and right marked, and a movement as a strip of frames.
- **Commands:** `screenshot` gets optional `views` (list of `front`, `back`, `left`, `right`,
  `three_quarter`; a grid, each cell captioned), `frames` (list of frame numbers; a strip,
  each cell captioned with the frame and seconds), `labels` (draws the classic joint names, the
  figure's left side in one colour and right in another) and `markers` (`false` hides the Bento
  joint markers). The program window and the user's camera never change.
- **Acceptance tests:**
  - Without the new fields the PNG is byte-identical to today's answer (same width).
  - `views` with 4 names gives a 2 by 2 grid of exactly twice the width and height of one cell;
    `frames` with 5 numbers gives 5 cells; the answer lists what is in each cell in order.
  - After any call, `state` returns the same camera and frame as before.
  - With `labels`, pixels of the left colour lie on the figure's left side in a front view and
    on the screen's left in a back view (checked on a T-pose by the colour's average x position).
  - More than 6 views or 12 frames, an unknown view or a frame outside the animation fails.
- **Files:** `src/controlapi.*`, `src/animationview.*` (off-screen render only),
  new `src/screenshotsheet.*`, `tests/tst_control.cpp`, `tools/mcp/commands.json`,
  `fuzz/control.dict`, `docs/CONTROL.md`, `docs/ASSISTANT_GUIDE.md`.
- **Kind:** technical (only the image sent to the assistant changes; nothing in the program's
  window). **Model:** strong (Opus): off-screen rendering and label placement.
- **Depends on:** `describe_motion` and `check_animation` merged; can run before or after job B,
  but not at the same time as A or B (shared `src/controlapi.*` and `commands.json`).

## Order

A, then B, then C. Each runs alone because all three edit `src/controlapi.*` and
`tools/mcp/commands.json`.
