# Roadmap (confirmed 22 November 2026)

Status: **confirmed by the project owner on 22 November 2026.** It replaces the proposal of 19 November 2026 (in the git history). It sets the order of the work; `docs/PRODUCT.md` says what the program is for and `docs/DECISIONS.md` holds the decisions. Running work is the open pull requests, the job queue is in the jobs list (#74) and the owner's tests are in the test list (#47); this file does not repeat them.

## Order

1. **GitHub setup and process.** Website-only changes skip the program builds (done, pull request 345). Then: fix the Windows `control` test that fails now and then, a workflow that merges technical pull requests with the `auto-merge` label when all required checks are green, and a review of the routines and processes.
2. **RLV tool (Script Lab).** Finish the common LSL functions (step 9b), run the gap list again (9c), add script warnings (lint), test the whole tool properly (automatic tests where possible, then the project owner), and build the Script Lab panel as in the clickable mockup. The Script Lab panel takes priority over the rest of step 4's build (the project owner, 23 November 2026): step 4 waits until the panel is done. The RLV tool comes first because it was not tested as a whole; the automatic whole-tool test is in place (pull request 364, `docs/SCRIPT_LAB.md`, "Tests"), the project owner's test of what needs a person is next. From the second gap list, step 1 (the touch events `touch` and `touch_end`, pull request 367), step 2 (cheap reads and state of the object itself: `llDie`, `llGetObjectDetails`, `llSetScriptState` and the others, pull request 365), step 3 (region and avatar reads: `llGetRegionName`, the clock functions, `llGetAgentList` and the others, pull request 370) and step 4 (money, web server and mail stubs: `llGiveMoney`, `llEmail`, `llRequestURL`, `llHTTPResponse` and the events `money`, `http_request` and `email`, pull request 373) and step 5 (prim media: `llSetPrimMediaParams`, `llGetPrimMediaParams` and `llClearPrimMedia`, record only, pull request 374) are done.
3. **Website.** Revise the text and content of the project website, including the download page (issue 147). Runs in parallel with the other steps, because it touches only the website files.
4. **New UX design in the rest of the program.** Build the clickable mockup (`docs/mockup/index.html`, design charter and design system) into the program one step at a time, with the project owner's notes from his tests: the light theme must reach the timeline, the overshoot must be visible on the easing tiles, the timeline must scroll consistently to the picked joint, Start green and Stop red. Each with automatic tests where the answer is exact.
5. **Easing per joint** (issue 11): easing choices for one joint, not only for the whole pose change.
6. **Reference library browser** (issue 19): a panel to browse and use the bundled poses and motions.
7. **Teaching texts** (issue 54): the beginner texts, once the texts are stable after the new design.
8. **Translations** and the language choice in Settings. The plumbing is built (pull request 338, `docs/TRANSLATION.md`); no translation exists yet.

## Later (not planned)

- A simpler, neutral body in the 3D view on the same Second Life skeleton, after the new UX design.
- Windows code signing (issue 146): on hold; the project owner decides later.
- Aurora track (separate repository): ideas only; nothing in PinglyMotion waits for it.

## Done before this roadmap

File formats (`.dingle`, `.anim` export and import, BVH retargeting with the joint mapping dialog, glTF import), the Blender round trip, test motions, the renderer, joint limits and IK, easing presets with the tile row, the transport buttons, Frame back and Frame forward, the timeline scrolling to the picked joint, the clickable mockup, the translation plumbing, version 0.1.0, and Script Lab steps 1 to 9a, 9b-1 to 9b-9 and steps 1 to 5 of the second gap list. Details are in the git history, `docs/SCRIPT_LAB.md` and `docs/SCRIPT_LAB_PLAN.md`.

## Not doing

Direct Maya support, MocapNET, FreeMoCap code (AGPL), importing Firestorm pose XML, features that rely on AI inside Aurora's own viewer.
