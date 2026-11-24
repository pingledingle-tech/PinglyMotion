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
