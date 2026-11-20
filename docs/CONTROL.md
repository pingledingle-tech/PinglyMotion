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
`set_frame`, `set_frame_count`, `set_fps`, `play`, `stop`, `new_animation`, `open_file`,
`save_file`, `screenshot`, `set_figure`, `set_camera`, `describe_pose`, `describe_motion`.

### Files

`open_file` and `save_file` only work in one folder: `PinglyMotion` in the Documents folder of the user
who runs PinglyMotion (for example `C:\Users\<name>\Documents\PinglyMotion` on Windows). It is made when
it is first needed, and `state` shows it as `folder`. A plain file name such as `pose.bvh` means a file
in that folder. Paths outside it, also through `..` or links, are refused.

- `open_file` opens `.dingle`, `.bvh` and `.avm` files (the ending in any case). A file that cannot be
  read gives an error that says why, at once: while remote control opens a file the program shows
  no message box (it would block the answer until someone clicks it).
- `save_file` never replaces another file. Saving again to the file that is open is fine.
- `new_animation` and `open_file` refuse while there are unsaved changes; save first.
- `PINGLYMOTION_CONTROL_FOLDER` sets another folder (used by the tests).

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
- `screenshot` draws the current frame, 4:3, whatever the size of the window.
- `set_camera` turns the camera around the figure (`view`: front, back, left, right, three_quarter, or an
  `angle`; also `height` and `distance`; `reset: true` for the start view). Left and right are the figure's
  own sides. It never changes the animation, so use it instead of turning the hip to see the side.
- `set_figure` shows the female or male body. It only changes the display; the saved BVH is the same.
- Mirroring (Tools > Mirror) applies to commands too: with it on, posing a left joint also poses the right one.
