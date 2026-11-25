# Player check: can a Script Lab run be rebuilt at any time t?

Job B0 of the plan "Script Lab: see and understand" (approved by the project owner, 24 November 2026, 00:08).
The planned player (job B3) plays a sandbox run like a film: stop, step forward, step back. The 3D view,
colours, restrictions and the menu follow the chosen time t. This page says whether that can be built on the
code as it is on `main` today, and what is missing.

**How this was checked.** Everything below was read in the code and in the existing tests. No experiment was
run: this job's machine has no Qt, so nothing was built. Where a test is named, it runs in the pull request
checks; this job did not run it. Nothing was tested in Second Life.

## Short answer

- **(a) The event log alone: no.** The log is a list of changes. It does not hold the start state or the full
  state. It is cut at 2000 entries per command, and some entries are skipped once it is full. The log of an
  `lsl_event` is lost at the next command.
- **(b) Run again up to t: yes, with gaps to fill.** The sandbox is built to be deterministic: a fixed
  simulated clock, a fixed order, fixed random numbers and fixed keys. It can stop at a time (`runUntil`). A new
  `Sandbox` is a clean start. What is missing: nothing stores the inputs of a session, the RLV state is shared
  with the rest of the program, and the real-time cap can break determinism.
- **Recommended base: (b), run once with a recorder.** Run again from the stored inputs once, take a small
  snapshot of the visible state after each moment, then step back and forth in the snapshots. Do not run again
  on every click.

## Per effect

| Effect | (a) From the log | (b) Run again to t | What is missing today |
|---|---|---|---|
| Colour and transparency (`labtint.*`) | Partly. `prim` entries with `color` and `alpha` are there, but not the start colour, and they are skipped when the log is full. `LabTint::fromFolder` reads only Live folder runs and gives only the last value. | Yes. | A reader that takes a time ("value at t"). A public read of the prim state: `Sandbox` keeps it (`PrimStore`), but has no accessor and `objectsJson()` leaves it out. |
| Prop place, size, rotation | Partly. `prim` entries `position`, `rotation`, `size` are logged as changes, same limits as above. The 3D view does not move props yet (job A1). | Yes. | Same accessor. A1 must exist first. |
| RLV restrictions | Partly. `rlv_command` entries hold the line and the results, and only a count of held restrictions. The full list needs the lines applied again to an `RlvState`, from the start state. | Yes, if the replay has its own `RlvState`. | `RlvState` is one shared object (the panel, `rlv_state` and the 3D view overrides read it). It is a `QObject` with no copy or snapshot, so the start state of a run with `keep_rlv` cannot be stored. The mode (RLV or RLVa) must be stored too. |
| Latest `llDialog` and its buttons | Yes, inside one command: `dialog` entries have `object`, `to`, `message`, `buttons`, `channel`. Lost after the next command. | Yes (`lastDialog()`, and per object in `objectsJson()`). | The dialog per time point. |
| Animation started by a script (`effectspreview.*`) | Yes in principle: `LabEffects::fromRun` already replays `animation_start` and `animation_stop` from the log. A reset, detach or `llDie` does not stop it there. | Yes. | `EffectsPreview` only moves forward (`advance(seconds)` and a real-time timer). It needs "show the frame at time t" (a seek). It now takes the start time from the end of the run. |
| Worn and locked markers on attachment points | No. `fromRun` reads worn objects from `objects` and locks from `rlv.state`, both end-of-run only. `attach`/`detach` inputs are logged; `llAttachToAvatar` is record only. | Yes (`objectsJson()` and the RLV state at t). | Snapshot of `objectsJson()` and the RLV state per time point. |
| Chat and log lines | Yes, they are the log, with `t` on every entry. | Yes. | The whole log of a session: today each `lsl_event` empties it (`newCommand()`), and it is cut at 2000 entries per command. |
| 3D view overrides (camera, environment, view modes) | Partly: from the `recorded()` values of the RLV state. | Yes, with a private `RlvState`. | They follow the shared `RlvState`. Showing t needs a way to point them at the replay's state, or to leave them out of the player. |

## Checks for (b), from the code

- **Stop at t:** `Sandbox::runUntil(end)` runs everything due up to `end`. An event always runs to its end, so t
  falls on event edges. `llSleep` moves a script's clock, so `now()` can end past `end`.
- **Clean start:** a `Sandbox` is used once (`addObject` only before `start()`), so a new one is a clean world.
  `Control::run` already makes a private `RlvState` when it gets none.
- **Same order every time:** objects in list order, timers before inputs, inputs in the order given
  (`sandbox.h`). Tests assert that the same request gives the same log (`tests/tst_lslworld.cpp`,
  `tests/tst_lsllink.cpp`); not run by this job.
- **Random numbers and keys:** one fixed generator per script (seed 12345, `interp.h`); keys, URLs and inventory
  keys are made from fixed seeds. `llGetUnixTime`, `llGetTimestamp` and `llGetDate` come from the simulated
  clock and the region's fixed start (`world.h`). No wall clock is read in the results.
- **Timers and listeners:** they live in the interpreter and run on the simulated clock, so they repeat.
- **Not deterministic: the real-time cap.** Each script may run 2000 ms of real time per command
  (`Limits::maxWallMs`, `interp.cpp`). A heavy script can stop at a different place on a slower machine or under
  load. A replay must check this (compare its log with the stored log) and say so when they differ.
- **Command edges:** `lsl_event` calls `newCommand()` (a fresh budget) and adds the input at `now()`. A replay must
  do the same at the same moments, not merge everything into one `lsl_run`; `lsl_run` also allows only 200 inputs.
  Not tested: that a run split in several `runUntil` calls gives the same log as one call. No test of that was found.

## What a player needs that is missing

1. **Session history:** `lsl::Session` keeps only the `Sandbox`. It must also keep the `lsl_run` request (the
   script text, not a path: a Live file can change), every `lsl_event` with its time and `seconds`, the RLV start
   state and the mode. Inputs from the panel's buttons go through the same `Control::event`, so they are caught too.
2. **Replay:** build a new `Sandbox` with its own `RlvState` from that history and run it to t (or to the end).
3. **Snapshots:** a hook after each moment that stores what the screen shows: objects, last dialog, RLV
   restrictions and recorded values, prim state, playing animations, and the log index. Needs the prim-state accessor.
4. **Seek in the 3D view:** `EffectsPreview` and the tint/prop code take a time, not only "the end".
5. **UI:** a player bar in the Script Lab panel. `TransportBar` and `Playback` work on whole numbers (frames), so
   the steps can be snapshot numbers. While the player is not at the end, the input buttons must be off, or an
   input must cut the future away. That is a design choice for the owner.

## Size and proposed split

**Large** in all, but it splits well. Proposed pull requests, in order:

1. **History and replay** (technical, no screen change): store the history in `Session`, replay it into a new
   `Sandbox` with a private `RlvState`, a check that the replay log equals the stored log, a prim-state accessor.
   Tests: replay equals original for `lsl_run` plus several `lsl_event`, with `keep_rlv`, with a dialog answer. Medium.
2. **Timeline of snapshots** (technical): the snapshot hook and "state at t". Tests per effect. Medium.
3. **Player bar and the panel** (visible): `TransportBar` in the Script Lab panel, the lists, objects view and
   dialog show the snapshot at t. Exact tests for buttons and steps. Medium.
4. **3D view follows t** (visible, taste round): animation seek, markers, colours and props at t; the overrides if
   the owner wants them. Medium.

Job A3 (record and replay of clicks and menu choices) also needs a stored input history. Build that part once, in
whichever job comes first, and reuse it.

## Files a player job touches

- Sandbox and session: `src/scriptlab/lsl/session.*`, `src/scriptlab/lsl/sandbox.*`, `src/scriptlab/lsl/control.cpp`.
- RLV: `src/scriptlab/rlvstate.*` (snapshot and restore), maybe `src/scriptlab/lsl/rlvworld.*`.
- Effects: `src/scriptlab/effectspreview.*`, `src/scriptlab/labeffects.*`, `src/scriptlab/labtint.*`.
- Script Lab panel: `src/scriptlab/scriptlabpanel*.*`, `src/scriptlab/objectsview.*`.
- 3D view: `src/animationview.*` (markers, preview), `src/pinglymotionwindow*.cpp` (tints, transport wiring),
  maybe `src/scriptlab/cameraoverride.*`, `environmentoverride.*`, `viewmodesoverride.*`.
- Play buttons: `src/transportbar.*` and `src/playback.h` (reused; changed only if a second bar needs it).
- Tests: new files under `tests/`, `tests/CMakeLists.txt`. Docs: `docs/SCRIPT_LAB.md`, `docs/CONTROL.md` if a
  control command is added.

Overlaps: A1 and A2 (the 3D view, the sandbox, the panel), B2 (the timeline and the view), A3 (input history).

## Open risks

- The real-time cap can make a replay differ from the run the user saw. It must be caught and shown, not hidden.
- Memory: a snapshot per moment of a 3600-second run with many events can grow large. Snapshots should be taken
  only when something visible changed, with a cap.
- Shared `RlvState`: a player that writes into it would change the real panel and the 3D view. The replay must use
  its own state.
- A held touch, `llSleep` and events that run past t make t snap to event edges; steps are moments, not exact seconds.
- What happens when the user sends an input while the player shows the past is not decided.
