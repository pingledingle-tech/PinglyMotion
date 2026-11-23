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
| EDIT-01 | 3 Edit menu | Undo is greyed | Undo is not built yet. Save a copy before big changes. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-02 | 3 Edit menu | Redo is greyed | Redo is not built yet. It comes with Undo. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-03 | 3 Edit menu | Paste mirrored is greyed | Paste mirrored is not built yet. Use Tools > Mirror to swap left and right. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-04 | 3 Edit menu | Set key: no joint is picked | Pick a joint first. Click it on the figure or in the list. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-05 | 3 Edit menu | Set key: the animation is playing | Stop playing first. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
| EDIT-06 | 3 Edit menu | Set key: the first frame is locked | The first frame is locked. Turn off Lock the first frame to set a key. | in the program (pull request 371, 22 Nov 2026) | #14 UX phase 1b |
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
