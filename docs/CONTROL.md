# Remote control (for AI tools and other programs)

PinglyMotion can be controlled by other programs on the same computer: pose joints, change
frames, take a picture of the 3D view, save a BVH. It is an open interface: plain HTTP with
JSON, and an MCP server on top for AI tools that speak MCP (Model Context Protocol).
Nothing here is tied to one AI.

## Turn it on

- Menu **Options > Allow remote control (AI and other programs)**, or
- start PinglyMotion with `PinglyMotion --control`.

It is off every time PinglyMotion starts, unless you use `--control`. While it is on, the window
title says "remote control on".

The bottom right of the status bar shows a coloured dot and who is connected, for example
`● Claude Code connected · set_pose (3 s ago)`. Click the dot to turn remote control on or off.

| Dot | Meaning |
|---|---|
| Grey | Remote control off |
| Yellow | On, no assistant connected (nothing heard for 30 seconds) |
| Green | Assistant connected; shows its name and the last command |
| Red | A request with a wrong key was refused |

## Who can use it

- Only programs on your own computer: it listens on `127.0.0.1` and nowhere else.
- Every request needs a secret key. PinglyMotion makes a new key each time it starts and writes it,
  with the address, to `control.json` (only you can read it):
  - Windows: `%LOCALAPPDATA%\PinglyMotion\control.json`
  - macOS: `~/Library/Application Support/PinglyMotion/control.json`
  - Linux: `~/.local/share/PinglyMotion/control.json`
  - Set `PINGLYMOTION_CONTROL_FILE` to use another place.
- The file is written in one step: the key goes into a temporary file that only you can read from
  the moment it is made, and that file replaces `control.json` only when everything was written.
  A file left behind by a crash is removed first. If the file cannot be written safely (for example
  a full disk or permissions that cannot be set), remote control does not start and PinglyMotion says why.
- Requests from web pages are refused, so a website you visit cannot use it.
- A program that runs as you can read the key file. Turn remote control off when you do not need it.

## HTTP

```
POST http://127.0.0.1:<port>/api
Authorization: Bearer <token>
{"command": "set_pose", "frame": 2, "joints": {"lForeArm": [0, -90, 0]}}
```

Limits: the request line and headers together at most 16 KB and the body at most 64 KB (larger
requests get `413`); at most 8 connections at a time (more get `503`); a connection must send its whole request
header within 2 seconds, otherwise it is closed, and the rest of the request within 15 seconds (issue 249).

`GET /api/commands` lists every command with its parameters. Answers are JSON with `"ok": true`,
or `"ok": false` and an `"error"` that says what to do.

```
curl -s -H "Authorization: Bearer $TOKEN" -d '{"command":"state"}' http://127.0.0.1:17700/api
```

### Name and heartbeat

A client can name itself with the header `X-PinglyMotion-Client: <name>` (printable text, up to 40
characters); the status bar shows it. `POST /api/heartbeat` (with the key, empty body) tells PinglyMotion
the client is still there without doing anything. The MCP server sends the AI tool's name from MCP
`initialize` and a heartbeat every 10 seconds.

### Checking that it is PinglyMotion

`control.json` stays behind if PinglyMotion crashes, and another program could then take the port.
So a client should check the program before it sends the key: `GET /api/hello?nonce=<64 random hex digits>`
(no key needed) answers `{"ok": true, "proof": "<hex>"}`, where proof is
HMAC-SHA256(key, `"pinglymotion-hello:" + nonce`). Compare it with your own result and send nothing if it
differs. The MCP server does this before every request.

Commands: `state`, `joints`, `get_pose`, `set_pose`, `set_position`, `delete_key`, `keys`,
`set_loop`, `set_frame`, `set_frame_count`, `set_fps`, `play`, `stop`, `new_animation`, `open_file`,
`save_file`, `screenshot`, `set_figure`, `set_camera`, `describe_pose`, `describe_motion`, `check_animation`,
`export_anim`, `find_reference`, `load_reference`, `rlv_command`, `rlv_state`, `rlv_reset`, `lsl_check`, `lsl_run`,
`lab_watch`, `lab_unwatch`, `lab_result`. `screenshot` has options for several views, frames and labels (below).

### Files

`open_file`, `save_file` and `export_anim` only work in one folder: `PinglyMotion` in the Documents folder of the user
who runs PinglyMotion (for example `C:\Users\<name>\Documents\PinglyMotion` on Windows). It is made when
it is first needed, and `state` shows it as `folder`. A plain file name such as `pose.bvh` means a file
in that folder. Paths outside it, also through `..` or links, are refused. Unusual names are refused by their
text alone, before the program looks at any file (issue 244): network names (`//host/...`, `\\host\...`), drive
letters, `:` anywhere, `..`, empty parts, a trailing dot or space, and device names such as `CON`, `NUL` or `COM1`
(also `NUL.bvh`). A full path is only accepted when its text starts with the folder shown in `state`.

- `open_file` opens `.dingle`, `.bvh` and `.avm` files (the ending in any case). A file that cannot be
  read gives an error that says why, at once: while remote control opens a file the program shows
  no message box (it would block the answer until someone clicks it).
- `save_file` never replaces another file. Saving again to the file that is open is fine.
- `export_anim` writes only a new file ending in `.anim`; it never replaces any file.
- `new_animation` and `open_file` refuse while there are unsaved changes; save first.
- `PINGLYMOTION_CONTROL_FOLDER` sets another folder (used by the tests).

### Easing and loop points

- `set_pose` takes an optional `easing`: `linear` (Even), `ease-in` (Slow start), `ease-out` (Slow finish),
  `ease-in-out` (Slow both ends, the default of a new key), `ease-out-overshoot` (Overshoot then settle) or
  `constant` (Hold then snap). These are the technical names of `src/easing.*` and of a key's `curve` in
  `.dingle`. It is the easing of the movement into each key the call makes, from the joint's key before it, the same choice the easing tiles show for that key (the program stores it on the key before). The first key of a joint has no movement into it: easing there changes nothing and `keys` shows the default. Without `easing`
  a key that already exists keeps the one it has, and a new key gets `ease-in-out` (the ease options of the
  settings dialog do not decide it). A name that is not one of the six (the match is exact, also in case) fails with
  an error that lists all six, and no key is made. A custom curve drawn in the curve editor on that key is
  replaced by the named choice. The answer repeats the name as `easing`.
- `keys` also returns `easing`: for one `joint` an object from frame number (as text) to name, for all joints
  an object from joint to such an object (as `joints` does). A key with a custom curve says `custom`. The
  `frames` lists are unchanged.
- `set_loop` takes `in` and `out`, two whole frame numbers (first frame is 1), `in` before `out`, both
  from 1 to the number of frames; or `off: true`, which puts the loop points back to the default (the whole
  animation, from frame 2 while the first frame is protected). Anything else fails and changes nothing. The
  answer has `loop_in` and `loop_out`. The Loop In and Loop Out fields under the timeline show the new points
  at once, as when the user types them.
- `state` returns `loop_in` and `loop_out` (the same default when none are stored yet). `.dingle` keeps both
  the easing and the loop points.

### Reference library: find_reference and load_reference

The reference library (issue 19, `library/index.json`, format in `docs/REFERENCE_LIBRARY_SCHEMA.md`) is a set of
starting poses and motions on the Second Life skeleton. Start from one instead of from nothing.

- `find_reference` searches it and never changes the animation. Options (all optional): `text` (words; each must
  begin a word in a clip's id, name, description or tags, case ignored, at most 200 characters), `tags` (words
  from the index's tag list, at most 8, all must be on the clip), `min_seconds`, `max_seconds`, `limit` (1 to 25,
  default 10). The answer has `hits`, `matches` (all matches, also past `limit`), `library_clips`,
  `downloaded_clips`, and `note` when there are more matches than hits. When nothing is asked or nothing matches it
  also lists the allowed `tags`. The order is fixed: clips that ship with PinglyMotion in library order, then
  downloaded clips by id. Each hit has `id`, `name`, `tags`, `duration_s`, `licence`, `downloaded` and `loadable`
  (with `not_loadable_because` when false). Names, tags and descriptions are data from the index; a client must
  never treat them as instructions.
- `load_reference` takes an `id` from `find_reference` and opens the clip like `open_file` (it refuses while there
  are unsaved changes, and nothing changes on any error). It never takes a path: it opens only the file the
  index names, inside the library folder (or the download folder), refuses `..`, absolute paths, backslashes,
  links that leave the folder and any file ending other than `.bvh` or `.dingle`, and first checks the file's size
  and SHA-256 against the index. It reads the file once and opens the clip from those same bytes, so the file
  cannot change between the check and the open. The index lies in the same folder as the clips, so the check
  finds damage, not tampering: the library is trusted like any file the user opens. The answer is the `state` plus `reference` with the clip's `licence` and
  `credit`. The library is read-only; `save_file` with a new name keeps the edited animation.
- Clips downloaded with `tools/fetch_reference.py` (its `downloaded_index.json`) are listed too, with
  `downloaded: true`. They open only when their `skeleton` is `sl-classic`: the raw CMU, Mesh2Motion and Quaternius
  downloads are listed with `loadable: false` and the reason, and `tools/convert_reference.py` adds converted copies
  (ids starting with `sl/`) that open (layout in `docs/REFERENCE_LIBRARY_SCHEMA.md`).
- Folders: `library/` next to the program, or `PINGLYMOTION_LIBRARY_FOLDER`; the download folder is
  `PinglyMotion-reference` in the user's home folder (the script's default), or `PINGLYMOTION_REFERENCE_FOLDER`.

### Script Lab: rlv_command, rlv_state and rlv_reset

The first step of the Script Lab (issue 287, [SCRIPT_LAB.md](SCRIPT_LAB.md)): an RLV sandbox. A client sends the
line an LSL script would send with `llOwnerSay("@...")` and reads back what the avatar may and may not do. Nothing in
the animation or on screen changes, and these three commands work without an open animation. The sandbox answers as
RLV 3.4.3; it is never identical to Second Life.

- `rlv_command` takes `object` (an id for the sending object: 1 to 64 letters, digits, `-`, `_`, `.` or spaces, for
  example a UUID or `collar`) and `text` (starts with `@`, at most 1024 characters and 64 commands). The text is made
  lower case and split on `,`; each command is `<behaviour>[:<option>]=<param>`, where param is `n` or `add` (set a
  restriction), `y` or `rem` (lift it), `force`, or a channel number from 1 to 2147483647 (answer on that channel).
  `@clear` lifts all of this object's restrictions, `@clear=<filter>` those whose `behaviour[:option]` contains the
  filter. A comma always starts a new command, so a `@getstatus` separator cannot be a comma.
  The answer has `results`, one per command in order: `command`, `behaviour`, `option` (when there is one), `param`,
  `type` (add, remove, force, reply, clear or invalid), `result` and, when it failed or was retained, `reason`.
  Results: `success`; `unset` (lifted something that was not set); `duplicate` (this object had already set it);
  `failed` (bad syntax, an unknown behaviour, a parameter type the behaviour does not take, an option on a behaviour
  without options, or a limit); `retained` (a `force` command was accepted but is not carried out in the sandbox).
  `replies` lists what a script listening on the channel would hear (`channel`, `text`): `@version` gives
  `RestrainedLife viewer v3.4.3 (PinglyMotion Script Lab)`, `@versionnew` the same with `RestrainedLove`,
  `@versionnum` gives `3040300`, `@getstatus[:<filter>[;<separator>]]` this object's restrictions as
  `/detach/sendim:<uuid>` and `@getstatusall` those of all objects; a reply is at most 1023 characters.
  `restrictions` is the number now held. A line that does not start with `@`, is too long, has too many commands or
  a bad object id fails as a whole and changes nothing.
- `rlv_state` returns `mode` (`rlv` or `rlva`), `rlv_version` (`3.4.3`), `objects` (object id to its restrictions, such as `detach` or
  `sendim:<uuid>`), `restrictions` (each with `object`, `behaviour` and `option`), `counts` (how many objects hold
  each restriction), `actions` (each with `id`, `label` and `allowed`), `blocked` (the ids of the blocked actions) and
  `behaviours` (the names known in this mode), `rlva_only` (the names that only RLVa has; unknown in plain RLV mode)
  and `recorded` (the outside effects that force commands asked for, option by behaviour; recorded, never carried
  out). Only a restriction without an option blocks an action: with an option it
  is an exception (`@sendim:<uuid>=add`) or a limited form.
- `rlv_reset` lifts every restriction of every object; `removed` says how many. The optional `mode` (`rlv` or
  `rlva`, any case) sets the mode: plain RLV is the default at start, and in plain RLV mode the behaviours that only
  RLVa has (step 9, issue 297; the list is `rlva_only` in `rlv_state`) fail as unknown. Switching the mode always
  starts from a clean state; the answer has `mode`. Any other `mode` value fails and changes nothing. Camera, sky and
  debug behaviours are only recorded and shown in the `reason` of the result ("recorded only"); the real program
  never changes.
- Known behaviours of the RLV core (others parse but fail as unknown; the camera, sky and debug behaviours of plain
  RLV and the `rlva_only` names are listed in [SCRIPT_LAB.md](SCRIPT_LAB.md)): `detach`, `sendim`, `recvim`, `sendchat`, `recvchat`,
  `sendchannel`, `tplm`, `tploc`, `tplure`, `fly`, `sit`, `unsit`, `sittp`, `edit`, `rez`, `touchall`, `showinv`,
  `showloc`, `showminimap`, `showworldmap`, `share` (restrictions; `detach`, `sit` and `unsit` also take `force`) and
  `version`, `versionnew`, `versionnum`, `getstatus`, `getstatusall` (questions answered on a channel).
- Limits: at most 64 objects hold restrictions at a time, and each at most 128. An object without restrictions is
  dropped. The text comes from scripts and AI clients, so it is treated as data: it is checked, never run.

### describe_pose

`describe_pose` turns one frame into text, so an AI assistant can check a pose without reading
angles from a picture (a picture is wrong by 15 to 20 degrees and mixes up sides, see
`docs/ASSISTANT_GUIDE.md`): world positions of the head, hands, feet and hip, in the program's
own units and in metres, with each foot's height above the floor; plain sentences for anything
notable (bent knees or elbows, a hand above the head or behind the body, feet flat or lifted,
weight on one foot, a lean, the head turned or tipped); the classic joints that moved from frame 1,
in everyday names, with "more" or "less"; whether the left and right arms and legs are symmetric;
and a one-line summary. It covers the classic joints only, not the Bento fingers, face, wings or
tail. The thresholds it uses to choose a word (for example "bent" or "lifted") each have a dead
zone around them, so a change of a degree or an inch right at the old boundary cannot flip the
wording back and forth.

### describe_motion

`describe_motion` turns the movement over a range of frames into text, because movement over time
is where a picture fails most. `from` and `to` are frame numbers; by default it covers frame 2 to the
last frame (Second Life does not play frame 1), or frames 1 to 2 in a two-frame animation. It never
changes the animation or the current frame. The answer holds:

- `joints`: every classic joint that moves, in the order they start moving (`order` lists the
  names, `still` the joints that do not move). For each: its main axis (the `set_pose` axis that
  changes most) with `start` and `end` angles; `reach`, the largest turn away from where it starts,
  measured as one rotation so that Euler angles swinging near gimbal lock do not count; `travel`,
  the total turn; the top speed in degrees per second with a speed class; the frame it first moves;
  and `segments`: the main axis `increases`, `decreases` or `holds`, from which frame to which, by
  how many degrees and how fast.
- `paths` of the hands and feet, measured relative to the hip so that walking forward does not hide
  a swing: each move from frame to frame with its shape (`straight`, `arc` or `out and back`), path
  length, distance between start and end, whether it rises or lowers, and its top speed; plus the
  lowest and highest world height. A new move starts where the hand or foot turns back.
- `contact`: the floor (the lowest ankle height in the range), for each foot the frames it is down,
  how often it lands and how fast it slides while down, the frames with both feet off the floor, and
  where the weight is (`both feet`, `left foot`, `right foot`, `in the air`).
- `hip`: start, end, lowest and highest height, the bob (highest minus lowest), and how far it ends
  forward, to the left and up, in the figure's own directions at the start of the range.
- `rhythm`: whether the motion repeats, with the period in frames and seconds, the beats per minute
  and the number of cycles; whether the legs and the arms move in opposition or together (the right
  side mirrored); whether the feet land in turn.
- `loop`: the loop points (`loop_in` at least frame 2) and the seam between them: `smooth`,
  `small jump` or `jump`, with the joints that jump most and how far the hip jumps.
- `keys`: what changes between consecutive keys, or only the count when there are more than 40 keys
  (motion capture).
- `summary`: one line.

Lengths are in the program's units (inches), angles in degrees, speeds per second. Lists are cut
after 8 entries with a `..._more` count. The words come from fixed thresholds: a joint moves when it
turns more than 5 degrees; it holds while slower than 10 degrees per second, and a turn under 2 degrees
counts as a hold; speed classes are slow (under 60 degrees per second), medium (from 60), fast
(from 180) and very fast (from 360); a hand or foot moves while faster than 2 inches per second, and
a move shorter than 1 inch is left out; a pause of fewer than 3 frames between two moves belongs to
the move before; a path is `out and back` when start and end are closer than 30% of its length,
otherwise `straight` when it never leaves the line between them by more than 10% of that line; a
foot is down at most 2 inches above the floor and up again above 3 inches (it can slide and still be
down); with both feet down the weight is on one foot when the hip is 3 inches towards it and back in
the middle within 1.5 inches; the motion repeats when the autocorrelation of all joint angles reaches
0.7 at some period of at least a quarter of a second (the first peak at least 90% as high as the best
one); legs or arms move in opposition below a correlation of -0.5 and together above 0.5; the loop
seam is smooth up to 2 degrees and half an inch, a small jump up to 10 degrees and 2 inches.

### check_animation

`check_animation` checks the whole animation against Second Life's rules and common mistakes, so an
AI assistant can fix a problem before the user uploads the file. It takes no parameters and never
changes the animation or the current frame. The answer has `errors`, `warnings` and `notes` (counts),
`findings` (errors first, then warnings, then notes; each with `check`, `severity`, `message` and the
frames, joints or numbers it is about), `passed` (the checks that found nothing) and a one-line
`summary`.

| Check | Severity | Finds |
|---|---|---|
| `length` | error | longer than 60 s, counted as the `.anim` export does: (frames - 2) times the frame time |
| `nothing_moves` | error | no joint and not the hip moves away from frame 1, so Second Life gets an empty animation |
| `hip_offset` | error | the hip more than 5 m (about 197 inches) from frame 1 on an axis; the `.anim` format cuts it |
| `frame1_posed` | warning | classic joints turned more than 5 degrees in frame 1 that keep that angle all the time: Second Life drops them, so their pose is lost |
| `joint_limits` | warning | a key more than 0.5 degrees past the joint's natural limit (`joints`), even with the limits turned off |
| `foot_under_floor` | warning | an ankle more than 2 inches below the floor Second Life stands the figure on (frame 1's hip at standing height, legs straight down) |
| `floating` | warning | both ankles more than 3 inches above that floor for 1 s or more (a jump is shorter; sitting on something is expected) |
| `foot_sliding` | warning | a foot that is down (within 2 inches of the lowest ankle, up again above 3, as `describe_motion`) moves along the floor faster than 20 inches per second for at least 3 frames |
| `jerk` | warning | one frame in which a joint turns at least 15 degrees, or the hip moves at least 6 inches, and that is at least 3 times both neighbouring frames |
| `dead_start` | warning | nothing moves (no joint faster than 10 degrees per second, the hip not faster than 2 inches per second) for at least 0.5 s from frame 2 |
| `loop_seam` | warning, or note | the loop seam as `describe_motion` judges it: a `jump` is a warning, a `small jump` a note |
| `dropped_joints` | note | classic joints that never move from frame 1 (within 0.001 radians, as the export compares): the export leaves them out |
| `identical_keys` | note | keys equal (within 0.01) to the key before and the key after them, or a last key equal to the one before: they change nothing |

Lists in a finding are cut after 8 entries; `count` gives the total where it matters.

### export_anim

`export_anim` writes the Second Life `.anim` file, the file the user uploads (Build > Upload >
Animation), with the same writer and the same defaults as File > Export for Second Life (.anim). It never
changes the animation. Parameters:

| Parameter | Default | Meaning |
|---|---|---|
| `path` | (needed) | file name ending in `.anim`, in the PinglyMotion folder; an existing file is never replaced |
| `priority` | 2 | 0 to 6, a whole number |
| `loop` | false | repeat between the loop points (as `.dingle` stores them; the whole animation when none are set) |
| `ease_in` | 0.3 | slow start in seconds, 0 to 60 |
| `ease_out` | 0.3 | slow finish in seconds, 0 to 60 |
| `hand_pose` | `Relaxed` | one of the 14 names of the export dialog (any case): `Fingers spread`, `Relaxed`, `Pointing`, `Fist`, `Relaxed, left hand only`, `Pointing, left hand only`, `Fist, left hand only`, `Relaxed, right hand only`, `Pointing, right hand only`, `Fist, right hand only`, `Salute, right hand`, `Typing`, `Peace sign, right hand`, `Flat hand, right hand` |

It runs `check_animation` first. On an error (longer than 60 s, nothing moves, the hip too far away)
nothing is written: the answer has `ok: false`, the reasons in `error` and the whole check in `check`.
Otherwise the answer has `file`, `bytes`, `seconds`, the options used (`priority`, `loop`, with
`loop_in` and `loop_out` as frame numbers when it loops, `ease_in`, `ease_out`, `hand_pose`),
`joints_written`, `joints_left_out` (joints that never move; the writer leaves them out, as Second
Life's own importer does) and `warnings` (the check's warnings, then the writer's, for example keys
thinned out to stay under Second Life's 250 000 bytes).

### screenshot with views, frames and labels

Without options `screenshot` gives one picture of the current frame as the program's camera sees it,
exactly as before. These options make one image an assistant can check its own work with (issue 129:
a model misreads angles and mixes up left and right in a single picture):

| Option | Meaning |
|---|---|
| `views` | a list of 1 to 6 of `front`, `back`, `left`, `right`, `three_quarter`: one picture per view, from that side at the camera's current height and distance, in a near-square grid (4 views: 2 by 2; 3: 2 by 2 with one empty place; 5 or 6: 3 by 2) |
| `frames` | a list of 1 to 12 frame numbers: one picture per frame, in rows of 4, with the program's camera; together with `views` one row per frame and one column per view |
| `labels` | `true`: a dot and the name of each classic joint; the figure's own left side in blue, its right side in orange, the middle in white; a legend in the first picture |
| `trail` | `true`: small dots where the hands and feet are at frames 2 to the last (at most 30 frames, evenly spread), blue for the left, orange for the right |
| `markers` | `false`: leaves out the small balls of the Bento joints (face, fingers) |
| `width` | width of one picture, 100 to 2000; default 800, or 400 with `views` or `frames`. The whole image may be at most 4000 pixels on each side |

Each picture is captioned (the view, and for frames the frame number and its time in seconds,
(frame - 1) / frames per second). The answer has `columns`, `rows`, `cell_width`, `cell_height`, `cells`
(in order, each with `cell`, `row`, `column` and its `view`, `frame` and `seconds`) and, with labels or a
trail, `legend`. The program window, its camera (also a panned camera) and the current frame stay as
they were: the 3D view turns its camera for each picture only and puts it back.

### Script Lab: lsl_check and lsl_run

Two commands for testing LSL scripts (Second Life's scripting language) in a sandbox inside PinglyMotion
(Script Lab, step 3, issue 289; plan in [SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md)). They do not need an
open animation and never change it. The code is in `src/scriptlab/lsl/`: an own recursive descent parser
and interpreter (QtCore only), not the Second Life compiler. The sandbox is never identical to Second
Life: test the final script there too.

- `lsl_check {source}` checks the script the way the Second Life compiler does, without running it:
  `valid`, `errors` and `warnings` (each `{line, column, message}`, both counted from 1) and `states`.
  Errors are syntax mistakes, undeclared names, wrong types or argument counts, unknown functions,
  events or states, a missing `return`, `state` in a function, and LSL's own rules (a declaration alone
  as the body of an `if`, a global that starts with a calculation, a list in a list). Warnings are valid
  LSL that the sandbox does not run yet: a function that is not supported yet, or an event it never raises.
- `lsl_run {source, seconds, inputs, keep_rlv}` runs the script with a simulated clock: `state_entry` of the default
  state first, then events until `seconds` (0 to 3600, default 10) have passed. Nothing waits for real
  time. `inputs` (at most 200) are chat or touches at given times:
  `[{"at":1,"channel":7,"text":"dance"},{"at":2,"type":"touch"}]`; `name` and `id` default to the owner.
  A script that does not compile is not run: the answer has `ok: false` and the `errors` of `lsl_check`.
  `llOwnerSay` text that starts with `@` is run as an RLV command (Script Lab step 4, issue 290): the sandbox's
  RLV state (the one `rlv_state` shows) gets it from the script's object (its key, as `llGetKey` returns), and
  the reply lines of `@version=<channel>`, `@getstatus=<channel>` and the like arrive in the script's `listen`
  event on that channel, as a viewer would answer (spoken by the owner). Other `llOwnerSay` text stays an `owner_say`
  line. The script runs in the mode the sandbox is in (plain RLV or RLVa, set with `rlv_reset`'s `mode`; a run never changes it). The RLV state is emptied before each run unless `keep_rlv` is `true`; what the script set stays in the
  state after the run, so `rlv_state` shows it. At most 100 `@` lines per run.

The answer of `lsl_run` has `log` (the event log, plain data), `log_truncated`, `stopped` and
`stop_reason`, `state` (the state at the end), `time`, `steps`, `open_listens`, `timer`, `warnings` and `rlv`
(`lines` run, `refused` over the cap, and `state`: the restrictions at the end in the shape of the `rlv_state` answer).
Every log entry has `t` (simulated seconds) and `type`:

| type | What happened | Other fields |
|---|---|---|
| `state` | the script entered a state | `name` |
| `event` | the script handled an event | `name`, `args` (as text) |
| `owner_say` | `llOwnerSay` | `text` |
| `say`, `whisper`, `shout`, `region_say` | chat | `channel`, `text` |
| `instant_message` | `llInstantMessage` | `to`, `text` |
| `rlv_command` | `llOwnerSay("@...")` went to the RLV core | `object`, `text`, `results` (one per command, as in `rlv_command`), `replies` (`channel`, `text`), `restrictions` (how many are held now), or `error` when the whole line was refused |
| `listen`, `listen_remove` | `llListen`, `llListenRemove` | `handle`, `channel`, `name`, `id`, `message` |
| `timer_set` | `llSetTimerEvent` | `seconds` |
| `sleep` | `llSleep` (moves the simulated clock) | `seconds` |
| `permissions` | `llRequestPermissions`; the owner's are granted at once, then `run_time_permissions` follows | `mask`, `granted`, `note` |
| `animation_start`, `animation_stop` | `llStartAnimation`, `llStopAnimation` (the 3D view plays them while the Script Lab's "Show effects in 3D view" box is ticked; [SCRIPT_LAB.md](SCRIPT_LAB.md)) | `name` |
| `input` | chat or a touch from `inputs` | `input`, `channel`, `name`, `text` |
| `debug` | what Second Life says on the debug channel without stopping the script (an animation without permission, `llRegionSay` on channel 0) | `channel`, `text` |
| `event_dropped` | more than 64 events were waiting, as in Second Life | `name`, `message` |
| `reset` | `llResetScript` | |
| `error` | a runtime error stopped the script (division by zero, too many listens) | `message`, `line`, `column` |
| `unsupported` | the script called a function the sandbox does not run yet, and was stopped there | `message`, `line`, `column` |
| `limit` | a limit stopped the script | `message`, `line`, `column` |
| `message_linked` | `llMessageLinked` (step 9a) | `link` (as given, for example -1 for `LINK_SET`), `num`, `text`, `id`, `scripts` (how many scripts are in the prims it reaches) |
| `linkset_data` | a Linkset Data write, delete or reset (step 9a; reads are not logged) | `op` (`write`, `delete`, `reset`), `name`, `value` (writes), `result` (`LINKSETDATA_OK`, `LINKSETDATA_NOUPDATE` and so on), `used` (bytes in use after it) |

#### Several objects and lsl_event (Script Lab step 5, issue 291)

`lsl_run` can run several scripts in one world: instead of `source` give `objects`, a list of 1 to 16 entries
`{name, source, key, owner, owner_name, position, attached}`. Only `name` (1 to 63 characters, each object its own) and
`source` are needed. `key` and `owner` are keys (1 to 36 letters, digits or `-`; default: the object keys
`c0ffee00-0000-4000-8000-000000000002`, `...03` and so on in list order, and the owner's key); `owner_name` needs
`owner`. `position` is `[x, y, z]` in region metres (x and y 0 to 256, z 0 to 4096; default `[128, 128, 20]`, where the
owner's avatar stands); `attached` is an attachment point from 1 to 55 (0, the default, means rezzed at `position`). A
worn object is where the avatar is, and gets `attach` with the owner's key after its `state_entry`. All scripts
together are at most 524 288 characters; nothing runs unless every script compiles (the answer then names the
`object` and lists its `errors`).

- One simulated clock for all objects. Things due at the same moment run in a fixed order, so the same request always
  gives the same log: every object's `state_entry` first, in list order; then timers before inputs, objects in list
  order, inputs in the order given; an object's events run before the next object's. A message said in `state_entry`
  is heard only by objects whose `state_entry` (and `llListen`) ran before, so put the listeners first.
- Objects hear each other's chat within the Second Life ranges: `llWhisper` 10 m, `llSay` 20 m, `llShout` 100 m,
  `llRegionSay` the whole region, `llRegionSayTo` only the object with that key, at any distance. An object never
  hears itself. The speaker's name and key are the object's, so `llListen` filters on name, key and message work.
- Each object's `llOwnerSay("@...")` goes to the shared RLV state under the object's own key: two objects can hold
  the same restriction (the count is 2), and `@clear` from one removes only its own. The cap of 100 `@` lines is per object (per script since step 9a).
- More inputs (in `inputs` and in `lsl_event`): `object` names the object (default the first); `type` is also
  `changed` (see below), `dialog` (`button`: the text of a button of that object's last `llDialog`; the avatar says it on the dialog's
  channel; in the sandbox the object that showed the dialog always hears it, other objects within 20 m of the avatar
  too), `changed` (`change`: what changed; step 9b-2, below), `attach` (`point`, default the object's point or 1) or `detach` (the object is dropped where the avatar stands
  and gets `attach` with `NULL_KEY`). Chat from an avatar is heard by the objects within `kind`'s range of the avatar
  (`say`, the default, 20 m; `whisper` 10 m; `shout` 100 m).
- New functions: `llDialog` (Second Life's checks: a message, at most 12 buttons of at most 24 bytes; an empty list
  gives `OK`; a refused dialog is a `debug` line), `llRegionSayTo`, `llGetPos`, `llGetAttached`, `llGetOwnerKey` (the
  key itself when the sandbox does not know it); `llKey2Name` knows every object. New event: `attach`.
- Log entries with several objects have `object` (the object's name). New entry types: `dialog` (`to`, `message`,
  `buttons`, `channel`) and `region_say_to` (`to`, `channel`, `text`); `input` entries can be `dialog`, `attach` and
  `detach` (with `error` when there is nothing to answer, detach or no such object).
- The answer with `objects` has `objects` (per object: `name`, `key`, `owner`, `attached`, `position`, `state`,
  `stopped`, `stop_reason`, `open_listens`, `timer`, `steps`, `restrictions` it holds, and its last `dialog`), `dialog`
  (the newest dialog of any object), `stopped` (true when every script stopped), `time`, `warnings` (each with
  `object`), `rlv` and `log`. A limit stops only the script that hit it; the others run on. The answer of a single
  script (`source`) is as before, plus `objects` and `dialog`.

`lsl_event {type, object, channel, text, kind, name, id, button, point, link, seconds}` sends one event to the world of the
last `lsl_run` (with `source` or `objects`; each `lsl_run` starts a new world) at the moment that run or the last event
ended, then runs the world on for `seconds` (default 1, at most 3600) of simulated time. The fields are those of an
input, without `at`. The answer has the `log` of this command only, `objects`, `dialog`, `rlv`, `stopped` and `time`.
The RLV state is not emptied. Each command gets a fresh real-time, step and event budget per script. It fails when no
`lsl_run` has run yet, when the object does not exist, or when every script has stopped. The Script Lab panel lists
the objects and the newest dialog after each `lsl_run` and `lsl_event` ([SCRIPT_LAB.md](SCRIPT_LAB.md)).

```
{"command": "lsl_run", "seconds": 2, "objects": [
   {"name": "hud", "source": "...", "attached": 35},
   {"name": "cuffs", "source": "...", "attached": 30}]}
{"command": "lsl_event", "type": "touch", "object": "hud"}
{"command": "lsl_event", "type": "dialog", "object": "hud", "button": "Lock"}
```

Examples: `examples/scriptlab/hud_cuffs_hud.lsl` and `hud_cuffs_cuffs.lsl`.

#### Several scripts in one object: link messages and Linkset Data (Script Lab step 9a, issue 305)

An entry of `objects` can give `scripts` instead of `source`: a list of 1 to 32 entries `{name, source, link}`
(`name` 1 to 63 characters, each script of the object its own, as `llGetScriptName` gives it; `link` the prim the
script is in, 1 the root, default 1), and `prims`, how many prims the object has (1 to 256; default the highest
`link`). Each script has its own interpreter: its own variables and memory, its own queue of 64 waiting events, its
own listens and timer. At most 64 scripts in all objects together. An entry with `source` is one script named
`Script` in a single prim, as before.

- `llMessageLinked(link, num, str, id)` queues `link_message(sender, num, str, id)` in every script of the prims the
  link reaches, the sending script too when its prim is reached; `sender` is the sender's link number. In an object
  of several prims the root is link 1 and the children 2 and up; `LINK_SET` reaches every prim, `LINK_ROOT` the root,
  `LINK_ALL_OTHERS` every prim but the sender's, `LINK_ALL_CHILDREN` every prim but the root, `LINK_THIS` the
  sender's prim, a number that prim. A single prim is link 0: there `0`, `LINK_THIS` and `LINK_SET` reach it and
  `LINK_ROOT`, `LINK_ALL_OTHERS` and `LINK_ALL_CHILDREN` reach nothing (Second Life wiki). A link number the object
  does not have reaches nothing, without an error. `llGetLinkNumber` answers 0 in a single prim.
- Linkset Data belongs to the object, not to a script: every script of the object reads and writes the same pairs,
  and they survive `llResetScript` and stay for `lsl_event`. `llLinksetDataWrite` answers `LINKSETDATA_OK`,
  `LINKSETDATA_EMEMORY` (more than 131 072 bytes, 128 KiB, in all; a pair costs the UTF-8 bytes of its name plus its
  value), `LINKSETDATA_ENOKEY` (empty name), `LINKSETDATA_NOTFOUND` (empty value for a name that is not there) or
  `LINKSETDATA_NOUPDATE` (the same value again); an empty value deletes the pair. `llLinksetDataRead` answers `""`
  for a missing name, `llLinksetDataDelete` `LINKSETDATA_OK` or `LINKSETDATA_NOTFOUND`; `llLinksetDataListKeys(start,
  count)` lists the names in alphabetical order (`count` below 1: all from `start`; a `start` below 0 counts as 0);
  `llLinksetDataAvailable`, `llLinksetDataCountKeys` and `llLinksetDataReset` as in Second Life. Every change sends
  `linkset_data(action, name, value)` to every script of the object, the writer too: `LINKSETDATA_UPDATE` (1) with
  the new value, `LINKSETDATA_DELETE` (2) with an empty value, `LINKSETDATA_RESET` (0) with an empty name and value.
  Not supported yet (known to `lsl_check`, which warns; the script stops there with `unsupported`): the protected
  functions (`llLinksetDataWriteProtected`, `llLinksetDataReadProtected`, `llLinksetDataDeleteProtected`) and
  `llLinksetDataFindKeys`, `llLinksetDataCountFound`, `llLinksetDataDeleteFound`.
- A `link_message` or `linkset_data` event whose arguments do not fit in the receiving script's free memory crashes
  that script with a Stack-Heap Collision (an `error` entry), as in Second Life; the other scripts run on.
- Chat: a prim never hears its own chat (no script in the same prim hears it); the other prims of the same object
  do, at distance 0 (Second Life wiki, `llListen`). Every prim has the object's key and name in the sandbox (in
  Second Life each prim has its own key), so a script's RLV lines are held under the object's key, whichever prim
  it is in. The cap of 100 `@` lines is per script.
- A touch (an input or `lsl_event` with `type` touch) can name `link`, the prim touched (default the root): the
  scripts of that prim that handle `touch_start` get it; when none does, the root's scripts get it, as Second Life
  passes touches on.
- Order: Second Life runs scripts at the same time, in no set order. The sandbox runs them one after the other in a
  fixed order (objects in list order, each object's scripts in list order; a script runs every event waiting for it
  before the next script runs), so the same request always gives the same log. A set of scripts that only works in
  one order may still fail in Second Life.
- When more than 64 events wait for a script, Second Life drops the rest without a word; the sandbox drops them too
  and writes an `event_dropped` entry for each. An endless exchange of link messages at one moment ends when a script
  reaches its limit of events or steps (the other scripts run on), never in a hang.
- Log entries of an object given with `scripts` have `script` (the script's name). The answer's `objects` entry of
  such an object has `prims` and `scripts` (per script: `name`, `link`, `state`, `stopped`, `stop_reason`,
  `open_listens`, `timer`, `steps` and `memory`, the rough byte count below); its `stopped` is true only when all its
  scripts stopped (then `stop_reason` names each script), and `steps` and `open_listens` add up its scripts.
  `linkset_data` (`count`, `used`, `available` and `pairs`, at most 200 pairs, each name and value cut at 1 024
  characters) is in every entry given with `scripts` and in any entry whose store is not empty.

```
{"command": "lsl_run", "seconds": 2, "objects": [{"name": "box", "prims": 2, "scripts": [
   {"name": "main", "source": "...", "link": 1},
   {"name": "door", "source": "...", "link": 2}]}]}
{"command": "lsl_event", "type": "touch", "object": "box", "link": 2}
```

#### The events on_rez and changed (Script Lab step 9b-2, issue 314)

- `on_rez(integer start_param)` is raised once when an object starts, after its `state_entry` and before `attach` (for a worn object).
  The parameter is the object's `start_parameter` (default 0; per object in `objects`, or at the top of the request with `source`).
  The log has an `event` entry named `on_rez`. Not as in Second Life: there, a script saved or reset in an object gets `state_entry` only,
  and a rezzed object gets `on_rez`; the sandbox always starts a fresh script, so it gives both, in that order. `llResetScript` runs
  `state_entry` again and does **not** raise `on_rez` again (as in Second Life). A `state` change in `state_entry` empties the event queue,
  so an `on_rez` still waiting is dropped (as with any waiting event).
- `changed(integer change)` comes from an input of type `changed` (in `inputs` and in `lsl_event`): `{"at": 1, "type": "changed", "object": "box", "change": "inventory"}`.
  `change` is a name (`inventory`, `allowed_drop`, `color`, `shape`, `scale`, `texture`, `link`, `owner`, `region`, `teleport`,
  `region_start`, `media`, `render_material`; also written `CHANGED_OWNER`, in any case), a list of names (the flags are added: `["link", "region_start"]` gives 1056), or
  the flags as a whole number (1 to 8191, only known flags). Every script of the object gets the event, in any prim; other objects do not.
  The log has an `input` entry (`input`: `changed`, `change`: the flags) and then the scripts' `event` entries. The event is only the event:
  `llGetOwner`, the prim count and the inventory do not change with it (`owner` does not give the object a new owner). This is a new input type
  for the existing `inputs`; no new control command.

#### Pure functions and object state (Script Lab step 9b-1, issue 310)

These run without any world: rotation maths (`llEuler2Rot`, `llRot2Euler`, `llRot2Fwd`, `llRot2Left`, `llRot2Up`,
`llAxisAngle2Rot`, `llRotBetween`), lists (`llListReplaceList`, `llList2ListStrided`, `llListSortStrided`), strings
(`llReplaceSubString`, `llStringToBase64`, `llBase64ToString`, `llIntegerToBase64`, `llBase64ToInteger`,
`llMD5String`, `llSHA1String`, `llEscapeURL`, `llUnescapeURL`) and a little object state (`llGetStartParameter`,
`llGetObjectDesc`, `llSetObjectDesc`, `llSetObjectName`, `llGetScale`, `llGetNumberOfPrims`). Where the sandbox has
to choose:

- Rotations are computed in 32-bit floats, as the Second Life servers do; `llRot2Fwd` and its two siblings turn the unit vector by the
  normalised rotation. `llRotBetween` of two opposite vectors turns half a turn about an axis perpendicular to the first one (Second Life's
  axis for that case is not documented).
- `llListSortStrided` sorts by the element at `stride_index` (negative counts from the end; out of range gives an empty list; a list whose
  length is not a multiple of the stride comes back unchanged; only `TRUE` (1) sorts ascending). The sort is stable here; Second Life's is
  not. Lists that mix types keep their order across types (as `llListSort` does here).
- `llList2ListStrided` counts the stride from the start of the list, not from `start` (`[0,1,2,3,4,5,6]` with start 1, end -1, stride 2 gives
  `[2,4,6]`, as on the Second Life wiki). A **start after the end** (a range that wraps) in `llList2ListStrided` and `llListReplaceList` stops the script with
  "not supported yet": the Second Life wiki contradicts itself about that case.
- `llReplaceSubString` replaces plain text (no pattern language), left to right without overlaps; a negative count looks from the end; an empty
  pattern changes nothing (sandbox choice).
- `llEscapeURL` leaves only ASCII letters and digits and writes the others as upper-case `%XX` of their UTF-8 bytes; `llUnescapeURL` keeps a
  `%` that is not followed by two hex digits. `llBase64ToString` gives `?` for bytes that are no valid text and for control characters
  other than tab and line breaks (an approximation of Second Life's "unprintable characters"); text that is not Base64 gives an empty string.
  `llMD5String` hashes `text + ":" + nonce` as UTF-8, and both hashes are lower-case hex.
- Object state: `llSetObjectName` (at most 63 characters) changes the name that `llGetObjectName`, `llKey2Name` and chat from the object use;
  the object stays addressable by the name it was given in `lsl_run` (`inputs`, log entries). `llSetObjectDesc` keeps at most 127 characters.
  The values at the start come from the object's entry in `lsl_run`: `description` (text, at most 127 characters), `scale` (`[x, y, z]` metres,
  each 0.001 to 64; default `[0.5, 0.5, 0.5]`; one size for the whole object), `start_parameter` (a whole number, default 0).
  For a single script (`source`) the same three options go at the top of the request; with `objects`, inside each object (at the top they are refused).
  `llGetNumberOfPrims` answers the object's `prims`.

What runs: the types `integer`, `float`, `string`, `key`, `vector`, `rotation` and `list`; global variables
and functions; states with `state_entry` and `state_exit`; `if`, `while`, `do`, `for`, `jump`, `return`;
the events `state_entry`, `state_exit`, `timer`, `listen`, `touch_start`, `run_time_permissions`, `attach`,
`link_message`, `linkset_data`, `on_rez` and `changed`; 111 built-in functions (chat, listen, timer, time, owner and object keys,
permissions, animations, dialogs, position, link messages, Linkset Data, list, string and maths functions, rotation
maths, Base64, MD5 and SHA-1, URL escaping, and a few object-state getters and setters). All 526
function names and signatures, 43 events and 1 082 constants are known: those of the viewer's LSL keyword list
(`src/scriptlab/lslbuiltins_data.inc`, made by `tools/gen_lsl_builtins.py`) plus what that list lacks (Linkset Data,
`llReplaceSubString`, `llListSortStrided`, `llName2Key`, `llStartObjectAnimation` and others; the events
`linkset_data`, `on_damage`, `on_death`, `final_damage`, `game_control`), taken from Second Life's own
lsl-definitions (`src/scriptlab/lslbuiltins_extra.inc`, made by `tools/gen_lsl_extra.py`; MIT licence, see
[CREDITS.md](../CREDITS.md)), so the checker knows every LSL function. LSL quirks are kept: the right side of an operator is evaluated
first, `&&` and `||` do not short-circuit and have the same precedence, integers wrap around, `(string)`
of a float has 6 decimals and of a vector 5, and `NULL_KEY` (a string) is true in an `if`.

Limits (the input is untrusted, an AI tool writes it): the script at most 262 144 characters and 100 000
tokens, nesting at most 100 levels; while running at most 500 000 steps, 64 nested function calls, 10 000
events, 65 open listens, 64 waiting events, strings of 65 536 characters, lists of 4 096 entries, about
256 KB for all variables together per script (Second Life gives a script 64 KB, code included; the sandbox's
count is rough, 16 bytes per value plus 2 per character, so it keeps the larger limit, and `llGetFreeMemory` answers
against 64 KB) and 2 seconds of real time; the log
keeps 2 000 entries, each text at most 1 024 characters. A limit stops the script; the last log entry and
`stop_reason` say which.

How an AI assistant uses it: write the script, `lsl_check` it, fix the errors at their line and column,
`lsl_run` it with inputs, read the log (what it said, which listens and animations), fix and run again.
Examples: `examples/scriptlab/` (also in the Windows zip).

### Script Lab Live: lab_watch, lab_unwatch and lab_result

Three commands for the write, run, fix loop with scripts on disk (Script Lab step 6, issue 292): the
assistant edits a `.lsl` file in the user's folder (a cloned repository, for example) with its own file
tools, PinglyMotion notices the save, checks and runs the script, and the assistant reads the outcome.
The Script Lab panel's Live page shows the same watch and the same results, so the user sees what the
assistant sees. The code is `src/scriptlab/labwatcher.*`.

- `lab_watch {path, seconds, inputs, keep_rlv}` watches the folder `path` (the whole path; a relative path,
  a file, a missing folder or a network folder is refused). Watching another folder stops the one before.
  `seconds`, `inputs` and `keep_rlv` are given to every run, as in `lsl_run`, and are checked the same way:
  `llOwnerSay("@...")` goes to the sandbox's RLV state (the one `rlv_state` and the panel show), which is
  emptied before each run unless `keep_rlv` is `true`. The
  newest script in the folder is run once at once. The answer has `folder` (the path without links),
  `scripts` (the `.lsl` files found, relative to the folder, sorted), `truncated` (true when the scan
  stopped at a limit), `runs` and `result` (as in `lab_result`; `null` when the folder has no script).
- `lab_unwatch` stops watching; `was_watching` says whether a folder was watched.
- `lab_result {file}` answers `watching`, `folder`, `scripts`, `runs` and `result`, the newest result
  (with `file`, a script path relative to the folder: that script's latest result). `runs` counts up by
  one with every run, so after a save the assistant calls `lab_result` until `runs` has gone up.

A result has `file`, `run` (the value of `runs` after it), `valid`, `errors` and `warnings` (each
`{line, column, message}`, as in `lsl_check`), `read_error` when the file could not be read (too large,
no permission), and `result`, the whole `lsl_run` answer (log, final state, `stopped`, `stop_reason`,
the RLV state at the end and the rest), only when the script compiled.

When a run happens: about 0.3 seconds after the last write to a `.lsl` file in the folder or its
subfolders (several writes in a row count as one change), also when an editor saves by writing a new
file and renaming it over the old one.

Limits and safety (the folder may hold anything):

- The folder is only read, never written. Only `.lsl` files are read (any upper or lower case).
- At most 200 scripts of at most 262 144 bytes each; the scan stops after 5 000 entries and 4 folder
  levels below the watched folder. Names that start with a dot (`.git` and the like, folders and files) are
  skipped on every system.
- Links are never followed out of the folder: a linked folder is not entered at all, and a linked file
  counts only when its target is inside the watched folder.
- Each run has every limit of `lsl_run`, including its 2 seconds of real time, so a script that loops
  forever is stopped and the program keeps answering. When several scripts change at once, at most 4
  are run in a row before the program draws and answers again; the rest follow.

## Set up an AI assistant

The Claude Desktop extension `PinglyMotion.mcpb` is the one way to connect an AI assistant. It holds an
MCP server (`tools/mcp/pinglymotion_mcp.js` in the source tree, Node, built-in modules only) that lets
the Claude app use remote control. The Windows zip and each release carry the extension. There is no
Python server any more, and no manual setup of a server by path.

### Quick way: the Claude Desktop extension

The Windows zip (and each release) has `PinglyMotion.mcpb`, an extension for the Claude app, and a
short `START HERE - Claude.txt` with the same steps. No file editing and no typed paths:

1. Unzip the download first (do not open the `.mcpb` from inside the zip). Nothing else has to be
   installed: the extension runs on the Node that the Claude app brings.
   If it does not start, say so in issue #7.
2. In the Claude app open **Settings > Extensions** (in newer versions: **Settings > Desktop app**, then
   **Manage** next to Extensions). Click **Advanced settings**, find the section **Extension Developer**,
   click **Install Extension...** and pick `PinglyMotion.mcpb`. The app shows a window about the
   extension; follow its prompts to finish. Alternative: drag the file into the Claude window.
   Double-clicking the file may also work, but on some Windows PCs it does nothing or Windows asks which
   app to use (the Claude app is not offered); then ignore that and use the Settings route. Companies
   and teams can block own extensions in the Claude app; then an administrator has to allow it.
3. Start PinglyMotion and turn on **Options > Allow remote control (AI and other programs)** (it is off
   at every start).
4. In a new chat in the Claude app, ask: "Raise the avatar's right arm and show me a picture." If
   Claude asks for permission to use PinglyMotion, allow it; if it does not ask, that is fine.

Update: install the `PinglyMotion.mcpb` from the new download the same way (step 2). The extension holds no
settings and no key; it talks only to `127.0.0.1` and checks that it really is PinglyMotion
before it sends the key. The extension is
built from this repository by `tools/mcp/build_mcpb.py` and is not signed. Claude Desktop ships its own
Node, so nothing else has to be installed.

Other assistants, and anything that does not speak MCP, can use the plain HTTP commands described above.

### Use it

Start PinglyMotion and turn on **Options > Allow remote control** (or start it with `--control`).
Then ask the assistant to pose the figure. The tools are listed even when PinglyMotion is not running,
and say what to do if one is called then. Restart the assistant after updating PinglyMotion: a new MCP
server refuses to talk to an old PinglyMotion.

## Good to know when posing

- Read `docs/ASSISTANT_GUIDE.md` first: the MCP tool `read_guide`, or the resource
  `pinglymotion://guide`, gives an AI assistant the same text (Second Life's rules, the skeleton,
  typical poses, the work rhythm and known mistakes) before it starts posing.
- Frame numbers start at 1, like the counter in the program. Frame 1 holds the reference pose
  that Second Life compares the others with, so it is protected: pose from frame 2.
- Angles are degrees, as in the rotation fields. Each joint bends around its own axes, for
  example the elbow (`lForeArm`) around Y and the knee (`lShin`) around X. `joints` shows the
  allowed range per axis; angles outside it are reduced and the answer says so.
- The classic joints keep their old names (`hip`, `abdomen`, `chest`, `neck`, `head`, `lShldr`,
  `lForeArm`, `lHand`, `lThigh`, `lShin`, `lFoot` and the `r` ones). Bento joints start with `m`.
- `screenshot` draws the current frame, 4:3, whatever the size of the window. With `views`, `frames`,
  `labels`, `trail` and `markers` it draws several pictures in one image (see above).
- `set_camera` turns the camera around the figure (`view`: front, back, left, right, three_quarter, or an
  `angle`; also `height` and `distance`; `reset: true` for the start view). Left and right are the figure's
  own sides. It never changes the animation, so use it instead of turning the hip to see the side.
- `set_figure` shows the female or male body. It only changes the display; the saved BVH is the same.
- `play` and `stop` do the same as the Start and Stop buttons under the timeline: `play` plays from the current frame (from the loop-in frame when it is on the last frame), repeating between the loop points while Loop is on (Options > Loop playback, or the Loop switch next to the buttons). To replay from the start, send `set_frame` with frame 1, then `play`.
- Mirroring (Tools > Mirror) applies to commands too: with it on, posing a left joint also poses the right one.
