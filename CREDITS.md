# Credits

PinglyMotion stands on the work of many people and projects. Thank you to all of them. This page lists what we use or learned from, what for, and under which licence. The licence notices that the licences require stay where they are (see `README.md`, `LICENSE`, and the folders named below); this page adds to them and replaces none of them.

Where a licence does not require a credit (CC0, ideas only), we give one anyway. If something is missing or wrong, please open an issue.

## Code and libraries

| Project | What we use it for | Licence | Link |
|---|---|---|---|
| QAvimator (by Zi Ree and the QAvimator team), itself a Qt port of Avimator | The origin of this program: the editor, the 3D view and the file handling started as QAvimator | GPL 2 or later | https://bitbucket.org/QAvimator_Team/qavimator |
| Qt 6 | The program's windows, 3D view widget, networking and tests | LGPL 3 / GPL (we use it as a shared library) | https://www.qt.io |
| OpenGL | The 3D view | Khronos specifications | https://www.khronos.org/opengl/ |
| CMake | Build system | BSD 3-Clause | https://cmake.org |
| Mesh2Motion (retargeting design) | The idea of the bone-name matching and swing-twist retargeting in `src/retarget.*`. No code copied; written from the idea | MIT | https://github.com/Mesh2Motion/mesh2motion-app |
| Second Life viewer and Firestorm viewer (Linden Research, Inc. and the Firestorm project) | Reference for the `.anim` format and joint data; `src/` logic is written from their documented behaviour | LGPL 2.1 | https://github.com/secondlife/viewer, https://github.com/FirestormViewer/phoenix-firestorm |

## Data and assets

| Project | What we use it for | Licence | Link |
|---|---|---|---|
| Firestorm viewer: `avatar_skeleton.xml` | The Bento skeleton (`bin/data/bento_joints.txt`, `bin/data/skin_joints.txt`, made by `tools/make_bento_skeleton.py`). A copy is in `tools/firestorm/` | LGPL 2.1 (Linden Research, Inc. and the Firestorm project) | https://github.com/FirestormViewer/phoenix-firestorm |
| Firestorm viewer: poser data | Friendly joint names, mirror partners and the 9 hand presets in `bin/data/firestorm/` (`joints.json`, `hand_presets/`); licence text in `bin/data/firestorm/LICENSE.txt` | LGPL 2.1 | https://github.com/FirestormViewer/phoenix-firestorm |
| Ruth2 v4 (RuthAndRoth; Shin Ingen, Ada Radius and many contributors) | The default avatar body, converted to `ruth2.pmm` | Mesh AGPL 3 or later; UV map CC BY Linden Lab; rig parts CC BY 3.0 Machinimatrix.org. Full notice: `bin/data/character/ruth2/` | https://github.com/RuthAndRoth/Ruth2 |
| Roth2 v2 (RuthAndRoth; Shin Ingen, Ada Radius and many contributors) | The male figure, converted to `roth2.pmm` | Body parts AGPL 3 or later; head and UV map CC BY Linden Lab; rig parts CC BY 3.0 Machinimatrix.org. The non-commercial nails are not used. Full notice: `bin/data/character/roth2/` | https://github.com/RuthAndRoth/Roth2 |
| "Maxine" from the Max Avatar Project (Ada Radius and the RuthAndRoth community) | The optional second body (Options > Body > Max), converted to `maxine.pmm` | CC BY 4.0. Full notice: `bin/data/character/max/` | https://github.com/RuthAndRoth/Max |
| Linden Lab | The Second Life skeleton and the UV map that Ruth2 and Roth2 build on | See the Ruth2 and Roth2 notices | https://secondlife.com |
| Machinimatrix.org | The rig parts that Ruth2 and Roth2 build on | CC BY 3.0 | https://www.machinimatrix.org |

## Motion data

| Project | What we use it for | Licence | Link |
|---|---|---|---|
| CMU Graphics Lab Motion Capture Database, BVH conversion by cgspeed.com (copy in `una-dinosauria/cmu-mocap`) | Test motions in `examples/cmu/` and 28 clips of the reference library in `library/` | CMU places no restrictions on use; no formal licence text. The data used in this project was obtained from mocap.cs.cmu.edu. The database was created with funding from NSF EIA-0196217. | http://mocap.cs.cmu.edu |
| Mesh2Motion animations | Eight test motions in `examples/mesh2motion/` and 27 clips of the reference library. Thank you to the Mesh2Motion project | CC0 1.0 (no credit required, given anyway) | https://github.com/Mesh2Motion/mesh2motion-app |

Each clip of the reference library carries its own licence and credit line in `library/index.json`.

## Ideas and design inspiration

No code was taken from these; they showed us what is worth building.

| Project | What we learned | Licence of the project | Link |
|---|---|---|---|
| Aurora Viewer (Rust Second Life viewer) | Ideas for the poser and animation editor interface (design only). The live link uses Aurora's LiveMesh interface as a client | GPL 3 | https://github.com/Aurora-Viewer/Aurora-Viewer |
| Firestorm poser | How a poser handles joint groups, mirroring and hand presets | LGPL 2.1 | https://github.com/FirestormViewer/phoenix-firestorm |
| Blender | Navigation habits (middle-drag, number pad views) and the export route described in `docs/BLENDER.md` | GPL | https://www.blender.org |
| sketchpunklabs/threejs_proto | The swing-twist retargeter that Mesh2Motion builds on | ISC (per its `package.json`) | https://github.com/sketchpunklabs/threejs_proto |
| anim-to-bvh | Table of Second Life joint names and BVH aliases, used as a reference for `.anim` import | MIT (per its `package.json`) | see `docs/OPEN_SOURCE_CHECK.md` |
| PoseForge, Black Dragon viewer, tanimbomb | Ideas only (no code read or copied); see `docs/REUSE_REFERENCES.md` and `docs/OPEN_SOURCE_CHECK.md` | Various | see those documents |

## Research

Ideas from these papers were re-implemented from scratch, in our own code, for the assistant commands (`describe_pose`, `describe_motion`, `check_animation`, labelled screenshots). Their own code, where it has a non-commercial licence, was not used.

| Work | What we use the idea for | Link |
|---|---|---|
| PoseScript: Linking 3D Human Poses and Natural Language | `describe_pose`: rule-made sentences from joint geometry | https://arxiv.org/abs/2210.11795 |
| MotionScript: Natural Language Descriptions for Human Motions | `describe_motion`: a motion in words per joint | https://arxiv.org/abs/2312.12634 |
| Set-of-Mark Prompting | Labelled joint markers in screenshots so an assistant can tell them apart | https://arxiv.org/abs/2310.11441 |
| Kimodo (NVIDIA): kinematic motion diffusion model | The joint map for its SOMA 77 skeleton (`src/retarget.*`) and the optional helper | https://github.com/nv-tlabs/kimodo, https://arxiv.org/abs/2603.15546 |
| Research reports (text to motion, video to motion) | The studies in `docs/research/` read and compared many more models and papers (MDM, MoMask, HY-Motion, WHAM, GVHMR, MediaPipe and others); each is credited and linked there | `docs/research/` |

## Tools

| Project | What we use it for | Licence | Link |
|---|---|---|---|
| XR Animator (ButzYung, SystemAnimatorOnline) | The recommended way to record yourself and get a BVH file (`docs/VIDEO_GUIDE.md`); an external tool, not part of PinglyMotion | Source CC BY-NC-SA 4.0 (see its own text) | https://github.com/ButzYung/SystemAnimatorOnline |
| MediaPipe (Google) | The pose tracking inside XR Animator | Apache 2.0 | https://github.com/google-ai-edge/mediapipe |
| Kimodo (NVIDIA) | Optional text-to-motion helper outside the program | Apache 2.0 code; models under the NVIDIA Open Model License | https://github.com/nv-tlabs/kimodo |
| Blender | Round-trip and comparison tools in `tools/blender/` | GPL | https://www.blender.org |
| Mixamo (Adobe), Rigify, Avastar (Machinimatrix) | Named in `docs/BLENDER.md` as routes for users; not bundled | Their own terms | see `docs/BLENDER.md` |
| Model Context Protocol and MCPB extension format (Anthropic and the MCP community) | The MCP server and one-click extension in `tools/mcp/` | MIT / Apache 2.0 | https://modelcontextprotocol.io |
| Node.js and Python | Run the MCP server and the project's tools | MIT / PSF | https://nodejs.org, https://www.python.org |

## Build and test infrastructure

| Project | What we use it for | Licence | Link |
|---|---|---|---|
| GitHub Actions: `actions/checkout`, `cache`, `upload-artifact`, `download-artifact`, `attest-build-provenance`, `dependency-review-action` | Builds, tests, releases and checks | MIT | https://github.com/actions |
| `github/codeql-action` | Code scanning | MIT | https://github.com/github/codeql-action |
| `jurplel/install-qt-action` | Installs Qt in the builds | MIT | https://github.com/jurplel/install-qt-action |
| libFuzzer and the sanitizers (LLVM, GCC) | Fuzzing and the sanitizer build | Apache 2.0 with LLVM exception | https://llvm.org |

## Standards and documentation

| Source | What we use it for | Link |
|---|---|---|
| Second Life Wiki (Linden Lab) | Animation upload rules, "Suggested BVH Joint Rotation Limits", Project Bento resources, puppetry notes | https://wiki.secondlife.com |
| Second Life viewer source (`llkeyframemotion.cpp`) | Reference for the `.anim` format | https://github.com/secondlife/viewer |
| BVH (Biovision Hierarchy) format | The main import and export format | https://research.cs.wisc.edu/graphics/Courses/cs-838-1999/Jeff/BVH.html |
| glTF 2.0 (Khronos) | Animation import | https://www.khronos.org/gltf/ |
| AAOS clinical range of motion | The basis of the finger joint limits | American Academy of Orthopaedic Surgeons |
| SHA-256, HMAC (NIST, IETF RFC 2104) | Challenge and session key of the Aurora live link; file checks | https://www.rfc-editor.org/rfc/rfc2104 |

## Licence of PinglyMotion

PinglyMotion itself is licensed under the GNU General Public License version 2 or later (`LICENSE`, `COPYING`). Bundled data files keep their own licences, listed above and in their folders.
