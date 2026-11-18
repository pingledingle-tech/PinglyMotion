# Live link to Aurora

Pingmator can show the pose you are working on, on your own avatar in the
Aurora viewer, while you work. Aurora puts it on your avatar with its local
poser: only you see it, nothing is uploaded and it costs nothing.

The link uses Aurora's **LiveMesh**, the same local link the Blender add-on
uses. At the moment the live poses need Aurora built from
[pingledingle-tech/Aurora-Viewer-Fork](https://github.com/pingledingle-tech/Aurora-Viewer-Fork)
(branch `feat/livemesh-pose`).

## Use it

1. In Aurora: open **LiveMesh** and click **Enable preview** (Connection tab).
2. In Pingmator: **Options > Live link to Aurora**. The title bar says
   "waiting for Aurora", then "live in Aurora" once connected. Aurora's chat
   says Pingmator is connected.
3. Turn a joint or move the frame slider: your avatar takes the pose.
   Playing the animation plays it on your avatar.

The link is off every time Pingmator starts. If Aurora is closed or LiveMesh
is off, Pingmator keeps trying every 2 seconds. A Blender add-on and
Pingmator cannot use LiveMesh at the same time (one client at a time).

## How it works

Pingmator connects to `127.0.0.1`, port 17655, and sends the 8 bytes
`AURBLD1\n`. Aurora answers with one line of JSON (`hello`); it must list the
features `auth` and `pose-bvh` and carry a random `challenge` (hex).

After that, every message from Pingmator is a 4-byte little-endian length
followed by that many bytes of JSON. Aurora answers each one with one line of
JSON, `{"ok":true}` or `{"ok":false,"error":"..."}`.

1. `{"type":"auth","client":"Pingmator x.y","response":"<hex>"}` where
   `response` is HMAC-SHA256 of the challenge bytes with the session key.
   Aurora writes a new key to `livemesh.json` each time LiveMesh starts, in a
   folder only your user can read:
   - Windows: `%LOCALAPPDATA%\Aurora\AuroraViewer\data\livemesh.json`
   - Linux: `~/.local/share/auroraviewer/livemesh.json`
   - macOS: `~/Library/Application Support/org.Aurora.AuroraViewer/livemesh.json`

   The key itself never goes over the connection. A wrong answer closes it.
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

Code: `src/auroralink.*`, `BVH::bvhPoseText` in `src/bvh.cpp`.
Tests: `tests/tst_auroralink.cpp` (with a stand-in for Aurora's LiveMesh).

## Planned

- Play, pause and frame position following each other both ways.
- Your avatar's shape from Aurora, so the figure in Pingmator matches it.
