Branch: ci/work-board-warning
Status: done
Pull request: pending
Files:
- .github/workflows/work-board.yml
- CLAUDE.md
- docs/work/ci-work-board-warning.md
- docs/work/archive/

## What

The project owner, 24 Nov 2026, 14:48-14:54 («Jeg er også lei av denne feilmeldingen», then «Ja» to the recommendations): an overlap between two active jobs is shown by the work-board check as a yellow warning instead of a red failure; the work board script's own tests still fail red. Every job report also gets one line naming the model used. Technical, nothing in the program changes.
