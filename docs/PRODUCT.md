# PinglyMotion: product vision and roadmap

This document brings together the product vision, user value, use cases and proposed direction. It complements [IDEAS.md](IDEAS.md), the work board in [work/README.md](work/README.md) and the linked issues. Issues and pull requests are the source for current task status. The project restarted in this repository on 18 November 2026: a number written "old #N" is an item in the old repository, see [MIGRATION.md](MIGRATION.md) for the moved items and [DECISIONS.md](DECISIONS.md) for the decisions. The sequence below is a proposal, not a commitment, schedule or replacement for existing technical decisions.

## Vision

Make it possible for newcomers to create, adapt and improve Second Life animations for free, without professional tools or paid add-ons. PinglyMotion should connect approachable posing and animation editing with open motion sources, optional AI assistance and preview on the user's avatar.

People should be able to work manually, with AI, or with both. PinglyMotion remains useful without AI or a viewer connection.

## People and the value they get

| Person and need | Value |
| --- | --- |
| A newcomer making a sit, stand or AO | An approachable way to create or adjust motion without mastering a professional 3D package. |
| A dancer or animator | Edit and reuse poses and motion sequences, with clearer timing and looping. |
| A creator with animation from another tool | A potential free route to adapt motion to the SL Bento skeleton and export it for SL. |
| Someone using AI | Describe and refine motion while retaining manual control, with visual and joint-data feedback where available. |
| A furniture or object creator | Reuse animations and, eventually, simple LSL controller templates. |

These are intended outcomes, not claims that each workflow is already implemented.

## The intended journey

**Create or import → adapt and edit → preview → export and use → reuse.**

Start with a manual pose or animation; in the future also investigate AI-generated sequences, other tools, open animation libraries and ordinary video. Adjust on the Bento skeleton, preview where possible, export to SL, and reuse the work in AOs, furniture or other objects.

## Use cases

### 1. Adjust a sitting pose for an avatar

- **User and need:** An SL user wants a sit to fit their avatar and furniture.
- **Journey:** Open or create the pose, select body parts, adjust joints, inspect the result, then save and export.
- **Value:** Less guesswork and easier correction. Aurora live preview could reduce repeated uploads.
- **Status:** Core manual editing exists. Live preview in Aurora works (old #78, old #95; tested on Windows with the Aurora fork). End-to-end export and SL upload still need validation.

### 2. Create an AO stand or sit

- **Journey:** Create a pose, add and adjust keys, refine the loop, export and use it in an AO.
- **Value:** Create personalised everyday movement without a paid authoring tool.
- **Needs:** Reliable editing, loop points, SL export and clear instructions. An AO of original work is an idea in [IDEAS.md](IDEAS.md); ownership of imported source material must be checked separately.

### 3. Make and refine a dance

- **Journey:** Create or import movement, adjust keyframes and transitions, preview and export.
- **Value:** Reuse movement and make timing changes without starting again.
- **Explore:** Reusable sequences, seamless looping, curves and possible BPM guides ([#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11)). Video-to-motion and complete AI-generated dances are research ideas, not established capabilities.

### 4. Preview on the user's avatar

- **Journey:** Connect PinglyMotion to Aurora, adjust a pose or frame and see the avatar locally in the viewer.
- **Value:** Judge fit on the actual avatar before uploading.
- **Status and limit:** Works today, see [AURORA_LINK.md](AURORA_LINK.md). The pose is sent as quaternions per joint when Aurora supports it (old #95), otherwise as BVH (old #78). The preview is local only: nothing is uploaded and others do not see it. It needs Aurora built from the fork until the change is proposed upstream. Synchronised playback and avatar shape are future ideas.

### 5. Ask an AI to help with a pose

- **Journey:** Enable local control, describe a pose, let the assistant use joint and frame commands, inspect a screenshot, and adjust.
- **Value:** A lower barrier for starting and iterating, while the user keeps control.
- **Status:** Local HTTP and MCP commands exist; setup and available commands are in [CONTROL.md](CONTROL.md). More advanced motion generation, visual assessment and collision or foot-contact analysis need investigation. The status bar shows whether an assistant is connected (old #43, done). `PinglyMotion.mcpb` installs the MCP server in the Claude app with a double-click, with the Node server, so no Python (not yet tried in Claude Desktop; #7).

### 6. Bring an external animation to SL

- **Journey:** Import or convert an animation, map and retarget its skeleton to Bento, review the result and export.
- **Value:** A possible free bridge from Blender and other sources to SL.
- **Status:** glTF import and Blender interoperability are later work ([#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12), [#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13)). FBX/Maya conversion paths and retargeting need investigation; direct support is not promised.

### 7. Turn ordinary video into movement

- **Journey:** Use an open tool to estimate motion from video, adapt it to Bento and edit the result.
- **Value:** Explore making dances and other motion without a mocap suit.
- **Status:** Idea only. Accuracy, missing depth, cleanup, rights and the integration workflow remain to be evaluated.

### 8. Make an animation usable in an object

- **Journey:** Export an animation and use it in an AO, poseball or furniture object, potentially with a generated LSL template.
- **Value:** Help users take finished animation into an SL object.
- **Status:** LSL templates and reusable libraries are ideas. LSL controls animations; it is not the animation authoring format. A full LSL IDE is outside the stated scope.

## Product areas and current direction

| Area | Direction | Tracking |
| --- | --- | --- |
| Posing | Grouped body/face/hand joints, hand presets, finer adjustment, visibility, mirroring and whole-pose controls. | [#14](https://github.com/pingledingle-tech/PinglyMotion/issues/14), old #59 joint limits |
| Timeline and motion | Keyframes, transport, zoom, interpolation, easing, looping and reusable sequences. | [#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11) |
| Aurora | Live preview with quaternion poses is done; playback synchronisation, avatar shape and an upstream proposal to Aurora later. | [AURORA_LINK.md](AURORA_LINK.md), old #78, old #95 |
| AI and openness | Improve the existing local API and MCP. Avoid tying the editor to a single assistant. Investigate easier installation and client status. | [CONTROL.md](CONTROL.md), [#7](https://github.com/pingledingle-tech/PinglyMotion/issues/7) |
| Files and SL export | Preserve existing BVH/AVM work, replace the parser and data model with regression and fuzz tests (done: opening, saving and removal of the old code), a new working format `.dingle` ([#26](https://github.com/pingledingle-tech/PinglyMotion/issues/26)), then native .anim export. | [#9](https://github.com/pingledingle-tech/PinglyMotion/issues/9), [#10](https://github.com/pingledingle-tech/PinglyMotion/issues/10), [#26](https://github.com/pingledingle-tech/PinglyMotion/issues/26) |
| Import and retargeting | Explore glTF, Blender, an own rigged avatar and an SL Bento bridge. | [#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12), [#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13) |
| New motion sources | Ordinary video and Mesh2Motion, subject to technical and licence checks. | [IDEAS.md](IDEAS.md) |
| Use and reuse | Pose and motion libraries, editable recipes, LSL templates and an own AO. | [IDEAS.md](IDEAS.md), [README roadmap](../README.md#where-pinglymotion-is-going) |

## Proposed roadmap

The documented technical decision is to modernise the renderer, then the BVH/AVM parser and data model, then add .anim export ([#8](https://github.com/pingledingle-tech/PinglyMotion/issues/8)). Product improvements can be worked on in small steps where dependencies and active work permit.

1. **Finish the foundation:** complete renderer work, protect existing animations and strengthen Windows and parser testing.
2. **Make a useful pose-to-preview journey:** improve practical posing and complete the Aurora connection. A proposed milestone is adjusting a sitting pose, previewing it on an avatar and saving the result.
3. **Harden the animation model:** replace the parser with tested error handling, round-trip coverage and fuzzing. Validate that frames, motion and loop data survive save/reopen.
4. **Improve animation and SL export:** develop .anim and timeline/easing improvements. Better interpolation can also benefit BVH; its usefulness does not depend entirely on .anim.
5. **Import and retarget:** establish a tested route for an external animation to the Bento skeleton; investigate Blender and own-avatar preview.
6. **Explore broader creation:** test video-to-motion, Mesh2Motion, AI-assisted sequences, reusable libraries and LSL templates in small experiments before committing to large integrations.

These stages are suggested groupings, not deadlines or a claim that every step must wait for every preceding feature. Actual prioritisation belongs to the project owner.

### Combined plan (18 November 2026)

This replaces the numbered work order of 16 November. The order of stages above still holds: posing comes before the parser, and the new parser keeps today's interface to the rest of the program. File formats were decided in old #118; the posing interface in [POSING_UX.md](POSING_UX.md) ([#14](https://github.com/pingledingle-tech/PinglyMotion/issues/14)).

**Done so far** (merged to `main`; "old" numbers are in the old repository):

- Renderer: modern shaders, GPU skinning, picking, lighting, floor shadow, old OpenGL code removed; OpenGL 3.3 core required (old #61, #99).
- Loop points (old #91); Qt 5 dropped (old #98); Qt maths instead of libquat (old #97).
- Joint limits following the SL standard, head and neck up to 45 degrees (old #59, #111, #129); new IK solver (old #130).
- Joint tabs and table (old #126); Firestorm joint data with names, groups, left/right pairs and hand presets, loader included (old #139); status bar with remote-control and Aurora indicators and the tips switch (old #43, #155).
- New file parser and data model: reading, saving and removal of the old token parser and writers are all done, with round-trip tests and fuzzing (old #62, parts 1 and 2 = old #140, #150; parts 3 and 4 = [#42](https://github.com/pingledingle-tech/PinglyMotion/pull/42) and [#43](https://github.com/pingledingle-tech/PinglyMotion/pull/43), issue [#9](https://github.com/pingledingle-tech/PinglyMotion/issues/9)).
- Live link to Aurora over LiveMesh with quaternion poses (old #78, #95).
- Security: the audit findings are fixed except a few small ones, see [SECURITY_AUDIT.md](SECURITY_AUDIT.md); fuzzing of `.prp` files and the control API (old #20), and of every new reader (BVH/AVM, `.dingle`, `.anim`, glTF). All fuzz targets run at the same time; a branch run takes about 10 minutes ([#71](https://github.com/pingledingle-tech/PinglyMotion/pull/71)).
- Version, source and commit shown in the window title ([#36](https://github.com/pingledingle-tech/PinglyMotion/pull/36)); the links inside the program point to this repository ([#35](https://github.com/pingledingle-tech/PinglyMotion/pull/35)).
- Ruth2 v4 body as data with a loader ([#2](https://github.com/pingledingle-tech/PinglyMotion/pull/2)) and the menu **Options > Body** to choose Ruth2 or Max ([#41](https://github.com/pingledingle-tech/PinglyMotion/pull/41)); the Linden system avatar is removed, Ruth2 is the default body and its makers are credited in Help > About and the README (issue [#15](https://github.com/pingledingle-tech/PinglyMotion/issues/15)). The male figure shows the Roth2 v2 body, chosen automatically from the figure type (no menu entry; Windows test by the project owner still to do), with credits in Help > About and the README.
- File formats, no menu yet (nothing changes on screen): `.anim` writer ([#63](https://github.com/pingledingle-tech/PinglyMotion/pull/63)), `.anim` reader with fuzz target ([#65](https://github.com/pingledingle-tech/PinglyMotion/pull/65)), BVH retargeting core ([#66](https://github.com/pingledingle-tech/PinglyMotion/pull/66)), glTF/GLB import reader ([#69](https://github.com/pingledingle-tech/PinglyMotion/pull/69); also reads meshopt-compressed animation data, as in the Mesh2Motion clips, with a converter tool `tools/glb2sl_bvh.cpp`, [#27](https://github.com/pingledingle-tech/PinglyMotion/issues/27)), `.dingle` reader and writer ([#51](https://github.com/pingledingle-tech/PinglyMotion/pull/51)) and the converted bundled files ([#57](https://github.com/pingledingle-tech/PinglyMotion/pull/57)). Blender round trip guide ([#68](https://github.com/pingledingle-tech/PinglyMotion/pull/68)) and scripted round trip with a BVH comparer (`tools/blender/`; the Blender script is not run in CI and not yet tested in Blender) and Maya live link note ([#67](https://github.com/pingledingle-tech/PinglyMotion/pull/67)).
- Posing UX plan ([POSING_UX.md](POSING_UX.md), old #135) and the format decisions (old #118, see [DECISIONS.md](DECISIONS.md)).

**Open pull requests (19 November 2026):** camera and keys ([#1](https://github.com/pingledingle-tech/PinglyMotion/pull/1)), test animations ([#3](https://github.com/pingledingle-tech/PinglyMotion/pull/3)), live link checks that the program is Aurora ([#46](https://github.com/pingledingle-tech/PinglyMotion/pull/46)) and opening `.dingle` files ([#70](https://github.com/pingledingle-tech/PinglyMotion/pull/70)). All four have green checks and wait for the project owner's Windows test (see [#47](https://github.com/pingledingle-tech/PinglyMotion/issues/47)). Also open, not done: privacy check fix for closed pull requests ([#73](https://github.com/pingledingle-tech/PinglyMotion/pull/73)), and the drafts live link rotation order ([#77](https://github.com/pingledingle-tech/PinglyMotion/pull/77)) and teaching texts step 2 ([#78](https://github.com/pingledingle-tech/PinglyMotion/pull/78)).

The work runs in **two tracks side by side**. They mostly change different files: the posing track the window, panels and 3D view; the format track the parser, the data model and file import/export. Where a step in one track needs a file another active job has, the work board ([work/README.md](work/README.md)) decides who waits. Within a track, one step at a time, each tested on Windows before it is merged.

**Track 1 – posing ([#14](https://github.com/pingledingle-tech/PinglyMotion/issues/14), steps in [POSING_UX.md](POSING_UX.md#order-of-work))**

1. Camera and keys: one scheme for new users, Blender and Second Life habits; old joint modifier keys removed. Beside it: a data file from Firestorm with joint names, groups, left/right pairs and hand presets (LGPL notice), and the status bar (Claude, Aurora, tips switch). The data file and the status bar are done; camera and keys are in [#1](https://github.com/pingledingle-tech/PinglyMotion/pull/1), waiting for a Windows test.
2. Friendly names, section headings and search in the joint list.
3. Floating, movable panels; Tab hides them; reset layout.
4. Selected-joint card: trackball, sliders, joint limit warning.
5. Left / Right / Mirror and the whole-pose buttons.
6. **Milestone:** adjust a sitting pose, preview it in Aurora, save it and upload it in SL.
7. The remaining posing steps: Hands tab, timeline, help tips and hint bar, command search, pose library and export selection, settings, visual polish and own icon, 3D view follows the tab.

**Track 2 – file formats (old #118)**

1. New parser and data model with fuzzing ([#9](https://github.com/pingledingle-tech/PinglyMotion/issues/9)): **done**. The old parser code is gone; reading and saving go through `animfile.*`, split into four pull requests.
2. `.anim` export and easing ([#10](https://github.com/pingledingle-tech/PinglyMotion/issues/10), [#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11)). Writer merged ([#63](https://github.com/pingledingle-tech/PinglyMotion/pull/63)). Menu: File > Export as Second Life .anim (a plain dialog with priority, slow start and finish, loop and hands; Save stays `.dingle` only), waiting for the project owner's Windows test. Easing ([#53](https://github.com/pingledingle-tech/PinglyMotion/issues/53)): the maths module exists, playback uses it, and a key can carry one of the six easing choices, stored as `curve` in `.dingle` and baked into the export (steps 1 to 4); the tile row to pick a choice (step 5) is not built yet.
3. `.anim` import ([#22](https://github.com/pingledingle-tech/PinglyMotion/issues/22)). Reader merged ([#65](https://github.com/pingledingle-tech/PinglyMotion/pull/65)). Menu: File > Import Second Life .anim opens the file as a new unsaved animation and lists what could not be imported; waiting for the project owner's Windows test.
4. BVH from any skeleton, retargeted to the SL skeleton ([#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16)). Core merged ([#66](https://github.com/pingledingle-tech/PinglyMotion/pull/66)). Menu: File > Import BVH from another skeleton shows which joints were matched to the avatar and which were left out (and why), with a switch for files that start in a T-pose or A-pose, then opens the result as a new unsaved animation; waiting for the project owner's Windows test. The hand-made mapping and the per-skeleton memory (step 4 of the issue) are still to do.
5. glTF/GLB import ([#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12)), then Blender ([#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13), docs first). No glTF export; FBX only on real demand. The own working format is `.dingle`; AVM is no longer written, old `.avm` files still open ([#26](https://github.com/pingledingle-tech/PinglyMotion/issues/26)). In the program: Save and Save As write `.dingle` only (a new animation is `pingle.dingle`), an opened `.avm` is saved as `.dingle` with the same name and the old file goes to the recycle bin after a read-back check, an opened `.bvh` is left alone, and the start poses load as `.dingle`; waiting for the project owner's Windows test. The layout, limits and version rules are in [DINGLE_FORMAT.md](DINGLE_FORMAT.md). Reader merged ([#69](https://github.com/pingledingle-tech/PinglyMotion/pull/69)), Blender guide merged ([#68](https://github.com/pingledingle-tech/PinglyMotion/pull/68)). Menu: File > Import glTF animation (`.glb` and `.gltf`) asks which animation and skeleton when the file has several, shows the same list of matched and left out joints as the BVH import, adds the reader's notes (for example ignored scale), and opens the result as a new unsaved animation; waiting for the project owner's Windows test. Two clips (`walk.glb`, `greeting.glb`) are in the Windows zip for it.

**Small jobs in between** (when a track waits for a Windows test): version in the window title ([#21](https://github.com/pingledingle-tech/PinglyMotion/issues/21), done in #36), and a new default body ([#15](https://github.com/pingledingle-tech/PinglyMotion/issues/15)).

**Later:** easy setup with an AI assistant ([#7](https://github.com/pingledingle-tech/PinglyMotion/issues/7)) once the posing interface has settled; larger AI features and libraries as small experiments ([#17](https://github.com/pingledingle-tech/PinglyMotion/issues/17)–[#20](https://github.com/pingledingle-tech/PinglyMotion/issues/20)). Manual use always works without AI.

## Product principles and decisions

- Keep the focus on Second Life and Bento, with manual use available without AI or Aurora.
- Continue modernising the existing C++/Qt project in small steps, rather than starting a rewrite ([#8](https://github.com/pingledingle-tech/PinglyMotion/issues/8)).
- Blender remains a modelling and rigging tool; PinglyMotion focuses on animation authoring and SL workflows.
- Validate real SL export and uploads, actual avatar behaviour, compatibility, and rights when using external models or motion.
- Distinguish working features, prototypes, planned tasks and ideas. Do not assume that a successful local preview is an SL upload.

## Maintaining this overview

Use the job files in [work/](work/README.md) for active jobs and coordination, issues for detailed requirements and progress, and pull requests for code under review. This document describes the product and user outcomes; update it when a decision or capability changes. Avoid inventing dates or treating exploratory ideas as shipped features.

The [README roadmap](../README.md#where-pinglymotion-is-going) and [IDEAS.md](IDEAS.md) contain earlier snapshots and should be reconciled when their active work allows it.
