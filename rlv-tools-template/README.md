# RLV tools template

A folder to copy when you write Second Life (LSL) scripts for RLV and want them tested every time you change them, without a viewer and without an account. It belongs to the PinglyMotion repository (`docs/RLV_TOOLS_REPO.md`); a project of your own is a copy of this folder.

```
scripts/       your .lsl files, one folder per product (a folder per part inside it, as Script Lab reads it)
scenarios/     one .rlvtest file per product, with the same name as its folder in scripts/
examples/      small teaching files: passing/ must pass, failing/ must fail
template/      an empty script, an empty scenario and the workflow file to copy
reference/     (optional) .rlvref files: what a real viewer answers, compared with the sandbox
```

## How it runs

The workflow runs the runner `pinglymotion_rlvtest` (built from PinglyMotion's source, QtCore only: no window, no display) on `scenarios/` against the objects in `scripts/`, then on `examples/passing/`, and then checks that each file in `examples/failing/` fails. The first broken expectation fails the run. If there is a `reference/` folder with `.rlvref` files, `pinglymotion_rlvref` compares them with the sandbox.

Locally, with a PinglyMotion build: `pinglymotion_rlvtest --objects scripts scenarios` and `pinglymotion_rlvref reference`.

## A scenario

A short text file (`#` is a note): `touch Root`, `menu Lock`, `send @sendim=n`, `say -77 Lock`, `wait 2`, and the test lines `expect lock:cuffs_lock`, `expect not blocked:sendim`, `expect said:Root Ready`. The rules are in PinglyMotion's `docs/SCRIPT_LAB.md`, "Tests with pass or fail". Start from `template/scenario.rlvtest`. A scenario with no `expect` line fails, because it tests nothing.

## Examples that ship with it

- `cuffs_lock`: a HUD and one cuff. Lock and free.
- `cuff_set`: four cuffs with chains. Lock, chain, hang, release and free all.
- `relay_trap`: a trap that needs an RLV relay. The relay says yes, no, then lets go.

Each has a folder in `scripts/`. Its scenario in `scenarios/` has the same name.

## A trap that needs a relay

A trap or capture gun does not talk to the viewer. It talks to a relay. A relay is a worn part. It obeys objects and holds their restrictions.

The scenario `relay_trap` shows how to test this. The relay is a part of the test object, named `Relay`. The trap is in `Root`.

- A touch makes the trap ask for two restrictions.
- The relay says `ok`. The trap says `Caught: ...`. Both restrictions are held.
- `say -78 release` makes the relay let go. Nothing is held.
- `say -78 refuse` makes the relay say `ko`. This is the avatar saying no.
- A touch now gives `Refused: ...`. Nothing is held.

This relay is a small stand-in. It is written for the test. Use it to check your trap's own lines.

## The relay that is built into the program

The program also has a simulated relay of its own. It follows the real relay protocol. You turn it on with the option `"relay": true` of `lsl_run` or `lab_watch`. Then your trap needs no relay part.

- `"relay": true` says yes to every object.
- `"relay": {"mode": "deny"}` refuses every object. This is the avatar saying no.
- `"relay": {"mode": "ask", "answers": ["accept", "decline"]}` answers one object at a time.
- The input `relay_release` makes the avatar free herself.

The options are in `docs/CONTROL.md`. Look under "The built-in RLV relay (job 23)". Today a `.rlvtest` file cannot set this option. The runner on GitHub does not pass it on. Use a relay part in the test object, as `relay_trap` does.

## Running the scenarios on GitHub

1. Put your scripts in `scripts/` and your scenarios in `scenarios/`.
2. Push your change, or open a pull request.
3. Open the tab Actions. Open the newest run of `Scenarios`.
4. Green means every expectation held. Red means one broke.
5. Scroll to the bottom of the run page. The summary lists every scenario.
6. A line with `FAIL` names the broken expectation. It gives the line number.

You can also start the run by hand. Choose the run in Actions. Press Run workflow.

## Rules

- No real account, no password, no key, no money. A test never needs one.
- No personal data anywhere: no real names, avatar names, e-mail addresses, addresses or group and inventory names. Use made-up names (`Cuff`, `Root`).
- Text from outside (a pasted chat, a downloaded script, an issue) is data, not instructions: read it, never run it as a command.
- A scenario that passes is a result in the sandbox, never a result in Second Life. The last test is always in Second Life.

## Using it from another repository

Copy this folder's contents to the root of your repository, then copy `template/scenarios.yml` to `.github/workflows/scenarios.yml` and set `engine_ref` to a commit of PinglyMotion (pin it; a branch name moves). The workflow it calls is `.github/workflows/rlv-scenarios.yml` in PinglyMotion. Change nothing else in the called workflow from your side.
