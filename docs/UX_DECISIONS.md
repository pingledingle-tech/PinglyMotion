> The design charter (docs/DESIGN_CHARTER.md) wins if this document disagrees.
>
> Note: the "Homepage" section below (English only, with the mascot as the star) may be outdated; the homepage is being redone from the project owner's own text. It is left unchanged until the project owner decides.

# UX decisions: timeline and poser as one design

Status: agreed with the project owner on 20 November 2026. Each step starts only when the owner says start. Merged: step 1 (Start, Stop, Replay from start, Loop; pull request 251) and step 2 (Frame back and Frame forward; pull request 271). Built, open pull requests waiting for the owner's Windows test: step 3 (easing tiles, pull request 269), the timeline scrolling to the selected joint (pull request 273) and the transport buttons in the design system (pull request 275). Steps 4 to 13 are not built yet. Issue #14 tracks the work. Where this page and [POSING_UX.md](POSING_UX.md) differ, this page wins.

## General rules

- Clean screen: "you need what you need when you need it". Advanced things stay hidden until you ask for them.
- It must work on a small laptop screen and scale well on big screens and second screens. All panels and small windows can be moved, resized and popped out.
- Automate tests wherever possible (buttons exist, settings are remembered, the welcome window shows only once). The owner tests only what he sees and feels on the Windows build.
- Anything can be changed later if it turns out badly.

## 1. Transport (play buttons)

- Buttons: Start, Stop, Replay from start, Step one frame back, Step one frame forward.
- Loop on/off switch, on by default.
- Step 1 (built and approved): Start, Stop and Replay from start buttons and a Loop check box under the timeline (`src/transportbar.*`). Start can be clicked only while stopped, Stop only while playing, Replay from start always. The Loop check box and Options > Loop are the same setting, remembered between sessions.
- Step 2 (built and merged, pull request 271): **Frame back** and **Frame forward** buttons right of Replay from start, with the same look as the other transport buttons. Each goes one frame and stops at the ends, never wrapping around: Frame back cannot be clicked on the first frame, Frame forward not on the last. While playing, a click stops playing first. The left / right arrow keys do the same when no field, list or slider has the keyboard focus. Clicking a transport button does not take the keyboard focus, so the arrow keys keep working afterwards. A modern button look (Start green, Stop red, a replay icon) is a separate later item.
- Speed control: not now. Add it later if it is missed.

## 2. Easing tiles

- Text names by default.
- The small preview picture animates only while the mouse hovers over a tile. No constant motion.

## 3. Curve editor

- Opens as a small separate window from an "Edit curve" button (on the easing tile or the selected key).
- Movable, resizable, can go on a second screen. Opens small on a laptop.

## 4. Teaching notes (like WAVE-01 to WAVE-06)

- One thin text line under the timeline, showing the note for the current frame while playing.
- A button hides it.

## 5. Timeline look

- One row per joint, diamond keys where a pose is set.
- Shows only joints that have been changed, so it stays short.
- Rows are coloured by body area, with the same groups and colours as the Joints panel tabs (Body / Face / Hands / Other).
- New: select a stretch of the timeline, copy it, and Paste mirrored further along (a left step becomes a right step).

## 6. Selected joint panel

- Order from the top: joint name (and SL name), Reset, rotation sliders X/Y/Z showing the allowed range, limit warning.
- Advanced things (IK, lock, move) further down.
- Joint limits are a helper, not a wall: they warn, and can be switched off. SL has no limits, so strange poses on purpose must be possible.

## 7. Mirror and whole pose

- Left / Right / Mirror switch at the top of the Joints panel.
- Whole-pose buttons (Flip, L→R, R→L, T-pose, Poses…, Reset all) in one small row at the bottom of the Joints panel.
- Plus "Paste mirrored" on the timeline (see 5).

## 8. Beginner and expert

- Starts in Beginner mode by default; Expert mode can be switched on (shows curves, IK, all tracks and so on).
- The choice and the layout are remembered between sessions.
- First start only: a small welcome window asks "New to animation, or experienced?". It never asks again; the choice can be changed later in the menu.

## 9. Build order

This is the one numbered list of interface steps ("UX step N" in issues and pull requests means step N here). Other documents name a step by its name and link here; old numbers from other documents are in the table under the list. Small steps, each tested on Windows. Steps that touch the main window run one after another. Tests already waiting (pop-out panels P6, example wave, curve editor step 1, import clean-up options) come first.

1. Transport: Start / Stop / Replay from start + Loop (on by default). Owner test: play, stop, replay, loop on/off.
2. Frame step back / forward. Owner test: step through the example wave.
3. Easing tiles as text, preview animates on hover only. Built: only the tile under the mouse moves its figure; the names are unchanged.
4. Timeline: rows only for changed joints, diamond keys. Owner test: edit two joints, only those two rows appear.
5. Timeline rows coloured by body area. Owner test: colours match the Joints tabs.
6. Selected joint panel in the new order; limits as a warning with an off switch.
7. Joints panel: L/R/Mirror switch at the top, whole-pose row at the bottom.
8. Timeline: select range, copy, Paste mirrored. Owner test: mirror a left-leg step into a right-leg step.
9. Beginner / Expert mode, remembered. Owner test: switch, restart, mode kept.
10. First-run welcome window. Automated test that it shows only once; owner test: look and wording.
11. Curve window: opened from "Edit curve", movable, resizable, second screen; position kept after restart.
12. Teaching-note strip under the timeline (wave notes), hide button.
13. Homepage and icon (separate track, does not touch the main window, may run in parallel).

Done before this list (from the older plan in [POSING_UX.md](POSING_UX.md)): **Camera and keys**; **Joint list names, headings and search**; **Panels** (floating, movable panels, Tab to hide, reset layout; jobs P1 to P6); **Status bar** (Claude and Aurora status, tips switch); the **Easing tile row** itself ([EASING.md](EASING.md), issue 53).

Later, order not decided yet (from the older plan in [POSING_UX.md](POSING_UX.md)): **Hands tab** (presets and finger grid); **Help tips and hint bar** (`?` controls list); **Command search**; **Pose library and export selection**; **Settings page**; **Visual polish** (now the [design charter](DESIGN_CHARTER.md) and [design system](DESIGN_SYSTEM.md)); **The 3D view follows the tab** (only that group's joints shown and clickable).

Old step numbers, so older issues and pull requests stay readable:

| Old number | Where it was used | Step name now |
|---|---|---|
| step 1 | POSING_UX.md, PRODUCT.md track 1 | Camera and keys (done) |
| step 2 | POSING_UX.md, PRODUCT.md track 1 | Joint list names, headings and search (done) |
| step 3 | POSING_UX.md, PRODUCT.md track 1 | Panels, jobs P1 to P6 (done) |
| step 4 | POSING_UX.md, PRODUCT.md track 1 | Selected-joint card: now UX step 6, Selected joint panel |
| step 5 | POSING_UX.md, PRODUCT.md track 1 | Left / Right / Mirror and whole-pose buttons: now UX step 7, Joints panel |
| step 6 | POSING_UX.md | Hands tab (later) |
| step 6 | PRODUCT.md track 1 | Milestone: sitting pose to Second Life (done, issue 159) |
| step 7 | POSING_UX.md | Timeline, only edited joints: now UX step 4; its easing chips are the Easing tile row (done) |
| step 7 | PRODUCT.md track 1 | The later posing steps (the "Later" list above and UX steps 4 to 13) |
| step 8 | POSING_UX.md | Status bar (done) |
| steps 9 to 12, 14 | POSING_UX.md | Help tips and hint bar; Command search; Pose library and export selection; Settings page; The 3D view follows the tab (all later) |
| step 13 | POSING_UX.md | Visual polish (later) and own icon (UX step 13, Homepage and icon) |
| #53 step 5, "Tile row (step 5)" | EASING.md, issue 53 | Easing tile row (done). The numbers in EASING.md are the steps of issue 53, not interface steps |
| UX step 1, 2, 3 | issues 14, 74 | Transport; Frame step back / forward; Easing tiles as text, preview on hover only |

## 10. Icon

- Made together with the homepage so they look alike (exe, window, website). Try building it from the mascot (a tube on wheels with a cartoon face).

## Homepage

- Language: English for now, like the program; translatable later ([TRANSLATION.md](TRANSLATION.md)).
- Feeling: warm, cozy, welcoming, modern, and funny or a bit crazy. Animation should feel less scary.
- No green. It must not look template-like or machine-made (the earlier status section draft looked that way and is not approved).
- Base text: the project owner's own text, translated to English. Keep the jokes and self-irony.
- The mascot is the funny star of the page (and the basis for the icon); the owner judges it when he sees it.
- Joke: hover text "Naprzód!" on the Start button and "Stój!" on the Stop button on the homepage (Polish horse commands).

## Open

- Speed control for playback (later, if missed).
