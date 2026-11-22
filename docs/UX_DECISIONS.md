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
- Colours (the project owner, 22 November 2026): Start green (`go_fill`), Replay from start violet (`replay_fill`), Stop red (`danger_fill`), each with its icon and word. Shown in the clickable mockup; the built transport bar still has the older look until a job changes it ([DESIGN_SYSTEM.md](DESIGN_SYSTEM.md), "Shape").
- Look (built, waiting for the owner's test; redone in the design system of 21 November 2026): Start is the main button (accent colour) with a play icon, Stop a danger button (danger colour) with a square icon, Replay from start a normal button with a round arrow, all with the same shape and Lucide icons in the colour of their text, from the theme tokens in the dark and the light theme. A button that cannot be clicked is pale and says why in its tip and the status line ("It is already playing. Click Stop to stop it."). Keyboard focus shows a ring; when Start or Stop can no longer be clicked, the focus moves to the other one, so Space starts and stops. The buttons are drawn by the program (`src/transportlook.*`, icons `src/lucideicons.*`), grow with the text size and need no picture files. Texts, order and behaviour are unchanged.

## 2. Easing tiles

- Text names by default.
- The small preview picture animates only while the mouse hovers over a tile. No constant motion.
- In the clickable mockup each tile draws its curve: time to the right, the pose from a dashed Start line to a dashed End line. The part of a curve that goes past End is drawn thicker, shaded, three times taller and marked "past End", so an overshoot can be seen at once (the project owner saw only the figures and not the overshoot in his test). On hover a dot runs along the curve and a small bar on the right shows how far the pose has come.

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
- Colours (built, UX phase 0a, 22 November 2026): the timeline and the track names take every colour from the design tokens and follow the dark and the light theme, also when the theme is switched while the program runs; details in [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md), "Colours in the timeline and the 3D view". The rows are not yet coloured by body area.
- New: select a stretch of the timeline, copy it, and Paste mirrored further along (a left step becomes a right step).

## 6. Selected joint panel

- Order from the top: joint name (and SL name), Reset, rotation sliders X/Y/Z showing the allowed range, limit warning.
- Advanced things (IK, lock, move) further down.
- Joint limits are a helper, not a wall: they warn, and can be switched off. SL has no limits, so strange poses on purpose must be possible.
- Self-explaining for beginners, as shown in the clickable mockup: the joint name with its body-area chip and key count; one plain line that says how to turn it (sliders or rings, then Set key); each slider has its axis badge (X red, Y green, Z blue) and a plain name ("Tilt forward or back", "Turn left or right", "Lean to the side"), the allowed range under it and the number in degrees; near a limit the slider says so and a warning card names the axis and offers "Turn limits off". "When you drag this joint in the 3D view" is a plain choice between "Pull the limb" (IK, explained in one line) and "Turn only this joint". Lock, Move and "Turn around the axes of: Joint / Avatar / Screen" fold away under **More options**, in both modes.

## 6a. Dragging a joint in the 3D view

Shown in the clickable mockup (`docs/mockup/index.html`); waits for the project owner's yes before it is built in the program.

- Click a joint to pick it. Press on a joint and drag to **pull the limb**: the joints above it bend so the limb follows the pointer (IK pull). One gesture does both: pressing an unpicked joint picks it and pulls it.
- While dragging: a grabbing cursor, a dashed line from where the joint was to the pointer, the pulled limb highlighted, and a small label next to the pointer that says what is happening ("Pulling Right Hand", "Release: set key, Esc: cancel"). The hint bar says the same.
- **Release** sets the key at the current frame for the pulled joints. **Esc** during the drag cancels, nothing changes. **Ctrl+Z** undoes the whole pull in one step.
- **Pull or turn only:** the IK switch in the Selected joint panel (on by default) chooses what a plain drag does. Holding **Shift when you press** the joint does the other one. "Turn only this joint" swings that joint so its own bone follows the pointer. The pelvis and torso have no limb above them, so they only turn.
- **Keyboard alternative:** with a joint picked and the 3D view focused, **Enter** (or G) picks the joint up, the **arrow keys** move it (Shift: small steps), **Enter** sets the key, **Esc** cancels. The "Pull with keys" button in the Selected joint panel does the same. Shift + Enter picks it up in the other mode.
- First-run hint "Pull a limb" (after "Pick a joint"); it is also in the Camera and keys card and the Keys and mouse list.
- The ring and the sliders still turn a joint without keys being set until Set key, as before.

## 6b. Rotation rings and the view-direction widget

Shown in the clickable mockup (`docs/mockup/index.html`); waits for the project owner's yes before it is built.

- Three rings, red X, green Y and blue Z, are drawn around the **picked** joint (never around another joint), in the frame of its parent. Dragging a ring turns the joint on that axis only; Shift gives fine steps. Each ring carries its letter. The back half of a ring is drawn faint. Hovering a ring thickens it and the hint bar says "Drag the red X ring to turn the joint: tilt forward or back".
- Top left of the 3D view: a round **view-direction widget** like Blender's navigation gizmo, with coloured X, Y, Z ends that turn with the camera. Clicking an end looks from there (front, back, left or right side, top, below); dragging the widget turns the view. With the keyboard the ends are reached with Tab and arrow keys and chosen with Enter. The current view's name is under it.
- Under it a vertical **view toolbar** with names and keys (names can be switched off in its Show menu; a short 3D view gets a two-column icon toolbar): what the left mouse button does (Pick and pull V, Look around O, Move the view M, Zoom Z, the active one marked with a bar and a frame), framing (Picked joint F, Whole figure Shift+F, Home view 0), **Look from** (Front, Side, Top, Back; also in the View menu and on Numpad 1, 3, 7) and **Show** (floor grid, rotation rings, joint dots, tool names).

## 6c. Panels: pop out, move, resize, dock back; Setup

Shown in the clickable mockup.

- Every panel title has a pop-out button. A popped-out panel floats over the window with a "Floating" tag; it moves by its title (or with the arrow keys on its grip button) and resizes by its corner (or Shift + arrow keys); the same button docks it back. The thin lines between panels change their width or height by dragging or with the arrow keys. **View > Reset layout** docks everything back in its first place and closes Script Lab.
- Script Lab (View > Script Lab, Ctrl+Shift+L, or the Script Lab button in the top bar) docks on the right next to Selected joint; tabs switch between them, and picking a joint in the 3D view brings Selected joint to the front.
- At large text or in a small window the panels rearrange instead of scrolling sideways: with less room the Joints panel joins the right side as a tab; with very little room everything stacks in one column and the page scrolls down only. Floating needs room, so pop-out is greyed out there with the reason.
- Built (UX phase 0c, 22 November 2026): the text size setting (section 8) is applied to the whole window, at start and when it changes (`src/textsize.*`: the system font times the percent; the timeline rows, the panel title buttons and the easing tiles grow with it). The panels follow these rules (`src/panellayout.h`, section 7, and `src/panelwindow.h`):
  - **Wide** (the first layout) when Joints, the middle column and Selected joint fit side by side; **medium** (Joints a tab next to Selected joint on the right, tabs on top, Selected joint in front) when one side column and the middle column fit; otherwise **narrow**: every panel docked in one column under the 3D view (Joints, Selected joint, Script Lab, Timeline), each scrolling down only. The one column is at least as wide as its widest panel; at large text in a very small window the window stays that wide.
  - The middle column is as wide as the wider of the 3D view's smallest width (240 pixels, not in the mockup: chosen by the job, the project owner may change it) and the timeline with its rows broken into shorter lines. Each panel's width is what its widest control needs at the current text size.
  - The rows of the timeline panel (easing tiles, Start / Stop / Replay from start and Frame back / Frame forward / Loop, loop points and frame / speed) break into two lines when they do not fit on one; the timeline panel scrolls down when the window is too low for it.
  - In the narrow layout nothing floats: a floating panel is docked, and its pop-out button is greyed out with the reason "The window is too small to float panels. Make it bigger or the text smaller." (from the mockup). When there is room again the user's own wide layout comes back, floating panels included; that wide layout is also the one saved for the next start.
  - Differences from the mockup: the program decides by the panels' real widths, not by the mockup's fixed widths (1180 and 760 pixels), and a low window does not switch to one column, because the panels already scroll down. View > Text size (Ctrl+=) steps the text size since UX phase 1a (see 6e); the Setup dialog comes later; the settings file holds it too (`text_size_percent`).
- **Setup** (gear button in the top bar, View > Setup, Help > Setup): beginner or experienced, mouse and keys habits, theme, text size and hints in one dialog, so the welcome choices can be found and changed at any time (the project owner could not find them before). Help > Show the welcome card again stays.

## 6d. Script Lab panel in the new design

Shown in the clickable mockup, following what the program's panel holds ([SCRIPT_LAB.md](SCRIPT_LAB.md)): a one-line explanation, the RLV / RLVa switch (switching removes every restriction, as after a relog), two pages **Live scripts** and **Type commands**. Live: an empty state with "Watch folder...", then the folder with Stop watching, an OK or Error status line that starts with the word, Open in editor, Show effects in 3D view, and folding parts with counts: the script read only with line numbers (an error line in bold red and underlined), errors and warnings, the event log (owner-say lines and RLV lines marked), the RLV restrictions now, objects and the newest dialog, Send input (Touch, Say, dialog buttons) and the simulated inventory and notecards. Commands: Object, Command with Run (Enter), example lines to try, Reset all, the restrictions, what the avatar may do (Allowed or Blocked as words with icons), the camera and environment boxes, and replies and messages. A "Run again" button is drawn as a proposal; the program runs a script when it is saved.

## 6e. Menus as in the mockup (built, UX phase 1a, 22 November 2026)

The File, View and Help menus follow the mockup (`docs/mockup/index.html`). One table, `src/menuspec.h`, holds every new text, key and reason; the window builds the items from it and `tests/menutexts` checks each one exactly (and against the mockup file itself).

- **File:** New animation (Ctrl+N), Save (Ctrl+S), Export for Second Life (.anim)... (Ctrl+Shift+E), Close animation (Ctrl+W). Close animation works since phase 1b (below).
- **View:** the group "Look from" (Front Numpad 1, Side Numpad 3, Top Numpad 7, Home view 0), the group "Panels" (Joints, Selected joint, Show timeline, Script Lab Ctrl+Shift+L, Hide panels Tab, Reset layout, Next area F6), then Light theme (T), Expert mode (Shift+E), Text size: N% (Ctrl+=, steps of 25 from 100 to 200 and back to 100) and Setup.... The letter and number keys (T, Shift+E, Shift+H, 0) work while the 3D view has the keys, like Tab, so they never take a letter typed into a field. Light theme moved here from Options (where it was "Dark theme") and is the same saved setting, turned round.
- **Help:** Setup, Show the welcome card again, Show hints (Shift+H, the same setting as the "Tips on" switch in the status bar), Keys and mouse (?), Camera and keys..., Show charter principles (C), then What's new, Open log, Clear log, About.
- **Greyed with a reason** (the reason is the item's tip and its status line text): Next area, Setup (both places), Show the welcome card again, Keys and mouse, Camera and keys, Show charter principles. Each stays greyed until its own step is built.
- **Expert mode** is switched and remembered (`expert_mode`), but nothing reads it yet; the status line says so. The extra controls come with later steps.
- **Differences from the mockup, on purpose:** the mockup's "Open the dance example" is "Open..." here (the program opens any file); Export keeps "(.anim)" because Export BVH sits next to it and the approved wording of 20 November 2026 (`docs/TEACHING_TEXTS.md`, A9) keeps the two apart. Edit, Tools and Options are not part of this step.

## 6f. Top bar, Edit menu and empty state (built, UX phase 1b, 22 November 2026)

- **Top bar** (`src/topbar.*`, texts in `src/menuspec.h`), right-aligned as in the mockup: Undo, Redo | Save, Export | Script Lab, Setup, theme. Each button follows a menu item (same key, same greyed state, same reason as its tip and status line). Undo, Redo and Setup are greyed with the reason of their menu item (not built yet). With no animation open Save and Export are greyed: "Open an animation first." The Script Lab button shows pressed while the panel is open. The theme button shows the sun in the dark theme and the moon in the light theme; its tip says what a click does ("Switch to the light theme" or "Switch to the dark theme", key T). It does the same as View > Light theme, and the choice is remembered (`dark_theme`).
- **Not in the top bar on purpose:** the mockup's brand and file name (the window title has them), and the old New, Open, Add, Save as, Cut, Copy, Paste and Reset camera buttons (they stay in the menus).
- **Edit menu:** Undo (Ctrl+Z), Redo (Ctrl+Shift+Z), a line, Set key (K), Paste mirrored (Ctrl+Shift+V), then Cut, Copy and Paste (the timeline's frames; the mockup has no such items yet). Undo, Redo and Paste mirrored are greyed with a reason. Set key presses the key button and never takes a key away; it is greyed with the reason: "Open an animation first.", "Pick a joint first. Click it on the figure or in the list.", "Stop playing first." or "The first frame is locked. Turn off Lock the first frame to set a key." (the mockup's first and third reason are longer than 12 words, so they are shortened).
- **Close animation** (Ctrl+W) asks about unsaved changes like New animation, then shows the empty state. A clean untitled animation stays behind it so the old code never meets "no animation"; the control commands see none (`state` and the other commands answer "No animation is open."; `new_animation`, `open_file`, `find_reference` and `load_reference` still work).
- **Empty state** (`src/emptystate.*`): the title "No animation open", the line "Pick a pose to start. You can change it later." and the choices Standing, Sitting and Dance example, then New animation. The choices open the library clips "Stand, idle" (`m2m-idle-a`), "Sit, idle" (`m2m-sitting-idle`) and "Dance, simple" (`m2m-dance-simple`), by the same route as `load_reference`; each has the 19 joints, and the clip's credit shows in the status line. While it shows, the Joints, Selected joint and Timeline panels and the actions that need an animation are greyed ("Open an animation first."). Differences from the mockup, on purpose: the mockup's line is longer than 12 words, and its choice texts claim "a note on each key" which the library clips do not have, so the texts are shorter and true.

## 7. Mirror and whole pose

- Left / Right / Mirror switch at the top of the Joints panel.
- Whole-pose buttons (Flip, L→R, R→L, T-pose, Poses…, Reset all) in one small row at the bottom of the Joints panel.
- Plus "Paste mirrored" on the timeline (see 5).

## 8. Beginner and expert

- Starts in Beginner mode by default; Expert mode can be switched on (shows curves, IK, all tracks and so on).
- The choice and the layout are remembered between sessions.
- Where the choices are kept (built in UX phase 0b; the text size is applied since UX phase 0c, see 6c; the theme and the hints switch work; Expert mode is switched in View since UX phase 1a, see 6e, but nothing reads it yet; the camera habit is not read yet): `UserSettings` in `src/usersettings.*`, in the same settings file as the other options. Expert mode (`expert_mode`, default off = Beginner), hints (`tips_on`, default on), camera habit (`camera_habit`: `pinglymotion` (default), `maya`, `blender` or `sl`), text size (`text_size_percent`, a whole number cut to 100 to 200, default 100) and theme (`dark_theme`, default dark). An unknown name or a value that is not a number gives the default; a number outside 100 to 200 is cut to the nearest end. Covered by `tests/usersettings`.
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
