Branch: docs/stop-hook-false-alarm
Status: active
Pull request: pending
Files:
- CLAUDE.md

## What

After a squash merge the remote branch is deleted, so the session's stop check counts the local branch commits as unpushed and gives a false alarm. Add the step that avoids it: switch the checkout to main and delete the local branch after the merge.
