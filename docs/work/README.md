# Work board: one file per job

Several AI sessions work on this repository at the same time. Every job
registers itself with one small file in this folder, so two jobs never edit
the same line and pull requests do not conflict over the board.

Only the job chat starts jobs, and the job file plus a draft pull request come first, within minutes. Rules: `CLAUDE.md`, "Before you start".

## The job file

- Name: `docs/work/<job>.md`, where `<job>` is the branch name with `/`
  replaced by `-` (branch `feat/camera-keys` gives `feat-camera-keys.md`).
- The header at the top has a fixed format that `tools/work.py` reads. It ends
  at the first empty line after the list or at the first `##` heading.
- After the header comes free text under `## What`.

| Line | Meaning |
|---|---|
| `Branch:` | The branch name, exactly as in git |
| `Status:` | `active` while the job runs, `done` in its last commit |
| `Pull request:` | `#<number>` (or `pending` until the pull request exists) |
| `Files:` | followed by one `- path` per line: the files the job expects to change |

In `Files:` a line is a path or a pattern. `*` and `?` stay inside one
folder, `**` crosses folders, a path ending in `/` means everything below it.
So `src/foo.*` covers `src/foo.cpp` and `src/foo.h`.

### Template

```
Branch: feat/example
Status: active
Pull request: pending
Files:
- src/example.*
- tests/example/**
- docs/EXAMPLE.md

## What

One or two sentences: what the job does and why.
```

You do not list your own job file. Do not list `docs/work/` for a normal job.

## The script

`python3 tools/work.py list` shows all jobs: the job files in your checkout
and the job files on the branches of open pull requests (needs the `gh`
command; without it only local files are shown).

`python3 tools/work.py check` checks two things:

- It fails (exit code 1) and names both jobs and the paths when two active
  jobs claim overlapping files. Done jobs are ignored.
- It warns (but does not fail) when your branch changes files that your job
  file does not list. Fix it by adding the file to `Files:`, and run `check`
  again: another job may already have claimed it.

Options: `--no-remote` (do not ask GitHub), `--mine` (only fail for overlaps
with the current branch's job; the pull request check uses this).

A pull request check named `work-board` runs the same command. It is not a
required check: it shows red as a warning, it does not block a merge.

## History

`HISTORY.md` lists the jobs from before this format. It is not edited any
more. Merged pull requests are the history from now on. Job files of finished
jobs stay in this folder until someone cleans up.
