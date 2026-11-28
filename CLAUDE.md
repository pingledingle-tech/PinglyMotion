# Shared Rules for AI Agents Working on PinglyMotion

This repository (`pingledingle-tech/PinglyMotion`, public) is worked on by multiple AI agents (Claude, Antigravity, etc.). These rules apply equally to every agent so work never collides and status stays accurate.

Technical commands and build tricks live in `docs/DEV.md`. Design authority lives in `docs/DESIGN_CHARTER.md` and `docs/DESIGN_SYSTEM.md` (`docs/design_tokens.json`). The job-file format is in `docs/work/README.md`.

## Single source of truth

- **Running now:** open pull requests on GitHub (`gh pr list`) and their latest comments.
- **Work packages:** `docs/packages/WP-*.md` (scope, owner approval, and checklist) and `docs/packages/PARKED.md`.
- **1-page overview:** `docs/STATUS.md` (generated from `docs/packages/` and open PRs via `python3 tools/work.py render-status --write`).
- **Owner beta testing:** `docs/testing/next-beta.md` (one short checklist for the current beta cycle; archived after testing).
- **Done:** the report comment on each merged pull request and checked `[x]` items in `docs/packages/`.

## Before you start

1. **Pull latest `main` and check what is running:** run `gh pr list` and `python3 tools/work.py list`. Never start work already covered by an open PR or active job file.
2. **Respect approval:** only start tasks from a package marked `Owner-approved: yes` (or when the project owner explicitly asks you to start it in chat). Propose work outside approved packages in one line first.
3. **Create a branch, job file, and Draft PR right away:**
   - Work in your own clone or worktree (`tools/setup_clone.sh` sets the noreply author and pre-push privacy hook).
   - Create `docs/work/<branch with "/" replaced by "-">.md` (see `docs/work/README.md`) and run `python3 tools/work.py archive` in your first commit.
   - Push and open a **Draft PR** immediately (`gh pr create --draft`) so every other agent sees the work is claimed.

## While you work

- **Stay in your claimed files:** if you need to edit another file, add it to `Files:` in your job file first and run `python3 tools/work.py check` to catch overlaps with other open PRs.
- **Keep PRs small and focused:** one task or cohesive fix per branch.
- **Update the package and test list in the same PR:**
  - Tick `- [x]` for completed items in `docs/packages/WP-*.md` (and refresh `docs/STATUS.md` with `python3 tools/work.py render-status --write`).
  - If the PR changes visible behaviour that needs human eyes (3D view, camera, subjective feel), add 1–3 short test steps to `docs/testing/next-beta.md`. Ordinary buttons, menus, and approved layouts must be covered by automated tests instead.
- **Design & Charter:** visible PRs follow `docs/DESIGN_CHARTER.md` and `docs/DESIGN_SYSTEM.md`, include automated GUI tests, and list the followed Charter principles (e.g. `P1, P6`) in the PR description.
- **Web-ready & credits:** new/changed code (especially 3D view and file I/O) follows `docs/DEV.md` ("Web-ready code"). External references or datasets go in `CREDITS.md` in the same PR.
- **Fuzzing:** after changing a parser or control command, run the `fuzz.yml` workflow on your branch (`docs/DEV.md`) and wait for green before merging.

## Finishing and merging

1. Set `Status: done` in `docs/work/<branch>.md` in your final commit.
2. Update your branch from `main` (`gh pr update-branch` + `git pull` — never rebase/force-push), run tests and `python3 tools/work.py check`, then mark the PR ready (`gh pr ready`).
3. Wait for all required checks to pass (`linux`, `windows`, `asan-ubsan`, `privacy`, `analyze`, `dependency-review`; skipped counts as passed). The `work-board` check is advisory.
4. **Cross-review:** when asked or on larger changes, one agent may review another agent's PR for edge cases, Charter compliance, and test coverage before merging.
5. **Squash-merge:** technical jobs and approved GUI/UX changes with passing automated tests may merge once all required checks (and `fuzz.yml` if applicable) are green. Technical PRs (not changing `.github/workflows/` or parsers/control commands before fuzz) may use the `auto-merge` label.
6. **Clean up and report:** close/reference the linked issue (`Closes #N` or `Part of #N`), post one concise final report comment on the PR, and delete your local branch.

## Commits and privacy (strict)

- **Author:** `pingledingle-tech <351915610+pingledingle-tech@users.noreply.github.com>`.
- **No personal data anywhere:** the repository and its edit history are public. Never write the project owner's real name, any private person's name, email addresses (except GitHub noreply), personal paths (`C:\Users\<name>`, `/home/<name>`), or SL account names in commits, code, docs, issues, PRs, or screenshots. Write `the project owner` or `the user`.
- **No AI session trailers:** no `Co-Authored-By` or session lines in commits.
- **Check before every push:** run `python3 tools/check_privacy.py origin/main HEAD`.

## Never

- Never force-push `main` or rewrite another agent's commits.
- Never weaken, skip, or dismiss security checks or alerts by hand.
- Never change repository settings, branch rules, or required checks without the project owner's explicit request.
