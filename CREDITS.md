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
| Lucide (Lucide Icons and Contributors; portions from Feather by Cole Bemis) | The one icon set of the design system (`docs/DESIGN_SYSTEM.md`). The path data of each icon is copied unchanged from Lucide 1.77.0 and drawn by `src/lucideicons.*`; first used for the play, square and rotate-ccw icons of the transport buttons, then for the top bar (undo, redo, save, upload, flask-conical, settings, sun, moon, sparkles, info). Licence text in `src/icons/lucide/LICENSE` | ISC | https://lucide.dev, https://github.com/lucide-icons/lucide |

## Data and assets

| Project | What we use it for | Licence | Link |
|---|---|---|---|
| Firestorm viewer: `avatar_skeleton.xml` | The Bento skeleton (`bin/data/bento_joints.txt`, `bin/data/skin_joints.txt`, made by `tools/make_bento_skeleton.py`). A copy is in `tools/firestorm/` | LGPL 2.1 (Linden Research, Inc. and the Firestorm project) | https://github.com/FirestormViewer/phoenix-firestorm |
| Firestorm viewer: poser data | Friendly joint names, mirror partners and the 9 hand presets in `bin/data/firestorm/` (`joints.json`, `hand_presets/`); licence text in `bin/data/firestorm/LICENSE.txt`. The presets are the ready-made finger poses of the Hands tab (UX phase 2c, `src/handpose.cpp`); how a preset's three angles make a turn follows Firestorm's `FSPoserAnimator::loadJointRotation()` and the viewer's `LLMatrix3::setRot()` and `LLQuaternion::getEulerAngles()` (`indra/llmath`), written anew, no code copied | LGPL 2.1 | https://github.com/FirestormViewer/phoenix-firestorm |
| Ruth2 v4 (RuthAndRoth; Shin Ingen, Ada Radius and many contributors) | The default avatar body, converted to `ruth2.pmm` | Mesh AGPL 3 or later; UV map CC BY Linden Lab; rig parts CC BY 3.0 Machinimatrix.org. Full notice: `bin/data/character/ruth2/` | https://github.com/RuthAndRoth/Ruth2 |
| Roth2 v2 (RuthAndRoth; Shin Ingen, Ada Radius and many contributors) | The male figure, converted to `roth2.pmm` | Body parts AGPL 3 or later; head and UV map CC BY Linden Lab; rig parts CC BY 3.0 Machinimatrix.org. The non-commercial nails are not used. Full notice: `bin/data/character/roth2/` | https://github.com/RuthAndRoth/Roth2 |
| "Maxine" from the Max Avatar Project (Ada Radius and the RuthAndRoth community) | The optional second body (Options > Body > Max), converted to `maxine.pmm` | CC BY 4.0. Full notice: `bin/data/character/max/` | https://github.com/RuthAndRoth/Max |
| QAvimator toolbar icons and start files (`src/icons/`, `bin/data/*.bvh`, `.avm`, `SL.lim`, `cube.obj`, `torus.obj`, `tools/legacy/`) | The toolbar pictures and the classic skeleton and start files carried over from QAvimator | GPL 2 or later (QAvimator) | https://bitbucket.org/QAvimator_Team/qavimator |
| Linden Lab | The Second Life skeleton and the UV map that Ruth2 and Roth2 build on | See the Ruth2 and Roth2 notices | https://secondlife.com |
| Machinimatrix.org | The rig parts that Ruth2 and Roth2 build on | CC BY 3.0 | https://www.machinimatrix.org |
| OpenCollar (the OpenCollar project and its contributors) | A real, large setup for the Script Lab: the 23 collar scripts (`src/collar/*.lsl`) copied unchanged as plain text into `examples/improve/OpenCollar/`, next to a verbatim copy of their licence (`examples/improve/OpenCollar/LICENSE`). Copied on 25 November 2026 from their repository at the commit of 3 July 2026 (`2500e6a`). The scripts stay as they are and full permission; nothing else is taken (no notecards, pictures, models, animations or sounds, which are under other terms). Script Lab, «improve a finished setup» ([SCRIPT_LAB.md](docs/SCRIPT_LAB.md)) | GPL 2 with the License Terms for OpenCollar (a verbatim copy of the licence goes with the scripts; scripts stay full permission; changes must be open with a change log) | https://github.com/OpenCollarTeam/OpenCollar |
| Lucide icons (Lucide Icons and Contributors; some icons derived from Feather by Cole Bemis) | The one icon set of the design system (`docs/DESIGN_SYSTEM.md`); first used as inline SVG in the UX mockup `docs/mockup/index.html`. Licence text: `docs/mockup/LUCIDE_LICENSE.txt` | ISC (Feather-derived icons MIT) | https://lucide.dev |

## Motion data

| Project | What we use it for | Licence | Link |
|---|---|---|---|
| CMU Graphics Lab Motion Capture Database, BVH conversion by cgspeed.com (copy in `una-dinosauria/cmu-mocap`) | Test motions in `examples/cmu/` and 47 clips of the reference library in `library/` (28 first batch, 19 second batch, trial numbers in each clip's `origin`; CMU index text from the same source commit used for the descriptions) | CMU places no restrictions on use; no formal licence text. The data used in this project was obtained from mocap.cs.cmu.edu. The database was created with funding from NSF EIA-0196217. | http://mocap.cs.cmu.edu |
| Mesh2Motion animations | Eight test motions in `examples/mesh2motion/` and 59 clips of the reference library (27 first batch, 32 second batch; licence file `LICENSE-CC0.MD` of the source repository re-read at commit `bcb014ea25e44f76b3add78463f72d1faec16418` on 21 November 2026). Thank you to the Mesh2Motion project | CC0 1.0 (no credit required, given anyway) | https://github.com/Mesh2Motion/mesh2motion-app |
| Quaternius, Universal Animation Library (standard, free tier), glTF copy in `J-Ponzo/gltf-universal-animation-library` | An optional download source of the reference library: `tools/fetch_reference.py` fetches it on the user's machine and `tools/convert_reference.py` converts its 45 animations to the Second Life skeleton. Nothing of it is stored in the repository. Thank you to Quaternius | CC0 1.0 (no credit required, given anyway) | https://quaternius.com/packs/universalanimationlibrary.html |

Each clip of the reference library carries its own licence and credit line in `library/index.json`.

## Ideas and design inspiration

No code was taken from these; they showed us what is worth building.

| Project | What we learned | Licence of the project | Link |
|---|---|---|---|
| Aurora Viewer (Rust Second Life viewer) | Ideas for the poser and animation editor interface (design only). The live link uses Aurora's LiveMesh interface as a client | GPL 3 | https://github.com/Aurora-Viewer/Aurora-Viewer |
| Firestorm poser | How a poser handles joint groups, mirroring and hand presets | LGPL 2.1 | https://github.com/FirestormViewer/phoenix-firestorm |
| Blender | Navigation habits (middle-drag, number pad views) and the export route described in `docs/BLENDER.md`; the upright tabs of its sidebar, drawn as side tabs in Mockup 2 of `docs/mockup/index.html` (design only) | GPL | https://www.blender.org |
| Second Life build tools (Linden Research, Inc.) | How props are edited in Mockup 2 of `docs/mockup/index.html`: move arrows, Ctrl for turning rings, Ctrl+Shift for size, and the shape fields cut, hollow and twist (design only) | Design reference, no code | https://secondlife.com |
| sketchpunklabs/threejs_proto | The swing-twist retargeter that Mesh2Motion builds on | ISC (per its `package.json`) | https://github.com/sketchpunklabs/threejs_proto |
| anim-to-bvh | Table of Second Life joint names and BVH aliases, used as a reference for `.anim` import | MIT (per its `package.json`) | see `docs/archive/OPEN_SOURCE_CHECK.md` |
| PoseForge, Black Dragon viewer, tanimbomb | Ideas only (no code read or copied); see `docs/archive/REUSE_REFERENCES.md` and `docs/archive/OPEN_SOURCE_CHECK.md` | Various | see those documents |
| Firestorm RLVa (Kitty Barnett and the Firestorm project) | Script Lab RLV core (`src/scriptlab/`, issue 287): the command syntax (`<behaviour>[:<option>]=<param>`), the parameter types, the return codes (success, unset, duplicate, failed, retained), the behaviour names and the `@clear` and `@getstatus` rules follow RLVa (`indra/newview/rlv*.cpp/h`). A modified port written anew with Qt types: no RLVa source file is copied and RLVa's viewer handler is not used. Step 9 (issue 297) adds the RLVa-only behaviour names, parameter types and the "RLV" / "RLVa only" marks, taken from the dictionary (`rlvdefines.h`, `rlvhelper.cpp`); the table is written fresh and no handler is ported. Since the source check (22 Nov 2026) the marks follow from the dictionary and the RLV wiki together (next rows). The Script Lab vision sphere in the 3D view (`EnvironmentOverride::applySphere`, 23 Nov 2026) follows RLVa's sphere effect (`indra/newview/rlveffects.cpp`, the shader `app_settings/shaders/class1/deferred/rlvF.glsl`, by Kitty Barnett): the mode numbers, the defaults and the strength by distance; the effect itself is written anew as a simple picture filter, no shader code is copied. Every file in `src/scriptlab/` carries a licence note naming RLVa | LGPL 2.1 | https://github.com/FirestormViewer/phoenix-firestorm |
| RestrainedLove API (RLV specification, Second Life wiki) | Meaning of the behaviours and of `@version`, `@versionnum` and `@getstatus` answers in the Script Lab RLV core. Since the source check (22 Nov 2026) the command names and parameter types of its "Restrained Love viewer v2.9 Specification" are the RLV standard for the table's marks: `tools/rlv_sources.py` turns the command syntax lines (`tests/data/rlv/rlv_wiki_capture.txt`) into `tests/data/rlv/rlv_wiki.tsv`, names and types only | Public documentation | https://wiki.secondlife.com/wiki/LSL_Protocol/RestrainedLoveAPI |
| Firestorm RLVa behaviour dictionary (Kitty Barnett and the Firestorm project) | The RLVa standard of the source check (22 Nov 2026): `tools/rlv_sources.py` reads `indra/newview/rlvhelper.cpp` and writes the names and parameter types to `tests/data/rlv/rlva.tsv`; no code or text is copied | LGPL 2.1 | https://github.com/FirestormViewer/phoenix-firestorm |
| Firestorm RLVa option handlers and viewer basics (Kitty Barnett, Linden Research, Inc. and the Firestorm project) | The Script Lab RLV option value check (lane 1, job C, `docs/RLV_OPTION_TYPES.md`): which values RLVa refuses follows the handlers in `indra/newview/rlvhandler.cpp`, `rlvhelper.cpp` and `rlvlocks.h`, the key rules of `LLUUID::set()` and `LLUUID::validate()` (`indra/llcommon/lluuid.cpp`), `LLStringUtil::convertToS32()` (`indra/llcommon/llstring.h`), the attachment point names of `indra/newview/character/avatar_lad.xml` and the wearable type names of `indra/llappearance/llwearabletype.cpp`. Rules and names only; the readers are written anew and no code is copied | LGPL 2.1 | https://github.com/FirestormViewer/phoenix-firestorm |
| Kokua viewer (Marine Kelley's RestrainedLove) | The plain RLV viewer the "does not work in Kokua" mark of the source check refers to. Its source (on Bitbucket) is the RLV reference, but it could not be reached from the job's sandbox, so nothing was read or taken from it yet; the wiki stands in for it | LGPL 2.1 | Bitbucket (not reached) |
| Catznip RLVa documentation | Named as the extra source for RLVa descriptions in the source check; it could not be reached from the job's sandbox, so nothing was read or taken from it yet | Public documentation | https://docs.catznip.com/rlva/allinone/ |
| LSL-PyOptimizer | Script Lab: possible reference for LSL behaviour only. Not used for the LSL interpreter; GPLv3, so it is never copied | GPLv3 | https://github.com/Sei-Lisa/LSL-PyOptimizer |
| Second Life viewer / Firestorm viewer: `keywords_lsl_default.xml` (Linden Research, Inc. and the Firestorm project) | Script Lab LSL interpreter (`src/scriptlab/lsl/`): the names and signatures of all LSL functions and events and the constant values in `src/scriptlab/lslbuiltins_data.inc`, generated by `tools/gen_lsl_builtins.py` from the Second Life viewer's copy of the file (branch `release`, same format as Firestorm's). Names, types and values only; tooltips left out | LGPL 2.1 | https://github.com/secondlife/viewer, https://github.com/FirestormViewer/phoenix-firestorm |
| SLua guide "ll Functions" by Suzanna Linn (community documentation) | The LSL-to-SLua converter (lane 1, job G, `docs/SLUA_RULES.md`): the rule that negative positions keep counting from the end in SLua's `ll` functions, which Linden Lab's definition files do not say. Facts only; no text or code copied | Public documentation | https://suzanna-linn.github.io/slua/moving-llfunctions |
| Second Life lsl-definitions: `lsl_definitions.yaml` (Linden Research, Inc.) | Script Lab LSL interpreter: the names and signatures of the functions, events and constants that the viewer's keyword list lacks (Linkset Data, `llReplaceSubString`, `llListSortStrided` and others) in `src/scriptlab/lslbuiltins_extra.inc`, generated by `tools/gen_lsl_extra.py`. Names, types and values only; tooltips left out | MIT | https://github.com/secondlife/lsl-definitions |
| Second Life lsl-definitions (Linden Research, Inc.): copy | A copy of `lsl_definitions.yaml`, `slua_definitions.yaml` and `generated/secondlife.d.luau`, unchanged, with the licence file and the commit it comes from, in `tests/data/lsl-definitions/`. Used by the definition check (`tools/lsl_definition_check.py`, its test and `docs/LSL_DEFINITION_CHECK.md`); the Script Lab editor's suggestions and help (names, types, argument names and the first sentence of each description, in `src/scriptlab/lslhelp_data.inc`, generated by `tools/gen_lsl_help.py`); and the LSL-to-SLua converter (lane 1, job G): what changes in SLua for each function and event, in `src/scriptlab/lsl/slua_data.inc`, generated by `tools/gen_slua_data.py` (`docs/SLUA_RULES.md`) | MIT (Linden Research, Inc.) | https://github.com/secondlife/lsl-definitions |
| Aurora Viewer: material used with credit | Under the decision of 23 November 2026 (`docs/DECISIONS.md`) Aurora's material may be used if it is credited here and in the docs. Aurora is GPL 3, so the part that uses Aurora's material follows GPL 3 terms. Nothing of it is used yet | GPL 3 | https://github.com/Aurora-Viewer/Aurora-Viewer |
| lslint (public domain), Second Life wiki, OpenSimulator (BSD) | Script Lab LSL interpreter: the grammar and type rules (lslint), the idea of some static warnings of `lsl_lint` (unused names, unreachable code, an assignment as a condition; lslint) and how the built-in functions behave (Second Life wiki, OpenSimulator), written from their description; no code copied | Public domain, BSD | https://github.com/Makopo/lslint, https://wiki.secondlife.com/wiki/LSL_Portal, http://opensimulator.org |

<!-- Luau and SLua (both MIT) are added to this page only when they are taken in (a Luau trial build, the converter's Lua output). Until then they are not credits. -->

## Research

Only ideas were taken from these papers and projects (no code copied); they were re-implemented from scratch, in our own code, for the assistant commands (`describe_pose`, `describe_motion`, `check_animation`, labelled screenshots). Their own code, where it has a non-commercial licence, was not used.

| Work | What we use the idea for | Link |
|---|---|---|
| PoseScript: Linking 3D Human Poses and Natural Language | `describe_pose`: rule-made sentences from joint geometry | https://arxiv.org/abs/2210.11795 |
| MotionScript: Natural Language Descriptions for Human Motions | `describe_motion`: a motion in words per joint | https://arxiv.org/abs/2312.12634 |
| Set-of-Mark Prompting | Labelled joint markers in screenshots so an assistant can tell them apart | https://arxiv.org/abs/2310.11441 |
| PoseFix | Ideas for the AI-help design: describing a pose and its change in words | https://arxiv.org/abs/2309.08480 |
| FreeMotion | Ideas for the AI-help design: how a language model can reason about motion | https://arxiv.org/abs/2406.10740 |
| Encoder-Free Human Motion Understanding via Structured Motion Descriptions | Ideas for the AI-help design: a motion as structured text for a language model | https://arxiv.org/abs/2604.21668 |
| How Much Do Large Language Models Know about Human Motion? | Ideas for the AI-help design: what a language model knows about human motion | https://arxiv.org/abs/2505.21531 |
| Can Vision Language Models Judge Action Quality? | Ideas for the AI-help design: judging movement quality with a language model, and why exact numbers back up pictures | https://arxiv.org/abs/2604.08294 |
| Kimodo benchmark metric definitions (NVIDIA) | Ideas for the AI-help design: metric definitions such as foot skate and contact consistency | https://research.nvidia.com/labs/sil/projects/kimodo/docs/benchmark/metrics.html |
| Blender MCP (ahujasid) | The idea of showing the assistant a strip of frames (`views`, frame strip screenshots). No code used | https://github.com/ahujasid/blender-mcp |
| Kimodo (NVIDIA): kinematic motion diffusion model | The joint map for its SOMA 77 skeleton (`src/retarget.*`) and the optional helper | https://github.com/nv-tlabs/kimodo, https://arxiv.org/abs/2603.15546 |
| MoMask (text-to-motion) and the SMPL body model's joint list | The ready joint map for the SMPL 22-joint skeleton (`src/retarget.*`): the joint names and tree of MoMask's BVH export and SMPL's joint names, as facts. No code, weights or motion used | https://github.com/EricGuo5513/momask-codes, https://smpl.is.tue.mpg.de |
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
| NumPy | Builds the Max body mesh (`tools/make_max_mesh.py`); a tool for developers, not shipped | BSD 3-Clause | https://numpy.org |
| Blender as a Python module (`bpy`) and uv (Astral) | Run the Blender round trip in CI (`.github/workflows/blender.yml`) | GPL (bpy); MIT or Apache 2.0 (uv) | https://pypi.org/project/bpy/, https://github.com/astral-sh/uv |
| Node.js and Python | Node runs the MCP server (Claude Desktop ships its own); Python runs the project's tools and tests | MIT / PSF | https://nodejs.org, https://www.python.org |

## Build and test infrastructure

| Project | What we use it for | Licence | Link |
|---|---|---|---|
| GitHub Actions: `actions/checkout`, `cache`, `upload-artifact`, `download-artifact`, `attest-build-provenance`, `dependency-review-action` | Builds, tests, releases and checks | MIT | https://github.com/actions |
| `github/codeql-action` | Code scanning | MIT | https://github.com/github/codeql-action |
| `jurplel/install-qt-action` | Installs Qt in the builds | MIT | https://github.com/jurplel/install-qt-action |
| aqtinstall (miurahr) | Installs Qt behind `install-qt-action` in the builds | MIT | https://github.com/miurahr/aqtinstall |
| Ubuntu packages `qt6-base-dev`, ccache, Xvfb | Linux builds and screenshot tests | LGPL 3 / GPL, GPL 3, X11 | https://packages.ubuntu.com |
| libFuzzer and the sanitizers (LLVM, GCC) | Fuzzing and the sanitizer build | Apache 2.0 with LLVM exception | https://llvm.org |

## Standards and documentation

| Source | What we use it for | Link |
|---|---|---|
| Second Life Wiki (Linden Lab) | Animation upload rules, "Suggested BVH Joint Rotation Limits", Project Bento resources, puppetry notes | https://wiki.secondlife.com |
| Second Life viewer source (`llkeyframemotion.cpp`) | Reference for the `.anim` format | https://github.com/secondlife/viewer |
| BVH (Biovision Hierarchy) format | The main import and export format | https://research.cs.wisc.edu/graphics/Courses/cs-838-1999/Jeff/BVH.html |
| glTF 2.0 (Khronos) | Animation import | https://www.khronos.org/gltf/ |
| AAOS clinical range of motion | The basis of the finger joint limits | American Academy of Orthopaedic Surgeons |
| RLV relay protocol (the open Restrained Love Relay specification, Second Life Wiki) | The message format, channel and meta commands of the simple relay example (`examples/objects/RLV_relay`, written in our own words) | https://wiki.secondlife.com/wiki/LSL_Protocol/Restrained_Love_Relay/Specification |
| SHA-256, HMAC (NIST, IETF RFC 2104) | Challenge and session key of the Aurora live link; file checks | https://www.rfc-editor.org/rfc/rfc2104 |

## Licence of PinglyMotion

PinglyMotion itself is licensed under the GNU General Public License version 2 or later (`LICENSE`, `COPYING`). Bundled data files keep their own licences, listed above and in their folders.
