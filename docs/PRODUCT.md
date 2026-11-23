# PinglyMotion: product vision and roadmap

This document brings together the product vision, user value, use cases and proposed direction. It complements [IDEAS.md](IDEAS.md), the work board in [work/README.md](work/README.md) and the linked issues. Progress status (what is done, what is next) is kept in one place: [ROADMAP.md](ROADMAP.md), with the open pull requests for running work and the jobs list ([#74](https://github.com/pingledingle-tech/PinglyMotion/issues/74)) for the job queue. This document does not repeat it. The project restarted in this repository on 18 November 2026: a number written "old #N" is an item in the old repository, see [MIGRATION.md](MIGRATION.md) for the moved items and [DECISIONS.md](DECISIONS.md) for the decisions. The sequence below is a proposal, not a commitment, schedule or replacement for existing technical decisions.

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
- **Status:** Core manual editing exists. Live preview in Aurora works (old #78, old #95; tested on Windows with the Aurora fork). End-to-end export and SL upload were validated in Firestorm on 21 November 2026 (export, upload, playback, loop, slow start and finish, hands, easing, BVH route, re-import).
- **Pass when:** a sit is adjusted, previewed on the avatar in Aurora, saved as `.dingle`, exported as `.anim`, uploaded and plays in SL as it looked in PinglyMotion. Fail if the upload is refused or the pose in SL differs.
- **Issues:** [#14](https://github.com/pingledingle-tech/PinglyMotion/issues/14), [#159](https://github.com/pingledingle-tech/PinglyMotion/issues/159)

### 2. Create an AO stand or sit

- **Journey:** Create a pose, add and adjust keys, refine the loop, export and use it in an AO.
- **Value:** Create personalised everyday movement without a paid authoring tool.
- **Needs:** Reliable editing, loop points, SL export and clear instructions. An AO of original work is an idea in [IDEAS.md](IDEAS.md); ownership of imported source material must be checked separately.
- **Pass when:** a looping stand made in PinglyMotion uploads, loops between its loop points without a jump, and overrides the lower-priority default stand when used in an AO.
- **Issues:** [#159](https://github.com/pingledingle-tech/PinglyMotion/issues/159), [#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11), [#53](https://github.com/pingledingle-tech/PinglyMotion/issues/53)

### 3. Make and refine a dance

- **Journey:** Create or import movement, adjust keyframes and transitions, preview and export.
- **Value:** Reuse movement and make timing changes without starting again.
- **Explore:** Reusable sequences, seamless looping, curves and possible BPM guides ([#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11)). Video-to-motion and complete AI-generated dances are research ideas, not established capabilities.
- **Pass when:** a dance of up to 60 seconds with eased keys exports, uploads and plays in SL with the same timing and easing as in PinglyMotion.
- **Issues:** [#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11), [#53](https://github.com/pingledingle-tech/PinglyMotion/issues/53), [#159](https://github.com/pingledingle-tech/PinglyMotion/issues/159)

### 4. Preview on the user's avatar

- **Journey:** Connect PinglyMotion to Aurora, adjust a pose or frame and see the avatar locally in the viewer.
- **Value:** Judge fit on the actual avatar before uploading.
- **Status and limit:** Works today, see [AURORA_LINK.md](AURORA_LINK.md). The pose is sent as quaternions per joint when Aurora supports it (old #95), otherwise as BVH (old #78). The preview is local only: nothing is uploaded and others do not see it. It needs Aurora built from the fork until the change is proposed upstream. Synchronised playback and avatar shape are future ideas.
- **Pass when:** changing a joint in PinglyMotion shows the same pose on the avatar in Aurora, without an upload. Done (old #78, old #95, [#23](https://github.com/pingledingle-tech/PinglyMotion/issues/23), [#64](https://github.com/pingledingle-tech/PinglyMotion/issues/64)); synchronised playback has no issue yet.

### 5. Ask an AI to help with a pose

- **Journey:** Enable local control, describe a pose, let the assistant use joint and frame commands, inspect a screenshot, and adjust.
- **Value:** A lower barrier for starting and iterating, while the user keeps control.
- **Status:** Local HTTP and MCP commands exist; setup and available commands are in [CONTROL.md](CONTROL.md). Every assistant can read the [assistant guide](ASSISTANT_GUIDE.md) with the `read_guide` tool (Second Life rules, joint names, which way each turn goes, typical poses, a work rhythm). More advanced motion generation, visual assessment and collision or foot-contact analysis need investigation. The status bar shows whether an assistant is connected (old #43, done). `PinglyMotion.mcpb` installs the MCP server in the Claude app (Settings > Extensions > Advanced settings > Install Extension...); it runs the Node server, so no Python. It is the only MCP server: the old Python server was removed (#7).
- **Pass when:** an assistant connected over MCP sets a pose from a plain description, the user can see and correct it by hand, and the program still works with the assistant disconnected.
- **Issues:** [#7](https://github.com/pingledingle-tech/PinglyMotion/issues/7), [#129](https://github.com/pingledingle-tech/PinglyMotion/issues/129)

### 6. Bring an external animation to SL

- **Journey:** Import or convert an animation, map and retarget its skeleton to Bento, review the result and export.
- **Value:** A possible free bridge from Blender and other sources to SL.
- **Status:** BVH import from another skeleton and glTF import exist ([#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16), [#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12)); the import window has clean-up choices (30 frames a second, start pose at frame 1, in place, smooth loop end, light smoothing) that are remembered between imports ([#17](https://github.com/pingledingle-tech/PinglyMotion/issues/17)); foot lock is not built yet; Blender interoperability is later work ([#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13)). FBX/Maya conversion paths and retargeting need investigation; direct support is not promised.
- **Pass when:** a BVH from another skeleton or a glTF clip imports with a clear list of matched and left-out joints, plays on the SL skeleton, and exports and uploads like a native animation.
- **Issues:** [#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16), [#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12), [#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13), [#159](https://github.com/pingledingle-tech/PinglyMotion/issues/159)

### 7. Turn ordinary video into movement

- **Journey:** Use an open tool to estimate motion from video, adapt it to Bento and edit the result.
- **Value:** Explore making dances and other motion without a mocap suit.
- **Status:** Idea only. Accuracy, missing depth, cleanup, rights and the integration workflow remain to be evaluated.
- **Pass when (research):** a short clip from a free tool becomes an editable animation on the Bento skeleton, with its licence and rights checked.
- **Issues:** [#17](https://github.com/pingledingle-tech/PinglyMotion/issues/17)

### 8. Make an animation usable in an object

- **Journey:** Export an animation and use it in an AO, poseball or furniture object, potentially with a generated LSL template.
- **Value:** Help users take finished animation into an SL object.
- **Status:** LSL templates and reusable libraries are ideas. LSL controls animations; it is not the animation authoring format. A full LSL IDE is outside the stated scope. The bounded exception is Script Lab, a sandbox for testing RLV and LSL scripts, with no built-in script editor ([SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md)).
- **Pass when:** an uploaded animation plays from an object with `llStartAnimation`. LSL templates and libraries have no issue yet ([IDEAS.md](IDEAS.md), [#19](https://github.com/pingledingle-tech/PinglyMotion/issues/19)).
- **Issues:** [#159](https://github.com/pingledingle-tech/PinglyMotion/issues/159), [#19](https://github.com/pingledingle-tech/PinglyMotion/issues/19)

## Product areas and current direction

| Area | Direction | Tracking |
| --- | --- | --- |
| Posing | Grouped body/face/hand joints, hand presets, finer adjustment, visibility, mirroring and whole-pose controls. | [#14](https://github.com/pingledingle-tech/PinglyMotion/issues/14), old #59 joint limits |
| Timeline and motion | Keyframes, transport, zoom, interpolation, easing, looping and reusable sequences. | [#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11) |
| Aurora | Live preview with quaternion poses; playback synchronisation, avatar shape and an upstream proposal to Aurora later. | [AURORA_LINK.md](AURORA_LINK.md), old #78, old #95 |
| AI and openness | Improve the existing local API and MCP. Avoid tying the editor to a single assistant. Investigate easier installation and client status. | [CONTROL.md](CONTROL.md), [#7](https://github.com/pingledingle-tech/PinglyMotion/issues/7) |
| Files and SL export | Preserve existing BVH/AVM work, replace the parser and data model with regression and fuzz tests, a new working format `.dingle` ([#26](https://github.com/pingledingle-tech/PinglyMotion/issues/26)), then native .anim export. | [#9](https://github.com/pingledingle-tech/PinglyMotion/issues/9), [#10](https://github.com/pingledingle-tech/PinglyMotion/issues/10), [#26](https://github.com/pingledingle-tech/PinglyMotion/issues/26) |
| Import and retargeting | Explore glTF, Blender, an own rigged avatar and an SL Bento bridge. | [#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12), [#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13) |
| New motion sources | Ordinary video and Mesh2Motion, subject to technical and licence checks. Eight CC0 Mesh2Motion clips are already test motions (in the Windows zip: `examples/mesh2motion`), credited in Help > About and the README. | [IDEAS.md](IDEAS.md) |
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

This replaces the numbered work order of 16 November. The order of stages above still holds: posing comes before the parser, and the new parser keeps today's interface to the rest of the program. File formats were decided in old #118; the posing interface in [POSING_UX.md](POSING_UX.md) ([#14](https://github.com/pingledingle-tech/PinglyMotion/issues/14)). Since 21 November 2026 the [design charter](DESIGN_CHARTER.md) is the top authority for the look and behaviour of the whole program and wins where older UX documents disagree.

What is done and what is next in each step is in [ROADMAP.md](ROADMAP.md); per pull request, in the open pull requests and the report comment on each merged one; the job queue is in the jobs list ([#74](https://github.com/pingledingle-tech/PinglyMotion/issues/74)).

The work runs in **two tracks side by side**. They mostly change different files: the posing track the window, panels and 3D view; the format track the parser, the data model and file import/export. Where a step in one track needs a file another active job has, the work board ([work/README.md](work/README.md)) decides who waits. Within a track, one step at a time, each tested on Windows before it is merged.

**Track 1 – posing ([#14](https://github.com/pingledingle-tech/PinglyMotion/issues/14)).** The one numbered list of interface steps, with the old numbers of this list, is in [UX_DECISIONS.md, "Build order"](UX_DECISIONS.md#9-build-order); the parts in order:

- Camera and keys: one scheme for new users, Blender and Second Life habits; old joint modifier keys removed. Beside it: a data file from Firestorm with joint names, groups, left/right pairs and hand presets (LGPL notice), and the status bar (Claude, Aurora, tips switch).
- Friendly names, section headings and search in the joint list.
- Floating, movable panels; Tab hides them; reset layout. Plan: [PANELS_PLAN.md](archive/PANELS_PLAN.md) (jobs P1 to P6).
- Selected joint panel: X / Y / Z sliders with the allowed range, the near-limit warning with "Turn limits off", Turn / Move, and the empty cards (values and texts built in UX phase 2b, [UX_DECISIONS.md, 6](UX_DECISIONS.md#6-selected-joint-panel); its look and the rings and dragging in the 3D view come later).
- Left / Right / Mirror and the whole-pose buttons.
- **Milestone:** adjust a sitting pose, preview it in Aurora, save it and upload it in SL (validated in Firestorm on 21 November 2026, [#159](https://github.com/pingledingle-tech/PinglyMotion/issues/159)).
- The remaining posing steps: Hands tab, timeline, help tips and hint bar, command search, pose library and export selection, settings, visual polish and own icon, 3D view follows the tab.

**Track 2 – file formats (old #118)**

1. New parser and data model with fuzzing ([#9](https://github.com/pingledingle-tech/PinglyMotion/issues/9)); reading and saving go through `animfile.*`.
2. `.anim` export and easing ([#10](https://github.com/pingledingle-tech/PinglyMotion/issues/10), [#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11), [#53](https://github.com/pingledingle-tech/PinglyMotion/issues/53)). Menu: File > Export for Second Life (.anim), a plain dialog with priority, slow start and finish, loop and hands; Save stays `.dingle` only. A key can carry one of six easing choices, stored as `curve` in `.dingle` and baked into the export. A pose change can also get a custom speed curve, drawn in a small dialog ("Custom curve..." next to the easing tiles, [#11](https://github.com/pingledingle-tech/PinglyMotion/issues/11) step 1), stored and baked the same way. Real upload and playback in SL: [#159](https://github.com/pingledingle-tech/PinglyMotion/issues/159).
3. `.anim` import ([#22](https://github.com/pingledingle-tech/PinglyMotion/issues/22)): opens the file as a new unsaved animation and lists what could not be imported.
4. BVH from any skeleton, retargeted to the SL skeleton ([#16](https://github.com/pingledingle-tech/PinglyMotion/issues/16)): shows which joints were matched and which were left out (and why), with a switch for T-pose or A-pose starts; a hand-made mapping per source skeleton in a small `.jointmap` file ([RETARGET_MAPPING.md](RETARGET_MAPPING.md)), found again by a fingerprint of the skeleton, edited in a dialog.
5. glTF/GLB import ([#12](https://github.com/pingledingle-tech/PinglyMotion/issues/12)), then Blender ([#13](https://github.com/pingledingle-tech/PinglyMotion/issues/13), docs first). No glTF export; FBX only on real demand. The own working format is `.dingle` ([#26](https://github.com/pingledingle-tech/PinglyMotion/issues/26), [DINGLE_FORMAT.md](DINGLE_FORMAT.md)); AVM is no longer written, old `.avm` files still open and are saved as `.dingle`, an opened `.bvh` is left alone.

**Small jobs in between** when a track waits for a Windows test.

**Later:** easy setup with an AI assistant ([#7](https://github.com/pingledingle-tech/PinglyMotion/issues/7)) once the posing interface has settled; larger AI features and libraries as small experiments ([#17](https://github.com/pingledingle-tech/PinglyMotion/issues/17)–[#20](https://github.com/pingledingle-tech/PinglyMotion/issues/20)). Manual use always works without AI.

## Product principles and decisions

- Keep the focus on Second Life and Bento, with manual use available without AI or Aurora.
- Continue modernising the existing C++/Qt project in small steps, rather than starting a rewrite ([#8](https://github.com/pingledingle-tech/PinglyMotion/issues/8)).
- Blender remains a modelling and rigging tool; PinglyMotion focuses on animation authoring and SL workflows.
- Validate real SL export and uploads, actual avatar behaviour, compatibility, and rights when using external models or motion.
- Distinguish working features, prototypes, planned tasks and ideas. Do not assume that a successful local preview is an SL upload.

## Maintaining this overview

Use the job files in [work/](work/README.md) for active jobs and coordination, issues for detailed requirements, pull requests for code under review, and [ROADMAP.md](ROADMAP.md) for progress status. This document describes the product and user outcomes; update it when a decision or capability changes. Avoid inventing dates or treating exploratory ideas as shipped features.

The [README roadmap](../README.md#where-pinglymotion-is-going) and [IDEAS.md](IDEAS.md) contain earlier snapshots and should be reconciled when their active work allows it.
