# Script Lab

Script Lab is a sandbox, being built, inside PinglyMotion for testing RLV and LSL scripts for Second Life. It is not a full
LSL IDE: there is no built-in script editor, and the scripts stay in the user's own folder or repository. The plan,
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
- **Actions**: every action the sandbox knows, with the word Allowed or Blocked; the colour (success or danger token)
  and a bold Blocked only back up the word.
- **Replies and messages**: each line run from the panel, the result of each command, the reason for a refusal, and
  the replies a script would hear (`Reply on channel 2222: ...`).
- **Reset**: removes every restriction of every object, like `rlv_reset`.
- `@clear` removes the restrictions of the object that sends it, as in Second Life: `@clear` from the panel leaves a
  `collar`'s restrictions in place; Reset removes all.
- The panel follows the state's `changed` signal, so lines sent with `rlv_command` or `rlv_reset` by an AI client show
  up in the lists at once. Replies to lines sent through the control commands go to that client, not to the panel log.
- Code: `src/scriptlab/scriptlabpanel.*`; test: `tests/scriptlabpanel/` (menu entry, restrictions and actions after
  `@detach=n,sendim=n`, `@clear`, Reset, replies, a line through `rlv_command`, colour tokens, and the panel's place
  after saving and loading the layout).

## What exists now (step 3, issue 289): the LSL interpreter

A small LSL interpreter, used through the control commands and, for scripts in a watched folder, the panel's Live
page (step 6, below). Since step 4 (issue 290) scripts talk to the RLV core.

- `src/scriptlab/lsl/`: lexer, recursive descent parser (no flex or bison), a checker with the Second Life compiler's
  name and type rules, the interpreter with a simulated clock, the built-in functions, and `world.*`, which turns what
  a script does into the event log (step 7 plays the animations). `rlvworld.*` (step 4) sends `llOwnerSay("@...")` to
  the RLV core and brings its replies back as listen events.
- `src/scriptlab/lslbuiltins_data.inc`: every LSL function with its signature, every event and the constants, made by
  `tools/gen_lsl_builtins.py` from the viewer's keyword list. 111 functions run (72 from step 3, five more from step 5, nine from step 9a, 25 from step 9b-1); the others give "not supported yet".
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
- The panel: below the replies, **Objects** lists the objects of the last run (name; where it is, or "Worn (point
  35)"; "Running" or "Stopped: <why>" in the danger colour and bold; the restrictions it holds) and **Dialog** shows the
  newest dialog (which object, the message, the buttons and the channel). The part is hidden until a script has run,
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
Code) edits in their own editor and sees the result in PinglyMotion at once. There is no built-in editor.

- The panel has two pages: **Commands** (the RLV lines of step 2) and **Live**. On Live, **Watch folder...** picks
  the folder (a cloned repository, for example); **Stop watching** ends it. When a watch starts, here or through
  `lab_watch`, the Live page comes to the front.
- About 0.3 seconds after a `.lsl` file in the folder is saved, it is checked and, when it compiles, run. The page
  shows a status line that starts with a word (`OK` or `Error`; the colour only backs it up), the script read-only
  with line numbers (an error's line number is bold in the danger colour and the line is underlined), the errors
  and warnings with line and column (Enter or a double-click shows the line in the script), the event log of the
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
is ticked. The box is off each time the program starts, so someone who only animates never sees a change. Runs that
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
- A touch can name the prim (`link`); a prim without a touch handler passes it to the root.
- Code: `src/scriptlab/lsl/sandbox.*` (scripts, prims, link messages), `world.*` (the Linkset Data store),
  `builtins.cpp`. Tests: `tests/tst_lsllink.cpp`. Fuzz: `fuzz_lsl` also runs every script as three scripts in one
  object of two prims and drives the Linkset Data store from the input.

## What exists now (step 9b-1, issue 310): the signature table and pure functions

- The signature table knows every function, event and constant of Second Life's own lsl-definitions (526 functions, 43
  events, 1 082 constants), not only the viewer's keyword list: `src/scriptlab/lslbuiltins_extra.inc`, made by
  `tools/gen_lsl_extra.py` from `lsl_definitions.yaml` (MIT, Linden Research; [CREDITS.md](../CREDITS.md)). The
  checker so accepts scripts that use `llReplaceSubString`, `llListSortStrided`, `llStartObjectAnimation`, `llSetLinkGLTFOverrides`
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

- **To object** (the objects of the world) and **Touch**: sends a touch to that object.
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

## Camera limits in the 3D view (job 7b)

The camera restrictions a script holds can limit the main 3D view's camera, only while the Commands page's **Show
camera limits in 3D view** box is ticked (under the Actions list). The box is off each time the program starts. The
Commands page now scrolls when the panel is small, as the Live page does.

- What is shown: `setcam_avdistmin` (and the older name `camdistmin`): the camera stays at least this many metres
  from the avatar; `setcam_avdistmax` (`camdistmax`): at most this far; `setcam_unlock` (`camunlock`): the camera
  focus stays on the avatar. With several objects the strictest limit wins (the largest minimum, the smallest
  maximum); a maximum below the minimum wins over it. A maximum smaller than the 3D view can come (3 inches) is shown
  as close as it goes. An option that is not a distance in metres is named in the text as not used.
  `setcam_avdistmin` and `setcam_avdistmax` are RLVa only, so they need RLVa mode (`rlv_reset` with `mode`); the old
  names `camdistmin` and `camdistmax` work in plain RLV mode too.
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

## RLV and RLVa mode (step 9, issue 297)

Every behaviour in the table is marked `RLV` (listed as plain RLV in the RLV wiki, and in RLVa), `RLVa only` (not listed in the wiki) or `RLV only` (in the wiki, but RLVa does not have it).
The marks follow the wiki. RLVa's own flags (`BHVR_EXTENDED` or `BHVR_EXPERIMENTAL` in `rlvhelper.cpp`) differ from the
wiki for some names: 20 names that RLVa flags as extended are plain RLV in the wiki and are marked `RLV` here
(`accepttprequest`, `allowidle`, `findfolders`, `getcam_avdistmax`, `getcam_avdistmin`, `getcam_fov`, `getcam_fovmax`,
`getcam_fovmin`, `interact`, `sendchannel_except`, `sendgesture`, `setcam_avdistmax`, `setcam_avdistmin`, `setcam_fov`,
`sharedunwear`, `sharedwear`, `shownearby`, `touchhud`, `tplocal`, `tprequest`).

- The mode is plain RLV at start. In plain RLV mode a RLVa-only behaviour answers as unknown, like any name the table
  does not have. In RLVa mode it is accepted. The third mark works the other way round: an `RLV only` behaviour
  (`camdrawmin`, `camdrawmax`, `camdrawalphamin`, `camdrawalphamax`, `camdrawcolor`, `defaultwear`, `denypermission`,
  `versionnumbl`, `getblacklist`, `getcam_zoommin`, `setrot`) is accepted in plain RLV mode and unknown in RLVa mode (it is also left out of
  `getcommand` there). `rlv_state` lists `rlva_only` only; the `RLV only` names are the ones in `behaviours` of plain mode
  that are missing in RLVa mode. Switching the mode removes every restriction and recorded effect (as
  after a relog). Set it with the control command `rlv_reset` and its `mode` field ([CONTROL.md](CONTROL.md)).
- Behaviours marked RLVa only (19): interaction and view (`showself`, `showselfhead`, `viewtransparent`,
  `viewwireframe`), camera (`setcam_origindistmin`, `setcam_origindistmax`, and the force commands `setcam_focus` and
  `setcam_mode`) and questions (`getcommand`, `getheightoffset`, `getaddattachnames`, `getaddoutfitnames`,
  `getattachnames`, `getoutfitnames`, `getremattachnames`, `getremoutfitnames`, `getcam_avdist`, `getcam_textures`).
- Plain RLV behaviours (all marked `RLV`, among them the 20 above) are known in both modes. Those with effects
  outside the sandbox: `setenv`, `setdebug`, `setcam`, `setcam_avdist`, `setcam_eyeoffset`, `setcam_eyeoffsetscale`, `setcam_focusoffset`, `setcam_fovmin`,
  `setcam_fovmax`, `setcam_mouselook`, `setcam_textures`, `setcam_unlock`.
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
  and `pay`. `acceptpermission` and `denypermission` (`RLV only`) are held as state: the sandbox grants every
  permission to a script anyway.
- Group, movement and teleport (all marked `RLV` except `setrot`): `setgroup` (restriction; `=force` with a group
  name or `none` is retained), `temprun`, `alwaysrun` and `jump` block their own action, `standtp` and `accepttp` are
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
  `_valuemax`, and the overlay `setoverlay` with `setoverlay_alpha`, `_texture`, `_tint`, `_tween`, `_touch`. They are
  held as restrictions (`n`/`y`), the `_<name>` ones also take `=force` (retained, the value is recorded), and they
  block nothing; the real view never changes.
- Three more questions (`RLV only`): `versionnumbl` answers `3040300,` (the version number, a comma, then the
  blacklist, which is empty), `getblacklist` answers empty, and `getcam_zoommin` always answers `1` (the sandbox has
  no camera).
- Effects outside the sandbox (camera, sky and water, debug settings, view modes) are only recorded as state and
  shown in the log ("recorded only"); the real program never changes, except the camera limits the user lets limit
  the 3D view with the "Show camera limits in 3D view" box (above, "Camera limits in the 3D view"). A force command of that kind is `retained`
  and its option is kept in `rlv_state` under `recorded`; a new or changed value is a change of the state (the panel
  and the 3D view follow it), and the object's `@clear` forgets its recorded values.
- The sandbox has no avatar, inventory or camera, so the RLVa questions about them answer empty (`getheightoffset`
  answers `0`). `getcommand` lists the behaviours known in the current mode.
- `setcam_*` behaviours with a distance need it as option (`@setcam_avdistmin:2=n`); without it the command fails.

## Limits

- Only the behaviours in the table; others parse but are reported as unknown.
- `force` commands are accepted (`retained`) but not carried out: there is no world to teleport in or sit on.
- LSL: up to 16 objects and 64 scripts (32 per object) in one run, at most 100 RLV lines per script and run, the
  events `state_entry`, `state_exit`, `timer`, `listen`, `touch_start`, `run_time_permissions`, `attach`,
  `link_message`, `linkset_data`, `on_rez` and `changed`, and 111 built-in functions; hard limits on
  steps, memory, nesting and time per script (and per `lsl_event`). Avatars other than the owner exist only as names
  and keys in inputs, and every avatar stands where the owner stands. The sandbox is never identical to Second Life;
  the final test is always in Second Life.

## Later

A description of a separate repository for the owner's RLV tools (template, examples, `.rlvtest` scenarios in CI; not created, the scenario runner does not exist) is in [RLV_TOOLS_REPO.md](RLV_TOOLS_REPO.md). Ideas that come after the planned steps (not planned yet, nothing is being built): see "Script Lab: later" in [ROADMAP.md](ROADMAP.md).

## Licence

The command syntax, return codes, behaviour names and the `@clear` and `@getstatus` rules follow Firestorm's RLVa
(LGPL 2.1); so do the RLVa-only names, parameter types and marks of step 9. The code is a modified port written anew with Qt types; no RLVa file is copied. See
[CREDITS.md](../CREDITS.md).
