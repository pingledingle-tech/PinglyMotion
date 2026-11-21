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
requests get `413`); at most 8 connections at a time (more get `503`).

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
`export_anim`, `find_reference`, `load_reference`. `screenshot` has options for several views, frames and labels (below).

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
  answer has `loop_in` and `loop_out`.
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
  and SHA-256 against the index. The answer is the `state` plus `reference` with the clip's `licence` and
  `credit`. The library is read-only; `save_file` with a new name keeps the edited animation.
- Clips downloaded with `tools/fetch_reference.py` (its `downloaded_index.json`) are listed too, with
  `downloaded: true`. They open only when their `skeleton` is `sl-classic`: the raw CMU, Mesh2Motion and Quaternius
  downloads are listed with `loadable: false` and the reason, and `tools/convert_reference.py` adds converted copies
  (ids starting with `sl/`) that open (layout in `docs/REFERENCE_LIBRARY_SCHEMA.md`).
- Folders: `library/` next to the program, or `PINGLYMOTION_LIBRARY_FOLDER`; the download folder is
  `PinglyMotion-reference` in the user's home folder (the script's default), or `PINGLYMOTION_REFERENCE_FOLDER`.

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
Animation), with the same writer and the same defaults as File > Export as Second Life .anim. It never
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

## Set up an AI assistant

`pinglymotion_mcp.py` is an MCP server: it lets an AI assistant such as the Claude app (Claude Desktop)
or Claude Code use remote control. It needs Python 3.8 or newer and nothing else; the assistant
starts it by itself.

The Windows zip has it in the `mcp` folder next to `PinglyMotion.exe`. Register that copy: unpacking a
new zip over the old folder then updates the program and the MCP server together. In the source
tree it is `tools/mcp/pinglymotion_mcp.py`.

The examples below use `C:\PinglyMotion`. Replace it with the folder where you unpacked PinglyMotion.

### Quick way: the Claude Desktop extension

The Windows zip (and each release) has `PinglyMotion.mcpb`, an extension for the Claude app, and a
short `START HERE - Claude.txt` with the same steps. No file editing and no typed paths:

1. Unzip the download first (do not open the `.mcpb` from inside the zip). Nothing else has to be
   installed: the extension runs on the Node that the Claude app brings, so Python is not needed.
   If it does not start, say so in issue #7.
2. Double-click `PinglyMotion.mcpb`. The Claude app opens an install window: click **Install**. If
   double-click does nothing, start the Claude app, open **Settings > Extensions** and drag the file in.
3. Start PinglyMotion and turn on **Options > Allow remote control (AI and other programs)** (it is off
   at every start).
4. In a new chat in the Claude app, ask: "Raise the avatar's right arm and show me a picture." If
   Claude asks for permission to use PinglyMotion, allow it.

Update: install the `PinglyMotion.mcpb` from the new download the same way. The extension holds no
settings and no key; it is the same MCP server as below, with the same protections (it talks only to
`127.0.0.1` and checks that it really is PinglyMotion before it sends the key). The extension is
built from this repository by `tools/mcp/build_mcpb.py` and is not signed. Claude Desktop does not
ship Python, so the extension uses the Node version of the server (`tools/mcp/pinglymotion_mcp.js`).
The manual way below still uses the Python server for now (issue #7).

The steps below are the manual way, also for Claude Code.

### 1. Install Python

Download it from https://www.python.org. On Windows, tick **Add python.exe to PATH** in the
installer. Check in a command window: `python --version`.

### 2a. Claude app (Claude Desktop)

1. Open **Settings > Developer > Edit Config**. This opens `claude_desktop_config.json`.
2. Add `pinglymotion` under `mcpServers`. If the file is empty or has only `{}`, use all of this:

   ```json
   {
     "mcpServers": {
       "pinglymotion": {
         "command": "python",
         "args": ["C:\\PinglyMotion\\mcp\\pinglymotion_mcp.py"]
       }
     }
   }
   ```

   If `mcpServers` already has other servers, add only the `"pinglymotion": { ... }` part, with a
   comma between the entries. Backslashes in the path must be written twice.
3. Save the file and quit the app completely (also from the system tray), then start it again.

If the app cannot find `python`, use the full path to `python.exe` as `command`. `where python` in a
command window shows it (again with double backslashes in the file).

### 2b. Claude Code

In a command window:

```
claude mcp add pinglymotion -- python "C:\PinglyMotion\mcp\pinglymotion_mcp.py"
```

### 2c. Other assistants

Any tool that takes an `mcpServers` list works the same way. On Linux and macOS use `python3` and
the path to `pinglymotion_mcp.py`.

### 3. Use it

Start PinglyMotion and turn on **Options > Allow remote control** (or start it with `--control`).
Then ask the assistant to pose the figure. The tools are listed even when PinglyMotion is not running,
and say what to do if one is called then. Restart the assistant after updating PinglyMotion: a new MCP
server refuses to talk to an old PinglyMotion.

### 4. Updating

Unpack each new zip **over the old folder**. Then the program and the MCP server you registered
are updated together.

If you unpack a new zip into another folder, the assistant still starts the old MCP server from the
folder you registered. PinglyMotion then works, but newer tools are missing or answer "Unknown tool".
Fix it in one of these ways:

- copy the new `mcp` folder over the old one, or
- register the new folder instead. Claude Code:
  `claude mcp remove pinglymotion`, then `claude mcp add` with the new path (see 2b).
  Claude app: change the path in `claude_desktop_config.json` and restart the app.

Claude Code registers the server for the folder you were in when you ran `claude mcp add`. Start
`claude` from that same folder (for example `cd C:\PinglyMotion`, then `claude`), or add `--scope user`
to `claude mcp add` so it works from every folder.

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
- Mirroring (Tools > Mirror) applies to commands too: with it on, posing a left joint also poses the right one.
