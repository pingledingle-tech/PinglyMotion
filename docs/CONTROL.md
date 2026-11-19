# Remote control (for AI tools and other programs)

Pingmator can be controlled by other programs on the same computer: pose joints, change
frames, take a picture of the 3D view, save a BVH. It is an open interface: plain HTTP with
JSON, and an MCP server on top for AI tools that speak MCP (Model Context Protocol).
Nothing here is tied to one AI.

## Turn it on

- Menu **Options > Allow remote control (AI and other programs)**, or
- start Pingmator with `pingmator --control`.

It is off every time Pingmator starts, unless you use `--control`. While it is on, the window
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
- Every request needs a secret key. Pingmator makes a new key each time it starts and writes it,
  with the address, to `control.json` (only you can read it):
  - Windows: `%LOCALAPPDATA%\Pingmator\control.json`
  - macOS: `~/Library/Application Support/Pingmator/control.json`
  - Linux: `~/.local/share/Pingmator/control.json`
  - Set `PINGLYMOTION_CONTROL_FILE` to use another place.
- The file is written in one step: the key goes into a temporary file that only you can read from
  the moment it is made, and that file replaces `control.json` only when everything was written.
  A file left behind by a crash is removed first. If the file cannot be written safely (for example
  a full disk or permissions that cannot be set), remote control does not start and Pingmator says why.
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

A client can name itself with the header `X-Pingmator-Client: <name>` (printable text, up to 40
characters); the status bar shows it. `POST /api/heartbeat` (with the key, empty body) tells Pingmator
the client is still there without doing anything. The MCP server sends the AI tool's name from MCP
`initialize` and a heartbeat every 10 seconds.

### Checking that it is Pingmator

`control.json` stays behind if Pingmator crashes, and another program could then take the port.
So a client should check the program before it sends the key: `GET /api/hello?nonce=<64 random hex digits>`
(no key needed) answers `{"ok": true, "proof": "<hex>"}`, where proof is
HMAC-SHA256(key, `"pingmator-hello:" + nonce`). Compare it with your own result and send nothing if it
differs. The MCP server does this before every request.

Commands: `state`, `joints`, `get_pose`, `set_pose`, `set_position`, `delete_key`, `keys`,
`set_frame`, `set_frame_count`, `set_fps`, `play`, `stop`, `new_animation`, `open_file`,
`save_file`, `screenshot`, `set_figure`, `set_camera`.

### Files

`open_file` and `save_file` only work in one folder: `Pingmator` in the Documents folder of the user
who runs Pingmator (for example `C:\Users\<name>\Documents\Pingmator` on Windows). It is made when
it is first needed, and `state` shows it as `folder`. A plain file name such as `pose.bvh` means a file
in that folder. Paths outside it, also through `..` or links, are refused.

- `open_file` opens `.dingle`, `.bvh` and `.avm` files (the ending in any case). A file that cannot be
  read gives an error that says why.
- `save_file` never replaces another file. Saving again to the file that is open is fine.
- `new_animation` and `open_file` refuse while there are unsaved changes; save first.
- `PINGLYMOTION_CONTROL_FOLDER` sets another folder (used by the tests).

## Set up an AI assistant

`pingmator_mcp.py` is an MCP server: it lets an AI assistant such as the Claude app (Claude Desktop)
or Claude Code use remote control. It needs Python 3.8 or newer and nothing else; the assistant
starts it by itself.

The Windows zip has it in the `mcp` folder next to `pingmator.exe`. Register that copy: unpacking a
new zip over the old folder then updates the program and the MCP server together. In the source
tree it is `tools/mcp/pingmator_mcp.py`.

The examples below use `C:\Pingmator`. Replace it with the folder where you unpacked Pingmator.

### 1. Install Python

Download it from https://www.python.org. On Windows, tick **Add python.exe to PATH** in the
installer. Check in a command window: `python --version`.

### 2a. Claude app (Claude Desktop)

1. Open **Settings > Developer > Edit Config**. This opens `claude_desktop_config.json`.
2. Add `pingmator` under `mcpServers`. If the file is empty or has only `{}`, use all of this:

   ```json
   {
     "mcpServers": {
       "pingmator": {
         "command": "python",
         "args": ["C:\\Pingmator\\mcp\\pingmator_mcp.py"]
       }
     }
   }
   ```

   If `mcpServers` already has other servers, add only the `"pingmator": { ... }` part, with a
   comma between the entries. Backslashes in the path must be written twice.
3. Save the file and quit the app completely (also from the system tray), then start it again.

If the app cannot find `python`, use the full path to `python.exe` as `command`. `where python` in a
command window shows it (again with double backslashes in the file).

### 2b. Claude Code

In a command window:

```
claude mcp add pingmator -- python "C:\Pingmator\mcp\pingmator_mcp.py"
```

### 2c. Other assistants

Any tool that takes an `mcpServers` list works the same way. On Linux and macOS use `python3` and
the path to `pingmator_mcp.py`.

### 3. Use it

Start Pingmator and turn on **Options > Allow remote control** (or start it with `--control`).
Then ask the assistant to pose the figure. The tools are listed even when Pingmator is not running,
and say what to do if one is called then. Restart the assistant after updating Pingmator: a new MCP
server refuses to talk to an old Pingmator.

### 4. Updating

Unpack each new zip **over the old folder**. Then the program and the MCP server you registered
are updated together.

If you unpack a new zip into another folder, the assistant still starts the old MCP server from the
folder you registered. Pingmator then works, but newer tools are missing or answer "Unknown tool".
Fix it in one of these ways:

- copy the new `mcp` folder over the old one, or
- register the new folder instead. Claude Code:
  `claude mcp remove pingmator`, then `claude mcp add` with the new path (see 2b).
  Claude app: change the path in `claude_desktop_config.json` and restart the app.

Claude Code registers the server for the folder you were in when you ran `claude mcp add`. Start
`claude` from that same folder (for example `cd C:\Pingmator`, then `claude`), or add `--scope user`
to `claude mcp add` so it works from every folder.

## Good to know when posing

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
