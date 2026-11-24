# Rules for AI sessions working on PinglyMotion

Several Claude sessions may work on this repository (`pingledingle-tech/PinglyMotion`, public) at the same time. These rules keep them from breaking each other's work. This file is the one place for process rules (jobs, pull requests, testing, merging, privacy); other documents point here. Commands and tricks are in `docs/DEV.md`, the job file format in `docs/work/README.md`, and which document wins in `docs/README.md`.

GitHub is the truth, not a chat: what is running is the open pull requests and their latest comments, what is done is the report comment on each merged pull request, what comes next is the queue in the jobs list (issue 74), and what the owner must test is the owner test list (issue 47). Where each kind of note goes: "Between chats" below.

## Before you start

1. **Only one chat starts jobs.** The project owner works alone with several chats at the same time, and only the one he calls the job chat starts jobs: it creates branches, job files, pull requests and scheduled tasks. Every other chat only answers, discusses and reads. If the owner asks such a chat to start a job, it says which chat is the job chat, or asks the owner once. A job chat's helpers (subagents and background jobs) work inside that one job and never start further jobs of their own. When the owner asks to switch the job chat, the old chat starts nothing more, not even from the queue; the queue runs again once the new chat has been told "du er jobb-chatten". A chat that is not the job chat and makes a plan for a job writes it as a private project note `claude/PLAN_<topic>.md`, never on GitHub; the job chat looks for these notes when it starts and when it gives status, and when the owner says start it moves the plan into the jobs list (issue 74) or a job file and marks the note as started. A chat that only explores (not a job) writes its finding as a private project note `claude/FUNN_<topic>.md`, never on GitHub; it becomes public only when a job actually uses it.
2. **A plan is not a start, an approval is.** A job starts when the project owner says "start", names the job, or when it is inside work he has already approved (the approved queue, roadmap, phase plan or gap list: "The job queue" below). A plan, a pasted text or a request to check something is not a start, and work outside the approved scope is proposed first. Background jobs never ask the owner questions; if blocked, they stop and report (see "When you are done", step 7).
3. **Your own clone.** Every job works in its own clone (for example in `/tmp`). One session runs at most two jobs at a time, because the git proxy allows only two git operations at once. After cloning, run `tools/setup_clone.sh` (commit author and a pre-push privacy check). Do not start long builds when a small test target or the pull request checks are enough (`docs/DEV.md`).
   **Model per job** (the project owner, 24 November 2026): the cheaper model (Sonnet) is the default, also for ordinary GUI jobs built from the design system with exact tests, docs, CI and small fixes. The strong model (Opus) only for failures nobody understands, new structure (for example the Script Lab window, helper mode), 3D drawing and shaders, interpreters and parsers, and security. A job on the cheaper model that fails twice on the same thing stops and reports, and is rerun on the strong model. The job chat sets no recurring check reminders: jobs report back themselves.
4. **Read first.** Pull the newest `main`. Read `docs/DESIGN_CHARTER.md`: it is the top authority for how the program looks and behaves, and wins over every older UX document and mockup; a job with a visible change copies colours, shapes, icons and sizes from `docs/DESIGN_SYSTEM.md` (`docs/design_tokens.json`) and never invents its own. Then read what is running: `python3 tools/work.py list` (job files in your checkout and on open pull requests; needs `gh`), the open pull requests with their latest comments (`gh api repos/pingledingle-tech/PinglyMotion/pulls?state=open`) and the jobs list (issue 74).
5. **No double jobs.** If another active job or draft pull request already covers the same issue or files, do not start. Comment what you found on that pull request, report it to the user, and wait. (Two chats once started the same job this way, the easing tile row.)
6. **Job file and draft pull request first, within minutes.** Create your branch and the job file `docs/work/<branch with "/" replaced by "-">.md` (format and template: `docs/work/README.md`). In the same first commit, run `python3 tools/work.py archive`: it moves done job files whose pull request is merged to `docs/work/archive/`. Push, and open the pull request right away as a **draft** with `gh api -X POST repos/pingledingle-tech/PinglyMotion/pulls --input <file.json>` (`"draft": true`), then put its number in `Pull request:`. Take the job out of the queue in the jobs list (issue 74) as soon as its pull request is open: from then on the open pull request shows that it runs. A draft runs only the quick checks; the builds, tests and CodeQL start when it is marked ready.

## The job queue

- The owner approves a roadmap, phase plan, gap list or numbered queue once. The approval stays valid for all work inside that scope; such work needs no new approval. Work outside it is proposed in one line and the owner answers yes or no.
- The job chat records the approved queue in the jobs list (issue 74) with the owner's exact words and the time (Oslo time).
- When a job slot is free, the job chat starts the next approved job by itself and says so in one line. If the queue is empty, it takes the next natural, independent task from the approved roadmap, phase plan or Script Lab gap list, adds it to the queue and starts it. A slot is never left idle because a queue needs refilling; if a job is blocked, take another independent approved task. Never start a duplicate or anything an active pull request already covers. At most two jobs run at once.
- The owner can say "stopp køen" (stop the queue) or "fortsett køen" (resume it) at any time, in plain words.
- Honest limit: a chat acts only when the owner writes in it or a background job reports back to it. So the queue moves at those moments (a job finishing, the owner saying "status"), not on its own while the job chat is closed; never claim that a chat runs continuously.
- No job waits for the owner's test: all owner tests are optional (the project owner, 24 November 2026, 19:34: «Jeg vil ha alle tester på valgfri»). See step 5 of "When you are done".
- **Security alerts** (standing rule, the project owner, 23 November 2026, 16:41 and 16:46):
  - **Read them** at every status and every time a slot is free: code scanning (CodeQL), Dependabot and secret scanning, as far as they can be read. Say in one line which kinds could not be read. Routes that work today (commands in `docs/DEV.md`, "Security alerts"): the code-scanning bot's review comments on pull requests (author `github-advanced-security[bot]`), the open-alert list that the nightly CodeQL run writes to its log and summary, and Dependabot's own pull requests. The session token gets 403 on the alert lists themselves, and the proxy blocks the secret scanning list.
  - **Queue them:** each real alert becomes a fix job in this same queue (one job per finding or small group, ordinary job flow), placed before other filler jobs. There is no separate security queue and no pull request outside the job flow.
  - **Done only when proven:** the tests that show the fix pass, all required checks are green, and CodeQL on the fixed code no longer reports the alert. A fix that changes a parser or the control commands also needs the fuzz run green.
  - **Never by hand:** no alert is dismissed or marked fixed by hand. A false positive goes to the project owner with the reason, and is dismissed only after his answer.
  - **Never weaker:** never weaken, skip or remove a security check or test to close an alert.
  - **Report:** a new high or critical alert is reported to the project owner in one line when it is found.

## While you work

- Work only on your own branch. Never push to `main`.
- Stay inside the files you listed. If you need another file, add it to `Files:` first and run `python3 tools/work.py check` to see whether another active job claims it.
- Keep the job small: one feature or fix per branch.
- **Web-ready:** there will be a web version later (the project owner, 24 November 2026). New and changed code, above all the 3D view and file open or save, follows `docs/DEV.md`, "Web-ready code", so nothing has to be built twice.
- **Credits:** when a job uses or learns from a new outside project, dataset, paper or tool, add it to `CREDITS.md` in the same pull request.
- **Docs in the same pull request:** when your change makes a document wrong (`docs/PRODUCT.md`, `docs/ROADMAP.md`, `docs/DECISIONS.md`, release notes, the issue it belongs to), fix it in the same pull request and tick the docs box in the template. A job is not done while a document still says the old thing. A job changes `docs/ROADMAP.md` only when the order or the status word of a step changes, never to log what was built: the build record is the report comment on the pull request.
- **Fuzzing:** after changing a parser or the control commands, start the fuzz workflow on your branch (command in `docs/DEV.md`) and tell the user the result. It must be green before the merge.

## When you are done

1. In your job file set `Status: done`, in your last commit. Do not touch other jobs' files.
2. Bring your branch up to date with "Update branch": `gh api -X PUT repos/pingledingle-tech/PinglyMotion/pulls/<number>/update-branch`, then `git pull`. Never rebase: "Update branch" adds a merge commit on GitHub, so your own branch never needs a force-push. Build and run the tests; they must pass. Run `python3 tools/work.py check`. Update the branch once, before marking it ready; after that, update it again only if GitHub reports a merge conflict. The build on `main` after every merge catches the rare case where two green pull requests break together; if `main` goes red, the job chat starts a fix job at the front of the queue.
3. Mark the pull request ready: `gh pr ready <number>`, or where GraphQL is blocked `gh api -X POST repos/pingledingle-tech/PinglyMotion/pulls/<number>/ccr/ready_for_review`.
4. Wait until all checks are green (`gh api repos/pingledingle-tech/PinglyMotion/commits/<sha>/check-runs`). Skipped checks count as passed.
5. Merge with squash (`gh api -X PUT repos/pingledingle-tech/PinglyMotion/pulls/<number>/merge -f merge_method=squash`) when the kind of change allows it:
   - **Technical jobs** (nothing the user sees on screen changes: parsers, file handling, tests, CI, tools, docs, data that no menu shows yet): the project owner has given standing permission (18 November 2026). Merge as soon as all required checks are green, and the fuzz run too if the job changed a parser or the control commands.
     Instead of merging by hand, a technical job may add the label `auto-merge` to its ready pull request: the Auto-merge workflow then squash-merges it once all required checks are green (`docs/DEV.md`). Never put the label on a visible change, on a parser or control-command change before its fuzz run is green, or on a pull request that changes `.github/workflows/` (the workflow leaves those alone). Steps 6 and 7 are still the session's.
   - **Visible changes that implement approved design or behaviour:** the specification for GUI work is the approved mockup `docs/mockup/index.html` with `docs/DESIGN_CHARTER.md` and `docs/DESIGN_SYSTEM.md`. Ordinary GUI behaviour (buttons, menus, dialogs, keyboard use, saving and reopening, the right string, a computed number, state that round-trips such as window position and size) is built together with automated tests that assert the exact expected result. With those tests and all required checks green, such a change merges under the same standing permission as a technical job, without an owner test; the owner is not asked to test ordinary buttons or approved layouts. Without the tests the job is not done: add them, do not merge without them. Minor visual differences are collected for a later combined review, they do not block the merge.
   - **Visible changes needing taste or judgment:** only what automation cannot judge: does a movement look natural, does an interaction feel right, subjective animation quality, or any other 3D view, camera or rendering change a human has to look at. All owner tests are optional (the project owner, 24 November 2026, 19:34: «Jeg vil ha alle tester på valgfri»): such a change merges when all required checks are green, like the kinds above, and its test goes on the test page and in issue 47 as "Optional, blocks nothing" (see "Testing by the project owner"). When unsure which kind a visible change is, treat it as this kind: give it an optional test.
   - **Asking the owner:** only when requirements conflict, a design choice is genuinely new, an essential behaviour cannot be verified safely, or there is real risk of data loss or security problems. Background jobs never ask; if blocked they stop and report.
   - **Charter field:** every visible pull request (either visible kind) fills in "Charter principles followed" in its text: the principle ids from `docs/DESIGN_CHARTER.md` (for example `P1, P6, P8`) with one line each on how. A visible pull request without it is not merged (the `charter-field` job warns).
6. **With every merge, in the same step:**
   - Close the issue behind the pull request when it is fully done (`Closes #N` in the text). If only part is done, write `Part of #N` and add a short status comment on the issue.
   - Update the owner test list (issue 47): remove a test as soon as the project owner has said ok, and write "Nothing to test right now" when the list is empty.
   - The jobs list (issue 74) gets no "merged" summary: the job left the queue when its pull request opened, and the report comment (step 7) is the record. If the job is somehow still in the queue, take it out.
   - Run `python3 tools/work.py check-lists` (needs `gh`; it warns about queue entries whose pull request is already merged or closed) and fix what it reports in issue 74.
   - Clean up your checkout: `git switch main && git pull && git branch -D <branch>`. The squash commit on `main` is a different commit, so the session's stop check otherwise reports your old local commits as "unpushed" (a false alarm). Do not edit the stop hook.
7. **Report on the pull request, last.** Every job's last step posts its final report as a comment on its own pull request (on its issue if it has no pull request), also when it stops blocked: what was done, merged or not, the checks, what is left or blocked, any test that must go on the test page with its steps, and one line `Model: <model>` (and whether the job had to be rerun on the strong model). Then report to the chat in one line (number, what, checks green). A new job chat reads the open pull requests and their latest comments and issue 74, not handover notes.

A merged pull request whose job file still says `Status: active` is a mistake.

## Testing by the project owner

Only for visible changes needing taste or judgment (step 5 above), not for ordinary GUI behaviour or approved layouts that automated tests cover. Every owner test is optional and blocks nothing (the project owner, 24 November 2026, 19:34); the owner tests when he wants to, and what he finds becomes a follow-up.

1. **Test it yourself first.** Before a visible pull request goes to the owner, the session tests the feature itself with the methods in `docs/DEV.md` ("How a session tests a visible change itself") and writes in the pull request what it did and saw, and what it could not test.
2. **One build to test.** The owner tests ONE Windows build: since all tests are optional and taste changes merge on green, that is normally the newest build that holds every change on the test list, and the test page and issue 47 carry that one link. Branch `test/combined` (`main` plus open pull requests, never merged into `main`) is only used when the owner wants to try something before it merges (helper and commands: `docs/DEV.md`, "The combined test build"). When such a pull request is ready, its session adds it to `test/combined`, waits for the build and puts the test on the test page and in issue 47 with the combined build's Windows link. It is rebuilt after every change to that set of pull requests and pushed with `--force-with-lease`; it is the only branch that may be force-pushed, and the owner approved that for every session. Each time it is built, update the Windows link on the test page and in issue 47 in the same step.
3. **One test page.** There is ONE permanent test page (a Claude artifact); its address is in the private project notes, never in the repository. Sessions update that page for each test round and never create a new one. It has the Windows download link, one plain step at a time (clicks in the program, no command line), a tick box and a note field for each step, and an "Ask Claude" field. Test files are inside the Windows zip (the folder `examples/` next to the exe); a step says "File > Open, then pick the file in the examples folder next to the program", and a test never asks the owner to download a separate file.
4. **Where tests live.** Tests live only in the page's store (collection `active`), never in the page's code, and in the owner test list (issue 47); a job adds a test to both in the same step and is not done until both agree. A session without the artifact tools says so, relies on issue 47 only, and writes in its report (step 7) which test must go on the page, with its steps, so the job chat adds it.
5. **Read before asking.** Before asking the owner anything about a test, and at the start of every session that deals with tests or merging, read the ticks and notes in the page store and issue 47 with its latest comments. Never ask the owner to test or approve the same thing twice.
6. **Approval.** The owner approves by saying ok in chat, by ticking every step with no note that reports a problem, or by pressing "Godkjenn og arkiver" on the page (which moves the test from `active` to `archive` itself). Then, in the same step: copy the test's definition (title, pull request, steps, links) and results (ticks, notes) to the store's `archive` collection if the page has not, remove it from `active` and from `tests`, comment on issue 47 ("approved on the test page" or "approved in chat", with the date), remove it from that list, and merge the pull request if all required checks are green (with the merge steps above). When a session merges on the owner's word, that is also his approval of the test. Never delete approved results without archiving them.
7. **Problems.** A note that reports a problem is not an approval: copy the note (no personal details) as a comment on the pull request or its issue, keep the test and answer it in the next round. A step with a note and no tick is a problem (the page shows it red).
8. **Retest.** A test can be put back from the archive when the owner asks or a change could break it ("Test på nytt" on the page, or a session adds it to `active` with empty ticks); the archive entry stays.
9. **The owner's word wins.** What the owner says in chat beats any list: fix the list, do not ask him. Never invent tasks for him: issue 47 holds only tests he actually has to do, and checklists for him are made only when he asks.

## Checks and branches

`main` is protected by a ruleset: changes only through a pull request, squash merge only, and the checks `linux`, `windows`, `asan-ubsan`, `privacy`, `analyze` and `dependency-review` must be green (skipped counts as passed). GitHub's own auto-merge needs GraphQL, so merging is done as in step 5, by hand or with the `auto-merge` label. The branch is deleted automatically after the merge. What runs on a pull request, after a merge, at night and before a release is in `docs/DEV.md`, "What runs where".

The `work-board` check (`tools/work.py check`) is not a required check: when two active jobs claim the same files it shows a yellow warning (not red, the project owner, 24 November 2026). Look at the overlap; it does not block a merge. It is red only when the work board script's own tests fail.

## Between chats

This applies to every chat working on this project, also chats that only discuss. One place per kind, nothing written in two places:

| What | Where |
|---|---|
| What is running | Nowhere by hand: the open pull requests |
| What is done | The report comment on the merged pull request |
| Queue and "waiting for the owner" | Jobs list (issue 74) |
| Tests for the owner | Test page and owner test list (issue 47) |
| A plan for a job | Project note `claude/PLAN_<topic>.md`, with a one-line status: `PLAN`, `STARTED <date time>, pull request <n>` or `DONE` |
| A finding from exploring | Project note `claude/FUNN_<topic>.md` |
| A decision by the owner | Quoted with its time where it matters; lasting ones also go into `docs/DECISIONS.md` through a job |
| Which chat is which, lessons, pages | The project's private status note |

- **Chat tag:** every chat's answer starts with a short tag line: the chat's role and the clock time of the owner's message, for example `Jobb-chat · 15:22`. Role names are Norwegian (`Jobb-chat`, `Gjennomgang`, `UX-plan`, `Nettside`, `Uten rolle` for a chat without a role). Only one chat carries `Jobb-chat` at a time.
- **Times:** always Oslo time, with the date when it is not today. Never guess a time: use the time the owner's own message carries.
- The per-chat behaviour and the owner's standard sentences live in the private Claude project instructions, not in this repository; this section only states where things go.

## Commits

- Author: `pingledingle-tech <351915610+pingledingle-tech@users.noreply.github.com>`, set with `git config user.name` and `user.email` after cloning (`tools/setup_clone.sh` does it).
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
- **If you find personal data** already in the repository: do not quote it, tell the user where it is (issue or file number only) and ask before editing. Tell the user that the old text stays in the edit history until they delete it.
- The `privacy` check (`tools/check_privacy.py`) blocks email addresses other than GitHub noreply addresses, and a list of blocked words in commits and added lines. Its blocked words are stored only as SHA-256 hashes; never write the words themselves in the repository. It also checks the title and description of every pull request when it is opened or edited (the `privacy` check fails), and the text of new or edited issues, comments and reviews (the `Privacy text` workflow run fails; its log names the issue or comment, never the word). The check is a safety net, not a replacement for this rule: it runs after the text is posted, so the text is already visible and in the edit history; fix it at once. Run it before every push: `python3 tools/check_privacy.py origin/main HEAD`.

## Never

- Never force-push to `main` or any branch except `test/combined`, delete someone else's branch or rewrite another job's commits.
- Never merge a pull request unless all required checks are green and step 5 of "When you are done" allows it. Never merge another job's pull request unless the user asks you to.
- Never change the repository settings, rules or required checks. Ask the user.
