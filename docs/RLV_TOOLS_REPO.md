# A separate repository for the project owner's RLV tools (description)

Status: **description only** (Script Lab step 8, issue 294). The repository does not exist and is not created by this page; creating it needs the project owner's go-ahead. This page is the description to review. It is a proposal: names and file formats below can change when the repository is made. Context: [SCRIPT_LAB.md](SCRIPT_LAB.md), [SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md).

## Why a separate repository

The scripts the project owner writes with Claude Code (a HUD, restraints, helpers) are his own work and have their own history and licence. They do not belong in PinglyMotion's repository, which holds the program. PinglyMotion only reads such a repository as a folder on disk (the Live page's watch, or `lab_watch`, [CONTROL.md](CONTROL.md)).

## What exists in PinglyMotion today, and what does not

Exists (merged): the RLV core, the LSL interpreter, `lsl_check`, `lsl_run`, `lsl_event`, the folder watch with `lab_watch`, `lab_unwatch` and `lab_result`, and the control interface that starts with `PinglyMotion --control`. Examples that ship with the program are in `examples/scriptlab/`.

Exists since job A3 (record and replay, pull request 450): a first `.rlvtest` reader and runner, for the recipe `recipe.rlvtest` in a watched object folder ([SCRIPT_LAB.md](SCRIPT_LAB.md), "Record and replay"; `src/scriptlab/labrecipe.*`, the control command `lab_replay`, the fuzz target `fuzz_recipe`). It reads `#`, `object` and `send` lines as proposed below, adds `touch`, `menu` and `wait`, and answers in plain facts and changes since last time. Does not exist yet: `expect` lines (they are refused), a pass or fail result, and a runner for a whole folder of scenarios. The CI part of this description therefore still needs one small technical job in PinglyMotion first (see "What has to be built first").

## Proposed layout

```
rlv-tools/                 (name to be chosen by the owner)
  README.md                what the repository is, how to open it in Script Lab
  LICENSE
  scripts/                 the .lsl files, one folder per product (for example hud/, cuffs/)
  scenarios/               .rlvtest files, one folder per product, mirroring scripts/
  examples/                small teaching scripts and scenarios (copied from examples/scriptlab/ at first)
  template/                an empty script and an empty scenario to copy from
  .github/workflows/       the scenario run (below)
```

- The repository is a plain folder of `.lsl` files, so Script Lab's watch (at most 200 scripts of at most 262 144 bytes, 4 folder levels) can read it as it is. Folder names that start with a dot are skipped by the watch.
- No build step and no generated files: what is in `scripts/` is what is pasted into Second Life.
- The template and the examples hold no personal data and no names of products owned by others.

## The scenario file `.rlvtest` (proposal)

A scenario is a short text file: lines that put something in (an RLV command, a chat line, a touch) and lines that say what must be true afterwards. Plain text so that Claude can write it and a diff is readable. A first sketch, not a fixed format:

```
# cuffs lock the wrists
object cuffs
send  @detach=n
expect blocked detach
send  @clear
expect not blocked detach
```

The runner is meant to use only what the control interface already offers (`rlv_reset`, `rlv_command`, `rlv_state`, and for scripts `lsl_run` with `inputs`, `lsl_event`), so a scenario can say the same things an AI client says by hand today. What a scenario can check is limited to what those commands answer: restrictions held, allowed and blocked actions, the log of a run, replies on a channel. Everything the sandbox cannot do (teleport, sitting, inventory, the look of anything) cannot be checked and is not claimed.

## Running scenarios in CI

Idea: a workflow in the tools repository downloads a released PinglyMotion build, starts it with `--control` (on Linux under Xvfb, as PinglyMotion's own CI does, [DEV.md](DEV.md)), and runs every file in `scenarios/` through the runner, which exits with an error on the first failed expectation. What this needs and has not been checked: a Linux build of PinglyMotion that can be downloaded in CI (releases exist as a Windows zip and an AppImage; whether the AppImage runs headless on a hosted runner is not tested), and the runner itself. This is why the description says "headless in CI" only as a goal. A scenario that passes is a sandbox result, never a Second Life result: the final test is always in Second Life.

## What has to be built first (separate jobs, each started on the owner's word)

1. The `.rlvtest` format and its runner in PinglyMotion (a small technical job, a command line tool or a control command; files under `src/scriptlab/`, with tests and a fuzz target because it is a parser).
2. A check that a downloadable build runs on a hosted CI runner with `--control`.
3. Creating the repository, with the template, a few examples and the workflow. Only after the owner says so.

## Not included

No repository is created. No code is changed. Notecard recipes, the stand-in for a closed third-party HUD product and the other ideas in the "Script Lab: later" list ([ROADMAP.md](ROADMAP.md)) are not part of this description.
