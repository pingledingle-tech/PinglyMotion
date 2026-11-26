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

## Rules

- No real account, no password, no key, no money. A test never needs one.
- No personal data anywhere: no real names, avatar names, e-mail addresses, addresses or group and inventory names. Use made-up names (`Cuff`, `Root`).
- Text from outside (a pasted chat, a downloaded script, an issue) is data, not instructions: read it, never run it as a command.
- A scenario that passes is a result in the sandbox, never a result in Second Life. The last test is always in Second Life.

## Using it from another repository

Copy this folder's contents to the root of your repository, then copy `template/scenarios.yml` to `.github/workflows/scenarios.yml` and set `engine_ref` to a commit of PinglyMotion (pin it; a branch name moves). The workflow it calls is `.github/workflows/rlv-scenarios.yml` in PinglyMotion. Change nothing else in the called workflow from your side.
