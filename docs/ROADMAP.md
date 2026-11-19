# Roadmap proposal (19 November 2026)

Status: **proposal for the project owner to confirm.** It sets an order for the open issues after the Aurora comparison (`docs/AURORA_STUDIO.md`) and the open-source check (`docs/OPEN_SOURCE_CHECK.md`). It does not replace `docs/PRODUCT.md` or `docs/DECISIONS.md`; where they disagree, those win until the owner decides.

## Starting point

- Done: parser and data model (#9), `.dingle` working format (#26: reader, writer and conversion merged; opening in #70; saving and the start poses built, waiting for the owner's Windows test), Ruth2 body and menu (the system avatar is removed, #15; Roth2 v2 for a male body is done, waiting for the owner's Windows test), version in title, `.anim` writer (#63), `.anim` reader (#65), BVH retargeting core (#66), glTF import reader (#69), Blender guide (#68), scripted Blender round trip with a BVH comparer (issue #13) and Maya note (#67). Fuzzing runs all targets at the same time, about 10 minutes on a branch (#71). None of the format jobs has a menu yet, so nothing changed on screen.
- Order already decided in `docs/DECISIONS.md`: `.anim` export (#10), then `.anim` import (#22), then BVH retargeting (#16), then glTF (#12), then Blender docs (#13). Rejected: glTF export, FBX (only on demand), Collada, VMD, VRMA, Firestorm pose XML, Assimp.
- New since then: Mesh2Motion's MIT-licensed retargeting is a ready reference (#16), and its CC0 animations are a source of test motions (#27).

## Proposed order

**State on 19 November 2026:** steps 1, 2, 3 and 5 are done as first parts (writer, reader, retargeting core, glTF reader, Blender guide). What is left in them is the menus (visible, so they need the owner's Windows test). Step 4 (#27) waits for the test motions in #3 (open). Step 6 (#11, #53): #53 steps 1 to 4 are merged (easing maths, playback through it, the choice stored as `curve` in `.dingle`, export test); the tile row (step 5, visible) is next. Step 7 (#14) is not started. `.dingle` (#26): jobs 1 to 3 are merged (#48, #51, #57); job 4, opening `.dingle` files, is in #70 and waits for the owner's Windows test. The table below is the original proposal.

| Step | Issue | Job | Why now | Depends on | Model |
|---|---|---|---|---|---|
| 1 | #10, then #22 | `.anim` export, then import, one job at a time (same format code); the export and import menus are built, waiting for the owner's Windows test | Decided order; gives priority, ease and loop points that BVH upload cannot; real motion needed to check it | #9 (done) | Sonnet |
| 2 | #16 | BVH retargeting core: name mapping, SL/Bento slots, swing-twist (core merged; the menu with the list of matched and left out joints is merged; the hand-made mapping core, `.jointmap` files found by skeleton fingerprint, is merged; left: the dialog that edits the mapping, visible) | New files, so it does not collide with step 1; Mesh2Motion removes most of the design risk | none | Opus |
| 3 | #12 | glTF 2.0 import using the retargeting core (reader merged; the menu is built, waiting for the owner's Windows test) | Needs step 2 | #16 core | Sonnet |
| 4 | #27 | Test motions: CMU clips (#3) and CC0 clips from Mesh2Motion | Needs #12 or a conversion script | #3, #12 | Haiku |
| 5 | #13 | Blender round trip: guide (#68) and scripted round trip with a BVH comparer (`tools/blender/`, not tested in Blender yet) merged; glTF part waits for the glTF menu | Docs and tools only | #12 | Sonnet |
| 6 | #11 | Per-key curve and easing editor, then #53 easing presets | Visible, so the owner tests on Windows | none | Sonnet |
| 7 | #14 | UX revision of poser and timeline | Visible, larger; discuss before building | #1 | Opus |

Visible changes (steps 6 and 7, and #40) need the project owner's Windows test before merge. Steps 1 to 5 are technical jobs under the standing permission in `CLAUDE.md`.

## What can run in parallel

- Step 1 and step 2 touch different files and can run at the same time. Step 2 must not touch the `.anim` code; step 1 must not touch the retargeting files.
- Documentation jobs (Maya and live-link notes, issue clean-up) never collide with code jobs.
- Steps 3 and 4 wait for step 2.

## Aurora track (separate repository, owner decides)

1. **Option C first:** try Aurora's editor offline with the environment variables from `docs/AURORA_STUDIO.md`, on the owner's Windows PC. No code, a few minutes. Result decides whether an own-named "studio mode" fork (option B) is worth it.
2. **Before any pull request to Aurora:** comment on the matching issue or feedback post and wait; follow their template and rules (section 5 of `docs/AURORA_STUDIO.md`). First candidate: joint limits in Rust (about a night to three days including learning the code).
3. Nothing in PinglyMotion's code is blocked by this track.

## Open decisions (owner)

1. ~~Product direction~~ **Decided 19 November 2026:** PinglyMotion continues as its own program (see `docs/DECISIONS.md`, #59).
2. ~~Whether to ask the Aurora developers before building~~ Not needed now that nothing waits for Aurora.
3. Whether to keep the Maya idea; today it is out of scope.

## Not doing

Direct Maya support, MocapNET, FreeMoCap code (AGPL), importing Firestorm pose XML, features that rely on AI inside Aurora's own viewer.
