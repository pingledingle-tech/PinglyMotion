# Script Lab

Script Lab is a planned sandbox inside PinglyMotion for testing RLV and LSL scripts for Second Life. It is not a full
LSL IDE: there is no built-in script editor, and the scripts stay in the user's own folder or repository. The plan,
the steps and the limits are in [SCRIPT_LAB_PLAN.md](SCRIPT_LAB_PLAN.md); the track is in [ROADMAP.md](ROADMAP.md).

## What exists now (step 1, issue 287)

The RLV core, without a panel or a menu: nothing on screen changes yet. The panel comes in step 2 (issue 288).

- `src/scriptlab/rlvcommand.*`: the parser for owner-say lines such as `@detach=n,sendim=n`.
- `src/scriptlab/rlvbehaviours.*`: a table of 26 common behaviours (which parameters each takes, whether it takes an
  option, which actions it blocks) and the RLV version the sandbox answers with, 3.4.3.
- `src/scriptlab/rlvstate.*`: the restrictions each object holds, how many objects hold each one, `@clear`, and the
  answers to `@version*` and `@getstatus*` on a channel.
- Control commands `rlv_command`, `rlv_state` and `rlv_reset` over the local HTTP interface and the MCP server
  ([CONTROL.md](CONTROL.md), "Script Lab").
- Tests: `tests/tst_rlvcommand.cpp`, `tests/tst_rlvstate.cpp` and the RLV part of `tests/tst_control.cpp`; fuzz
  target `fuzz/fuzz_rlv.cpp`.

## How an AI assistant uses it

Claude (or any client) writes an RLV line or a script, sends the line with `rlv_command`, reads `rlv_state`, fixes the
line and sends it again. Example:

```
{"command": "rlv_command", "object": "collar", "text": "@detach=n,sendim=n"}
{"command": "rlv_state"}          -> blocked: ["detach", "sendim"]
{"command": "rlv_command", "object": "collar", "text": "@clear"}
```

## Limits

- Only the behaviours in the table; others parse but are reported as unknown.
- `force` commands are accepted (`retained`) but not carried out: there is no world to teleport in or sit on.
- No LSL yet (step 3). The sandbox is never identical to Second Life; the final test is always in Second Life.

## Licence

The command syntax, return codes, behaviour names and the `@clear` and `@getstatus` rules follow Firestorm's RLVa
(LGPL 2.1). The code is a modified port written anew with Qt types; no RLVa file is copied. See
[CREDITS.md](../CREDITS.md).
