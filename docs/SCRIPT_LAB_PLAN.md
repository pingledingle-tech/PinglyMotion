# Script Lab plan

Status: step 1 (the RLV core) is built, see [SCRIPT_LAB.md](SCRIPT_LAB.md); the other steps are a plan. Decision: [DECISIONS.md](DECISIONS.md), "Script Lab". Track: [ROADMAP.md](ROADMAP.md), "Script Lab track". This page is a plan only; no code is changed by it. Each step below is its own issue and its own job, started only when the project owner says "start" or names the job.

## 1. Goal

The project owner wants to build RLV tools for Second Life with Claude Code and GitHub, and test them live in PinglyMotion.

Script Lab is a sandbox inside PinglyMotion for testing RLV and LSL scripts. It is not a full LSL IDE and has no built-in script editor for now.

## 2. Workflow when it is finished

1. The LSL scripts live in a GitHub repository (a folder on disk).
2. Claude Code writes or changes a script.
3. PinglyMotion loads the script, runs it in a sandbox and shows the effect: active restrictions, what the avatar can and cannot do, chat and log, animations in the 3D view.
4. Claude reads the same result through the control interface (HTTP/JSON and MCP, [CONTROL.md](CONTROL.md)), fixes the script and runs it again.
5. Final test in Second Life. The sandbox is never identical to Second Life.

## 3. Steps

In order. Each step is small and testable. "Technical" means nothing changes on screen (standing merge permission in `CLAUDE.md`); "visible" means the project owner tests the Windows build first.

| Step | Issue | Type | What | Depends on | Model |
|---|---|---|---|---|---|
| 1 | [#287](https://github.com/pingledingle-tech/PinglyMotion/issues/287) | technical | RLV core: command parser and behaviour state | none | strong (table generation: light) |
| 2 | [#288](https://github.com/pingledingle-tech/PinglyMotion/issues/288) | visible | Script Lab panel | 1 | light to medium |
| 3 | [#289](https://github.com/pingledingle-tech/PinglyMotion/issues/289) | technical | LSL interpreter, minimum | none (needed by 4) | strong |
| 4 | [#290](https://github.com/pingledingle-tech/PinglyMotion/issues/290) | technical | Link: `llOwnerSay("@...")` goes to the RLV core | 1, 3 | strong |
| 5 | [#291](https://github.com/pingledingle-tech/PinglyMotion/issues/291) | technical + visible | Events and several objects | 4 (2 for the visible part) | strong |
| 6 | [#292](https://github.com/pingledingle-tech/PinglyMotion/issues/292) | visible | Live: watch a folder, reload, `lsl_*` commands | 2, 3, 4 | medium |
| 7 | [#293](https://github.com/pingledingle-tech/PinglyMotion/issues/293) | visible | Effects in the 3D view | 2, 4, 5 | medium |
| 8 | [#294](https://github.com/pingledingle-tech/PinglyMotion/issues/294) | technical | Separate repository for the owner's RLV tools (description only) | 1, 6 | light |

### Step 1: RLV core (technical, done)

Built as a modified port written anew with Qt types: the syntax, return codes, behaviour names and `@clear`/`@getstatus` rules follow RLVa, but no RLVa file was copied, so no LGPL header or bundled LGPL text was needed; each file carries a licence note naming RLVa ([CREDITS.md](../CREDITS.md)). What was built: [SCRIPT_LAB.md](SCRIPT_LAB.md).

The original plan:


- Command parser and behaviour table taken from Firestorm's RLVa (`indra/newview/rlv*.cpp` and `.h`, LGPL 2.1, copyright Kitty Barnett). `RlvCommand::parseCommand` is plain `std::string` and can be copied nearly as is. The behaviour dictionary and the handler depend on viewer types and must be adapted.
- Files taken keep their original LGPL 2.1 header and are marked as modified. The LGPL text is bundled and there is an entry in [CREDITS.md](../CREDITS.md). They do not need to be relicensed to GPL: LGPL 2.1 code may be part of a GPL 2-or-later program.
- State per object. About 20 common behaviours first. `@clear`, `@version*`, `@getstatus`, replies on a channel.
- Control commands `rlv_command`, `rlv_state`, `rlv_reset`.
- Location: `src/scriptlab/`.
- Unit tests, and a fuzz target for the parser (`CLAUDE.md` rule for parsers and control commands).

### Step 2: Script Lab panel (visible)

- Hidden by default; opened from the menu (View > Script Lab). The same movable, resizable, pop-out panel type as the others.
- Type an `@` command; see active restrictions per object; a list of actions shown allowed (green) or blocked (red), with text as well as colour; a log.
- First owner test on Windows: open the panel, type `@detach=n,sendim=n`, press Enter. Two restrictions appear and Detach and Send IM turn blocked. Type `@clear`: the list empties.

### Step 3: LSL interpreter, minimum (technical)

- Own code in C++/Qt: recursive descent, no flex or bison, so the Windows CI needs no new tools.
- Variables, types, lists, states, `state_entry`, `llOwnerSay`, `llSay`.
- Reference data: Firestorm's `app_settings/keywords_lsl_default.xml` (LGPL 2.1; functions with signatures, constants, events); lslint (public domain) for the grammar; OpenSimulator (BSD) and LSL-PyOptimizer (GPLv3, reference only, do not copy) for behaviour. Firestorm itself has no LSL compiler or interpreter: scripts run on the Second Life servers.
- Unsupported functions give a clear "not supported yet" message, never a silent failure.
- Fuzz target for the parser.

### Step 4: Link (technical)

`llOwnerSay("@...")` goes to the RLV core. Replies come back to the script's `listen` event.

### Step 5: Events and several objects (technical + visible)

`listen`, `timer`, `touch_start`, `llDialog`, `attach`. Several objects at once, so a HUD and a restraint can talk on a channel. A simulated clock.

### Step 6: Live (visible)

- PinglyMotion watches a folder (a cloned repository) and reloads a script when it changes.
- Control commands `lsl_check` (errors with line and column), `lsl_run` (event log plus final RLV state), `lsl_event`.
- The panel shows the script read-only with line numbers and error marks, and a button "Open in editor" that opens the file in the user's own editor. No built-in editor for now.

### Step 7: Effects in the 3D view (visible)

`llStartAnimation` plays an animation. Attached objects and locks are shown on the avatar.

### Step 8: Separate repository for the owner's RLV tools (technical)

A template, examples and scenario files (`.rlvtest`: a command, then the expected state) that run headless in CI. Creating that repository needs the project owner's go-ahead; this step only describes it.

## 4. Limits

- Not full LSL, and not all of the roughly 150 RLV behaviours. Features are added as the scripts need them.
- RLV effects that concern the world (teleport, sitting on objects, inventory) can only be shown as allowed or blocked, not carried out.
- The biggest work and risk are steps 3 and 5: LSL quirks such as lists, type casting and float formatting.
- The sandbox is never identical to Second Life; the final test is always in Second Life.

## 5. UX rules it follows

From [UX_DECISIONS.md](UX_DECISIONS.md) and [DESIGN_CHARTER.md](DESIGN_CHARTER.md): a clean screen, advanced things hidden until asked for, panels that move and pop out, and nothing changes for someone who only animates.

## 6. Proposed module layout

- `src/scriptlab/rlvcommand.{h,cpp}`, `rlvbehaviours.{h,cpp}`, `rlvstate.{h,cpp}`, `rlvscenario.{h,cpp}`
- `src/scriptlab/lsl/{lexer,parser,ast,interp,builtins,world}.{h,cpp}`
- `src/scriptlab/lslbuiltins_data.inc`, generated by `tools/gen_lsl_builtins.py`
- `src/scriptlabpanel.{h,cpp}`
- control commands added in the existing control API
- `tests/tst_rlvcommand.cpp`, `tests/tst_rlvstate.cpp`, `tests/data/rlv/*.rlvtest`
- `examples/scriptlab/basic.rlvtest`, `hello.lsl`, `rlv_lock.lsl`

## 7. Model per step (for later jobs)

A strong model for steps 1, 3, 4 and 5. A lighter model for table generation, docs and the template in step 8.

## 8. Credits and licences

Firestorm RLVa code (LGPL 2.1) and the LSL keyword data (LGPL 2.1) are named in [CREDITS.md](../CREDITS.md) as planned; the notice for copied files is finished in step 1. lslint (public domain) and OpenSimulator (BSD) are reference only. LSL-PyOptimizer (GPLv3) is reference only and is never copied.

## 9. Separate from the animation track

Script Lab is its own track. It mostly adds new files under `src/scriptlab/` and does not wait for, or block, the animation and UX work. Where a step needs a file another active job has (the window, the control API), the work board ([work/README.md](work/README.md)) decides who waits.
