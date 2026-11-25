Branch: job/ci-text-filter
Status: done
Pull request: #516
Files:
- tools/ci/changes.py
- tests/test_ci_changes.py
- docs/DEV.md
- docs/work/job-ci-text-filter.md

## What

The CI path filter treated every change under `docs/` as text-only and skipped the builds, although tests read files there (and `.github/workflows/pages.yml`). Files a test reads are no longer text-only: the ones read by Linux-job tests run the full builds, the ones read only by the website tests run the website job. A new test derives the read files from the tests and fails if one is still text-only.
