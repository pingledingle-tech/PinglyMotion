# Shared Rules for AI Agents Working on PinglyMotion

This repository (`pingledingle-tech/PinglyMotion`, public) is worked on by multiple AI agents (Claude, Antigravity, etc.). These rules apply equally to every agent so work never collides and status stays accurate.

Technical commands and build tricks live in `docs/DEV.md`. Design authority lives in `docs/DESIGN_CHARTER.md` and `docs/DESIGN_SYSTEM.md` (`docs/design_tokens.json`). The job-file format is in `docs/work/README.md`.

## Single source of truth

- **Running now:** open pull requests on GitHub (`gh pr list`) and their latest comments.
- **Work packages (replaces issue #74):** `docs/packages/WP-*.md` (scope, owner approval, and checklist) and `docs/packages/PARKED.md`.
- **1-page overview:** `docs/STATUS.md` — generated from `docs/packages/` and open PRs via `python3 tools/work.py render-status --write`. Never edit `docs/STATUS.md` by hand.
- **Owner beta testing (replaces issue #47):** `docs/testing/next-beta.md` holds the short repo-side checklist for the current beta cycle (archived under `docs/testing/archive/` after testing). Sessions with access to the owner's private interactive test page keep it in sync with `docs/testing/next-beta.md` for ticking and notes.
- **Done:** the final report comment on each merged pull request and checked `[x]` items in `docs/packages/`.

## Coordinator, approval, and starting work

1. **One coordinator starts jobs:** Only the chat designated by the project owner as the coordinator (`Jobb-chat`) starts queued jobs by itself, unless the project owner explicitly asks another chat to take a specific task or package. Other chats explore, review, or prepare drafts/plans when asked.
2. **Approval:** Only start tasks from a package marked `Owner-approved: yes` (previously approved roadmap/queue scope stays valid), or when the project owner explicitly says to start it. Propose work outside approved scope in one line first.
3. **Model choice per job:** Use the standard/cheaper model (e.g. Sonnet) by default, including standard GUI work built from `docs/DESIGN_SYSTEM.md` with exact automated tests, docs, CI, and small fixes. Use the stronger model (e.g. Opus) for unexplained failures, new architecture, 3D drawing/shaders, parsers/interpreters/control commands, and security. If the standard model fails twice on the same issue, escalate to the stronger model.
4. **Read first:** Pull latest `main`, read `docs/DESIGN_CHARTER.md` and `docs/DESIGN_SYSTEM.md` for UI work, and check `gh pr list` and `python3 tools/work.py list`. Never duplicate work covered by an open PR or active job file.
5. **Job file and Draft PR first:**
   - Work in your own clone or worktree (`tools/setup_clone.sh` sets the noreply author and pre-push privacy hook).
   - Create `docs/work/<branch with "/" replaced by "-">.md` (see `docs/work/README.md`) and run `python3 tools/work.py archive` in your first commit.
   - Push and open a **Draft PR** immediately (`gh pr create --draft`) so every other agent sees the work is claimed.

## Security alerts (standing rule)

- **Read them** at status checks and before starting filler work: CodeQL (PR comments from `github-advanced-security[bot]` and nightly logs), Dependabot PRs, and secret scanning as far as accessible. Note any kind that could not be read.
- **Prioritize real alerts:** Queue real security fixes ahead of filler work using the normal PR flow.
- **Done only when proven:** Relevant tests pass, all required checks are green, CodeQL no longer reports the alert, and `fuzz.yml` is green if a parser or control command changed.
- **Never by hand & never weaker:** Never dismiss an alert by hand (report false positives to the project owner first), and never weaken or skip a security check to close an alert. Report new high/critical alerts in one line.

## While you work

- **Stay in your claimed files:** If you need another file, add it to `Files:` in your job file first and run `python3 tools/work.py check`.
- **Keep PRs small and focused:** One task or cohesive fix per branch.
- **Update package & beta checklist in the same PR:**
  - Tick `- [x]` for completed items in `docs/packages/WP-*.md` and refresh `docs/STATUS.md` (`python3 tools/work.py render-status --write`).
  - If the PR changes visible behaviour needing human eyes (3D view, camera, subjective feel), add 1–3 short test steps to `docs/testing/next-beta.md` (and sync the owner's private test page if available). Ordinary buttons, menus, and approved layouts must be covered by automated tests instead.
- **Design & Charter:** Visible PRs follow `docs/DESIGN_CHARTER.md` and `docs/DESIGN_SYSTEM.md`, include automated GUI tests, and list the followed Charter principles (e.g. `P1, P6`) in the PR description.
- **Web-ready & credits:** New/changed code (especially 3D view and file I/O) follows `docs/DEV.md` ("Web-ready code"). External references or datasets go in `CREDITS.md` in the same PR.
- **Fuzzing:** After changing a parser or control command, run `fuzz.yml` on your branch (`docs/DEV.md`) and wait for green before merging.

## Finishing and merging

1. Set `Status: done` in `docs/work/<branch>.md` in your final commit.
2. Update your branch from `main` (`gh pr update-branch` + `git pull` — never rebase/force-push), run tests and `python3 tools/work.py check`, then mark the PR ready (`gh pr ready`).
3. Wait for all required checks green (`linux`, `windows`, `asan-ubsan`, `privacy`, `analyze`, `dependency-review`; skipped counts as passed). `work-board` is advisory.
4. **Cross-review:** When asked or on larger changes, another agent may review the PR for edge cases, Charter compliance, and test coverage before merging.
5. **Squash-merge:** Technical jobs and approved GUI/UX changes with passing automated tests may merge once all required checks (and `fuzz.yml` if applicable) are green. Technical PRs (not touching `.github/workflows/` or parsers/control commands before fuzz) may use the `auto-merge` label.
6. **Final report comment (last step):** Every job posts a concise final report comment on its PR (or issue if no PR), even if blocked: what was done, merged or not, check results, any test steps for `next-beta.md`/the test page, and `Model: <model>`. Close/reference the linked issue (`Closes #N` / `Part of #N`) and delete your local branch.

## Commits and privacy (strict)

- **Author:** `pingledingle-tech <351915610+pingledingle-tech@users.noreply.github.com>`.
- **No personal data anywhere:** The repository and its edit history are public. Never write the project owner's real name, any private person's name, email addresses (except GitHub noreply), personal paths (`C:\Users\<name>`, `/home/<name>`), or SL account names in commits, code, docs, issues, PRs, or screenshots. Write `the project owner` or `the user`.
- **No AI session trailers:** No `Co-Authored-By` or session lines in commits.
- **Check before every push:** Run `python3 tools/check_privacy.py origin/main HEAD`.

## Never

- Never force-push `main` or rewrite another agent's commits.
- Never weaken, skip, or dismiss security checks or alerts by hand.
- Never change repository settings, branch rules, or required checks without the project owner's explicit request.
