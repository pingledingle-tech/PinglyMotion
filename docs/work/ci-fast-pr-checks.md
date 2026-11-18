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

Pull requests run only the fast checks; the slow ones run every night on
`main` and by hand before a release. CodeQL on pull requests analyses the
program without the test programs (their traced build took most of the
15-21 minutes) and skips drafts. Fuzzing runs nightly only. `main` is not
built again after a merge; the nightly run checks it and keeps the caches
warm. The sanitizer job moves into `build.yml`. Required check names do not
change. `CLAUDE.md`: open pull requests as drafts again, so the heavy checks
do not run on every push while a job is in progress.
