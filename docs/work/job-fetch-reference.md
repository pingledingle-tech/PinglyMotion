Branch: job/fetch-reference
Status: active
Pull request: pending
Files:
- tools/fetch_reference.py
- tools/reference_manifest.json
- tests/test_fetch_reference.py
- .github/workflows/reference-fetch.yml
- docs/DEV.md
- docs/REFERENCE_LIBRARY_PLAN.md

## What

Job 2 of the reference library plan (issue 19): `tools/fetch_reference.py` downloads the full CMU BVH set and the Mesh2Motion clips from pinned official addresses, checks every file against pinned SHA-256 hashes, stores licence and credit next to the data and writes its own downloaded index. Standard library only, HTTPS only, safe paths, size limits, resumable.
