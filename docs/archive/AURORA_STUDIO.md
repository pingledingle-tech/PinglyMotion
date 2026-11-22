Archived 22 November 2026: analysis finished; the product direction was decided on 19 November 2026 (DECISIONS.md, issue 59).

# Aurora Viewer: what its animation studio has, what it lacks, and our options

Status: **analysis.** The product direction was decided on 19 November 2026: PinglyMotion continues as its own program (`docs/DECISIONS.md`, issue 59); the Aurora options below (offline trial, a studio fork, contributions) are not started. Written 19 November 2026 from a read of the Aurora Viewer source (a fork of it was cloned and read; nothing was built or run). Numbers are measured line counts of the Rust source at one commit and will drift.

Aurora Viewer is a Second Life viewer written in Rust (Vulkan, egui), licence GPLv3. PinglyMotion is GPLv2-or-later, which is compatible, but any code moved over must be rewritten from C++ to Rust.

## 1. What Aurora's studio is

The studio lives in one module, `crates/aurora-viewer/src/ik_edit` (about 38 000 lines): a poser, a clip animation editor and an IK workbench. It reads and writes BVH and `.anim`. Animation parsing and the skeleton are in `crates/aurora-assets` (`import_animation.rs`, `anim.rs`, `skeleton.rs`, `import_gltf.rs`).

The editor can run offline: the environment variable `AURORA_DEMO=1` starts a demo world without login, and scenario variables such as `AURORA_DEMO_IK_EDIT=clip` open the editor directly. These are scripted test scenes, not a clean start. **Not verified by running.**

## 2. Gaps found in Aurora (code read)

| Gap | What the code shows | Our related issue |
|---|---|---|
| Joint limits | Poser, editor and IK solver have none; rotations are set directly | `docs/POSING_UX.md` |
| Retargeting | BVH import matches joint names and aliases only; the editor refuses unknown joints | #16 |
| glTF/GLB animation | glTF is read for meshes and materials only | #12 |
| Per-key curves | One curve per track | #11 |
| Beginner help | Little in-program teaching | `docs/TEACHING.md` |
| Fuzz target for LiveMesh | None found | #23, #46 |

Things that conflict with Aurora's own rules and cannot go upstream: remote control for AI (their policy is "No AI in the viewer you download"), text-to-motion, video-to-motion with an AI model.

## 3. Why we cannot simply lift the studio out

Compiling `ik_edit` pulls in 126 of 242 modules of the viewer crate, 960 000 of 988 000 lines (97 %), because `scene`, `world`, `ui`, `build` and `demo` refer to each other in a circle. The editor's own interface to the rest is narrow (a rig, an IK solver, an avatar, a floater widget, a projector, a palette, and nine methods on `World`), but a real extraction would still need new glue for avatar drawing, windows, dialogs, settings and translations, and every upstream change to the editor would have to be carried over by hand.

## 4. Three ways forward

- **A. Extract the studio into our own program.** Not realistic: months of work, and the result is behind upstream from day one.
- **B. Thin fork with a "studio mode".** Keep the whole viewer; change only the start so it opens offline in the animation editor. Few files touched, so it stays easy to keep in step with upstream. Needs its own name and icon (see section 6). Estimate: days for the mode itself.
- **C. No code.** A Windows shortcut or script that sets `AURORA_DEMO=1` and `AURORA_DEMO_IK_EDIT=clip` and starts the official Aurora program. Shows in minutes whether B is worth doing and how the editor feels.

Our own additions (joint limits, retargeting, glTF animation, curves, help texts) can be one branch each on top of B, or pull requests to Aurora, whichever the project owner decides.

## 5. How Aurora takes contributions

- Ideas go to their feedback board; GitHub issues are for bugs. Comment on an issue before starting work on it.
- The pull request template must be complete (an incomplete one is closed after 3 days). At most 10 pull requests in review. CI starts after a maintainer approves.
- A human must test the change, and their `scripts/check.ps1` must be green.
- Interface text must exist in English, French and Japanese (a test checks parity).
- No mention of AI in commits or pull requests, and no AI-generated art.

## 6. Naming and licence if we make a modified version

Their `TRADEMARKS.md`: a modified version needs its **own name and icon**, must keep Aurora's copyright and licence notices (also in About) and say that it is modified. "Based on Aurora Viewer" is allowed without asking.

## 7. Maya and the Blender live link

- Maya has no native glTF export (plug-ins exist). It has a Python API (`maya.cmds`, OpenMaya, `commandPort`, `mayapy` for batch use).
- Aurora's Blender live link (LiveMesh, protocol v1) is a local TCP connection with a key file and mesh snapshots; its installer is Blender-specific.
- A Maya link would be a separate client of the same protocol. **Not checked in detail**; the open question is whether the protocol can carry animation poses from Maya the way we send `pose` messages today. Direct Maya support stays out of scope until the BVH, `.anim` and glTF routes work (see `docs/DECISIONS.md`).

## 8. Open questions for the project owner

1. (Decided 19 Nov 2026: PinglyMotion continues as its own program, see `docs/DECISIONS.md`.) Which product: our features as contributions to Aurora, an own-named studio fork, PinglyMotion as the stand-alone and AI-drivable editor, or a mix?
2. (Moot after the decision of 19 Nov 2026.) What happens to the open PinglyMotion jobs if the answer is not "PinglyMotion"?
3. Ask the Aurora developers first (their Discord or feedback board), or try option C alone?
