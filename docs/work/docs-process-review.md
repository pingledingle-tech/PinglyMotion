Branch: docs/process-review
Status: done
Pull request: #351
Files:
- CLAUDE.md
- docs/DEV.md
- docs/DECISIONS.md
- docs/README.md
- docs/work/README.md
- .github/pull_request_template.md
- tools/work.py
- tests/test_work.py

## What

Review of the written process (roadmap step 1, "routines and processes"): one winning place per rule, a shorter CLAUDE.md, the new rule that every job posts its final report as a comment on its own pull request, `tools/work.py archive` as part of the merge routine, and the done job files archived now. `tools/work.py check` no longer asks a job to list job files that `archive` moved into `docs/work/archive/`. No workflow, check, script or required check changes behaviour. Edits to CLAUDE.md, docs/DEV.md, docs/DECISIONS.md and docs/work/README.md wait until pull requests 346 and 348 are merged or closed.
