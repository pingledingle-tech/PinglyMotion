# Documents of PinglyMotion

One overview of every document in the repository: what it is, who it is for, and whether it is **current** guidance, **reference**, a **plan** or **history**. Audience: **user** (a person using the program), **developer** (a person changing the code), **AI session** (a Claude session working on this repository), **AI client** (an assistant connected to the running program).

## Which document wins

When documents disagree, use this order:

1. **Process** (jobs, pull requests, merging, privacy): [CLAUDE.md](../CLAUDE.md).
2. **Look and behaviour of the program:** [DESIGN_CHARTER.md](DESIGN_CHARTER.md), then [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md), then [UX_DECISIONS.md](UX_DECISIONS.md), then older plans ([POSING_UX.md](POSING_UX.md)) and mockup pictures.
3. **Numbering of interface steps** ("UX step N"): [UX_DECISIONS.md, "Build order"](UX_DECISIONS.md#9-build-order).
4. **Words shown to users:** the glossary in [TEACHING_TEXTS.md](TEACHING_TEXTS.md#glossary-the-words-to-use).

Progress (what is done, what is next) is in [ROADMAP.md](ROADMAP.md); running work is the open pull requests, the job queue is the jobs list (issue 74) and the owner's tests are the owner test list (issue 47), on GitHub, not in files.

## Start here

| Document | What it is | For | Status |
|---|---|---|---|
| [../README.md](../README.md) | What PinglyMotion is, what you can do, tests, credits | user, developer | current |
| [../CLAUDE.md](../CLAUDE.md) | Rules for AI sessions: jobs, the job queue, pull requests, merging, commits, privacy, where notes go between chats | AI session | current guidance |
| [DEV.md](DEV.md) | Build and test commands, tricks for sessions, how to test a visible change | developer, AI session | current guidance |
| [PRODUCT.md](PRODUCT.md) | Product vision, user value, use cases, proposed direction | developer, AI session | plan |

## For users

| Document | What it is | For | Status |
|---|---|---|---|
| [FIRST_ANIMATION.md](FIRST_ANIMATION.md) | Make a short arm movement in about ten minutes | user | current guidance |
| [BLENDER.md](BLENDER.md) | Working with Blender: export BVH and glTF, axes, the Second Life skeleton | user | current guidance |
| [VIDEO_GUIDE.md](VIDEO_GUIDE.md) | From a video of yourself to a Second Life animation with XR Animator | user | current guidance |
| [TEXT_TO_MOTION_GUIDE.md](TEXT_TO_MOTION_GUIDE.md) | Make a motion from a sentence with the optional Kimodo helper (not yet tried on a real PC) | user | current guidance |
| [AURORA_LINK.md](AURORA_LINK.md) | Live link that shows your pose on your avatar in the Aurora viewer | user | current guidance |
| [index.html](index.html) | The project website (homepage) | user | current |
| [download.html](download.html) | Website page: download (early preview), uninstall and system changes | user | current |
| [privacy.html](privacy.html) | Website page: privacy policy (what the program, the helper and the website do with data) | user | current guidance |
| [SECURITY_PRIVACY.md](SECURITY_PRIVACY.md) | Security and privacy in plain words: what goes over the internet, what a connected AI can see, each claim marked tested or not tested, honest limits; a kept place for the legal text | user, developer | current guidance |
| [site.css](site.css), [site.js](site.js) | Shared style, language picker and light/dark switch of the website pages | developer | current |

## Formats and reference

| Document | What it is | For | Status |
|---|---|---|---|
| [DINGLE_FORMAT.md](DINGLE_FORMAT.md) | Specification of `.dingle`, the working format | developer | reference |
| [ANIM_FORMAT.md](ANIM_FORMAT.md) | Second Life's `.anim` format, from the viewer source | developer | reference |
| [RETARGET_MAPPING.md](RETARGET_MAPPING.md) | The `.jointmap` file for hand-made joint mappings | developer | reference |
| [REFERENCE_LIBRARY_SCHEMA.md](REFERENCE_LIBRARY_SCHEMA.md) | Format of the reference library index | developer | reference |
| [CONTROL.md](CONTROL.md) | Remote control interface (HTTP, JSON, MCP): every command | AI client, developer | reference |
| [ASSISTANT_GUIDE.md](ASSISTANT_GUIDE.md) | How an assistant animates in PinglyMotion: Second Life rules, words to use, recipes (served by the `read_guide` tool) | AI client | current guidance |
| [RENDERING.md](RENDERING.md) | How the 3D view is drawn (OpenGL 3.3 shaders) | developer | reference |
| [TRANSLATION.md](TRANSLATION.md) | How the program's texts are translated: add a language, the checks, what is not translated | developer, AI session | current guidance |
| [DECISIONS.md](DECISIONS.md) | Decision log with reasons and what was turned down | developer, AI session | reference |
| [MIGRATION.md](MIGRATION.md) | The move to this repository, and old issue numbers mapped to new ones | developer, AI session | reference |
| [SCRIPT_LAB.md](SCRIPT_LAB.md) | What the Script Lab (RLV and LSL sandbox) does today, with limits | user, developer, AI client | current guidance |
| [RLV_OPTION_TYPES.md](RLV_OPTION_TYPES.md) | What the option of each RLV command must be, how RLVa checks it and what the Script Lab does with a bad value | developer, AI client | current guidance |

## Design

| Document | What it is | For | Status |
|---|---|---|---|
| [DESIGN_CHARTER.md](DESIGN_CHARTER.md) | Top authority for how the program looks and behaves; principles P1 and on | AI session, developer | current guidance |
| [DESIGN_SYSTEM.md](DESIGN_SYSTEM.md) | Exact colours, shapes, icons and sizes | AI session, developer | current guidance |
| [design_tokens.json](design_tokens.json) | The same values for code; must agree with DESIGN_SYSTEM.md | developer | reference |
| [UX_DECISIONS.md](UX_DECISIONS.md) | Timeline and poser as one design; the "Build order" of interface steps | AI session, developer | current guidance |
| [POSING_UX.md](POSING_UX.md) | Older posing interface plan: mouse, keys, mockup pictures (`images/posing-ux/`) | developer | plan (older; UX_DECISIONS wins) |
| [mockup/index.html](mockup/index.html) | Clickable picture of the planned interface (not the program) | developer | plan |
| [TEACHING_TEXTS.md](TEACHING_TEXTS.md) | Inventory of texts in the program, voice guide, the glossary | developer, AI session | current guidance |
| [TEACHING.md](TEACHING.md) | Register of the 45 beginner texts (welcome card, tips, hints) | developer, AI session | plan (approved wording) |
| [EASING.md](EASING.md) | Names, pictures and descriptions of the six easing choices | developer | current guidance |

## Process and development

| Document | What it is | For | Status |
|---|---|---|---|
| [../CLAUDE.md](../CLAUDE.md) | See "Start here" | AI session | current guidance |
| [work/README.md](work/README.md) | Work board: one job file per job, format and the `tools/work.py` script (also holds the job files, `HISTORY.md` and `archive/` of finished jobs) | AI session | current guidance |
| [WORK.md](WORK.md) | Short pointer to the work board (the old shared table is gone) | AI session | history |
| [../fuzz/README.md](../fuzz/README.md) | Fuzz targets, corpora and how to run them | developer | reference |
| [CODE_AUDIT.md](CODE_AUDIT.md) | Review of `src/` on 20 November 2026 (cleanup issues) | developer | history (report) |
| [CODE_ANALYSIS.md](CODE_ANALYSIS.md) | Tool results (coverage, analysers) on 21 November 2026 | developer | history (report) |

## Plans and roadmap

| Document | What it is | For | Status |
|---|---|---|---|
| [ROADMAP.md](ROADMAP.md) | Order of the open work and the one place for progress status | AI session, developer | plan |
| [IDEAS.md](IDEAS.md) | Possibilities, not promises | developer, AI session | plan |
| [SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md) | Plan and steps of the Script Lab | AI session, developer | plan |
| [RLV_TOOLS_REPO.md](RLV_TOOLS_REPO.md) | Description of a separate repository for the owner's RLV tools (not created) | AI session | plan |
| [REFERENCE_LIBRARY_PLAN.md](REFERENCE_LIBRARY_PLAN.md) | Plan and licence readings for the reference library | developer | plan |

## Security

| Document | What it is | For | Status |
|---|---|---|---|
| [../SECURITY.md](../SECURITY.md) | Supported versions and how to report a vulnerability | user, developer | current guidance |
| [SECURITY_AUDIT.md](SECURITY_AUDIT.md) | Security audit of 14 November 2026 with status per finding | developer | history (with status) |
| [SECURITY_AUDIT_21NOV.md](SECURITY_AUDIT_21NOV.md) | Security audit of 21 November 2026 | developer | history (with status) |
| [../CREDITS.md](../CREDITS.md) | Projects, data and tools we use or learned from, with licences | user, developer | current guidance |

## Research

Reports with sources read on the date they state; prices and licences change.

| Document | What it is | For | Status |
|---|---|---|---|
| [research/TEXT_TO_MOTION.md](research/TEXT_TO_MOTION.md) | Text-to-motion AI and Second Life (issue 18) | developer, AI session | reference |
| [research/VIDEO_TO_MOTION.md](research/VIDEO_TO_MOTION.md) | Animations from video (issue 17) | developer, AI session | reference |
| [research/LICENCE_PRIVATE_USE.md](research/LICENCE_PRIVATE_USE.md) | Licence check of those tools for private, non-selling use (not legal advice) | developer, AI session | reference |

## Archive

- [archive/](archive/README.md): finished, superseded or never-planned documents, kept for history. Nothing there is current guidance.
- [work/archive/](work/README.md): job files of finished jobs (see the work board's History section).
