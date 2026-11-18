# Why the Aurora link uses LiveMesh

Background for the [live link to Aurora](AURORA_LINK.md) (#78, research 15 November 2026). Before choosing how Pingmator sends poses to Aurora, we looked at how other 3D tools stream poses, and at what the protocol has to fix about 3D conventions.

## Existing live links

| | Transport | Data | Open | Security | For Pingmator |
| --- | --- | --- | --- | --- | --- |
| **Aurora LiveMesh** (Blender link) | TCP 127.0.0.1:17655, magic `AURBLD1`, length-prefixed JSON, size limits | Mesh snapshots; Aurora sends the SL joints and rest axes in `hello`; features are negotiated | Documented in Aurora (`docs/BLENDER_LIVE.md`) | Loopback, opt-in, magic and limits; no key | Already exists – reuse it |
| **Unreal Live Link** | Unreal Message Bus over UDP, LAN | A "subject" (skeleton) described once, then light frames (transforms, curves, time) | Needs Unreal's own libraries | Trusted studio network | Not reusable, but a good model |
| **VMC protocol** (VTuber tools) | OSC over UDP | Per bone: name, position, quaternion; blend shapes | Open, many apps | None | Only ~55 humanoid bones, not all 133 Bento bones. Possible later as an extra input |
| **Rokoko Studio Live** | JSON over UDP | Versioned JSON: body, fingers, face | Documented | Key only for the command API | Pattern: versioned JSON |
| **Omniverse / USD** | Server, USD layers | Whole scene | USD is open | Login | Far too heavy |

What we took from this:

- There is no single open standard for live poses. Unreal Live Link is tied to Unreal; VMC is open but has too few bones and no security.
- The common pattern is **describe the skeleton once, then send light frames** (a quaternion per bone, positions where needed).
- Versioned JSON with feature negotiation is normal.
- Security is usually weak (UDP, no authentication). Our choice – loopback TCP, magic, size limits and a key – is stricter than all of them.

## 3D conventions the protocol fixes

Tools disagree on axes, rotations and units:

| Tool | Up | Handedness |
| --- | --- | --- |
| Blender, 3ds Max | Z | right |
| Maya, Houdini | Y | right |
| Unity, Cinema 4D | Y | left |
| Unreal | Z | left |
| SL avatar | Z | right (X forward, Y left) |
| SL BVH upload | Y | Z forward, X sideways |

So the link uses **one** frame, the SL avatar frame (X forward, Y left, Z up, right-handed, metres), and every client converts on its own side.

- **Rotations:** BVH uses Euler angles with a channel order per joint, which is easy to get wrong. Live links send quaternions; the `pose` message uses local quaternions `(x, y, z, w)` relative to SL's rest pose.
- **Rest pose:** joints are named by their SL names (`mPelvis`, `mHandLeft`, …); Aurora already sends the joint list and rest axes in `hello`.
- **Units:** metres.
- **Which joints:** a frame lists only the joints it sets; the others stay as they are, like SL animations.

## Decision

- Reuse Aurora's LiveMesh protocol instead of a separate Pingmator link. The Blender add-on is not changed.
- New features in LiveMesh: `auth` (key file and HMAC challenge, required for poses), `pose-bvh` (BVH text, the first step) and `pose` (quaternions). How they work is described in [AURORA_LINK.md](AURORA_LINK.md).
- Pingmator's client is written from Aurora's documentation, not copied (Aurora is GPLv3, Pingmator GPLv2).
- A local pose on the user's own avatar fits Second Life's Third-Party Viewer Policy; reading poses back must stay limited to the user's own avatar.

## Sources

- Unreal Live Link: https://dev.epicgames.com/documentation/en-us/unreal-engine/live-link-in-unreal-engine
- VMC protocol: https://protocol.vmc.info/english.html
- Rokoko custom streaming: https://support.rokoko.com/hc/en-us/articles/4410416376977-Custom-Streaming
- Aurora LiveMesh: `docs/BLENDER_LIVE.md` in Aurora-Viewer/Aurora-Viewer
