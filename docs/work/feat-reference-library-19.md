Branch: feat/reference-library-19
Status: active
Pull request: #221
Files:
- tools/convert_reference.py
- tests/test_convert_reference.py
- tools/fetch_reference.py
- tools/reference_manifest.json
- tests/test_fetch_reference.py
- .github/workflows/reference-fetch.yml
- docs/REFERENCE_LIBRARY_SCHEMA.md
- docs/REFERENCE_LIBRARY_PLAN.md
- docs/CONTROL.md
- CREDITS.md

## What

Technical part of the reference library (issue 19), nothing on screen: `tools/convert_reference.py` converts the clips downloaded by `tools/fetch_reference.py` (CMU BVH, Mesh2Motion GLB, Quaternius glTF) to the Second Life skeleton and adds them to the downloaded index, so `load_reference` can open them. Adds the Quaternius Universal Animation Library (standard, CC0) as a pinned download source, documents the download folder layout and index entries, and credits Quaternius.
