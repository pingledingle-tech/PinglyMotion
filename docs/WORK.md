# Work board

Every job writes itself here before it starts. See `CLAUDE.md` for the rules.

**Merging:** when a job is done and all checks are green, it tells the project owner, who merges (or lets the job merge). See `CLAUDE.md`.

## Active jobs

| Branch | What | Files it changes | Pull request |
|---|---|---|---|

**Overlap:** none.

## Done

| Branch | What | Pull request |
|---|---|---|
| `ci/save-minutes` | Save Actions minutes: draft pull requests skip build, sanitizers and fuzz (privacy still runs); a new push cancels the older run of the same pull request | #157 |
| `feat/status-bar-88` | #88 step 8: one status bar at the bottom: "Tips on" switch (saved as the user setting `tips_on`, default on; tooltips come later), remote-control indicator (#43) and Aurora link status (off / connecting / live, dot turns the link on or off), rounded items in the theme colours; tests in `tests/statusbar/`; not Windows-tested yet | #155 |
| `feat/open-with-new-parser-62` | #62 part 2 of 4: opening a BVH/AVM file (`BVH::animRead`, `BVH::checkFile`) uses the new parser `src/animfile.*` and builds the same `BVHNode` tree as before (tested against the old parser for every round-trip file); errors name the line; old parser kept as `BVH::legacyAnimRead` until part 4 | #150 |
| `fix/head-neck-limits-129` | #129: head and neck bend -45..45 (X) instead of -37..22 (AAOS cervical flexion/extension); `SL.lim` regenerated, tests; all other limits unchanged | #156 |
| `fix/control-hardening-144-145` | #144: `control.json` written atomically (`QSaveFile`, owner-only from creation, every step checked), stale file removed at start, server does not start when the file cannot be written safely; #145: complete header blocks above 16 KB get 413, receive buffer capped at header + body limit; tests with simulated write failures and 20–120 KB headers | #148 |
| `docs/anim-format` | #63/#134 preparation: `docs/ANIM_FORMAT.md` describes the SL `.anim` format (byte layout, quantisation, BVH-to-anim conversion, upload limits, worked example, open questions) | #142 |
| `ci/fuzz-animfile` | Fuzz workflow runs `fuzz_animfile` (added in #140) like the other targets: BVH/AVM start corpus and `fuzz/animfile.dict`, same time per target; `fuzz/README.md` updated | #152 |
| `feat/new-parser-62` | #62 part 1 of 4: new BVH/AVM parser and plain data model (`src/animfile.*`) beside the old code, not used by the program yet; audit limits, tests that it reads every round-trip and corpus file like the old parser and refuses the audit's crafted files (`tests/animfile/`), fuzz target `fuzz_animfile` | #140 |
| `docs/security-audit-status` | Status table for every finding in `docs/SECURITY_AUDIT.md` (19 fixed, 3 partly, 2 open, 5 not applicable), checked on `main` with ASan/UBSan; new issues #143–#147 | #141 |
| `night-b-firestorm-joint-data` | #88 preparation: joint data from Firestorm in `bin/data/firestorm/` (friendly names, sections, mirror partners, `dontFlipOnMirror`, 9 hand presets stored as found; LGPL 2.1), `tools/make_firestorm_joints.py`, loader `src/jointinfo.*` and tests; no UI change, preset rotation conversion still open | #139 |
| `docs/work-order-formats` | File format order from #118 added to the agreed work order in `docs/PRODUCT.md`; #43 and #20 marked done | #136 |
| `fix/ik-solver` | #128: IK solves for the value just set; #127: new IK solver (damped least squares on the rotation channels, warm start, limit per update) – limbs stretch calmly when a goal is out of reach; IK results re-recorded, hip sweep test | #130 |
| `docs/github-pages-website` | Prepare the website for GitHub Pages | #104 |
| `docs/readme-product-home` | Refresh the project front page | #103 |
| `docs/refresh-ideas` | Refresh the idea bank and link to the product overview | #94 |
| `docs/product-vision-use-cases` | Product vision, user value, use cases and roadmap | #93 |
| `work-rules` | Rules and work board | #45 |
| `docs/ai-setup` | AI assistant setup guide | #39 |
| `feat/max-body` | Max avatar with fingers and face | #46 |
| `ci-push-main` | Build once per pull request, privacy check, commit rules | #48 |
| `auto-merge-rules` | Rules: jobs update their branch and turn on auto-merge | #49 |
| `docs-skip-build` | Text-only pull requests skip the build | #50 |
| `feat/joints-hover` | Show/hide joint groups, hover highlight with joint name | #51 |
| `release-0.3.0-beta` | Version 0.3.0, beta releases, Linux AppImage | #52 |
| `fix/windows-test` | Fixes from the Windows test (#53–#56) | #57 |
| `qt-6.12` | Qt 6.12.0 LTS in the builds (aqtinstall pinned to a commit) | #58 |
| `feat/gl33-renderer` | Renderer step 1 (#61): body drawn with OpenGL 3.3 shaders, old path as fallback | #72 |
| `ci/codeql-text-skip` | CodeQL: text-only pull requests pass quickly (required checks no longer wait) | #74 |
| `feat/gpu-skinning` | Renderer step 2 (#61): skinning and joint colours on the GPU, `tools/compare_renderers.py` | #73 |
| `fuzz/prp-control` | Fuzzing of `.prp` prop files and the control commands (#20) | #75 |
| `feat/color-picking` | Renderer step 3 (#61): picking by colour ID on the GPU, `GL_SELECT` only as fallback | #76 |
| `fix/codeql-pick-size` | CodeQL: pick area size computed as qsizetype (alerts from #76) | #79 |
| `feat/nicer-shading` | Renderer step 4 (#61): nicer body shading; `PINGMATOR_CLASSIC_SHADING` keeps the old look | #80 |
| `feat/light-sliders` | Options > Light…: brightness, shine and warmth sliders for the body light | #81 |
| `feat/drop-glu` | Renderer step 5a (#61): no GLU calls any more (link to GLU removed later with `src/CMakeLists.txt`) | #82 |
| `feat/mesh-shader` | Renderer step 5b (#61): Bento balls, props and handles drawn with the shader and the new light | #83 |
| `feat/lines-rings-floor` | Renderer step 5c (#61): rotation rings, lines and floor drawn with the shader | #84 |
| `feat/floor-shadow` | Renderer (#61): shadow of the figure on the floor | #85 |
| `feat/soft-shadow` | Renderer (#61): soft floor shadow, on/off in Options > Light… | #86 |
| `chore/remove-dead-code` | Removed functions that are never called, the disabled playback timer and the GLU link (#61) | #87 |
| `chore/remove-legacy-gl` | Removed the old OpenGL path (fixed function drawing, GL_SELECT, CPU skinning, classic light, compare tool); OpenGL 3.3 required (#61) | #89 |
| `feat/aurora-link` | Live link to Aurora over Aurora's LiveMesh (auth + pose-bvh; closes #11–#13) | #78 |
| `test/roundtrip-corpus` | Round-trip tests for all BVH/AVM files, BVH/AVM start corpus for the new parser (#62); found #91 | #90 |
| `chore/own-matrix-stack` | Clean-up step 3a (#61): own matrix stack and drawing state (`src/scenestate.h`); picking through the shaders | #92 |
| `feat/core-profile` | Clean-up step 3b (#61): OpenGL 3.3 core context; lines drawn as strips by a geometry shader | #96 |
| `fix/loop-points` | #91: "no loop points" (-1) kept and loop out kept inside the animation when a file is opened | #100 |
| `docs/work-order` | Agreed work order (16 Nov) added to `docs/PRODUCT.md` | #101 |
| `docs/rendering-and-link` | New docs: `docs/RENDERING.md` (how the 3D view is drawn) and `docs/LIVE_LINK_RESEARCH.md` (why the Aurora link uses LiveMesh) | #102 |
| `feat/aurora-quaternions` | Aurora link: send the pose as quaternions per joint when Aurora supports it | #95 |
| `docs/refresh-after-quaternions` | Docs brought up to date after #78, #95 and the Qt 5 decision; duplicate download section removed from the README | #106 |
| `chore/renderer-leftovers` | #99: unused sphere/cone.obj removed, INSTALL without GLU and with OpenGL 3.3, `/*` comment warning fixed, temporary block in `animationmanager.h` re-labelled (still used by `Scene`) | #107 |
| `ci/drop-glu` | #99 CI part: GLU no longer installed in the workflows (the Qt 5 job gets `libgl-dev` instead) | #108 |
| `chore/drop-qt5` | #98: Qt 5 dropped (CMake, `linux-qt5` job), deprecated Qt 6 calls and the `auroralink.h` member order fixed; no compiler warnings. Windows-tested | #109 |
| `feat/control-indicator` | #43: remote-control indicator in the status bar (grey/yellow/green/red dot, client name, last command), MCP heartbeat. Windows-tested | #113 |
| `docs/no-personal-names` | Privacy rule in `CLAUDE.md`: no personal names, contact details or personal paths anywhere (issues, pull requests, comments, files) | #123 |
| `ci/privacy-text` | Privacy check also reads pull request titles/descriptions (`privacy` check) and new or edited issues, comments and reviews (`Privacy text` workflow); tests in `tests/test_check_privacy.py` | #131 |
| `feat/joint-tabs` | #88 step 1: joint table with tabs Body / Face / Hands / Other instead of the "Edit Part" drop-down (list only, the 3D view is unchanged) | #126 |
| `docs/posing-ux` | #88: posing UX plan with mockup pictures (`docs/POSING_UX.md`) | #135 |
| `docs/combined-plan` | Combined plan in `docs/PRODUCT.md`: posing track (#88) and file format track side by side | #137 |
| `docs/posing-notes` | Posing notes from the first AI-made animation (a bow) and a plan for teaching assistants to animate | #115 |
| `chore/qt-maths-ik` | #97: IK on Qt maths instead of libquat, libquat removed, IK tests. Windows-tested | #112 |
| `chore/private-repo` | Private repository: CodeQL and dependency review skipped while private; merge rules without auto-merge (owner merges, or lets the job merge, when checks are green) | #124 |
| `fix/ik-rotation-order` | #116: IK composes rotation channels in the same order as the 3D view; tests that IK and the view agree. Windows-tested; found #127, #128 | #125 |
| `docs/reuse-references` | Parked notes: viewer code and tools that may be reused later (`.anim` format, Firestorm poser, PoseForge ideas) | #114 |
| `fix/joint-limits` | #59: joint limits from the SL wiki (locked axes ±5), finger range of motion (bend, spread ±25, twist ±15), tests; IK results re-recorded | #111 |
| `docs/release-notes-0.4` | Draft release notes for 0.4.0-beta (`docs/RELEASE_NOTES_0.4.0.md`); no tag, release or version change | #132 |
