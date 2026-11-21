# Script Lab

Script Lab is a sandbox, being built, inside PinglyMotion for testing RLV and LSL scripts for Second Life. It is not a full
LSL IDE: there is no built-in script editor, and the scripts stay in the user's own folder or repository. The plan,
the steps and the limits are in [SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md); the track is in [ROADMAP.md](ROADMAP.md).

## What exists now (steps 1 and 2, issues 287 and 288)

The RLV core (step 1) and the Script Lab panel (step 2).

- `src/scriptlab/rlvcommand.*`: the parser for owner-say lines such as `@detach=n,sendim=n`.
- `src/scriptlab/rlvbehaviours.*`: a table of 26 common behaviours (which parameters each takes, whether it takes an
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
  `tools/gen_lsl_builtins.py` from the viewer's keyword list. 72 functions run; the others give "not supported yet".
- Control commands `lsl_check` (errors and warnings with line and column) and `lsl_run` (the event log) over the local
  HTTP interface and the MCP server ([CONTROL.md](CONTROL.md), "Script Lab: lsl_check and lsl_run": what runs, the log
  entries and the limits).
- Tests: `tests/tst_lsllexer.cpp`, `tests/tst_lslparser.cpp`, `tests/tst_lslinterp.cpp`, `tests/tst_lslrlv.cpp` and the LSL part of
  `tests/tst_control.cpp`; fuzz target `fuzz/fuzz_lsl.cpp`. Examples: `examples/scriptlab/`.

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
- `lsl_event` from the plan became the `inputs` option of `lab_watch`: the chat and touches given to every run. Each
  run starts fresh with a simulated clock, so there is no script that keeps running between commands to send an
  event to.
- Code: `src/scriptlab/labwatcher.*` and the Live page in `src/scriptlab/scriptlabpanel.*`. Tests:
  `tests/tst_labwatcher.cpp` (a saved script runs within the debounce, errors with line and column, saving again,
  Stop watching, the limits, links, an endless loop), the Live part of `tests/scriptlabpanel/` and of
  `tests/tst_control.cpp`.

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

## RLV and RLVa mode (step 9, issue 297)

Every behaviour in the table is marked `RLV` (in the plain RLV API) or `RLVa only` (RLVa's extended and experimental
command set). The marks come from RLVa's own behaviour dictionary (`rlvhelper.cpp`: the entries flagged
`BHVR_EXTENDED` or `BHVR_EXPERIMENTAL`), not from memory.

- The mode is plain RLV at start. In plain RLV mode a RLVa-only behaviour answers as unknown, like any name the table
  does not have. In RLVa mode it is accepted. Switching the mode removes every restriction and recorded effect (as
  after a relog). Set it with the control command `rlv_reset` and its `mode` field ([CONTROL.md](CONTROL.md)).
- RLVa-only behaviours by group: teleport and chat (`accepttprequest`, `tprequest`, `tplocal`, `sendchannel_except`),
  interaction and view (`interact`, `touchhud`, `sendgesture`, `showself`, `showselfhead`, `shownearby`,
  `viewtransparent`, `viewwireframe`, `allowidle`), shared folder (`sharedwear`, `sharedunwear`), camera
  (`setcam_avdistmin`, `setcam_avdistmax`, `setcam_origindistmin`, `setcam_origindistmax`, and the force commands
  `setcam_focus`, `setcam_fov`, `setcam_mode`) and questions (`findfolders`, `getcommand`, `getheightoffset`,
  `getaddattachnames`, `getaddoutfitnames`, `getattachnames`, `getoutfitnames`, `getremattachnames`,
  `getremoutfitnames`, `getcam_avdist`, `getcam_avdistmin`, `getcam_avdistmax`, `getcam_fov`, `getcam_fovmin`,
  `getcam_fovmax`, `getcam_textures`).
- Plain RLV behaviours with effects outside the sandbox are known in both modes: `setenv`, `setdebug`, `setcam`,
  `setcam_avdist`, `setcam_eyeoffset`, `setcam_eyeoffsetscale`, `setcam_focusoffset`, `setcam_fovmin`,
  `setcam_fovmax`, `setcam_mouselook`, `setcam_textures`, `setcam_unlock`.
- Effects outside the sandbox (camera, sky and water, debug settings, view modes) are only recorded as state and
  shown in the log ("recorded only"); the real program never changes. A force command of that kind is `retained`
  and its option is kept in `rlv_state` under `recorded`.
- The sandbox has no avatar, inventory or camera, so the RLVa questions about them answer empty (`getheightoffset`
  answers `0`). `getcommand` lists the behaviours known in the current mode.
- `setcam_*` behaviours with a distance need it as option (`@setcam_avdistmin:2=n`); without it the command fails.

## Limits

- Only the behaviours in the table; others parse but are reported as unknown.
- `force` commands are accepted (`retained`) but not carried out: there is no world to teleport in or sit on.
- LSL: one script at a time (one object id for it), at most 100 RLV lines per run, the events `state_entry`, `state_exit`, `timer`, `listen`, `touch_start` and
  `run_time_permissions`, and 72 built-in functions; hard limits on steps, memory, nesting and time (step 5 adds more
  events and several objects). The sandbox is never identical to Second Life; the final test is always in Second Life.

## Later

Ideas that come after the planned steps (not planned yet, nothing is being built): see "Script Lab: later" in [ROADMAP.md](ROADMAP.md).

## Licence

The command syntax, return codes, behaviour names and the `@clear` and `@getstatus` rules follow Firestorm's RLVa
(LGPL 2.1); so do the RLVa-only names, parameter types and marks of step 9. The code is a modified port written anew with Qt types; no RLVa file is copied. See
[CREDITS.md](../CREDITS.md).
