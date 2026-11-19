# Live link to Aurora

Pingmator can show the pose you are working on, on your own avatar in the
Aurora viewer, while you work. Aurora puts it on your avatar with its local
poser: only you see it, nothing is uploaded and it costs nothing.

The link uses Aurora's **LiveMesh**, the same local link the Blender add-on
uses, and Aurora's own LiveMesh key. At the moment the live poses need Aurora
built from
[pingledingle-tech/Aurora-Viewer-Fork](https://github.com/pingledingle-tech/Aurora-Viewer-Fork)
(branch `feat/livemesh-pose-on-main`, pull request #2 there).

## Use it

1. In Aurora (logged in, or in demo mode): **Build > LiveMesh > Enable preview**.
2. In Pingmator: **Options > Live link to Aurora**, or click the dot in the
   status bar. The title bar says "waiting for Aurora", then "live in Aurora"
   once connected. Aurora's chat says Pingmator is connected.
3. Turn a joint or move the frame slider: your avatar takes the pose.
   Playing the animation plays it on your avatar.

The link is off every time Pingmator starts. If Aurora is closed or its
preview is off, Pingmator keeps trying every 2 seconds. The Blender add-on and
Pingmator cannot use LiveMesh at the same time (one client at a time).

### Messages in the status bar

- *Aurora's LiveMesh preview is not on.* There is no token file (see below):
  Aurora is not running, or its preview is off. Turn it on in Aurora.
- *Aurora refused the key.* The token file holds another key than the running
  preview, for example one left by an Aurora that crashed. Turn the preview off
  and on in Aurora.
- *This Aurora version cannot show live poses.* Aurora closed the connection at
  Pingmator's greeting, or its answer lists no pose feature: it is an Aurora
  without the pose link. Use the build named above.
- *Aurora's LiveMesh is on, but it cannot show live poses right now (...).*
  The preview runs without the avatar skeleton; Aurora's reason is in the
  brackets.
- *The program on the LiveMesh port did not prove that it is Aurora.* /
  *... cannot prove that it is Aurora.* Some other program answered on the
  port, or it does not know the key. Pingmator sends no pose to it.

## How it works

Each time the LiveMesh preview starts, Aurora draws a new random key (64 hex
characters, 32 bytes) and writes it to `blender-live/<port>.token` in its data
folder, readable only by your user; it removes the file when the preview stops:

- Windows: `%LOCALAPPDATA%\Aurora\AuroraViewer\data\blender-live\17655.token`
- Linux: `~/.local/share/auroraviewer/blender-live/17655.token`
  (`$XDG_DATA_HOME` when it is set)
- macOS: `~/Library/Application Support/org.Aurora.AuroraViewer/blender-live/17655.token`

Pingmator looks for the token file of port 17655 first. If that one is missing
but another `<port>.token` is there (a port changed in Aurora's LiveMesh
window), it uses the newest of them and connects to that port. No token file
means the preview is off: Pingmator does not connect and says so.

Pingmator connects to `127.0.0.1` on that port and sends the 8 bytes
`AURPOS1\n`, the greeting of a pose tool (the Blender add-on sends `AURBLD2\n`
and the key itself; Pingmator never sends the key). An Aurora without the pose
link closes the connection at once. Aurora answers with one line of JSON
(`hello`); it must list the features `auth`, `auth-mutual` and `pose-bvh` or
`pose`, and carry a random `challenge` (32 bytes, hex).

After that, every message from Pingmator is a 4-byte little-endian length
followed by that many bytes of JSON. Aurora answers each one with one line of
JSON, `{"ok":true}` or `{"ok":false,"error":"..."}`.

1. `{"type":"auth","client":"Pingmator x.y","response":"<hex>","nonce":"<hex>"}`:
   `response` is HMAC-SHA256 of the challenge bytes with the key; `nonce` is
   16 random bytes. A wrong response gets `"error":"unauthorized"` and Aurora
   closes the connection. A right one gets
   `{"ok":true,"server_proof":"<hex>"}`, the HMAC-SHA256 of the text
   `aurora-hello:` followed by the nonce bytes, with the same key. Pingmator
   checks it (constant-time) and sends no pose before it matches: so it knows
   the program on the port could read the token file, as Aurora does.
2. When Aurora lists the feature `pose`:
   `{"type":"pose","rotations":{"lForeArm":[x,y,z,w],...},"positions":{"hip":[x,y,z]}}`
   whenever the pose changes: one quaternion per joint in Second Life's avatar
   axes (X forward, Y left, Z up), relative to the rest pose; the hip offset
   from frame 0 in metres. Aurora accepts Pingmator's joint names (it knows the
   old BVH names as aliases).
   Otherwise `{"type":"pose-bvh","bvh":"..."}` whenever the pose changes (checked every
   50 ms; the next pose waits for Aurora's answer to the previous one). The
   BVH has 3 frames: the first is neutral (no rotation), then the pose twice,
   so every joint in the pose counts as a change. Errors are shown in
   Pingmator's status bar.
3. A length of 0 is a heartbeat, sent after 3 seconds without a message
   (Aurora closes a link that is quiet for 10 seconds).

Aurora's side: `crates/aurora-viewer/src/blender_live/transport.rs`,
`pose.rs` and `token.rs` in the fork, and its `docs/BLENDER_LIVE.md`
("Wire protocol", "Live poses").

Code: `src/auroralink.*`, `BVH::bvhPoseText` in `src/bvh.cpp`.
Tests: `tests/tst_auroralink.cpp`, with a stand-in for Aurora's LiveMesh and
its token file. The environment variable `PINGMATOR_AURORA_TOKEN_DIR` sets the
token folder instead of Aurora's data folder; the tests use it, and it can
point Pingmator at a test server by hand.

## Planned

- Play, pause and frame position following each other both ways.
- Your avatar's shape from Aurora, so the figure in Pingmator matches it.
