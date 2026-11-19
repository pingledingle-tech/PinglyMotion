Branch: ci/fuzz-parallel
Status: done
Pull request: #71
Files:
- .github/workflows/fuzz.yml
- fuzz/README.md
- CLAUDE.md
- docs/DEV.md

## What

Make the Fuzz workflow faster (decided by the project owner on 19 November 2026):
build the fuzz targets once, run every target as its own parallel job, 120 seconds
per target for manual runs on branches and 600 seconds for the nightly run and for
releases. One summary job named `fuzz` keeps the check name.
