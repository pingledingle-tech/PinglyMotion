Branch: docs/testing-rule-mechanical-checks
Status: active
Pull request: pending
Files:
- CLAUDE.md
- docs/TESTING_RULE_AUDIT.md

## What

Sharpen the "who says yes" rule in `CLAUDE.md` so visible changes that are
mechanically checkable (exact text, round-tripping state) can merge under
the technical standing permission when the job adds an automated check for
them, while changes needing taste/judgment still need the project owner's
Windows test. Also add a short audit of recent merged "visible" jobs,
noting which would now count as mechanically checkable.
