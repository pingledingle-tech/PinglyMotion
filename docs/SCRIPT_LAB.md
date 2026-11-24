# Script Lab

Script Lab is a sandbox, being built, inside PinglyMotion for testing RLV and LSL scripts for Second Life. It is not a full
LSL IDE: it gets a small editor and an LSL-to-Lua converter, but no debugger and no project building, and the scripts stay in the user's own folder or repository. The plan,
the steps and the limits are in [SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md); the track is in [ROADMAP.md](ROADMAP.md).

## What exists now (steps 1 and 2, issues 287 and 288)

The RLV core (step 1) and the Script Lab panel (step 2).

- `src/scriptlab/rlvcommand.*`: the parser for owner-say lines such as `@detach=n,sendim=n`.
- `src/scriptlab/rlvbehaviours.*`: a table of 206 behaviours (which parameters each takes, whether it takes an
  option, which actions it blocks) and the RLV version the sandbox answers with, 3.4.3.
- `src/scriptlab/rlvstate.*`: the restrictions each object holds, how many objects hold each one, `@clear`, and the
  answers to `@version*` and `@getstatus*` on a channel.
- Control commands `rlv_command`, `rlv_state` and `rlv_reset` over the local HTTP interface and the MCP server
  ([CONTROL.md](CONTROL.md), "Script Lab").
- Tests: `tests/tst_rlvcommand.cpp`, `tests/tst_rlvstate.cpp` and the RLV part of `tests/tst_control.cpp`; fuzz
  target `fuzz/fuzz_rlv.cpp`.

### The panel (step 2, issue 288)

View > Script Lab opens the panel. It is hidden until asked for, so someone who only animates never sees it. It is the
same kind of panel as the others: it can be docked left or right, floated, resized and closed, and its place, size and
visibility are saved with the layout. View > Reset layout closes it again.

- **Object** and **Command**: type an owner-say line such as `@detach=n,sendim=n` and press Enter (or Run). The line
  goes through the same RLV state as `rlv_command`, sent from the object in the Object field (`panel` unless changed).
  A line that ran is cleared from the field; a refused line stays so it can be fixed.
- **Restrictions**: every held restriction with its option and the object that holds it. With none, a line says what
  to type.
- **What the avatar may do** (was "Actions"): every action the sandbox knows, with the word Allowed or Blocked; the colour (success or danger token)
  and a bold Blocked only back up the word. Since job A2 every row is an Allowed / Blocked choice (below, "Choose a restriction from the list").
- **Replies and messages**: each line run from the panel, the result of each command, the reason for a refusal, and
  the replies a script would hear (`Reply on channel 2222: ...`).
- **Reset all** (was "Reset"): removes every restriction of every object, like `rlv_reset`.
- `@clear` removes the restrictions of the object that sends it, as in Second Life: `@clear` from the panel leaves a
  `collar`'s restrictions in place; Reset removes all.
- The panel follows the state's `changed` signal, so lines sent with `rlv_command` or `rlv_reset` by an AI client show
  up in the lists at once. Replies to lines sent through the control commands go to that client, not to the panel log.
- Code: `src/scriptlab/scriptlabpanel.*`; test: `tests/scriptlabpanel/` (menu entry, restrictions and actions after
  `@detach=n,sendim=n`, `@clear`, Reset, replies, a line through `rlv_command`, colour tokens, and the panel's place
  after saving and loading the layout).

### The panel in the new design (slices 1 to 3)

The panel follows the approved mockup (`docs/mockup/index.html`, the Script Lab page; [UX_DECISIONS.md](UX_DECISIONS.md), 6d):

- **Viewer rules** at the top: **RLV** or **RLVa** (RLVa at start, as the RLV state). Switching calls the RLV state's mode
  switch, so every restriction goes, as after a relog, and the log under Replies and messages says how many. `rlv_reset`
  with a `mode` moves the switch too. Next to it a chip says how many restrictions are held now ("2 held").
- Two pages, **Live scripts** (first) and **Type commands** (the old Live and Commands pages). Since job UX-B four tabs: **Who talks to whom** and **Player** followed (see "One window with its own tabs" below).
- Live scripts with no folder watched shows an empty state: "No folder watched", one line, **Watch folder...** and
  **Try the example scripts** (watches `examples/scriptlab` next to the program and runs `rlv_lock.lsl` at once;
  greyed with a reason when that folder is missing).
- Every part folds with its header (click, Space or Enter); the chevron points right or down, and a short count sits on
  the right: Script (empty, or "● unsaved changes" since the editor of lane 1 job F1), Errors and warnings ("none", "1 error", "2 errors, 1 warning"), Event log ("3 lines",
  "not run"), RLV restrictions now (a number), Objects (a number), Send input; on Type commands Restrictions (a number),
  What the avatar may do and Replies and messages (a number of lines run, or "none"). Objects and Send input start
  folded, as in the mockup. Objects, Send input and Show effects in 3D view also show without a folder once an
  `lsl_run` world exists.
- Errors and warnings also lists the static warnings (`lsl_lint`) of a script that compiled.
- Slice 2: the status line is a box in its colour (success, danger, or the border colour while nothing ran) with the
  circle-check, circle-x or info icon and its first word ("OK:", "Error:") in bold in that colour. **Run again** (next
  to Open in editor) reads the shown script from the folder again and runs it from the start, with a fresh clock;
  it is greyed with the reason when there is no script yet or the script is no longer in the folder. On Type
  commands a **Try:** row has five example lines (`@detach=n,sendim=n`, `@tploc=n`, `@clear`, `@version=2222`,
  `@showself=n`); a click puts the line in the command field and runs it, and the row wraps in a narrow panel.
  While hints are on (Help > Show hints, the Tips switch) a line on top says "A safe sandbox for RLV and LSL scripts.
  Nothing here reaches Second Life." Each part's open or folded state is saved at once (`scriptlab_parts` in the
  settings) and comes back at the next start. The newest dialog ("Dialog from hud", the message and channel, and its
  buttons) sits inside Objects, under the objects.
- The top bar's Script Lab button now opens and closes the docked panel. Before slice 2 it only showed itself
  pressed: it set the panel's View action to checked, which does not show a panel (only triggering it does).
- Slice 3: the last part on Live scripts, **Inventory of <object>** (folded at first, as in the mockup), shows the
  simulated inventory of the object chosen under Send input (the first object at start) in the world of the last
  `lsl_run`: the object's scripts first, then the items given with the run, each with its name and type ("Script",
  "Animation", "Notecard, 4 lines" with the notebook-text icon), the count ("3 items") and the caption "Simulated:
  given with the run, nothing is read from Second Life." It follows the world at once: an item a script removes
  goes from the list. Without a world it says "No inventory yet. An AI assistant gives one with lsl_run." A Live
  folder run keeps no world, so it gives no inventory here. The empty state's two buttons now wrap under each other
  in a narrow docked panel instead of being cut off.
- Code: `src/scriptlab/scriptlabpanel.*`, `src/scriptlab/scriptlabsection.*` (a folding part); test:
  `tests/scriptlabpanel/tst_scriptlabdesign.cpp` (exact texts, counts, the switch both ways, the empty state, folding
  and keys, the tokens in both themes, the summary line, Try:, Run again, the saved fold state, the explanation, the
  dialog inside Objects, the top bar button, and the mockup's texts looked up in the mockup file).

## What exists now (step 3, issue 289): the LSL interpreter

A small LSL interpreter, used through the control commands and, for scripts in a watched folder, the panel's Live
page (step 6, below). Since step 4 (issue 290) scripts talk to the RLV core.

- `src/scriptlab/lsl/`: lexer, recursive descent parser (no flex or bison), a checker with the Second Life compiler's
  name and type rules, the interpreter with a simulated clock, the built-in functions, and `world.*`, which turns what
  a script does into the event log (step 7 plays the animations). `rlvworld.*` (step 4) sends `llOwnerSay("@...")` to
  the RLV core and brings its replies back as listen events.
- `src/scriptlab/lslbuiltins_data.inc`: every LSL function with its signature, every event and the constants, made by
  `tools/gen_lsl_builtins.py` from the viewer's keyword list. 430 functions run (72 from step 3, five more from step 5, nine from step 9a, 25 from step 9b-1, 36 from step 9b-3, ten from step 9b-4, eight from step 9b-5, two from step 9b-6, nine from step 9b-7, 40 from step 9b-8, one from step 9b-9, 17 from step 2, 12 from step 3, nine from step 4, three from step 5, seven from step 6 and six from step 7 of the second gap list, eleven from the second round of pure functions, and 20 from the land and parcel job: 18 land and parcel functions plus `llAsin` and `llLog10`, and 16 from the leftover reads and media job, and 13 from the avatar and camera job, and five from the several-objects job: `llGiveInventoryList`, `llRemoteLoadScriptPin`, `llAllowInventoryDrop`, `llGetInventoryCreator` and `llGetInventoryDesc`, with `llSetRemoteScriptAccessPin` made real, and 15 from the limits, targets and region job of the third gap-list run: `llGetMemoryLimit`, `llSetMemoryLimit`, `llMinEventDelay`, `llGetNumberOfSides`, `llGetLinkNumberOfSides`, `llGetObjectPrimCount`, `llGetObjectMass`, `llGetBoundingBox`, `llGetEnergy`, `llTarget`, `llTargetRemove`, `llGetRegionTimeDilation`, `llGetRegionFPS`, `llGetRegionAgentCount` and `llGetSimulatorHostname`, and 12 from steps 4 to 6 of that run: `llSetRegionPos`, `llListStatistics`, `llRot2Angle`, `llXorBase64`, `llXorBase64Strings`, `llXorBase64StringsCorrect`, `llModPow`, `llSetForce`, `llGetForce`, `llSetVelocity`, `llApplyRotationalImpulse` and `llGroundRepel`, and four from steps 7 to 9: `llSetDamage`, `llSetKeyframedMotion`, `llCreateLink` and `llBreakAllLinks`, and three from steps 10 to 12: `llParcelMediaCommandList`, `llParcelMediaQuery` and `llReleaseCamera`, and twelve from steps 13 to 15: `llCastRay`, `llModifyLand`, `llRequestInventoryData`, `llGetTextureOffset`, `llGetTextureRot`, `llGetTextureScale`, `llGetMinScaleFactor`, `llGetMaxScaleFactor`, `llScaleByFactor`, `llGetObjectAnimationNames`, `llStartObjectAnimation` and `llStopObjectAnimation`, and eight from steps 16 and 17: `llSound`, `llTakeCamera`, `llCloud` and the five remote data functions `llOpenRemoteDataChannel`, `llCloseRemoteDataChannel`, `llRemoteDataSetRegion`, `llSendRemoteData` and `llRemoteDataReply`, and 14 from step 19 (the key-value store, the experience functions, `llGetSimStats`, `llScriptProfiler` and `llManageEstateAccess`), and 26 from step 18: `llGetAccel`, `llGetCenterOfMass`, `llGetGeometricCenter`, `llGetMassMKS`, `llGetOmega`, `llGetPhysicsMaterial`, `llSetPhysicsMaterial`, `llGetTorque`, `llSetTorque`, `llSetForceAndTorque`, `llSetAngularVelocity`, `llSetHoverHeight`, `llStopHover`, `llRotTarget`, `llRotTargetRemove`, `llBreakLink`, `llDetectedGrab`, `llCollisionSprite`, `llEdgeOfWorld`, `llGroundContour`, `llGroundNormal`, `llGroundSlope`, `llGetRegionFlags`, `llWind`, `llScriptDanger` and `llGetSPMaxMemory`); the others give "not supported yet".
- Control commands `lsl_check` (errors and warnings with line and column) and `lsl_run` (the event log) over the local
  HTTP interface and the MCP server ([CONTROL.md](CONTROL.md), "Script Lab: lsl_check and lsl_run": what runs, the log
  entries and the limits).
- Tests: `tests/tst_lsllexer.cpp`, `tests/tst_lslparser.cpp`, `tests/tst_lslinterp.cpp`, `tests/tst_lslrlv.cpp` and the LSL part of
  `tests/tst_control.cpp`; fuzz target `fuzz/fuzz_lsl.cpp`. Examples: `examples/scriptlab/`.

## What exists now (step 5, issue 291): events and several objects

Several scripts run in one world, so a HUD and a restraint can talk on a channel.

- `src/scriptlab/lsl/sandbox.*`: up to 16 objects, each with its own script, key, name, owner, position or
  attachment point, listens and timer, on one simulated clock. Chat between objects follows the Second Life ranges
  (whisper 10 m, say 20 m, shout 100 m, `llRegionSay` the region, `llRegionSayTo` one object); an object never hears
  itself. Things due at the same moment run in a fixed order (objects in list order, timers before inputs), so a run
  is deterministic. Each object's RLV lines are held under its own key, so `@clear` from one object leaves the
  others' restrictions alone.
- New: the `attach` event, `llDialog`, `llRegionSayTo`, `llGetPos`, `llGetAttached`, `llGetOwnerKey`. Inputs from
  outside: chat from an avatar (with a range), a touch, a dialog button, attach and detach.
- `src/scriptlab/lsl/session.*`: keeps the world of the last `lsl_run`, so the control command `lsl_event` can send it
  one more event and run it on.
- Control commands: `lsl_run` with `objects` instead of `source`, and `lsl_event` ([CONTROL.md](CONTROL.md),
  "Several objects and lsl_event"). The single-script form of `lsl_run` works as before.
- The panel: on the Live scripts page, the folding part **Objects** lists the objects of the last run (name; where it is, or "Worn (point
  35)"; "Running" or "Stopped: <why>" in the danger colour and bold; the restrictions it holds) and, under them, the
  newest dialog (which object, the message and the channel) with one button for each of its buttons. The part is hidden until a script has run,
  and follows every `lsl_run` and `lsl_event` at once. Code: `src/scriptlab/objectsview.*`.
- Tests: `tests/tst_lslworld.cpp` (two objects talking on a channel, whisper out of range not heard, timer order,
  touch, dialog, attach and detach through `lsl_event`, two objects holding the same restriction where `@clear`
  removes only its own, limits that stop one script while the others run on, refused input) and
  `tests/scriptlabpanel/tst_scriptlabobjects.cpp` (the panel's exact rows, dialog text and colours). Fuzz: `fuzz_lsl`
  runs every script as two objects with every input type; `fuzz_control` takes a list of requests, so `lsl_event`
  after `lsl_run` is reached.
- Example: `examples/scriptlab/hud_cuffs_hud.lsl` with `hud_cuffs_cuffs.lsl` (a HUD asks with a dialog and tells the
  cuffs on a channel; the cuffs lock with `@detach=n`).

## What exists now (step 6, issue 292): Live

The Script Lab watches a folder of scripts and runs a script again each time it is saved, so the user (or Claude
Code) edits in their own editor and sees the result in PinglyMotion at once. Since lane 1 job F1 the script can also be changed in the panel itself ("The editor" below).

- The panel has two pages: **Type commands** (the RLV lines of step 2) and **Live scripts**. On Live scripts, **Watch folder...** (in the
  empty state) picks the folder (a cloned repository, for example); **Stop watching** ends it. When a watch starts, here or through
  `lab_watch`, the Live page comes to the front.
- Since lane 1 job D, **Watch folder...** watches the folder as one object, so a save runs all its scripts together ("The folder model" below); **Try the example scripts** watches the examples each on its own, because they are separate examples.
- About 0.3 seconds after a `.lsl` file in the folder is saved, it is checked and, when it compiles, run. The page
  shows a status line that starts with a word (`OK` or `Error`; the colour only backs it up), the script
  with line numbers (read-only until job F1, now in the editor) (an error's line number is bold in the danger colour and the line is underlined), the errors
  and warnings with line and column (a click or Enter shows the line in the script; since job R3 also a click on an error or RLV line of the event log), the event log of the
  run, and the RLV restrictions at the end. The script's `llOwnerSay("@...")` lines go to the same RLV state the
  Commands page shows (emptied before each run, as `lsl_run` does unless `keep_rlv` is true). **Open in editor** opens the file in the program the computer uses for
  `.lsl` files; if there is none, the status line says so.
- Control commands `lab_watch`, `lab_unwatch` and `lab_result` ([CONTROL.md](CONTROL.md), "Script Lab Live"):
  the same watch and the same results as the panel.
- Safety: the folder is only read; at most 200 scripts of at most 262 144 bytes, 5 000 entries and 4 folder levels;
  names that start with a dot, such as `.git`, are skipped; links are not followed out of the folder; every run has the
  interpreter's limits, including 2 seconds of real time.
- `lab_watch` has an `inputs` option: the chat and touches given to every run. Each run starts fresh with a
  simulated clock. To send events to a world that keeps running between commands, use `lsl_run` and then
  `lsl_event` (step 5).
- Code: `src/scriptlab/labwatcher.*` and the Live page in `src/scriptlab/scriptlabpanel.*`. Tests:
  `tests/tst_labwatcher.cpp` (a saved script runs within the debounce, errors with line and column, saving again,
  Stop watching, the limits, links, an endless loop), the Live part of `tests/scriptlabpanel/` and of
  `tests/tst_control.cpp`.

## What exists now (step 7, issue 293): effects in the 3D view

What a script run does to the avatar shows in the 3D view, only while the Live page's **Show effects in 3D view** box
is ticked (an on/off button since job U3; "ticked" means on). The box is off each time the program starts, so someone who only animates never sees a change. Runs that
count: each Live run, and each `lsl_run` and `lsl_event` from the control commands (the newest of them is shown).
An animation started in one command keeps playing through the `lsl_event` commands that follow in the same world.

- **Animations:** the run's event log is replayed: `llStartAnimation` starts an animation, `llStopAnimation` stops it,
  starting one that already plays changes nothing. The newest animation still playing at the end of the run is shown;
  the others are named in the text under the box ("Also playing, not shown"). It plays on a preview figure from the
  frame it has reached after the run's simulated seconds (it starts on frame 1, moves one frame per frame time and
  repeats between its loop points), then on in real time.
- **Where an animation comes from:** a file named like the animation, `<name>.dingle`, `.bvh` or `.avm` (in that order),
  next to the script, in the watched folder, then in the `examples` folder next to the program (in the Windows zip;
  `examples/` in the source tree), each up to two folder levels down. Names are compared with the folder listing (case
  matters, as in Second Life); a name with a folder separator, a leading dot or a character no file name can hold is
  refused, links and hidden entries are skipped, and files over 16 MB are not taken. So a script can never reach a file
  outside these folders. Example: `examples/scriptlab/anim_wave.lsl` waves with the bundled `wave.dingle`.
- **Worn objects and locks:** objects of the run with an attachment point (`attached`, step 5) are small
  boxes at the point's joint (accent colour); an object holding `@detach=n` and points locked with `@detach:<point>=n`
  get a ball (danger colour). HUD points are named in the text only. The text under the box says the same in words
  ("Worn: cuffs on Left Hand, locked.", "Locked points: Neck."), so colour is never the only sign. An object holding
  `@detach=n` that is not worn (a single script is rezzed, not worn) locks nothing on the avatar, and the text says so.
  So with `lsl_run` and `objects`, `attach` and `detach` (`lsl_event`) make the box appear and go, and `@clear` the ball.
- **Safety:** the preview has its own copy of the animation, loaded with a reader of its own. The user's open
  animations, the scene, the timeline and the "changed" mark are never touched, and nothing is written. While the
  preview plays, the figure cannot be clicked. Unticking the box, stopping the watch, a run in which nothing plays or a
  script that does not compile removes the preview, and the 3D view shows the user's animation again exactly as it was.
- An unknown animation is an Error line under the box (in the danger colour as well), for example
  `Error: Animation "dance1" was not found: put dance1.dingle, dance1.bvh or dance1.avm next to the script or in the
  examples folder.` Second Life's built-in animations are not bundled.
- Code: `src/scriptlab/labeffects.*` (the run answer read as data, the lookup, the frame), `src/scriptlab/effectspreview.*`
  (the preview the 3D view draws), the box in `src/scriptlab/scriptlabpanel.*`, a few lines in `src/animationview.cpp`.
  Tests: `tests/tst_labeffects.cpp` and `tests/tst_scriptlabeffects.cpp`.
- With an AI assistant: `lab_watch` the folder (or `lsl_run`), with a script that calls `llStartAnimation("wave")`;
  with the box ticked the control command `screenshot` shows the waving figure. The box itself is ticked by the user in
  the panel.

## What exists now (step 9a, issue 305): several scripts in one object

An object can hold several scripts in one or more prims, as Second Life objects usually do (a main script, a menu,
an RLV relay, a store), and they talk to each other.

- `lsl_run`'s `objects` entries take `scripts` (`[{name, source, link}]`, 1 to 32) and `prims` instead of `source`;
  at most 64 scripts in all. Each script has its own interpreter: its own memory, its own queue of 64 events, its own
  listens and timer. Log entries name the `script`; the object entry in the answer has `prims`, `scripts` and
  `linkset_data`. The old shape is unchanged.
- `llMessageLinked`, `link_message` and `llGetLinkNumber` with `LINK_SET`, `LINK_ROOT`, `LINK_ALL_OTHERS`,
  `LINK_ALL_CHILDREN`, `LINK_THIS` and link numbers (a single prim is link 0, the root is 1 otherwise).
- Linkset Data: `llLinksetDataWrite`, `Read`, `Delete`, `Reset`, `ListKeys`, `Available`, `CountKeys` and the
  `linkset_data` event for every script. The data belongs to the object, survives a script reset and stays for
  `lsl_event`; 128 KiB counted in UTF-8. The protected and pattern functions say "not supported yet".
- As in Second Life: a link message or `linkset_data` value too big for the receiver's free memory crashes it with a
  Stack-Heap Collision; more than 64 waiting events are dropped (the log shows each); a prim does not hear its own
  chat. Not as in Second Life: scripts run one after the other in a fixed order, not in parallel; memory is a rough
  count with 256 KB per script; every prim has the object's key. Details: [CONTROL.md](CONTROL.md), "Several scripts
  in one object"; reasons: [DECISIONS.md](DECISIONS.md).
- A touch can name the prim (`link`); a prim without a touch handler (`touch_start`, `touch` or `touch_end`) passes it to the root.
- Code: `src/scriptlab/lsl/sandbox.*` (scripts, prims, link messages), `world.*` (the Linkset Data store),
  `builtins.cpp`. Tests: `tests/tst_lsllink.cpp`. Fuzz: `fuzz_lsl` also runs every script as three scripts in one
  object of two prims and drives the Linkset Data store from the input.

## What exists now (step 9b-1, issue 310): the signature table and pure functions

- The definition check (lane 1, job L1-A) lists what the interpreter lacks compared with those definitions: [LSL_DEFINITION_CHECK.md](LSL_DEFINITION_CHECK.md). A test fails if a function the sandbox runs, or an event it raises, is not in the definitions.

- The signature table knows every function, event and constant of Second Life's own lsl-definitions (526 functions, 43
  events, 1 082 constants), not only the viewer's keyword list: `src/scriptlab/lslbuiltins_extra.inc`, made by
  `tools/gen_lsl_extra.py` from `lsl_definitions.yaml` (MIT, Linden Research; [CREDITS.md](../CREDITS.md)). The
  checker so accepts scripts that use `llReplaceSubString`, `llListSortStrided`, `llSetLinkGLTFOverrides`
  and the like, and warns "not supported yet" for those that do not run. (`llWarp2Pos` is in neither list: it is no LSL function.)
- 25 more functions run: rotation maths, `llListReplaceList`, `llList2ListStrided`, `llListSortStrided`,
  `llReplaceSubString`, Base64, MD5, SHA-1, URL escaping, `llGetStartParameter`, `llGetObjectDesc`, `llSetObjectDesc`,
  `llSetObjectName`, `llGetScale`, `llGetNumberOfPrims`. The choices and the few cases that say "not supported yet" are in
  [CONTROL.md](CONTROL.md), "Pure functions and object state". New `lsl_run` fields: `description`, `scale`, `start_parameter`.
- Tests: `tests/tst_lslpure.cpp`, exact answers computed outside the program.
- Limits are unchanged: 256 KB of variables per script in the sandbox (Second Life: 64 KB).
## Input buttons (issue 308)

The **Send input** part at the bottom of the Live page (the page scrolls when the panel is small) sends one simulated
input to the world of the last `lsl_run` and runs that world on for one simulated second. It calls the same code as the
control command `lsl_event`, so the log lines are exactly the ones `lsl_event` answers; there is no new control command.
The Live folder watch keeps no world between runs, so the buttons work on a world made by `lsl_run` (an AI assistant, or
a script run through the control commands), the one the Objects part shows.

- **To object** (the objects of the world) and **Touch**: sends a click to that object (`touch_start`, `touch` and
  `touch_end` at once). Holding a touch longer is only possible through `hold` in the control commands for now.
- **Channel**, **Name** and **Text** with **Say** (or Enter in Text): the avatar says the line on the channel. Name empty
  means the owner's avatar. Every object within range hears it; the object chosen above is not used for chat.
- **Dialog from ...**: the newest dialog, with one button for each of its buttons. A button answers it, as the avatar
  says the button text on the dialog's channel.
- A line under the part says in words why the buttons are off (no script has run; every script has stopped; Say needs
  text) and the greyed buttons carry the same reason as a tooltip. After an input, **Sent: ...** (success colour) or
  **Error: ...** (danger colour) says what happened, and the log under it lists the input and every event it caused. The
  panel's restrictions, objects and dialog follow the new state at once.
- Tab visits every control in screen order; Space presses a button and Enter in Text says the line.
- Test: `tests/scriptlabpanel/tst_scriptlabinput.cpp` (disabled reasons, the exact log lines of a touch, a chat line and a
  dialog answer with the HUD and cuffs example, refused input, a stopped world, keyboard use, the dark theme).
- With an AI assistant: the world is shared. The owner can press Touch or a dialog button in the world the assistant just
  made, and the assistant's next `lsl_event` and `rlv_state` continue from there, so the owner can show a case by hand
  and the assistant reads what changed (restrictions, dialog, time). The log lines have the same shape as `lsl_event`'s,
  so an expected answer can be written once and checked in both places.

## What exists now (step 9b-2, issue 314): on_rez and changed

- `on_rez` is raised at the start of every object with its `start_parameter` (default 0), after `state_entry`; `llResetScript` does not raise it again.
- `changed` comes from the new input type `changed` (`change`: a name such as `inventory`, `owner`, `link`, `region_start`, a list, or the flags as a number)
  and reaches every script of the object. No new control command; details in [CONTROL.md](CONTROL.md), "The events on_rez and changed".
- Tests: `tests/tst_lslevents.cpp`. Fuzz: `fuzz_lsl` also sends a `changed` input and a start parameter.

## What exists now (step 9b-3, issue 317): prim state, L$ logic and record-only functions

- The sandbox keeps simulated prim state (hover text, colour, alpha and texture per face, position, rotation, size, status, sit text) and records each change as a `prim` log entry; scripts can read it
  back with `llGetColor`, `llGetPos`, `llGetLinkPrimitiveParams` and the like. `llSetPrimitiveParams`, `llSetLinkPrimitiveParams` and `llSetLinkPrimitiveParamsFast` run 15 rules; other rules say "not supported yet".
- `llTextBox` and `llDialog` with `!!llTextBox!!` give a text-box `dialog` entry; `llTransferLindenDollars` with `PERMISSION_DEBIT` runs its logic and `transaction_result` without moving any L$.
- `llParticleSystem`, `llLinkParticleSystem`, `llTriggerSound`, `llPlaySound`, `llLoopSound`, `llStopSound`, `llPreloadSound`, `llTargetOmega` and `llLoadURL` are recorded only. Since job A4 a particle system with `PSYS_SRC_TARGET_KEY` (a chain or ribbon) names the prim that emits and its target, so the 3D view can draw the chain ("Chain and ribbon lines" below).
- Nothing is drawn or played and the 3D view is not touched. Details and the places where the sandbox simplifies: [CONTROL.md](CONTROL.md), "Prim state, L$ and recorded-only functions".
- Tests: `tests/tst_lslprims.cpp`. Limits unchanged (256 KB of variables per script; Second Life gives 64 KB).

## What exists now (step 9b-4): inventory, notecards and rezzing

- Objects get a simulated inventory from the `lsl_run` input (`inventory`: items with name, type, key and, for a notecard, lines); the object's scripts are in it too. Nothing is read from disk.
- `llGetInventoryNumber`, `llGetInventoryName` (sorted by name), `llGetInventoryType`, `llGetInventoryKey`, `llGiveInventory` (to the avatar, or into another object of the world), `llRemoveInventory`,
  `llGetNotecardLine`, `llGetNumberOfNotecardLines`, `llRezObject` and `llRezAtRoot` run; the events `dataserver` and `object_rez` are raised, and giving or removing an item raises `changed` with `CHANGED_INVENTORY`.
- A rezzed object is only a key and a log entry: it does not run. Nothing is drawn and the 3D view is not touched. Details: [CONTROL.md](CONTROL.md), "Inventory, notecards and rezzing".
- Tests: `tests/tst_lslinventory.cpp`. Limits unchanged; an item list holds at most 224 items, a notecard 2000 lines and 65536 characters.

## What exists now (step 9b-5): sitting and pose balls

- New inputs `sit` and `stand` (also for `lsl_event`) put an avatar on a prim of an object and take it off; every script of the object gets `changed` with `CHANGED_LINK`.
- `llSitTarget`, `llLinkSitTarget`, `llAvatarOnSitTarget`, `llAvatarOnLinkSitTarget`, `llUnSit`, `llGetAnimationList`, `llGetAnimation` and `llGetAgentInfo` run. An avatar sitting on the object gets the animation permission
  at once, so the usual pose ball (sit, `changed`, `llRequestPermissions`, `llStartAnimation`) runs. `llGetNumberOfPrims` counts sitting avatars.
- Nothing about the avatar's place on the sit target is simulated, and standing up does not stop an animation by itself. Details: [CONTROL.md](CONTROL.md), "Sitting and pose balls".
- Tests: `tests/tst_lslsitting.cpp`.

## What exists now (step 9b-6): controls

- `llTakeControls` and `llReleaseControls` run (with `PERMISSION_TAKE_CONTROLS`, which an avatar sitting on the object gets at once). The new input type `control` (`levels`, `edges`; also for `lsl_event`) raises the `control` event in scripts that took those controls.
- Nothing moves and a held key is not repeated: one input is one change. Details: [CONTROL.md](CONTROL.md), "Controls". Tests: `tests/tst_lslcontrols.cpp`.

## What exists now (third gap-list run, steps 1 to 3): limits, targets and region reads

- Fifteen more functions run (technical, nothing changes on screen). Limit reads: `llGetMemoryLimit` (65536 at the start and after a reset) and `llSetMemoryLimit` (TRUE and stored for a limit from the memory the script uses now up to 65536, else FALSE), `llMinEventDelay` (record only), `llGetNumberOfSides` (6) and `llGetLinkNumberOfSides`, `llGetObjectPrimCount`, `llGetObjectMass` (the same mass as `llGetMass` for the own object), `llGetBoundingBox` (the box round all prims of an object, relative to its root prim, along the region's axes) and `llGetEnergy` (1.0).
- `llTarget(position, range)` gives a handle (1, 2, 3, ...; each script has its own) and `llTargetRemove(handle)` removes it. The only thing that moves an object is the `step` input of the motion model, so that is the only automatic source: after each `step` every target within its range of the object raises `at_target(handle, targetpos, ourpos)` in the script that set it, and when there are targets but none is reached `not_at_target()` is raised once. A target stays until `llTargetRemove`, as in Second Life. `llSetPos` and the manual `at_target` and `not_at_target` inputs work as before; `llSetPos` raises nothing on its own.
- Region reads: `llGetRegionTimeDilation` (1.0), `llGetRegionFPS` (45.0), `llGetRegionAgentCount` (the agents of the world: the owner and the other avatars) and `llGetSimulatorHostname` (`sim-sandbox.invalid`), canned values kept in `RegionSettings`.
- Details and the places where the sandbox guesses: [CONTROL.md](CONTROL.md), "Limits, targets and region reads". Tests: `tests/tst_lsllimits.cpp`.

## What exists now (third gap-list run, steps 4 to 6): region position, pure maths and force records

- Twelve more functions run (technical, nothing changes on screen). `llSetRegionPos` moves the object (no 10 m limit; FALSE for an attachment, a physical object or a target outside x and y -10 to 266, z 0 to 4096). `llListStatistics` (all ten `LIST_STAT_*` modes), `llRot2Angle`, `llXorBase64`, `llXorBase64Strings`, `llXorBase64StringsCorrect` (the two old names are deprecated) and `llModPow` (unsigned 32-bit) are pure. `llSetForce` and `llGetForce` record and read back a force, `llSetVelocity` sets the velocity `llGetVel` reads, `llApplyRotationalImpulse` and `llGroundRepel` are record only.
- `llWarp2Pos` is no LSL function and stays unknown.
- Details and the places where the sandbox guesses: [CONTROL.md](CONTROL.md), "Region position, pure maths and force records". Tests: `tests/tst_lslsteps456.cpp`.

## What exists now (third gap-list run, step 19): key-value store, experiences, simulator statistics, profiler and estate access

- 14 more functions run (technical, nothing changes on screen). The key-value store: `llCreateKeyValue`, `llReadKeyValue`, `llUpdateKeyValue`, `llDeleteKeyValue`, `llDataSizeKeyValue`, `llKeyCountKeyValue` and `llKeysKeyValue` answer with a `dataserver` event (`1,<value>` or `0,<XP_ERROR_*>`). Experiences: `llAgentInExperience`, `llGetExperienceDetails`, `llGetExperienceErrorMessage` and `llRequestExperiencePermissions` (with `experience_permissions` and `experience_permissions_denied`). Also `llGetSimStats` (canned numbers), `llScriptProfiler` (with `llGetSPMaxMemory`) and `llManageEstateAccess` (estate-owner check, throttle, lists).
- `lsl_run` takes `experience` (puts every script of the world into one simulated experience, with its store, members and blocked avatars) and `estate` (owner, managers and the access lists). A new input type `experience` (`kind` `allow`, `deny`, `set` or `delete`), for `inputs` and for `lsl_event`, answers a permission request or changes the store from outside. The events `experience_permissions` and `experience_permissions_denied` are raised.
- Pathfinding (`llCreateCharacter`, `llNavigateTo` and the rest) is still not supported; `llNavigateTo` is now the example of "not supported yet" in the tests.
- Details and the places where the sandbox guesses: [CONTROL.md](CONTROL.md), "Key-value store, experiences, simulator statistics, profiler and estate access". Tests: `tests/tst_lslsteps19.cpp`.

## What exists now (third gap-list run, step 18): physics reads, rotation targets and ground reads

- 26 more functions run (technical, nothing changes on screen). Physics reads and settings: `llGetAccel` (the change of velocity per second in the motion model's last `step`; `ZERO_VECTOR` in an attachment, in a child prim and before any step), `llGetOmega`, `llSetAngularVelocity`, `llGetTorque`, `llSetTorque`, `llSetForceAndTorque` (stored records, as `llSetForce`; the model does not use them), `llGetMassMKS` (`llGetMass` times 100), `llGetCenterOfMass` and `llGetGeometricCenter`, `llGetPhysicsMaterial` and `llSetPhysicsMaterial` (gravity 1.0, restitution 0.3, friction 0.5, density 1000.0 at the start; out-of-range values are cut to the wiki's ranges). `llSetHoverHeight`, `llStopHover` and `llCollisionSprite` are record only.
- `llRotTarget(rotation, error)` and `llRotTargetRemove(handle)` work like `llTarget`: handles 1, 2, 3, ...; after each `step` input `at_rot_target(handle, targetrot, ourrot)` is raised for every target whose angle to the object's rotation is within `error`, and `not_at_rot_target()` once when there are targets and none is reached. The checker no longer warns that these two events are never raised.
- `llBreakLink(link)` (needs `PERMISSION_CHANGE_LINKS`, like `llBreakAllLinks`) separates the object that `llCreateLink` joined and that holds that link number; any other number logs a note.
- Reads: `llDetectedGrab` (always `ZERO_VECTOR`), `llEdgeOfWorld` (always TRUE: the region has no neighbour), `llGroundSlope`, `llGroundContour` (`ZERO_VECTOR`) and `llGroundNormal` (`<0,0,1>`) for the flat ground, `llGetRegionFlags` (`REGION_FLAG_ALLOW_DIRECT_TELEPORT`), `llWind` (`ZERO_VECTOR`), `llScriptDanger` and `llGetSPMaxMemory` (0: the profiler is not supported).
- The pathfinding functions are still not supported (step 19 added the experience, key-value, `llGetSimStats`, `llScriptProfiler` and `llManageEstateAccess` functions).
- Details and the places where the sandbox guesses: [CONTROL.md](CONTROL.md), "Physics reads, rotation targets and ground reads". Tests: `tests/tst_lslsteps18.cpp`.

## What exists now (third gap-list run, steps 16 and 17): obsolete functions and remote data

- Eight more functions run (technical, nothing changes on screen). Step 16, the obsolete functions that were still missing: `llSound` and `llTakeCamera` are recorded only (the wiki says `llTakeCamera` was never implemented) and `llCloud` gives 0.0 (the wiki: it only returns 0.0 since 2013). Step 17, remote data (XML-RPC, deprecated): `llOpenRemoteDataChannel`, `llCloseRemoteDataChannel`, `llRemoteDataSetRegion`, `llSendRemoteData` and `llRemoteDataReply`, with the `remote_data` event and the constants `REMOTE_DATA_CHANNEL` (1), `REMOTE_DATA_REQUEST` (2) and `REMOTE_DATA_REPLY` (3). Nothing goes on the network: a request or an answer comes from a new input type `remote_data` of `lsl_run` and `lsl_event`. `lsl_check` no longer warns that `remote_data` is never raised (`path_update` is now the example in the tests).
- `llGetAccel` was still not supported at this step (it runs since step 18); `llWarp2Pos` is no LSL function.
- Details and the places where the sandbox guesses: [CONTROL.md](CONTROL.md), "Obsolete functions and remote data". Tests: `tests/tst_lslsteps1617.cpp`.

## What exists now (third gap-list run, steps 13 to 15): ray casts, land, landmark data, texture reads, scale factor and object animations

- Twelve more functions run (technical, nothing changes on screen). `llCastRay` casts a line against the boxes of the objects and avatars of the world and the flat ground (options `RC_REJECT_TYPES`, `RC_DETECT_PHANTOM`, `RC_DATA_FLAGS`, `RC_MAX_HITS`; the list ends with the hit count). `llModifyLand` is record only (a `land_edit` entry; the ground does not change). `llRequestInventoryData` answers for a landmark of the inventory (new optional `position` of an inventory item) with a `dataserver` event. `llGetTextureOffset`, `llGetTextureRot` and `llGetTextureScale` read back the texture values of the prim's faces; `llOffsetTexture`, `llScaleTexture` and `llRotateTexture` now keep their values there (and are still recorded). `llGetMinScaleFactor`, `llGetMaxScaleFactor` and `llScaleByFactor` read and change the size of the whole object. `llStartObjectAnimation`, `llStopObjectAnimation` and `llGetObjectAnimationNames` keep the list of playing object animations (a `object_animation` entry for each change).
- `llGetAccel` was still not supported at this step (it runs since step 18); `llWarp2Pos` is no LSL function.
- Details and the places where the sandbox guesses: [CONTROL.md](CONTROL.md), "Ray casts, land, landmark data, texture reads, scale factor and object animations". Tests: `tests/tst_lslsteps131415.cpp`.

## What exists now (third gap-list run, steps 10 to 12): parcel media, the continuous collision event and llReleaseCamera

- Three more functions run (technical, nothing changes on screen). `llParcelMediaCommandList` and `llParcelMediaQuery` keep and read back the media of the sandbox's one parcel (URL, TEXTURE, TYPE, DESC, SIZE, AUTO_ALIGN, and the status left by STOP, PAUSE, PLAY, LOOP and UNLOAD); nothing is fetched or played, and only a script owned by the parcel's owner may use them. `llReleaseCamera` needs `PERMISSION_CONTROL_CAMERA` and is recorded.
- The `collision` event is raised now: a `collision` input without `phase` is the continuous event (while colliding), next to `collision_start` and `collision_end`. `lsl_check` no longer warns about it.
- `llGetAccel` was still not supported at this step (it runs since step 18); `llWarp2Pos` is no LSL function.
- Details and the places where the sandbox guesses: [CONTROL.md](CONTROL.md), "Parcel media, the continuous collision event and llReleaseCamera". Tests: `tests/tst_lslsteps101112.cpp`.

## What exists now (third gap-list run, steps 7 to 9): damage, keyframed motion and linking objects

- Four more functions run (technical, nothing changes on screen). `llSetDamage` stores the damage (cut to -100 to 100). `llSetKeyframedMotion` checks and stores a list of frames and options (`KFM_MODE`, `KFM_DATA`, `KFM_COMMAND`); the `step` input plays it (straight-line moves, an evenly spread turn, `llGetVel` is the frame's speed, no falling while a motion is stored). `llCreateLink` and `llBreakAllLinks` need `PERMISSION_CHANGE_LINKS` and join objects of the world into a linkset (new children at link 2, newest first) and separate them again; every script gets `changed(CHANGED_LINK)`.
- `llGetDamage` is no LSL function and stays unknown; `llGetAccel` was still not supported at this step (it runs since step 18).
- Details and the places where the sandbox guesses: [CONTROL.md](CONTROL.md), "Damage, keyframed motion and linking objects". Tests: `tests/tst_lslsteps789.cpp`.

## What exists now (second gap list, step 6, part 1): movement and physics stubs

- Seven more functions run: `llMoveToTarget`, `llStopMoveToTarget`, `llPushObject`, `llApplyImpulse`, `llSetBuoyancy`, `llRotLookAt` and `llVolumeDetect`. Record only, the same pattern as the step 9b-8 stubs: each call writes a `recorded_only` entry with its exact arguments, nothing moves and no physics runs. `llApplyImpulse` and `llSetBuoyancy` became real functions in part 3, below.
- Collision events need a motion model and are not part of this step (part 2, below); neither are the vehicle functions (step 7). Details: [CONTROL.md](CONTROL.md), "Recorded only". Tests: `tests/tst_lslmovement.cpp`.

## What exists now (second gap list, step 6, part 2): collision-family events

- Still no motion model, so nine more events never happen on their own; new `inputs` types raise them for testing, with given data, not simulated collisions: `collision` (`phase` `"start"`/`"end"`, optional `link`, `name`/`id` of the other object or avatar, as a touch) raises `collision_start`/`collision_end`; `land_collision` (`phase` `"start"`, `"end"` or left out for the continuous `land_collision`, and `pos`) and `moving` (`phase` only, `moving_start`/`moving_end` take no arguments) go to every script of the object; `at_target` (`tnum`, `targetpos`, `ourpos`) and `not_at_target` likewise.
- `touch_end` was already raised since step 1 of the second gap list; nothing changed there. The checker's "never raised" warning no longer applies to the nine events above: 32 of the 43 LSL events are raised now.
- The one-line gap-list fix for `transaction_result` (missing from `Parser::eventIsRaised`) was already in place; nothing to do there.
- Part 3, below, gives `moving_start`/`moving_end` a second, automatic source (the motion model); the manual `moving` input above still works the same way. `collision`, `land_collision`, `at_target` and `not_at_target` are unchanged: still manual only.

## What exists now (second gap list, step 6, part 3): a simple motion model

A deliberately small slice, not a physics engine: one more `inputs` type and two functions that are real now, so `llGetVel` and `moving_start`/`moving_end` can change on their own, without a manual input, for the first time.

- `llApplyImpulse` and `llSetBuoyancy` are real now (no longer record only). `llApplyImpulse(vector impulse, integer local)` changes the object's velocity **at once** by `impulse / mass` (the same mass as `llGetMass`); `local` turns the impulse by the object's rotation first, as `llSetPos`'s local flag does. `llSetBuoyancy(float buoyancy)` stores the value, clamped to 0 (full gravity) .. 1 (floats); Second Life allows other values, the sandbox does not.
- The new `inputs` type `step` (`dt`: seconds, 0 to 60) advances the model by that many seconds for one object: velocity changes by simple gravity (-9.8 m/s² along z, scaled by `(1 - buoyancy)`), then position changes by velocity. Not continuous physics: call `step` as many times as the test needs, with whatever `dt` it needs; nothing advances between calls on its own.
- `llGetVel` now answers the model's velocity (it always answered `ZERO_VECTOR` before).
- `step` also fires `moving_start`/`moving_end` on its own when the object's speed crosses 0.1 m/s (the sandbox's own choice), in addition to the manual `moving` input from part 2. A worn object's `step` input does nothing but note that the model does not apply to an attachment (nothing about an attachment's motion is simulated).
- Left out on purpose, for a later job: `llMoveToTarget`/`llStopMoveToTarget` stay record only (the model does not seek a target yet); `at_target`/`not_at_target` now also come from `llTarget` targets after a `step` (see "limits, targets and region reads", below), besides the manual inputs; `llPushObject`, `llRotLookAt` and `llVolumeDetect` stay record only too; real collisions from motion (two objects actually touching) and the vehicle functions (step 7, below) are untouched.
- Details: [CONTROL.md](CONTROL.md), "A simple motion model". Tests: `tests/tst_lslmotion.cpp`.
- Details: [CONTROL.md](CONTROL.md), "Movement and physics stubs" and "Collision-family events". Tests: `tests/tst_lslcollision.cpp`.

## What exists now (second gap list, step 7): vehicle function stubs

- Six more functions run: `llSetVehicleType`, `llSetVehicleFloatParam`, `llSetVehicleVectorParam`, `llSetVehicleRotationParam`, `llSetVehicleFlags` and `llRemoveVehicleFlags`. Record only, the same pattern as the step 6 part 1 movement stubs: each call writes a `recorded_only` entry with its exact arguments, and no vehicle physics is simulated. Details: [CONTROL.md](CONTROL.md), "Recorded only". Tests: `tests/tst_lslmovement.cpp`.

## What exists now (land and parcel, second gap list)

- 18 more functions run, with no parcel model: the whole 256 by 256 m region is one canned parcel (`RegionSettings` in `world.h`). `llAddToLandBanList`, `llRemoveFromLandBanList`, `llResetLandBanList`, `llAddToLandPassList`, `llRemoveFromLandPassList`, `llResetLandPassList`, `llMapDestination` and `llEjectFromLand` only write a `recorded_only` entry; `llReturnObjectsByOwner` and `llReturnObjectsByID` do the same and answer 0. `llGetLandOwnerAt`, `llOverMyLand`, `llGetParcelFlags`, `llGetParcelPrimCount`, `llGetParcelMaxPrims`, `llGetParcelPrimOwners` and `llGetParcelDetails` give canned answers (a position outside the region is on no parcel); `llSetParcelMusicURL` records and stores the address and `llGetParcelMusicURL` reads it back. Also `llAsin` and `llLog10`. No `lsl_run` option sets the parcel values yet. Tests: `tests/tst_lslland.cpp`.

## What exists now (leftover reads and media, second gap list)

- 16 more functions run, all with canned answers or record only: `llGetDisplayName`, `llRequestDisplayName`, `llRequestUsername` and `llRequestSimulatorData` (the `dataserver` event comes at once), `llGetGMTclock`, `llGetEnv`, `llSetContentType` (a `recorded_only` entry), the link media functions `llSetLinkMedia`, `llGetLinkMedia` and `llClearLinkMedia` (the same storage as `llSetPrimMediaParams`), and the touch data `llDetectedTouchFace`, `llDetectedTouchUV`, `llDetectedTouchST`, `llDetectedTouchNormal`, `llDetectedTouchBinormal` and `llDetectedTouchPos`. `llGetObjectDetails` now answers the flags 50 to 56 (`OBJECT_HEALTH` to `OBJECT_VOLUME_DETECT`). The constants table already had every `OBJECT_*` constant of the wiki (1 to 56); none was added. Details: [CONTROL.md](CONTROL.md), "Leftover reads and media". Tests: `tests/tst_lslrest.cpp`.

## What exists now (avatar and camera)

- 13 more functions run, record only or with canned answers: `llAttachToAvatar`, `llAttachToAvatarTemp` and `llDetachFromAvatar` (need `PERMISSION_ATTACH`; a `recorded_only` entry, the object's attachment point does not change), `llGetAttachedList`, `llGetAgentLanguage`, `llForceMouselook` (record only), `llGetCameraPos`, `llGetCameraRot`, `llGetCameraAspect`, `llGetCameraFOV` (need `PERMISSION_TRACK_CAMERA`) and `llTeleportAgentHome`, `llTeleportAgent`, `llTeleportAgentGlobalCoords` (need `PERMISSION_TELEPORT`, owner only, record only). Details: [CONTROL.md](CONTROL.md), "Avatar and camera". Tests: `tests/tst_lslavatar.cpp`.

## What exists now (second round of pure functions): JSON, llListRandomize and maths

- Eleven more pure functions run: `llJsonGetValue`, `llJsonSetValue`, `llJsonValueType`, `llList2Json`, `llJson2List`, `llListRandomize`, `llLog`, `llAcos`, `llAxes2Rot`, `llAngleBetween` and `llRot2Axis`. No side effects and no log entries.
- JSON follows the Second Life wiki ("Json usage in LSL" and the function pages): the `JSON_*` constants, `JSON_APPEND` and `JSON_DELETE`; the reader is lenient like Second Life's (an empty value such as `[ , ]` or `{"a":,}` is `JSON_NULL`, the last of two equal keys wins, a number reads back as the text it was written with); `llJsonSetValue` and `llList2Json` take `"true"`, `"false"`, `"null"`, numbers, objects, arrays and text already in quotes as JSON, and anything else as a JSON string; a changed object or array is written again without spaces. Sandbox choices where the wiki says nothing: NaN and infinity go into `llList2Json` as strings, a malformed object or array gives `[JSON_INVALID]` from `llJson2List`, a whole-number value too large for an integer comes out of `llJson2List` as a float, and nesting deeper than 100 levels counts as invalid JSON.
- `llListRandomize` uses the sandbox's random numbers, the same in every run, so a test gets the same order each time; a stride under 1 counts as 1, and a list whose length is no multiple of the stride comes back unchanged (the wiki).
- `llLog` gives 0 for a value of 0 or less, `llAcos` gives NaN outside -1..1, `llRot2Axis(ZERO_ROTATION)` gives `ZERO_VECTOR`, and `llAngleBetween` is 0 to PI (the wiki's reference code).
- `Parser::eventIsRaised` already listed `transaction_result`; nothing changed in the parser. Tests: `tests/tst_lslpure.cpp`.

## What exists now (step 9b-9): HTTP requests with canned replies

- `llHTTPRequest` runs against replies given in the new `http` option of `lsl_run` (URL, method, status, body); the sandbox never goes on the network. A request without a matching reply gets status 499.
- The `http_response` event comes at once in every script of the requesting prim; long bodies are cut at `HTTP_BODY_MAXLENGTH` with `HTTP_BODY_TRUNCATED` in the metadata, and the throttle of 25 requests per object in 20 seconds applies. Each request is an `http_request` log entry. Details: [CONTROL.md](CONTROL.md), "HTTP requests with canned replies". Tests: `tests/tst_lslhttp.cpp`.

## What exists now (step 9b-8): record-only stubs and animation overrides

- 37 more functions for sound, texture animation, camera, particles, look-at, pay price and click action only write a `recorded_only` entry; nothing is played or drawn.
- `llSetAnimationOverride`, `llResetAnimationOverride` and `llGetAnimationOverride` keep the overrides of the script and log an `animation_override` entry; they need `PERMISSION_OVERRIDE_ANIMATIONS`. Details: [CONTROL.md](CONTROL.md), "Recorded only". Tests: `tests/tst_lslstubs.cpp`.

## What exists now (step 9b-7): sensors

- `llSensor`, `llSensorRepeat` and `llSensorRemove` scan for the avatars of the new `avatars` option of `lsl_run` (positions given in the input; the owner stands at the avatar's place) and for the other objects of the world,
  by name, key, type, range (96 m at most) and arc; the events `sensor` and `no_sensor` follow, and `llDetectedPos`, `Rot`, `Vel`, `Type`, `Group` and `Owner` join `llDetectedKey` and `llDetectedName`.
- Avatars never move and nothing is drawn. Details: [CONTROL.md](CONTROL.md), "Sensors". Tests: `tests/tst_lslsensors.cpp`.

## What exists now (second gap list, step 4): money, web server and mail stubs

- Nine more functions run: `llGiveMoney`, `llEmail`, `llGetNextEmail`, `llRequestURL`, `llRequestSecureURL`,
  `llReleaseURL`, `llGetFreeURLs`, `llHTTPResponse` and `llGetHTTPHeader`. The events `money`, `http_request` and
  `email` are raised now, so `lsl_check` no longer warns about them.
- **Nothing real happens:** no L$ moves, no socket is opened and no mail is sent. Outgoing calls are log entries with
  their exact arguments (`give_money`, `email_sent`, `url_request`, `url_release`, `http_server_response`). A URL is a
  fake one on the host `sandbox.invalid`, which never resolves, made the same way in every run; the world has 38.
  `llEmail` makes the script sleep 20 simulated seconds, as in Second Life.
- **Inputs from outside:** `lsl_run` inputs and `lsl_event` have three new types: `money` (an avatar pays the
  object), `http_request` (a request to a URL a script got, with path, query, body and headers) and `email` (an email
  arrives in the object's mailbox for `llGetNextEmail`). Details: [CONTROL.md](CONTROL.md), "Money, web server and mail".
- Second Life rules kept: the 25 seconds to answer a request and the 30 seconds its headers can be read, 64 waiting
  requests per script (then 503), a body cut to 2048 bytes and header values to 255, lower-case header names, a
  query that needs a slash after the URL (else 500), `URL_REQUEST_DENIED` with an empty body, and URLs freed by a
  reset, `llDie` and a region change or restart.
- Doubts and what is not done: [CONTROL.md](CONTROL.md), the end of "Money, web server and mail".
- Tests: `tests/tst_lslmoney.cpp` (exact answers for every function, input and error) and the scenario
  `lsl_money_web_mail`. The test of the number of functions that run now expects 255.

## What exists now (second gap list, step 5): prim media

- Three more functions run: `llSetPrimMediaParams`, `llGetPrimMediaParams` and `llClearPrimMedia`. Record only:
  nothing is fetched or played, no connection is opened, and the media never "loads". The media of a face is kept in
  the world state of the prim the script is in (faces 0 to 5), so a script reads back what it set; every call is a
  `prim_media` log entry with its exact arguments. No forced delay is simulated (Second Life makes the script sleep 1
  second in each).
- Second Life rules kept: the 15 `PRIM_MEDIA_*` rules, a wrong face, odd rule list, unknown rule, wrong value type
  or value out of range answered with `STATUS_BOUNDS_ERROR`, `STATUS_MALFORMED_PARAMS` or `STATUS_TYPE_MISMATCH`
  (nothing is stored then), `STATUS_WHITELIST_FAILED` for an address the whitelist does not allow, and an empty list
  from `llGetPrimMediaParams` for a face without media. Details and the doubts: [CONTROL.md](CONTROL.md), "Prim media".
- Tests: `tests/tst_lslmedia.cpp` (exact answers) and the scenario `lsl_prim_media`. The test of the number of
  functions that run now expects 258.

## What exists now (second gap list, step 3): region and avatar reads

- 12 more functions run: `llGetRegionName`, `llGetRegionCorner`, `llGround`, `llWater`, `llGetAgentSize`,
  `llGetAgentList`, `llRequestAgentData`, `llGetSunDirection`, `llGetTimeOfDay`, `llGetTimestamp`, `llGetWallclock`
  and `llGetDate`.
- **Canned region:** the sandbox has one region with settings: name `Sandbox`, corner `<256000, 256000, 0>`, flat
  ground and water at 20 m, a day of 4 hours that starts at noon, and the start time 2026-01-01 00:00:00 UTC. `lsl_run`
  can give other values with `region` (`name`, `corner`, `ground`, `water`, `day_length`, `time_of_day`, `unix_start`),
  and an avatar can give `height`. Details: [CONTROL.md](CONTROL.md), "The region and the avatars in it".
- **Time is never real:** every clock function (also `llGetUnixTime` and the creation time in `llGetObjectDetails`)
  reads the start time plus the simulated seconds, so every run gives the same answers and the tests never depend on
  the computer's clock. `llGetWallclock` is Second Life time (US Pacific, with the US summer-time rule).
- Doubts, written down instead of guessed (the sandbox chose): the sun's path (a simple circle from east to west
  through the top; Second Life's sun leans to one side and the sky can be set per parcel), the avatar width and depth
  of `llGetAgentSize` (0.45 and 0.6 m), `DATA_BORN` (the start date of the run), `DATA_PAYINFO` (`"0"`), no answer for
  an unknown `DATA_*` number, the order of `llGetAgentList` (the owner, the listed avatars, then sitters), and that the
  three scopes give the same list because the sandbox is one parcel.
- Not done here: `llGetRegionFPS`, `llGetRegionTimeDilation`, `llGetSimulatorHostname`,
  ground that is not flat (`llGroundNormal`, `llGroundSlope`), the forced delay of `llRequestAgentData` (0.1 s).
- Already in place: the checker knows that the sandbox raises `transaction_result` (`Parser::eventIsRaised`); this step
  adds a test for it.
- Tests: `tests/tst_lslregion.cpp` (exact answers for every function, also the summer-time changes and a day that
  wraps round) and the scenario `lsl_region_reads`. The test of the number of functions that run now expects 246.

## What exists now (second gap list, step 2): reads and state of the object itself

- 17 more functions run: `llDie`, `llGetVel`, `llGetMass`, `llSetScale`, `llSetLocalRot`, `llGetLinkName`, `llGetLinkKey`,
  `llDetectedLinkNumber`, `llGetObjectPermMask`, `llGetInventoryPermMask`, `llGetObjectDetails`, `llSameGroup`,
  `llSetScriptState`, `llGetScriptState`, `llResetOtherScript`, `llGetCreator` and `llGetUsername`.
- **`llDie`, the end state chosen:** the calling script stops at the call (nothing after it runs) with a `die` log entry
  (with line and column); then the object is deleted: every other script of the object stops with its own `die` entry,
  the avatars sitting on it stand up, and the answer's `objects` entry gets `deleted: true` (with `stopped: true` and
  the stop reasons). A deleted object is no longer heard, found by sensors, named by `llKey2Name`, known to
  `llGetObjectDetails` or a target for `llGiveInventory`, and an input sent to it is an `input` entry with the error
  "The object was deleted with llDie.". In a worn object `llDie` does nothing and the script runs on (Second Life wiki).
  Second Life may let the script run a little after the call; the sandbox stops it at once, so a script that relies on
  code after `llDie` fails here first.
- **Not running scripts:** `llSetScriptState(name, FALSE)` on another script of the same prim keeps its variables,
  state, listens and timer, but it gets no events: the waiting ones are dropped, new ones too, and its timer and
  sensor wait; set running again, its timer and sensor count their interval from that moment. A script can stop
  itself (the rest of the event runs, then nothing more) but not start itself. The answer marks such a script
  `running: false`. A script that stopped (an error or a limit) cannot be started or reset this way, as in Second Life.
- What is canned (the same in every run) and every choice: [CONTROL.md](CONTROL.md), "Reads and state of the object itself".
- Doubts, written down instead of guessed (the Second Life wiki does not say, and the sandbox chose):
  the text of the debug line for a missing script (`llSetScriptState`, `llGetScriptState`, `llResetOtherScript`) is
  the sandbox's own; the one for `llGetInventoryPermMask` (`No item named '...'`) is from memory of Second Life, not
  from the wiki; whether a script that is not running keeps the events waiting for it (the sandbox drops them);
  `llSetLocalRot` in the root prim (the sandbox treats it as `llSetRot`); the default next-owner mask (the sandbox
  gives `PERM_MOVE | PERM_TRANSFER`, 532480); the mass formula (a box of 10 lindograms per cubic metre; Second
  Life's mass also depends on the shape, hollow, cut and physics shape, which the sandbox does not have), and a worn
  object's `llGetMass` (Second Life gives the avatar's mass, which is not known here, so the sandbox gives the
  object's own); the values `llGetObjectDetails` gives for the costs, the avatar-only flags and the creation time.
  The brief for this step said that unknown `llGetObjectDetails` flags give an empty list; the wiki says each unknown
  flag gives `OBJECT_UNKNOWN_DETAIL` (-1) in the list (an unknown key gives an empty list), and the sandbox follows
  the wiki.
- Not done here: the keys of child prims (from `llGetLinkKey`) are not known to
  `llGetObjectDetails`; `llSameGroup` in an attachment does not use the owner's active group; the forced delay of
  `llSetLocalRot` (0.2 s) is not simulated, as for the other prim setters.
- Tests: `tests/tst_lslownobject.cpp` (exact answers for every function) and the scenarios `lsl_own_object_reads` and
  `lsl_own_object_state`. The tests that used `llGetMass` as the example of a function that does not run yet now use
  `llGetEnergy`.

## What exists now (second gap list, step 1): touch and touch_end

- A touch now raises all three touch events of Second Life: `touch_start`, then `touch` while the touch is held (once
  for a click, then every 0.1 simulated seconds), then `touch_end` when it is let go. `num_detected` is 1, and
  `llDetectedName`, `llDetectedKey`, `llDetectedOwner` and `llDetectedLinkNumber` work in all three.
- New input option `hold` (seconds, 0 to 60, default 0, a click) for a touch, in the `inputs` of `lsl_run` and
  `lab_watch` and in `lsl_event`. A touch held past the end of a run goes on in the next `lsl_event`. The checker no
  longer warns that `touch` and `touch_end` are never raised: 20 of the 43 events are raised now.
- A prim with any touch handler keeps its touches; only a prim with none passes them to the root.
- Choices and what is not done (the touch data `llDetectedTouch*`, several avatars touching at once, holding a touch
  from the Live page): [CONTROL.md](CONTROL.md), "Touches". Tests: `tests/tst_lsltouch.cpp` (exact event times for a
  click, a held touch and a release on a repeat, routing to the prim, `lsl_event`, a state change while held, refused
  values) and the scenario `lsl_touch_phases`.

## Camera limits in the 3D view (job 7b)

The camera restrictions a script holds can limit the main 3D view's camera, only while the Commands page's **Show
camera limits in 3D view** box is ticked (under the Actions list). The box is off each time the program starts. The
Commands page now scrolls when the panel is small, as the Live page does.

- What is shown: `setcam_avdistmin` (and the older name `camdistmin`): the camera stays at least this many metres
  from the avatar; `setcam_avdistmax` (`camdistmax`): at most this far; `setcam_unlock` (`camunlock`): the camera
  focus stays on the avatar. With several objects the strictest limit wins (the largest minimum, the smallest
  maximum); a maximum below the minimum wins over it. A maximum smaller than the 3D view can come (3 inches) is shown
  as close as it goes. An option that is not a distance in metres is named in the text as not used.
  `setcam_avdistmin`, `setcam_avdistmax` and the old names `camdistmin` and `camdistmax` work in both modes.
- Field of view (part 2): `setcam_fovmin` and `setcam_fovmax` keep the 3D view's vertical field of view at least or at
  most this many radians (the view's own is 60 degrees); the old zoom limits are a multiplier of the default 60
  degrees: `camzoommax:2` keeps it at 30 degrees or more (no zooming in past 2x), `camzoommin:2` at 30 degrees or less.
  The view shows its own 60 degrees moved into the limits (strictest wins, a maximum below the minimum wins over it,
  1 to 170 degrees). A field of view limit alone does not move the camera.
- Offsets (part 3): `setcam_focusoffset:x/y/z` makes the camera look at the avatar's middle moved by this many metres
  (Second Life axes: x forward, y left, z up); `setcam_eyeoffset:x/y/z` (times `setcam_eyeoffsetscale`) holds the
  camera at the avatar's middle moved by this offset, looking at the focus: turning and zooming then do nothing, only
  the distance limits still hold along that line. With several objects holding an offset, the first one in the
  restriction list is used. An offset that is not three numbers of at most 100 m, or a scale that is not a number
  above 0, is named as not used; a scale without an eye offset changes nothing and the text says so.
- While a distance limit, an offset or `setcam_unlock` holds, the camera turns around the avatar's middle (its hip at rest; it does not follow the
  animation) and panning cannot move it away; turning, zooming within the limits, the number pad views, `set_camera`
  and the AI's `screenshot` keep to the limits.
- The text under the box says in words what limits the camera, for example "Camera limited by the script: max
  distance 3 m." and which objects hold it; with the box off it says that the script limits the camera and that
  ticking the box shows it.
- The user's camera is never changed for good: when the limits come on, the camera is kept as it was; unticking
  the box, `@clear`, Reset, a mode switch or the last limit going away puts exactly that camera back. Moves made
  while the limits were on are not kept. Nothing is saved.
- Force commands (job 7c, part 1): a force command that changes a recorded value now tells the 3D view (the RLV
  state sends the same change signal as for a restriction). With the box ticked, `setcam_fov:<radians>=force` is
  the field of view the view starts from instead of its own 60 degrees (the min and max limits still hold over it);
  `setcam_focus:<avatar key>=force` (the sandbox avatar's key is the one `llGetOwner` gives; an optional
  `;distance;direction` is ignored) keeps the camera focus on the avatar; the offsets sent as `=force`
  (`setcam_focusoffset`, `setcam_eyeoffset`, `setcam_eyeoffsetscale`) are used as above when no object holds that
  offset as a restriction, and the text marks them "(forced)". Another focus target is named as not shown (the 3D
  view has only the avatar); `setcam_mode=force` (for example mouselook) is named as not shown, because the 3D view
  always looks from outside the avatar; mouselook (`setcam_mouselook`) stays recorded only. A forced value belongs
  to the object that sent it: another force command changes it, and that object's `@clear` (no filter, or a filter
  that `behaviour:option` contains), Reset or a mode switch forgets it, so the camera from before comes back
  (decision in [DECISIONS.md](DECISIONS.md)).
- Code: `src/scriptlab/cameraoverride.*` (the limits read from the RLV state and the words), the constraint in
  `src/camera.*`, the box in `src/scriptlab/scriptlabpanel.*`, a few lines in `src/animationview.cpp`. Tests:
  `tests/tst_cameraoverride.cpp` (with the force commands), `tests/tst_scriptlabcamera.cpp` and the constraint, field of view and eye part of
  `tests/tst_camera.cpp`.

## Environment in the 3D view (job 7c)

The 3D view has no sky and no water: a plain background and fog towards the background colour. With the Commands
page's **Show environment in 3D view** box ticked (under the camera box; off each time the program starts), what a
script asks of the sky and the draw distance is shown as a rough hint in that background and fog:

- `setenv_daytime:<0..1>=force`: the background takes a sky colour for the time of day (the RLV API's points:
  midnight 0, sunrise 0.25, midday 0.567, sunset 0.75; four fixed colours with straight blends between them, not
  Second Life's sky). `-1` is the region's own sky and changes nothing.
- `setenv_bluehorizon:<r/g/b>=force` (or the old one-part names `setenv_bluehorizonr`, `_g`, `_b`; a part not given is
  0): the background takes that colour (each part 0 to 1); it wins over the time of day. The fog follows the
  background, as in the view's own look, and keeps the user's fog switch and density.
- `camdrawmax:<metres>` with `camdrawalphamax` (default 1) and `camdrawcolor` (default black) held as restrictions
  (RLV only, so plain RLV mode: `rlv_reset` with `mode` `rlv`): fog of that colour that reaches the alpha at that distance from the camera (exponential, as the view's
  own fog; an alpha of 1 is shown as 0.99); the background beyond takes the colour at that alpha. With several
  objects the smallest distance and the largest alpha win, the colour of the first one. `camdrawmin` and
  `camdrawalphamin` are named as not shown (the fog starts at the camera).
- `setsphere` (held; RLVa only): the vision sphere, a whole-picture effect (part 1 of the job asked for by the project
  owner on 23 November 2026, 11:03: «we should have that, but that should be just made simple»). The drawn picture
  is read back with its depth, each pixel's distance from the avatar's hip (or from the camera with
  `setsphere_origin:1`) is worked out, and the effect is put on the picture as RLVa's sphere shader does it: the
  strength runs in a straight line from `setsphere_valuemin` at `setsphere_distmin` to `setsphere_valuemax` at
  `setsphere_distmax` (metres); below the inner distance it is 0 and beyond the outer one it stays at `valuemax`
  (`setsphere_distextend`: 1 the default, 2 keeps `valuemin` inside, 3 both, 0 neither). Empty background counts as
  far away. RLVa's defaults are used for a value not sent: mode 0, origin 0, both distances 0, both strengths 1,
  colour black, so `@setsphere=n` alone turns everything beyond the hip black, as in RLVa. The values take only
  `=force` (`=n` is refused, as in RLVa). Modes (`setsphere_mode`), each a simple version; the final check is in
  Second Life:
  - 0, colour fade: each pixel moves towards the colour by the strength. The colour is `setsphere_param:<r>/<g>/<b>`
    (each 0 to 1) or `setsphere_color:<r>/<g>/<b>`; RLVa keeps both in one value, and here `setsphere_param` wins
    when both were sent.
  - 1, blur: the picture blurred by about 6 pixels, mixed in by the strength (up to 1).
  - 2, blur with a size: blurred by half of `setsphere_param`'s first number in pixels (1 to 64), wherever the
    strength is above 0.
  - 4, pixel: blocks of `setsphere_param:<width>/<height>` pixels (1 to 64), wherever the strength is above 0; with
    no size the blocks are 1 x 1 and nothing changes (the text under the box says so).
  - 3, the colour split (chromatic aberration), is named as not shown. `setsphere_tween` is named as not shown (the
    view changes at once). A value that is not a usable number is named as not used and its default is kept.
  The settings are one small struct (`EnvironmentOverride::Sphere`), which the panel's sliders (below) drive too.
  camdraw (RLV only) and setsphere (RLVa only) are never known in the same mode.
- **Vision sphere sliders** (part 2, the same request of 23 November 2026: «we should have that, but that should be
  just made simple»): a folding part "Vision sphere" under the environment box on the Type commands page, folded at
  start. Sliders for the inner distance and the outer distance (0 to 20 m in steps of half a metre; each pushes the
  other so the inner one is never past the outer one), Darkness (0 to 100 percent, the strength at the outer
  distance; inside the inner distance the view stays clear, and the strength grows in a straight line between them)
  and a colour button that opens Qt's own colour dialog (each part kept in hundredths, 0 to 1). The mode is always
  the colour fade (0). Under them, the finished RLV line in a read-only field, all of it shown (it wraps), with Copy
  (the line on the clipboard; the button then reads "Copied" until a value changes) and Clear. The line is what a
  script sends with llOwnerSay, for example `@setsphere=n,setsphere_mode:0=force,setsphere_distmin:1=force,`
  `setsphere_distmax:4=force,setsphere_valuemin:0=force,setsphere_valuemax:1=force,setsphere_param:0/0/0=force`
  (one line, no spaces): the restriction, then each value with `=force`, the colour as `setsphere_param`. The note
  under it says "RLVa only". The rules above are not changed: `=force` only, `=n` refused, RLVa only.
- Moving a control sets the same sphere the script's `@setsphere` sets (`EnvironmentOverride::setTrySphere`), so
  the 3D view changes at once. While it is set it is shown instead of the script's sphere, and the text under the
  box says so ("Vision sphere from the sliders: ..."; with a script's sphere held too: "The sliders' sphere is shown
  instead of the script's. Press Clear to show the script's again."); the part's count reads "sliders on" while the box is ticked. Clear
  takes it off the view (the sliders keep their values). It follows the box and the panel like the script's
  sphere: with the box off every control is greyed with the reason "Tick Show environment in 3D view to try the
  sphere." and nothing reaches the picture; hiding the panel turns it off.
- Code: `src/scriptlab/spherecontrols.*`; test `tests/tst_spherecontrols.cpp` (the exact line for slider and colour
  values, the line sent as a script gives the same sphere back, a slider move changes exact pixels of the picture,
  greyed with the box off, Clear, Copy, no user path in any text, the tab order, and pictures dark and light at 100
  to 200 percent text without clipping).
- Any other `setenv_<name>` (presets, sun, moon, clouds, haze, water) is named as not shown; an option that is not
  a usable number or `r/g/b` colour is named as not used.
- The text under the box says what is shown ("Environment set by the script: sky colour 0.8/0.55/0.45 from daytime
  0.25."), that it is a rough hint, which objects set it, and what is not shown. With the box off it says what the
  script changes and that ticking the box shows a hint.
- The view's own look is never changed: the background and fog are worked out again for every picture, so unticking
  the box, `@clear` (forced values belong to their object), Reset or a mode switch gives exactly the picture from
  before (checked pixel by pixel in the self-test).
- The vision sphere follows the same box and the same rules: off until it is ticked, off while the Script Lab panel
  is hidden and back as ticked when it is shown again, and a picture without it is left byte for byte as it was
  (checked in the tests, and in the real program by comparing screenshots).
- Code: `src/scriptlab/environmentoverride.*` (the values, `Sphere::strengthAt` and `applySphere`, the picture
  work), the box in `src/scriptlab/scriptlabpanel.*`, a few lines in `src/animationview.cpp` (`draw()` and
  `drawSphere()`). Tests: `tests/tst_environmentoverride.cpp` and `tests/tst_scriptlabcamera.cpp`.

## View modes in the 3D view (job 7c)

With the Commands page's **Show view modes in 3D view** box ticked (under the environment box; off each time the
program starts), the view restrictions a script holds change the 3D view where that takes little drawing work:

- `showself` (RLVa only): your own avatar is hidden: the figure, its shadow and the Script Lab markers are not drawn
  (and cannot be clicked).
- `setcam_textures` / `camtextures` (option: a texture UUID): the floor's checkerboard, the only texture-like pattern
  in the view, is drawn in one plain colour (the middle of its two tile colours, red while the first frame is
  protected); the figure has no textures. A texture cannot be loaded here; its UUID is named.
- `setoverlay` (RLVa only) with `setoverlay_texture`, `setoverlay_alpha` and `setoverlay_tint` (sent as `=force`, the
  only way RLVa takes them; the last value wins; defaults tint 1/1/1 and alpha 1): the picture cannot be loaded here, so the whole 3D view is covered by
  the tint at the alpha. Without `setoverlay_texture` nothing is shown (as an overlay without a picture), and the text
  says so; `setoverlay_tween` is named (the overlay changes at once).
- Named as not shown: `showselfhead` (the figure's head cannot be hidden on its own), `viewwireframe` and
  `viewtransparent` (they block switching to a wireframe view or a transparent-object highlight, which the 3D view
  does not have, so blocking them changes nothing).
- The text under the box says what is changed, which objects hold it and what is not shown; with the box off it says
  what the script changes and that ticking the box shows it.
- Nothing is changed for good: the view asks for the modes for every picture, so unticking the box, `@clear`, Reset
  or a mode switch gives exactly the picture from before (checked pixel by pixel in the self-test). The control
  command `screenshot` shows the same picture as the window.
- Code: `src/scriptlab/viewmodesoverride.*`, the box in `src/scriptlab/scriptlabpanel.*`, a few lines in
  `src/animationview.cpp` (`drawAnimations()`, the floor hook, `drawOverlay()`). Tests:
  `tests/tst_viewmodesoverride.cpp` and `tests/tst_scriptlabcamera.cpp`.

## Static warnings (lint)

The control command `lsl_lint` looks at a script without running it and warns about valid LSL that is most likely a
mistake: variables and parameters that are never read, functions that are never called, a variable that hides
another, code after `return`, `jump` or `state`, `if (a = b)`, `if (a);`, `a = a;`, `==` between two lists (it only
compares lengths), division by a literal 0, deprecated functions, and RLV commands in `llOwnerSay("@...")` that the
sandbox's RLV parser refuses or does not know (also the constant start of text built with `+`). Every warning has a
line, a column, a stable id and a short message; the ids and the exact rules are in [CONTROL.md](CONTROL.md),
"Script Lab: lsl_check and lsl_run". A script that does not compile gets its errors (as `lsl_check`) and no warnings.
The example scripts and the fuzz corpus give no warnings (the test `tests/tst_lsllint.cpp` checks it). Code:
`src/scriptlab/lsl/lint.*`. The panel's Live scripts page shows them under Errors and warnings, after the checker's own.

An assistant can run `lsl_lint` after `lsl_check` passes and before `lsl_run`:

```
{"command": "lsl_lint", "source": "default { state_entry() { integer a; } }"}
                                  -> warnings: [{"line": 1, "column": 35, "id": "unused-local", "message": "The local variable a is never used."}]
```

## How an AI assistant uses it

Claude (or any client) writes an RLV line or a script, sends the line with `rlv_command`, reads `rlv_state`, fixes the
line and sends it again. Example:

```
{"command": "rlv_command", "object": "collar", "text": "@detach=n,sendim=n"}
{"command": "rlv_state"}          -> blocked: ["detach", "sendim"]
{"command": "rlv_command", "object": "collar", "text": "@clear"}
```

With the Script Lab panel open, the project owner sees the same restrictions and actions the assistant just set, so
both look at one state.

With scripts on disk (step 6): `lab_watch` the folder once, then edit a `.lsl` file with the assistant's own file
tools, call `lab_result` until `runs` goes up, read the errors (line and column) or the log, fix the file and save
again. No copying and pasting of scripts; the panel's Live page shows the same result to the user.

For LSL: write the script, `lsl_check` it, fix each error at its line and column, `lsl_run` it with inputs (chat or
touches at given times), read the log (what it said, its listens, timers and animations), fix and run again:

```
{"command": "lsl_check", "source": "default { state_entry() { llOwnerSay(1); } }"}
                                  -> errors: [{"line": 1, "column": 38, "message": "Argument 1 of llOwnerSay must be a string, not an integer."}]
{"command": "lsl_run", "source": "...", "seconds": 5, "inputs": [{"at": 1, "channel": 7, "text": "dance"}]}
```

A script can use RLV: `llOwnerSay("@detach=n,sendim=n")` runs in the RLV core, `llOwnerSay("@version=2222")` is
answered in the script's `listen` event on channel 2222. The log shows each command with its results, and the answer
has `rlv.state` (the restrictions at the end, as `rlv_state` shows them); the same state is what the panel shows.
Examples: `examples/scriptlab/rlv_lock.lsl` and `rlv_status.lsl`.

## HUD workflow (what works today)

A script-driven HUD is developed in two parts that the sandbox treats differently.

1. **One script at a time, live.** Keep the HUD script in the folder that Script Lab watches (Live page, or `lab_watch`). Each save checks and runs it again; the Live page and `lab_result` show errors with line and column, the log (what it said, its dialogs, listens and timers) and the RLV state at the end. The `inputs` option of `lab_watch` gives every run the chat lines and touches to test with; each run starts fresh. Claude edits the file with its own file tools and reads `lab_result`.
2. **HUD and the thing it controls together.** For a HUD that talks to cuffs or a collar on a channel, send both scripts in one `lsl_run` with `objects` (the HUD as worn, the other object rezzed). Then `lsl_event` sends a touch or a dialog button to that world and runs it on, and `rlv_state` or the panel's Objects part shows who holds which restriction. Example: `examples/scriptlab/hud_cuffs_hud.lsl` with `hud_cuffs_cuffs.lsl`. The folder watch runs each `.lsl` file on its own, so it does not do this part.
3. **A HUD of several scripts.** When the HUD itself has several scripts (for example a menu script and a main script that talk with `llMessageLinked`, or settings kept in Linkset Data), give that object `scripts` instead of `source` in `lsl_run` (step 9a, above); each log line names the script.

What it checks: that the buttons (touch, dialog buttons, chat on a channel) cause the right messages and RLV commands. What it does not do: show how the HUD looks (textures, layout, buttons on screen are not previewed), or reproduce Second Life exactly. The texture is uploaded in Second Life by the project owner, and the final test is in Second Life. The Live page has buttons for sending a touch, a chat line and a dialog answer (below, "Input buttons"); the same inputs also go through `inputs`, `lsl_run` and `lsl_event`.

## RLV and RLVa mode (step 9, issue 297; source check 22 Nov 2026)

RLVa (the RLV of Firestorm) is the default standard. Every behaviour in the table is marked `RLV` (in RLVa and in the
RLV wiki's specification, "Restrained Love viewer v2.9 Specification"), `RLVa only` (in RLVa, not in the wiki's
specification: it does not work in Kokua or another plain RLV viewer) or `RLV only` (in the wiki, but RLVa does not
have it). A behaviour both have can still have a parameter type only RLVa takes (the "does not work in Kokua" mark on
a type): `fly=force` and `addoutfit:<folder>=force` (another name of `attach`).

The marks are not set by hand. `tools/rlv_sources.py` makes two data files: `tests/data/rlv/rlva.tsv` from RLVa's
behaviour dictionary (`indra/newview/rlvhelper.cpp` in a Firestorm checkout) and `tests/data/rlv/rlv_wiki.tsv` from
the command syntax lines of the wiki page (`tests/data/rlv/rlv_wiki_capture.txt`). The test `rlvsources`
(`tests/tst_rlvsources.cpp`) fails on any difference between them and the table: a name missing on either side, a
parameter type (`n`/`y`, `=force`, `=<channel>`) that differs from RLVa's (or from the wiki's for an `RLV only` name),
or a mark that does not follow from where the name is. RLVa makes a `_sec` form of every behaviour it flags strict,
and the table has them all. Commands the wiki lists under "Unofficial Commands" (`allowidle`) do not count as RLV.
Aurora, LibreMetaverse and other viewers are not checked. To check a newer RLVa:
`python3 tools/rlv_sources.py rlva <firestorm>/indra/newview/rlvhelper.cpp --source "<where it came from>"`, then
run the test and fix the table where it fails.

- The mode is RLVa at start. In RLVa mode an `RLV only` behaviour (`camdrawmin`, `camdrawmax`, `camdrawalphamin`,
  `camdrawalphamax`, `camdrawcolor`, `defaultwear`, `denypermission`, `versionnumbl`, `getblacklist`, `getcam_zoommin`,
  `setrot`) answers as unknown, like any name the table does not have (it is also left out of `getcommand` there).
  Plain RLV mode (the Kokua check) works the other way round: an `RLVa only` behaviour answers as unknown and an
  RLVa-only parameter type answers "does not take". `rlv_state` lists `rlva_only`; the `RLV only` names are the ones
  in `behaviours` of plain mode that are missing in RLVa mode. Switching the mode removes every restriction and
  recorded effect (as after a relog). Set it with the control command `rlv_reset` and its `mode` field
  ([CONTROL.md](CONTROL.md)).
- Behaviours marked RLVa only (62): interaction and view (`showself`, `showselfhead`, `viewtransparent`,
  `viewwireframe`, `allowidle`, `buy`, `pay`, `jump`), camera (`setcam`, `setcam_avdist`, `setcam_eyeoffset`,
  `setcam_eyeoffsetscale`, `setcam_focusoffset`, `setcam_mouselook`, `setcam_origindistmin`, `setcam_origindistmax`,
  and the force commands `setcam_focus` and `setcam_mode`), the vision sphere and the overlay (`setsphere` and its
  ten `setsphere_<name>` values, `setoverlay`, `setoverlay_alpha`, `_texture`, `_tint`, `_tween`, `_touch`), more
  secure forms (`accepttp_sec`, `accepttprequest_sec`, `recvchatfrom_sec`, `recvemotefrom_sec`, `recvimfrom_sec`,
  `sendimto_sec`, `startim_sec`, `startimto_sec`, `sendchannel_except_sec`, `shownametags_sec`), the `addoutfit`
  names of the shared folder force commands (`addoutfitall`, `addoutfitover`, `addoutfitallover`, `addoutfitthis`,
  `addoutfitallthis`, `addoutfitthisover`, `addoutfitallthisover`) and questions (`getcommand`, `getheightoffset`,
  `getaddattachnames`, `getaddoutfitnames`, `getattachnames`, `getoutfitnames`, `getremattachnames`,
  `getremoutfitnames`, `getcam_avdist`, `getcam_textures`).
- RLVa flags some names as extended or experimental that the wiki lists as plain RLV; they are marked `RLV`
  (`accepttprequest`, `findfolders`, `getcam_avdistmax`, `getcam_avdistmin`, `getcam_fov`, `getcam_fovmax`,
  `getcam_fovmin`, `interact`, `sendchannel_except`, `sendgesture`, `setcam_avdistmax`, `setcam_avdistmin`,
  `setcam_fov`, `sharedunwear`, `sharedwear`, `shownearby`, `touchhud`, `tplocal`, `tprequest`).
- Plain RLV behaviours (marked `RLV`) are known in both modes. Those with effects outside the sandbox: `setenv`,
  `setdebug`, `setcam_fovmin`, `setcam_fovmax`, `setcam_textures`, `setcam_unlock`.
- Old names (aliases) share the state of the new name: `camdistmax` (`setcam_avdistmax`), `camdistmin`
  (`setcam_avdistmin`), `camavdist` (`setcam_avdist`), `camtextures` (`setcam_textures`), `camunlock`
  (`setcam_unlock`) and `fartouch` (`touchfar`). Holding one is holding the other (the same restriction under both
  names is a duplicate, `=y` under either name lifts it, the action it blocks is blocked), and `@getstatus` shows the
  name that was used. `camzoommax` and `camzoommin` (the old zoom limits, option: a zoom multiplier) are separate
  entries, recorded only, because a multiplier is not a field of view angle. `touchfar` (option: distance in metres)
  blocks the action "Touch objects farther away".
- Chat and instant messages (all marked `RLV`). Restrictions are `n` or `y`, exceptions `add` or `rem`; a restriction
  with an option (a UUID or a channel) is an exception or a limited form and blocks nothing in the Allowed / Blocked list.
  Chat: `chatshout`, `chatnormal`, `chatwhisper` (the voice is changed, not blocked), `redirchat` and `rediremote`
  (option: channel), `emote` (the exception form: emotes stay allowed), `recvchat_sec`, `recvchatfrom`, `recvemote`,
  `recvemote_sec`, `recvemotefrom`, `sendchannel_sec`. Instant messages: `sendim_sec`, `sendimto`, `startim`,
  `startimto`, `recvim_sec`, `recvimfrom`. Other secure forms: `share_sec`, `tplure_sec`, `tprequest_sec`,
  `shownames_sec`. The `_sec` form blocks the same action as the plain one, but it is a separate restriction (it is not
  a duplicate of the plain one) and the sandbox does not model how exceptions of different objects combine with it;
  the same goes for `permissive`, which is only held as state.
- `@notify:<channel>[;filter]=add` (`rem` removes it): from then on the object hears `/<rule>=n` on that channel each
  time it adds a restriction and `/<rule>=y` each time it lifts one (also for each restriction `@clear` removes; the
  rule is written with the name that was used, and the option if there is one). A duplicate or an unheld `=y` says
  nothing, and changes of `notify` itself are not announced. The filter must be part of the rule name. Channel 0, a
  negative number, a decimal or text as channel are refused. A full `@clear` removes the notification too, so it hears
  nothing about the rest. In the sandbox a notification only tells about changes made by the object that holds it (a
  limit of the sandbox: answers go to the object that ran the line). One line makes at most 256 answers.
- Outfits and attachments (marked `RLV` unless stated). The sandbox has no avatar and no inventory, so a `=force`
  command is accepted and answered `retained` (it is not carried out and nothing is held): `detachme`, `remattach`,
  `remoutfit`, the shared-folder commands `attach`, `attachover`, `attachoverorreplace`, `attachall`, `attachallover`,
  `attachalloverorreplace`, `detachall`, and the "this" family `attachthis`, `attachthisover`,
  `attachthisoverorreplace`, `attachallthis`, `attachallthisover`, `attachallthisoverorreplace`, `detachthis`,
  `detachallthis`. Restrictions (with an option they are an exception or a limited form and block nothing):
  `addattach`, `remattach` (option: attachment point), `addoutfit`, `remoutfit` (option: layer), `unsharedwear`,
  `unsharedunwear`, and `attachthis`, `attachallthis`, `detachthis`, `detachallthis` (option: folder). Their
  `_except` forms (`attachthis_except` and the other three; option: folder) only hold the exception. `defaultwear`
  (`RLV only`) is held as state and blocks nothing.
- Touch, view and money (all `RLV`): `touchworld`, `touchattach`, `touchattachself`, `touchattachother` and `touchfar`
  block their own action in the Allowed / Blocked list; `touchthis` and `touchme` only hold the exception;
  `showhovertextall`, `showhovertexthud`, `showhovertextworld` (and `showhovertext:<uuid>` for one object), `shownames`,
  `shownametags`, `viewnote`, `viewscript`, `viewtexture`, `editworld`, `editattach` (and `editobj:<uuid>`), `buy`
  and `pay` (`buy` and `pay` are RLVa only). `acceptpermission` and `denypermission` (`RLV only`) are held as state: the sandbox grants every
  permission to a script anyway.
- Group, movement and teleport (all marked `RLV` except `setrot`): `setgroup` (restriction; `=force` with a group
  name or `none` is retained), `temprun`, `alwaysrun` and `jump` (RLVa only) block their own action, `standtp` and `accepttp` are
  held as state (the sandbox has no avatar position and no teleports), and `adjustheight`, `tpto`, `sitground` and
  `setrot` are force commands, answered `retained` and not carried out. `setrot` is marked `RLV only`.
- Questions with fixed sandbox answers. The sandbox has no avatar, group or inventory model yet:
  `getsitid` answers the null key (`00000000-0000-0000-0000-000000000000`), `getgroup` answers `none`, `getoutfit`
  answers 16 zeros (one per clothing layer, gloves to physics; with a layer name as option, one `0`, and empty for a
  name that is none of them) and `getattach` answers 41 zeros (index 0 is "none", then 40 attachment points from
  chest to root; one `0` for a point name as option). `getinv`, `getinvworn`, `findfolder`, `getpath` and
  `getpathnew` answer empty because there is no folder model yet; the same is true of `findfolders` and the
  `getadd*`, `getrem*` and `get*names` questions.
- Families by prefix: `setdebug_<name>:<value>=force` and `setenv_<name>:<value>=force` (any name) are retained and the
  value is recorded (`recorded` in `rlv_state`, at most 256 entries, a known name can still change); `getdebug_<name>`
  and `getenv_<name>` answer the recorded value of the same name, or empty. They are marked `RLV` and left out of
  `getcommand` (it lists the table, not the prefixes).
- More effects that are only recorded (camera group): `camdrawmin`, `camdrawmax`, `camdrawalphamin`,
  `camdrawalphamax`, `camdrawcolor` (`RLV only`; option required), the sphere effect `setsphere` with
  `setsphere_mode`, `_origin`, `_color`, `_distmin`, `_distmax`, `_distextend`, `_param`, `_tween`, `_valuemin`,
  `_valuemax`, and the overlay `setoverlay` with `setoverlay_alpha`, `_texture`, `_tint`, `_tween`, `_touch` (all
  `RLVa only`). `camdraw*`, `setsphere`, `setoverlay` and `setoverlay_touch` are held as restrictions (`n`/`y`); the
  `setsphere_<name>` values, `setoverlay_alpha`, `_texture`, `_tint` and `_tween` take only `=force`, as in RLVa
  (retained, the value is recorded). They block nothing; the real view never changes.
- Three more questions (`RLV only`): `versionnumbl` answers `3040300,` (the version number, a comma, then the
  blacklist, which is empty), `getblacklist` answers empty, and `getcam_zoommin` always answers `1` (the sandbox has
  no camera).
- Effects outside the sandbox (camera, sky and water, debug settings, view modes) are only recorded as state and
  shown in the log ("recorded only"); the real program never changes, except the camera limits the user lets limit
  the 3D view with the "Show camera limits in 3D view" box (above, "Camera limits in the 3D view") and the sky and
  draw distance hint of the "Show environment in 3D view" box (above, "Environment in the 3D view") and the view
  modes of the "Show view modes in 3D view" box (above, "View modes in the 3D view"). A force command of that kind is `retained`
  and its option is kept in `rlv_state` under `recorded`; a new or changed value is a change of the state (the panel
  and the 3D view follow it), and the object's `@clear` forgets its recorded values.
- The sandbox has no avatar, inventory or camera, so the RLVa questions about them answer empty (`getheightoffset`
  answers `0`). `getcommand` lists the behaviours known in the current mode.
- `setcam_*` behaviours with a distance need it as option (`@setcam_avdistmin:2=n`); without it the command fails.
- Option values are checked as RLVa checks them, in RLVa mode (lane 1, job C; `docs/RLV_OPTION_TYPES.md`): a key, a
  number, a channel, an attachment point, a clothing layer, x/y/z or a camera mode. A bad value is refused with no
  change, and the log says what to write instead, e.g. `@detach:x=n: "x" is not an attachment point. Use a name such as
  spine or left hand.` A refused `=y` is `unset`. It never stops the script. Folder paths, filters and names are not
  checked, and plain RLV mode checks nothing (not confirmed). `lsl_lint` gives the same warning as `rlv-param`.

## Tests: what is covered and what is not (22 November 2026)

Everything below runs by itself on every pull request (the C++ tests on Linux, Windows and under the sanitizers; the
smoke run on Linux and under the sanitizers) unless it says otherwise.

- **RLV core:** the parser (`tests/tst_rlvcommand.cpp`), the state with `@clear` and `@getstatus`
  (`tst_rlvstate.cpp`), the behaviour table (`tst_rlvtable.cpp`), the RLVa and RLV modes (`tst_rlva.cpp`) and the
  table checked against RLVa's source and the RLV wiki (`tst_rlvsources.cpp`, `test_rlv_sources.py`). Fuzz target
  `fuzz_rlv`.
- **LSL:** lexer, parser and checker, interpreter and lint (`tst_lsllexer`, `tst_lslparser`, `tst_lslinterp`,
  `tst_lsllint`), and one test per step: several objects (`tst_lslworld`), linked scripts (`tst_lsllink`), pure
  functions (`tst_lslpure`), `on_rez` and `changed` (`tst_lslevents`), prim state and L$ (`tst_lslprims`), inventory
  (`tst_lslinventory`), sitting (`tst_lslsitting`), controls (`tst_lslcontrols`), sensors (`tst_lslsensors`), stubs
  (`tst_lslstubs`), HTTP (`tst_lslhttp`), reads and state of the object itself (`tst_lslownobject`), touches (`tst_lsltouch`), region and avatar reads (`tst_lslregion`), money, web server and mail (`tst_lslmoney`), prim media (`tst_lslmedia`), several objects giving, loading scripts and inventory drop (`tst_lslobjects`) and RLV from a script (`tst_lslrlv`). Fuzz targets `fuzz_lsl` and
  `fuzz_control`.
- **Live:** `tst_labwatcher.cpp` and the Live part of `tst_control.cpp`.
- **Effects in the 3D view:** `tst_labeffects`, `tst_scriptlabeffects`, `tst_cameraoverride`,
  `tst_environmentoverride`, `tst_viewmodesoverride` and `tst_scriptlabcamera` check the values the 3D view is given
  (animation and frame, camera limits, sky colour and fog distance, view modes), not the picture.
- **The panel:** `tests/scriptlabpanel/` (the Commands page's rows, words and colour tokens, the Objects and Dialog
  part, the input buttons, the panel's place after saving the layout), offscreen.
- **The whole tool through the control commands:** `tests/data/scriptlab/scenarios.json` holds 34 scenarios (108
  requests): each of the eight example scripts in `examples/scriptlab`, each kind of RLV command (restriction on and
  off, duplicates, two objects and `@clear`, force commands and recorded values, questions answered on a channel,
  exceptions, `@notify`, the RLVa and RLV modes, unknown behaviours and refused values, refused requests), RLV from a
  script, several objects with chat ranges, linked scripts with Linkset Data, a notecard from canned inventory, a pose
  ball (sit and stand), controls, sensors, canned HTTP replies, `transaction_result`, reads and state of the object
  itself (`llGetObjectDetails`, `llSetScriptState`, `llResetOtherScript`, `llDie`), the touch phases (a click and a held touch), region and avatar reads (a given region, the clock, the sun, `llGetAgentList`, `llRequestAgentData`), money, web server and mail (a URL, an incoming request and its answer, a payment, an email), prim media (media set, read back, wrong values and cleared), and error cases (a script that
  does not compile, an unknown function, a function not supported yet, a runaway list, wrong `lsl_event` and `lsl_run`
  input). Each request lists the exact values some fields of its answer must have (whole event logs, RLV results and
  replies, the state). `tests/tst_scriptlabwhole.cpp` sends them to the control interface in a unit test (ctest
  `scriptlabwhole`, all three builds); `tests/smoke_scriptlab.py`, run by `tests/smoke_control.py`, sends the same
  file to the real program under Xvfb over its HTTP interface and then runs a script, `rlv_state`, `lsl_check` and
  `rlv_reset` through the Node MCP server, as an AI tool does. A new example script must be added to a scenario (both
  tests fail otherwise). After an intended change of an answer:
  `cd bin && PINGLYMOTION_UPDATE_SCENARIOS=1 <build>/tests/pinglymotion_scriptlabwhole_tests` writes the expected
  values again; read the difference before committing it.

Not tested automatically (gaps):

- How the effects look in the 3D view (the figure playing the animation, the camera limits, the fog and the view
  modes) and how the panel looks and feels: a person has to look, on the Windows build.
- The Live page in the real program with a real folder (the watch is tested offscreen and through `lab_watch` in
  `tst_control.cpp`, not in the smoke run), and `lsl_event` before any `lsl_run` in the real program (the scenarios
  share one running program).
- Second Life itself: the sandbox is never identical to it; the final test is always in Second Life.
- Known differences, pinned by the scenarios so that a change shows: a dialog answer sent with `lsl_event` may be any text, not only a button the dialog
  showed (it is said on the dialog's channel, scenario `example_hud_cuffs`).

## Limits

- Only the behaviours in the table; others parse but are reported as unknown.
- `force` commands are accepted (`retained`) but not carried out: there is no world to teleport in or sit on.
- LSL: up to 16 objects and 64 scripts (32 per object) in one run, at most 100 RLV lines per script and run, the
  events `state_entry`, `state_exit`, `timer`, `listen`, `touch_start`, `touch`, `touch_end`, `run_time_permissions`, `attach`,
  `link_message`, `linkset_data`, `on_rez`, `changed` and the events of step 9b (`dataserver`, `object_rez`, `control`, `sensor`, `no_sensor`,
  `http_response`, `transaction_result`), `money`, `http_request` and `email` (second gap list, step 4), and 282 built-in functions; HTTP
  only with canned replies and a fake web server, never on the network; no L$ and no mail; hard limits on
  steps, memory, nesting and time per script (and per `lsl_event`). Avatars other than the owner exist only as names
  and keys in inputs, and every avatar stands where the owner stands. The sandbox is never identical to Second Life;
  the final test is always in Second Life.

## The editor (lane 1, job F1: built)

Plan: [ROADMAP.md](ROADMAP.md), "Script Lab: folder, editor, converter", F1. The Script part of the Live page is an editor in the program's own text box (`src/scriptlab/labeditor.*`, colours in `src/scriptlab/lslhighlighter.*`, no outside editor component). Test: `tests/scriptlabpanel/tst_labeditor.cpp`.

- **Typing and saving.** The script can be typed in. LSL words have colours from the `syntax` tokens ([DESIGN_SYSTEM.md](DESIGN_SYSTEM.md), "Code colours"): keywords (also semibold), types, events, functions, constants, strings, numbers and comments (also italic). The function, event and constant names are the ones the checker knows (`lsl/builtins.h`). The colours are decoration only: an error is a wavy underline in the danger colour, a circle-x icon and the bold number in the line number strip, the message as a tip and in the error list (warnings: the warning colour and a triangle icon). While there are unsaved edits the part's count says "● unsaved changes". **Save** (Ctrl+S, greyed with "Nothing to save. Change the script first.") writes the file (Windows line ends are kept), and the folder watch checks and runs it at once; the line under the text then says "Saved and run 15:30. 0 errors." (or "1 error.", ", 2 warnings", "Saved 15:30. Not run: ..."). Undo and redo (Ctrl+Z, Ctrl+Shift+Z or Ctrl+Y) keep working after a save.
- **Live never overwrites typing.** Since job D the Live page runs the folder as one object; the editor shows the script whose save started the run, or the one picked in the tree. A run of another script does not take the editor away from unsaved edits; the editor follows the runs again once they are saved or undone. When the shown file changes on disk while there are no unsaved edits, the new text is shown as one undo step ("Loaded the changed file. Ctrl+Z brings the old text back."). With unsaved edits the editor asks once, in a bar above the text: "The file changed outside." with **Keep my changes** (nothing changes; Save later writes the text over the file; the same file text does not ask again, a new change does), **Load the new one** (one undo step, so Ctrl+Z brings the edits back) and **Show the difference** (lines starting with `- ` are yours, `+ ` are in the file, two spaces are the same). While the question is open the run's marks are not put on the typed text.
- **Keys.** Line numbers; **Go to line** (Ctrl+G: a row with the cursor's line number, Enter jumps, Esc closes; "There is no line 99. The script has 8 lines."); **Find** (Ctrl+F: a row, case does not matter, Enter or F3 next, Shift+Enter previous, it goes round, "Match 1 of 2" or "Not found: ..."). Enter keeps the indent, one step (4 spaces) more after `{`, and between `{` and `}` it opens a block with the cursor inside; `}` on a line with only spaces goes back one step. Tab inserts spaces to the next step (with several lines selected it indents them), Shift+Tab takes one step back. A typed `"` gets its closing quote (not before a word or after `\`), typing the closing quote steps over it, and a selection gets quotes round it. The bracket next to the cursor and its partner have a box round them (brackets in strings and comments do not count). The editor's keys win over the window's while the text has the focus (File > Save is Ctrl+S too).
- **A way out for keyboard users (charter P8, WCAG 2.1.2).** Tab types an indent, so **Esc, then Tab** moves the focus on (Esc, then Shift+Tab moves back). The first time the text gets the focus the line says "Tab adds an indent. To move on, press Esc, then Tab."; after Esc it says "Press Tab to move on, or keep typing."
- **Starter help (charter P4).** With no script the part shows "No script open", "Start from an example, or save a .lsl file in the folder." and **New from example**, a menu of the scripts in `examples/scriptlab`. Picking one puts its text in the editor as a new, unsaved file of the same name in the watched folder (`hello_2.lsl` and so on when the name is taken); nothing is written before Save. A short card says what an RLV line is: "A script sends an RLV line to the viewer. It starts with @, like @detach=n. The viewer then blocks or allows one thing." It goes away while hints are off.
- **Theme and size.** Both themes; the text uses the system's fixed-width face at the program's text size (View > Text size, Setup).
- **Long differences.** Show the difference compares line by line while the comparison table stays at or under 4 000 000 cells (`LabEditor::DifferenceTableLimit`, about 16 MB; the size is counted in 64 bits, so it cannot overflow); a bigger difference, or one where a side is empty, lists all of your lines, then all of the new ones (CodeQL alert on pull request 428, fixed in pull request 431).
- **Not in job F1:** suggestions and help text and the language choice (built in jobs F2 and F3, below), RLV suggestions and warnings (built in jobs R3 and R4, below), Lua.

### Suggestions and help (lane 1, job F2: built)

The engine is `src/scriptlab/labsuggest.*` (QtCore only); the editor shows it (`src/scriptlab/labeditor.*`). Tests: `tests/scriptlabpanel/tst_labsuggest.cpp` (exact lists, help texts and keys) and `tests/test_gen_lsl_help.py` (the help data and our own lines).

- **What is suggested.** Every function, event and constant of Linden Lab's definitions (`tests/data/lsl-definitions`, MIT) that a script may use (god-mode, private and deprecated ones are left out), the types and keywords, and the global variables and functions made in the script itself. Names that start with the typed text in the same case come first, then any case, then names that start with it after `ll` (`OwnerSay` finds `llOwnerSay`); alphabetical inside each group. Nothing is suggested inside a string or a comment, after a `.` or in a number. A name typed in full alone shows no list.
- **When.** The list comes up by itself after two characters of a name, in Beginner and Expert mode (charter P3, P4: a beginner does not know there is something to ask for). **Ctrl+Space** opens it at any time, also with nothing typed (then every name). **Esc** closes it, and it stays closed until typing goes on (Backspace does not count); the same Esc still arms the way out, so Esc, then Tab, moves the focus on as in F1. A character that cannot be in a name, the arrow keys left and right, a click or leaving the text close it. Setup, "Script editor": **Show suggestions only when I press Ctrl+Space** (off at start; remembered as `scriptlab_suggest_on_request`): then the list comes only with Ctrl+Space and follows the typing once open.
- **The list never takes Enter or Tab by itself (charter P11).** Before an arrow key nothing is chosen: Enter is a line break and Tab an indent, as without the list (and the list closes). **Down** moves into the list at the first row, **Up** at the last; the arrows go round, Page Up and Page Down jump 5 rows. Only then **Enter** or **Tab** takes the chosen name; typing more starts over (not chosen again). A click on a row takes it too. Taking a function puts in `name()` with the cursor inside the brackets (after them when it has no parameters), or only the name when a `(` follows already; an event puts in its parameters, `touch_start(integer num_detected)`. Taking is one undo step. The chosen row has the selection fill and bold text (charter P10). The list never takes the focus; it lies over the text, inside the text box, and moves nothing.
- **Help for the highlighted name.** Under the list, the same controls in both modes (charter P12): one plain sentence of what it does in Second Life (Beginner and Expert), then in Expert mode one dense line, for example `Function · returns nothing · llOwnerSay(string msg)`, `Event · touch_start(integer num_detected)`, `Constant · integer · value 1`. The plain sentence is our own words for about 130 common names (`src/scriptlab/lslhelp_plain.inc`, at most 12 words, no avoided word: "Sends a chat line only the owner sees. RLV lines use it."); every other name shows the first sentence of Linden Lab's own description (`src/scriptlab/lslhelp_data.inc`, made by `python3 tools/gen_lsl_help.py`; the test fails when it is out of date with the copy of the definitions). With hints on, a last line says "Press Down to pick, then Enter or Tab. Esc closes the list." Both themes and the text size follow Setup; the colours are tokens (`surface_raised`, `border`, `selection`, `on_accent`, `text`, `text_muted`).
- **For later jobs.** The engine has three layers: the language (which characters make a name, what is a string or comment, and the name a built-in has: `ownerSayName()` is `llOwnerSay` in LSL and `ll.OwnerSay` in SLua), the providers, and the engine that joins and sorts them. The RLV provider (jobs R3 and R4, below) is added through `LabCodeEdit::suggestEngine()`; the context tells it when the cursor is inside a string and where the string starts. An item can replace more than the typed name (`replaceFrom`), carry a mark ("RLVa only") and an example for Ctrl+Enter.
- **Not tested:** a screen reader reading the list (the list has the accessible name "Suggestions", but no screen reader was tried). Whether the list helps or irritates while typing is for the project owner's taste.

### RLV in the editor (lane 1, jobs R3 and R4: built)

The editor uses the RLV table. Provider: `src/scriptlab/rlvsuggest.*` (QtCore only, added to the engine by the editor). Help rows: `src/scriptlab/rlvhelp_plain.inc`. Warnings: `Rlv::plainProblem()` in `src/scriptlab/rlvbehaviours.*`. Tests: `tests/scriptlabpanel/tst_rlvsuggest.cpp` (lists, help, keys, marks, warnings in the editor, the jump to the line) and `tests/tst_rlvexamples.cpp` (one row per command, every example valid, every sentence short, the exact warnings).

- **Where.** Only inside the text of `llOwnerSay(...)` when it starts with `@`, and only the first text in the brackets. In code, in other strings and in comments nothing changes. The name of "send text to the owner" comes from the language layer (`Language::ownerSayName()`), so SLua's `ll.OwnerSay` is a small later step; the RLV provider answers for LSL only today.
- **What.** After `@` or `,`: every RLV command of the table (219), the four families with their ending (`setenv_<setting>`; taking it puts in `setenv_` and keeps an ending already typed) and `clear`, alphabetical, filtered by what is typed (case does not matter). After `name:`: the option values of its value type (job C): the 56 attachment points, the 17 clothing layers or the three camera modes; for the other types one sample value (from the command's example, or the type's own sample) with a sentence that says what the option must be ("A number, such as 1.5."). Nothing while a value is being typed. After `=`: the forms the command takes, in the order n, y, add, rem, force, then 2222 for a question.
- **When.** The list comes up by itself after `@`, `,`, `:` and `=`, and while a name is typed, in Beginner and Expert mode. The F2 rules stay: Enter and Tab take a row only after an arrow key moved into the list, Esc closes it until typing goes on, and Setup's "Show suggestions only when I press Ctrl+Space" holds for RLV too.
- **Help (same controls in both modes, charter P12).** One plain sentence on what the command does in Second Life (our own words, at most 12 words: "Locks this object on you, so it cannot be taken off."), then "Ctrl+Enter puts in the example: @detach=n". **Ctrl+Enter** replaces the command typed so far with the whole example, as one undo step. Expert mode adds one dense line: the kind (restriction, force, reply), RLV and RLVa, RLVa only or RLV only, the group, and the option with its value type, for example `Restriction, force · RLV and RLVa · Group: inventory · Option: attachment point, optional`. The table's own descriptions stay as they are: they also say what the sandbox does.
- **Marks.** While the panel's viewer rule is RLV, commands that only RLVa knows show "(RLVa only)" after the name, and so does an RLVa-only form such as `fly=force`; while it is RLVa, commands that only plain RLV knows show "(RLV only)". The mark is a word in the row and its tip, never only a colour (charter P10).
- **Warnings.** The static check's RLV warnings now start with the command and say the fix in plain words: `@detach=x: x is not valid. Use n, y or force.`, `@detach: the = part is missing. End it with =n, =y or =force.`, `@detatch=n: detatch is not an RLV command. Did you mean detach?`, `@sendchat=force: sendchat does not take =force. Use n or y.`, `@redirchat=n: redirchat needs an option after ":". Example: @redirchat:2222=n`, and job C's value warnings (`@detach:x=n: "x" is not an attachment point. Use a name such as spine or left hand.`). After a save and run they show in the script (wavy line, triangle icon, the bold line number, the text as a tip) and in Errors and warnings ("Warning, line 5, column 20: ..."). One click on a warning, or Enter, shows the code line; a click on an error line or an RLV line of the event log does the same (an RLV line goes to the first script line that holds the same text). A warning never stops a run.
- **Examples.** One example per command, in our own words, in `rlvhelp_plain.inc`; `tests/tst_rlvexamples.cpp` checks that every example parses, names its own command and passes the table's checks and job C's option check (plain RLV mode for the commands only RLV has). The keys in the examples are made up.
- **Window size (the project owner, 23 November 2026, 17:48: «Det lar seg ikke endre str på. Og slik det blir nå må det ved bruk ha større plass»).** Found: nothing in the panel fixed its size; docked, the splitter at its side already worked (the panel then started at its natural width, about 375 pixels). Floating, the window is frameless (`Qt::Tool | Qt::FramelessWindowHint`), because a dock with its own title bar gets no system frame from Qt, so its edges could not be dragged. Now the panel starts at 480 by 720 pixels (`ScriptLabPanel::defaultSize()`), never gets smaller than 300 by 240 (`minimumPanelSize()`), both scaled with the text size, and a floating panel has a size grip in its bottom right corner (tip: "Drag to change the size of the Script Lab window"). The size, docked or floating, is remembered with the rest of the panel layout (`panel_layout`, written at close). Test: `tests/scriptlabpanel/tst_rlvsuggest.cpp` (`windowSize`, `windowSizeRemembered`). Not built: dragging the edges of the floating window (only the grip), and a keyboard way to resize it. How the default size looks is for the project owner's taste.
- **Not tested:** a screen reader reading the suggestion list (the list has the accessible name "Suggestions"; no screen reader was tried). Whether the RLV list helps or irritates while typing is for the project owner's taste.
- **Not in jobs R3 and R4:** Lua suggestions (Lua stays last; while the Lua switch is off nothing says Lua), RLV warnings while typing before a save (they come with the run), and the viewer rule in the static check (it judges each command in the family that knows it, as before).

### The language choice (lane 1, job F3: built, only the choice)

- Setup, "Script editor": **Lua (trial): show the language choice in the editor**, off at start (remembered as `scriptlab_lua_switch`). While it is off nothing in the editor says Lua, there is no Lua icon, and the language is LSL, whatever was chosen before (charter P5).
- While it is on, the editor's tool row shows **Language** with LSL or SLua (Lua, trial) at its right end, after the other buttons, so showing it moves nothing (charter P12). The choice is remembered (`scriptlab_language`, `lsl` or `slua`). SLua gives no suggestions, no checks and does not run: the line under the text says "Lua is a trial. No Lua suggestions or checks yet." Scripts are still checked and run as LSL. Lua stays last (the project owner, 23 November 2026, 14:00: «Lua er helt til slutt»).
- Tests: `tests/scriptlabpanel/tst_labsuggest.cpp` (switch off hides every Lua word, the choice is remembered, nothing moves) and `tests/setupdialog/tst_setupdialog.cpp`.

## The folder model and a short maker guide (lane 1, job B: the core; job D: the panel)

Plan: [ROADMAP.md](ROADMAP.md), "Script Lab: folder, editor, converter".

**What exists now:** the folder model is built in the core (`src/scriptlab/labfolder.*`, tests `tests/tst_labfolder.cpp` and the linkset part of `tests/tst_labwatcher.cpp` and `tests/tst_control.cpp`). An AI assistant uses it with `lab_watch` and `"linkset": true` ([CONTROL.md](CONTROL.md), "The folder is one object"). Since job D the panel's **Watch folder...** uses it too, and the Live page shows the object as a tree ("The folder tree in the panel" below); **Try the example scripts** still runs each example on its own, because the examples next to the program are separate examples, not one object.

- A top folder is one object (one linkset), named after the folder. Scripts and notecards right in the top folder, and in a subfolder named `Root` (any case), are in the root prim (link 1). Every other subfolder is one child prim, links 2, 3, ... in name order. All scripts run together in one simulated object, so `llMessageLinked` and `link_message` work between the scripts of different prims.
- A script's name is its file name without `.lsl`. A notecard is a `.txt` or `.notecard` file, named without the extension; `llGetNotecardLine` reads it.
- All scripts are checked at once. Nothing runs while one script does not compile or a limit is broken; every error names its file, script and prim. Every line of the event log names its source: the script, the prim and the file, or the prim and `sandbox` for an input.
- Limits for the whole folder: the path rules of Live (whole path, no network folder, at most 4 096 characters), at most 5 000 entries, 32 scripts and 256 prims, at most 262 144 bytes per file and 524 288 bytes in all, paths inside the folder up to 255 characters, names up to 63 characters, and every script and notecard name used once. Endless loops are stopped by the interpreter's limits; the 2 seconds of real time are shared by all scripts of a run, so a folder of endless loops stops after about 2 seconds and the log says which script in which prim stopped.
- Not as in Second Life (the sandbox's choices): one inventory per object, so a script or notecard name may be used only once in the whole object (Second Life allows the same name in different prims); `llGetLinkName` does not give the folder names yet; a folder inside a prim's folder is not read (Second Life prims are not nested either); the scripts run one after the other in a fixed order (link order, then file name).

**Maker guide:**

```
MyObject/            one object
  Root/              one prim (the root)
    main.lsl
    settings.txt     a notecard
  Cuff_left/         another prim
    cuff.lsl
```

- One top folder per object; one subfolder per prim; scripts (`.lsl`) and notecards (`.txt`) in that prim's folder. A single-prim object can keep its files right in the top folder.
- Give every script and notecard its own name in the whole object.
- Copy scripts out of Second Life by hand: open the script in the viewer, select all, copy, paste into a file. The tool cannot read your inventory.
- Warnings you can expect: a script that cannot be checked, a limit that was reached, and a script or prim that stopped. Each one names the script and the prim.
- The sandbox is never identical to Second Life; the final test is always in Second Life.

## The folder tree in the panel (lane 1, job D)

Code: `src/scriptlab/labtree.*` and the Live page in `src/scriptlab/scriptlabpanel.*`. Test: `tests/scriptlabpanel/tst_labtree.cpp`.

- **Object part.** A folding part "Object" above the Script part, shown while a folder is watched as one object. Its count is "2 scripts in 2 prims". The tree has the columns Name, Kind, State and Memory: the object (box icon, "Object, 2 prims"), each prim (square icon, "Root prim, link 1" or "Prim, link 2"), and in each prim its scripts (file-code icon, "LSL script") and notecards (notebook-text icon, "Notecard, 4 lines").
- **Colour dots and words.** Each script has a dot and a word: OK (`success`), Warning (`warning`), Error or Stopped (`danger`), Not run (`text_muted`: another script did not compile, or a limit was broken). The tip of the State cell says why. A script the folder refused (a name used twice, a file too large) has an Error row with the reason. A prim and the object sum up their scripts, for example "1 error, 1 not run", with the dot of the most serious state, or "No scripts".
- **Memory per script** is what the script's values used at the end of the run, in KB with one decimal ("4.0 KB"); a prim and the object show the sum. It is the sandbox's rough count: code is not counted, so a script with no values shows "0.0 KB". Second Life gives a script 64 KB.
- **Picking.** A click, or Enter, on a script shows it in the Script part with its own errors and warnings; picking runs nothing. The pick holds until the next run; then the editor follows the script whose save started it. A click on an event log line of another script shows that script first.
- **Status line.** A folder of several scripts is named as the object: "OK: the object MyObject ran without errors (run 3), 12 log line(s)." When the shown script is fine but the object did not run: "Error: the object MyObject was not run (run 3): Lid/lid.lsl does not compile. Pick it in the tree to see why." or "... it breaks 1 limit(s) or rule(s), listed under Errors and warnings." A folder of one script keeps the old wording with the file name.
- **Event log.** With several scripts every line names its source: "lid (Lid): owner_say: lid got hello", or "Root: " for an input. A folder of one script keeps the short lines.
- **Folder problems** (limits and rules) are lines in Errors and warnings: "Folder error, Lid/main.lsl: The name "main" is used twice ...", and things left out as "Folder note, ...".
- **Maker guide.** Six short lines (since job U3 as four Rules and two Warnings) and the example folder in a fixed-width font. It was a folding part "How to lay out the folder" on the Live page; since the UX small fixes (24 Nov 2026, the project owner: it looked like a function) it is the topic "How to lay out a folder" in the Script Lab help view, opened with the ? button at the right of the Viewer rules row (`src/scriptlab/labhelpview.*`). The help view also has Start here, Words (every word tip), Hints and one topic per example in `examples/objects` with its guide texts word for word. The Object part's open or folded state is remembered (`tree` in `scriptlab_parts`).
- **Not yet:** `llGetLinkName` still does not give the folder names (it needs a change to `lsl_run`'s objects, a control-command change of its own); the Lua mark comes with Lua (only `.lsl` files are read).

## Props as prims (lane 1, job E)

Code: `src/scriptlab/labtint.*`, `Prop::setTint` in `src/prop.*`, wired in `src/pinglymotionwindow.cpp`. Tests: `tests/tst_labtint.cpp`, `tests/tst_scene.cpp` (`propTint`), `tests/scriptlabpanel/tst_labtree.cpp` (`propTints`).

- While **Show effects in 3D view** is ticked and a folder is watched as one object, a prop in the scene with the same name as a prim (the prim's folder name, for example `Lid`) shows the colour and transparency the scripts gave that prim: `llSetColor`, `llSetAlpha`, `llSetLinkColor`, `llSetLinkAlpha` and `PRIM_COLOR`. The last value given to the whole prim (`ALL_SIDES`) or to face 0 counts; a prop has one colour, so other faces are left out. Unticking the box, a run without colours or Stop watching gives the props their own colour back.
- A colour without an alpha keeps the prop solid; an alpha without a colour keeps the prop's own colour. A hovered or picked prop keeps its highlight colour, with the tint's transparency.
- Built since in job A1 (below, "Props from prims, click as touch, moving props"): props made from the prims by themselves, a click on a prop as a touch, and a prop that moves when a script moves its prim.

## Props from prims, click as touch, moving props (job A1)

Built 24 November 2026, pull request 447 (plan "Script Lab: see and understand", approved by the project owner on 24 November 2026, 00:08; his choices of 23 November, 23:05 to 23:55). Code: `src/scriptlab/labprimprops.*` (which prop, and where it goes), `LabWatcher::touch` in `src/scriptlab/labwatcher.*` (the kept world), `ScriptLabPanel::updatePrimProps` and `touchPrim` in `src/scriptlab/scriptlabpanel.*`, two more scene hooks and the click in `src/pinglymotionwindow.cpp`. Tests: `tests/tst_labprimprops.cpp` (exact props and places), `tests/tst_labwatcher.cpp` (`touchGoesToThePrim`), `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` (`propsFromPrims`: the whole chain with a fake scene).

- **Props from prims.** While **Show effects in 3D view** is on and a folder is watched as one object, every prim gets a prop named like the prim (its folder's name; the object's name for a root without a `Root` folder). A prop of that name already in the scene is used as it is; nothing is made twice. A root prim that is only the top folder, with no script or notecard in it, gets no prop (First colour's root). The props stay in the scene after the effects are turned off, like the example props of job U3.
- **Which prop.** The sandbox knows no shape and no place on the body for a prim, so: (1) the prop of the same name in the folder's `example.json`, when there is one (the example objects); (2) otherwise a word in the prim's name, first match wins (case does not matter; "right", or a name ending in `_r` or starting with `r_`, is the right side, everything else the left):

  | Word in the name | Shape | Attached to | Place | Size | Turn |
  |---|---|---|---|---|---|
  | ankle | torus | lFoot / rFoot | 0, 2, 0 | 3.4, 3.4, 1.2 | 90, 0, 0 |
  | wrist, cuff | torus | lHand / rHand | -1 / 1, 0, 0 | 2.8, 2.8, 1.2 | 0, 90, 0 |
  | collar, neck | torus | neck | 0, 1.5, 0 | 5, 5, 1.6 | 90, 0, 0 |
  | blindfold, eye | torus | head | 0, 3.1, 0.6 | 8.2, 8.2, 1.4 | 90, 0, 0 |
  | gag, ball | sphere | head | 0, 0.9, 3.6 | 1.6, 1.6, 1.6 | 0, 0, 0 |
  | hood, hat, head | cone | head | 0, 4, 0 | 9, 9, 10 | -90, 0, 0 |
  | hud, or the name Root | box | not attached | -20, 45, 0 | 8, 8, 1 | 0, 0, 0 |
  | anything else (the default) | box | not attached | 20, 45, 0, then 6 further along x for each next one | 4, 4, 4 | 0, 0, 0 |

  The values are those of the Gear set example, in the scene's units (inches); "cuff" means a wrist cuff unless the name also says ankle.
- **Click is touch.** A click on a prop whose name is a prim of the watched object (while the effects are on) sends a touch (`touch_start`, `touch`, `touch_end`, as the Touch input does) to that prim, in the world of the last folder run, and runs it on for one simulated second. The event log first says "You clicked Collar.", then what the scripts did. A prim with no touch handler passes the touch to the root, as in Second Life. The world is kept between clicks (a counter counts on); the next save runs the folder again from the start. The click also picks the prim's row in the tree (job U3), and the prop is picked on the Props tab as before. Touch, Say and the dialog buttons under Send input still go to the world of the last `lsl_run`, not to the watched folder.
- **Moving props.** When a script changes a prim's position, size or rotation (`llSetPos`, `llSetScale`, `llSetRot`, `llSetLocalRot`, `llScaleByFactor`, `llSetRegionPos`, and `PRIM_POSITION`, `PRIM_POS_LOCAL`, `PRIM_SIZE`, `PRIM_ROTATION` and `PRIM_ROT_LOCAL` in `llSetPrimitiveParams` and the link functions), the prop jumps to its new place, with no animation ("Hopp er greit"). The last value in the run counts, also after a click. A move the sandbox refused (outside the region, worn, physical) is left out.
- **How a move becomes a place.** The change from the prim's start is added to where the prop stood: position, the change from 0, 0, 0 for a child prim (local to the root) or from 128, 128, 20 for the root (in the region), with Second Life x, y, z as the scene's z, x, y and 1 m as 39.37 units; size, each side of the prop times the prim's side divided by 0.5 m (the prim's own x, y, z to the prop's x, y, z); rotation, the script's turn (renamed to the scene's axes) before the prop's own turn, back as the prop's three angles to a hundredth of a degree. Each prop follows only its own prim: turning the root does not carry the other props along.
- **Back again.** Turning the effects off, a run in which no script moves that prim, or Stop watching puts each moved prop back where it stood before the first move. While the effects are on, a moved prop's place comes from the script; a prop no script moves is left where the user put it.
- **Not built:** animated movement, chains and physics, mesh and textures, the Send input buttons reaching the watched folder, holding a touch on a prop. The llDialog box over the 3D view is built since in job A2 (below). Nothing here was tested in Second Life.

## The menu box over the 3D view (job A2)

Built 24 November 2026, pull request 448 (plan "Script Lab: see and understand", approved by the project owner on 24 November 2026, 00:08; his choice of 23 November, 23:10: «A, ja», the menu shows as in Second Life, where the user clicked). Code: `src/scriptlab/labdialogbox.*` (the box and the button grid both menus use), `LabWatcher::dialog` and `LabWatcher::answerDialog` in `src/scriptlab/labwatcher.*`, `ScriptLabPanel::setMenuBoxHost`, `refreshMenuBox` and `answerMenu` in `src/scriptlab/scriptlabpanel.*`, two lines in `src/pinglymotionwindow.cpp`. Tests: `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` (`menuBoxAtTheClick`, `menuBoxTextBox`), `tests/tst_labwatcher.cpp` (`dialogAnswerGoesToTheMenu`).

- **When.** While **Show effects in 3D view** is on and a folder is watched as one object, a menu a script of the object shows (`llDialog`, or `llTextBox` for a typed answer) appears as a box over the 3D view. It stands next to the click on the prop that made it: to the right of the click, or to the left when there is no room, always inside the view. A menu without a click (for example in `state_entry`) stands in the top right corner.
- **What it holds.** "Menu from <object>", the message, one button per button text in rows of three (the first row on top, the same order as the panel's menu), a line "A button says its text on channel -77. Only the script hears it.", and **Ignore**. A text box has a field and **Send** instead of the buttons ("What you type is said on channel -88. Only the script hears it."); Send is greyed until something is typed. An `&` in a button text shows as itself.
- **A click on a button** sends the same `lsl_event` input as the panel's menu does (`{"type": "dialog", "object": ..., "button": ...}`, made by `LabWatcher::dialogAnswer` for both): the owner's avatar says the text on the menu's channel, and the world of the folder run runs on one simulated second. The event log first says `You clicked "Lock" in the menu (channel -77).`, then what the scripts did. The box closes, as in Second Life; a menu the answer opens (a sub-menu) shows again at the same place.
- **Ignore** or **Esc** closes the box and sends nothing. The same menu does not come back by itself; a new click that shows it again does. Turning the effects off hides an open menu; a save runs the folder from the start, so an old menu goes.
- **Keys.** The first button (or the text field) gets the keys when the box opens; Tab goes through the buttons row by row, then Ignore; Space presses a button, Enter in the field sends.
- **The panel's menu stays.** "Dialog from ..." inside Objects still shows the newest menu of the world of the last `lsl_run` (the one Send input talks to) and answers it there. The box only shows menus of the watched folder's world. Both make their buttons with `LabDialogBox::fillButtonGrid`.
- **Look:** a `surface` box with a `border` line and `radius.panel`, normal buttons (`surface_raised`, `radius.control`, 32 pixels high), the channel line in `text_muted`, the focus ring on keyboard focus; 336 pixels wide at 100 percent text size. How it feels next to the props is for the combined taste round.
- **Not built:** dragging the box, more than one menu open at once (a new menu replaces the old one, as the sandbox keeps only the newest), the "Block" and timeout of a real Second Life menu, and Send input reaching the watched folder.

## Choose a restriction from the list (job A2)

Built in the same pull request (the project owner, 23 November 2026, 23:50: «bør den listen over om noen restriksjoner kunne brukes som drop-down? Jeg så bare grønne statuser der, men kan ikke scripte»). Code: `src/scriptlab/labrestrictionchoice.*` (QtCore: which command, the line, the words, where the script line goes) and `ScriptLabPanel::chooseAction`, `applyChoiceValue` and `useChoiceInScript`. Tests: `tests/tst_labrestrictionchoice.cpp` (every row in both modes, the value fields, the words, the place in a script) and `tests/scriptlabpanel/tst_scriptlabpanel.cpp` (`chooseRestriction`, `useInScript`).

- **Every row has a choice.** In "What the avatar may do" (Type commands page) each row's Now cell is a drop-down with **Allowed** and **Blocked**. It always shows the state, also after a line typed in Command, a Try: button, `rlv_command` or a script. With the keys the list is one Tab stop: the arrow keys pick a row, and Space or Enter switches it between Allowed and Blocked.
- **Blocked** makes the line and runs it from the object in the Object field (`panel` unless changed), the same way the Command field does, so it shows in Replies and messages and Restrictions too. The command is the one of the same name when it blocks the row, otherwise the first plain command of the table that does (no `_sec` form, no old name). Examples: "Send instant messages" makes `@sendim=n`, "Take off locked attachments" `@detach=n`. A test runs every row in both modes and checks that the line really blocks that row and that Allowed lifts it again.
- **The row turns red:** the action's words and the word Blocked in the `danger` colour and bold; the word stays, so it never rests on colour alone (charter P10).
- **The box under the list** shows "Send instant messages: Blocked", the line (fixed-width), and the line in plain words, one short sentence each: "This line blocks: Send instant messages.", the editor's sentence for the command (the RLV help of job R3), "@ starts an RLV line, and sendim is the command.", "=n means no: the avatar may not do this." A last line says what happened ("Ran from panel. Blocked now.").
- **Allowed** lifts what the Object field's object holds of that command (`@sendim=y`; every option it holds, `=rem` for an open channel) and says "This line lifts the block again." and "=y means yes: the avatar may do this again." When another object holds it too the row stays Blocked and the box says "Still blocked, because collar holds it too." (in the `danger` colour); only that object can lift it, as in Second Life. "Nothing to lift. panel held nothing of it." when there was nothing.
- **A value field** for the commands that take one: a distance for `sittp`, `touchfar` and `tplocal` ("Farther than (metres)", empty: the viewer's own limit; `@touchfar:3=n`), and a channel that stays open for `sendchannel` ("Still allow channel"; `@sendchannel=n,sendchannel:5=add`). **Apply** or Enter lifts the object's old limit and runs the new line. A wrong value is refused in words ("Type a distance in metres, such as 1.5.", "Type a channel number from 1, such as 5.") and nothing changes. A row with a distance limit says Blocked, because the action is blocked farther away (the sandbox's own Allowed / Blocked list counts only lines without an option).
- **Use in script** puts `llOwnerSay("@sendim=n");` into the script of the Live page as one undo step, shows the Live page and says "Put in the script at line 5. Save to run it." Nothing is saved by itself. Where it goes: under the cursor's line when the cursor is inside an event; otherwise at the start of `state_entry`; otherwise at the start of the first event; otherwise at the cursor. Braces in strings and comments do not count. Without an open script the button is greyed and says why under it ("Open a script on the Live scripts page first.").
- **Modes:** a row only RLVa has (for example "Jump") is greyed in plain RLV mode, with "Only RLVa has this. Switch Viewer rules to RLVa." as its tip; and the other way round.
- **Still there:** the Command field and the Try: buttons work as before.
- **Not built:** choices for exceptions with a key (a UUID means nothing to a beginner), a value field for other value types, and holding the box's line for several rows at once (it speaks of the last row chosen).

## Record and replay (job A3)

Built 24 November 2026, pull request 450 (plan "Script Lab: see and understand", approved by the project owner on 24 November 2026, 00:08: «Ta opp det jeg gjør»). Code: `src/scriptlab/labrecipe.*` (QtCore: the file format, the replay, the facts and the comparison), the recording and replay part of `src/scriptlab/labwatcher.*`, `ScriptLabPanel::setRecording`, `replayNow` and `refreshRecipe` in `src/scriptlab/scriptlabpanel.*`, the control command `lab_replay` in `src/controlapi.cpp` ([CONTROL.md](CONTROL.md), "Record and replay"). Tests: `tests/tst_labrecipe.cpp` (format, recording, replay, the exact comparison), `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` (`recordAndReplay`: the button and the line), `tests/tst_control.cpp` (`lab_replay`). Fuzz target: `fuzz_recipe`.

- **Record what I do.** While a folder is watched as one object, a button **Record what I do** stands on the **Player** tab (job UX-B; it stood under **Show effects in 3D view** on the Live page before). Pressed, it says **Stop recording** and has the same on look as the effects button. While it is on, every click on a prop (a touch, job A1), every press on a button of the menu box (job A2) and every RLV line typed in Command or made by an Allowed / Blocked choice is one step. The line under the button counts the steps ("Recording. Click props and menu buttons. Steps so far: 3."), and says "Turn on Show effects in 3D view to click props." while the effects are off.
- **The recipe** is `recipe.rlvtest` at the top of the object folder: plain text, one step per line, `#` lines are notes. `touch Collar`, `menu Lock`, `send @sendim=n` (from the object of the last `object <name>` line, `panel` at first) and `wait 2`. It holds no personal data: part names, button texts and RLV lines only. The file is written at each step; the first step replaces an older recipe, so starting and stopping without a step keeps it. The format is the scenario format proposed in [RLV_TOOLS_REPO.md](RLV_TOOLS_REPO.md) (`#`, `object`, `send`), with `touch`, `menu` and `wait` added; its `expect` lines are not built and are refused with a reason, so there is one format, not two. It is neither a script nor a notecard, so the folder model does not read it.
- **Replay.** Stopping replays the new steps once. After that, every save of a script (every folder run) replays the recipe: the whole folder from the start in a world of its own, with an RLV state of its own (the panel's restrictions, the event log and the props are not touched), then each step one simulated second after the one before, as a click does. A step that cannot be done (no part of that name, no menu open, an RLV line refused) is skipped and named. The sandbox is deterministic, so the same scripts and recipe give the same answer every time.
- **What is true at the end** (the facts): an object that holds `@detach` is locked ("Cuffs is locked."), every action that is blocked ("Send instant messages is blocked."), the colour of each prim a script coloured, by its nearest plain name ("Cuff is red."), the last line each prim said to the owner or on channel 0 ("Root is saying "Ready."."), a menu that is still open, and every script stopped.
- **What changed since last time.** The facts are compared with those kept from the replay before, one sentence per change: "Before: Cuffs was locked. Now: it is not locked." When the object does not run, one sentence says so instead of a list. The facts and the changes are kept in `recipe.rlvresult` next to the recipe, which is the "before" of the next replay.
- **The line under the button** after a replay: "Replayed your 3 step(s).", then either "Nothing changed since last time.", or "What changed since last time:" and the changes, or, the first time, "This is the first time, so there is nothing to compare.", "Now:" and the facts. "The object did not run, so your steps were not replayed." and errors (a recipe with a mistake: the line and the reason) are in the `danger` colour with the word Error.
- **Beginner and Expert.** Beginner mode shows only the button and the line. Expert mode adds a folded part **Recipe** under it: the file's name, the recipe's text (read only, fixed-width) and **Replay now** (greyed with its reason while recording or without a recipe). Switching moves nothing else (charter P12).
- **An AI assistant** runs the same recipe with `lab_replay`, and reads the newest replay in `lab_result` ([CONTROL.md](CONTROL.md)). It may write the recipe file itself.
- **Not built:** `expect` lines (a recipe as a test with a pass or fail), recording a typed llTextBox answer from the panel's own menu or the Send input buttons (they talk to the world of `lsl_run`, not the watched folder), held touches and prim places in the facts. The player that steps through the replay is built (job B3, "The player" below). Nothing here was tested in Second Life.

## The player (job B3)

Built 24 November 2026, pull request 454 (plan "Script Lab: see and understand", approved by the project owner on 24 November 2026, 00:08; his wish of 23 November: «Det siste må visualiseres skikkelig. Og som en slags spiller.»). Built on the player check ([PLAYER_CHECK.md](PLAYER_CHECK.md), recommendation (b): run once with a recorder, then step in the snapshots) and on record and replay above, whose recipe is the stored input. Code: `src/scriptlab/labplayer.*` (the widget, and `LabPlayerModel` without a window), the moments in `LabRecipe::replay` (`src/scriptlab/labrecipe.*`), a few lines in `LabWatcher::replayRecipe` (first difference) and the hook-up in `src/scriptlab/scriptlabpanel.*` (`player()`, `shownFolder()`). Tests: `tests/scriptlabplayer/tst_scriptlabplayer.cpp` (the rows, lines, buttons and reasons, playing, the list and the keys, first difference, the props in the real panel), `tests/tst_labrecipe.cpp` (`momentsOfAReplay`, `momentEqualsAShorterReplay`, `fingerprintsRoundTrip`, `firstDifferenceRules`, `firstDifferenceAfterAChangedScript`). Fuzz target: `fuzz_recipe` (the fingerprint lines and the moments).

- **Moments.** While the recipe is replayed, the replay keeps one moment right after the start and one right after every step: the step, the simulated time, how far the event log had come, whether the step was done, and the facts true then (made the same way as the facts at the end). Nothing is run again when the user steps: the sandbox is deterministic, and a test checks that moment k equals the end of the same recipe cut after step k. Steps are moments, not exact seconds (an event always runs to its end).
- **Where.** Under the recipe line on the **Player** tab (job UX-B; it was on the Live page before), while a folder is watched as one object and a replay has moments. Beginner mode shows it (it is part of the first meeting: 3D, menu, player and step list).
- **The play buttons** are the animation timeline's own bar (`TransportBar`, the same look and icons): **Start** plays one moment per second to the last and stops (on the last moment it starts from the start, as the timeline does), **Stop** stops on the moment, **Replay from start** goes to the start and plays, **Step back** and **Step forward** move one moment and stop at the first and last. The Loop switch is hidden (a replay plays once). Every greyed button says why in its tip.
- **The list of moments:** "Start", "Step 1: Click Root.", "Step 2: Press "Lock" in the menu." A step that could not be done ends with "Not done." and the warning icon. A pick jumps there; the arrow keys move one step. The line above says where the player is: "Step 2 of 3, at 12 s: Press "Lock" in the menu."
- **True at this moment:** locks first ("Locked: Cuffs" in `danger` with a crossed circle; "Free: Cuffs" in `success` with a check when that part is locked at another moment of the replay), then actions ("Blocked: Send instant messages" with the ban icon; "Allowed: ..." when blocked at another moment), then a step that could not be done (`warning`), then colours, an open menu and what each part said. Rows that changed at this step are bold and end with "(changed at this step)", so the change never rests on colour or weight alone (charter P10).
- **First difference.** The result file keeps a fingerprint of every moment (`@<moment> <16 hex digits>` lines). After the next replay, **Go to first difference** jumps to the first moment whose facts differ from the replay before (the list marks it "First difference."). When the recipe got longer, it is the first new moment; when shorter, the last one. A result file from before the player has no fingerprints, so there is nothing to compare.
- **The 3D view follows.** While the user looks at a moment, the props show that moment's colours and places (the replay's log up to the moment, read the same way as a run's). **Show now** goes back to the live run; a new replay and a click on a prop do too, because a click is an input to the world of now, not to the replay.
- **Expert mode** adds the event log up to the moment (read only, fixed width), with a "--- Step 2: menu Lock ---" line before each step.
- **An AI assistant** reads the same moments in `lab_replay` and in `replay` of `lab_result`: `frames` and `first_difference` ([CONTROL.md](CONTROL.md), "Record and replay").
- **Not built (yet):** the animation a script started, the worn and lock markers, the 3D view overrides (camera, environment, view modes) and the menu box over the 3D view do not follow the moment; they show the live run. The open menu at a moment is listed in "True at this moment" instead. Moments between steps (inside a long wait) are not kept. Nothing here was tested in Second Life.

## The channel map: who talks to whom (job A4)

Built 24 November 2026, pull request 453 (plan "Script Lab: see and understand", approved by the project owner on 24 November 2026, 00:08; his wishes of 23 November: «Det siste må visualiseres skikkelig», «server som lytter og sender kommandoer må med»). Code: `src/scriptlab/labchannelmap.*` (QtCore: reads the run's event log and makes the boxes and arrows), `src/scriptlab/channelmapview.*` (the drawing, the clicks and the keys), `ScriptLabPanel::refreshChannelMap`, `activateMapBox` and `activateMapArrow` in `src/scriptlab/scriptlabpanel.*`. Tests: `tests/tst_labchannelmap.cpp` (which boxes and arrows a real folder run and given logs give, letter for letter) and `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` (`channelMap`: the part, the clicks, the keys, the fold, the word tips, the colours).

- **Where.** A folding part **Who talks to whom** on the tab of the same name (job UX-B; it was on the Live scripts page, right after the Event log), while a folder is watched as one object. A click on an arrow brings Live scripts to the front for the line, unless Live scripts is popped out and in sight. Beginner mode keeps it folded and Expert mode opens it; switching the mode folds or opens it, and the user can still open or fold it by hand. Nothing else moves (charter P12). Lines between the parts in the 3D view are not built for messages (that choice: [UX_DECISIONS.md](UX_DECISIONS.md), 6r).
- **Boxes.** Every prim of the object, in link order (as the tree), then the boxes outside the object as they come up: **You** (the avatar: menus, open chat and clicks), **Your viewer (RLV)** (the RLV lines a script sends) and **Web server** (llHTTPRequest; the sandbox never goes online, it answers from canned replies). A box outside the object has a dashed border. A prim that hears chat or a link message from one box and passes messages on to another is a **hub**: its name gets "(hub)" and an `accent` border. That is the "server" of a Second Life setup that listens and sends commands.
- **Arrows.** One row per arrow: the sender, the arrow with its words over it, the receiver. The words say what kind and on which channel, and the first text: "say on channel -77: "lock"", "link message 7002: "lock"", "menu on channel -7001: "Gear set"", "menu answer on channel -7001: "Lock all"", "RLV line: "@detach=n"", "answer on channel 2222: ...", "web request: "GET https://..."", "click". The same sender, receiver, kind and channel make one arrow with "(3 times)". At most 200 arrows; a line under the map says when there were more.
- **Who hears what** follows the sandbox's own rules: the listens each script opened (llListen; llListenRemove, a state change, a reset or a stop removes them), their name, key and message filters, a prim never hears itself, open chat (channel 0) reaches you, llRegionSayTo only the object with that key, a link message the prims its link number names that hold a script, a menu answer is said by you on the menu's channel. Distances are not checked, and llListenControl is not in the log, so a listen switched off still counts.
- **Nobody listens.** A message nobody gets is an arrow to nobody: a dashed line in the `danger` colour that ends in a bar, the circle-x icon and the reason in words: "Nobody listens on channel 7.", "Nobody listens on channel -7001, so the menu answer is lost.", "Nobody with that key listens on channel 5.", "No script gets link message 4.", "Your viewer refused this line: ...". The part's header counts them ("4 messages, 1 problem") and a line under the map says "1 arrow reached nobody. It has a cross and says why."
- **Clicks.** A click on an arrow (or Enter on it) shows the script line that sent it in the editor: the call of the function that wrote the log line, found in the parsed script (comments do not count), the one whose line holds the message text when there are several. A click on a prim's box picks that prim in the tree and lights up its prop when selection follows (job U3). Keys: Up and Down between rows, Left and Right between sender, arrow and receiver, Home and End, Enter or Space; the focused part has the focus ring. Every part has a tooltip in words.
- **Word tips.** "Channel" and "Listener" get the one-line explanation the first time the map is open and shows an arrow, as the other word tips (one at a time, closed for good, off with Help > Show hints).
- **Chains:** lines in the 3D view between two parts a script links are in "Chain and ribbon lines in the 3D view" below.
- **Not built:** the map for a run that is not a watched folder (the Objects part's `lsl_run` world), the map in a control command for an AI assistant, distances, and moving pictures of messages. Nothing here was tested in Second Life.

## Chain and ribbon lines in the 3D view (job A4, part 2)

Built 24 November 2026, pull request 455 (plan "Script Lab: see and understand", approved by the project owner on 24 November 2026, 00:08; his question of 23 November: «kart som streker i 3D?»). Part 1 of job A4 is the channel map above. Code: the particle entries in `src/scriptlab/lsl/builtins.cpp`, `src/scriptlab/labchains.*` (QtCore: which chains a run leaves, and whether the object is locked), `src/chainlines.h` (the line's pieces, colour and word tag), `ScriptLabPanel::chains` and `chainsChanged`, and `AnimationView::setChainLines`, `drawChainLines` and `placeChainLabels` in `src/animationview.*`. Tests: `tests/tst_labchains.cpp` (the exact log entries, the chains of a folder run, locked and free, replaced and removed chains, the dashes, colours and tag) and `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp` (`chainLines`, and the window's wiring in `windowWiring`).

- **What makes a chain.** A script in a prim calls `llParticleSystem` (or `llLinkParticleSystem` for one link) with `PSYS_SRC_TARGET_KEY` and the key of another prim of the same object (`llGetKey()` for the root, `llGetLinkKey(n)`), as LockGuard and LockMeister style chains do. The sandbox logs the prim that emits (`emitter_link`), the key (`target`) and, when it is a prim of the object, its link (`target_link`). A newer particle system of the same prim replaces its chain; an empty rule list removes it ([CONTROL.md](CONTROL.md), "Prim state, L$ and recorded-only functions").
- **What the 3D view shows.** While **Show effects in 3D view** is on and the props from prims are shown (job A1), a straight line between the centres of the two props, on top of the figure. No physics, no sag, no particles.
- **Locked or free.** While the object holds `@detach` (without an option, the same rule as the recipe's facts), the line is solid in the `danger` colour with a tag **Locked**; otherwise dashed in the `success` colour with a tag **Free**. The tag stands in the middle of the line: the word in the line's colour, a 1 px border in the same colour, on `surface`. The words carry the meaning, the colour and the dashes back it up (charter P10). A lock or an unlock (a click, the panel's RLV lines, Reset) changes the line at once.
- **Not built:** chains to an avatar or to another object (the target must be a prim of the watched object), chain physics, textures of the particles, a chain that is shown only while particles are on screen in Second Life (the sandbox has no particle time). Nothing here was tested in Second Life.

## Convert LSL to SLua (lane 1, job G: level 1 built)

Code: `src/scriptlab/lsl/lsl2slua.*` (the entry, `Lsl2Slua::convert()`), `slualevel1.*` (the layer with the SLua rules), `luair.*` (a small Lua tree), `luaprinter.*` (writes the tree as text), `sluadata.*` with `slua_data.inc` (what changes in SLua for each function and event, made by `tools/gen_slua_data.py` from Linden Lab's definition files). Tests: `tests/tst_lsl2slua.cpp` with exact expected text for seven small scripts in `tests/data/slua/` (states, lists and strings, events, jump, user functions, integer and float math, vector and rotation members), and `tests/test_gen_slua_data.py`. The fuzz target `fuzz_lsl` converts every input. The rules and their sources: [SLUA_RULES.md](SLUA_RULES.md).

- **What it does:** turns an LSL script that compiles into SLua text in "LSL style" (level 1): the script keeps its own shape (positions from 0, integer TRUE and FALSE inside), and the converter changes it only where SLua differs: `ll.` names, 1-based positions into `ll` functions, "not found" back to -1, booleans from `ll` back to 1 and 0 where the script uses them as numbers, the `evts` table of touch, collision and sensor events, states as tables with a `state()` function, LSL's integer division and casts, list and text joins, and vectors that cannot be changed in place.
- **Warnings as data:** each has the line and column in the LSL text, a plain message and a suggested fix, for the panel to show later. `jump` gives "This jump cannot be converted automatically; rewrite it by hand." and a comment in the text. At most 200 warnings; the last says how many more there were.
- **Bad input:** a script that does not compile gives its LSL errors and no text. The lexer and parser limit size and nesting; the output is limited to 4 MB. Nothing is run.
- **Not built:** a panel or button for it (the language choice in the editor stays only a choice), running Lua, level 2 "real Lua style". **The output is not tested in Second Life** (not in a Lua beta region); the tests check the exact text the converter writes, not that Second Life runs it.

## Example objects (job E1)

Six example objects in `examples/objects/`, each one folder in the folder model with props of the same names and a plain-words guide stored as data (`example.json`): First colour, Button HUD, Gear set (the main example: HUD, wrist and ankle cuffs, collar, hood, gag and blindfold), Timed lock, a simple RLV relay and a work system. Everything about them, the guide texts, a recipe of what the program can do now and what a later example button must do: [EXAMPLES.md](EXAMPLES.md). Tests: `tests/tst_examples.cpp`, `tests/test_examples.py`. **Try the example scripts** still watches the loose scripts in `examples/scriptlab/`, one at a time.

## Script Lab for beginners (job U3)

Built 23 November 2026, pull request 443. For someone who has never scripted: **Make example project** on the empty Live page copies the first example (First colour) into `Documents/PinglyMotion/Scripts`, puts its props in the scene, watches it, shows the effects and a steps card with a button for each step (Help > Script Lab steps opens it again); **New folder** makes an empty folder there and watches it (limits on name and path; nothing is ever overwritten or deleted). The tree and the editor stand side by side when the panel is wide; a click on a piece in the tree lights up its prop in the 3D view and the other way round (switch: Follow selection in 3D view); short tips explain prim, root prim, notecard, sandbox, Viewer rules, RLV, RLVa and "N held", once each. Show effects in 3D view is an on/off button. Details and every choice: [UX_DECISIONS.md](UX_DECISIONS.md), 6o. Code: `src/scriptlab/labbeginner.*`, `src/scriptlab/scriptlabpanel.*`. Test: `tests/scriptlabbeginner/tst_scriptlabbeginner.cpp`.

## Later

A description of a separate repository for the owner's RLV tools (template, examples, `.rlvtest` scenarios in CI; not created) is in [RLV_TOOLS_REPO.md](RLV_TOOLS_REPO.md). Since job A3 the program reads and replays `.rlvtest` recipes in a watched object folder ("Record and replay" above), but without `expect` lines. Ideas that come after the planned steps (not planned yet, nothing is being built): see "Script Lab: later" in [ROADMAP.md](ROADMAP.md).

## Licence

The command syntax, return codes, behaviour names and the `@clear` and `@getstatus` rules follow Firestorm's RLVa
(LGPL 2.1); so do the RLVa-only names, parameter types and marks of step 9. The code is a modified port written anew with Qt types; no RLVa file is copied. See
[CREDITS.md](../CREDITS.md).

## One window with its own tabs (job UX-B)

Built 24 November 2026, pull request 464 (the new UX direction, approved by the project owner on 24 November 2026, 13:26: «Fiks dette og legg i køer»; [UX_DECISIONS.md](UX_DECISIONS.md), 6w). Code: `src/scriptlab/labtabs.*` (the tab bar that notices a tab dragged out, the window of a popped tab, the remembered state) and `src/scriptlab/scriptlabpanel.*` (the pages, Pop out, Put back, big by default). Tests: `tests/scriptlabwindow/tst_scriptlabwindow.cpp`.

- **The tabs**, in this order: **Live scripts**, **Type commands**, **Who talks to whom** (the channel map) and **Player** (Record what I do, the recipe and the player). Each has a Lucide icon (`file-code`, `terminal`, `network`, `rotate-ccw`, as Mockup 2 draws them) and a tip. Every part the panel had is on one of them; nothing new was added. The Viewer rules row and the ? button stay above the tabs. The map and player tabs say what fills them while no folder is watched as one object. Improve a setup and HUD from Mockup 2 do not exist yet, so they have no tab.
- **Keys.** Ctrl+Tab and Ctrl+Shift+Tab (also Ctrl+Page Down and Ctrl+Page Up) go round the tabs from any field in Script Lab, also from the code editor. The arrow keys move between tabs on the tab bar, as before.
- **Big by default.** The first time Script Lab opens docked it takes three fifths of the main window's width (at least its first size, 600 by 720 at 100 percent text), and the first time it floats it takes four fifths of the screen, in the middle. Each happens once; after that it opens as the user left it (the saved panel layout). It docks on any side (left, right, top or bottom) or floats; it can be moved, resized and closed as every panel (UX-A).
- **Pop out.** The **Pop out** button beside the tabs (or dragging a tab and letting go well outside the tab bar) puts that tab in a window of its own: a normal window with the tab's name on top and **Put back**, moved and resized by its frame. Closing the window or pressing **Put back** puts the tab back in its place among the others. The last tab cannot be popped out; the button is greyed and says why. Popped windows show while Script Lab is open and hide while it is closed, and are never lost.
- **Remembered.** The tab in front, the popped tabs and the place and size of every tab's own window (setting `scriptlab_windows`), and which first sizes were given (`scriptlab_sized`). A place on a screen that is gone is pulled onto a screen. View > Reset layout puts every tab back, Live scripts in front, and Script Lab opens big again.
- **Not built:** Improve a setup and HUD tabs (they do not exist), the Animate / Script switch (job UX-F), helper mode (UX-C) and the guide page (UX-D).
