# Roadmap

The order of the work and one status word for each step. Nothing else: no build history. Order confirmed by the project owner on 22 November 2026, 12:19 («Ja»).

Where the details are: running work is the open pull requests; what a job did is the report comment on its merged pull request; the queue is the jobs list (issue 74); the owner's tests are the test list (issue 47); what the program is for is [PRODUCT.md](PRODUCT.md); decisions and reasons are in [DECISIONS.md](DECISIONS.md).

**Rule:** a job changes this file only when the order or the status word of a step changes, never to log what was built (`CLAUDE.md`, "While you work").

Status words: done, running, next, waiting, parked.

## Order

| # | Step | Status | Details |
|---|---|---|---|
| 1 | GitHub setup and process (stable tests, `auto-merge` label, process review) | running | [DEV.md](DEV.md), [DECISIONS.md](DECISIONS.md), "Working method and repository" |
| 2 | RLV tool (Script Lab): functions, lint, whole-tool tests, the panel as in the mockup. Takes priority over step 4 (the project owner, 23 Nov 2026). The panel is built; the owner's test of what needs a person is next | running | [SCRIPT_LAB.md](SCRIPT_LAB.md), [SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md), [UX_DECISIONS.md](UX_DECISIONS.md) 6d |
| 3 | Website: text and content, the download page (issue 147); runs in parallel, touches only website files | running | issue 147 |
| 4 | New UX design in the rest of the program, built step by step from the clickable mockup, including the UX direction of 24 Nov 2026 (windows, helper mode, beginner animation view) | running | [UX_DECISIONS.md](UX_DECISIONS.md), `docs/DESIGN_CHARTER.md`, `docs/DESIGN_SYSTEM.md`, [DECISIONS.md](DECISIONS.md) |
| 5 | Easing per joint (issue 11) | done | [EASING.md](EASING.md) |
| 6 | Reference library browser (issue 19) and two figures in one scene | done (the look waits for the combined review) | [UX_DECISIONS.md](UX_DECISIONS.md) 6l, 6m |
| 7 | Teaching texts (issue 54), once the texts are stable | waiting | [TEACHING.md](TEACHING.md) |
| 8 | Translations and the language choice in Settings (the plumbing is built, no translation exists) | waiting | [TRANSLATION.md](TRANSLATION.md) |

## Script Lab: folder, editor, converter

Order of the track (the project owner, 23 Nov 2026). Details in [SCRIPT_LAB.md](SCRIPT_LAB.md); converter rules in [SLUA_RULES.md](SLUA_RULES.md).

| Part | Status |
|---|---|
| A. Definition check against Linden Lab's lists | done |
| B. The folder is the object (core) | done |
| C. RLV type check | done |
| D. The panel (tree, Live page, maker guide) | done |
| E. Props as prims, menus, record and replay, who talks to whom, chain lines, the player | done |
| F. The editor in our own text box (F1, F2, F3) | done |
| R3 and R4. The editor uses the RLV table | done |
| G. Convert LSL to SLua, level 1 (library, no panel yet) | done |
| Luau trial build, convert level 2, Lua in the tool | parked until last ([DECISIONS.md](DECISIONS.md), "Lua comes last") |

A change to a parser or the control commands needs a green fuzz run.

## Web version

Decided, time not set (the project owner, 24 Nov 2026, 12:10). No web job runs. Until then new code is web-ready ([DEV.md](DEV.md), "Web-ready code"); the decision is in [DECISIONS.md](DECISIONS.md), Technology, 24 Nov 2026. Status: waiting.

## Parked, later and not doing

Face (Bento face joints) is parked. A neutral body, Windows code signing (issue 146) and the Aurora track are later and not planned. The reasons and the "not doing" list are in [DECISIONS.md](DECISIONS.md), "Later, parked and not doing".
