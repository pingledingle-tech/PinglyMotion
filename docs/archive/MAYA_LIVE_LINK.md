Archived 22 November 2026: research finished; direct Maya support is out of scope (DECISIONS.md).

# Maya and the Aurora live link

Research note for [#62](https://github.com/pingledingle-tech/PinglyMotion/issues/62),
written 19 November 2026. **Documentation only. Nothing is built, nothing is
planned.** Direct Maya support stays out of scope, as decided (see
[DECISIONS.md](../DECISIONS.md), 18 November 2026, and section 7 of
`docs/archive/AURORA_STUDIO.md` on pull request #58).

Background reading: [LIVE_LINK_RESEARCH.md](LIVE_LINK_RESEARCH.md) (why we use
Aurora's link) and [AURORA_LINK.md](../AURORA_LINK.md) (how PinglyMotion talks to
Aurora today).

## 1. The question

Autodesk Maya is a professional 3D program that many animators use. Aurora
Viewer has a "live link" called **LiveMesh**: a program on the same computer
connects to Aurora and Aurora shows the result on your own avatar, at once,
without uploading anything. Blender uses it for meshes; PinglyMotion uses it
for poses.

The question: **could a small script inside Maya connect to Aurora the same
way and send a pose, so an animator sees the Maya pose on their avatar?** And
if that is too much, is there a simpler way to get Maya animation into
PinglyMotion?

## 2. How Aurora's live link works today

There are now **two versions** of the link, and they no longer match.

### 2a. Aurora upstream (the official program)

Read from Aurora's own description, `docs/BLENDER_LIVE.md` on the `main`
branch of `Aurora-Viewer/Aurora-Viewer`, **commit `cc9c980`** (full hash
`cc9c98097e3bd3078d830ddd3e1e7f0a2b4e97e2`, 19 November 2026). In plain words:

- **Where it listens.** Aurora waits for a connection on the computer itself
  only (address 127.0.0.1), port 17655 by default. Nobody on the network can
  reach it. The user turns it on in Aurora (**Build → LiveMesh → Enable
  preview**); it is off every time Aurora starts.
- **Key file.** Each time the preview starts, Aurora picks a new random key
  (64 characters) and writes it to a small file, `blender-live/<port>.token`,
  in Aurora's data folder. Only the user's own account can read that folder
  on Linux and macOS. The file is deleted when the preview stops. A client
  proves it runs as the same user by reading that file.
- **Greeting.** The client sends the 8 characters `AURBLD2` plus a line break,
  then the key. A wrong key closes the connection. (An older client that sends
  `AURBLD1` is told "protocol 2" and must update.) Aurora answers with one line
  of JSON (a simple text format) that lists: the protocol number, the Second
  Life joints it supports, their rest axes, the **features** it offers, and
  size limits (64 MiB per message, 256 MiB after unpacking).
- **Message framing.** After the greeting, every message from the client is a
  4-byte length followed by that many bytes of JSON. A length of zero is a
  "still here" heartbeat. Aurora answers each message with one line of JSON:
  "ok" (with a revision number) or an error.
- **Features** are switched on only when both sides support them:
  compressed messages, packed geometry, non-rigged objects, linked objects,
  sending the avatar's shape to Blender, and preparing levels of detail for
  upload.
- **What it carries.** Meshes only. Aurora's document says plainly that
  Blender poses and animation playback are **not** streamed: Aurora keeps
  playing its own animations on the previewed mesh.

### 2b. Our fork (what PinglyMotion uses)

PinglyMotion's link is described in [AURORA_LINK.md](../AURORA_LINK.md), which is
the up-to-date description; read it for the details. In short: for live poses
it needs Aurora built from our fork (`pingledingle-tech/Aurora-Viewer-Fork`,
branch `feat/livemesh-pose-on-main`). The fork adds pose features that upstream
does not have:

- `auth` and `auth-mutual`: the client uses Aurora's own key from
  `blender-live/<port>.token` (the same token file as upstream). The key itself
  is never sent: Aurora sends a random challenge and the client answers with
  an HMAC-SHA256 signature, and Aurora proves the same back.
- `pose-bvh`: a pose sent as a small BVH text.
- `pose`: a pose sent as one rotation (quaternion) per joint, in Second Life's
  avatar axes, relative to the rest pose, plus the hip position in metres.

PinglyMotion greets with the 8 bytes `AURPOS1` plus a line break, the greeting
of a pose tool; an Aurora without the pose link closes the connection at once.
It does not use `AURBLD2` and never sends the key. So PinglyMotion's link
needs the fork and does not work with the official Aurora, which has no pose
feature. (Our `LIVE_LINK_RESEARCH.md` table still says upstream has "no key";
that was true on 15 November and is now out of date.)

## 3. Could a Maya script be a client and send a pose?

**Technically yes, against our fork. Not against upstream**, because upstream
has no pose feature at all.

Maya runs Python scripts inside the program, and its Python includes the
standard parts a client needs: network connections (`socket`), JSON, and the
HMAC signature (`hmac`, `hashlib`). No extra installation would be needed.
Maya can also read joint data through its script commands (`maya.cmds`) and
its programming interface (OpenMaya, for example the joint class
`MFnIkJoint`). So the plumbing is not the hard part.

What such a script would need:

- **The key file.** It must find and read the key file for the version of
  Aurora it talks to (`blender-live/<port>.token`), in the right folder for
  Windows, Linux or macOS, and answer the greeting the right way. A pose tool
  (our fork, `docs/AURORA_LINK.md`) and a mesh tool (upstream, section 2a)
  greet differently, so the script would have to pick one.
- **Stay responsive.** Maya only allows its scene to be read from its main
  thread. The script would read the pose on the main thread (for example when
  the time slider changes) and do the network work in a background thread, so
  Maya does not freeze. It must send a heartbeat within 10 seconds and wait for
  Aurora's answer before sending the next pose.
- **Axes and units.** Maya is "Y up" and works in centimetres by default. The
  link uses Second Life's avatar frame: X forward, Y left, Z up, in metres.
  Every rotation and the hip position must be converted. Getting one axis
  wrong turns the avatar on its side or mirrors it.
- **Second Life joint names.** Aurora knows joints by their Second Life names
  (`mPelvis`, `mHandLeft`, …; our fork also accepts the old BVH names). The
  Maya rig must use these names, or the script needs a name table for the rig
  (a "retargeting" table). A Maya rig with other bones, or fewer, would need
  the same retargeting work as [#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16).
- **Rest pose.** The `pose` message is relative to Second Life's rest pose
  (arms straight out). A Maya joint's rotation is built from several parts
  (its own rotation, a fixed "joint orient", a rotation axis and a rotation
  order). If the Maya rig stands in an A-pose, or its joints are oriented
  differently from Second Life's, the script must first remove the Maya rest
  pose and then express the difference in Second Life's axes. Aurora's
  greeting includes the rest axes, which helps, but the Maya rig's own rest
  pose still has to be known.
- **One client at a time.** LiveMesh takes one client. Maya and PinglyMotion
  (or Blender) could not be connected at once.

In short: a few hundred lines of Python, but the difficult part is the same as
for any import, namely axes, rest pose and joint names, plus keeping up with a
protocol that is not settled.

## 4. The simpler file route

Instead of a live connection, Maya animation can reach PinglyMotion as a file.

- **BVH.** Maya has no built-in BVH export; free and paid scripts exist.
  PinglyMotion already opens BVH files. Today they must use Second Life's
  skeleton and joint names; BVH from any other skeleton needs retargeting
  ([#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16)).
- **glTF.** Maya has no built-in glTF export either. Plug-ins exist: the free,
  open-source Maya2glTF, and commercial exporters (for example from ProtoTech
  and SimLab, some on the Autodesk Marketplace). PinglyMotion's glTF/GLB import
  is planned ([#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12)),
  after retargeting.
- **FBX through Blender.** Maya exports FBX natively. PinglyMotion will not read
  FBX (decided 18 November 2026), but Blender opens FBX for free and exports BVH
  or glTF. So a working route today is: Maya → FBX → Blender → BVH with Second
  Life joint names → PinglyMotion.

The file route needs the same care with axes, rest pose and joint names, but it
is done once per file, inside tools we already plan to test, and it does not
depend on Aurora's protocol.

## 5. Recommendation

- **Direct Maya support stays out of scope, as decided.** No Maya script, no
  Maya plug-in.
- **Use the file route.** For Maya users, document the route Maya → FBX →
  Blender → BVH (or glTF) → PinglyMotion as part of the Blender docs
  ([#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13)), once
  retargeting (#16) and glTF import (#12) work.
- **A live Maya link is possible later**, as a separate client of the same
  protocol, but only after the `pose` feature exists in upstream Aurora and the
  greeting and key file are settled. Until then any Maya script would depend on
  our fork and break whenever upstream changes.
- **More urgent than Maya:** our own link and upstream have drifted apart
  (section 2b). That affects PinglyMotion users now and should get its own
  issue.

## 6. Open questions

1. Will the `pose` and `auth` features be offered to Aurora upstream, or stay
   in our fork? Aurora's document says pose streaming is "outside the live
   bridge's current scope".
2. Should our fork and upstream keep two greetings (`AURPOS1` for pose tools,
   `AURBLD2` for mesh tools), or should they be merged?
3. Is there real demand from Maya users? Without it, the file route is enough.
4. Who would test a Maya client? It needs a Maya licence and a Windows machine
   with Aurora, which the project does not have.
5. If a Maya client is ever written: it must be written from Aurora's
   documentation, not copied from Aurora's Blender add-on (GPLv3), as for our
   own client.

## Sources

- Aurora LiveMesh protocol: `docs/BLENDER_LIVE.md` in
  [Aurora-Viewer/Aurora-Viewer](https://github.com/Aurora-Viewer/Aurora-Viewer),
  commit `cc9c98097e3bd3078d830ddd3e1e7f0a2b4e97e2`, section "Wire protocol v1"
  and "Hand/foot IK" (poses not streamed).
- PinglyMotion: [AURORA_LINK.md](../AURORA_LINK.md),
  [LIVE_LINK_RESEARCH.md](LIVE_LINK_RESEARCH.md), [DECISIONS.md](../DECISIONS.md);
  `docs/archive/AURORA_STUDIO.md` section 7 (pull request #58).
- Python in Maya and threads (scene access from the main thread only):
  [Python in Maya](https://help.autodesk.com/cloudhelp/2020/ENU/Maya-Scripting/files/GUID-C0F27A50-3DD6-454C-A4D1-9E3C44B3C990.htm),
  [Using Python (Maya 2025)](https://help.autodesk.com/cloudhelp/2025/ENU/Maya-Scripting/files/GUID-55B63946-CDC9-42E5-9B6E-45EE45CFC7FC.htm),
  [Python and threading](https://download.autodesk.com/us/maya/2011help/files/WS1a9193826455f5ff-7f10494411f03a24f24-7e6e.htm).
- Maya `commandPort` (Maya's own port for remote commands, the reverse
  direction): [commandPort](https://help.autodesk.com/cloudhelp/2022/ENU/Maya-Tech-Docs/CommandsPython/commandPort.html).
- Maya joint data:
  [MFnIkJoint class](https://help.autodesk.com/cloudhelp/2018/ENU/Maya-SDK/cpp_ref/class_m_fn_ik_joint.html),
  [joint command](https://help.autodesk.com/cloudhelp/ENU/MayaCRE-Tech-Docs/Commands/joint.html).
- Python standard library sockets:
  [socket](https://docs.python.org/3.10/library/socket.html).
- glTF export plug-ins for Maya:
  [Maya2glTF](https://awesome.ecosyste.ms/projects/github.com%2Fiimachines%2FMaya2glTF),
  [maya-glTF](https://github.com/oxpi/maya-glTF),
  [ProtoTech glTF Exporter](https://prototechsolutions.com/3d-products/maya/gltf-exporter/),
  [SimLab Maya to glTF](https://www.simlab-soft.com/3d-plugins/gltf-from-maya-main.aspx).
- BVH for Maya (third-party):
  [BVH import/export for Maya](https://www.highend3d.com/maya/plugin/bvh-file-import-export-for-maya-for-maya).
- Axes per tool: table in [LIVE_LINK_RESEARCH.md](LIVE_LINK_RESEARCH.md).
