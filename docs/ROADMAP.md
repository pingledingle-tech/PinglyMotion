# Roadmap proposal (19 November 2026)

Status: **proposal for the project owner to confirm.** It sets an order for the open issues after the Aurora comparison (`docs/archive/AURORA_STUDIO.md`) and the open-source check (`docs/archive/OPEN_SOURCE_CHECK.md`). It does not replace `docs/PRODUCT.md` or `docs/DECISIONS.md`; where they disagree on goals or decisions, those win until the owner decides. This file is the one place for progress status; `docs/PRODUCT.md` does not repeat it, and running pull requests and the owner's tests are in the jobs list (#74) and the test list (#47).

## Current state (20 November 2026)

- Foundation done: modern renderer (OpenGL 3.3 core, GPU skinning), joint limits on the SL standard and a new IK solver, joint tabs with the Firestorm joint data, status bar, new parser and data model with round-trip tests and fuzzing (#9), security audit fixes (`docs/SECURITY_AUDIT.md`), version in the window title, Ruth2 and Roth2 bodies (#15), live link to Aurora with quaternion poses (old #78, #95).
- File formats (steps 1 to 5 below): `.dingle` (#26), `.anim` export and import (#10, #22), BVH retargeting with the hand-made `.jointmap` core and its mapping dialog (#16, done), glTF import (#12), all with their menus; Blender guide and scripted round trip, run in Blender 5.3 and in CI (#13).
- Test motions (step 4, #27): the Mesh2Motion and CMU clips are in `examples/`.
- Easing (step 6, #11, #53): #53 steps 1 to 5 are done (maths, playback, `curve` in `.dingle`, export, tile row); the wave (step 6, `examples/wave.dingle`) is done; the custom speed curve for one pose change (#11, first step) is merged, per-joint choices are still missing.
- Posing (step 7, #14): camera and keys, joint list names and panels P1 to P6 are done; the Start, Stop and Replay from start buttons with the Loop switch are done (pull request 251, 21 Nov 2026); the selected-joint card, mirror and whole-pose buttons and the rest are open. The agreed UX decisions and build order are in [UX_DECISIONS.md](UX_DECISIONS.md) (20 November).
- Design (21 November 2026): the whole interface becomes one modern design. [DESIGN_CHARTER.md](DESIGN_CHARTER.md) is the top UX document (principles P1 to P13) and wins over UX_DECISIONS.md; values are in [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md).
- Validated on 21 November 2026 in Firestorm: export, upload, playback, loop, slow start and finish, hands, easing, the BVH route and re-import (#159, closed).

## Starting point (19 November 2026)

- Order already decided in `docs/DECISIONS.md`: `.anim` export (#10), then `.anim` import (#22), then BVH retargeting (#16), then glTF (#12), then Blender docs (#13). Rejected: glTF export, FBX (only on demand), Collada, VMD, VRMA, Firestorm pose XML, Assimp.
- New since then: Mesh2Motion's MIT-licensed retargeting is a ready reference (#16), and its CC0 animations are a source of test motions (#27).

## Proposed order

The table is the original proposal; the state of each step is under "Current state" above.

| Step | Issue | Job | Why now | Depends on | Model |
|---|---|---|---|---|---|
| 1 | #10, then #22 | `.anim` export, then import, one job at a time (same format code) | Decided order; gives priority, ease and loop points that BVH upload cannot; real motion needed to check it | #9 (done) | Sonnet |
| 2 | #16 | BVH retargeting core: name mapping, SL/Bento slots, swing-twist | New files, so it does not collide with step 1; Mesh2Motion removes most of the design risk | none | Opus |
| 3 | #12 | glTF 2.0 import using the retargeting core | Needs step 2 | #16 core | Sonnet |
| 4 | #27 | Test motions: CMU clips (#3) and CC0 clips from Mesh2Motion | Needs #12 or a conversion script | #3, #12 | Haiku |
| 5 | #13 | Blender round trip: guide, then a scripted round trip; glTF part after #12 | Docs and tools only | #12 | Sonnet |
| 6 | #11 | Per-key curve and easing editor, then #53 easing presets | Visible, so the owner tests on Windows | none | Sonnet |
| 7 | #14 | UX revision of poser and timeline | Visible, larger; discuss before building | #1 | Opus |

Visible changes (steps 6 and 7, and #40) need the project owner's Windows test before merge. Steps 1 to 5 were technical jobs under the standing permission in `CLAUDE.md`, except their menus: the `.anim` export and import menus (#110, #111), the BVH menu (#116) and the glTF import menu (#117) were visible, and the project owner tested them on Windows before they were merged.

## What can run in parallel

- Step 1 and step 2 touch different files and can run at the same time. Step 2 must not touch the `.anim` code; step 1 must not touch the retargeting files.
- Documentation jobs (Maya and live-link notes, issue clean-up) never collide with code jobs.
- Steps 3 and 4 waited for step 2 (done).

## Script Lab track (separate from animation and UX)

Status: **steps 1 to 7, 9 and 9a done, 9b in progress; 8 is a description only** (the RLV core, issue 287: parser, behaviour state and the control commands `rlv_command`, `rlv_state`, `rlv_reset`; the Script Lab panel, issue 288: View > Script Lab; the LSL interpreter, issue 289, with `lsl_check` and `lsl_run`; the link, issue 290: `llOwnerSay("@...")` runs as an RLV command; events and several objects, issue 291: `lsl_run` with `objects`, `lsl_event`, `llDialog`, `attach`; Live, issue 292: the folder watch, the panel's Live page and `lab_watch`, `lab_unwatch`, `lab_result`; effects in the 3D view, issue 293: the "Show effects in 3D view" box; RLVa-only behaviours and the RLV / RLVa mode switch, issue 297; several scripts in one object with link messages and Linkset Data, issue 305; input buttons in the panel, issue 308; RLV camera distance limits, `setcam_unlock`, the field of view limits and the focus and eye offsets shown in the 3D view with the "Show camera limits in 3D view" box (job 7b), and the force commands `setcam_fov`, `setcam_focus` on the avatar and the forced offsets (job 7c part 1; `setcam_mode` and mouselook stay recorded only), and the sky colour and draw distance (`setenv_daytime`, `setenv_bluehorizon`, `camdraw*`, `setsphere`) as the background and fog with the "Show environment in 3D view" box (job 7c part 2), and `showself`, `setcam_textures` and `setoverlay` with the "Show view modes in 3D view" box (job 7c part 3); see [SCRIPT_LAB.md](SCRIPT_LAB.md)); step 8 (the repository itself) is described but not created, 9b is in progress (9b-1 and 9b-2 merged on 22 Nov 2026, 9b-3 is an open pull request) and 9c is planned. A sandbox for testing RLV and LSL scripts written with Claude Code; decision in [DECISIONS.md](DECISIONS.md), plan in [SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md). It mostly adds new files under `src/scriptlab/` and does not wait for the animation and UX steps above. One issue per step, started only on the project owner's word:

1. RLV core ([#287](https://github.com/pingledingle-tech/PinglyMotion/issues/287), technical) — done
2. Script Lab panel ([#288](https://github.com/pingledingle-tech/PinglyMotion/issues/288), visible) — done
3. LSL interpreter, minimum ([#289](https://github.com/pingledingle-tech/PinglyMotion/issues/289), technical) — done, with the control commands `lsl_check` and `lsl_run`
4. Link: `llOwnerSay("@...")` to the RLV core ([#290](https://github.com/pingledingle-tech/PinglyMotion/issues/290), technical) — done
5. Events and several objects ([#291](https://github.com/pingledingle-tech/PinglyMotion/issues/291), technical + visible) — done: several objects on one simulated clock, `lsl_event`, `llDialog`, `attach`; the panel lists the objects and the newest dialog
6. Live: folder watch and `lsl_*` commands ([#292](https://github.com/pingledingle-tech/PinglyMotion/issues/292), visible) — done: the Live page and `lab_watch`, `lab_unwatch`, `lab_result`
7. Effects in the 3D view ([#293](https://github.com/pingledingle-tech/PinglyMotion/issues/293), visible) — done: the Live page's "Show effects in 3D view" box (off by default) plays the animation a script started on a preview figure and shows worn objects and locks as markers
8. Separate repository for the owner's RLV tools, description only ([#294](https://github.com/pingledingle-tech/PinglyMotion/issues/294), technical) — description written in [RLV_TOOLS_REPO.md](RLV_TOOLS_REPO.md); the repository is not created and the `.rlvtest` runner is not built, both need the project owner's word
9. RLVa-only behaviours and a plain RLV / RLVa mode switch ([#297](https://github.com/pingledingle-tech/PinglyMotion/issues/297), technical) — done: every behaviour is marked `RLV`, `RLVa only` or `RLV only`, plain RLV is the start mode, `rlv_reset` has a `mode` field
   - 9a. Several scripts in one object and link messages (`llMessageLinked`, `link_message`, Linkset Data; [#305](https://github.com/pingledingle-tech/PinglyMotion/issues/305), technical) — done: `scripts` and `prims` in `lsl_run`'s objects, `llMessageLinked` with the `LINK_*` targets, `llGetLinkNumber`, `llLinksetData*` and the `linkset_data` event
   - 9b. More common LSL functions (technical) — in progress: 9b-1 ([#310](https://github.com/pingledingle-tech/PinglyMotion/issues/310)) the signature table from Second Life's lsl-definitions and 25 pure functions (rotation maths, strided lists, `llReplaceSubString`, Base64, hashes, URL escaping, object name, description, scale); 9b-2 ([#314](https://github.com/pingledingle-tech/PinglyMotion/issues/314)) the events `on_rez` and `changed`; 9b-3 ([#317](https://github.com/pingledingle-tech/PinglyMotion/issues/317)) prim state in the log, L$ logic and record-only stubs; 9b-4 simulated inventory, notecards and rezzing (`dataserver`, `object_rez`); 9b-5 sitting and pose balls (`sit` and `stand` inputs, `changed` with `CHANGED_LINK`); 9b-6 controls (`llTakeControls`, the `control` input and event); 9b-7 sensors (`llSensor`, `llSensorRepeat`, `sensor`, `no_sensor`); 9b-8 record-only stubs and animation overrides
   - 9c. Gap list: what Second Life scripts use that the sandbox does not run yet (technical) — planned, no issue yet

### Script Lab: later (not planned yet)

Ideas only; nothing is being built now. They come after steps 3 to 7 and each needs the project owner's word first.

1. **Notecard recipes:** read a recipe from a notecard scripting tool (triggers, actions, menus, conditions, including sending RLV commands) and show what it does. It is its own language, so it is a new step after the LSL parts. Only test recipes are used, and the tool's author is asked before anything is built against it.
2. **Stand-in for a closed third-party RLV HUD product:** a simulated copy that listens on the product's control chat channel (69), logs what it hears and answers, so a user's own HUD can be tested without the real product.
3. **Input buttons in the panel:** done (issue 308): the Live page's Send input part.
4. **Script warnings (lint):** warn when a script uses something Second Life would reject, such as a wrong function name.
5. **Animating simple objects:** `llSetPos` and rotation on simple shapes such as a box or a sphere in the 3D view. A much simpler alternative: jump straight to a "closed" state when a lock triggers.
6. **Chain and leash visuals:** a plain line between two cuffs or a leash. Second Life uses `llParticleSystem` for this; for now only the logic is tested.
7. **HUD workflow:** an AI makes the HUD picture and the script, Script Lab tests that the buttons do the right things, and the owner uploads the texture in Second Life. The appearance cannot be previewed.

A script repository from another creator may later be worked on through a fork and a pull request, only with that creator's permission and after steps 3 to 6.

Out of scope: full LSL, all functions and all RLV behaviours; a slow ring or collar closing animation, chain physics, particles as pictures, mesh and texture preview; real money transfers (only the logic and messages around them). The final test is always in Second Life.

## Aurora track (separate repository, owner decides)

1. **Option C first:** try Aurora's editor offline with the environment variables from `docs/archive/AURORA_STUDIO.md`, on the owner's Windows PC. No code, a few minutes. Result decides whether an own-named "studio mode" fork (option B) is worth it.
2. **Before any pull request to Aurora:** comment on the matching issue or feedback post and wait; follow their template and rules (section 5 of `docs/archive/AURORA_STUDIO.md`). First candidate: joint limits in Rust (about a night to three days including learning the code).
3. Nothing in PinglyMotion's code is blocked by this track.

## Open decisions (owner)

1. ~~Product direction~~ **Decided 19 November 2026:** PinglyMotion continues as its own program (see `docs/DECISIONS.md`, #59).
2. ~~Whether to ask the Aurora developers before building~~ Not needed now that nothing waits for Aurora.
3. Whether to keep the Maya idea; today it is out of scope.

## Not doing

Direct Maya support, MocapNET, FreeMoCap code (AGPL), importing Firestorm pose XML, features that rely on AI inside Aurora's own viewer.
