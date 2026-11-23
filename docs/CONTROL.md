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
After an answer (any status) the server ends its side of the connection, reads and drops whatever the client still
sends, and closes when the client closes, at most 2 seconds later. So an answer is never lost: closing at once with
unread request bytes would send a TCP reset, and on Windows a reset throws away an answer the client has already
received (a refused client then saw no `503` at all).

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
`export_anim`, `find_reference`, `load_reference`, `rlv_command`, `rlv_state`, `rlv_reset`, `lsl_check`, `lsl_lint`, `lsl_run`,
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
- After File > Close animation no animation is open. Then every command that works on an animation answers "No animation is open."; `new_animation`, `open_file`, `find_reference` and `load_reference` still work and start one.
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
  `rlva`, any case) sets the mode: RLVa (as in Firestorm) is the default at start; in RLVa mode the behaviours that
  only plain RLV has (such as `camdrawmax`) fail as unknown, and in plain RLV mode (the RLV specification, as in Kokua)
  the behaviours and parameter types that only RLVa has fail (step 9, issue 297; the list of RLVa-only names is
  `rlva_only` in `rlv_state`). Switching the mode always
  starts from a clean state; the answer has `mode`. Any other `mode` value fails and changes nothing. Camera, sky and
  debug behaviours are only recorded and shown in the `reason` of the result ("recorded only"); the real program
  never changes.
- Known behaviours of the RLV core: the table in `src/scriptlab/rlvbehaviours.cpp` holds 219 behaviours (restrictions,
  questions answered on a channel such as `version` and `getstatus`, and force commands where they exist; each is marked
  `RLV`, `RLVa only` or `RLV only`, and the test `rlvsources` checks names, parameter types and marks against RLVa's
  source and the RLV wiki, see [SCRIPT_LAB.md](SCRIPT_LAB.md), "RLV and RLVa mode"). The names known in the current mode are in `behaviours` of `rlv_state`; the
  groups and the camera, sky and debug behaviours that are only recorded are described in
  [SCRIPT_LAB.md](SCRIPT_LAB.md). Other names parse but fail as unknown.
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
- `lsl_lint {source}` gives static warnings without running the script: valid LSL that is most likely a
  mistake (`src/scriptlab/lsl/lint.*`). The answer has `valid`, `errors` (a script that does not compile gets the
  `errors` of `lsl_check` and no warnings) and `warnings`, each `{line, column, id, message}`, sorted by place, at
  most 200. The ids stay the same between versions: `unused-local`, `unused-param`, `unused-global` (never read;
  "set but never read" when it is only assigned), `unused-function` (never called, or only by itself; unused event
  parameters are fine), `shadowed` (a local or parameter hides a variable of the same name), `unreachable` (code
  after `return`, `jump` or `state`, up to the next label), `assign-in-condition` (`if (a = b)`, also in `while`,
  `do` and `for`), `empty-body` (`if (a);`, also `while` and `for`), `self-assign` (`a = a;`), `list-compare`
  (`==` and `!=` between lists only compare their lengths; comparing with `[]` is fine), `divide-by-zero` (`/` or
  `%` by a literal 0), `deprecated` (once per function), and for `llOwnerSay` text that starts with `@`, checked
  with the sandbox's RLV parser and behaviour table (as `rlv_command` does): `rlv-syntax` (bad syntax),
  `rlv-unknown` (a behaviour unknown in both RLV and RLVa) and `rlv-param` (a parameter or option the behaviour
  does not take; RLVa-only behaviours are judged as RLVa). Text built with `+` is checked as far as its constant
  start goes: the complete commands, and the behaviour name of the last one when it ends before the changing part
  (`"@sit:" + (string)id + "=force"` checks `sit`).
- `lsl_run {source, seconds, inputs, keep_rlv}` runs the script with a simulated clock: `state_entry` of the default
  state first, then events until `seconds` (0 to 3600, default 10) have passed. Nothing waits for real
  time. `inputs` (at most 200) are chat or touches at given times:
  `[{"at":1,"channel":7,"text":"dance"},{"at":2,"type":"touch"}]`; `name` and `id` default to the owner. A touch
  raises `touch_start`, `touch` and `touch_end`; `hold` is how long it is held ("Touches" below).
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
| `input` | chat or a touch from `inputs` | `input`, `channel`, `name`, `text`; a held touch `hold`; `money`, `http_request` and `email`: "Money, web server and mail" |
| `debug` | what Second Life says on the debug channel without stopping the script (an animation without permission, `llRegionSay` on channel 0) | `channel`, `text` |
| `event_dropped` | more than 64 events were waiting, as in Second Life | `name`, `message` |
| `reset` | `llResetScript`, or `llResetOtherScript` from another script (then the reset script's entry) | |
| `reset_other` | `llResetOtherScript` reset another script of the prim (its `reset` entry follows) | `target` (the script's name) |
| `script_state` | `llSetScriptState` changed whether a script runs | `target` (the script's name), `running` |
| `die` | `llDie` deleted the object: the calling script's entry (with `line` and `column`), then one for each other script of the object | `message`, `line`, `column` |
| `error` | a runtime error stopped the script (division by zero, too many listens) | `message`, `line`, `column` |
| `unsupported` | the script called a function the sandbox does not run yet, and was stopped there | `message`, `line`, `column` |
| `limit` | a limit stopped the script | `message`, `line`, `column` |
| `message_linked` | `llMessageLinked` (step 9a) | `link` (as given, for example -1 for `LINK_SET`), `num`, `text`, `id`, `scripts` (how many scripts are in the prims it reaches) |
| `linkset_data` | a Linkset Data write, delete or reset (step 9a; reads are not logged) | `op` (`write`, `delete`, `reset`), `name`, `value` (writes), `result` (`LINKSETDATA_OK`, `LINKSETDATA_NOUPDATE` and so on), `used` (bytes in use after it) |
| `prim` | a change of the simulated prim state (step 9b-3): nothing is drawn | `op` (`sit_target` (step 9b-5: `set`, `offset`, `rotation`), `text`, `color`, `alpha`, `texture`, `position`, `rotation`, `size`, `name`, `description`, `fullbright`, `glow`, `physics`, `phantom`, `temp_on_rez`, `status`, `sit_text`), `link` (0 in a single-prim object), the values (`face`, `text`, `color`, `alpha`, `texture`, `position`, `rotation`, `size`, `value`, `status`) and sometimes `note` |
| `linden_dollars` | `llTransferLindenDollars` (step 9b-3; simulated, no L$ ever moves) | `to`, `amount`, `transaction` (the key the call returned), `result` (`ok` or the failure tag), `note` |
| `give_inventory` | `llGiveInventory` (step 9b-4) | `to`, `item`, `result` (`given`, `not_found`, `unknown_target` or `name_exists`) |
| `give_inventory_list` | `llGiveInventoryList` (several-objects job) | `to`, `folder`, `items` (the names found and given), `result` (`given`, `not_found` when no item was found, `unknown_target`), `missing` and `name_exists` (names, only when there are any) |
| `remote_pin` | `llSetRemoteScriptAccessPin` (several-objects job; the pin itself is not logged) | `enabled` |
| `remote_load` | `llRemoteLoadScriptPin` (several-objects job) | `target`, `item` (the script's name), `running`, `param`, `result` (`loaded`, `replaced`, `wrong_pin`, `unknown_target`, `no_source`, `too_many_scripts` or `other_owner_attachment`) |
| `allow_drop` | `llAllowInventoryDrop` (several-objects job) | `allow`, and `note` when a child prim's call was ignored |
| `inventory` | a script removed an item with `llRemoveInventory` (step 9b-4) | `op` (`remove`), `item` |
| `dataserver_request` | `llGetNotecardLine`, `llGetNumberOfNotecardLines` or `llRequestInventoryData` (item, no notecard) found the item; the `dataserver` event follows (step 9b-4) | `function`, `notecard`, `line` (not for the line count), `query` (the key the call returned) |
| `land_edit` | `llModifyLand` (steps 13 to 15) | `action`, `brush`, `name`, `size_m`, `position`, `result` (`ok`, `bad_value` or `not_owner`) |
| `object_animation` | `llStartObjectAnimation` or `llStopObjectAnimation` changed the list (steps 13 to 15) | `op` (`start`, `stop`), `animation` |
| `rez` | `llRezObject` or `llRezAtRoot` (step 9b-4) | `item`, `position`, `velocity`, `rotation`, `param`, `at_root`, `result` (`ok` or `too_far`), `object` (the new object's key, with `ok`) |
| `sensor_set` | `llSensorRepeat` started a repeating scan (step 9b-7) | `name`, `id`, `sense` (the type mask), `range`, `arc`, `rate` |
| `sensor_remove` | `llSensorRemove`, or a rate of 0 or less, ended the repeat (only when one was running) | |
| `sensor` | a scan ran (`llSensor` or a repeat), and the `sensor` or `no_sensor` event follows | `found` (how many), `names` |
| `controls` | `llTakeControls` took controls or `llReleaseControls` gave them back (step 9b-6) | `op` (`take`, `release`), `avatar`, and for `take` `controls`, `accept`, `pass_on` |
| `unsit` | `llUnSit` took an avatar off the object (step 9b-5; a `stand` input is an `input` entry instead) | `avatar`, `by` (`llUnSit`) |
| `animation_override` | `llSetAnimationOverride` or `llResetAnimationOverride` succeeded (step 9b-8) | `op` (`set` or `reset`), `state`, `animation` |
| `http_request` | `llHTTPRequest` passed its checks (step 9b-9; nothing goes on the network) | `method`, `url`, `mimetype`, `body` (when not empty), `headers` (the custom headers as `[name, value]` pairs, when given), `result` (`canned`, `no_reply` or `throttled`), and except for `throttled` `request` (the key the call returned), `status` and `truncated_at` (when the reply body was cut) |
| `give_money` | `llGiveMoney` (second gap list, step 4; simulated, no L$ ever moves) | `to`, `amount`, `result` (`ok` or the failure tag), `note` |
| `prim_media` | `llSetPrimMediaParams`, `llGetPrimMediaParams`, `llClearPrimMedia` (second gap list, step 5; record only) | `link`, `face`, `op` (`set`, `get` or `clear`), `params` (the rule list as its elements; not for `clear`), `result` (a `STATUS_*` name for `set` and `clear`; `ok`, `no_media` or a `STATUS_*` name for `get`), `note` (`set` and `clear`) |
| `email_sent` | `llEmail` (second gap list, step 4; no mail is sent) | `address`, `subject`, `message`, `bytes` (with the header), `result` (`recorded` or `too_long`), `note` |
| `email_check` | `llGetNextEmail` (second gap list, step 4) | `address`, `subject` (the filters), `found`, `left` (when found) |
| `url_request` | `llRequestURL` or `llRequestSecureURL` (second gap list, step 4) | `secure`, `request` (the key the call returned), `result` (`granted` or `denied`), `url` (when granted) |
| `url_release` | `llReleaseURL` (second gap list, step 4) | `url`, `result` (`released` or `not_found`) |
| `http_server_response` | `llHTTPResponse` (second gap list, step 4; nothing is sent) | `request`, `status`, `body`, `result` (`sent`, `timed_out` or `unknown_request`) |
| `damage` | `llSetDamage` (third gap-list run, step 7) | `damage` (the stored value, -100 to 100), `note` (when the value was cut to that range) |
| `keyframed` | `llSetKeyframedMotion` (step 8) | `op` (`set`, `command`, `clear` or `refused`), `applied`; `set`: `frames`, `mode` (`forward`, `loop`, `ping_pong`, `reverse`), `data` (1 rotation, 2 translation, 3 both), `seconds`; `command`: `command` (`play`, `stop`, `pause`); `note` (why not applied) |
| `link` | `llCreateLink` and `llBreakAllLinks` (step 9) | `op` (`create` or `break_all`); `create`: `target`, `parent`, `linked`, with `root` and `prims` when it linked, else `note`; `break_all`: `released` (objects separated), `root`, `objects`, or `note` when nothing was linked |
| `recorded_only` | a function whose effect the sandbox does not play: `llParticleSystem`, `llLinkParticleSystem`, `llTriggerSound`, `llPlaySound`, `llLoopSound`, `llStopSound`, `llPreloadSound`, `llTargetOmega`, `llLoadURL`, the 37 stubs of step 9b-8, the five movement stubs of the second gap list, step 6, part 1, and the six vehicle stubs of step 7 (see "Recorded only"; `llApplyImpulse` and `llSetBuoyancy` moved out of this list in part 3) | `function`, `args`, `note` |

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
  (`say`, the default, 20 m; `whisper` 10 m; `shout` 100 m). `money`, `http_request` and `email` (second gap list,
  step 4): "Money, web server and mail" below.
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

`lsl_event {type, object, channel, text, kind, name, id, button, point, link, amount, url, method, body, headers, address, subject, phase, pos, tnum, targetpos, ourpos, seconds}` sends one event to the world of the
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
  scripts of that prim that handle `touch_start`, `touch` or `touch_end` get it; when none does, the root's scripts
  get it, as Second Life passes touches on. Each script gets only the touch events it has a handler for.
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

#### Prim state, L$ and recorded-only functions (Script Lab step 9b-3, issue 317)

The sandbox keeps a simulated state for every prim of an object (shared by all its scripts) and records each change as a `prim` log
entry. Nothing is drawn, played or sent; the 3D view is not touched. A prim is a box with 6 faces; `ALL_SIDES` is every face, a face number
outside 0 to 5 changes nothing (as in Second Life, silently).

- **Set:** `llSetText` (at most 254 characters, colour and alpha clamped to 0..1), `llSetAlpha`, `llSetColor`, `llSetTexture`, the link forms
  `llSetLinkAlpha`, `llSetLinkColor`, `llSetLinkTexture`, `llSetPos`, `llSetRot`, `llSetStatus`, `llSetSitText`, and
  `llSetPrimitiveParams`, `llSetLinkPrimitiveParams`, `llSetLinkPrimitiveParamsFast` with the rules `PRIM_LINK_TARGET`, `PRIM_POSITION`, `PRIM_POS_LOCAL`,
  `PRIM_SIZE`, `PRIM_ROTATION`, `PRIM_ROT_LOCAL`, `PRIM_COLOR`, `PRIM_TEXTURE`, `PRIM_TEXT`, `PRIM_NAME`, `PRIM_DESC`, `PRIM_FULLBRIGHT`, `PRIM_GLOW`,
  `PRIM_PHYSICS`, `PRIM_PHANTOM` and `PRIM_TEMP_ON_REZ`. Any other rule (for example `PRIM_TYPE`, `PRIM_FLEXIBLE`, `PRIM_POINT_LIGHT`) stops the script with
  "... is not supported yet". A malformed rule (a wrong type or a missing value) is a `debug` line on the debug channel; the rules before it are kept and the rest of the list is skipped.
- **Read:** `llGetColor`, `llGetAlpha` (for `ALL_SIDES` the average of the faces), `llGetTexture`, `llGetPos`, `llGetLocalPos`, `llGetRootPosition`, `llGetRot`,
  `llGetLocalRot`, `llGetRootRotation`, `llGetStatus`, and `llGetPrimitiveParams` / `llGetLinkPrimitiveParams` with `PRIM_NAME`, `PRIM_DESC`, `PRIM_POSITION`,
  `PRIM_POS_LOCAL`, `PRIM_SIZE`, `PRIM_ROTATION`, `PRIM_ROT_LOCAL`, `PRIM_TEXT`, `PRIM_COLOR`, `PRIM_TEXTURE`, `PRIM_FULLBRIGHT`, `PRIM_GLOW`, `PRIM_PHYSICS`, `PRIM_PHANTOM`,
  `PRIM_TEMP_ON_REZ`. A set of links (`LINK_SET` and so on) or a link the object does not have gives an empty list.
- **Where the sandbox simplifies:** a new prim starts white, opaque, with Second Life's default plywood texture, size 0.5 m, and `STATUS_ROTATE_X`, `_Y` and `_Z` set. `llSetPos`
  moves a rezzed object at most 10 m per call and not at all when the target is outside the region (0 to 256 in x and y, 0 to 4096 in z); in a child prim it sets the offset from the root,
  and `PRIM_POSITION` in a child prim takes region coordinates. A worn object's `llSetPos` only records the offset (`llGetPos` stays at the avatar). Rotations multiply as in LSL
  (a child's region rotation is its local rotation times the root's). `llSetObjectName` and the root prim's `PRIM_NAME` are the object's name. The root prim's `PRIM_SIZE` is `llGetScale`.
- **Text boxes:** `llTextBox` is `llDialog` with the single button `!!llTextBox!!`, and so is `llDialog` with that button first. The log entry is a `dialog` entry with `buttons` empty and `text_box: true`
  (the newest dialog in the answer has it too). The avatar's typed text is answered with the existing `dialog` input: `{"type": "dialog", "button": "the text typed"}` is said on the dialog's channel.
- **`llTransferLindenDollars` and `PERMISSION_DEBIT`:** logic and messages only; no L$ ever moves and the sandbox has no balance (the owner always has enough). The function returns a key and a
  `transaction_result(key id, integer success, string data)` event follows with that key. Without `PERMISSION_DEBIT` (the sandbox grants the owner's permissions at once, as for every permission) the
  call logs a `debug` line and fails with `MISSING_PERMISSION_DEBIT` (the debug wording is the sandbox's). Other failures: `INVALID_AMOUNT` (0 or less), `INVALID_DESTINATION` (not a key),
  `LINDENDOLLAR_ENTITYDOESNOTEXIST` (the key of an object; objects cannot be paid). Success gives `data` as `<avatar key>,<amount>`. The failure tags and the success format follow the Second Life wiki
  (page "transaction_result"); `INSUFFICIENTFUNDS`, `THROTTLED` and the like never happen here. Each call is a `linden_dollars` log entry.
- **Recorded only:** `llParticleSystem`, `llLinkParticleSystem`, `llTriggerSound`, `llPlaySound`, `llLoopSound`, `llStopSound`, `llPreloadSound`, `llTargetOmega`, `llLoadURL` do nothing but write a
  `recorded_only` entry with the function and its arguments (a list argument as its length). Nothing is played or opened.
  Step 9b-8 adds 37 more with the same behaviour: `llTriggerSoundLimited`, `llLoopSoundMaster`, `llLoopSoundSlave`, `llPlaySoundSlave`, `llSoundPreload`, `llAdjustSoundVolume`, `llSetSoundQueueing`, `llSetSoundRadius`, `llCollisionSound`, `llSetTextureAnim`, `llSetLinkTextureAnim`, `llOffsetTexture`, `llScaleTexture` (both also keep their values on the faces since steps 13 to 15), `llRotateTexture` (likewise), `llSetCameraEyeOffset`, `llSetCameraAtOffset`, `llSetCameraParams`, `llClearCameraParams`, `llSetLinkCamera`, `llSetPayPrice`, `llSetClickAction`, `llSetTouchText`, `llPointAt`, `llStopPointAt`, `llLookAt`, `llStopLookAt`, `llMakeExplosion`, `llMakeFountain`, `llMakeSmoke`, `llMakeFire`, `llCollisionFilter`, `llPassTouches`, `llPassCollisions`, `llSetRemoteScriptAccessPin` (real since the several-objects job: see "Several objects: giving, remote script load and inventory drop"), `llSetParcelMusicURL`, `llRefreshPrimURL`, `llSetPrimURL`.
- **Land and parcel (second gap list):** the region is one parcel (x and y from 0 up to 256). Off the region: `llGetLandOwnerAt` gives NULL_KEY, `llGetParcelFlags` and `llGetParcelMaxPrims` 0, the list functions `[]`. `llOverMyLand` is 1 for a key the sandbox knows, 0 otherwise. `llGetParcelPrimCount` counts this object's prims for TOTAL and OWNER, 0 for other categories. `llGetParcelDetails` skips an unknown PARCEL_DETAILS_* number. Ban list, pass list, `llMapDestination`, `llEjectFromLand` write a `recorded_only` entry; `llReturnObjectsByOwner` and `llReturnObjectsByID` do too and answer 0. `llSetParcelMusicURL` records and stores the address for `llGetParcelMusicURL`. Canned values: name `Sandbox Parcel`, area 65536, 15000 prims, flags 134217803, TP routing 2.
- **Animation overrides (step 9b-8):** `llSetAnimationOverride(state, animation)`, `llResetAnimationOverride(state)` (also `"ALL"`) and `llGetAnimationOverride(state)` need `PERMISSION_OVERRIDE_ANIMATIONS`; without it a debug line says so. An unknown state name gives a debug line. A successful set or reset writes an `animation_override` entry (`op`, `state`, `animation`), and the get answers the stored name, or an empty string.
- **Movement and physics stubs (second gap list, step 6, part 1):** `llMoveToTarget`, `llStopMoveToTarget`, `llPushObject`, `llRotLookAt` and `llVolumeDetect` do nothing but
  write a `recorded_only` entry with the function and its exact arguments, the same way as the step 9b-8 stubs above: no motion model runs, the object never moves or turns, and nothing is simulated of
  target seeking, push, look-at rotation or volume detection. A test script's intent (target, look-at rotation, detect flag) can still be read back from the log. `llApplyImpulse` and `llSetBuoyancy` are
  real functions now (part 3, below), not record only; the vehicle functions are not part of this step (step 7, below).
- **Collision-family events (second gap list, step 6, part 2):** there is still no motion model, so these events never happen on their own; new `inputs` types raise them for testing, with exactly the
  llDetected* data or vector arguments given. `collision` (`phase`: `"start"`, `"end"` or left out for the continuous `collision`; `link`: the prim hit, default the root; `name`/`id`: the other object or avatar, as for `touch`) raises
  `collision_start`, `collision_end` or `collision(num_detected)` (always 1) in the scripts of that prim that handle it, or the root's when none does (collisions are passed on, as touches are). `land_collision`
  (`phase`: `"start"`, `"end"` or left out for the continuous `land_collision`; `pos`: `[x, y, z]`, the event's own vector argument) and `moving` (`phase`: `"start"` or `"end"`, no detected data: `moving_start`
  and `moving_end` take no arguments) go to every script of the object, as `changed` does. `at_target` (`tnum`, `targetpos`, `ourpos`: the event's own arguments; `llTarget`'s target number is not tracked)
  and `not_at_target` (no fields) likewise go to every script. None of this simulates collisions, gravity or `llTarget`; it only lets a script's handler be tested against given data. Part 3, below, gives
  `moving_start`/`moving_end` a second source (the motion model itself); this manual `moving` input is unchanged.
- **A simple motion model (second gap list, step 6, part 3):** not a physics engine, one `inputs` type and two real functions, for one object at a time. `llApplyImpulse(vector impulse, integer local)`
  changes the object's velocity **at once** by `impulse / mass` (the mass `llGetMass` gives); `local` turns the impulse by the object's root rotation first, as `llSetPos`'s local flag does.
  `llSetBuoyancy(float buoyancy)` stores the value, clamped to 0 (full gravity) .. 1 (floats); other values that Second Life allows are refused here by clamping, not by an error. The new `inputs` type
  `step` (`dt`: seconds, 0 to 60; `object`: as for the other types) advances the model for one object by `dt` simulated seconds: velocity changes by simple gravity (`-9.8` m/s² along z, scaled by
  `(1 - buoyancy)`), then position changes by the new velocity; nothing happens between `step` inputs on its own. `llGetVel` answers the model's velocity (always `ZERO_VECTOR` before this step). Each
  `step` writes an `input` entry (`dt`, the new `velocity` and `position`), and `llApplyImpulse` writes its own `velocity` entry (`op: "impulse"`, the impulse turned, the mass, the new velocity);
  `llSetBuoyancy` writes a `velocity` entry too (`op: "buoyancy"`). When the object's speed crosses `0.1` m/s (the sandbox's own threshold) a `step` input also fires `moving_start` or `moving_end` itself,
  with a `moving` log entry (`phase`) that says it came from the model, not a manual input. A worn object's `step` does nothing but log a note that the model does not apply to an attachment. An object with keyframed motion (`llSetKeyframedMotion`, below) is moved by its frames instead of by gravity, and its `step` entry also has `keyframed` (`playing`, or `paused_or_ended`) and the new `rotation`. Left out on
  purpose, for a later job: `llMoveToTarget`/`llStopMoveToTarget` stay record only, so the model never seeks a target; `at_target`/`not_at_target` come from the manual inputs and, since the third gap-list run, from `llTarget` targets after a `step` ("Limits, targets and region reads"); `llPushObject`,
  `llRotLookAt` and `llVolumeDetect` stay record only; real collisions from motion (two objects actually touching) are untouched.
- **Vehicle function stubs (second gap list, step 7):** `llSetVehicleType`, `llSetVehicleFloatParam`, `llSetVehicleVectorParam`, `llSetVehicleRotationParam`, `llSetVehicleFlags` and `llRemoveVehicleFlags`
  do nothing but write a `recorded_only` entry with the function and its exact arguments, the same way as the movement stubs above: no vehicle physics is simulated, and the object's position, rotation
  and velocity never change because of these calls. A test script's intent (vehicle type, parameter, value, flags) can still be read back from the log.
- Not done: other `PRIM_*` rules, the prim's real shape and face count, touch text, sit targets, physics and everything that needs a 3D world; they say "not supported yet" where a function is involved.

#### HTTP requests with canned replies (Script Lab step 9b-9)

The sandbox never goes on the network: no request leaves the program. `lsl_run` takes `http` (at most 64), the replies to give:
`[{"url": "https://example.com/api", "method": "GET", "status": 200, "body": "{\"ok\":true}"}]`. Only `url` is needed: the exact URL (case-sensitive), or a prefix that ends in `*`
(`"*"` alone answers every URL); `method` is `GET`, `POST`, `PUT` or `DELETE` (any case; omitted: every method); `status` is 100 to 599 (default 200); `body` is text of at most 65 536 characters (default empty).
A request gets the first reply in list order that matches its method and URL. The same list is used for a single script and for `objects`, and it is kept for `lsl_event`.

- `llHTTPRequest(url, parameters, body)` checks the URL (it must start with `http://` or `https://`, any case, name a host and have no spaces or control characters) and the parameter list:
  `HTTP_METHOD` (`"GET"`, `"POST"`, `"PUT"` or `"DELETE"`, upper case; default `GET`), `HTTP_MIMETYPE` (default `text/plain;charset=utf-8`), `HTTP_BODY_MAXLENGTH` (0 to 16 384; default 2 048),
  `HTTP_CUSTOM_HEADER` (a name and a value; not `Content-Type`; all custom headers together at most 4 096 characters), `HTTP_USER_AGENT` (no spaces), `HTTP_VERBOSE_THROTTLE`, and `HTTP_VERIFY_CERT`, `HTTP_PRAGMA_NO_CACHE`,
  `HTTP_ACCEPT` and `HTTP_EXTENDED_ERROR`, which are accepted and change nothing here. A request that fails a check writes a `debug` line that starts with `llHTTPRequest:` and says what is wrong, returns `NULL_KEY`
  and raises no event (the sandbox's own wording; Second Life's messages are not documented in full).
- A request that passes returns a request key (`5c11ab00-...`, numbered in the script like the notecard query keys), writes an `http_request` entry and queues `http_response(key request_id, integer status, list metadata, string body)`
  at once (Second Life answers later) in every script of the prim that made the request, as Second Life does. With no matching reply the status is 499 and the body is empty (Second Life gives 499 for a request that
  times out after 60 seconds). A reply body longer than `HTTP_BODY_MAXLENGTH` UTF-8 bytes is cut there (never inside a character) and `metadata` is `[HTTP_BODY_TRUNCATED, <the limit>]`; otherwise `metadata` is empty.
- The throttle of Second Life: at most 25 requests per object in 20 seconds (counted over the last 20 simulated seconds). A request over it returns `NULL_KEY`, writes an `http_request` entry with `result: throttled`
  and, unless `HTTP_VERBOSE_THROTTLE` is `FALSE`, a `debug` line.
- Incoming HTTP (`llRequestURL` and the `http_request` event): "Money, web server and mail" below.
- Not done: the RFC 7807 answers of `HTTP_EXTENDED_ERROR`, status 415 for `HTTP_ACCEPT`,
  the per-owner throttle, a reply that comes later or never, and response headers.

#### Sensors (Script Lab step 9b-7)

`lsl_run` takes `avatars` (at most 64), the people the sensors can find: `[{"name": "Ann", "position": [128, 133, 20], "velocity": [1, 0, 0], "rotation": [0, 0, 0, 1], "in_group": true, "key": "..."}]`. Only `name` is needed;
`position` is region metres as for objects (default `[128, 128, 20]`), `key` defaults to `a0000000-0000-4000-8000-000000000001` and so on in list order. The owner always stands at `[128, 128, 20]`, unless an avatar
with the owner's key is listed. Avatars do not move: a scan always sees them where the input put them. The same list is used for a single script and for `objects`. `height` (metres, default 1.8) is for `llGetAgentSize` (second gap list, step 3).

- `llSensor(name, id, type, range, arc)` scans once, now, and queues `sensor(integer num_detected)` (or `no_sensor()` when nothing is found) at once. It finds the avatars (type `AGENT`, 1) and the other objects of the
  world (type `SCRIPTED`, 8; the sensing object itself is never found) that match: `name` exact and case-sensitive (empty: any), `id` (empty or `NULL_KEY`: any), a `type` mask that has the type's bit (a mask of 0, or `ACTIVE` and
  `PASSIVE` alone, finds nothing: the sandbox has no passive or moving objects), within `range` metres (a range of 0 or less finds nothing; more than 96 counts as 96) of the sensing object's position, and inside `arc` radians
  (more than PI counts as PI) of its forward direction (its x axis turned by the root prim's rotation, as set with `llSetRot`; something at distance 0 is always inside). The nearest come first, ties in list order (avatars,
  then objects), and at most 16 are returned. A worn object is where the avatar is, and the wearer is found like anyone else (Second Life may leave the wearer out). Each scan is a `sensor` log entry.
- `llSensorRepeat(name, id, type, range, arc, rate)` does the same every `rate` seconds, the first scan one `rate` after the call (a sandbox choice), at least 0.01 s apart. A new call replaces the earlier repeat; a `rate`
  of 0 or less (or `NaN`) removes it (sandbox choice); `llSensorRemove` removes it; a state change removes it. They write `sensor_set` and `sensor_remove` entries.
- `llDetectedName`, `llDetectedKey`, `llDetectedOwner` (an avatar is its own owner), `llDetectedType` (1 or 8, not the combinations Second Life reports for moving or physical objects), `llDetectedPos`, `llDetectedRot`
  (an object's root rotation), `llDetectedVel` (an avatar's `velocity`; objects do not move, so zero) and `llDetectedGroup` (1 for an avatar with `in_group`, else 0) read the list of the `sensor` event. An index out of range
  gives `NULL_KEY`, an empty string, the zero vector, the identity rotation or 0. A touch has one detected avatar with name, key and owner only (its position, rotation and velocity are not simulated; the touch data `llDetectedTouch*` is canned, see "Leftover reads and media").
- Not done: `llDetectedGrab`, passive and physical objects, avatars that move, and the `collision` and `land_collision` events.

#### Controls (Script Lab step 9b-6)

A new input type `control`, for `inputs` of `lsl_run` and for `lsl_event`: `{"at": 2, "type": "control", "levels": ["forward", "left"], "edges": "forward", "id": "<avatar key>"}` says which keys the avatar
holds now (`levels`) and which just changed (`edges`). Each is a name (`forward` or `fwd`, `back`, `left`, `right`, `up`, `down`, `rot_left`, `rot_right`, `lbutton`, `ml_lbutton`; also written `CONTROL_FWD`), a list of names,
or the `CONTROL_*` flags as a whole number; at least one must not be empty. `id` and `name` default to the owner. The log has an `input` entry (`input`: `control`, `id`, `levels`, `edges`).

- `llTakeControls(controls, accept, pass_on)` needs `PERMISSION_TAKE_CONTROLS` (an avatar sitting on the object gets it at once, the owner as for every permission); without it a `debug` line
  `Script trying to take controls but PERMISSION_TAKE_CONTROLS permission not set` is logged (the wording is the sandbox's) and nothing is taken. A taken set replaces the earlier one; `llReleaseControls` clears it.
  Each is a `controls` log entry (`take` with `controls`, `accept`, `pass_on`; `release` only when something was held).
- The `control(key id, integer level, integer edge)` event goes to a script that holds controls, has the permission from that avatar, and whose taken controls the input touches; `level` and `edge` are cut to the
  controls the script took (Second Life's exact filtering with `accept` false is not simulated: `accept` and `pass_on` are only recorded). A script that did not take the control, or holds the permission from another avatar, gets nothing.
- Not done: the avatar's movement itself (nothing moves), the repeat of `control` while a key is held (send one input for each change), and `llTakeCamera`/camera permissions.

#### Sitting and pose balls (Script Lab step 9b-5)

Two input types, for `inputs` of `lsl_run` and for `lsl_event`: `sit` (`{"at": 1, "type": "sit", "object": "chair", "link": 2, "id": "<avatar key>", "name": "<avatar name>"}`) puts an avatar on a prim
of an object (`link` 1 to the prim count, default the root; `id` and `name` default to the owner), and `stand` (`{"type": "stand", "id": ...}`; `object` is optional: without it the avatar's object is used)
takes it off. Each is an `input` entry (`input`: `sit` or `stand`, `id`, `name`; for `sit` also `on_target`, true when the avatar took the prim's sit target) and every script of the object gets `changed`
with `CHANGED_LINK` (32). An avatar that is already sitting, a `stand` of an avatar that does not sit on the object, a link the object does not have and a 17th avatar on one object (16 at most) are an `input`
entry with an `error`. The answer's `objects` entries have `sitters` (`key`, `name`, `link`, `on_target`) while someone sits.

- **Sit targets:** `llSitTarget(offset, rotation)` sets the sit target of the script's prim and `llLinkSitTarget(link, offset, rotation)` that of a prim by link number (the `LINK_*` values reach the prims they name);
  an offset and rotation that are both zero (rotation with x, y and z zero) remove it. Each call is a `prim` entry with `op` `sit_target` (`set`, `offset`, `rotation`). An avatar takes a prim's sit target when it has one and
  no one else sits on it; otherwise it sits on the prim without a target (as in Second Life) and `llAvatarOnSitTarget` does not name it. `llAvatarOnSitTarget` is the avatar on the script's prim's target (`NULL_KEY` when none),
  `llAvatarOnLinkSitTarget(link)` that of one link (`NULL_KEY` for a link the object does not have or a set of links). The sit position and the avatar's real place are not simulated, and the 3D view is not touched.
- **`llGetNumberOfPrims`** counts the avatars that sit on the object, as Second Life does. `llGetLinkNumber` of a one-prim object stays 0 (Second Life gives 1 while an avatar sits on it).
- **`llUnSit(key)`** takes an avatar off the script's own object (no other object; a key that does not sit there does nothing) and writes an `unsit` entry; every script of the object gets `changed` with `CHANGED_LINK`.
  Second Life also asks for the right to do it (object owner or land owner); the sandbox does not.
- **Permissions of a sitter:** `llRequestPermissions` on an avatar that sits on the object grants `PERMISSION_TRIGGER_ANIMATION` and `PERMISSION_TAKE_CONTROLS` at once (as Second Life does for sitters); the other bits
  are not granted, and when none is left nothing is granted and no `run_time_permissions` comes. The `permissions` entry says so. The owner still gets everything at once, any other key nothing. So the usual pose ball works:
  `changed` with `CHANGED_LINK`, `llAvatarOnSitTarget`, `llRequestPermissions`, `run_time_permissions`, `llStartAnimation`. Standing up does not stop the animation or take the permission away here: the script
  must stop it itself (in Second Life the animation stops when the avatar stands).
- **`llGetAnimationList(key)`** lists the keys of the animations this script started for that avatar (the avatar it holds permissions for) and has not stopped; an animation in the inventory (type `animation`) gives its
  key, any other name a key made from the object's key and the name. The avatar's own animations (standing, sitting) are not in the list. **`llGetAnimation(key)`** answers `Sitting` for an avatar sitting on any object of the
  world, `Standing` for the owner, and an empty string for any other key. **`llGetAgentInfo(key)`** answers `AGENT_SITTING | AGENT_ON_OBJECT` (0x30) for a sitting avatar, plus `AGENT_ATTACHMENTS` (2) for the owner
  while any object of the world is worn, and 0 for everyone else (walking, flying, typing and the other flags are never set).
- Not done: the avatar's position and rotation on the sit target, `llSitOnLink`, `llGetObjectPrimCount`, touch-to-sit (`llSetClickAction`), sit-text display, stand-up stopping animations, and the `control` event (the next step).

#### Inventory, notecards and rezzing (Script Lab step 9b-4)

An object holds the inventory the request gives it, and nothing else: no file is read. Give `inventory` inside an object of `objects`, or at the top of the
request with `source`: a list of at most 224 items `{name, type, key, lines}`. `name` is 1 to 63 characters (each item its own); `type` is `texture`, `sound`,
`landmark`, `clothing`, `object`, `notecard`, `script`, `bodypart`, `animation`, `gesture`, `setting` or `material` (also written `INVENTORY_NOTECARD`, any case;
omitted when `lines` is given, which means a notecard); `key` is the asset key (default: the MD5 of the object's key, `/` and the name, written as a key, so it is
the same in every run); `lines` is a notecard's list of text lines (at most 2000 lines and 65536 characters per notecard, 131072 per object). The object's scripts
are in the inventory too, as type `script` (an item may not have a script's name). The answer's `objects` entries list the inventory as it is at the end (`name`, `type`).

- `llGetInventoryNumber(type)` counts the items of that type (`INVENTORY_ALL`, -1, counts every item; a type with no items, or an unknown number, gives 0).
  `llGetInventoryName(type, index)` lists them sorted by name, upper and lower case alike, and gives an empty string for an index out of range (also a negative one).
  `llGetInventoryType(name)` gives the type or `INVENTORY_NONE` (-1; the name must match exactly). `llGetInventoryKey(name)` gives the item's key or `NULL_KEY`.
  Every item counts as full permission here: Second Life gives `NULL_KEY` for an item the script may not read.
- `llGiveInventory(destination, name)` writes a `give_inventory` entry: `given` to the owner's avatar (or the owner of any object of the world) and to another object of
  the world (the item is copied into that object's inventory and every script there gets `changed` with `CHANGED_INVENTORY`), `not_found` when this object has no such item,
  `unknown_target` for a key the sandbox does not know (including `NULL_KEY`), `name_exists` when the receiving object already has an item of that name (Second Life would rename
  the item). The item stays in the giver (Second Life keeps a copy-permission item, too). `llGiveInventory` does not sleep here (Second Life sleeps 2 seconds).
- `llRemoveInventory(name)` removes the item (a missing name does nothing, as in Second Life) and every script of the object gets `changed` with `CHANGED_INVENTORY`;
  an `inventory` entry is logged. Removing the running script itself stops the script as "not supported yet". Removing another script only removes the item: that script keeps running here.
- `llGetNotecardLine(name, line)` and `llGetNumberOfNotecardLines(name)` return a query key (`5c11ab00-...` numbered in the script, like every `llGenerateKey` and
  `llTransferLindenDollars` key) and queue `dataserver(key query, string data)` at once (Second Life answers some time later): the line (cut at 255 bytes, never in the middle of a
  character), `EOF` (`"\n\n\n"`) for a line past the end or a negative one, or the line count as text. A name that is missing or is not a notecard gives a `debug` line
  `Couldn't find notecard <name>` and `NULL_KEY`, and no event. The queue holds 64 events, so asking for more than 64 lines at once drops the rest (`event_dropped`), as Second Life does.
- `llRezObject(name, position, velocity, rotation, param)` and `llRezAtRoot` need an item of type `object` (else a `debug` line `Couldn't find object '<name>'`) and a position
  within 10 m of the rezzing object (exactly 10 m is fine; farther gives a `rez` entry with `result: too_far` and nothing else). On success the entry has the new object's key and
  the rezzing script gets `object_rez(key)`. The rezzed object is only a key: it does not run, has no `on_rez` and does not appear in the world (a copy-permission item stays in the
  inventory). The sandbox does not check the other limits of Second Life (the region's object limit, the rez throttle, no-copy items disappearing).
- Not done: `llGetNotecardLineSync` and `llGetInventoryAcquireTime`. Giving a list, loading a script into another object and inventory drop: the next section.

#### Several objects: giving, remote script load and inventory drop (Script Lab gap list)

Between objects of the same `lsl_run` world these do what Second Life does; for anything else the sandbox only logs.
No forced delay is simulated (Second Life sleeps 3 seconds in `llGiveInventoryList` and `llRemoteLoadScriptPin`, 0.2 in `llSetRemoteScriptAccessPin`).

- An inventory item (in `inventory`, see above) may also have `creator` (a key) and `description` (at most 127 characters), and a landmark a `position` (three numbers; steps 13 to 15, see "Ray casts, land, landmark data, texture reads, scale factor and object animations").
  **`llGetInventoryCreator(name)`** gives `creator`, or the key of the owner of the object that holds it when none is given; an item given with
  `llGiveInventory` or `llGiveInventoryList` keeps the giver's answer, so the receiver sees the giver's owner. **`llGetInventoryDesc(name)`** gives
  `description` (empty by default). For a missing item both log the `debug` line `No item named '<name>'` and give `NULL_KEY` and an empty string
  (the wiki says an error is shouted, but gives neither its text nor the return value).
- **`llGiveInventoryList(target, folder, list)`** gives every item of the list this object has (other list entries than strings and keys are skipped, a
  name twice counts once) and writes one `give_inventory_list` entry. To another object of the world: no folder is made, each item it does not have
  yet goes into its inventory (names it has already are listed under `name_exists`; Second Life would rename them) and its scripts get one `changed`
  with `CHANGED_INVENTORY` for the whole list. To the owner's avatar (or the owner of any object): `given` and nothing else (the folder is only logged).
  Missing items are listed under `missing` (Second Life shouts an error whose text the wiki does not give); with no item found the result is `not_found`.
- **`llSetRemoteScriptAccessPin(pin)`** sets the object's pin for remote loading; 0 (the start) turns remote loading off. The sandbox keeps one pin per
  object, where Second Life keeps one per prim. A `remote_pin` entry says whether loading is on; the pin is not logged.
- **`llRemoteLoadScriptPin(target, name, pin, running, start_param)`** copies a script of this object into another object of the world and writes a
  `remote_load` entry. The sandbox has code only for the object's own scripts (the ones given with `source` or `scripts`), so a script that is only an
  inventory item gives `no_source` and nothing is loaded. The target must have set the same pin, not 0 (else `wrong_pin` and the `debug` line
  `Task <this object> trying to illegally load script onto task <target>!`, Second Life's text). A target worn by another owner gives
  `other_owner_attachment` without a `debug` line (Second Life fails without a word there). The target's own key gives the `debug` line
  `Unable to add item!` and a name that is missing or not a script `Couldn't find script '<name>'` (the sandbox's own text; the wiki gives none);
  neither writes a `remote_load` entry. A key the sandbox does not know gives `unknown_target`. More than 32 scripts in the target or 64 in the
  world give `too_many_scripts`. A loaded script goes into the target's root prim, after its other scripts, and appears in its inventory; a script
  of the same name is replaced (`replaced`: it starts again with fresh memory). It is added once the scripts running at that moment have finished,
  at the same simulated time. With `running` not 0 it gets `state_entry` (no `on_rez`); with 0 it waits until another script of the prim calls
  `llSetScriptState(name, TRUE)`, then gets `state_entry`. `llGetStartParameter` in it gives `start_param` until the script resets, then 0. The
  target's other scripts get `changed` with `CHANGED_INVENTORY`.
- **`llAllowInventoryDrop(add)`** lets anyone drop items into the object (`allow_drop` entry). In a linkset only the root prim's call counts; a
  child prim's call is ignored (with a `note`). A new input type `drop`, for `inputs` of `lsl_run` and for `lsl_event`, drops an item:
  `{"at": 1, "type": "drop", "object": "box", "item": "Gift", "item_type": "texture", "name": "Ann", "id": "<avatar key>"}` (`item` is needed, 1 to 63
  characters; `item_type` as an inventory `type`, default `notecard`; no `name` and `id`: the owner). The owner may always drop, and the scripts get
  `changed` with `CHANGED_INVENTORY`. Anyone else only after `llAllowInventoryDrop(TRUE)`, and then the scripts get `changed` with
  `CHANGED_ALLOWED_DROP` (64); a script from them is refused with `Not permitted to edit this!` (Second Life wiki). The dropped item's creator is
  the avatar who dropped it. A name the object already has is refused (`name_exists`; Second Life would rename the new item). The `input` entry
  has `item`, `item_type`, `id`, `result` (`dropped`, `refused` or `name_exists`), `change` when dropped, and `error` when not.

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

#### Touches (Script Lab, second gap list, step 1)

- A touch input (`inputs` of `lsl_run` and `lab_watch`, or `lsl_event` with `type` touch) raises the three touch events of
  Second Life in order: `touch_start` when the avatar presses the mouse button, `touch` while it is held, and `touch_end`
  when it is let go (Second Life wiki, `touch`, `touch_start`, `touch_end`). `num_detected` is 1: one avatar touches at a time.
- `hold` (seconds, 0 to 60, default 0) is how long the touch is held. A touch at `t` gives `touch_start` and `touch` at `t`,
  then `touch` at `t + 0.1`, `t + 0.2`, ... while that is before `t + hold`, and `touch_end` at `t + hold`. So a click
  (`hold` 0) gives `touch_start`, `touch` and `touch_end` at the same time. Second Life raises `touch` about once per
  minimum event delay while the button is held; the fixed 0.1 simulated seconds is a sandbox choice.
- `llDetectedName`, `llDetectedKey`, `llDetectedOwner` (the avatar) and `llDetectedLinkNumber` (the prim touched) work in
  all three events. The log has one `input` entry for the touch (with `hold` when it is more than 0); the later phases
  show as the scripts' `event` entries only.
- The events go to the prim's scripts as described under "Several scripts in one object". The prim and the scripts are
  chosen again for each phase, so a script that changes state while the touch is held gets the events its new state
  handles.
- A touch held past the end of `lsl_run` (or of `lsl_event` with `seconds`) goes on in the next `lsl_event`. `lsl_event`
  with `hold` and without `seconds` runs on at least until the touch is let go.
- `{"command": "lsl_event", "type": "touch", "hold": 1.5}` holds the touch 1.5 seconds: `touch_start`, 15 `touch` events and `touch_end`.
- Not done: where on the face a click lands (the touch data is canned, see "Leftover reads and media"), several avatars touching at once, and the Live
  page's Touch button holding a touch (it sends a click).

#### Reads and state of the object itself (Script Lab, second gap list, step 2)

Seventeen functions that read or change the script's own object. Where Second Life's behaviour is not documented the
sandbox chooses, and [SCRIPT_LAB.md](SCRIPT_LAB.md) lists those doubts.

- **`llDie()`** stops the calling script at once with a `die` entry; then the object is deleted: its other scripts stop
  (a `die` entry each), sitting avatars stand up, and the answer's `objects` entry has `deleted: true`. A deleted object
  is not heard, not found by sensors, unknown to `llKey2Name`, `llGetOwnerKey` and `llGetObjectDetails`, and no target for
  `llGiveInventory`; an input sent to it is an `input` entry with `error` "The object was deleted with llDie.".
  In a worn object `llDie` does nothing (as in Second Life).
- **`llGetVel()`** is always `ZERO_VECTOR`: nothing moves in the sandbox. **`llGetMass()`** adds up the prims as boxes of 10
  lindograms per cubic metre (Second Life's default density, 1000 kg/m3, in units of 100 kg): the root prim with the
  object's scale, each child with its own size (a 0.5 m cube is 1.25). A worn object gives its own mass (Second Life:
  the avatar's).
- **`llSetScale(size)`** sets the script's prim size, each part clamped to 0.01..64 (as the wiki says); in the root it is
  `llGetScale`. It does nothing while `STATUS_PHYSICS` is set. A `prim` entry with `op` `size`, as `PRIM_SIZE`.
  **`llSetLocalRot(rot)`** sets a child prim's rotation relative to the root; in the root prim it sets the object's
  rotation, as `llSetRot`. A `prim` entry with `op` `rotation`. The forced delays are not simulated.
- **`llGetLinkName(link)`** and **`llGetLinkKey(link)`**: `LINK_THIS` and link numbers; `0` only in a single prim nobody sits
  on. The root has the object's name and key; a child its name (`PRIM_NAME`, default `Object`) and a key made from the
  object's key and its link number (the MD5 of `<object key>/link/<n>`, written as a key, the same in every run); avatars
  sitting on the object are the links after the prims, in the order they sat, with their name and key. Any other
  number (also `LINK_SET` and the like) gives `NULL_KEY`, for `llGetLinkName` too (Second Life wiki).
- **`llDetectedLinkNumber(i)`**: in `touch_start`, `touch` and `touch_end`, the prim touched (a touch input's `link`; 0 in a single prim). A touch on
  a prim without a touch script goes to the root's scripts with the number of the prim touched. Out of range, or in
  an event that is not a touch (such as `sensor`), it is 0.
- **Permission masks:** every object and item counts as made by its owner: `llGetObjectPermMask` and
  `llGetInventoryPermMask` give `PERM_ALL` (2147483647) for `MASK_BASE` and `MASK_OWNER`, 0 for `MASK_GROUP` and
  `MASK_EVERYONE`, `PERM_MOVE | PERM_TRANSFER` (532480) for `MASK_NEXT`; `MASK_COMBINED` changes nothing; another mask
  gives 0. `llGetInventoryPermMask` on a missing item gives 0 and a `debug` line `No item named '<name>'`.
- **`llGetCreator()`** is the owner's key. **`llGetUsername(id)`** is the legacy name of an avatar the sandbox knows (the
  owner, the `avatars` of the request, sitters) in lower case with a dot, without the last name `Resident` (`Ann Smith`
  gives `ann.smith`, `Bob Resident` gives `bob`, the owner `owner`); any other key gives `""`.
- **`llSameGroup(id)`**: every object of the sandbox is set to one group, key `9a0c0000-0000-4000-8000-000000000001`.
  `TRUE` for that key, for any object of the world, and for an avatar with `in_group`; `FALSE` for the owner and other
  avatars, `NULL_KEY` and any other key.
- **`llSetScriptState(name, running)`**, **`llGetScriptState(name)`** and **`llResetOtherScript(name)`** reach the scripts
  in the same prim (Second Life wiki). A name that is not a script of the prim gives a `debug` line `<function>: there
  is no script named "<name>" in this prim.` (the sandbox's wording) and, for `llGetScriptState`, `FALSE`. A script that
  is not running keeps its variables, state, listens and timer but gets no events (waiting and new ones are dropped)
  and its timer and sensor wait; running again, they count their interval from then. A script can stop itself (the
  rest of the event still runs; `llGetScriptState` on itself then answers `FALSE`) but not start itself. A stopped script
  (error or limit) counts as not running and is not changed. `llResetOtherScript` resets a running script (a `reset_other`
  entry, then the script's `reset`, `state` and `state_entry`); a script that is not running is not reset; on the
  calling script itself it is `llResetScript`. A change writes a `script_state` entry; the answer's script entries (or a
  single script's object entry) have `running: false` while a script is not running.
- **`llGetObjectDetails(id, params)`** knows the objects of the world (their root prim; not child prim keys and not deleted
  objects) and the avatars (the owner, the `avatars` of the request, sitters). An unknown key gives an empty list.
  Entries of `params` that are not integers are skipped; each unknown flag gives `OBJECT_UNKNOWN_DETAIL` (-1); the flags 50 to 56 give canned values, see "Leftover reads and media". For an object: name, description,
  position, rotation (the root's), velocity and omega zero, owner, group (the sandbox group), creator and rezzer (the
  owner), running and total script counts, script memory 65536 per running script, script and character time 0.0,
  prim equivalence and prim count the prims, server, streaming and physics cost 1.0 per prim (canned), root (its own
  key), attachment point, pathfinding type `OPT_LEGACY_LINKSET` (`OPT_OTHER` when worn), physics and phantom (from
  `llSetStatus`; 0 when worn), temp on rez, render weight 0, hover height 0.0, body shape -1.0, last owner `NULL_KEY`,
  click action 0, inventory count (the scripts too), group tag `""`, temp attached 0, attached slots 0, creation and
  rez time the start of the run (`2026-01-01T00:00:00.000000Z` unless `region` gives another `unix_start`), select count 0, sit count, animated count 0, animated slots 0, account level
  -1, material `PRIM_MATERIAL_WOOD`, mass (as `llGetMass`), hover text, its colour and alpha, link number (0 for a single
  prim nobody sits on, else 1), scale. For an avatar (per the wiki where it says): its name, `""` description, where it
  stands, its own key as owner and as root (or the object it sits on), `NULL_KEY` group, creator, last owner and rezzer,
  the scripts in the objects it wears, attachment point 0, `OPT_AVATAR`, render weight -1 (unknown), body shape 0.0,
  attached slots 38 minus the objects it wears, animated slots 1, account level 0, scale, mass, text colour and text alpha zero, and 0 or empty
  for the object-only flags (all canned).

#### The region and the avatars in it (Script Lab, second gap list, step 3)

Twelve functions that read the region and the avatars in it: `llGetRegionName`, `llGetRegionCorner`, `llGround`,
`llWater`, `llGetAgentSize`, `llGetAgentList`, `llRequestAgentData`, `llGetSunDirection`, `llGetTimeOfDay`,
`llGetTimestamp`, `llGetWallclock` and `llGetDate`. Every value is canned: the same in every run, from the region's
settings and the sandbox's simulated clock (seconds since the start of the run), never the computer's clock.

`lsl_run` takes `region` (optional), the settings of the one region every object stands in; each field is optional:

| Field | Default | Meaning |
|---|---|---|
| `name` | `Sandbox` | `llGetRegionName`; 1 to 35 characters |
| `corner` | `[256000, 256000, 0]` | `llGetRegionCorner`: the region's south-west corner in whole metres on the grid (z is 0) |
| `ground` | `20` | `llGround`: the ground is flat, so every offset gives this height (0 to 4096 m) |
| `water` | `20` | `llWater` at every offset (0 to 4096 m; 20 is Second Life's usual water height) |
| `day_length` | `14400` | seconds of one day of the region's sun (60 to 2592000; 14400 is Second Life's default 4 hours) |
| `time_of_day` | `7200` | `llGetTimeOfDay` at the start: seconds since the region's midnight (0 up to `day_length`; 7200 is noon) |
| `unix_start` | `1767225600` | `llGetUnixTime` at the start: UTC seconds since 1970 (2026-01-01 00:00:00 UTC; at most 2000000000) |

The world is kept for `lsl_event`, the region too. An avatar in `avatars` can also give `height` (0.5 to 3 m, default 1.8).

- **`llGetTimestamp()`** is the UTC time `unix_start` plus the simulated seconds, `YYYY-MM-DDThh:mm:ss.ffffffZ` with
  microseconds (`2026-01-01T00:00:01.250000Z` after 1.25 s). **`llGetDate()`** is its date, `YYYY-MM-DD`.
  **`llGetUnixTime()`** is `unix_start` plus the whole simulated seconds.
- **`llGetWallclock()`** is the seconds since midnight in Second Life time, US Pacific time: UTC-8, and UTC-7 in summer
  time from the second Sunday of March at 2:00 to the first Sunday of November at 2:00 (the US rule since 2007). The
  default start is 16:00 on 31 December, so it gives 57600.0.
- **`llGetTimeOfDay()`** is `time_of_day` plus the simulated seconds, modulo `day_length`. **`llGetSunDirection()`** is
  the unit vector `<sin a, 0, -cos a>` with `a = 2 PI * time of day / day_length`: straight down at midnight, east
  (`<1, 0, 0>`) at a quarter of the day, straight up at noon, west at three quarters. A part within 0.000001 of a whole
  number is made whole. Second Life's sun also leans to one side and its sky can be set per parcel; the sandbox's
  sun does not.
- **`llGetAgentSize(id)`** is `<0.45, 0.6, height>` for an avatar in the region (the owner, the `avatars` of the request,
  avatars sitting on an object), and `ZERO_VECTOR` for any other key (objects too). Width and depth are the sandbox's
  choice; Second Life works the size out from the avatar's shape.
- **`llGetAgentList(scope, options)`**: the sandbox is one region with one parcel, so `AGENT_LIST_PARCEL`,
  `AGENT_LIST_PARCEL_OWNER` and `AGENT_LIST_REGION` give the same keys: the owner first (unless `avatars` lists the
  owner, then in that place), the `avatars` of the request in their order, then avatars sitting on an object that are
  not listed, in the order they sat. Any other scope gives `["INVALID_SCOPE"]`. `options` is not used.
- **`llRequestAgentData(id, data)`** returns a query key and writes a `dataserver_request` entry
  (`{"function": "llRequestAgentData", "avatar", "data", "query"}`); for an avatar in the region the `dataserver` event
  comes at once with: `DATA_ONLINE` `"1"` (every avatar of the sandbox is online), `DATA_NAME` the avatar's name,
  `DATA_BORN` the date of the start of the run (`2026-01-01`), `DATA_RATING` `"0,0,0,0,0,0"`, `DATA_PAYINFO` `"0"`. Any
  other key, or another `data` number, gets no `dataserver` event. The forced delay (0.1 s) is not simulated.

#### Money, web server and mail (Script Lab, second gap list, step 4)

Nine functions and three events, all simulated: no L$ moves, nothing goes on the network (no socket is opened), and no
mail is sent. Every value is the same in every run.

- **`llGiveMoney(destination, amount)`** always returns 0, as in Second Life. It writes a `give_money` entry with
  `result`: `ok`; `MISSING_PERMISSION_DEBIT` without `PERMISSION_DEBIT` (also a `debug` line "Script trying to pay L$
  but PERMISSION_DEBIT permission not set", the sandbox's wording); `INVALID_DESTINATION` (not a key);
  `INVALID_AMOUNT` (0 or less); `LINDENDOLLAR_ENTITYDOESNOTEXIST` (an object of the world, not an avatar). The same
  checks as `llTransferLindenDollars`; the sandbox owner always has enough L$.
- **`llEmail(address, subject, message)`** writes an `email_sent` entry and makes the script sleep 20 simulated seconds,
  as Second Life does. Second Life puts a header in front of the message (`Object-Name: <name>`, `Region: <region>
  (<corner x>, <corner y>)`, `Local-Position: (<x>, <y>, <z>)` and an empty line); address, subject, header and
  message together may be at most 4096 bytes. Longer: `result: too_long` and a `debug` line, nothing would be sent
  (the sleep still happens). `bytes` is the count with the header.
- **`llGetNextEmail(address, subject)`** takes the first email in the object's mailbox whose sender and subject match
  (an empty filter matches anything; upper and lower case alike) and raises `email(string time, string address, string
  subject, string message, integer num_left)` in the calling script: `time` is the UTC time the email arrived as text
  (`unix_start` plus the whole simulated seconds, "The region" above), `num_left` the emails still waiting. No match:
  no event. Each call writes an `email_check` entry. Emails come only from `email` inputs; the mailbox holds at most
  100.
- **`llRequestURL()`** and **`llRequestSecureURL()`** return a request key and raise `http_request(request, method,
  body)` in the calling script with `method` `URL_REQUEST_GRANTED` and the URL as `body`: `http://sandbox.invalid:12046/cap/<key>`
  (`https://...:12043/...` for a secure one), without a trailing slash. `.invalid` never resolves (RFC 2606), so the
  URL cannot reach anything. `<key>` is the MD5 of `<object key>/<script name>/url <n>` written as a key, `n` counting
  the URLs that script was given, so a URL is never given twice and is the same in every run. The whole world has 38
  URLs (Second Life's cap per resident; Second Life also counts them per parcel); when they are used the method is
  `URL_REQUEST_DENIED` and the body empty. A `url_request` entry is written.
- **`llReleaseURL(url)`** frees a URL of the calling script (`url_release` entry, `result` `released` or
  `not_found`). **`llGetFreeURLs()`** is 38 minus the URLs held in the whole world. A script reset (`reset` entry with
  `urls_released`), `llDie`, and a `changed` input with `region`, `teleport` or `region_start` (its `input` entry has
  `urls_released`) free the URLs too, as Second Life does on a reset, a region change and a region restart.
- **`llHTTPResponse(request, status, body)`** answers a request from outside: an `http_server_response` entry with
  `result` `sent`, `timed_out` (more than 25 simulated seconds after the request came; Second Life has then answered
  the caller itself) or `unknown_request` (not a waiting request of this script, or already answered). Nothing is sent.
- **`llGetHTTPHeader(request, name)`** reads a header of a waiting request, up to 30 seconds after it came and until it
  is answered; `""` otherwise. Names are lower case (a name with upper case letters finds nothing, as in Second Life):
  `x-script-url` (the URL), `x-path-info` (the path after the URL, before `?`), `x-query-string` (after the first
  `?`), `x-remote-ip` (always `192.0.2.10`, a documentation address, RFC 5737) and the input's `headers` (names made
  lower case; `user-agent` only when the input gives one). Values are cut to 255 bytes.

New input types, in `inputs` of `lsl_run` and in `lsl_event`:

| type | Fields | What happens |
|---|---|---|
| `money` | `amount` (needed: the L$ paid, 1 to 2147483647), `id` and `name` (the payer; default the owner), `object`, `link` (the prim paid, default the root) | the scripts of that prim that have a `money` event get `money(id, amount)`; when none has one, the root's scripts that have one; when no script of the object has one, the `input` entry has `error` (the object cannot be paid). The `input` entry has `id`, `amount`, `name` and `note` |
| `http_request` | `url` (a URL a script got, with any path and query; default the newest URL of `object`), `method` (`GET`, `POST`, `PUT` or `DELETE`, any case; default `GET`), `body` (only for `POST` and `PUT`; cut to 2048 bytes), `headers` (an object of at most 32 names, letters, digits and `-`, with text values) | the script that holds the URL gets `http_request(request, method, body)`; `request` is `c0de0000-0000-4000-8000-<n>`, counted in the world. The `input` entry has `method`, `url`, `body`, `body_truncated_at` (when cut), `request`, and `object` and `script` when the world names them. No script holds the URL: `status` 404 and `error`; a query without a slash after the URL (`<url>?a=1` instead of `<url>/?a=1`): `status` 500, as in Second Life; the script already has 64 requests waiting for `llHTTPResponse`: `status` 503, no event |
| `email` | `address` (the sender, default `sender@sandbox`), `subject`, `body` (the message, at most 4096 characters), `object` | the email waits in the object's mailbox for `llGetNextEmail`; the sandbox raises no event when it arrives. The `input` entry has `address`, `subject` and `waiting` (emails in the mailbox), or `error` when the mailbox is full |

```
{"command": "lsl_run", "source": "...", "inputs": [
   {"at": 1, "type": "http_request", "method": "POST", "url": "http://sandbox.invalid:12046/cap/<key>/items?x=1", "body": "hello"},
   {"at": 2, "type": "money", "amount": 50},
   {"at": 3, "type": "email", "address": "mail1@sandbox", "subject": "Order", "body": "two apples"}]}
{"command": "lsl_event", "type": "http_request"}
```

- Doubts, written down instead of guessed (the sandbox chose): the wording of the `debug` lines, the order of an
  email's header lines, the 100 emails a mailbox holds, that every script of the prim with a `money` event gets a
  payment, and that an email arrives without an event (the `email` event comes only from `llGetNextEmail`).
- Not done: the content type of the answer (`llSetContentType` is only recorded), emails between objects of the world (an `llEmail`
  to `<object key>@lsl.secondlife.com` is only recorded), the pay price dialog (`llSetPayPrice` stays recorded only),
  the URLs lost when an object is taken and rezzed again, and the forced delays of other functions.

#### Prim media (Script Lab, second gap list, step 5)

`llSetPrimMediaParams`, `llGetPrimMediaParams` and `llClearPrimMedia` are record only: nothing is fetched or played
and no connection is opened. The media of each face (0 to 5) of the prim the script is in is kept, so a script reads
back what it set. Every call writes a `prim_media` entry. No forced delay is simulated.

- **`llSetPrimMediaParams(face, rules)`** returns a `STATUS_*` number. `rules` is `[PRIM_MEDIA_..., value, ...]`. A face
  with no media starts from these defaults: `ALT_IMAGE_ENABLE` 0, `CONTROLS` 0 (standard), `CURRENT_URL` and `HOME_URL`
  empty, `AUTO_LOOP` 0, `AUTO_PLAY` 0, `AUTO_SCALE` 1, `AUTO_ZOOM` 0, `FIRST_CLICK_INTERACT` 0, `WIDTH_PIXELS` and
  `HEIGHT_PIXELS` 0, `WHITELIST_ENABLE` 0, `WHITELIST` empty, `PERMS_INTERACT` 7 and `PERMS_CONTROL` 0. Rules given
  later change only their own values; the yes/no rules store 0 or 1. Answers: `STATUS_OK` (0);
  `STATUS_BOUNDS_ERROR` (1002) for a face outside 0 to 5 (`ALL_SIDES` too), `CONTROLS` other than 0 or 1, a width or
  height outside 0 to 2048, `PERMS_*` outside 0 to 7, a URL over 1024 bytes, a whitelist over 1024 bytes or 64
  entries; `STATUS_MALFORMED_PARAMS` (1000) for an odd number of list elements or an unknown rule number;
  `STATUS_TYPE_MISMATCH` (1001) for a rule number that is not an integer or a value of the wrong type;
  `STATUS_WHITELIST_FAILED` (2001) when `WHITELIST_ENABLE` is on and the host of `CURRENT_URL` is not a listed host or
  a sub-domain of one (the sandbox compares the host only, ignores scheme, port and path, and takes `*.` as the
  host itself). The first problem found (in list order) is answered, and nothing is stored when there is one.
- **`llGetPrimMediaParams(face, rules)`** returns the values in the order asked: integers (yes/no rules 0 or 1) and
  strings. An empty list when the face has no media (also for a wrong list of rules). When the face is wrong or the
  list holds a rule number that is not an integer or not known, the list holds one element, the `STATUS_*` number
  (1002, 1001, 1000). The `get` entry has `result` `ok`, `no_media` or the `STATUS_*` name.
- **`llClearPrimMedia(face)`** removes the media of a face (the next `llSetPrimMediaParams` starts from the defaults)
  and returns `STATUS_OK`, also when the face had none; `STATUS_BOUNDS_ERROR` for a face outside 0 to 5.
- Doubts, written down instead of guessed (the sandbox chose): the defaults (`AUTO_SCALE` 1, `PERMS_INTERACT` 7), that
  the answers for a wrong `llGetPrimMediaParams` call are a one-element list, that clearing a face without media is
  `STATUS_OK`, that the whitelist compares hosts only, and that a prim has exactly 6 faces.
- Not done: permission
  checks of the media (who may set it), and the 1 second sleep.

#### Leftover reads and media (Script Lab, second gap list)
Canned answers and record-only functions; nothing is fetched, played or shown. Where the Second Life wiki is unclear the function gives its documented return type with a harmless value.
- **Display names:** `llGetDisplayName(id)` gives the avatar's name without the last name `Resident` (`Guest Resident` gives `Guest`, `Ann Smith` stays); `""` for a key that is no avatar of the sandbox. `llRequestDisplayName(id)` and `llRequestUsername(id)` return a query key, write a `dataserver_request` entry (`function`, `avatar`, `query`) and, for an avatar of the sandbox, raise `dataserver` at once with the display name or the user name (as `llGetUsername`); any other key gets no event.
- **`llRequestSimulatorData(region, data)`** returns a query key and writes a `dataserver_request` entry (`function`, `region`, `data`, `query`). Only the sandbox's own region is known (name compared without regard to case): `DATA_SIM_POS` gives its corner as a vector text, `DATA_SIM_STATUS` `up`, `DATA_SIM_RATING` `PG`. For another region the status is `unknown` and the rating `UNKNOWN` (the wiki), and `DATA_SIM_POS` gets no event. Any other number gets no event.
- **`llGetGMTclock()`** gives whole seconds since midnight UTC of the simulated clock (the region's start time plus the simulated seconds). **`llGetEnv(key)`** gives the wiki's keys with canned values (for example `sim_channel` `Second Life Server`, `sim_version` `10.11.30.215699`, `dynamic_pathfinding` `enabled`, `agent_limit` `40`, `region_start_time` the region's `unix_start`, `frame_number` 1 plus 45 per simulated second); `""` for any other key (`region_name` is not on the wiki's list). **`llSetContentType`** writes a `recorded_only` entry.
- **Link media:** `llSetLinkMedia(link, face, params)`, `llGetLinkMedia(link, face, params)` and `llClearLinkMedia(link, face)` use the storage and the rules of `llSetPrimMediaParams`, `llGetPrimMediaParams` and `llClearPrimMedia`, so media set with one family reads back with the other. `llSetLinkMedia` and `llClearLinkMedia` take a link number, `LINK_THIS` or a set of links, and `ALL_SIDES`; no such link gives `STATUS_NOT_FOUND` (1003), a face outside 0 to 5 `STATUS_BOUNDS_ERROR`. A whitelist that refuses the address changes no face. `llGetLinkMedia` takes one link and one face (`ALL_SIDES` is `STATUS_BOUNDS_ERROR`); a set of links gives the empty list, a missing link `[STATUS_NOT_FOUND]`. A `prim_media` entry with `op` `set`, `get` or `clear` and the `link` as given.
- **Touch data:** inside `touch_start`, `touch` and `touch_end`, `llDetectedTouchFace` gives 0, `llDetectedTouchUV` and `llDetectedTouchST` `<0.5, 0.5, 0>`, `llDetectedTouchNormal` `<0, 0, 1>`, `llDetectedTouchBinormal` `<1, 0, 0>` and `llDetectedTouchPos` the touched prim's position (a sandbox choice: where a click lands is not simulated). Anywhere else, and for an index with nobody, they give `TOUCH_INVALID_FACE`, `TOUCH_INVALID_TEXCOORD` or `TOUCH_INVALID_VECTOR`.
- **`llGetObjectDetails` flags 50 to 56:** `OBJECT_HEALTH` and `OBJECT_DAMAGE` 0.0, `OBJECT_DAMAGE_TYPE` 0 (`DAMAGE_TYPE_GENERIC`), `OBJECT_LOCKED` and `OBJECT_VOLUME_DETECT` 0 (`llVolumeDetect` is record only). `OBJECT_PERMS` and `OBJECT_PERMS_COMBINED` give five integers each (base, owner, group, everyone, next owner), the same as `llGetObjectPermMask` (`2147483647, 2147483647, 0, 0, 532480`), five zeros for an avatar.
- Not done: the forced delays, a display name that differs from the user name, `DATA_SIM_RELEASE`, other regions in `llRequestSimulatorData`.

#### Limits, targets and region reads (Script Lab, third gap-list run, steps 1 to 3)
Canned-answer and small state functions; nothing changes on screen. Where the Second Life wiki is unclear the function gives its documented return type with a harmless value.
- **Limits:** `llGetMemoryLimit()` is 65536 (a Mono script) and goes back to it when the script resets. `llSetMemoryLimit(limit)` stores the limit and answers TRUE when it is from the memory the script uses now (`llGetUsedMemory`) up to 65536; otherwise FALSE and nothing changes (the wiki gives no exact rule for the low end). Nothing is enforced: a script may use more than its limit. `llMinEventDelay(delay)` writes a `recorded_only` entry. `llGetEnergy()` is 1.0.
- **Sides:** `llGetNumberOfSides()` is 6 and `llGetLinkNumberOfSides(link)` is 6 for a link number the object has (0 and 1 for a single prim) and for `LINK_THIS`, else 0: every simulated prim is a box.
- **Other objects:** `llGetObjectPrimCount(id)` is the prim count of the object with that key (avatars sitting on it are not counted), 0 for an avatar or an unknown key. `llGetObjectMass(id)` is the object's mass, worked out as `llGetMass` does (the two agree for the own object), 50.0 for an avatar (canned: the sandbox has no avatar body) and 0.0 for an unknown key. `llGetBoundingBox(id)` is `[min, max]`: the smallest box along the region's axes round all prims (each a box turned by its rotation; a child's place and rotation are turned by the root's), relative to the root prim's position; for an avatar the sandbox's avatar box (0.45 by 0.6 by the height) round the avatar's position; `[]` for an unknown key.
- **`llTarget(position, range)`** gives a handle: 1 for the first, then 2, 3 and so on (the wiki does not say where handles start); every script has its own targets and handles, and a script reset clears them. At most 64 targets are kept per script (the oldest goes). **`llTargetRemove(handle)`** removes it (an unknown handle does nothing). A target stays until it is removed, as in Second Life.
- **`at_target` and `not_at_target` from targets:** the `step` input is the only thing that moves an object in the sandbox, so after each `step` of an object (not a worn one) every script's targets are checked against the new position: each target within `range` metres queues `at_target(handle, targetpos, ourpos)`, and when the script has targets but none is reached `not_at_target()` is queued once. `llSetPos` and the other ways a position changes raise nothing. The manual `at_target` and `not_at_target` inputs work as before.
- **Region:** `llGetRegionTimeDilation()` is 1.0, `llGetRegionFPS()` is 45.0, `llGetSimulatorHostname()` is `sim-sandbox.invalid` (a name that never resolves), and `llGetRegionAgentCount()` is the number of agents in the world, as `llGetAgentList(AGENT_LIST_REGION)` lists them: the owner plus every other avatar of the `avatars` option (an avatar given with the owner's key is the owner). The canned values are fields of `RegionSettings` (C++ only; no `lsl_run` option sets them yet).
- Not done: enforcing the memory limit, a physics-based mass for avatars, the forced delays.

#### Region position, pure maths and force records (Script Lab, third gap-list run, steps 4 to 6)
Technical functions; nothing changes on screen. Where the Second Life wiki is unclear the function gives its documented return type with a harmless value.
- **`llSetRegionPos(position)`** moves the whole object to a region position and answers TRUE; it logs a `prim` entry (`op` `position`) like `llSetPos`. There is no 10 m limit (the wiki documents none). It answers FALSE, logs the same entry with a `note` and does not move for an attachment, for a physical object (`STATUS_PHYSICS`) and for a target outside x and y -10 to 266 and z 0 to 4096. The ground snap for a target below the terrain is not modelled. `llWarp2Pos` is no LSL function (neither the viewer's keyword list nor Second Life's definitions know it) and stays unknown: "not supported yet".
- **`llListStatistics(mode, list)`** with every `LIST_STAT_*` mode. Only integers and floats count; an empty list, or one without numbers, gives 0.0. Where the wiki is silent: the median of an even count is the mean of the two middle values; `STD_DEV` is the sample deviation (n - 1, which matches the wiki's example, 0.0 for one value); `GEOMETRIC_MEAN` is 0.0 when a value is 0 or the product is negative (the wiki contradicts itself there); an unknown mode gives 0.0.
- **`llRot2Angle(rot)`** is the unsigned angle of the rotation, 0 to PI, by the wiki's formula (the same one `llAngleBetween` uses); the rotation need not be normalised.
- **`llXorBase64(text, key)`**, **`llXorBase64Strings`** and **`llXorBase64StringsCorrect`** decode both Base64 strings, xor the text with the repeating key and encode the result; an empty key returns the text unchanged. The wiki does not describe the old bug of `llXorBase64Strings`, so it behaves like `llXorBase64`; `llXorBase64StringsCorrect` also ends the key at its first NUL byte, as the wiki says. The two old names are deprecated (the lint warns; they still run).
- **`llModPow(a, b, c)`** is a to the power b modulo c with all three read as unsigned 32-bit values and the answer given as a signed integer (the wiki's two examples are in the tests); a modulus of 0 gives 0.
- **Force and velocity (record only, no physics):** `llSetForce(force, local)` stores the force on the object (turned by the object's rotation when `local`) and logs a `velocity` entry (`op` `force`); `llGetForce()` reads it back (`<0,0,0>` before any call). The motion model does not use the force. `llSetVelocity(velocity, local)` sets the velocity `llGetVel` and the motion model use (`velocity` entry, `op` `set`), with no physics check, as `llApplyImpulse` makes none. `llApplyRotationalImpulse` and `llGroundRepel` (obsolete) write a `recorded_only` entry.

#### Damage, keyframed motion and linking objects (Script Lab, third gap-list run, steps 7 to 9)
Technical functions; nothing changes on screen. Where the Second Life wiki is unclear the choice is written here and in `tests/tst_lslsteps789.cpp`.
- **`llSetDamage(damage)`** stores the damage and logs a `damage` entry. The wiki's range is -100.0 (full heal) to 100.0 (instant kill); a value outside it (or NaN) is cut to that range and the entry says so. Nothing is damaged in the sandbox, so nothing reads the value. `llGetDamage` is no LSL function (the wiki and the viewer's keyword list do not know it) and stays unknown.
- **`llSetKeyframedMotion(keyframes, options)`** checks and stores the motion; the `step` input plays it. A frame is a position (a move in region axes), a rotation (a turn in the object's own axes) and a time in seconds, relative to the transform before it; `KFM_DATA` with only `KFM_TRANSLATION` or only `KFM_ROTATION` leaves the other out of every frame. Options are integer flag and value pairs: `KFM_MODE` (`KFM_FORWARD`, `KFM_LOOP`, `KFM_PING_PONG`, `KFM_REVERSE`), `KFM_DATA`, and `KFM_COMMAND` (`KFM_CMD_PLAY`, `KFM_CMD_STOP`, `KFM_CMD_PAUSE`), which must be the only option and cannot be in the call that sets keyframes. An empty list with `KFM_COMMAND` controls the motion; an empty list without it ends the motion. Errors, each a `debug` line on `DEBUG_CHANNEL` (`llSetKeyframedMotion: ...`; the wording is the sandbox's own) plus a `keyframed` entry with `applied` false, and nothing changes: an unknown flag or value, an odd option list, a list that does not fit the frame size, a wrong type in a frame, a time under 0.1 s (the wiki's documented minimum), a move faster than 250 m/s, a rotation without length, and a call from an attachment, from a physical object (`STATUS_PHYSICS`) or from a script that is not in the root prim. Choices where the wiki is silent: `KFM_MODE` and `KFM_DATA` may be left out with a keyframe list (forward, both kinds of data; the wiki says they "must" be given); a time may be an integer or a float; `KFM_REVERSE` plays the frames last to first, each undone (the wiki only says "in reverse"); `KFM_PING_PONG` plays forward and then undoes the frames last to first; `KFM_LOOP` starts at the first frame again and the frames stay relative, so the object does not jump back. In the `step` model a frame moves the object in a straight line and spreads its turn evenly, the shortest way; `llGetVel` answers the speed of the frame in progress (0 when paused or ended); a keyframed object does not fall (the model's gravity is skipped while a motion is stored, even after `KFM_FORWARD` ended); a finished forward or reverse motion goes back to its first frame for `KFM_CMD_PLAY`. Not modelled: collisions, the motion pausing while the object is selected, the ban on moving prims with `llSetPos` while it plays, keeping the motion after the script is removed, and the forced delay.
- **`llCreateLink(target, parent)`** and **`llBreakAllLinks()`** need `PERMISSION_CHANGE_LINKS` granted by the owner (the sandbox grants the owner's request at once); without it a `debug` line `Script trying to change links but PERMISSION_CHANGE_LINKS permission not set!` (the wording is a guess) is written and nothing else happens. The sandbox links the objects of its world, by key: the target must be an object of the world, not attached, with the same owner, with every object of the set within 64 m (the wiki's linkability page says 54 m and has a banner for 64 m since 27 June 2023; the sandbox takes the largest distance between two objects, which is a little less strict than the prims' bounding sphere), not in the same linkset already, and the whole linkset at most 256 prims. A script in an attachment fails silently (a `link` entry with `note`). With `parent` TRUE the script's object is the root, otherwise the target is. The new children join at link 2, the newest first, as in the wiki's example (A1, B1, B2, B3, A2 for A with two prims and B with three). Every object keeps its scripts and their own `link`; `llGetNumberOfPrims`, `llGetLinkNumber` and the prims the scripts read and change follow the linkset, and every script of the objects involved gets `changed(CHANGED_LINK)` (the wiki does not say it does; Second Life does). `llBreakAllLinks` separates every object linked with `llCreateLink` and gives each its prims back; a linkset an object has from the start (`prims`) never separates (a `link` entry with a `note`). Limits of the model: an object that is linked can only be the root of the next link (joining two linksets, or linking a child to another root, is refused with a `note`); link messages and `llSetLinkPrimitiveParams` still use each object's own link numbers; positions and rotations of the prims are not recomputed; every sandbox object counts as modifiable (so the wiki's "Delink failed because you do not have edit permission" never happens); the 0.1 s forced delay and the energy cost are not played.

#### Ray casts, land, landmark data, texture reads, scale factor and object animations (Script Lab, third gap-list run, steps 13 to 15)
Technical functions; nothing changes on screen. Where the Second Life wiki is unclear the choice is written here and in `tests/tst_lslsteps131415.cpp`. The wiki's forced delays are not modelled, as for every other function.
- **`llCastRay(start, end, options)`:** an object is one box (the smallest box along the region's axes round all its prims, the same as `llGetBoundingBox`), an avatar a box of 0.45 by 0.6 metres and its height, the ground the flat plane at `llGround` inside the region (256 by 256 m). A ray that starts inside a box does not hit it (the wiki says this for the script's own prim). Options are integer pairs: `RC_REJECT_TYPES` (`RC_REJECT_AGENTS` 1, `RC_REJECT_PHYSICAL` 2, `RC_REJECT_NONPHYSICAL` 4, `RC_REJECT_LAND` 8), `RC_DETECT_PHANTOM`, `RC_DATA_FLAGS` (`RC_GET_NORMAL` 1, `RC_GET_ROOT_KEY` 2, `RC_GET_LINK_NUM` 4) and `RC_MAX_HITS` (default 1; 0 and less count as 1, more than 256 as 256). The answer is, for each hit, the key, the link number (with `RC_GET_LINK_NUM`), the place and the normal (with `RC_GET_NORMAL`), then the number of hits; no hit is `[0]`. Phantom objects are hit only with `RC_DETECT_PHANTOM` (also when `RC_REJECT_NONPHYSICAL` would reject them); worn objects are never hit.
  Guesses: hits come nearest first (the wiki gives no order); a hit gives the root key also without `RC_GET_ROOT_KEY` (a hit is on a whole object); the link number is 0 for an object of one prim, an avatar and the ground, and 1 (the root) for a linkset; the ground gives `NULL_KEY`; the normal is that of the face the ray enters; a list of options that is not pairs of integers, or that names an unknown option, gives `[RCERR_UNKNOWN]` (-1); rejecting every type (`RC_REJECT_TYPES` 15) stops the script with an `error` entry (the wiki says runtime error); the wiki's warning about ends far outside the region is not modelled.
- **`llModifyLand(action, brush)`** is record only: one `land_edit` entry with `action`, `brush`, `name` (`level`, `raise`, `lower`, `smooth`, `noise` or `revert`), `size_m` (2, 4 or 8), the object's `position` and `result`: `ok`, `bad_value` (an action not 0 to 5 or a brush not 0 to 2) or `not_owner` (the parcel belongs to somebody else than the object's owner, see `RegionSettings::parcelOwner`). The ground stays flat and `llGround` does not change.
- **`llRequestInventoryData(name)`** works for a landmark (the wiki: other item types are not supported). An inventory item of type `landmark` may have `position`, a list of three numbers (the place as an offset from the region's origin, default `<0,0,0>`, a guess; other types refuse the field). The call returns a query key, writes a `dataserver_request` entry (`function`, `item`, `query`) and raises `dataserver` with the place as vector text. A missing item, a notecard or any other type gives `NULL_KEY` and a `debug` line (`Couldn't find item '<name>'`, or `llRequestInventoryData: only landmarks are supported, '<name>' is not one`; both texts are guesses).
- **Texture reads:** `llGetTextureOffset(face)` and `llGetTextureScale(face)` give a vector (u, v, 0), `llGetTextureRot(face)` the angle in radians, of the script's own prim. `ALL_SIDES` gives face 0 (the wiki says so for `llGetTextureOffset`; a guess for the other two); a face the prim does not have gives `<0,0,0>` or 0.0. The values come from `PRIM_TEXTURE` and from `llOffsetTexture(u, v, face)`, `llScaleTexture(u, v, face)` and `llRotateTexture(radians, face)`, which keep them on the faces now (`ALL_SIDES` sets every face) and still write their `recorded_only` entry as before.
- **Scale factor:** `llGetMinScaleFactor()` is 0.01 over the smallest side of any prim; `llGetMaxScaleFactor()` is the smaller of 64 over the largest side and 54 over the distance of the farthest child from the root (the wiki gives only these limits, so the formulas are a guess). `llScaleByFactor(factor)` multiplies the size of every prim and the offset of every child, writes a `prim` entry (`op` `size`, with `factor`) for each prim and gives TRUE. It gives FALSE and changes nothing when the factor is not above 0, a prim side would leave 0.01 to 64 m, a child would be more than 54 m from the root, the object is physical or it is in keyframed motion. A limit is reached with a tolerance of 1e-5 (relative), so a factor read from the two functions above works (the wiki warns that Second Life may refuse it).
- **Object animations:** `llStartObjectAnimation(name)` starts an animation of the object's inventory (type `animation`; a name that is not there gives a `debug` line `Couldn't find animation '<name>'`, a guess), `llStopObjectAnimation(name)` stops it (the name or the animation's key; a name that is not playing does nothing) and `llGetObjectAnimationNames()` gives the names in the order they were started. A name is started once. Each start and stop writes an `object_animation` entry (`op`, `animation`). Guess: the list is kept per script (the wiki says per prim).
- Not done: a changing ground, the region-edge behaviour of `llCastRay`, hits on single prims of a linkset, the real list of animations of an animated object.

#### Parcel media, the continuous collision event and llReleaseCamera (Script Lab, third gap-list run, steps 10 to 12)
Technical functions; nothing changes on screen. Where the Second Life wiki is unclear the choice is written here and in `tests/tst_lslsteps101112.cpp`.
- **Parcel media (record only, nothing is fetched or played):** the one parcel of the sandbox has one media state: URL, TEXTURE (a key, the null key at the start), TYPE and DESC (strings, empty at the start), SIZE (two integers, 0 and 0), AUTO_ALIGN (0), LOOP_SET, TIME, AGENT and a status. Every call writes a `parcel_media` log entry (`op` `command` or `query`, `allowed`, the `status`: `stop`, `pause`, `play` or `loop`; `stopped` says why a list ended early; `note` when not allowed).
- **`llParcelMediaCommandList(commands)`** takes the wiki's forms: `PARCEL_MEDIA_COMMAND_STOP`, `PAUSE`, `PLAY`, `LOOP` and `UNLOAD` alone; `URL`, `TYPE` and `DESC` followed by a string, `TEXTURE` and `AGENT` by a key, `SIZE` by two integers, `AUTO_ALIGN` by an integer (any non-zero value is 1) and `TIME` and `LOOP_SET` by a float. Guesses: an entry that is no command number, no known command, or a command whose value is missing or of the wrong type (an integer for a float, say) ends the list there, what came before stays done and the entry says why in `stopped`; `UNLOAD` stops the media and clears URL and TEXTURE; `STOP` also sets the time back to 0.
- **`llParcelMediaQuery(query)`** answers in the order asked: `URL`, `DESC` and `TYPE` as strings, `TEXTURE` as a key, `SIZE` as two integers (width, then height) and `AUTO_ALIGN` as an integer. Every other command number is skipped (a guess); an empty list gives `[]`.
- Both functions only work for a script whose owner owns the parcel (the sandbox's parcel belongs to the objects' owner unless C++ code gives it another, see `RegionSettings::parcelOwner`); otherwise nothing happens and a query gives `[]`. The wiki's forced delay of 2 seconds is not modelled, as for every other function.
- **The continuous `collision` event:** a `collision` input without `phase` raises `collision(num_detected)` (always 1), with the same llDetected* data as `collision_start`, in the scripts of the prim hit that have a `collision` handler (the root's when none does). The wiki says it repeats while the objects touch; the sandbox has no motion model, so one input is one call, and a test sends one input for each call. `lsl_check` no longer warns that the sandbox never raises `collision`. A `phase` that is not `"start"` or `"end"` now answers `"phase" must be "start" or "end" (leave it out for the continuous collision).`; a `collision` input without `phase` is no longer an error.
- **`llReleaseCamera(avatar)`** (deprecated in Second Life, the wiki points to `llClearCameraParams`) needs `PERMISSION_CONTROL_CAMERA` (2048), granted by the avatar given; without it a `debug` line `Script trying to control camera but PERMISSION_CONTROL_CAMERA permission not set` (the text is the sandbox's guess) and nothing happens. An avatar that did not grant it, or that is not in the world, is ignored with a `camera` entry (`released` false, a guess). Otherwise the call is recorded (`recorded_only`); the sandbox has no camera to give back. `llTakeCamera` is still not supported.

#### Avatar and camera (Script Lab, second gap list)
Record-only and canned-answer functions; nothing is attached, moved or shown. Where the Second Life wiki is unclear the function gives its documented return type with a harmless value.
- **Attach and detach:** `llAttachToAvatar(point)`, `llAttachToAvatarTemp(point)` and `llDetachFromAvatar()` need `PERMISSION_ATTACH`; without it a `debug` line `Script trying to attach but PERMISSION_ATTACH permission not set` is written and nothing else happens. With it a `recorded_only` entry is written. The object's attachment point does not change, so `llGetAttached` and the `attach` event stay as they are (the `attach` and `detach` inputs still do that).
- **`llGetAttachedList(id)`** gives the keys of the objects of the world that the avatar wears, in the order of the objects, HUD points (31 to 38) left out; `["NOT FOUND"]` for a key that is no avatar of the sandbox (the wiki). **`llGetAgentLanguage(id)`** gives `en` for an avatar of the sandbox and `""` for any other key.
- **Camera reads:** `llGetCameraPos`, `llGetCameraRot`, `llGetCameraAspect` and `llGetCameraFOV` need `PERMISSION_TRACK_CAMERA`; without it a `debug` line `Script trying to track camera but PERMISSION_TRACK_CAMERA permission not set` is written and the answer is `ZERO_VECTOR`, `ZERO_ROTATION` or 0.0. With it the sandbox has no camera and answers its canned view: the owner's place `<128, 128, 20>`, `ZERO_ROTATION`, aspect 1.777778 and field of view 1.047198 radians (60 degrees).
- **Record only:** `llForceMouselook`, `llSetCameraParams`, `llClearCameraParams`, `llSetCameraEyeOffset` and `llSetCameraAtOffset` write `recorded_only` entries (the camera calls do not check `PERMISSION_CONTROL_CAMERA` yet).
- **Teleports:** `llTeleportAgentHome`, `llTeleportAgent` and `llTeleportAgentGlobalCoords` need `PERMISSION_TELEPORT` (`debug` line `Script trying to teleport but PERMISSION_TELEPORT permission not set` without it) and only teleport the owner (any other key: `debug` line `<function>: only the owner can be teleported`); otherwise a `recorded_only` entry is written and nobody moves.
- Not done: attachment state that changes, a camera that moves, the teleport throttle, Experiences, the forced delays.

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
  each 0.001 to 64; default `[0.5, 0.5, 0.5]`; one size for the whole object), `start_parameter` (a whole number, default 0), and (step 9b-4) `inventory`.
  For a single script (`source`) the same three options go at the top of the request; with `objects`, inside each object (at the top they are refused).
  `llGetNumberOfPrims` answers the object's `prims`.

What runs: the types `integer`, `float`, `string`, `key`, `vector`, `rotation` and `list`; global variables
and functions; states with `state_entry` and `state_exit`; `if`, `while`, `do`, `for`, `jump`, `return`;
the events `state_entry`, `state_exit`, `timer`, `listen`, `touch_start`, `touch`, `touch_end`, `run_time_permissions`, `attach`,
`link_message`, `linkset_data`, `on_rez`, `changed`, `dataserver`, `object_rez`, `control`, `sensor`, `no_sensor`, `http_response` and `transaction_result`; 234 built-in functions (chat, listen, timer, time, owner and object keys,
permissions, animations, dialogs, position, link messages, Linkset Data, list, string and maths functions, rotation
maths, Base64, MD5 and SHA-1, URL escaping, object-state getters and setters, simulated prim state, a simulated inventory with notecards and rezzing, sitting and pose balls, controls, sensors, HTTP requests with canned replies, reads and state of the object itself (`llGetObjectDetails`, `llDie`, `llSetScriptState` and others), and functions that are only recorded). All 526
function names and signatures, 43 events and 1 082 constants are known: those of the viewer's LSL keyword list
(`src/scriptlab/lslbuiltins_data.inc`, made by `tools/gen_lsl_builtins.py`) plus what that list lacks (Linkset Data,
`llReplaceSubString`, `llListSortStrided`, `llName2Key`, `llSetLinkGLTFOverrides` and others; the events
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
