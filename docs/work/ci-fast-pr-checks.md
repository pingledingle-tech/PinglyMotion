Branch: ci/fast-pr-checks
Status: active
Pull request: pending
Files:
- .github/workflows/build.yml
- .github/workflows/sanitizers.yml
- .github/workflows/codeql.yml
- .github/workflows/fuzz.yml
- .github/workflows/release.yml
- README.md
- fuzz/README.md
- CLAUDE.md

## What

Pull requests run the fast checks; the slow ones run every night on `main`
and before every release. CodeQL C++ stays a required check on pull requests,
but builds the program without the test programs, with `-j 4`, and skips
drafts; the nightly and release runs also analyse the test code. Fuzzing runs
nightly, before releases and by hand. After a merge, `main` gets only the
Linux build and tests. A release runs all checks and publishes only when they
pass. The sanitizer job moves into `build.yml`. Check names and the ruleset
do not change. `CLAUDE.md`: pull requests are opened as drafts again, so the
heavy checks do not run on every push while a job is in progress.
