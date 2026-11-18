# Rules for AI sessions working on Pingmator

Several Claude sessions may work on this repo at the same time. These rules keep them from breaking each other's work.

## Before you start

1. Pull the newest `main` and read `docs/WORK.md`.
2. If another active job already touches a file you need, do not start. Tell the user which job and which file, and wait.
3. Add your job to the "Active jobs" table in `docs/WORK.md` (branch, what you are doing, which files you expect to change), commit it on your branch and open a draft pull request right away, so the other sessions can see it.

## While you work

- Work only on your own branch. Never push to `main`.
- Stay inside the files you listed. If you need another file, update your row in `docs/WORK.md` first and check it against the other jobs again.
- Keep the job small: one feature or fix per branch.

## When you are done

1. In `docs/WORK.md`, move your row from "Active jobs" to "Done", on your branch.
2. Bring your branch up to date with `main` (rebase, or the pull request's "Update branch": `gh api -X PUT repos/pingledingle-tech/Pingmator/pulls/<number>/update-branch`). Build and run the tests (see README, "Tests"). They must pass.
3. Mark the pull request ready for review. While it is a draft, only the privacy check runs; marking it ready starts the build, sanitizers and fuzzing (and the Windows zip).
4. Wait until all checks on the pull request are green (`gh api repos/pingledingle-tech/Pingmator/commits/<sha>/check-runs`). Skipped checks are fine.
5. Tell the user the pull request number and that it is ready to merge. The user merges it ("Squash and merge"), or tells you in this session that you may merge it. Only then merge it yourself, with squash: `gh api -X PUT repos/pingledingle-tech/Pingmator/pulls/<number>/merge -f merge_method=squash`.
6. If another pull request is merged first, update your branch again (step 2) and wait for green checks before merging.

The repository is private on a free account, so GitHub has no required checks and no auto-merge here. Nothing stops a merge with red checks: that is why step 4 matters.

The branch is deleted automatically after the merge.

## Commits

- Author: `pingledingle-tech <351915610+pingledingle-tech@users.noreply.github.com>`. Set `git config user.name` and `user.email` after cloning.
- No personal names or email addresses (see "Privacy" below), and no `Co-Authored-By` or `Claude-Session` lines. The `privacy` check fails on pull requests that break this rule; do not merge while it is red.
- Commit messages, issues and pull requests in English.

## Privacy: no personal data anywhere

The repository is private for now, but it may be made public again, and then everything becomes visible: files, commits, issues, pull requests, comments, reviews, release notes and the website, and their history too: an edited issue still shows the old text in its edit history, and only the repository owner can delete that history, by hand. So personal data must never be written in the first place.

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
- Never merge a pull request unless the user said so in this session and all checks are green. Never merge another job's pull request unless the user asks you to.
- Never change the repository settings, rules or required checks. Ask the user.
