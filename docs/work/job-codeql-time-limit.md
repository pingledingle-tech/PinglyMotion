Branch: job/codeql-time-limit
Status: done
Pull request: #499
Files:
- .github/workflows/codeql.yml
- .github/workflows/build.yml
- docs/DEV.md

## What

Raise the CodeQL C++ job time limit (45 minutes was too tight for nightly, manual and release runs that also build the test code) and the sanitizer build limit (27 minute green run against a limit of 40). No check is weakened or removed.
