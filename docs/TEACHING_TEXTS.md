# Teaching texts in the program: inventory, voice guide and proposed wording

Status: **approved by the project owner on 20 November 2026.** Part of issue #54. Step 1 (menus and labels, sections 3.1 to 3.3) is built; steps 2 to 4 (section 5) are not. The tables below still show the wording before step 1 in the column "Current wording".

How this page relates to `docs/TEACHING.md`: that page is the register of the 45 planned beginner texts (welcome card, tips, hint bar, next-step line, starter poses, warnings). The owner approved them on 19 November 2026. This page covers what is **already in the program and the docs today**: menus, labels, dialogs, messages and error texts. It checks them against the same rules and proposes new wording where they break the rules.

Source of the inventory: `python3 tools/extract_texts.py` (all `tr()` strings, tips and `.ui` labels in `src/`; 828 strings on 20 November 2026). Run it with `--markdown` to get the raw list again. This page keeps every text a beginner meets in the first hour in a table of its own (sections 3.1 to 3.7). The about 440 technical file-error details are handled as patterns (section 3.8), because they follow a few shapes.

## 1. Voice guide

The rules in `docs/TEACHING.md` (section 3) stand. They are short: say what to do, one idea per sentence, plain words, no pop-ups for teaching, a way out for every warning, English through `tr()`, never claim what is not true. This page adds what the inventory showed was missing.

1. **Menu items: sentence case, verb first.** "Export for Second Life", not "Export For Second Life". Three dots only when a dialog follows.
2. **Messages have two lines.** Line 1 says what happened in plain words. Line 2 says what to do. The technical detail goes after that, in a "Details" line, never first.
3. **One name per thing.** Use the glossary below. If the program calls one thing by two names, one of them goes.
4. **A word keeps one meaning.** "Slow start" is a choice of movement. It must not also mean a fade time. "Key" is a saved pose. It must not also mean a password.
5. **Say "PinglyMotion" or "it".** Not "the program" and not "the application".
6. **No blame, no alarm.** Write "could not be read", not "bad file" or "invalid". No exclamation marks.
7. **Units in words.** "seconds", "frames", not "s" or "fps", except in a narrow label where the unit is added in the tip.
8. **Second Life words stay.** Priority, hand pose, `.anim` and the joint names are Second Life's own words. Explain them once, in a tip.
9. **Tip: at most 12 words per sentence, 2 lines.** Dialog text may be longer but keeps one idea per sentence.

### Glossary: the words to use

| Use | Do not use | Meaning, in one line | Where the program disagrees today |
|---|---|---|---|
| key | key frame, keyframe | A saved pose at one moment. | G1 "Select a key frame on the timeline", G2 "Default Keyframe Type", parser errors |
| pose | position (for the whole figure) | How the figure stands at one moment. | none |
| joint | bone, node | A part that turns, such as an elbow. | "bone" only in technical details |
| turn | rotate, rotation | What you do to a joint. | G3 "X Rotation:" and others |
| frame | step | One step in time. | none |
| time marker | slider, playhead, scrubber | The marker you drag to pick a moment. | none |
| timeline | track view | The strip that shows your keys over time. | none |
| loop | repeat | The animation plays again and again. | G4 "Loop In:" and "Loop Out:" |
| easing choice | ease, curve, interpolation | How a move speeds up or slows down between two keys. | G5 "Ease In", "Ease Out" in Settings |
| slow start, slow finish | ease in, ease out | Two of the six easing choices. | G6 export dialog uses them for fade times |
| fade in, fade out | slow start, slow finish (for export times) | How long a Second Life animation takes to start and stop. | G6 |
| figure | avatar, model, mesh | The 3D person in the view. Say "avatar" only for Second Life or Aurora. | mixed |
| animation | motion, clip | All the movement in one file. | none |
| panel | dock, widget | A window part that can be moved or hidden. | none |
| tips | tooltips | Short help when the mouse rests on a control. | none |
| prop | object, attachment | A thing the figure holds or wears in the view. | none |
| mirror | flip | Swap left and right. | none |
| skeleton | rig | The joints of a figure. Only in import dialogs. | none |
| connection code | key (for the remote link) | The secret a program shows to prove who it is. | G7 "Aurora refused the key" |
| T-pose | (explain on first use) | Standing with the arms straight out to the sides. | docs only |
| the movement between two keys | interpolation | How the figure gets from one key to the next. | none |
| drag the hand and the arm follows | IK, inverse kinematics | Moving a hand or foot moves the joints above it. | none |
| the furthest this joint can turn | joint limit | The range a joint is allowed to turn in Second Life. | none |
| build extra keys | bake | Only if it must appear at all. | none |
| (never shown) | quaternion, Euler | Ways to store a turn. Never shown to the user. | none |

This is the one glossary. Other pages link here; the AI assistant guide keeps a short copy of the beginner rows ([ASSISTANT_GUIDE.md](ASSISTANT_GUIDE.md), section 6), because AI clients read it on its own. `tools/check_texts.py` has its own list of avoided words for the register in `docs/TEACHING.md`; change it too when a "Do not use" word is added here.

## 2. How to read the inventory

Columns: **ID** (for talking about a row; not a register ID, so `tools/check_texts.py` ignores them), **Where** (file, or menu), **Current wording**, **Purpose**, **Proposed**, **Change**. In **Change**, `no` means the current wording already follows the rules, `yes` means the proposal differs, `verify` means the proposal depends on what the control really does and the job that changes it must check the code first. A line break inside a text is written as `/`.

## 3. Inventory and proposed wording

### 3.1 Main window: menus

Where: `src/pinglymotionwindow.ui`.

| ID | Current wording | Purpose | Proposed | Change |
|---|---|---|---|---|
| A1 | File, Edit, Options, View, Help, Tools | Menu titles | no change | no |
| A2 | New / Open... / Add... | Start or open an animation | New / Open... / Add to this animation... (since UX phase 1a New is "New animation", as in the mockup) | yes, verify |
| A3 | Save / Save As... | Save the animation | Save / Save as... | yes |
| A4 | Load Props... / Save Props... | Props file | Open props... / Save props... | yes |
| A5 | Exit | Quit | no change | no |
| A6 | Import glTF animation... | Import | no change | no |
| A7 | Import BVH from another skeleton... | Import with matching of joints | no change. Add tip: "Fits a BVH made for another skeleton onto this figure." | tip is new |
| A8 | Import Second Life .anim... | Import | no change | no |
| A9 | Export as Second Life .anim... | Export for Second Life | Export for Second Life (.anim)... | yes |
| A10 | Export For Second Life | Old BVH export | Export BVH... Tip: "Makes a BVH file. For Second Life use Export for Second Life (.anim)." | yes, verify |
| A11 | Undo / Redo / Cut / Copy / Paste | Edit | no change | no |
| A12 | Mirror, tip "Mirror Animation or Track" | Swap left and right | Mirror. Tip: "Swaps left and right in the whole animation or in one joint." | yes, verify |
| A13 | Optimize BVH | Tools | Remove extra keys. Tip: "Removes keys that change nothing." | yes, verify |
| A14 | Joint Limits | Option | Stop joints at their furthest turn | yes, verify |
| A15 | Protect First Frame | Option | Lock the first frame. Tip: "Keeps frame 0 as it is, so the loop has no jump." | yes |
| A16 | Loop | Option | Loop playback | yes, verify |
| A17 | Skeleton | View option | Show skeleton | yes, verify |
| A18 | Timeline | View option | Show timeline | yes |
| A19 | Configure PinglyMotion... | Settings | Settings... | yes |
| A20 | Reset Camera, tip "Reset camera view to default position" | Reset the 3D view | Reset view. Tip: "Puts the camera back where it started." | yes |
| A21 | About... | Credits | no change | no |
| A22 | What's New... | Release notes | What's new... | yes |
| A23 | Dark theme | Option | no change; since UX phase 1a it is View > Light theme (T), the same setting turned round (docs/UX_DECISIONS.md, 6e) | no |
| A24 | Light... / tip "Change how the body is lit: brightness, shine, warmth and the shadow on the floor." | Lighting dialog | Lighting... Tip keeps its wording. | yes (label only) |
| A25 | Body: Ruth2, Max (tips "The open source Ruth2 avatar, with fingers and a face that follow the Bento bones.") | Pick the figure | Body: no change. In the tips write "Bento bones" as "the extra Second Life bones" on first use. | yes |
| A26 | Hide panels / "Hide all panels to see only the figure. Tab again brings them back." | Show only the figure | no change | no |
| A27 | Reset layout / "Put the panels back where they were at the first start." | Layout | no change | no |
| A28 | Allow remote control (AI and other programs) / "Lets programs on this computer pose and play the animation. Off every time PinglyMotion starts, unless started with --control." | Turn on the control link | no change to the label. Tip: "Lets programs on this computer pose and play the animation. It is off each time you start." | yes (tip) |
| A29 | Live link to Aurora / "Shows the pose on your avatar in the Aurora viewer while you work." | Turn on the live link | no change | no |
| A30 | Hide in lists / "Leave hidden joints out of the timeline and the part list. Off: they are listed dimmed." | Joint list option | Tip: "Leaves hidden joints out of the timeline and the joint list. Off: they show dimmed." | yes |

### 3.2 Main window: labels and panels

Where: `src/pinglymotionwindow.ui`, `src/pinglymotionwindow.cpp`, `src/jointlist.cpp`, `src/paneltitlebar.cpp`.

| ID | Current wording | Purpose | Proposed | Change |
|---|---|---|---|---|
| B1 | Animation: / Figure: female, male / Size: | Top row | no change | no |
| B2 | Rotation (group), X Rotation: / Y Rotation: / Z Rotation: | Turn the selected joint | Turn (group), Turn X: / Turn Y: / Turn Z: | yes |
| B3 | Alt / Ctrl / Shift next to the rotation fields | Old joint keys | Remove if still shown. The 18 November decision removed these keys. | verify |
| B4 | Position (group), X Position: etc. | Move the hip or a prop | no change | no |
| B5 | Scale (group), X Scale: etc. (props) | Size of a prop | Size (group), Size X: etc. | yes |
| B6 | Prop: / Attach to: / * not attached * | Prop panel | no change / no change / (not attached) | yes |
| B7 | Loop In: (10%) / Loop Out: (20%) | Loop points | Loop starts at: / Loop ends at: | yes. The percent hints go into the tip: "Where the loop starts, as a share of the animation." |
| B8 | Frame 0 of N | Where you are | no change | no |
| B9 | FPS: | Playback speed | Frames per second: | yes |
| B10 | Selected joint (panel title) / Joints (panel title) / Timeline (panel title) | Panel titles | no change | no |
| B11 | Body, Face, Hands, Other (joint groups) | Joint list | no change | no |
| B12 | Search joints / "Type part of a joint name, for example "elbow" or "left hand"." | Joint search | no change | no |
| B13 | Second Life name: %1 / Name in the animation file: %1 / Moves the whole avatar. | Joint list tips | Second Life name: %1 / Name in the file: %1 / Moves the whole figure. | yes |
| B14 | Close this panel (bring it back from the Options menu) / Put this panel back into the main window / Make this panel a window of its own | Panel buttons | no change | no |
| B15 | Start / Stop / Replay from start, tips "Play the animation from where the playhead is" / "Stop playing; the playhead stays on this frame" / "Go back to the first frame and play"; when a button cannot be clicked: "It is already playing. Click Stop to stop it." / "It is not playing. Click Start to play it." (`src/transportbar.cpp`) | Transport buttons and the reason a button cannot be clicked (charter P6) | no change | no |
| B16 | Pop-out button greyed out while the panels stand in one column: "The window is too small to float panels. Make it bigger or the text smaller." (`src/paneltitlebar.cpp`, wording from the clickable mockup) | Why a panel cannot float now (charter P6) | no change | no |
| B17 | Selected joint panel (UX phase 2b, `src/selectedjointpanel.h`): "Nothing to turn yet", "Pick a joint" with three steps and "Pick the right shoulder for me", the axis names "Tilt forward or back" / "Turn left or right" / "Lean to the side", "allowed range" / "no limits", "Near its limit: X (tilt forward or back)." with WARN-01 and "Turn limits off", "Warn near the joint's limits", Reset joint, Pull with keys, the drag choice, More options with Change: Turn / Move, and the greyed reasons | Turn one joint; replaces B2, B3 and B4 on screen (the old labels are hidden) | wording from the clickable mockup; the differences are listed in UX_DECISIONS.md, 6 | no |

### 3.3 Settings, About, update

Where: `src/usersettingsdialog.ui`, `src/aboutdialog.ui`, `src/updatenotificationdialog.ui`, `src/whatsnewdialog.ui`.

| ID | Current wording | Purpose | Proposed | Change |
|---|---|---|---|---|
| C1 | Configure PinglyMotion (title) | Settings | Settings | yes |
| C2 | Default Keyframe Type: Ease In / Ease Out | Default for new keys | Default for new keys: Slow start / Slow finish | yes, verify |
| C3 | OpenGL (group) / Use Fog / Floor Translucency: | Graphics | Graphics / Use fog / Floor see-through: | yes |
| C4 | &OK / &Cancel / &Apply | Buttons | no change | no |
| C5 | About PinglyMotion / Check for Updates / Check Now / Automatically Check for Updates | About box | About PinglyMotion / Updates / Check now / Check for updates automatically | yes |
| C6 | Update Checker / "An update is available!" | New version | Update / "A new version is available." | yes |
| C7 | What's New (title) | Release notes | What's new | yes |

### 3.4 Status, indicators and tips switch

Where: `src/controlindicator.cpp`, `src/easingtiles.cpp`.

| ID | Current wording | Purpose | Proposed | Change |
|---|---|---|---|---|
| D1 | Remote control off / Remote control on, no assistant connected / %1 connected / %1 failed / %1 s ago, %1 min ago, %1 h ago | Status bar indicator | no change | no |
| D2 | Remote control: request with a wrong key refused | Security status | Remote control: a request with a wrong code was refused | yes (G7) |
| D3 | Aurora off / Waiting for Aurora / Aurora live | Indicator | no change | no |
| D4 | Tips on / Tips off / "Help tips: short help when the mouse rests on a control. / Click to turn them on or off." | Tips switch | no change | no |
| D5 | Select a key frame on the timeline | Easing row with no key picked | Click a key on the timeline to pick its easing. | yes (G1) |
| D6 | Even / Slow start / Slow finish / Slow both ends / Overshoot, then settle / Hold, then snap, and the six hints | Easing tiles (approved, in the program) | no change | no |

### 3.5 File dialogs and messages

Where: `src/pinglymotionwindow.cpp`, `src/animation.cpp`, `src/bvh.cpp`.

| ID | Current wording | Purpose | Proposed | Change |
|---|---|---|---|---|
| E1 | Load Animation File / Select Animation File to Load / Select Animation File to Add / Open Animation File | Dialog titles | Open animation / Open an animation / Pick an animation to add / Open animation | yes |
| E2 | Save Animation File / Save Props / Load Props / Export for Second Life / Export as Second Life .anim | Dialog titles | Save animation / Save props / Open props / Export for Second Life / Export for Second Life (.anim) | yes |
| E3 | Animation file not found: %1 | Open fails | This file is not there any more. / Check the name, then try again. | yes |
| E4 | %1 could not be read: %2 | Open fails | This file could not be read. / %2 / Check that it is an animation file, then try again. | yes |
| E5 | This file comes from a newer version of PinglyMotion. / Some parts of it are not shown here. / Saving it will remove those parts. Save under another name to keep the original. | Newer file | no change | no |
| E6 | Your animation was saved, but the new file did not pass the check. / The old file %1 was left where it is. | Save check failed | no change | no |
| E7 | Your animation was saved. / The old file %1 could not be moved to the recycle bin. / It is still there. You can delete it yourself. | Old file | no change | no |
| E8 | Animation was exported for Second Life as: / %1 | Export done | Your animation was exported as: / %1 | yes |
| E9 | The file could not be written: %1 / Could not write %1: %2 | Write fails | This file could not be saved. / %2 / Check that the folder exists and has room, then try again. | yes |
| E10 | Saved as: / %1 | Save done | no change | no |
| E11 | Please note: / What could not be imported: | Header for warnings | Good to know: / What could not be imported: | yes (first only) |
| E12 | This file could not be opened. / %1 / Check that the file is still there, then try again. | Import fails | no change | no |
| E13 | This file is too big for an animation. / Pick a smaller file, then try again. | Import fails | no change | no |
| E14 | Line %1: %2 / This file could not be read. / %1 / Check that it is a BVH file, then try again. | BVH parse error | no change | no |
| E15 | The skeleton of the program could not be read. / Reinstall the program, then try again. | Broken install | PinglyMotion could not read its own skeleton file. / Install it again, then try again. | yes |
| E16 | The animation could not be opened. / Free some disk space, then try again. | Disk full | no change | no |
| E17 | Opened %1. | Import done | no change | no |
| E18 | File Exists / A file with the name "%1" does already exist. Do you want to overwrite it? | Overwrite | A file named "%1" is already there. Replace it? | yes |
| E19 | Unsaved Changes / Do you want to save the changes to "%1"? / %1 animations have unsaved changes. Do you want to save them? / If you don't save, your changes will be lost. / Don't Save | Close with changes | Unsaved changes / Save your changes to "%1"? / %1 animations have unsaved changes. Save them? / Without saving, your changes are lost. / Don't save | yes |
| E20 | Only .dingle, .bvh and .avm files can be opened. | Wrong file type | Open a .dingle or .bvh file. (.avm is an old format.) | yes |
| E21 | The file name must end in .dingle or .bvh. / Old .avm files are not saved any more. Save as .dingle instead. | Save as wrong type | no change | no |
| E22 | OpenGL 3.3 needed / "PinglyMotion needs a graphics card and driver with OpenGL 3.3 or newer" / The 3D view needs OpenGL 3.3, which this computer does not have. | No 3D | PinglyMotion needs a graphics card and driver that support OpenGL 3.3. / Update the graphics driver, then start again. | yes |
| E23 | Missing Limits File / Limits file not found at: %1 / Error reading limits file | Broken install | PinglyMotion could not read its file of joint limits. / Install it again, then try again. | yes |

### 3.6 Export dialog, retarget dialog, import choice

Where: `src/animexportdialog.cpp`, `src/animexport.cpp`, `src/retargetdialog.cpp`, `src/gltfimportdialog.cpp`, `src/animimportconvert.cpp`.

| ID | Current wording | Purpose | Proposed | Change |
|---|---|---|---|---|
| F1 | Export as Second Life .anim (title) | Dialog title | Export for Second Life (.anim) | yes |
| F2 | Priority (0 to 6): / When two animations move the same joint, / the higher number wins. | Priority | no change | no |
| F3 | Slow start: / How long the animation takes to fade in. | Fade in time | Fade in: / How long the animation takes to fade in. | yes (G6) |
| F4 | Slow finish: / How long the animation takes to fade out. | Fade out time | Fade out: / How long the animation takes to fade out. | yes (G6) |
| F5 | Play again and again / The loop points of the animation are used. | Loop | no change | no |
| F6 | Hands: / How the fingers look while it plays. / Fingers spread, Relaxed, Pointing, Fist, and the left/right variants, Salute, Typing, Peace sign, Flat hand | Hand pose | no change | no |
| F7 | Choose file... | Go on | Choose file... | no |
| F8 | Length: %1 seconds. Second Life accepts up to %2 seconds. | Length check | no change | no |
| F9 | There is nothing to export yet. / Open a file or make a pose, then try again. | Empty | no change | no |
| F10 | The animation has no length. / Add more frames, then try again. | No frames | no change | no |
| F11 | This animation is too long to export. / | Too long | This animation is too long to export. / Shorten it, or delete keys at the end. (as WARN-02 in the register) | yes |
| F12 | The file was not saved. / %1 / Check the animation, then try again. | Export fails | no change | no |
| F13 | The ease times were shortened to fit the animation. | Warning | The fade times were shortened to fit the animation. | yes (G6) |
| F14 | The priority was changed to %1 (the range is 0 to %2). | Warning | no change | no |
| F15 | The joint %1 was left out: Second Life does not accept it. | Warning | no change | no |
| F16 | Keys were removed to keep the file under %1 bytes. | Warning | Some keys were removed so the file stays under %1 bytes. | yes |
| F17 | %1 has its own skeleton. / The program matched its joints to the avatar's joints. | Retarget intro | %1 has its own skeleton. / PinglyMotion matched its joints to the figure's joints. | yes |
| F18 | Joint in your file / Joint on the avatar / What happens | Table headers | Joint in your file / Joint on the figure / What happens | yes |
| F19 | My file starts in a T-pose or A-pose / Tick this if the first frame is a standing pose with the arms out. | Option | My file starts with the arms out to the sides / Tick this if the first frame is a T-pose or A-pose (arms out to the sides). | yes |
| F20 | Remember my changes for files with this skeleton / The next file from the same skeleton gets your changes. | Option | no change | no |
| F21 | Use the automatic match / Undo all your changes in this list. | Button | no change | no |
| F22 | Matched / Left out / Changed by you / Left out by you / (leave out) | Status words | no change | no |
| F23 | %1 of %2 joints matched. %3 were left out. | Summary | no change | no |
| F24 | These avatar joints need a joint from your file: %1. / Pick one in the list, or use the automatic match. | Error | These joints on the figure need a joint from your file: %1. / Pick one in the list, or use the automatic match. | yes |
| F25 | This file holds more than one choice. / Pick the ones you want. | glTF choice | no change | no |
| F26 | These joints are not on the skeleton and were left out: %1. | Import note | no change | no |
| F27 | The holds that pin a joint to something were left out. | Import note | Joints pinned in place (constraints) were left out. | yes |
| F28 | Priority, slow start and finish, and hands are not kept. | Import note | Priority, fade times and hand poses are not kept. | yes (G6) |

### 3.7 Remote link to Aurora

Where: `src/auroralink.cpp`, `src/controlserver.cpp`.

| ID | Current wording | Purpose | Proposed | Change |
|---|---|---|---|---|
| H1 | Aurora's LiveMesh preview is not on. In Aurora: Build > LiveMesh > Enable preview. | Aurora step | no change | no |
| H2 | This Aurora version cannot show live poses. Use an Aurora build with live poses (Aurora-Viewer-Fork). | Aurora version | no change | no |
| H3 | Aurora's LiveMesh is on, but it cannot show live poses right now (%1). | Aurora state | no change | no |
| H4 | The program on the LiveMesh port cannot prove that it is Aurora. The link was closed. / did not prove | Safety | The program on the LiveMesh port could not show that it is Aurora. The link was closed. | yes |
| H5 | Aurora refused the key. Turn LiveMesh's preview off and on in Aurora, then try again. | Aurora state | Aurora refused the connection code. Turn LiveMesh's preview off and on in Aurora, then try again. | yes (G7) |
| H6 | The connection file %1 could not be written safely: %2 / its permissions could not be set (%1) / other users could read it | Control link | The remote control could not start safely: %2. Details: %1 | yes, verify |

### 3.8 File error details (patterns)

Where: `src/animfile.cpp` (25), `src/animimport.cpp` (29), `src/bvh.cpp` (16), `src/dingle.cpp` (78), `src/retarget.cpp` (72), `src/retargetmapping.cpp` (53), `src/gltfimport.cpp` (163), `src/propfile.cpp` (6). About 440 strings. They explain why a file could not be read. A beginner reads them only after a wrong file was picked.

What is wrong with them today: they use words from file formats (accessor, buffer view, channel, depth first order, fingerprint, generator, key frame property, the compressed offset) and start with the technical cause. Many have no line that says what to do.

Proposal, one pattern for all:

1. The dialog shows **two plain lines**: what happened and what to do. The wording is already in section 3.5 (E12 to E14) and it stays.
2. The technical text from these files goes into the dialog's **Details** part, collapsed by default, with the text unchanged. Programmers and people who report a problem still get it.
3. Where the Details text is also shown alone (a warning list after an import), words from the glossary replace file-format words:

| Today | Proposed |
|---|---|
| key frame, key frames | key, keys |
| rotation | turn |
| "Line %1: %2" | no change |
| "A motion value is too large." | "A number in the file is too large." |
| "The joint %1 had keys at the same time; only the last of each was kept." | no change |
| "The file says it has %1 frames, but has fewer motion values." | "The file says it has %1 frames, but not all are there. It may be cut off." |
| "The compressed data is broken (wrong start)." | "The file looks damaged. Try to export it again from the program that made it." |
| "Only glTF version 2 is supported." | "Only glTF version 2 files work here. Export again as glTF 2." |
| "This is not a glTF file (it has no asset version)." | "This is not a glTF file. Pick a .glb or .gltf file." |
| "The file uses compressed data (%1), which is not supported." | "This file uses compression (%1) that PinglyMotion cannot read. Export it again without compression." |
| "%1: the joints must be in depth first order, a parent before its children." | "The joints in this file are in the wrong order. The file may be damaged." |
| "The mapping has no skeleton fingerprint, so it cannot be saved for a skeleton." | "This match cannot be saved for later, because the skeleton cannot be told apart." |

The job that changes the texts takes the remaining details one file at a time and applies the same table. The check script `tools/check_texts.py` can be extended to read this table so it stays true.

### 3.9 Docs that teach

| ID | Where | Current wording (short) | Purpose | Proposed | Change |
|---|---|---|---|---|---|
| I1 | `docs/FIRST_ANIMATION.md` | "The figure holds one pose, a T-pose." | First page | "The figure holds one pose, a T-pose: arms straight out to the sides." | yes |
| I2 | `docs/FIRST_ANIMATION.md` | "Slow start: how long the animation takes to fade in." and "Slow finish ..." in step 7 | Export box | "Fade in" and "Fade out", with the dialog | yes (G6), same job as F3 and F4 |
| I3 | `docs/FIRST_ANIMATION.md` | "File > Export For Second Life still makes a .bvh file instead." | Menu names | Follow A9 and A10 | yes |
| I4 | `docs/FIRST_ANIMATION.md` | "Frame 0 is locked ... turn off Options > Protect First Frame" | Option | Follow A15 | yes |
| I5 | `docs/FIRST_ANIMATION.md` | "Options > Configure PinglyMotion" etc. | Menu names | Follow A19 and the rest of 3.1 | yes |
| I6 | `docs/FIRST_ANIMATION.md` | all other steps | First page | no change. It uses key, turn, loop and fade in plain words. | no |
| I7 | `README.md` quick start | not checked line by line | Entry | Check against the glossary in the texts job | verify |
| I8 | `docs/index.html` (website) | not checked line by line | Entry | Check against the glossary in the texts job | verify |

The AI assistant guide (`docs/ASSISTANT_GUIDE.md`) is for assistants, not for beginners. The glossary does not apply to it.

### 3.10 Texts that are planned but not in the program yet

The 45 register texts (`docs/TEACHING.md`) have no screen yet, except the six easing hints and the six wave notes (the notes line under the timeline, UX phase 3c). Three notes for the screen jobs:

- **Welcome card.** The owner asked what "Standing" means. Proposal to put in front of him: WELCOME-05 becomes "Pick a ready-made pose to start. You can change it later." This changes an approved text, so it needs his ok.
- **Control names in the register** ("Set key", "Play") are placeholders. The real key and play buttons have icons and no words. The tips TIP-01 and TIP-02 describe them correctly. The next-step texts (NEXT-03 to NEXT-06) say "Set a key" and "Press Play". The screen job should either add the words to the buttons or change the texts to "Click the key button" and "Click the play button". Update: since the transport buttons (UX step 1), the single play button is gone; there are buttons with the words Start, Stop and Replay from start. TIP-02 ("Press it again to stop") no longer fits and needs a new text for the Start button, which needs the owner's ok.
- **Register and glossary.** The register already follows the glossary: it says key, turn, slow start and slow finish. No change.

## 4. Open decisions for the owner

1. **Approve the glossary** (section 1). Most proposals follow from it.
2. **Sentence case for all menus** (rule 1). It changes about 20 labels.
3. **Fade in and fade out** instead of slow start and slow finish in the export dialog (G6). The Second Life name for these is "ease in" and "ease out". This is the only place where plain words and Second Life's own words differ in a way that confuses. The proposal uses plain words.
4. **Details part in error dialogs** (3.8). It is the only change in how the program looks, not just its words. It needs a small window job.

## 5. After the owner's ok

One job changes the texts, in small steps, each as its own pull request:

1. Menus and labels (3.1 to 3.3). **Built** (pull request "Texts step 1: approved wording for menus and labels"); the test `pinglymotion_menutexts_tests` (`tests/menutexts`) checks every new text exactly. Left as they are, after checking the code: A2 stays "Add..." (it opens a second animation next to this one, so "Add to this animation..." would not be true); A10 has no dots ("Export BVH"), because it saves at once without a dialog; B3 (the Alt, Ctrl and Shift labels next to the turn fields) waits for a check of what those keys still do; in B7 the percent hints stay next to the fields for now (moving them into the tip changes the layout); in C2 only the group title changed ("Default for new keys"), the two boxes keep "Ease In" and "Ease Out" until it is checked which of slow start and slow finish each one gives.
2. Dialogs and messages (3.5 to 3.7), with the tests that check the exact wording.
3. File error details (3.8).
4. The docs (3.9).

Visible text that matches a computed or typed answer can be checked by tests, so under the rules in `CLAUDE.md` these jobs merge with a test that asserts the exact new wording. The existing tests that name old wording change in the same pull request.

## 6. Regenerating the inventory

`python3 tools/extract_texts.py` prints one line per string with file and line, the kind (`tr`, `tip`, `msg`, `ui:<property>`) and the text. With `--markdown` it prints a table. Run it before the texts job and compare with sections 3.1 to 3.8: a string that is in the output and not on this page is new.
