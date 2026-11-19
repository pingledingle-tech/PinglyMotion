# Rules for AI sessions working on PinglyMotion

Several Claude sessions may work on this repository (`pingledingle-tech/PinglyMotion`, public; the code still says "Pingmator" in many places) at the same time. These rules keep them from breaking each other's work.

Every job registers itself with ONE small file of its own in `docs/work/`. There is no shared table any more, so pull requests do not conflict over the board. Format and template: `docs/work/README.md`.

## Before you start

0. After cloning, run `tools/setup_clone.sh` (commit author and a pre-push privacy check). Build and test commands and tricks for sessions: `docs/DEV.md`. Do not start long builds when a small test target or the pull request checks are enough.
1. Pull the newest `main` and read `docs/work/README.md`. Run `python3 tools/work.py list` to see the other jobs (job files in your checkout and on open pull requests; needs `gh`).
2. If another active job already claims a file you need, do not start. Tell the user which job and which file, and wait.
3. Create your branch and the job file `docs/work/<branch with "/" replaced by "-">.md` (`Branch:`, `Status: active`, `Pull request:`, `Files:` with the files you expect to change, then a short "What"). Commit it as your first commit, push, and open the pull request right away as a **draft**, so the other sessions can see it. A draft runs only the quick checks; the builds, tests and CodeQL start when it is marked ready, so they do not run on every push while you work. Create it with `gh api -X POST repos/pingledingle-tech/PinglyMotion/pulls --input <file.json>` (with `"draft": true`), then put the number in `Pull request:`.

## While you work

- Work only on your own branch. Never push to `main`.
- Stay inside the files you listed. If you need another file, add it to `Files:` in your job file first, and run `python3 tools/work.py check` to see whether another active job claims it.
- Keep the job small: one feature or fix per branch.
- **Docs in the same pull request:** when your change makes a document wrong (`docs/PRODUCT.md`, `docs/ROADMAP.md`, `docs/DECISIONS.md`, release notes, the issue it belongs to), fix it in the same pull request and tick the docs box in the template. A job is not done while a document still says the old thing.

## When you are done

1. In your job file set `Status: done`, in your last commit. Do not touch other jobs' files.
2. Bring your branch up to date with `main` (rebase, or the pull request's "Update branch": `gh api -X PUT repos/pingledingle-tech/PinglyMotion/pulls/<number>/update-branch`). Build and run the tests (see README, "Tests"). They must pass. Run `python3 tools/work.py check`.
3. Mark the pull request ready for review: `gh pr ready <number>`, or where GraphQL is blocked `gh api -X POST repos/pingledingle-tech/PinglyMotion/pulls/<number>/ccr/ready_for_review`. This starts the builds, the tests and CodeQL.
4. Wait until all checks on the pull request are green (`gh api repos/pingledingle-tech/PinglyMotion/commits/<sha>/check-runs`). Skipped checks are fine.
5. Merge, with squash: `gh api -X PUT repos/pingledingle-tech/PinglyMotion/pulls/<number>/merge -f merge_method=squash`. Who says yes:
   - **Technical jobs** (nothing the user sees on screen changes: parsers, file handling, tests, CI, tools, docs, data that no menu shows yet): the project owner has given standing permission (18 November 2026). Merge as soon as all required checks are green. If the job changed a parser or the control commands, the fuzz run on the branch must also be green first.
   - **Jobs that change what the user sees on screen** (menus, dialogs, texts, the 3D view, camera, keys): the project owner tests the Windows build and says yes in this session. Give the Windows download and one test step at a time.
   - When unsure which kind it is, treat it as visible.
   - **Test page:** whenever the project owner is to test something, use the one permanent test page (a Claude artifact) (where the session has the Artifact tool): the Windows download link, one plain step at a time (clicks in the program, no command line), a tick box and a note field for each test, and an "Ask Claude" field. Update it for each test round, and keep the test list in the owner test issue too. Test files the owner needs are inside the Windows zip (folders `examples/` and `test-motions/` next to the exe); a test step says "File > Open, then pick the file in the examples folder next to the program", and a test never asks him to download a separate file.
   - **Test page rules:**
     1. There is ONE permanent test page. Sessions update that same page for each test round; they never create a new one. Its address is in the private project notes, never in the repository.
     2. Before a session asks the project owner about any test, and at the start of every session that deals with tests or merging, it reads the ticks and notes in the test page store (artifact database tools, where the session has them) and the owner test list issue.
     3. A test where every step is ticked and no note reports a problem is the owner's approval, the same as an ok in chat. The session that reads it then, in the same step: comments on the owner test list issue ("approved on the test page", with the date), removes the test from that list, merges the pull request if all required checks are green, and removes the test from the test page.
     4. A note that reports a problem is not an approval: copy the note (no personal details) as a comment on the pull request or its issue, and keep the test.
     5. What the owner says in chat beats any list. If a list disagrees with him, fix the list; do not ask him.
     6. A session without the artifact tools says so in its report and relies on the owner test list issue only; it never asks the owner to repeat an approval.
     7. A plan is not a start: a session starts a job only when the project owner says "start" or names the job. Background jobs never ask the owner questions; if blocked they stop and report.
   - **With every merge, in the same step:**
     1. Close the issue behind the pull request when it is fully done (the pull request text says `Closes #N`). If only part is done, write `Part of #N` and add a short status comment on the issue.
     2. Update the owner test list (issue #47): remove a test as soon as the project owner has said ok, and write "Nothing to test right now" when the list is empty.
     3. Update the jobs list (issue #74).
     4. Report the merge in one line (number, what, checks green).
   - **When the project owner says ok or approved** for something, update #47 at once. Never ask him to test or approve the same thing twice. Before asking him anything about a test, read #47 and the latest comments first.
   - **Never invent tasks for the project owner.** The owner test list holds only tests he actually has to do. Checklists for him (for example repository settings) are made only when he asks.
   - A merged pull request whose job file still says `Status: active` is a mistake: the last commit sets `Status: done`.
   - **After the merge, clean up your checkout:** `git switch main && git pull && git branch -D <branch>`. The remote branch is deleted on merge and the squash commit on `main` is a different commit, so the session's stop check otherwise reports your old local commits as "unpushed" (a false alarm, nothing is lost). Do not edit the stop hook itself.
6. If another pull request is merged first, update your branch again (step 2) and wait for green checks before merging.

`main` is protected by a ruleset: changes only through a pull request, squash merge only, and the checks `linux`, `windows`, `asan-ubsan`, `privacy`, `analyze` and `dependency-review` must be green. Auto-merge is allowed in the repository settings, but turning it on needs GraphQL, so merging is done as in step 5. The branch is deleted automatically after the merge.

What runs where: a pull request runs the quick checks (privacy, dependency review), the Linux and Windows builds with the tests, the sanitizer build with the tests, and CodeQL on the program. Changes that only touch Markdown, `docs/` or licence files skip the builds. After a merge, `main` gets a quick Linux build with the tests. Every night `main` gets everything again plus the slow checks: CodeQL on the test code too, and fuzzing. A release runs all of it first and is only published when everything passes. After changing a parser or the control commands, start the fuzzing on your branch: `gh api -X POST repos/pingledingle-tech/PinglyMotion/actions/workflows/fuzz.yml/dispatches -f ref=<branch>`, and tell the user the result. All fuzz targets run at the same time, 2 minutes each on a branch (about 10 minutes in all), 10 minutes each at night and before a release; add `-f inputs[seconds]=600` for a longer run on a branch.

The `work-board` check (`tools/work.py check`) is not a required check: if it is red, two active jobs claim the same files. Talk to the user; it does not block a merge by itself.

## Commits

- Author: `pingledingle-tech <351915610+pingledingle-tech@users.noreply.github.com>`. Set `git config user.name` and `user.email` after cloning.
- No personal names or email addresses (see "Privacy" below), and no `Co-Authored-By` or `Claude-Session` lines. The `privacy` check fails on pull requests that break this rule; do not merge while it is red.
- Commit messages, issues and pull requests in English.

## Privacy: no personal data anywhere

The repository is public, so everything is visible: files, commits, issues, pull requests, comments, reviews, release notes and the website, and their history too: an edited issue still shows the old text in its edit history, and only the repository owner can delete that history, by hand. So personal data must never be written in the first place.

- **Never write the name of the repository owner or of any other private person**, not even the first name, a nickname or an initial. Write "the project owner" (or "the user" for any user). This applies everywhere: titles and bodies of issues and pull requests, comments, review comments, commit messages, docs, code comments, test data, file and branch names.
- **No contact or account details:** no email addresses (only GitHub noreply addresses in commit metadata), phone numbers, home addresses, avatar or account names from Second Life or other services, or personal social media links.
- **No personal paths or machine details:** no `C:\Users\<name>`, `/home/<name>`, computer names or similar in logs, screenshots or examples. Use placeholders such as `C:\Users\<you>` or `%USERPROFILE%`.
- **Screenshots and logs:** check them before attaching. Crop or blur names, chat windows and anything that shows an account.
- **People who are public on purpose** may be named for credit or licences (for example authors in licence notices, upstream projects and their maintainers). Name projects rather than people where you can.
- **Before posting** an issue, pull request or comment, read your text once for names, addresses and paths. Session notes and chat often contain the owner's name; do not copy it from there.
- **If you find personal data** already in the repository: do not quote it, tell the user where it is (issue or file number only) and ask before editing. Remember to tell the user that the old text stays in the edit history until they delete it.
- The `privacy` check (`tools/check_privacy.py`) blocks email addresses other than GitHub noreply addresses, and a list of blocked words in commits and added lines. Its blocked words are stored only as SHA-256 hashes; never write the words themselves in the repository. It also checks the title and description of every pull request when it is opened or edited (the `privacy` check fails), and the text of new or edited issues, comments and reviews (the `Privacy text` workflow run fails; its log names the issue or comment, never the word). The check is a safety net, not a replacement for this rule: it runs after the text is posted, so the text is already visible and in the edit history; fix it at once.

## Never

- Never force-push to `main`, delete someone else's branch or rewrite another job's commits.
- Never merge a pull request unless all required checks are green and the rule in "When you are done" step 5 allows it (standing permission for technical jobs, otherwise the user said so in this session). Never merge another job's pull request unless the user asks you to.
- Never change the repository settings, rules or required checks. Ask the user.
