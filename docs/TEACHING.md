# Teaching texts: rules and register

This page is the home of every text that teaches a beginner inside the program. It holds the goal, the places where the program teaches, the writing rules, and a register (a list) of all texts. It is step 1 of issue #54.

**All 45 texts in this register were reviewed by the project owner on 19 November 2026 and approved unchanged.** They are final wording. The six easing hints (`EASE-01` to `EASE-06`) are in the program (the tile row, #53 step 5); the wave notes (`WAVE-01` to `WAVE-06`) are stored in `examples/wave.dingle` and shown in the notes line under the timeline (UX phase 3c); the rest is not yet.

**Review note:** the owner, a beginner, asked what the welcome button "Standing" means; the label alone did not say it is a ready-made starting pose. The texts were approved as they are, but the welcome card should make clear that the three buttons are ready-made starting points.

Words used on this page:

- A **key** is a saved pose at one moment.
- A **tip** is a short help text that appears when the mouse rests on a control.
- The **hint bar** is the strip in the 3D view that shows the most useful controls.
- The **register** is the table in section 4. It lists every text with an ID.
- **`tr()`** is the way the program looks up a text, so that translations can be added later without changing code.

## 1. Goal and who it is for

The goal: a person who has never animated can make a good animation in the first hour. They should be able to pose the figure, set keys, play the result and save it. They should not need a course, a video or another program.

The texts are for that person. They are not for people who already know animation tools. Those people can ignore the texts, and a "Tips off" switch exists for them.

Why one list: texts are written once and used by many jobs. If each job invents its own wording, the voice drifts and technical words creep in. One list keeps the voice the same everywhere.

Related plans: the beginner plan in #14, easing choices in #53, research in #11, and `docs/POSING_UX.md` (not edited here).

## 2. The nine places where the program teaches

| # | Place | When it shows | Length | Built by |
|---|---|---|---|---|
| 1 | Welcome card (Standing, Sitting, Dance example, Skip) | First start, and from Help | 1 title, 3 button labels, 1 line | #14 beginner plan, job 9 |
| 2 | Tips on hover | Mouse rests on a control. On by default. A "Tips off" switch exists. | 2 short lines | #14 job 8 (the tips setting `tips_on` is merged) |
| 3 | Hint bar in the 3D view | Depends on what is under the mouse | 1 short line | #14 job 8 |
| 4 | "What to do next" line | Changes with what the user just did: nothing selected, joint selected, joint changed, no key yet, key set, animation longer than one frame. Text only, never a pop-up. | 1 line | #14 job 10 |
| 5 | Easing hints under the tiles | Mouse rests on one of the six choices | 1 line each | #53 step 5 |
| 6 | Notes in the example wave | On the keys of the example | 1 to 2 lines per key | #53 step 6, #14 jobs 4 and 5 |
| 7 | Names and descriptions of starter poses | In the starter list | name + 1 line | #14 jobs 4 and 5 |
| 8 | Warnings and errors (near the furthest a joint can turn, export too long, nothing moved) | When it happens | what happened + what to do | many jobs, #10 dialog |
| 9 | A short page "Your first animation" | Help menu and the website | one screen | docs |

## 3. Writing rules

These rules are a draft until the project owner approves them.

1. **Say what to do, not what is wrong.** Write "Drag the ring to turn the arm". Do not write "Rotation is not possible here".
2. **Short and direct.** One idea per sentence. At most 12 words per sentence in tips and hints. Use "you" or the command form ("Drag", "Press", "Pick").
3. **Plain words.** The technical word may follow in brackets, but only where it helps.
4. **No pop-ups for teaching.** Text stays in the window. It never interrupts the user.
5. **A way out for every warning.** Say how to fix the problem.
6. **English now.** All texts go through `tr()`, so translations can follow without code changes. This was decided in `docs/POSING_UX.md`.
7. **Never claim what is not true.** If a feature is experimental, say so. Do not describe a button or a behaviour that is not in the program yet.

### Plain-word table

The words to use and the words to avoid in texts the user sees are in the one glossary: [TEACHING_TEXTS.md, "Glossary: the words to use"](TEACHING_TEXTS.md#glossary-the-words-to-use). It applies to the program's texts; this page and other documents may use the technical words when they explain things to a programmer. `tools/check_texts.py` checks the register below against the avoided words.

## 4. The register

Each row is one text. Columns:

- **ID**: a short fixed name. The letters tell the place (EASE, WAVE, WELCOME, and so on). The number never changes, even if the wording does.
- **Place**: one of the nine places in section 2.
- **Trigger**: when the text shows.
- **Text**: the wording. A line break inside a text is written as `<br>`. An empty cell means no draft yet.
- **Status**: `draft`, `reviewed by the owner`, or `in the program`.
- **Used by**: the issue or pull request that builds the screen.

### Place 5: easing hints

The six choices come from #53. The hint shows under the tiles when the mouse rests on one. Each hint says when the choice fits.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EASE-01 | 5 Easing hint | Mouse rests on "Even" | Same speed all the way. Use it for machines or a steady spin. | in the program (pull request 134, 19 Nov 2026) | #53 step 5 |
| EASE-02 | 5 Easing hint | Mouse rests on "Slow start" | Starts gently, then speeds up. Pick it for lifting an arm. | in the program (pull request 134, 19 Nov 2026) | #53 step 5 |
| EASE-03 | 5 Easing hint | Mouse rests on "Slow finish" | Moves at speed, then settles into the new pose. Good for putting a hand down. | in the program (pull request 134, 19 Nov 2026) | #53 step 5 |
| EASE-04 | 5 Easing hint | Mouse rests on "Slow both ends" | Starts and stops softly. Good for most arm and head moves. | in the program (pull request 134, 19 Nov 2026) | #53 step 5 |
| EASE-05 | 5 Easing hint | Mouse rests on "Overshoot, then settle" | Goes a little past the pose, then comes back. Good for a lively wave. | in the program (pull request 134, 19 Nov 2026) | #53 step 5 |
| EASE-06 | 5 Easing hint | Mouse rests on "Hold, then snap" | Stays in the old pose, then jumps to the new one. Good for a blink. | in the program (pull request 134, 19 Nov 2026) | #53 step 5 |

### Place 6: notes in the example wave

The wave is a short loop with five keys, one saved pose each. Each pose change uses a different choice from place 5, and the note says which. The last pose matches the first, so the loop has no jump. The example is our own: `examples/wave.dingle`, made by `tools/wave_example.py` (#53 step 6). The notes are stored in the file under `extensions` and listed in [FIRST_ANIMATION.md](FIRST_ANIMATION.md); the program shows them in the notes line under the timeline (UX phase 3c, [UX_DECISIONS.md, 4](UX_DECISIONS.md#4-teaching-notes-like-wave-01-to-wave-06)).

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| WAVE-01 | 6 Wave note | Key 1: arm hangs down | A key is a saved pose. This one has the arm down.<br>The wave starts and ends here, so it loops. | reviewed by the owner (19 Nov 2026); in `examples/wave.dingle` (pull request 162) | #53 step 6 |
| WAVE-02 | 6 Wave note | Key 2: arm lifted, change uses "Slow both ends" | The arm lifts here.<br>Slow both ends makes it start and stop softly. | reviewed by the owner (19 Nov 2026); in `examples/wave.dingle` (pull request 162) | #53 step 6 |
| WAVE-03 | 6 Wave note | Key 3: hand out, change uses "Overshoot, then settle" | The hand swings out, goes a little past, then settles.<br>That is Overshoot, then settle. | reviewed by the owner (19 Nov 2026); in `examples/wave.dingle` (pull request 162) | #53 step 6 |
| WAVE-04 | 6 Wave note | Key 4: hand back, change uses "Even" | The hand swings back at one steady speed.<br>That is Even. It looks stiff, so try Slow both ends. | reviewed by the owner (19 Nov 2026); in `examples/wave.dingle` (pull request 162) | #53 step 6 |
| WAVE-05 | 6 Wave note | Key 5: arm down again, change uses "Slow finish" | The arm slows down as it lands.<br>It matches the first key, so the loop has no jump. | reviewed by the owner (19 Nov 2026); in `examples/wave.dingle` (pull request 162) | #53 step 6 |
| WAVE-06 | 6 Wave note | After the last key: a small task | Your turn: make the arm stop more softly. | reviewed by the owner (19 Nov 2026); in `examples/wave.dingle` (pull request 162) | #53 step 6 |

### Top bar, Edit menu and empty state (UX phase 1b)

Built in pull request 371 (22 Nov 2026). The texts are in `src/menuspec.h` and `src/emptystate.h`; `tests/emptystate` and `tests/menutexts` check them letter for letter.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| EMPTY-01 | 4 Empty state | Title when no animation is open | No animation open | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EMPTY-02 | 4 Empty state | The one line under the title | Pick a pose to start. You can change it later. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EMPTY-03 | 4 Empty state | Line under the choice Standing | A calm standing pose. A good start for most moves. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EMPTY-04 | 4 Empty state | Line under the choice Sitting | A calm seated pose. Good for a quiet scene. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EMPTY-05 | 4 Empty state | Line under the choice Dance example | A short, simple dance. Change it to learn. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EMPTY-06 | 4 Empty state | Reason: a control needs an animation | Open an animation first. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-01 | 3 Edit menu | Undo is greyed: nothing to undo | Nothing to undo yet. | in the program (pull request 446, job B1; the mockup's text) | job B1 Undo and Redo |
| EDIT-02 | 3 Edit menu | Redo is greyed: nothing to redo | Nothing to redo. Redo works after Undo. | in the program (pull request 446, job B1; the mockup's text) | job B1 Undo and Redo |
| EDIT-03 | 3 Edit menu | Paste mirrored is greyed | Paste mirrored is not built yet. Use Tools > Mirror to swap left and right. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-04 | 3 Edit menu | Set key: no joint is picked | Pick a joint first. Click it on the figure or in the list. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-05 | 3 Edit menu | Set key: the animation is playing | Stop playing first. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-06 | 3 Edit menu | Set key: the first frame is locked | The first frame is locked. Turn off Lock the first frame to set a key. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-07 | 4 What to do next | After Undo | Undone. | in the program (pull request 446, job B1; the mockup's text) | job B1 Undo and Redo |
| EDIT-08 | 4 What to do next | After Redo | Redone. | in the program (pull request 446, job B1; the mockup's text) | job B1 Undo and Redo |
| EDIT-09 | 4 What to do next | Undo or Redo while a drag or a pull runs | Finish the change you are making first. | in the program (pull request 446, job B1) | job B1 Undo and Redo |
| TOPBAR-01 | 2 Tip on hover | Mouse rests on Script Lab in the top bar | Script Lab: test RLV and LSL scripts | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| TOPBAR-02 | 2 Tip on hover | Mouse rests on Setup in the top bar | Beginner or experienced, mouse and keys habits, theme, text size and hints. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| TOPBAR-03 | 2 Tip on hover | Mouse rests on the theme button in the dark theme | Switch to the light theme | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| TOPBAR-04 | 2 Tip on hover | Mouse rests on the theme button in the light theme | Switch to the dark theme | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |

### Script Lab editor (lane 1, job F1)

Built in pull request 428 (23 Nov 2026). The texts are in `src/scriptlab/labeditor.cpp`; `tests/scriptlabpanel/tst_labeditor.cpp` checks them letter for letter. The numbers and names in LABED-10, LABED-16 and LABED-17 are examples: the program puts in the real time, counts and file name.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LABED-01 | 4 Empty state | Script Lab editor: no script open, heading | No script open | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-02 | 4 Empty state | Script Lab editor: no script open, the line | Start from an example, or save a .lsl file in the folder. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-03 | 4 Empty state | Script Lab editor: the button that opens an example as a new file | New from example | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-04 | 4 Empty state | Script Lab editor: the RLV card title, while hints are on | What is an RLV line? | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-05 | 4 Empty state | Script Lab editor: the RLV card text, while hints are on | A script sends an RLV line to the viewer. It starts with @, like @detach=n. The viewer then blocks or allows one thing. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-06 | 8 Warning | The shown file changed on disk while there are unsaved edits | The file changed outside. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-07 | 8 Warning | Answer: keep the typed text | Keep my changes | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-08 | 8 Warning | Answer: show the file as it is now | Load the new one | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-09 | 8 Warning | Answer: show which lines differ | Show the difference | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-10 | 4 What to do next | After Save, when the run is done (time and counts from the run) | Saved and run 15:30. 0 errors. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-11 | 4 What to do next | The text box gets the focus for the first time | Tab adds an indent. To move on, press Esc, then Tab. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-12 | 4 What to do next | Esc pressed in the text box | Press Tab to move on, or keep typing. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-13 | 4 What to do next | Answer Keep my changes | Your changes are kept. Save to write them to the file. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-14 | 4 What to do next | Answer Load the new one | Loaded the new file. Ctrl+Z brings your changes back. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-15 | 4 What to do next | The file changed on disk, nothing was typed | Loaded the changed file. Ctrl+Z brings the old text back. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-16 | 8 Warning | Go to line: the number is past the end (numbers from the script) | There is no line 99. The script has 8 lines. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-17 | 4 What to do next | New from example picked (the file name) | New script hello.lsl from an example. Press Ctrl+S to save and run it. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-18 | 8 Warning | New from example while there are unsaved edits | Save or undo your changes first. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-19 | 2 Tip on hover | Save is greyed: nothing changed | Nothing to save. Change the script first. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-20 | 2 Tip on hover | Mouse rests on the text box | Type to change the script. Ctrl+S saves and runs it. | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |
| LABED-21 | 2 Tip on hover | The part's count while there are unsaved edits (the mockup's words) | ● unsaved changes | in the program (pull request 428, 23 Nov 2026) | lane 1 job F1 |

### Script Lab editor: suggestions and the language choice (lane 1, jobs F2 and F3)

Built in pull request 431 (23 Nov 2026). The texts are in `src/scriptlab/labeditor.cpp`, `src/scriptlab/labsuggest.cpp` and `src/setupdialog.h`; `tests/scriptlabpanel/tst_labsuggest.cpp` and `tests/setupdialog/tst_setupdialog.cpp` check them letter for letter. The help sentences for single names ("Sends a chat line only the owner sees. RLV lines use it.") are not in this table: our own lines are in `src/scriptlab/lslhelp_plain.inc`, checked with the same rules (at most 12 words a sentence, no avoided word) by `tests/test_gen_lsl_help.py`; every other name shows the first sentence of Linden Lab's own description.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LABSG-01 | 4 What to do next | Script Lab editor: the line under the suggestion list, while hints are on | Press Down to pick, then Enter or Tab. Esc closes the list. | in the program (pull request 431, 23 Nov 2026) | lane 1 job F2 |
| LABSG-02 | 2 Tip on hover | Help in the list for a name made in the script (a function) | A function made in this script. | in the program (pull request 431, 23 Nov 2026) | lane 1 job F2 |
| LABSG-03 | 2 Tip on hover | Help in the list for a name made in the script (a variable) | A variable made in this script. | in the program (pull request 431, 23 Nov 2026) | lane 1 job F2 |
| LABSG-04 | 1 Welcome card (Setup) | Setup, Script editor: the choice for experienced users | Show suggestions only when I press Ctrl+Space | in the program (pull request 431, 23 Nov 2026) | lane 1 job F2 |
| LABSG-05 | 2 Tip on hover | Setup: the tip of that choice | Off: suggestions come up while you type. Enter never picks one by itself. | in the program (pull request 431, 23 Nov 2026) | lane 1 job F2 |
| LABSG-06 | 1 Welcome card (Setup) | Setup, Script editor: the Lua switch (off at start) | Lua (trial): show the language choice in the editor | in the program (pull request 431, 23 Nov 2026) | lane 1 job F3 |
| LABSG-07 | 2 Tip on hover | Setup: the tip of the Lua switch | Lua is not checked or run yet. Scripts stay LSL. | in the program (pull request 431, 23 Nov 2026) | lane 1 job F3 |
| LABSG-08 | 2 Tip on hover | The language box in the editor's tool row, only while the Lua switch is on | The script language. Lua is a trial: no checks or suggestions yet. | in the program (pull request 431, 23 Nov 2026) | lane 1 job F3 |
| LABSG-09 | 4 What to do next | SLua was chosen in the language box | Lua is a trial. No Lua suggestions or checks yet. | in the program (pull request 431, 23 Nov 2026) | lane 1 job F3 |

### Script Lab editor: RLV suggestions, help and warnings (lane 1, jobs R3 and R4)

Built in pull request 433 (23 Nov 2026). The texts are in `src/scriptlab/rlvsuggest.cpp`, `src/scriptlab/labeditor.cpp`, `src/scriptlab/rlvbehaviours.cpp` (the warnings) and `src/scriptlab/scriptlabpanel.cpp`; `tests/scriptlabpanel/tst_rlvsuggest.cpp` and `tests/tst_rlvexamples.cpp` check them letter for letter. The command names, values and examples in the rows are examples: the program puts in the real ones. The plain sentence for each command ("Locks this object on you, so it cannot be taken off.") is not in this table: the 223 lines are in `src/scriptlab/rlvhelp_plain.inc`, checked with the same rules (at most 12 words a sentence, no avoided word) by `tests/tst_rlvexamples.cpp`.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LABRLV-01 | 2 Tip on hover | Script Lab editor: under the help of an RLV command in the suggestion list (the example from the table) | Ctrl+Enter puts in the example: @detach=n | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-02 | 2 Tip on hover | After a command name in the list while the viewer rule is RLV | (RLVa only) | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-03 | 2 Tip on hover | After a command name in the list while the viewer rule is RLVa | (RLV only) | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-04 | 2 Tip on hover | Help for the form n after "=" | Turns the rule on. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-05 | 2 Tip on hover | Help for the form y after "=" | Lifts the rule again. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-06 | 2 Tip on hover | Help for the form force after "=" | Does it once, right now. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-07 | 2 Tip on hover | Help for the channel after "=" (a question) | A channel number. The answer comes back on it. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-08 | 2 Tip on hover | Help for a key option after ":" | A key (UUID): 36 letters and digits with dashes. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-09 | 2 Tip on hover | Help for a number option after ":" | A number, such as 1.5. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-10 | 2 Tip on hover | Help for an option with no fixed type (the sample comes from the example) | An example value. Change it to what you need. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-11 | 2 Tip on hover | Help for clear in the list | Lifts every rule this object holds. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-12 | 8 Warning | A form the command does not know (command and forms from the table) | @detach=x: x is not valid. Use n, y or force. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-13 | 8 Warning | The = part is missing | @detach: the = part is missing. End it with =n, =y or =force. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-14 | 8 Warning | An unknown command close to a known one | @detatch=n: detatch is not an RLV command. Did you mean detach? | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-15 | 8 Warning | A form the command does not take | @sendchat=force: sendchat does not take =force. Use n or y. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-16 | 8 Warning | A needed option is missing (the example from the table) | @redirchat=n: redirchat needs an option after ":". Example: @redirchat:2222=n | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-17 | 8 Warning | An option on a command that takes none | @sendchat:x=n: sendchat takes no option. Remove ":x". | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-18 | 8 Warning | A command the chosen viewer does not know | @setcam=n: setcam is RLVa only. RLV viewers do not know it. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-19 | 2 Tip on hover | The tip of Errors and warnings | Click a line, or press Enter, to show it in the script | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-20 | 2 Tip on hover | The tip of the event log | What the script did in the last run, in simulated seconds. Click an error or RLV line to show it in the script. | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |
| LABRLV-21 | 2 Tip on hover | The size grip in the corner of the floating Script Lab window | Drag to change the size of the Script Lab window | in the program (pull request 433, 23 Nov 2026) | lane 1 job R3 |

### Script Lab for beginners (job U3)

Built in pull request 443 (23 Nov 2026). The texts are in `src/scriptlab/scriptlabpanel.cpp` and `src/scriptlab/labbeginner.cpp`; `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` checks them letter for letter. In LABNEW-06, LABNEW-20 and LABNEW-21 the program puts in the real path, title and numbers. The word tips (LABNEW-35 to LABNEW-42) show one at a time with the word as the title (Prim, Root prim, Notecard, Sandbox, Viewer rules, RLV, RLVa, N held). The button "Make example project" and the step texts it shows come from the example guides (EXCOLOUR rows in [EXAMPLES.md](EXAMPLES.md)).

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LABNEW-01 | 4 Empty state | Script Lab, Live scripts: no folder watched, the line | Script Lab shows your scripts from a folder on your PC. Choose a folder to see them here. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-02 | 4 Empty state | The main button: copy the first example and open it | Make example project | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-03 | 2 Tip on hover | Make example project | Copies a working example to your folder and shows it in 3D. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-04 | 4 Empty state | Button: make an empty folder under Documents, PinglyMotion, Scripts | New folder | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-05 | 2 Tip on hover | New folder (the program adds the folder's path) | Makes an empty folder for your scripts and watches it. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-06 | 4 What to do next | New folder: the question for the name (the program puts in the path) | Name of the new folder. It goes in %1. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-07 | 4 What to do next | New folder: the name it suggests | My object | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-08 | 8 Warning | New folder: no name | Type a name for the folder first. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-09 | 8 Warning | New folder: name over 40 characters | The name is too long. Use at most 40 characters. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-10 | 8 Warning | New folder: a sign that is not allowed | Use only letters, digits, spaces, - and _ in the name. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-11 | 8 Warning | New folder: CON, NUL, COM1 and the like | Windows keeps this name for itself. Pick another name. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-12 | 8 Warning | New folder: the folder is there already (nothing is overwritten) | A folder with this name is already there. Pick another name. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-13 | 8 Warning | New folder or example: the path is over 200 characters | The folder path would be too long. Use a shorter name. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-14 | 8 Warning | New folder or example: the folder could not be made | Could not make the folder. Check that Documents can be written. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-15 | 8 Warning | Make example project: the examples are not next to the program | The example objects are not next to the program. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-16 | 8 Warning | Make example project: example.json cannot be read | The example could not be read. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-17 | 4 What to do next | Object part: the line over the tree | This tree is your object. Each folder in it is one prim. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-18 | 4 What to do next | Object part: the switch that links the tree and the 3D view | Follow selection in 3D view | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-19 | 2 Tip on hover | Follow selection in 3D view | Click a piece here to light up its prop in 3D view. Click a prop and its row is picked here. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-20 | 4 What to do next | Steps card: the title (the example's title) | Steps: First colour | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-21 | 4 What to do next | Steps card: progress (the numbers from the steps) | 3 of 5 done | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-22 | 4 What to do next | Steps card: the heading over the result, shown first | You should see | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-23 | 4 What to do next | Steps card: the heading over what to change next | Then try | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-24 | 2 Tip on hover | Steps card: the close button | Close the steps. Help, Script Lab steps shows them again. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-25 | 4 What to do next | Step button: show Props in the Joints panel | Show Props | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-26 | 4 What to do next | Step button: put the example's props in the scene | Add the props | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-27 | 4 What to do next | Step button: show the Live scripts page | Show Live scripts | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-28 | 4 What to do next | Step button: watch the example's folder (made first when needed) | Watch the folder | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-29 | 4 What to do next | Step button: turn on Show effects in 3D view | Show effects | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-30 | 8 Warning | A step the program cannot do yet (Touch, Say, dialog buttons): line and tip | This step does not work in the program yet. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-31 | 4 What to do next | Help menu item | Script Lab steps | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-32 | 2 Tip on hover | Help, Script Lab steps | Show the steps of the example project again. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-33 | 2 Tip on hover | A step's tick (for screen readers too) | Done | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-34 | 2 Tip on hover | A step's empty tick | Not done | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-35 | 4 What to do next | Word tip: Prim (same words as EXCOLOUR-03) | One part of an object. Each folder is one prim. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-36 | 4 What to do next | Word tip: Root prim | The main part of the object. It is the folder Root. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-37 | 4 What to do next | Word tip: Notecard | A text file that a script can read. Here it is a .txt file. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-38 | 4 What to do next | Word tip: Sandbox | A safe place to test. Nothing here reaches Second Life. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-39 | 4 What to do next | Word tip: Viewer rules (also the tip of the words Viewer rules) | Which kind of RLV the test viewer follows. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-40 | 4 What to do next | Word tip: RLV | Rules a script sends to your viewer, like no detaching. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-41 | 4 What to do next | Word tip: RLVa | A newer kind of RLV, used in Firestorm and other viewers. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-42 | 4 What to do next | Word tip: N held (the chip by Viewer rules) | How many RLV rules hold the avatar right now. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-43 | 2 Tip on hover | A word tip's close button | Close this tip. It does not come back. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-44 | 4 What to do next | How to lay out the folder: heading over the rules | Rules | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-45 | 8 Warning | How to lay out the folder: heading over the warnings | Warnings | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-46 | 2 Tip on hover | Show effects in 3D view, for screen readers, when on | On: the 3D view shows what the scripts do. | in the program (pull request 443, 23 Nov 2026) | job U3 |
| LABNEW-47 | 2 Tip on hover | Show effects in 3D view, for screen readers, when off | Off. | in the program (pull request 443, 23 Nov 2026) | job U3 |

### Script Lab: props from prims (job A1)

Built in pull request 447 (24 Nov 2026). The texts are in `src/scriptlab/scriptlabpanel.cpp`; `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` checks them letter for letter. The program puts in the prop's name (%1) and the reason (%2).

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LABPRIM-01 | 4 What to do next | Event log: a click on a prop in the 3D view, before the touch it sends | You clicked %1. | in the program (pull request 447, 24 Nov 2026) | job A1 |
| LABPRIM-02 | 8 Warning | Status line: the touch from a click could not be sent | Error: the touch on %1 was not sent. %2 | in the program (pull request 447, 24 Nov 2026) | job A1 |

### Script Lab: record and replay (job A3)

Built in pull request 450 (24 Nov 2026). The texts are in `src/scriptlab/scriptlabpanel.cpp`, `src/scriptlab/labwatcher.cpp` and `src/scriptlab/labrecipe.cpp`; `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` and `tests/tst_labrecipe.cpp` check them letter for letter. The facts and changes put in part names, button texts and colours from the run.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LABREC-01 | 1 Label | Live page: the button that starts recording, under Show effects in 3D view | Record what I do | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-02 | 1 Label | The same button while recording | Stop recording | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-03 | 2 Tip on hover | Record what I do | Records your clicks on props, menu buttons and RLV lines. After every save, the program repeats them and says what changed. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-04 | 3 Empty state | The line under the button: no recipe in the folder yet | No recipe yet. Press Record what I do, then click your props. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-05 | 4 What to do next | The line while recording; the program puts in the number of steps | Recording. Click props and menu buttons. Steps so far: %1. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-06 | 4 What to do next | The line while recording and the effects are off | Turn on Show effects in 3D view to click props. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-07 | 4 What to do next | The line after stopping without a step | Recording stopped. No steps were recorded, so the recipe stays as it was. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-08 | 4 What to do next | The line after a replay with every step done | Replayed your %n step(s). | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-09 | 8 Warning | The line after a replay where steps were skipped | Replayed %1 of %2 steps. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-10 | 4 What to do next | The line after the first replay of new steps | This is the first time, so there is nothing to compare. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-11 | 1 Label | Heading over the facts after the first replay | Now: | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-12 | 1 Label | Heading over the changes after a replay | What changed since last time: | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-13 | 4 What to do next | The line after a replay that changed nothing | Nothing changed since last time. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-14 | 8 Warning | The line when the object did not run, in the danger colour | The object did not run, so your steps were not replayed. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-15 | 4 What to do next | The line when a recipe is there but not replayed yet | The recipe is replayed after the next save. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-16 | 1 Label | Expert mode: the folded part with the recipe file | Recipe | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-17 | 1 Label | Expert mode: the button in the Recipe part | Replay now | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-18 | 2 Tip on hover | Replay now | Repeats the recipe's steps now, without a save. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-19 | 4 What to do next | Recipe part: the file line; the program puts in the two file names | File: %1 in the object folder. %2 is kept next to it. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-20 | 3 Empty state | Recipe part: no recipe file yet; the program puts in the file name | No recipe file yet. Recording makes %1 in the object folder. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-21 | 8 Warning | Replay now greyed while recording, as its tip | Stop recording first. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-22 | 8 Warning | Replay now greyed without a recipe, as its tip | Record what you do first. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-23 | 8 Warning | Error line: Replay now without a recipe file | There is no recipe yet. Record what you do first. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-24 | 8 Warning | Error line: a recipe with only notes | The recipe has no steps. Record what you do first. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-25 | 8 Warning | Error line: a recipe the program cannot read; the program puts in the line and the reason | The recipe has a mistake. %1 | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-26 | 2 Tip on hover | Recipe part: the recipe's text | The recipe file. You may edit it in your own editor. | in the program (pull request 450, 24 Nov 2026) | job A3 |
| LABREC-27 | 4 What to do next | One change since last time; the program puts in the part, the old state and the new state | Before: %1 was %2. Now: it is %3. | in the program (pull request 450, 24 Nov 2026) | job A3 |

### Script Lab: the channel map (job A4)

Built in pull request 453 (24 Nov 2026). The texts are in `src/scriptlab/scriptlabpanel.cpp`, `src/scriptlab/channelmapview.cpp`, `src/scriptlab/labchannelmap.cpp` and `src/scriptlab/labbeginner.cpp`; `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` and `tests/tst_labchannelmap.cpp` check them letter for letter; the chain tags (LABMAP-21 and 22) are in `src/scriptlab/labchains.cpp` and `tests/tst_labchains.cpp` checks them. The arrows' own words ("say on channel 7", "link message 5", "menu on channel -77") are built from the run and are not in this list.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LABMAP-01 | 1 Label | Live page: the folding part after the event log, while a folder is watched as one object | Who talks to whom | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-02 | 4 What to do next | Channel map: the line over the map | Each box is a part. Each arrow is a message, with its channel. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-03 | 2 Tip on hover | Channel map: the map itself | Click a box to pick it in the tree. Click an arrow to see the line that sent it. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-04 | 3 Empty state | Channel map: the run sent no message | Nothing talked yet. Save a script or click a prop. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-05 | 8 Warning | Channel map: an arrow on a channel nobody listens on; the program puts in the channel | Nobody listens on channel %1. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-06 | 8 Warning | Channel map: a menu answer nobody hears; the program puts in the channel | Nobody listens on channel %1, so the menu answer is lost. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-07 | 8 Warning | Channel map: llRegionSayTo to a key that does not hear it; the program puts in the channel | Nobody with that key listens on channel %1. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-08 | 8 Warning | Channel map: a link message no script gets; the program puts in its number | No script gets link message %1. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-09 | 8 Warning | Channel map: an RLV line the viewer refused; the program puts in the reason | Your viewer refused this line: %1 | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-10 | 4 What to do next | Word tip by the channel map: Channel | A number that scripts talk on. Only listeners on it hear. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-11 | 4 What to do next | Word tip by the channel map: Listener | A script that waits for messages on one channel. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-12 | 8 Warning | Channel map: under the map, one arrow reached nobody | 1 arrow reached nobody. It has a cross and says why. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-13 | 8 Warning | Channel map: under the map, several arrows reached nobody; the program puts in the number | %1 arrows reached nobody. Each has a cross and says why. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-14 | 8 Warning | Channel map: under the map, more arrows than it shows; the program puts in the number | The map shows the first %1 arrows only. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-15 | 2 Tip on hover | Channel map: the box You | You, the avatar. You see menus and open chat, and you click. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-16 | 2 Tip on hover | Channel map: the box Your viewer (RLV) | Your viewer. It gets the RLV lines a script sends. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-17 | 2 Tip on hover | Channel map: the box Web server | A web server outside Second Life. The sandbox never goes online. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-18 | 2 Tip on hover | Channel map: a box that hears messages and passes them on (its name ends in "(hub)") | This part listens and passes messages on, like a server. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-19 | 2 Tip on hover | Channel map: an arrow from a script | Click to see the script line that sent it. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-20 | 2 Tip on hover | Channel map: an arrow from outside the scripts (a click, a menu answer, a viewer answer) | It came from outside the scripts. | in the program (pull request 453, 24 Nov 2026) | job A4 |
| LABMAP-21 | 1 Label | 3D view: the tag in the middle of a chain line while the object holds @detach | Locked | in the program (pull request 455, 24 Nov 2026) | job A4 |
| LABMAP-22 | 1 Label | 3D view: the tag in the middle of a chain line while the object is free | Free | in the program (pull request 455, 24 Nov 2026) | job A4 |

### Script Lab: the player (job B3)

Built in pull request 454 (24 Nov 2026). The texts are in `src/scriptlab/labplayer.cpp` (and the moment "Start." in `src/scriptlab/labrecipe.cpp`); `tests/scriptlabplayer/tst_scriptlabplayer.cpp` checks them letter for letter. The rows put in part names, actions and facts from the replay. The Start, Stop and Replay from start labels are the timeline's (`TransportBar`).

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LABPLAY-01 | 1 Label | Live page: the player's title, under the recipe line | Player: watch your steps again | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-02 | 1 Label | Player: the back button of the bar | Step back | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-03 | 1 Label | Player: the forward button of the bar | Step forward | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-04 | 1 Label | Player: the button that jumps to where the replay first differed | Go to first difference | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-05 | 1 Label | Player: the button that shows the live run in the 3D view again | Show now | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-06 | 1 Label | Player: heading over the rows | True at this moment: | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-07 | 1 Label | Player, Expert mode: heading over the log | Log up to this moment: | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-08 | 1 Label | Player: the first moment in the list | Start | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-09 | 1 Label | Player: a step in the list; the program puts in the number and the step in plain words | Step %1: %2 | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-10 | 8 Warning | Player: added to a step in the list that could not be done |  Not done. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-11 | 4 What to do next | Player: added to the moment where the replay first differed |  First difference. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-12 | 4 What to do next | Player: where it is, on the start; the program puts in the time | Start, at %1. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-13 | 4 What to do next | Player: where it is, on a step; the program puts in the step, the number of steps, the time and the step | Step %1 of %2, at %3: %4 | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-14 | 8 Warning | Player: added to the line when the step could not be done |  It could not be done. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-15 | 3 Empty state | Player: no replay yet | No replay yet. Press Record what I do, click your props, then stop recording. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-16 | 4 What to do next | Player: a time; the program puts in the seconds | %1 s | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-17 | 8 Warning | Player row, in the danger colour: a part that holds @detach; the program puts in the name | Locked: %1 | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-18 | 4 What to do next | Player row, in the success colour: a part locked at another moment, not now | Free: %1 | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-19 | 8 Warning | Player row, in the danger colour: a blocked action | Blocked: %1 | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-20 | 4 What to do next | Player row, in the success colour: an action blocked at another moment, not now | Allowed: %1 | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-21 | 4 What to do next | Player row: added to a row that changed at this step (also bold) | %1 (changed at this step) | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-22 | 3 Empty state | Player rows: nothing true at this moment | Nothing locked, coloured or said yet. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-23 | 4 What to do next | Player: the line while a moment is shown | The props in the 3D view show this moment. Press Show now to see now again. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-24 | 4 What to do next | Player: the line while now is shown | The props in the 3D view show now. Pick a moment to see it there. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-25 | 2 Tip on hover | Player: the list of moments | Pick a moment to see what was true then. The arrow keys move one step. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-26 | 2 Tip on hover | Player: Start | Play the steps one by one, one each second | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-27 | 2 Tip on hover | Player: Stop | Stop playing; the player stays on this step | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-28 | 2 Tip on hover | Player: Replay from start | Go back to the start and play the steps | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-29 | 2 Tip on hover | Player: Step back | Go to the step before | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-30 | 2 Tip on hover | Player: Step forward | Go to the next step | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-31 | 8 Warning | Player: Step back greyed, as its tip | This is the start. There is no step before it. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-32 | 8 Warning | Player: Step forward greyed, as its tip | This is the last step. Click Replay from start to see it again. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-33 | 8 Warning | Player: every button greyed without a replay, as its tip | No replay yet. Record what you do first. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-34 | 2 Tip on hover | Player: Go to first difference; the program puts in the step | Go to step %1, where this replay first differed | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-35 | 2 Tip on hover | Player: Go to first difference when the start already differed | Go to the start: it already differed from the replay before | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-36 | 8 Warning | Player: Go to first difference greyed after a first replay | This is the first replay, so there is nothing to compare with. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-37 | 8 Warning | Player: Go to first difference greyed when nothing differed | Every step gave the same as the replay before. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-38 | 2 Tip on hover | Player: Show now | Show the props as they are now, not at this moment | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-39 | 8 Warning | Player: Show now greyed, as its tip | The 3D view already shows now. | in the program (pull request 454, 24 Nov 2026) | job B3 |
| LABPLAY-40 | 4 What to do next | Player, Expert mode: the log line before a step; the program puts in the number and the step's line | --- Step %1: %2 --- | in the program (pull request 454, 24 Nov 2026) | job B3 |

### Library panel and two figures (drawn in the mockup, 23 Nov 2026)

Drafts for the library panel (roadmap item 6, issue 19) and for two figures in one scene (Expert mode only), drawn in `docs/mockup/index.html` and described in [UX_DECISIONS.md](UX_DECISIONS.md), sections 6l and 6m. Nothing is built yet; the project owner looks at the mockup first. `<clip name>` and `<file name>` stand for the real name. The mockup's own helper texts (for example "Mockup: no library folder") are not program texts and are not listed.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| LIB-01 | Library panel | Panel title, View > Library, top bar button | Library | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-02 | Library panel | The line on top while hints are on | Ready-made moves to start from. Pick one, then press Use this. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-03 | Library panel | The search field (its name and grey text) | Search the library | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-04 | 2 Tip on hover | Mouse rests on the search field | Type words like walk, wave or sit. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-05 | Library panel | Label over the kinds of move | Kind of move | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-06 | Library panel | The kinds, each with its number of clips | All, Walk, Dance, Sit, Stand, Gesture, Idle, Wave | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-07 | Library panel | The count over the list (all clips, or a search) | 106 clips<br>12 of 106 clips | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-08 | Library panel | Empty state: the search finds nothing (title) | No clip found | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-09 | Library panel | Empty state: the search finds nothing (line) | Try another word, or show all clips. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-10 | Library panel | Empty state: the button | Show all clips | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-11 | Library panel | Empty state: no library folder (title) | No library found | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-12 | Library panel | Empty state: no library folder (line) | The library folder next to PinglyMotion is missing. Install PinglyMotion again to get it back. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-13 | Library panel | The preview button, stopped and playing | Play preview<br>Stop preview | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-14 | Library panel | The labels of the facts under the preview | Length<br>Skeleton<br>From<br>Licence | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-15 | Library panel | The skeleton of every clip | Second Life, 19 joints | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-16 | Library panel | Licence of a CMU clip | No restrictions on use. No formal licence text. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-17 | Library panel | Licence of a Mesh2Motion clip | CC0 1.0: free to use for anything. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-18 | Library panel | The main button (Enter in the list) | Use this | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-19 | 2 Tip on hover | Mouse rests on Use this | Opens this move as your animation. Save it under your own name. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-20 | 6 Reason | Use this is greyed: the clip is open now | This move is open now. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-21 | Library panel | Chip on the clip that is open now | Open now | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-22 | Library panel | Use this with unsaved changes: title, line, buttons | Open &lt;clip name&gt;?<br>Your animation has changes that are not saved.<br>Cancel<br>Open without saving<br>Save, then open | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-23 | 4 Status line | A clip was opened from the library | Opened &lt;clip name&gt; from the library. Save it under your own name. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-24 | 2 Tip on hover | Mouse rests on Library in the top bar | Library: ready-made moves to start from | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-25 | 4 Status line | The panel opens or closes | Library open. It shares the right side with Selected joint: switch with the tabs.<br>Library closed. View > Library opens it again. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| LIB-26 | Library panel | Second button, Expert mode only, and its tip | Add as a second figure<br>Puts this move on a second figure in the same scene. | in the program | src/figures.h, two figures (pull request 439) |
| LIB-27 | 4 Status line | Show all clips was pressed | Showing all 106 clips. | in the program (pull request 437, 23 Nov 2026) | Roadmap item 6, issue 19 (pull request 429) |
| FIG-01 | File menu | Expert mode only; the item ends with three dots | Add a second figure | in the program | src/figures.h, two figures (pull request 439) |
| FIG-02 | 6 Reason | Add a second figure is greyed | The scene has two figures. Remove one first. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-03 | Add a second figure window | Title and the line under it | Add a second figure<br>The second figure plays next to the first, in the same scene. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-04 | Add a second figure window | First choice (name and line) | From the library<br>Pick a ready-made move, then press Add as a second figure. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-05 | Add a second figure window | Second choice; the name ends with three dots | From a file<br>A .dingle, BVH or .avm file. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-06 | 3D view | Heading of the Figures box (two figures only) | Figures | in the program | src/figures.h, two figures (pull request 439) |
| FIG-07 | 2 Tip on hover | Mouse rests on a figure in the Figures box or in the 3D view | This figure is active. Its joints and keys show.<br>Click to make this figure active. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-08 | 4 Status line | A second figure was added | Added figure 2: &lt;file name&gt;. Click a figure to make it active. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-09 | 4 Status line | The other figure was made active | Figure 2 is active: &lt;file name&gt;. Its joints and keys show now. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-10 | 6 Reason | A figure is clicked in Beginner mode | Switching figures is in Expert mode. View > Expert mode turns it on. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-11 | Timeline | The row over each figure's joints | Active<br>Make active<br>Remove | in the program | src/figures.h, two figures (pull request 439) |
| FIG-12 | 2 Tip on hover | Mouse rests on Remove | Its file is not changed. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-13 | Remove figure window | Remove with unsaved changes: title, line, buttons | Remove figure 2?<br>&lt;file name&gt; has changes that are not saved.<br>Cancel<br>Remove without saving<br>Save, then remove | in the program | src/figures.h, two figures (pull request 439) |
| FIG-14 | 4 Status line | A figure was removed (the second line only when figure 1 went) | Figure 2 removed from the scene. Its file is not changed.<br>&lt;file name&gt; is figure 1 now. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-15 | Selected joint panel | Chip beside the key count (two figures only) | Figure 2 | in the program | src/figures.h, two figures (pull request 439) |
| FIG-16 | Top bar | After the file name (two figures only) | figure 2 of 2 | in the program | src/figures.h, two figures (pull request 439) |
| FIG-17 | 4 Status line | From the library was chosen in the Add window | Pick a move, then press Add as a second figure. | in the program | src/figures.h, two figures (pull request 439) |
| FIG-18 | 2 Tip on hover | Mouse rests on a greyed row of the other figure in the timeline | Click to make figure 2 active and pick this joint. | in the program | src/figures.h, two figures (pull request 439) |

### Mockup 2 (proposal, drawn in the mockup, 24 Nov 2026)

Drafts for everything drawn in Mockup 2 in `docs/mockup/index.html` (the project owner, 24 Nov 2026, 11:48: «lage en ny mockup med alt vi har planlagt nå»), described in [UX_DECISIONS.md](UX_DECISIONS.md), section 6t. **PROPOSAL, not approved until the project owner says so**: nothing here is built because it is drawn. The mockup keeps these texts in its `M2T` table; `tests/test_site_mockup_m2.py` checks that both say the same. `%1` and `%2` stand for what the program puts in. Texts Mockup 2 reuses word for word from rows above (for example LABNEW, LABREC, LABMAP and LIB) and from the example guides in [EXAMPLES.md](EXAMPLES.md) are not listed again. The mockup's own helper texts (the PROPOSAL mark, Questions, Script Lab place, Approved mockup, the Built, Being built, Planned and Proposal marks and the notes) are not program texts and are not listed.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| SIDE-01 | 2 Tip on hover | A side tab (Joints, Selected joint, Script Lab, Library) | Shows this panel on this side. One panel at a time, so nothing overlaps. | draft | Mockup 2 (pull request 456), proposal |
| SIDE-02 | 4 What to do next | Status line after a side tab is picked; the program puts in the panel and left or right | %1 shows on the %2 side now. | draft | Mockup 2 (pull request 456), proposal |
| SIDE-03 | 3 Empty state | The side area while the picked tab's panel floats | %1 floats in its own window. Its dock-back button puts it back here. | draft | Mockup 2 (pull request 456), proposal |
| SIDE-04 | Side tabs | The name of a tab strip, for screen readers | Panels on this side | draft | Mockup 2 (pull request 456), proposal |
| SIDE-05 | Side tabs | The words put into SIDE-02 | left<br>right | draft | Mockup 2 (pull request 456), proposal |
| SIDE-06 | 4 What to do next | Status line after the top bar button of the panel that shows is pressed; the program puts in both panels | %1 shows again. %2 waits in its tab. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-01 | Script Lab | The line beside Viewer rules; the program puts in RLV or RLVa | Now: %1 rules. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-02 | 2 Tip on hover | The ? button in the Script Lab title (its name) | Help for Script Lab | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-03 | 2 Tip on hover | The ? button in the Script Lab title | Explanations, pictures and the examples, in a bigger view. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-04 | Script Lab | Page tab | Player | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-05 | Script Lab | Page tab | Improve a setup | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-06 | Script Lab | Page tab | HUD | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-07 | 2 Tip on hover | A folding part's header on the Live scripts page | Opening one part folds the others, so the page stays short. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-08 | 2 Tip on hover | Page tab Who talks to whom | Arrows between the parts: who sends what to whom. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-09 | 2 Tip on hover | Page tab Player | Step back and forth through your recorded steps. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-10 | 2 Tip on hover | Page tab Improve a setup | Check a setup you made before and see what can be better. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-11 | 2 Tip on hover | Page tab HUD | Try your HUD on screen, as in Second Life. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-12 | Script Lab | Improve a setup page: heading | Improve a finished setup | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-13 | Script Lab | Improve a setup page: the line under the heading | Bring in a setup you made before. Then see what can be better. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-14 | Script Lab | Improve a setup page: the main button | Import a setup | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-15 | Script Lab | Improve a setup page: step 2 button | Record today's behaviour | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-16 | Script Lab | Improve a setup page: step 3 button | Show what can be better | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-17 | Script Lab | Improve a setup page: the three steps | 1. Import the folder.<br>2. Record what it does today.<br>3. See what can be better. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-18 | 4 What to do next | Improve a setup page: after the import; the program puts in the name and the numbers | %1 imported: %2 prims, %3 scripts. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-19 | 4 What to do next | Improve a setup page: after recording; the program puts in the number | Today's behaviour is kept: %1 steps. Every later save is checked against it. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-20 | Script Lab | Improve a setup page: heading over the findings | What can be better | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-21 | 8 Warning | Improve a setup page: the lint finding; the program puts in the number | Warnings in the scripts: %1. Click one to see its line. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-22 | Script Lab | Improve a setup page: heading over the memory table | Memory per script | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-23 | Script Lab | Improve a setup page: the memory table's columns and its caption | Script<br>Memory used<br>Second Life gives each script 64 KB. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-24 | 8 Warning | Improve a setup page: the listener finding; the program puts in the number | Listeners open: %1. Close the ones you do not need. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-25 | 8 Warning | Improve a setup page: the timer finding; the program puts in the number | Timers that never stop: %1. Stop a timer when it has nothing to do. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-26 | 4 What to do next | Improve a setup page: the last line | Change one thing, then save. Your recording runs again and says what changed. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-27 | 6 Reason | Show what can be better is greyed, as its tip | Record today's behaviour first, so you can see that nothing breaks. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-28 | 6 Reason | Record today's behaviour is greyed, as its tip | Import a setup first. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-29 | Script Lab | HUD page: heading | HUD: buttons on your screen | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-30 | Script Lab | HUD page: the line under the heading | In Second Life a HUD sits on your screen. Here it shows in a corner of the 3D view. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-31 | Script Lab | HUD page: the on and off button | Show the HUD on screen | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-32 | 4 What to do next | HUD page: the line while the HUD shows | Click the HUD to touch it, as in Second Life. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-33 | 4 What to do next | HUD page: the last touch; the program puts in the prim | Touched: %1. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-34 | Script Lab | HUD page: caption | How the HUD looks is still made in Second Life. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-35 | 3 Empty state | HUD page: no HUD in the watched folder | Watch a folder with a HUD first. The Gear set has one. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-36 | 3D view | The title of the HUD in the corner of the 3D view | HUD (top right) | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-37 | 4 What to do next | Status line after Make example project | Example project made: First colour. The box turns red. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-38 | 4 What to do next | Status line after the Gear set example is opened | Watching Gear_set. Click the flat box beside the figure. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-39 | Script Lab | Steps card: the button after the last step of First colour | Next example: Gear set | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-40 | Script Lab | Player row: an open menu; the program puts in its message and the part | Menu open: %1 (%2) | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-41 | Script Lab | Player row: what a part said last; the program puts in the part and the line | %1 said: %2 | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-42 | Script Lab | Player row: a part's colour; the program puts in the part and the colour | %1 is %2. | draft | Mockup 2 (pull request 456), proposal |
| LABTWO-43 | 4 What to do next | The box under the restriction list after Allowed is chosen | Ran from panel. Allowed now. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-01 | 3D view | The edit card in the 3D view; the program puts in the prop's name | Edit prop: %1 | draft | Mockup 2 (pull request 456), proposal |
| PROPED-02 | 3D view | The edit card: the three choices | Move<br>Turn<br>Size | draft | Mockup 2 (pull request 456), proposal |
| PROPED-03 | 3 Hint bar | The edit card: the line under the choices | Ctrl turns it, Ctrl+Shift changes its size. This works as in Second Life. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-04 | 3D view | The label next to the pointer while an arrow is dragged | Moving %1 along %2. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-05 | 3D view | The label next to the pointer while a ring is dragged | Turning %1 around %2. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-06 | 3D view | The label next to the pointer while a small square is dragged | Changing the size of %1 along %2. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-07 | 4 What to do next | Status line after a drag on a prop; the program puts in the prop | %1 moved. Ctrl+Z undoes it.<br>%1 turned. Ctrl+Z undoes it.<br>%1 changed size. Ctrl+Z undoes it. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-08 | 4 What to do next | Status line after Props is chosen in the Joints panel | Props: a click picks a prop to edit. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-09 | 4 What to do next | Status line after Avatar is chosen in the Joints panel | Avatar: a click on a prop touches it while effects show. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-10 | 3 Empty state | Joints panel, Props: no props in the scene | No props yet. Add one with a shape button. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-11 | Joints panel | Props: the shape buttons | Box<br>Ring<br>Cone<br>Ball | draft | Mockup 2 (pull request 456), proposal |
| PROPED-12 | 2 Tip on hover | A shape button; the program puts in the shape | Add a %1 prop | draft | Mockup 2 (pull request 456), proposal |
| PROPED-13 | Selected joint panel | The heading while a prop is picked | Selected prop | draft | Mockup 2 (pull request 456), proposal |
| PROPED-14 | Selected joint panel | The heading over the place of a prop | Place | draft | Mockup 2 (pull request 456), proposal |
| PROPED-15 | Selected joint panel | The heading over the size of a prop | Size | draft | Mockup 2 (pull request 456), proposal |
| PROPED-16 | Selected joint panel | The size fields of a prop | Width<br>Height<br>Depth | draft | Mockup 2 (pull request 456), proposal |
| PROPED-17 | Selected joint panel | The folded part with the shape fields of a prop | Shape like Second Life | draft | Mockup 2 (pull request 456), proposal |
| PROPED-18 | Selected joint panel | The shape fields of a prop | Cut begin<br>Cut end<br>Hollow<br>Twist | draft | Mockup 2 (pull request 456), proposal |
| PROPED-19 | 3 Empty state | Selected joint panel while Props is chosen and no prop is picked | Pick a prop in the 3D view or in the list. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-20 | 3 Hint bar | The hint bar while Move is on | Drag an arrow to move the prop. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-21 | 3 Hint bar | The hint bar while Turn is on (or Ctrl is held) | Drag a ring to turn the prop. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-22 | 3 Hint bar | The hint bar while Size is on (or Ctrl+Shift is held) | Drag a small square to change the size. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-23 | Selected joint panel | The label over the four shapes of a prop | Shape | draft | Mockup 2 (pull request 456), proposal |
| PROPED-24 | 4 What to do next | Status line after Esc during a drag on a prop | Change cancelled. Nothing changed. | draft | Mockup 2 (pull request 456), proposal |
| PROPED-25 | 3D view | The second line of the label next to the pointer while a prop is dragged | Release: keep it · Esc: cancel | draft | Mockup 2 (pull request 456), proposal |
| PROPED-26 | 4 What to do next | Status line after a shape button; the program puts in the new prop's name | %1 added. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-01 | Help view | The title of the help view, and the ? button in the top bar | Help | draft | Mockup 2 (pull request 456), proposal |
| HELPV-02 | Help view | The title when the ? button of Script Lab opened it | Script Lab help | draft | Mockup 2 (pull request 456), proposal |
| HELPV-03 | Help view | The topics on the left | Start here<br>How to lay out a folder<br>Words<br>Hints | draft | Mockup 2 (pull request 456), proposal |
| HELPV-04 | Help view | A topic for each example; the program puts in its title | Example: %1 | draft | Mockup 2 (pull request 456), proposal |
| HELPV-05 | Help view | The button by each step | Show me | draft | Mockup 2 (pull request 456), proposal |
| HELPV-06 | 2 Tip on hover | The Show me button | Shows where this is, then does the step. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-07 | Help view | The small badge on the control Show me points at | Here | draft | Mockup 2 (pull request 456), proposal |
| HELPV-08 | 4 What to do next | Status line after Show me; the program puts in the control's name | Here is %1. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-09 | Help view | Start here: the text | Script Lab tests Second Life scripts safely on your PC.<br>Make the example project first. It shows a result at once.<br>Then follow the steps of an example. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-10 | Help view | Start here: the three steps | Make the example project.<br>Turn on Show effects in 3D view.<br>Click a prop in the 3D view. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-11 | Help view | How to lay out a folder: the first line | One folder is one object. Each folder inside it is one prim. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-12 | Help view | How to lay out a folder: the rules | Put scripts (.lsl) and notecards (.txt) in their prim's folder.<br>Give every script and notecard its own name.<br>Name the main prim's folder Root.<br>Copy scripts out of Second Life by hand. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-13 | Help view | How to lay out a folder: the warnings | A script that cannot be checked shows its line.<br>The final test is always in Second Life. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-14 | Help view | How to lay out a folder: caption of the picture | The picture shows the Gear set folder. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-15 | Help view | Words: the first line | The words Script Lab uses, in plain words. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-16 | Help view | Hints: the text | Hints are small cards next to the part they talk about.<br>The cross closes a hint. The next one waits until you ask. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-17 | Help view | Hints: the button | Show the next hint | draft | Mockup 2 (pull request 456), proposal |
| HELPV-18 | Help view | The headings of an example topic | What it is<br>Steps<br>The pieces<br>New words | draft | Mockup 2 (pull request 456), proposal |
| HELPV-19 | Help view | An example topic: the main button | Open this example | draft | Mockup 2 (pull request 456), proposal |
| HELPV-20 | 4 What to do next | Status line after the cross or Esc on a hint card | Hint closed. Help shows the next one when you want it. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-21 | 2 Tip on hover | The ? button in the top bar | Explanations, pictures and examples. F1 opens it too. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-22 | Help view | An example topic: caption of the picture | The picture shows the example with its props. | draft | Mockup 2 (pull request 456), proposal |
| HELPV-23 | Help view | The heading over the example topics | Examples | draft | Mockup 2 (pull request 456), proposal |

### UX small fixes (24 Nov 2026)

Built in pull request 458 (the project owner, 24 Nov 2026, 11:29-11:42: «Alt bør inn som en»). The texts are in `src/guidancespec.h`, `src/scriptlab/scriptlabpanel.cpp` and `src/scriptlab/labhelpview.cpp`. The help view's topics follow Mockup 2's help view (HELPV rows above, a proposal) where the program does the same thing, and say what the program really does where it differs (no pictures and no Show me buttons yet; the next hint comes back with Help > Show hints off and on, as Help has no "Show the next hint" item). The example guide steps were checked against the program and changed in [EXAMPLES.md](EXAMPLES.md): "press" Show effects in 3D view (it is a button with a tick, not a tick box), "open Send input" first (it is folded at first), "Live scripts is its first tab", and the relay's line written out, with where its channel number is.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| UXFIX-01 | 4 What to do next | Status line after the close button or Esc on a hint card | Hint closed. Turn Help > Show hints off and on for the next one. | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-02 | 2 Tip on hover | The ? button in the Script Lab panel's top row: its name, the first line of its tip | Help for Script Lab | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-03 | 2 Tip on hover | The ? button: the second line of its tip | Explanations, the words and every example, in a bigger view. | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-04 | 2 Tip on hover | RLV or RLVa, for screen readers, before the line on switching | Chosen.<br>Not chosen. | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-05 | Help view | The window title | Script Lab help | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-06 | Help view | The topic list, for screen readers | Help topics | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-07 | Help view | The topics on the left, then the heading over the examples | Start here<br>How to lay out a folder<br>Words<br>Hints<br>Examples | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-08 | Help view | A topic for each example; the program puts in its title | Example: %1 | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-09 | Help view | Start here: the text | Script Lab tests Second Life scripts safely on your PC.<br>Make the example project first. It shows a result at once.<br>Then follow the steps of an example. | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-10 | Help view | Start here: the three steps, then the last line | Press Make example project on the Live scripts tab.<br>Press Show effects in 3D view, so it shows a tick.<br>Click a prop in the 3D view.<br>Help, Script Lab steps shows the steps of the example again. | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-11 | Help view | How to lay out a folder: the headings (Rules and Warnings as LABNEW-44 and LABNEW-45; the old folding part's six lines and example folder, word for word) | Rules<br>Warnings<br>Example | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-12 | Help view | Words: the first line, then every word tip (LABNEW-35 to LABNEW-42, LABMAP) | The words Script Lab uses, in plain words. | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-13 | Help view | Hints: the text | Hints are small cards next to the part they talk about.<br>The cross closes a hint. Turn Help > Show hints off and on for the next one.<br>A closed word tip in Script Lab does not come back. All the words are under Words here. | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-14 | Help view | The headings of an example topic, over its guide texts from example.json | What it is<br>Steps<br>The pieces<br>New words<br>You should see<br>Then try | in the program (pull request 458, 24 Nov 2026) | UX small fixes |
| UXFIX-15 | Help view | The button at the bottom | Close | in the program (pull request 458, 24 Nov 2026) | UX small fixes |

### AI notice (job K, 23 Nov 2026)

What the AI Assistant window will say about what a connected AI can see and do, below its "How it's kept safe" list (`src/aiconnectdialog.cpp`; keep `tools/mcp/README.txt` in step). Every line was checked against the code first; the proof for each is in [SECURITY_PRIVACY.md](SECURITY_PRIVACY.md), section 2 (for AI-08, section 1). Not built yet: the project owner reads the texts first. "It is off each time PinglyMotion starts" is already in the program's safety list, so it is not repeated here.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| AI-01 | AI Assistant window | Section title under "How it's kept safe" | What an AI can see and do | draft (23 Nov 2026) | Job K, security bundle (pull request 432) |
| AI-02 | AI Assistant window | First line of the notice | When it is on, an AI app on this PC can help.<br>It can read and change the animation that is open. | draft (23 Nov 2026) | Job K, security bundle (pull request 432) |
| AI-03 | AI Assistant window | Second line of the notice | It can take pictures of the 3D view.<br>It can save files in the PinglyMotion folder. | draft (23 Nov 2026) | Job K, security bundle (pull request 432) |
| AI-04 | AI Assistant window | Third line of the notice (Script Lab Live) | It can read scripts in a folder it names.<br>It never changes them. | draft (23 Nov 2026) | Job K, security bundle (pull request 432) |
| AI-05 | AI Assistant window | Fourth line of the notice | Your AI app may send what it sees to its company.<br>That company's privacy rules apply. | draft (23 Nov 2026) | Job K, security bundle (pull request 432) |
| AI-06 | AI Assistant window | Fifth line of the notice | PinglyMotion itself sends nothing to any AI company. | draft (23 Nov 2026) | Job K, security bundle (pull request 432) |
| AI-07 | AI Assistant window | Sixth line of the notice | The status bar shows which AI is connected.<br>The name comes from the AI app itself. | draft (23 Nov 2026) | Job K, security bundle (pull request 432) |
| AI-08 | AI Assistant window | Last line of the notice | PinglyMotion only goes online to look for updates.<br>Turn that off in Help, About. | draft (23 Nov 2026) | Job K, security bundle (pull request 432) |

### Places 1 to 4, 7 to 9

These texts were first drafts for the places that had no text. The owner reviewed them on 19 November 2026 (see "Review order" below). Control names such as "Set key" and "Play" are placeholders: the job that builds the screen checks them against the real labels before the status changes. Tips and hints get more rows when the tips and hint jobs start.

| ID | Place | Trigger | Text | Status | Used by |
|---|---|---|---|---|---|
| WELCOME-01 | 1 Welcome card | First start: title | Make your first animation | reviewed by the owner (19 Nov 2026) | #14 job 9 |
| WELCOME-02 | 1 Welcome card | Button label: Standing | Standing | reviewed by the owner (19 Nov 2026) | #14 job 9 |
| WELCOME-03 | 1 Welcome card | Button label: Sitting | Sitting | reviewed by the owner (19 Nov 2026) | #14 job 9 |
| WELCOME-04 | 1 Welcome card | Button label: Dance example | Dance example | reviewed by the owner (19 Nov 2026) | #14 job 9 |
| WELCOME-05 | 1 Welcome card | The one line under the title | Pick a pose to start. You can change it later. | reviewed by the owner (19 Nov 2026) | #14 job 9 |
| WELCOME-06 | 1 Welcome card | Link that closes the card (Skip) | Skip | reviewed by the owner (19 Nov 2026) | #14 job 9 |
| TIP-01 | 2 Tip on hover | Mouse rests on the button that sets a key | Saves the pose you see now as a key.<br>A key is a saved pose. | reviewed by the owner (19 Nov 2026) | #14 job 8 |
| TIP-02 | 2 Tip on hover | Mouse rests on the play button | Plays your animation from the start.<br>Press it again to stop. | reviewed by the owner (19 Nov 2026) | #14 job 8 |
| TIP-03 | 2 Tip on hover | Mouse rests on the time marker | Drag it to pick a moment.<br>Pose the figure there, then set a key. | reviewed by the owner (19 Nov 2026) | #14 job 8 |
| TIP-04 | 2 Tip on hover | Mouse rests on the save button | Saves your animation to a file.<br>You can open it again later. | reviewed by the owner (19 Nov 2026) | #14 job 8 |
| HINT-01 | 3 Hint bar | Mouse is over the figure, no joint under it | Click a joint to pick it. | reviewed by the owner (19 Nov 2026) | #14 job 8 |
| HINT-02 | 3 Hint bar | Mouse is over a joint | Click to pick this joint. | reviewed by the owner (19 Nov 2026) | #14 job 8 |
| HINT-03 | 3 Hint bar | Mouse is over the turning ring of a picked joint | Drag the ring to turn the joint. | reviewed by the owner (19 Nov 2026) | #14 job 8 |
| HINT-04 | 3 Hint bar | Mouse is over empty space | Drag to look around the figure. | reviewed by the owner (19 Nov 2026) | #14 job 8 |
| NEXT-01 | 4 What to do next | Nothing selected | Click a joint on the figure to pick it. | reviewed by the owner (19 Nov 2026) | #14 job 10 |
| NEXT-02 | 4 What to do next | A joint is selected | Drag the ring to turn the joint. | reviewed by the owner (19 Nov 2026) | #14 job 10 |
| NEXT-03 | 4 What to do next | A joint was changed | Like the pose? Set a key to save it. | reviewed by the owner (19 Nov 2026) | #14 job 10 |
| NEXT-04 | 4 What to do next | No key set yet | Set a key to save your first pose. | reviewed by the owner (19 Nov 2026) | #14 job 10 |
| NEXT-05 | 4 What to do next | A key was just set | Move the time marker, pose again, and set a second key. | reviewed by the owner (19 Nov 2026) | #14 job 10 |
| NEXT-06 | 4 What to do next | The animation is longer than one frame | Press Play to watch your animation. | reviewed by the owner (19 Nov 2026) | #14 job 10 |
| POSE-01 | 7 Starter pose | Starter list: Standing (name and one line) | Standing<br>Arms down, feet together. A calm start for most moves. | reviewed by the owner (19 Nov 2026) | #14 jobs 4 and 5 |
| POSE-02 | 7 Starter pose | Starter list: Sitting (name and one line) | Sitting<br>Knees bent, back straight. Good for a quiet scene. | reviewed by the owner (19 Nov 2026) | #14 jobs 4 and 5 |
| POSE-03 | 7 Starter pose | Starter list: Dance example (name and one line) | Dance example<br>A short loop with a note on each key. Change it to learn. | reviewed by the owner (19 Nov 2026) | #14 jobs 4 and 5 |
| WARN-01 | 8 Warning | A joint is near the furthest it can turn | This joint is almost at the furthest it can turn.<br>Turn it back a little, or pick another joint. | in the program | #14 job 6 |
| WARN-02 | 8 Warning | The export is too long | This animation is too long to export.<br>Shorten it, or delete keys at the end. | reviewed by the owner (19 Nov 2026) | #10 |
| WARN-03 | 8 Warning | Nothing was moved | Nothing moved between your keys.<br>Change a pose at one of them, then try again. | reviewed by the owner (19 Nov 2026) | #10 |
| PAGE-01 | 9 First animation page | Page title | Your first animation | reviewed by the owner (19 Nov 2026) | step 6 of #54 |
| PAGE-02 | 9 First animation page | Step 1 | Pick a pose on the welcome card. | reviewed by the owner (19 Nov 2026) | step 6 of #54 |
| PAGE-03 | 9 First animation page | Step 2 | Click a joint, then drag its ring to turn it. | reviewed by the owner (19 Nov 2026) | step 6 of #54 |
| PAGE-04 | 9 First animation page | Step 3 | Set a key to save the pose. | reviewed by the owner (19 Nov 2026) | step 6 of #54 |
| PAGE-05 | 9 First animation page | Step 4 | Move the time marker, pose again, and set another key. | reviewed by the owner (19 Nov 2026) | step 6 of #54 |
| PAGE-06 | 9 First animation page | Step 5 | Press Play. Pick an easing choice to change how the move feels. | reviewed by the owner (19 Nov 2026) | step 6 of #54 |
| PAGE-07 | 9 First animation page | Step 6 | Save your animation. | reviewed by the owner (19 Nov 2026) | step 6 of #54 |

### Review order

The owner reviews one place at a time, in the order a beginner meets them. For each place, the question is: "Do I understand this without help?"

1. Place 1, the welcome card (6 texts).
2. Place 4, "what to do next" (6 texts). It carries most of the teaching.
3. Place 3, the hint bar, and place 2, the tips.
4. Place 7, the starter poses.
5. Place 8, the warnings.
6. Place 9, the first-animation page.
7. Places 5 and 6, the easing hints and the wave notes (first drafts from step 1).

A row that passes gets the status `reviewed by the owner`. A row that does not pass is rewritten and reviewed again.

## 5. How a text gets into the program

1. **Register first.** A new text is first written here, with an ID and the status `draft`. No screen job invents its own wording.
2. **Owner review.** The project owner reads the texts of one place at a time and asks: "Do I understand this without help?" Texts that pass get the status `reviewed by the owner`. Texts that do not pass are rewritten.
3. **Wording taken by ID.** The job that builds the screen copies the wording from the register by its ID, and passes it through `tr()`. The text in the program must match the register.
4. **Status set in the same pull request.** The same pull request that adds the screen changes the status of its rows to `in the program`.

A text that changes later changes in the register first. Then the screen follows. If a text is dropped, its row stays with a note, so the ID is never reused.

The check script `tools/check_texts.py` (run `python3 tools/check_texts.py`) reads this register and warns about long sentences, avoided words, empty or placeholder texts, duplicate IDs and unknown IDs. It runs in the quick checks as a warning only. A later step of #54 is a test with a person who has never animated.
